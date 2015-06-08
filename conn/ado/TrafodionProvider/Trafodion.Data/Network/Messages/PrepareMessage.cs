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
    internal class PrepareMessage: INetworkMessage
    {
        public int DialogueId;
        public int AsyncEnable;
        public int QueryTimeout;
        public short StmtType;
        public StatementType SqlStmtType;
        public string Label;
        public int LabelCharset;
        public string CursorName;
        public int CursorNameCharset;
        public string ModuleName;
        public int ModuleNameCharset;
        public long ModuleTimestamp;
        public string SqlText;
        public int SqlTextCharset;
        public string StmtOptions;
        public string ExplainLabel;
        public int MaxRowsetSize;
        public byte [] TransactionId;

        private byte[] _label;
        private byte[] _cursorName;
        private byte[] _moduleName;
        private byte[] _sqlText;
        private byte[] _stmtOptions;
        private byte[] _explainLabel;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt32(DialogueId);
            ds.WriteInt32(AsyncEnable);
            ds.WriteInt32(QueryTimeout);
            ds.WriteInt16(StmtType);
            ds.WriteInt32((int)SqlStmtType);
            ds.WriteStringWithCharset(_label, LabelCharset);
            ds.WriteStringWithCharset(_cursorName, CursorNameCharset);
            ds.WriteStringWithCharset(_moduleName, ModuleNameCharset);
            if(ModuleName.Length > 0)
            {
                ds.WriteInt64(ModuleTimestamp);
            }
            ds.WriteStringWithCharset(_sqlText, SqlTextCharset);
            ds.WriteString(_stmtOptions);
            ds.WriteString(_explainLabel);
            ds.WriteInt32(MaxRowsetSize);
            ds.WriteString(TransactionId);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 50; //5*4 Int32, 1*2 Int16, 7*4 string len

            _label = enc.GetBytes(Label, enc.Transport);
            if (_label.Length > 0)
            {
                len += _label.Length + 5; //null + charset
            }

            _cursorName = enc.GetBytes(CursorName, enc.Transport);
            if (_cursorName.Length > 0)
            {
                len += _cursorName.Length + 5; //null + charset
            }

            _moduleName = enc.GetBytes(ModuleName, enc.Transport);
            if (_moduleName.Length > 0)
            {
                len += _moduleName.Length + 13; //null + charset + timestamp
            }

            _sqlText = enc.GetBytes(SqlText, enc.Transport);
            if (_sqlText.Length > 0)
            {
                len += _sqlText.Length + 5; //null + charset
            }

            _stmtOptions = enc.GetBytes(StmtOptions, enc.Transport);
            if (_stmtOptions.Length > 0)
            {
                len += _stmtOptions.Length + 1;
            }

            _explainLabel = enc.GetBytes(ExplainLabel, enc.Transport);
            if (_explainLabel.Length > 0)
            {
                len += _explainLabel.Length + 1;
            }

            if (TransactionId.Length > 0)
            {
                len += TransactionId.Length + 1;
            }

            return len;
        }
    }
}
