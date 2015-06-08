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

    /// <summary>
    /// Holds the metadata of a parameter or column.
    /// </summary>
    internal class Descriptor : INetworkReply
    {
        internal int DataOffset;
        internal int NullOffset;
        internal int Version;
        internal FileSystemType FsType;
        internal DateTimeCode DtCode;
        internal int MaxLength;
        internal int Precision;
        internal int Scale;
        internal bool Nullable;
        internal bool Signed;
        internal FileSystemType NdcsDataType;
        internal int NdcsPrecision;
        internal TrafodionDBEncoding SqlEncoding;
        internal TrafodionDBEncoding NdcsEncoding;
        internal string ColumnName;
        internal string TableName;
        internal string CatalogName;
        internal string SchemaName;
        internal string HeadingName;
        internal int IntLeadPrec;
        internal ParameterDirection Direction;

        private TrafodionDBDbType _TrafodionDBDbType;
        private DbType _dbType;

        internal TrafodionDBDbType TrafodionDBDataType
        {
            get
            {
                return this._TrafodionDBDbType;
            }

            set
            {
                this._TrafodionDBDbType = value;
                this._dbType = TrafodionDBUtility.MapTrafodionDBDbType(this._TrafodionDBDbType);
            }
        }

        internal DbType DataType
        {
            get
            {
                return this._dbType;
            }

            set
            {
                this._dbType = value;
                this._TrafodionDBDbType = TrafodionDBUtility.MapDbType(this._dbType);
            }
        }

        /// <summary>
        /// Reads structured data out of the DataStream
        /// </summary>
        /// <param name="ds">The DataStream object.</param>
        /// <param name="enc">The TrafodionDBEncoder object.</param>
        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            this.DataOffset = ds.ReadInt32();
            this.NullOffset = ds.ReadInt32();
            this.Version = ds.ReadInt32();
            this.FsType = (FileSystemType)ds.ReadInt32();
            this.DtCode = (DateTimeCode)ds.ReadInt32();
            this.MaxLength = ds.ReadInt32();
            this.Precision = ds.ReadInt32();
            this.Scale = ds.ReadInt32();
            this.Nullable = ds.ReadInt32() == 1;
            this.Signed = ds.ReadInt32() == 1;
            this.NdcsDataType = (FileSystemType)ds.ReadInt32();
            this.NdcsPrecision = ds.ReadInt32();
            this.SqlEncoding = (TrafodionDBEncoding)ds.ReadInt32();
            this.NdcsEncoding = (TrafodionDBEncoding)ds.ReadInt32();
            this.ColumnName = enc.GetString(ds.ReadString(), enc.Transport);
            this.TableName = enc.GetString(ds.ReadString(), enc.Transport);
            this.CatalogName = enc.GetString(ds.ReadString(), enc.Transport);
            this.SchemaName = enc.GetString(ds.ReadString(), enc.Transport);
            this.HeadingName = enc.GetString(ds.ReadString(), enc.Transport);
            this.IntLeadPrec = ds.ReadInt32();

            int pm = ds.ReadInt32();
            if (pm == 2)
            {
                this.Direction = ParameterDirection.InputOutput;
            }
            else if (pm == 4)
            {
                this.Direction = ParameterDirection.Output;
            }
            else
            {
                this.Direction = ParameterDirection.Input;
            }

            this.MapFileSystemType();
        }

        internal static Descriptor[] ReadListFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            int count = ds.ReadInt32();
            Descriptor[] desc = new Descriptor[count];

            for (int i = 0; i < desc.Length; i++)
            {
                desc[i] = new Descriptor();
                desc[i].ReadFromDataStream(ds, enc);
            }

            return desc;
        }

        /// <summary>
        /// Maps the descriptor to a DbType and TrafodionDBDbType
        /// </summary>
        private void MapFileSystemType()
        {
            switch (this.FsType)
            {
                case FileSystemType.Varchar:
                case FileSystemType.VarcharDblByte:
                case FileSystemType.VarcharLong:
                case FileSystemType.VarcharWithLength:
                    this.TrafodionDBDataType = TrafodionDBDbType.Varchar;
                    break;
                case FileSystemType.Char:
                case FileSystemType.CharDblByte:
                    this.TrafodionDBDataType = TrafodionDBDbType.Char;
                    break;
                case FileSystemType.Numeric:
                    this.TrafodionDBDataType = TrafodionDBDbType.Numeric;
                    break;
                case FileSystemType.NumericUnsigned:
                    this.TrafodionDBDataType = TrafodionDBDbType.NumericUnsigned;
                    break;
                case FileSystemType.Decimal:
                case FileSystemType.DecimalLarge:
                    this.TrafodionDBDataType = TrafodionDBDbType.Decimal;
                    break;
                case FileSystemType.DecimalUnsigned:
                case FileSystemType.DecimalLargeUnsigned:
                    this.TrafodionDBDataType = TrafodionDBDbType.DecimalUnsigned;
                    break;
                case FileSystemType.Integer:
                    this.TrafodionDBDataType = (this.NdcsDataType == FileSystemType.Numeric) ? TrafodionDBDbType.Numeric : TrafodionDBDbType.Integer;
                    break;
                case FileSystemType.IntegerUnsigned:
                    this.TrafodionDBDataType = (this.NdcsDataType == FileSystemType.Numeric) ? TrafodionDBDbType.NumericUnsigned : TrafodionDBDbType.IntegerUnsigned;
                    break;
                case FileSystemType.LargeInt:
                    this.TrafodionDBDataType = (this.NdcsDataType == FileSystemType.Numeric) ? TrafodionDBDbType.Numeric : TrafodionDBDbType.LargeInt;
                    break;
                case FileSystemType.SmallInt:
                    this.TrafodionDBDataType = (this.NdcsDataType == FileSystemType.Numeric) ? TrafodionDBDbType.Numeric : TrafodionDBDbType.SmallInt;
                    break;
                case FileSystemType.SmallIntUnsigned:
                    this.TrafodionDBDataType = (this.NdcsDataType == FileSystemType.Numeric) ? TrafodionDBDbType.NumericUnsigned : TrafodionDBDbType.SmallIntUnsigned;
                    break;
                case FileSystemType.Float:
                    this.TrafodionDBDataType = TrafodionDBDbType.Float;
                    break;
                case FileSystemType.Real:
                    this.TrafodionDBDataType = TrafodionDBDbType.Real;
                    break;
                case FileSystemType.Double:
                    this.TrafodionDBDataType = TrafodionDBDbType.Double;
                    break;
                case FileSystemType.DateTime:
                    switch (this.DtCode)
                    {
                        case DateTimeCode.Date:
                            this.TrafodionDBDataType = TrafodionDBDbType.Date;
                            break;
                        case DateTimeCode.Time:
                            this.TrafodionDBDataType = TrafodionDBDbType.Time;
                            break;
                        case DateTimeCode.Timestamp:
                            this.TrafodionDBDataType = TrafodionDBDbType.Timestamp;
                            break;
                        default:
                            throw new Exception("internal error, unknown datetimecode");
                    }

                    break;
                case FileSystemType.Interval:
                    this.TrafodionDBDataType = TrafodionDBDbType.Interval;
                    break;
                default:
                    throw new Exception("internal error, unknown fstype");
            }
        }
    }
}
