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
using System.ComponentModel;
using System.Data.Odbc;
using System.IO;
using System.Runtime.Serialization;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Connections.Controls;

namespace Trafodion.Manager.Framework.Connections
{
    public class MyConnectionDefinitionComparer : IEqualityComparer<ConnectionDefinition>
    {
        public bool Equals(ConnectionDefinition x, ConnectionDefinition y)
        {
            return x.Equals(y);
        }

        public int GetHashCode(ConnectionDefinition obj)
        {
            return (obj.Name + obj.Host + obj.Port + obj.ClientDataSource + 
                    obj.UserName + obj.UserSpecifiedRole).GetHashCode();
        }
    }

    /// <summary>
    /// The definition of a connection.  The user sees these as "Systems" but there's not a one-to-one
    /// correspondence since there might be several connection definitions that refer to a given system.
    /// There might be one connection defintion using the services user and another using a regular
    /// user for instance.  A connection definition associates a user-chosen
    /// name with the various fields such as username, password, ip address, port, that are are 
    /// needed to make an ODBC connection string.
    /// </summary>
    [Serializable]
    public class ConnectionDefinition : IComparable
    {

        #region MemberVariables

        private int _suppressEvents = 0;

        private string _roleName = null;

        public const int ROLE_NAME_MAX_LENGTH = 128;
        public const int USER_NAME_MAX_LENGTH = 128;
        public const int PASSWORD_MAX_LENGTH = 128;

        private string theName = "";

        private string theHost = "";

        private string thePort = "";

        private string theServerDataSource = "";
        private string theClientDataSource = "";

        [NonSerialized]
        private object _syncRootDoConnection = new object();

        [NonSerialized]
        private string _theConnectedDataSource = "";

        private string theUserName = "";

        [NonSerialized]
        private string _databaseUserName = "";

        [NonSerialized()]
        private string thePassword = "";

        private string theDefaultCatalog = "";

        private string theDefaultSchema = "";

        private string theDriverString = "";

        [NonSerialized()]
        private State theState = State.NotTested;

        [NonSerialized()]
        private long _maxUserTableSize = -1;

        [NonSerialized()]
        private bool thePasswordIsSet = false;

        [NonSerialized()]
        private static List<ConnectionDefinition> theConnectionDefinitions = new List<ConnectionDefinition>();
        [NonSerialized]
        private Dictionary<string, List<string>> theComponentPrivileges = new Dictionary<string, List<string>>();

        [NonSerialized()]
        static private EventHandlerList theEventHandlers = new EventHandlerList();

        private Dictionary<string, string> theProperties = new Dictionary<string, string>();

        private Dictionary<string, object> thePropertyObjects = new Dictionary<string, object>();

        [NonSerialized]
        private Dictionary<string, string> theSessionProperties = new Dictionary<string, string>();

        //Note: this is to be used for the M6 Live Feed broker server name.  We'll try to get this right
        //      after the connection is open.  But, only get it once for a ConnectionDefinition.
        [NonSerialized]
        protected string _theODBCServerName = null;

        #endregion

        /// <summary>
        /// Identifies the server version i.e. SQ100 (M5), SQ110 (M6), SQ120 (M7)
        /// </summary>
        public enum SERVER_VERSION
        {
            SQ100 = 100, //M5
            SQ110 = 110, //M6
            SQ120 = 120, //M7
            SQ130 = 130, //M8 pre-release
            SQ131 = 131, //M8 
            SQ132 = 132, //M8 SP1
            SQ133 = 133, //M8 SP2
            SQ134 = 134, //M8 SP3
            SQ135 = 135, //M8 SP4
            SQ140 = 140, //M9
            SQ141 = 141, //M9 SP1
            SQ142 = 142, //M9 SP2
            SQ143 = 143, //M9 SP3
            SQ150 = 150, //M10
            SQ151 = 151, //M10 SP1
            SQ160 = 160 //M11
        };
        

        // Define Property keys here.
        static public string UserRoleName = "UserRoleName"; //Literal for the property key for saving the user specified role name
        static public string CertificateFilePath = "CertificateFilePath"; //Literal for the property key for saving the certificate file name
        [NonSerialized]
        public const String SQLWB_DataSource = "QWB_DS";
        public const string AdminLoadDataSource = "Admin_Load_DataSource";
        public const string DefaultDataSource = "TDM_Default_DataSource";
        public const string LiveFeedHostNameProperty = "Live_Feed_Host"; // Literal for the property key for Live Feed host name
        public const string LiveFeedPortProperty = "Live_Feed_Port"; // Literal for the property key for Live Feed Host port number
        public const string LiveFeedRetryTimerProperty = "Live_Feed_Retry_Timer"; // Literal for the property key for Live Feed Retry Timer
        public const string LiveFeedOnlyConnectProperty = "LiveFeedOnlyConnect";

        private const string METRIC_QUERY_3_VIEW = "METRIC_QUERY_3";
        private const string METRIC_QUERY_2_VIEW = "METRIC_QUERY_2";
        private const string MANAGEABILITY_INSTANCE_REPOSITORY = "MANAGEABILITY.INSTANCE_REPOSITORY.";
        private const string METRIC_QUERY_3_VIEW_FULL = MANAGEABILITY_INSTANCE_REPOSITORY + METRIC_QUERY_3_VIEW;
        private const string METRIC_QUERY_2_VIEW_FULL = MANAGEABILITY_INSTANCE_REPOSITORY + METRIC_QUERY_2_VIEW;
        private const string SESSION_STATS_VIEW_1 = MANAGEABILITY_INSTANCE_REPOSITORY + "METRIC_SESSION_1";
        private const string SESSION_STATS_VIEW_2 = MANAGEABILITY_INSTANCE_REPOSITORY + "METRIC_SESSION_2";

        public const string DefaultLiveFeedRetryTimer = "30";  // seconds
        public const string DefaultPlatformVersion = "1.4.0"; // default as M8 SP5
        
        protected Dictionary<string, object> ThePropertyObjects
        {
            get { return thePropertyObjects; }
            set { thePropertyObjects = value; }
        }        

        /// <summary>
        /// Clean objects of TypeLoadExceptionHolder type, which is incorrectly generated due to missing deprecated classes.
        /// </summary>
        /// <param name="c"></param>
        [OnSerializing]
        void OnSerializing(StreamingContext c)
        {
            if (this.thePropertyObjects != null && this.thePropertyObjects.Keys != null)
            {
                // Copy Keys to another array is necessary to avoid exception caused by directly iterating Keys to delete item.
                string[] keys = new string[this.thePropertyObjects.Keys.Count];
                this.thePropertyObjects.Keys.CopyTo(keys, 0);
                foreach (string key in keys)
                {
                    object objValue;
                    this.thePropertyObjects.TryGetValue(key, out objValue);
                    if (objValue != null && 0 == string.Compare("System.Runtime.Serialization.TypeLoadExceptionHolder", objValue.GetType().FullName, true))
                    {
                        this.ThePropertyObjects.Remove(key);
                    }
                }
            }
        }
        
        protected Dictionary<string, string> TheProperties
        {
            get { return theProperties; }
            set { theProperties = value; }
        }

        public Dictionary<string, string> TheSessionProperties
        {
            get { return theSessionProperties; }
            set { theSessionProperties = value; }
        }

        public string MetricQueryView
        {
            get
            {
                return (this.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150) ? METRIC_QUERY_3_VIEW : METRIC_QUERY_2_VIEW;
            }
        }

        public string MetricQueryViewFull
        {
            get
            {
                return (this.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150) ? METRIC_QUERY_3_VIEW_FULL : METRIC_QUERY_2_VIEW_FULL;
            }
        }

        public string SessionStatsView
        {
            get
            {
                return (this.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ141) ? SESSION_STATS_VIEW_2 : SESSION_STATS_VIEW_1;
            }
        }

        static public int TheMinPortNumber
        {
            get { return 1; }
        }

        static public int TheDefaultPortNumber
        {
            get { return 37800; }
        }

        static public int TheMaxPortNumber
        {
            get { return 65535; }
        }

        static public int theDefaultLiveFeedPortNumber
        {
            get { return -1; }
        }

        static public string TheDefaultHpOdbcDriverString
        {
            get
            {
                return Properties.Resources.DefaultTrafodionOdbcDriverString;
            }
        }

        public void SuppressEvents()
        {
            _suppressEvents++;
        }

        public void AllowEvents()
        {
            if (_suppressEvents > 0)
            {
                _suppressEvents--;
            }
        }

        static public List<ConnectionDefinition> ActiveConnectionDefinitions
        {
            get
            {
                return theConnectionDefinitions.FindAll(delegate(ConnectionDefinition aConnectionDefinition)
                {
                    return (aConnectionDefinition.TheState == State.TestSucceeded);
                });
            }
        }

        static public List<ConnectionDefinition> OtherConnectionDefinitions
        {
            get
            {
                return theConnectionDefinitions.FindAll(delegate(ConnectionDefinition aConnectionDefinition)
                {
                    return (aConnectionDefinition.TheState != State.TestSucceeded);
                });
            }
        }

        [NonSerialized]
        protected int _systemCatalogVersion = -1;
        [NonSerialized]
        protected string _serverTimeZoneName;
        [NonSerialized]
        protected int _serverTimeZoneOffsetSeconds = -1;
        [NonSerialized]
        private string _platformReleaseVersion = "";
        [NonSerialized]
        private string _platformReleaseVersionString = "";
       
