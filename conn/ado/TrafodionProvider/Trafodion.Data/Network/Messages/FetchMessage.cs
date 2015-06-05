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

using System;

namespace Trafodion.Data
{
    internal class FetchMessage: INetworkMessage
    {
        public int DialogueId;
        public int AsyncEnable;
        public int QueryTimeout;
        public int StmtHandle;
        public string Label;
        public int LabelCharset;
        public long MaxRowCount;
        public long MaxRowLength;
        public string CursorName;
        public int CursorNameCharset;
        public string Options;

        private byte[] _label;
        private byte[] _cursorName;
        private byte[] _options;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt32(DialogueId);
            ds.WriteInt32(AsyncEnable);
            ds.WriteInt32(QueryTimeout);
            ds.WriteInt32(StmtHandle);
            ds.WriteStringWithCharset(_label, LabelCharset);
            ds.WriteInt64(MaxRowCount);
            ds.WriteInt64(MaxRowLength);
            ds.WriteStringWithCharset(_cursorName, CursorNameCharset);
            ds.WriteString(_options);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 44; //4*4 Int32, 2*8 Int64, 3*4 string len

            _cursorName = enc.GetBytes(CursorName, enc.Transport);
            if (_cursorName.Length > 0)
            {
                len += _cursorName.Length + 4; //charset
            }

            _label = enc.GetBytes(Label, enc.Transport);
            if (_label.Length > 0)
            {
                len += _label.Length + 4; //charset
            }

            _options = enc.GetBytes(Options, enc.Transport);
            if (_options.Length > 0)
            {
                len += _options.Length + 1;
            }

            return len;
        }
    }
}
