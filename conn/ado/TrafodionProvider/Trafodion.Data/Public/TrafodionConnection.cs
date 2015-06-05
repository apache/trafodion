/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2015 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

namespace Trafodion.Data
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Data;
    using System.Data.Common;
    using System.Diagnostics;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;
    using System.Net.Sockets;

    /// <summary>
    /// Represents an open connection to a TrafodionDB database. This class cannot be inherited.
    /// </summary>
    public sealed class TrafodionDBConnection : DbConnection, IDbConnection
    {
        private static Dictionary<string, ConnectionPool> _connPools;

        // all the private members for the connection are actually stored on the Network object to support connection pooling

        private TrafodionDBMetaData _metaData;
        private TrafodionDBSecurity _security;

        private ConnectionContext _cc;
        private UserDescription _ud;

        private ConnectionState _state; // never set this directly, use SetState to make sure events are fired properly
        private string _serverVersion;
        private string _datasource; // the server side datasource name
        private string _database;
        private string _asAddress;
        private int _asPort;

        private long _labelId; // the current label id to be used when creating new statements
        private object _labelLock; // a locking object to ensure that duplicate labels are not generated

        //to sycronize Fetch, Execute, Disconnect. Each connection obj has isolated lock.
        internal readonly List<String> dataAccessLock = new List<String>();

        static TrafodionDBConnection()
        {
            TrafodionDBConnection._connPools = new Dictionary<string, ConnectionPool>();
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBConnection class.
        /// </summary>
        public TrafodionDBConnection()
            : this(string.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBConnection class.
        /// </summary>
        /// <param name="connectionString">The connection string for this connection</param>
        public TrafodionDBConnection(string connectionString)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Public);
            }

            this._labelId = 1;
            this._labelLock = new object();

            this._serverVersion = string.Empty;
            this._datasource = string.Empty;
            this._database = null;
            this._state = ConnectionState.Closed;

            this.ConnectionStringBuilder = new TrafodionDBConnectionStringBuilder(connectionString);
            this.Network = new TrafodionDBNetwork(this);
            this.Commands = new List<TrafodionDBCommand>();
            this._metaData = new TrafodionDBMetaData(this);
        }

        /// <summary>
        /// Event for viewing status and warning messages during operation.
        /// </summary>
        public event TrafodionDBInfoMessageEventHandler InfoMessage;

        /// <summary>
        /// Gets or sets the string used to open the connection.
        /// </summary>
        public override string ConnectionString
        {
            get
            {
                return this.ConnectionStringBuilder.ConnectionString;
            }

            set
            {
                if (this._state != ConnectionState.Closed)
                {
                    string msg = TrafodionDBResources.FormatMessage(TrafodionDBMessage.InvalidConnectionState, this._state);
                    TrafodionDBException.ThrowException(this, new InvalidOperationException(msg));
                }

                this.ConnectionStringBuilder.ConnectionString = value;
            }
        }

        /// <summary>
        /// The current server datasource name.
        /// </summary>
        public override string DataSource
        {
            get
            {
                return this._datasource;
            }
        }

        /// <summary>
        /// The current catalog.
        /// </summary>
        public override string Database
        {
            get
            {
                if (this._database == null)
                {
                    string cat = string.Empty;
                    int index;

                    using (TrafodionDBCommand cmd = this.CreateCommand())
                    {
                        cmd.CommandText = "control query default showcontrol_unexternalized_attrs 'ON'";
                        cmd.ExecuteNonQuery();
                        cmd.CommandText = "SHOWCONTROL DEFAULT CATALOG, match full, no header";
                        cat = (string)cmd.ExecuteScalar();
                        index = cat.IndexOf('.');

                        if (cat[0] != '\"' && index != -1)
                        {
                            cat = cat.Substring(index + 1);
                        }
                    }

                    this._database = cat;
                }

                return this._database;
            }
        }

        /// <summary>
        /// Gets the TrafodionDB server version.
        /// </summary>
        public override string ServerVersion
        {
            get { return this._serverVersion; }
        }

        /// <summary>
        /// Gets the current <code>ConnectionState</code>.  Currently supports <code>Open</code> and <code>Closed</code>
        /// </summary>
        public override ConnectionState State
        {
            get {
                if (this.Network.isIdleTimeout)
                {
                    TrafodionDBException.ThrowException(this, new CommunicationsFailureException("Idle Timeout"));
                }
                return this._state;
            }
        }

        /// <summary>
        /// Gets the remote segment and process in the form:<code>segment</code>.$<code>process</code>
        /// </summary>
        public string RemoteProcess
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the remote TCP/IP port currently being used
        /// </summary>
        public int RemotePort
        {
            get;
            private set;
        }

        /// <summary>
        /// Gets the remote address currently connected to.
        /// </summary>
        public string RemoteAddress
        {
            get;
            private set;
        }

        internal TrafodionDBTransaction Transaction
        {
            get;
            set;
        }

        internal List<TrafodionDBCommand> Commands // a list of the currently allocated commands
        {
            get;
            set;
        }

        internal TrafodionDBNetwork Network
        {
            get;
            set;
        }

        internal TrafodionDBConnectionStringBuilder ConnectionStringBuilder
        {
            get;
            set;
        }

        internal Version[] ServerVersionList // the raw version information from the server
        {
            get;
            set;
        }

        internal ByteOrder ByteOrder
        {
            get { return this.Network.ByteOrder; }
        }

        internal TrafodionDBEncoder Encoder
        {
            get { return this.Network.Encoder; }
        }

        /// <summary>
        /// Provides access to the default TrafodionDBFactory instance.
        /// </summary>
        protected override DbProviderFactory DbProviderFactory
        {
            get
            {
                return TrafodionDBFactory.Instance;
            }
        }

        /// <summary>
        /// Releases the resources used by the TrafodionDBConnection.
        /// </summary>
        public new void Dispose()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public);
            }

            GC.SuppressFinalize(this);

            if (this.State == ConnectionState.Open)
            {
                try
                {
                    this.Close();
                }
                catch
                {
                }

                try
                {
                    this.Network.Dispose();
                }
                catch
                {
                }
            }

            base.Dispose();
        }

        /// <summary>
        /// Starts a database transaction.
        /// </summary>
        /// <returns>An object representing the new transaction.</returns>
        public new TrafodionDBTransaction BeginTransaction()
        {
            return this.BeginTransaction(IsolationLevel.ReadCommitted);
        }

        /// <summary>
        /// Starts a database transaction.
        /// </summary>
        /// <returns>An object representing the new transaction.</returns>
        IDbTransaction IDbConnection.BeginTransaction()
        {
            return this.BeginTransaction();
        }

        /// <summary>
        /// Starts a database transaction with the specified isolation level.
        /// </summary>
        /// <param name="isolationLevel">The isolation level under which the transaction should run.</param>
        /// <returns>An object representing the new transaction.</returns>
        public new TrafodionDBTransaction BeginTransaction(IsolationLevel isolationLevel)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public, isolationLevel);
            }

            if (this._state != ConnectionState.Open)
            {
                throw new Exception("connection is not open");
            }

            if (this.Transaction != null)
            {
                throw new InvalidOperationException("multiple transactions are not supported");
            }

            if (isolationLevel == IsolationLevel.Chaos || isolationLevel == IsolationLevel.Snapshot)
            {
                throw new ArgumentException("unsupported IsolationLevel");
            }

            this.Transaction = new TrafodionDBTransaction(this, isolationLevel);

            return this.Transaction;
        }

        /// <summary>
        /// Starts a database transaction with the specified isolation level.
        /// </summary>
        /// <param name="il">The isolation level under which the transaction should run. </param>
        /// <returns>An object representing the new transaction.</returns>
        IDbTransaction IDbConnection.BeginTransaction(IsolationLevel il)
        {
            return this.BeginTransaction(il);
        }

        /// <summary>
        /// Issues a "set catalog" command
        /// </summary>
        /// <param name="databaseName">Catalog name</param>
        public override void ChangeDatabase(string databaseName)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public, databaseName);
            }

            try
            {
                using (TrafodionDBCommand cmd = this.CreateCommand())
                {
                    cmd.CommandText = "set catalog " + databaseName;
                    cmd.ExecuteNonQuery();

                    this._database = databaseName;
                }
            }
            catch (Exception e)
            {
                throw e;
            }
        }

        /// <summary>
        /// Closes the connection, freeing all server and client resources.
        /// </summary>
        public override void Close()
        {
            Close(false);
        }

        internal void Close(bool forceClose)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public, forceClose);
            }

            try
            {

                lock (this.dataAccessLock)
                {
                    // unless we need to force close the connection in the pool, ignore requests if the connection is already
                    // marked as closed
                    if (this.State != ConnectionState.Closed || forceClose)
                    {
                        // pool the connection
                        if (!forceClose && this.ConnectionStringBuilder.MaxPoolSize > 0)
                        {
                            TrafodionDBConnection._connPools[this.ConnectionStringBuilder.ConnectionString].AddPooledConnection(this);

                            while (this.Commands.Count > 0)
                            {
                                this.RemoveCommand(this.Commands[0]);
                            }
                        }
                        // close the connection
                        else if (!this.Network.IsClosed)
                        {
                            // note that we are not checking for an active transaction
                            // just let them disconnect even if we have a transaction in progress
                            this.Disconnect();
                        }

                        this.SetState(ConnectionState.Closed);
                    }
                }
            }
            catch
            { //close should never throw an exception
            }
        }

        /// <summary>
        /// Creates a new <code>TrafodionDBCommand</code> assocaited with this connection.
        /// </summary>
        /// <returns>The <code>TrafodionDBCommand</code> object created</returns>
        public new TrafodionDBCommand CreateCommand()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public);
            }

            TrafodionDBCommand cmd = new TrafodionDBCommand(string.Empty, this, this.Transaction);
            this.Commands.Add(cmd);

            return cmd;
        }

        /// <summary>
        /// Creates a new <code>TrafodionDBCommand</code> with the given label assocaited with this connection.
        /// </summary>
        /// <param name="cmdLabel">The label to be associated with the newly created TrafodionDBCommand.</param>
        /// <returns>The <code>TrafodionDBCommand</code> object created</returns>
        public new TrafodionDBCommand CreateCommand(string cmdLabel)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public, cmdLabel);
            }

            TrafodionDBCommand cmd = new TrafodionDBCommand(string.Empty, this, this.Transaction, cmdLabel);
            this.Commands.Add(cmd);

            return cmd;
        }

        /// <summary>
        /// Creates and returns a TrafodionDBCommand object associated with the TrafodionDBConnection.
        /// </summary>
        /// <returns>A TrafodionDBCommand object.</returns>
        IDbCommand IDbConnection.CreateCommand()
        {
            return this.CreateCommand();
        }

        /// <summary>
        /// Retrieves a list of valid metadata collections.  This is equivilant to calling <code>GetSchema("MetaDataCollections")</code>
        /// </summary>
        /// <returns>A DataTable containing information about the metadata collections available.</returns>
        public override DataTable GetSchema()
        {
            return this._metaData.GetSchema("MetaDataCollections", new string[0]);
        }

        /// <summary>
        /// Retrieves metadata information without restrictions based on the <code>collectionName</code> provided.  Valid collections can be obtained by using "MetaDataCollections" as the collection name
        /// </summary>
        /// <param name="collectionName">The metadata collection name.</param>
        /// <returns>A <code>DataTable</code> containing the metadata requested.</returns>
        public override DataTable GetSchema(string collectionName)
        {
            return this._metaData.GetSchema(collectionName, new string[0]);
        }

        /// <summary>
        /// Retrieves metadata information based on the <code>collectionName</code> and restrictions provided.  Valid collections can be obtained by using "MetaDataCollections" as the collection name
        /// </summary>
        /// <param name="collectionName">The metadata collection name.</param>
        /// <param name="restrictionValues">A list of restriction values.</param>
        /// <returns>A <code>DataTable</code> containing the metadata requested.</returns>
        public override DataTable GetSchema(string collectionName, string[] restrictionValues)
        {
            if (restrictionValues == null)
            {
                restrictionValues = new string[0];
            }

            return this._metaData.GetSchema(collectionName, restrictionValues);
        }

        /// <summary>
        /// Opens the connection to the TrafodionDB Server.
        /// </summary>
        public override void Open()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Public, this.ConnectionStringBuilder.TraceableConnectionString);
            }

            if (this._state != ConnectionState.Closed)
            {
                throw new Exception("bad connection state");
            }

            bool initialized = false;
            string connectionString = this.ConnectionStringBuilder.ConnectionString;
            ConnectionPool pool;
            Monitor.Enter(TrafodionDBConnection._connPools);
            if (this.ConnectionStringBuilder.MaxPoolSize > 0)
            {
                if (!TrafodionDBConnection._connPools.ContainsKey(connectionString))
                {
                    pool = new ConnectionPool(this.ConnectionStringBuilder);
                    TrafodionDBConnection._connPools.Add(connectionString, pool);
                }
                else
                {
                    pool = TrafodionDBConnection._connPools[connectionString];
                }

                TrafodionDBConnection conn = pool.GetPooledConnection();
                if (conn != null)
                {
                    CopyProperties(conn);
                    initialized = true;
                }
            }
            Monitor.Exit(TrafodionDBConnection._connPools);


            if (!initialized)
            {
                this.ParseServerString(this.ConnectionStringBuilder.Server);

                this.CreateConnectionContext();
                this.CreateUserDescription();

                GetObjRefReply reply = this.GetObjRef();

                this._cc.InContextOptions1 |= ConnectionContextOptions1.CertTimestamp;

                if (reply.securityEnabled)
                {
                    this.SecureLogin(reply);
                }
                else
                {
                    this._ud.Password = this.EncodePassword(this.ConnectionStringBuilder.Password);
                    this.InitDiag(false);
                }

                this.SetState(ConnectionState.Open);
            }
        }

        /// <summary>
        /// Returns an encrypted base 64 encoded string of the data provided.
        /// </summary>
        /// <param name="data">The data to encrypt.</param>
        /// <returns>An encrypted base 64 encoded string.</returns>
        public string EncryptData(byte [] data)
        {
            return this._security.EncryptData(data);
        }

        internal void Cancel()
        {
            if (!this.ConnectionStringBuilder.IgnoreCancel)
            {
                // we need to create a new network object to handle cancel
                TrafodionDBNetwork net = new TrafodionDBNetwork(this);
                net.ByteOrder = this.Network.ByteOrder;
                net.DialogueId = this.Network.DialogueId;

                CancelMessage message;
                CancelReply reply;

                message = new CancelMessage()
                {
                    ServerType = 2,
                    ServerObjRef = this.RemotePort.ToString(),
                    StopType = 0
                };

                reply = net.Cancel(this._asAddress, this._asPort, message);
                if (reply.error != CancelError.Success)
                {
                    throw new Exception("error during cancel: " + reply.error + " " + reply.errorDetail + " " + reply.errorText);
                }
            }
        }

        internal void SendInfoMessage(object sender, TrafodionDBException e)
        {
            if (this.InfoMessage != null && e.Warnings.Count > 0)
            {
                this.InfoMessage(sender, new TrafodionDBInfoMessageEventArgs(e.Warnings));
            }
        }

        internal void SetState(ConnectionState state)
        {
            if (this._state != state)
            {
                StateChangeEventArgs ea = new StateChangeEventArgs(this._state, state);
                this._state = state;
                this.OnStateChange(ea);
            }
        }

        internal string GenerateStatementLabel()
        {
            string label;

            lock (this._labelLock)
            {
                label = "SQL_CUR_" + this._labelId++;
            }

            return label;
        }

        internal bool RemoveCommand(TrafodionDBCommand cmd)
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this, TraceLevel.Internal, cmd);
            }

            CloseMessage message;
            CloseReply reply;

            // if we still have an open datareader, flag the datareader to free on close
            if (cmd.DataReader != null && !cmd.DataReader.IsClosed)
            {
                cmd.DataReader.FreeOnClose = true;
            }
            else if (this._state == ConnectionState.Open)
            {
                message = new CloseMessage()
                {
                    Label = cmd.Label,
                    Option = CloseResourceOption.Free
                };

                reply = this.Network.Close(message);
                switch (reply.error)
                {
                    case CloseError.Success:
                        break;
                    case CloseError.SqlError:
                        TrafodionDBException e = new TrafodionDBException(reply.errorDesc);
                        this.SendInfoMessage(this, e);
                        throw e;
                    case CloseError.InvalidConnection:
                    case CloseError.ParamError:
                    case CloseError.TransactionError:
                    default:
                        throw new TrafodionDBException(TrafodionDBMessage.InternalError, null, "FreeCommand", reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText);
                }
            }

            return this.Commands.Remove(cmd);
        }

        /// <summary>
        /// Creates and returns a DbCommand object associated with the current connection.
        /// </summary>
        /// <returns>A DbCommand object.</returns>
        protected override DbCommand CreateDbCommand()
        {
            return this.CreateCommand();
        }

        /// <summary>
        /// Starts a database transaction.
        /// </summary>
        /// <param name="isolationLevel">Specifies the isolation level for the transaction.</param>
        /// <returns>An object representing the new transaction.</returns>
        protected override DbTransaction BeginDbTransaction(IsolationLevel isolationLevel)
        {
            return this.BeginTransaction(isolationLevel);
        }

        private void Disconnect()
        {
            TermDialogueMessage message;
            TermDialogueReply reply;

            message = new TermDialogueMessage();

            int cmdNum = this.Commands.Count;

            while (cmdNum > 0)
            {
                this.Commands[--cmdNum].Dispose();
            }
            reply = this.Network.TermDiag(message);

            // in theory you could get a transaction in progress error from TermDiag
            // we will fail silently and close the connection, the server will cleanup anything leftover
            // TODO: maybe this should be sent as a warning?
            this.Network.CloseIO();
        }

        private GetObjRefReply GetObjRef()
        {
            GetObjRefMessage message;
            GetObjRefReply reply = null;
            bool success = false;
            short retryCount = this.ConnectionStringBuilder.RetryCount;
            int retryTime = this.ConnectionStringBuilder.RetryTime;

            message = new GetObjRefMessage()
            {
                ConnectionContext = this._cc,
                UserDescription = this._ud,
                ServerType = 2,
                RetryCount = retryCount,
                ClientUsername = Environment.UserName,
            };
            Exception retryEx = null;
            for (int i = 0; !success && i < retryCount; i++)
            {
                try
                {
                    reply = this.Network.GetObjRef(this.RemoteAddress, this.RemotePort, message);

                    switch (reply.error)
                    {
                        case GetObjRefError.Success:
                            success = true;
                            break;
                        case GetObjRefError.ASNoSrvrHdl:
                            throw new TrafodionDBException(TrafodionDBMessage.NoServerHandle);
                        case GetObjRefError.DSNotAvailable:
                            throw new TrafodionDBException(TrafodionDBMessage.DSNotAvailable);
                        case GetObjRefError.PortNotAvailable:
                            throw new TrafodionDBException(TrafodionDBMessage.PortNotAvailable);
                        case GetObjRefError.ASTryAgain:

                            // dont sleep the last time
                            if (i < retryCount - 1)
                            {
                                Thread.Sleep(retryTime);
                            }

                            break;

                        case GetObjRefError.ASNotAvailable:
                        case GetObjRefError.ASParamError:
                        case GetObjRefError.LogonUserFailure:
                        case GetObjRefError.InvalidUser:
                        case GetObjRefError.ASTimeout:
                        default:
                            throw new TrafodionDBException(TrafodionDBMessage.InternalError, "GetObjRef", reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText);
                    }
                }
                catch (Exception e)
                {
                    retryEx = e;
                    SocketException se = null;
                    if (typeof(SocketException) == e.GetType())
                    {
                        se = (SocketException)e;
                    }else if (e.InnerException != null && typeof(SocketException) == e.InnerException.GetType())
                    {
                        se = (SocketException)e.InnerException;
                    }
                    if (se!=null && se.ErrorCode == 10061 && i < retryCount)
                    {
                        Thread.Sleep(retryTime);
                        continue;
                    }
                }

            }

            // we looped <retryCount> times and still failed
            if (!success)
            {
                throw new TrafodionDBException(TrafodionDBMessage.TryAgain, retryEx!=null?retryEx.Message:null);
            }

            // backup connection info
            this._asAddress = this.RemoteAddress;
            this._asPort = this.RemotePort;

            // TCP:\<{IP Address|Machine Name}>.<Process Name>/<port>:NonStopODBC

            if (this.ByteOrder == ByteOrder.BigEndian)
            {
                string[] parts = reply.serverObjRef.Split(new char[] { '\\', ',', '/', ':' });
                this.RemoteProcess = parts[2];
                this.RemoteAddress = parts[3];
                this.RemotePort = Int32.Parse(parts[4]);
            }
            else
            {
                //int colonIndex;
                string[] parts = reply.serverObjRef.Split(new char[] { ',','/' });
                this.RemoteProcess = parts[0];
                this.RemoteAddress = parts[1];
                this.RemotePort = Int32.Parse(parts[2].Remove(parts[2].IndexOf(':')));
            }

            this.ServerVersionList = reply.serverVersion;

            this._datasource = reply.datasource;

            if (reply.datasource != this._cc.Datasource)
            {
                this.SendInfoMessage(this, new TrafodionDBException(TrafodionDBMessage.ConnectedToDefaultDS, null, reply.datasource));
            }

            this.Network.DialogueId = reply.dialogueId;
            this._cc.CtxDataLang = 0;
            this._cc.CtxErrorLang = 0;

            return reply;
        }

        private InitDialogueReply InitDiag(bool downloadCertificate)
        {
            InitDialogueMessage message;
            InitDialogueReply reply=null;
            TrafodionDBException e;
            bool success = false;
            short retryCount = this.ConnectionStringBuilder.RetryCount;

            string sessionName = this.ConnectionStringBuilder.SessionName;
            if (sessionName != null && sessionName.Length > 0)
            {
                this._cc.InContextOptions1 |= ConnectionContextOptions1.SessionName;
            }

            message = new InitDialogueMessage()
            {
                UserDescription = this._ud,
                ConnectionContext = this._cc,
                OptionFlags1 = this._cc.InContextOptions1,
                OptionFlags2 = this._cc.InContextOptions2,
                SessionName = sessionName,
                ClientUserName = Environment.UserName
            };
            for (int i = 0; !success && i < retryCount; i++)
            {
                reply = this.Network.InitDiag(this.RemoteAddress, this.RemotePort, message);

                switch (reply.error)
                {
                    case InitDialogueError.Success:
                        success = true;
                        Version msdbVersion = reply.outConnectionContext.serverVersion[1];
                        string major = (msdbVersion.MajorVersion < 10 ? "0" : "") + msdbVersion.MajorVersion;
                        string minor = (msdbVersion.MinorVersion < 10 ? "0" : "") + msdbVersion.MinorVersion;
                        string buildid = msdbVersion.BuildId + "";
                        for (int j = buildid.Length; j < 4; j++ )
                        {
                            buildid = "0" + buildid;
                        }
                        this._serverVersion = major + "." + minor + "." + buildid;
                        break;
                    case InitDialogueError.InvalidUser:
                        if (reply.outConnectionContext.certificate == null)
                        {
                            e = new TrafodionDBException(reply.errorDesc);
                            this.SendInfoMessage(this, e);
                            throw e;
                        }

                        break;
                    case InitDialogueError.SQLError:
                        e = new TrafodionDBException(reply.errorDesc);
                        this.SendInfoMessage(this, e);
                        throw e;

                    case InitDialogueError.InvalidConnection:
                    case InitDialogueError.ParamError:
                    case InitDialogueError.SQLInvalidHandle:
                    case InitDialogueError.SQLNeedData:
                    default:
                        throw new TrafodionDBException(TrafodionDBMessage.InternalError, null, "InitDiag", reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText);
                }
            }

            return reply;
        }

        private void ParseServerString(string server)
        {
            int idx = server.IndexOf(':');

            if (idx == -1)
            {
                this.RemotePort = 18650;
                this.RemoteAddress = server;
            }
            else
            {
                this.RemotePort = Int32.Parse(server.Substring(idx + 1));
                this.RemoteAddress = server.Substring(0, idx);
            }
        }

        private void CreateConnectionContext()
        {
            TrafodionDBConnectionStringBuilder b = this.ConnectionStringBuilder;

            this._cc = new ConnectionContext()
            {
                // properties
                Datasource = b.Datasource,
                Catalog = b.Catalog,
                Schema = b.Schema,
                UserRole = b.Rolename,
                ComputerName = Environment.MachineName,
                ClientVproc = ProductVersion.Vproc,

                // TODO: these need to be error checked
                CpuToUse = b.CpuToUse,
                CpuToUseEnd = -1,
                RowSetSize = b.FetchBufferSize,

                // timeouts
                IdleTimeoutSec = b.IdleTimeout,
                LoginTimeoutSec = b.LoginTimeout * 1000,
                QueryTimeoutSec = 0,

                // hard coded
                AccessMode = 0, // read only
                AutoCommit = 1, // auto commit
                CtxACP = 1252,
                CtxCtrlInferNCHAR = -1,
                CtxDataLang = 15, // charset
                CtxErrorLang = 15, // charset

                // unused
                Location = string.Empty,
                ConnectOptions = string.Empty,
                DiagnosticFlag = 0,

                InContextOptions1 = ConnectionContextOptions1.ClientUsername | ConnectionContextOptions1.FetchAhead,
                InContextOptions2 = 0,

                ProcessId = Process.GetCurrentProcess().Id,

                TxnIsolationLevel = (int)TransactionIsolation.ReadCommmited,

                WindowText = b.ApplicationName,

                ClientVersion = this.CreateVersionList()
            };
        }

        private void CreateUserDescription()
        {
            this._ud = new UserDescription()
            {
                UserDescType = UserDescType.UNAUTHENTICATED_USER_TYPE,
                UserName = this.ConnectionStringBuilder.User,
                Password = new byte[0],
                DomainName = string.Empty,
                UserSid = new byte[0]
            };
        }

        private Version[] CreateVersionList()
        {
            Version[] v = new Version[2];
            v[0] = new Version()
            {
                ComponentId = 20, // this is actually JDBC
                MajorVersion = ProductVersion.Major,
                MinorVersion = ProductVersion.Minor,
                BuildId = BuildOptions.RowwiseRowset | BuildOptions.Charset | BuildOptions.PasswordSecurity
            };
            v[1] = new Version()
            {
                ComponentId = 8,
                MajorVersion = 3,
                MinorVersion = 0,
                BuildId = 0
            };

            // TODO: delayed error mode should be set here
            return v;
        }

        private byte[] EncodePassword(string pass)
        {
            byte[] b = System.Text.ASCIIEncoding.ASCII.GetBytes(pass);
            byte[] key = System.Text.ASCIIEncoding.UTF8.GetBytes("ci4mg04-3;" +
                "b,hl;y'd1q" + "x8ngp93nGp" + "oOp4HlD7vm" +
                ">o(fHoPdkd" + "khp1`gl0hg" + "qERIFdlIFl" +
                "w48fgljksg" + "3oi5980rfd" + "4t8u9dfvkl");

            for (int i = 0; i < b.Length; ++i)
            {
                int j = i % 100;
                b[i] ^= key[j];
            }

            return b;
        }

        // this function was taken from JDBC -- needs to be rewritten!
        private void SecureLogin(GetObjRefReply objRefReply)
        {
            InitDialogueReply initReply = null;

            string remoteHost = this.RemoteProcess.Substring(0, this.RemoteProcess.IndexOf('.'));
            byte[] passwordBytes = ASCIIEncoding.ASCII.GetBytes(this.ConnectionStringBuilder.Password);
            byte[] roleBytes = ASCIIEncoding.ASCII.GetBytes(this.ConnectionStringBuilder.Rolename);

            try
            {
                this._security = new TrafodionDBSecurity(
                    this,
                    null,
                    null,
                    objRefReply.cluster,
                    objRefReply.processId,
                    objRefReply.serverNode,
                    objRefReply.timestamp);
            }
            catch (Exception e)
            {
                this.CleanupServer(); // MXOSRVR is expecting InitDiag, clean it up since we failed
                throw e;
            }

            try
            {
                this._security.OpenCertificate();
                this._security.EncryptPassword(passwordBytes, roleBytes, out this._ud.Password);
            }
            catch
            {
                this.DownloadCertificate(); // otherwise, download and continue

                this._ud.UserName = this.ConnectionStringBuilder.User;
                this._security.EncryptPassword(passwordBytes, roleBytes, out this._ud.Password);
            }

            this._cc.ConnectOptions = this._security.GetCertExpDate();
            initReply = this.InitDiag(false);

            // error but no exception means we redownload cert
            if (initReply.error == InitDialogueError.InvalidUser)
            {
                if (initReply.outConnectionContext != null || initReply.outConnectionContext.certificate != null)
                {
                    // we got a certificate back, switch to it, continue
                    this._security.SwitchCertificate(initReply.outConnectionContext.certificate);
                }

                this._cc.ConnectOptions = this._security.GetCertExpDate();
                this._security.EncryptPassword(passwordBytes, roleBytes, out this._ud.Password);
                this.InitDiag(false); // re-initdiag
            }
        }

        private void CleanupServer()
        {
            this._ud.UserName = null;
            this._ud.Password = null;

            try
            {
                this.InitDiag(false); // send dummy init diag to clean up server
            }
            catch
            {
            }
        }

        private void DownloadCertificate()
        {
            InitDialogueReply reply = null;

            this._ud.UserName = string.Empty;
            this._ud.Password = new byte[0];
            this._cc.ConnectOptions = string.Empty;

            reply = this.InitDiag(true);

            if (reply.outConnectionContext == null || reply.outConnectionContext.certificate == null)
            {
                throw new Exception("certifcate was null during download");
            }

            this._security.SwitchCertificate(reply.outConnectionContext.certificate);
        }

        private void CopyProperties(TrafodionDBConnection conn)
        {
            this._asAddress = conn._asAddress;
            this._asPort = conn._asPort;
            this._cc = conn._cc;
            this._database = conn._database;
            this._datasource = conn._datasource;
            this._labelId = conn._labelId;
            this._labelLock = conn._labelLock;
            this._metaData = conn._metaData;
            this._security = conn._security;
            this._serverVersion = conn._serverVersion;
            this._state = ConnectionState.Open;
            this._ud = conn._ud;

            this.RemoteAddress = conn.RemoteAddress;
            this.RemotePort = conn.RemotePort;
            this.RemoteProcess = conn.RemoteProcess;
            this.Network = conn.Network;
        }

        internal void ResetIdleTimeout()
        {
            SetConnectionOptMessage message;

            message = new SetConnectionOptMessage()
            {
                Option = ConnectionOption.ResetIdleTimer,
                IntValue = 0,
                StringValue = string.Empty
            };

            this.Network.SetConnectionOpt(message); //ignore reply, just generate exception if the network is dead
        }

        public override int ConnectionTimeout
        {
            get
            {
                return this.ConnectionStringBuilder.LoginTimeout;
            }
        }
    }
}
