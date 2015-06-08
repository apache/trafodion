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
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    /// <summary>
    /// Provides a way of reading a forward-only stream of rows from a TrafodionDB database. This class cannot be inherited.
    /// </summary>
    public sealed class TrafodionDBDataReader : DbDataReader, IDataReader
    {
        // name to ordinal lookups.  these will not be generated for every result but only when GetOrdinal is called for the first time
        private Dictionary<string, int> _csNameMapping;
        private Dictionary<string, int> _isNameMapping;
        private Dictionary<string, Dictionary<string, bool>> _keyInfo;

        private TrafodionDBCommand _cmd;
        private TrafodionDBNetwork _network;
        private Descriptor[][] _desc;

        private DataStream _ds;
        private ByteOrder _byteOrder;

        private CommandBehavior _behavior;

        private long _rowsAffected;

        private int _currentRow;
        private int _rowCount;
        private bool _endOfData;
        private int _currentResult;
        private int _rowLength;
        private int _fetchSize;
        private int _fieldCount;

        private int _dataOffset;
        private bool _prefetch;
        private FetchReply _prefetchReply;
        private MethodInvoker _prefetchInvoke;
        ManualResetEvent _fetchEvent;

        private bool _isUniqueSelect;
        private bool _multipleResults;
        private string[] _stmtLabels;

        private bool _isClosed;
        private bool _freeOnClose;
        private bool _isStmtValid;
        // no data constructor
        internal TrafodionDBDataReader(TrafodionDBCommand cmd)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(cmd.Connection, TraceLevel.Public);
            }

            this._cmd = cmd;
            this._network = cmd.Network;

            this._behavior = CommandBehavior.Default;
            this._rowsAffected = cmd.RowsAffected;
            this._byteOrder = cmd.ByteOrder;

            this._currentResult = -1;
            this._currentRow = -1;
            this._rowCount = 0;
            this._fieldCount = 0;

            this._isClosed = false;
            this._freeOnClose = false;
            this._endOfData = true;
            this._isStmtValid = true;
        }

        internal TrafodionDBDataReader(TrafodionDBCommand cmd, PrepareReply prepareReply, ExecuteReply executeReply)
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(cmd.Connection, TraceLevel.Public);
            }

            this._cmd = cmd;
            this._network = cmd.Network;

            this._behavior = cmd.CommandBehavior;
            this._rowsAffected = cmd.RowsAffected;
            this._byteOrder = cmd.ByteOrder;

            this._currentResult = 0;
            this._freeOnClose = false;
            this._isStmtValid = true;

            this._prefetch = this._cmd.Connection.ConnectionStringBuilder.DoubleBuffering;
            if (this._prefetch)
            {
                this._fetchEvent = new ManualResetEvent(true);
                this._prefetchInvoke = new MethodInvoker(this.Prefetch);
            }

            // SPJ with multiple resultsets
            if (executeReply.numResultSets > 0)
            {
                this._multipleResults = true;
                this._stmtLabels = executeReply.stmtLabels;
            }
            else
            {
                this._multipleResults = false;
            }

            // setup descriptors
            if (prepareReply != null && this._multipleResults == false)
            {
                this._desc = new Descriptor[1][];
                this._desc[0] = prepareReply.outputDesc;
                this._rowLength = prepareReply.outputRowLength;
            }
            else
            {
                this._desc = executeReply.outputDesc;
                this._rowLength = executeReply.rowLength;
            }

            this._fetchSize = Math.Max(
                this._cmd.Connection.ConnectionStringBuilder.FetchRowCount,
                this._cmd.Connection.ConnectionStringBuilder.FetchBufferSize * 1024 / this._rowLength);

            // initialize defaults
            this.NewResult();

            // handling for unique selects
            if (executeReply.data != null && executeReply.data.Length > 0)
            {
                // make sure whatever is done here is similar to what happens after a Fetch call
                this._ds = new DataStream(executeReply.data, this._byteOrder);
                this._rowCount = 1;
                this._isUniqueSelect = true;
            }
            else if (executeReply.returnCode == ReturnCode.NoDataFound)
            {
                this._isUniqueSelect = true;
                this._endOfData = true;
            }
            else
            {
                this._isUniqueSelect = false;
                this._ds = new DataStream(this._byteOrder);
            }

 //           if ((this._behavior & CommandBehavior.SchemaOnly) > 0)
 //           {
 //               this._endOfData = true;
 //           }
            if ((this._behavior & CommandBehavior.SingleRow) > 0)
            {
                this._fetchSize = 1;
            }

            // fetch the first set so we can populate HasRows properly
            if ((this._behavior & CommandBehavior.SchemaOnly) == 0 && this._isUniqueSelect == false)
            {
                lock (this._network.Connection.dataAccessLock)
                {
                        this.Fetch();
                }
            }

            this._currentRow = -1;
        }

        /// <summary>
        /// Gets a value that indicates the depth of nesting for the current row.  Always returns 0.
        /// </summary>
        public override int Depth
        {
            get { return 0; }
        }

        /// <summary>
        /// Gets the number of columns in the current row.
        /// </summary>
        public override int FieldCount
        {
            get { return this._isClosed ? 0 : this._fieldCount; }
        }

        /// <summary>
        /// Gets a value that indicates whether the TrafodionDBDataReader contains one or more rows.
        /// </summary>
        public override bool HasRows
        {
            get { return !this._endOfData; }
        }

        /// <summary>
        /// Retrieves a Boolean value that indicates whether the specified TrafodionDBDataReader instance has been closed.
        /// </summary>
        public override bool IsClosed
        {
            get
            {
                return this._isClosed;
            }
        }

        internal bool IsStmtValid
        {
            get { return this._isStmtValid; }
            set { this._isStmtValid = value; }
        }

        /// <summary>
        /// Gets the number of rows changed, inserted, or deleted.
        /// </summary>
        public override int RecordsAffected
        {
            get
            {
                return (int)this._rowsAffected;
            }
        }

        /// <summary>
        /// Gets or sets the number of rows to fetch during each network message.
        /// </summary>
        public int FetchSize
        {
            get
            {
                return this._fetchSize;
            }

            set
            {
                if (value < 0)
                {
                    TrafodionDBException.ThrowException(this._cmd.Connection, new ArgumentOutOfRangeException("FetchSize cannot be less than 0."));
                }

                this._fetchSize = value;
            }
        }

        internal bool FreeOnClose
        {
            get { return this._freeOnClose; }
            set { this._freeOnClose = value; }
        }

        /// <summary>
        /// Gets the value of the specified column in its native format given the column name.
        /// </summary>
        /// <param name="name">The column name. </param>
        /// <returns>The value of the specified column in its native format.</returns>
        public override object this[string name]
        {
            get
            {
                return this.GetValue(this.GetOrdinal(name));
            }
        }

        /// <summary>
        /// Gets the value of the specified column in its native format given the column ordinal.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column in its native format.</returns>
        public override object this[int ordinal]
        {
            get { return this.GetValue(ordinal); }
        }

        /// <summary>
        /// Releases all resources used by the current instance of the TrafodionDBDataReader class.
        /// </summary>
        public new void Dispose()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public);
            }

            try
            {
                lock (this._network.Connection.dataAccessLock)
                {
                    if (!this._isClosed)
                    {
                        this._cmd.Reset();
                        this.Close();
                    }
                }
            }
            catch
            {
            }
            finally
            {
                base.Dispose();
            }
        }

        /// <summary>
        /// Closes the TrafodionDBDataReader object.
        /// </summary>
        public override void Close()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public);
            }

            if (this._cmd.Connection.State == ConnectionState.Closed)
            {
                this._isClosed = true;
            }

            if (!this._isClosed)
            {
                // if we already fetched through the entire cursor, we dont need to close it
                if (!this._endOfData || this._freeOnClose)
                {
                    CloseMessage message;
                    CloseReply reply;

                    message = new CloseMessage()
                    {
                        Label = this.GetLabel(),
                        Option = this._freeOnClose?CloseResourceOption.Free:CloseResourceOption.Close
                    };

                    reply = this._network.Close(message);
                    this.IsStmtValid = false;
                    switch (reply.error)
                    {
                        case CloseError.Success:
                            break;
                        case CloseError.SqlError:
                            TrafodionDBException e = new TrafodionDBException(reply.errorDesc);
                            this._cmd.Connection.SendInfoMessage(this, e);
                            throw e;
                        case CloseError.InvalidConnection:
                        case CloseError.ParamError:
                        case CloseError.TransactionError:
                        default:
                            TrafodionDBException.ThrowException(this._cmd.Connection, new InternalFailureException("CloseReader: " + reply.error.ToString() + " " + reply.errorDetail + " " + reply.errorText, null));
                            break;
                    }
                }

                this._isClosed = true;

                if ((this._behavior & CommandBehavior.CloseConnection) > 0)
                {
                    this._network.Connection.Close();
                }
            }
        }

        /// <summary>
        /// Gets the value of the BigNumeric column as a string object.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public string GetBigNumeric(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);
            string str;

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if ((desc.TrafodionDBDataType != TrafodionDBDbType.Numeric && desc.TrafodionDBDataType != TrafodionDBDbType.NumericUnsigned) &&
                (desc.FsType != FileSystemType.Numeric && desc.FsType != FileSystemType.NumericUnsigned))
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);

            str = this.ConvertBigNumToString(this._ds.ReadBytes(desc.MaxLength), desc.Scale);

            return str;
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <param name="ordinal">N/A</param>
        /// <returns>N/A</returns>
        /// <exception cref="InvalidCastException">Always</exception>
        public override bool GetBoolean(int ordinal)
        {
            throw new InvalidCastException("GetBoolean");
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <param name="ordinal">N/A</param>
        /// <returns>N/A</returns>
        /// <exception cref="InvalidCastException">Always</exception>
        public override byte GetByte(int ordinal)
        {
            throw new InvalidCastException("GetByte");
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <param name="ordinal">N/A</param>
        /// <param name="dataOffset">N/A</param>
        /// <param name="buffer">N/A</param>
        /// <param name="bufferOffset">N/A</param>
        /// <param name="length">N/A</param>
        /// <returns>N/A</returns>
        /// <exception cref="InvalidCastException">Always</exception>
        public override long GetBytes(int ordinal, long dataOffset, byte[] buffer, int bufferOffset, int length)
        {
            throw new InvalidCastException("GetBytes");
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <param name="ordinal">N/A</param>
        /// <returns>N/A</returns>
        /// <exception cref="InvalidCastException">Always</exception>
        public override char GetChar(int ordinal)
        {
            throw new InvalidCastException("GetChar");
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <param name="ordinal">N/A</param>
        /// <param name="dataOffset">N/A</param>
        /// <param name="buffer">N/A</param>
        /// <param name="bufferOffset">N/A</param>
        /// <param name="length">N/A</param>
        /// <returns>N/A</returns>
        /// <exception cref="InvalidCastException">Always</exception>
        public override long GetChars(int ordinal, long dataOffset, char[] buffer, int bufferOffset, int length)
        {
            throw new InvalidCastException("GetChars");
        }

        /// <summary>
        /// Gets a string representing the data type of the specified column.
        /// </summary>
        /// <param name="ordinal">The zero-based ordinal position of the column to find.</param>
        /// <returns>The string representing the data type of the specified column.</returns>
        public override string GetDataTypeName(int ordinal)
        {
            return this.GetDescriptor(ordinal).TrafodionDBDataType.ToString();
        }

        /// <summary>
        /// Gets the value of the specified column as a DateTime object.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override DateTime GetDateTime(int ordinal)
        {
            DateTime ret;
            int year, month, day, hour, minute, second;
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            this._ds.Position = this.GetDataOffset(ordinal);

            if (desc.TrafodionDBDataType == TrafodionDBDbType.Timestamp)
            {
                year = this._ds.ReadUInt16();
                month = this._ds.ReadByte();
                day = this._ds.ReadByte();
                hour = this._ds.ReadByte();
                minute = this._ds.ReadByte();
                second = this._ds.ReadByte();

                // the default constructor only takes milliseconds
                // create the object and add the partial seconds after if nessisary
                ret = new DateTime(year, month, day, hour, minute, second);

                if (desc.Precision > 0)
                {
                    // partial seconds are not always the same units in TrafodionDB
                    // depending on precision, the integer value represents from .1 to .000001 seconds
                    // there are 7 digits for fractional seconds so use that as our base power for conversion
                    ret = ret.AddTicks(this._ds.ReadUInt32() * ((long)Math.Pow(10, 7 - desc.Precision)));
                }
            }
            else if (desc.TrafodionDBDataType == TrafodionDBDbType.Date)
            {
                year = this._ds.ReadUInt16();
                month = this._ds.ReadByte();
                day = this._ds.ReadByte();

                ret = new DateTime(year, month, day);
            }
            else
            {
                throw new InvalidCastException();
            }

            return ret;
        }

        /// <summary>
        /// Gets the value of the specified column as a TimeSpan object.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public TimeSpan GetTimeSpan(int ordinal)
        {
            TimeSpan ret;
            int hour, minute, second;
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.Time)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);

            hour = this._ds.ReadByte();
            minute = this._ds.ReadByte();
            second = this._ds.ReadByte();
            ret = new TimeSpan(0, hour, minute, second);

            if (desc.Precision > 0)
            {
                // partial seconds are not always the same units in TrafodionDB
                // depending on precision, the integer value represents from .1 to .000001 seconds
                // there are 7 digits for fractional seconds so use that as our base power for conversion
                long ticks = this._ds.ReadUInt32() * TrafodionDBUtility.PowersOfTen[7 - desc.Precision];
                ret = ret.Add(new TimeSpan(ticks));
            }

            return ret;
        }

        /// <summary>
        /// Gets the value of the specified column as a Decimal object.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override decimal GetDecimal(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);
            decimal ret;
            byte[] buf;
            bool neg;

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.Decimal && desc.TrafodionDBDataType != TrafodionDBDbType.DecimalUnsigned &&
                    desc.TrafodionDBDataType != TrafodionDBDbType.Numeric && desc.TrafodionDBDataType != TrafodionDBDbType.NumericUnsigned)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);

            switch (desc.FsType)
            {
                case FileSystemType.Decimal:
                case FileSystemType.DecimalLarge:
                case FileSystemType.DecimalLargeUnsigned:
                case FileSystemType.DecimalUnsigned:
                    buf = this._ds.ReadBytes(desc.MaxLength);
                    if ((buf[0] & 0x80) > 0)
                    {
                        neg = true;
                        buf[0] = (byte)(buf[0] & 0x7F);
                    }
                    else
                    {
                        neg = false;
                    }

                    ret = Decimal.Parse(ASCIIEncoding.ASCII.GetString(buf));

                    if (neg)
                    {
                        ret *= -1;
                    }

                    break;
                case FileSystemType.Integer:
                case FileSystemType.IntegerUnsigned:
                    ret = this._ds.ReadInt32();
                    break;
                case FileSystemType.Numeric:
                case FileSystemType.NumericUnsigned:
                    throw new Exception("numeric columns > 18 precision must be accessed via getString");
                case FileSystemType.SmallInt:
                case FileSystemType.SmallIntUnsigned:
                    ret = this._ds.ReadInt16();
                    break;
                case FileSystemType.LargeInt:
                    ret = this._ds.ReadInt64();
                    break;
                default:
                    throw new InvalidCastException("getdecimal");
            }

            if (desc.Scale > 0)
            {
                ret /= TrafodionDBUtility.PowersOfTen[desc.Scale];
            }

            return ret;
        }

        /// <summary>
        /// Gets the value of the specified column as a Double object.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override double GetDouble(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.Double && desc.TrafodionDBDataType != TrafodionDBDbType.Float)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);
            return BitConverter.Int64BitsToDouble(this._ds.ReadInt64());
        }

        /// <summary>
        /// Returns an IEnumerator that iterates through the TrafodionDBDataReader.
        /// </summary>
        /// <returns>An IEnumerator for the TrafodionDBDataReader.</returns>
        public override IEnumerator GetEnumerator()
        {
            while (this.Read())
            {
                yield return this;
            }
        }

        /// <summary>
        /// Gets the Type that is the data type of the object
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal</param>
        /// <returns>The Type that is the data type of the object.</returns>
        public override Type GetFieldType(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);
            Type t = null;

            // make sure these match GetValue()
            switch (desc.TrafodionDBDataType)
            {
                case TrafodionDBDbType.Char:
                case TrafodionDBDbType.Varchar:
                case TrafodionDBDbType.Interval:
                    t = typeof(string);
                    break;
                case TrafodionDBDbType.Timestamp:
                case TrafodionDBDbType.Date:
                    t = typeof(DateTime);
                    break;
                case TrafodionDBDbType.Time:
                    t = typeof(TimeSpan);
                    break;
                case TrafodionDBDbType.Integer:
                    t = typeof(int);
                    break;
                case TrafodionDBDbType.IntegerUnsigned:
                    t = typeof(uint);
                    break;
                case TrafodionDBDbType.Decimal:
                case TrafodionDBDbType.DecimalUnsigned:
                case TrafodionDBDbType.Numeric:
                case TrafodionDBDbType.NumericUnsigned:
                    if (desc.FsType == FileSystemType.Numeric || desc.FsType == FileSystemType.NumericUnsigned)
                    {
                        t = typeof(string);
                    }
                    else
                    {
                        t = typeof(decimal);
                    }

                    break;
                case TrafodionDBDbType.Double:
                case TrafodionDBDbType.Float:
                    t = typeof(double);
                    break;
                case TrafodionDBDbType.Real:
                    t = typeof(float);
                    break;
                case TrafodionDBDbType.LargeInt:
                    t = typeof(long);
                    break;
                case TrafodionDBDbType.SmallInt:
                    t = typeof(short);
                    break;
                case TrafodionDBDbType.SmallIntUnsigned:
                    t = typeof(ushort);
                    break;
                default:
                    throw new Exception("internal error");
            }

            return t;
        }

        /// <summary>
        /// Gets the value of the specified column as a single-precision floating point number.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override float GetFloat(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.Real)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);

            // this should be optimized if possible
            return BitConverter.ToSingle(BitConverter.GetBytes(this._ds.ReadInt32()), 0);
        }

        /// <summary>
        /// Not supported.
        /// </summary>
        /// <param name="ordinal">N/A</param>
        /// <returns>N/A</returns>
        /// <exception cref="InvalidCastException">Always</exception>
        public override Guid GetGuid(int ordinal)
        {
            throw new InvalidCastException();
        }

        /// <summary>
        /// Gets the value of the specified column as a 16-bit signed integer.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override short GetInt16(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.SmallInt)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);
            return this._ds.ReadInt16();
        }

        /// <summary>
        /// Gets the value of the specified column as a 16-bit unsigned integer.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public ushort GetUInt16(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.SmallIntUnsigned)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);
            return this._ds.ReadUInt16();
        }

        /// <summary>
        /// Gets the value of the specified column as a 32-bit signed integer.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override int GetInt32(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.Integer)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);
            return this._ds.ReadInt32();
        }

        /// <summary>
        /// Gets the value of the specified column as a 32-bit unsigned integer.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public uint GetUInt32(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.IntegerUnsigned)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);
            return this._ds.ReadUInt32();
        }

        /// <summary>
        /// Gets the value of the specified column as a 64-bit signed integer.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override long GetInt64(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            if (desc.TrafodionDBDataType != TrafodionDBDbType.LargeInt)
            {
                throw new InvalidCastException();
            }

            this._ds.Position = this.GetDataOffset(ordinal);
            return this._ds.ReadInt64();
        }

        /// <summary>
        /// Gets the name of the specified column.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal. </param>
        /// <returns>The name of the specified column.</returns>
        public override string GetName(int ordinal)
        {
            Descriptor desc = this._desc[this._currentResult][ordinal];
            return desc.ColumnName;
        }

        /// <summary>
        /// Gets the column ordinal, given the name of the column.
        /// </summary>
        /// <param name="name">The name of the column. </param>
        /// <returns>The zero-based column ordinal.</returns>
        public override int GetOrdinal(string name)
        {
            Descriptor[] desc = this._desc[this._currentResult];
            int ord;

            // create lookup tables for future calls to GetOrdinal
            if (this._csNameMapping == null || this._isNameMapping == null)
            {
                this._csNameMapping = new Dictionary<string, int>(desc.Length);
                this._isNameMapping = new Dictionary<string, int>(desc.Length);

                for (int i = 0; i < desc.Length; i++)
                {
                    this._csNameMapping.Add(desc[i].ColumnName, i);
                    this._isNameMapping.Add(desc[i].ColumnName.ToLower(), i);
                }
            }

            if (this._csNameMapping.ContainsKey(name))
            {
                // case sensitive lookup
                ord = this._csNameMapping[name];
            }
            else if (this._isNameMapping.ContainsKey((name = name.ToLower())))
            {
                // insensitive lookup
                ord = this._isNameMapping[name];
            }
            else
            {
                throw new IndexOutOfRangeException("column not found");
            }

            return ord;
        }

        /// <summary>
        /// Returns a DataTable that describes the column metadata of the TrafodionDBDataReader.
        /// </summary>
        /// <returns>A DataTable that describes the column metadata.</returns>
        /// <exception cref="InvalidOperationException">The TrafodionDBDataReader is closed.</exception>
        /// <remarks>
        /// For the GetSchemaTable method returns metadata about each column in the following order:
        /// ColumnName
        /// ColumnOrdinal
        /// ColumnSize
        /// NumericPrecision
        /// NumericScale
        /// DataType
        /// DataTypeName
        /// ProviderType
        /// ProviderSpecificDataType
        /// IsLong
        /// AllowDBNull
        /// IsUnique
        /// IsKey
        /// BaseCatalogName
        /// BaseSchemaName
        /// BaseTableName
        /// BaseColumnName
        /// </remarks>
        public override DataTable GetSchemaTable()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public);
            }

            if (this.IsClosed)
            {
                throw new InvalidOperationException("data reader is closed");
            }

            bool isKey;

            if ((this._behavior & CommandBehavior.KeyInfo) > 0 && this._keyInfo == null)
            {
                this._keyInfo = new Dictionary<string, Dictionary<string, bool>>(this.FieldCount);
            }

            Descriptor[] desc = this._desc[this._currentResult];
            DataTable dt = new DataTable("GetSchemaTable");
            dt.Columns.Add("ColumnName", typeof(string));
            dt.Columns.Add("ColumnOrdinal", typeof(int));
            dt.Columns.Add("ColumnSize", typeof(int));
            dt.Columns.Add("NumericPrecision", typeof(int));
            dt.Columns.Add("NumericScale", typeof(int));
            dt.Columns.Add("DataType", typeof(Type));
            dt.Columns.Add("DataTypeName", typeof(string));
            dt.Columns.Add("ProviderType", typeof(int));
            dt.Columns.Add("ProviderSpecificDataType", typeof(string));
            dt.Columns.Add("IsLong", typeof(bool));
            dt.Columns.Add("AllowDBNull", typeof(bool));

            dt.Columns.Add("IsUnique", typeof(bool));
            dt.Columns.Add("IsKey", typeof(bool));
            dt.Columns.Add("BaseCatalogName", typeof(string));
            dt.Columns.Add("BaseSchemaName", typeof(string));
            dt.Columns.Add("BaseTableName", typeof(string));
            dt.Columns.Add("BaseColumnName", typeof(string));

            for (int i = 0; i < desc.Length; i++)
            {
                isKey = false;

                if ((this._behavior & CommandBehavior.KeyInfo) > 0)
                {
                    string id = string.Format(
                        "{0}.{1}.{2}",
                        desc[i].CatalogName,
                        desc[i].SchemaName,
                        desc[i].TableName);

                    if (this._keyInfo.ContainsKey(id))
                    {
                        isKey = this._keyInfo[id].ContainsKey(desc[i].ColumnName);
                    }
                    else
                    {
                        Dictionary<string, bool> keyInfo = new Dictionary<string, bool>(this.FieldCount);
                        DataTable keyDt = this._cmd.Connection.GetSchema(
                            "PRIMARYKEYS",
                            new string[] { desc[i].CatalogName, desc[i].SchemaName, desc[i].TableName });

                        foreach (DataRow r in keyDt.Rows)
                        {
                            keyInfo.Add(r["ColumnName"].ToString(), true);
                        }

                        isKey = keyInfo.ContainsKey(desc[i].ColumnName);
                        this._keyInfo.Add(id, keyInfo);
                    }
                }

                dt.Rows.Add(
                    new object[]
                    {
                        desc[i].ColumnName,
                        i,
                        desc[i].MaxLength,
                        desc[i].Precision,
                        desc[i].Scale,
                        this.GetFieldType(i),
                        this.GetFieldType(i).ToString(),
                        (int)desc[i].TrafodionDBDataType,
                        desc[i].TrafodionDBDataType,
                        false,
                        desc[i].Nullable,
                        isKey, // this should really refere to IsUnique
                        isKey,
                        desc[i].CatalogName,
                        desc[i].SchemaName,
                        desc[i].TableName,
                        desc[i].ColumnName
                    });
            }

            return dt;
        }

        /// <summary>
        /// Gets the value of the specified column as a string.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        /// <exception cref="InvalidCastException">The specified cast is not valid.</exception>
        public override string GetString(int ordinal)
        {
            Descriptor desc;
            int dataOffset;
            string ret;

            this.CheckBounds(ordinal);
            this.CheckDBNull(ordinal);

            desc = this.GetDescriptor(ordinal);
            dataOffset = this.GetDataOffset(ordinal);

            this._ds.Position = dataOffset;

            switch (desc.TrafodionDBDataType)
            {
                case TrafodionDBDbType.Interval:
                    ret = System.Text.ASCIIEncoding.ASCII.GetString(this._ds.ReadBytes(desc.MaxLength));
                    break;
                case TrafodionDBDbType.Char:
                    if( desc.SqlEncoding == TrafodionDBEncoding.UTF8)
                    {
                        if (desc.Precision != 0)
                        {
                            desc.MaxLength = desc.Precision;
                        }
                    }
                    ret = this._cmd.Connection.Network.Encoder.GetString(this._ds.ReadBytes(desc.MaxLength), desc.NdcsEncoding);
                    break;
                case TrafodionDBDbType.Varchar:
                    bool shortLength = desc.Precision < Math.Pow(2, 15); // we have 2 byte vs 4 byte length based on precision
                    int length = shortLength ? this._ds.ReadInt16() : this._ds.ReadInt32();

                    ret = this._cmd.Connection.Network.Encoder.GetString(this._ds.ReadBytes(length), desc.NdcsEncoding);
                    break;
                default:
                    ret = this.GetValue(ordinal).ToString();
                    break;
            }

            return ret;
        }

        /// <summary>
        /// Gets the value of the specified column in its native format.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>The value of the specified column.</returns>
        public override object GetValue(int ordinal)
        {
            // MAKE SURE TO UPDATE GetFieldType IF THIS CHANGES
            Descriptor desc = this.GetDescriptor(ordinal);
            object ret = DBNull.Value;

            if (!this.IsDBNull(ordinal))
            {
                switch (desc.TrafodionDBDataType)
                {
                    case TrafodionDBDbType.Char:
                    case TrafodionDBDbType.Varchar:
                    case TrafodionDBDbType.Interval:
                        ret = this.GetString(ordinal);
                        break;
                    case TrafodionDBDbType.Date:
                    case TrafodionDBDbType.Timestamp:
                        ret = this.GetDateTime(ordinal);
                        break;
                    case TrafodionDBDbType.Time:
                        ret = this.GetTimeSpan(ordinal);
                        break;
                    case TrafodionDBDbType.Decimal:
                    case TrafodionDBDbType.DecimalUnsigned:
                    case TrafodionDBDbType.Numeric:
                    case TrafodionDBDbType.NumericUnsigned:
                        if (desc.FsType == FileSystemType.Numeric || desc.FsType == FileSystemType.NumericUnsigned)
                        {
                            ret = this.GetBigNumeric(ordinal);
                        }
                        else
                        {
                            ret = this.GetDecimal(ordinal);
                        }

                        break;
                    case TrafodionDBDbType.Float:
                    case TrafodionDBDbType.Double:
                        ret = this.GetDouble(ordinal);
                        break;
                    case TrafodionDBDbType.Real:
                        ret = this.GetFloat(ordinal);
                        break;
                    case TrafodionDBDbType.Integer:
                        ret = this.GetInt32(ordinal);
                        break;
                    case TrafodionDBDbType.IntegerUnsigned:
                        ret = this.GetUInt32(ordinal);
                        break;
                    case TrafodionDBDbType.LargeInt:
                        ret = this.GetInt64(ordinal);
                        break;
                    case TrafodionDBDbType.SmallInt:
                        ret = this.GetInt16(ordinal);
                        break;
                    case TrafodionDBDbType.SmallIntUnsigned:
                        ret = this.GetUInt16(ordinal);
                        break;
                    default:
                        throw new Exception("internal error");
                }
            }

            return ret;
        }

        /// <summary>
        /// Populates an array of objects with the column values of the current row.
        /// </summary>
        /// <param name="values">An array of Object into which to copy the attribute columns. </param>
        /// <returns>The number of instances of Object in the array</returns>
        public override int GetValues(object[] values)
        {
            int i;

            for (i = 0; i < values.Length && i < this._fieldCount; i++)
            {
                values[i] = this.GetValue(i);
            }

            return i;
        }

        /// <summary>
        /// Gets a value that indicates whether the column contains non-existent or missing values.
        /// </summary>
        /// <param name="ordinal">The zero-based column ordinal.</param>
        /// <returns>true if the specified column value is equivalent to DBNull; otherwise false.</returns>
        public override bool IsDBNull(int ordinal)
        {
            Descriptor desc = this.GetDescriptor(ordinal);
            bool ret;

            if (desc.NullOffset != -1)
            {
                this._ds.Position = this.GetNullOffset(ordinal);
                ret = this._ds.ReadInt16() == -1;
            }
            else
            {
                ret = false;
            }

            return ret;
        }

        /// <summary>
        /// Advances the data reader to the next result.
        /// </summary>
        /// <returns>true if there are more result sets; otherwise false.</returns>
        public override bool NextResult()
        {
            if (TrafodionDBTrace.IsPublicEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Public);
            }

            bool moreResults = false;

            if (this._desc != null && (this._behavior & CommandBehavior.SingleResult) == 0)
            {
                moreResults = ++this._currentResult < this._desc.Length;

                if (moreResults)
                {
                    this.NewResult();
                }
            }

            return moreResults;
        }

        /// <summary>
        /// Advances the TrafodionDBDataReader to the next record.
        /// </summary>
        /// <returns>true if there are more rows; otherwise false.</returns>
        public override bool Read()
        {
            bool ret;
            lock (this._network.Connection.dataAccessLock)
            {
                if (!this._endOfData)
                {
                    if (++this._currentRow >= this._rowCount)
                    {
                        if (this._isUniqueSelect)
                        {
                            this._endOfData = true;
                        }
                        else
                        {
                            this.Fetch();
                        }
                    }
                }

                if (this._endOfData)
                {
                    this.Close();
                }

                ret = !this._endOfData;

                // TODO: i dont like this being checked for every fetch
                if ((this._behavior & CommandBehavior.SingleRow) > 0)
                {
                    this._endOfData = true;
                }
            }
            return ret;
        }

        internal void SetClosed(bool closed)
        {
            this._isClosed = true;
        }

        private void Prefetch()
        {
            FetchMessage message = new FetchMessage()
            {
                AsyncEnable = 0,
                QueryTimeout = 0,
                StmtHandle = this._cmd.Handle,
                Label = this.GetLabel(),
                LabelCharset = 1,
                MaxRowCount = this.FetchSize,
                MaxRowLength = 0,
                CursorName = string.Empty,
                CursorNameCharset = 1,
                Options = string.Empty
            };

            this._prefetchReply = this._network.Fetch(message, this._ds, BufferType.Alt, 0 /* no timeout */);

            this._fetchEvent.Set();
        }

        private FetchReply SyncFetch()
        {
            int networkIOCmdTimeout = this._cmd.CommandTimeout == 0 ? 0 : this._cmd.CommandTimeout;
            FetchMessage message = new FetchMessage()
            {
                AsyncEnable = 0,
                QueryTimeout = 0,
                StmtHandle = this._cmd.Handle,
                Label = this.GetLabel(),
                LabelCharset = 1,
                MaxRowCount = this.FetchSize,
                MaxRowLength = 0,
                CursorName = string.Empty,
                CursorNameCharset = 1,
                Options = string.Empty
            };

            return this._network.Fetch(message, this._ds, BufferType.Main, networkIOCmdTimeout);
        }

        private void Fetch()
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Internal);
            }

            if (!this.IsStmtValid)
            {
                this.Dispose();
                TrafodionDBException.ThrowException(null,new TrafodionDBException(TrafodionDBMessage.StmtInvalid, null));
            }

            FetchReply reply;
            TrafodionDBException ex;

            this._currentRow = -1;

            if (this._prefetch)
            {
                this._fetchEvent.WaitOne();

                if (this._prefetchReply == null)
                {
                    reply = SyncFetch();
                }
                else
                {
                    reply = this._prefetchReply;

                    //swap the buffers
                    byte[] temp = this._ds.GetBuffer(BufferType.Main);
                    this._ds.SetBuffer(BufferType.Main, this._ds.GetBuffer(BufferType.Alt));
                    this._ds.SetBuffer(BufferType.Alt, temp);
                }
            }
            else
            {
                reply = SyncFetch();
            }

            this._rowCount = (int)reply.rowsAffected;

            switch (reply.returnCode)
            {
                case ReturnCode.Success:
                    this._currentRow = 0;
                    this._dataOffset = reply.dataOffset;

                    if (this._prefetch && this._fetchSize > 1)
                    {
                        this._fetchEvent.Reset();
                        this._prefetchInvoke.BeginInvoke(null, null);
                    }
                    break;
                case ReturnCode.SuccessWithInfo:
                    ex = new TrafodionDBException(reply.errorList);
                    this._cmd.Connection.SendInfoMessage(this, ex);

                    // begin regular Success operations
                    this._currentRow = 0;
                    this._dataOffset = reply.dataOffset;
                    if (this._prefetch && this._fetchSize > 1)
                    {
                        this._prefetchInvoke.BeginInvoke(null, null);
                    }
                    break;
                case ReturnCode.NoDataFound:
                    this._endOfData = true;
                    break;
                default:
                    ex = new TrafodionDBException(reply.errorList);
                    this._cmd.Connection.SendInfoMessage(this, ex);
                    throw ex;
            }
        }

        private string GetLabel()
        {
            return this._multipleResults ? this._stmtLabels[this._currentResult] : this._cmd.Label;
        }

        private Descriptor GetDescriptor(int ordinal)
        {
            return this._desc[this._currentResult][ordinal];
        }

        private int GetNullOffset(int ordinal)
        {
            return this._desc[this._currentResult][ordinal].NullOffset + (this._rowLength * this._currentRow) + this._dataOffset;
        }

        private int GetDataOffset(int ordinal)
        {
            return this._desc[this._currentResult][ordinal].DataOffset + (this._rowLength * this._currentRow) + this._dataOffset;
        }

        private void CheckBounds(int i)
        {
            // this can definitely be optimized further
            if (this._isClosed)
            {
                throw new Exception("data reader is closed");
            }

            if (i < 0 || i > this._fieldCount)
            {
                throw new IndexOutOfRangeException("index out of bounds");
            }

            if (this._currentRow < 0 || this._currentRow > this._rowCount)
            {
                throw new Exception("before first row, call Read() first");
            }
        }

        private void CheckDBNull(int i)
        {
            if (this.IsDBNull(i))
            {
                throw new Exception("the column is null.  call IsDBNull first.");
            }
        }

        private void NewResult()
        {
            if (TrafodionDBTrace.IsInternalEnabled)
            {
                TrafodionDBTrace.Trace(this._cmd.Connection, TraceLevel.Internal);
            }

            this._endOfData = false;
            this._currentRow = -1;
            this._rowCount = 0;
            this._isClosed = false;
            this._fieldCount = this._desc[this._currentResult].Length;

            this._csNameMapping = null;
            this._isNameMapping = null;
        }

        // helper for bignum -- we dont want to use a full DataStream
        private ushort ReadUInt16(byte[] buf, int pos)
        {
            ushort ret;

            if (this._byteOrder == ByteOrder.LittleEndian)
            {
                ret = (ushort)(buf[pos] & 0x00ff | ((buf[pos + 1] << 8) & 0xff00));
            }
            else
            {
                ret = (ushort)(buf[pos + 1] & 0x00ff | ((buf[pos] << 8) & 0xff00));
            }

            return ret;
        }

        private string ConvertBigNumToString(byte[] sourceData, int scale)
        {
            string strVal = string.Empty;
            bool negative;

            if (this._byteOrder == ByteOrder.BigEndian)
            {
                negative = (sourceData[sourceData.Length - 2] & 0x80) > 0;
                sourceData[sourceData.Length - 2] &= 0x7F; // force sign to 0, continue normally
            }
            else
            {
                negative = (sourceData[sourceData.Length - 1] & 0x80) > 0;
                sourceData[sourceData.Length - 1] &= 0x7F; // force sign to 0, continue normally
            }


            ushort[] dataInShorts = new ushort[sourceData.Length / 2];
            for (int i = 0; i < dataInShorts.Length; i++)
            {
                dataInShorts[i] = this.ReadUInt16(sourceData, i * 2); // copy the data
            }

            int curPos = dataInShorts.Length - 1; // start at the end
            // get rid of any trailing 0's
            while (curPos >= 0 && dataInShorts[curPos] == 0)
            {
                curPos--;
            }

            int remainder = 0;
            long temp; // we need to use a LONG since we will have to hold up to
            // 32-bit UNSIGNED values

            // we now have the huge value stored in 2 bytes chunks
            // we will divide by 10000 many times, converting the remainder to
            // String
            // when we are left with a single chunk <10000 we will handle it using a
            // special case
            while (curPos >= 0 || dataInShorts[0] >= 10000)
            {
                // start on the right, divide the 16 bit value by 10000
                // use the remainder as the upper 16 bits for the next division
                for (int j = curPos; j >= 0; j--)
                {
                    // these operations got messy when java tried to infer what size
                    // to store the value in
                    // leave these as separate operations for now...always casting
                    // back to a 64 bit value to avoid sign problems
                    temp = remainder;
                    temp &= 0xFFFF;
                    temp = temp << 16;
                    temp += dataInShorts[j];

                    dataInShorts[j] = (ushort)(temp / 10000);
                    remainder = (int)(temp % 10000);
                }

                // if we are done with the current 16bits, move on
                if (dataInShorts[curPos] == 0)
                {
                    curPos--;
                }

                // go through the remainder and add each digit to the final String
                for (int j = 0; j < 4; j++)
                {
                    strVal = (remainder % 10) + strVal;
                    remainder /= 10;
                }
            }

            // when we finish the above loop we still have 1 <10000 value to include
            remainder = dataInShorts[0];
            for (int j = 0; j < 4; j++)
            {
                strVal = (remainder % 10) + strVal;
                remainder /= 10;
            }

            // TODO: optimize
            if (scale > 0)
            {
                strVal = strVal.Insert(Math.Max(strVal.Length - scale, 0), ".");
            }

            strVal = strVal.TrimStart(new char[] { '0' });

            if (negative)
            {
                strVal = "-" + strVal;
            }

            return strVal;
        }
    }
}
