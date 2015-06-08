//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//

using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Data.Odbc;
using System.Text;
using System.Text.RegularExpressions;
using Trafodion.Manager.Framework.Queries;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.DatabaseArea.Queries;

namespace Trafodion.Manager.DatabaseArea
{

    /// <summary>
    /// Class to provide data from a DB based on a SQL.
    /// The point list will be populated based on the PointConfig list provided
    /// </summary>
    public class DatabaseDataProvider : GenericDataProvider
    {
        const string _error08S01 = "08S01";
        protected SimpleReportDefinition reportDefinition = new SimpleReportDefinition("");
        protected OdbcCommand command = null;
        NCCQueryPlan theNCCQueryPlan = null;
        protected DataTable _theDataTable = null;
        protected ConnectionDefinition.ChangedHandler _theConnectionChangeHandler;
        protected List<ReportParameter> _theReportParameters = null;

        public static string ReportDefinitionKey   = "report_definition";
        public static string ElapsedTimeKey        = "elapsed_time";
        public static string ReportParametersKey   = "report_parameters";
        public static string SQLTextKey            = "sql_text";

        private bool _theRefreshMode = true;
        private bool _ReturnsTabularData = true;
        //public enum SessionMode { WMSMode, CMDMode, SQLMode };
        private DefaultConnectionProvider.SessionMode theCurrentSessionMode = DefaultConnectionProvider.SessionMode.SQLMode;
        private IConnectionProvider _theConnectionProvider = null;
        private bool _explainMode;
        NCCWorkbenchQueryData _theWbqd = null;
        MyConnectionDefinitionComparer _connectionDefinitionComparator = new MyConnectionDefinitionComparer();

        private const string TRACE_SUB_AREA_NAME = "DB Data Provider";

        public DatabaseDataProvider(DataProviderConfig aDataProviderConfig) : base(aDataProviderConfig)
        {
            _theConnectionProvider = new DefaultConnectionProvider();
            _theConnectionChangeHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
            ConnectionDefinition.Changed += this._theConnectionChangeHandler;
        }

        ~DatabaseDataProvider()
        {
            ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
        }

        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if ((this.DataProviderConfig != null) 
                && (this.DataProviderConfig.ConnectionDefinition != null)
                && (_connectionDefinitionComparator.GetHashCode(this.DataProviderConfig.ConnectionDefinition) ==
                _connectionDefinitionComparator.GetHashCode(aConnectionDefinition)))
            {
                if (aConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                {
                    // Store current state for future restoring
                    bool currentTimerState = this.DataProviderConfig.TimerPaused;

                    this.Stop();
                    this.StopTimer();

                    // Only when the reason is "Disconnected", restore the state above
                    if (aReason == ConnectionDefinition.Reason.Disconnected)
                    {
                        this.DataProviderConfig.TimerPaused = currentTimerState;
                    }
                }
            }
        }

        /// <summary>
        /// Indicates if the data returned by the execution is tabular
        /// </summary>
        public bool ReturnsTabularData
        {
            get { return _ReturnsTabularData; }
            set { _ReturnsTabularData = value; }
        }

        /// <summary>
        /// Refreshes the data with the existing parameters
        /// </summary>
        public override void RefreshData()
        {
            _theRefreshMode = true;
            base.RefreshData();
        }


        public IConnectionProvider TheConnectionProvider
        {
            get { return _theConnectionProvider; }
            set { _theConnectionProvider = value; }
        }

        /// <summary>
        /// Determines if explain results should be returned
        /// </summary>
        public bool ExplainMode
        {
            get { return _explainMode; }
            set { _explainMode = value; }
        }
        
        /// <summary>
        /// Returns the explain data model
        /// </summary>
        public NCCWorkbenchQueryData WorkbenchQueryData
        {
            get { return _theWbqd; }
            set { _theWbqd = value; }
        }

        /// <summary>
        /// Returns the custom event args for the DatabaseDataProvider
        /// </summary>
        /// <returns></returns>
        public override DataProviderEventArgs GetDataProviderEventArgs()
        {
            DataProviderEventArgs evtArgs = new DataProviderEventArgs();
            evtArgs.AddEventProperty(ReportDefinitionKey, reportDefinition);
            evtArgs.AddEventProperty(ElapsedTimeKey, TimeTakenToFetchData);
            evtArgs.AddEventProperty(ReportParametersKey, _theReportParameters);
            string query = reportDefinition.GetProperty(ReportDefinition.ACTUAL_QUERY) as String;
            evtArgs.AddEventProperty(SQLTextKey, query);
            return evtArgs;
        }

