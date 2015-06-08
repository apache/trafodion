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
using System.Data.Odbc;
using System.Text;
using System.Data;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;
namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// WMS queries
    /// </summary>
    static public class Queries
    {
        #region Wms result set column names

        internal const string MAX_CPU_BUSY = "MAX_CPU_BUSY";
        internal const string MAX_MEM_USAGE = "MAX_MEM_USAGE";
        internal const string STATS_INTERVAL = "STATS_INTERVAL";
        internal const string TRACE_OBJECT = "TRACE_OBJECT";
        internal const string TRACE_FILEPATH = "TRACE_FILEPATH";
        internal const string TRACE_FILENAME = "TRACE_FILENAME";
        internal const string SERVICE_NAME = "SERVICE_NAME";
        internal const string SERVICE_PRIORITY = "SERVICE_PRIORITY";
        internal const string ACTIVE_TIME = "ACTIVE_TIME";
        internal const string SQL_PLAN = "SQL_PLAN";
        internal const string SQL_TEXT = "SQL_TEXT";
        internal const string EXEC_TIMEOUT = "EXEC_TIMEOUT";
        internal const string WAIT_TIMEOUT = "WAIT_TIMEOUT";
        internal const string SQL_DEFAULTS = "SQL_DEFAULTS";
        internal const string COMMENT = "COMMENT";
        internal const string STATE = "STATE";
        internal const string PLAN = "PLAN";
        internal const string NO_PLAN = "NO_PLAN";
        internal const string TEXT = "TEXT";
        internal const string NO_TEXT = "NO_TEXT";
        internal const string RULE_INTERVAL = "RULE_INTERVAL";
        internal const string HOLD_TIMEOUT = "HOLD_TIMEOUT";
        internal const string MAX_ROWS_FETCHED = "MAX_ROWS_FETCHED";
        internal const string TRAFODION_DEFAULT_SERVICE = "TRAFODION_DEFAULT_SERVICE";
        internal const string HPS_MANAGEABILITY = "HPS_MANAGEABILITY";
        internal const string HPS_TRANSPORTER = "HPS_TRANSPORTER";
        #endregion Wms result set column names

        private const string TRACE_SUB_AREA_NAME = "WMS Commands";

        /// <summary>
        /// Opens a WMS session
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        static public Connection WmsOpen(ConnectionDefinition connectionDefinition)
        {
            Connection connection = null;
            OdbcCommand command = null;
            try
            {
                connection = new Connection(connectionDefinition);
                command = new OdbcCommand("WMSOPEN", connection.OpenOdbcConnection);
                //int result = command.ExecuteNonQuery();
                int result = ExecuteNonQuery(command);
                return connection;
            }
            finally
            {
                if (command != null)
                {
                    command.Dispose();
                }
            }
        }

        /// <summary>
        /// Closes a WMS session
        /// </summary>
        /// <param name="aConnection"></param>
        static public void WmsClose(Connection aConnection)
        {
            OdbcCommand command = null;
            try
            {
                command = new OdbcCommand("WMSCLOSE", aConnection.OpenOdbcConnection);
                //int result = command.ExecuteNonQuery();
                int result = ExecuteNonQuery(command);
            }
            finally
            {
                if (command != null)
                {
                    command.Dispose();
                }
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
        }

        static public void ManageQuery(ConnectionDefinition aConnectionDefinition, string aQueryId, WmsCommand.QUERY_ACTION queryAction)
        {
            string operation = "";
            switch (queryAction)
            {
                case WmsCommand.QUERY_ACTION.CANCEL_QUERY:
                    operation = "CANCEL";
                    break;
                case WmsCommand.QUERY_ACTION.RELEASE_QUERY:
                    operation = "RELEASE";
                    break;
                case WmsCommand.QUERY_ACTION.HOLD_QUERY:
                    operation = "HOLD";
                    break;
            }

            ExecuteWmsNonQuery(aConnectionDefinition, string.Format("{0} QUERY {1}", operation, aQueryId));
        }

        public static DataTable ExecuteWmsReadToDataTable(ConnectionDefinition connectionDefinition, string commandText)
        {
            DataTable result = null;

            Connection connection = new Connection(connectionDefinition);
            OdbcCommand odbcCommand = null;

            try
            {
                odbcCommand = new OdbcCommand();
                odbcCommand.Connection = connection.OpenOdbcConnection;
                odbcCommand.CommandText = "WMSOPEN";
                ExecuteNonQuery(odbcCommand);

                odbcCommand.CommandText = commandText;
                using (OdbcDataReader dataReader = ExecuteReader(odbcCommand))
                {
                    result = Utilities.GetDataTableForReader(dataReader);
                }

                odbcCommand.CommandText = "WMSCLOSE";
                ExecuteNonQuery(odbcCommand);
            }
            finally
            {
                if (odbcCommand != null)
                {
                    odbcCommand.Dispose();
                }
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return result;
        }
                
        /// <summary>
        /// Fetch the status of services in a given system
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aServiceName"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteStatusService(Connection aConnection, string aServiceName)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("STATUS SERVICE {0}", aServiceName));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteReader();
            return ExecuteReader(theQuery);
        }


       
        static public OdbcDataReader ExecuteReorgProgressSummaryInfo(Connection aConnection, string qid)
        {
            StringBuilder sbCommand = new StringBuilder();
            sbCommand.Append(" SELECT	cast(tokenstr('TOTALTABLES',a) as int)		    totalTables,");
            sbCommand.Append(" 	cast(tokenstr('TABLESREORGNOTNEEDED',a) as int)		    tablesReorgNotNeeded,");
            sbCommand.Append(" 	cast(tokenstr('TABLESREORGNOTSTARTED',a) as int)	    tablesReorgNotStarted,");
            sbCommand.Append(" 	cast(tokenstr('TABLESREORGRUNNING',a) as int)		    tablesReorgRunning,");
            sbCommand.Append(" 	cast(tokenstr('TABLESREORGCOMPLETED',a) as int)		    tablesReorgCompleted,");
            sbCommand.Append(" 	cast(tokenstr('TABLESREORGERROR',a) as int)		        tablesReorgError,");
            sbCommand.Append(" 	cast(tokenstr('TABLESREORGPROGRESS',a) as int)		    tablesReorgProgress,");
            sbCommand.Append(" 	cast(tokenstr('TOTALPARTITIONS',a) as int)		        totalPartitions,");
            sbCommand.Append(" 	cast(tokenstr('PARTITIONSREORGNOTNEEDED',a) as int)	    partitionsReorgNotNeeded,");
            sbCommand.Append(" 	cast(tokenstr('PARTITIONSREORGNOTSTARTED',a) as int)	partitionsReorgNotStarted,");
            sbCommand.Append(" 	cast(tokenstr('PARTITIONSREORGRUNNING',a) as int)	    partitionsReorgRunning,");
            sbCommand.Append(" 	cast(tokenstr('PARTITIONSREORGCOMPLETED',a) as int)	    partitionsReorgCompleted,");
            sbCommand.Append(" 	cast(tokenstr('PARTITIONSREORGERROR',a) as int)		    partitionsReorgError,");
            sbCommand.Append(" 	cast(tokenstr('PARTITIONSREORGPROGRESS',a) as int)	    partitionsReorgProgress");
            sbCommand.Append(" FROM (get reorg progress for qid {0},options 'tf') x(a) ");

            OdbcCommand theQuery = new OdbcCommand(
                String.Format(sbCommand.ToString(), qid));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public OdbcDataReader ExecuteReorgProgressDetailInfo(Connection aConnection, string qid)
        {
            StringBuilder sbCommand = new StringBuilder();
            sbCommand.Append(" SELECT ");
            sbCommand.Append(" cast(tokenstr('TABLENAME',a) as char(100)) 			    tableName,");
            sbCommand.Append(" cast(tokenstr('TOTALPARTITIONS',a) as int) 			    totalPartitions,");
            sbCommand.Append(" cast(tokenstr('PARTITIONSREORGNOTNEEDED',a) as int)		partitionsReorgNotNeeded,");
            sbCommand.Append(" cast(tokenstr('PARTITIONSREORGNOTSTARTED',a) as int) 	partitionsReorgNotStarted,");
            sbCommand.Append(" cast(tokenstr('PARTITIONSREORGRUNNING',a) as int)	 	partitionsReorgRunning,");
            sbCommand.Append(" cast(tokenstr('PARTITIONSREORGCOMPLETED',a) as int) 		partitionsReorgCompleted,");
            sbCommand.Append(" cast(tokenstr('PARTITIONSREORGERROR',a) as int) 		    partitionsReorgError,");
            sbCommand.Append(" cast(tokenstr('PARTITIONSREORGPROGRESS',a) as int) 		partitionsReorgProgress");
            sbCommand.Append(" FROM (get reorg progress for qid {0},options 'tf dt') x(a)");

            OdbcCommand theQuery = new OdbcCommand(
                String.Format(sbCommand.ToString(), qid));
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        /// <summary>
        /// Starts a service
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aWmsService"></param>
        /// <returns></returns>
        static public int ExecuteStartService(Connection aConnection, WmsService aWmsService)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("START SERVICE {0}",
                aWmsService.Name));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Stops a service
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aWmsService"></param>
        /// <returns></returns>
        static public int ExecuteStopService(Connection aConnection, WmsService aWmsService)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("STOP SERVICE {0}",
                aWmsService.Name));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }
        
        /// <summary>
        /// Deletes a service
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aWmsService"></param>
        /// <param name="isImmediate"></param>
        /// <returns></returns>
        static public int ExecuteDeleteService(Connection aConnection, WmsService aWmsService, bool isImmediate)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("DELETE SERVICE {0} {1}", 
                aWmsService.Name,
                isImmediate ? "IMMEDIATE" : ""));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Place the Service on hold
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aWmsService"></param>
        /// <returns></returns>
        static public int ExecuteHoldService(Connection aConnection, WmsService aWmsService)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("HOLD SERVICE {0}", aWmsService.Name));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Holds all services
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aWmsSystem"></param>
        /// <returns></returns>
        static public int ExecuteHoldAllServices(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("HOLD SERVICE ALL");

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Suspends all WMS activities and puts WMS in a holding state
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public int ExecuteHoldWMS(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("HOLD WMS");

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Releases the hold on the service
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aWmsService"></param>
        /// <returns></returns>
        static public int ExecuteReleaseService(Connection aConnection, WmsService aWmsService)
        {
            OdbcCommand theQuery = new OdbcCommand(String.Format("RELEASE SERVICE {0}", aWmsService.Name));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Release all services on a system
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public int ExecuteReleaseAllServices(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("RELEASE SERVICE ALL");

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }        

        /// <summary>
        /// Release WMS activities and puts service(s) in an active state
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public int ExecuteReleaseWMS(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("RELEASE WMS");

            theQuery.Connection = aConnection.OpenOdbcConnection;
            //return theQuery.ExecuteNonQuery();
            return ExecuteNonQuery(theQuery);
        }

        static public int ExecuteWmsNonQuery(ConnectionDefinition aConnectionDefinition, string anWmsCommand)
        {
            Connection aConnection = new Connection(aConnectionDefinition);
            OdbcCommand odbcCommand = null;
            int result = 0;
            try
            {
                odbcCommand = new OdbcCommand();
                odbcCommand.Connection = aConnection.OpenOdbcConnection;
                odbcCommand.CommandText = "WMSOPEN";
                ExecuteNonQuery(odbcCommand);

                odbcCommand.CommandText = anWmsCommand;
                result = ExecuteNonQuery(odbcCommand);

                odbcCommand.CommandText = "WMSCLOSE";
                ExecuteNonQuery(odbcCommand);
            }
            finally
            {
                if (odbcCommand != null)
                {
                    odbcCommand.Dispose();
                }
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
            return result;
        }

        static private int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.WMS, TRACE_SUB_AREA_NAME, false);
        }

        static private OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.WMS, TRACE_SUB_AREA_NAME, false);
        }
        
    }
}
