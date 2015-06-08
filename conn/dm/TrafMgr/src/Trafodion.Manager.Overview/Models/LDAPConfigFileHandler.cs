//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using System.IO;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// call config procedures and extract data
    /// </summary>
    public class LDAPConfigFileHandler : FileHandler
    {
        private const String _ListLDAPConfigProcedure = "CALL MANAGEABILITY.NCI.LISTLDAPCONFIG(?, ?, ?, ?, ?);";
        private const String _UpdateLDAPConfigProcedure = "CALL MANAGEABILITY.NCI.UPDATELDAPCONFIG('{0}','{1}','{2}',{3});";
        private const String _ViewCertificateProcedure = "CALL MANAGEABILITY.NCI.VIEWCERTIFICATE('{0}', ?, ?);";

        public enum ConfigOperation { List, Update };

        public LDAPConfigFileHandler(ConnectionDefinition aConnectionDefinition)
        {
            this.theConnectionDefinition = aConnectionDefinition;
        }

        /// <summary>
        /// load configuration file content and default role of dbroot user
        /// </summary>
        /// <param name="configContent"></param>
        /// <param name="cerContent"></param>
        /// <param name="LDAPRCContent"></param>
        /// <param name="dbRootDefaultRole"></param>
        public void LoadConfigAndDefaultRole(out string configContent, out string cerContent, out string LDAPRCContent)
        {
            listLDAPConfig(out configContent, out cerContent, out LDAPRCContent);
            //dbRootDefaultRole = getAuthenticationTypeOfDB_ROOT();
        }

        /// <summary>
        /// get content of config files
        /// </summary>
        /// <param name="configContent">content of .sqldapconfig</param>
        /// <param name="cerContent">content of certifiate file</param>
        /// <param name="LDAPRCContent">content of .ldaprc. it is used only if TLS_CACERTFilename doesn't exist in .sqldapconfig</param>
        public void listLDAPConfig(out string configContent, out string cerContent, out string LDAPRCContent)
        {
            Connection theConnection = GetConnection();

            try
            {
                OdbcCommand theQuery = new OdbcCommand(_ListLDAPConfigProcedure, theConnection.OpenOdbcConnection);
                theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                //since command cobject is preserved, clear the parameters every time
                theQuery.Parameters.Clear();

                OdbcParameter param1 = theQuery.Parameters.Add("@CONFIGCONTENT", OdbcType.Text, 10240);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;

                OdbcParameter param2 = theQuery.Parameters.Add("@CERCONTENT", OdbcType.Text, 10240);
                param2.Direction = System.Data.ParameterDirection.Output;
                param2.DbType = System.Data.DbType.String;

                OdbcParameter param3 = theQuery.Parameters.Add("@LDAPRCCONTENT", OdbcType.Text, 10240);
                param3.Direction = System.Data.ParameterDirection.Output;
                param3.DbType = System.Data.DbType.String;

                OdbcParameter param4 = theQuery.Parameters.Add("@TEMPCONFIGFILENAME", OdbcType.Text, 256);
                param4.Direction = System.Data.ParameterDirection.Output;
                param4.DbType = System.Data.DbType.String;

                OdbcParameter param5 = theQuery.Parameters.Add("@TEMPCERFILENAME", OdbcType.Text, 256);
                param5.Direction = System.Data.ParameterDirection.Output;
                param5.DbType = System.Data.DbType.String;

                //Execute the SPJ
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "CONFIG", true);

                //Get config
                configContent = theQuery.Parameters["@CONFIGCONTENT"].Value.ToString();
                if ((configContent == null) || (configContent.Trim().Length == 0))
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPCONFIGFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        //read the temp file
                        configContent = getTemporaryFileContent(temporaryFileName);
                    }
                }

                //Get certificate content
                cerContent = theQuery.Parameters["@CERCONTENT"].Value as string;
                if ((cerContent == null) || (cerContent.Trim().Length == 0))
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPCERFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        cerContent = getTemporaryFileContent(temporaryFileName);
                    }
                }

                //get LDAPRC content
                LDAPRCContent = theQuery.Parameters["@LDAPRCCONTENT"].Value as string;

                if (configContent != null && configContent.StartsWith("\0\0"))
                    configContent = configContent.Substring(2);
                if (cerContent!=null && cerContent.StartsWith("\0\0"))
                    cerContent = cerContent.Substring(2);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        public void updateLDAPConfig(string configContent,string cerPath, string cerContent)
        {
            Connection theConnection = GetConnection();
            int creatFlag,i=0;
            int MaxDataSize = 10240;
            string partConfigContent, partCerContent, leftConfigContent, leftCerContent;

            try
            {
                do
                {
                    if (configContent.Length > i * MaxDataSize)
                    {
                        if (configContent.Length > (i + 1) * MaxDataSize)
                            partConfigContent = configContent.Substring(i * MaxDataSize, MaxDataSize);
                        else
                            partConfigContent = configContent.Substring(i * MaxDataSize, configContent.Length - i * MaxDataSize);
                    }
                    else
                        partConfigContent = null;
                    if (configContent.Length > (i + 1) * MaxDataSize)
                        leftConfigContent = configContent.Substring((i + 1) * MaxDataSize);
                    else
                        leftConfigContent = null;
                    if (cerContent.Length > i * MaxDataSize)
                    {
                        if (cerContent.Length > (i + 1) * MaxDataSize)
                            partCerContent = cerContent.Substring(i * MaxDataSize, MaxDataSize);
                        else
                            partCerContent = cerContent.Substring(i * MaxDataSize, cerContent.Length - i * MaxDataSize);
                    }
                    else
                        partCerContent = null;
                    if (cerContent.Length > (i + 1) * MaxDataSize)
                        leftCerContent = cerContent.Substring((i + 1) * MaxDataSize);
                    else
                        leftCerContent = null;

                    if (i == 0)
                    {
                        creatFlag = 1;
                    }
                    else
                    {
                        creatFlag = 0;
                    }
                    string cmdText = String.Format(_UpdateLDAPConfigProcedure, new object[] { partConfigContent, cerPath, partCerContent, creatFlag});
                    OdbcCommand theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                    theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                    //since command object is preserved, clear the parameters every time
                    theQuery.Parameters.Clear();
                    //Execute the SPJ
                    Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "CONFIG", true);
                    i++;
                } while (leftConfigContent != null && leftConfigContent.Length > 0 || leftCerContent != null && leftCerContent.Length > 0);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        public void viewCertifcate(string filePath,out string cerContent)
        {
            Connection theConnection = GetConnection();

            try
            {
                string cmdText = String.Format(_ViewCertificateProcedure, new object[] { filePath });
                OdbcCommand theQuery = new OdbcCommand(cmdText, theConnection.OpenOdbcConnection);
                theQuery.CommandType = System.Data.CommandType.StoredProcedure;
                //since command object is preserved, clear the parameters every time
                theQuery.Parameters.Clear();

                OdbcParameter param1 = theQuery.Parameters.Add("@CERCONTENT", OdbcType.Text, 10240);
                param1.Direction = System.Data.ParameterDirection.Output;
                param1.DbType = System.Data.DbType.String;
                OdbcParameter param2 = theQuery.Parameters.Add("@TEMPCERFILENAME", OdbcType.Text, 256);
                param2.Direction = System.Data.ParameterDirection.Output;
                param2.DbType = System.Data.DbType.String;

                //Execute the SPJ
                Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "CONFIG", true);

                //Get the certificate content
                cerContent = theQuery.Parameters[0].Value is DBNull ? string.Empty : (string)theQuery.Parameters["@CERCONTENT"].Value;
                if ((cerContent == null) || (cerContent.Trim().Length == 0))
                {
                    string temporaryFileName = theQuery.Parameters["@TEMPCERFILENAME"].Value.ToString();
                    if ((temporaryFileName != null) && (temporaryFileName.Trim().Length > 0))
                    {
                        cerContent = getTemporaryFileContent(temporaryFileName);
                    }
                }
                if (cerContent != null && cerContent.StartsWith("\0\0"))
                    cerContent = cerContent.Substring(2);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// get the authentication type of user DB__ROOT
        /// </summary>
        /// <returns></returns>
        public string getAuthenticationTypeOfDB_ROOT(){
            Connection theConnection = GetConnection();

            try
            {
                OdbcCommand theQuery = new OdbcCommand("SELECT AUTHENTICATION_TYPE FROM TRAFODION_INFORMATION_SCHEMA.USER_INFO WHERE USER_NAME='DB__ROOT'", theConnection.OpenOdbcConnection);
                theQuery.CommandType = System.Data.CommandType.Text;
                //since command object is preserved, clear the parameters every time
                theQuery.Parameters.Clear();
                OdbcDataReader reader= Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "CONFIG", true);
                //Get the data
                reader.Read();

                return reader.GetString(0);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        public void canRemoveSection(out bool disableEnterprise, out bool disableCluster, out string dbRootDefaultRole)
        {
            Connection theConnection = GetConnection();
            string sql = "SELECT count(*) from MANAGEABILITY.TRAFODION_INFORMATION_SCHEMA.USER_INFO where authentication_type = 'LOCAL' or authentication_type = 'ENTERPRISE'";

            try
            {
                dbRootDefaultRole = getAuthenticationTypeOfDB_ROOT();

                OdbcCommand theQuery = new OdbcCommand(sql, theConnection.OpenOdbcConnection);
                theQuery.CommandType = System.Data.CommandType.Text;
                //since command object is preserved, clear the parameters every time
                theQuery.Parameters.Clear();
                OdbcDataReader reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "CONFIG", true);
                //Get the data
                reader.Read();
                int count = reader.GetInt32(0);
                disableEnterprise = (count != 0);
                reader.Close();

                sql = "SELECT count(*) from MANAGEABILITY.TRAFODION_INFORMATION_SCHEMA.USER_INFO where authentication_type = 'REMOTE' or authentication_type = 'CLUSTER'";
                theQuery.CommandText = sql;
                theQuery.Parameters.Clear();
                reader = Utilities.ExecuteReader(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Overview, "CONFIG", true);
                //Get the data
                reader.Read();
                count = reader.GetInt32(0);

                disableCluster = (count != 0);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }
    }
}
