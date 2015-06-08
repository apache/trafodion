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
using System.Data;
using System.Data.Odbc;
using System.Linq;
using Trafodion.Manager.Framework.Connections;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.SecurityArea.Controls;
using System.Collections;
using System.Collections.Generic;

namespace Trafodion.Manager.SecurityArea.Model
{
    public class DirectoryServer : SecurityObject
    {
        #region Fields

        // To disable DEFAULT DS.
        public const string DEFAULT_LDAP_DOMAIN_NAME = "TRAFODIONDIRECTORYSERVER";
        public const string DEFAULT_ACTIVE_DIRECTORY_GLOBAL_CATALOG_DOMAIN_NAME = "TRAFODIONDIRECTORYSERVER";
        public const string DEFAULT_ENTRY_DOMAIN_NAME = "DEFAULT";
        public const short  DEFAULT_ENTRY_USAGE_PRIORITY = 0;
        public const int    DEFAULT_SERVER_VERSION = 3;
        public const short  DEFAULT_ENCRYPTION = (short) ENCRYPTION_SUPPORT.NONE;

        public const short ATTR_VECTOR_HOST                 = 1;
        public const short ATTR_VECTOR_PORT_NUMBER          = 1 << 1; // 2;
        public const short ATTR_VECTOR_VERSION              = 1 << 2; // 4;
        public const short ATTR_VECTOR_SEARCHUSERDN         = 1 << 3; // 8;
        public const short ATTR_VECTOR_SEARCHUSERPASSWORD   = 1 << 4; // 16;
        public const short ATTR_VECTOR_ENCRYPTION           = 1 << 5; // 32;
        public const short ATTR_VECTOR_CACERT               = 1 << 6; // 64;
        public const short ATTR_VECTOR_CONFIGTEXT           = 1 << 7; // 128;

        // Watch out the enum starts with 1 and index start with 0 (the label will be used to construct the 
        // combobox.  
        public enum ENCRYPTION_SUPPORT : short { NONE, SSL, TLS };

        public static string[] ENCRYPTION_SUPPORT_LABEL = { "No Encryption", "Use SSL", "Use TLS" };

        public enum SERVER_TYPE : short { LDAP = 0, ACTIVE_DIRECTORY };

        // This has to match the column order specified in Queries.ExecuteInfoDirectoryServer() method
        public enum DetailDSColumns { SERVER_TYPE = 0, DOMAIN_NAME, USAGE_PRI, LDAPHOST, LDAPPORT, LDAPVERSION, SEARCHUSERDN, 
                                      SEARCHUSERPWD, LDAPENABLESSL, TIME_STAMP, CACERT, LINE_NUM, RAW_TEXT, PARAM_NAME, PARAM_VALUE };

        public enum CONFIG_PARAMETERS_TYPE { LDAP_COMMON_CONFIG = 0, LDAP_CONFIG, AD_COMMON_CONFIG, AD_CONFIG, };
        
        public enum LDAP_COMMON_CONFIG_PARAMETERS 
        { 
            DirectoryBase = 0, 
            UniqueIdentifier, 
            UserIdentifierFormat,
            Comment
        };

        public enum LDAP_CONFIG_PARAMETERS 
        { 
            DirectoryBase = 0, 
            UniqueIdentifier,
            Comment
        };

        public enum AD_COMMON_CONFIG_PARAMETERS
        {
            DirectoryBase = 0, 
            UniqueIdentifier, 
            UserIdentifier,
            UserIdentifierFormat, 
            UserIdentifierMapping, 
            DomainAttribute,
            DomainAttributeFormat,
            Comment
        };

        public enum AD_CONFIG_PARAMETERS
        {
            DirectoryBase = 0,
            UniqueIdentifier,
            Comment
        };

        public const string CONFIG_PARAMETER_COMMENT = "Comment";

        public const string SERVER_TYPE_CONFIG_PARAMETER_NAME = "SERVERTYPE";
        public enum SERVER_TYPE_CONFIG_PARAMETER_VALUES { LDAP = 0, ADGC, ADDC };

