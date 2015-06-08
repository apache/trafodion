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
using System.Linq;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.UserManagement.Model
{
    public class UserMgmtSystemModel
    {
        private static Dictionary<ConnectionDefinition, UserMgmtSystemModel> _activeSystemModels = new Dictionary<ConnectionDefinition, UserMgmtSystemModel>(new MyConnectionDefinitionComparer());

        public static readonly string NameStatusColumn = "Name";
        public static readonly string StatusColumName = "Status";
        public static readonly string MessageColumName = "Message";

        private ConnectionDefinition _theConnectionDefinition = null;
        private Connection _theCurrentConnection = null;

        private const string SuccessStatus = "Success";
        private const string FailureStatus = "Failure";

        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _theConnectionDefinition;
            }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public Connection CurrentConnection
        {
            get
            {
                return _theCurrentConnection;
            }
            set
            {
                _theCurrentConnection = value;
            }
        }

        public bool IsAdminUser
        {
            get
            {
                return (!string.IsNullOrEmpty(ConnectionDefinition.DatabaseUserName) && ConnectionDefinition.DatabaseUserName.ToUpper().Equals("DB__ROOT"))
                    ||
                    (!string.IsNullOrEmpty(ConnectionDefinition.RoleName) && 
                    (ConnectionDefinition.RoleName.ToUpper().Equals("DB__USERADMIN") ||
                    (ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140 && ConnectionDefinition.RoleName.ToUpper().Equals("DB__ROOTROLE"))));
            }
        }


        private UserMgmtSystemModel(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefinition = aConnectionDefinition;
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeSystemModels.ContainsKey(aConnectionDefinition))
                _activeSystemModels.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;

        }

        ~UserMgmtSystemModel()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

        public static UserMgmtSystemModel FindSystemModel(ConnectionDefinition connectionDefinition)
        {
            UserMgmtSystemModel systemModel = null;
            _activeSystemModels.TryGetValue(connectionDefinition, out systemModel);
            if (systemModel == null)
            {
                systemModel = new UserMgmtSystemModel(connectionDefinition);
            }
            return systemModel;
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
                _activeSystemModels.Remove(aConnectionDefinition);
            }
        }

        /// <summary>
        /// This method registers the users passed and returns a data table that has the status of the operation
        /// </summary>
        /// <returns></returns>
        public DataTable RegisterUsers(ArrayList aUserList, List<string> listGrantRoles,List<string> grantCompPrivilegesCmd)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    string[] userMapping = (string[])aUserList[0];
                    AddStatusToTable(userMapping[1], FailureStatus, "Unable to get connection", statusTable);
                    return statusTable;
                }

                int result = -1;
                StringBuilder sbUser = new StringBuilder();
                DataTable dt = new DataTable();
                string defaultRole="";
                for (int i = 0; i < aUserList.Count; i++)
                {
                    string[] userMapping = (string[])aUserList[i];
                    sbUser.Append(Utilities.ExternalUserName(userMapping[1]));
                    if (i < aUserList.Count - 1)
                    {
                        sbUser.Append(",");
                    }

                    try
                    {
                        result = Queries.RegisterUser(CurrentConnection, userMapping);
                        AddStatusToTable(userMapping[1], SuccessStatus, "Register User", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userMapping[1], FailureStatus, ex.Message, statusTable);
                    }
                }

                StringBuilder sbGrantRoles = new StringBuilder();
                  for (int i = 0; i < listGrantRoles.Count; i++)
                {
                    sbGrantRoles.Append(Utilities.ExternalUserName(listGrantRoles[i]));
                    if (i < listGrantRoles.Count - 1)
                    {
                        sbGrantRoles.Append(",");
                    }
                }
                if (sbGrantRoles.Length > 0 && sbUser.Length>0)
                {
                    string grantRoles = sbGrantRoles.ToString();
                    try
                    {
                        result = Queries.GrantRoleToUser(CurrentConnection, grantRoles, sbUser.ToString());
                        AddStatusToTable(sbUser.ToString(), SuccessStatus, "Grant Role to User", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(sbUser.ToString(), FailureStatus, ex.Message, statusTable);
                    }
                }

                if (grantCompPrivilegesCmd.Count > 0 && sbUser.Length > 0)
                {                    
                    foreach (string cmd in grantCompPrivilegesCmd)
                    {
                        string cmdTmp = string.Empty;
                        for (int i = 0; i < aUserList.Count; i++)
                        {
                            string[] userMapping = (string[])aUserList[i];                            
                            try
                            {
                                cmdTmp = cmd.Replace("<GRANTEE_LIST>", Utilities.ExternalUserName(userMapping[1]));
                                result = Queries.ExecuteNonQueryCommand(CurrentConnection, cmdTmp);
                                AddStatusToTable(cmdTmp, SuccessStatus, "Grant Component privileges to User", statusTable);
                            }
                            catch (Exception ex)
                            {
                                AddStatusToTable(cmdTmp, FailureStatus, ex.Message, statusTable);
                            }
                        }
                    
                    }

                }

              
            }
            finally
            {
                CloseConnection();
            }
            return statusTable;
        }

        /// <summary>
        /// This method takes in the list of user names and un register each one of them.
        /// 
        /// </summary>
        /// <param name="originalUser"></param>
        /// <param name="changedUser"></param>
        /// <returns></returns>
        public DataTable UnRegisterUsers(List<string> userNames)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable(userNames[0], FailureStatus, "Unable to get connection", statusTable);
                    return statusTable;
                }

                foreach (string userName in userNames)
                {
                    try
                    {
                        Queries.UnRegisterUser(CurrentConnection, userName);
                        AddStatusToTable(userName, SuccessStatus, "UnRegister User", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userName, FailureStatus, ex.Message, statusTable);
                    }
                }

            }
            finally
            {
                CloseConnection();
            }
            return statusTable;
        }

        /// <summary>
        /// This method will set Primary role for a user
        /// </summary>
        /// <param name="userName"></param>
        /// <param name="roleName"></param>
        /// <param name="roleID"></param>
        public DataTable SetPrimaryRole(string userName, string roleName)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable(userName, FailureStatus, "Unable to get connection", statusTable);
                    return statusTable;
                }

                try
                {
                    Queries.SetPrimaryRole(CurrentConnection, userName,roleName);
                    AddStatusToTable(userName, SuccessStatus, "", statusTable);
                }
                catch (Exception ex)
                {
                    AddStatusToTable(userName, FailureStatus, ex.Message, statusTable);
                }


            }
            finally
            {
                CloseConnection();
            }
            return statusTable;
        }

        private DataTable GetStatusDataTable()
        {
            DataTable table = new DataTable();
            table.Columns.Add(NameStatusColumn);
            table.Columns.Add(StatusColumName);
            table.Columns.Add(MessageColumName);
            return table;
        }

        public bool GetConnection()
        {
            if (this._theCurrentConnection == null && this._theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _theCurrentConnection = new Connection(ConnectionDefinition);
                return true;
            }
            else
            {
                return true;
            }
        }

        public void CloseConnection()
        {
            if (this._theCurrentConnection != null)
            {
                _theCurrentConnection.Close();
                _theCurrentConnection = null;
            }
        }

        private void AddStatusToTable(string anOperation, string aStatus, string aMessage, DataTable aTable)
        {
            aTable.Rows.Add(new object[] { anOperation, aStatus, aMessage });
        }

        public int GetSuccessRowCount(DataTable aDataTable)
        {
            DataRow[] successRows = aDataTable.Select(string.Format("Status = '{0}'", SuccessStatus));
            return successRows.Length;
        }

        public int GetFailureRowCount(DataTable aDataTable)
        {
            DataRow[] successRows = aDataTable.Select(string.Format("Status = '{0}'", FailureStatus));
            return successRows.Length;
        }

        public static string GetAdditionalRoleString(List<string> roles)
        {
            string value = "";
            if (roles != null)
            {
                int roleCount = roles.Count;
                for (int i = 0; i < roleCount; i++)
                {
                    value += (i < (roleCount - 1)) ? roles[i] + ", " : roles[i];
                }
            }
            return value;
        }

        public DataTable GetRoleListByUserName(string userName)
        {
            OdbcDataReader dsReader = null;
            DataTable roleListTbl = new DataTable();
            try
            {
                if (!GetConnection())
                {
                    throw new Exception("Unable to get connection.");
                }
                roleListTbl.Columns.Add("LOGON_ROLE_ID_NAME", typeof(string));
                roleListTbl.Columns.Add("ROLE_NAME", typeof(string));
                roleListTbl.Columns.Add("ROLE_ID", typeof(int));
                if (!string.IsNullOrEmpty(userName))
                {
                    userName = userName.Replace("'", "''");
                }
                dsReader = Queries.GetRolesByUserName(CurrentConnection, userName);

                while (dsReader.Read())
                {
                    object logonRoleId = dsReader.GetValue(0);
                    object roleName = dsReader.GetValue(1);
                    object roleID = dsReader.GetValue(2);

                    if(roleName != System.DBNull.Value)
                    {
                        DataRow dr = roleListTbl.NewRow();
                        dr["LOGON_ROLE_ID_NAME"] = (logonRoleId != System.DBNull.Value) ? logonRoleId.ToString() : "NONE";
                        dr["ROLE_NAME"] = roleName.ToString();
                        if (roleID != System.DBNull.Value)
                        {
                            dr["ROLE_ID"] = roleID.ToString();
                        }
                        roleListTbl.Rows.Add(dr);
                    }
                }
            }
            finally
            {
                CloseConnection();
            }
            return roleListTbl;

        }

        public Hashtable GetUserRolesAndPrivileges(string userName)
        {
            Hashtable htReturn = new Hashtable();
            htReturn.Add("ROLES",GetRolesWithDefaultRoleUserName(userName));
            htReturn.Add("PRIVILEGES",GetComponentPrivileges(userName));
            return htReturn;
        }

        public DataTable GetComponentPrivileges(string userName)
        {
            DataTable dtReturn = new DataTable();
            dtReturn.Columns.Add("Component", typeof(string));
            dtReturn.Columns.Add("Privilege", typeof(string));
            dtReturn.Columns.Add("With Grant Option", typeof(Boolean));
            dtReturn.Columns.Add("Grantor", typeof(string));
            dtReturn.Columns.Add("Description", typeof(string));
 
            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_theConnectionDefinition);
            string granteeName = userName;
            List<long> selectedComponents = new List<long>();
            List<string> privTypes = new List<string>();
            IEnumerable<ComponentPrivilege> privilegesForUser;
            _sqlMxSystem.RefreshComponentPrivileges();
            if (_sqlMxSystem.ComponentPrivilegeUsages.Count > 0)
            {
                selectedComponents.AddRange(
                        from c in _sqlMxSystem.Components
                        select c.ComponentUID);

                privilegesForUser =
                (from priv in _sqlMxSystem.ComponentPrivileges
                 where priv.GranteeName.Equals(granteeName)
                 select priv).ToArray();
                var q =

                from c in _sqlMxSystem.ComponentPrivilegeUsages.Where<ComponentPrivilegeUsage>(c => selectedComponents.Contains(c.ComponentUID))

                join p in privilegesForUser on c.ComponentUID equals p.ComponentUID into ps

                from pr in ps.Where<ComponentPrivilege>(pr => pr.ComponentUID == c.ComponentUID && pr.PrivType.Equals(c.PrivType)).DefaultIfEmpty()
                select new
                {
                    ComponentID = c.ComponentUID,
                    PrivilegeName = c.PrivName,
                    PrivilgesType = c.PrivType,
                    IsGranted = pr == null ? false : true,
                    WithGrant = pr == null ? false : pr.Grantable,
                    Grantor = pr == null ? "" : pr.GrantorName
                };

                foreach (var v in q)
                {
                    if (v.IsGranted)
                    {
                        DataRow row = dtReturn.NewRow();
                        row["Component"] = _sqlMxSystem.GetComponentName(v.ComponentID);
                        row["Privilege"] = v.PrivilegeName;
                        row["With Grant Option"] = v.WithGrant;
                        row["Grantor"] = v.Grantor;
                        row["Description"] = _sqlMxSystem.GetComponentPrivilegeDescription(v.ComponentID, v.PrivilgesType);
                        dtReturn.Rows.Add(row);
                    }

                }
            }

            return dtReturn;
        }

        public DataTable GetRolesWithDefaultRoleUserName(string userName)
        {
            OdbcDataReader dsReader = null;
            DataTable roleListTbl = new DataTable();
            try
            {
                roleListTbl = this.GetRoleListByUserName(userName);
                roleListTbl.Columns.Add("DEFAULT_ROLE", System.Type.GetType("System.Boolean"));
                roleListTbl.AcceptChanges();
                if (roleListTbl != null && roleListTbl.Rows.Count > 0)
                { 
                    if (!GetConnection())
                    {
                        throw new Exception("Unable to get connection.");
                    }

                    dsReader = Queries.GetUserDetails(CurrentConnection, userName);
                    if (dsReader != null && dsReader.Read())
                    {
                        foreach (DataRow dr in roleListTbl.Rows)
                        {
                            if (dr["ROLE_NAME"].ToString().Equals(dsReader["LOGON_ROLE_ID_NAME"]))
                            {
                                dr["DEFAULT_ROLE"] = true;
                            }
                            else
                            {
                                dr["DEFAULT_ROLE"] = false;
                            }
                        }
                        roleListTbl.AcceptChanges();
                    }
                }
            }
            finally
            {
                CloseConnection();
            }
            return roleListTbl;
        }

        public DataTable AlterUserRolesAndPrivileges(ArrayList arrGrantRoles, ArrayList arrRevokeRoles, 
            String[] userInfo,List<string> revokePrivilegesList, 
            List<string> alterPrivilegesList)
        {
            DataTable statusTable = GetStatusDataTable();
            AlterUserRoles(arrGrantRoles, arrRevokeRoles, userInfo, ref statusTable);
            AlterUserPrivileges(revokePrivilegesList, alterPrivilegesList, ref statusTable);
            return statusTable;
        }

        private void AlterUserPrivileges(List<string> revokePrivilegesList,
            List<string> alterPrivilegesList,ref DataTable statusTable)
        {
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable("Alter component privileges", FailureStatus, "Unable to get connection", statusTable);
                    return;
                }

                foreach (string cmd in revokePrivilegesList)
                {
                    try
                    {
                        int  result = Queries.ExecuteNonQueryCommand(CurrentConnection, cmd);
                        AddStatusToTable(cmd, SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(cmd, FailureStatus, ex.Message, statusTable);
                    }
                }

                foreach (string cmd in alterPrivilegesList)
                {
                    try
                    {
                        int result = Queries.ExecuteNonQueryCommand(CurrentConnection, cmd);
                        AddStatusToTable(cmd, SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(cmd, FailureStatus, ex.Message, statusTable);
                    }
                }
            }
            finally
            {
                CloseConnection();
            }
            return;
        }

        private void AlterUserRoles(ArrayList arrGrantRoles, ArrayList arrRevokeRoles, String[] userInfo, ref DataTable statusTable)
        {            
            try
            {                
                StringBuilder sbGrantRoles = new StringBuilder();
                for (int i = 0; i < arrGrantRoles.Count; i++)
                {
                    sbGrantRoles.Append(Utilities.ExternalUserName(arrGrantRoles[i].ToString()));
                    if (i < arrGrantRoles.Count - 1)
                    {
                        sbGrantRoles.Append(",");
                    }
                }

                StringBuilder sbRevokeRoles = new StringBuilder();
                for (int i = 0; i < arrRevokeRoles.Count; i++)
                {
                    sbRevokeRoles.Append(Utilities.ExternalUserName(arrRevokeRoles[i].ToString()));
                    if (i < arrRevokeRoles.Count - 1)
                    {
                        sbRevokeRoles.Append(",");
                    }
                }

                if (!GetConnection())
                {
                    AddStatusToTable(userInfo[0], FailureStatus, "Unable to get connection", statusTable);
                    return;
                }

                int result = -1;

                //Set user immutable option
                if (!userInfo[5].Equals(string.Empty))
                {
                    try
                    {
                        result = Queries.SetUserImmutable(CurrentConnection, userInfo[0], userInfo[5]);
                        AddStatusToTable(userInfo[0], SuccessStatus, "Alter User Set|Reset Immutable ", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userInfo[0], FailureStatus, ex.Message, statusTable);
                    }
                }

                //Set new dir user name
                if (!userInfo[2].Equals(string.Empty))
                {
                    try
                    {
                        result = Queries.SetExternalName(CurrentConnection, userInfo[0], userInfo[2]);
                        AddStatusToTable(userInfo[0], SuccessStatus, "Alter User Set External Name ", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userInfo[0], FailureStatus, ex.Message, statusTable);
                    }
                }

                //Set new User Valid or not
                if (!userInfo[3].Equals(string.Empty))
                {
                    try
                    {
                        result = Queries.SetUserValid(CurrentConnection, userInfo[0], userInfo[3]);
                        AddStatusToTable(userInfo[0], SuccessStatus, "Alter User Set Online|Offline ", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userInfo[0], FailureStatus, ex.Message, statusTable);
                    }
                }

                //Set user authentication option
                if (!userInfo[4].Equals(string.Empty))
                {
                    try
                    {
                        result = Queries.SetUserAuthenticationOption(CurrentConnection, userInfo[0], userInfo[4]);
                        AddStatusToTable(userInfo[0], SuccessStatus, _theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150 ? 
                            "Alter User Set Enterprise|Cluster " : "Alter User Set Local|Remote ", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userInfo[0], FailureStatus, ex.Message, statusTable);
                    }
                }

                if (sbGrantRoles.Length > 0)
                {
                    string grantRoles = sbGrantRoles.ToString();                    
                    try
                    {
                        result = Queries.GrantRoleToUser(CurrentConnection, grantRoles, Utilities.ExternalUserName(userInfo[0]));
                        AddStatusToTable(grantRoles, SuccessStatus, "Grant role to user", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(grantRoles, FailureStatus, ex.Message, statusTable);
                    }
                }
                
                if (sbRevokeRoles.Length > 0)
                {

                    string revokeRoles = sbRevokeRoles.ToString();
                    try
                    {
                        result = Queries.RevokeRoleFromUser(CurrentConnection, revokeRoles, Utilities.ExternalUserName(userInfo[0]));
                        AddStatusToTable(revokeRoles, SuccessStatus, "Revoke Role from user", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(revokeRoles, FailureStatus, ex.Message, statusTable);
                    }
                }

                //Set default role.All the grants should complete first in the alter user screen before the primary role is set.
                if (!userInfo[1].Equals(string.Empty))
                {
                    try
                    {
                        result = Queries.SetPrimaryRole(CurrentConnection, userInfo[0], userInfo[1]);
                        AddStatusToTable(userInfo[0], SuccessStatus, "Alter User Set primary role ", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userInfo[0], FailureStatus, ex.Message, statusTable);
                    }
                }

            }
            finally
            {
                CloseConnection();
            }
            return;
        }
        
    }
}
