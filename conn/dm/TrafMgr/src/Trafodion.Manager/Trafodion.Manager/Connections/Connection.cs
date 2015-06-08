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
using System.ComponentModel;
using System.Data;
using System.Data.Odbc;
using System.Windows.Forms;
using Trafodion.Manager.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using System.Collections.Generic;

namespace Trafodion.Manager.Framework.Connections
{

    /// <summary>
    /// A connection.  It has an open ODBC connection generated from a given connection definition.
    /// </summary>
    public class Connection
    {
        public static readonly string DefaultSessionName = "MANAGEABILITY";
        private static readonly string _sessionKey = ";SESSION=";
        private static readonly string _serverDSN = ";ServerDSN=";
        private OdbcInfoMessageEventHandler _odbcInfoMessageHandler = null;
        private bool _displayWarnings = false;
        private bool _useCache = true;
        private const int ODBCWarningPasswordExpiring = 8857;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition to use</param>
        public Connection(ConnectionDefinition aConnectionDefinition)
        {

            // Save it
            theConnectionDefinition = aConnectionDefinition;

            //Instantiate the handler for ODBC Info messages
            _odbcInfoMessageHandler = new OdbcInfoMessageEventHandler(InfoMessageHandler);

            // Check to see if its password has been set
            if (!aConnectionDefinition.PasswordIsSet)
            {

                // It has not.  Throw an apropriate exception.
                throw new PasswordNotSetException(aConnectionDefinition);

            }

            // Check to see if it has been tested.
            if (aConnectionDefinition.TheState == ConnectionDefinition.State.NotTested)
            {

                // It has not been tested.  Do so and only show a dialog if the test fails.
                aConnectionDefinition.DoTest(true);

            }

            // Check to see if the connection test failed this time or previously if it had already been done.
            if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestFailed)
            {

                // It has not tested successfully.  Throw an apropriate exception.
                throw new MostRecentConnectionTestFailedException(aConnectionDefinition);

            }

            //// Open an ODBC connection
            //theOdbcConnection = GetOpenOdbcConnection(ConnectionDefinition);

            //// Tell anyone interested that a connection has been opened
            //FireOpened(ConnectionDefinition);

        }

        /// <summary>
        /// Used for testing connection definitions
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="testMode"></param>
        public Connection(ConnectionDefinition aConnectionDefinition, bool testMode)
        {
            theConnectionDefinition = aConnectionDefinition;

            //Instantiate the handler for ODBC Info messages
            _odbcInfoMessageHandler = new OdbcInfoMessageEventHandler(InfoMessageHandler);

            // Check to see if its password has been set
            if (!aConnectionDefinition.PasswordIsSet)
            {
                // It has not.  Throw an apropriate exception.
                throw new PasswordNotSetException(aConnectionDefinition);
            }
        }
    
        /// <summary>
        /// Close our ODBC connection
        /// </summary>
        public void Close()
        {

            // Check to see if we have one
            if (theOdbcConnection != null)
            {
                lock (theOdbcConnection)
                {
                    //// We do; close it
                    //theOdbcConnection.Close();
                    //// Releases all resources used by this ODBC connection.. 
                    //theOdbcConnection.Dispose();

                    //// Notify the ODBC Driver Manager that environment handle can be released when the last underlying connection is released.
                    //OdbcConnection.ReleaseObjectPool();

                    if (_useCache)
                    {
                        //return it back to the pool or close it
                        ConnectionRegistry.Instance.CloseODBCConnection(this, theOdbcConnection);
                    }
                    else
                    {
                        // We do; close it
                        theOdbcConnection.Close();

                        // Releases all resources used by this ODBC connection.. 
                        theOdbcConnection.Dispose();

                        OdbcConnection.ReleaseObjectPool();

                        //Reset state to the default
                        _useCache = true;
                    }

                    // And tell anyone who is interested
                    FireClosed(TheConnectionDefinition);

                }
            }

            theOdbcConnection = null;

        }

        /// <summary>
        /// Preturns true if the current ODBC conneciton state is Open
        /// </summary>
        public bool IsConnectionOpen
        {
            get
            {
                if (theOdbcConnection != null)
                {
                    if (theOdbcConnection.State == ConnectionState.Open)
                        return true;
                    else
                        return false;
                }
                else
                    return false;
            }
        }

