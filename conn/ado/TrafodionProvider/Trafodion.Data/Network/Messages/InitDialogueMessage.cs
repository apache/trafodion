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
    internal class InitDialogueMessage: INetworkMessage
    {
        public UserDescription UserDescription;
        public ConnectionContext ConnectionContext;

        public int DialogueId;
        public ConnectionContextOptions1 OptionFlags1;
        public uint OptionFlags2;
        public String SessionName;
        public String ClientUserName;

        private byte[] _sessionName;
        private byte[] _clientUserName;

        public void WriteToDataStream(DataStream ds)
        {
            UserDescription.WriteToDataStream(ds);
            ConnectionContext.WriteToDataStream(ds);

            ds.WriteInt32(DialogueId);
            ds.WriteUInt32((uint)OptionFlags1);
            ds.WriteUInt32(OptionFlags2);

            if ((OptionFlags1 & ConnectionContextOptions1.SessionName) > 0)
            {
                ds.WriteString(_sessionName);
            }

            if ((OptionFlags1 & ConnectionContextOptions1.ClientUsername) > 0)
            {
                ds.WriteString(_clientUserName);
            }
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 12; // 3*4 Int32

            len += UserDescription.PrepareMessageParams(enc);
            len += ConnectionContext.PrepareMessageParams(enc);

            if ((OptionFlags1 & ConnectionContextOptions1.SessionName) > 0)
            {
                len += 4;

                _sessionName = enc.GetBytes(SessionName, enc.Transport);
                if (_sessionName.Length > 0)
                {
                    len += _sessionName.Length + 1;
                }
            }

            if ((OptionFlags1 & ConnectionContextOptions1.ClientUsername) > 0)
            {
                len += 4;

                _clientUserName = enc.GetBytes(ClientUserName, enc.Transport);
                if (_clientUserName.Length > 0)
                {
                    len += _clientUserName.Length + 1;
                }
            }

            return len;
        }
    }
}
