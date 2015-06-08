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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Represents a system for NDCS connetion management
    /// </summary>
    public class NDCSSystem : NDCSObject
    {
        #region Private memeber variables

        //Static dictionary that holds the NDCSSystem singletons for each connection definition
        private static Dictionary<string, NDCSSystem> _activeNDCSSystems =
            new Dictionary<string, NDCSSystem>();
        private ConnectionDefinition _connectionDefinition = null;
        private ConnectionDefinition.ChangedHandler _theConnectionDefinitionChangedHandler = null;
        List<NDCSDataSource> _theNDCSDataSources = null;
        List<NDCSService> _theNDCSServices = null;
        List<NDCSUser> _theNDCSUsers = null;
        private Connection _theCurrentConnection = null;
        private bool _theConnectivitySupported = false;
        private Exception _theTestException = null;

        #endregion private member variables

        #region Public properties

        /// <summary>
        /// Indicates if the user has INFO command privileges
        /// </summary>
        public bool UserHasInfoPrivilege
        {
            get
            {
                return _connectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ADD") ||
                    _connectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER") ||
                    _connectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_DELETE");
            }

        }
        /// <summary>
        /// Indicates the connectivity support for this system
        /// </summary>
        public bool ConnectivitySupported
        {
            get { return _theConnectivitySupported; }
            set { _theConnectivitySupported = value; }
        }

        /// <summary>
        /// Returns the exception got at the the Test Connectivity.
        /// </summary>
        public Exception TestException
        {
            get { return _theTestException; }
        }

        /// <summary>
        /// Returns all of the NDCS Services for this system.
        /// </summary>
        public List<NDCSService> NDCSServices
        {
            get
            {
                if (_theNDCSServices == null)
                {
                    _theNDCSServices = GetServices();
                }
                return _theNDCSServices;
            }
        }

        /// <summary>
        /// Returns all of the DataSource for this system.
        /// </summary>
        public List<NDCSDataSource> NDCSDataSources
        {
            get
            {
                if (_theNDCSDataSources == null)
                {
                    _theNDCSDataSources = GetDataSources();
                }
                return _theNDCSDataSources;
            }
        }

        /// <summary>
        /// Returns all of the NDCS Users for this system.
        /// </summary>
        public List<NDCSUser> NDCSUsers
        {
            get
            {
                if (_theNDCSUsers == null)
                {
                    _theNDCSUsers = GetUsers();
                }
                return _theNDCSUsers;
            }
        }
        /// <summary>
        /// Returns the Name of the System
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Name;
        }

        /// <summary>
        /// There is nothing to be returned.
        /// </summary>
        public override string DDLText
        {
            get
            {
                return "";
            }
        }


        /// <summary>
        /// The System name (The name in Connection Definition)
        /// </summary>
        public override string Name
        {
            get { return ConnectionDefinition.Name; }
            set { ; }
        }

        /// <summary>
        /// ConnectionDefinition of this system
        /// </summary>
        public override ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
        }


        #endregion Public properties

        #region Public methods

        /// <summary>
        /// This method will be used to create a new Datasource.
        /// </summary>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        public NDCSDataSource AddDataSource(NDCSDataSource aDataSource)
        {
            if (aDataSource != null)
            {
                OdbcCommand anOpenCommand = null;
                try
                {
                    anOpenCommand = this.GetCommand();
                    int result = Queries.ExecuteAddDataSource(anOpenCommand, aDataSource);
                }
                catch (Exception ex)
                {
                    throw new Exception(String.Format("DataSource {0} could not be added. Error: {1}", aDataSource.Name, ex.Message), ex);
                }
                finally
                {
                    this.CloseCommand(anOpenCommand);
                }
            }

            // Now, the basic data source attributes have been added.  Go on to update 
            // the rest of the attributes (CQDs, SETs, Defines, etc).
            aDataSource.Update();
            return aDataSource;
        }

        /// <summary>
        /// This method will be used to create a new user.
        /// </summary>
        /// <param name="aUser"></param>
        /// <returns></returns>
        public NDCSUser AddUser(NDCSUser aUser)
        {
            if (aUser != null)
            {
                OdbcCommand anOpenCommand = null;
                try
                {
                    anOpenCommand = this.GetCommand();
                    int result = Queries.ExecuteAddUser(anOpenCommand, aUser);
                    _theNDCSUsers.Add(aUser);
                }
                finally
                {
                    this.CloseCommand(anOpenCommand);
                }
            }
            return aUser;
        }
        /// <summary>
        /// This method will be used to delete the given Datasource.
        /// </summary>
        /// <param name="aDataSource"></param>
        /// <returns></returns>
        public void DeleteDataSource(NDCSDataSource aDataSource)
        {
            if (aDataSource != null)
            {
                OdbcCommand anOpenCommand = null;
                try
                {
                    anOpenCommand = this.GetCommand();
                    int result = Queries.ExecuteDeleteDataSource(anOpenCommand, aDataSource);
                    _theNDCSDataSources.Remove(aDataSource);
                    OnModelChangedEvent(NDCS_MODEL_ACTION.DELETE_DATASOURCE, aDataSource);
                }
                catch (Exception ex)
                {
                    //.net framework internal bug error occurs,ignore this error and continue remove the ds node.
                    if (ex.Message.Contains("Arithmetic operation resulted in an overflow"))
                    {
                        _theNDCSDataSources.Remove(aDataSource);
                        OnModelChangedEvent(NDCS_MODEL_ACTION.DELETE_DATASOURCE, aDataSource);
                    }
                    else
                    {
                        throw new Exception(String.Format("Datasource {0} could not be deleted. Error: {1}", aDataSource.Name, ex.Message), ex);
                    }
                }
                finally
                {
                    this.CloseCommand(anOpenCommand);
                }
            }
        }
        /// <summary>
        /// This method will be used to delete the given user.
        /// </summary>
        /// <param name="aUser"></param>
        /// <returns></returns>
        public void DeleteUser(NDCSUser aUser)
        {
            if (aUser != null)
            {
                OdbcCommand anOpenCommand = null;
                try
                {
                    anOpenCommand = this.GetCommand();
                    int result = Queries.ExecuteDeleteUser(anOpenCommand, aUser);
                    _theNDCSUsers.Remove(aUser);
                }
                finally
                {
                    this.CloseCommand(anOpenCommand);
                }
            }
        }
        
        /// <summary>
        /// Refreshes the list of Services and Datasources of this system
        /// </summary>
        public override void Refresh()
        {
            RefreshServices();
            RefreshDataSources();
            RefreshUsers();
        }

        /// <summary>
        /// Returns the status for all services in the system
        /// </summary>
        /// <returns>DataTable</returns>
        public DataTable GetAllServiceStatus()
        {
            DataTable allServicesStatus = new DataTable();
            foreach (NDCSService service in NDCSServices)
            {
                allServicesStatus.Merge(service.GetStatus());
            }
            return allServicesStatus;
        }

        /// <summary>
        /// Returns the status for all datasources in the system
        /// </summary>
        /// <returns>DataTable</returns>
        public DataTable GetAllDataSourceStatus()
        {
            DataTable allDataSourceStatus = new DataTable();
            foreach (NDCSService service in NDCSServices)
            {
                allDataSourceStatus.Merge(service.GetDataSourceStatus());
            }
            return allDataSourceStatus;
        }


        /// <summary>
        /// Given a service name, finds the service in the system. If none is found,
        /// null shall be returned.
        /// </summary>
        /// <param name="aServiceName"></param>
        /// <returns></returns>
        public NDCSService FindNDCSService(string aServiceName)
        {
            foreach (NDCSService service in NDCSServices)
            {
                if (service.Equals(aServiceName))
                {
                    return service;
                }
            }
            return null;
        }

        /// <summary>
        /// Given a DataSource name, finds the DataSource in the system. If none is found,
        /// null shall be returned.
        /// </summary>
        /// <param name="aDataSourceName"></param>
        /// <returns></returns>
        public NDCSDataSource FindNDCSDataSource(string aDataSourceName)
        {
            foreach (NDCSDataSource dataSource in NDCSDataSources)
            {
                if (dataSource.Equals(aDataSourceName))
                {
                    return dataSource;
                }
            }
            return null;
        }

        /// <summary>
        /// Finds an instance of NDCSSystem for this connection definition. If a system does not 
        /// exists, a new instance is created.
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        public static NDCSSystem FindNDCSSystem(ConnectionDefinition connectionDefinition)
        {
            //If Singleton not already instantiated for the given connection definition, create one 
            NDCSSystem NDCSSystem = null;
            if (connectionDefinition != null)
            {
                _activeNDCSSystems.TryGetValue(connectionDefinition.Name, out NDCSSystem);
            }

            if (NDCSSystem == null)
            {
                NDCSSystem = new NDCSSystem(connectionDefinition);
            }
            return NDCSSystem;
        }

        /// <summary>
        /// Close connections for all NDCSSystem objects and unregister from the 
        /// connection definition event handlers
        /// </summary>
        public static void ReleaseResources()
        {
            foreach (KeyValuePair<string, NDCSSystem> kvp in _activeNDCSSystems)
            {
                kvp.Value.CloseConnection();
                kvp.Value.ReleaseConnectionDefinitionHandler();
            }
            _activeNDCSSystems.Clear();
        }

        /// <summary>
        /// Returns a ODBC Command object for the system
        /// </summary>
        /// <returns>OdbcCommand</returns>
        public OdbcCommand GetCommand()
        {
            OdbcCommand command = null;
            this._theCurrentConnection = new Connection(_connectionDefinition);

            try
            {
                command = Queries.OpenCommand(this._theCurrentConnection);
            }
            catch (Exception ex)
            {
                //We want to re-try beacuse the connection could have been
                //closed due to timeout
                CloseConnection();
                return GetCommandWithNewConnection();
            }

            return command;
        }

        /// <summary>
        /// Closes the open Connection
        /// </summary>
        public void CloseConnection()
        {
            Queries.CloseConnection(this._theCurrentConnection);
            this._theCurrentConnection = null;
        }

        /// <summary>
        /// Remove connection definition handlers
        /// </summary>
        public void ReleaseConnectionDefinitionHandler()
        {
            if (this._theConnectionDefinitionChangedHandler != null)
            {
                ConnectionDefinition.Changed -= _theConnectionDefinitionChangedHandler;
            }
        }

        /// <summary>
        /// Closes the open ODBC Command passed
        /// </summary>
        /// <param name="aCommand"></param>
        public void CloseCommand(OdbcCommand aCommand)
        {
            Queries.CloseCommand(aCommand);
        }

        /// <summary>
        /// Clears the list so that the next call will do the populate again.
        /// </summary>
        public void RefreshServices()
        {
            if (_theNDCSServices != null)
            {
                _theNDCSServices.Clear();
            }
            this._theNDCSServices = null;
        }

        /// <summary>
        /// Clears the list so that the next call will do the populate again.
        /// </summary>
        public void RefreshDataSources()
        {
            if (_theNDCSDataSources != null)
            {
                _theNDCSDataSources.Clear();
            } 
            this._theNDCSDataSources = null;
        }

        /// <summary>
        /// Clears the list so that the next call will do the populate again.
        /// </summary>
        public void RefreshUsers()
        {
            if (_theNDCSUsers != null)
            {
                _theNDCSUsers.Clear();
            }
            this._theNDCSUsers = null;
        }

        ///// <summary>
        ///// Indicates if the current logged on role has NDCS Operator privileges
        ///// </summary>
        //public bool IsNDCSOperator
        //{
        //    get
        //    {
        //        //if (!string.IsNullOrEmpty(ConnectionDefinition.RoleName) &&
        //        //    (ConnectionDefinition.RoleName.Equals("DB__ROOT") || ConnectionDefinition.RoleName.Equals("DB__USERADMINUSER")))
        //        //{
        //            return true;
        //        //}
        //        //else
        //        //{
        //        //    return false;
        //        //}
        //    }
        //}

        #endregion Public methods

        #region private methods

        /// <summary>
        /// Retry the get command again.
        /// </summary>
        /// <returns></returns>
        private OdbcCommand GetCommandWithNewConnection()
        {
            OdbcCommand command = null;
            this._theCurrentConnection = new Connection(this._connectionDefinition);

            try
            {
                command = Queries.OpenCommand(this._theCurrentConnection);
            }
            catch (Exception ex)
            {
                CloseConnection();
                throw ex;
            }

            return command;
        }

        /// <summary>
        /// Get the list of NDCS services in this system.
        /// </summary>
        /// <returns></returns>
        private List<NDCSService> GetServices()
        {
            OdbcDataReader reader = null;
            OdbcCommand anOpenCommand = null;
            List<NDCSService> services = new List<NDCSService>();
            if (!ConnectivitySupported)
            {
                return services;
            }

            try
            {
                anOpenCommand = GetCommand();
                reader = Queries.ExecuteListService(anOpenCommand);

                while (reader.Read())
                {
                    string serviceName = reader["Service"] as string;
                    if ((serviceName != null) && (serviceName.Trim() != ""))
                    {
                        NDCSService service = new NDCSService(this, serviceName.Trim());
                        services.Add(service);
                    }
                }

                services.Sort();

            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.ServiceListCouldNotBeObtainedException, Name, ex.Message), ex);
            }
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                    reader.Dispose();
                }
                CloseCommand(anOpenCommand);
            }

            return services;
        }

        /// <summary>
        /// Get the list of Data Sources in this system.
        /// </summary>
        /// <returns></returns>
        private List<NDCSDataSource> GetDataSources()
        {
            OdbcDataReader reader = null;
            OdbcCommand anOpenCommand = null;
            List<NDCSDataSource> dataSources = new List<NDCSDataSource>();

            if (!ConnectivitySupported)
            {
                return dataSources;
            }

            try
            {
                anOpenCommand = GetCommand();
                reader = Queries.ExecuteListDS(anOpenCommand);

                //This ArrayList stores the name of any DS that fails to load
                ArrayList failedDatasource = new ArrayList();
                while (reader.Read())
                {
                    string dataSourceName = reader[0] as string;
                    if ((dataSourceName != null) && (dataSourceName.Trim() != ""))
                    {
                        try
                        {
                            NDCSDataSource dataSource = new NDCSDataSource(this, dataSourceName.Trim());
                            dataSource.Populate();
                            dataSources.Add(dataSource);
                        }
                        catch (Exception DatasourceException)
                        {
                            //Loading this particular datasource has failed.
                            failedDatasource.Add(dataSourceName);
                        }
                    }
                }

                dataSources.Sort();

                //Handle the message if any datasource failed to load
                if (failedDatasource.Count > 0)
                {
                    //throw new Exception("Failed to load datasource: \r\n" + failedDatasource[0]);
                }

            }
            catch (Exception ex)
            {
                throw new Exception(String.Format(Properties.Resources.DatasourceListCouldNotBeObtainedException, Name, ex.Message), ex);
            }
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                    reader.Dispose();
                }
                CloseCommand(anOpenCommand);
            }
            return dataSources;
        }

        /// <summary>
        /// Get the list of Data Sources in this system.
        /// </summary>
        /// <returns></returns>
        private List<NDCSUser> GetUsers()
        {
            OdbcDataReader reader = null;
            OdbcCommand anOpenCommand = null;
            List<NDCSUser> users = new List<NDCSUser>();

            if (!ConnectivitySupported || !UserHasInfoPrivilege)
            {
                return users;
            }

            try
            {
                anOpenCommand = GetCommand();
                reader = Queries.ExecuteInfoUsers(anOpenCommand);

                while (reader.Read())
                {
                    string userName = reader.GetString(0).TrimEnd();
                    string userType = reader.GetString(1).TrimEnd();
                    string grantorName = "";
                    if (reader.FieldCount > 2)
                    {
                        grantorName = reader.GetString(2);
                    }
                    if (!String.IsNullOrEmpty(userName))
                    {
                        users.Add(new NDCSUser(this, userName, userType, grantorName));
                    }
                }
            }
            catch (Exception ex)
            {
                Trafodion.Manager.Framework.Logger.OutputErrorLog("Failed to fetch HPDCS user privileges : " + ex.Message);
                throw new Exception(String.Format(Properties.Resources.FailedToListPrivileges, Name, ex.Message), ex);
            }
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                    reader.Dispose();
                }
                CloseCommand(anOpenCommand);
            }
            users.Sort();
            return users;
        }

        private NDCSUser FindUserByName(string roleName)
        {
            NDCSUser ndcsUser = NDCSUsers.Find(delegate(NDCSUser aNDCSUser)
            {
                return (aNDCSUser.Name.Equals(ConnectionDefinition.RoleName));
            });

            return ndcsUser;
        }

        /// <summary>
        /// Constructs a NDCSSystem instance using the connection definition
        /// Private constructor. You should instantiate a singleton using the 
        /// FindSystem static method
        /// </summary>
        /// <param name="aConnectionDefinition">A Connection Definition</param>
        private NDCSSystem(ConnectionDefinition aConnectionDefinition)
        {
            if (aConnectionDefinition == null)
            {
                return;
            }
            // Rememeber our connection
            _connectionDefinition = aConnectionDefinition;
            if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                this.ConnectivitySupported = TestConnectivity(aConnectionDefinition);

                //Add this instance to the static dictionary, so in future 
                //if need a reference to this system, 
                //we can look up using the connection definition
                if (!_activeNDCSSystems.ContainsKey(aConnectionDefinition.Name))
                    _activeNDCSSystems.Add(aConnectionDefinition.Name, this);
            }

            _theConnectionDefinitionChangedHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
            //Subscribe to connection definition's events, so that you can 
            //maintain the static dictionary
            ConnectionDefinition.Changed += _theConnectionDefinitionChangedHandler;

        }

        /// <summary>
        /// If the connection definition has changed/removed, the static 
        /// dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        private void ConnectionDefinition_Changed(object aSender,
            ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                //If a connection definition has been invalidated, 
                //remove the NDCSSystem instance from the static dictionary
                NDCSSystem system = null;
                if (_activeNDCSSystems.TryGetValue(aConnectionDefinition.Name, out system))
                {
                    if (system != null)
                    {
                        system.CloseConnection();
                        _activeNDCSSystems.Remove(aConnectionDefinition.Name);
                    }
                }
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            }
        }

        /// <summary>
        /// Test this connection see if it supports NDCS management functions.
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        private bool TestConnectivity(ConnectionDefinition aConnectionDefinition)
        {
            try
            {
                OdbcCommand command = GetCommand();
                CloseCommand(command);
                return true;
            }
            catch (Exception ex)
            {
                this._theTestException = ex;
                Trafodion.Manager.Framework.Logger.OutputErrorLog("Check for connectivity support failed: "+ex.Message);
                //Any error at this point will cause TrafodionManager to state that Connectivity is
                //not supported
                return false;
                //Regex r = new Regex("\\s*A\\s*syntax\\s*error\\s*occurred\\s*.*");
                //if ((ex.Message != null) && (r.IsMatch(ex.Message)))
                //{
                //    return false;
                //}
                //else
                //{
                //    throw ex;
                //}
            }
        }

        #endregion private methods

        #region Test Code
        public static void Main(String[] args)
        {
            ConnectionDefinition con = GetConnectionDefinition();
            NDCSSystem system = NDCSSystem.FindNDCSSystem(con);
            NDCSDataSource dataSource = system.FindNDCSDataSource("yser_DS");
            if (dataSource == null)
            {
                return;
            }

            dataSource = new NDCSDataSource(system, "private_testing");

            dataSource.MaxServerCount = 3;
            dataSource.AvailableServerCount = 1;
            dataSource.InitialServerCount = 1;
            dataSource.StartAutomatically = true;
            system.AddDataSource(dataSource);

            dataSource.Start("$ZTDM");

            //Get datasources
            //Console.Out.WriteLine("Data Sources");
            //Console.Out.WriteLine("-------------");
            //List<NDCSDataSource> dict = system.GetDataSources();
            //foreach (NDCSDataSource ds in dict)
            //{
            //    Console.Out.WriteLine(ds.Name);
            //    List<NDCSSession> sessions =  ds.NDCSSessions;
            //    foreach (NDCSSession session in sessions)
            //    {
            //        Console.Out.WriteLine(session.ToString());
            //    }
            //}

            ////Get Services
            //Console.Out.WriteLine("Services");
            //Console.Out.WriteLine("-------------");
            //Dictionary<string, NDCSService> services = system.GetServices();
            //foreach (KeyValuePair<string, NDCSService> kv in services)
            //{
            //    Console.Out.WriteLine(kv.Value.Name);
            //    NDCSService service = kv.Value;
            //    List<NDCSDataSource> datasources = service.NDCSDataSources;
            //    foreach(NDCSDataSource datasource in datasources)
            //    {
            //        Console.Out.WriteLine(datasource.ToString());
            //    }
            //}

            dataSource.Trace("$ZTDM", true);

            dataSource.Trace("$ZTDM", false);

            dataSource.Stop("$ZTDM", StopMode.STOP_IMMEDIATE, "Testing");

            NDCSService service = system.FindNDCSService("$ZTDM");

            service.Stop(StopMode.STOP_IMMEDIATE, "Testing");

            service.Start();

            Console.Out.WriteLine("End test");
        }


        private static ConnectionDefinition GetConnectionDefinition()
        {
            ConnectionDefinition con = new ConnectionDefinition();
            return con;
        }

        #endregion
    }
}
