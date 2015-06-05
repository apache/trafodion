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
    /// <summary>
    /// Represents a collection of connection properties sent to the server during logon.
    /// </summary>
    internal class ConnectionContext: INetworkMessage
    {
        public string Datasource;
        public string Catalog;
        public string Schema;
        public string Location;
        public string UserRole;

        public short AccessMode;
        public short AutoCommit;

        public int QueryTimeoutSec;
        public int IdleTimeoutSec;
        public int LoginTimeoutSec;
        public short TxnIsolationLevel;
        public short RowSetSize;
        public int DiagnosticFlag;
        public int ProcessId;

        public string ComputerName;
        public string WindowText;

        public uint CtxACP;
        public uint CtxDataLang;
        public uint CtxErrorLang;
        public short CtxCtrlInferNCHAR;
        public short CpuToUse;
        public short CpuToUseEnd;

        public string ClientVproc;
        public string ConnectOptions;

        public Version [] ClientVersion;

        public ConnectionContextOptions1 InContextOptions1;
        public uint InContextOptions2;

        private byte[] _datasource;
        private byte[] _catalog;
        private byte[] _schema;
        private byte[] _location;
        private byte[] _userRole;

        private byte[] _computerName;
        private byte[] _windowText;

        private byte[] _connectOptions;

        //public so the top layer can write it
        public byte[] _clientVproc;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteString(_datasource);
            ds.WriteString(_catalog);
            ds.WriteString(_schema);
            ds.WriteString(_location);
            ds.WriteString(_userRole);

            ds.WriteInt16(AccessMode);
            ds.WriteInt16(AutoCommit);

            ds.WriteInt32(QueryTimeoutSec);
		    ds.WriteInt32(IdleTimeoutSec);
		    ds.WriteInt32(LoginTimeoutSec);
		    ds.WriteInt16(TxnIsolationLevel);
		    ds.WriteInt16(RowSetSize);

		    ds.WriteInt32(DiagnosticFlag);
		    ds.WriteInt32(ProcessId);

		    ds.WriteString(_computerName);
		    ds.WriteString(_windowText);

		    ds.WriteUInt32(CtxACP);
		    ds.WriteUInt32(CtxDataLang);
		    ds.WriteUInt32(CtxErrorLang);
		    ds.WriteInt16(CtxCtrlInferNCHAR);

		    ds.WriteInt16(CpuToUse);
		    ds.WriteInt16(CpuToUseEnd);
		    ds.WriteString(_connectOptions);

            ds.WriteInt32(ClientVersion.Length);
            for (int i = 0; i < ClientVersion.Length; i++)
            {
                ClientVersion[i].WriteToDataStream(ds);
            }
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 82; //9*4 Int32, 7*2 Int16, 8*4 string len

            _datasource = enc.GetBytes(Datasource, enc.Transport);
            if (_datasource.Length > 0)
            {
                len += _datasource.Length + 1;
            }

            _catalog = enc.GetBytes(Catalog, enc.Transport);
            if (_catalog.Length > 0)
            {
                len += _catalog.Length + 1;
            }

            _schema = enc.GetBytes(Schema, enc.Transport);
            if (_schema.Length > 0)
            {
                len += _schema.Length + 1;
            }

            _location = enc.GetBytes(Location, enc.Transport);
            if (_location.Length > 0)
            {
                len += _location.Length + 1;
            }

            _userRole = enc.GetBytes(UserRole, enc.Transport);
            if (_userRole.Length > 0)
            {
                len += _userRole.Length + 1;
            }

            _computerName = enc.GetBytes(ComputerName, enc.Transport);
            if (_computerName.Length > 0)
            {
                len += _computerName.Length + 1;
            }

            _windowText = enc.GetBytes(WindowText, enc.Transport);
            if (_windowText.Length > 0)
            {
                len += _windowText.Length + 1;
            }

            _connectOptions = enc.GetBytes(ConnectOptions, enc.Transport);
            if (_connectOptions.Length > 0)
            {
                len += _connectOptions.Length + 1;
            }

            for (int i = 0; i < ClientVersion.Length; i++)
            {
                len += ClientVersion[i].PrepareMessageParams(enc);
            }

            _clientVproc = enc.GetBytes(ClientVproc, enc.Transport);

            return len;
        }
    }
}
