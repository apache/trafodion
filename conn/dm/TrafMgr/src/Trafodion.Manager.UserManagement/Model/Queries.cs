//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.UserManagement.Model

{
    /// <summary>
    /// User management queries
    /// </summary>
    static public class Queries
    {
        public const string TRACE_SUB_AREA_NAME = "UserManagement Queries";

        /// <summary>
        /// register user based on the user mapping.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserMapping"></param>
        /// <returns></returns>
        static public int RegisterUser(Connection aConnection, string[] aUserMapping)
        {
            StringBuilder sbRegistryString = new StringBuilder();
            sbRegistryString.Append("REGISTER USER ");
            sbRegistryString.Append(Utilities.ExternalUserName(aUserMapping[0]));
            sbRegistryString.Append(" AS ");
            sbRegistryString.Append(Utilities.ExternalUserName(aUserMapping[1]));
            sbRegistryString.Append(" LOGON ROLE ");
            sbRegistryString.Append(Utilities.ExternalUserName(aUserMapping[2]));
            sbRegistryString.Append(" ");
            sbRegistryString.Append(aUserMapping[3]);
            sbRegistryString.Append(" ");
            sbRegistryString.Append(aUserMapping[4]);
            OdbcCommand theQuery = new OdbcCommand(sbRegistryString.ToString());
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// un register user from data base
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <returns></returns>
        static public int UnRegisterUser(Connection aConnection, string aUserName)
        {
            string unRegistryString = "UNREGISTER USER " + Utilities.ExternalUserName(aUserName);

            OdbcCommand theQuery = new OdbcCommand(unRegistryString);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Execute a SQL command
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="command"></param>
        /// <returns></returns>
        static public int ExecuteNonQueryCommand(Connection aConnection, string command)
        {
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static private int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UserManagement, TRACE_SUB_AREA_NAME, false);
        }

        static private OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UserManagement, TRACE_SUB_AREA_NAME, false);
        }

        #region RoleManagement

        /// <summary>
        /// Get User details from user_infor table.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <returns></returns>
        static public OdbcDataReader GetUserDetails(Connection aConnection, string aUserName)
        {
            string query = String.Format("SELECT USER_NAME,USER_ID,USER_CREATOR,USER_CREATOR_NAME,LOGON_ROLE_ID,LOGON_ROLE_ID_NAME,EXTERNAL_USER_NAME,USER_VERSION,CREATE_TIME,REDEF_TIME,IS_AUTO_REGISTERED,IS_VALID_USER FROM TRAFODION_INFORMATION_SCHEMA.USER_INFO WHERE USER_NAME = '{0}' FOR BROWSE ACCESS", aUserName.ToUpper());
            OdbcCommand theQuery = new OdbcCommand(query);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }

        /// <summary>
        /// Get All roles assigned on a user name from USER_ROLE_INFO view.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <returns></returns>
        static public OdbcDataReader GetRolesByUserName(Connection aConnection, string aUserName)
        {
            //String.Format("SELECT ROLE_NAME,ROLE_ID FROM TRAFODION_INFORMATION_SCHEMA.USER_ROLE_INFO LEFT JOIN TRAFODION_INFORMATION_SCHEMA.USER_INFO ON GRANTEE_ID=USER_ID WHERE USER_NAME= '{0}' FOR BROWSE ACCESS", aUserName.ToUpper()); 
            
            string query = String.Format("SELECT LOGON_ROLE_ID_NAME, ROLE_NAME, ROLE_ID " +
                                            "FROM TRAFODION_INFORMATION_SCHEMA.USER_INFO " +
                                            "LEFT JOIN TRAFODION_INFORMATION_SCHEMA.USER_ROLE_INFO " +
                                            "ON USER_ID = GRANTEE_ID " +
                                            "WHERE USER_NAME = '{0}' " +
                                            "FOR READ UNCOMMITTED ACCESS", aUserName.ToUpper());

            OdbcCommand theQuery = new OdbcCommand(query);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }


        static public OdbcDataReader GetUsersByRoleName(Connection aConnection, string aRoleName)
        {
            //Fetch data SQL
            StringBuilder sbSql = new StringBuilder();
            sbSql.Append(" SElECT 	USER_NAME,");
            sbSql.Append("		USER_ID,");
            sbSql.Append("		EXTERNAL_USER_NAME, ");
            sbSql.Append("		CREATE_TIME, ");
            sbSql.Append("		REDEF_TIME, ");
            sbSql.Append("		IS_AUTO_REGISTERED, ");
            sbSql.Append("		IS_VALID_USER ");
            sbSql.Append(" FROM TRAFODION_INFORMATION_SCHEMA.USER_ROLE_INFO R LEFT JOIN TRAFODION_INFORMATION_SCHEMA.USER_INFO U ");
            sbSql.Append(" ON R.GRANTEE_ID=U.USER_ID ");
            sbSql.Append(string.Format("WHERE R.ROLE_NAME='{0}'", aRoleName.Trim()));
            sbSql.Append(" AND R.GRANTEE_ID IN (SELECT USER_ID FROM TRAFODION_INFORMATION_SCHEMA.USER_INFO) ");
            sbSql.Append(" ORDER BY U.USER_NAME FOR READ UNCOMMITTED ACCESS ");

            OdbcCommand theQuery = new OdbcCommand(sbSql.ToString());
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }

        /// <summary>
        /// Add a role
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="roleName"></param>
        /// <returns></returns>
        static public int ExecuteAddRole(Connection aConnection, string roleName)
        {
            string cmd = "CREATE ROLE " + roleName;

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }


        /// <summary>
        /// Drop a role according to role name[RESTRICT | CASCADE]
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="roleName"></param>
        /// <param name="isCascade"></param>
        /// <returns></returns>
        static public int ExecutDropRole(Connection aConnection, string roleName, bool isCascade)
        {
            const string DROP_COMMAND_TEXT = "DROP ROLE {0} {1}";

            string externalRoleName = Utilities.ExternalUserName(roleName);
            string cascadeOption = isCascade ? "CASCADE" : string.Empty;
            string cmdText = string.Format(DROP_COMMAND_TEXT, externalRoleName, cascadeOption);

            OdbcCommand command = new OdbcCommand(cmdText);

            command.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(command);
        }

        /// <summary>
        /// Grant a role to a user
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="roleName"></param>
        /// <returns></returns>
        static public int GrantRoleToUser(Connection aConnection, string roleName,string userName)
        {
            string cmd = " GRANT ROLE " + roleName + " TO " + userName;

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Revoke a role From a user
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="roleName"></param>
        /// <returns></returns>
        static public int RevokeRoleFromUser(Connection aConnection, string roleName, string userName)
        {
            string cmd = "REVOKE ROLE " + roleName + " FROM " + userName;

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }


        /// <summary>
        /// Revoke an existing role of the user
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="aRoleName"></param>
        /// <returns></returns>
        static public int ExecuteRevokeRole(Connection aConnection, string aUserName, string aRoleName)
        {

            string cmd = "REVOKE ROLE " + Utilities.ExternalUserName(aRoleName) + " FROM " + Utilities.ExternalUserName(aUserName);

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Set Primary role for a user
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="aRoleName"></param>
        /// <returns></returns>
        static public int SetPrimaryRole(Connection aConnection, string aUserName, string aRoleName)
        {

            string cmd = "ALTER USER  " + Utilities.ExternalUserName(aUserName) + " SET LOGON ROLE " + Utilities.ExternalUserName(aRoleName);

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public int SetExternalName(Connection aConnection, string aUserName, string externalName)
        {

            string cmd = "ALTER USER  " + Utilities.ExternalUserName(aUserName) + " SET EXTERNAL NAME  " + Utilities.ExternalUserName(externalName);

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public int SetUserValid(Connection aConnection, string aUserName, string userValid)
        {

            string cmd = "ALTER USER  " + Utilities.ExternalUserName(aUserName) + " SET " + userValid;

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public int SetUserAuthenticationOption(Connection aConnection, string aUserName, string authenticationOption)
        {

            string cmd = string.Format("ALTER USER {0} SET {1}", Utilities.ExternalUserName(aUserName), authenticationOption);

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public int SetUserImmutable(Connection aConnection, string aUserName, string immutable)
        {

            string cmd = string.Format("ALTER USER {0} {1}", Utilities.ExternalUserName(aUserName), immutable);

            OdbcCommand theQuery = new OdbcCommand(cmd);

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }
  

        #endregion
    }
}