        /// <summary>
        /// Gets called when the user stops the data provider 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchCancel(Trafodion.Manager.Framework.WorkerThread worker)
        {
            //ConnectionDefinition.Changed -= this._theConnectionChangeHandler;

            if (_explainMode)
            {
                if (this.theNCCQueryPlan != null)
                {
                    this.theNCCQueryPlan.CancelLastExecutedQuery();
                }
            }
            else
            {
                if (this.command != null)
                {
                    this.command.Cancel();
                }
            }
        }

        /// <summary>
        /// Gets called when there is an error in the data provider 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchError(Trafodion.Manager.Framework.WorkerThread worker)
        {
            //ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
        }

        /// <summary>
        /// Check if the query needes any parameter and if so get it from the user
        /// </summary>
        /// <param name="predefinedParametersHash"></param>
        public override void DoPrefetchSetup(Hashtable predefinedParametersHash)
        {
            DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
            if (! IsConnectionDefinitionValid())
            {
                throw new FetchVetoException();
            }
            
            //If the connection is in a connected state assosiate the listener
            //ConnectionDefinition.Changed += this._theConnectionChangeHandler;

            //This is the complete query text as provided by the user
            reportDefinition.QueryText = dbConfig.SQLText;

            Trafodion.Manager.DatabaseArea.Queries.ReportParameterProcessor theReportParameterProcessor = Trafodion.Manager.DatabaseArea.Queries.ReportParameterProcessor.Instance;
            //in refresh mode, we will try to use the existing user inputs if they are available
            if ((_theRefreshMode) && (_theReportParameters != null))
            {
                //We have the params so just use it
            }
            else
            {

                //Check if the query needs params. If so populate it
                _theReportParameters = theReportParameterProcessor.getReportParams(reportDefinition);
                if (_theReportParameters.Count > 0)
                {
                    if (!theReportParameterProcessor.populateParamsFromUser(_theReportParameters, predefinedParametersHash))
                    {
                        _theReportParameters = null;
                        _theRefreshMode = false;
                        throw new FetchVetoException();
                    }
                }
            }
            reportDefinition.SetProperty(ReportDefinition.PARAMETERS, _theReportParameters);

            //Set the appropriate queries in the report definition
            string statement = theReportParameterProcessor.getQueryForExecution(reportDefinition.QueryText, _theReportParameters);
            reportDefinition.SetProperty(ReportDefinition.RAW_QUERY, reportDefinition.QueryText);
            reportDefinition.SetProperty(ReportDefinition.ACTUAL_QUERY, statement);

            //We are done it with refresh mode, hence re-setting it to default value
            _theRefreshMode = false;
        }

        private bool IsConnectionDefinitionValid()
        {
            if ((this.DataProviderConfig != null)
                && (this.DataProviderConfig.ConnectionDefinition != null))
            {
                return this.DataProviderConfig.ConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded;
            }
            return false;
        }


        public override void DoFetchProgress(Trafodion.Manager.Framework.WorkerThread worker, ProgressChangedEventArgs e)
        {
            //do nothing
        }