        private string theConnectedSegmentName = null;
        /// <summary>
        /// The ODBC server version version.release
        /// </summary>
        protected string _odbcServerVersion = "";

        private const string TRACE_SUB_AREA_NAME = "Connection Defn";

        /// <summary>
        /// Property: PlatformReleaseVersion - the platform's release version
        /// </summary>
        public string PlatformReleaseVersion
        {
            set 
            {
                if (_platformReleaseVersion != value)
                {
                    _platformReleaseVersion = value;
                    FireChanged(Reason.PlatformReleaseVersion);
                }
            }
            get { return _platformReleaseVersion; }
        }

        /// <summary>
        /// The states of a connection definition
        /// </summary>
        public enum State
        {
            NotTested,      // The connection has not been tested since the definition was last changed
            TestFailed,     // The connection has been tested since the definition was last changed
                            // and the test failed
            TestSucceeded,  // The connection has been tested since the definition was last changed
                            // and the test succeeded
            PasswordExpired, 
            LiveFeedAuthFailed,   // The LiveFeed authentication failed
            LiveFeedTestSucceeded // The LiveFeed connection has been tested
        }

        /// <summary>
        /// The persistence key for connection definitions
        /// </summary>
        private static readonly string PersistenceKey = "ConnectionDefinitionsPersistence";

        /// <summary>
        /// The class supports persistence all of its members
        /// </summary>
        public static void Initialize()
        {
            Persistence.PersistenceHandlers += new Persistence.PersistenceHandler(ConnectionDefinitionPersistence);
        }

