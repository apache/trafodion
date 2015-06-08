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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// Model for a WMS service
    /// </summary>
    public class WmsService : WmsObject
    {
        #region Private member variables

        private WmsSystem _wmsSystem;
        private string _name;
        private string _priority;
        private string _maxCpuBusy;
        private int _maxMemUsage;
        private int _maxSSDUsage;
        private int _maxExecQueries;
        private int _maxEsps;
        private string _cancelQueryIfClientDisappears;
        private string _checkQueryEstimatedResourceUse;
        private Int64 _maxRowsFetched;
        private string _activeTime;
        private string _sqlPlan;
        private string _sqlText;
        private int _execTimeOut;
        private int _waitTimeOut;
        private int _holdTimeOut;
        private string _sqlDefaults;
        private string _comment;
        private string _state;
        private bool _overrideDSNPriority;
        private List<string> _compilationRules = new List<string>();
        private List<string> _executionRules = new List<string>();

        #endregion Private Fields

        #region Public properties

        public const string ACTIVE = "ACTIVE";
        public const string HOLD = "HOLD";
        public const string STOPPED = "STOPPED";

        public List<WmsRule> AssociatedRules
        {
            get
            {
                List<WmsRule> associatedRules = new List<WmsRule>();
                foreach (WmsRule wmsRule in _wmsSystem.WmsRules)
                {
                    foreach (string serviceName in wmsRule.AssociatedServiceNames)
                    {
                        if (Name.Equals(serviceName))
                        {
                            associatedRules.Add(wmsRule);
                            break;
                        }
                    }
                }
                return associatedRules;
            }
        }
        public List<WmsRule> AssociatedConnectionRules
        {
            get
            {
                List<WmsRule> associatedConnectionRules = new List<WmsRule>();
                foreach (WmsRule wmsConnectiontionRule in _wmsSystem.WmsConnectionRules)
                {
                    foreach (string serviceName in wmsConnectiontionRule.AssociatedServiceNames)
                    {
                        if (Name.Equals(serviceName))
                        {
                            associatedConnectionRules.Add(wmsConnectiontionRule);
                            break;
                        }
                    }
                }
                return associatedConnectionRules;
            }
        }
        public List<WmsRule> AssociatedCompilationRules
        {
            get
            {
                List<WmsRule> associatedCompilationRules = new List<WmsRule>();
                foreach (WmsRule wmsCompilationRule in _wmsSystem.WmsCompilationRules)
                {
                    foreach (string serviceName in wmsCompilationRule.AssociatedServiceNames)
                    {
                        if (Name.Equals(serviceName))
                        {
                            associatedCompilationRules.Add(wmsCompilationRule);
                            break;
                        }
                    }
                }
                return associatedCompilationRules;
            }
        }
        public List<WmsRule> AssociatedExecutionRules
        {
            get
            {
                List<WmsRule> associatedExecutionRules = new List<WmsRule>();
                foreach (WmsRule wmsExecutionRule in _wmsSystem.WmsExecutionRules)
                {
                    foreach (string serviceName in wmsExecutionRule.AssociatedServiceNames)
                    {
                        if (Name.Equals(serviceName))
                        {
                            associatedExecutionRules.Add(wmsExecutionRule);
                            break;
                        }
                    }
                }
                return associatedExecutionRules;
            }
        }
        /// <summary>
        /// The Create wms command to create this wms service
        /// </summary>
        public override string CreateCommandString
        {
            get
            {
                // Available for versions before M9
                if (this._connectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    return String.Format("ADD SERVICE {0} {1} {2}, MAX_CPU_BUSY {3}, MAX_MEM_USAGE {4}, " +
                                            "MAX_ROWS_FETCHED {5}," + Environment.NewLine +
                                            "       ACTIVE {6}, {7}, {8}," + Environment.NewLine +
                                            "       EXEC_TIMEOUT {9}, WAIT_TIMEOUT {10}, HOLD_TIMEOUT {11}, " +
                                            "{12} {13}, " +
                                            "{14} {15}, " +
                                            "{16}{17};",
                                            FormattedName,
                                            this.OverrideDSNPriority ? "PRIORITY_OVERRIDE" : "PRIORITY",
                                            this.Priority, this.MaxCpuBusy, this.MaxMemUsage,
                                            this.MaxRowsFetched, this.ActiveTime, this.SqlPlan, this.SqlText,
                                            this.ExecTimeOut, this.WaitTimeOut, this.HoldTimeOut,
                                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                                            CommentSqlClauseString,
                                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);

                }
                else if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    return String.Format("ADD SERVICE {0} {1} {2}, MAX_CPU_BUSY {3}, MAX_MEM_USAGE {4}, " +
                                            "MAX_ROWS_FETCHED {5}," + Environment.NewLine +
                                            "       ACTIVE {6}, {7}, {8}," + Environment.NewLine +
                                            "       EXEC_TIMEOUT {9}, WAIT_TIMEOUT {10}, HOLD_TIMEOUT {11}, " +
                                            "{12} {13}, " +
                                            "{14} {15}, " +
                                            "{16} {17}, " +
                                            "{18}{19};",
                                            FormattedName,
                                            this.OverrideDSNPriority ? "PRIORITY_OVERRIDE" : "PRIORITY",
                                            this.Priority, this.MaxCpuBusy, this.MaxMemUsage,
                                            this.MaxRowsFetched, this.ActiveTime, this.SqlPlan, this.SqlText,
                                            this.ExecTimeOut, this.WaitTimeOut, this.HoldTimeOut, 
                                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                                            WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, this._cancelQueryIfClientDisappears,
                                            CommentSqlClauseString,
                                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                }
                else
                {
                    return String.Format("ADD SERVICE {0} {1} {2}, MAX_CPU_BUSY {3}, MAX_MEM_USAGE {4}, " +
                                            "MAX_ROWS_FETCHED {5}," + Environment.NewLine +
                                            "       ACTIVE {6}, {7}, {8}," + Environment.NewLine +
                                            "       EXEC_TIMEOUT {9}, WAIT_TIMEOUT {10}, HOLD_TIMEOUT {11}, " +
                                            "{12} {13}, " +
                                            "{14} {15}, " +
                                            "{16} {17}, " +
                                            "{18} {19}, " +
                                            "{20}{21};",
                                            FormattedName,
                                            this.OverrideDSNPriority ? "PRIORITY_OVERRIDE" : "PRIORITY",
                                            this.Priority, this.MaxCpuBusy, this.MaxMemUsage,
                                            this.MaxRowsFetched, this.ActiveTime, this.SqlPlan, this.SqlText,
                                            this.ExecTimeOut, this.WaitTimeOut, this.HoldTimeOut, 
                                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                                            WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, this._cancelQueryIfClientDisappears,
                                            WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, this._checkQueryEstimatedResourceUse,
                                            CommentSqlClauseString,
                                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                }
            }
        }

        /// <summary>
        /// The Alter wms command to alter this wms service
        /// </summary>
        public override string AlterCommandString
        {
            get
            {
                //If system service, then only certain attributes can be modified
                if (isSystemService)    
                {
                    // Available for version before M9
                    if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        return String.Format("ALTER SERVICE {0} MAX_CPU_BUSY {1}, MAX_MEM_USAGE {2}, " +
                            "MAX_ROWS_FETCHED {3}, {4}, {5}," + Environment.NewLine +
                            "         EXEC_TIMEOUT {6}, WAIT_TIMEOUT {7}, HOLD_TIMEOUT {8}, " +
                            "{9} {10}, " +
                            "{11} {12}, " +
                            "{13}{14};",
                            FormattedName,
                            this.MaxCpuBusy, this.MaxMemUsage, this.MaxRowsFetched,
                            this.SqlPlan, this.SqlText, this.ExecTimeOut, this.WaitTimeOut, this.HoldTimeOut, 
                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                            CommentSqlClauseString,
                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                    }
                    else if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        return String.Format("ALTER SERVICE {0} MAX_CPU_BUSY {1}, MAX_MEM_USAGE {2}, " +
                            "MAX_ROWS_FETCHED {3}, {4}, {5}," + Environment.NewLine +
                            "         EXEC_TIMEOUT {6}, WAIT_TIMEOUT {7}, HOLD_TIMEOUT {8}, " +
                            "{9} {10}, " +
                            "{11} {12}, " +
                            "{13} {14}, " +
                            "{15}{16};",
                            FormattedName,
                            this.MaxCpuBusy, this.MaxMemUsage, this.MaxRowsFetched,
                            this.SqlPlan, this.SqlText, this.ExecTimeOut, this.WaitTimeOut, this.HoldTimeOut, 
                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                            WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, this._cancelQueryIfClientDisappears,
                            CommentSqlClauseString,
                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                    }
                    else
                    {
                        return String.Format("ALTER SERVICE {0} MAX_CPU_BUSY {1}, MAX_MEM_USAGE {2}, " +
                            "MAX_ROWS_FETCHED {3}, {4}, {5}," + Environment.NewLine +
                            "         EXEC_TIMEOUT {6}, WAIT_TIMEOUT {7}, HOLD_TIMEOUT {8}, " +
                            "{9} {10}, " +
                            "{11} {12}, " +
                            "{13} {14}, " +
                            "{15} {16}, " +
                            "{17}{18};",
                            FormattedName,
                            this.MaxCpuBusy, this.MaxMemUsage, this.MaxRowsFetched,
                            this.SqlPlan, this.SqlText, this.ExecTimeOut, this.WaitTimeOut, this.HoldTimeOut,
                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                            WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, this._cancelQueryIfClientDisappears,
                            WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, this._checkQueryEstimatedResourceUse,
                            CommentSqlClauseString,
                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                    }
                }
                else //For user created services
                {
                    // Available for version before M9
                    if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        return String.Format("ALTER SERVICE {0} PRIORITY {1}, MAX_CPU_BUSY {2},MAX_MEM_USAGE {3}, MAX_ROWS_FETCHED {4}, " +
                            "ACTIVE {5}, {6}, {7}," + Environment.NewLine +
                            "         EXEC_TIMEOUT {8}, WAIT_TIMEOUT {9}, HOLD_TIMEOUT {10}, " +
                            "{11} {12}, " +
                            "{13} {14}, " +
                            "{15}{16};",
                            FormattedName,
                            Priority, MaxCpuBusy, MaxMemUsage, MaxRowsFetched,
                            ActiveTime, SqlPlan, SqlText, ExecTimeOut, WaitTimeOut, HoldTimeOut, 
                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                            CommentSqlClauseString,
                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                    }
                    else if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        return String.Format("ALTER SERVICE {0} PRIORITY {1}, MAX_CPU_BUSY {2},MAX_MEM_USAGE {3}, MAX_ROWS_FETCHED {4}, " +
                            "ACTIVE {5}, {6}, {7}," + Environment.NewLine +
                            "         EXEC_TIMEOUT {8}, WAIT_TIMEOUT {9}, HOLD_TIMEOUT {10}, " +
                            "{11} {12}, " +
                            "{13} {14}, " +
                            "{15} {16}, " +
                            "{17}{18};",
                            FormattedName,
                            Priority, MaxCpuBusy, MaxMemUsage, MaxRowsFetched,
                            ActiveTime, SqlPlan, SqlText, ExecTimeOut, WaitTimeOut, HoldTimeOut, 
                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                            WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, this._cancelQueryIfClientDisappears,
                            CommentSqlClauseString,
                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                    }
                    else
                    {
                        return String.Format("ALTER SERVICE {0} PRIORITY {1}, MAX_CPU_BUSY {2},MAX_MEM_USAGE {3}, MAX_ROWS_FETCHED {4}, " +
                            "ACTIVE {5}, {6}, {7}," + Environment.NewLine +
                            "         EXEC_TIMEOUT {8}, WAIT_TIMEOUT {9}, HOLD_TIMEOUT {10}, " +
                            "{11} {12}, " +
                            "{13} {14}, " +
                            "{15} {16}, " +
                            "{17} {18}, " +
                            "{19}{20};",
                            FormattedName,
                            Priority, MaxCpuBusy, MaxMemUsage, MaxRowsFetched,
                            ActiveTime, SqlPlan, SqlText, ExecTimeOut, WaitTimeOut, HoldTimeOut, 
                            WmsCommand.COL_MAX_EXEC_QUERIES, this._maxExecQueries,
                            WmsCommand.COL_MAX_ESPS, this._maxEsps,
                            WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, this._cancelQueryIfClientDisappears,
                            WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, this._checkQueryEstimatedResourceUse,
                            CommentSqlClauseString,
                            String.IsNullOrEmpty(SqlDefaultsSqlClauseString) ? "" : ", " + SqlDefaultsSqlClauseString);
                    }
                }
            }
        }

        public string AssociateCompilationRuleString
        {
            get
            {
                string associationString = "";
                if (AssociatedCompilationRules.Count > 0)
                {
                    associationString += "ALTER SERVICE " + FormattedName + " COMP ( ";
                    for (int i = 0; i < AssociatedCompilationRules.Count; i++)
                    {
                        associationString += AssociatedCompilationRules[i].Name;
                        if (i < AssociatedCompilationRules.Count - 1)
                        {
                            associationString += " , ";
                        }
                    }
                    associationString += " );";
                }
                return associationString;
            }
        }

        public string AssociateExecutionRuleString
        {
            get
            {
                string associationString = "";
                if (AssociatedExecutionRules.Count > 0)
                {
                    associationString += "ALTER SERVICE " + FormattedName + " EXEC ( ";
                    for (int i = 0; i < AssociatedExecutionRules.Count; i++)
                    {
                        associationString += AssociatedExecutionRules[i].Name;
                        if (i < AssociatedExecutionRules.Count - 1)
                        {
                            associationString += " , ";
                        }
                    }
                    associationString += " );";
                }
                return associationString;
            }
        }

        public override string DDLText
        {
            get { return isSystemService ? AlterCommandString : CreateCommandString; }
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
        public bool isSystemService
        {
            get
            {
                return isASystemService(this.Name);
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
        /// Option to override datasource priority with service priority
        /// </summary>
        public bool OverrideDSNPriority
        {
            get
            {
                return _overrideDSNPriority;
            }
            set
            {
                _overrideDSNPriority = value;
            }
        }
        /// <summary>
        /// Service priority
        /// </summary>
        public string Priority
        {
            get
            {
                return _priority;
            }
            set
            {
                _priority = value;
            }
        }

        /// <summary>
        /// State of the service
        /// </summary>
        public string State
        {
            get
            {
                return _state;
            }
            set
            {
                _state = value;
            }
        }
        /// <summary>
        /// Maximum allowed CPU busy in Percent
        /// </summary>
        public string MaxCpuBusy
        {
            get
            {
                return _maxCpuBusy;
            }
            set
            {
                _maxCpuBusy = value;
            }
        }

        /// <summary>
        /// Maximum allowed memory usage in Percent
        /// </summary>
        public int MaxMemUsage
        {
            get
            {
                return _maxMemUsage;
            }
            set
            {
                _maxMemUsage = value;
            }
        }

        /// <summary>
        /// Maximum allowed memory usage in Percent
        /// </summary>
        public int MaxSSDUsage
        {
            get
            {
                return _maxSSDUsage;
            }
            set
            {
                _maxSSDUsage = value;
            }
        }

        /// <summary>
        /// Maximum rows fetched.
        /// </summary>
        public Int64 MaxRowsFetched
        {
            get
            {
                return _maxRowsFetched;
            }
            set
            {
                _maxRowsFetched = value;
            }
        }

        /// <summary>
        /// Active time for this service
        /// </summary>
        public string ActiveTime
        {
            get
            {
                return _activeTime;
            }
            set
            {
                _activeTime = value;
            }
        }

        /// <summary>
        /// Indicates if SQL Plan is included or not
        /// </summary>
        public string SqlPlan
        {
            get
            {
                return _sqlPlan;
            }
            set
            {
                _sqlPlan = value;
            }
        }

        /// <summary>
        /// Indicates if SQL Text is included or not
        /// </summary>
        public string SqlText
        {
            get
            {
                return _sqlText;
            }
            set
            {
                _sqlText = value;
            }
        }

        /// <summary>
        /// Indicates if SQL Plan is included or not
        /// </summary>
        public bool IsSqlPlan
        {
            get
            {
                return _sqlPlan.Equals(WmsCommand.PLAN);
            }
            set
            {
                if (value)
                {
                    _sqlPlan = WmsCommand.PLAN;
                }
                else
                {
                    _sqlPlan = WmsCommand.NO_PLAN;
                }
            }
        }
        /// <summary>
        /// Indicates if SQL Text is included or not
        /// </summary>
        public bool IsSqlText
        {
            get
            {
                return _sqlText.Equals(WmsCommand.TEXT);
            }
            set
            {
                if (value)
                {
                    _sqlText = WmsCommand.TEXT;
                }
                else
                {
                    _sqlText = WmsCommand.NO_TEXT;
                }
            }
        }

        /// <summary>
        /// Execution timeout in seconds
        /// </summary>
        public int ExecTimeOut
        {
            get
            {
                return _execTimeOut;
            }
            set
            {
                _execTimeOut = value;
            }
        }

        /// <summary>
        /// Wait timeout in seconds
        /// </summary>
        public int WaitTimeOut
        {
            get
            {
                return _waitTimeOut;
            }
            set
            {
                _waitTimeOut = value;
            }
        }

        /// <summary>
        /// Hold timeout in minutes. 
        /// </summary>
        public int HoldTimeOut
        {
            get
            {
                return _holdTimeOut;
            }
            set
            {
                _holdTimeOut = value;
            }
        }



        /// <summary>
        /// MAX_EXEC_QUERIES is a threshold for better using the system resources. 
        /// It’s the maximum number of executing queries allowed in the instance. 
        /// When the threshold used by specific service is reached, new queries will be sent to the waiting queue. 
        /// When number of current executing queries in the instance becomes lower than the threshold value, the waiting queries will be released for execution.
        /// </summary>
        public int MaxExecQueries
        {
            get { return _maxExecQueries; }
            set { _maxExecQueries = value; }
        }


        public string CancelQueryIfClientDisappears
        {
            get
            {
                return _cancelQueryIfClientDisappears;
            }
            set
            {
                _cancelQueryIfClientDisappears = value;
            }
        }

        public string CheckQueryEstimatedResourceUse
        {
            get
            {
                return _checkQueryEstimatedResourceUse;
            }
            set { _checkQueryEstimatedResourceUse = value; }
        }

        /// <summary>
        /// MAX_ESPS is a threshold for better using the system resources. 
        /// It’s the average number of ESPs in the instance. 
        /// When the threshold used by specific service is reached, new queries will be sent to the waiting queue. 
        /// When average number of ESPs in the instance becomes lower than the threshold value, the waiting queries will be released for execution.
        /// </summary>
        public int MaxEsps
        {
            get { return _maxEsps;  }
            set { _maxEsps = value; }
        }

        /// <summary>
        /// Sql Defaults Clause in Add/Alter service command
        /// </summary>
        public string SqlDefaults
        {
            get
            {
                return _sqlDefaults;
            }
            set
            {
                _sqlDefaults = value;
            }
        }

        /// <summary>
        /// SQL_DEFAULTS clause for the Add/Alter service command
        /// </summary>
        internal string SqlDefaultsSqlClauseString
        {
            get
            {
                if (this._wmsSystem.AdminCanSetSQLDefaultsOnService)
                {
                    if (!String.IsNullOrEmpty(_sqlDefaults))
                        return WmsCommand.COL_SQL_DEFAULTS + " \"" + _sqlDefaults + "\"";
                    //else if (_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.R24)
                    else
                        return WmsCommand.COL_SQL_DEFAULTS + " RESET";
                }

                return "";
            }
        }

        /// <summary>
        /// Comments
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

        /// <summary>
        /// COMMENT clause for the Add/Alter service command
        /// </summary>
        internal string CommentSqlClauseString
        {
            get
            {
                return (WmsCommand.COL_COMMENT +
                    (String.IsNullOrEmpty(_comment) ? " ''" : " \"" + _comment + "\""));
            }
        }

        public List<string> CompilationRules
        {
            get
            {
                LoadCompilationRules();
                return _compilationRules;
            }
        }
        public List<string> ExecutionRules
        {
            get
            {
                LoadExecutionRules();
                return _executionRules;
            }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs a new WmsService instance
        /// </summary>
        /// <param name="wmsSystem"></param>
        public WmsService(WmsSystem wmsSystem)
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
            _name = dataRow[WmsCommand.COL_SERVICE_NAME] as string;
            _state = dataRow[WmsCommand.COL_STATE] as string;
            _priority = dataRow[WmsCommand.COL_SERVICE_PRIORITY] as string;
            _maxCpuBusy = dataRow[WmsCommand.COL_MAX_CPU_BUSY].ToString();
            _maxMemUsage = (int)dataRow[WmsCommand.COL_MAX_MEM_USAGE];

            try
            {
                _maxSSDUsage = (int)dataRow[WmsCommand.COL_MAX_SSD_USAGE];
            }
            catch { }

            // Available for M7 or later
            if (this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _maxExecQueries = (int)dataRow[WmsCommand.COL_MAX_EXEC_QUERIES];
            }

            // Avaiable for M8 or later
            if (this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
            {
                _maxEsps = (int)dataRow[WmsCommand.COL_MAX_ESPS];
            }

            // Avaiable for M9 or later
            if (this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                _cancelQueryIfClientDisappears = (string)dataRow[WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS];
            }

            // Avaiable for M10 or later
            if (this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _checkQueryEstimatedResourceUse = (string)dataRow[WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE];
            }

            _activeTime = dataRow[WmsCommand.COL_ACTIVE_TIME] as string;
            _sqlPlan = dataRow[WmsCommand.COL_SQL_PLAN] as string;
            _sqlText = dataRow[WmsCommand.COL_SQL_TEXT] as string;
            _execTimeOut = (int)dataRow[WmsCommand.COL_EXEC_TIMEOUT];
            _waitTimeOut = (int)dataRow[WmsCommand.COL_WAIT_TIMEOUT];
            try
            {
                _comment = dataRow[WmsCommand.COL_COMMENT] as string;
            }
            catch (Exception ex)
            {
                _comment = "";
            }

            if (!String.IsNullOrEmpty(ActiveTime))
            {
                ActiveTime = ActiveTime.Replace("FROM", "");
            }

            //if (_wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.R24)
            //{
                if (dataRow[WmsCommand.COL_MAX_ROWS_FETCHED] is long)
                    _maxRowsFetched = (Int64)dataRow[WmsCommand.COL_MAX_ROWS_FETCHED];
                else
                    if (dataRow[WmsCommand.COL_MAX_ROWS_FETCHED] is decimal)
                        _maxRowsFetched = Decimal.ToInt64((decimal)dataRow[WmsCommand.COL_MAX_ROWS_FETCHED]);

                _holdTimeOut = (int)dataRow[WmsCommand.COL_HOLD_TIMEOUT];
            //}
            //else
            //{
            //    _maxRowsFetched = -1;
            //    _holdTimeOut = -1;
            //}

            //SQL Defaults only applies to services user
            //if (ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
                _sqlDefaults = dataRow[WmsCommand.COL_SQL_DEFAULTS] as string;

        }

        /// <summary>
        /// Load the compilation rules associated with this service
        /// </summary>
        public void LoadCompilationRules()
        {
            this._compilationRules.Clear();

            GetConnection();

            try
            {
                DataTable dataTable = WmsCommand.executeCommand(
                    String.Format("STATUS SERVICE {0} {1}", Name, WmsCommand.COMP_RULE_TYPE),
                    _connection.OpenOdbcConnection, 60
                    );

                foreach (DataRow dataRow in dataTable.Rows)
                {
                    _compilationRules.Add(dataRow["RULE_NAME"] as string);
                }
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
        /// Load the execution rules associated with this service
        /// </summary>
        public void LoadExecutionRules()
        {
            this._executionRules.Clear();
            GetConnection();
            try
            {
                DataTable dataTable = WmsCommand.executeCommand(
                    String.Format("STATUS SERVICE {0} {1}", Name, WmsCommand.EXEC_RULE_TYPE),
                    _connection.OpenOdbcConnection, 60
                    );

                foreach (DataRow dataRow in dataTable.Rows)
                {
                    _executionRules.Add(dataRow["RULE_NAME"] as string);
                }
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
        /// Refreshes the service attributes
        /// </summary>
        override public void Refresh()
        {
            Comment = null; //This will force the comments to be re-read

            GetConnection();

            try
            {
                string cmd = "STATUS SERVICE " + FormattedName;
                DataTable dataTable = WmsCommand.executeCommand(cmd, _connection.OpenOdbcConnection, 60);

                //Set the attributes
                if (dataTable.Rows.Count > 0)
                {
                    SetAttributes(dataTable.Rows[0]);
                    OnWmsModelEvent(WmsCommand.WMS_ACTION.STATUS_SERVICE, this);
                }
                else
                {
                    //No records returned. Maybe the service has been removed
                    WmsSystem.WmsServices.Remove(this);
                    this.OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_SERVICE, this);
                }

                WmsSystem.WmsServices.Sort();

            }
            catch (OdbcException)
            {
                //Failed to fetch. Maybe the service has been removed
                WmsSystem.WmsServices.Remove(this);
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_SERVICE, this);
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
        /// Adds the service to the WMS system configuration
        /// </summary>
        public void Add()
        {
            GetConnection();
            try
            {
                WmsCommand.executeNonQuery(CreateCommandString, _connection.OpenOdbcConnection, 60);
                //If add is successful, add the service to the WmsSystem's services list.
                _wmsSystem.AddServiceToList(this);
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
        /// Alters the service
        /// </summary>
        public void Alter()
        {
            GetConnection();

            try
            {
                WmsCommand.executeNonQuery(AlterCommandString, _connection.OpenOdbcConnection, 60);
                OnWmsModelEvent(WmsCommand.WMS_ACTION.ALTER_SERVICE, this);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        public void AlterServiceRuleAssociation(string associationString)
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery(associationString, _connection.OpenOdbcConnection, 60);
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
        /// Starts the service from a STOPPED state
        /// </summary>
        public void Start()
        {
            GetConnection();
            try
            {
                WmsCommand.executeNonQuery("START SERVICE " + FormattedName, _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.ACTIVE_STATE;
                OnWmsModelEvent(WmsCommand.WMS_ACTION.START_SERVICE, this);
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
        /// Stops the service from a ACTIVE state
        /// </summary>
        public void Stop(bool stopImmediately)
        {
            GetConnection();
            try
            {
                String stopCmd = "STOP SERVICE " + FormattedName;
                if (stopImmediately) 
                {
                    stopCmd += " IMMEDIATE";
                }                    
                WmsCommand.executeNonQuery(stopCmd, _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.STOPPED_STATE;
                OnWmsModelEvent(WmsCommand.WMS_ACTION.STOP_SERVICE, this);
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
        /// Deletes the service
        /// </summary>
        /// <param name="isImmediate">Indicates if the delete should be immediate</param>
        public void Delete(bool isImmediate)
        {
            GetConnection();
            try
            {
                WmsCommand.executeNonQuery("DELETE SERVICE " + FormattedName, _connection.OpenOdbcConnection, 60);
                _wmsSystem.WmsServices.Remove(this);
                OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_SERVICE, this);
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
        /// Places the service on hold
        /// </summary>
        public void Hold()
        {
            GetConnection();
            try
            {
                WmsCommand.executeNonQuery("HOLD SERVICE " + FormattedName, _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.HOLD_STATE;
                OnWmsModelEvent(WmsCommand.WMS_ACTION.HOLD_SERVICE, this);
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
        /// Releases the service from hold
        /// </summary>
        public void Release()
        {
            GetConnection();
            try
            {
                WmsCommand.executeNonQuery("RELEASE SERVICE " + FormattedName, _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.ACTIVE_STATE;
                OnWmsModelEvent(WmsCommand.WMS_ACTION.RELEASE_SERVICE, this);
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
        /// Returns the service name
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Name;
        }
    }
}
