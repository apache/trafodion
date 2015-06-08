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
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
	/// <summary>
	/// Summary description for a Database system
	/// </summary>
	public class TrafodionSystem : TrafodionObject
	{
        private static Dictionary<ConnectionDefinition, TrafodionSystem> _activeTrafodionSystems = new Dictionary<ConnectionDefinition, TrafodionSystem>(new MyConnectionDefinitionComparer());
        private ConnectionDefinition _connectionDefinition;
        private List<TrafodionCatalog> _sqlMxCatalogs = null;
        private List<ComponentPrivilege> _componentPrivileges = null;
        private List<ComponentModel> _components = null;
        private List<ComponentPrivilegeUsage> _componentPrivilegeUsages = null;
        private ArrayList _RoleAndUserList = null;
        private List<string> _rolesForDBUser = null;
        private bool _isSpjSupportedCalled = false;
        private bool _isSpjSupported = false;
        private string _spjVersion = "";
        private string _udrRoot = "/home/udr/";
        private static object _syncForLoadingTrafodionCatalogs = new object();
        long _totalSQLSpace = -1;
        long _freeSQLSpace = -1;

        /// <summary>
        /// Constructs a TrafodionSystem instance using the connection definition
        /// </summary>
        /// <param name="aConnectionDefinition">A Connection Definition</param>
        public TrafodionSystem(ConnectionDefinition aConnectionDefinition)
            :base(aConnectionDefinition.Name, 0)
        {

           
            // Rememeber our connection
            _connectionDefinition = aConnectionDefinition;

            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if(!_activeTrafodionSystems.ContainsKey(aConnectionDefinition))
                _activeTrafodionSystems.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
        }

        ~TrafodionSystem()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
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
                _activeTrafodionSystems.Remove(aConnectionDefinition);
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            }
        }
        
        /// <summary>
        /// Returns the Connection object associated with this system
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return new Connection(ConnectionDefinition);
        }

        /// <summary>
        /// Finds an instance of TrafodionSystem for this connection definition. If a system does not exists, a new instance is created
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        public static TrafodionSystem FindTrafodionSystem(ConnectionDefinition connectionDefinition)
        {
            TrafodionSystem sqlMxSystem = null;
            _activeTrafodionSystems.TryGetValue(connectionDefinition, out sqlMxSystem);
            if (sqlMxSystem == null)
            {
                sqlMxSystem = new TrafodionSystem(connectionDefinition);
            }
            return sqlMxSystem;
        }

        /// <summary>
        /// Finds a TrafodionCatalog object from the TrafodionCatalogs list using the catalog UID
        /// </summary>
        /// <param name="aCatalogUID">The UID of the catalog</param>
        /// <returns></returns>
        public TrafodionCatalog FindCatalog(long aCatalogUID)
        {
            TrafodionCatalog theTrafodionCatalog = TrafodionCatalogs.Find(delegate(TrafodionCatalog aTrafodionCatalog)
            {
                return aTrafodionCatalog.UID == aCatalogUID;
            });
            if (theTrafodionCatalog == null)
            {
                //If catalog cannot be found, read all the catalogs again from database
                LoadTrafodionCatalogs();
                theTrafodionCatalog = TrafodionCatalogs.Find(delegate(TrafodionCatalog aTrafodionCatalog)
                {
                    return aTrafodionCatalog.UID == aCatalogUID;
                });
            }

            //If catalog still cannot be determined after the reload, then error
            if (theTrafodionCatalog == null)
                throw new Exception("Catalog not found in System " + ExternalName + ".");

            return theTrafodionCatalog;
        }

        /// <summary>
        /// Finds a TrafodionCatalog object from the TrafodionCatalogs list using the catalog name
        /// </summary>
        /// <param name="aCatalogName">The name of the catalog</param>
        /// <returns></returns>
        public TrafodionCatalog FindCatalog(string aCatalogName)
        {
            TrafodionCatalog theTrafodionCatalog = TrafodionCatalogs.Find(delegate(TrafodionCatalog aTrafodionCatalog)
            {
                return aTrafodionCatalog.ExternalName == aCatalogName;
            });

            //if (theTrafodionCatalog == null)
            //{
            //    theTrafodionCatalog = LoadTrafodionCatalog(aCatalogName);
            //}

            //If catalog still cannot be determined after the reload, then error
            if (theTrafodionCatalog == null)
                throw new Exception("Catalog not found in System " + ExternalName + ".");

            return theTrafodionCatalog;
        }

        /// <summary>
        /// Resets the index model
        /// </summary>
        override public void Refresh()
        {
            base.Refresh();
            _sqlMxCatalogs = null;
            _componentPrivileges = null;
            _componentPrivilegeUsages = null;
            _components = null;
        }

        public long TotalSQLSpace
        {
            get { return _totalSQLSpace; }
        }

        public void RefreshComponentPrivileges()
        {
            _componentPrivileges = null;
            _componentPrivilegeUsages = null;
            _components = null;
            LoadComponentPrivileges();
        }
        /// <summary>
        /// Creates a catalog model
        /// </summary>
        /// <param name="catalogName">Name of the catalog</param>
        /// <returns></returns>
        public TrafodionCatalog LoadTrafodionCatalog(string catalogName)
        {
            Connection connection = null;
            TrafodionCatalog sqlMxCatalog = null;

            try
            {
                connection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectCatalogAttributes(connection, catalogName);
                if(theReader.Read())
                {
                    sqlMxCatalog = new TrafodionCatalog(this, catalogName,
                            theReader.GetInt64(0),
                            theReader.GetString(1).Trim());
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return sqlMxCatalog;
        }

        /// <summary>
        /// Loads all the catalogs for this system
        /// </summary>
        /// <returns></returns>
        private bool LoadTrafodionCatalogs()
		{
            Connection connection = null;

            try
            {
                connection = GetConnection();

                _sqlMxCatalogs = new List<TrafodionCatalog>();
                
                OdbcDataReader theReader = Queries.ExecuteSelectCatalogsAttributes(connection);
                while (theReader.Read())
                {
                    string catalogName = theReader.GetString(0).TrimEnd();

                    _sqlMxCatalogs.Add(new TrafodionCatalog(this, catalogName,
                        theReader.GetInt64(1),
                        theReader.GetString(2).Trim()));
                }

            }
            catch (Exception ex)
            {
                throw new Exception("Failed to load list of catalogs : " + ex.Message);
            }
            finally
			{
                if (connection != null)
                {
                    connection.Close();
                }
			}

			return true;
		}

        public ArrayList RoleAndUserList
        {
            get
            {
                if (_RoleAndUserList==null)
                {
                    if (!LoadRoleAndUserList())
                    {
                        return new ArrayList();
                    }
                }
                return _RoleAndUserList;
            }
        }

        /// <summary>
        /// Roles for the current logon user
        /// </summary>
        public List<string> RolesForCurrentUser
        {
            get
            {
                if (_rolesForDBUser == null)
                {
                    GetRolesForCurrentUser();
                }
                return _rolesForDBUser;
            }
        }

        /// <summary>
        /// List of of catalogs in this system
        /// </summary>
        public List<TrafodionCatalog> TrafodionCatalogs
		{
			get
			{
                lock (_syncForLoadingTrafodionCatalogs)
                {
                    if (_sqlMxCatalogs == null)
                    {
                        /*if (!LoadTrafodionCatalogs())
                        {
                            return new List<TrafodionCatalog>();
                        }*/
                        _sqlMxCatalogs = new List<TrafodionCatalog>();
                        _sqlMxCatalogs.Add(new TrafodionCatalog(this, "TRAFODION"));
                    }
                }
                return _sqlMxCatalogs;
			}
            set
            {
                _sqlMxCatalogs = value;
            }
		}

        public void LoadComponentPrivileges()
        {
            if (_components == null)
            {
                _components = PrivilegesLoader.LoadComponents(this);
            }
            if (_componentPrivilegeUsages == null)
            {
                _componentPrivilegeUsages = PrivilegesLoader.LoadComponentPrivilegeUsages(this);
            }
            if (_componentPrivileges == null)
            {
                _componentPrivileges = PrivilegesLoader.LoadComponentPrivileges(this);
            }
        }
        public List<ComponentPrivilege> ComponentPrivileges
        {
            get
            {
                if (_componentPrivileges == null)
                {
                    LoadComponentPrivileges();
                }
                return _componentPrivileges;
            }
        }
        public List<ComponentPrivilegeUsage> ComponentPrivilegeUsages
        {
            get
            {
                if (_componentPrivilegeUsages == null)
                {
                    _componentPrivilegeUsages = PrivilegesLoader.LoadComponentPrivilegeUsages(this);
                }
                return _componentPrivilegeUsages;
            }
        }

        public List<ComponentModel> Components
        {
            get
            {
                if (_components == null)
                {
                    _components = PrivilegesLoader.LoadComponents(this);
                }
                return _components;
            }
        }

        public string GetComponentName(long componentId)
        {
            ComponentModel theComponent = Components.Find(delegate(ComponentModel aComponentModel)
            {
                return aComponentModel.ComponentUID == componentId;
            });

            return (theComponent == null ? "" : theComponent.ComponentName);
        }

        public long GetComponentUID(string componentName)
        {
            ComponentModel theComponent = Components.Find(delegate(ComponentModel aComponentModel)
            {
                return aComponentModel.ComponentName.Equals(componentName);
            });

            return (theComponent == null ? -1 : theComponent.ComponentUID);
        }

        public string GetComponentPrivilegeName(long componentId, string privType)
        {
            ComponentPrivilegeUsage theComponentUsage = ComponentPrivilegeUsages.Find(delegate(ComponentPrivilegeUsage aComponentPrivilegeUsage)
            {
                return aComponentPrivilegeUsage.ComponentUID == componentId && aComponentPrivilegeUsage.PrivType.Equals(privType);
            });

            return (theComponentUsage == null ? "" : theComponentUsage.PrivName);
        }

        public string GetComponentPrivilegeDescription(long componentId, string privType)
        {
            ComponentPrivilegeUsage theComponentUsage = ComponentPrivilegeUsages.Find(delegate(ComponentPrivilegeUsage aComponentPrivilegeUsage)
            {
                return aComponentPrivilegeUsage.ComponentUID == componentId && aComponentPrivilegeUsage.PrivType.Equals(privType);
            });

            return (theComponentUsage == null ? "" : theComponentUsage.PrivDescription);
        }

        /// <summary>
        /// Connection definition associated with this system
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
        }

        /// <summary>
        /// Find the column encoding of a sql object
        /// </summary>
        /// <param name="catalogName">catalog name</param>
        /// <param name="schemaName">schema name</param>
        /// <param name="objectName">object name</param>
        /// <param name="objectType">object type</param>
        /// <param name="colName">column name</param>
        /// <returns></returns>
        public string GetColumnEncoding(string catalogName, string schemaName, string objectName, string objectType, string colName)
        {
            try
            {
                TrafodionCatalog aTrafodionCatalog = FindCatalog(catalogName);
                if (aTrafodionCatalog != null)
                {
                    TrafodionSchema aTrafodionSchema = aTrafodionCatalog.FindSchema(schemaName);
                    TrafodionSchemaObject aTrafodionSchemaObject = null;

                    if (aTrafodionSchema != null)
                    {
                        switch (objectType)
                        {
                            case TrafodionTable.ObjectType:
                                {
                                    aTrafodionSchemaObject = aTrafodionSchema.FindTable(objectName);
                                    break;
                                }
                            case TrafodionMaterializedView.ObjectType:
                                {
                                    aTrafodionSchemaObject = aTrafodionSchema.FindMaterializedView(objectName);
                                    break;
                                }
                            case TrafodionView.ObjectType:
                                {
                                    aTrafodionSchemaObject = aTrafodionSchema.FindView(objectName);
                                    break;
                                }
                            case TrafodionProcedure.ObjectType:
                                {
                                    aTrafodionSchemaObject = aTrafodionSchema.FindProcedure(objectName);
                                    break;
                                }
                        }
                        if (aTrafodionSchemaObject != null && aTrafodionSchemaObject is IHasTrafodionColumns)
                        {
                            IHasTrafodionColumns sqlMxObjectWithColumns = ((IHasTrafodionColumns)aTrafodionSchemaObject);
                            if (sqlMxObjectWithColumns.Columns != null)
                            {
                                TrafodionColumn column = sqlMxObjectWithColumns.Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
                                {
                                    return aTrafodionColumn.InternalName.Equals(colName);
                                });

                                if (column != null)
                                {
                                    return column.TheCharacterSet;
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
            return "";
        }


        /// <summary>
        /// Checks to see if the stored procedures needed for SPJ Management have been installed
        /// on the system.
        /// </summary>
        public bool IsSpjSupported
        {
            get 
            {
                if (!this._isSpjSupportedCalled)
                {
                    PCFModel spjUtil = new PCFModel(ConnectionDefinition);
                    try
                    {
                        string pingResult = spjUtil.ping();
                        _isSpjSupportedCalled = true;
                        _isSpjSupported = true;
                        if (!string.IsNullOrEmpty(pingResult))
                        {
                            if (pingResult.StartsWith("TrafodionManager_SPJ_VERSION"))
                            {
                                string tempString = pingResult.Substring("TrafodionManager_SPJ_VERSION".Length + 1);
                                int separatorIndex = tempString.IndexOf(';');
                                if (separatorIndex > 0)
                                {
                                    _spjVersion = tempString.Substring(0, separatorIndex);
                                }
                            }
                        }
#if DEBUG
                        Console.WriteLine(pingResult);
#endif
                    }
                    catch (Exception ex)
                    {
                        _isSpjSupported = false;
                        Trafodion.Manager.Framework.Logger.OutputErrorLog("Check for SPJ support failed : " + ex.Message);
                    }
                }
                return _isSpjSupported; 
            }
        }

        public string SPJVersion
        {
            get { return _spjVersion; }
        }
        public string UDRRoot
        {
            get { return _udrRoot; }
        }

        public List<string[]> GetDatabaseUsers()
        {

            Connection connection = null;
            List<string[]> databaseUserList = new List<string[]>();
            try
            {
                connection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectDatabaseUserNames(connection);
                while (theReader.Read())
                {
                    string[] userName = {theReader.GetString(0).TrimEnd(),theReader.GetString(1)};
                    databaseUserList.Add(userName);
                }

            }
            catch (Exception ex)
            {
                throw new Exception("Failed to load list of database users : " + ex.Message);
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return databaseUserList;
        }

        public List<string[]> GetDatabaseRoles()
        {

            Connection connection = null;
            List<string[]> rolesList = new List<string[]>();
            try
            {
                connection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectDatabaseRoles(connection);
                while (theReader.Read())
                {
                    string[] roleName = {theReader.GetString(0).TrimEnd(),theReader.GetString(1).TrimEnd()};
                    rolesList.Add(roleName);
                }

            }
            catch (Exception ex)
            {
                throw new Exception("Failed to load list of database users : " + ex.Message);
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }

            return rolesList;
        }

        /// <summary>
        /// Gets all the roles granted to the current logon user
        /// </summary>
        public void GetRolesForCurrentUser()
        {

            Connection connection = null;
            try
            {
                _rolesForDBUser = new List<string>();
                connection = GetConnection();

                OdbcDataReader theReader = Queries.GetRolesForDBUser(connection, _connectionDefinition.DatabaseUserName);
                while (theReader.Read())
                {
                    _rolesForDBUser.Add(theReader.GetString(0).TrimEnd());
                }

            }
            catch (Exception ex)
            {
                throw new Exception("Failed to load list of roles for current user: " + ex.Message);
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }
        }
        
        public bool LoadRoleAndUserList()
        {
            _RoleAndUserList = new ArrayList();
            _RoleAndUserList.Add(GetDatabaseUsers());
            _RoleAndUserList.Add(GetDatabaseRoles());
            GetRolesForCurrentUser();
            return true;
        }


        public bool DoesPrivilegeExist<T>(ConnectionDefinition aConnectionDefinition, string granteeName, string ansiObjectName, string privilegeType)  where T : TrafodionObject
        {

            bool privilegeExists = false;

            try
            {
                //Load the SQL model for the Alerts View and look up the privileges
                string[] nameParts = Utilities.CrackSQLAnsiName(ansiObjectName);
                Type sqlMxObjectType = typeof(T);

                TrafodionCatalog aTrafodionCatalog = null;
                TrafodionSchema aTrafodionSchema = null;
                TrafodionSchemaObject sqlMxObject = null;

                if (TrafodionObject.Exists<TrafodionCatalog>(TrafodionCatalogs, nameParts[0]))
                {
                    aTrafodionCatalog = FindCatalog(nameParts[0]);
                }
                else
                {
                    aTrafodionCatalog = LoadTrafodionCatalog(nameParts[0]);
                }

                if (aTrafodionCatalog != null)
                {
                    if (TrafodionObject.Exists<TrafodionSchema>(aTrafodionCatalog.TrafodionSchemas, nameParts[1]))
                    {
                        aTrafodionSchema = aTrafodionCatalog.FindSchema(nameParts[1]);
                    }
                    else
                    {
                        aTrafodionSchema = aTrafodionCatalog.LoadTrafodionSchema(nameParts[1]);
                    }

                    if (aTrafodionSchema != null)
                    {
                        if (sqlMxObjectType.IsAssignableFrom(typeof(TrafodionSchema)))
                        {

                        }
                        else
                        {
                            sqlMxObject = aTrafodionSchema.GetSchemaObjectByName<TrafodionSchemaObject>(nameParts[2]);

                            if (TrafodionObject.Exists<TrafodionView>(aTrafodionSchema.TrafodionViews, nameParts[2]))
                            {
                                sqlMxObject = aTrafodionSchema.FindView(nameParts[2]);
                            }
                            else
                            {
                                sqlMxObject = aTrafodionSchema.LoadViewByName(nameParts[2]);
                            }

                            if (sqlMxObject != null)
                            {
                                //Reload the objects privileges. It may have changed since the last time it was looked up.
                                sqlMxObject.ClearPrivileges();

                                //Look up if the grantee has the specified privilege type on the object
                                bool objectLevelPrivilege = sqlMxObject.DoesUserHaveObjectPrivilege(granteeName, privilegeType);
                                if (!objectLevelPrivilege)
                                {
                                    //IF NO object level privilege, check if the grantee has schema level privilege
                                    return aTrafodionSchema.DoesUserHavePrivilege(aConnectionDefinition.RoleName, privilegeType);
                                }
                                else
                                {
                                    return objectLevelPrivilege;
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                throw ex;
            }

            //User does NOT have specified privilege
            return privilegeExists;
        }
        /// <summary>
        /// Fetches the total SQL disk space and the available free space
        /// </summary>
        public void FetchSQLSpaceUsage()
        {
            if (_totalSQLSpace  == -1) //not fetched once yet.
            {
                Connection connection = null;
                try
                {
                    connection = GetConnection();

                    OdbcDataReader theReader = Queries.ExecuteSelectTotalSQLSpace(connection);
                    while (theReader.Read())
                    {
                        long diskSpace = theReader.GetInt64(1);
                        long freeDiskSpace = theReader.GetInt64(2);
                        _totalSQLSpace += diskSpace;
                        _freeSQLSpace += freeDiskSpace;
                    }

                }
                catch (Exception ex)
                {
                    throw new Exception("Failed to get total SQL space info : " + ex.Message);
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
	}
}
