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
    internal class GetObjRefMessage: INetworkMessage
    {
        public ConnectionContext ConnectionContext;
        public UserDescription UserDescription;

        public int ServerType;
        public short RetryCount;

        public string ClientUsername;

        private byte[] _clientUsername;

        public void WriteToDataStream(DataStream ds)
        {
            ConnectionContext.WriteToDataStream(ds);
            UserDescription.WriteToDataStream(ds);

            ds.WriteInt32(ServerType);
            ds.WriteInt16(RetryCount);

            //TODO: splitting up how the struct is written is messy
            ds.WriteUInt32((uint)ConnectionContext.InContextOptions1);
            ds.WriteUInt32(ConnectionContext.InContextOptions2);
            ds.WriteString(ConnectionContext._clientVproc);

            ds.WriteString(_clientUsername);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 22; //5*4 Int32, 1*2 Int16

            len += ConnectionContext.PrepareMessageParams(enc);
            len += UserDescription.PrepareMessageParams(enc);

            if (ConnectionContext._clientVproc.Length > 0)
            {
                len += ConnectionContext._clientVproc.Length + 1;
            }

            _clientUsername = enc.GetBytes(ClientUsername, enc.Transport);
            if (_clientUsername.Length > 0)
            {
                len += _clientUsername.Length + 1;
            }

            return len;
        }
    }
}
