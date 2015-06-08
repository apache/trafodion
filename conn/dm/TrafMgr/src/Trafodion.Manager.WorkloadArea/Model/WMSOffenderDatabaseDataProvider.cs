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
using System.ComponentModel;
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class WMSOffenderDatabaseDataProvider: DatabaseDataProvider
    {
        public enum FetchDataOption { Option_None, Option_SQLText, Option_QueryStats, Option_ChildrenProcesses, 
                                      Option_WmsDetail, Option_SQLPlan, Option_SessionStats, Option_Explain, Option_Warning, 
                                      Option_PState, Option_ProcessDetails, Option_CancelQuery, Option_QueryDetails, Option_PerTableStats };

        private string _theQueryText = "";
        private string _theQid = "";
        private string _theSTART_TS = "";
        private string _viewName = "";
        private string _processName = "";
        private FetchDataOption _option = FetchDataOption.Option_None;

        public WMSOffenderDatabaseDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        public string QueryID
        {
            get { return _theQid; }
            set { _theQid = value; }
        }

        public string START_TS
        {
            get { return _theSTART_TS; }
            set { _theSTART_TS = value; }
        }

        public string QueryText
        {
            get { return _theQueryText; }
            set { _theQueryText = value; }
        }

        public string ViewName 
        {
            get { return _viewName; }
            set { _viewName = value; }
        }

        public string ProcessName
        {
            get { return _processName; }
            set { _processName = value; }
        }

        public FetchDataOption FetchRepositoryDataOption
        {
            get { return _option; }
            set { _option = value; }
        }

        public DataTable DatabaseDataTable
        {
            get { return _theDataTable; }
            set { _theDataTable = value; }
        }

        private void DoFetchSQLTextData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            //Get the data from the server
            _theDataTable = new DataTable();
            Connection con = null;
            OdbcDataReader reader = null;
            bool result = false;
            int overflow = 0;
            bool wmsOpened = false;

            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                if (command != null)
                {
                    command.Connection = con.OpenOdbcConnection;
                }
                else
                {
                    command = new OdbcCommand();
                    command.Connection = con.OpenOdbcConnection;
                }

                command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
                try
                {
                    command.CommandText = "WMSOPEN";
                    command.ExecuteNonQuery();
                    wmsOpened = true;

                    string sql = "STATUS QUERY " + QueryID + " TEXT";
                    command.CommandText = sql;
                    try
                    {
                        reader = ExecuteReader(command);
                        _theDataTable = Utilities.GetDataTableForReader(reader);
                        if (_theDataTable != null)
                        {
                            foreach (DataRow r in _theDataTable.Rows)
                            {
                                object[] cols = r.ItemArray;
                                _theQueryText = (string)cols[0];
                                _viewName = "WMS";
                                result = true;
                                break;
                            }
                        }
                    }
                    catch (OdbcException ex)
                    { }
                    finally
                    {
                        if ((reader != null) && (!reader.IsClosed))
                        {
                            reader.Close();
                        }
                    }
                }
                finally
                {

                    if (wmsOpened)
                    {
                        command.CommandText = "WMSCLOSE";
                        command.ExecuteNonQuery();
                    }
                }

                if (!result)
                {
                    // If no data is returned from WMS, try to get it from the Repository.
                    DoFetchTrafodionQueryTextFromRepository();
                }
            }
            finally
            {
                ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                CloseCommand(command);
                if (con != null)
                {
                    con.Close();
                }
            }
        }

        private bool DoFetchTrafodionQueryTextFromRepository()
        {
            bool result = false;
            OdbcDataReader reader = null;

            command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_NO_TIMEOUT;
            if (!string.IsNullOrEmpty(START_TS))
            {

                string sqlText = "SELECT SQL_TEXT, FRAGMENT_NUM, EXEC_START_LCT_TS FROM MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_QUERY_SQLTEXT_1" +
                     " WHERE EXEC_START_LCT_TS = TIMESTAMP '{0}' AND QUERY_ID = '{1}'" +
                     " ORDER BY EXEC_START_LCT_TS, FRAGMENT_NUM ASC FOR READ UNCOMMITTED ACCESS";
                command.CommandText = String.Format(sqlText, START_TS, QueryID);
            }
            else
            {
                string sqlText = "SELECT SQL_TEXT, FRAGMENT_NUM, EXEC_START_LCT_TS FROM MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_QUERY_SQLTEXT_1" +
                     " WHERE QUERY_ID = '{0}'" +
                     " ORDER BY EXEC_START_LCT_TS, FRAGMENT_NUM ASC FOR READ UNCOMMITTED ACCESS";
                command.CommandText = String.Format(sqlText, QueryID);
            }
            reader = ExecuteReader(command);

            _theDataTable = Utilities.GetDataTableForReader(reader);

            if (_theDataTable != null && _theDataTable.Rows.Count > 0)
            {
                // Only take the first entry
                _theQueryText = "";
                //DateTime start_ts = (DateTime)_theDataTable.Rows[0]["EXEC_START_LCT_TS"];
                foreach (DataRow r in _theDataTable.Rows)
                {
                    // if (start_ts.Equals((DateTime)r["EXEC_START_LCT_TS"]))
                    {
                        _theQueryText += r["SQL_TEXT"].ToString();
                    }
                }
                _viewName = "MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_QUERY_SQLTEXT_1";
                result = true;
            }
            else
            {
                result = false;
            }

            return result;
        }

        private void DoFetchQueryStatsData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            //Get the data from the server
            _theDataTable = new DataTable();
            Connection con = null;
            OdbcDataReader reader = null;

            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                if (command != null)
                {
                    command.Connection = con.OpenOdbcConnection;
                }
                else
                {
                    command = new OdbcCommand();
                    command.Connection = con.OpenOdbcConnection;
                }

                command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_NO_TIMEOUT;
                if (!string.IsNullOrEmpty(_theSTART_TS))
                {

                    command.CommandText = String.Format("SELECT * FROM {0} WHERE EXEC_START_LCT_TS = TIMESTAMP '{1}' AND QUERY_ID = '{2}' FOR READ UNCOMMITTED ACCESS",
                                                        this.DataProviderConfig.ConnectionDefinition.MetricQueryViewFull,
                                                        START_TS, QueryID );
                }
                else
                {
                    command.CommandText = String.Format("SELECT * FROM {0} WHERE QUERY_ID = '{1}' FOR READ UNCOMMITTED ACCESS",
                                                        this.DataProviderConfig.ConnectionDefinition.MetricQueryViewFull,
                                                        QueryID);
                }

                try
                {
                    reader = ExecuteReader(command);

                    _theDataTable = Utilities.GetDataTableForReader(reader);
                }
                finally
                {
                    if ((reader != null) && (!reader.IsClosed))
                    {
                        reader.Close();
                    }
                }
            }
            finally
            {
                ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                CloseCommand(command);
                if (con != null)
                {
                    con.Close();
                }
            }
        }

        private void DoFetchWMSData(string sql)
        {
            //Get the data from the server
            _theDataTable = new DataTable();
            Connection con = null;
            OdbcDataReader reader = null;
            bool wmsOpened = false;

            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                if (command != null)
                {
                    command.Connection = con.OpenOdbcConnection;
                }
                else
                {
                    command = new OdbcCommand();
                    command.Connection = con.OpenOdbcConnection;
                }

                command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
                command.CommandText = "WMSOPEN";
                command.ExecuteNonQuery();
                wmsOpened = true;

                command.CommandText = sql;

                try
                {
                    reader = ExecuteReader(command);
                    _theDataTable = Utilities.GetDataTableForReader(reader);
                }
                finally
                {
                    if ((reader != null) && (!reader.IsClosed))
                    {
                        reader.Close();
                    }
                }
            }
            finally
            {
                if (wmsOpened)
                {
                    command.CommandText = "WMSCLOSE";
                    command.ExecuteNonQuery();
                }

                ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                CloseCommand(command);
                if (con != null)
                {
                    con.Close();
                }
            }
        }

        private void DoFetchQueryDetails(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql = string.Format("STATUS QUERY {0} MERGED", QueryID);
            DoFetchWMSData(sql);
        }

        private void DoFetchChildrenProcessesData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql= "STATUS PROCESS " + ProcessName + " CHILDREN";
            DoFetchWMSData(sql);
        }

        private void DoFetchWmsDetailData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql = "STATUS WMS DETAIL";
            DoFetchWMSData(sql);
        }

        private void DoFetchSQLPlanData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            try
            {
	            string sql = "STATUS QUERY " + QueryID + " PLAN";
	            DoFetchWMSData(sql);
            }
            catch (OdbcException ex)
            {}

            //throw new Exception("Testing ...");
        }

        private void DoFetchSessionStats(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e) 
        {
            //Get the data from the server
            _theDataTable = new DataTable();
            Connection con = null;
            OdbcDataReader reader = null;

            try 
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                if (command != null)
                {
                    command.Connection = con.OpenOdbcConnection;
                }
                else
                {
                    command = new OdbcCommand();
                    command.Connection = con.OpenOdbcConnection;
                }

                command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_NO_TIMEOUT;
                
                //Originally, it should be SESSION_ID='{1}', but in parameter 1 the single quotes are already contained
                string sqlText = "SELECT * FROM {0} WHERE SESSION_ID = {1} FOR READ UNCOMMITTED ACCESS";

                string SessionID = GenSessionIDFromQueryID(QueryID);

                command.CommandText = String.Format(sqlText,
                                                    this.DataProviderConfig.ConnectionDefinition.SessionStatsView,
                                                    SessionID);

                try
                {
                    reader = ExecuteReader(command);
                    _theDataTable = Utilities.GetDataTableForReader(reader);
                }
                finally
                {
                    if ((reader != null) && (!reader.IsClosed))
                    {
                        reader.Close();
                    }
                }
            }
            finally
            {
                ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                CloseCommand(command);
                if (con != null)
                {
                    con.Close();
                }
            }

            //throw new Exception("Testing ...");

        }

        private void DoFetchExplainPlan(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            //Get the data from the server
            _theDataTable = new DataTable();
            Connection con = null;

            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                if (command != null)
                {
                    command.Connection = con.OpenOdbcConnection;
                }
                else
                {
                    command = new OdbcCommand();
                    command.Connection = con.OpenOdbcConnection;
                }

                command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_NO_TIMEOUT;

                if (DoFetchTrafodionQueryTextFromRepository())
                {
                    reportDefinition.QueryText = _theQueryText;
                    reportDefinition.SetProperty(ReportDefinition.RAW_QUERY, _theQueryText);
                    reportDefinition.SetProperty(ReportDefinition.ACTUAL_QUERY, _theQueryText);

                    base.ExplainMode = true;
                    base.DoFetchData(worker, e);
                }
            }
            finally
            {
                ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                CloseCommand(command);
                if (con != null)
                {
                    con.Close();
                }
            }

            //throw new Exception("Testing ...");
        }

        private void DoFetchWarning(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql = string.Format("STATUS QUERY {0} WARN", QueryID);
            DoFetchWMSData(sql);
        }

        /// <summary>
        /// Fetch Pstate
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        private void DoFetchPState(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql = string.Format("STATUS PROCESS {0} PSTATE", ProcessName);
            DoFetchWMSData(sql);
        }

        /// <summary>
        /// Fetch Pstate
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        private void DoFetchProcessDetails(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql = string.Format("STATUS PROCESS {0}", ProcessName);
            DoFetchWMSData(sql);

        }


        private void DoCancelQuery(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            Connection con = null;
            bool wmsOpened = false;

            try
            {
                DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
                con = GetConnection(this.DataProviderConfig.ConnectionDefinition);
                if (command != null)
                {
                    command.Connection = con.OpenOdbcConnection;
                }
                else
                {
                    command = new OdbcCommand();
                    command.Connection = con.OpenOdbcConnection;
                }

                command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
                command.CommandText = "WMSOPEN";
                command.ExecuteNonQuery();
                wmsOpened = true;

                wmsOpened = true;
                command.CommandText = "CANCEL QUERY " + QueryID;
                command.ExecuteNonQuery();
            }
            finally
            {
                if (wmsOpened)
                {
                    command.CommandText = "WMSCLOSE";
                    command.ExecuteNonQuery();
                }

                ConnectionDefinition.Changed -= this._theConnectionChangeHandler;
                CloseCommand(command);
                if (con != null)
                {
                    con.Close();
                }
            }
        }

        /// <summary>
        /// Fetch Per Table Stats
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        private void DoFetchTableStats(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            string sql = string.Format("STATUS QUERY {0} STATS PERTABLE", QueryID);
            DoFetchWMSData(sql);
        }

        //Generate SessionID from QueryID
        private string GenSessionIDFromQueryID(string queryID) 
        {
            string sessionID = string.Format("(values(QUERYID_EXTRACT('{0}', 'SESSIONID')))", queryID);

            return sessionID;
        }

        /// <summary>
        /// This is where the actual fetch of the data will happen. This can be on a
        /// seperate thread if needed
        /// </summary>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e)
        {
            if (_option == FetchDataOption.Option_QueryDetails)
            {
                DoFetchQueryDetails(worker, e);
            }
            else if (_option == FetchDataOption.Option_SQLText)
            {
                DoFetchSQLTextData(worker, e);
            }
            else if (_option == FetchDataOption.Option_QueryStats)
            {
                DoFetchQueryStatsData(worker, e);
            }
            else if (_option == FetchDataOption.Option_ChildrenProcesses)
            {
                DoFetchChildrenProcessesData(worker, e);
            }
            else if (_option == FetchDataOption.Option_WmsDetail)
            {
                DoFetchWmsDetailData(worker, e);
            }
            else if (_option == FetchDataOption.Option_SQLPlan)
            {
                DoFetchSQLPlanData(worker, e);
            }
            else if(_option== FetchDataOption.Option_SessionStats)
            {
                DoFetchSessionStats(worker, e);
            }
            else if (_option == FetchDataOption.Option_Explain)
            {
                DoFetchExplainPlan(worker, e);
            }
            else if (_option == FetchDataOption.Option_Warning)
            {
                DoFetchWarning(worker, e);
            }
            else if (_option == FetchDataOption.Option_PState)
            {
                DoFetchPState(worker, e);
            }
            else if (_option == FetchDataOption.Option_ProcessDetails)
            {
                DoFetchProcessDetails(worker, e);
            }
            else if (_option == FetchDataOption.Option_CancelQuery)
            {
                DoCancelQuery(worker, e);
            }
            else if (_option == FetchDataOption.Option_PerTableStats)
            {
                DoFetchTableStats(worker, e);
            }
        }        
    }
}
