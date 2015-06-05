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

namespace Trafodion.Data.ETL
{
    using System;

    internal class GetNeoInfoReply: INetworkReply
    {
        public Header Header;
        public SegmentInformation[] SegmentInfo;

        public string NeoSchemaVersion;
        public string TransporterSchemaVersion;
        public string TransporterVersion;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            Header = new Header();
            Header.ReadFromDataStream(ds, enc);

            this.SegmentInfo = SegmentInformation.ReadListFromDataStream(ds, enc);

            this.NeoSchemaVersion = enc.GetString(ds.ReadString(), TrafodionDBEncoding.ISO88591);
            this.TransporterSchemaVersion = enc.GetString(ds.ReadString(), TrafodionDBEncoding.ISO88591);
            this.TransporterVersion = enc.GetString(ds.ReadString(), TrafodionDBEncoding.ISO88591);
        }
    }
}
