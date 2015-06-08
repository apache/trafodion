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
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Model 
{

    public class WmsCommand
    {
        public enum WMS_ACTION
        {
            STATUS_WMS = 11, ALTER_WMS = 12, START_WMS = 13, STOP_WMS = 14, HOLD_WMS = 15, RELEASE_WMS = 16,
            STATUS_SERVICE = 21, ADD_SERVICE = 22, ALTER_SERVICE = 23, DELETE_SERVICE = 24, START_SERVICE = 25,
            STOP_SERVICE = 26, HOLD_SERVICE = 27, RELEASE_SERVICE = 28, LIST_RULE_SERVICE = 29,
            HOLD_ALL_SERVICES = 30, RELEASE_ALL_SERVICES = 31,
            STATUS_RULE = 41, ADD_RULE = 42, ALTER_RULE = 43, DELETE_RULE = 44, RULE_ASSOC_CHANGED = 46,
            STATUS_ADMIN = 51, ADD_ADMIN = 52, ALTER_ADMIN = 53, DELETE_ADMIN = 54
        };

        public enum QUERY_ACTION
        {
            CANCEL_QUERY = 1, HOLD_QUERY = 2, RELEASE_QUERY = 3
        }

        public enum WMS_PRIVILEGE
        {
            ADMIN_ADD,
            ADMIN_ALTER,
            ADMIN_DELETE,
            ADMIN_STATUS,
            ADMIN_STOP,
            ADMIN_START,
            ADMIN_CANCEL,
            ADMIN_HOLD,
            ADMIN_RELEASE,
            ADMIN_REPREPARE,
            ADMIN_LOAD_QUERY,
            ADMIN_EXECUTE_QUERY,
            ADMIN_RESTART
        }


        #region Wms result set column names

        internal const String COL_QUERY_ID = "QUERY_ID";
        internal const String COL_QUERY_STATE = "QUERY_STATE";
        internal const String COL_QUERY_SUBSTATE = "QUERY_SUBSTATE";
        internal const String COL_QUERY_TEXT = "QUERY_TEXT";
        internal const String COL_START_TS = "START_TS";
        internal const String COL_QUERY_START_TIME = "QUERY_START_TIME";
        internal const String COL_MAX_CPU_BUSY = "MAX_CPU_BUSY";
        internal const String COL_MAX_MEM_USAGE = "MAX_MEM_USAGE";
        internal const String COL_MAX_SSD_USAGE = "MAX_SSD_USAGE";
        internal const String COL_SSD_STATE = "SSD_STATE";
        internal const String COL_CPU_BUSY = "CPU_BUSY";
        internal const String COL_MEM_USAGE = "MEM_USAGE";
        internal const String COL_SSD_USAGE = "SSD_USAGE";
        internal const String COL_STATS_INTERVAL = "STATS_INTERVAL";
        internal const String COL_TRACE_OBJECT = "TRACE_OBJECT";
        internal const String COL_TRACE_FILEPATH = "TRACE_FILEPATH";
        internal const String COL_TRACE_FILENAME = "TRACE_FILENAME";
        internal const String COL_SERVICE_NAME = "SERVICE_NAME";
        internal const String COL_SERVICE_PRIORITY = "SERVICE_PRIORITY";
        internal const String COL_ACTIVE_TIME = "ACTIVE_TIME";
        internal const String COL_SQL_PLAN = "SQL_PLAN";
        internal const String COL_SQL_TEXT = "SQL_TEXT";
        internal const String COL_EXEC_TIMEOUT = "EXEC_TIMEOUT";
        internal const String COL_WAIT_TIMEOUT = "WAIT_TIMEOUT";
        internal const String COL_SQL_DEFAULTS = "SQL_DEFAULTS";
        internal const String COL_MAX_TRANSACTION_ROLLBACK_MINUTES = "MAX_TRANSACTION_ROLLBACK_MINUTES";
        internal const String COL_CANCEL_QUERY_IF_CLIENT_DISAPPEARS = "CANCEL_QUERY_IF_CLIENT_DISAPPEARS";
        internal const String COL_CHECK_QUERY_EST_RESOURCE_USE = "CHECK_QUERY_EST_RESOURCE_USE";
        internal const String COL_COMMENT = "COMMENT";
        internal const String COL_STATE = "STATE";
        internal const String COL_RULE_INTERVAL = "RULE_INTERVAL";
        internal const String COL_RULE_INTERVAL_QUERY_EXEC_TIME = "RULE_INTERVAL_QUERY_EXEC_TIME";
        internal const String COL_HOLD_TIMEOUT = "HOLD_TIMEOUT";
        internal const String COL_MAX_ROWS_FETCHED = "MAX_ROWS_FETCHED";
        internal const String COL_RULE_NAME = "NAME";
        internal const String COL_RULE_NAME_STATUSWMSCONN = "RULE_NAME";
        internal const String COL_RULE_OPER = "OPER";
        internal const String COL_RULE_EXPR = "EXPR";
        internal const String COL_RULE_EXPR_PRTY = "EXPR_PRTY";
        internal const String COL_RULE_TYPE = "TYPE";
        internal const String COL_RULE_WARN_LEVEL = "WARN_LEVEL";
        internal const String COL_RULE_ACTION = "ACT"; 
        internal const String COL_ADMIN_ROLE_NAME = "USER_ROLE";
        internal const String COL_RULE_AGGR_QUERY_TYPES = "AGGR_QUERY_TYPES";
        internal const String COL_RULE_AGGR_REPOS_INTERVAL = "AGGR_REPOS_INTERVAL";
        internal const String COL_RULE_AGGR_WMS_INTERVAL = "AGGR_WMS_INTERVAL";
        internal const String COL_RULE_AGGR_EXEC_INTERVAL = "AGGR_EXEC_INTERVAL";
        internal const String COL_RULE_AGGR_STATS_ONCE = "AGGR_STATS_ONCE";

        internal const String COL_MAX_EXEC_QUERIES = "MAX_EXEC_QUERIES";
        internal const String COL_MAX_ESPS = "MAX_AVG_ESPS";
        internal const String COL_CANARY_INTERVAL_MINUTES = "CANARY_INTERVAL_MINUTES";
        internal const String COL_CANARY_EXEC_SECONDS = "CANARY_EXEC_SECONDS";
        internal const String COL_CANARY_TIMEOUT_SECONDS = "CANARY_TIMEOUT_SECONDS";
        internal const String COL_CANARY_QUERY = "CANARY_QUERY";

        #endregion Wms result set column names

        internal const string PLAN = "PLAN";
        internal const string NO_PLAN = "NO_PLAN";
        internal const string TEXT = "TEXT";
        internal const string NO_TEXT = "NO_TEXT";
        internal const string ACTIVE_STATE = "ACTIVE";
        internal const string HOLD_STATE = "HOLD";
        internal const string TRANSIENT_STATE = "TRANSIENT";
        internal const string STOPPED_STATE = "STOPPED";
        internal const string CONN_RULE_TYPE = "CONN";
        internal const string EXEC_RULE_TYPE = "EXEC";
        internal const string COMP_RULE_TYPE = "COMP";
        internal const string EST_MAX_CPU_BUSY = "EST_MAX_CPU_BUSY";
        internal const string QUERY_STATE_HOLDING = "HOLDING";
        internal const string QUERY_STATE_WAITING = "WAITING";
        internal const string QUERY_STATE_COMPLETED = "COMPLETED";
        internal const string QUERY_STATE_SUSPENDED = "SUSPENDED";
        internal const string QUERY_STATE_REJECTED = "REJECTED";
        internal const string QUERY_STATE_EXECUTING = "EXECUTING";
        internal const string SWITCH_ON = "ON";
        internal const string SWITCH_OFF = "OFF";

        public const double DEFAULT_MAX_CPU_BUSY = 100.0;
		public const double DEFAULT_MAX_MEMORY_USAGE = 85.0;
        public const string NO_ACTION = "NO-ACTION";
        private const string TRACE_SUB_AREA_NAME = "WMS Commands";

        public static String escapeServiceName(String serviceName) {
            if (serviceName.StartsWith("\"")  &&  serviceName.EndsWith("\"") )
                return serviceName;

            return "\"" + serviceName + "\"";
        }

        /// <summary>
        /// Returns a user friendly display string for the internal rule type
        /// </summary>
        /// <param name="ruleType"></param>
        /// <returns></returns>
        public static string GetDisplayRuleType(string ruleType)
        {
            string displayType = "";

            if (!string.IsNullOrEmpty(ruleType))
            {
                switch (ruleType)
                {
                    case WmsCommand.CONN_RULE_TYPE:
                        displayType = "Connection";
                        break;
                    case WmsCommand.EXEC_RULE_TYPE:
                        displayType = "Execution";
                        break;
                    case WmsCommand.COMP_RULE_TYPE:
                        displayType = "Compilation";
                        break;
                }
            }
            return displayType;
        }

        /// <summary>
        /// Executes a WMS command
        /// </summary>
        /// <param name="wmsCmdStr">The WMS command string</param>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <returns></returns>
        public static DataTable executeCommand(string wmsCmdStr, ConnectionDefinition aConnectionDefinition)
        {
            Connection aConnection = new Connection(aConnectionDefinition);

            try
            {
                return executeCommand(wmsCmdStr, aConnection.OpenOdbcConnection, GeneralOptions.GetOptions().CommandTimeOut);
            }
            finally
            {
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
        }

        public static DataTable executeCommand(String wmsCmdStr, OdbcConnection dbConn, int cmdTimeout)
        {
            if ((null == wmsCmdStr) || (0 >= wmsCmdStr.Length))
            {
                throw new Exception("Invalid WMS command specified [null or empty string]");
            }

            DataTable wms_dt = new DataTable();
            bool needToExecWMSClose = false;

            try
            {
                OdbcCommand theCommand = new OdbcCommand("WMSOPEN", dbConn);
                theCommand.CommandTimeout = cmdTimeout;

                OdbcCommand wmsCloseCmd = new OdbcCommand("WMSCLOSE", dbConn);
                wmsCloseCmd.CommandTimeout = cmdTimeout;

                needToExecWMSClose = true;
                OdbcAccess.executeQuery(theCommand, -1);

                theCommand.CommandText = wmsCmdStr;
                wms_dt = OdbcAccess.executeQuery(theCommand, -1);

                theCommand.Dispose();

                needToExecWMSClose = false;
                OdbcAccess.executeQuery(wmsCloseCmd, -1);
                wmsCloseCmd.Dispose();
            }
            catch (Exception e)
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.WMS,
                        TRACE_SUB_AREA_NAME,
                        "NWMS error executing command." + Environment.NewLine +
                                                "     " + wmsCmdStr + Environment.NewLine +
                                                "      Details = " + e.Message);
                }

                if (needToExecWMSClose)
                {
                    try
                    {
                        OdbcCommand cmd = new OdbcCommand("WMSCLOSE", dbConn);
                        cmd.CommandTimeout = cmdTimeout;
                        OdbcAccess.executeQuery(cmd, -1);
                    }
                    catch (Exception)
                    {
                        //  Ignore any WMSCLOSE errors.
                    }
                }

                throw;
            }


            return wms_dt;

        }  /*  End of  executeCommand  method.  */

        public static int executeNonQuery(string wmsCmdStr, ConnectionDefinition aConnectionDefinition)
        {
            Connection connection = new Connection(aConnectionDefinition);
            int result = 0;
            try
            {
                result = executeNonQuery(wmsCmdStr, connection.OpenOdbcConnection, 60);
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }
            return result;
        }

        public static int executeNonQuery(String wmsCmdStr, OdbcConnection dbConn, int cmdTimeout)
        {
            if ((null == wmsCmdStr) || (0 >= wmsCmdStr.Length))
                throw new Exception("Invalid WMS command specified [null or empty string]");

            bool needToExecWMSClose = false;
            int result = 0;

            try
            {
                OdbcCommand theCommand = new OdbcCommand("WMSOPEN", dbConn);
                theCommand.CommandTimeout = cmdTimeout;

                OdbcCommand wmsCloseCmd = new OdbcCommand("WMSCLOSE", dbConn);
                wmsCloseCmd.CommandTimeout = cmdTimeout;

                needToExecWMSClose = true;

                OdbcAccess.executeQuery(theCommand, -1);

                theCommand.CommandText = wmsCmdStr;
                result = Utilities.ExecuteNonQuery(theCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.WMS, TRACE_SUB_AREA_NAME, true);
                theCommand.Dispose();

                needToExecWMSClose = false;

                OdbcAccess.executeQuery(wmsCloseCmd, -1);
                wmsCloseCmd.Dispose();

            }
            catch (Exception e)
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.WMS,
                        TRACE_SUB_AREA_NAME,
                        "NWMS error executing command." + Environment.NewLine +
                                                "     " + wmsCmdStr + Environment.NewLine +
                                                "      Details = " + e.Message);
                }

                if (needToExecWMSClose)
                {
                    try
                    {
                        OdbcCommand cmd = new OdbcCommand("WMSCLOSE", dbConn);
                        cmd.CommandTimeout = cmdTimeout;
                        OdbcAccess.executeQuery(cmd, -1);

                    }
                    catch (Exception)
                    {
                        //  Ignore any WMSCLOSE errors.
                    }
                }

                throw;
            }

            return result;
        }  /*  End of  executeCommand  method.  */
	}
}
