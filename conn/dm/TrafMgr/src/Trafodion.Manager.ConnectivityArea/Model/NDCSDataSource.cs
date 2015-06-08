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

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// This class represents a Data Source
    /// </summary>
    public class NDCSDataSource : NDCSObject
    {

        //Change map

        /// <summary>
        /// MAX_SERVER_CHANGED: max server attribute has been changed
        /// </summary>
        public const int MAX_SERVER_CHANGED                 = 1 << 0;
        /// <summary>
        /// AVAIL_SERVER_CHANGED: avail server attribute has been changed
        /// </summary>
        public const int AVAIL_SERVER_CHANGED               = 1 << 1;
        /// <summary>
        /// INIT_SERVER_CHANGED: init server attribute has been changed
        /// </summary>
        public const int INIT_SERVER_CHANGED                = 1 << 2;
        /// <summary>
        /// START_AHEAD_CHANGED: start ahead attribute has been changed
        /// </summary>
        public const int START_AHEAD_CHANGED                = 1 << 3;
        /// <summary>
        /// SERVER_IDLE_TIMEOUT_CHANGED: server idle timeout attribute has been changed
        /// </summary>
        public const int SERVER_IDLE_TIMEOUT_CHANGED        = 1 << 4;
        /// <summary>
        /// CONNECTION_TIMEOUT_CHANGED: conn timeout attribute has been changed
        /// </summary>
        public const int CONNECTION_TIMEOUT_CHANGED         = 1 << 5;
        /// <summary>
        /// AUTOSTART_CHANGED: start mode attribute has been changed
        /// </summary>
        public const int AUTOSTART_CHANGED                  = 1 << 6;
        /// <summary>
        /// CPULIST_CHANGED: CPU list has been changed
        /// </summary>
        public const int CPULIST_CHANGED                    = 1 << 7;
        /// <summary>
        /// PRIORITY_CHANGED: process priority attribute has been changed
        /// </summary>
        public const int PRIORITY_CHANGED                   = 1 << 8;
        /// <summary>
        /// STATS_CHANGED: stats collection has been changed
        /// </summary>
        public const int STATS_CHANGED                      = 1 << 9;
        /// <summary>
        /// ENV_CONTROL_CHANGED: CQDs list or Control table list has been changed
        /// </summary>
        public const int ENV_CONTROL_CHANGED                = 1 << 10;
        /// <summary>
        /// ENV_DEFINE_CHANGED: Define list has been changed
        /// </summary>
        public const int ENV_DEFINE_CHANGED                 = 1 << 11;
        /// <summary>
        /// ENV_SET_CHANGED: Set list has been changed
        /// </summary>
        public const int ENV_SET_CHANGED                    = 1 << 12;
        /// <summary>
        /// RESOURCE_CHANGED: Resource list has been changed.
        /// </summary>
        public const int RESOURCE_CHANGED                   = 1 << 13;

        // Define default values

        /// <summary>
        /// DEFAULT_MAX_SERVER_COUNT denotes the default max server count
        /// </summary>
        public const int DEFAULT_MAX_SERVER_COUNT = 1;
        /// <summary>
        /// DEFAULT_AVAIL_SERVER_COUNT denotes the default avail server count
        /// </summary>
        public const int DEFAULT_AVAIL_SERVER_COUNT = 0;
        /// <summary>
        /// DEFAULT_INIT_SERVER_COUNT denotes the default init server count
        /// </summary>
        public const int DEFAULT_INIT_SERVER_COUNT = 0;
        /// <summary>
        /// DEFAULT_START_AHEAD_COUNT denotes the default start ahead count
        /// </summary>
        public const int DEFAULT_START_AHEAD_COUNT = 0;
        /// <summary>
        /// DEFAULT_PROCESS_PRIORITY denote the default process priority
        /// </summary>
        public const int DEFAULT_PROCESS_PRIORITY = -1;
        /// <summary>
        /// DEFAULT_DSAUTOMATION denotes the default auto start mode
        /// </summary>
        public const int DEFAULT_DSAUTOMATION = 1;
        /// <summary>
        /// DEFAULT_DEFAULT_FLAG denotes default default flag
        /// </summary>
        public const int DEFAULT_DEFAULT_FLAG = 0;
        /// <summary>
        /// DEFAULT_CPU_LIST denotes the default cpu list
        /// </summary>
        public const string DEFAULT_CPU_LIST = "";
        /// <summary>
        /// TIMEOUT_VALUE_SYSTEM_DEFAULT denotes system default timeout value
        /// </summary>
        public const long TIMEOUT_VALUE_SYSTEM_DEFAULT = 0;
        /// <summary>
        /// TIMEOUT_VALUE_NO_TIMEOUT denotes no timeout
        /// </summary>
        public const long TIMEOUT_VALUE_NO_TIMEOUT = -1;

        // Define minimum/maximum values

        /// <summary>
        /// MIN_SERVER_COUNT denotes the minimum server count
        /// </summary>
        public const int MIN_SERVER_COUNT = 0;
        /// <summary>
        /// MAX_SERVER_COUNT denotes the maximum server count
        /// </summary>
        public const int MAX_SERVER_COUNT = 32767;
        /// <summary>
        /// MIN_PROCESS_PRIORITY denotes the minimum process priority
        /// </summary>
        public const int MIN_PROCESS_PRIORITY = 1;
        /// <summary>
        /// MAX_PROCESS_PRIORITY denotes the maximum process priority
        /// </summary>
        public const int MAX_PROCESS_PRIORITY = 199;
        /// <summary>
        /// MIN_TIMEOUT_VALUE denotes the minimum timeout value
        /// </summary>
        public const int MIN_TIMEOUT_VALUE = 1;
        /// <summary>
        /// MAX_TIMEOUT_VALUE denotes the maximum timeout value
        /// </summary>
        public const int MAX_TIMEOUT_VALUE = 65536;
        /// <summary>
        /// MIN_ESTIMATED_COST_LIMIT denotes the minimum estimated cost limit
        /// </summary>
        public const int MIN_ESTIMATED_COST_LIMIT = 0;
        /// <summary>
        /// MAX_ESTIMATED_COST_LIMIT denotes the maximum estimated cost limit
        /// </summary>
        public const int MAX_ESTIMATED_COST_LIMIT = UInt16.MaxValue;
        /// <summary>
        /// MAX_DS_NAME_LENGTH denotes the maximum length of a DS Name
        /// </summary>
        public const int MAX_DS_NAME_LENGTH = 128;

        // Recommended values

        /// <summary>
        /// RECOMMENDED_PROCESS_PRIORITY denotes the recommended process priority
        /// </summary>
        public const int RECOMMENDED_PROCESS_PRIORITY = 183;
        /// <summary>
        /// RECOMMENDED_SERVER_IDLE_TIMEOUT denotes the recommended server idle timeout
        /// </summary>
        public const int RECOMMENDED_SERVER_IDLE_TIMEOUT = 10;
        /// <summary>
        /// RECOMMENDED_CONN_IDLE_TIMEOUT denotes the recommended conn idle timeout
        /// </summary>
        public const int RECOMMENDED_CONN_IDLE_TIMEOUT = 10;

        /// <summary>
        /// ADMIN_LOAD_DATASOURCE is the default Data Source name used by Admin and Load application
        /// </summary>
        public const string ADMIN_LOAD_DATASOURCE = "Admin_Load_DataSource";
        /// <summary>
        /// DEFAULT_DATASOURCE is the default Data Source name
        /// </summary>
        public const string DEFAULT_DATASOURCE = "TDM_Default_DataSource";

        List<string> systemDataSources = new List<string>() { DEFAULT_DATASOURCE, ADMIN_LOAD_DATASOURCE };

        #region Private member variables

        private NDCSSystem _theNDCSSystem;
        private string _theDSName;	                    //data source name
        private int _theMaxServerCount = DEFAULT_MAX_SERVER_COUNT;	            
                                                        //maximum number of servers 
        private int _theAvailableServerCount = DEFAULT_AVAIL_SERVER_COUNT;	    
                                                        //number of available servers 
        private int _theInitialServerCount = DEFAULT_INIT_SERVER_COUNT;	        
                                                        //number of servers running at startup
        //private int _theStartAheadCount = DEFAULT_START_AHEAD_COUNT;	        
                                                        //The no. of start ahead servers
        private long _theServerIdleTimeout = TIMEOUT_VALUE_SYSTEM_DEFAULT;	    //server idle timeout
        private long _theConnectionIdleTimeout = TIMEOUT_VALUE_SYSTEM_DEFAULT;	//connection idle timeout
        private long _theLastUpdated = 0;	            //last updated time
        private long _theRefreshRate = 0;	            //not used – Ignored.
        private List<Define> _theDefines;
        private List<Set> _theSets;
        private List<CQD> _theCQDs;
        private List<ControlledTable> _theControlledTables;
        private List<NDCSResource> _theResources;
        private string _theCPUList = DEFAULT_CPU_LIST;
        private ResourceStatistics _theResourceStats = new ResourceStatistics();
        private int _theProcessPriority = DEFAULT_PROCESS_PRIORITY;
        private int _theDSAutomation = DEFAULT_DSAUTOMATION;
                                            //data source automation (If DSAutomation is 0 or a negative number, 
                                            //the data source starts automatically.  If it is greater than 0, 
                                            //data source has to be started manually.)
        private short _theDefaultFlag = DEFAULT_DEFAULT_FLAG;	            
                                            //flag indicating default data source (if defaultFlag is 1, 
                                            //then the data source is the default data source.  
                                            //If defaultFlag is 0, the data source is not a default data source.)
        private DataSourceState _theDSState = DataSourceState.DS_NOT_AVAILABLE; 
                                            //data source state. The most common data source states are 
                                            //DS_STARTED (data source is running) and DS_STOPPED 
                                            //(data source is stopped).
        private int _theCurrentSrvrRegistered = 0;	    //number of registered server
        private int _theCurrentSrvrConnected = 0;	    //number of connected server
        private double _theStateChangeTime;	        //last state change time

        #endregion Private member variables

        #region Public types and properties
        /// <summary>
        /// ENUM DataSourceState defines state of a DataSource.
        /// </summary>
        public enum DataSourceState {  DS_NOT_AVAILABLE = 0, 
                                DS_STARTING=1, 
                                DS_STARTED=2, 
                                DS_STOPPING=3, 
                                DS_STOPPED=4,
                                DS_UNINITIALIZED=5 };

        /// <summary>
        /// The display names of the datasource states
        /// </summary>
        public static string[] DataSourceStateNames = new string[] { "Not Available", "Starting", "Started", "Stopping", "Stopped", "Uninitialized" };

        /// <summary>
        /// The Defines in the datasource
        /// </summary>
        public List<Define> Defines
        {
            get { return _theDefines; }
            set { _theDefines = value; }
        }

        /// <summary>
        /// The Set statements for this datasource
        /// </summary>
        public List<Set> Sets
        {
            get { return _theSets; }
            set { _theSets = value; }
        }

        /// <summary>
        /// The CQDs defined for this datasource
        /// </summary>
        public List<CQD> CQDs
        {
            get { return _theCQDs; }
            set { _theCQDs = value; }
        }

        /// <summary>
        /// The list of NDCSResources associated with the datasource
        /// </summary>
        public List<NDCSResource> Resources
        {
            get { return _theResources; }
            set { _theResources = value; }
        }

        /// <summary>
        /// The controlled tables associated with the DS
        /// </summary>
        public List<ControlledTable> ControlledTables
        {
            get { return _theControlledTables; }
            set { _theControlledTables = value; }
        }

        /// <summary>
        /// The CPUList associated with the datasource
        /// </summary>
        public string CPUList
        {
            get { return _theCPUList; }
            set { _theCPUList = value; }
        }

        /// <summary>
        /// The ResourceStatistics associated with the datasource
        /// </summary>
        public ResourceStatistics ResourceStats
        {
            get { return _theResourceStats; }
            set { _theResourceStats = value; }
        }

        /// <summary>
        /// The ProcessPriority assigned to processes created by the DS
        /// </summary>
        public int ProcessPriority
        {
            get { return _theProcessPriority; }
            set { _theProcessPriority = value; }
        }

        /// <summary>
        /// The DS Name
        /// </summary>
        public override string Name
        {
            get { return _theDSName; }
            set { _theDSName = value; }
        }

        /// <summary>
        /// The delimited datasource name
        /// </summary>
        public string DelimitedName
        {
            get
            {
                return NDCSName.GetDelimitedName(Name);
            }
        }

        /// <summary>
        /// Is this datasource a system created datasource
        /// </summary>
        public bool IsSystemDataSource
        {
            get
            {
                return systemDataSources.Contains(Name);
            }
        }
        /// <summary>
        /// The maximum number of servers that NDCS should start for the data source.
        /// Note: part of the info DS
        /// </summary>
        public int MaxServerCount
        {
            get { return _theMaxServerCount; }
            set { _theMaxServerCount = value; }
        }

        /// <summary>
        /// The number of NDCS servers that NDCS should always have available 
        /// for ODBC client connections when the data source is in the started state.
        /// The default value is 0. This value cannot exceed the maximum server value.
        /// Note: part of the info DS
        /// </summary>
        public int AvailableServerCount
        {
            get { return _theAvailableServerCount; }
            set { _theAvailableServerCount = value; }
        }

        /// <summary>
        /// Note: part of the info DS
        /// The number of NDCS servers that NDCS starts for the data source 
        /// when the data source is started.
        /// The range is 0 to 200. The default value is 0.
        /// This value cannot exceed the maximum server value.
        /// </summary>
        public int InitialServerCount
        {
            get { return _theInitialServerCount; }
            set { _theInitialServerCount = value; }
        }

        ///// <summary>
        ///// The number of servers started 
        ///// Note: part of the info DS
        ///// </summary>
        //public int StartAheadCount
        //{
        //    get { return _theStartAheadCount; }
        //    set { _theStartAheadCount = value; }
        //}

        /// <summary>
        /// The number of minutes that an NDCS server remains idle before 
        /// it stops itself. 
        /// Note: part of the Info DS
        /// </summary>
        public long ServerIdleTimeout
        {
            get { return _theServerIdleTimeout; }
            set { _theServerIdleTimeout = value; }
        }

        /// <summary>
        /// The number of minutes that a client-server connection remains idle 
        /// before the NDCS server terminates the connection.
        /// Note: part of the info DS
        /// </summary>
        public long ConnectionIdleTimeout
        {
            get { return _theConnectionIdleTimeout; }
            set { _theConnectionIdleTimeout = value; }
        }

        /// <summary>
        /// Indiactes when the DS was last updated
        /// Note: part of the info DS
        /// </summary>
        public long LastUpdated
        {
            get { return _theLastUpdated; }
            set { _theLastUpdated = value; }
        }

        /// <summary>
        /// Used to set the automation flag from the DB
        /// </summary>
        public int DSAutomation
        {
            set { _theDSAutomation = value; }
        }	            

        /// <summary>
        /// NDCS automatically starts the initial NDCS servers for the 
        /// data source when the NDCS service is started.         
        /// </summary>
        public bool StartAutomatically
        {
            get { return _theDSAutomation <= 0; }
            set
            {
                if (value)
                {
                    _theDSAutomation = 0;
                }
                else
                {
                    _theDSAutomation = 1;
                }
            }
        }

        /// <summary>
        /// Indicated that this DS is the default DS
        /// Note: part of the status DS
        /// </summary>
        public short DefaultFlag
        {
            get { return _theDefaultFlag; }
            set { _theDefaultFlag = value; }
        }

        /// <summary>
        /// The current state of the DS for a given service.
        /// Note: part of the status DS
        /// </summary>
        public DataSourceState DSState
        {
            get { return _theDSState; }
            set { _theDSState = value; }
        }

        /// <summary>
        /// The count of servers running
        /// Note: part of the status DS
        /// </summary>
        public int CurrentSrvrRegistered
        {
            get { return _theCurrentSrvrRegistered; }
            set { _theCurrentSrvrRegistered = value; }
        }

        /// <summary>
        /// The count of servers connected
        /// Note: part of the status DS
        /// </summary>
        public int CurrentSrvrConnected
        {
            get { return _theCurrentSrvrConnected; }
            set { _theCurrentSrvrConnected = value; }
        }

        /// <summary>
        /// The last time the state changed for this DS
        /// Note: part of the status DS
        /// </summary>
        public double StateChangeTime
        {
            get { return _theStateChangeTime; }
            set { _theStateChangeTime = value; }
        }

        /// <summary>
        /// The NDCS system associated with this DS
        /// </summary>
        public NDCSSystem NDCSSystem
        {
            get { return _theNDCSSystem; }
            set { _theNDCSSystem = value; }
        }

        /// <summary>
        /// List of sessions associated with the DS
        /// </summary>
        public List<NDCSSession> NDCSSessions
        {
            get 
            {
                List<NDCSSession> sessions = new List<NDCSSession>();
                OdbcDataReader reader = null;
                OdbcCommand anOpenCommand = null;
                string serviceName = null;

                try
                {
                    anOpenCommand = this.NDCSSystem.GetCommand();
                    List<NDCSService> services = NDCSSystem.NDCSServices;
                    foreach (NDCSService service in services)
                    {
                        serviceName = service.Name;
                        reader = Queries.ExecuteStatusServerForDataSource(anOpenCommand, serviceName, DelimitedName);
                       
                        if (reader.Read())
                        {
        
                            NDCSSession session = new NDCSSession();
                            session.NDCSService = service;
                            session.NDCSServer = new NDCSServer(this.NDCSSystem, (NDCSServer.ServerTypeEnum)((int)reader["srvrType"]));
                            session.NDCSServer.NodeId = (short)reader["srvrNodeId"];
                            session.NDCSServer.ProcessId = (long)((decimal)reader["srvrProcessId"]);
                            session.NDCSServer.ServerState = (NDCSServer.ServerStateEnum)((int)reader["SrvrState"]);
                            session.NDCSServer.ProcessName = reader["ProcessName"] as string;
                            //session.NDCSServer.ServerObjRef = reader["SrvrObjRef"] as string;
                            session.NDCSServer.Port = int.Parse(reader["SrvrPort"] as string);
                            session.NDCSServer.LastUpdatedTime = (long)((decimal)reader["LastUpdatedTime"]);
                            session.NDCSServer.ComponentIDVersion = (short)reader["srvrComponentIDVersion"];
                            session.NDCSServer.MajorVersion = (short)reader["srvrMajorVersion"];
                            session.NDCSServer.MinorVersion = (short)reader["srvrMinorVersion"];
                            session.NDCSServer.BuildID = (int)((decimal)reader["srvrBuildID"]);

                            session.NDCSDataSource = this;
                            session.ComputerName = reader["ComputerName"] as string;
                            session.ClientProcessId = (uint)((decimal)reader["clientProcessId"]);
                            session.UserName = reader["UserName"] as string;
                            session.ClientUserName = reader["ClientUserName"] as string;
                            session.WindowText = reader["WindowText"] as string;
                            string dialogIdStr = reader["DIALOGID"] as string;
                            try
                            {
                                session.DialogueId = int.Parse(dialogIdStr);
                            }
                            catch (Exception ex)
                            {
                            }


                            sessions.Add(session);

                        }
                        //close the reader
                        reader.Close();
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format(Properties.Resources.GetServerForServiceException, Name, serviceName, ex.Message), ex);
                }
                finally
                {
                    if (reader != null)
                    {
                        reader.Close();
                        reader.Dispose();
                    }
                    NDCSSystem.CloseCommand(anOpenCommand);
                }
                return sessions;
            }
        }

        /// <summary>
        /// Returns the Name of the DataSource
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
                return "NDCS DataSource";
            }
        }

        /// <summary>
        /// Connection definition associated with this DataSource
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return NDCSSystem.ConnectionDefinition; }
        }

        /// <summary>
        /// Indicates if trace is enabled for this DS for a given service
        /// </summary>
        public bool TraceEnabled
        {
            get { return (GetTraceState((int)DSState)); }
        }

        public Set FindSet(string setName)
        {
            Set theSet = Sets.Find(delegate(Set aSet)
            {
                return aSet.Name.Equals(setName);

            });

            return theSet;
        }

        public ControlledTable FindControlledTable(string tableName)
        {
            ControlledTable theControlledTable = this._theControlledTables.Find(delegate(ControlledTable aControlledTable)
            {
                return aControlledTable.Name.Equals(tableName);

            });

            return theControlledTable;
        }

        #endregion Public types and properties
        
        #region Constructor
        /// <summary>
        /// Constructor for a new NDCS DataSource
        /// </summary>
        /// <param name="aSystem">The NDCS System where this DataSource belongs</param>
        /// <param name="aName">The NDCS DataSource name</param>
        public NDCSDataSource(NDCSSystem aSystem, string aName)
        {
            _theNDCSSystem = aSystem;
            _theDSName = NDCSName.ExternalForm(aName);

        }
        #endregion

        #region Public Methods
        
        /// <summary>
        /// Populates the DS with the basic information. To fully populate, call  PopulateDetail
        /// </summary>
        public void Populate()
        {
            LoadInfoDS();
        }

        /// <summary>
        /// Fully populates the DS
        /// </summary>
        public void PopulateDetail()
        {
            if (_theCQDs != null) _theCQDs.Clear();
            if (_theDefines != null) _theDefines.Clear();
            if (_theSets != null) _theSets.Clear();
            if (_theControlledTables != null) _theControlledTables.Clear();
            if (_theResources != null) _theResources.Clear();
            _theResourceStats = new ResourceStatistics();
            this._theCPUList = "";
            this._theProcessPriority = -1;

            Populate();
            LoadInfoEVARs();

            LoadStatus();
        }
        /// <summary>
        /// Loads the status for the DS
        /// </summary>
        public void LoadStatus()
        {
            OdbcDataReader dsReader = null;
            OdbcCommand anOpenCommand = null;
            string serviceName = null;
            try
            {
                serviceName = ((NDCSService)NDCSSystem.NDCSServices[0]).Name;
                anOpenCommand = this.NDCSSystem.GetCommand();
                dsReader = Queries.ExecuteStatusDataSource(anOpenCommand, serviceName, DelimitedName);
                if (dsReader.Read())
                {
                    _theDSAutomation = (int)dsReader["DSAutomation"];
                    _theDefaultFlag = (short)dsReader["defaultFlag"];
                }
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.GetStatusForDsAndServiceException, Name, serviceName, ex.Message), ex);
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
        }

        /// <summary>
        /// Get DataSource status for all services in the same system.
        /// </summary>
        /// <returns></returns>
        public DataTable GetAllStatus()
        {
            DataTable dataTable = new DataTable();
            foreach (NDCSService service in NDCSSystem.NDCSServices)
            {
                dataTable.Merge(GetStatus(service.Name));
            }
            return dataTable;
        }

        /// <summary>
        /// Get DataSource status for the given service.
        /// </summary>
        /// <param name="aServiceName"></param>
        /// <returns></returns>
        public DataTable GetStatus(string aServiceName)
        {
            OdbcDataReader dsReader = null;
            OdbcCommand anOpenCommand = null;
            DataTable dataTable = new DataTable();
            DataRow row;

            dataTable.Columns.Add(Properties.Resources.DSName, typeof(NDCSDataSource));
            dataTable.Columns.Add(Properties.Resources.ServiceName, typeof(System.String));
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
            dataTable.Columns.Add(Properties.Resources.StateChangeTime, typeof(EpochTimestamp)); //not in DBAdmin

            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                dsReader = Queries.ExecuteStatusDataSource(anOpenCommand, aServiceName, DelimitedName);
                if (dsReader.Read())
                {
                    row = dataTable.NewRow();
                    row[Properties.Resources.DSName] = this;
                    row[Properties.Resources.ServiceName] = aServiceName;

                    row[Properties.Resources.DSState] = GetStateName((int)dsReader["DSState"]);
                    if (NDCSSystem.ConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
                    {
                        row[Properties.Resources.DSTracing] = GetTraceState((int)dsReader["DSState"]);
                    }
                    //Count
                    row[Properties.Resources.CurrentServerRegistered] = (int)dsReader["CurrentSrvrRegistered"];
                    //Connected
                    row[Properties.Resources.CurrentServerConnected] = (int)dsReader["CurrentSrvrConnected"];
                    //Maximum
                    row[Properties.Resources.MaxServerCount] = (int)dsReader["MaxSrvrCnt"];
                    row[Properties.Resources.InitServerCount] = (int)dsReader["InitSrvrCnt"];
                    row[Properties.Resources.AvailServerCount] = (int)dsReader["AvailSrvrCnt"];
                    // SQ returns it as decimal but Trafodion returns it as int. 
                    row[Properties.Resources.StateChangeTime] = new EpochTimestamp((long)(decimal)dsReader["StateChangeTime"]);
                    dataTable.Rows.Add(row);
                }
            }
            catch (Exception ex)
            {
                //throw new Exception(String.Format(Properties.Resources.GetStatusForDsAndServiceException, Name, aServiceName, ex.Message), ex);
                row = dataTable.NewRow();
                row[Properties.Resources.DSName] = this;
                row[Properties.Resources.ServiceName] = aServiceName;
                row[Properties.Resources.DSState] = "Error Obtaining status - " + ex.Message;
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
        /// Get State display name 
        /// </summary>
        /// <param name="aState"></param>
        /// <returns></returns>
        public static string GetStateName(int aState)
        {
            return DataSourceStateNames[aState % 1024];
        }
        
        /// <summary>
        /// Returns all sessions for all NDCS services for this DS
        /// </summary>
        /// <returns></returns>
        public DataTable GetAllSessions()
        {
            DataTable dataTable = new DataTable();
            foreach (NDCSService service in NDCSSystem.NDCSServices)
            {
                dataTable.Merge(GetSessionsForService(service));
            }
            return dataTable;
        }

        /// <summary>
        /// Returns all sessions for the specific service provided for this DS
        /// </summary>
        /// <param name="aNdcsService"></param>
        /// <returns></returns>
        public DataTable GetSessionsForService(NDCSService aNdcsService)
        {
            return new NDCSSessionHelp().GetSessionsForDataSource(aNdcsService, Name);
        }
        
         /// <summary>
        /// Stop the DataSource in the given NDCS Service.
        /// </summary>
        public void Stop(NDCSService aNdcsService, StopMode aStopMode, string aReason)
        {
            Stop(aNdcsService.Name, aStopMode, aReason);
        }

         /// <summary>
        /// Stop the DataSource in the given NDCS Service.
        /// </summary>
        public void Stop(string aServiceName, StopMode aStopMode, string aReason)
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                int status = Queries.ExecuteStopDataSource(anOpenCommand, aServiceName, DelimitedName, aStopMode, aReason);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.ErrorStoppingDsForServiceException, Name, aServiceName, ex.Message), ex);
            }
            finally
            {
                NDCSSystem.CloseCommand(anOpenCommand);
            }
        }

        /// <summary>
        /// Start the DataSource in the given NDCS Service
        /// </summary>
        public void Start(NDCSService aNdcsService)
        {
            Start(aNdcsService.Name);
        }

        /// <summary>
        /// Start the DataSource in the given NDCS Service 
        /// </summary>
        public void Start(string aServiceName)
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                int status = Queries.ExecuteStartDataSource(anOpenCommand, aServiceName, DelimitedName);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.ErrorStartingDsForServiceException, Name, aServiceName, ex.Message), ex);
            }
            finally
            {
                NDCSSystem.CloseCommand(anOpenCommand);
            }
        }

        /// <summary>
        /// Refresh this DataSource.
        /// </summary>
        public override void Refresh()
        {
            try
            {
                Populate();
            }
            catch (Exception ex)
            {
                if (ex.Message.Contains("does not exist"))
                {
                    OnModelRemovedEvent();
                }
                else
                {
                    throw new Exception(ex.Message);                    
                }
            }
        }

        /// <summary>
        /// Updates datasource record in the database with the attributes
        /// </summary>
        /// <returns>a DataTable that has error messages for any operations that might have failed</returns>
        public DataTable Update()
        {
            DataTable errorMessagesTable = new DataTable();
            errorMessagesTable.Columns.Add("Command");
            errorMessagesTable.Columns.Add("Error Text");

            //Load the latest copy of the DS from the DB
            NDCSDataSource dsFromDB = new NDCSDataSource(NDCSSystem, Name);
            dsFromDB.PopulateDetail();

            //Compare with this to find out what needs to be updated
            int changedBitmap = HasChanged(dsFromDB);

            //If any changes have happened, call the appropriate alter method to 
            // make the changes

            if (changedBitmap != 0)
            {
                OdbcCommand anOpenCommand = null;
                try
                {
                    anOpenCommand = this.NDCSSystem.GetCommand();
                    Queries.ExecuteAlterDataSourceControlEVARs(anOpenCommand, this, dsFromDB, changedBitmap, ref errorMessagesTable);
                    Queries.ExecuteAlterDataSourceSetEVARs(anOpenCommand, this, dsFromDB, changedBitmap, ref errorMessagesTable);
                    Queries.ExecuteAlterDataSource(anOpenCommand, this, changedBitmap, ref errorMessagesTable);
                    Queries.ExecuteAlterDataSourceCPUList(anOpenCommand, this, changedBitmap, ref errorMessagesTable);
                    Queries.ExecuteAlterDataSourcePriority(anOpenCommand, this, changedBitmap, ref errorMessagesTable);
                    Queries.ExecuteAlterDataSourceStats(anOpenCommand, this, changedBitmap, ref errorMessagesTable);
                }
                catch (Exception ex)
                {
                    errorMessagesTable.Rows.Add(new string[] { "", ex.Message });
                }
                finally
                {
                    NDCSSystem.CloseCommand(anOpenCommand);
                }
            }
            return errorMessagesTable;
        }

        /// <summary>
        /// Method to enable/disable trace for a DS
        /// </summary>
        /// <param name="aEnable"></param>
        public void Trace(NDCSService aNdcsService, bool aEnable)
        {
            Trace(aNdcsService.Name, aEnable);
        }

        /// <summary>
        /// Method to enable/disable trace for a DS
        /// </summary>
        /// <param name="aEnable"></param>
        public void Trace(string aServiceName, bool aEnable)
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                int status = Queries.ExecuteAlterDataSourceTrace(anOpenCommand, aServiceName, DelimitedName, aEnable);
            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.TraceChangeException, Name, ex.Message), ex);
            }
            finally
            {
                NDCSSystem.CloseCommand(anOpenCommand);
            }
        }

        /// <summary>
        /// Based on a DS State, computes if trace is enabled
        /// </summary>
        /// <param name="aDSState"></param>
        /// <returns></returns>
        public static bool GetTraceState(int aDSState)
        {
            return ((int)aDSState >= 1024);
        }

        /// <summary>
        /// This method checks to see if the current DS is similar to the passed DS
        /// The returned bitmap will indicate the attributes that have been changed
        /// </summary>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        public int HasChanged(NDCSDataSource aDataSource)
        {
            int hasChanged = 0;
            //Check for the Alter DS attributes
            hasChanged = hasChanged | HasDSChanged(aDataSource);

            //Check for ALTER DS CPULIST attributes
            if (!AreObjectsEqual(this.CPUList, aDataSource.CPUList))
            {
                hasChanged = hasChanged | CPULIST_CHANGED;
            }

            //Check for ALTER DS PRIORITY attributes
            if (this.ProcessPriority != aDataSource.ProcessPriority)
            {
                hasChanged = hasChanged | PRIORITY_CHANGED;
            }

            //Check for ALTER DS STATS attributes
            if (this.ResourceStats.GetIntValue() != aDataSource.ResourceStats.GetIntValue())
            {
                hasChanged = hasChanged | STATS_CHANGED;
            }

            //Check for ALTER DS RESOURCE attributes
            if (!AreListsEqual(GetObjectList(this.Resources), GetObjectList(aDataSource.Resources)))
            {
                hasChanged = hasChanged | RESOURCE_CHANGED;
            }

            //Check if any of the control evars have changed - We have to check both CQDs and Controlled tables
            if (!AreListsEqual(GetObjectList(this.CQDs), GetObjectList(aDataSource.CQDs)))
            {
                hasChanged = hasChanged | ENV_CONTROL_CHANGED;
            }
            if (!AreControlledTableListsEqual(GetObjectList(this.ControlledTables), GetObjectList(aDataSource.ControlledTables)))
            {
                hasChanged = hasChanged | ENV_CONTROL_CHANGED;
            }
            //Check if any of the set evars have changed
            if (!AreListsEqual(GetObjectList(this.Sets), GetObjectList(aDataSource.Sets)))
            {
                hasChanged = hasChanged | ENV_SET_CHANGED;
            }

            //Check if any of the define evars have changed
            if (!AreListsEqual(GetObjectList(this.Defines), GetObjectList(aDataSource.Defines)))
            {
                hasChanged = hasChanged | ENV_DEFINE_CHANGED;
            }

            return hasChanged;

        }

        /// <summary>
        /// Comparison methods
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public override bool Equals(Object obj)
        {
            NDCSDataSource dataSource = obj as NDCSDataSource;
            if (dataSource != null)
            {
                if (this.Name.Equals(dataSource.Name, StringComparison.Ordinal))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            string dataSourceName = obj as string;
            if ((dataSourceName != null) && (this.Name.Equals(dataSourceName, StringComparison.Ordinal)))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Add a new define object to this Data Source.
        /// </summary>
        /// <param name="aDefineString"></param>
        public void AddDefine(String aDefineString)
        {
            if (this._theDefines == null)
            {
                _theDefines = new List<Define>();
            }

            //parse the define
            Define define = new Define(aDefineString);

            _theDefines.Add(define);

        }

        /// <summary>
        /// Add a new Set statement to this Data Source. 
        /// </summary>
        /// <param name="aSetString"></param>
        public void AddSet(String aSetString)
        {
            if (this._theSets == null)
            {
                _theSets = new List<Set>();
            }
            //Parse the set string
            Set set = new Set(aSetString);

            _theSets.Add(set);
        }

        /// <summary>
        /// Add a new CQD to this Data source.
        /// </summary>
        /// <param name="aCQDString"></param>
        public void AddCQD(String aCQDString)
        {
            if (this._theCQDs == null)
            {
                _theCQDs = new List<CQD>();
            }
            //Parse the CQD string
            CQD cqd = new CQD(aCQDString);

            _theCQDs.Add(cqd);
        }

        /// <summary>
        /// Add a new Control table statement to this Data source.
        /// </summary>
        /// <param name="aCTString"></param>
        public void AddControlTable(String aCTString)
        {
            if (this._theControlledTables == null)
            {
                _theControlledTables = new List<ControlledTable>();
            }
            //Parse the CQD string
            ControlledTable controlledTable = new ControlledTable(aCTString);

            ControlledTable ct = _theControlledTables.Find(controlledTable.Equals);
            if (ct == null)
            {
                _theControlledTables.Add(controlledTable);
            }
            else
            {
                ct.Merge(controlledTable);
            }
        }

        /// <summary>
        /// Add a new resource to this Data source. 
        /// </summary>
        /// <param name="anAttribute"></param>
        /// <param name="aLimit"></param>
        /// <param name="anActionID"></param>
        public void AddResource(String anAttribute, long aLimit, long anActionID)
        {
            if (this._theResources == null)
            {
                _theResources = new List<NDCSResource>();
            }
            //Create the new object
            NDCSResource resource = new NDCSResource();
            resource.AttributeName = anAttribute;
            resource.Limit = aLimit;
            resource.ActionID = anActionID;

            NDCSResource existingResource = _theResources.Find(resource.Equals);
            if (existingResource == null)
            {
                _theResources.Clear();
                _theResources.Add(resource);
            }
        }

        #endregion public methods

        #region private methods

        private static List<object> GetObjectList(List<NDCSResource> aList)
        {
            List<object> list = new List<object>();
            if (aList != null)
            {
                foreach (NDCSResource resource in aList)
                {
                    list.Add(resource);
                }
            }
            return list;
        }

        private static List<object> GetObjectList(List<ControlledTable> aList)
        {
            List<object> list = new List<object>();
            if (aList != null)
            {
                foreach (ControlledTable ct in aList)
                {
                    list.Add(ct);
                }
            }
            return list;
        }

        private static List<object> GetObjectList(List<CQD> aList)
        {
            List<object> list = new List<object>();
            if (aList != null)
            {
                foreach (CQD cqd in aList)
                {
                    list.Add(cqd);
                }
            }
            return list;
        }

        private static List<object> GetObjectList(List<Set> aList)
        {
            List<object> list = new List<object>();
            if (aList != null)
            {
                foreach (Set set in aList)
                {
                    list.Add(set);
                }
            }
            return list;
        }


        private static List<object> GetObjectList(List<Define> aList)
        {
            List<object> list = new List<object>();
            if (aList != null)
            {
                foreach (Define define in aList)
                {
                    list.Add(define);
                }
            }
            return list;
        }

        private bool AreControlledTableListsEqual(List<object> aList, List<object> anotherList)
        {
            if (!AreListsEqual(aList, anotherList))
            {
                return false;
            }

            foreach (object obj in aList)
            {
                bool match = false;
                ControlledTable aTable = obj as ControlledTable;
                if (aTable != null)
                {
                    foreach (object innerObj in anotherList)
                    {
                        ControlledTable innerTable = innerObj as ControlledTable;
                        if (aTable.DeepCompare(innerTable))
                        {
                            match = true;
                            break;
                        }
                    }
                    if (!match)
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Given two lists checks to see if they have the same elements
        /// </summary>
        /// <param name="aList"></param>
        /// <param name="anotherList"></param>
        /// <returns></returns>
        private bool AreListsEqual(List<object> aList, List<object> anotherList)
        {
            //if both are null then they are the same
            if ((aList == null) && (anotherList == null))
            {
                return true;
            }

            //if only one of them is null then they are different
            if ((aList == null) || (anotherList == null))
            {
                return false;
            }

            //If the counts don't match they are different
            if (aList.Count != anotherList.Count)
            {
                return false;
            }

            //Check if there are any items in the first list that are different in the second
            foreach (object obj in aList)
            {
                if (anotherList.IndexOf(obj) < 0)
                {
                    return false;
                }
            }

             return true;
        }

        /// <summary>
        /// This methods only checks the attributes of the DS to indicate if
        /// the alter DS method needs to be invoked
        /// </summary>
        /// <returns></returns>
        private int HasDSChanged(NDCSDataSource aDataSource)
        {
            int changeMap = 0;
            if (aDataSource.MaxServerCount != this.MaxServerCount)
            {
                changeMap = changeMap | MAX_SERVER_CHANGED;
            }
            if (aDataSource.AvailableServerCount != this.AvailableServerCount)
            {
                changeMap = changeMap | AVAIL_SERVER_CHANGED;
            }
            if (aDataSource.InitialServerCount != this.InitialServerCount)
            {
                changeMap = changeMap | INIT_SERVER_CHANGED;
            }
            //if (aDataSource.StartAheadCount != this.StartAheadCount)
            //{
            //    changeMap = changeMap | START_AHEAD_CHANGED;
            //}
            if (aDataSource.ServerIdleTimeout != this.ServerIdleTimeout)
            {
                changeMap = changeMap | SERVER_IDLE_TIMEOUT_CHANGED;
            }
            if (aDataSource.ConnectionIdleTimeout != this.ConnectionIdleTimeout)
            {
                changeMap = changeMap | CONNECTION_TIMEOUT_CHANGED;
            }
            if (aDataSource.StartAutomatically != this.StartAutomatically)
            {
                changeMap = changeMap | AUTOSTART_CHANGED;
            }
            return changeMap;
        }

        private void LoadInfoDS()
        {
            if (_theNDCSSystem.UserHasInfoPrivilege)
            {
                OdbcDataReader reader = null;
                OdbcCommand anOpenCommand = null;

                try
                {
                    anOpenCommand = this.NDCSSystem.GetCommand();
                    reader = Queries.ExecuteInfoDataSource(anOpenCommand, DelimitedName);
                    if (reader.Read())
                    {
                        MaxServerCount = (int)reader["MaxSrvrCnt"];
                        InitialServerCount = (int)reader["InitSrvrCnt"];
                        AvailableServerCount = (int)reader["AvailSrvrCnt"];
                        ServerIdleTimeout = (long)reader["SrvrIdleTimeout"];
                        ConnectionIdleTimeout = (long)reader["ConnIdleTimeout"];

                        try
                        {
                            DateTime dateTime = (DateTime)reader["LastUpdated"];
                            LastUpdated = dateTime.Ticks;
                        }
                        catch (Exception ex)
                        {
                            LastUpdated = 0;
                        }
                        //RefreshRate = (long)reader["RefreshRate"]; -- Not used
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format("Error encountered while loading attributes for Datasource {0}. Error: {1}", Name, ex.Message), ex);
                }
                finally
                {
                    if (reader != null)
                    {
                        reader.Close();
                        reader.Dispose();
                    }
                    NDCSSystem.CloseCommand(anOpenCommand);
                }
            }
        }

        private void LoadInfoEVARs()
        {
            if (_theNDCSSystem.UserHasInfoPrivilege)
            {
                OdbcDataReader reader = null;
                OdbcCommand anOpenCommand = null;
                try
                {
                    anOpenCommand = this.NDCSSystem.GetCommand();

                    // The API requires us to put the type if we want to specify the DSName.
                    // Therefore, we now have to walk through all different types for a specific DS.
                    for (int i = 0; i < 6; i++)
                    {
                        reader = Queries.ExecuteInfoEnvironment(anOpenCommand, i, DelimitedName);
                        while (reader.Read())
                        {

                            int VarType = (short)reader["EVarType"];
                            string VarValue = reader["EVarVal"] as string;
                            short VarSequence = (short)reader["EVARSEQ"];

                            switch (VarType)
                            {
                                case Queries.ENV_TYPE_SET:
                                    AddSet(VarValue);
                                    break;
                                case Queries.ENV_TYPE_CONTROL:
                                    {
                                        if (CQD.IsCQD(VarValue))
                                        {
                                            AddCQD(VarValue);
                                        }
                                        else if (ControlledTable.IsControlTable(VarValue))
                                        {
                                            AddControlTable(VarValue);
                                        }
                                    }
                                    break;
                                case Queries.ENV_TYPE_DEFINE:
                                    AddDefine(VarValue);
                                    break;
                                case Queries.ENV_TYPE_STATISTICS:
                                    int resStats = -1;
                                    try
                                    {
                                        resStats = int.Parse(VarValue);
                                    }
                                    catch (Exception ex)
                                    {
                                        //do nothing
                                    }
                                    if (resStats >= 0)
                                    {
                                        ResourceStats = new ResourceStatistics(resStats);
                                    }
                                    break;
                                case Queries.ENV_TYPE_CPULIST:
                                    this._theCPUList = VarValue;
                                    break;
                                case Queries.ENV_TYPE_PROCESSPRIORITY:
                                    this._theProcessPriority = ushort.Parse(VarValue);
                                    break;
                            }
                        }

                        //close the reader as we are done with it
                        reader.Close();
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format("Error encountered while loading details for Datasource {0}. Error: {1}", Name, ex.Message), ex);
                }
                finally
                {
                    if (reader != null)
                    {
                        reader.Close();
                        reader.Dispose();
                    }
                    NDCSSystem.CloseCommand(anOpenCommand);
                }
            }
        }

        #endregion private methods
    }
}