        /// <summary>
        /// Handles loading and saving our persisted state.
        /// </summary>
        /// <param name="aDictionary">The persistence dictionary</param>
        /// <param name="aPersistenceOperation">Whether to load or save our state</param>
        static void ConnectionDefinitionPersistence(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {

            // Load or Save?
            switch (aPersistenceOperation)
            {

                    // Load
                case Persistence.PersistenceOperation.PreLoad:
                    {
                        //A new persistence file is being loaded. Let's disconnect all the connections.
                        foreach (ConnectionDefinition theConnectionDefinition in ConnectionDefinitions)
                        {
                            theConnectionDefinition.ClearPassword();
                        }
                        break;
                    }
                case Persistence.PersistenceOperation.Load:
                    {
                        if (aDictionary.ContainsKey(PersistenceKey))
                        {

                            // Get the persisted connections
                            List<ConnectionDefinition> theScratchConnectionDefinitions = aDictionary[PersistenceKey] as List<ConnectionDefinition>;

                            // Remove all of the existing connections
                            while (ConnectionDefinitions.Count > 0)
                            {

                                // Also fires an event for each removal
                                Remove(null, ConnectionDefinitions[0]);

                            }

                            if (theScratchConnectionDefinitions != null)
                            {
                                // Add all of the persisted connections
                                foreach (ScratchConnectionDefinition theScratchConnectionDefinition in theScratchConnectionDefinitions)
                                {

                                    // Also fires an event for each addition
                                    Add(null, new ConnectionDefinition(theScratchConnectionDefinition));

                                }
                            }

                        }

                        break;
                    }

                    // Save
                case Persistence.PersistenceOperation.Save:
                    {
                        List<ConnectionDefinition> theScratchConnectionDefinitions = new List<ConnectionDefinition>();
                        foreach (ConnectionDefinition theConnectionDefinition in ConnectionDefinitions)
                        {
                            ScratchConnectionDefinition theScratchConnectionDefinition = new ScratchConnectionDefinition(theConnectionDefinition);
                            theScratchConnectionDefinition.ClearPassword();
                            theScratchConnectionDefinitions.Add(theScratchConnectionDefinition);
                        }
                        aDictionary.Add(PersistenceKey, theScratchConnectionDefinitions);
                        break;
                    }
                default:
                    {
                        throw new ApplicationException("Unknown persistence operation");
                    }
            }
        }

        /// <summary>
        /// Creates a new uninitialized connection definition
        /// </summary>
        public ConnectionDefinition()
        {

            // Always start with the default port number
            thePort = TheDefaultPortNumber.ToString();
            SetProperty(LiveFeedPortProperty, theDefaultLiveFeedPortNumber.ToString());

            // And the default driver
            theDriverString = TheDefaultHpOdbcDriverString;

        }

        /// <summary>
        /// Creates a new instance connection definition that is a copy of an existing one
        /// </summary>
        public ConnectionDefinition(ConnectionDefinition aConnectionDefinition)
        {
            Set(aConnectionDefinition);
        }

        /// <summary>
        /// Causes this connection definition to become a copy of another
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public void Set(ConnectionDefinition aConnectionDefinition)
        {
            Name = aConnectionDefinition.Name;
            Host = aConnectionDefinition.Host;
            Port = aConnectionDefinition.Port;
            ConnectedDataSource = aConnectionDefinition.ConnectedDataSource;
            ClientDataSource = aConnectionDefinition.ClientDataSource;
            UserName = aConnectionDefinition.UserName;
            thePassword = aConnectionDefinition.Password;
            theDefaultCatalog = aConnectionDefinition.DefaultCatalog;
            theDefaultSchema = aConnectionDefinition.DefaultSchema;
            DriverString = aConnectionDefinition.DriverString;
            _odbcServerVersion = aConnectionDefinition._odbcServerVersion;
            _systemCatalogVersion = aConnectionDefinition._systemCatalogVersion;
            _theODBCServerName = aConnectionDefinition._theODBCServerName;
            _platformReleaseVersion = aConnectionDefinition._platformReleaseVersion;
            _platformReleaseVersionString = aConnectionDefinition._platformReleaseVersionString;
            _serverTimeZoneName = aConnectionDefinition._serverTimeZoneName;
            _serverTimeZoneOffsetSeconds = aConnectionDefinition._serverTimeZoneOffsetSeconds;

            theState = aConnectionDefinition.theState;
            thePasswordIsSet = aConnectionDefinition.PasswordIsSet;
            if (aConnectionDefinition.TheProperties != null)
            {
                TheProperties = new Dictionary<string,string>(aConnectionDefinition.TheProperties);
            }

            if (aConnectionDefinition.thePropertyObjects != null)
            {
                thePropertyObjects = new Dictionary<string,object>(aConnectionDefinition.thePropertyObjects);
            }

            if (aConnectionDefinition.TheSessionProperties != null)
            {
                theSessionProperties = new Dictionary<string, string>(aConnectionDefinition.TheSessionProperties);
            }
            if (aConnectionDefinition.ComponentPrivileges != null)
            {
                theComponentPrivileges = new Dictionary<string, List<string>>(aConnectionDefinition.ComponentPrivileges);
            }
        }

        /// <summary>
        /// Causes this connection definition to become a copy of another
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public void SetHard(ConnectionDefinition aConnectionDefinition)
        {
            //this.remo
            theConnectionDefinitions.Remove(this);
            theConnectionDefinitions.Add(aConnectionDefinition);
            aConnectionDefinition.FireChanged(Reason.Added);
        }

        /// <summary>
        /// Convert this connection definition to a string
        /// </summary>
        /// <returns>The user-chosen name of this connection definition</returns>
        override public string ToString()
        {
            return Name;
        }

        //User quota in MB
        public long MaxUserTableSize
        {
            get { return _maxUserTableSize; }
        }

        public Dictionary<string, List<string>> ComponentPrivileges
        {
            get { return theComponentPrivileges; }
        }

        /// <summary>
        /// The connection definition's fully qualifed schema name.  Assembled from or decomposed
        /// to separate catalog and schema names.  Trafodion users do not know about catalog names.
        /// </summary>
        public string FullyQualifiedDefaultSchema
        {
            get
            {

                // Prepend the catalog name if any
                return ((DefaultCatalog.Length > 0) ? (DefaultCatalog + ".") : "") + DefaultSchema;

            }
            set
            {

                if ((value == null) || (value.Length == 0))
                {
                    //DefaultCatalog = "TRAFODION";
                    DefaultSchema = "";
                }
                else
                {
                    string[] theParts = value.Split('.'); // TODO - This is not correct for quoted names
                    if (theParts.Length == 1)
                    {

                        // If the name has no dot in it we assume that it is the TRAFODION catalog
                        //DefaultCatalog = "TRAFODION";
                        DefaultSchema = theParts[0];

                    }
                    else
                    {
                        //DefaultCatalog = theParts[0];
                        DefaultSchema = theParts[1];
                    }
                }
            }
        }

        /// <summary>
        /// Read only property that gives a standard user-friendly descriptive string for this connection definition.
        /// </summary>
        public string Description
        {
            get
            {
                return Name
                + Properties.Resources.HostLabel + Host
                + Properties.Resources.PortLabel + Port
                + ((ConnectedDataSource.Length > 0) ? (Properties.Resources.DatasourceLabel + ConnectedDataSource) : "")
                + Properties.Resources.UserLabel + UserName
                + ((RoleName == null) ? (String.IsNullOrEmpty(UserSpecifiedRole) ? "" : (Properties.Resources.RoleLabel + UserSpecifiedRole))
                                        :
                                        ((RoleName.Length > 0) ? (Properties.Resources.RoleLabel + RoleName) : ""))
                + ((FullyQualifiedDefaultSchema.Length > 0) ? (Properties.Resources.DefaultSchemaLabel + FullyQualifiedDefaultSchema) : "")
                + (string.IsNullOrEmpty(ConnectedServiceName) ? "" : (Properties.Resources.ConnectedServiceLabel + ConnectedServiceName))
                + " ]";
            }
        }

        public string FormattedDescription
        {
            get
            {
                StringBuilder descBuilder = new StringBuilder();
                descBuilder.AppendFormat("System : {0}, Host: {1} , Port: {2}", Name, Host, Port);
                //descBuilder.AppendFormat(", Platform Version : {0}", (string.IsNullOrEmpty(PlatformReleaseVersion) ? DefaultPlatformVersion : _platformReleaseVersion));
                descBuilder.AppendFormat(string.IsNullOrEmpty(PlatformReleaseVersion) ? "" : ", Platform Version : " + PlatformReleaseVersion);
                descBuilder.AppendLine();
                descBuilder.Append("Directory Service User : " + UserName);
                descBuilder.Append(string.IsNullOrEmpty(DatabaseUserName) ? "" : ", Database User : " + DatabaseUserName);
                descBuilder.Append(string.IsNullOrEmpty(RoleName) ? "" : ", Role : " + RoleName);
                descBuilder.AppendLine();
                descBuilder.Append(ConnectedDataSource.Length > 0 ? "Data Source : " + ConnectedDataSource : "");
                descBuilder.Append(string.IsNullOrEmpty(ConnectedDataSource) ? "" : ", ");
                descBuilder.Append(ConnectedServiceName.Length > 0 ? "WMS Service : " + ConnectedServiceName : "");
                descBuilder.Append(string.IsNullOrEmpty(ConnectedServiceName) ? "" : ", ");
                descBuilder.Append((FullyQualifiedDefaultSchema.Length > 0) ? ("Default Schema: " + FullyQualifiedDefaultSchema) : "");
                return descBuilder.ToString();
            }
        }
        /// <summary>
        /// Read only property that returns what the user entered for DNS name or IP address of the host/server as a string
        /// </summary>
        public string Server
        {
            get
            {
                return Host;
            }
        }

        public static string DefaultCertificateDirectory
        {
            get
            {
                // We don't know it yet.  Try the environment variables.
                string theHomeDirectory = Environment.GetEnvironmentVariable("HOME");

                if (theHomeDirectory == null || theHomeDirectory.Trim().Equals(""))
                {
                    // We don't know it yet.  Try the environment object.
                    string homedrive = Environment.GetEnvironmentVariable("HOMEDRIVE");

                    if (!string.IsNullOrEmpty(homedrive))
                    {
                        theHomeDirectory = homedrive;
                        string homepath = Environment.GetEnvironmentVariable("HOMEPATH");
                        if (!string.IsNullOrEmpty(homepath))
                        {
                            try
                            {
                                theHomeDirectory = System.IO.Path.Combine(homepath, homepath);
                            }
                            catch (Exception ex)
                            {
                                return null;
                            }

                        }
                    }
                }
                return theHomeDirectory;
            }
        }

        public string DefaultCertificateFileName
        {
            get
            {
                String defaultCertName = MasterSegmentName.Trim();
                if (defaultCertName.Length > 5)
                    return defaultCertName.Substring(0, 5);
                return defaultCertName;
            }
        }        
        /// <summary>
        /// Cause the connection definition to need to have a password defined
        /// </summary>
        public void ClearPassword()
        {
            thePasswordIsSet = false; // This flag forces the user to explicitly set an empty password
            theState = State.NotTested;
            thePassword = "";  // Cannot use property becasue is SETs thePasswordIsSet
            _systemCatalogVersion = -1;
            _roleName = null; //Force the system to recompute the role name everytime a new connection is done.
            _theConnectedDataSource = "";
            PlatformReleaseVersion = "";
            PlatformReleaseVersionString = "";
	        _theODBCServerName = null;
            theSessionProperties.Clear();
            theComponentPrivileges.Clear();
            FireChanged(Reason.Disconnected);
        }
        
        /// <summary>
        /// Cause the connection definiton to clear its password.
        /// </summary>
        public void PasswordExpired()
        {
            ClearPassword();
            theState = State.PasswordExpired;
        }

        public void ResetState()
        {
            theState = State.NotTested;
        }

        public string ClientDSNConnectionString
        {
            get
            {
                string userRoleName = UserSpecifiedRole;
                OdbcConnectionStringBuilder builder = new OdbcConnectionStringBuilder();
                builder.Add("Driver", @DriverString);
                builder.Add("DSN", @ClientDataSource);
                builder.Add("UID", @EncloseInBraces(UserName));
                builder.Add("PWD", @EncloseInBraces(Password));
                if (!string.IsNullOrEmpty(userRoleName))
                    builder.Add("Rolename", @userRoleName);
                builder.Add("CATALOG", @DefaultCatalog);
                builder.Add("SCHEMA", @EncloseInBraces(SchemaForConnectionString));
                builder.Add("Application", "TrafodionManager");
                if (!string.IsNullOrEmpty(CertificateDir))
                    builder.Add("CERTIFICATEDIR", CertificateDir);
                if (!string.IsNullOrEmpty(CertificateFileName))
                    builder.Add("CERTIFICATEFILE", CertificateFileName);
                return builder.ConnectionString;
            }
        }

        /// <summary>
        /// The server timezone
        /// </summary>
        public string ServerTimeZoneName
        {
            get { return _serverTimeZoneName != null ? _serverTimeZoneName : ""; }
        }

        public TimeSpan ServerGMTOffset
        {
            get { return TimeSpan.FromSeconds(_serverTimeZoneOffsetSeconds); }
        }

        string EncloseInBraces(string value)
        {
            if(!string.IsNullOrEmpty(value))
            {
                if (value.StartsWith("{"))
                {
                    value = '{' + value;
                }
                bool doesEndWithRightBrace = value.EndsWith("}");
                value = value.Replace("}", "}}");
                if (doesEndWithRightBrace)
                {
                    value = value + "}";
                }

                if(value.IndexOf('}') >= 0)
                {
                    if (!value.StartsWith("{"))
                    {
                        value = '{' + value;
                    }
                    if (!value.EndsWith("}"))
                    {
                        value = value + "}";
                    }
                }
            }

            return value;
        }

        /// <summary>
        /// Read only property that returns the appropriate ODBC connection string for this connection definition
        /// </summary>
        public string ConnectionString
        {
            get
            {
                string userRoleName = UserSpecifiedRole;
                OdbcConnectionStringBuilder builder = new OdbcConnectionStringBuilder();
                builder.Add("Driver", @DriverString);
                //builder.Add("ServerDSN", @ConnectedDataSource);
                builder.Add("SERVER", "TCP:" + Host + "/" + Port);
                builder.Add("UID", @EncloseInBraces(UserName));
                builder.Add("PWD", @EncloseInBraces(Password));
                if (!string.IsNullOrEmpty(userRoleName))
                    builder.Add("Rolename", @userRoleName);
                builder.Add("CATALOG", @DefaultCatalog);
                builder.Add("SCHEMA", @EncloseInBraces(SchemaForConnectionString));
                builder.Add("Application", "TrafodionManager");
                if (!string.IsNullOrEmpty(CertificateDir))
                    builder.Add("CERTIFICATEDIR", CertificateDir);
                if (!string.IsNullOrEmpty(CertificateFileName))
                    builder.Add("CERTIFICATEFILE", CertificateFileName);

                return builder.ConnectionString;
            }
        }
        /// <summary>
        /// Read only property that returns the appropriate ODBC connection string for this connection definition using Admin_Load_DataSource as the ServerDS
        /// </summary>
        public string AdminConnectionString
        {
            get
            {
                string userRoleName = UserSpecifiedRole;
                OdbcConnectionStringBuilder builder = new OdbcConnectionStringBuilder();
                builder.Add("Driver", @DriverString);
                builder.Add("ServerDSN", AdminLoadDataSource);
                builder.Add("SERVER", "TCP:" + Host + "/" + Port);
                builder.Add("UID", @EncloseInBraces(UserName));
                builder.Add("PWD", @EncloseInBraces(Password));
                if (!string.IsNullOrEmpty(userRoleName))
                    builder.Add("Rolename", @userRoleName);
                builder.Add("CATALOG", @DefaultCatalog);
                builder.Add("SCHEMA", @EncloseInBraces(SchemaForConnectionString));
                builder.Add("Application", "TrafodionManager");
                if (!string.IsNullOrEmpty(CertificateDir))
                    builder.Add("CERTIFICATEDIR", CertificateDir);
                if (!string.IsNullOrEmpty(CertificateFileName))
                    builder.Add("CERTIFICATEFILE", CertificateFileName);

                return builder.ConnectionString;
            }
        }

        public string SQLWBConnectionString
        {
            get
            {
                string userRoleName = UserSpecifiedRole;
                OdbcConnectionStringBuilder builder = new OdbcConnectionStringBuilder();
                builder.Add("Driver", @DriverString);
                builder.Add("ServerDSN", SQLWB_DataSource);
                builder.Add("SERVER", "TCP:" + Host + "/" + Port);
                builder.Add("UID", @EncloseInBraces(UserName));
                builder.Add("PWD", @EncloseInBraces(Password));
                if (!string.IsNullOrEmpty(userRoleName))
                    builder.Add("Rolename", @userRoleName);
                builder.Add("CATALOG", @DefaultCatalog);
                builder.Add("SCHEMA", @EncloseInBraces(SchemaForConnectionString));
                builder.Add("Application", "TrafodionManager");
                if (!string.IsNullOrEmpty(CertificateDir))
                    builder.Add("CERTIFICATEDIR", CertificateDir);
                if (!string.IsNullOrEmpty(CertificateFileName))
                    builder.Add("CERTIFICATEFILE", CertificateFileName);
                return builder.ConnectionString;
            }
        }
        /// <summary>
        /// Memberwise comparison of this connection definition with another
        /// </summary>
        /// <param name="aConnectionDefinition">Another connection definition</param>
        /// <returns></returns>
        public bool Equals(ConnectionDefinition aConnectionDefinition)
        {
            bool ret = (
            (aConnectionDefinition.Name == Name) &&
            (aConnectionDefinition.Host == Host) &&
            (aConnectionDefinition.Port == Port) &&
            (aConnectionDefinition.ClientDataSource == ClientDataSource) &&
            (aConnectionDefinition.UserName == UserName) &&
            (aConnectionDefinition.UserSpecifiedRole == UserSpecifiedRole) &&
            (aConnectionDefinition.Password == Password) &&
            (aConnectionDefinition.DefaultCatalog == DefaultCatalog) &&
            (aConnectionDefinition.DefaultSchema == DefaultSchema) &&
            (aConnectionDefinition.DriverString == DriverString) &&
            (aConnectionDefinition.LiveFeedPort == LiveFeedPort)
            );
            return ret;
        }

        public override bool Equals(object obj)
        {
            return Equals(obj as ConnectionDefinition);
        }

        /// <summary>
        /// Get a hash code.   Need not be unique.
        /// </summary>
        /// <returns>A hash code</returns>
        override public int GetHashCode()
        {
            return (int) TheState;
        }

        /// <summary>
        /// Test this connection definition.  A modal dialog shows the result of the test.
        /// </summary>
        //public void DoTest()
        //{
        //    DoTest(this);
        //}

        /// <summary>
        /// Test this connection definition.  A modal dialog shows the result of the test.
        /// The modal dialog can be optionally suppressed upon success.
        /// </summary>
        /// <param name="suppressSuccessMessage">True to suppress the modal dialog if test succeeds</param>
        public void DoTest(bool suppressSuccessMessage)
        {
            DoTest(this, suppressSuccessMessage);
        }

        /// <summary>
        /// Test a connection definition.  A momdal dialog shows the result of the test.
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition to test</param>
        //public static void DoTest(ConnectionDefinition aConnectionDefinition)
        //{
        //    DoTest(aConnectionDefinition, false);
        //}

        /// <summary>
        /// ODBC error messages sometimes consist of two identical lines of text.  If so, eliminate one.
        /// </summary>
        /// <param name="oe">An ODBC exception</param>
        /// <returns>The text</returns>
        public static string FixOdbcExceptionMessage(System.Data.Odbc.OdbcException oe)
        {
            string[] theSplits = oe.Message.Split(new char[] { '\r', '\n' });
            int theCount = theSplits.Length;
            if ((theCount > 1) && (theSplits[0] == theSplits[theCount - 1]))
            {
                return theSplits[0];
            }
            return oe.Message;
        }
        
		
		public void DoTestOnly()
        { 
            DoConnect(this, false, true);
        }

        /// <summary>
        /// Test a connection definition.  A modal dialog shows the result of the test.
        /// The modal dialog can be optionally suppressed upon success.
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition to test</param>
        /// <param name="suppressSuccessMessage">True to suppress the modal dialog if test succeeds</param>
		public static void DoTest(ConnectionDefinition aConnectionDefinition, bool suppressSuccessMessage)
        {
            DoConnect(aConnectionDefinition, suppressSuccessMessage, false);
        }
		
        public static void DoConnect(ConnectionDefinition aConnectionDefinition, bool suppressSuccessMessage, bool isTestOnly)
        {
            Connection aConnection = null;
            //OdbcConnection theOdbcConnection = null;

            string theProblemString = null;

            // Check to see if the connection definition is worthy of actual testing
            if (aConnectionDefinition.Name.Length == 0)
            {
                theProblemString = "You must specify a connection name.";
            }
            else if (!aConnectionDefinition.PasswordIsSet)
            {
                theProblemString = "You must specify the password (even if it is empty).";
            }
            else if (aConnectionDefinition.UserName.Length == 0)
            {
                theProblemString = "You must specify the username.";
            }
            else if (aConnectionDefinition.Host.Length == 0)
            {
                theProblemString = "You must specify the host.";
            }
            else if (aConnectionDefinition.Port.Length == 0)
            {
                theProblemString = "You must specify the port.";
            }

            // Did we find any show stoppers?
            if (theProblemString != null)
            {
                aConnectionDefinition.ShowMessageBox(theProblemString);
                return;
            }

            State tempState = aConnectionDefinition.theState;

			ConnectionArgs connectionArgs = new ConnectionArgs(aConnectionDefinition, suppressSuccessMessage, isTestOnly);
            BackgroundConnectDialog bcd = new BackgroundConnectDialog(connectionArgs);
            DialogResult result = bcd.ShowDialog();
            if (result == DialogResult.OK)
            {
                //if (isTestOnly)
                //{
                //    // SHow success message if not suppressed
                //    //aConnectionDefinition.ShowMessageBox("ODBC Connection test succeeded.");
                //}
                //else
                //{
                //    FireChanged(aConnectionDefinition, aConnectionDefinition, Reason.Tested);
                //}
                if (!isTestOnly)
                    FireChanged(aConnectionDefinition, aConnectionDefinition, Reason.Tested);
            }
            else
            {

                if (!isTestOnly)
                {
                    aConnectionDefinition.theState = tempState;
                    //if (aConnectionDefinition.theState != State.LiveFeedTestSucceeded)
                    //{
                    //   // aConnectionDefinition.theState = tempState;
                    //}
                    //else
                    //{
                    //    FireChanged(aConnectionDefinition, aConnectionDefinition, Reason.Tested);
                    //}

                }
                else
                {
                    aConnectionDefinition.theState = State.TestFailed;
                }
            }
        }


        public void DoBackgroundConnects(bool isTestOnly, BackgroundWorker worker, DoWorkEventArgs e)
        {
            lock (_syncRootDoConnection)
            {
                if (this.TheProperties.ContainsKey(LiveFeedOnlyConnectProperty)
                    && this.TheProperties[LiveFeedOnlyConnectProperty] == true.ToString())
                {
                    //Step 1: Live Feed
                    DoLiveFeedBackgroundConnect(isTestOnly, worker, e);
                    if (worker.CancellationPending)
                        return;

                    if (theState != State.LiveFeedTestSucceeded)
                    {
                        //not connected
                        //Here we handle a special state: LiveFeedAuthFailed state, because when authentication failure which means name/password is not correct so as no need to do a ODBC connection later, 
                        //so we immediately tell user to input the right user name and password and try again.
                        if (theState == State.LiveFeedAuthFailed)
                        {
                            MessageBox.Show(Properties.Resources.AuthenticationErrorMessage, Properties.Resources.AuthenticationError, MessageBoxButtons.OK);

                            e.Cancel = true;
                            return;
                        }
                        else
                        {
                            MessageBox.Show(Properties.Resources.TestLiveFeedFailMessage, Properties.Resources.LiveFeedAuthenticationError, MessageBoxButtons.OK);
                        }
                    }
                    else
                    {
                        if (isTestOnly)
                        {
                            MessageBox.Show(Description + "\n\n" + "Live Feed Connection test succeeded.", "Connection Test Result", MessageBoxButtons.OK);
                        }
                    }
                }
                else
                {
                    bool blnLiveFeedTest = false;

                    //Step 1: Live Feed
                    if (isTestOnly)
                    {
                        DoLiveFeedBackgroundConnect(isTestOnly, worker, e);
                        if (worker.CancellationPending)
                            return;
                        if (theState == State.LiveFeedTestSucceeded) blnLiveFeedTest = true;
                    }

                    //Step 1: ODBC
                    DoODBCBackgroundConnect(isTestOnly, worker, e);


                    if (isTestOnly)
                    {
                        if (theState == State.TestSucceeded && blnLiveFeedTest)
                        {
                            MessageBox.Show(Description + "\n\n" + "Both ODBC and Live Feed Connection test succeeded.", "Connection Test Result", MessageBoxButtons.OK);
                        }
                        else if (blnLiveFeedTest)
                        {
                            MessageBox.Show(Description + "\n\n" + "ODBC Connection test failed.", "Connection Test Result", MessageBoxButtons.OK);
                        }
                        else
                        {
                            MessageBox.Show(Description + "\n\n" + "Both ODBC and Live feed Connection test failed.", "Connection Test Result", MessageBoxButtons.OK);
                        }
                    }

                }
            }
        }


        public void DoLiveFeedBackgroundConnect(bool isTestOnly, BackgroundWorker worker, DoWorkEventArgs e)
        {
            if (theState != State.LiveFeedTestSucceeded)
            {
                worker.ReportProgress(0, "Opening Live Feed connection...");

                try
                {
                    FireOnLiveFeedTest();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error opening the Live Feed connection: " + ex.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                    return;
                }
            }
        }

        public void DoODBCBackgroundConnect(bool isTestOnly, BackgroundWorker worker, DoWorkEventArgs e)
        {
            Connection aConnection = null;            
            // No show stoppers, get ODBC to try it out
            
            try
            {
                if (!isTestOnly)
                {
                    if (theState != State.LiveFeedTestSucceeded)
                    {
                        theState = State.NotTested;
                    }
                }
				ConnectedDataSource = "";

                worker.ReportProgress(0, "Opening ODBC connection...");
                aConnection = new Connection(this, true);
                aConnection.DisplayWarnings = true;
                OdbcConnection odbcConnection = isTestOnly ? aConnection.OpenNonCachedOdbcConnection : aConnection.OpenOdbcConnection;
                aConnection.DisplayWarnings = false;

                if (odbcConnection != null && odbcConnection.State != System.Data.ConnectionState.Closed)
                {   
                    //if (!isTestOnly)
                    //{
                        theState = State.TestSucceeded;
                        OdbcServerVersion = odbcConnection.ServerVersion;

                        try
                        {
                            //Load the session attributes for the connection using the same odbconnection
                            //Load this only once, i.e. if there are no existing connections in the registry
                            if (!ConnectionRegistry.Instance.ConnectionExists(this))
                            {
                                if (worker.CancellationPending)
                                {
                                    if (!isTestOnly)
                                    {
                                        //theState = State.TestFailed;
                                        theState = State.TestFailed;
                                    }
                                    e.Cancel = true;
                                    return;
                                }

                                worker.ReportProgress(0, "Fetching session attributes...");
                                if (!TrafodionContext.Instance.isCommunityEdition)
                                {
                                    GetSessionAttributes(odbcConnection);
                                }
                                if (worker.CancellationPending)
                                {
                                    e.Cancel = true;
                                    if (!isTestOnly)
                                    {
                                        theState = State.TestFailed;
                                    }
                                    return;
                                }
                                worker.ReportProgress(0, "Fetching system attributes...");
                                GetSystemAttributes(odbcConnection);
                                if (worker.CancellationPending)
                                {
                                    e.Cancel = true;
                                    if (!isTestOnly)
                                    {
                                        theState = State.TestFailed;
                                    }
                                    return;
                                }

                                _theODBCServerName = Host;
                                
                                if (!TrafodionContext.Instance.isCommunityEdition)
                                {
                                worker.ReportProgress(0, "Fetching Component privileges for user...");
                                GetComponentPrivileges(odbcConnection);
                                if (worker.CancellationPending)
                                {
                                    e.Cancel = true;
                                    if (!isTestOnly)
                                    {
                                        theState = State.TestFailed;
                                    }
                                    return;
                                }
                                GetUserQuota(odbcConnection);
                            }
                            }
                            worker.ReportProgress(100, "Notifying connected state to listeners");
                        }
                        catch (Exception ee)
                        {
                            if (!isTestOnly)
                            {
                                theState = State.TestFailed;
                            }
                            e.Cancel = true;
                            throw new Exception("Error loading session attributes : " + ee.Message);
                        }
                    //}
                    //else 
                    //{
                    //    MessageBox.Show(Description + "\n\n" + "ODBC Connection test succeeded.", "Connection Test Result", MessageBoxButtons.OK);
                    //}
                }
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
				e.Cancel = true;
                if (!isTestOnly)
                {
                    if (theState != State.LiveFeedTestSucceeded)
                    {
                        theState = State.TestFailed;
                    }
                }
                // Got an ODBC connection.  Fix it and show it.
                throw new Exception(FixOdbcExceptionMessage(oe));
            }
            finally
            {
                if (aConnection != null)
                {
                    aConnection.Close();
                }
            }
        }

        /// <summary>
        /// Gets the current connections session attributes
        /// </summary>
        private void GetSessionAttributes(OdbcConnection odbcConnection)
        {
            if (TheState != State.TestSucceeded)
                return;

            OdbcDataReader odbcReader = null;
            try
            {
                theSessionProperties.Clear();
                OdbcCommand odbcCommand = new OdbcCommand("GET SERVICE", odbcConnection);
                odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                if (odbcReader.Read())
                {
                    for (int colNum = 0; colNum < odbcReader.FieldCount; colNum++)
                    {
                        theSessionProperties.Add(odbcReader.GetName(colNum), odbcReader.GetString(colNum));
                    }
                }
            }
            //catch (Exception ex)
            //{
            //}
            finally
            {
                if (odbcReader != null)
                {
                    odbcReader.Close();
                }
            }
        }

        /// <summary>
        /// Gets the component privileges for the logged in user
        /// </summary>
/*        private void GetComponentPrivileges(OdbcConnection odbcConnection)
        {
            if (TheState != State.TestSucceeded)
                return;

            theComponentPrivileges.Clear(); 


            string queryString = "";
            List<string> components = new List<string>();
            queryString = "GET COMPONENTS, NO HEADER";
            OdbcDataReader odbcReader = null;
            try
            {
                OdbcCommand odbcCommand = new OdbcCommand(queryString, odbcConnection);
                odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                while (odbcReader.Read())
                {
                    components.Add(odbcReader.GetString(0).Trim());
                }
            }
            //catch (Exception ex)
            //{

            //}
            finally
            {
                if (odbcReader != null)
                {
                    odbcReader.Close();
                }
            }

            foreach (string component in components)
            {
                List<string> privileges = new List<string>();

                queryString = string.Format("GET CURRENT_USER PRIVILEGES ON COMPONENT {0} cascade, NO HEADER", component);

                try
                {
                    OdbcCommand odbcCommand = new OdbcCommand(queryString, odbcConnection);
                    odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                    while (odbcReader.Read())
                    {
                        string priv = odbcReader.GetString(0).Trim();
                        string[] privData = priv.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                        if (privData.Length > 1)
                        {
                            privileges.Add(privData[1].Trim());
                        }
                    }
                }
                //catch (Exception ex)
                //{

                //}
                finally
                {
                    if (odbcReader != null)
                    {
                        odbcReader.Close();
                    }
                    theComponentPrivileges.Add(component, privileges);
                }
            }
        }
        */

        private void GetComponentPrivileges(OdbcConnection odbcConnection)
        {
            if (TheState != State.TestSucceeded)
                return;

            theComponentPrivileges.Clear();

            string queryString = "";
            OdbcDataReader odbcReader = null;
            int userID = DatabaseUserID;

            //If user id is not looked up yet, get it from the database
            if (userID == -1)
            {

                queryString = string.Format("select cast(user_id as char(10)) from TRAFODION.trafodion_security_schema.users " +
                                             "where user_name = '{0}' for read uncommitted access", this.DatabaseUserName);
                List<string> components = new List<string>();
                try
                {
                    OdbcCommand odbcCommand = new OdbcCommand(queryString, odbcConnection);
                    odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                    while (odbcReader.Read())
                    {
                        userID = odbcReader.GetInt32(0);
                    }
                }
                finally
                {
                    if (odbcReader != null)
                    {
                        odbcReader.Close();
                    }
                }
            }

            if(userID > -1)
            {
                queryString = string.Format("select distinct c.component_name, priv_name from " +
                                            "TRAFODION.trafodion_security_schema.components c, " +
                                            "TRAFODION.trafodion_security_schema.component_privileges p, " +
                                            "TRAFODION.trafodion_security_schema.component_privilege_usage u " +
                                            "where (c.component_uid=p.component_uid) and " +
                                            "(c.component_uid = u.component_uid) and " +
                                            "(p.privilege_type = u.priv_type) and  " +
                                            "(u.component_priv_class != 'IN') and  " +
                                            "((p.grantee = {0}) " +
                                            "or (p.grantee in (select role_id from  " +
                                            "TRAFODION.trafodion_security_schema.role_usage " +
                                            "where grantee = {0}))) order by 1  " +
                                            "for read uncommitted access ", userID);
   
                try
                {
                    OdbcCommand odbcCommand = new OdbcCommand(queryString, odbcConnection);
                    odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                    String componentName = "";
                    List<string> privileges = new List<string>();
                    while (odbcReader.Read())
                    {
                        String currentComponentName = odbcReader.GetString(0).Trim();
                        if (!currentComponentName.Equals(componentName, StringComparison.InvariantCultureIgnoreCase))
                        {
                            if (!string.IsNullOrEmpty(componentName))
                            {
                                theComponentPrivileges.Add(componentName, privileges);
                            }
                            componentName = currentComponentName;
                            privileges = new List<string>();
                        }
                        string priv = odbcReader.GetString(1).Trim();
                        privileges.Add(priv);
                    }
                    if (!string.IsNullOrEmpty(componentName) && !theComponentPrivileges.ContainsKey(componentName))
                    {
                        theComponentPrivileges.Add(componentName, privileges);
                    }
                }
                finally
                {
                    if (odbcReader != null)
                    {
                        odbcReader.Close();
                    }
                }
            }
        }

        public bool ComponentPrivilegeExists(string componentName, string privilegeName)
        {
            if (theComponentPrivileges.ContainsKey(componentName))
            {
                List<string> privileges = theComponentPrivileges[componentName];
                return privileges.Contains(privilegeName);
            }
            else
            {
                return false;
            }
        }
        
        /// <summary>
        /// Gets the current system attributes
        /// </summary>
        private void GetSystemAttributes(OdbcConnection odbcConnection)
        {
            if (TheState != State.TestSucceeded)
                return;

            OdbcDataReader odbcReader = null;
            try
            {
                OdbcCommand odbcCommand = new OdbcCommand("INFO SYSTEM", odbcConnection);
                odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                if (odbcReader.Read())
                {
                    for (int colNum = 0; colNum < odbcReader.FieldCount; colNum++)
                    {
                        string columnName = odbcReader.GetName(colNum);
                        if (columnName.Equals("TM_ZONE"))
                        {
                            _serverTimeZoneName = odbcReader.GetString(colNum);
                        }
                        else if (columnName.Equals("TM_GMTOFF_SEC"))
                        {
                            string value = odbcReader.GetString(colNum);
                            Int32.TryParse(value, out _serverTimeZoneOffsetSeconds);
                        }
                        else if (columnName.Equals("NDCS_VERSION"))
                        {
                            string value = odbcReader.GetString(colNum);
                            int id = value.IndexOf("Release");
                            PlatformReleaseVersion = value.Substring(id + 8, 5);
                            PlatformReleaseVersionString = value.Substring(id);
                        }
                    }
                }

                // Once the odbc session is established and the server (m6) does not tell which version it is
                // we'll just use the default version.
                if (string.IsNullOrEmpty(PlatformReleaseVersion))
                {
                    PlatformReleaseVersion = DefaultPlatformVersion;
                }
            }
            //catch (Exception ex)
            //{
            //    PlatformReleaseVersion = DefaultPlatformVersion;
            //}
            finally
            {
                if (odbcReader != null)
                {
                    odbcReader.Close();
                }
            }
        }

        /// <summary>
        /// Get the current CQDs
        /// </summary>
        private void GetUserQuota(OdbcConnection odbcConnection)
        {
            if (TheState != State.TestSucceeded)
                return;

            OdbcDataReader odbcReader = null;
            try
            {
                OdbcCommand odbcCommand = new OdbcCommand("SHOWCONTROL QUERY DEFAULT POS_ABSOLUTE_MAX_TABLE_SIZE , MATCH FULL, NO HEADER", odbcConnection);
                odbcReader = Utilities.ExecuteReader(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, TRACE_SUB_AREA_NAME, true);
                if(odbcReader.Read())
                {
                    string spaceQuota = odbcReader.GetString(0).Trim();

                    //If return value is not long, set to -1
                    if (!long.TryParse(spaceQuota, out _maxUserTableSize))
                    {
                        _maxUserTableSize = -1;
                    };
                }
            }
            catch (Exception ex)
            {
                _maxUserTableSize = -1;
                //If operation fails, the role name continues to be null.
            }
            finally
            {
                if (odbcReader != null)
                {
                    odbcReader.Close();
                }
            }
        }

        //private void GetODBCServerName(OdbcConnection odbcConnection)
        //{
        //    try
        //    {
        //        _theODBCServerName = OdbcAccess.GetODBCServerName(odbcConnection);
        //    }
        //    catch (Exception ex)
        //    {
        //        // That's all leave it to blank.
        //        if (Logger.IsTracingEnabled)
        //        {
        //            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
        //                               TraceOptions.TraceArea.Framework,
        //                               TRACE_SUB_AREA_NAME,
        //                               "GetODBCServerName: ODBC Exception e : " + ex.Message);
        //        }
        //    }
        //}

        // Show the result of a connection test
        private void ShowMessageBox(string aMessage)
        {
            // By default, the message box displays an OK button.
            MessageBox.Show(Utilities.GetForegroundControl(), Description + "\n\n" + aMessage, "Connection Test Result", MessageBoxButtons.OK);
        }


        public object GetPropertyObject(String aPropertyName)
        {
            object property = null;
            if (thePropertyObjects.ContainsKey(aPropertyName))
            {
                property = this.thePropertyObjects[aPropertyName];
            }
            return property;
        }

        public void SetPropertyObject(String aPropertyName, object aValue)
        {
            if (this.thePropertyObjects.ContainsKey(aPropertyName))
            {
                this.thePropertyObjects.Remove(aPropertyName);
            }
            thePropertyObjects.Add(aPropertyName, aValue);
        }


        public string GetProperty(String aPropertyName)
        {
            string property = null;
            if (theProperties.ContainsKey(aPropertyName))
            {
                property = this.theProperties[aPropertyName];
            }
            return property;
        }

        public void SetProperty(String aPropertyName, string aValue)
        {
            if (this.theProperties.ContainsKey(aPropertyName))
            {
                this.theProperties.Remove(aPropertyName);
            }
            theProperties.Add(aPropertyName, aValue);
        }

        /// <summary>
        /// Compares this connection definiton's name with that of another.  Throws an exception
        /// if applied to an object that is not a connection definition
        /// </summary>
        /// <param name="anObject"></param>
        /// <returns>The usual CompareTo result</returns>
        public int CompareTo(object anObject)
        {
            if (anObject is ConnectionDefinition)
            {
                ConnectionDefinition theConnectionDefinition = (ConnectionDefinition)anObject;
                return ToString().CompareTo(theConnectionDefinition.ToString());
            }
            throw new ArgumentException(anObject.GetType() + " is not a ConnectionDefinition");
        }

        /// <summary>
        /// The reason included in a connection definition change event
        /// </summary>
        public enum Reason
        {
            Added,                  // Connection definition was added
            Name,                   // Name was set or changed
            Host,                   // Host was set or changed
            Port,                   // Port was set or changed
            ServerDataSource,       // ServerDataSource was set or changed
            ClientDataSource,       // ClientDataSource was set or changed
            UserName,               // UserName was set or changed
            Password,               // Password was set or changed
            DefaultCatalog,         // DefaultCatalog was set or changed
            DefaultSchema,          // DefaultSchema was set or changed
            DriverString,             // DriverPath was set or changed
            Tested,                 // Connection definition was tested
            Removed,                 // Connection definition was removed
            UserSpecifiedRole,        //The user specified role name was changed
            Connected,        //A connection has been established
            Disconnected,        //The user specified role name was changed
            LiveFeedPort,        //The user specified live feed port was changed
            LiveFeedRetryTimer,  //The user specified live feed retry timer was changed
            PlatformReleaseVersion    //The platform release version was set.
        }

        /// <summary>
        /// The name that user has specified for the connection definition
        /// </summary>
        public string Name
        {
            get { return theName; }
            set
            {

                // Eliminate no-change events
                if (theName.Equals(value))
                {
                    return;
                }
                else
                {
                    // this is a new connection definition
                    theName = value;
                    FireChanged(Reason.Name);
                }
            }
        }

        /// <summary>
        /// The host name.
        /// </summary>
        public string Host
        {
            get { return theHost; }
            set
            {

                // Eliminate no-change events
                if (theHost.Equals(value))
                {
                    return;
                }

                theState = State.NotTested;
                theHost = value;
                FireChanged(Reason.Host);
            }
        }

        /// <summary>
        /// The port number as a string
        /// </summary>
        public string Port
        {
            get { return thePort; }
            set
            {

                // Eliminate no-change events
                if (thePort.Equals(value))
                {
                    return;
                }
                if(theState!=ConnectionDefinition.State.LiveFeedTestSucceeded)
                    theState = State.NotTested; //When live feed connected, user may change the odbc port and then save it. So we cannot set state to not tested
                thePort = value;
                FireChanged(Reason.Port);
            }
        }

        public string ClientDataSource
        {
            get { return theClientDataSource; }
            set 
            {
                // Eliminate no-change events
                if ((theClientDataSource != null) && (theClientDataSource.Equals(value)))
                {
                    return;
                }

                theClientDataSource = value;
                FireChanged(Reason.ClientDataSource);
            }
        }

        public string ConnectedDataSource
        {
            get { return _theConnectedDataSource != null ? _theConnectedDataSource : ""; }
            set { _theConnectedDataSource = value; }
        }

        /// <summary>
        /// The userid to use.
        /// </summary>
        public string UserName
        {
            get { return theUserName; }
            set
            {

                // Eliminate no-change events
                if (theUserName.Equals(value))
                {
                    return;
                }

                theState = State.NotTested;
                theUserName = value;

                // Clear the role name as it may have changed.
                _roleName = null;

                FireChanged(Reason.UserName);
            }
        }
        /// <summary>
        /// The role that the user had specified at logon
        /// </summary>
        public string UserSpecifiedRole
        {
            get 
            {
                string userSpecifiedRole = GetProperty(UserRoleName) as string;
                return (userSpecifiedRole == null) ? "" : userSpecifiedRole; 
            }
            set
            {

                SetProperty(UserRoleName, value);
                FireChanged(Reason.UserSpecifiedRole);
            }
        }

        public string CertificateFileFullPath
        {
            get
            {
                string certificateFile = GetProperty(CertificateFilePath) as string;
                return string.IsNullOrEmpty(certificateFile) ? "" : certificateFile;
            }
            set
            {
                SetProperty(CertificateFilePath, value);
            }
        }

        public string CertificateDir
        {
            get
            {
                string certificateFile = GetProperty(CertificateFilePath) as string;
                if (string.IsNullOrEmpty(certificateFile))
                    return "";

                if (File.Exists(certificateFile))
                {
                    FileInfo fileInfo = new FileInfo(certificateFile);
                    return fileInfo.DirectoryName;
                }

                return "";
            }
        }

        public string CertificateFileName
        {
            get
            {
                string certificateFile = GetProperty(CertificateFilePath) as string;
                if (string.IsNullOrEmpty(certificateFile))
                    return "";

                if (File.Exists(certificateFile))
                {
                    FileInfo fileInfo = new FileInfo(certificateFile);
                    return fileInfo.Name;
                }

                return "";
            }
        }

        /// <summary>
        /// Property: LiveFeedHostName - the host name for the Live Feed Server
        /// </summary>
        public string LiveFeedHostName
        {
            get
            {
                return Host;
            }
        }

        /// <summary>
        /// Property: LiveFeedPort - the port number for the Live Feed Server
        /// </summary>
        public string LiveFeedPort
        {
            get
            {
                string port = GetProperty(LiveFeedPortProperty) as string;
                return (string.IsNullOrEmpty(port) ? "" : port);
            }
            set
            {
                SetProperty(LiveFeedPortProperty, value);
                FireChanged(Reason.LiveFeedPort);
            }
        }

        public string LiveFeedRetryTimer
        {
            get
            {
                string timer = GetProperty(LiveFeedRetryTimerProperty) as string;
                return (string.IsNullOrEmpty(timer) ? DefaultLiveFeedRetryTimer : timer);
            }
            set
            {
                SetProperty(LiveFeedRetryTimerProperty, value);
                FireChanged(Reason.LiveFeedRetryTimer);
            }
        }

        /// <summary>
        /// The userid's password.  Not persisted.
        /// </summary>
        public string Password
        {
            get { return thePassword; }
            set
            {

                // Eliminate no-change events.  The password is null after loading the ConnectionDefinition
                // from persistence since it was not saved to persistence.
                if ((thePassword != null) && thePassword.Equals(value) && thePasswordIsSet)
                {
                    return;
                }

                theState = State.NotTested;
                thePasswordIsSet = !string.IsNullOrEmpty(value);
                thePassword = value;
                FireChanged(Reason.Password);
            }
        }

        //used for change password, but do not change connection status.
        public void SetPassword(string aPassword)
        {
            if (thePassword.Equals(aPassword))
            {
                return;
            }
            thePasswordIsSet = true;
            thePassword = aPassword;
            FireChanged(Reason.Password);
        }

        /// <summary>
        /// True if the password has been set.  We distinguish between password not set and empty password.
        /// An empty password must be entered explicitly.
        /// </summary>
        public bool PasswordIsSet
        {
            get
            {
                return thePasswordIsSet;
            }
        }

        /// <summary>
        /// The default catalog as entered by the user.
        /// </summary>
        public string DefaultCatalog
        {
            get { return "TRAFODION"; }

            /*get { return theDefaultCatalog; }
            set
            {

                // Eliminate no-change events
                if (theDefaultCatalog.Equals(value))
                {
                    return;
                }

                theDefaultCatalog = value;
                FireChanged(Reason.DefaultCatalog);
            }*/
        }

        /// <summary>
        /// The default schema as entered by the user.  May be catalog.schema form.
        /// </summary>
        public string DefaultSchema
        {
            get 
            {
                string currentSchema = "";
                if (theSessionProperties != null)
                {
                    if (theSessionProperties.ContainsKey("CUR_SCHEMA"))
                    {
                        currentSchema = theSessionProperties["CUR_SCHEMA"];
                        if (!string.IsNullOrEmpty(currentSchema))
                        {
                            string[] nameParts = Utilities.CrackSQLAnsiName(currentSchema);
                            if (nameParts.Length >= 2)
                            {
                                currentSchema = nameParts[1];
                            }
                        }
                    }
                    if (!string.IsNullOrEmpty(currentSchema))
                    {
                        currentSchema = Utilities.InternalForm(currentSchema);
                        currentSchema = Utilities.ExternalForm(currentSchema);
                    }
                }
                return string.IsNullOrEmpty(currentSchema) ? theDefaultSchema : currentSchema; 
            }
            set
            {
 
                // Eliminate no-change events
                if (theDefaultSchema.Equals(value))
                {
                    return;
                }

                theDefaultSchema = value;
                FireChanged(Reason.DefaultSchema);
            }
        }

        public string SchemaForConnectionString
        {
            get
            {
                string currentSchema = "";
                if (theSessionProperties != null)
                {
                    if (theSessionProperties.ContainsKey("CUR_SCHEMA"))
                    {
                        currentSchema = theSessionProperties["CUR_SCHEMA"];
                        if (!string.IsNullOrEmpty(currentSchema))
                        {
                            string[] nameParts = Utilities.CrackSQLAnsiName(currentSchema);
                            if (nameParts.Length >= 2)
                            {
                                currentSchema = nameParts[1];
                            }
                        }
                    }
                }
                return string.IsNullOrEmpty(currentSchema) ? theDefaultSchema : currentSchema;
            }
        }

        public string ConnectedServiceName
        {
            get
            {
                string connectedServiceName = "";
                if (theSessionProperties != null)
                {
                    if (theSessionProperties.ContainsKey("CUR_SERVICE"))
                    {
                        connectedServiceName = theSessionProperties["CUR_SERVICE"];
                    }
                }
                return connectedServiceName;
            }
        }

        public string DatabaseUserName
        {
            get
            {
                string dbUserName = "";
                if (theSessionProperties != null)
                {
                    if (theSessionProperties.ContainsKey("DB_USER_NAME"))
                    {
                        dbUserName = theSessionProperties["DB_USER_NAME"];
                    }
                }
                return dbUserName;
            }
        }

        public Int32 DatabaseUserID
        {
            get
            {
                Int32 dbUserID = -1;
                if (theSessionProperties != null)
                {
                    if (theSessionProperties.ContainsKey("USER_ID"))
                    {
                        Int32.TryParse(theSessionProperties["USER_ID"], out dbUserID);
                    }
                }
                return dbUserID;
            }
        }
        /// <summary>
        /// State of controlled schema access. Enabled/disabled
        /// </summary>
        public bool IsControlledSchemaAccessEnabled
        {
            get
            {
                bool isControlled = false;
                if (theSessionProperties != null)
                {
                    if (theSessionProperties.ContainsKey("DEFAULT_SCHEMA_ACCESS_ONLY"))
                    {
                        isControlled = theSessionProperties["DEFAULT_SCHEMA_ACCESS_ONLY"].Equals("ON");
                    }
                }
                return isControlled;
            }
        }

        /// <summary>
        /// The ODBC driver string.
        /// </summary>
        public string DriverString
        {
            get
            {
                return ((theDriverString != null) && (theDriverString.Length > 0)) ? theDriverString : Properties.Resources.DefaultTrafodionOdbcDriverString; 
            }
            set
            {

                // Eliminate no-change events
                if (theDriverString.Equals(value))
                {
                    return;
                }

                theDriverString = value;
                FireChanged(Reason.DriverString);
            }
        }


        /// <summary>
        /// Read only property that returns true if the userid is a services user.
        /// </summary>
        public bool IsServicesUser
        {
            get
            {
                return (!string.IsNullOrEmpty(RoleName) && RoleName.ToUpper().Equals("DB__SERVICES"));
            }
        }

        /// <summary>
        /// Returns true if the user belongs to ROLE.DBA role
        /// </summary>
        public bool isRoleDBAdmin
        {
            get
            {
                return (!string.IsNullOrEmpty(RoleName) && RoleName.ToUpper().Equals("DB__ADMIN"));
            }
        }

        /// <summary>
        /// Read only property that gets the connection defintion's state as a string suitable for display
        /// </summary>
        public string StateString
        {
            get
            {

                switch (theState)
                {
                    case State.NotTested:
                        {
                            return "Disconnected";
                        }
                    case State.TestFailed:
                        {
                            return "Test failed";
                        }
                    case State.TestSucceeded:
                        {
                            return "Connected";
                        }
                    case State.LiveFeedTestSucceeded:
                        return "Live Feed Connected";
                    default:
                        {
                            if (!PasswordIsSet)
                            {
                                return "Password is not set";
                            }
                            return "State has no text";
                        }
                }
            }
        }

        public string MasterSegmentName
        {
            get
            {
                return SystemCatalogName;
            }
        }
        /// <summary>
        /// The system catalog name string.  The system catalog name is needed for many metadata queries.  
        /// </summary>
        /// <returns>The system catalog name</returns>
        public string SystemCatalogName
        {
            get
            {
                return "TRAFODION";
            }
        }

        //public int SystemCatalogVersion
        //{
        //    get
        //    {
        //        if (_systemCatalogVersion == -1)
        //        {
        //            GetSystemCatalogVersion();
        //        }
        //        return _systemCatalogVersion;
        //    }
        //}

        /// <summary>
        /// Given a segment name, tries to find the system catalog name for that segment. If none is found,
        /// the system catalog for the primary segment is returned.
        /// </summary>
        /// <param name="aSegment"></param>
        /// <returns></returns>
        public string SystemCatalogNameForSegment(String aSegment)
        {
            return SystemCatalogName;
        }



        /// <summary>
        /// Server Version. The default is set to SQ110 
        /// </summary>
        public SERVER_VERSION ServerVersion
        {
            get
            {
                if (_platformReleaseVersion.StartsWith("1.0"))
                    return SERVER_VERSION.SQ100;
                else if (_platformReleaseVersion.StartsWith("1.1"))
                    return SERVER_VERSION.SQ110;
                else if (_platformReleaseVersion.StartsWith("1.2"))
                    return SERVER_VERSION.SQ120;
                else if (_platformReleaseVersion.StartsWith("1.3.0")) //M8 pre-release
                    return SERVER_VERSION.SQ130;
                else if (_platformReleaseVersion.StartsWith("1.3.1")) //M8 release
                    return SERVER_VERSION.SQ131;
                else if (_platformReleaseVersion.StartsWith("1.3.2")) //M8 SP1
                    return SERVER_VERSION.SQ132;
                else if (_platformReleaseVersion.StartsWith("1.3.3")) //M8 SP2
                    return SERVER_VERSION.SQ133;
                else if (_platformReleaseVersion.StartsWith("1.3.4")) //M8 SP3
                    return SERVER_VERSION.SQ134;
                else if (_platformReleaseVersion.StartsWith("1.3")) //M8 SP4 and higher M8 SPs
                    return SERVER_VERSION.SQ135;
                else if (_platformReleaseVersion.StartsWith("1.4.0")) // M9
                    return SERVER_VERSION.SQ140;
                else if (_platformReleaseVersion.StartsWith("1.4")) //M9 SP1
                    return SERVER_VERSION.SQ141;
                else if (_platformReleaseVersion.StartsWith("1.5.0")) //M10
                    return SERVER_VERSION.SQ150;
                else if (_platformReleaseVersion.StartsWith("1.5")) //M10 SP1
                    return SERVER_VERSION.SQ151;
                else if (_platformReleaseVersion.StartsWith("1.6")) //M11
                    return SERVER_VERSION.SQ160;
                else
                    return SERVER_VERSION.SQ141;
            }
        }

        /// <summary>
        /// Platform Release Verson String - is the same as Server Version, but returns the original platform version string retrieved from the server. 
        /// </summary>
        public string PlatformReleaseVersionString
        {
            set { _platformReleaseVersionString = value; }
            get { return _platformReleaseVersionString; }
        }

        /// <summary>
        /// The ODBC server version is needed to determine the version of the system. 
        /// </summary>
        public string OdbcServerVersion
        {
            get
            {
                return _odbcServerVersion;
            }
            set { _odbcServerVersion = value; }
        }

        public static bool isServerVersionOnOldR24SUT(String odbcServerVersion)
        {
            String[] parts = odbcServerVersion.Split(new char[] { '.' });
            if (3 <= parts.Length)
            {
                try
                {
                    int nRelease = Int32.Parse(parts[0]);
                    int dotRelease = Int32.Parse(parts[1]);
                    int sutNumber = Int32.Parse(parts[2]);
                    if ((2 == nRelease) && (04 == dotRelease) && (37 >= sutNumber))
                        return true;

                }
                catch (Exception)
                {
                }
            }
            return false;
        } 


        /// <summary>
        /// Read only property that returns the role name of the user
        /// </summary>
        public string RoleName
        {
            get 
            {
                string roleName = "";
                    if (theSessionProperties != null)
                    {
                        if (theSessionProperties.ContainsKey("ROLE_NAME"))
                        {
                            roleName = theSessionProperties["ROLE_NAME"];
                        }
                    }
                    return roleName; 
            }
        }


        /// <summary>
        /// Read only property that gets the connection defintion's state as an enum
        /// </summary>
        public State TheState
        {
            get { return theState; }
        }

        /// <summary>
        /// The list of all connection definitions
        /// </summary>
        public static List<ConnectionDefinition> ConnectionDefinitions
        {
            get
            {
                return theConnectionDefinitions;
            }
        }

        /// <summary>
        /// Add this connection definition to the list.  An Added event will be fired.
        /// </summary>
        /// <param name="anAdder">The source for the event</param>
        public void Add(object anAdder)
        {
            Add(anAdder, this);
        }

        /// <summary>
        /// Add a given connection definition to the list.  An Added event will be fired.
        /// </summary>
        /// <param name="anAdder">The source for the event</param>
        /// <param name="aConnectionDefinition">The connection definition</param>
        static public void Add(object anAdder, ConnectionDefinition aConnectionDefinition)
        {
            theConnectionDefinitions.Add(aConnectionDefinition);
            aConnectionDefinition.FireChanged(anAdder, Reason.Added);
        }

        static public ConnectionDefinition Find(string aName)
        {
            return theConnectionDefinitions.Find(delegate(ConnectionDefinition aConnectionDefinition)
            {
                return aConnectionDefinition.Name.Equals(aName);
            });
        }

        /// <summary>
        /// Remove this connection definition from the list.  A Removed event will be fired.
        /// </summary>
        /// <param name="aRemover">The source for the event</param>
        public void Remove(object aRemover)
        {
            Remove(aRemover, this);
        }

        /// <summary>
        /// Remove a given connection definition from the list.  A Removed event will be fired.
        /// </summary>
        /// <param name="aRemover">The source for the event</param>
        /// <param name="aConnectionDefinition">The connection definition</param>
        static public void Remove(object aRemover, ConnectionDefinition aConnectionDefinition)
        {
            theConnectionDefinitions.Remove(aConnectionDefinition);
            aConnectionDefinition.FireChanged(aRemover, Reason.Removed);
        }

        /// <summary>
        /// A connection definition handler
        /// </summary>
        /// <param name="aSender">The source for the event</param>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <param name="aReason">The reason for the event</param>
        public delegate void ChangedHandler(object aSender, ConnectionDefinition aConnectionDefinition, Reason aReason);

        public delegate void LiveFeedTestHandler(object aSender, ConnectionDefinition aConnectionDefinition);

        [NonSerialized()]
        private static readonly string theChangedKey = "Changed";
        private const string theLiveFeedTestKey = "LiveFeedTest";

        /// <summary>
        /// Add an event handler to, or remove one from, the list
        /// </summary>
        static public event ChangedHandler Changed
        {
            add 
            {
                if (!ExistsHandler(theChangedKey, value))
                {
                    theEventHandlers.AddHandler(theChangedKey, value);
                }
            }
            remove { theEventHandlers.RemoveHandler(theChangedKey, value);
            }
        }

        static bool ExistsHandler(string key, ChangedHandler value)
        {
            ChangedHandler handlerList = (ChangedHandler)theEventHandlers[key];
            if (handlerList != null)
            {
                foreach (Delegate dgate in handlerList.GetInvocationList())
                {
                    if (dgate.Target.GetHashCode() == value.Target.GetHashCode())
                        return true;
                }
            }
            return false;
        }


        static public event LiveFeedTestHandler OnLiveFeedTest;
        static public event ChangedHandler OnLiveFeedPropertyChanged;

        /// <summary>
        /// Fire the live feed test event
        /// </summary>
        /// <param name="e"></param>
        public void FireOnLiveFeedTest()
        {
            if (OnLiveFeedTest != null)
            {
                OnLiveFeedTest(this, this);
            }
        }

        public void FireOnLiveFeedPropertyChanged(Reason aReason) 
        {
            if (OnLiveFeedPropertyChanged != null) 
            {
                OnLiveFeedPropertyChanged(this, this, aReason);
            }
               
        }

        /// <summary>
        /// Fire an event with this connection definition as the source
        /// </summary>
        /// <param name="aReason">The reason for the event</param>
        private void FireChanged(Reason aReason)
        {
            FireChanged(this, this, aReason);
        }

        /// <summary>
        /// Fire an event with a given object as the source
        /// </summary>
        /// <param name="aSender">The source for the event</param>
        /// <param name="aReason">The reason for the event</param>
        private void FireChanged(object aSender, Reason aReason)
        {
            FireChanged(aSender, this, aReason);
        }

        /// <summary>
        /// Fire an event for a given connection definition with a given object as the source
        /// </summary>
        /// <param name="aSender">The source for the event</param>
        /// <param name="aConnectionDefinition">The connection definition</param>
        /// <param name="aReason">The reason for the event</param>
        static private void FireChanged(object aSender, ConnectionDefinition aConnectionDefinition, Reason aReason)
        {

            // Check to see if this is a connection definition that should not fire change events
            if ((aConnectionDefinition._suppressEvents > 0) || (aConnectionDefinition is ScratchConnectionDefinition))
            {

                // If so, don't publish its changes
                return;

            }

            // Get the list of the right kind of handlers
            ChangedHandler theChangedHandlers = (ChangedHandler)theEventHandlers[theChangedKey];

            // Check to see if there any
            if (theChangedHandlers != null)
            {                
                // Multicast to them all
                theChangedHandlers(aSender, aConnectionDefinition, aReason);

            }
        }

        public void CompleteLiveFeedTest(string errorMsg)
        {
            if (string.IsNullOrEmpty(errorMsg))
            {
                this.theState = State.LiveFeedTestSucceeded;
            }
            else if (errorMsg.Contains("113003034"))
            {
                this.theState = State.LiveFeedAuthFailed;
            }
            else
            {
                this.theState = State.TestFailed;
                throw new Exception(errorMsg);
            }

            // Always notify all intersted parties of the test results.
            //FireChanged(this, this, Reason.Tested);
        }

        public void CompleteODBCTest() 
        {
            FireChanged(this, this, Reason.Tested);
        }

    }

    /// <summary>
    /// This class exists to suppress change events on connection definitions as they are being loaded and saved.
    /// See "if (aConnectionDefinition is ScratchConnectionDefinition)" in the persistence code above.
    /// </summary>
    [Serializable]
    public class ScratchConnectionDefinition : ConnectionDefinition
    {

        /// <summary>
        /// Constructor
        /// </summary>
        public ScratchConnectionDefinition()
            : base()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">An existing connection definition</param>
        public ScratchConnectionDefinition(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }

    }

}