        public enum ACTIVE_DIRECTORY_DOMAIN_TYPE { GLOBAL_CATALOG = 0, DOMAIN_CONTROLLER };
        public static string[] ACTIVE_DIRECTORY_DOMAIN_TYPE_LABEL = { "Global Catalog", "Domain Controller" };

        private string      _domainName = null;
        private short       _usagePriority = 0;
        private short       _attrPresentVector = 0;     // bit map to indicate an attr is present
        private short       _updatedAttrVector = 0;     // bit map to indicate an attr is updated
        private string      _hostName = null;
        private int         _portNumber = 0;
        private int         _version = 0;
        private string      _searchUserDN = null;
        private string      _searchUserPassword = null;
        private short       _encryption = 0;
        private string      _caCert = null;
        private string      _configText = null;
        private long        _timeStamp = 0;
        private ArrayList   _configParameters = new ArrayList();
        private ArrayList   _commonConfigParameters = new ArrayList();
        private bool        _commonConfigParametersUpdated = false;
        private SERVER_TYPE _serverType = SERVER_TYPE.LDAP;
        private ACTIVE_DIRECTORY_DOMAIN_TYPE _domainType = ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG;

        #endregion Fields

        #region Properties

        /// <summary>
        /// ServerType: the server type of this directory server.
        /// </summary>
        public SERVER_TYPE ServerType
        {
            get { return _serverType; }
            set { _serverType = value; }
        }

        /// <summary>
        /// DomainType: type active directory domain type. Meaningful only if server type is active directory.
        /// </summary>
        public ACTIVE_DIRECTORY_DOMAIN_TYPE DomainType
        {
            get { return _domainType; }
            set { _domainType = value; }
        }

        /// <summary>
        /// Domain Name: the domain name of the Directory Server.
        /// </summary>
        public string DomainName
        {
            get { return _domainName; }
            set { _domainName = value; }
        }

        /// <summary>
        /// UsagePriority: the usage priority of the Directory Server.
        /// </summary>
        public short UsagePriority
        {
            get { return _usagePriority; }
            set { _usagePriority = value; }
        }

        /// <summary>
        /// AttrPresentVector: bit map indicating whether an attribute is present
        /// </summary>
        public short AttrPresentVector
        {
            get { return _attrPresentVector; }
            set { _attrPresentVector = value; }
        }

        /// <summary>
        /// UpdatedAttrVector: bit map indicating whether an attribute is updated
        /// </summary>
        public short UpdatedAttrVector
        {
            get { return _updatedAttrVector; }
            set { _updatedAttrVector = value; }
        }

        /// <summary>
        /// CommonParametersUpdated: if the commom parameters have been updated.
        /// </summary>
        public bool CommonParametersUpdated
        {
            get { return _commonConfigParametersUpdated; }
            set { _commonConfigParametersUpdated = value; }
        }

        /// <summary>
        /// HostName: the Host Name of the Directory Server.
        /// </summary>
        public string HostName
        {
            get { return _hostName; }
            set 
            {
                if (_hostName != value)
                {
                    _hostName = value;
                    SetUpdatedVector(ATTR_VECTOR_HOST);
                }
            }
        }

        /// <summary>
        /// PortNumber: the Port Number of the Directory Server.
        /// </summary>
        public int PortNumber
        {
            get { return _portNumber; }
            set 
            { 
                if (_portNumber != value)
                {
                    _portNumber = value;
                    SetUpdatedVector(ATTR_VECTOR_PORT_NUMBER);
                }
            }
        }

        /// <summary>
        /// SearchUserDN: the Search User DN of the Directory Server.
        /// </summary>
        public string SearchUserDN
        {
            get { return _searchUserDN; }
            set 
            {
                if (_searchUserDN != value)
                {
                    _searchUserDN = value;
                    SetUpdatedVector(ATTR_VECTOR_SEARCHUSERDN);
                }
            }
        }

        /// <summary>
        /// SearchUserPassword: the Search User Password of the Directory Server.
        /// </summary>
        public string SearchUserPassword
        {
            get { return _searchUserPassword; }
            set 
            {
                if (_searchUserPassword != value)
                {
                    _searchUserPassword = value;
                    SetUpdatedVector(ATTR_VECTOR_SEARCHUSERPASSWORD);
                }
            }
        }

