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
    /// Updates the value of a connection property on the server.
    /// </summary>
    internal class SetConnectionOptMessage: INetworkMessage
    {
        public int DialogueId;
        public ConnectionOption Option;
        public int IntValue;
        public string StringValue;

        private byte[] _stringValue;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt32(DialogueId);
            ds.WriteInt16((short)Option);
            ds.WriteInt32(IntValue);
            ds.WriteString(_stringValue);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 14; // 2*4 Int32, 1*2 Int16, 1*4 string len

            _stringValue = enc.GetBytes(StringValue, enc.Transport);
            if (_stringValue.Length > 0)
            {
                len += _stringValue.Length + 1;
            }

            return len;
        }
    }
}
