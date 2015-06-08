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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;
using System.Text;

namespace Trafodion.Manager.SecurityArea.Model
{
    /// <summary>
    /// Security management queries
    /// </summary>
    static public class Queries
    {
        #region Fields
        
        private const string MANAGEABILITY_CATALOG = "MANAGEABILITY";
        private const string TRAFODION_CATALOG = "TRAFODION";
        private const string CONFIG_SCHEMA = "CONFIG_MANAGEMENT";
        private const string TRAFODION_SECURITY_SCHEMA = "TRAFODION_SECURITY";
        private const string TRAFODION_SPJ_SCHEMA = "TRAFODION_SPJ";
        private const string CONFIG_TABLE = "CONFIG";
        private const string CONFIG_TEXT_TABLE = "CONFIGTXT";
        private const string USERINFO_VIEW = "USERINFO";
        private const string RESET_ALL_POLICY = "RESETALL";
        private const string DEFAULT_RECORD = "DEFAULT";
        private const string BLANK = "";
        public const string TRACE_SUB_AREA_NAME = "Sec Queries";

        #endregion Fields

        #region Public methods

        /// <summary>
        /// to retrieve a configured directory server entry. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aDomainName"></param>
        /// <param name="aUsagePriority"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteInfoDirectoryServer(Connection aConnection, string aDomainName, int aUsagePriority)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT C.DOMAIN_NAME, C.USAGE_PRI, C.LDAPHOST, C.LDAPPORT, C.LDAPVERSION, C.SEARCHUSERDN, " + 
                              "C.SEARCHUSERPWD, C.LDAPENABLESSL, C.TIME_STAMP, C.CACERT, T.LINE_NUM, T.RAW_TEXT, T.PARAM_NAME, T.PARAM_VALUE " +
                              "FROM {0}.{1}.{2} as C " +
                              "LEFT JOIN {0}.{1}.{3} as T ON C.DOMAIN_NAME = T.DOMAIN_NAME AND C.USAGE_PRI = T.USAGE_PRI " + 
                              "WHERE C.DOMAIN_NAME= '{4}' AND C.USAGE_PRI = {5} ORDER BY T.LINE_NUM " +
                              "FOR READ UNCOMMITTED ACCESS", 
                              MANAGEABILITY_CATALOG, 
                              CONFIG_SCHEMA, 
                              CONFIG_TABLE,
                              CONFIG_TEXT_TABLE,
                              aDomainName,
                              aUsagePriority));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader retReader = ExecuteReader(theQuery);
            return retReader;
        }

        /// <summary>
        /// To retrieve a the config text for a specific directory server entry.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aDomainName"></param>
        /// <param name="aUsagePriority"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteInfoDirectoryServerConfigText(Connection aConnection, string aDomainName, int aUsagePriority)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT LINE_NUM, RAW_TEXT " +
                    "FROM {0}.{1}.{2} " +
                        "WHERE DOMAIN_NAME = '{3}' AND USAGE_PRI = {4} ORDER BY LINE_NUM " +
                        "FOR READ UNCOMMITTED ACCESS", MANAGEABILITY_CATALOG, CONFIG_SCHEMA, CONFIG_TEXT_TABLE, aDomainName, aUsagePriority));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader retReader = ExecuteReader(theQuery);
            return retReader;
        }

        /// <summary>
        /// To retrieve a the config parameters for a specific directory server entry.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aDomainName"></param>
        /// <param name="aUsagePriority"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteInfoDefaultDirectoryServerConfigParameters(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand(
                String.Format("SELECT LINE_NUM, PARAM_NAME, PARAM_VALUE FROM {0}.{1}.{2} " +
                              "WHERE DOMAIN_NAME = '{3}' AND USAGE_PRI = {4} ORDER BY LINE_NUM " +
                              "FOR READ UNCOMMITTED ACCESS", 
                              MANAGEABILITY_CATALOG, 
                              CONFIG_SCHEMA, 
                              CONFIG_TEXT_TABLE, 
                              DirectoryServer.DEFAULT_ENTRY_DOMAIN_NAME, 
                              DirectoryServer.DEFAULT_ENTRY_USAGE_PRIORITY));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader retReader = ExecuteReader(theQuery);
            return retReader;
        }

        /// <summary>
        /// To retrieve info for the logged on user. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <returns></returns>
        static public OdbcDataReader ExecuteInfoLoggedOnDBUser(Connection aConnection, string aUserName)
        {
            OdbcCommand theQuery = new OdbcCommand(
                // Remove the last login timestamp for performance concern.
                //String.Format("SELECT USER_NAME, DEFAULT_ROLE, LAST_DB_LOGIN, LOGIN_UTC_OFFSET " +
                  String.Format("SELECT USER_NAME, DEFAULT_ROLE " +
                        "FROM {0}.{1}.{2} " +
                        "WHERE USER_NAME = '{3}' " +
                        "FOR READ UNCOMMITTED ACCESS", 
                        new object[] { TRAFODION_CATALOG, TRAFODION_SECURITY_SCHEMA, USERINFO_VIEW, aUserName }));

            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader retReader = ExecuteReader(theQuery);
            return retReader;
        }

        /// <summary>
        /// To change self password.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="anOldPassword"></param>
        /// <param name="aNewPassword"></param>
        /// <returns></returns>
        static public int ExecuteChangePassword(Connection aConnection, string aUserName, string anOldPassword, string aNewPassword)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.CHANGEPASSWORD(?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = aUserName;
            OdbcParameter param2 = theQuery.Parameters.Add("@OLDPASSWORD", OdbcType.Text, 128);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.DbType = System.Data.DbType.String;
            param2.Value = anOldPassword;
            OdbcParameter param3 = theQuery.Parameters.Add("@NEWPASSWORD", OdbcType.Text, 128);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.DbType = System.Data.DbType.String;
            param3.Value = aNewPassword;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// To create a new directory server entry.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="server"></param>
        /// <returns></returns>
        static public int ExecuteCreateDirectoryServer(Connection aConnection, DirectoryServer server)
        {
            //string encryptedPassword = (server.SearchUserPassword.Length > 0) ? aConnection.EncryptPassword(server.SearchUserPassword) : "";
            //OdbcCommand theQuery = new OdbcCommand(
            //    String.Format("CALL {0}.{1}.LDAPCreateConfig('{2}', {3}, '{4}', {5}, {6}, " +
            //                  "'{7}', '{8}', {9},  '{10}',  '{11}')",
            //                  new object[] { MANAGEABILITY_CATALOG, 
            //                                 TRAFODION_SPJ_SCHEMA,
            //                                 server.DomainName, 
            //                                 server.UsagePriority, 
            //                                 server.HostName, 
            //                                 server.PortNumber, 
            //                                 server.Version, 
            //                                 server.SearchUserDN, 
            //                                 encryptedPassword, 
            //                                 server.EncryptionForSPJCall, 
            //                                 server.CACert, 
            //                                 server.ConfigText }));

            // We should have used the params, but we also need to make sure it is ISO encoding, so ...
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.LDAPCreateConfig(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@DOMAINNAME", OdbcType.Text, server.DomainName.Length);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = server.DomainName;

            OdbcParameter param2 = theQuery.Parameters.Add("@USAGEPRI", OdbcType.Int);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = server.UsagePriority;

            OdbcParameter param3 = theQuery.Parameters.Add("@THEHOST", OdbcType.Text, server.HostName.Length);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = server.HostName;

            OdbcParameter param4 = theQuery.Parameters.Add("@PORT", OdbcType.Int);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = server.PortNumber;

            OdbcParameter param5 = theQuery.Parameters.Add("@VERSION", OdbcType.Int);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = server.Version;

            OdbcParameter param6 = theQuery.Parameters.Add("@SEARCHUSERDN", OdbcType.Text, server.SearchUserDN.Length);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = server.SearchUserDN;

            string encryptedPassword = (server.SearchUserPassword.Length > 0) ? aConnection.EncryptPassword(server.SearchUserPassword) : "";
            OdbcParameter param7 = theQuery.Parameters.Add("@SEARCHUSERPWD", OdbcType.Text, encryptedPassword.Length);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = encryptedPassword;

            OdbcParameter param8 = theQuery.Parameters.Add("@SSL", OdbcType.SmallInt);
            param8.Direction = System.Data.ParameterDirection.Input;
            param8.Value = server.EncryptionForSPJCall;

            OdbcParameter param9 = theQuery.Parameters.Add("@CACERT", OdbcType.Text, server.CACert.Length);
            param9.Direction = System.Data.ParameterDirection.Input;
            param9.Value = server.CACert;

            OdbcParameter paramA = theQuery.Parameters.Add("@CONFIG", OdbcType.Text, server.ConfigText.Length);
            paramA.Direction = System.Data.ParameterDirection.Input;
            paramA.Value = server.ConfigText;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// To alter a directory server entry. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="server"></param>
        /// <returns></returns>
        static public int ExecuteAlterDirectoryServer(Connection aConnection, DirectoryServer server)
        {
            if (server.UpdatedAttrVector == 0)
            {
                // There is no change. Do nothing. 
                return 0;
            }
            //string encryptedPassword = (server.SearchUserPassword.Length > 0) ? aConnection.EncryptPassword(server.SearchUserPassword) : "";
            //OdbcCommand theQuery = new OdbcCommand(
            //    String.Format("CALL {0}.{1}.LDAPAlterConfig('{2}', {3}, {4}, '{5}', {6}, {7}, " +
            //                  "'{8}', '{9}', {10},  '{11}',  '{12}')",
            //                  new object[] { MANAGEABILITY_CATALOG, 
            //                                 TRAFODION_SPJ_SCHEMA,
            //                                 server.DomainName, 
            //                                 server.UsagePriority,
            //                                 server.UpdatedAttrVector, 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_HOST) ? server.HostName : ""), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_PORT_NUMBER) ? server.PortNumber : 0), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_VERSION) ? server.Version : 0), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERDN) ? server.SearchUserDN : ""), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERPASSWORD) ? encryptedPassword : ""), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_ENCRYPTION) ? server.EncryptionForSPJCall : 0), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_CACERT) ? server.CACert : ""), 
            //                                 (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_CONFIGTEXT) ? server.ConfigText : "")
            //                                }));

            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.LDAPAlterConfig(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param0 = theQuery.Parameters.Add("@DOMAINNAME", OdbcType.Text, server.DomainName.Length);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = server.DomainName;

            OdbcParameter param1 = theQuery.Parameters.Add("@USAGEPRI", OdbcType.Int);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = server.UsagePriority;

            OdbcParameter param2 = theQuery.Parameters.Add("@ATTRVEC", OdbcType.SmallInt);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = server.UpdatedAttrVector;

            OdbcParameter param3 = theQuery.Parameters.Add("@THEHOST", OdbcType.Text, server.HostName.Length);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_HOST) ? server.HostName : "");

            OdbcParameter param4 = theQuery.Parameters.Add("@PORT", OdbcType.Int);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_PORT_NUMBER) ? server.PortNumber : 0);

            OdbcParameter param5 = theQuery.Parameters.Add("@VERSION", OdbcType.Int);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_VERSION) ? server.Version : 0);

            OdbcParameter param6 = theQuery.Parameters.Add("@SEARCHUSERDN", OdbcType.Text, server.SearchUserDN.Length);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERDN) ? server.SearchUserDN : "");

            string encryptedPassword = "";
            if (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_SEARCHUSERPASSWORD))
            {
                encryptedPassword = aConnection.EncryptPassword(server.SearchUserPassword);
            }
            OdbcParameter param7 = theQuery.Parameters.Add("@SEARCHUSERPWD", OdbcType.Text, encryptedPassword.Length);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = encryptedPassword;

            OdbcParameter param8 = theQuery.Parameters.Add("@SSL", OdbcType.SmallInt);
            param8.Direction = System.Data.ParameterDirection.Input;
            param8.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_ENCRYPTION) ? server.EncryptionForSPJCall : 0);

            OdbcParameter param9 = theQuery.Parameters.Add("@CACERT", OdbcType.Text, server.CACert.Length);
            param9.Direction = System.Data.ParameterDirection.Input;
            param9.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_CACERT) ? server.CACert : "");

            OdbcParameter paramA = theQuery.Parameters.Add("@CONFIG", OdbcType.Text, server.ConfigText.Length);
            paramA.Direction = System.Data.ParameterDirection.Input;
            paramA.Value = (server.CheckUpdatedVector(DirectoryServer.ATTR_VECTOR_CONFIGTEXT) ? server.ConfigText : "");

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// To alter the default common parameters for active directory. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="server"></param>
        /// <returns></returns>
        static public int ExecuteAlterDirectoryServerCommonParameters(Connection aConnection, DirectoryServer server)
        {
            if (!server.CommonParametersUpdated)
            {
                return 0; // it is done.
            }

            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.LDAPAlterConfig(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param0 = theQuery.Parameters.Add("@DOMAINNAME", OdbcType.Text, DirectoryServer.DEFAULT_ENTRY_DOMAIN_NAME.Length);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = DirectoryServer.DEFAULT_ENTRY_DOMAIN_NAME;

            OdbcParameter param1 = theQuery.Parameters.Add("@USAGEPRI", OdbcType.Int);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = DirectoryServer.DEFAULT_ENTRY_USAGE_PRIORITY;

            OdbcParameter param2 = theQuery.Parameters.Add("@ATTRVEC", OdbcType.SmallInt);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = DirectoryServer.ATTR_VECTOR_CONFIGTEXT;

            OdbcParameter param3 = theQuery.Parameters.Add("@THEHOST", OdbcType.Text, BLANK.Length);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = BLANK;

            OdbcParameter param4 = theQuery.Parameters.Add("@PORT", OdbcType.Int);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = 0;

            OdbcParameter param5 = theQuery.Parameters.Add("@VERSION", OdbcType.Int);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = 0;

            OdbcParameter param6 = theQuery.Parameters.Add("@SEARCHUSERDN", OdbcType.Text, BLANK.Length);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = BLANK;

            OdbcParameter param7 = theQuery.Parameters.Add("@SEARCHUSERPWD", OdbcType.Text, BLANK.Length);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = BLANK;

            OdbcParameter param8 = theQuery.Parameters.Add("@SSL", OdbcType.SmallInt);
            param8.Direction = System.Data.ParameterDirection.Input;
            param8.Value = 0;

            OdbcParameter param9 = theQuery.Parameters.Add("@CACERT", OdbcType.Text, BLANK.Length);
            param9.Direction = System.Data.ParameterDirection.Input;
            param9.Value = BLANK;

            OdbcParameter paramA = theQuery.Parameters.Add("@CONFIG", OdbcType.Text, server.CommonConfigText.Length);
            paramA.Direction = System.Data.ParameterDirection.Input;
            paramA.Value = server.CommonConfigText;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// To drop a directory server entry.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aDomainName"></param>
        /// <param name="aUsagePriority"></param>
        /// <returns></returns>
        static public int ExecuteDropDirectoryServer(Connection aConnection, string aDomainName, int aUsagePriority)
        {
            //OdbcCommand theQuery = new OdbcCommand(
            //    String.Format("CALL {0}.{1}.LDAPDropConfig('{2}', {3})",
            //                  new object[] { MANAGEABILITY_CATALOG, TRAFODION_SPJ_SCHEMA, aDomainName, aUsagePriority }));

            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.LDAPDropConfig(?, ?)");
            OdbcParameter param1 = theQuery.Parameters.Add("@DOMAINNAME", OdbcType.Text, aDomainName.Length);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = aDomainName;
            OdbcParameter param2 = theQuery.Parameters.Add("@USAGEPRI", OdbcType.Int);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = aUsagePriority;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// This method is used to add Remotely authenticated users
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUser"></param>
        /// <returns></returns>
        static public int ExecuteAddOffPlatformUser(Connection aConnection, User aUser)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.addUser(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            
            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUser.UserName;
            
            OdbcParameter param1 = theQuery.Parameters.Add("@ROLENAME", OdbcType.Text, ConnectionDefinition.ROLE_NAME_MAX_LENGTH);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = aUser.DefaultRole;
            
            OdbcParameter param2 = theQuery.Parameters.Add("@USERTYPE", OdbcType.SmallInt);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = (int)aUser.UserType;
            
            OdbcParameter param3 = theQuery.Parameters.Add("@ATTRVECTOR", OdbcType.SmallInt);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = 0;
            
            OdbcParameter param4 = theQuery.Parameters.Add("@PASSWORD", OdbcType.Text, ConnectionDefinition.PASSWORD_MAX_LENGTH);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = "";

            OdbcParameter param5 = theQuery.Parameters.Add("@EXPIRATIONDAYS", OdbcType.SmallInt);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = 0;
            
            OdbcParameter param6 = theQuery.Parameters.Add("@EXPIRATIONDATE", OdbcType.Text);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = "";
            
            OdbcParameter param7 = theQuery.Parameters.Add("@DEFAULTSUBVOLUME", OdbcType.Text, 32);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = "";
            
            OdbcParameter param8 = theQuery.Parameters.Add("@INITIALDIRECTORY", OdbcType.Text, 128);
            param8.Direction = System.Data.ParameterDirection.Input;
            param8.Value = "";
            
            OdbcParameter param9 = theQuery.Parameters.Add("@DEFAULTSECURITY", OdbcType.Text, 128);
            param9.Direction = System.Data.ParameterDirection.Input;
            param9.Value = "";
            
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// This method is used to add locally authenticated users
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUser"></param>
        /// <returns></returns>
        static public int ExecuteAddPlatformUser(Connection aConnection, User aUser)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.addUser(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUser.UserName;

            OdbcParameter param1 = theQuery.Parameters.Add("@ROLENAME", OdbcType.Text, ConnectionDefinition.ROLE_NAME_MAX_LENGTH);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = aUser.DefaultRole;

            OdbcParameter param2 = theQuery.Parameters.Add("@USERTYPE", OdbcType.SmallInt);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = (int)aUser.UserType;

            OdbcParameter param3 = theQuery.Parameters.Add("@ATTRVECTOR", OdbcType.SmallInt);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = User.GetAttributeVectorForAdd(aUser);

            OdbcParameter param4 = theQuery.Parameters.Add("@PASSWORD", OdbcType.Text, ConnectionDefinition.PASSWORD_MAX_LENGTH);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = ((aUser.Password != null) && (aUser.Password.Length > 0)) ? aConnection.EncryptPassword(aUser.Password) : "";

            OdbcParameter param5 = theQuery.Parameters.Add("@EXPIRATIONDAYS", OdbcType.SmallInt);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = aUser.ExpirationDays;

            OdbcParameter param6 = theQuery.Parameters.Add("@EXPIRATIONDATE", OdbcType.Text);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = aUser.ExpirationDate;

            OdbcParameter param7 = theQuery.Parameters.Add("@DEFAULTSUBVOLUME", OdbcType.Text, 32);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = aUser.DefaultSubvolume;

            OdbcParameter param8 = theQuery.Parameters.Add("@INITIALDIRECTORY", OdbcType.Text, 128);
            param8.Direction = System.Data.ParameterDirection.Input;
            param8.Value = aUser.InitialDirectory;

            OdbcParameter param9 = theQuery.Parameters.Add("@DEFAULTSECURITY", OdbcType.Text, 128);
            param9.Direction = System.Data.ParameterDirection.Input;
            param9.Value = aUser.DefaultSecurity;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Alters the attributes of the locally authenticated users
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUser"></param>
        /// <param name="changeVector"></param>
        /// <returns></returns>
        static public int ExecuteAlterPlatformUser(Connection aConnection, User aUser, int changeVector)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.alterUser(?, ?, ?, ?, ?, ?, ?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUser.UserName;

            OdbcParameter param1 = theQuery.Parameters.Add("@ATTRVECTOR", OdbcType.SmallInt);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = changeVector;

            OdbcParameter param2 = theQuery.Parameters.Add("@PASSWORD", OdbcType.Text, ConnectionDefinition.PASSWORD_MAX_LENGTH);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = ((aUser.Password != null) && (aUser.Password.Length > 0)) ? aConnection.EncryptPassword(aUser.Password) : "";

            OdbcParameter param3 = theQuery.Parameters.Add("@EXPIRATIONDAYS", OdbcType.SmallInt);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = aUser.ExpirationDays;

            OdbcParameter param4 = theQuery.Parameters.Add("@EXPIRATIONDATE", OdbcType.Text);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = aUser.ExpirationDate;

            OdbcParameter param5 = theQuery.Parameters.Add("@DEFAULTSUBVOLUME", OdbcType.Text, 32);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = aUser.DefaultSubvolume;

            OdbcParameter param6 = theQuery.Parameters.Add("@INITIALDIRECTORY", OdbcType.Text, 128);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = aUser.InitialDirectory;

            OdbcParameter param7 = theQuery.Parameters.Add("@DEFAULTSECURITY", OdbcType.Text, 128);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = aUser.DefaultSecurity;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Get the password expiration details for a Role
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aRole"></param>
        static public void ExecuteGetSysAttributeForRole(Connection aConnection, Role aRole)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.SECGETSYSATTRSFORROLE(?, ?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@ROLENAME", OdbcType.Text, ConnectionDefinition.ROLE_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aRole.Name;

            OdbcParameter param1 = theQuery.Parameters.Add("@PASSWORDEXPIRYDAYS", OdbcType.Text, 6);
            param1.Direction = System.Data.ParameterDirection.Output;
            param1.DbType = System.Data.DbType.String;

            OdbcParameter param2 = theQuery.Parameters.Add("@PASSWORDEXPIRYDATE", OdbcType.Text, 20);
            param2.Direction = System.Data.ParameterDirection.Output;
            param2.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            int result = ExecuteNonQuery(theQuery);

            try
            {
                aRole.ExpirationDays = int.Parse(theQuery.Parameters[1].Value as string);
                aRole.ExpirationDate = theQuery.Parameters[2].Value as string;
            }
            catch (Exception ex)
            {
                //do nothing
            }
        }
        
        /// <summary>
        /// This method is used to get details of locally authenticated users
        ///
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUser">The user shall have the user name populated. The other variables will be populated
        /// from the values returned by the SPJ</param>
        /// <returns></returns>
        static public void ExecuteGetSysAttributeForUser(Connection aConnection, User aUser)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.SECGETSYSATTRSFORUSER(?, ?, ?, ?, ?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUser.UserName;

            OdbcParameter param1 = theQuery.Parameters.Add("@PASSWORDEXPIRYDAYS", OdbcType.Text, 6);
            param1.Direction = System.Data.ParameterDirection.Output;
            param1.DbType = System.Data.DbType.String;

            OdbcParameter param2 = theQuery.Parameters.Add("@PASSWORDEXPIRYDATE", OdbcType.Text, 20);
            param2.Direction = System.Data.ParameterDirection.Output;
            param2.DbType = System.Data.DbType.String;

            OdbcParameter param3 = theQuery.Parameters.Add("@DEFAULTSUBVOL", OdbcType.Text, 17);
            param3.Direction = System.Data.ParameterDirection.Output;
            param3.DbType = System.Data.DbType.String;

            OdbcParameter param4 = theQuery.Parameters.Add("@DEFAULTSECURITY", OdbcType.Text, 4);
            param4.Direction = System.Data.ParameterDirection.Output;
            param4.DbType = System.Data.DbType.String;

            OdbcParameter param5 = theQuery.Parameters.Add("@INITIALDIRECTORY", OdbcType.Text, 128);
            param5.Direction = System.Data.ParameterDirection.Output;
            param5.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            int result = ExecuteNonQuery(theQuery);

            try
            {
                aUser.ExpirationDays = int.Parse(theQuery.Parameters[1].Value as string);
                aUser.ExpirationDate = theQuery.Parameters[2].Value as string;
            }
            catch (Exception ex)
            {
                //do nothing
            }
            aUser.DefaultSubvolume = theQuery.Parameters[3].Value as string;
            aUser.DefaultSecurity = theQuery.Parameters[4].Value as string;
            aUser.InitialDirectory = theQuery.Parameters[5].Value as string;
        }
        /// <summary>
        /// Grant a new role to a user. This role will be an aditional role
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="aRoleName"></param>
        /// <returns></returns>
        static public int ExecuteGrantRole(Connection aConnection, string aUserName, string aRoleName)
        {
            return GrantRole(aConnection, aUserName, aRoleName, 0);
        }

        /// <summary>
        /// Grant a role to a user that would be their default role
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="aRoleName"></param>
        /// <returns></returns>
        static public int ExecuteGrantDefaultRole(Connection aConnection, string aUserName, string aRoleName)
        {
            return GrantRole(aConnection, aUserName, aRoleName, 1);
        }

        /// <summary>
        /// Helper method to grant role
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="aRoleName"></param>
        /// <param name="aRoleType"></param>
        /// <returns></returns>
        static private int GrantRole(Connection aConnection, string aUserName, string aRoleName, int aRoleType)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.grantRole(?, ?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUserName;

            OdbcParameter param1 = theQuery.Parameters.Add("@ROLENAME", OdbcType.Text, ConnectionDefinition.ROLE_NAME_MAX_LENGTH);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = aRoleName;

            OdbcParameter param2 = theQuery.Parameters.Add("@SETASDEFAULT", OdbcType.Int);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = aRoleType;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Set the default role of the user. The role will be one of the existing roles of the user
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <param name="aRoleName"></param>
        /// <returns></returns>
        static public int ExecuteSetDefaultRole(Connection aConnection, string aUserName, string aRoleName)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.setDefaultRole(?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUserName;

            OdbcParameter param1 = theQuery.Parameters.Add("@ROLENAME", OdbcType.Text, ConnectionDefinition.ROLE_NAME_MAX_LENGTH);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = aRoleName;

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

            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.revokeRole(?, ?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUserName;

            OdbcParameter param1 = theQuery.Parameters.Add("@ROLENAME", OdbcType.Text, ConnectionDefinition.ROLE_NAME_MAX_LENGTH);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = aRoleName;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Delete a user
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aUserName"></param>
        /// <returns></returns>
        static public int ExecuteDeleteUser(Connection aConnection, string aUserName)
        {
            String command = "CALL MANAGEABILITY.TRAFODION_SPJ.deleteUser(?)";
            OdbcCommand theQuery = new OdbcCommand(command);
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param0 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param0.Direction = System.Data.ParameterDirection.Input;
            param0.Value = aUserName;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        static public OdbcDataReader GetUser(Connection aConnection, string aUserName)
        {
            string query = String.Format("Select USERNAME as EXTERNAL_NAME, ISDEFAULT as IS_PRIMARY, ROLE as ROLE_NAME, CREATEDBY as CREATING_USER, USERTYPE as USER_TYPE, CREATETIME from MANAGEABILITY.TRAFODION_SECURITY.USER_INFO WHERE USERNAME = '{0}' FOR BROWSE ACCESS", aUserName.ToUpper());
            //string query = String.Format("SELECT EXTERNAL_NAME, IS_PRIMARY, ROLE_NAME, CREATING_USER, USER_TYPE  FROM MANAGEABILITY.USER_MANAGEMENT.USERS WHERE EXTERNAL_NAME = '{0}' ORDER BY EXTERNAL_NAME ASC FOR BROWSE ACCESS", aUserName);
            OdbcCommand theQuery = new OdbcCommand(query);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }
        static public OdbcDataReader GetRolesForOperation(Connection aConnection, string currentRole, string userName, string operation)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecGetRolesAvailable(?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@LogonRoleName", OdbcType.Text, 128);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = currentRole.Trim();

            OdbcParameter param2 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, ConnectionDefinition.USER_NAME_MAX_LENGTH);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = userName.Trim();

            OdbcParameter param3 = theQuery.Parameters.Add("@OperationType", OdbcType.Text, 5);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = operation.Trim();

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
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.ADDROLE(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@roleName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = roleName;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Delete a role
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="roleName"></param>
        /// <returns></returns>
        static public int ExecuteDeleteRole(Connection aConnection, string roleName)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.DELETEROLE(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@roleName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = roleName;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Alters the role attributes
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="role"></param>
        /// <param name="attrVector"></param>
        /// <param name="oldPassword"></param>
        /// <returns></returns>
        static public int ExecuteAlterRole(Connection aConnection, Role role, int attrVector, string oldPassword)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.ALTERROLE(?,?,?,?,?,?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@roleName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = role.Name;

            OdbcParameter param2 = theQuery.Parameters.Add("@attrVector", OdbcType.SmallInt);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.DbType = System.Data.DbType.Int16;
            param2.Value = attrVector;

            OdbcParameter param3 = theQuery.Parameters.Add("@oldPassword", OdbcType.Text);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.DbType = System.Data.DbType.String;
            param3.Value = oldPassword;

            OdbcParameter param4 = theQuery.Parameters.Add("@newPassword", OdbcType.Text);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.DbType = System.Data.DbType.String;
            param4.Value = role.Password;

            OdbcParameter param5 = theQuery.Parameters.Add("@expirationDate", OdbcType.Text);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.DbType = System.Data.DbType.String;
            param5.Value = role.ExpirationDate;

            OdbcParameter param6 = theQuery.Parameters.Add("@expirationDays", OdbcType.Int);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.DbType = System.Data.DbType.Int32;
            param6.Value = role.ExpirationDays;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }

        /// <summary>
        /// Fetches the grant count and default role count for a given role name
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aRoleName"></param>
        /// <returns></returns>
        static public OdbcDataReader GetRoleDetails(Connection aConnection, string aRoleName)
        {
            string query = String.Format("SELECT GRANTCOUNT, DEFAULTROLECOUNT FROM MANAGEABILITY.TRAFODION_SECURITY.ROLE_INFO WHERE ROLENAME = '{0}' FOR READ UNCOMMITTED ACCESS", aRoleName.ToUpper());
            OdbcCommand theQuery = new OdbcCommand(query);
            theQuery.Connection = aConnection.OpenOdbcConnection;
            OdbcDataReader reader = ExecuteReader(theQuery);
            return reader;
        }

        /// <summary>
        /// Create a self signing certificate
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aCredential"></param>
        /// <returns></returns>
        static public int ExecuteCreateCertificate(Connection aConnection, Credentials aCredential, out string certString)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.CREATECERTIFICATE(?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@systemName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = aCredential.SystemName;

            OdbcParameter param2 = theQuery.Parameters.Add("@keySize", OdbcType.Text);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.DbType = System.Data.DbType.Int32;
            param2.Value = aCredential.KeySize;

            OdbcParameter param3 = theQuery.Parameters.Add("@subject", OdbcType.Text);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.DbType = System.Data.DbType.String;
            param3.Value = aCredential.Subject;

            OdbcParameter param4 = theQuery.Parameters.Add("@CERTIFICATE", OdbcType.Text, 9012);
            param4.Direction = System.Data.ParameterDirection.Output;
            param4.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;

            int result = ExecuteNonQuery(theQuery);
            certString = theQuery.Parameters[3].Value as string;

            return result;
        }

        /// <summary>
        /// Create a Certificate Signing Request
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aCredential"></param>
        /// <returns></returns>
        static public int ExecuteCreateCSR(Connection aConnection, Credentials aCredential, out String csrString)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.CREATECSR(?,?,?,?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@systemName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = aCredential.SystemName;

            OdbcParameter param2 = theQuery.Parameters.Add("@keySize", OdbcType.Text);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.DbType = System.Data.DbType.Int32;
            param2.Value = aCredential.KeySize;

            OdbcParameter param3 = theQuery.Parameters.Add("@subject", OdbcType.Text);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.DbType = System.Data.DbType.String;
            param3.Value = aCredential.Subject;

            OdbcParameter param4 = theQuery.Parameters.Add("@CSR", OdbcType.Text, 8192);
            param4.Direction = System.Data.ParameterDirection.Output;
            param4.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            int result = ExecuteNonQuery(theQuery);
            csrString = theQuery.Parameters[3].Value as string;

            return result;

        }

        /// <summary>
        /// Deploy a CA certificate
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aCredential"></param>
        /// <returns></returns>
        static public int ExecuteInsertCertificate(Connection aConnection, Credentials aCredential)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.INSERTCERTIFICATE(?,?,?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@systemName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = aCredential.SystemName;

            OdbcParameter param2 = theQuery.Parameters.Add("@certificate", OdbcType.Text);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.DbType = System.Data.DbType.String;
            param2.Value = aCredential.Certificate;

            OdbcParameter param3 = theQuery.Parameters.Add("@cacertificate", OdbcType.Text);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.DbType = System.Data.DbType.String;
            param3.Value = aCredential.CACertificate;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteNonQuery(theQuery);
        }
        /// <summary>
        /// To retrieve all security policies.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public string ExecuteInfoPolicies(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigGetAllSettings(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@CurrentSettings", OdbcType.Text, 32000);
            param1.Direction = System.Data.ParameterDirection.Output;
            param1.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            ExecuteNonQuery(theQuery);
            return param1.Value as string;
        }

        /// <summary>
        /// To retrieve the factory default policies. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public string ExecuteInfoFactoryDefaultPolicies(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigGetAllDefault(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@CurrentSettings", OdbcType.Text, 32000);
            param1.Direction = System.Data.ParameterDirection.Output;
            param1.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            ExecuteNonQuery(theQuery);
            return param1.Value as string;
        }

        /// <summary>
        /// To retrieve the most strong policies. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <returns></returns>
        static public string ExecuteInfoMostStrongPolicies(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigGetAllMostSecure(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@CurrentSettings", OdbcType.Text, 32000);
            param1.Direction = System.Data.ParameterDirection.Output;
            param1.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            ExecuteNonQuery(theQuery);
            return param1.Value as string;
        }

        /// <summary>
        /// To retrieve system policies. 
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aSystemName"></param>
        /// <param name="anAutoDownloadCertificate"></param>
        /// <param name="anEnforceCertificateExpiry"></param>
        /// <param name="aPasswordEncryptionRequired"></param>
        /// <returns></returns>
        static public int ExecuteInfoSystemPolicies(Connection aConnection, string aSystemName,
                                                    out bool anAutoDownloadCertificate, 
                                                    out bool anEnforceCertificateExpiry, 
                                                    out bool aPasswordEncryptionRequired)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.InfoPwdSecPolicy(?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@sysName", OdbcType.Text, 5);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            if (!string.IsNullOrEmpty(aSystemName) && aSystemName.Length > 5)
            {
                param1.Value = aSystemName.Substring(0, 5);
            }
            else
            {
                param1.Value = aSystemName;
            }

            OdbcParameter param2 = theQuery.Parameters.Add("@allowAutoDownload", OdbcType.Int);
            param2.Direction = System.Data.ParameterDirection.Output;
            OdbcParameter param3 = theQuery.Parameters.Add("@enforceExpiration", OdbcType.Int);
            param3.Direction = System.Data.ParameterDirection.Output;
            OdbcParameter param4 = theQuery.Parameters.Add("@requirePWEncryption", OdbcType.Int);
            param4.Direction = System.Data.ParameterDirection.Output;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            anAutoDownloadCertificate =  ((int)param2.Value) > 0;
            anEnforceCertificateExpiry = ((int)param3.Value) > 0;
            aPasswordEncryptionRequired = ((int)param4.Value) > 0;
            return retCode;
        }

        /// <summary>
        /// To update system policies.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aSystemName"></param>
        /// <param name="anAutoDownloadCertificate"></param>
        /// <param name="anEnforceCertificateExpiry"></param>
        /// <param name="aPasswordEncryptionRequired"></param>
        /// <returns></returns>
        static public int ExecuteAlterSystemPolicies(Connection aConnection, string aSystemName,
                                            ref bool anAutoDownloadCertificate,
                                            ref bool anEnforceCertificateExpiry,
                                            ref bool aPasswordEncryptionRequired)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.AlterPwdSecPolicy(?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@sysName", OdbcType.Text, 5);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            if (!string.IsNullOrEmpty(aSystemName) && aSystemName.Length > 5)
            {
                param1.Value = aSystemName.Substring(0, 5);
            }
            else
            {
                param1.Value = aSystemName;
            }
            OdbcParameter param2 = theQuery.Parameters.Add("@allowAutoDownload", OdbcType.Int);
            param2.Direction = System.Data.ParameterDirection.Input;
            OdbcParameter param3 = theQuery.Parameters.Add("@enforceExpiration", OdbcType.Int);
            param3.Direction = System.Data.ParameterDirection.Input;
            OdbcParameter param4 = theQuery.Parameters.Add("@requirePWEncryption", OdbcType.Int);
            param4.Direction = System.Data.ParameterDirection.Input;
            OdbcParameter param5 = theQuery.Parameters.Add("@allowAutoDownloadO", OdbcType.Int);
            param5.Direction = System.Data.ParameterDirection.Output;
            OdbcParameter param6 = theQuery.Parameters.Add("@enforceExpirationO", OdbcType.Int);
            param6.Direction = System.Data.ParameterDirection.Output;
            OdbcParameter param7 = theQuery.Parameters.Add("@requirePWEncryptionO", OdbcType.Int);
            param7.Direction = System.Data.ParameterDirection.Output;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            anAutoDownloadCertificate = ((int)param5.Value) > 0;
            anEnforceCertificateExpiry = ((int)param6.Value) > 0;
            aPasswordEncryptionRequired = ((int)param7.Value) > 0;
            return retCode;
        }

        /// <summary>
        /// Alter certificate and connection policy settings
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="aSystemName"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteAlterSystemPolicies(Connection aConnection, string aSystemName, Policies thePolicies)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.AlterPwdSecPolicy(?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@sysName", OdbcType.Text, 5);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            if (!string.IsNullOrEmpty(aSystemName) && aSystemName.Length > 5)
            {
                param1.Value = aSystemName.Substring(0, 5);
            }
            else
            {
                param1.Value = aSystemName;
            }
            OdbcParameter param2 = theQuery.Parameters.Add("@allowAutoDownload", OdbcType.Int);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = thePolicies.AutoDownloadCertificate ? 1:0;
            OdbcParameter param3 = theQuery.Parameters.Add("@enforceExpiration", OdbcType.Int);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = thePolicies.EnforceCertificateExpiry ? 1:0;
            OdbcParameter param4 = theQuery.Parameters.Add("@requirePWEncryption", OdbcType.Int);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = thePolicies.PasswordEncryptionRequired ? 1:0;
            OdbcParameter param5 = theQuery.Parameters.Add("@allowAutoDownloadO", OdbcType.Int);
            param5.Direction = System.Data.ParameterDirection.Output;
            OdbcParameter param6 = theQuery.Parameters.Add("@enforceExpirationO", OdbcType.Int);
            param6.Direction = System.Data.ParameterDirection.Output;
            OdbcParameter param7 = theQuery.Parameters.Add("@requirePWEncryptionO", OdbcType.Int);
            param7.Direction = System.Data.ParameterDirection.Output;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Alter security logging policies.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteAlterLoggingPolicies(Connection aConnection, Policies thePolicies)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigSetLoggingOptions(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@UserMgmt", OdbcType.Text, 2);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = (thePolicies.LogUserManagement) ? "Y":"N";
            OdbcParameter param2 = theQuery.Parameters.Add("@UserMgmtRequired", OdbcType.Text, 2);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = (thePolicies.LogUserManagementRequired) ? "Y":"N";
            OdbcParameter param3 = theQuery.Parameters.Add("@PwdChange", OdbcType.Text, 2);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = (thePolicies.LogChangePassword) ? "Y" : "N";
            OdbcParameter param4 = theQuery.Parameters.Add("@PwdChangeRequired", OdbcType.Text, 2);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = (thePolicies.LogChangePasswordRequired) ? "Y" : "N";
            OdbcParameter param5 = theQuery.Parameters.Add("@PlatformLogonsFail", OdbcType.Text, 2);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = (thePolicies.LogPlatformLogonFailure) ? "Y" : "N";
            OdbcParameter param6 = theQuery.Parameters.Add("@PlatformLogonsFailRequired", OdbcType.Text, 2);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = (thePolicies.LogPlatformLogonFailure) ? "Y" : "N";
            OdbcParameter param7 = theQuery.Parameters.Add("@DatabaseLogonsFail", OdbcType.Text, 2);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = (thePolicies.LogDatabaseLogonFailure) ? "Y" : "N";
            OdbcParameter param8 = theQuery.Parameters.Add("@DatabaseLogonsFailRequired", OdbcType.Text, 2);
            param8.Direction = System.Data.ParameterDirection.Input;
            param8.Value = (thePolicies.LogDatabaseLogonFailure) ? "Y" : "N";
            OdbcParameter param9 = theQuery.Parameters.Add("@PlatformLogonsOK", OdbcType.Text, 2);
            param9.Direction = System.Data.ParameterDirection.Input;
            param9.Value = (thePolicies.LogPlatformLogonOK) ? "Y" : "N";
            OdbcParameter paramA = theQuery.Parameters.Add("@PlatformLogonsOKRequired", OdbcType.Text, 2);
            paramA.Direction = System.Data.ParameterDirection.Input;
            //paramA.Value = (thePolicies.LogPlatformLogonOKRequired) ? "Y":"N";
            paramA.Value = "N"; // this option has been removed, so we always say NO to it. 
            OdbcParameter paramB = theQuery.Parameters.Add("@DatabaseLogonsOK", OdbcType.Text, 2);
            paramB.Direction = System.Data.ParameterDirection.Input;
            paramB.Value = (thePolicies.LogDatabaseLogonOK) ? "Y":"N";
            OdbcParameter paramC = theQuery.Parameters.Add("@DatabaseLogonsOKRequired", OdbcType.Text, 2);
            paramC.Direction = System.Data.ParameterDirection.Input;
            paramC.Value = (thePolicies.LogDatabaseLogonOKRequired) ? "Y":"N";
            OdbcParameter paramD = theQuery.Parameters.Add("@LogFileAgesInDays", OdbcType.Int);
            paramD.Direction = System.Data.ParameterDirection.Input;
            paramD.Value = thePolicies.LogFileAgesInDays;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Alter power role reset password policies
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteAlterResetPwdPolicies(Connection aConnection, Policies thePolicies)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigPWResetRequiresCurrentPW(?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@PowerUser", OdbcType.Text, 2);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = (thePolicies.PwdReqForPowerRoleReset) ? "Y":"N";
            OdbcParameter param2 = theQuery.Parameters.Add("@SuperUser", OdbcType.Text, 2);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = (thePolicies.PwdReqForSuperReset) ? "Y":"N";

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Alter password complexity policies
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteAlterPwdComplexityPolicies(Connection aConnection, Policies thePolicies)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigPasswordComplexity(?, ?, ?, ?, ?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@DigitRequired", OdbcType.Text, 2);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = thePolicies.PwdQualReqNumber ? "Y" : "N";
            OdbcParameter param2 = theQuery.Parameters.Add("@UpperCaseRequired", OdbcType.Text, 2);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = thePolicies.PwdQualReqUpper ? "Y" : "N";
            OdbcParameter param3 = theQuery.Parameters.Add("@LowerCaseRequired", OdbcType.Text, 2);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = thePolicies.PwdQualReqLower ? "Y" : "N";
            OdbcParameter param4 = theQuery.Parameters.Add("@SpecialCharRequired", OdbcType.Text, 2);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = thePolicies.PwdQualReqSpecChar ? "Y" : "N";
            OdbcParameter param5 = theQuery.Parameters.Add("@MinimumQualityRequired", OdbcType.Text, 2);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = thePolicies.PwdQualReqCriteria.ToString();
            OdbcParameter param6 = theQuery.Parameters.Add("@PwdQualNoUserName", OdbcType.Text, 2);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = thePolicies.PwdQualNoUserName ? "Y" : "N";
            OdbcParameter param7 = theQuery.Parameters.Add("@PwdQualNoRepeatChars", OdbcType.Text, 2);
            param7.Direction = System.Data.ParameterDirection.Input;
            param7.Value = thePolicies.PwdQualNoRepeatChars ? "Y" : "N";

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Alter logon failure policies.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteAlterDelayAfterLogonFailurePolicies(Connection aConnection, Policies thePolicies)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigDelayAfterLogonFailure (?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@FailuresBeforeDelay", OdbcType.Text, 8);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = thePolicies.PwdAuthFailsBeforeDelay.ToString();
            OdbcParameter param2 = theQuery.Parameters.Add("@DelayLengthInSeconds", OdbcType.Text, 8);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = thePolicies.PwdAuthFailDelayInSecs.ToString();

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Alter policy attributes related to Safeguard options
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteAlterSafeGuardSysOptions(Connection aConnection, Policies thePolicies)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigSetSysOptions(?, ?, ?, ?, ?, ?)");

            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@PwdQualMinLength", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = thePolicies.PwdQualMinLength.ToString();
            OdbcParameter param2 = theQuery.Parameters.Add("@PwdCtrlGracePeriod", OdbcType.Text);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = thePolicies.PwdCtrlGracePeriod.ToString();
            OdbcParameter param3 = theQuery.Parameters.Add("@PwdHistory", OdbcType.Text);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.Value = thePolicies.PwdHistory.ToString();
            OdbcParameter param4 = theQuery.Parameters.Add("@PwdCanChangeWithin", OdbcType.Text);
            param4.Direction = System.Data.ParameterDirection.Input;
            param4.Value = thePolicies.PwdCanChangeWithin.ToString();
            OdbcParameter param5 = theQuery.Parameters.Add("@PwdCtrlDefaultExprDate", OdbcType.Text);
            param5.Direction = System.Data.ParameterDirection.Input;
            param5.Value = string.IsNullOrEmpty(thePolicies.PwdDefaultExprDate) ? " " : thePolicies.PwdDefaultExprDate;

            OdbcParameter param6 = theQuery.Parameters.Add("@PwdCtrlDefaultExprDays", OdbcType.Text);
            param6.Direction = System.Data.ParameterDirection.Input;
            param6.Value = thePolicies.PwdDefaultExprDays.ToString();

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Alter a single given security policy attribute.
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="option"></param>
        /// <param name="value"></param>
        /// <returns></returns>
        static public int ExecuteAlterOneOption(Connection aConnection, string option, string value)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigSetOneOption (?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@OptionName", OdbcType.Text, option.Length);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = option;
            OdbcParameter param2 = theQuery.Parameters.Add("@NewSetting", OdbcType.Text, value.Length);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.Value = value;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Reset all policies to factory default setting
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteResetAllToDefaultPolicies(Connection aConnection)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigResetAllToDefault (?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@Really", OdbcType.Text, 8);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = RESET_ALL_POLICY;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        /// <summary>
        /// Reset policies to most secure setting
        /// </summary>
        /// <param name="aConnection"></param>
        /// <param name="thePolicies"></param>
        /// <returns></returns>
        static public int ExecuteResetAllToMostSecurePolicies(Connection aConnection)
        {
            int retCode = 0;
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SecConfigResetAllToMostSecure (?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@Really", OdbcType.Text, 8);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.Value = RESET_ALL_POLICY;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            retCode = ExecuteNonQuery(theQuery);
            return retCode;
        }

        static public OdbcDataReader ExecuteSecConfigFunctionsForRole(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SECCONFIGFUNCTIONSFORROLE(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@roleName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = aConnection.TheConnectionDefinition.RoleName;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public OdbcDataReader ExecuteSelectRoleNames(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("SELECT TRIM(ROLENAME)AS ROLENAME FROM MANAGEABILITY.TRAFODION_SECURITY.ROLE_INFO ORDER BY ROLENAME FOR READ UNCOMMITTED ACCESS");
            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public OdbcDataReader ExecuteSecCanChangeTheseRolesPW(Connection aConnection)
        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SECCANCHANGETHESEROLESPW(?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@roleName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = aConnection.TheConnectionDefinition.RoleName;

            theQuery.Connection = aConnection.OpenOdbcConnection;
            return ExecuteReader(theQuery);
        }

        static public int ExecuteSecGetSFGAttrForUser(Connection aConnection, string userName,
                                                out string expiryDays,
                                                out string expiryDate,
                                                out string defaultSubVol,
                                                out string defaultSecurity,
                                                out string initialDirectory)


        {
            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.SECGETSYSATTRSFORUSER(?,?,?,?,?,?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;

            OdbcParameter param1 = theQuery.Parameters.Add("@userName", OdbcType.Text);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = userName;

            OdbcParameter param2 = theQuery.Parameters.Add("@passwordExpiryDays", OdbcType.Text, 6);
            param2.Direction = System.Data.ParameterDirection.Output;
            param2.DbType = System.Data.DbType.String;

            OdbcParameter param3 = theQuery.Parameters.Add("@passwordExpiryDate", OdbcType.Text, 20);
            param3.Direction = System.Data.ParameterDirection.Output;
            param3.DbType = System.Data.DbType.String;

            OdbcParameter param4 = theQuery.Parameters.Add("@defaultSubVol", OdbcType.Text, 17);
            param4.Direction = System.Data.ParameterDirection.Output;
            param4.DbType = System.Data.DbType.String;

            OdbcParameter param5 = theQuery.Parameters.Add("@defaultSecurity", OdbcType.Text, 4);
            param5.Direction = System.Data.ParameterDirection.Output;
            param5.DbType = System.Data.DbType.String;

            OdbcParameter param6 = theQuery.Parameters.Add("@initialDirectory", OdbcType.Text, 128);
            param6.Direction = System.Data.ParameterDirection.Output;
            param6.DbType = System.Data.DbType.String;

            theQuery.Connection = aConnection.OpenOdbcConnection;

            int result = ExecuteNonQuery(theQuery);
            expiryDays = theQuery.Parameters[1].Value as string;
            expiryDate = theQuery.Parameters[2].Value as string;
            defaultSubVol = theQuery.Parameters[3].Value as string;
            defaultSecurity = theQuery.Parameters[4].Value as string;
            initialDirectory = theQuery.Parameters[5].Value as string;

            return result;
        }

        static public string SelectTrafCatPrivilegesByRoleNameQueryText(string systemCatalogName, string roleName, string catalogName, int schemaVersion)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append("SELECT SCHEMA_NAME,");
            sb.Append(" OBJECT_NAME,");
            sb.Append(" OBJECT_TYPE,");
            sb.Append(" CASE GRANTOR  ");
            sb.Append(" WHEN  -2 THEN 'SYSTEM'  ");
            sb.Append(" WHEN  -1 THEN 'PUBLIC' ");
            sb.Append(" ELSE USER(GRANTOR) ");
            sb.Append(" END AS GRANTED_BY,");
            sb.Append(" PRIVILEGE_TYPE,");
            sb.Append(" COLUMN_NAME ");
            sb.Append("FROM");
            sb.Append("(");
            sb.Append("  SELECT OBJ.OBJECT_NAME AS SCHEMA_NAME,");
            sb.Append("  OBJ.OBJECT_NAME AS OBJECT_NAME,");
            sb.Append("  'SCHEMA' AS OBJECT_TYPE,");
            sb.Append("  SCP.GRANTOR, ");
            sb.Append(" CASE SCP.PRIVILEGE_TYPE ");
            sb.Append(" WHEN 'AD' THEN 'ALL PRIVILEGES'  ");
            sb.Append(" WHEN 'A' THEN 'ALTER ALL'  ");
            sb.Append(" WHEN 'AB' THEN 'ALTER TABLE'  ");
            sb.Append(" WHEN 'AM' THEN 'ALTER MV'  ");
            sb.Append(" WHEN 'AS' THEN 'ALTER SYNONYM'  ");
            sb.Append(" WHEN 'AT' THEN 'ALTER TRIGGER'  ");
            sb.Append(" WHEN 'AV' THEN 'ALTER VIEW'  ");
            sb.Append(" WHEN 'AG' THEN 'ALTER MV GROUP'  ");
            sb.Append(" WHEN 'C' THEN 'CREATE ALL'  ");
            sb.Append(" WHEN 'CB' THEN 'CREATE TABLE'  ");
            sb.Append(" WHEN 'CM' THEN 'CREATE MV'  ");
            sb.Append(" WHEN 'CP' THEN 'CREATE PROCEDURE'  ");
            sb.Append(" WHEN 'CS' THEN 'CREATE SYNONYM'  ");
            sb.Append(" WHEN 'CT' THEN 'CREATE TRIGGER'  ");
            sb.Append(" WHEN 'CV' THEN 'CREATE VIEW'  ");
            sb.Append(" WHEN 'CG' THEN 'CREATE MV GROUP'  ");
            sb.Append(" WHEN 'D' THEN 'DELETE'  ");
            sb.Append(" WHEN 'DR' THEN 'DROP ALL OBJECTS'  ");
            sb.Append(" WHEN 'DB' THEN 'DROP TABLE'  ");
            sb.Append(" WHEN 'DM' THEN 'DROP MV'  ");
            sb.Append(" WHEN 'DP' THEN 'DROP PROCEDURE'  ");
            sb.Append(" WHEN 'DS' THEN 'DROP SYNONYM'  ");
            sb.Append(" WHEN 'DT' THEN 'DROP TRIGGER'  ");
            sb.Append(" WHEN 'DV' THEN 'DROP VIEW'  ");
            sb.Append(" WHEN 'DG' THEN 'DROP MV GROUP'  ");
            sb.Append(" WHEN 'E' THEN 'EXECUTE'  ");
            sb.Append(" WHEN 'I' THEN 'INSERT'  ");
            sb.Append(" WHEN 'M' THEN 'MAINTAIN'  ");
            sb.Append(" WHEN 'R' THEN 'REFERENCE'  ");
            sb.Append(" WHEN 'RF' THEN 'REFRESH MV'  ");
            sb.Append(" WHEN 'RO' THEN 'REORG'  ");
            sb.Append(" WHEN 'S' THEN 'SELECT'  ");
            sb.Append(" WHEN 'T' THEN 'TRIGGER'  ");
            sb.Append(" WHEN 'U' THEN 'UPDATE' ");
            sb.Append(" WHEN 'US' THEN 'UPDATE STATS'   ");
            sb.Append(" ELSE SCP.PRIVILEGE_TYPE ");
            sb.Append(" END AS PRIVILEGE_TYPE,");
            sb.Append(" ' ' AS COLUMN_NAME");
            sb.AppendFormat("        FROM {0}."_MD_".OBJECTS OBJ, ", catalogName, schemaVersion);
            sb.AppendFormat("        {0}."_MD_".SCH_PRIVILEGES SCP, ", catalogName, schemaVersion);
            sb.Append("             MANAGEABILITY.USER_MANAGEMENT.ROLES RL ");
            sb.Append("        WHERE OBJ.OBJECT_NAME_SPACE = 'SL' AND  ");
            sb.Append("              OBJ.OBJECT_UID = SCP.TABLE_UID AND ");
            sb.AppendFormat("              RL.ROLE_NAME = '{0}' AND ", roleName);
            sb.Append("              SCP.GRANTEE = RL.ROLE_ID AND ");
            sb.Append("              NOT (OBJ.OBJECT_NAME LIKE '"_MD_"%' OR  ");
            sb.Append("                   OBJ.OBJECT_NAME  = '@MAINTAIN_SCHEMA@' OR  ");
            sb.Append("                   OBJ.OBJECT_NAME  LIKE 'HP\\_%') ");
            sb.Append("  ");
            sb.Append(" FOR READ UNCOMMITTED ACCESS ");
            sb.Append("UNION");
            sb.Append("  SELECT SCH.SCHEMA_NAME, ");
            sb.Append("         OBJ.OBJECT_NAME, ");
            sb.Append("         CASE OBJ.OBJECT_TYPE ");
            sb.Append("           WHEN 'BT' THEN 'TABLE' ");
            sb.Append("           WHEN 'VI' THEN 'VIEW' ");
            sb.Append("           WHEN 'UR' THEN 'PROCEDURE' ");
            sb.Append("           WHEN 'MV' THEN 'MAT. VIEW' ");
            sb.Append("           WHEN 'SY' THEN 'SYNONYM' ");
            sb.Append("           ELSE  OBJ.OBJECT_TYPE ");
            sb.Append("         END AS OBJECT_TYPE, ");
            sb.Append(" 	TAB.GRANTOR, ");
            sb.Append("         CASE TAB.PRIVILEGE_TYPE ");
            sb.Append("           WHEN 'E' THEN 'EXECUTE' ");
            sb.Append("           WHEN 'S' THEN 'SELECT' ");
            sb.Append("           WHEN 'D' THEN 'DELETE' ");
            sb.Append("           WHEN 'I' THEN 'INSERT' ");
            sb.Append("           WHEN 'U' THEN 'UPDATE' ");
            sb.Append("           WHEN 'R' THEN 'REFER' ");
            sb.Append("           ELSE  TAB.PRIVILEGE_TYPE ");
            sb.Append("         END AS PRIVILEGE_TYPE,");
            sb.Append("         ' ' AS COLUMN_NAME ");
            sb.AppendFormat("        FROM {0}.SYSTEM_SCHEMA.SCHEMATA SCH, ", systemCatalogName);
            sb.AppendFormat("             {0}."_MD_".OBJECTS OBJ, ", catalogName, schemaVersion);
            sb.AppendFormat("             {0}."_MD_".TBL_PRIVILEGES TAB, ", catalogName, schemaVersion);
            sb.Append("             MANAGEABILITY.USER_MANAGEMENT.ROLES RL ");
            sb.Append("        WHERE SCH.SCHEMA_UID = OBJ.SCHEMA_UID AND  ");
            sb.Append("            OBJ.OBJECT_UID = TAB.TABLE_UID AND  ");
            sb.AppendFormat("              RL.ROLE_NAME = '{0}' AND ", roleName);
            sb.Append("            TAB.GRANTEE = RL.ROLE_ID AND ");
            sb.Append("            NOT (OBJ.OBJECT_TYPE = 'NN' OR  ");
            sb.Append("                 OBJ.OBJECT_TYPE = 'PK' OR  ");
            sb.Append("                 OBJ.OBJECT_TYPE = 'UC' OR  ");
            sb.Append("                 OBJ.OBJECT_TYPE = 'IX' OR  ");
            sb.Append("                 OBJ.OBJECT_TYPE = 'SL') AND ");
            sb.Append("                 OBJ.OBJECT_SECURITY_CLASS = 'UT' AND ");
            sb.Append("              NOT ( SCH.SCHEMA_NAME = '@MAINTAIN_SCHEMA@' OR  ");
            sb.Append("                    SCH.SCHEMA_NAME LIKE 'HP\\_%')");
            sb.Append(" FOR READ UNCOMMITTED ACCESS  ");
            sb.Append("UNION");
            sb.Append("");
            sb.Append("  SELECT SCH.SCHEMA_NAME, ");
            sb.Append("         OBJ.OBJECT_NAME, ");
            sb.Append("         CASE OBJ.OBJECT_TYPE ");
            sb.Append("           WHEN 'BT' THEN 'TABLE' ");
            sb.Append("           WHEN 'VI' THEN 'VIEW' ");
            sb.Append("           WHEN 'MV' THEN 'MAT. VIEW' ");
            sb.Append("           WHEN 'SY' THEN 'SYNONYM' ");
            sb.Append("           ELSE  OBJ.OBJECT_TYPE ");
            sb.Append("         END AS OBJECT_TYPE, ");
            sb.Append(" 	COLP.GRANTOR, ");
            sb.Append("         CASE COLP.PRIVILEGE_TYPE ");
            sb.Append("           WHEN 'U' THEN 'UPDATE' ");
            sb.Append("           WHEN 'R' THEN 'REFER' ");
            sb.Append("           ELSE COLP.PRIVILEGE_TYPE ");
            sb.Append("         END AS COLUMN_PRIV, ");
            sb.Append("         COLS.COLUMN_NAME");
            sb.AppendFormat("        FROM {0}.SYSTEM_SCHEMA.SCHEMATA SCH, ", systemCatalogName);
            sb.AppendFormat("        {0}."_MD_".COLS COLS, ", catalogName, schemaVersion);
            sb.AppendFormat("        {0}."_MD_".COL_PRIVILEGES COLP, ", catalogName, schemaVersion);
            sb.AppendFormat("        {0}."_MD_".OBJECTS OBJ, ", catalogName, schemaVersion);
            sb.Append("             MANAGEABILITY.USER_MANAGEMENT.ROLES RL ");
            sb.Append("        WHERE SCH.SCHEMA_UID = OBJ.SCHEMA_UID AND  ");
            sb.Append("              OBJ.OBJECT_UID = COLP.TABLE_UID AND ");
            sb.AppendFormat("              RL.ROLE_NAME = '{0}' AND ", roleName);
            sb.Append("              COLP.GRANTEE = RL.ROLE_ID AND ");
            sb.Append("              COLP.COLUMN_NUMBER = COLS.COLUMN_NUMBER AND  ");
            sb.Append("              COLP.TABLE_UID = COLS.OBJECT_UID AND  ");
            sb.Append("              OBJ.OBJECT_SECURITY_CLASS = 'UT' AND ");
            sb.Append("              NOT (SCH.SCHEMA_NAME = '@MAINTAIN_SCHEMA@' OR  ");
            sb.Append("                   SCH.SCHEMA_NAME LIKE 'HP\\_%') ");
            sb.Append("  FOR READ UNCOMMITTED ACCESS ");
            sb.Append(") AS CP");
            return sb.ToString();
        }

        //static public OdbcDataReader ExecuteSelectAllCatPrivilegesByRoleName(string roleName, Connection aConnection)
        //{
        //}

        #endregion Public methods

        #region Private methods

        static private int ExecuteNonQuery(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteNonQuery(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Security, TRACE_SUB_AREA_NAME, false);
        }

        static private OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand)
        {
            return Utilities.ExecuteReader(anOpenCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Security, TRACE_SUB_AREA_NAME, false);
        }

        #endregion Private Methods
    }
}
