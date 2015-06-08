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
    public class Role
    {
        private static Dictionary<ConnectionDefinition, Role> _activeSystemModels = 
            new Dictionary<ConnectionDefinition, Role>(new MyConnectionDefinitionComparer());        
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

        private Role(ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefinition = aConnectionDefinition;
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeSystemModels.ContainsKey(aConnectionDefinition))
                _activeSystemModels.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;

        }

        ~Role()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

        public static Role FindSystemModel(ConnectionDefinition connectionDefinition)
        {
            Role systemModel = null;
            _activeSystemModels.TryGetValue(connectionDefinition, out systemModel);
            if (systemModel == null)
            {
                systemModel = new Role(connectionDefinition);
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
        /// Creating Role(s) and returns a data table that has the status of the operation
        /// </summary>
        /// <returns></returns>
        public DataTable CreateRoles(System.Collections.ArrayList aRoleList, List<string> listGrantToUsers, List<string> listGrantCompPrivilegesCmd)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    string[] roleMapping = (string[])aRoleList[0];
                    AddStatusToTable(roleMapping[1], FailureStatus, "Unable to get connection", statusTable);
                    return statusTable;
                }

                int result = -1;
                StringBuilder sbRole = new StringBuilder();
                DataTable dt = new DataTable();
                for (int i = 0; i < aRoleList.Count; i++)
                {
                    string roleName = Utilities.ExternalUserName((string)aRoleList[i]);
                    sbRole.Append(roleName);
                    if (i < aRoleList.Count - 1)
                    {
                        sbRole.Append(",");
                    }

                    try
                    {
                        result = Queries.ExecuteAddRole(CurrentConnection, roleName);
                        AddStatusToTable(roleName, SuccessStatus, "Add Role(s)", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(roleName, FailureStatus, ex.Message, statusTable);
                    }
                }

                StringBuilder sbGrantToUsers = new StringBuilder();

                for (int i = 0; i < listGrantToUsers.Count; i++)
                {
                    sbGrantToUsers.Append(Utilities.ExternalUserName(listGrantToUsers[i]));
                    if (i < listGrantToUsers.Count - 1)
                    {
                        sbGrantToUsers.Append(",");
                    }
                }

                if (sbGrantToUsers.Length > 0 && sbRole.Length > 0)
                {
                    string grantToUsers = sbGrantToUsers.ToString();
                    try
                    {
                        result = Queries.GrantRoleToUser(CurrentConnection, sbRole.ToString(),grantToUsers );
                        AddStatusToTable(grantToUsers, SuccessStatus, "Grant Role to User", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(grantToUsers, FailureStatus, ex.Message, statusTable);
                    }
                }

                if (listGrantCompPrivilegesCmd.Count > 0 && aRoleList.Count > 0)
                {                    
                    foreach (string cmd in listGrantCompPrivilegesCmd)
                    {
                        string cmdTmp = string.Empty;
                        for (int i = 0; i < aRoleList.Count; i++)
                        {
                            string roleName = Utilities.ExternalUserName((string)aRoleList[i]);

                            try
                            {
                                cmdTmp = cmd.Replace("<GRANTEE_LIST>", roleName);
                                result = Queries.ExecuteNonQueryCommand(CurrentConnection, cmdTmp);
                                AddStatusToTable(cmdTmp, SuccessStatus, "Grant Component privileges to Role", statusTable);
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
        /// Drop Role(s) and returns a data table that has the status of the operation
        /// </summary>
        /// <param name="roleNames"></param>
        /// <param name="isCascade"></param>
        /// <returns></returns>
        public DataTable DropRoles(List<string> roleNames, bool isCascade)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable(roleNames[0], FailureStatus, "Unable to get connection", statusTable);
                    return statusTable;
                }

                foreach (string roleName in roleNames)
                {
                    try
                    {
                        Queries.ExecutDropRole(CurrentConnection, roleName, isCascade);
                        AddStatusToTable(roleName, SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(roleName, FailureStatus, ex.Message, statusTable);
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
        /// Revoke Role from users and returns a data table that has the status of the operation
        /// </summary>
        /// <param name="originalUser"></param>
        /// <param name="changedUser"></param>
        /// <returns></returns>
        public DataTable RevokeRoleFromUsers(System.Collections.ArrayList aUserList)
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

                DataTable dt = new DataTable();
                for (int i = 0; i < aUserList.Count; i++)
                {
                    string[] userName = (string[])aUserList[i];
                    try
                    {
                        result = Queries.RevokeRoleFromUser(CurrentConnection,
                            Utilities.ExternalUserName(userName[0]),
                            Utilities.ExternalUserName(userName[1]));
                        AddStatusToTable(userName[1], SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userName[1], FailureStatus, ex.Message, statusTable);
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
        /// Grant roles to user list
        /// </summary>
        /// <returns></returns>
        public DataTable GrantMultipleUsers(System.Collections.ArrayList aUserList)
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

                DataTable dt = new DataTable();
                for (int i = 0; i < aUserList.Count; i++)
                {
                    string[] userName = (string[])aUserList[i];
                    try
                    {
                        result = Queries.GrantRoleToUser(CurrentConnection,
                            Utilities.ExternalUserName(userName[0]),
                            Utilities.ExternalUserName(userName[1]));
                        AddStatusToTable(userName[1], SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(userName[1], FailureStatus, ex.Message, statusTable);
                    }
                }
            }
            finally
            {
                CloseConnection();
            }
            return statusTable;
        }



        public Hashtable GetRoleUsersAndPrivileges(string roleName)
        {
            Hashtable htReturn = new Hashtable();
            htReturn.Add("USERS", GetGrantedUsersByRoleName(roleName));
            htReturn.Add("PRIVILEGES", GetComponentPrivileges(roleName));
            return htReturn;
        }

        public DataTable GetComponentPrivileges(string roleName)
        {
            DataTable dtReturn = new DataTable();
            dtReturn.Columns.Add("Component", typeof(string));
            dtReturn.Columns.Add("Privilege", typeof(string));
            dtReturn.Columns.Add("With Grant Option", typeof(Boolean));
            dtReturn.Columns.Add("Grantor", typeof(string));
            dtReturn.Columns.Add("Description", typeof(string));

            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_theConnectionDefinition);
            string granteeName = roleName;
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

        public DataTable GetGrantedUsersByRoleName(string roleName)
        {
            OdbcDataReader dsReader = null;
            DataTable userListTbl = new DataTable();
            try
            {

                    if (!GetConnection())
                    {
                        throw new Exception("Unable to get connection.");
                    }
                    userListTbl.Columns.Add("USER_NAME", typeof(string));
                    userListTbl.Columns.Add("USER_ID", typeof(int));
                    userListTbl.Columns.Add("EXTERNAL_USER_NAME", typeof(string));

                    dsReader = Queries.GetUsersByRoleName(CurrentConnection, roleName);
                    while (dsReader.Read())
                    {
                        DataRow dr = userListTbl.NewRow();
                        dr["USER_NAME"] = dsReader["USER_NAME"].ToString();
                        dr["USER_ID"] = Convert.ToInt32(dsReader["USER_ID"]);
                        dr["EXTERNAL_USER_NAME"] = dsReader["EXTERNAL_USER_NAME"].ToString();
                        userListTbl.Rows.Add(dr);
                    }
                 userListTbl.AcceptChanges();
            }
            finally
            {
                CloseConnection();
            }
            return userListTbl;
        }

        public DataTable AlterRoleUsersAndPrivileges(ArrayList arrGrantUsers, ArrayList arrRevokeUsers,
                         string aRole, List<string> revokePrivilegesList,
                        List<string> alterPrivilegesList)
        {
            DataTable statusTable = GetStatusDataTable();
            AlterRoles(arrGrantUsers, arrRevokeUsers, aRole, ref statusTable);
            AlterRolePrivileges(revokePrivilegesList, alterPrivilegesList, ref statusTable);
            return statusTable;
        }

        private void AlterRolePrivileges(List<string> revokePrivilegesList,
                    List<string> alterPrivilegesList, ref DataTable statusTable)
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
                        int result = Queries.ExecuteNonQueryCommand(CurrentConnection, cmd);
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

        public void AlterRoles(ArrayList arrGrantUsers, ArrayList arrRevokeUsers, String aRole, ref DataTable statusTable)
        {
            
            try
            {
                StringBuilder sbGrantUsers = new StringBuilder();
                for (int i = 0; i < arrGrantUsers.Count; i++)
                {
                    sbGrantUsers.Append(Utilities.ExternalUserName(arrGrantUsers[i].ToString()));
                    if (i < arrGrantUsers.Count - 1)
                    {
                        sbGrantUsers.Append(",");
                    }
                }

                StringBuilder sbRevokeUsers = new StringBuilder();
                for (int i = 0; i < arrRevokeUsers.Count; i++)
                {
                    sbRevokeUsers.Append(Utilities.ExternalUserName(arrRevokeUsers[i].ToString()));
                    if (i < arrRevokeUsers.Count - 1)
                    {
                        sbRevokeUsers.Append(",");
                    }
                }

                if (!GetConnection())
                {
                    AddStatusToTable(aRole, FailureStatus, "Unable to get connection", statusTable);
                    return;
                }

                int result = -1;
                if (sbGrantUsers.Length > 0)
                {
                    string grantUsers = sbGrantUsers.ToString();
                    try
                    {
                        result = Queries.GrantRoleToUser(CurrentConnection, Utilities.ExternalUserName(aRole), grantUsers);
                        AddStatusToTable(grantUsers, SuccessStatus, "Grant Role to User", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(grantUsers, FailureStatus, ex.Message, statusTable);
                    }
                }

                if (sbRevokeUsers.Length > 0)
                {

                    string revokeUsers = sbRevokeUsers.ToString();
                    try
                    {
                        result = Queries.RevokeRoleFromUser(CurrentConnection, Utilities.ExternalUserName(aRole), revokeUsers);
                        AddStatusToTable(revokeUsers, SuccessStatus, "Revoke Role From User", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(revokeUsers, FailureStatus, ex.Message, statusTable);
                    }
                }


            }
            finally
            {
                CloseConnection();
            }
            return;
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
    }
}
