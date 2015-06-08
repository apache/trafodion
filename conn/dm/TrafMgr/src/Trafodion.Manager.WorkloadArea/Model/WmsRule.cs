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
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class WmsRule : WmsObject
    {
        #region Private member variables

        private WmsSystem _wmsSystem;
        private string _name;
        private string _ruleType;
        private string _operator;
        private string _expression;
        private string _warnLevel;
        private string _actionType;
        private string _actionCommand = "";
        private string _aggregate = "";
        private string _aggrQueryTypes = "";
        private Int16 _aggrReposInterval = 0;
        private Int16 _aggrWmsInterval = 0;
        private Int16 _aggrExecInterval = 0;
        private string _statsCollectOnce = "OFF";
        private string _comment;
        private List<string> _serviceNames = null;

        #endregion Private Fields

        #region Public properties

        public List<string> AssociatedServiceNames
        {
            get
            {
                if (_serviceNames == null)
                    LoadAssociatedServiceNames();

                return _serviceNames;
            }
        }

        public List<WmsService> AssociatedServices
        {
            get
            {
                List<WmsService> associatedServices = new List<WmsService>();
                foreach (string serviceName in AssociatedServiceNames)
                {
                    WmsService wmsService = _wmsSystem.FindService(serviceName);
                    if (wmsService != null)
                    {
                        associatedServices.Add(wmsService);
                    }
                }
                return associatedServices;
            }
        }

        /// <summary>
        /// The Create wms command to create this wms service
        /// </summary>
        public override string CreateCommandString
        {
            get
            {
                //add rule conn r28 (appl EXCEL2, session ABC2, dsn ANAN_WMS2, SQL_CMD "SET SCHEMA MANAGEABILITY.ANAN_SCHEMA;");;
                //ADD RULE COMP rc4 (OR, EST_USED_ROWS > 5000, EST_TOTAL_TIME > 1000, WARN-HIGH, REJECT);;
                return String.Format("ADD RULE {0} {1} ({2}{3}{4}{5}{6}){7};",
                    RuleType, FormattedName,
                    RuleType.Equals(WmsCommand.CONN_RULE_TYPE) ? Expression : Operator + ", " + Expression,
                    String.IsNullOrEmpty(WarnLevel) ? "" : ", " + WarnLevel,
                    String.IsNullOrEmpty(ActionType) ? "" : ", " + ActionType,
                    String.IsNullOrEmpty(FormattedActionCommandString) ? "" : " " + FormattedActionCommandString,
                    String.IsNullOrEmpty(Aggregate) ? "" : ", " + Aggregate,
                    String.IsNullOrEmpty(Comment) ? "" : " COMMENT \"" + Comment + "\""
                    );
            }
        }

        /// <summary>
        /// The Alter wms command to alter this wms service
        /// </summary>
        public override string AlterCommandString
        {
            get
            {
                return String.Format("ALTER RULE {0} {1} ({2}{3}{4}{5}{6}){7};",
                    RuleType, FormattedName, 
                    RuleType.Equals(WmsCommand.CONN_RULE_TYPE) ? Expression : Operator + ", " + Expression,
                    String.IsNullOrEmpty(WarnLevel) ? "" : ", " + WarnLevel,
                    String.IsNullOrEmpty(ActionType) ? "" : ", " + ActionType,
                    String.IsNullOrEmpty(FormattedActionCommandString) ? "" : " " + FormattedActionCommandString,
                    String.IsNullOrEmpty(Aggregate) ? "" : ", " + Aggregate,
                    String.IsNullOrEmpty(Comment) ? "" : " COMMENT \"" + Comment + "\""
                    );
            }
        }
        public override string DDLText
        {
            get { return isSystemRule ? AlterCommandString : CreateCommandString; }
        }

        /// <summary>
        /// System in which the service is defined
        /// </summary>
        public WmsSystem WmsSystem
        {
            get
            {
                return _wmsSystem;
            }
        }

        /// <summary>
        /// Identifies if this service is system created
        /// </summary>
        public bool isSystemRule
        {
            get
            {
                return isASystemObject(this.Name);
            }
        }
        /// <summary>
        /// Name of the service
        /// </summary>
        override public string Name
        {
            get
            {
                return _name;
            }
            set
            {
                _name = value;
            }
        }

        /// <summary>
        /// Formatted name of the service
        /// </summary>
        public string FormattedName
        {
            //get { return NCCWMSCommand.escapeServiceName(Name); }
            get
            {
                return Name;
            }
        }

        /// <summary>
        /// Rule type
        /// </summary>
        public string RuleType
        {
            get
            {
                return _ruleType;
            }
            set
            {
                _ruleType = value;
            }
        }

        /// <summary>
        /// Rule Expression
        /// </summary>
        public string Expression
        {
            get
            {
                if (_expression == null)
                {
                    Refresh();
                }
                return _expression;
            }
            set
            {
                _expression = value;
            }
        }

        /// <summary>
        /// Rule expression operator
        /// </summary>
        public string Operator
        {
            get { return _operator; }
            set { _operator = value; }
        }

        /// <summary>
        /// Rule Warning level
        /// </summary>
        public string WarnLevel
        {
            get
            {
                return _warnLevel;
            }
            set
            {
                _warnLevel = value;
            }
        }
        /// <summary>
        /// Rule Action Type
        /// </summary>
        public string ActionType
        {
            get
            {
                return _actionType;
            }
            set
            {
                _actionType = value;
            }
        }

        /// <summary>
        /// SQL command
        /// </summary>
        public string ActionCommand
        {
            get
            {
                return _actionCommand;
            }
            set
            {
                _actionCommand = value;
            }
        }

        /// <summary>
        /// Aggregate SQL command
        /// </summary>
        public string Aggregate
        {
            get { return getAggregateString(); }
        }

        /// <summary>
        /// Aggregate Query Types
        /// </summary>
        public string AggregateQueryTypes
        {
            get { return _aggrQueryTypes; }
            set { _aggrQueryTypes = value; }
        }

        /// <summary>
        /// Aggregate Repository Interval
        /// </summary>
        public Int16 AggregateRepositoryInterval
        {
            get { return _aggrReposInterval; }
            set { _aggrReposInterval = value; }
        }

        /// <summary>
        /// Aggregate WMS Interval
        /// </summary>
        public Int16 AggregateWmsInterval
        {
            get { return _aggrWmsInterval; }
            set { _aggrWmsInterval = value; }
        }

        /// <summary>
        /// Aggregate Execute Interval
        /// </summary>
        public Int16 AggregateExecInterval
        {
            get { return _aggrExecInterval; }
            set { _aggrExecInterval = value; }
        }

        public bool IsStatsCollectOnce
        {
            get { return (!string.IsNullOrEmpty(_statsCollectOnce) && _statsCollectOnce.Trim().Equals("ON")); }
            set {_statsCollectOnce = value ? "ON" : "OFF"; }
        }

        public string FormattedActionCommandString
        {
            get
            {
                if (!String.IsNullOrEmpty(ActionCommand))
                {
                    string actionString = ActionCommand;

                    if (RuleType.Equals(WmsCommand.CONN_RULE_TYPE) ||
                        (RuleType.Equals(WmsCommand.COMP_RULE_TYPE) &&
                         (ActionType.Equals("EXEC") || ActionType.Equals("SQL_CMD")))
                        )
                    {
                        if (!actionString.Equals("NO-ACTION") && !actionString.EndsWith(";"))
                            actionString += ";";
                    }

                    if (actionString.StartsWith("\"") && actionString.EndsWith("\""))
                        return actionString;
                    else
                    {
                        if(actionString.Equals("NO-ACTION"))
                            return actionString;
                        else
                            return "\"" + actionString + "\"";
                    }
                }
                return "";
            }
        }
        /// <summary>
        /// Rule comment
        /// </summary>
        public string Comment
        {
            get
            {
                return _comment;
            }
            set
            {
                _comment = value;
            }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs a new WmsRules instance
        /// </summary>
        /// <param name="wmsSystem"></param>
        public WmsRule(WmsSystem wmsSystem)
            :base(wmsSystem.ConnectionDefinition)
        {
            _wmsSystem = wmsSystem;
        }

        /// <summary>
        /// Read and Set service attributes from the OdbcDataReader
        /// </summary>
        /// <param name="dataReader"></param>
        internal void SetAttributes(DataRow dataRow)
        {
            _name = dataRow[WmsCommand.COL_RULE_NAME] as string;
            _ruleType = dataRow[WmsCommand.COL_RULE_TYPE] as string;
            _operator = dataRow[WmsCommand.COL_RULE_OPER] as string;
            string act = dataRow[WmsCommand.COL_RULE_ACTION] as string;
            if (_ruleType.Equals(WmsCommand.CONN_RULE_TYPE) && !String.IsNullOrEmpty(act))
            {
                _actionType = "SQL_CMD";
                _actionCommand = act;
            }
            else if (_ruleType.Equals(WmsCommand.COMP_RULE_TYPE) && !String.IsNullOrEmpty(act))
            {
                if (!act.Equals("REJECT") && !act.Equals("HOLD"))
                {
                    _actionType = "SQL_CMD";

                    _actionCommand = act;
                }
                else
                {
                    _actionType = act;
                    _actionCommand = "";
                }
            }
            else
            {
                _actionType = act;
                _actionCommand = "";
            }
            _comment = dataRow[WmsCommand.COL_COMMENT] as string;

            string warnLevel = dataRow[WmsCommand.COL_RULE_WARN_LEVEL] as string;
            if (warnLevel.Equals("LOW"))
                _warnLevel = "WARN-LOW";
            else if (warnLevel.Equals("MEDIUM"))
                _warnLevel = "WARN-MEDIUM";
            else if (warnLevel.Equals("HIGH"))
                _warnLevel = "WARN-HIGH";
            else if (warnLevel.Equals("NO-WARN"))
                _warnLevel = "NO-WARN";

            _aggrQueryTypes = dataRow[WmsCommand.COL_RULE_AGGR_QUERY_TYPES] as string;
            _aggrReposInterval = Int16.Parse(dataRow[WmsCommand.COL_RULE_AGGR_REPOS_INTERVAL].ToString());
            _aggrWmsInterval = Int16.Parse(dataRow[WmsCommand.COL_RULE_AGGR_WMS_INTERVAL].ToString());
            _aggrExecInterval = Int16.Parse(dataRow[WmsCommand.COL_RULE_AGGR_EXEC_INTERVAL].ToString());
            if (this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _statsCollectOnce = dataRow[WmsCommand.COL_RULE_AGGR_STATS_ONCE].ToString();
            }
        }

        string getAggregateString()
        {
            string aggregate = "";
            if (RuleType.Equals(WmsCommand.CONN_RULE_TYPE))
            {
                if (!string.IsNullOrEmpty(_aggrQueryTypes))
                {
                    string[] queryTypes = _aggrQueryTypes.Split(',');
                    if (queryTypes.Length > 0)
                    {
                        aggregate = "AGGREGATE (QUERY_TYPE(";
                        for (int i = 0; i < queryTypes.Length; i++)
                        {
                            if (i > 0)
                                aggregate += ", ";
                            aggregate += queryTypes[i];
                        }
                        aggregate += "), ";
                        aggregate += "WMS_INTERVAL " + _aggrWmsInterval + ", ";
                        aggregate += "REPOS_INTERVAL " + _aggrReposInterval + ", ";
                        aggregate += "EXEC_INTERVAL " + _aggrExecInterval;

                        if (_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                        {
                            aggregate += ", STATS_ONCE " + _statsCollectOnce;
                        }
                        aggregate += ") ";
                    }
                }
                else
                {
                    aggregate = "AGGREGATE NO-ACTION";
                }
            }
            return aggregate;
        }
        /// <summary>
        /// Refreshes the rule attributes
        /// </summary>
        override public void Refresh()
        {
            try
            {
                GetConnection();
                DataTable dataTable = WmsCommand.executeCommand("STATUS RULE " + Name,
                    _connection.OpenOdbcConnection, 60);

                //Set the attributes
                if (dataTable.Rows.Count > 0)
                {
                    _serviceNames = null;

                    SetAttributes(dataTable.Rows[0]);

                    _expression = ProcessRuleExpression(dataTable);

                    OnWmsModelEvent(WmsCommand.WMS_ACTION.STATUS_RULE, this);
                }
                else
                {
                    //No records returned. Maybe the rule has been removed
                    WmsSystem.WmsRules.Remove(this);
                    this.OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_RULE, this);
                }

                WmsSystem.WmsRules.Sort();
            }
            catch (OdbcException)
            {
                //Failed to fetch. Maybe the rule has been removed
                WmsSystem.WmsRules.Remove(this);
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_RULE, this);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        public static string ProcessRuleExpression(DataTable aDataTable)
        {
            DataRow[] dataRows = new DataRow[aDataTable.Rows.Count];
            aDataTable.Rows.CopyTo(dataRows, 0);
            return ProcessRuleExpression(dataRows);
        }

        public static string ProcessRuleExpression(DataRow[] dataRows)
        {
            string expression = "";
            short rows = 0;

            foreach (DataRow dataRow in dataRows)
            {
                string oper = dataRow[WmsCommand.COL_RULE_OPER] as string;
                string expr = dataRow[WmsCommand.COL_RULE_EXPR] as string;
                string ruleType = dataRow[WmsCommand.COL_RULE_TYPE] as string;

                if (ruleType.Equals(WmsCommand.CONN_RULE_TYPE))
                {
                    int index = expr.IndexOf("=");
                    if (index != -1)
                    {
                        string attr = expr.Substring(0, index);
                        string value = expr.Substring(index + 2);
                        if (value.Contains(" ") && !value.StartsWith("\"") && !value.EndsWith("\""))
                        {
                            value = "\"" + value + "\"";
                        }

                        expr = attr + value;
                    }
                    else
                    {
                        int icaseStart = expr.IndexOf("ICASE (");
                        int icaseEnd = expr.LastIndexOf(")");
                        if (icaseEnd != -1 && icaseStart != -1 && icaseEnd > icaseStart)
                        {
                            string attr = expr.Substring(0, icaseStart);
                            int n = icaseStart + 7;
                            string value = expr.Substring(n, icaseEnd - n);
                            if (value.Contains(" ") && !value.StartsWith("\"") && !value.EndsWith("\""))
                            {
                                value = "\"" + value + "\"";
                            }

                            expr = attr + "ICASE (" + value + ")";
                        }
                    }
                }
                if (rows == 0)
                {
                    //if (ruleType.Equals(WmsCommand.CONN_RULE_TYPE))
                    //{
                    //    expression = expr;
                    //}
                    //else
                    //{
                    //    expression = oper + ", " + expr;                    
                    //}
                    expression = expr;
                }
                else
                {
                    expression += "," + Environment.NewLine + " " + expr;
                }

                rows++;
            }
            return expression;
        }

        /// <summary>
        /// Adds the rule to the WMS system configuration
        /// </summary>
        public void Add()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery(CreateCommandString, _connection.OpenOdbcConnection, 60);

                //If add is successful, add the rule to the WmsSystem's rule list.
                _wmsSystem.AddRuleToList(this);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }

        }

        /// <summary>
        /// Alters the rule
        /// </summary>
        public void Alter()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery(AlterCommandString, _connection.OpenOdbcConnection, 60);
                OnWmsModelEvent(WmsCommand.WMS_ACTION.ALTER_RULE, this);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        /// <summary>
        /// Deletes the rule
        /// </summary>
        /// <param name="isImmediate">Indicates if the delete should be immediate</param>
        public void Delete()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery("DELETE RULE " + Name, _connection.OpenOdbcConnection, 60);
                _wmsSystem.DeleteRuleFromList(this);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        /// <summary>
        /// Resets the service name associations
        /// Raises an event, so the listeners can update their associations.
        /// </summary>
        public void ResetAssociatedServiceNames()
        {
            if (_serviceNames != null)
            {
                _serviceNames.Clear();
            }
            _serviceNames = null;
            OnWmsModelEvent(WmsCommand.WMS_ACTION.RULE_ASSOC_CHANGED, this);
        }

        private void LoadAssociatedServiceNames()
        {
            _serviceNames = new List<string>();
            try
            {
                GetConnection();
                DataTable dataTable = WmsCommand.executeCommand("STATUS SERVICE RULE " + Name,
                    _connection.OpenOdbcConnection, 60);
                foreach (DataRow dataRow in dataTable.Rows)
                {
                    _serviceNames.Add(dataRow[WmsCommand.COL_SERVICE_NAME] as string);
                }
            }
            catch (Exception)
            {
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        /// <summary>
        /// Returns the rule name
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Name;
        }

        //protected void GetConnection()
        //{
        //    if (_connection == null)
        //    {
        //        _connection = new Connection(ConnectionDefinition);
        //    }
        //} 
    }
}
