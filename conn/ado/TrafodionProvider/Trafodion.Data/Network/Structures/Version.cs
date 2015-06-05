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
    /// Represents a component version.
    /// </summary>
    internal class Version: INetworkReply, INetworkMessage
    {
        public short ComponentId;
        public short MajorVersion;
        public short MinorVersion;
        public BuildOptions BuildId;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteInt16(ComponentId);
            ds.WriteInt16(MajorVersion);
            ds.WriteInt16(MinorVersion);
            ds.WriteUInt32((uint)BuildId);
        }

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            ComponentId = ds.ReadInt16();
            MajorVersion = ds.ReadInt16();
            MinorVersion = ds.ReadInt16();
            BuildId = (BuildOptions) ds.ReadUInt32();
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            return 10; //1*4 Int32, 3*2 Int16
        }

        public static Version [] ReadListFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            int len = ds.ReadInt32();
            Version [] version = new Version[len];

            for (int i = 0; i < len; i++)
            {
                version[i] = new Version();
                version[i].ReadFromDataStream(ds, enc);
            }

            return version;
        }
    }
}
