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
    using System.Collections.Generic;
    using System.Data;
    using System.IO;
    using System.Text;
    using System.Threading;

    /// <summary>
    /// Represents a TrafodionDB Transporter parallel extract operation
    /// </summary>
    public class TrafodionDBParallelExtract : IDisposable
    {
        private const int DBT_SVR_PARALLELEXTRACTINFO = 10101;
        private const int DBT_SVC_GETNEOINFO = 10001;
        private const string ProducerQuery = "INSERT INTO TABLE ( EXTRACT_TARGET ('ESP', {0} ) ) ( {1} )";

        private TrafodionDBConnection _producer;
        private List<ParallelConsumer> _consumers;

        private int _parallelStreams;
        private TrafodionDBDataReader prdr;
        private TrafodionDBCommand cmd;
        ManualResetEvent locker;

        /// <summary>
        /// Initializes a new instance of the NVTParallelExtract class.
        /// </summary>
        public TrafodionDBParallelExtract()
        {
            this.ParallelStreams = 2;
            this.CacheSize = 1048576; // 1mb
            this.QuoteIdentifier = "\"";
            this.QuoteEscapeSequence = "\"\"\"";
            this.FieldDelimiter = "|";
            this.RowDelimiter = "\n";
            this.FetchSize = 100;
        }

        /// <summary>
        /// The base connection string to use for the parallel extract.
        /// </summary>
        public string ConnectionString
        {
            get;
            set;
        }

        /// <summary>
        /// The sql text to execute.
        /// </summary>
        public string CommandText
        {
            get;
            set;
        }

        /// <summary>
        /// The number of parallel
        /// </summary>
        public int ParallelStreams
        {
            get
            {
                return this._parallelStreams;
            }
            set
            {
                // TODO: is 16 the max or just per segment max?
                if (value < 2)
                {
                    throw new ArgumentOutOfRangeException(); //todo - error text
                }

                this._parallelStreams = value;
            }
        }

        /// <summary>
        /// The size of the in memory cache (per thread) to buffer before writing data to the stream.
        /// </summary>
        public int CacheSize
        {
            get;
            set;
        }

        public string QuoteIdentifier
        {
            get;
            set;
        }

        public string QuoteEscapeSequence
        {
            get;
            set;
        }

        public string FieldDelimiter
        {
            get;
            set;
        }

        public string RowDelimiter
        {
            get;
            set;
        }

        public int FetchSize
        {
            get;
            set;
        }

        internal bool[] StringValue
        {
            get;
            private set;
        }

        internal string RowFormat
        {
            get;
            private set;
        }

        /// <summary>
        /// Closes all connections and frees all resources used by the NVTParallelExtract object.
        /// </summary>
        public void Close()
        {
            // TODO:

            this._producer.Close();

            for (int i = 0; i < this._consumers.Count; i++)
            {
                this._consumers[i].Cancel();
            }
        }

        /// <summary>
        /// Releases all resources used by the NVTParallelExtract object.
        /// </summary>
        public void Dispose()
        {
            // TODO:

            Close();
        }

        /// <summary>
        /// Creates an array of TrafodionDBDataReaders for the associated CommandText
        /// </summary>
        /// <returns>An array of TrafodionDBDataReaders</returns>
        public TrafodionDBDataReader [] Execute()
        {
            // use a single ConnectionStringBuilder for all the connections
            TrafodionDBConnectionStringBuilder csb = new TrafodionDBConnectionStringBuilder(this.ConnectionString);

            this._producer = new TrafodionDBConnection(csb.ConnectionString);
            this._producer.Open();

            GetNeoInfoReply infoReply = GetNeoInfo();
            // TODO: do something with this info

            cmd = this._producer.CreateCommand();
            cmd.CommandText = "control query default dbtr_process 'ON'";
            cmd.ExecuteNonQuery();
/*            cmd.CommandText = "cqd allow_nullable_unique_key_constraint 'ON'";
            cmd.ExecuteNonQuery();
            cmd.CommandText = "set session default dbtr_process 'ON'";
            cmd.ExecuteNonQuery();
            cmd.CommandText = "control query default allow_audit_attribute_change 'ON'";
            cmd.ExecuteNonQuery();
            cmd.CommandText = "set transaction autobegin on";
            cmd.ExecuteNonQuery();
            cmd.CommandText = "control query default auto_query_retry 'OFF'";
            cmd.ExecuteNonQuery();
 */         cmd.CommandText = "get service";
            string value=(string)cmd.ExecuteScalar();
            System.Console.WriteLine("service:"+value);
            locker=new ManualResetEvent(false);

            cmd.CommandText = string.Format(ProducerQuery, this.ParallelStreams, this.CommandText);
            cmd.Prepare();
            // TODO: since this auto fetches the first batch of results immediately, we use Schema Only so no fetch occurs
            // this could be a problem if we need to fetch this later on to get error messages

            // TODO: we need to fetch this eventually for errors?

            prdr = cmd.ExecuteReader(CommandBehavior.SchemaOnly);

            Thread thr=new Thread(new ThreadStart(ProducerFetch));
            //locker = new ManualResetEvent(false);
            //locker1 = new ManualResetEvent(false);
            thr.Start();
            //locker.WaitOne();
            //Thread.Sleep(5000);

            GetParallelExtractInfoMessage message = new GetParallelExtractInfoMessage()
            {
                NumStreams = this.ParallelStreams,
                ServerStmtHandle = cmd.Handle
            };
            GetParallelExtractInfoReply extractReply = GetParallelExtractInfo(message, this.ParallelStreams);
            locker.Set();
            this._consumers = new List<ParallelConsumer>(this.ParallelStreams);
            ParallelConsumer consumer;

            for (int i = 0; i < this.ParallelStreams; i++)
            {
                //csb.CpuToUse = (short)((extractReply.QueryInfo[i].SegmentHint-1) * 16 + extractReply.QueryInfo[i].CpuHint);
                csb.CpuToUse = -1;
                consumer = new ParallelConsumer(this, csb.ConnectionString, extractReply.QueryInfo[i].QueryText, i);
                consumer.Error += new ParallelConsumer.ErrorHandler(ConsumerError);
                this._consumers.Add(consumer);

                consumer.Execute();
            }

            TrafodionDBDataReader[] readers = new TrafodionDBDataReader[this.ParallelStreams];

            // wait for all threads to finish
            for(int i=0;i<this._consumers.Count;i++)
            {
                this._consumers[i].Join();
                readers[i] = this._consumers[i].DataReader;
                readers[i].FetchSize = this.FetchSize;
                System.Console.WriteLine("execute/first_fetch return from customer query!");
            }

            return readers;
        }

        /// <summary>
        /// Writes any outstanding data to the provided Stream.
        /// </summary>
        /// <param name="stream">The stream to write to.</param>
        public void WriteToStream(Stream stream)
        {
            BuildFormatString();

            // TODO: this does NOT handle ORDER BY clauses properly
            for(int i=0;i<this._consumers.Count;i++)
            {
                this._consumers[i].Fetch(stream);
            }
            System.Console.WriteLine("fetching from "+this._consumers.Count+" stream");
            for (int i = 0; i < this._consumers.Count; i++)
            {
                this._consumers[i].Join();
                System.Console.WriteLine("fetch: on stream " + "i:"+i+"finished!");
            }
        }
        private void ProducerFetch()
        {
            locker.WaitOne();
            if (prdr.Read())
            {
                System.Console.WriteLine("fetch return from producer query - with more row left!");
            }
            else
                System.Console.WriteLine("fetch return from producer query - without row left!");
        }
        /// <summary>
        /// Writes any outstanding data to the provided Stream.
        /// </summary>
        /// <param name="stream">The stream to write to.</param>
        public void WriteToStreams(Stream [] streams)
        {
            if (streams.Length != this.ParallelStreams)
            {
                throw new ArgumentException("number of Stream objects must match the number of ParallelStreams");
            }

            BuildFormatString();

            // TODO: this does NOT handle ORDER BY clauses properly
            for (int i = 0; i < this._consumers.Count; i++)
            {
                this._consumers[i].Fetch(streams[i]);
            }

            for (int i = 0; i < this._consumers.Count; i++)
            {
                this._consumers[i].Join();
            }
        }

        internal void BuildFormatString()
        {
            TrafodionDBDataReader dr = this._consumers[1].DataReader;
            this.StringValue = new bool[dr.FieldCount];

            StringBuilder format = new StringBuilder();
            for (int i = 0; i < dr.FieldCount; i++)
            {
                this.StringValue[i] = dr.GetFieldType(i) == typeof(string);

                if (this.StringValue[i])
                {
                    format.Append(this.QuoteIdentifier);
                    format.Append("{");
                    format.Append(i);
                    format.Append("}");
                    format.Append(this.QuoteIdentifier);
                }
                else
                {
                    format.Append("{");
                    format.Append(i);
                    format.Append("}");
                }

                if(i != dr.FieldCount - 1)
                {
                    format.Append(this.FieldDelimiter);
                }
            }

            format.Append(this.RowDelimiter);

            this.RowFormat = format.ToString();
        }

        internal void ConsumerError(int id, string message)
        {
            Console.WriteLine("failed: {0} {1}", id, message);

            // TODO: cancel other threads
            // TODO: report error to user
        }

        private GetNeoInfoReply GetNeoInfo()
        {
            int IOTimeout = 0;
            GetNeoInfoReply reply = new GetNeoInfoReply();

            this._producer.Network.DoIO(new GetNeoInfoMessage(), reply, (OperationId)DBT_SVC_GETNEOINFO, IOTimeout);

            return reply;
        }

        private GetParallelExtractInfoReply GetParallelExtractInfo(GetParallelExtractInfoMessage message, int len)
        {
            int IOTimeout = 0;
            GetParallelExtractInfoReply reply = new GetParallelExtractInfoReply(len);

            this._producer.Network.DoIO(message, reply, (OperationId)DBT_SVR_PARALLELEXTRACTINFO, IOTimeout);

            return reply;
        }
    }
}
