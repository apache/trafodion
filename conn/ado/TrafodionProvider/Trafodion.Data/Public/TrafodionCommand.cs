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
    using System.Data;
    using System.Data.Common;
    using System.Text;
    using System.Threading;
    using System.Collections.Generic;

    /// <summary>
    /// Represents a SQL statement or stored procedure to execute against a TrafodionDB database. This class cannot be inherited.
    /// </summary>
    public sealed class TrafodionDBCommand : DbCommand, IDbCommand, ICloneable
    {
        private TrafodionDBConnection _conn;
        private TrafodionDBTransaction _trans;
        private TrafodionDBParameterCollection _parameters;
        private CommandType _type;

        private PrepareReply _prepareReply; // we want to keep this around for execute
        private string _commandText;
        private int _inputRowLength = 0;
        private long _rowsAffected = 0;
        private int _handle = 0;
        private QueryType _queryType;
        private bool _isEmptyInsert; //for rwrs insert empty row

        private TrafodionDBNetwork _network;
        private TrafodionDBDataReader _reader;
        private CommandBehavior _behavior;
        private StatementType _statementType;

        private bool _isPrepared;

        private bool _hasPrepareDescriptors;

        private List<Object []> _batchedParams;

        private int _commandTimeout = 0;
        private bool _commandTimeoutUserDefined = false;

        /// <summary>
        /// Initializes a new instance of the TrafodionDBCommand class.
        /// </summary>
        public TrafodionDBCommand()
            : this(string.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBCommand class with the text of the query.
        /// </summary>
        /// <param name="cmdText">The text of the query.</param>
        public TrafodionDBCommand(string cmdText)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Public, cmdText);
            }

            this._parameters = new TrafodionDBParameterCollection(this);

            this._type = CommandType.Text;
            this.CommandText = cmdText;
            this._hasPrepareDescriptors = false;
            this._isPrepared = false;
            this._batchedParams = new List<object[]>();
            this._isEmptyInsert = false;
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBCommand class with the text of the query and a TrafodionDBConnection.
        /// </summary>
        /// <param name="cmdText">The text of the query.</param>
        /// <param name="connection">A TrafodionDBConnection to associate with this command.</param>
        public TrafodionDBCommand(string cmdText, TrafodionDBConnection connection)
            : this(cmdText, connection, connection.Transaction)
        {
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBCommand class with the text of the query, a TrafodionDBConnection, and the TrafodionDBTransaction.
        /// </summary>
        /// <param name="cmdText">The text of the query. </param>
        /// <param name="connection">A TrafodionDBConnection to associate with this command.</param>
        /// <param name="transaction">Not used.  The TrafodionDBTransaction associated with the Command will always be referenced from the TrafodionDBConnection object.</param>
        public TrafodionDBCommand(string cmdText, TrafodionDBConnection connection, TrafodionDBTransaction transaction)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(connection, TraceLevel.Public, cmdText, transaction);
            }

            this._parameters = new TrafodionDBParameterCollection(this);

            this.Connection = connection;
            this._type = CommandType.Text;

            this.CommandText = cmdText;
            this.Transaction = transaction;

            this._hasPrepareDescriptors = false;
            this._isPrepared = false;
            this._batchedParams = new List<object[]>();
            this._isEmptyInsert = false;

            if (this._conn != null)
            {
                this._network = this._conn.Network;
                this.Label = this._conn.GenerateStatementLabel();
            }
        }

        /// <summary>
        /// Initializes a new instance of the TrafodionDBCommand class with the text of the query, a TrafodionDBConnection, a TrafodionDBTransaction, and a command label.
        /// </summary>
        /// <param name="cmdText">The text of the query. </param>
        /// <param name="connection">A TrafodionDBConnection to associate with this command.</param>
        /// <param name="transaction">Not used.  The TrafodionDBTransaction associated with the Command will always be referenced from the TrafodionDBConnection object.</param>
        /// <param name="label">The label to be associated with the TrafodionDBCommand.</param>
        public TrafodionDBCommand(string cmdText, TrafodionDBConnection connection, TrafodionDBTransaction transaction, string label)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(connection, TraceLevel.Public, cmdText);
            }

            this._parameters = new TrafodionDBParameterCollection(this);

            this.Connection = connection;
            this._type = CommandType.Text;

            this.CommandText = cmdText;
            this.Transaction = transaction;

            this._hasPrepareDescriptors = false;
            this._isPrepared = false;
            this._batchedParams = new List<object[]>();
            this._isEmptyInsert = false;

            this.Label = label;

            if (this._conn != null)
            {
                this._network = this._conn.Network;
            }
        }

        /// <summary>
        /// Gets or sets the SQL statement to execute.
        /// </summary>
        public override string CommandText
        {
            get
            {
                return this._commandText;
            }

            set
            {
                this.Reset();
                this._commandText = value;
            }
        }

        /// <summary>
        /// Gets or sets the wait time before terminating the attempt to execute a command and generating an error.
        /// </summary>
        public override int CommandTimeout
        {
            get
            {
                if (!this._commandTimeoutUserDefined && this._conn != null)
                {
                    return this._conn.ConnectionStringBuilder.CommandTimeout;
                }

                return this._commandTimeout;
            }
            set
            {
                this._commandTimeout = value;
                this._commandTimeoutUserDefined = true;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating how the CommandText property is to be interpreted.
        /// </summary>
        public override CommandType CommandType
        {
            get
            {
                return this._type;
            }

            set
            {
                if (value != CommandType.Text && value != CommandType.StoredProcedure)
                {
                    throw new TrafodionDBException(TrafodionDBMessage.UnsupportedCommandType, null, value.ToString());
                }

                this._type = value;
            }
        }

        /// <summary>
        /// Gets or sets the TrafodionDBConnection used by this instance of the TrafodionDBCommand.
        /// </summary>
        public new TrafodionDBConnection Connection
        {
            get
            {
                return this._conn;
            }

            set
            {
                // only take action if we are getting an updated value
                if (this._conn != value)
                {
                    if (this._conn != null)
                    {
                        // cleanup the old connection
                        this._conn.RemoveCommand(this);
                    }

                    if (value != null)
                    {
                        this._conn = value; // switch to the new connection
                        this._conn.Commands.Add(this);
                        this._network = this._conn.Network; // switch the network layer to use
                        this.Label = this._conn.GenerateStatementLabel();
                    }
                    else
                    {
                        this._conn = null;
                        this._network = null;
                    }

                    this.Reset(); // reset the command so it can be reused
                }
            }
        }

        /// <summary>
        /// Gets the TrafodionDBParameterCollection.
        /// </summary>
        public new TrafodionDBParameterCollection Parameters
        {
            get
            {
                return this._parameters;
            }
        }

        /// <summary>
        /// Gets or sets the TrafodionDBTransaction associated with this command.
        /// </summary>
        /// <remarks>
        /// Not used.  The TrafodionDBTransaction associated with the Command will always be referenced from the TrafodionDBConnection object.
        /// </remarks>
        public new TrafodionDBTransaction Transaction
        {
            get
            {
                return this._trans;
            }

            set
            {
                this._trans = value;
            }
        }

        /// <summary>
        /// Gets or sets how command results are applied to the DataRow when used by the Update method of the TrafodionDBDataAdapter.
        /// </summary>
        public override UpdateRowSource UpdatedRowSource { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the command object should be visible in a Windows Form Designer control.
        /// </summary>
        public override bool DesignTimeVisible { get; set; }

        /// <summary>
        /// Gets the Statement Label associated with this command.
        /// </summary>
        public string Label { get; internal set; }

        /// <summary>
        /// Gets the number of rows changed, inserted, or deleted.
        /// </summary>
        public long RowsAffected
        {
            get
            {
                return this._rowsAffected;
            }
        }

        /// <summary>
        /// Gets or sets the TrafodionDBConnection used by this instance of the TrafodionDBCommand.
        /// </summary>
        IDbConnection IDbCommand.Connection
        {
            get
            {
                return this.Connection;
            }

            set
            {
                this.Connection = (TrafodionDBConnection)value;
            }
        }

        /// <summary>
        /// Gets the TrafodionDBParameterCollection.
        /// </summary>
        IDataParameterCollection IDbCommand.Parameters
        {
            get { return this.Parameters; }
        }

        /// <summary>
        /// Gets or sets the TrafodionDBTransaction within which the TrafodionDBCommand executes.
        /// </summary>
        IDbTransaction IDbCommand.Transaction
        {
            get
            {
                return this.Transaction;
            }

            set
            {
                this.Transaction = (TrafodionDBTransaction)value;
            }
        }

        internal TrafodionDBDataReader DataReader
        {
            get
            {
                return this._reader;
            }
        }

        internal TrafodionDBNetwork Network
        {
            get
            {
                return this._network;
            }

            set
            {
                this._network = value;
            }
        }

        internal CommandBehavior CommandBehavior
        {
            get
            {
                return this._behavior;
            }
        }

        internal int Handle
        {
            get
            {
                return this._handle;
            }
        }

        internal bool IsPrepared
        {
            get { return this._isPrepared; }
            set { this._isPrepared = value; }
        }

        internal ByteOrder ByteOrder
        {
            get
            {
                return this._conn.ByteOrder;
            }
        }

        /// <summary>
        /// Gets or sets the DbConnection used by this DbCommand.
        /// </summary>
        protected override DbConnection DbConnection
        {
            get
            {
                return this.Connection;
            }

            set
            {
                this.Connection = (TrafodionDBConnection)value;
            }
        }

        /// <summary>
        /// Gets the collection of DbParameter objects.
        /// </summary>
        protected override DbParameterCollection DbParameterCollection
        {
            get
            {
                return this.Parameters;
            }
        }

        /// <summary>
        /// Gets or sets the DbTransaction within which this DbCommand object executes.
        /// </summary>
        protected override DbTransaction DbTransaction
        {
            get
            {
                return this.Transaction;
            }

            set
            {
                this.Transaction = (TrafodionDBTransaction)value;
            }
        }

        private bool HasParameters
        {
            get
            {
                bool foundParam = false;

                string[] s = TrafodionDBUtility.RemoveStringLiterals.Split(this.CommandText);
                for (int i = 0; i < s.Length; i++)
                {
                    if (s[i].IndexOf('?') != -1)
                    {
                        foundParam = true;
                        break;
                    }
                }

                return foundParam;
            }
        }

        public bool isRWRS
        {
            get
            {
                if (this._queryType == QueryType.InsertRWRS)
                {
                    return true;
                }
                return false;
            }

        }
        private bool isRWRSLoad
        {
            get
            {
                if (TrafodionDBUtility.FindRowwiseUserLoad.Match(this.CommandText.ToLower()).Success)
                {
                    return true;
                }
                return false;
            }
        }

        private int MaxRowSize
        {
            get
            {
                int value = 0;
                if (this.CommandText.ToLower().IndexOf("max rowset size") != -1)
                {
                    //Convert.ToInt32(TrafodionDBUtility.FindRowSize.Match(this.CommandText).Value);
                    value = 1;
                }

                return value;
            }
        }
        /// <summary>
        /// Releases all resources used by the TrafodionDBCommand.
        /// </summary>
        public new void Dispose()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this.Connection, TraceLevel.Public);
            }

            GC.SuppressFinalize(this);
            // make sure the connection frees this label
            if (this._conn != null)
            {
                this.Reset();
                this._conn.RemoveCommand(this);
            }
            base.Dispose();
        }

        /// <summary>
        /// Tries to cancel the execution of a TrafodionDBCommand.
        /// </summary>
        public override void Cancel()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(null, TraceLevel.Public);
            }

            this.Connection.Cancel();
        }

        // TODO: add overloaded parameter functions

        /// <summary>
        /// Creates a new instance of a TrafodionDBParameter object.
        /// </summary>
        /// <returns>A TrafodionDBParameter object.</returns>
        public new TrafodionDBParameter CreateParameter()
        {
            return new TrafodionDBParameter();
        }

        /// <summary>
        /// Sends the CommandText to the Connection and builds a TrafodionDBDataReader.
        /// </summary>
        /// <returns>A TrafodionDBDataReader object.</returns>
        public new TrafodionDBDataReader ExecuteReader()
        {
            return this.ExecuteReader(CommandBehavior.Default);
        }

        /// <summary>
        /// Sends the CommandText to the Connection, and builds a TrafodionDBDataReader using one of the CommandBehavior values.
        /// </summary>
        /// <param name="behavior">One of the CommandBehavior values.</param>
        /// <returns>A TrafodionDBDataReader object.</returns>
        public new TrafodionDBDataReader ExecuteReader(CommandBehavior behavior)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Public, behavior);
            }

            this._behavior = behavior;

            this.Execute();

            //if they called ExecuteReader on a statement with no results, generate a dummy reader
            if (this._reader == null)
            {
                this._reader = new TrafodionDBDataReader(this);
            }

            return this._reader;
        }

        public override int ExecuteNonQuery()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Public,this._commandText);
            }

            this.Execute();

            return (int)this._rowsAffected;
        }

        /// <summary>
        /// Executes the query, and returns the first column of the first row in the result set returned by the query. Additional columns or rows are ignored.
        /// </summary>
        /// <returns>The first column of the first row in the result set or a null reference if the result set is empty.</returns>
        public override object ExecuteScalar()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Public);
            }

            object ret;

            using (TrafodionDBDataReader dr = this.ExecuteReader())
            {
                dr.FetchSize = 1;

                ret = this._reader.Read() ? this._reader.GetValue(0) : null;
            }

            // we dont want this reader saved on the object
            this._reader = null;

            return ret;
        }

        /// <summary>
        /// Creates a prepared version of the command on the TrafodionDB Server.
        /// </summary>
        public override void Prepare()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._conn, TraceLevel.Public);
            }
            PrepareMessage message;
            TrafodionDBException ex;

            if (this._statementType == StatementType.Unknown)
            {
                this.UpdateStatementType();
            }
            if (!this._isPrepared)
            {
                message = new PrepareMessage()
                {
                    AsyncEnable = 0,
                    QueryTimeout = this.CommandTimeout * 1000,
                    StmtType = 0,
                    SqlStmtType = this._statementType,
                    Label = this.Label,
                    LabelCharset = 1,
                    CursorName = string.Empty,
                    CursorNameCharset = 1,
                    ModuleName = string.Empty,
                    ModuleNameCharset = 1,
                    ModuleTimestamp = 0,
                    SqlText = this.CommandText,
                    SqlTextCharset = 1,
                    StmtOptions = string.Empty,
                    ExplainLabel = string.Empty,
                    MaxRowsetSize = this.MaxRowSize,
                    TransactionId = new byte[4]
                };
                lock (this._conn.dataAccessLock)
                {
                    this._prepareReply = this._network.Prepare(message, this._commandTimeoutUserDefined ? this._commandTimeout : this._conn.ConnectionStringBuilder.CommandTimeout);
                }
                if (this._prepareReply.returnCode == ReturnCode.SuccessWithInfo)
                {
                    ex = new TrafodionDBException(this._prepareReply.errorList);
                    this.Connection.SendInfoMessage(this, ex);
                }
                else if (this._prepareReply.returnCode != ReturnCode.Success)
                {
                    ex = new TrafodionDBException(this._prepareReply.errorList);
                    this.Connection.SendInfoMessage(this, ex);
                    throw ex;
                }

                this._queryType = this._prepareReply.queryType;
                this._parameters.Prepare(this._prepareReply.inputDesc);
                if (this.isRWRS)
                {
                    this.alignRWRSOffsets();
                }
                else
                {
                    this._inputRowLength = this._prepareReply.inputRowLength;
                }
                this._isPrepared = true;
                this._hasPrepareDescriptors = this._prepareReply.outputDesc.Length > 0;
                this._handle = this._prepareReply.stmtHandle;
            }
        }

        /// <summary>
        /// Creates a new instance of a TrafodionDBParameter object.
        /// </summary>
        /// <returns>A TrafodionDBParameter object.</returns>
        IDbDataParameter IDbCommand.CreateParameter()
        {
            return this.CreateParameter();
        }

        /// <summary>
        /// Sends the CommandText to the Connection, and builds a TrafodionDBDataReader using one of the CommandBehavior values.
        /// </summary>
        /// <param name="behavior">One of the CommandBehavior values. </param>
        /// <returns>A SqlDataReader object.</returns>
        IDataReader IDbCommand.ExecuteReader(CommandBehavior behavior)
        {
            return this.ExecuteReader(behavior);
        }

        /// <summary>
        /// Sends the CommandText to the Connection and builds a TrafodionDBDataReader.
        /// </summary>
        /// <returns>TrafodionDBDataReader</returns>
        IDataReader IDbCommand.ExecuteReader()
        {
            return this.ExecuteReader();
        }

        /// <summary>
        /// Creates a new TrafodionDBCommand object that is a copy of the current instance.
        /// </summary>
        /// <returns>A TrafodionDBCommand object.</returns>
        object ICloneable.Clone()
        {
            TrafodionDBCommand cloneCommand = new TrafodionDBCommand(this._commandText, this._conn, this._trans);
            cloneCommand.CommandType = this.CommandType;
            cloneCommand.CommandTimeout = this.CommandTimeout;
            cloneCommand.Label = this.Label;
            cloneCommand.IsPrepared = this.IsPrepared;

            foreach (TrafodionDBParameter parameter in Parameters)
            {
                cloneCommand.Parameters.Add((parameter as ICloneable).Clone()); //clone parameter
            }

            return cloneCommand;
        }

        /// <summary>
        /// Creates a new instance of a DbParameter object.
        /// </summary>
        /// <returns>A DbParameter object.</returns>
        protected override DbParameter CreateDbParameter()
        {
            return this.CreateParameter();
        }

        /// <summary>
        /// Executes the command text against the connection.
        /// </summary>
        /// <param name="behavior">An instance of CommandBehavior.</param>
        /// <returns>A DbDataReader.</returns>
        protected override DbDataReader ExecuteDbDataReader(CommandBehavior behavior)
        {
            return this.ExecuteReader(behavior);
        }

        internal void Reset()
        {
            // this._parameters.Clear();
            this._isPrepared = false;
            this._hasPrepareDescriptors = false;

            this._rowsAffected = 0;
            this._handle = 0;
        }

        private void Execute()
        {
            ExecuteMessage message;
            ExecuteReply reply;
            TrafodionDBException ex;
            OperationId id;
            byte[] buf;
            int rowCount;
            Timer timer = null;

            // compatibilty for Visual Studio that sometimes uses [ ] as delimiters without checking the DataSourceInformation
            // this is apparently a bug in VS and other vendors have had to work around this
            //  http://bugs.mysql.com/bug.php?id=35852
            if (this._conn.ConnectionStringBuilder.SqlServerMode)
            {
                this.CommandText = TrafodionDBUtility.ConvertBracketIdentifiers(this.CommandText);
            }

            if (this._reader != null)
            {
                this._reader.Close();
                this._reader = null;
            }

            if (this.HasParameters && this.Parameters.Count == 0)
            {
                throw new Exception("parameter count does not match prepared statement");
            }

            // check to see if they changed any of the official param information
            if (this._isPrepared)
            {
                for (int i = 0; i < this._parameters.Count; i++)
                {
                    if (!this._parameters[i].Verified)
                    {
                        this._isPrepared = false;
                        break;
                    }
                }
            }

            if (!this._isPrepared && this.Parameters.Count > 0)
            {
                this.Prepare();
            }

            id = this._isPrepared ? OperationId.SRVR_API_SQLEXECUTE2 : OperationId.SRVR_API_SQLEXECDIRECT;

            if (this._parameters != null && this._parameters.Count > 0 && !this._isEmptyInsert)
            {
                if (this._batchedParams.Count == 0)
                {
                    this.AddBatch();
                }

                rowCount = this._batchedParams.Count;
                buf = this.CreateDataBuffer();
            }
            else
            {
                rowCount = 0;
                buf = new byte[0];
            }

            this.ClearBatch();
            this.UpdateStatementType();

            message = new ExecuteMessage()
            {
                AsyncEnable = 0,
                QueryTimeout = this.CommandTimeout * 1000,
                InputRowCount = rowCount,
                MaxRowsetSize = this.isRWRS?this._inputRowLength:this.MaxRowSize,
                SqlStmtType = this._statementType,
                StmtHandle = this._handle,
                StmtType = 0,
                SqlText = this.CommandText,
                SqlTextCharset = 1,
                CursorName = string.Empty,
                CursorNameCharset = 1,
                Label = this.Label,
                LabelCharset = 1,
                ExplainLabel = string.Empty,
                Data = buf,
                TransactionId = new byte[0]
            };

            //if (this.CommandTimeout > 0)
            //{
            //    TimerCallback timerDelegate = new TimerCallback(QueryTimeOutEvent);
            //    timer = new Timer(timerDelegate, this, this.CommandTimeout * 1000, Timeout.Infinite);
            //}
            lock (this._conn.dataAccessLock)
            {
                reply = this._network.Execute(message, id, this._commandTimeoutUserDefined ? this._commandTimeout : this._conn.ConnectionStringBuilder.CommandTimeout);
            }
            //if (timer != null)
            //{
            //    timer.Dispose();
            //}

            switch (reply.returnCode)
            {
                case ReturnCode.Success:
                case ReturnCode.NoDataFound:
                    // Insert Empty Row for RWRS insert using user load
                    if (this.isRWRS && this.isRWRSLoad && this.isEmptyInsert == false)
                    {
                        this.isEmptyInsert = true;
                        this.Execute();
                    }
                    break;
                case ReturnCode.SuccessWithInfo:
                    ex = new TrafodionDBException(reply.errorList);
                    this.Connection.SendInfoMessage(this, ex);
                    break;
                default:
                    ex = new TrafodionDBException(reply.errorList);
                    this.Connection.SendInfoMessage(this, ex);
                    throw ex;
            }

            // if INSERT UPDATE or DELETE
            QueryType qt = this._isPrepared ? this._prepareReply.queryType : reply.queryType;
            if ((qt >= QueryType.InsertUnique && qt <= QueryType.DeleteNonUnique) || qt == QueryType.InsertRWRS)
            {
                this._rowsAffected = reply.rowsAffected;
            }
            else
            {
                this._rowsAffected = -1;
            }

            // set output params
            // i dislike using the a DataReader object here as we have to cheat close it for the statement to remain valid
            if ((qt == QueryType.CallNoResults || qt == QueryType.CallWithResults) && reply.data != null && reply.data.Length > 0)
            {
                ExecuteReply paramReply = new ExecuteReply()
                {
                    returnCode = reply.returnCode,
                    data = reply.data,
                    outputDesc = new Descriptor[1][] { this._prepareReply.outputDesc }
                };

                this._reader = new TrafodionDBDataReader(this, null, paramReply);
                this._reader.Read();

                for (int i = 0; i < this._parameters.Count; i++)
                {
                    if (this._parameters[i].Direction == ParameterDirection.Output ||
                        this._parameters[i].Direction == ParameterDirection.InputOutput)
                    {
                        this._parameters[i].Value = this._reader.GetValue(i);
                    }
                }

                // we do not want to close the DataReader normally so we can preserve the label
                this._reader.SetClosed(true);
                this._reader = null;

                reply.data = new byte[0];
            }

            // we need to check both prepare and execute for output descriptors
            if (this._hasPrepareDescriptors)
            {
                this._reader = new TrafodionDBDataReader(this, this._prepareReply, reply);
            }
            else if (reply.outputDesc.Length > 0)
            {
                this._reader = new TrafodionDBDataReader(this, null, reply);
            }

        }

        // TODO: maybe a hashtable of function pointers would clean this up a bit with separate funcs for each datatype?
        private void InsertParam(DataStream ds, Descriptor desc, object value)
        {
            string str;
            byte[] buf;
            byte[] temp;

            short s;
            ushort us;
            int i;
            uint ui;
            long l;
            double d;
            decimal dec;

            switch (desc.FsType)
            {
                case FileSystemType.Char:
                    str = Convert.ToString(value);

                    // TODO: needs to be optimized
                    buf = new byte[desc.MaxLength];
                    temp = this._conn.Network.Encoder.GetBytes(str, desc.NdcsEncoding);
                    i = temp.Length;

                    if (i > desc.MaxLength)
                    {
                        throw new Exception("data too long for char column");
                    }

                    System.Buffer.BlockCopy(temp, 0, buf, 0, i);

                    if (desc.NdcsEncoding == TrafodionDBEncoding.UCS2)
                    {
                        for (int j = i; j < buf.Length; j += 2)
                        {
                            buf[j] = 0;
                            buf[j + 1] = (byte)' ';
                        }
                    }
                    else
                    {
                        for (int j = i; j < buf.Length; j++)
                        {
                            buf[j] = (byte)' ';
                        }
                    }

                    ds.WriteBytes(buf);

                    break;
                case FileSystemType.Varchar:
                case FileSystemType.VarcharWithLength:
                    str = Convert.ToString(value);

                    buf = this._conn.Network.Encoder.GetBytes(str, desc.NdcsEncoding);
                    if (buf.Length > desc.MaxLength)
                    {
                        throw new Exception("varchar data too long");
                    }

                    ds.WriteInt16((short)buf.Length);
                    ds.WriteBytes(buf);

                    break;
                case FileSystemType.DateTime:
                    DateTime dt = Convert.ToDateTime(value);

                    switch (desc.DtCode)
                    {
                        case DateTimeCode.Date:
                            str = dt.ToString(TrafodionDBUtility.DateFormat);
                            break;
                        case DateTimeCode.Time:
                            str = dt.ToString(TrafodionDBUtility.TimeFormat[desc.Precision]);
                            break;
                        case DateTimeCode.Timestamp:
                            str = dt.ToString(TrafodionDBUtility.TimestampFormat[desc.Precision]);
                            break;
                        default:
                            throw new Exception("internal error: bad datetime");
                    }

                    if (str.Length > desc.MaxLength)
                    {
                        throw new Exception("Internal Error: datetime data too long");
                    }

                    ds.WriteBytes(System.Text.ASCIIEncoding.ASCII.GetBytes(str));

                    break;
                case FileSystemType.Interval:
                    str = Convert.ToString(value);
                    if (str.Length > desc.MaxLength)
                    {
                        throw new Exception("interval data too long");
                    }

                    ds.WriteBytes(System.Text.ASCIIEncoding.ASCII.GetBytes(str));
                    break;
                case FileSystemType.SmallInt:
                    if (desc.Scale > 0)
                    {
                        d = Convert.ToDouble(value);
                        s = (short)(d * TrafodionDBUtility.PowersOfTen[desc.Scale]);
                    }
                    else
                    {
                        s = Convert.ToInt16(value);
                    }

                    ds.WriteInt16(s);
                    break;
                case FileSystemType.SmallIntUnsigned:
                    if (desc.Scale > 0)
                    {
                        d = Convert.ToDouble(value);
                        us = (ushort)(d * TrafodionDBUtility.PowersOfTen[desc.Scale]);
                    }
                    else
                    {
                        us = Convert.ToUInt16(value);
                    }

                    ds.WriteUInt16(us);
                    break;
                case FileSystemType.Integer:
                    if (desc.Scale > 0)
                    {
                        d = Convert.ToDouble(value);
                        i = (int)(d * TrafodionDBUtility.PowersOfTen[desc.Scale]);
                    }
                    else
                    {
                        i = Convert.ToInt32(value);
                    }

                    ds.WriteInt32(i);
                    break;
                case FileSystemType.IntegerUnsigned:
                    if (desc.Scale > 0)
                    {
                        d = Convert.ToDouble(value);
                        ui = (uint)(d * TrafodionDBUtility.PowersOfTen[desc.Scale]);
                    }
                    else
                    {
                        ui = Convert.ToUInt32(value);
                    }

                    ds.WriteUInt32(ui);
                    break;
                case FileSystemType.LargeInt:
                    if (desc.Scale > 0)
                    {
                        d = Convert.ToDouble(value);
                        l = (long)(d * TrafodionDBUtility.PowersOfTen[desc.Scale]);
                    }
                    else
                    {
                        l = Convert.ToInt64(value);
                    }

                    ds.WriteInt64(l);
                    break;

                // TODO: real and double need to be optimized
                case FileSystemType.Real:
                    ds.WriteInt32(BitConverter.ToInt32(BitConverter.GetBytes(Convert.ToSingle(value)), 0));
                    break;
                case FileSystemType.Double:
                    ds.WriteInt64(BitConverter.DoubleToInt64Bits(Convert.ToDouble(value)));
                    break;

                case FileSystemType.Decimal:
                    bool neg;
                    dec = Convert.ToDecimal(value);

                    dec *= TrafodionDBUtility.PowersOfTen[desc.Scale];
                    if (dec < 0)
                    {
                        dec *= -1;
                        neg = true;
                    }
                    else
                    {
                        neg = false;
                    }

                    str = dec.ToString("0").PadLeft(desc.Precision, '0');

                    if (str.Length > desc.MaxLength)
                    {
                        throw new Exception("data too long for decimal column: " + value);
                    }

                    buf = System.Text.ASCIIEncoding.ASCII.GetBytes(str);

                    if (neg)
                    {
                        buf[0] |= (byte)0x80;
                    }

                    ds.WriteBytes(buf);
                    break;
                case FileSystemType.Numeric:
                case FileSystemType.NumericUnsigned:
                    str = value.ToString();

                    // check for valid characters
                    if (!TrafodionDBUtility.ValidateNumeric.IsMatch(str))
                    {
                        throw new Exception("invalid numeric string: " + str);
                    }

                    // we need to unscale the value -- messy as we have to keep it as a string
                    i = str.IndexOf('.');
                    if (i > 0)
                    {
                        //TODO: we need some way of checking numeric overflow here
                        if(desc.Scale < str.Length - (i + 1))
                        {
                            //make sure the length of decimal less than Scale
                            str.Remove(i + desc.Scale);
                        }
                        str = str.Replace(".", "");

                        //make sure the string length less than the MaxLength
                        if (desc.MaxLength < str.Length)
                        {
                            str.Remove(desc.MaxLength);
                        }

                        str = str.PadRight(str.Length + desc.Scale - (str.Length - i), '0');
                    }
                    else
                    {
                        str = str.PadRight(str.Length + desc.Scale, '0');
                    }

                    ds.WriteBytes(this.ConvertStringToBigNum(str, desc.MaxLength, desc.Scale));
                    break;

                default:
                    throw new NotSupportedException("Internal Error: Invalid FSType " + desc.FsType);
            }
        }

        private byte[] CreateDataBuffer()
        {
            DataStream ds;
            byte[] data;
            object value;
            Descriptor desc;
            int dataLength;
            int dataTotalLength;
            int offset = 0;

            int batchCount = this._batchedParams.Count;
            int paramCount = this._parameters.Count;

            data = new byte[this._inputRowLength * batchCount];
            ds = new DataStream(data, this.ByteOrder);

            for (int i = 0; i < batchCount; i++)
            {
                for (int j = 0; j < paramCount; j++)
                {
                    value = this._batchedParams[i][j];
                    desc = this._parameters[j].Descriptor;

                    if (value == null || value == DBNull.Value)
                    {
                        if(!desc.Nullable)
                        {
                            throw new Exception("cannot assign null to non nullable column");
                        }

                        ds.Position = (desc.NullOffset * batchCount) + (2 * i);
                        ds.WriteInt16((short)-1);
                    }
                    else
                    {
                        dataLength = desc.MaxLength;

                        if (desc.TrafodionDBDataType == TrafodionDBDbType.Varchar ||
                            desc.TrafodionDBDataType == TrafodionDBDbType.NVarchar)
                        {
                            dataLength += 2;

                            if (dataLength % 2 != 0)
                                dataLength++;
                        }

                        if (this.isRWRS)
                        {
                            ds.Position = (desc.DataOffset) + (this._inputRowLength * i);
                        }
                        else
                        {
                            ds.Position = (desc.DataOffset * batchCount) + (dataLength * i);
                        }
                        this.InsertParam(ds, desc, value);
                    }
                }
            }

            return data;
        }

        // TODO: look into optimizing this with string.Compare(,,true) == 0 instead of .ToLower() and Equals
        private void UpdateStatementType()
        {
            string str = TrafodionDBUtility.RemoveComments.Replace(this.CommandText, string.Empty);
            string[] tokens = TrafodionDBUtility.Tokenize.Split(str, 3, 0);

            StatementType type = StatementType.Unknown;

            str = tokens[(tokens[0].Length > 0) ? 0 : 1].ToUpper();

            if (str.Equals("SELECT") || str.Equals("SHOWSHAPE") || str.Equals("INVOKE")
                    || str.Equals("SHOWCONTROL") || str.Equals("SHOWDDL") || str.Equals("EXPLAIN")
                    || str.Equals("SHOWPLAN") || str.Equals("REORGANIZE") || str.Equals("MAINTAIN")
                    || str.Equals("SHOWLABEL") || str.Equals("VALUES")
                    || str.Equals("REORG") || str.Equals("SEL") || str.Equals("GET") || str.Equals("SHOWSTATS")
                    || str.Equals("GIVE") || str.Equals("STATUS") || str.Equals("INFO") || str.Equals("LIST"))
            {
                type = StatementType.Select;
            }
            else if (str.Equals("UPDATE") || str.Equals("MERGE"))
            {
                type = StatementType.Update;
            }
            else if (str.Equals("DELETE") || str.Equals("STOP") || str.Equals("START"))
            {
                type = StatementType.Delete;
            }
            else if (str.Equals("INSERT") || str.Equals("INS"))
            {
                if (this.HasParameters)
                {
                    type = StatementType.InsertParam;
                }
                else
                {
                    type = StatementType.Insert;
                }
            }
            else if (str.Equals("ADD") || str.Equals("ALTER"))
            {
                type = StatementType.Insert;
            }
            else if (str.Equals("CREATE"))
            {
                type = StatementType.Create;
            }
            else if (str.Equals("GRANT"))
            {
                type = StatementType.Grant;
            }
            else if (str.Equals("DROP"))
            {
                type = StatementType.Drop;
            }
            else if (str.Equals("CALL"))
            {
                type = StatementType.Call;
            }
            else if (str.Equals("EXPLAIN"))
            {
                type = StatementType.Explain;
            }
            else if (str.Equals("INFOSTATS"))
            {
                type = StatementType.Stats;
            }
            else if (str.Equals("WMSOPEN"))
            {
                type = StatementType.WmsOpen;
            }
            else if (str.Equals("WMSCLOSE"))
            {
                type = StatementType.WmsClose;
            }
            else if (str.Equals("CMDOPEN"))
            {
                type = StatementType.CommandOpen;
            }
            else if (str.Equals("CMDCLOSE"))
            {
                type = StatementType.CommandClose;
            }
            else
            {
                type = StatementType.Unknown;
            }

            this._statementType = type;
        }

        // this assumes we have a valid numeric string -- make sure to check before calling
        // TODO: this was copied from JDBC which does not have unsigned types.  there might be clean up to do here in terms of using smaller unsigned types
        private byte[] ConvertStringToBigNum(string bignum, int targetLength, int targetScale)
        {
            byte[] sourceData = ASCIIEncoding.ASCII.GetBytes(bignum);
            byte[] targetData = new byte[targetLength];
            int[] targetInShorts = new int[targetLength / 2];

            int length;
            int temp;
            int tarPos = 1;

            // remove leading 0s and sign character
            int zeros = 0;
            while (zeros < sourceData.Length && (sourceData[zeros] == '0' || sourceData[zeros] == '-'))
            {
                zeros++;
            }

            // convert from characters to values
            for (int i = zeros; i < sourceData.Length; i++)
            {
                sourceData[i] -= (byte)'0';
            }

            length = sourceData.Length - zeros; // we have a new length

            // iterate through 4 bytes at a time
            for (int i = 0; i < length; i += 4)
            {
                int temp1 = 0;
                int j = 0;

                // get 4 bytes worth of data or as much that is left
                for (j = 0; j < 4 && i + j < length; j++)
                {
                    temp1 = (temp1 * 10) + sourceData[zeros + i + j];
                }

                int power = (int)TrafodionDBUtility.PowersOfTen[j]; // get the power of ten based on how many digits we got

                temp = (targetInShorts[0] * power) + temp1; // move the current digits over and then add our new value in
                targetInShorts[0] = temp & 0xFFFF; // we save only up to 16bits -- the rest gets carried over

                // we do the same thing for the rest of the digits now that we have an upper bound
                for (j = 1; j < targetInShorts.Length; j++)
                {
                    int t = (int)((temp & 0xFFFF0000) >> 16);
                    temp = (targetInShorts[j] * power) + t;

                    targetInShorts[j] = temp & 0xFFFF;
                }

                int carry = (int)((temp & 0xFFFF0000) >> 16);
                if (carry > 0)
                {
                    targetInShorts[tarPos++] = carry;
                }
            }

            // convert the data back to bytes
            for (int i = 0; i < targetInShorts.Length; i++)
            {
                if (this.ByteOrder == ByteOrder.LittleEndian)
                {
                    targetData[(i * 2) + 1] = (byte)((targetInShorts[i] & 0xFF00) >> 8);
                    targetData[i * 2] = (byte)(targetInShorts[i] & 0xFF);
                }
                else
                {
                    targetData[i * 2] = (byte)((targetInShorts[i] & 0xFF00) >> 8);
                    targetData[(i * 2) + 1] = (byte)(targetInShorts[i] & 0xFF);
                }
            }

            // add sign
            if (bignum[0] == '-')
            {
                targetData[targetData.Length - 2] |= 0x80;
            }

            return targetData;
        }

        // if the execution time is large than QueryTimeOut, we issue command cancel
        private void QueryTimeOutEvent(object source)
        {
            TrafodionDBCommand command = (TrafodionDBCommand)source;
            command.Cancel();
        }
        //batch support

        public int BatchCount
        {
            get
            {
                return this._batchedParams.Count;
            }
        }

        public void AddBatch()
        {
            if (!this._isPrepared)
            {
                throw new Exception("can only add batch values when statement is prepared");
            }

            if(this._batchedParams.Count != 0)
            {
                return;
            }
            object[] values = new object[this.Parameters.Count];
            for (int i = 0; i < values.Length; i++)
            {
                values[i] = this.Parameters[i].Value;
            }

            this._batchedParams.Add(values);
        }

        // when rowupdating need to add new datarow into batchedparams
        public void AddBatch(object[] values)
        {
            this._batchedParams.Add(values);
        }

        public void ClearBatch()
        {
            this._batchedParams.Clear();
        }

        //for rowwise
        public void alignRWRSOffsets()
        {
            Descriptor desc;
            int varOffset = 0;
            int memOffset = 0;
            int offset = 0;

            for (int i = 0; i < this._parameters.Count; i++)
            {
                desc = this._parameters[i].Descriptor;
                if (desc.Nullable)
                {
                    memOffset = ((memOffset + 2 - 1) >> 1) << 1;
                    memOffset += sizeof(short);
                }
                desc.DataOffset = this.alignColumnOffsets(memOffset, desc.MaxLength, desc.FsType, out offset);
                memOffset = offset;
            }
            this._inputRowLength = this.alignColumnOffsets(memOffset, this._parameters[0].Descriptor.MaxLength, this._parameters[0].Descriptor.FsType, out offset);
        }

        private int alignColumnOffsets(int offset, int maxLength, FileSystemType fsType, out int OmemOffset)
        {
            int varOffset = 0;
            int memOffset = offset;

            switch (fsType)
            {
                case FileSystemType.Char:
                case FileSystemType.Varchar:
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                case FileSystemType.VarcharLong:
                case FileSystemType.VarcharWithLength:
                    memOffset = ((memOffset + 2 - 1) >> 1) << 1;
                    varOffset = memOffset;
                    memOffset += maxLength + 2;
                    break;
                case FileSystemType.Interval:
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                case FileSystemType.SmallInt:
                case FileSystemType.SmallIntUnsigned:
                    memOffset = ((memOffset + 2 - 1) >> 1) << 1;
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                case FileSystemType.Integer:
                case FileSystemType.IntegerUnsigned:
                case FileSystemType.Real:
                    memOffset = ((memOffset + 4 - 1) >> 2) << 2;
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                case FileSystemType.LargeInt:
                case FileSystemType.Float:
                case FileSystemType.Double:
                    memOffset = ((memOffset + 8 - 1) >> 3) << 3;
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                case FileSystemType.DateTime:
                    memOffset = ((memOffset + 2 - 1) >> 1) << 1;
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                case FileSystemType.Decimal:
                case FileSystemType.DecimalLarge:
                case FileSystemType.DecimalLargeUnsigned:
                case FileSystemType.DecimalUnsigned:
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
                default:
                    varOffset = memOffset;
                    memOffset += maxLength;
                    break;
            }
            OmemOffset = memOffset;
            return varOffset;
        }

        public int RowsetParameterCount
        {
            get
            {
                return 3;
            }
        }

        //for rwrs insert empty row
        private bool isEmptyInsert
        {
            set { this._isEmptyInsert = value; }
            get { return this._isEmptyInsert;  }
        }
    }
}