        /// <summary>
        /// Returns an open ODBC connection
        /// </summary>
        public OdbcConnection OpenAdminOdbcConnection
        {
            get
            {
                if ((theOdbcConnection != null) && (isAdminConnection == false))
                {
                    throw new Exception("A Non Admin OdbcConnection is associated with this Connection object. Please create a new Connection object and then request an admin OdbcConnection");
                }
                isAdminConnection = true;
                theOdbcConnection = ConnectionRegistry.Instance.GetODBCConnection(this, ConnectionDefinition.AdminLoadDataSource);
                return theOdbcConnection;
                //return GetODBCConnection(ConnectionDefinition.AdminLoadDataSource);
            }
        }

        /// <summary>
        /// Returns an open ODBC connection
        /// </summary>
        public OdbcConnection OpenOdbcConnection
        {
            get
            {
                if ((theOdbcConnection != null) && (isAdminConnection == true))
                {
                    throw new Exception("An Admin OdbcConnection is associated with this Connection object. Please create a new Connection object and then request an OdbcConnection");
                }

                isAdminConnection = false;
                theOdbcConnection = ConnectionRegistry.Instance.GetODBCConnection(this, TheConnectionDefinition.ConnectedDataSource);
                return theOdbcConnection;
                //return GetODBCConnection(TheConnectionDefinition.ConnectedDataSource);
            }
        }

        /// <summary>
        /// Returns an open ODBC connection
        /// </summary>
        public OdbcConnection OpenNonCachedOdbcConnection
        {
            get
            {
                if (theOdbcConnection != null)
                {
                    if (theOdbcConnection.State == ConnectionState.Closed)
                    {
                        theOdbcConnection.Open();

                        // Tell anyone who is interested
                        FireOpened(TheConnectionDefinition);
                    }
                    return theOdbcConnection;
                }

                isAdminConnection = false;
                _useCache = false;
                theOdbcConnection = GetODBCConnection(TheConnectionDefinition.ConnectedDataSource);
                return theOdbcConnection;
            }
        }
        public OdbcConnection OpenNonCachedSQWBOdbcConnection
        {
            get
            {
                if (theOdbcConnection != null)
                {
                    if (theOdbcConnection.State == ConnectionState.Closed)
                    {
                        theOdbcConnection.Open();
                        FireOpened(TheConnectionDefinition);
                    }
                    return theOdbcConnection;
                }
                isAdminConnection = false;
                _useCache = false;
                theOdbcConnection = GetODBCConnection(ConnectionDefinition.SQLWB_DataSource);
                return theOdbcConnection;
            }
        }

