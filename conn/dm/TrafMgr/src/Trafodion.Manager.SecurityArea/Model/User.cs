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
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.SecurityArea.Model
{
    public class User : SecurityObject
    {
        #region Member variables
        //A delegate that will get called when a call happens to the Trafodion backend
        public delegate void SecurityBackendOperation(object sender, EventArgs e);
        public event SecurityBackendOperation OnSecurityBackendOperation;

        //the bits for the bit vector
        public const int Password_Bit               = 1 << 0;
        public const int ExpirationDays_Bit         = 1 << 1;
        public const int ExpirationDate_Bit         = 1 << 2;
        public const int Default_Subvolume_Bit      = 1 << 3;
        public const int InitialDirectory_Bit       = 1 << 4;
        public const int DefaultSecurity_Bit        = 1 << 5;

        public enum UserTypeEnum { PlatformUser=2, DBUser , PlatformDBUser };
        public const string SuccessStatus = "Success";
        public const string FailureStatus = "Failure";

        public const string Status = "Status";
        public const string Operation = "Operation";
        public string[] StatusColumns = { Operation, Status, "Message" };
        public enum EditMode { Create, CreateLike, Update, GrantRevokeRole };
        #endregion

        #region private variables
        string  _userName;
        string  _defaultRole;
        List<string> _additionalRoles = new List<string>();
        UserTypeEnum _userType;
        int     _attributeVector;
        string  _password;
        int     _expirationDays;
        string  _expirationDate;
        string  _defaultSubvolume;
        string  _initialDirectory;
        string  _defaultSecurity;

        //read only attributes returned by server
        string _creationTime;
        string _creatingUser;
        string _lastDbLogin;

        //state variables
        bool _populated = false;


        #endregion private variables

        #region Properties

        public string UserName
        {
            get { return _userName; }
            set { _userName = value; }
        }

        public string DefaultRole
        {
            get { return _defaultRole; }
            set { _defaultRole = value; }
        }

        public List<string> AdditionalRoles
        {
            get { return _additionalRoles; }
            set { _additionalRoles = value; }
        }

        public UserTypeEnum UserType
        {
            get { return _userType; }
            set { _userType = value; }
        }

        public int AttributeVector
        {
            get { return _attributeVector; }
            set { _attributeVector = value; }
        }

        public string Password
        {
            get { return _password; }
            set { _password = value; }
        }

        public string EncryptedPassword
        {
            get 
            {
                return _password; 
            }
        }

        public int ExpirationDays
        {
            get { return _expirationDays; }
            set { _expirationDays = value; }
        }

        public string ExpirationDate
        {
            get { return _expirationDate; }
            set { _expirationDate = value; }
        }
        public string DefaultSubvolume
        {
            get { return _defaultSubvolume; }
            set { _defaultSubvolume = value; }
        }

        public string InitialDirectory
        {
            get { return _initialDirectory; }
            set { _initialDirectory = value; }
        }

        public string DefaultSecurity
        {
            get { return _defaultSecurity; }
            set { _defaultSecurity = value; }
        }


        public string LastDbLogin
        {
            get { return _lastDbLogin; }
            set { _lastDbLogin = value; }
        }
        public string CreationTime
        {
            get { return _creationTime; }
            set { _creationTime = value; }
        }

        public string CreatingUser
        {
            get { return _creatingUser; }
            set { _creatingUser = value; }
        }

        #endregion public variables

        #region Constructors
        public User(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }

        public User(ConnectionDefinition aConnectionDefinition, string aUserName) 
            : base(aConnectionDefinition)
        {
            _userName = aUserName;
        }

        public User(User aUser) : this(aUser.ConnectionDefinition, aUser.UserName)
        {
            copyUser(aUser, this);
        }

        #endregion

        #region Public methods
        public void Refresh()
        {
            _populated = false;
            User aUser = this.GetUser(UserName);
            copyUser(aUser, this);
        }

        //Obtains the user details in an async manner
        //public void GetUserDetails()
        //{
        //    if (_populated)
        //    {

        //    }
        //    else
        //    {
        //        DatabaseDataProviderConfig _dbConfig = new DatabaseDataProviderConfig();
        //        _dbConfig.SQLText = "SELECT USER_NAME, ROLE_NAME FROM Manageability.user_management.userdata ORDER BY USER_NAME ASC FOR BROWSE ACCESS";
        //        _dbConfig.TimerPaused = true;
        //        _dbConfig.RefreshRate = 0;
        //        _dbConfig.ConnectionDefinition = this.ConnectionDefinition;

        //        DatabaseDataProvider dbDataProvider = new DatabaseDataProvider(_dbConfig);
        //        dbDataProvider.OnNewDataArrived += invoke_userDetailsObtained;
        //        dbDataProvider.OnErrorEncountered += invoke_errorEncounteredWhileGettingUserData;
        //    }
        //}

        public void AddAdditionalRole(string aRole)
        {
            if (_additionalRoles == null)
            {
                _additionalRoles = new List<string>();
            }
            //TODO: deal with case sensitivity
            if (!_additionalRoles.Contains(aRole))
            {
                _additionalRoles.Add(aRole);
            }
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

        public static string GetUserTypeString(UserTypeEnum userType)
        {
            string val = "";
            switch (userType)
            {
                case UserTypeEnum.DBUser:
                    val = "Remotely Authenticated";
                    break;
                case UserTypeEnum.PlatformDBUser:
                    val = "Locally Authenticated";
                    break;
                case UserTypeEnum.PlatformUser:
                    val = "Locally Authenticated";
                    break;
            }
            return val;
        }

        /// <summary>
        /// Computes the attribute vector for the parameters of a user for add
        /// </summary>
        /// <param name="aUser"></param>
        /// <returns></returns>
        public static int GetAttributeVectorForAdd(User aUser)
        {
            int attrVector = 0;
            if ((aUser.UserType == UserTypeEnum.PlatformUser) || (aUser.UserType == UserTypeEnum.PlatformDBUser))
            {
                SystemSecurity systemSecurity = SystemSecurity.FindSystemModel(aUser.ConnectionDefinition);
                Policies thePolicies = systemSecurity.Policies;
                attrVector = ((aUser.Password != null) && (aUser.Password.Length > 0)) ? (attrVector | Password_Bit) : attrVector;
                attrVector = (aUser.ExpirationDays != thePolicies.PwdDefaultExprDays) ? attrVector | ExpirationDays_Bit : attrVector;
                attrVector = AreDatesDifferent(aUser.ExpirationDate, thePolicies.PwdDefaultExprDate) ? (attrVector | ExpirationDate_Bit) : attrVector;
           }
            if (aUser.UserType == UserTypeEnum.PlatformUser)
            {
                attrVector = ((aUser.DefaultSubvolume != null) && (aUser.DefaultSubvolume.Length > 0)) ? (attrVector | Default_Subvolume_Bit) : attrVector;
                attrVector = ((aUser.InitialDirectory != null) && (aUser.InitialDirectory.Length > 0)) ? (attrVector | InitialDirectory_Bit) : attrVector;
                attrVector = ((aUser.DefaultSecurity != null) && (aUser.DefaultSecurity.Length > 0)) ? (attrVector | DefaultSecurity_Bit) : attrVector;
            }
            return attrVector;
        }

        /// <summary>
        /// Computes the attribute vector for the parameters of alter
        /// </summary>
        /// <param name="originalUser"></param>
        /// <param name="changedUser"></param>
        /// <returns></returns>
        public static int GetAttributeVectorForAlter(User originalUser, User changedUser)
        {
            int attrVector = 0;

            if ((changedUser.UserType == UserTypeEnum.PlatformUser) || (changedUser.UserType == UserTypeEnum.PlatformDBUser))
            {
                attrVector = (changedUser.Password.Length > 0) ? (attrVector | Password_Bit) : attrVector;
                attrVector = (originalUser.ExpirationDays != changedUser.ExpirationDays) ? (attrVector | ExpirationDays_Bit) : attrVector;
                //attrVector = (!originalUser.ExpirationDate.Equals(changedUser.ExpirationDate)) ? (attrVector | ExpirationDate_Bit) : attrVector;
                attrVector = AreDatesDifferent(originalUser.ExpirationDate, changedUser.ExpirationDate) ? (attrVector | ExpirationDate_Bit) : attrVector;
            }
            if (changedUser.UserType == UserTypeEnum.PlatformUser)
            {
                attrVector = (!originalUser.DefaultSubvolume.Equals(changedUser.DefaultSubvolume)) ? (attrVector | Default_Subvolume_Bit) : attrVector;
                attrVector = (!originalUser.InitialDirectory.Equals(changedUser.InitialDirectory)) ? (attrVector | InitialDirectory_Bit) : attrVector;
                attrVector = (!originalUser.DefaultSecurity.Equals(changedUser.DefaultSecurity)) ? (attrVector | DefaultSecurity_Bit) : attrVector;
            }

            return attrVector;
        }

        public static bool AreDatesDifferent(string sourceDateString, string targetDateString)
        {
            DateTime sourceDate;
            DateTime targetDate;

            DateTime.TryParse(sourceDateString, out sourceDate);
            DateTime.TryParse(targetDateString, out targetDate);

            return (sourceDate != targetDate);
        }

        /// <summary>
        /// Default get properties with populatePlatforProperties set to true
        /// </summary>
        /// <param name="aUserName"></param>
        /// <returns></returns>
        public User GetUser(string aUserName)
        {
            return GetUserWithDetails(aUserName, true);
        }
        /// <summary>
        /// This method returns a fully populated user given the user Name
        /// 
        /// </summary>
        /// <returns></returns>
        public User GetUserWithDetails(string aUserName, bool populatePlatforProperties)
        {
            OdbcDataReader dsReader = null;
            User user = new User(ConnectionDefinition, aUserName);
            if (!GetConnection())
                return null;

            try
            {
                dsReader = Queries.GetUser(CurrentConnection, aUserName);
                bool gotData = false;
                while (dsReader.Read())
                {
                    gotData = true;
                    //set the user type
                    user.UserType = (UserTypeEnum)dsReader["USER_TYPE"];

                    //Set the roles
                    string isPrimary = ((string)dsReader["IS_PRIMARY"]).Trim();
                    if (isPrimary.Equals("Y", StringComparison.CurrentCultureIgnoreCase))
                    {
                        user.DefaultRole = ((string)dsReader["ROLE_NAME"]).Trim();
                    }
                    else
                    {
                        user.AddAdditionalRole(((string)dsReader["ROLE_NAME"]).Trim());
                    }
                }

                //if we did not get any data for the user, throw an exception
                if (!gotData)
                {
                    throw new Exception("No data found.");
                }

                //Get the properties of the platform user
                if (populatePlatforProperties)
                {
                    if ((user.UserType == UserTypeEnum.PlatformDBUser) || (user.UserType == UserTypeEnum.PlatformUser))
                    {
                        Queries.ExecuteGetSysAttributeForUser(CurrentConnection, user);
                    }
                }
            }
            catch (Exception ex)
            {
                throw new Exception(string.Format("Error encountered while obtaining details for user {0} : {1}", aUserName, ex.Message));
            }
            finally
            {
                CloseConnection();
            }
            return user;
        }


        /// <summary>
        /// This method adds the user passed and returns a data table that has the status of the operation
        /// Note: The User must be populated prior to the call.
        /// </summary>
        /// <returns></returns>
        public DataTable AddUser(User aUser)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable(string.Format("Failed to add user {0}", aUser.UserName), FailureStatus, "Unable to get connection", statusTable);
                }
                //Notify that a long operation is about to start and specify the number of steps involved
                FireStartUserEvent((aUser.AdditionalRoles.Count + 1));
                int result = -1;

                //Add User
                if (aUser.UserType == UserTypeEnum.DBUser)
                {
                    result = Queries.ExecuteAddOffPlatformUser(CurrentConnection, aUser);
                }
                else if ((aUser.UserType == UserTypeEnum.PlatformDBUser) || (aUser.UserType == UserTypeEnum.PlatformUser))
                {
                    result = Queries.ExecuteAddPlatformUser(CurrentConnection, aUser);
                }
                AddStatusToTable(string.Format("Add user {0}", aUser.UserName), SuccessStatus, "", statusTable);

                //Associate Roles - DB Users can have multiple roles
                if ((aUser.UserType == UserTypeEnum.DBUser) || (aUser.UserType == UserTypeEnum.PlatformDBUser))
                {
                    foreach (string role in aUser.AdditionalRoles)
                    {
                        try
                        {
                            Queries.ExecuteGrantRole(CurrentConnection, aUser.UserName, role);
                            AddStatusToTable(string.Format("Role {0} has been granted to user {1}", role, aUser.UserName), SuccessStatus, "", statusTable);
                        }
                        catch (Exception ex)
                        {
                            AddStatusToTable(string.Format("Failed to grant role {0} to user {1}", role, aUser.UserName), FailureStatus, ex.Message, statusTable);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                AddStatusToTable(string.Format("Failed to add user {0}", this.UserName), FailureStatus, ex.Message, statusTable);
            }
            finally
            {
                FireEndUserEvent();
                CloseConnection();
            }
            return statusTable;
        }

        /// <summary>
        /// This method adds the user passed and returns a data table that has the status of the operation
        /// Note: The User must be populated prior to the call.
        /// </summary>
        /// <returns></returns>
        public DataTable AddMultipleDBUsers(List<User> users)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable("Failed to add users", FailureStatus, "Unable to get connection", statusTable);
                }

                //Notify that a long operation is about to start and specify the number of steps involved
                int steps = 0;
                if (users.Count > 0)
                {
                    steps = (users[0].AdditionalRoles.Count + 1) * users.Count;
                }
                FireStartUserEvent(steps);

                foreach (User aUser in users)
                {
                    try
                    {
                        if (aUser.UserType == UserTypeEnum.DBUser)
                        {
                            Queries.ExecuteAddOffPlatformUser(CurrentConnection, aUser);
                            AddStatusToTable(string.Format("Add user {0}", aUser.UserName), SuccessStatus, "", statusTable);

                            foreach (string role in aUser.AdditionalRoles)
                            {
                                try
                                {
                                    Queries.ExecuteGrantRole(CurrentConnection, aUser.UserName, role);
                                    AddStatusToTable(string.Format("Role {0} has been granted to user {1}", role, aUser.UserName), SuccessStatus, "", statusTable);
                                }
                                catch (Exception ex)
                                {
                                    AddStatusToTable(string.Format("Failed to grant role {0} to user {1}", role, aUser.UserName), FailureStatus, ex.Message, statusTable);
                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(string.Format("Failed to add user {0}", this.UserName), FailureStatus, ex.Message, statusTable);
                    }
                }
            }
            finally
            {
                FireEndUserEvent();
                CloseConnection();
            }
            return statusTable;
        }

        /// <summary>
        /// This method takes in the original user and the user object whose attributes have been 
        /// changed from the UI. It checks checks for the changes and makes the appropriate backend
        /// calls to commit those changes.
        /// 
        /// </summary>
        /// <param name="originalUser"></param>
        /// <param name="changedUser"></param>
        /// <returns></returns>
        public DataTable AlterUser(User originalUser, User changedUser)
        {
            SanitizeUser(originalUser);
            SanitizeUser(changedUser);
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable(string.Format("Failed to alter user {0}", changedUser.UserName), FailureStatus, "Unable to get connection", statusTable);
                }

                //Original user and Changed User must have the same names
                if (!originalUser.UserName.Equals(changedUser.UserName))
                {
                    AddStatusToTable(string.Format("The user name cannot be changed. Original Name {0} and new name is {1}", originalUser.UserName, changedUser.UserName), FailureStatus, "", statusTable);
                    return statusTable;
                }

                //Fetch the existing user details from the server

                //Notify that a long operation is about to start and specify the number of steps involved
                FireStartUserEvent(StepsForGrantRevokeRole(originalUser, changedUser) + 1);

                //Check to see if any attributes have changed and invoke the alterUserSPJ if needed
                AlterUserAttributes(originalUser, changedUser, statusTable);

                //Grant new roles and Revoke roles that have been removed from the additional roles list
                GrantRevokeUserRoles(originalUser, changedUser, statusTable);

                //Set the default role if needed
                SetDefaultRole(originalUser, changedUser, statusTable);
            }
            catch (Exception ex)
            {
                AddStatusToTable(string.Format("Failed to alter user {0}", changedUser.UserName), FailureStatus, ex.Message, statusTable);
            }
            finally
            {
                FireEndUserEvent();
                CloseConnection();
            }
            return statusTable;
        }

        /// <summary>
        /// This method takes in the list of user names and deletes each one of them.
        /// 
        /// </summary>
        /// <param name="originalUser"></param>
        /// <param name="changedUser"></param>
        /// <returns></returns>
        public DataTable DeleteUsers(List<string> userNames)
        {
            DataTable statusTable = GetStatusDataTable();
            try
            {
                if (!GetConnection())
                {
                    AddStatusToTable("Failed to delete users", FailureStatus, "Unable to get connection", statusTable);
                }

                //Notify that a long operation is about to start and specify the number of steps involved
                FireStartUserEvent(userNames.Count);

                foreach (string userName in userNames)
                {
                    try
                    {
                        //revoke the role of the user
                        Queries.ExecuteDeleteUser(CurrentConnection, userName);
                        AddStatusToTable(string.Format("Delete user {0}", userName), SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(string.Format("Failed to delete user {0}", userName), FailureStatus, ex.Message, statusTable);
                    }
                }

            }
            catch (Exception ex)
            {
                AddStatusToTable("Failed to delete users", FailureStatus, ex.Message, statusTable);
            }
            finally
            {
                FireEndUserEvent();
                CloseConnection();
            }
            return statusTable;
        }

        //Helper method to check if all operations were successful
        public bool AreAllOperationsSuccessful(DataTable aDataTable)
        {
            foreach (DataRow dr in aDataTable.Rows)
            {
                if ( ((string)dr[Status]).Equals(FailureStatus))
                {
                    return false;
                }
            }
            return true;
        }

        //Helper method to check if all operations with the name specified were successful
        public bool AreAllOperationsSuccessful(DataTable aDataTable, string aOperationName)
        {
            foreach (DataRow dr in aDataTable.Rows)
            {
                string op = (string)dr[Operation];
                string status = (string)dr[Status];

                if ((op.IndexOf(aOperationName, StringComparison.InvariantCultureIgnoreCase) >= 0) && (status.Equals(FailureStatus)))
                {
                    return false;
                }
            }
            return true;
        }

        //Helper method to check if some operations with the name specified were successful
        public int GetOperationFailureCount(DataTable aDataTable, string aOperationName)
        {
            int ret = 0;
            foreach (DataRow dr in aDataTable.Rows)
            {
                string op = (string)dr[Operation];
                string status = (string)dr[Status];

                if ((op.IndexOf(aOperationName, StringComparison.InvariantCultureIgnoreCase) >= 0) && (status.Equals(FailureStatus)))
                {
                    ret++;
                }
            }
            return ret;
        }

        //This method will get the list of roles that can be used to add or alter the roles of the user passed by the
        //user name.
        public List<string> GetRolesForOperation(string currentRole, string userName, string operation)
        {
            List<string> ret = new List<string>();
            if (!GetConnection())
            {
                throw new Exception("Unable to get connection to obtain the list of roles");
            }

            OdbcDataReader dataReader = Queries.GetRolesForOperation(CurrentConnection, currentRole, userName, operation);
            while (dataReader.Read())
            {
                ret.Add(dataReader.GetString(0).Trim());
            }
            return ret;
        }

        public void SetDefaultRole(string aUserName, string aRoleName)
        {
            if (!GetConnection())
            {
                throw new Exception("Unable to get connection to obtain the list of roles");
            }

            Queries.ExecuteSetDefaultRole(CurrentConnection, aUserName, aRoleName);
        }

        #endregion

        #region Private methods
        //Helper method to alter the user attributes
        private void AlterUserAttributes(User originalUser, User changedUser, DataTable statusTable)
        {
            if ((originalUser.UserType == UserTypeEnum.PlatformUser) || (originalUser.UserType == UserTypeEnum.PlatformDBUser))
            {
                int arrtibuteForAlter = GetAttributeVectorForAlter(originalUser, changedUser);
                if (arrtibuteForAlter > 0)
                {
                    try
                    {
                        //invoke alter user
                        Queries.ExecuteAlterPlatformUser(CurrentConnection, changedUser, arrtibuteForAlter);
                        AddStatusToTable(string.Format("Alter user {0}", changedUser.UserName), SuccessStatus, "", statusTable);
                    }
                    catch (Exception ex)
                    {
                        AddStatusToTable(string.Format("Failed to alter user {0}", changedUser.UserName), FailureStatus, ex.Message, statusTable);
                    }
                }
            }
        }


        private int StepsForGrantRevokeRole(User originalUser, User changedUser)
        {
            int steps = 0;
            //Only DB users have additional roles
            if ((originalUser.UserType == UserTypeEnum.DBUser) || (originalUser.UserType == UserTypeEnum.PlatformDBUser))
            {
                //Check to see if any roles need to be revoked
                List<string> rolesToRevoke = getRolesToRevoke(originalUser, changedUser);
                steps += rolesToRevoke.Count;

                //Check to see if new roles need to be granted to the user
                List<string> rolesToGrant = getRolesToGrant(originalUser, changedUser);
                steps += rolesToGrant.Count;

                //Check to see if a change in default role needed
                if (IsSetDefaultRoleNeeded(originalUser, changedUser))
                {
                    steps++;
                }
            }
            return steps;
        }

        //Helper method to grant and revoke role to users during alter
        private void GrantRevokeUserRoles(User originalUser, User changedUser, DataTable statusTable)
        {
            //Only DB users have additional roles
            if ((originalUser.UserType == UserTypeEnum.DBUser) || (originalUser.UserType == UserTypeEnum.PlatformDBUser))
            {
                //Check to see if any roles need to be revoked
                List<string> rolesToRevoke = getRolesToRevoke(originalUser, changedUser);
                if (rolesToRevoke.Count > 0)
                {
                    foreach (string role in rolesToRevoke)
                    {
                        try
                        {
                            //revoke the role of the user
                            Queries.ExecuteRevokeRole(CurrentConnection, changedUser.UserName, role);
                            AddStatusToTable(string.Format("Revoked role {0} of user {1}", role, changedUser.UserName), SuccessStatus, "", statusTable);
                        }
                        catch (Exception ex)
                        {
                            AddStatusToTable(string.Format("Failed to revoke role {0} of user {1}", role, changedUser.UserName), FailureStatus, ex.Message, statusTable);
                        }
                    }
                }

                //Check to see if new roles need to be granted to the user
                List<string> rolesToGrant = getRolesToGrant(originalUser, changedUser);
                if (rolesToGrant.Count > 0)
                {
                    foreach (string role in rolesToGrant)
                    {
                        try
                        {
                            //grant a role to the user
                            Queries.ExecuteGrantRole(CurrentConnection, changedUser.UserName, role);
                            AddStatusToTable(string.Format("Granted role {0} to user {1}", role, changedUser.UserName), SuccessStatus, "", statusTable);
                        }
                        catch (Exception ex)
                        {
                            AddStatusToTable(string.Format("Failed to grant role {0} to user {1}", role, changedUser.UserName), FailureStatus, ex.Message, statusTable);
                        }
                    }
                }
            }
        }

        //Helper method to set the default role of a user
        private void SetDefaultRole(User originalUser, User changedUser, DataTable statusTable)
        {
            if (IsSetDefaultRoleNeeded(originalUser, changedUser))
            {
                try
                {
                    if (changedUser.AdditionalRoles.Contains(changedUser.DefaultRole) || originalUser.AdditionalRoles.Contains(changedUser.DefaultRole))
                    {
                        //call set default role
                        Queries.ExecuteSetDefaultRole(CurrentConnection, changedUser.UserName, changedUser.DefaultRole);
                        AddStatusToTable(string.Format("Set default role {0} to user {1}", changedUser.DefaultRole, changedUser.UserName), SuccessStatus, "", statusTable);
                    }
                    else
                    {
                        //call grant default role
                        Queries.ExecuteGrantDefaultRole(CurrentConnection, changedUser.UserName, changedUser.DefaultRole);
                        AddStatusToTable(string.Format("Granted default role {0} to user {1}", changedUser.DefaultRole, changedUser.UserName), SuccessStatus, "", statusTable);
                    }
                }
                catch (Exception ex)
                {
                    AddStatusToTable(string.Format("Failed to grant default role {0} to user {1}", changedUser.DefaultRole, changedUser.UserName), FailureStatus, ex.Message, statusTable);
                }
            }
        }

        //check if change to default role is needed
        private bool IsSetDefaultRoleNeeded(User originalUser, User changedUser)
        {
            bool ret = false;
            if ((originalUser.UserType == UserTypeEnum.DBUser) || (originalUser.UserType == UserTypeEnum.PlatformDBUser))
            {
                if (!originalUser.DefaultRole.Equals(changedUser.DefaultRole))
                {
                    ret = true;
                }
            }
            return ret;
        }

        private void FireStartUserEvent(int totalCount)
        {
            if (OnSecurityBackendOperation != null)
            {
                UserEventArgs evtArgs = new UserEventArgs("", "", "");
                evtArgs.Type = UserEventArgs.EventType.EventStart;
                evtArgs.EventCount = totalCount;
                OnSecurityBackendOperation(this, evtArgs);
            }
        }


        private void FireUserEvent(string anOperation, string aStatus, string aMessage)
        {
            if (OnSecurityBackendOperation != null)
            {
                UserEventArgs evtArgs = new UserEventArgs(anOperation, aStatus, aMessage);
                evtArgs.Type = UserEventArgs.EventType.EventProgress;
                OnSecurityBackendOperation(this, evtArgs);
            }
        }

        private void FireEndUserEvent()
        {
            if (OnSecurityBackendOperation != null)
            {
                UserEventArgs evtArgs = new UserEventArgs("", "", "");
                evtArgs.Type = UserEventArgs.EventType.EventEnd;
                OnSecurityBackendOperation(this, evtArgs);
            }
        }


        private DataTable GetStatusDataTable()
        {
            DataTable table = new DataTable();
            foreach (string column in StatusColumns)
            {
                table.Columns.Add(column);
            }
            return table;
        }

        private void AddStatusToTable(string anOperation, string aStatus, string aMessage, DataTable aTable)
        {
            aTable.Rows.Add(new object[] { anOperation, aStatus, aMessage });
            FireUserEvent(anOperation, aStatus, aMessage);
        }

        private void SanitizeUser(User aUser)
        {
            aUser.UserName = (aUser.UserName == null) ? "" : aUser.UserName.Trim().ToUpper();            
            aUser.DefaultRole = (aUser.DefaultRole == null) ? "" : aUser.DefaultRole.Trim().ToUpper();
            
            if (aUser.AdditionalRoles == null)
            {
                aUser.AdditionalRoles = new List<string>();
            }            
            //Trim, uppercase and remove duplicates
            List<string> tempRoles = aUser.AdditionalRoles;
            aUser.AdditionalRoles = new List<string>();
            foreach (string role in tempRoles)
            {
                aUser.AddAdditionalRole(role.Trim().ToUpper());
            }

            aUser.ExpirationDate = (aUser.ExpirationDate == null) ? "" : aUser.ExpirationDate.Trim().ToUpper();
            aUser.DefaultSubvolume = (aUser.DefaultSubvolume == null) ? "" : aUser.DefaultSubvolume.Trim().ToUpper();
            aUser.InitialDirectory = (aUser.InitialDirectory == null) ? "" : aUser.InitialDirectory.Trim();
            aUser.DefaultSecurity = (aUser.DefaultSecurity == null) ? "" : aUser.DefaultSecurity.Trim().ToUpper();

        }

        //helper to obtain the list of roles to be revoked for a user
        private List<string> getRolesToRevoke(User originalUser, User changedUser)
        {
            List<string> list = new List<string>();
            foreach (string role in originalUser.AdditionalRoles)
            {
                if ((!changedUser.AdditionalRoles.Contains(role)) && (!role.Equals(changedUser.DefaultRole)))
                {
                    list.Add(role);
                }
            }
            return list;
        }        
        
        //helper to obtain the list of roles to be granted to the user
        private List<string> getRolesToGrant(User originalUser, User changedUser)
        {
            List<string> list = new List<string>();
            foreach (string role in changedUser.AdditionalRoles)
            {
                if ((!originalUser.AdditionalRoles.Contains(role)) && (!role.Equals(originalUser.DefaultRole)))
                {
                    list.Add(role);
                }
            }
            return list;
        }


        private void copyUser(User sourceUser, User destinationUser)
        {
            destinationUser.DefaultRole = sourceUser.DefaultRole;
            destinationUser.UserType = sourceUser.UserType;
            destinationUser.AttributeVector = sourceUser.AttributeVector;
            destinationUser.Password = sourceUser.Password;
            destinationUser.ExpirationDays = sourceUser.ExpirationDays;
            destinationUser.ExpirationDate = sourceUser.ExpirationDate;
            destinationUser.DefaultSubvolume = sourceUser.DefaultSubvolume;
            destinationUser.InitialDirectory = sourceUser.InitialDirectory;
            destinationUser.DefaultSecurity = sourceUser.DefaultSecurity;
            foreach (string role in sourceUser.AdditionalRoles)
            {
                destinationUser.AddAdditionalRole(role);
            }
        }
        #endregion
    }

    public class UserEventArgs : EventArgs
    {
        public enum EventType {EventStart, EventProgress, EventEnd};

        string _Operation;
        string _Status;
        string _Message;
        EventType _Type;
        int _eventCount;


        public UserEventArgs(string anOperation, string aStatus, string aMessage)
        {
            this._Operation = anOperation;
            this._Status = aStatus;
            this._Message = aMessage;
        }


        public string Operation
        {
            get { return _Operation; }
            set { _Operation = value; }
        }

        public string Status
        {
            get { return _Status; }
            set { _Status = value; }
        }

        public string Message
        {
            get { return _Message; }
            set { _Message = value; }
        }

        public EventType Type
        {
            get { return _Type; }
            set { _Type = value; }
        }

        public int EventCount
        {
            get { return _eventCount; }
            set { _eventCount = value; }
        }
    }
}
