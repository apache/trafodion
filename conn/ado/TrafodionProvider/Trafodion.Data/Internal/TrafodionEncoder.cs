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
using System.Collections.Generic;
using System.Text;

namespace Trafodion.Data
{
    internal class TrafodionDBEncoder
    {
        private TrafodionDBEncoding _transport;
        private ByteOrder _byteOrder;
        private Dictionary<TrafodionDBEncoding, Encoding> _encodings;

        public TrafodionDBEncoder(ByteOrder bo)
        {
            _encodings = new Dictionary<TrafodionDBEncoding, Encoding>(5);
            _encodings.Add(TrafodionDBEncoding.Default, Encoding.Default);
            _encodings.Add(TrafodionDBEncoding.ISO88591, Encoding.GetEncoding("ISO8859-1"));
            _encodings.Add(TrafodionDBEncoding.UCS2, Encoding.BigEndianUnicode);
            _encodings.Add(TrafodionDBEncoding.MS932, Encoding.GetEncoding(932));
            _encodings.Add(TrafodionDBEncoding.UTF8, Encoding.UTF8);

            this.ByteOrder = bo; //trigger property update
            this._transport = TrafodionDBEncoding.UTF8; //trigger property update
        }

        public ByteOrder ByteOrder
        {
            get
            {
                return _byteOrder;
            }
            set
            {
                this._byteOrder = value;

                if (_byteOrder == ByteOrder.LittleEndian)
                {
                    //little endian
                    _encodings[TrafodionDBEncoding.UCS2] =  Encoding.GetEncoding(1200);
                }
                else
                {
                    //big endian
                    _encodings[TrafodionDBEncoding.UCS2] = Encoding.GetEncoding(1201);
                }
            }
        }

        public TrafodionDBEncoding Transport
        {
            get
            {
                return _transport;
            }
        }

        public string GetString(byte[] buf, TrafodionDBEncoding c)
        {
            return _encodings[c].GetString(buf);
        }

        public byte[] GetBytes(string s, TrafodionDBEncoding c)
        {
            if (s == null)
            {
                return new byte[0];
            }

            return _encodings[c].GetBytes(s);
        }
    }
}