        /// <summary>
        /// This is where the actual fetch of the data will happen. This can be on a
        /// seperate thread if needed
        /// </summary>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            Connection con = null;
            long executionTime = 0;
            long fetchTime = 0;
            bool returnsResultset = true;
            int rowsAffected = 0;
            long t1 = DateTime.Now.Ticks;
            OdbcDataReader reader = null;
            try
            {
                string passedCatalog = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.CATALOG_NAME))) ? _theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.CATALOG_NAME] as string : null;
                string passedSchema = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.SCHEMA_NAME))) ? _theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.SCHEMA_NAME] as string : null;
                string sessionName = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.SESSION_NAME))) ? _theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.SESSION_NAME] as string : null;
                string reportName = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.REPORT_NAME))) ? _theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.REPORT_NAME] as string : null;
                int rowCount = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.ROW_COUNT))) ? (int)_theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.ROW_COUNT] : -1;

                t1 = DateTime.Now.Ticks;
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                //con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                con = _theConnectionProvider.GetConnection(this.DataProviderConfig.ConnectionDefinition);
                command = _theConnectionProvider.GetCommand(con, null);

                //Set the catalog and schema if needed
                setCatalogAndSchema(con, command);

                if (_explainMode)
                {
                    OdbcConnection theOdbcConnection = command.Connection;
                    string errorStr = "";
                    string tableStatsErrorStr = "";
                    string statement = reportDefinition.GetProperty(ReportDefinition.ACTUAL_QUERY) as String;
                    theNCCQueryPlan = new NCCQueryPlan(con);
                    NCCWorkbenchQueryData wbqd = theNCCQueryPlan.GetExplainPlan(statement, out errorStr, out tableStatsErrorStr, worker);
                    executionTime = DateTime.Now.Ticks - t1;
                    reportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_TIME, executionTime);
                    reportDefinition.SetProperty(ReportDefinition.LAST_EXECUTED_AT_TIME, DateTime.Now.ToLocalTime().ToString());

                    if (!String.IsNullOrEmpty(errorStr))
                    {
                        wbqd.Result = NCCWorkbenchQueryData.ExplainResult.Get_Explain_Error;
                    }
                    else if (!String.IsNullOrEmpty(tableStatsErrorStr))
                    {
                        wbqd.Result = NCCWorkbenchQueryData.ExplainResult.Get_TableStats_Error;
                    }
                    _theWbqd = wbqd;
                    //set the fetch time and rows affected to 0
                    reportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, rowsAffected);
                    reportDefinition.SetProperty(ReportDefinition.LAST_FETCH_TIME, fetchTime);
                }
                else
                {
                    //Get the data from the server
                    _theDataTable = new DataTable();

                    //Execute open command if needed
                    if (dbConfig.OpenCommand != null)
                    {
                        command.CommandText = dbConfig.OpenCommand;
                        ExecuteNonQuery(con, command);
                    }

                    //Set the command timeout if it has been set in the configuration
                    command.CommandTimeout = 0;
                    if (dbConfig.CommandTimeout >= 0)
                    {
                        command.CommandTimeout = dbConfig.CommandTimeout;
                    }

                    //Set the command text to execute
                    command.CommandText = reportDefinition.GetProperty(ReportDefinition.ACTUAL_QUERY) as String;

                    //Check if the command returns a resultset
                    returnsResultset = SqlModeHandler.ReturnsResultset(command.CommandText);


                    //Execute the statement
                    if (returnsResultset)
                    {
                        reader = ExecuteReader(con, command);
                    }
                    else
                    {
                        rowsAffected = this.ExecuteNonQuery(con, command);
                    }

                    //get the execution time
                    executionTime = DateTime.Now.Ticks - t1;
                    reportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_TIME, executionTime);
                    reportDefinition.SetProperty(ReportDefinition.LAST_EXECUTED_AT_TIME, DateTime.Now.ToLocalTime().ToString());

                    //try to 
                    t1 = DateTime.Now.Ticks;
                    _theDataTable = new DataTable();

                    //The assumption is that the data returned will be in tabular format. If not, we will set
                    //the flag appropriately
                    _ReturnsTabularData = true;

                    //If we expect the execution to return the resultset, we will try to get it
                    if (returnsResultset)
                    {
                        bool isSelect = SqlModeHandler.isSelect(command.CommandText);
                        //If we have a select statement, we populate the table with the data returned
                        if (isSelect)
                        {
                            // Populate the data table with data
                            populateDatatableWithData(_theDataTable, reader);
                            rowsAffected = _theDataTable.Rows.Count;
                        }
                        else
                        {
                            //We try to see if the data reader can be read
                            bool readReader = canReadReader(reader);
                            if (readReader)
                            {
                                // If the result has more than 1 columns, the result will be displayed in grid.
                                // If the result has only 1 column, the result in all rows will be appended into 
                                // a single string and displayed in the result text box.  The reason is that there
                                // are many commands (such as explain, showddl, maintain, etc) would result in only 
                                // one column with many rows.  Here we'll display all of the rows in separate lines.
                                if (reader.FieldCount > 1)
                                {
                                    // Populate the data table with data
                                    populateDatatableWithData(_theDataTable, reader);
                                    rowsAffected = _theDataTable.Rows.Count;
                                }
                                else
                                {
                                    StringBuilder stringBuilder = new StringBuilder();
                                    while (reader.Read())
                                    {
                                        stringBuilder.AppendLine(reader.GetString(0));
                                    }
                                    populateDatatableWithExecutionMessage(_theDataTable, stringBuilder.ToString());
                                    _ReturnsTabularData = false;
                                }
                            }
                            else
                            {
                                populateDatatableWithExecutionMessage(_theDataTable, "The statement executed successfully");
                                _ReturnsTabularData = false;
                                rowsAffected = 0;
                            }
                        }
                    }
                    else
                    {
                        //populate table with execution status
                        populateDatatableWithExecutionStatus(_theDataTable, rowsAffected);
                        _ReturnsTabularData = false;
                    }

                    fetchTime = DateTime.Now.Ticks - t1;
                    reportDefinition.SetProperty(ReportDefinition.ROWS_AFFECTED, rowsAffected);
                    reportDefinition.SetProperty(ReportDefinition.LAST_FETCH_TIME, fetchTime);
                }
            }
            finally
            {
                //ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                if ((reader != null) && (!reader.IsClosed))
                {
                    reader.Close();
                }
                CloseCommand(con, command);
                _theConnectionProvider.ReleaseConnection(con);
                //if (con != null)
                //{
                //    con.Close();
                //}
            }
        }

        /// <summary>
        /// Populate the column information in the data table
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="reader"></param>
        private bool canReadReader(OdbcDataReader reader)
        {
            bool readReader = false;
            try
            {
                //readReader = reader.HasRows;
                readReader = (reader.FieldCount > 0);
            }
            catch (OdbcException oe)
            {
                if (oe.Message.Contains("Invalid cursor state"))
                {
                    readReader = false;
                }
                else
                {
                    throw oe;
                }
            }
            return readReader;
        }

        private void populateDatatableWithData(DataTable aDataTable, OdbcDataReader reader)
        {
            // Add columns to the result data table
            int theFieldCount = reader.FieldCount;
            for (int colNum = 0; colNum < theFieldCount; colNum++)
            {
                try
                {
                    string colName = reader.GetName(colNum);

                    //_theDataTable.Columns.Add(GetExternalColumnName(colName), reader.GetFieldType(colNum));
                    aDataTable.Columns.Add(colName, reader.GetFieldType(colNum));
                }
                catch (Exception ex)
                {
                    aDataTable.Columns.Add(new DataColumn());
                }
            }

            int rowsRead = 0;
            //Populate it with data
            while (reader.Read())
            {
                rowsRead++;
                if ((DataProviderConfig.MaxRowCount > 0) && (rowsRead > DataProviderConfig.MaxRowCount))
                {
                    break;
                }
                //worker.ReportProgress();
                object[] theCurrRow = new object[reader.FieldCount];
                for (int currField = 0; currField < reader.FieldCount; currField++)
                {
                    try
                    {
                        theCurrRow[currField] = reader.GetValue(currField);
                    }
                    catch (Exception ex)
                    {
                        try
                        {
                            theCurrRow[currField] = reader.GetString(currField);
                        }
                        catch (Exception e1)
                        {
                            Console.WriteLine("data read failed : " + e1.Message);
                        }
                    }
                }

                //Add rows to the result data table
                aDataTable.Rows.Add(theCurrRow);
            }
        }

        private void populateDatatableWithExecutionStatus(DataTable aDataTable, int rowsAffected)
        {
            aDataTable.Columns.Add("Execution Status", typeof(string));
            object[] theCurrRow = new object[1];
            theCurrRow[0] = string.Format("Total rows affected {0}", rowsAffected);
            aDataTable.Rows.Add(theCurrRow);
        }


        private void populateDatatableWithExecutionMessage(DataTable aDataTable, string message)
        {
            aDataTable.Columns.Add("Execution Status", typeof(string));
            object[] theCurrRow = new object[1];
            theCurrRow[0] = (message != null) ? message.Trim() : "";
            aDataTable.Rows.Add(theCurrRow);
        }

        private void setCatalogAndSchema(Connection con, OdbcCommand command)
        {
            string passedCatalog = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.CATALOG_NAME))) ? _theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.CATALOG_NAME] as string : null;
            string passedSchema = ((_theDefaultParameters != null) && (_theDefaultParameters.ContainsKey(Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.SCHEMA_NAME))) ? _theDefaultParameters[Trafodion.Manager.Framework.Queries.ReportParameterProcessorBase.SCHEMA_NAME] as string : null;

            theCurrentSessionMode = _theConnectionProvider.GetSessionMode(con.TheConnectionDefinition);
            //check if the catalog and schema settings have changed since the last time, but do it only when the session is in SQL mode
            if (theCurrentSessionMode == DefaultConnectionProvider.SessionMode.SQLMode)
            {
                if ((passedCatalog != null) && (passedSchema != null))
                {
                    if ((this.DataProviderConfig.ConnectionDefinition.DefaultCatalog != null)
                        && (this.DataProviderConfig.ConnectionDefinition.DefaultSchema != null)
                        && this.DataProviderConfig.ConnectionDefinition.DefaultCatalog.Equals(passedCatalog)
                        && this.DataProviderConfig.ConnectionDefinition.DefaultSchema.Equals(passedSchema))
                    {
                        //If they are same, nothing to do.
                    }
                    else
                    {
                        //If catalog or schema has changed, issue a fresh set schema command.
                        string SettingCommandSchema = "SET SCHEMA " + passedCatalog + "." + passedSchema + ";";
                        command.CommandText = SettingCommandSchema;
                        
                        // test for open communication link
                        ExecuteNonQuery(con, command);

                        //Save the catalog and schema to the open connection, so we remember it for the next execution
                        //this.DataProviderConfig.ConnectionDefinition.DefaultCatalog = passedCatalog;
                        this.DataProviderConfig.ConnectionDefinition.DefaultSchema = passedSchema;
                    }
                }
            }
        }

        protected int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UW, TRACE_SUB_AREA_NAME, true);
        }
        private int ExecuteNonQuery(Connection con, OdbcCommand anOpenCommand)
        {
            int result = -1;
            try
            {
                result = Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UW, TRACE_SUB_AREA_NAME, true);
                SqlModeHandler.UpdateSessionMode(anOpenCommand.CommandText, con, _theConnectionProvider);
            }
            catch (Exception ex)
            {
                string errorMessage = ex.Message;
                if (errorMessage.Contains(_error08S01))
                {
                    // ERROR [08S01]: The communication link between the driver and the data source to which
                    // the driver was attempting to connect failed before the function completed processing.
                    _theConnectionProvider.ClearConnection(con);
                }

                //Re-throw the same exception
                throw ex;
            }
            return result;
        }
        protected OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UW, TRACE_SUB_AREA_NAME, true);
        }

        private OdbcDataReader ExecuteReader(Connection con, OdbcCommand anOpenCommand)
        {
            OdbcDataReader reader = null;
            try
            {
                reader = Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UW, TRACE_SUB_AREA_NAME, true);
                SqlModeHandler.UpdateSessionMode(anOpenCommand.CommandText, con, _theConnectionProvider);
            }
            catch (Exception ex)
            {
                string errorMessage = ex.Message;
                if (errorMessage.Contains(_error08S01))
                {
                    // ERROR [08S01]: The communication link between the driver and the data source to which
                    // the driver was attempting to connect failed before the function completed processing.
                    _theConnectionProvider.ClearConnection(con);
                }

                //Re-throw the same exception
                throw ex;
            }
            return reader;
        }

        /// <summary>
        /// Returns the connection for the connection definition
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        protected Connection GetConnection(ConnectionDefinition connectionDefinition)
        {
            Connection connection = null;
            connection = new Connection(connectionDefinition);
            return connection;
        }

        private void CloseCommand(Connection con, OdbcCommand aOpenCommand)
        {
            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;

                if (aOpenCommand != null)
                {
                    if (dbConfig.CloseCommand != null)
                    {
                        command.CommandText = dbConfig.CloseCommand;
                        ExecuteNonQuery(con, command);
                    }
                }
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }

        public void CloseCommand(OdbcCommand aOpenCommand)
        {
            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;

                if (aOpenCommand != null)
                {
                    if (dbConfig.CloseCommand != null)
                    {
                        command.CommandText = dbConfig.CloseCommand;
                        ExecuteNonQuery(command);
                    }
                }
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }


        /// <summary>
        /// Returns the data as a datatable after it has been fetched 
        /// </summary>
        /// <returns></returns>
        public override DataTable GetDataTable()
        {
            return _theDataTable;
        }
    }

    /// <summary>
    /// Helper class to deal with SQL patterns
    /// </summary>
    public class SqlModeHandler
    {
        const string _theCreateCatalogRegexp = @"^(\s)*(create)(\s)+(catalog)(\s)+";
        const string _theCreateSchemaRegexp = @"^(\s)*(create)(\s)+(schema)(\s)+";
        const string _theDropCatalogRegexp = @"^(\s)*(drop)(\s)+(catalog)(\s)+";
        const string _theDropSchemaRegexp = @"^(\s)*(drop)(\s)+(schema)(\s)+";

        //This would return a data set
        const string _selectRegexp = "^(\\s)*(select)(\\s)+";

        //This would return a plan
        const string _explainRegexp = "^(\\s)*(explain)(\\s)+";

        //These would return an int indicating the number of rows affected
        const string _insertRegexp = "^(\\s)*(insert)(\\s)+";
        const string _deleteRegexp = "^(\\s)*(delete)(\\s)+";
        const string _updateRegexp = "^(\\s)*(update)(\\s)+";

        //These are the special mode swich commands
        const string _theWMSOpenRegexp = "^(\\s)*(wmsopen)(\\s)*";
        const string _theWMSCloseRegexp = "^(\\s)*(wmsclose)(\\s)*";
        const string _theCMDOpenRegexp = "^(\\s)*(cmdopen)(\\s)*";
        const string _theCMDCloseRegexp = "^(\\s)*(cmdclose)(\\s)*";

        //Create the regular expression to evaluate
        public static Regex _selectReg = new Regex(_selectRegexp);
        public static Regex _explainReg = new Regex(_explainRegexp);
        public static Regex _insertReg = new Regex(_insertRegexp);
        public static Regex _deleteReg = new Regex(_deleteRegexp);
        public static Regex _updateReg = new Regex(_updateRegexp);

        static Regex _theCreateCatalogReg = new Regex(_theCreateCatalogRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        static Regex _theCreateSchemaReg = new Regex(_theCreateSchemaRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        static Regex _theDropCatalogReg = new Regex(_theDropCatalogRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        static Regex _theDropSchemaReg = new Regex(_theDropSchemaRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);

        public static Regex _theWMSOpenReg = new Regex(_theWMSOpenRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        public static Regex _theWMSCloseReg = new Regex(_theWMSCloseRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        public static Regex _theCMDOpenReg = new Regex(_theCMDOpenRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);
        public static Regex _theCMDCloseReg = new Regex(_theCMDCloseRegexp, RegexOptions.Compiled | RegexOptions.IgnoreCase);

        /// <summary>
        /// Update the session's mode
        /// </summary>
        /// <param name="aStatement"></param>
        /// <param name="aConnection"></param>
        public static void UpdateSessionMode(string aStatement, Connection aConnection, IConnectionProvider aConnectionProvider)
        {
            // Looking for mode switch:
            // Look for WMSOpen or CMDOpen first.
            if (_theWMSOpenReg.IsMatch(aStatement))
            {
                // Found the WMSOpen command, switch to WMS mode
                aConnectionProvider.SetSessionMode(aConnection.TheConnectionDefinition, DefaultConnectionProvider.SessionMode.WMSMode);
            }
            else if (_theCMDOpenReg.IsMatch(aStatement))
            {
                // Found the CMDOpen command, switch to CMD mode
                aConnectionProvider.SetSessionMode(aConnection.TheConnectionDefinition, DefaultConnectionProvider.SessionMode.CMDMode);
            }
            else if (_theCMDCloseReg.IsMatch(aStatement))
            {
                if (aConnectionProvider.GetSessionMode(aConnection.TheConnectionDefinition) == DefaultConnectionProvider.SessionMode.CMDMode)
                {
                    aConnectionProvider.SetSessionMode(aConnection.TheConnectionDefinition, DefaultConnectionProvider.SessionMode.SQLMode);
                }
            }
            else if (_theWMSCloseReg.IsMatch(aStatement))
            {
                if (aConnectionProvider.GetSessionMode(aConnection.TheConnectionDefinition) == DefaultConnectionProvider.SessionMode.WMSMode)
                {
                    aConnectionProvider.SetSessionMode(aConnection.TheConnectionDefinition, DefaultConnectionProvider.SessionMode.SQLMode);
                }
            }
        }

        /// <summary>
        /// Checks the SQL statement to see what kind of result is returned by the execution
        /// </summary>
        /// <param name="statement"></param>
        /// <returns></returns>
        public static bool ReturnsResultset(string statement)
        {
            bool ret = true;
            if ((_insertReg.IsMatch(statement.ToLower())) || (_deleteReg.IsMatch(statement.ToLower())) || (_updateReg.IsMatch(statement.ToLower())))
            {
                ret = false;
            }
            return ret;
        }

        /// <summary>
        /// Returns if a statement is a select statement
        /// </summary>
        /// <param name="statement"></param>
        /// <returns></returns>
        public static bool isSelect(string statement)
        {
            return _selectReg.IsMatch(statement.ToLower());
        }
    }

}