        /// <summary>
        /// Version: the LDAP Version of the Directory Server.
        /// </summary>
        public int Version
        {
            get { return _version; }
            set 
            {
                if (_version != value)
                {
                    _version = value;
                    SetUpdatedVector(ATTR_VECTOR_VERSION);
                }
            }
        }

        /// <summary>
        /// Encryption: the encryption argorithm of the Directory Server.
        /// </summary>
        public short Encryption
        {
            get { return _encryption; }
            set 
            {
                if (_encryption != value)
                {
                    _encryption = value;
                    SetUpdatedVector(ATTR_VECTOR_ENCRYPTION);
                }
            }
        }

        /// <summary>
        /// EncryptionForSPJCall: the encryption argorithm to be used for SPJ call.
        /// Note: The parameter input for encryption argorithm needs to add 1 to the definition of
        /// Encryption argoritm. 
        /// </summary>
        public short EncryptionForSPJCall
        {
            get { return (short)(Encryption + 1); }
        }

        /// <summary>
        /// CACert: the CA certificate of the Directory Server.
        /// </summary>
        public string CACert
        {
            get { return _caCert; }
            set 
            {
                if (_caCert != value)
                {
                    _caCert = value;
                    SetUpdatedVector(ATTR_VECTOR_CACERT);
                }
            }
        }

        /// <summary>
        /// ConfigText: the Config Text of the Directory Server.
        /// Note: The config text is assembled from the config parameters.
        /// </summary>
        public string ConfigText
        {
            get 
            {
                string text = "";
                foreach (string[] pair in _configParameters.ToArray())
                {
                    if (!String.IsNullOrEmpty(pair[0]) && !String.IsNullOrEmpty(pair[1]))
                    {
                        text += ProcessingOneConfigParameter(pair);
                    }
                }

                // Now, inject the server type parameter.
                if (ServerType == SERVER_TYPE.LDAP)
                {
                    text += String.Format("{0} {1} {2}", SERVER_TYPE_CONFIG_PARAMETER_NAME, SERVER_TYPE_CONFIG_PARAMETER_VALUES.LDAP.ToString(), Environment.NewLine);
                }
                else
                {
                    if (DomainType == ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG)
                    {
                        text += String.Format("{0} {1} {2}", SERVER_TYPE_CONFIG_PARAMETER_NAME, SERVER_TYPE_CONFIG_PARAMETER_VALUES.ADGC.ToString(), Environment.NewLine);
                    }
                    else
                    {
                        text += String.Format("{0} {1} {2}", SERVER_TYPE_CONFIG_PARAMETER_NAME, SERVER_TYPE_CONFIG_PARAMETER_VALUES.ADDC.ToString(), Environment.NewLine);
                    }
                }
                
                return text; 
            }
        }

        /// <summary>
        /// ConfigParameters: the config parameters for the direcotry server
        /// </summary>
        public ArrayList ConfigParameters
        {
            get { return _configParameters; }
            set 
            {
                if (_configParameters != null && value != null)
                {
                    if (!AreParemeterListsEqual(_configParameters, value))
                    {
                        SetUpdatedVector(ATTR_VECTOR_CONFIGTEXT);
                    }
                }
                _configParameters = value;
            }
        }

        /// <summary>
        /// CommonConfigParameters: common config parameters for the active directory server. 
        /// Note: only valid for active directory
        /// </summary>
        public ArrayList CommonConfigParameters
        {
            get { return _commonConfigParameters; }
            set
            {
                if (_commonConfigParameters != null && value != null)
                {
                    _commonConfigParametersUpdated = !AreParemeterListsEqual(_commonConfigParameters, value);
                }
                _commonConfigParameters = value;
                
            }
        }

        /// <summary>
        /// CommonConfigText: the Config Text of the common parameters of an active Directory.
        /// Note: The config text is assembled from the common config parameters.
        /// Note: This is valid only for active directory.
        /// </summary>
        public string CommonConfigText
        {
            get
            {
                string text = "";

                // Make sense only to LDAP server and active directory/Glboal Catalog.
                foreach (string[] pair in _commonConfigParameters.ToArray())
                {
                    if (!String.IsNullOrEmpty(pair[0]) && !String.IsNullOrEmpty(pair[1]))
                    {
                        text += ProcessingOneConfigParameter(pair);
                    }
                }

                return text;
            }
        }

