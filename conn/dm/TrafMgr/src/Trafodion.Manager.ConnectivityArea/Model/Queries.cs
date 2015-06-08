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
using System.Linq;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// NDCS queries
    /// </summary>
    static public class Queries
    {

        #region NDCS configuration and management API constants

        // To start open/close a NDCS command session.
        internal const string CMD_OPEN      = "CMDOPEN";
        internal const string CMD_CLOSE     = "CMDCLOSE";

        // NDCS supported commands
        internal const string CMD_INFO      = "INFO";
        internal const string CMD_LIST      = "LIST";
        internal const string CMD_ADD       = "ADD";
        internal const string CMD_ALTER     = "ALTER";
        internal const string CMD_DELETE    = "DELETE";
        internal const string CMD_START     = "START";
        internal const string CMD_STATUS    = "STATUS";
        internal const string CMD_STOP      = "STOP";

        // NDCS supported object types
        internal const string OBJ_DS        = "DS";
        internal const string OBJ_SERVICE   = "SERVICE";
        internal const string OBJ_EVAR      = "EVAR";
        internal const string OBJ_RESOURCE  = "RESOURCE";
        internal const string OBJ_SERVER = "SERVER";
        internal const string OBJ_USER = "USER";

        // NDCS Command attributes
        internal const string MAX_SERVER    = "MAXSERVER";
        internal const string AVAIL_SERVER  = "AVAILSERVER";
        internal const string INIT_SERVER   = "INITSERVER";
        internal const string START_AHEAD   = "STARTAHEAD";
        internal const string SERVER_IDLE_TIMEOUT = "SRVRIDLETIMEOUT";
        internal const string CONNECT_TIMEOUT  = "CONNTIMEOUT";
        internal const string AUTO_START    = "AUTOSTART";
        internal const string TRACE         = "TRACE";
        internal const string ON            = "ON";
        internal const string OFF           = "OFF";
        internal const string CPU_LIST      = "CPULIST";
        internal const string PRIORITY      = "PRIORITY";
        internal const string SYSTEM_DEFAULT = "SYSTEM_DEFAULT";
        internal const string LIMIT_VALUE   = "LIMITVALUE";
        internal const string ACTION        = "ACTION";

        internal const string ALL_STAT = "ALLSTAT";
        internal const string SQL_PREPARE_STAT = "SQLPREPARESTAT";
        internal const string SQL_EXECUTE_STAT = "SQLEXECUTESTAT";
        internal const string SQL_EXECDIRECT_STAT = "SQLEXECDIRECTSTAT";
        internal const string SQL_FETCH_STAT = "SQLFETCHSTAT";
        internal const string SQL_STMT_STAT = "SQLSTMTSTAT";
        internal const string SESSION_INFO_STAT = "SESSIONINFOSTAT";
        internal const string CONNECT_INFO_STAT = "CONNINFOSTAT";

        internal const string DS_NAME       = "DSNAME";
        internal const string VALUE         = "VALUE";
        internal const string STOP_MODE     = "STOPMODE";
        internal const string IMMEDIATE     = "IMMEDIATE";
        internal const string DISCONNECT    = "DISCONNECT";
        internal const string REASON        = "REASON";
        internal const string DIAG_ID       = "DIAGID";
        internal const string SERVICE = "SERVICE";
        internal const string PERMISSION = "PERMISSION";

        // EVAR types
        /// <summary>
        /// ENV_TYPE_RESOURCE type
        /// </summary>
        public const int ENV_TYPE_RESOURCE = -1;
        /// <summary>
        /// ENV_TYPE_SET type
        /// </summary>
        public const int ENV_TYPE_SET = 0;
        /// <summary>
        /// ENV_TYPE_CONTROL type
        /// </summary>
        public const int ENV_TYPE_CONTROL = 1;
        /// <summary>
        /// ENV_TYPE_DEFINE type
        /// </summary>
        public const int ENV_TYPE_DEFINE = 2;
        /// <summary>
        /// ENV_TYPE_STATISTICS type
        /// </summary>
        public const int ENV_TYPE_STATISTICS = 3;
        /// <summary>
        /// ENV_TYPE_CPULIST type
        /// </summary>
        public const int ENV_TYPE_CPULIST = 4;
        /// <summary>
        /// ENV_TYPE_PROCESSPRIORITY type
        /// </summary>
        public const int ENV_TYPE_PROCESSPRIORITY = 5;

        // Return for no update
        /// <summary>
        /// NO_UPDATE
        /// </summary>
        public const int NO_UPDATE = 0;

        private const string TRACE_SUB_AREA_NAME = "NDCS Commands";

        #endregion NDCS configuration and management API constants

        ///// <summary>
        ///// Opens a NDCS command session
        ///// </summary>
        ///// <param name="connectionDefinition"></param>
        ///// <returns>Connection</returns>
        //static private Connection NDCSOpen(ConnectionDefinition connectionDefinition)
        //{
        //    Connection connection = null;
        //    OdbcCommand command = null;
        //    try
        //    {
        //        connection = new Connection(connectionDefinition);
        //        command = new OdbcCommand(CMD_OPEN, connection.OpenOdbcConnection);
        //        int result = command.ExecuteNonQuery();
        //        return connection;
        //    }
        //    finally
        //    {
        //        if (command != null)
        //        {
        //            command.Dispose();
        //        }
        //    }
        //}


        ///// <summary>
        ///// Closes a NCDS session
        ///// </summary>
        ///// <param name="aConnection"></param>
        //static private void NCDSClose(Connection aConnection)
        //{
        //    OdbcCommand command = null;
        //    try
        //    {
        //        command = new OdbcCommand(CMD_CLOSE, aConnection.OpenOdbcConnection);
        //        int result = command.ExecuteNonQuery();
        //    }
        //    finally
        //    {
        //        if (command != null)
        //        {
        //            command.Dispose();
        //        }
        //        if (aConnection != null)
        //        {
        //            aConnection.Close();
        //        }
        //    }
        //}

        #region Public Methods
        /// <summary>
        /// Open a new command object.
        /// </summary>
        /// <param name="connection"></param>
        /// <returns>OdbcCommand</returns>
        //public static OdbcCommand OpenCommand(ConnectionDefinition connectionDefinition)
        //{
        //    Connection connection = null;
        //    OdbcCommand command = null;
        //    string existingServerDS = connectionDefinition.ServerDataSource;
        //    connectionDefinition.ServerDataSource = "Admin_Load_DataSource";
        //    connection = new Connection(connectionDefinition);
        //    command = new OdbcCommand(CMD_OPEN, connection.OpenOdbcConnection);
        //    connectionDefinition.ServerDataSource = existingServerDS;
        //    int result = ExecuteNonQuery(command);
        //    return command;
        //}

        public static OdbcCommand OpenCommand(Connection connection)
        {
            if (connection != null)
            {
                OdbcCommand command = new OdbcCommand(CMD_OPEN, connection.OpenAdminOdbcConnection);
                int result = ExecuteNonQuery(command);
                return command;
            }
            return null;
        }

        /// <summary>
        /// Close the current command object.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        public static void CloseCommand(OdbcCommand command)
        {
            try
            {
                if (command != null)
                {
                    command.CommandText = CMD_CLOSE;
                    OdbcConnection connection = command.Connection;
                    try
                    {
                        ExecuteNonQuery(command);
                        command.Dispose();
                    }
                    finally
                    {
                        if (connection != null)
                        {
                            connection.Close();
                        }
                    }
                }
            }
            catch (Exception)
            {
                //do nothing
            }
        }
        /// <summary>
        /// Close the current command object.
        /// </summary>
        /// <param name="aConnection"></param>
        public static void CloseConnection(Connection aConnection)
        {
            try
            {
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
            catch (Exception)
            {
                //do nothing
            }
        }

        /// <summary>
        /// Get all the services
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteListService(OdbcCommand anOpenCommand)
        {
            anOpenCommand.CommandText = "LIST SERVICE";
            return ExecuteReader(anOpenCommand); 
        }

        /// <summary>
        /// Get all the NDCS users
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteInfoUsers(OdbcCommand anOpenCommand)
        {
            anOpenCommand.CommandText = String.Format("{0} {1}", CMD_INFO, OBJ_USER);
            OdbcDataReader retReader = ExecuteReader(anOpenCommand);
            return retReader;
        }

        /// <summary>
        /// To Add an NDCS privilege
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="ndcsUser"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAddUser(OdbcCommand anOpenCommand, NDCSUser ndcsUser)
        {
            StringBuilder command = new StringBuilder();
            command.AppendFormat("{0} {1} {2}, PERMISSION {3}", CMD_ADD, OBJ_USER, ndcsUser.DelimitedName, ndcsUser.DelimitedType);

            anOpenCommand.CommandText = command.ToString();
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// To alter a User
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="ndcsUser"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAlterUser(OdbcCommand anOpenCommand, NDCSUser ndcsUser)
        {
            StringBuilder command = new StringBuilder();
            command.AppendFormat("{0} {1} {2}, PERMISSION {3}", CMD_ALTER, OBJ_USER, ndcsUser.DelimitedName, ndcsUser.DelimitedType);

            anOpenCommand.CommandText = command.ToString();
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// To Delete a user
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="ndcsUser"></param>
        /// <returns></returns>
        static public int ExecuteDeleteUser(OdbcCommand anOpenCommand, NDCSUser ndcsUser)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} {2}",
                        CMD_DELETE, OBJ_USER, ndcsUser.Name
                        );
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// Get all the DataSources
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteListDS(OdbcCommand anOpenCommand)
        {
            anOpenCommand.CommandText = String.Format("{0} {1}", CMD_LIST, OBJ_DS);
            OdbcDataReader retReader = ExecuteReader(anOpenCommand);
            return retReader;
        }

        /// <summary>
        /// Fetch the basic information for the given Data source
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSourceName"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteInfoDataSource(OdbcCommand anOpenCommand, string aDataSourceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} {2}", CMD_INFO, OBJ_DS, aDataSourceName);
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// Fetch a specific Environment type values for a DataSource
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aEnvironmentType"></param>
        /// <param name="aDataSourceName"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteInfoEnvironment(
           OdbcCommand anOpenCommand, int aEnvironmentType, string aDataSourceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} {2} {3}",
                                    CMD_INFO, 
                                    OBJ_EVAR, 
                                    aEnvironmentType, 
                                    String.Format(", {0} {1}", DS_NAME, aDataSourceName));
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// Fetch the Status for a NDCS Service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteStatusService(OdbcCommand anOpenCommand,
            string aServiceName)
        {   
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\"",
                        CMD_STATUS, OBJ_SERVICE, aServiceName);
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// Fetch the Status for a DataSource for a specific NDCS Service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <param name="aDataSourceName"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteStatusDataSource(OdbcCommand anOpenCommand,  
            string aServiceName, string aDataSourceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\", {3}",
                        CMD_STATUS, OBJ_DS, aServiceName, 
                        String.Format("{0} {1}", DS_NAME, aDataSourceName));
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// Fetch all DataSources' status for a NDCS service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteStatusDataSource(OdbcCommand anOpenCommand,  
            string aServiceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\"",
                        CMD_STATUS, OBJ_DS, aServiceName);
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// Fetch all sessions for a given NDCS Service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteStatusServer(OdbcCommand anOpenCommand,
            string aServiceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\"",
                        CMD_STATUS, OBJ_SERVER, aServiceName
                        );
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// Fetch all sessions for a given NDCS Service and given Data Source
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <param name="aDataSourceName"></param>
        /// <returns>OdbcDataReader</returns>
        static public OdbcDataReader ExecuteStatusServerForDataSource(OdbcCommand anOpenCommand,
            string aServiceName, string aDataSourceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\", {3}",
                        CMD_STATUS, OBJ_SERVER, aServiceName
                        , String.Format("{0} {1}", DS_NAME, aDataSourceName)
                        );
            return ExecuteReader(anOpenCommand);
        }

        /// <summary>
        /// To Add a DataSource
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAddDataSource(OdbcCommand anOpenCommand, 
            NDCSDataSource aDataSource)
        {
            StringBuilder command = new StringBuilder();
            command.AppendFormat("{0} {1} {2}", CMD_ADD, OBJ_DS, aDataSource.DelimitedName);
            if (aDataSource.MaxServerCount > 0)
            {
                command.AppendFormat(",{0} {1}", MAX_SERVER, aDataSource.MaxServerCount);
            }
            if (aDataSource.AvailableServerCount > 0)
            {
                command.AppendFormat(",{0} {1}", AVAIL_SERVER, aDataSource.AvailableServerCount);
            }
            if (aDataSource.InitialServerCount > 0)
            {
                command.AppendFormat(",{0} {1}", INIT_SERVER, aDataSource.InitialServerCount);
            }
            //if (aDataSource.StartAheadCount > 0)
            //{
            //    command.AppendFormat(",{0} {1}", START_AHEAD, aDataSource.StartAheadCount);
            //}
            if ((aDataSource.ServerIdleTimeout > 0) ||
                (aDataSource.ServerIdleTimeout == NDCSDataSource.TIMEOUT_VALUE_NO_TIMEOUT))
            {
                command.AppendFormat(",{0} \"{1}\"", SERVER_IDLE_TIMEOUT, aDataSource.ServerIdleTimeout);
            }
            if ((aDataSource.ConnectionIdleTimeout > 0) ||
                (aDataSource.ConnectionIdleTimeout == NDCSDataSource.TIMEOUT_VALUE_NO_TIMEOUT))
            {
                command.AppendFormat(",{0} \"{1}\"", CONNECT_TIMEOUT, aDataSource.ConnectionIdleTimeout);
            }

            anOpenCommand.CommandText = command.ToString();
            try
            {
                return ExecuteNonQuery(anOpenCommand);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.AddDsException, aDataSource.Name, ex.Message), ex);
            }
        }

        /// <summary>
        /// Alter a DataSource's basic attributes
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <param name="aChangeBitmap"></param> 
        /// <returns></returns>
        static public int ExecuteAlterDataSource(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource, int aChangeBitmap, ref DataTable errorMessagesTable)
        {
            if (aDataSource == null)
            {
                return NO_UPDATE;
            }
            
            StringBuilder command = new StringBuilder();
            if ((aChangeBitmap & NDCSDataSource.MAX_SERVER_CHANGED) == NDCSDataSource.MAX_SERVER_CHANGED) 
            {
                command.AppendFormat(",{0} {1}", MAX_SERVER, aDataSource.MaxServerCount);
            }
            if ((aChangeBitmap & NDCSDataSource.INIT_SERVER_CHANGED) == NDCSDataSource.INIT_SERVER_CHANGED) 
            {
                // Note: Since at the incoming, we have mapped AvailServer to InitServer, now we have to do 
                //       the reverse. 
                command.AppendFormat(",{0} {1}", INIT_SERVER, aDataSource.InitialServerCount);
            }
            //if ((aChangeBitmap & NDCSDataSource.INIT_SERVER_CHANGED) == NDCSDataSource.INIT_SERVER_CHANGED) 
            //{
            //    // Note: Since at the incoming, we have mapped InitServer to StartAhead, now we have to do 
            //    //       the reverse. 
            //    command.AppendFormat(",{0} {1}", START_AHEAD, aDataSource.InitialServerCount);
            //}
            if ((aChangeBitmap & NDCSDataSource.AVAIL_SERVER_CHANGED) == NDCSDataSource.AVAIL_SERVER_CHANGED) 
            {
                // Note: Since at the incoming, we have mapped StartAhead to Available, now we have to do 
                //       the reverse. 
                command.AppendFormat(",{0} {1}", AVAIL_SERVER, aDataSource.AvailableServerCount);
            }
            if ((aChangeBitmap & NDCSDataSource.SERVER_IDLE_TIMEOUT_CHANGED) == NDCSDataSource.SERVER_IDLE_TIMEOUT_CHANGED) 
            {
                if ((aDataSource.ServerIdleTimeout > 0) ||
                    (aDataSource.ServerIdleTimeout == NDCSDataSource.TIMEOUT_VALUE_NO_TIMEOUT))
                {
                    command.AppendFormat(",{0} \"{1}\"", SERVER_IDLE_TIMEOUT, aDataSource.ServerIdleTimeout);
                }
                else if (aDataSource.ServerIdleTimeout == NDCSDataSource.TIMEOUT_VALUE_SYSTEM_DEFAULT)
                {
                    command.AppendFormat(",{0} \"{1}\"", SERVER_IDLE_TIMEOUT, SYSTEM_DEFAULT);
                }
            }
            if ((aChangeBitmap & NDCSDataSource.CONNECTION_TIMEOUT_CHANGED) == NDCSDataSource.CONNECTION_TIMEOUT_CHANGED) 
            {
                if ((aDataSource.ConnectionIdleTimeout > 0) ||
                    (aDataSource.ConnectionIdleTimeout == NDCSDataSource.TIMEOUT_VALUE_NO_TIMEOUT))
                {
                    command.AppendFormat(",{0} \"{1}\"", CONNECT_TIMEOUT, aDataSource.ConnectionIdleTimeout);
                }
                else if (aDataSource.ConnectionIdleTimeout == NDCSDataSource.TIMEOUT_VALUE_SYSTEM_DEFAULT)
                {
                    command.AppendFormat(",{0} \"{1}\"", CONNECT_TIMEOUT, SYSTEM_DEFAULT);
                }
            }
            if ((aChangeBitmap & NDCSDataSource.AUTOSTART_CHANGED) == NDCSDataSource.AUTOSTART_CHANGED) 
            {
                command.AppendFormat(",{0} {1}", AUTO_START, (aDataSource.StartAutomatically) ? "ON" : "OFF");
            }

            if (command.ToString().Trim().Length == 0)
            {
                return 0;
            }
            
            anOpenCommand.CommandText = String.Format("{0} {1} {2} {3}",
                    CMD_ALTER, OBJ_DS, aDataSource.DelimitedName, command.ToString());
            try
            {
                return ExecuteNonQuery(anOpenCommand);
            }
            catch (Exception ex)
            {
                //throw new Exception(String.Format(Properties.Resources.UpdateDsException, aDataSource.Name, ex.Message), ex);
                errorMessagesTable.Rows.Add(new string[] { ex.Message });
            }
            return NO_UPDATE;
        }

        /// <summary>
        /// To enable/disable Data Source trace for a given NDCS service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <param name="aDataSourceName"></param>
        /// <param name="aTraceFlag"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAlterDataSourceTrace(OdbcCommand anOpenCommand,
            string aServiceName, string aDataSourceName, bool aTraceFlag)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} {2} {3} {4}",
                    CMD_ALTER, OBJ_DS, aDataSourceName
                    , String.Format(",{0} \"{1}\"", SERVICE, aServiceName)
                    , String.Format(",{0} {1}", TRACE, (aTraceFlag) ? "ON" : "OFF")
                    );

            try
            {
                return ExecuteNonQuery(anOpenCommand);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.UpdateTraceException, aDataSourceName, aServiceName, ex.Message), ex);
            }

            //return NO_UPDATE;
        }

        /// <summary>
        /// Alter Data Source's CPU List. 
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <param name="aChangeBitmap"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAlterDataSourceCPUList(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource, int aChangeBitmap, ref DataTable errorMessagesTable)
        {
            if ((aChangeBitmap & NDCSDataSource.CPULIST_CHANGED) == NDCSDataSource.CPULIST_CHANGED) 
            {
                String cpuList = null;
                if (aDataSource.CPUList == null || aDataSource.CPUList.Trim() == "")
                {
                    cpuList = "SYSTEM_DEFAULT";
                }
                else
                {   
                    cpuList = aDataSource.CPUList.Trim();
                }

                anOpenCommand.CommandText = String.Format("{0} {1} {2}, {3}",
                    CMD_ALTER, OBJ_DS, aDataSource.DelimitedName
                    , String.Format("{0} \"{1}\"", CPU_LIST, cpuList)
                    );
                try
                {
                    return ExecuteNonQuery(anOpenCommand);
                }
                catch (Exception ex)
                {
                    //throw new Exception(String.Format(Properties.Resources.UpdateCPUListException, aDataSource.Name, ex.Message), ex);
                    errorMessagesTable.Rows.Add(new string[] { ex.Message });
                }
            }
            return NO_UPDATE;
        }

        /// <summary>
        /// Alter Data source's process priority setting
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <param name="aChangeBitmap"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAlterDataSourcePriority(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource, int aChangeBitmap, ref DataTable errorMessagesTable)
        {
            if ((aChangeBitmap & NDCSDataSource.PRIORITY_CHANGED) == NDCSDataSource.PRIORITY_CHANGED)
            {
                String priority = null;
                if (aDataSource.ProcessPriority == NDCSDataSource.DEFAULT_PROCESS_PRIORITY)
                {
                    priority = "SYSTEM_DEFAULT";
                }
                else
                {   
                    priority = aDataSource.ProcessPriority.ToString();
                }

                anOpenCommand.CommandText = String.Format("{0} {1} {2}, {3}",
                    CMD_ALTER, OBJ_DS, aDataSource.DelimitedName
                    , String.Format("{0} \"{1}\"", PRIORITY, priority)
                    );
                try
                {
                    return ExecuteNonQuery(anOpenCommand);
                }
                catch (Exception ex)
                {
                    //throw new Exception(String.Format(Properties.Resources.UpdateProcessPriorityException, aDataSource.Name, ex.Message), ex);
                    errorMessagesTable.Rows.Add(new string[] { ex.Message });
                }
            }
            return NO_UPDATE;
        }

        /// <summary>
        /// Alter a Data source's setting for statistics recording
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <param name="aChangeBitmap"></param>
        /// <returns>execution status</returns>
        static public int ExecuteAlterDataSourceStats(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource, int aChangeBitmap, ref DataTable errorMessagesTable)
        {

            if ((aChangeBitmap & NDCSDataSource.STATS_CHANGED) == NDCSDataSource.STATS_CHANGED)
            {
                StringBuilder command = new StringBuilder();
                if (aDataSource.ResourceStats.GetIntValue() == ResourceStatistics.ALL_STATS_FLAG)
                {
                    command.AppendFormat("{0} {1}", ALL_STAT, ON);
                }
                else
                {   
                    command.Append(aDataSource.ResourceStats.ToString());
                }

                anOpenCommand.CommandText = String.Format("{0} {1} {2}, {3}",
                    CMD_ALTER, OBJ_DS, aDataSource.DelimitedName, command.ToString());
                try
                {
                    return ExecuteNonQuery(anOpenCommand);
                }
                catch (Exception ex)
                {
                    //throw new Exception(String.Format(Properties.Resources.UpdateStatsException, aDataSource.Name, ex.Message), ex);
                    errorMessagesTable.Rows.Add(new string[] { ex.Message });
                }
            }
            return NO_UPDATE;
        }

        /// <summary>
        /// To Delete a Data source
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        static public int ExecuteDeleteDataSource(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} {2}",
                        CMD_DELETE, OBJ_DS, aDataSource.DelimitedName
                        );
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// To Alter CQD and control table ENVs for a given Data source.  This method loop through the list
        /// of CQDs and Controlled Tables in the DataSource object and add any new entries.
        /// If any entries have been modified or removed, then it will DELETE all the CQDs and controlled tables and re-add them
        /// This is because we do not have an ALTER or DELETE NDCS command to update or remove a specific CQD or controlled table.
        /// </summary>
        /// <param name="anOpenCommand">The odbc command</param>
        /// <param name="currentDataSource">The datasource object that represents the current UI instance</param>
        /// <param name="dbDataSource">The datasource object that represents what's stored in the database</param>
        /// <param name="aChangeBitmap">An integer value that indicates if the properties have been changed</param>
        /// <param name="errorMessagesTable">A data table of error messages if the commands fail</param>
        /// <returns></returns>
        static public int ExecuteAlterDataSourceControlEVARs(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource, NDCSDataSource dbDataSource, int aChangeBitmap, ref DataTable errorMessagesTable)
        {
            if ((aChangeBitmap & NDCSDataSource.ENV_CONTROL_CHANGED) == NDCSDataSource.ENV_CONTROL_CHANGED)
            {
                //Generate the list of NDCS commands
                List<string> commands = GenerateAddAlterControlStmts(CMD_ADD, aDataSource, dbDataSource);

                if (commands.Count > 0)
                {
                    //Execute the commands
                    foreach (string command in commands)
                    {
                        anOpenCommand.CommandText = command;
                        try
                        {
                            ExecuteNonQuery(anOpenCommand);
                        }
                        catch (Exception ex)
                        {
                            errorMessagesTable.Rows.Add(new string[] { command, ex.Message });
                        }
                    }
                    return commands.Count;
                }
            }

            return NO_UPDATE;
        }

        /// <summary>
        /// To Alter Set ENVs for a given Data source.  This method loop through the list
        /// of Sets in the DataSource object and add any new Sets.
        /// If any SETs have been modified or removed, then it will DELETE all the SETs and re-add the SETs
        /// This is because we do not have an ALTER or DELETE NDCS command to update or remove a specific SET.
        /// </summary>
        /// <param name="anOpenCommand">The odbc command</param>
        /// <param name="currentDataSource">The datasource object that represents the current UI instance</param>
        /// <param name="dbDataSource">The datasource object that represents what's stored in the database</param>
        /// <param name="aChangeBitmap">An integer value that indicates if the SET properties have been changed</param>
        /// <param name="errorMessagesTable">A data table of error messages if the SET commands fail</param>
        /// <returns></returns>
        static public int ExecuteAlterDataSourceSetEVARs(OdbcCommand anOpenCommand,
            NDCSDataSource currentDataSource, NDCSDataSource dbDataSource, int aChangeBitmap, ref DataTable errorMessagesTable)
        {
            if ((aChangeBitmap & NDCSDataSource.ENV_SET_CHANGED) == NDCSDataSource.ENV_SET_CHANGED)
            {
                //Generate the list of NDCS commands
                List<string> commands = GenerateAddAlterSetStmts(CMD_ADD, currentDataSource, dbDataSource);

                //Execute the commands
                if (commands.Count > 0)
                {
                    foreach (string command in commands)
                    {
                        anOpenCommand.CommandText = command;
                        try
                        {
                            ExecuteNonQuery(anOpenCommand);
                        }
                        catch (Exception ex)
                        {
                            errorMessagesTable.Rows.Add(new string[] { command, ex.Message });
                        }
                    }
                    return commands.Count;
                }                
            }
            return NO_UPDATE;
        }

        /// <summary>
        /// To delete a EVAR for the given Data source
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aDataSource"></param>
        /// <param name="aEVARType"></param>
        /// <returns>execution status</returns>
        static public int ExecuteDeleteDataSourceEVAR(OdbcCommand anOpenCommand,
            NDCSDataSource aDataSource, int aEVARType)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} {2} {3}",
                    CMD_DELETE, OBJ_EVAR, aEVARType
                    , String.Format("{0} {1}", DS_NAME, aDataSource.DelimitedName)
                    );
            try
            {
                return ExecuteNonQuery(anOpenCommand);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.DeleteEnvironmentVariablesException, aDataSource.Name, ex.Message), ex);
            }
        }

        /// <summary>
        /// Start the given data source
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <param name="aDataSourceName"></param>
        static public int ExecuteStartDataSource(OdbcCommand anOpenCommand, string aServiceName, 
            string aDataSourceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\", {3}",
                        CMD_START, OBJ_DS, aServiceName,
                        String.Format("{0} {1}", DS_NAME, aDataSourceName));
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// Start the given NDCS Service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        static public int ExecuteStartService(OdbcCommand anOpenCommand, string aServiceName)
        {
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\"",
                        CMD_START, OBJ_SERVICE, aServiceName);
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// Stop the given data source in a given NDCS Service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <param name="aDataSourceName"></param>
        /// <param name="aStopMode"></param>
        /// <param name="aReason"></param>
        static public int ExecuteStopDataSource(OdbcCommand anOpenCommand, string aServiceName,
            string aDataSourceName, NDCSObject.StopMode aStopMode, string aReason)
        {
            string stop_mode = (aStopMode == NDCSObject.StopMode.STOP_DISCONNECT) ? DISCONNECT : IMMEDIATE;
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\" {3} {4} {5}",
                        CMD_STOP, OBJ_DS, aServiceName
                        , String.Format(", {0} {1}", DS_NAME, aDataSourceName)
                        , String.Format(", {0} {1}", STOP_MODE, stop_mode)
                        , String.Format(", {0} {1}", REASON, NDCSName.GetDelimitedName(aReason))
                        );

            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// Stop the given NDCS Service
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServiceName"></param>
        /// <param name="aStopMode"></param>
        /// <param name="aReason"></param>
        static public int ExecuteStopService(OdbcCommand anOpenCommand, string aServiceName,
            NDCSObject.StopMode aStopMode, string aReason)
        {
            string stop_mode = (aStopMode == NDCSObject.StopMode.STOP_DISCONNECT) ? DISCONNECT : IMMEDIATE;
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\" {3} {4}",
                        CMD_STOP, OBJ_SERVICE, aServiceName
                        , String.Format(",{0} {1}", STOP_MODE, stop_mode)
                        , String.Format(",{0} {1}", REASON, NDCSName.GetDelimitedName(aReason))
                        );
            return ExecuteNonQuery(anOpenCommand);
        }

        /// <summary>
        /// Stop a given session (server)
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aServerName"></param>
        /// <param name="aDiagID"></param>
        /// <param name="aStopMode"></param>
        static public int ExecuteStopServer(OdbcCommand anOpenCommand, string aServerName,
            int aDiagID, NDCSObject.StopMode aStopMode)
        {
            string stop_mode = (aStopMode == NDCSObject.StopMode.STOP_DISCONNECT) ? DISCONNECT : IMMEDIATE;
            anOpenCommand.CommandText = String.Format("{0} {1} \"{2}\", {3}",
                        CMD_STOP, OBJ_SERVER, aServerName.Trim(),
                        string.Format("{0} {1}", DIAG_ID, aDiagID)//, STOP_MODE, stop_mode)
                        );
            return ExecuteNonQuery(anOpenCommand);
        }

        #endregion Public Methods

        #region Private Methods

        /// <summary>
        /// To Add or Alter Control ENVs for a given Data Source.  
        /// Since both CQDs and Control Tables are CONTROL ENVs, 
        /// this method will loop through all of them to construct a command.
        /// </summary>
        /// <param name="aCommand"></param>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        static private List<string> GenerateAddAlterControlStmts(string aCommand, NDCSDataSource currentDataSource, NDCSDataSource dbDataSource)
        {
            
            List<string> commands = new List<string>();
            string commandPrefix = string.Format("{0} {1} {2}, {3} {4}, {5} \"{6}\"",
                        aCommand, 
                        OBJ_EVAR, 
                        ENV_TYPE_CONTROL,
                        DS_NAME, 
                        currentDataSource.DelimitedName, 
                        VALUE, 
                        " {0}");

            //Get the commands for the data source CQDs as it exists in the database
            string[] dbCQDCommands = dbDataSource.CQDs!= null ? CQD.GenerateControlStmts(dbDataSource.CQDs.ToArray()) : new string[0];
            
            //Get the commands for the data source CQDs as it exists in the UI
            string[] currentCQDCommands = currentDataSource.CQDs != null ? CQD.GenerateControlStmts(currentDataSource.CQDs.ToArray()) : new string[0];

            bool evarsUpdatedOrDeleted = false;//A flag to indicate if any control evars have been updated or removed.

            //Iterate through the CQDs in database and make sure they have not changed
            foreach (String cqd in dbCQDCommands)
            {
                if (currentCQDCommands.Contains(cqd))
                    continue;

                //We are comparing the CQD commands, so if a value has changed or it has been removed, 
                //it will not be found in the current set in the UI, so we set the flag to true.
                evarsUpdatedOrDeleted = true;
                break;
            }

            //Get the commands for the data source control tables as it exists in the database
            string[] currentControlTableCommands = currentDataSource.ControlledTables != null ? ControlledTable.GenerateControlStmts(currentDataSource.ControlledTables.ToArray()) : new string[0];

            //Get the commands for the data source control tables as it exists in the UI
            string[] dbControlTableCommands = dbDataSource.ControlledTables != null ? ControlledTable.GenerateControlStmts(dbDataSource.ControlledTables.ToArray()) : new string[0];

            //If CQD has changed, no need to check if control table has changed, since we are going to delete/re-insert anyways.
            if (!evarsUpdatedOrDeleted)
            {
                //Iterate through the control tables in database and make sure they have not changed
                foreach (String ct in dbControlTableCommands)
                {
                    if (currentControlTableCommands.Contains(ct))
                        continue;

                    //We are comparing the control table commands, so if a value has changed or it has been removed, 
                    //it will not be found in the current set in the UI, so we set the flag to true.
                    evarsUpdatedOrDeleted = true;
                    break;
                }
            }

            //If any controls have been updated or removed, then we delete all control evars and add the ones in the UI one by one.
            if (evarsUpdatedOrDeleted)
            {
                commands.Add(String.Format("Delete Evar 1, DSNAME {0}", currentDataSource.DelimitedName));

                //Add the CQDs
                foreach (String cqd in currentCQDCommands)
                {
                    commands.Add(String.Format(commandPrefix, cqd));
                }
                // Add the control tables
                foreach (String ct in currentControlTableCommands)
                {
                    commands.Add(String.Format(commandPrefix, ct));
                }
            }
            else
            {
                //If no control properties of datasource in database have been modified, then iterate through the controls in the UI 
                //and if they dont exist in database, add them
                foreach (String cqd in currentCQDCommands)
                {
                    if (dbCQDCommands.Contains(cqd))
                        continue;

                    commands.Add(String.Format(commandPrefix, cqd)); //Add the new CQDs
                }
                foreach (String ct in currentControlTableCommands)
                {
                    if (dbControlTableCommands.Contains(ct))
                        continue;

                    commands.Add(String.Format(commandPrefix, ct)); //Add the new control tables
                }
            }

            return commands;
        }

        /// <summary>
        /// To Add Set ENVs for a given Data Source.  
        /// This method will loop through list of Sets to construct a command.
        /// </summary>
        /// <param name="aCommand"></param>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        static private List<string> GenerateAddAlterSetStmts(string aCommand, NDCSDataSource currentDataSource, NDCSDataSource dbDataSource)
        {
            List<string> commands = new List<string>();
            string commandPrefix = string.Format("{0} {1} {2}, {3} {4}, {5} \"{6}\"",
                        aCommand,
                        OBJ_EVAR,
                        ENV_TYPE_SET,
                        DS_NAME,
                        currentDataSource.DelimitedName,
                        VALUE,
                        " {0}");

            //Get the SET commands for the data source as it exists in the database
            string[] existingSetCommands = dbDataSource.Sets!= null ? Set.GenerateSetStmts((dbDataSource.Sets)) : new string[0];

            //Get the SET commands for the data source as it exists in the UI
            string[] modifiedSetCommmands = currentDataSource.Sets != null ? Set.GenerateSetStmts(currentDataSource.Sets) : new string[0];

            bool setsUpdatedOrDeleted = false; //A flag to indicate if any SETs have been updated or removed.

            //First iterate through the SETs in database and make sure they have not changed
            foreach (String set in existingSetCommands)
            {
                if (modifiedSetCommmands.Contains(set))
                    continue;

                //We are comparing the SET commands, so if a value has changed or it has been removed, 
                //it will not be found in the current set in the UI, so we set the flag to true.
                setsUpdatedOrDeleted = true;
                break;
            }

            //If any SETs have been updated or removed, then we delete all SET evars and add the ones in the UI one by one.
            if (setsUpdatedOrDeleted)
            {
                commands.Add(String.Format("Delete Evar 0, DSNAME {0}", currentDataSource.DelimitedName));

                foreach (String set in modifiedSetCommmands)
                {
                    commands.Add(String.Format(commandPrefix, set));
                }
            }
            else
            {
                //If no SETs in database have been modified, then iterate through the SETs in the UI 
                //and if they dont exist in database, add them
                foreach (String set in modifiedSetCommmands)
                {
                    if (existingSetCommands.Contains(set))
                        continue;

                    commands.Add(String.Format(commandPrefix, set)); //Add the new ones.
                }
            }
            return commands;
        }


        /// <summary>
        /// To Add or Alter Resources.  
        /// This method will loop through all of them to construct a command.
        /// </summary>
        /// <param name="aCommand"></param>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        static private String GenerateAddAlterResourceStmts(string aCommand, NDCSDataSource aDataSource)
        {
            StringBuilder command = new StringBuilder();
            if ((aDataSource.Resources != null) && (aDataSource.Resources.Count > 0))
            {
                // Do the first part.
                command.AppendFormat("{0} {1} \"{2}\" {3} {4} {5}",
                    aCommand, 
                    OBJ_RESOURCE, 
                    aDataSource.Resources[0].AttributeName
                    , String.Format(",{0} {1}", DS_NAME, aDataSource.DelimitedName)
                    , String.Format(",{0} {1}", LIMIT_VALUE, aDataSource.Resources[0].Limit)
                    , String.Format(",{0} \"{1}\"", ACTION, NDCSResource.ActionNames[aDataSource.Resources[0].ActionID]));
            }
            return command.ToString();
        }


        static private int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            //anOpenCommand.CommandTimeout = 0;
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connectivity, TRACE_SUB_AREA_NAME, false);
        }

        static private OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            //anOpenCommand.CommandTimeout = 0;
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connectivity, TRACE_SUB_AREA_NAME, false);
        }

        #endregion Private Methods
    }
}
