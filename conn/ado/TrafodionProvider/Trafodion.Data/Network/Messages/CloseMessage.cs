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
    internal enum CloseResourceOption: short
    {
        Close = 0,
        Free = 1
    }

    internal class CloseMessage: INetworkMessage
    {
        public int DialogueId;
        public string Label;
        public CloseResourceOption Option;

        private byte[] _label;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt32(DialogueId);
            ds.WriteString(_label);
            ds.WriteInt16((short)Option);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 10; //1*4 Int32, 1*2 Int16, 1*4 string len

            _label = enc.GetBytes(Label, enc.Transport);
            if (_label.Length > 0)
            {
                len += _label.Length + 1;
            }

            return len;
        }
    }
}
