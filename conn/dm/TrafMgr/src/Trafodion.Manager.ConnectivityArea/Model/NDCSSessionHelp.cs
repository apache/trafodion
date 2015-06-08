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


namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Helper class to get session information for Services and Datasources
    /// </summary>
    class NDCSSessionHelp
    {
        #region Public Methods
        /// <summary>
        /// Gets all sessions for the service provided
        /// </summary>
        /// <param name="aNdcsService"></param>
        /// <returns></returns>
        public DataTable GetSessionsForService(NDCSService aNdcsService)
        {
            return GetSessions(aNdcsService, null);
        }

        /// <summary>
        /// Gets all sessions for a specific DS and a specific NDCS service
        /// </summary>
        /// <param name="aNdcsService"></param>
        /// <param name="aDataSourceName"></param>
        /// <returns></returns>
        public DataTable GetSessionsForDataSource(NDCSService aNdcsService, string aDataSourceName)
        {
            return GetSessions(aNdcsService, aDataSourceName);
        }
        #endregion Public Methods

        #region Private Methods
        private DataTable GetSessions(NDCSService aNdcsService, string aDataSourceName)
        {
            bool forDataSource = (aDataSourceName != null && aDataSourceName.Trim() != "") ? true : false;
            bool forService = !forDataSource;

            OdbcDataReader dsReader = null;
            OdbcCommand anOpenCommand = null;
            DataTable dataTable = new DataTable();

            DataRow row;

            dataTable.Columns.Add(Properties.Resources.ProcessName, typeof(NDCSSession)); 
            dataTable.Columns.Add(Properties.Resources.ServiceName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.DSName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.ServerState, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.PIDClient, typeof(int));
            dataTable.Columns.Add(Properties.Resources.ApplicationName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.UserName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.ComputerName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.Node, typeof(int));
            dataTable.Columns.Add(Properties.Resources.ServerPIN, typeof(int));
            dataTable.Columns.Add(Properties.Resources.Port, typeof(int));
            dataTable.Columns.Add(Properties.Resources.LastUpdatedTime, typeof(EpochTimestamp));

            
            try
            {
                anOpenCommand = aNdcsService.NDCSSystem.GetCommand();
                if (forDataSource)
                {
                    dsReader = Queries.ExecuteStatusServerForDataSource(anOpenCommand, aNdcsService.Name, NDCSName.GetDelimitedName(aDataSourceName));
                }
                else
                {
                    dsReader = Queries.ExecuteStatusServer(anOpenCommand, aNdcsService.Name);
                }

                DataColumn column = dataTable.Columns.Add(Properties.Resources.ClientUserName, typeof(System.String));
                if (dataTable.Columns.Contains(Properties.Resources.UserName))
                {
                    int userNameIndex = dataTable.Columns[Properties.Resources.UserName].Ordinal;
                    column.SetOrdinal(userNameIndex+1);
                }

                while (dsReader.Read())
                {

                    row = dataTable.NewRow();
                    //if (aNdcsService.NDCSSystem.IsNDCSOperator)
                    {
                        // Fill in the session only if the user has the ability to stop it.
                        NDCSSession session = new NDCSSession();
                        session.NDCSService = aNdcsService;
                        NDCSServer server = new NDCSServer(aNdcsService.NDCSSystem, 
                                                           NDCSServer.ServerTypeEnum.CORE_SRVR);
                        session.NDCSServer = server;
                        server.ProcessName = dsReader["ProcessName"] as string;
                        server.Port = int.Parse(dsReader["SrvrPort"] as string);
                        session.DialogueId = (int)dsReader["DialogID"];
                        row[Properties.Resources.ProcessName] = session;
                    }

                    row[Properties.Resources.ServiceName] = aNdcsService.Name;
                    string dsName = NDCSName.ExternalForm((dsReader["DSName"] as string).TrimEnd());
                    row[Properties.Resources.DSName] = dsName;
                    row[Properties.Resources.ServerState] = NDCSServer.ServerStateNames[(int)dsReader["SrvrState"]];
                    row[Properties.Resources.PIDClient] = (long)dsReader["ClientProcessID"];
                    row[Properties.Resources.ApplicationName] = dsReader["WindowText"] as string;
                    row[Properties.Resources.UserName] = dsReader["UserName"] as string;
                    row[Properties.Resources.ClientUserName] = dsReader["ClientUserName"] as string;
                    row[Properties.Resources.ComputerName] = dsReader["ComputerName"] as string;
                    row[Properties.Resources.Node] = (short)dsReader["SrvrNodeID"];
                    row[Properties.Resources.ServerPIN] = (long)dsReader["SrvrProcessID"];
                    row[Properties.Resources.Port] = int.Parse(dsReader["SrvrPort"] as string);
                    row[Properties.Resources.LastUpdatedTime] = new EpochTimestamp((int)dsReader["LastUpdatedTime"]);
                    dataTable.Rows.Add(row);
                }
                dsReader.Close();
            }
            catch (Exception ex)
            {
                //throw new Exception(String.Format(Properties.Resources.GetServerStatusForServiceException, aNdcsService.Name, aDataSourceName, ex.Message), ex);
                row = dataTable.NewRow();
                row[Properties.Resources.ServiceName] = aNdcsService.Name;
                row[Properties.Resources.ServerState] = "Error obtaining status - " + ex.Message;
                dataTable.Rows.Add(row);
            }
            finally
            {
                
                if (dsReader != null)
                {
                    dsReader.Close();
                    dsReader.Dispose();
                }
                aNdcsService.NDCSSystem.CloseCommand(anOpenCommand);
            }

            return dataTable;
        }
        #endregion Private Methods
    }
}
