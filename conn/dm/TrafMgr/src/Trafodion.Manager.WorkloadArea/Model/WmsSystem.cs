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
using Trafodion.Manager.Framework.Connections;
using System.Data.Odbc;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// Model for a WMS system
    /// </summary>
    public class WmsSystem : WmsObject
    {
        #region Private member variables

        private static Dictionary<string, WmsSystem> _activeWmsSystems = new Dictionary<string, WmsSystem>();
        private List<WmsService> _wmsServices = null;
        private List<WmsRule> _wmsRules = null;
        private List<WmsAdminRole> _wmsAdminRoles = null;
        private bool isConfigurationLoaded = false;
        private string _state;
        private int _maxCpuBusy;
        private double _cpuBusy;
        private int _maxMemUsage;
        private double _memUsage;
        private int _maxSSDUsage;
        private double _ssdUsage;
        private int _statsInterval;
        private string _traceObject;
        private string _traceFilePath;
        private string _traceFileName;
        private int _ruleInterval;
        private int _ruleIntervalQueryExecTime;
        private int _maxExecQueries;
        private int _maxEsps;
        private int _maxTransactionRollbackMinutes = 5;
        private string _cancelQueryIfClientDisappears = WmsCommand.SWITCH_ON;
        private string _checkQueryEstimatedResourceUse = WmsCommand.SWITCH_OFF;
        private int _canaryIntervalMinutes = 5;
        private int _canaryExecSeconds = 0;
        private int _canaryTimeoutSeconds = 0;
        private string _canaryQuery = "SELECT ROW COUNT FROM MANAGEABILITY.NWMS_SCHEMA.WMS_CANARY";
        private int _execTimeout;
        private int _waitTimeout;
        private int _holdTimeout;
        private Int64 _maxRowsFetched;
        private List<ConnRuleAssociation> _connectionRuleAssociations = new List<ConnRuleAssociation>();
        private bool _sqlDefaultsEnabledForAdmins = false;

        #endregion Private member Variables

        #region Public properties

        /// <summary>
        /// The Create command string for the WMS system object
        /// </summary>
        public override string CreateCommandString
        {
            //Wms System is preconfigured. So no create command string applies
            get
            {
                return "";
            }
        }

        /// <summary>
        /// The Alter command string for the WMS system object
        /// </summary>
        public override string AlterCommandString
        {
            get
            {
                // Available for version before M9
                if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    return String.Format("ALTER WMS {0} {1}, {2} {3}, {4} {5}, {6} {7}, " + Environment.NewLine +
                                            "          {8} {9}, {10} {11}, {12} {13}, {14} {15}, {16} {17}, {18} {19}, {20} {21} {22};",
                                    WmsCommand.COL_MAX_CPU_BUSY, MaxCpuBusy, WmsCommand.COL_MAX_MEM_USAGE, MaxMemUsage,
                                    WmsCommand.COL_STATS_INTERVAL, StatsInterval,
                                    WmsCommand.COL_MAX_ROWS_FETCHED, MaxRowsFetched,
                                    WmsCommand.COL_EXEC_TIMEOUT, ExecTimeout, WmsCommand.COL_WAIT_TIMEOUT, WaitTimeout,
                                    WmsCommand.COL_HOLD_TIMEOUT, HoldTimeout, WmsCommand.COL_RULE_INTERVAL, RuleInterval,
                                    WmsCommand.COL_RULE_INTERVAL_QUERY_EXEC_TIME, RuleIntervalQueryExecTime,
                                    WmsCommand.COL_MAX_EXEC_QUERIES, _maxExecQueries,
                                    WmsCommand.COL_MAX_ESPS, _maxEsps,
                                    TraceOptionSqlClauseString);
                }
                // Available for M9
                else if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    return String.Format("ALTER WMS {0} {1}, {2} {3}, {4} {5}, {6} {7}, " + Environment.NewLine +
                                            "          {8} {9}, {10} {11}, {12} {13}, {14} {15}, {16} {17}, {18} {19}, {20} {21}, {22} {23}, {24} {25}, {26} {27}, {28} \"{29}\" {30};",
                                    WmsCommand.COL_MAX_CPU_BUSY, MaxCpuBusy, WmsCommand.COL_MAX_MEM_USAGE, MaxMemUsage,
                                    WmsCommand.COL_STATS_INTERVAL, StatsInterval,
                                    WmsCommand.COL_MAX_ROWS_FETCHED, MaxRowsFetched,
                                    WmsCommand.COL_EXEC_TIMEOUT, ExecTimeout, WmsCommand.COL_WAIT_TIMEOUT, WaitTimeout,
                                    WmsCommand.COL_HOLD_TIMEOUT, HoldTimeout, WmsCommand.COL_RULE_INTERVAL, RuleInterval,
                                    WmsCommand.COL_RULE_INTERVAL_QUERY_EXEC_TIME, RuleIntervalQueryExecTime,
                                    WmsCommand.COL_MAX_EXEC_QUERIES, _maxExecQueries,
                                    WmsCommand.COL_MAX_ESPS, _maxEsps,
                                    WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, _cancelQueryIfClientDisappears,
                                    WmsCommand.COL_CANARY_INTERVAL_MINUTES, _canaryIntervalMinutes,
                                    WmsCommand.COL_CANARY_EXEC_SECONDS, _canaryExecSeconds,
                                    WmsCommand.COL_CANARY_QUERY, _canaryQuery,
                                    TraceOptionSqlClauseString);
                }
                // Available for M10
                else if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ160)
                {
                    return String.Format("ALTER WMS {0} {1}, {2} {3}, {4} {5}, {6} {7}, " + Environment.NewLine +
                                            "          {8} {9}, {10} {11}, {12} {13}, {14} {15}, {16} {17}, {18} {19}, {20} {21}, {22} {23}, {24} {25}, {26} {27}, {28} {29}, {30} \"{31}\" {32};",
                                    WmsCommand.COL_MAX_CPU_BUSY, MaxCpuBusy, WmsCommand.COL_MAX_MEM_USAGE, MaxMemUsage,
                                    WmsCommand.COL_STATS_INTERVAL, StatsInterval,
                                    WmsCommand.COL_MAX_ROWS_FETCHED, MaxRowsFetched,
                                    WmsCommand.COL_EXEC_TIMEOUT, ExecTimeout, WmsCommand.COL_WAIT_TIMEOUT, WaitTimeout,
                                    WmsCommand.COL_HOLD_TIMEOUT, HoldTimeout, WmsCommand.COL_RULE_INTERVAL, RuleInterval,
                                    WmsCommand.COL_RULE_INTERVAL_QUERY_EXEC_TIME, RuleIntervalQueryExecTime,
                                    WmsCommand.COL_MAX_EXEC_QUERIES, _maxExecQueries,
                                    WmsCommand.COL_MAX_ESPS, _maxEsps,
                                    WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, _cancelQueryIfClientDisappears,
                                    WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, _checkQueryEstimatedResourceUse,
                                    WmsCommand.COL_CANARY_INTERVAL_MINUTES, _canaryIntervalMinutes,
                                    WmsCommand.COL_CANARY_EXEC_SECONDS, _canaryExecSeconds,
                                    WmsCommand.COL_CANARY_QUERY, _canaryQuery,
                                    TraceOptionSqlClauseString);
                }
                // Available for M11 and later
                else
                {
#warning : below should be restored once CANARY_TIMEOUT_SECONDS and MAX_TRANSACTION_ROLLBACK_MINUTES are availbe
                    //return String.Format("ALTER WMS {0} {1}, {2} {3}, {4} {5}, {6} {7}, " + Environment.NewLine +
                    //                        "          {8} {9}, {10} {11}, {12} {13}, {14} {15}, {16} {17}, {18} {19}, {20} {21}, {22} {23}, {24} {25}, {26} {27}, {28} {29}, {30} {31}, {32} {33}, {34} \"{35}\" {36};",
                    //                WmsCommand.COL_MAX_CPU_BUSY, MaxCpuBusy, WmsCommand.COL_MAX_MEM_USAGE, MaxMemUsage,
                    //                WmsCommand.COL_STATS_INTERVAL, StatsInterval,
                    //                WmsCommand.COL_MAX_ROWS_FETCHED, MaxRowsFetched,
                    //                WmsCommand.COL_EXEC_TIMEOUT, ExecTimeout, WmsCommand.COL_WAIT_TIMEOUT, WaitTimeout,
                    //                WmsCommand.COL_HOLD_TIMEOUT, HoldTimeout, WmsCommand.COL_RULE_INTERVAL, RuleInterval,
                    //                WmsCommand.COL_RULE_INTERVAL_QUERY_EXEC_TIME, RuleIntervalQueryExecTime,
                    //                WmsCommand.COL_MAX_EXEC_QUERIES, _maxExecQueries,
                    //                WmsCommand.COL_MAX_ESPS, _maxEsps,
                    //                WmsCommand.COL_MAX_TRANSACTION_ROLLBACK_MINUTES, _maxTransactionRollbackMinutes,
                    //                WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, _cancelQueryIfClientDisappears,
                    //                WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, _checkQueryEstimatedResourceUse,
                    //                WmsCommand.COL_CANARY_INTERVAL_MINUTES, _canaryIntervalMinutes,
                    //                WmsCommand.COL_CANARY_EXEC_SECONDS, _canaryExecSeconds,
                    //                WmsCommand.COL_CANARY_TIMEOUT_SECONDS, _canaryTimeoutSeconds,
                    //                WmsCommand.COL_CANARY_QUERY, _canaryQuery,
                    //                TraceOptionSqlClauseString);

#warning : below should be removed once CANARY_TIMEOUT_SECONDS & MAX_TRANSACTION_ROLLBACK_MINUTES are availbe
                    return String.Format("ALTER WMS {0} {1}, {2} {3}, {4} {5}, {6} {7}, " + Environment.NewLine +
                                            "          {8} {9}, {10} {11}, {12} {13}, {14} {15}, {16} {17}, {18} {19}, {20} {21}, {22} {23}, {24} {25}, {26} {27}, {28} {29}, {30} \"{31}\" {32};",
                                    WmsCommand.COL_MAX_CPU_BUSY, MaxCpuBusy, WmsCommand.COL_MAX_MEM_USAGE, MaxMemUsage,
                                    WmsCommand.COL_STATS_INTERVAL, StatsInterval,
                                    WmsCommand.COL_MAX_ROWS_FETCHED, MaxRowsFetched,
                                    WmsCommand.COL_EXEC_TIMEOUT, ExecTimeout, WmsCommand.COL_WAIT_TIMEOUT, WaitTimeout,
                                    WmsCommand.COL_HOLD_TIMEOUT, HoldTimeout, WmsCommand.COL_RULE_INTERVAL, RuleInterval,
                                    WmsCommand.COL_RULE_INTERVAL_QUERY_EXEC_TIME, RuleIntervalQueryExecTime,
                                    WmsCommand.COL_MAX_EXEC_QUERIES, _maxExecQueries,
                                    WmsCommand.COL_MAX_ESPS, _maxEsps,
                                    WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS, _cancelQueryIfClientDisappears,
                                    WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE, _checkQueryEstimatedResourceUse,
                                    WmsCommand.COL_CANARY_INTERVAL_MINUTES, _canaryIntervalMinutes,
                                    WmsCommand.COL_CANARY_EXEC_SECONDS, _canaryExecSeconds,
                                    WmsCommand.COL_CANARY_QUERY, _canaryQuery,
                                    TraceOptionSqlClauseString);
                }
            }
        }

        public override string DDLText
        {
            get { return AlterCommandString; }
        }

        public List<ConnRuleAssociation> ConnectionRuleAssociations
        {
            get
            {
                if (_connectionRuleAssociations == null)
                {
                    LoadConnectionRuleAssociations();
                }
                return _connectionRuleAssociations;
            }
        }

        /// <summary>
        /// Get the state of the system
        /// </summary>
        public string State
        {
            get
            {
                LoadConfiguration();
                return _state;
            }
        }

        /// <summary>
        /// Name
        /// </summary>
        public override string Name
        {
            get
            {
                return ConnectionDefinition.Name;
            }
            set
            {
                ;
            }
        }
        /// <summary>
        /// Maximum allowed CPU busy percentage
        /// </summary>
        public int MaxCpuBusy
        {
            get
            {
                LoadConfiguration();
                return _maxCpuBusy;
            }
            set
            {
                _maxCpuBusy = value;
            }
        }

        /// <summary>
        /// Maximum allowed memory usage percentage
        /// </summary>
        public int MaxMemUsage
        {
            get
            {
                LoadConfiguration();
                return _maxMemUsage;
            }
            set
            {
                _maxMemUsage = value;
            }
        }

        /// <summary>
        /// Maximum allowed overflow usage percentage
        /// </summary>
        public int MaxSSDUsage
        {
            get
            {
                LoadConfiguration();
                return _maxSSDUsage;
            }
            set
            {
                _maxSSDUsage = value;
            }
        }

        /// <summary>
        /// current CPU busy percentage
        /// </summary>
        public double CpuBusy
        {
            get
            {
                LoadConfiguration();
                return _cpuBusy;
            }
        }

        /// <summary>
        /// current memory usage percentage
        /// </summary>
        public double MemUsage
        {
            get
            {
                LoadConfiguration();
                return _memUsage;
            }
        }

        /// <summary>
        /// current overflow usage percentage
        /// </summary>
        public double SSDUsage
        {
            get
            {
                LoadConfiguration();
                return _ssdUsage;
            }
        }

        /// <summary>
        /// Frequency in seconds that the stats data is updated
        /// </summary>
        public int StatsInterval
        {
            get
            {
                LoadConfiguration();
                return _statsInterval;
            }
            set
            {
                _statsInterval = value;
            }
        }

        /// <summary>
        /// Trace object type
        /// </summary>
        public string TraceObject
        {
            get
            {
                LoadConfiguration();
                return _traceObject;
            }
            set
            {
                _traceObject = value;
            }
        }

        /// <summary>
        /// Trace file path
        /// </summary>
        public string TraceFilePath
        {
            get
            {
                LoadConfiguration();
                return _traceFilePath;
            }
            set
            {
                _traceFilePath = value;
            }
        }

        /// <summary>
        /// Trace file name
        /// </summary>
        public string TraceFileName
        {
            get
            {
                LoadConfiguration();
                return _traceFileName;
            }
            set
            {
                _traceFileName = value;
            }
        }

        /// <summary>
        /// Frequency in seconds when the rule data is refreshed within the rule engine
        /// </summary>
        public int RuleInterval
        {
            get
            {
                LoadConfiguration();
                return _ruleInterval;
            }
            set
            {
                _ruleInterval = value;
            }
        }

        /// <summary>
        /// Interval, in minutes, after a query starts executing when WMS starts evaluating the execution rules
        /// </summary>
        public int RuleIntervalQueryExecTime
        {
            get
            {
                LoadConfiguration();
                return _ruleIntervalQueryExecTime;
            }
            set { _ruleIntervalQueryExecTime = value; }
        }

        /// <summary>
        /// MAX_EXEC_QUERIES is a threshold for better using the system resources. 
        /// It’s the maximum number of executing queries allowed in the instance. 
        /// When the threshold used by specific service is reached, new queries will be sent to the waiting queue. 
        /// When number of current executing queries in the instance becomes lower than the threshold value, the waiting queries will be released for execution.
        /// </summary>
        public int MaxExecQueries
        {
            get
            {
                LoadConfiguration();
                return _maxExecQueries;
            }
            set { _maxExecQueries = value; }
        }

        /// <summary>
        /// MAX_ESPS is a threshold for better using the system resources. 
        /// It’s the average number of ESPs in the instance. 
        /// When the threshold used by specific service is reached, new queries will be sent to the waiting queue. 
        /// When average number of ESPs in the instance becomes lower than the threshold value, the waiting queries will be released for execution.
        /// </summary>
        public int MaxEsps
        {
            get
            {
                LoadConfiguration();
                return _maxEsps;
            }
            set { _maxEsps = value; }
        }

        /// <summary>
        /// How often the Canary Query will be run
        /// </summary>
        public int CanaryIntervalMinutes
        {
            get
            {
                LoadConfiguration();
                return _canaryIntervalMinutes;
            }
            set { _canaryIntervalMinutes = value; }
        }

        /// <summary>
        /// The expected time the Canary Query should return in
        /// </summary>
        public int CanaryExecSeconds
        {
            get
            {
                LoadConfiguration();
                return _canaryExecSeconds;
            }
            set { _canaryExecSeconds = value; }
        }

        /// <summary>
        /// The SQL command text of the very light weight query
        /// </summary>
        public string CanaryQuery
        {
            get
            {
                LoadConfiguration();
                return _canaryQuery;
            }
            set { _canaryQuery = value; }
        }

        public int MaxTransactionRollbackMinutes
        {
            get
            {
                LoadConfiguration();
                return _maxTransactionRollbackMinutes;
            }
            set { _maxTransactionRollbackMinutes = value; }
        }

        public string CancelQueryIfClientDisappears
        {
            get
            {
                LoadConfiguration();
                return _cancelQueryIfClientDisappears;
            }
            set { _cancelQueryIfClientDisappears = value; }
        }

        public string CheckQueryEstimatedResourceUse
        {
            get
            {
                LoadConfiguration();
                return _checkQueryEstimatedResourceUse;
            }
            set { _checkQueryEstimatedResourceUse = value; }
        }

        public int CanaryTimeoutSeconds
        {
            get
            {
                LoadConfiguration();
                return _canaryTimeoutSeconds;
            }
            set { _canaryTimeoutSeconds = value; }
        }

        /// <summary>
        /// Execution timeout in minutes. System level defaults inherited by service is added.
        /// </summary>
        public int ExecTimeout
        {
            get
            {
                LoadConfiguration();
                return _execTimeout;
            }
            set
            {
                _execTimeout = value;
            }
        }

        /// <summary>
        /// Wait timeout in minutes. System level defaults inherited by service is added.
        /// </summary>
        public int WaitTimeout
        {
            get
            {
                LoadConfiguration();
                return _waitTimeout;
            }
            set
            {
                _waitTimeout = value;
            }
        }

        /// <summary>
        /// Hold timeout in minutes. System level defaults inherited by service is added.
        /// </summary>
        public int HoldTimeout
        {
            get
            {
                LoadConfiguration();
                return _holdTimeout;
            }
            set
            {
                _holdTimeout = value;
            }
        }

        /// <summary>
        /// Maximum rows fetched. System level defaults inherited by service is added. 
        /// </summary>
        public Int64 MaxRowsFetched
        {
            get
            {
                LoadConfiguration();
                return _maxRowsFetched;
            }
            set
            {
                _maxRowsFetched = value;
            }
        }

        /// <summary>
        /// TRACE clause for the ALTER WMS command
        /// </summary>
        internal string TraceOptionSqlClauseString
        {
            get
            {
                return ""; //Trace options hidden for Trafodion

                //if (!ConnectionDefinition.IsWmsAdminRole)
                //{
                //    return "";
                //}
                //else
                //{

                //    if (String.IsNullOrEmpty(TraceObject))
                //    {
                //        return "";
                //    }
                //    else
                //    {
                //        string traceOption = string.Format("{0}{1}{2}",
                //            String.IsNullOrEmpty(TraceObject) ? "" : TraceObject,
                //            String.IsNullOrEmpty(TraceFilePath) ? "" : String.Format(", FILEPATH \"{0}\"", TraceFilePath),
                //            String.IsNullOrEmpty(TraceFileName) ? "" : ", FILENAME " + TraceFileName);

                //        return ((0 >= traceOption.Trim().Length) ? "" : ", TRACE (" + traceOption + ")");
                //    }
                //}
            }
        }


        /// <summary>
        /// Checks if SQL Defaults on a service level are enabled for all WMS Admin users.
        /// </summary>
        public bool AdminCanSetSQLDefaultsOnService
        {
            get { return this._sqlDefaultsEnabledForAdmins; }

        }  /*  End  of  Property  AdminCanSetSQLDefaultsOnService.  */

        /// <summary>
        /// List of WMS services in this system
        /// </summary>
        public List<WmsService> WmsServices
        {
            get
            {
                if (_wmsServices == null)
                {
                    LoadServices();
                }
                return _wmsServices;
            }
            set
            {
                _wmsServices = value;
            }
        }

        /// <summary>
        /// Returns the list of WMS Services that are currently in ACTIVE state
        /// </summary>
        public List<WmsService> ActiveWmsServices
        {
            get
            {
                return WmsServices.FindAll(IsActiveService);
            }
        }

        /// <summary>
        /// Returns the list of WMS Services that are currently execeeding thresholds
        /// </summary>
        public List<WmsService> ServicesOverThreshold
        {
            get
            {
                return WmsServices.FindAll(IsServiceExceedingThreshold);
            }
        }

        /// <summary>
        /// Checks if a service is in ACTIVE state
        /// </summary>
        /// <param name="service"></param>
        /// <returns></returns>
        private bool IsActiveService(WmsService service)
        {
            if (service.State.Equals(WmsService.ACTIVE, StringComparison.CurrentCultureIgnoreCase))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Helper method to determine if service is exceeding threshold
        /// </summary>
        /// <param name="service"></param>
        /// <returns></returns>
        private bool IsServiceExceedingThreshold(WmsService service)
        {
            int serviceBusy = -1;
            try
            {
                serviceBusy = int.Parse(service.MaxCpuBusy);
            }
            catch (Exception ex)
            {
                serviceBusy = -1;
            }

            if ((serviceBusy > 0 && serviceBusy < this.CpuBusy) ||
                (service.MaxMemUsage > 0 && service.MaxMemUsage < this.MemUsage)||
                (service.MaxSSDUsage > 0 && service.MaxSSDUsage < this.SSDUsage))
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        public bool IsRulesLoaded
        {
            get { return _wmsRules != null; }
        }

        public bool IsServicesLoaded
        {
            get { return _wmsServices != null; }
        }

        /// <summary>
        /// List of WMS rules in this system
        /// </summary>
        public List<WmsRule> WmsRules
        {
            get
            {
                if (_wmsRules == null)
                {
                    LoadRules();
                }
                return _wmsRules;
            }
            set
            {
                _wmsRules = value;
            }
        }

        /// <summary>
        /// List of Connection rules in this system
        /// </summary>
        public List<WmsRule> WmsConnectionRules
        {
            get
            {
                List<WmsRule> connectionRules = new List<WmsRule>();
                foreach (WmsRule rule in WmsRules)
                {
                    if (rule.RuleType == WmsCommand.CONN_RULE_TYPE)
                        connectionRules.Add(rule);
                }
                return connectionRules;
            }
        }
        /// <summary>
        /// List of compilation rules in this system
        /// </summary>
        public List<WmsRule> WmsCompilationRules
        {
            get
            {
                List<WmsRule> compilationRules = new List<WmsRule>();
                foreach (WmsRule rule in WmsRules)
                {
                    if (rule.RuleType == WmsCommand.COMP_RULE_TYPE)
                        compilationRules.Add(rule);
                }
                return compilationRules;
            }
        }
        /// <summary>
        /// List of execution rules in this system
        /// </summary>
        public List<WmsRule> WmsExecutionRules
        {
            get
            {
                List<WmsRule> executionRules = new List<WmsRule>();
                foreach (WmsRule rule in WmsRules)
                {
                    if (rule.RuleType == WmsCommand.EXEC_RULE_TYPE)
                        executionRules.Add(rule);
                }
                return executionRules;
            }
        }

        /// <summary>
        /// List of WMS admin roles in this system
        /// </summary>
        public List<WmsAdminRole> WmsAdminRoles
        {
            get
            {
                if (_wmsAdminRoles == null)
                {
                    LoadAdminRoles();
                }
                return _wmsAdminRoles;
            }
            set
            {
                _wmsAdminRoles = value;
            }
        }

        public bool NeedDefaultRuleAssociations
        {
            get
            {
                //return ConnectionDefinition.ServerVersion == ConnectionDefinition.SERVER_VERSION.R24;
                return false;
            }
        }

        public string AssociateConnectionRuleString
        {
            get
            {
                string associationString = "";
                bool prefixComma = false;
                bool atLeastOneAssociation = false;
                bool wmsNeedsDefaultRuleAssoc = NeedDefaultRuleAssociations;
                if (ConnectionRuleAssociations.Count > 0)
                {
                    associationString += "ALTER WMS CONN ( ";
                    for (int i = 0; i < ConnectionRuleAssociations.Count; i++)
                    {
                        if (!wmsNeedsDefaultRuleAssoc && isASystemService(ConnectionRuleAssociations[i].serviceName))
                            continue;

                        // After the first time in here, prefix a comma always.
                        if (prefixComma)
                            associationString += ", ";
                        else
                            prefixComma = true;

                        associationString += ConnectionRuleAssociations[i].ruleName + " " +
                                             ConnectionRuleAssociations[i].serviceName;

                        atLeastOneAssociation = true;
                    }
                    associationString += " );";
                }

                return atLeastOneAssociation ? associationString : "";
            }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs a WmsSystem instance
        /// </summary>
        /// <param name="ws">The workspace for which this model is created</param>
        private WmsSystem(ConnectionDefinition aConnectionDefinition)
            :base(aConnectionDefinition)
        {
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeWmsSystems.ContainsKey(aConnectionDefinition.Name))
                _activeWmsSystems.Add(aConnectionDefinition.Name, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;

        }

        ~WmsSystem()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

        /// <summary>
        /// Finds an instance of WmsSystem for this connection definition. If a system does not exists, a new instance is created
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        public static WmsSystem FindWmsSystem(ConnectionDefinition connectionDefinition)
        {
            //If Singleton not already instantiated for the given connection definition, create one 
            WmsSystem wmsSystem = null;
            _activeWmsSystems.TryGetValue(connectionDefinition.Name, out wmsSystem);
            if (wmsSystem == null)
            {
                wmsSystem = new WmsSystem(connectionDefinition);
            }
            return wmsSystem;
        }

        /// <summary>
        /// If the connection definition has changed/removed, the static dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                //If a connection definition has been invalidated, remove the WmsSystem instance from the static dictionary
                _activeWmsSystems.Remove(aConnectionDefinition.Name);
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            }
        }
        /// <summary>
        /// Resets the model
        /// </summary>
        override public void Refresh()
        {
            Reset();
            LoadConfiguration();
        }

        /// <summary>
        /// Reads the WMS system configuration
        /// </summary>
        public void LoadConfiguration()
        {
            //If configuration already loaded, no need to refetch
            if (isConfigurationLoaded)
                return;

            GetConnection();

            try
            {
                DataTable wms_dt = WmsCommand.executeCommand("STATUS WMS", _connection.OpenOdbcConnection, 60);
                LoadConfigurationFromDataTable(wms_dt);
                isConfigurationLoaded = true;
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        public void LoadConfigurationFromDataTable(DataTable wms_dt)
        { 
            //Read the system configuration info
            if (wms_dt.Rows.Count > 0)
            {
                _state = (string)wms_dt.Rows[0][WmsCommand.COL_STATE];
                _maxCpuBusy = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_MAX_CPU_BUSY].ToString());
                _maxMemUsage = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_MAX_MEM_USAGE].ToString());
                _statsInterval = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_STATS_INTERVAL].ToString());
                _cpuBusy = Double.Parse(wms_dt.Rows[0][WmsCommand.COL_CPU_BUSY].ToString());
                _memUsage = Double.Parse(wms_dt.Rows[0][WmsCommand.COL_MEM_USAGE].ToString());
                _maxRowsFetched = Int64.Parse(wms_dt.Rows[0][WmsCommand.COL_MAX_ROWS_FETCHED].ToString());
                _execTimeout = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_EXEC_TIMEOUT].ToString());
                _waitTimeout = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_WAIT_TIMEOUT].ToString());
                _holdTimeout = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_HOLD_TIMEOUT].ToString());
                _ruleInterval = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_RULE_INTERVAL].ToString());
                _ruleIntervalQueryExecTime = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_RULE_INTERVAL_QUERY_EXEC_TIME].ToString());

                try
                {
                    //Overflow Support in Trafodion
                    _maxSSDUsage = Int32.Parse(wms_dt.Rows[0][WmsCommand.COL_MAX_SSD_USAGE].ToString());
                    _ssdUsage = Double.Parse(wms_dt.Rows[0][WmsCommand.COL_SSD_USAGE].ToString());
                }
                catch { }

                //Trace options only apply to Services users
                if (this.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
                {
                    if (wms_dt.Columns.Contains(WmsCommand.COL_TRACE_OBJECT))
                        _traceObject = wms_dt.Rows[0][WmsCommand.COL_TRACE_OBJECT].ToString();
                    if (wms_dt.Columns.Contains(WmsCommand.COL_TRACE_FILEPATH))
                        _traceFilePath = wms_dt.Rows[0][WmsCommand.COL_TRACE_FILEPATH].ToString();
                    if (wms_dt.Columns.Contains(WmsCommand.COL_TRACE_FILENAME))
                        _traceFileName = wms_dt.Rows[0][WmsCommand.COL_TRACE_FILENAME].ToString();
                }

                _maxExecQueries = (int)wms_dt.Rows[0][WmsCommand.COL_MAX_EXEC_QUERIES];
                _maxEsps = (int)wms_dt.Rows[0][WmsCommand.COL_MAX_ESPS];

                // Available for M9 or later
                if (this.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    _canaryIntervalMinutes = (int)wms_dt.Rows[0][WmsCommand.COL_CANARY_INTERVAL_MINUTES];
                    _canaryExecSeconds = (int)wms_dt.Rows[0][WmsCommand.COL_CANARY_EXEC_SECONDS];
                    _canaryQuery = (string)wms_dt.Rows[0][WmsCommand.COL_CANARY_QUERY];

                    _cancelQueryIfClientDisappears = (string)wms_dt.Rows[0][WmsCommand.COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS];
                }

                // Available for M10 or later
                if (this.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    _checkQueryEstimatedResourceUse = (string)wms_dt.Rows[0][WmsCommand.COL_CHECK_QUERY_EST_RESOURCE_USE];
                }


#warning : Below code should be restored once CANARY_TIMEOUT_SECONDS and MAX_TRANSACTION_ROLLBACK_MINUTES are ready
                //// Available for M11 or later
                //if (this.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ160)
                //{
                //    _maxTransactionRollbackMinutes = (int)wms_dt.Rows[0][WmsCommand.COL_MAX_TRANSACTION_ROLLBACK_MINUTES];
                //    _canaryTimeoutSeconds = (int)wms_dt.Rows[0][WmsCommand.COL_CANARY_TIMEOUT_SECONDS];
                //}
            }
        }

        /// <summary>
        /// Loads all the WMS services for this system
        /// </summary>
        /// <returns></returns>
        public void LoadServices()
        {

            GetConnection();

            try
            {
                DataTable services_dt = WmsCommand.executeCommand("STATUS SERVICE", _connection.OpenOdbcConnection, 60);
                LoadServices(services_dt);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        public void LoadServices(DataTable services_dt)
        {

            try
            {
                if ((null != services_dt) && services_dt.Columns.Contains(WmsCommand.COL_SQL_DEFAULTS))
                    this._sqlDefaultsEnabledForAdmins = true;

            }
            catch (Exception)
            {
            }

            //Reset the list
            _wmsServices = new List<WmsService>();
            foreach (DataRow row in services_dt.Rows)
            {
                //Construct the model for the WMS service
                WmsService wmsService = new WmsService(this);
                //Set the service attributes
                wmsService.SetAttributes(row);
                //Add to the services list
                _wmsServices.Add(wmsService);
            }

            //Sort the list using service name
            _wmsServices.Sort();
        }

        /// <summary>
        /// Loads all the WMS rules for this system
        /// </summary>
        /// <returns></returns>
        private void LoadRules()
        {           
            //if (ConnectionDefinition.ServerVersion == ConnectionDefinition.SERVER_VERSION.R23)
            //    return;

            GetConnection();

            try
            {
                DataTable _rules_dt = WmsCommand.executeCommand("STATUS RULE", _connection.OpenOdbcConnection, 60);
                LoadRules(ConsolidateRowsForRule(_rules_dt));
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }
        
        
        public void LoadRules(DataTable rulesDataTable) 
        {
            //Reset the list
            _wmsRules = new List<WmsRule>();

            foreach (DataRow row in rulesDataTable.Rows)
            {
                WmsRule wmsRule = new WmsRule(this);

                //Set the rule attributes
                wmsRule.SetAttributes(row);

                //Add to the rule list
                _wmsRules.Add(wmsRule);
            }

            //Sort the list using rule name
            _wmsRules.Sort();
        
        }

        /// <summary>
        /// status command returns multiple rows per rule. This method consolidates the information into single row
        /// </summary>
        /// <param name="sourceRulesTable"></param>
        /// <returns></returns>
        public DataTable ConsolidateRowsForRule(DataTable sourceRulesTable)
        {
            DataRow[] uniqueRows = sourceRulesTable.Select(string.Format("{0} <= 1", WmsCommand.COL_RULE_EXPR_PRTY));

            DataTable dataTable = sourceRulesTable.Clone();

            foreach (DataRow row in uniqueRows)
            {
                string ruleName = row[sourceRulesTable.Columns[WmsCommand.COL_RULE_NAME].Ordinal] as string;
                string ruleAction = row[sourceRulesTable.Columns[WmsCommand.COL_RULE_ACTION].Ordinal] as string;
                DataRow[] exprRows = sourceRulesTable.Select(string.Format("{0} = '{1}'", WmsCommand.COL_RULE_NAME, ruleName));
                string expression = WmsRule.ProcessRuleExpression(exprRows);
                row[sourceRulesTable.Columns[WmsCommand.COL_RULE_EXPR].Ordinal] = expression.Replace("\n", "");
                if (String.IsNullOrEmpty(ruleAction))
                {
                    row[sourceRulesTable.Columns[WmsCommand.COL_RULE_ACTION].Ordinal] = WmsCommand.NO_ACTION;
                }

                dataTable.Rows.Add(row.ItemArray);
            }
            dataTable.Columns.Remove(WmsCommand.COL_RULE_EXPR_PRTY);
            return dataTable;
        }

        /// <summary>
        /// Loads all the live queries
        /// </summary>
        /// <returns>DataTable</returns>
        public DataTable LoadQueries()
        {
            DataTable queriesDataTable = new DataTable();
            GetConnection();

            try
            {
                queriesDataTable = WmsCommand.executeCommand("STATUS QUERIES ALL MERGED", _connection.OpenOdbcConnection, 60);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
            return queriesDataTable;
        }

        /// <summary>
        /// Load connection rules and their service associations
        /// </summary>
        private void LoadConnectionRuleAssociations()
        {
            _connectionRuleAssociations = new List<ConnRuleAssociation>();

            try
            {
                GetConnection();

                DataTable _rules_dt = WmsCommand.executeCommand("STATUS WMS CONN", _connection.OpenOdbcConnection, 60);
                foreach (DataRow dataRow in _rules_dt.Rows)
                {
                    string ruleName = dataRow[WmsCommand.COL_RULE_NAME_STATUSWMSCONN] as string;
                    string serviceName = dataRow[WmsCommand.COL_SERVICE_NAME] as string;
                    if (!String.IsNullOrEmpty(serviceName))
                    {
                        _connectionRuleAssociations.Add(new ConnRuleAssociation(ruleName, serviceName));
                    }
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
        /// Loads all the WMS admin roles for this system
        /// </summary>
        /// <returns></returns>
        private void LoadAdminRoles()
        {
            //Reset the list
            _wmsAdminRoles = new List<WmsAdminRole>();

            //DataTable wms_dt = _ws.executeWMSCommand("STATUS ADMIN");            
            DataTable wms_dt = new DataTable();

            foreach (DataRow row in wms_dt.Rows)
            {
                //Construct the model for the WMS admin role
                WmsAdminRole wmsAdminRole = new WmsAdminRole(this);
                //Set the admin role attributes
                wmsAdminRole.SetAttributes(row);
                //Add to the admin role list
                _wmsAdminRoles.Add(wmsAdminRole);
            }

            //Sort the list using admin role name
            _wmsAdminRoles.Sort();
        }

        /// <summary>
        /// Finds a WMS service using its name
        /// </summary>
        /// <param name="aServiceName"></param>
        /// <returns></returns>
        public WmsService FindService(string aServiceName)
        {
            WmsService theWmsService = WmsServices.Find(delegate(WmsService aWmsService)
            {
                return aWmsService.Name.Equals(aServiceName);
            });

            if (theWmsService == null)
            {
                //If service cannot be found, read all the services again from database
                LoadServices();
                theWmsService = WmsServices.Find(delegate(WmsService aWmsService)
                {
                    return aWmsService.Name.Equals(aServiceName);
                });
            }

            return theWmsService;
        }

        /// <summary>
        /// Finds a WMS rule using its name
        /// </summary>
        /// <param name="aRuleName"></param>
        /// <returns></returns>
        public WmsRule FindRule(string aRuleName)
        {
            WmsRule theWmsRule = WmsRules.Find(delegate(WmsRule aWmsRule)
            {
                return aWmsRule.Name.Equals(aRuleName);
            });

            if (theWmsRule == null)
            {
                //If rule cannot be found, read all the rules again from database
                LoadRules();
                theWmsRule = WmsRules.Find(delegate(WmsRule aWmsRule)
                {
                    return aWmsRule.Name.Equals(aRuleName);
                });
            }

            return theWmsRule;
        }

        /// <summary>
        /// Finds a WMS admin role using its name
        /// </summary>
        /// <param name="anAdminRoleName"></param>
        /// <returns></returns>
        public WmsAdminRole FindAdminRole(string anAdminRoleName)
        {
            WmsAdminRole theWmsAdminRole = WmsAdminRoles.Find(delegate(WmsAdminRole anWmsAdminRole)
            {
                return anWmsAdminRole.Name.Equals(anAdminRoleName);
            });

            if (theWmsAdminRole == null)
            {
                //If admin role cannot be found, read all the admin roles again from database
                LoadAdminRoles();
                theWmsAdminRole = WmsAdminRoles.Find(delegate(WmsAdminRole anWmsAdminRole)
                {
                    return anWmsAdminRole.Name.Equals(anAdminRoleName);
                });
            }

            return theWmsAdminRole;
        }

        /// <summary>
        /// Alter the system configuration
        /// </summary>
        public void Alter()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery(AlterCommandString, _connection.OpenOdbcConnection, 60);
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
        /// Alter the WMS connection rule associations
        /// </summary>
        /// <param name="connRuleString"></param>
        public void AlterConnectionRuleAssociations(string associationString)
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
        /// Hold all services
        /// </summary>
        public void HoldAllServices()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery("HOLD SERVICE ALL", _connection.OpenOdbcConnection, 60);
                this.WmsServices = null;
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.HOLD_ALL_SERVICES, this);
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
        /// Release all services
        /// </summary>
        public void ReleaseAllServices()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery("RELEASE SERVICE ALL", _connection.OpenOdbcConnection, 60);
                this.WmsServices = null;
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.RELEASE_ALL_SERVICES, this);
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
        /// Suspends all WMS activities and puts WMS in a holding state
        /// </summary>
        public void HoldWMS()
        {
            try
            {
                GetConnection();
                WmsCommand.executeNonQuery("HOLD WMS", _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.HOLD_STATE;
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.HOLD_WMS, this);
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
        /// Activates all WMS activities and puts WMS in an active state
        /// </summary>
        public void ReleaseWMS()
        {
            try
            {
                WmsCommand.executeNonQuery("RELEASE WMS", _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.ACTIVE_STATE;
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.RELEASE_WMS, this);
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
        /// Stops all WMS activities and puts WMS in a stopped state
        /// </summary>
        public void StopWMS(bool isImmediateStop)
        {
            try
            {
                GetConnection();
                String stopCommand = "STOP WMS";
                if (isImmediateStop)
                    stopCommand += " IMMEDIATE";

                WmsCommand.executeNonQuery(stopCommand, _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.STOPPED_STATE;
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.STOP_WMS, this);
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
        /// Starts all WMS activities and releases WMS from a stopped state
        /// </summary>
        public void StartWMS()
        {
            try
            {
                WmsCommand.executeNonQuery("START WMS", _connection.OpenOdbcConnection, 60);
                _state = WmsCommand.ACTIVE_STATE;
                this.OnWmsModelEvent(WmsCommand.WMS_ACTION.START_WMS, this);
            }
            finally
            {
                if (_connection != null)
                {
                    _connection.Close();
                }
            }
        }

        public void ResetConfiguration()
        {
            isConfigurationLoaded = false;
        }

        /// <summary>
        /// Reset Canary Query SQL Text
        /// </summary>
        public void ResetCanaryQuerySqlText()
        {
            try
            {
                OdbcConnection openConnection = _connection.OpenOdbcConnection;

                // Send RESET command
                WmsCommand.executeNonQuery("ALTER WMS CANARY_QUERY RESET", openConnection, 60);

                // Load new value after RESET
                DataTable dtWmsStatus = WmsCommand.executeCommand("STATUS WMS", openConnection, 60);
                this._canaryQuery = (string)dtWmsStatus.Rows[0][WmsCommand.COL_CANARY_QUERY];
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
        /// Reset the model state
        /// </summary>
        public void Reset()
        {
            if (_wmsServices != null)
            {
                _wmsServices.Clear();
                _wmsServices = null;
            }
            if (_wmsRules != null)
            {
                _wmsRules.Clear();
                _wmsRules = null;
            }
            if (_wmsAdminRoles != null)
            {
                _wmsAdminRoles.Clear();
                _wmsAdminRoles = null;
            }
            if (_connectionRuleAssociations != null)
            {
                _connectionRuleAssociations.Clear();
                _connectionRuleAssociations = null;
            }
            isConfigurationLoaded = false;
        }

        public void AddServiceToList(WmsService wmsService)
        {
            _wmsServices.Add(wmsService);
            OnWmsModelEvent(WmsCommand.WMS_ACTION.ADD_SERVICE, wmsService);
        }

        public void AddRuleToList(WmsRule wmsRule)
        {
            _wmsRules.Add(wmsRule);
            OnWmsModelEvent(WmsCommand.WMS_ACTION.ADD_RULE, wmsRule);
        }

        public void DeleteRuleFromList(WmsRule wmsRule)
        {
            _wmsRules.Remove(wmsRule);
            OnWmsModelEvent(WmsCommand.WMS_ACTION.DELETE_RULE, wmsRule);
        }

        public void AddAdminRoleToList(WmsAdminRole wmsAdminRole)
        {
            _wmsAdminRoles.Add(wmsAdminRole);
            OnWmsModelEvent(WmsCommand.WMS_ACTION.ADD_ADMIN, wmsAdminRole);
        }

        public void ResetConnectionRuleAssociations()
        {
            if (_wmsRules != null)
            {
                foreach (WmsRule rule in WmsConnectionRules)
                {
                    rule.ResetAssociatedServiceNames();
                }
            }
            if (_connectionRuleAssociations != null)
            {
                _connectionRuleAssociations.Clear();
                _connectionRuleAssociations = null;
            }
        }

        public struct ConnRuleAssociation
        {
            public string ruleName;
            public string serviceName;

            public ConnRuleAssociation(string r, string s)
            {
                ruleName = r;
                serviceName = s;
            }
        }
    }
}