        /// <summary>
        /// Creates a ODBC connection given a server DS Name
        /// </summary>
        /// <param name="serverDsName"></param>
        /// <returns></returns>
        public OdbcConnection GetODBCConnection(string serverDsName)
        {
            bool successful = false;

            // Check to see if we have an ODBC connection
            if (theOdbcConnection == null)
            {
                // We do not; create the ODBC connection
                theOdbcConnection = new OdbcConnection();
            }

            // Make sure that the ODBC connection is open
            if (theOdbcConnection.State == System.Data.ConnectionState.Closed)
            {
                try
                {
                    if (_displayWarnings)
                    {
                        theOdbcConnection.InfoMessage += this._odbcInfoMessageHandler;
                    }
                    bool usingClientDSN = false;  
                    //Try 
                    try
                    {
                        if (! String.IsNullOrEmpty(TheConnectionDefinition.ClientDataSource))
                        {
                            if (string.IsNullOrEmpty(TheConnectionDefinition.ConnectedDataSource))
                            {
                                TheConnectionDefinition.ConnectedDataSource = TheConnectionDefinition.ClientDataSource;
                            }
                            usingClientDSN = OdbcAccess.IsUsingClientDSN(TheConnectionDefinition);

                            if (usingClientDSN)
                            {
                                if (serverDsName.Equals(ConnectionDefinition.AdminLoadDataSource))
                                {
                                    theOdbcConnection.ConnectionString = TheConnectionDefinition.AdminConnectionString + SessionString;
                                }
                                else
                                {
                                    theOdbcConnection.ConnectionString = TheConnectionDefinition.ClientDSNConnectionString + SessionString;
                                }

                                GeneralOptions generalOptions = GeneralOptions.GetOptions();
                                if (generalOptions != null)
                                {
                                    theOdbcConnection.ConnectionTimeout = generalOptions.ConnectionTimeOut;
                                }
                                else
                                {
                                    // Assign a longer timeout value, the default is only 15 secs.  
                                    theOdbcConnection.ConnectionTimeout = 180;
                                }
                                DateTime startTime = DateTime.Now;
                                theOdbcConnection.Open();
                                DateTime endtime = DateTime.Now;
                                TimeSpan duration = endtime - startTime;
                                Logger.OutputToLog(TraceOptions.TraceOption.PERF, TraceOptions.TraceArea.Connection, "Open", String.Format("\t{0}\t{1}", "Open new odbc connection", Utilities.formatTimeSpan(duration)));
                            }
                        }                        
                    }
                    catch (Exception e) 
                    {
                        usingClientDSN = false;
                    }

                    if (!usingClientDSN)
                    {
                        ////Append the session name to the connection string.
                        if (serverDsName.Equals(ConnectionDefinition.AdminLoadDataSource))
                        {

                            theOdbcConnection.ConnectionString = TheConnectionDefinition.AdminConnectionString + SessionString;
                        }
                        else if (serverDsName.Equals(ConnectionDefinition.SQLWB_DataSource))
                        {
                            theOdbcConnection.ConnectionString = TheConnectionDefinition.SQLWBConnectionString + SessionString;
                        }
                        else
                        {
                            theOdbcConnection.ConnectionString = TheConnectionDefinition.ConnectionString + this.GetServerDSNString(TheConnectionDefinition.ConnectedDataSource) + SessionString;
                        }
                        GeneralOptions generalOptions = GeneralOptions.GetOptions();
                        if (generalOptions != null)
                        {
                            theOdbcConnection.ConnectionTimeout = generalOptions.ConnectionTimeOut;
                        }
                        else
                        {
                            // Assign a longer timeout value, the default is only 15 secs.  
                            theOdbcConnection.ConnectionTimeout = 180;
                        }
                        DateTime startTime = DateTime.Now;
                        theOdbcConnection.Open();
                        DateTime endtime = DateTime.Now;
                        TimeSpan duration = endtime - startTime;
                        Logger.OutputToLog(TraceOptions.TraceOption.PERF, TraceOptions.TraceArea.Connection, "Open", String.Format("\t{0}\t{1}", "Open new odbc connection", Utilities.formatTimeSpan(duration)));
                    }
                    
                }
                finally
                {
                    if (theOdbcConnection != null && _displayWarnings)
                    {
                        theOdbcConnection.InfoMessage -= this._odbcInfoMessageHandler;
                    }

                    if (TheConnectionDefinition.TheState == ConnectionDefinition.State.PasswordExpired)
                    {
                        FireClosed(TheConnectionDefinition);
                        TheConnectionDefinition.ResetState();
                        theOdbcConnection = null;
                        successful = false;
                    }
                    else
                    {
                        successful = true;
                    }
                }

                // Tell anyone who is interested
                if (successful)
                {
                    try
                    {
                        List<string> startupSQLStatements = TrafodionContext.Instance.StartupSQLStatements;
                        foreach (string sqlStatement in startupSQLStatements)
                        {
                            OdbcCommand odbcCommand = new OdbcCommand(sqlStatement, theOdbcConnection);
                            Utilities.ExecuteNonQuery(odbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, "Connection", true);
                            odbcCommand.Dispose();
                        }
                    }
                    catch (Exception ex)
                    {
                    }
                    FireOpened(TheConnectionDefinition);
                }

            }

            // Return the open ODBC connection
            return theOdbcConnection;
        }

