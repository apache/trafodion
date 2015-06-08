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
using System.Linq;
using System.Text;
using System.Collections;
using Trafodion.Manager.Framework.Connections;
using System.Data.Odbc;
using System.Data;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Represents a NDCS Service
    /// </summary>
    public class NDCSService : NDCSObject
    {
        #region Private member variables

        private NDCSSystem _theNDCSSystem;
        private NDCSServer _theAssociationServer;
        private NDCSServer _theConfigurationServer;
        private string _theServiceName;

        #endregion Private member variables

        #region Public properties

        /// <summary>
        /// The NDCSSystem that has this NDCS Service
        /// </summary>
        public NDCSSystem NDCSSystem
        {
            get { return _theNDCSSystem; }
            set { ; }
        }

        /// <summary>
        /// The Association server associated with this service
        /// </summary>
        public NDCSServer AssociationServer
        {
            get { return _theAssociationServer; }
        }

        /// <summary>
        /// The Configuration server associated with this service
        /// </summary>
        public NDCSServer ConfigurationServer
        {
            get { return _theConfigurationServer; }
        }

        /// <summary>
        /// DataSoruces configured in the NDCSService
        /// </summary>
        public List<NDCSDataSource> NDCSDataSources
        {
            get
            {
                List<NDCSDataSource> dataSources = new List<NDCSDataSource>();
                OdbcDataReader dsReader = null;
                OdbcCommand anOpenCommand = null;

                try
                {
                    anOpenCommand = this._theNDCSSystem.GetCommand();
                    dsReader = Queries.ExecuteStatusDataSource(anOpenCommand, _theServiceName);
                    while (dsReader.Read())
                    {
                        string dsName = dsReader["DSName"] as string;
                        NDCSDataSource ds = new NDCSDataSource(this._theNDCSSystem, dsName.Trim());
                        ds.DSAutomation = (int)dsReader["DSAutomation"];
                        ds.DefaultFlag = (short)dsReader["DefaultFlag"];
                        ds.DSState = (NDCSDataSource.DataSourceState)dsReader["DSState"];
                        ds.MaxServerCount = (int)dsReader["MAXSRVRCNT"];
                        ds.InitialServerCount = (int)dsReader["InitSrvrCnt"];
                        ds.AvailableServerCount = (int)dsReader["AvailSrvrCnt"];
                        //ds.StartAheadCount = (int)dsReader["AvailSrvrCnt"];
                        ds.CurrentSrvrRegistered = (int)dsReader["CurrentSrvrRegistered"];
                        ds.CurrentSrvrConnected = (int)dsReader["CurrentSrvrConnected"];
                        ds.StateChangeTime = (double)((decimal)dsReader["StateChangeTime"]);

                        dataSources.Add(ds);
                    }

                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format(Properties.Resources.GetDsStatusForServiceException, Name, ex.Message), ex);
                }
                finally
                {
                    if (dsReader != null)
                    {
                        dsReader.Close();
                        dsReader.Dispose();
                    }
                    if (anOpenCommand != null)
                    {
                        NDCSSystem.CloseCommand(anOpenCommand);
                    }
                }
                return dataSources;
            }
        }

        /// <summary>
        /// The name of the NDCS service
        /// </summary>
        public override string Name
        {
            get { return _theServiceName; }
            set { ; }
        }

        /// <summary>
        /// Returns the Name of the NDCSService
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Name;
        }

        /// <summary>
        /// Returns the command to replicate
        /// </summary>
        public override string DDLText
        {
            get
            {
                return "";
            }
        }

        /// <summary>
        /// Connection definition associated with this NDCSService
        /// </summary>
        public override ConnectionDefinition ConnectionDefinition
        {
            get { return NDCSSystem.ConnectionDefinition; }
        }
        #endregion Public properties

        #region Constructor
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aSystem"></param>
        /// <param name="aServiceName"></param>
        public NDCSService(NDCSSystem aSystem, string aServiceName)
        {
            _theNDCSSystem = aSystem;
            _theServiceName = aServiceName;
            //PopulateService(aSystem);
        }
        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Refreshes the NDCSservice attributes
        /// </summary>
        override public void Refresh()
        {
            //NDCSSystem.RefreshServices();
            PopulateService(_theNDCSSystem);
        }

        /// <summary>
        /// Returns the status for the service. Displays the Association 
        /// and the Configuration server.
        /// </summary>
        /// <returns></returns>
        public DataTable GetStatus()
        {
            DataRow row = null;
            // Now, define column definitions for datatable.
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(Properties.Resources.ServiceName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.ServiceType, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.ProcessName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.ServiceState, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.Node, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.ServerPIN, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.Port, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.LastUpdatedTime, typeof(EpochTimestamp));

            try
            {
                // First populate the server status.
                PopulateService(_theNDCSSystem);


                // Fill the first row with the association server. 
                row = dataTable.NewRow();
                row[Properties.Resources.ServiceName] = Name;
                row[Properties.Resources.ServiceType] = Properties.Resources.AssociationServer;
                row[Properties.Resources.ProcessName] = AssociationServer.ProcessName;
                row[Properties.Resources.ServiceState] = NDCSServer.ServerStateNames[(int)AssociationServer.ServerState];
                row[Properties.Resources.Node] = AssociationServer.NodeId;
                row[Properties.Resources.ServerPIN] = AssociationServer.ProcessId;
                row[Properties.Resources.Port] = AssociationServer.Port;
                row[Properties.Resources.LastUpdatedTime] = new EpochTimestamp((long)AssociationServer.LastUpdatedTime);
                dataTable.Rows.Add(row);

                // Fill the second row with configuration server. 
                row = dataTable.NewRow();
                row[Properties.Resources.ServiceName] = Name;
                row[Properties.Resources.ServiceType] = Properties.Resources.ConfigurationServer;
                row[Properties.Resources.ProcessName] = ConfigurationServer.ProcessName;
                row[Properties.Resources.ServiceState] = NDCSServer.ServerStateNames[(int)ConfigurationServer.ServerState];
                row[Properties.Resources.Node] = ConfigurationServer.NodeId;
                row[Properties.Resources.ServerPIN] = ConfigurationServer.ProcessId;
                row[Properties.Resources.Port] = ConfigurationServer.Port;
                row[Properties.Resources.LastUpdatedTime] = new EpochTimestamp((long)ConfigurationServer.LastUpdatedTime);
                dataTable.Rows.Add(row);
            }
            catch (Exception ex)
            {
                row = dataTable.NewRow();
                row[Properties.Resources.ServiceName] = Name;
                row[Properties.Resources.ServiceType] = Properties.Resources.AssociationServer;
                row[Properties.Resources.ServerState] = "Error obtaining status - " + ex.Message;
                dataTable.Rows.Add(row);

                row = dataTable.NewRow();
                row[Properties.Resources.ServiceName] = Name;
                row[Properties.Resources.ServiceType] = Properties.Resources.ConfigurationServer;
                row[Properties.Resources.ServerState] = "Error obtaining status - " + ex.Message;
                dataTable.Rows.Add(row);
            }
            return dataTable;
        }

        /// <summary>
        /// Get status for all DataSource for this service.
        /// </summary>
        /// <returns></returns>
        public DataTable GetDataSourceStatus()
        {
            OdbcDataReader dsReader = null;
            OdbcCommand anOpenCommand = null;
            DataTable dataTable = new DataTable();
            DataRow row;

            dataTable.Columns.Add(Properties.Resources.ServiceName, typeof(System.String));
            //dataTable.Columns.Add(Properties.Resources.DSName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.DSName, typeof(NDCSDataSource));
            dataTable.Columns.Add(Properties.Resources.DSState, typeof(System.String));
            if (NDCSSystem.ConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
            {
                dataTable.Columns.Add(Properties.Resources.DSTracing, typeof(System.Boolean));
            }
            dataTable.Columns.Add(Properties.Resources.CurrentServerRegistered, typeof(int));
            dataTable.Columns.Add(Properties.Resources.CurrentServerConnected, typeof(int));
            dataTable.Columns.Add(Properties.Resources.MaxServerCount, typeof(int));
            dataTable.Columns.Add(Properties.Resources.InitServerCount, typeof(int));
            dataTable.Columns.Add(Properties.Resources.AvailServerCount, typeof(int));
            dataTable.Columns.Add(Properties.Resources.StateChangeTime, typeof(EpochTimestamp));

            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                dsReader = Queries.ExecuteStatusDataSource(anOpenCommand, Name);
                while (dsReader.Read())
                {
                    row = dataTable.NewRow();
                    row[Properties.Resources.ServiceName] = Name;
                    //row[Properties.Resources.DSName] = (dsReader["DSName"] as string).Trim();
                    string dsName = NDCSName.ExternalForm((dsReader["DSName"] as string).Trim());
                    row[Properties.Resources.DSName] = NDCSSystem.FindNDCSDataSource(dsName);
                    row[Properties.Resources.DSState] = NDCSDataSource.GetStateName((int)dsReader["DSState"]);
                    if (NDCSSystem.ConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
                    {
                        row[Properties.Resources.DSTracing] = NDCSDataSource.GetTraceState((int)dsReader["DSState"]);
                    }
                    //Count
                    row[Properties.Resources.CurrentServerRegistered] = (int)dsReader["CurrentSrvrRegistered"];
                    //Connected
                    row[Properties.Resources.CurrentServerConnected] = (int)dsReader["CurrentSrvrConnected"];
                    //Maximum
                    row[Properties.Resources.MaxServerCount] = (int)dsReader["MaxSrvrCnt"];
                    row[Properties.Resources.InitServerCount] = (int)dsReader["InitSrvrCnt"];
                    row[Properties.Resources.AvailServerCount] = (int)dsReader["AvailSrvrCnt"];
                    row[Properties.Resources.StateChangeTime] = new EpochTimestamp((long)(decimal)dsReader["StateChangeTime"]);
                    dataTable.Rows.Add(row);
                }

                dsReader.Close();
            }
            catch (Exception ex)
            {
                //throw new Exception(String.Format(Properties.Resources.GetDsStatusForServiceException, Name, ex.Message), ex);
                row = dataTable.NewRow();
                row[Properties.Resources.ServiceName] = Name;
                row[Properties.Resources.DSState] = "Error obtaining status - " + ex.Message;
                if (NDCSSystem.ConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
                {
                    row[Properties.Resources.DSTracing] = false;
                }
                dataTable.Rows.Add(row);
            }
            finally
            {
                if (dsReader != null)
                {
                    dsReader.Close();
                    dsReader.Dispose();
                }
                NDCSSystem.CloseCommand(anOpenCommand);
            }

            return dataTable;
        }
        
        /// <summary>
        /// Returns all sessions for this service
        /// </summary>
        /// <returns></returns>
        public DataTable GetSessions()
        {
            return new NDCSSessionHelp().GetSessionsForService(this);
        }

        /// <summary>
        /// Comparison methods
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(Object obj)
        {
            NDCSService service = obj as NDCSService;
            if (service != null)
            {
                if (this.Name.Equals(service.Name, StringComparison.InvariantCultureIgnoreCase))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            string serviceName = obj as string;
            if ((serviceName != null) && (this.Name.Equals(serviceName, StringComparison.InvariantCultureIgnoreCase)))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// To start this NDCS Service
        /// </summary>
        public void Start()
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                int status = Queries.ExecuteStartService(anOpenCommand, Name);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.ErrorStartingServiceException, Name, ex.Message), ex);
            }
            finally
            {
                NDCSSystem.CloseCommand(anOpenCommand);
            }
        }

        /// <summary>
        /// To stop this NDCS Service
        /// </summary>
        /// <param name="aStopMode"></param>
        /// <param name="aReason"></param>
        public void Stop(StopMode aStopMode, string aReason)
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                int status = Queries.ExecuteStopService(anOpenCommand, Name, aStopMode, aReason);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.ErrorStoppingServiceException, Name, ex.Message), ex);
            }
            finally
            {
                NDCSSystem.CloseCommand(anOpenCommand);
            }
        }


        #endregion Public methods

        #region Private methods
        private void PopulateService(NDCSSystem aSystem)
        {
            OdbcDataReader serviceReader = null;
            OdbcCommand anOpenCommand = null;

            try
            {
                anOpenCommand = aSystem.GetCommand();
                serviceReader = Queries.ExecuteStatusService(anOpenCommand, _theServiceName);
                if (serviceReader.Read())
                {
                    _theAssociationServer = new NDCSServer(aSystem, NDCSServer.ServerTypeEnum.AS_SRVR);
                    _theAssociationServer.NodeId = (short)serviceReader["ASNodeId"];
                    _theAssociationServer.ProcessId = (long)serviceReader["ASProcessId"];
                    _theAssociationServer.ServerState = (NDCSServer.ServerStateEnum)((long)serviceReader["ASSrvrState"]);
                    _theAssociationServer.ProcessName = serviceReader["ASProcessName"] as string;
                    _theAssociationServer.Port = int.Parse(serviceReader["ASPort"] as string);
                    _theAssociationServer.LastUpdatedTime = (int)serviceReader["ASLastUpdatedTime"];
                    _theAssociationServer.ComponentIDVersion = (short)serviceReader["ASComponentIDVersion"];
                    _theAssociationServer.MajorVersion = (short)serviceReader["ASMajorVersion"];
                    _theAssociationServer.MinorVersion = (short)serviceReader["ASMinorVersion"];
                    _theAssociationServer.BuildID = (int)((long)serviceReader["ASBuildID"]);

                    _theConfigurationServer = new NDCSServer(aSystem, NDCSServer.ServerTypeEnum.CFG_SRVR);
                    _theConfigurationServer.NodeId = (short)serviceReader["CfgNodeId"];
                    _theConfigurationServer.ProcessId = (long)serviceReader["CfgProcessId"];
                    _theConfigurationServer.ServerState = (NDCSServer.ServerStateEnum)((long)serviceReader["CfgSrvrState"]);
                    _theConfigurationServer.ProcessName = serviceReader["CfgProcessName"] as string;
                    _theConfigurationServer.Port = int.Parse(serviceReader["CfgPort"] as string);
                    _theConfigurationServer.LastUpdatedTime = (int)serviceReader["CfgLastUpdatedTime"];
                    _theConfigurationServer.ComponentIDVersion = (short)serviceReader["CfgComponentIDVersion"];
                    _theConfigurationServer.MajorVersion = (short)serviceReader["CfgMajorVersion"];
                    _theConfigurationServer.MinorVersion = (short)serviceReader["CfgMinorVersion"];
                    _theConfigurationServer.BuildID = (int)((long)serviceReader["CfgBuildID"]);
                }
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format("Error encountered while obtaining status for service {0}. Error: {1}", Name, ex.Message), ex);
            }
            finally
            {
                if (serviceReader != null)
                {
                    serviceReader.Close();
                    serviceReader.Dispose();
                }
                NDCSSystem.CloseCommand(anOpenCommand);
            }
        }
        #endregion Private methods
    }
}
