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

using System;
namespace Trafodion.Data.ETL
{
    internal class QueryInformation: INetworkReply
    {
        public string QueryText;
        public int SegmentHint;
        public byte CpuHint;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            // we wont use this specifically
            // just read them as a group since the params are not grouped for individual reads
            throw new System.NotImplementedException();
        }

        internal static QueryInformation[] ReadListFromDataStream(DataStream ds, TrafodionDBEncoder enc, int len)
        {
            QueryInformation[] queryInfo = new QueryInformation[len];

            for (int i = 0; i < len; i++)
            {
                queryInfo[i] = new QueryInformation();
                ds.ReadInt32(); // unused charset
                queryInfo[i].QueryText = enc.GetString(ds.ReadString(),enc.Transport);
            }

            byte[] b = new byte[4];
            for (int i = 0; i < len; i++)
            {
                uint temp = ds.ReadUInt32();

                // TODO: this might break on Trafodion-ADO with the endianess changes, not sure how
                // this is packed on the server side
                queryInfo[i].SegmentHint = (int)(temp >> 8);
                queryInfo[i].CpuHint = (byte)(temp & 0xFF);
            }

            return queryInfo;
        }
    }
}
