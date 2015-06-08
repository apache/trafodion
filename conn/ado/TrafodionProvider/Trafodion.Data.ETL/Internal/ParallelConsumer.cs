/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2015 Hewlett-Packard Development Company, L.P.
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

namespace Trafodion.Data.ETL
{
    using System;
    using System.Data;
    using System.Threading;
    using System.Text;
    using System.IO;

    internal class ParallelConsumer
    {
        private TrafodionDBParallelExtract _extractor;

        private TrafodionDBConnection _conn;
        private TrafodionDBCommand _cmd;
        private TrafodionDBDataReader _dr;

        private Thread _thread;
        private string _connectionString;
        private string _commandText;
        private int _id;

        private Stream _stream;

        public ParallelConsumer(TrafodionDBParallelExtract extractor, string connectionString, string commandText, int id)
        {
            this._extractor = extractor;

            this._connectionString = connectionString;
            this._commandText = commandText;
            this._id = id;
        }

        public TrafodionDBDataReader DataReader
        {
            get
            {
                return this._dr;
            }
        }

        public delegate void ErrorHandler(int id, string message);
        public event ErrorHandler Error;

        public void Execute()
        {
            this._thread = new Thread(new ThreadStart(ExecuteProc));
            this._thread.Start();
        }

        public void Fetch(Stream stream)
        {
            this._stream = stream;

            this._thread = new Thread(new ThreadStart(FetchProc));
            this._thread.Start();
        }

        public void Join()
        {
            this._thread.Join();
        }

        public void Cancel()
        {
            try { this._cmd.Cancel(); }
            catch {}
            try { this._conn.Close(); }
            catch { }
            try { this._thread.Abort(); }
            catch { }
        }

        private void ExecuteProc()
        {
            try
            {
                this._conn = new TrafodionDBConnection(this._connectionString);
                this._conn.Open();
                this._cmd = this._conn.CreateCommand();
                this._cmd.CommandText = "control query default dbtr_process 'ON'";
                this._cmd.ExecuteNonQuery();
/*              this._cmd.CommandText = "cqd allow_nullable_unique_key_constraint 'ON'";
                this._cmd.ExecuteNonQuery();
                this._cmd.CommandText = "set session default dbtr_process 'ON'";
                this._cmd.ExecuteNonQuery();
                this._cmd.CommandText = "control query default allow_audit_attribute_change 'ON'";
                this._cmd.ExecuteNonQuery();
                this._cmd.CommandText = "set transaction autobegin on";
                this._cmd.ExecuteNonQuery();
                this._cmd.CommandText = "control query default auto_query_retry 'OFF'";
                this._cmd.ExecuteNonQuery();
  */            this._cmd.CommandText = this._commandText;

                this._dr = this._cmd.ExecuteReader(CommandBehavior.CloseConnection);

            }
            catch (Exception e)
            {
                this.Error(this._id, e.ToString());
            }
        }

        private void FetchProc()
        {
            try
            {
                int cacheSize = this._extractor.CacheSize;
                TextWriter tw = new StreamWriter(this._stream);

                // TODO: calculate this in a better way
                StringBuilder sb = new StringBuilder(cacheSize);
                object[] v = new object[this._dr.FieldCount];

                while(this._dr.Read())
                {
                    for (int i = 0; i < this._dr.FieldCount; i++)
                    {
                        if (this._extractor.StringValue[i])
                        {
                            // TODO: this seems expensive, is there any other way?
                            v[i] = this._dr.GetString(i).Replace(this._extractor.QuoteIdentifier, this._extractor.QuoteEscapeSequence);
                        }
                        else
                        {
                            v[i] = this._dr.GetValue(i);
                        }
                    }
                    sb.AppendFormat(this._extractor.RowFormat, v);

                    // TODO: this is a bad check as we always expand the buffer once
                    if(sb.Length > cacheSize)
                    {
                        lock(this._stream)
                        {
                            tw.Write(sb.ToString());
                            tw.Flush();
                        }

                        sb.Length = 0; // reset
                    }
                }

                if (sb.Length > 0)
                {
                    lock (this._stream)
                    {
                        tw.Write(sb.ToString());
                        tw.Flush();
                    }
                }
            }
            catch (Exception e)
            {
                this.Error(this._id, e.ToString());
            }
        }
    }
}