        /// <summary>
        /// Handle Odbc informational messages during connection open
        /// New password expiring warning should look like this: 
        /// 8857 HY011 99999 BEGINNER INFRM LOGONLY Local authentication : User: {username}
        /// : password expired but in grace period. Change password using NCI or TrafodionManager
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="eArgs"></param>
        private void InfoMessageHandler(Object sender,
                                           OdbcInfoMessageEventArgs eArgs)
        {
            String defaultDataSourceWarning = "Connected to the default data source:";
            String passwordExpiringWarning = "password expired but in grace period";
            int idx;
            DataTable warningsTable = new DataTable();
            warningsTable.Columns.Add("Warning Text");


            //If the user specified datasource does not exist or is not available
            //get the currently connected datasource from the Info message
            foreach (OdbcError oe in eArgs.Errors)
            {
                String msg = oe.Message;

                if (-1 != (idx = msg.IndexOf(defaultDataSourceWarning)))
                {
                    String defaultDSN = msg.Substring(idx + defaultDataSourceWarning.Length);
                    defaultDSN = defaultDSN.Trim();
                    if (defaultDSN.EndsWith("."))
                        defaultDSN = defaultDSN.Substring(0, defaultDSN.Length - 1);

                    if (!String.IsNullOrEmpty(TheConnectionDefinition.ConnectedDataSource))
                    {
                        //If the user specified datasource does not match with currently connected datasource
                        //display a warning message box
                        if (!TheConnectionDefinition.ConnectedDataSource.Equals(defaultDSN))
                        {
                            if (eArgs.Errors.Count == 1)
                            {
                                warningsTable.Rows.Add(new object[] { String.Format(Properties.Resources.DefaultDataSourceConnectedWarning,
                                        TheConnectionDefinition.ClientDataSource, Environment.NewLine + Environment.NewLine, defaultDSN)});
                            }
                            else
                            {
                                warningsTable.Rows.Add(new object[] { String.Format(Properties.Resources.DefaultDataSourceConnectedWarning,
                                        TheConnectionDefinition.ClientDataSource, "", defaultDSN)});
                            }
                        }
                    }
                    //Remember the currently connected datasource in the connection definition
                    //This datasource will be used for all future connections made on this connection definition
                    //But this connected datasource information is not persisted. It is used only for the current session of this conenction definition
                    this.TheConnectionDefinition.ConnectedDataSource = defaultDSN;
                }
                else if (-1 != (idx = msg.IndexOf(passwordExpiringWarning)))
                {
                    ChangePasswordDialog dialog = new ChangePasswordDialog(this.TheConnectionDefinition);
                    dialog.StatusText = Properties.Resources.PasswordExpiredMessage;
                    DialogResult result = dialog.ShowDialog();

                    if (result == DialogResult.OK)
                    {
                        return;
                    }

                    this.TheConnectionDefinition.PasswordExpired();
                    return;
 
                }
                else
                {
                    if (!ConnectionRegistry.Instance.ConnectionExists(TheConnectionDefinition))
                    {
                        warningsTable.Rows.Add(new object[] { msg });
                    }
                }
            }

            if (warningsTable.Rows.Count > 1)
            {
                string summaryMessage = String.Format("{0} warnings were reported.", warningsTable.Rows.Count);
                TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(summaryMessage, warningsTable, System.Drawing.SystemIcons.Warning);
                md.ShowDialog();
            }
            else if(warningsTable.Rows.Count == 1)
            {
                MessageBox.Show(warningsTable.Rows[0].ItemArray[0].ToString(), Properties.Resources.Warning, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        // The connection definition that we are using
        private ConnectionDefinition theConnectionDefinition;

        /// <summary>
        /// Readonly property for the connection definition that we are using
        /// </summary>
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return theConnectionDefinition; }
        }

        /// <summary>
        /// Get the system catalog name for the connection. It uses the segment name from the 
        /// OdbcConnection Datasource to determine the system catalog name
        /// </summary>
        public string SystemCatalogName
        {
            get
            {
                return theConnectionDefinition != null ? theConnectionDefinition.SystemCatalogName : "TRAFODION";
            }
        }

        /// <summary>
        /// The datasource name has the following fromat.
        /// 
        /// The following code parses the segment name from the datasource name
        /// </summary>
        /// <returns></returns>
        private String DatasourceSegment()
        {
            if (theOdbcConnection != null)
            {
                String ds = theOdbcConnection.DataSource;

                if (ds != null)
                {
                    int startIdx = ds.IndexOf("\\");
                    int endIdx = ds.IndexOf("(");
                    if ((startIdx >= 0) && (endIdx > 0) && (endIdx > startIdx))
                    {
                        return ds.Substring(startIdx + 1, (endIdx - startIdx - 1));
                    }
                }
            }
            return null;
        }

        /// <summary>
        /// The default session connection string
        /// </summary>
        public static string DefaultSessionString
        {
            get { return _sessionKey + DefaultSessionName; }
        }

        /// <summary>
        /// The session connection string that is passed in the odbc connection
        /// </summary>
        public string SessionString
        {
            get
            {
                //If session name property is null use the default session name
                if (String.IsNullOrEmpty(SessionName))
                {
                    return _sessionKey + DefaultSessionName;
                }
                else
                {
                    return _sessionKey + SessionName;
                }
            }
        }

        private string GetServerDSNString(string dsName)
        {
            if (String.IsNullOrEmpty(dsName))
            {
                return "";
            }
            else
            {
                return _serverDSN + dsName;
            }
        }

        /// <summary>
        /// The odbc session name
        /// </summary>
        public string SessionName
        {
            get { return sessionName; }
            set { sessionName = value; }
        }

        //Specifies if the connection was created for the Admin load DS
        public bool IsAdminConnection
        {
            get { return isAdminConnection; }
        }

        /// <summary>
        /// Specifies if warnings should be displayed when new odbc connections are opened
        /// </summary>
        public bool DisplayWarnings
        {
            get { return _displayWarnings; }
            set { _displayWarnings = value; }
        }

        /// <summary>
        /// Our underlying ODBC connection
        /// </summary>
        private OdbcConnection theOdbcConnection = null;
        private string sessionName = null;
        private bool isAdminConnection = false;


        /// <summary>
        /// Handler for ODBC connection opened
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        public delegate void OpenedHandler(ConnectionDefinition aConnectionDefinition);

        /// <summary>
        /// Handler for ODBC connection closed
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        public delegate void ClosedHandler(ConnectionDefinition aConnectionDefinition);


        /// <summary>
        /// The list of parites interested in opening and/or closing ODBC conncetions
        /// </summary>
        static private EventHandlerList theEventHandlers = new EventHandlerList();

        /// <summary>
        /// The key for opened event listeners
        /// </summary>
        private static readonly string theOpenedKey = "OdbcConnectionOpened";

        /// <summary>
        /// The key for closed event listeners
        /// </summary>
        private static readonly string theClosedKey = "OdbcConnectionClosed";

        /// <summary>
        /// Add an ODBC connection opened handler
        /// </summary>
        static public event OpenedHandler Opened
        {
            add { theEventHandlers.AddHandler(theOpenedKey, value); }
            remove { theEventHandlers.RemoveHandler(theOpenedKey, value); }
        }

        /// <summary>
        /// Tell everyone interrested that an ODBC connection has been opened
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        static private void FireOpened(ConnectionDefinition aConnectionDefinition)
        {
            OpenedHandler theOpenedHandlers = (OpenedHandler)theEventHandlers[theOpenedKey];

            if (theOpenedHandlers != null)
            {
                theOpenedHandlers(aConnectionDefinition);
            }
        }

        /// <summary>
        /// Add an ODBC connection closed handler
        /// </summary>
        static public event ClosedHandler Closed
        {
            add { theEventHandlers.AddHandler(theClosedKey, value); }
            remove { theEventHandlers.RemoveHandler(theClosedKey, value); }
        }

        /// <summary>
        /// Tell everyone interrested that an ODBC connection has been closed
        /// </summary>
        /// <param name="aConnectionDefinition">The associated connection definition</param>
        static private void FireClosed(ConnectionDefinition aConnectionDefinition)
        {
            ClosedHandler theClosedHandlers = (ClosedHandler)theEventHandlers[theClosedKey];

            if (theClosedHandlers != null)
            {
                theClosedHandlers(aConnectionDefinition);
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="password"></param>
        /// <returns></returns>
        public string EncryptPassword(string password)
        {
            if (String.IsNullOrEmpty(password))
            {
                // Just return an empty string.
                return "";
            }

            OdbcConnection connection = OpenOdbcConnection;
            return OdbcAccess.EncryptPassword(connection, password);
        }

        /// <summary>
        /// Change user passwords
        /// </summary>
        /// <param name="name">Either an User name or a Role name</param>
        /// <param name="oldPassword"></param>
        /// <param name="newPassword"></param>
        public void ChangePassword(string name, string oldPassword, string newPassword)
        {
            OdbcConnection connection = OpenOdbcConnection;
            string encryptedOldPwd = this.EncryptPassword(oldPassword);
            string encryptedNewPwd = this.EncryptPassword(newPassword);

            OdbcCommand theQuery = new OdbcCommand("CALL MANAGEABILITY.TRAFODION_SPJ.CHANGEPASSWORD(?, ?, ?)");
            theQuery.CommandType = System.Data.CommandType.StoredProcedure;
            OdbcParameter param1 = theQuery.Parameters.Add("@USERNAME", OdbcType.Text, name.Length);
            param1.Direction = System.Data.ParameterDirection.Input;
            param1.DbType = System.Data.DbType.String;
            param1.Value = name;
            OdbcParameter param2 = theQuery.Parameters.Add("@OLDPASSWORD", OdbcType.Text, encryptedOldPwd.Length);
            param2.Direction = System.Data.ParameterDirection.Input;
            param2.DbType = System.Data.DbType.String;
            param2.Value = encryptedOldPwd;
            OdbcParameter param3 = theQuery.Parameters.Add("@NEWPASSWORD", OdbcType.Text, encryptedNewPwd.Length);
            param3.Direction = System.Data.ParameterDirection.Input;
            param3.DbType = System.Data.DbType.String;
            param3.Value = encryptedNewPwd;
//#if DEBUG
//            Console.WriteLine("CALL MANAGEABILITY.TRAFODION_SPJ.CHANGEPASSWORD(?, ?, ?):");
//            Console.WriteLine(String.Format("Param {0}=[{1}]", 1, name));
//            Console.WriteLine(String.Format("Param {0}=[{1}]", 2, encryptedOldPwd));
//            Console.WriteLine(String.Format("Param {0}=[{1}]", 3, encryptedNewPwd));
//#endif
//            theQuery.CommandTimeout = 0;
//            theQuery.Connection = connection;
//            theQuery.ExecuteNonQuery();
            theQuery.Connection = connection;
            Utilities.ExecuteNonQuery(theQuery, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Connection, "Change Password", false);
            //after change password, we should keep new password in connection definition.
            if (theConnectionDefinition.Password.IndexOf('/') > 0)
            {
                string tmp = theConnectionDefinition.Password;
                string rolePassword = tmp.Substring(tmp.IndexOf('/') + 1, (tmp.Length - tmp.IndexOf('/') - 1));
                this.theConnectionDefinition.SetPassword(newPassword + "/" + rolePassword);
            }
            else
            {
                this.theConnectionDefinition.SetPassword(newPassword);
            }
        }
    }

    /// <summary>
    /// Base class for execptions related to use of connection definitions
    /// </summary>
    public class ConnectionDefinitionException : Exception
    {

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">The connection definition exhibiting the exception</param>
        public ConnectionDefinitionException(ConnectionDefinition aConnectionDefinition)
        {
            theConnectionDefinition = aConnectionDefinition;
        }

        /// <summary>
        /// Get the connection definition exhibiting the exception
        /// </summary>
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return theConnectionDefinition; }
        }

        /// <summary>
        /// The connection definition exhibiting the exception
        /// </summary>
        ConnectionDefinition theConnectionDefinition;

    }

    /// <summary>
    /// Exception generated when trying to use a connection definition whose password has not been set
    /// </summary>
    public class PasswordNotSetException : ConnectionDefinitionException
    {
        public PasswordNotSetException(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }
    }

    /// <summary>
    /// Exception generated when trying to use a connection definition whose connection test has failed
    /// </summary>
    public class MostRecentConnectionTestFailedException : ConnectionDefinitionException
    {
        public MostRecentConnectionTestFailedException(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }
    }
}