        /// <summary>
        /// AreParemeterListsEqual: A compare method for parameter lists.
        /// </summary>
        /// <param name="sourceParameterList"></param>
        /// <param name="targetParameterList"></param>
        /// <returns></returns>
        public static bool AreParemeterListsEqual(ArrayList sourceParameterList, ArrayList targetParameterList)
        {
            if (sourceParameterList.Count != targetParameterList.Count)
                return false;
            for (int i = 0; i < sourceParameterList.Count; i++)
            {
                string[] sourceParam = (string[]) sourceParameterList[i];
                string[] targetParam = (string[])targetParameterList[i];
                if (sourceParam[0].Equals(targetParam[0]) && sourceParam[1].Equals(targetParam[1]))
                    continue;
                return false;
            }
            return true;
        }

        /// <summary>
        /// IsParameterListEmpty: Return true if the list is empty.
        /// </summary>
        /// <param name="paramList"></param>
        /// <returns></returns>
        public static bool IsParameterListEmpty(ArrayList paramList)
        {
            foreach (string[] pair in paramList.ToArray())
            {
                if (!String.IsNullOrEmpty(pair[0]) && !String.IsNullOrEmpty(pair[1]))
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// TimeStamp: the time stampof the last update to the Directory Server.
        /// </summary>
        public long TimeStamp
        {
            get { return _timeStamp; }
            set { _timeStamp = value; }
        }

        #endregion Properties

        #region Constructors

        ///// <summary>
        ///// Constructor
        ///// </summary>
        ///// <param name="aConnectionDefinition"></param>
        ///// <param name="aDomainName"></param>
        ///// <param name="aUsagePri"></param>
        //public DirectoryServer(ConnectionDefinition aConnectionDefinition, SERVER_TYPE aServerType, string aDomainName, short aUsagePri) 
        //    : base(aConnectionDefinition)
        //{
        //    _serverType = aServerType;
        //    _domainName = aDomainName;
        //    _usagePriority = aUsagePri;
        //}

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aRow"></param>
        public DirectoryServer(ConnectionDefinition aConnectionDefinition, iGRow aRow)
            : base(aConnectionDefinition)
        {
            // Fill all attributes from a ROW data from IGrid
            _domainName = aRow.Cells[(int)DirectoryServersUserControl.GridColumns.DOMAIN_NAME].Text.Trim();
            _usagePriority = short.Parse(aRow.Cells[(int)DirectoryServersUserControl.GridColumns.USAGE_PRI].Text.Trim());

            LoadAttributes();           
        }

        /// <summary>
        /// Constructor for a vinilla directory server and let the UI User control to fill in everything. 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aRow"></param>
        public DirectoryServer(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            CommonConfigParameters = GetCommonConfigParameters(aConnectionDefinition);
            CommonParametersUpdated = false;  // reset the flag for now.
        }

        #endregion Constructors

        #region Public mehtods

        /// <summary>
        /// Refresh all attributes from server. 
        /// </summary>
        public void Refresh()
        {
            LoadAttributes();
            //GetConfigText();
        }

        public static ArrayList GetCommonConfigParameters(ConnectionDefinition aConnectionDefinition)
        {
            OdbcDataReader dsReader = null;
            Connection conn = new Connection(aConnectionDefinition);
            if (null == conn)
            {
                return null;
            }

            ArrayList commonParameters = new ArrayList();

            try
            {
                dsReader = Queries.ExecuteInfoDefaultDirectoryServerConfigParameters(conn);
                while (dsReader.Read())
                {
                    string param = null;
                    string value = null;

                    try
                    {
                        param = (dsReader[DetailDSColumns.PARAM_NAME.ToString()] as string).Trim();
                    }
                    catch (Exception) { }

                    try
                    {
                        value = (dsReader[DetailDSColumns.PARAM_VALUE.ToString()] as string).Trim();
                    }
                    catch (Exception) { }

                    if (!string.IsNullOrEmpty(param))
                    {
                        commonParameters.Add(new string[] { param, value });
                    }
                }
            }
            finally
            {
                conn.Close();
            }

            return commonParameters;
        }

        /// <summary>
        /// Get default directory server. 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        //public static DirectoryServer GetDefaultDirectoryServer(ConnectionDefinition aConnectionDefinition)
        //{
        //    DirectoryServer defaultDS = new DirectoryServer(aConnectionDefinition, DEFAULT_SERVER_NAME, DEFAULT_SERVER_USAGE_PRI);
        //    defaultDS.Refresh();
        //    return defaultDS;
        //}

        /// <summary>
        /// To create a directory server entry. 
        /// </summary>
        public void Create()
        {

            if (!GetConnection())
                return;
            try
            {
                Queries.ExecuteCreateDirectoryServer(CurrentConnection, this);
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// To update the directory server entry. 
        /// </summary>
        public void Alter()
        {

            if (!GetConnection())
                return;
            try
            {
                Queries.ExecuteAlterDirectoryServer(CurrentConnection, this);
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// To update the common config parameters for the active directory. 
        /// </summary>
        public void AlterCommonConfigParameters()
        {
            if (!CommonParametersUpdated || 
                (ServerType == SERVER_TYPE.ACTIVE_DIRECTORY && DomainType == ACTIVE_DIRECTORY_DOMAIN_TYPE.DOMAIN_CONTROLLER))
            {
                // Nothing updated or this is a domain controller. 
                return;
            }

            if (!GetConnection())
                return;
            try
            {
                Queries.ExecuteAlterDirectoryServerCommonParameters(CurrentConnection, this);
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// To remove the directory server entry.
        /// </summary>
        public void Drop()
        {
            if (!GetConnection())
                return;
            try
            {
                Queries.ExecuteDropDirectoryServer(CurrentConnection, DomainName, UsagePriority);
            }
            finally
            {
                CloseConnection();
            }
        }

        /// <summary>
        /// To check if an attribute is present. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckPresentVector(short vector)
        {
            return ((AttrPresentVector & vector) > 0);
        }

        /// <summary>
        /// To set the given attribute as present.
        /// </summary>
        /// <param name="vector"></param>
        public void SetPresentVector(short vector)
        {
            AttrPresentVector |= vector;
        }

        /// <summary>
        /// To check if an attribute is updated. 
        /// </summary>
        /// <param name="vector"></param>
        /// <returns></returns>
        public bool CheckUpdatedVector(short vector)
        {
            return ((UpdatedAttrVector & vector) > 0);
        }

        /// <summary>
        /// to set the given attribute as present. 
        /// </summary>
        /// <param name="vector"></param>
        public void SetUpdatedVector(short vector)
        {
            UpdatedAttrVector |= vector;
        }

        /// <summary>
        /// to reset the present vector. 
        /// </summary>
        public void ResetPresentVector()
        {
            AttrPresentVector = 0;
        }

        /// <summary>
        /// To reset the update vector.
        /// </summary>
        public void ResetUpdatedVector()
        {
            UpdatedAttrVector = 0;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Input a config parameter name and value pairs and return a string to represent the parameter
        /// to be inserted into the Directory Server.
        /// </summary>
        /// <returns></returns>
        private string ProcessingOneConfigParameter(string[] pair)
        {
            string text = "";

            if (pair[0].Trim().Equals(CONFIG_PARAMETER_COMMENT, StringComparison.OrdinalIgnoreCase))
            {
                if (pair[1].StartsWith("#"))
                {
                    text = String.Format("{0} {1}", pair[1], Environment.NewLine);
                }
                else
                {
                    text = String.Format("#{0} {1}", pair[1], Environment.NewLine);
                }
            }
            else
            {
                text = String.Format("{0} {1} {2}", pair[0], pair[1], Environment.NewLine);
            }

            return text;
        }

        /// <summary>
        /// To load attributes from server. 
        /// </summary>
        private void LoadAttributes()
        {
            LoadAttributes(_domainName, _usagePriority);
            LoadCommonConfigParameters();
        }

        /// <summary>
        /// To load attributes from server. 
        /// </summary>
        /// <param name="aDomainName"></param>
        /// <param name="aUsagePriority"></param>
        private void LoadAttributes(string aDomainName, short aUsagePriority)
        {
            OdbcDataReader dsReader = null;
            bool first_time = true;
            SERVER_TYPE_CONFIG_PARAMETER_VALUES server_type = SERVER_TYPE_CONFIG_PARAMETER_VALUES.LDAP;

            if (!GetConnection())
                return;

            try
            {
                dsReader = Queries.ExecuteInfoDirectoryServer(CurrentConnection, aDomainName, aUsagePriority);
                _configParameters.Clear();

                while (dsReader.Read())
                {
                    if (first_time)
                    {
                        _attrPresentVector = 0;
                        try
                        {
                            HostName = (dsReader[DetailDSColumns.LDAPHOST.ToString()] as string).Trim();
                            if (!String.IsNullOrEmpty(HostName)) SetPresentVector(ATTR_VECTOR_HOST);
                        }
                        catch (Exception) { HostName = ""; }

                        try
                        {
                            PortNumber = (int)dsReader[DetailDSColumns.LDAPPORT.ToString()];
                            SetPresentVector(ATTR_VECTOR_PORT_NUMBER);
                        }
                        catch (Exception) { PortNumber = 0; }

                        try
                        {
                            Version = (int)dsReader[DetailDSColumns.LDAPVERSION.ToString()];
                            SetPresentVector(ATTR_VECTOR_VERSION);
                        }
                        catch (Exception) { Version = DEFAULT_SERVER_VERSION; }

                        try
                        {
                            SearchUserDN = (dsReader[DetailDSColumns.SEARCHUSERDN.ToString()] as string).Trim();
                            if (!String.IsNullOrEmpty(SearchUserDN)) SetPresentVector(ATTR_VECTOR_SEARCHUSERDN);
                        }
                        catch (Exception) { SearchUserDN = ""; }

                        try
                        {
                            SearchUserPassword = (dsReader[DetailDSColumns.SEARCHUSERPWD.ToString()] as string).Trim();
                            if (!String.IsNullOrEmpty(SearchUserPassword)) SetPresentVector(ATTR_VECTOR_SEARCHUSERPASSWORD);
                        }
                        catch (Exception) { SearchUserPassword = ""; }

                        try
                        {
                            Encryption = (short)dsReader[DetailDSColumns.LDAPENABLESSL.ToString()];
                            SetPresentVector(ATTR_VECTOR_ENCRYPTION);
                        }
                        catch (Exception) { Encryption = DEFAULT_ENCRYPTION; }

                        try
                        {
                            CACert = (dsReader[DetailDSColumns.CACERT.ToString()] as string).Trim();
                            if (!String.IsNullOrEmpty(CACert)) SetPresentVector(ATTR_VECTOR_CACERT);
                        }
                        catch (Exception) { CACert = ""; }

                        ParseParameters(dsReader, ref server_type);
                       
                        first_time = false;
                    }
                    else
                    {
                        ParseParameters(dsReader, ref server_type);
                    }
                }

                if (_configParameters.Count > 0)
                {
                    SetPresentVector(ATTR_VECTOR_CONFIGTEXT);
                }

                // We should have gotten the server type in the parameters.
                if (server_type == SERVER_TYPE_CONFIG_PARAMETER_VALUES.LDAP)
                {
                    ServerType = SERVER_TYPE.LDAP;
                }
                else
                {
                    ServerType = SERVER_TYPE.ACTIVE_DIRECTORY;
                    if (server_type == SERVER_TYPE_CONFIG_PARAMETER_VALUES.ADGC)
                    {
                        DomainType = ACTIVE_DIRECTORY_DOMAIN_TYPE.GLOBAL_CATALOG;
                    }
                    else
                    {
                        DomainType = ACTIVE_DIRECTORY_DOMAIN_TYPE.DOMAIN_CONTROLLER;
                    }
                }
            }
            finally
            {
                CloseConnection();

                // We have just loaded the attributes from server, reset the update vector.
                ResetUpdatedVector();
            }
        }

        /// <summary>
        /// Parse a parameter from a row read from DataReader.
        /// Note: A param will be added only if both param name and value are not null. 
        /// </summary>
        /// <param name="dsReader"></param>
        /// <param name="server_type"></param>
        private void ParseParameters(OdbcDataReader dsReader, ref SERVER_TYPE_CONFIG_PARAMETER_VALUES server_type)
        {
            string param = null;
            string value = null;

            try
            {
                param = (dsReader[DetailDSColumns.PARAM_NAME.ToString()] as string).Trim();
            }
            catch (Exception) { }

            try
            {
                value = (dsReader[DetailDSColumns.PARAM_VALUE.ToString()] as string).Trim();
            }
            catch (Exception) { }

            if (!String.IsNullOrEmpty(param) && !String.IsNullOrEmpty(value))
            {
                if (param.Equals(SERVER_TYPE_CONFIG_PARAMETER_NAME))
                {
                    try
                    {
                        server_type = (SERVER_TYPE_CONFIG_PARAMETER_VALUES)Enum.Parse(typeof(SERVER_TYPE_CONFIG_PARAMETER_VALUES), value);
                    }
                    catch (Exception)
                    { }
                }
                else
                {
                    _configParameters.Add(new string[] { param, value });
                }
            }
        }

        /// <summary>
        /// Load common config parameters from server.
        /// Note: the common config parameters are loaded from the 'DEFAULT' entry. 
        /// </summary>
        private void LoadCommonConfigParameters()
        {

            OdbcDataReader dsReader = null;

            if (!GetConnection())
                return;

            try
            {
                dsReader = Queries.ExecuteInfoDefaultDirectoryServerConfigParameters(CurrentConnection);
                _commonConfigParameters.Clear();
                while (dsReader.Read())
                {
                    string param = null;
                    string value = null;

                    try
                    {
                        param = (dsReader[DetailDSColumns.PARAM_NAME.ToString()] as string).Trim();
                    }
                    catch (Exception) { }

                    try
                    {
                        value = (dsReader[DetailDSColumns.PARAM_VALUE.ToString()] as string).Trim();
                    }
                    catch (Exception) { }

                    if (!string.IsNullOrEmpty(param))
                    {
                        _commonConfigParameters.Add(new string[] { param, value });
                    }
                }
            }
            finally
            {
                CloseConnection();

                // We have just loaded the attributes from server, reset the update vector.
                _commonConfigParametersUpdated = false;
            }
        }



        /// <summary>
        /// To retrive the config text of the directory server. 
        /// </summary>
        private void GetConfigText()
        {
            OdbcDataReader dsReader = null;

            if (!GetConnection())
                return;

            bool first = true;
            try
            {
                dsReader = Queries.ExecuteInfoDirectoryServerConfigText(CurrentConnection, _domainName, _usagePriority);
                while (dsReader.Read())
                {
                    string text = dsReader["RAW_TEXT"] as string;
                    if (first)
                    {
                        _configText += text.Trim();
                        first = false;
                    }
                    else
                    {
                        _configText += "\r\n" + text.Trim();
                    }
                }
            }
            finally
            {
                CloseConnection();
            }

            if (!String.IsNullOrEmpty(_configText))
            {
                SetPresentVector(ATTR_VECTOR_CONFIGTEXT);
            }
        }

        /// <summary>
        /// To retrive the config prameters of the directory server. 
        /// </summary>
        //private void GetConfigParameters()
        //{
        //    OdbcDataReader dsReader = null;

        //    if (!GetConnection())
        //        return;

        //    bool first = true;
        //    _dataTable = new DataTable();
        //    _dataTable.Columns.Add("Name");
        //    _dataTable.Columns.Add("Value");
        //    try
        //    {
        //        dsReader = Queries.ExecuteInfoDirectoryServerConfigParameters(CurrentConnection, _domainName, _usagePriority);
        //        while (dsReader.Read())
        //        {
        //            string name = dsReader["PARAM_NAME"] as string;
        //            string value = dsReader["PARAM_VALUE"] as string;
        //            _dataTable.Rows.Add(new object[] {name, value});
        //        }
        //    }
        //    finally
        //    {
        //        CloseConnection();
        //    }

        //    if (!String.IsNullOrEmpty(_configText))
        //    {
        //        SetPresentVector(ATTR_VECTOR_CONFIGTEXT);
        //    }
        //}


        #endregion Private methods
    }
}
