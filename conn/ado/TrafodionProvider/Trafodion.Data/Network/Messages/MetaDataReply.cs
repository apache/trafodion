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
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Trafodion.Data
{
    /*internal enum MetaDataError : int
    {
        Success = 0,
        ParamError = 1,
        InvalidConnection = 2,
        SqlError = 3,
        InvalidHandle = 4,
    }

    internal class MetaDataReply: INetworkReply
    {
        public MetaDataError Error;
        public int ErrorDetail;
        public ErrorDesc[] ErrorDescList;
        public string ErrorText;

        public string p2;
        public Descriptor[] Descriptors;
        public string ProxySyntax;

        public void ReadFromDataStream(DataStream ds)
        {
            Error = (MetaDataError) ds.ReadInt32();
            ErrorDetail = ds.ReadInt32();

            switch (Error)
            {
                case MetaDataError.SqlError:
                    ErrorDescList = ErrorDesc.ReadListFromDataStream(ds);
                    break;
                case MetaDataError.ParamError:
                    ErrorText = ds.ReadString();
                    break;
                case MetaDataError.Success:
                    p2 = ds.ReadString();
                    Descriptors = Descriptor.ReadListFromDataStream(ds);
                    ErrorDescList = ErrorDesc.ReadListFromDataStream(ds);
                    ProxySyntax = ds.ReadString();
                    break;
            }
        }
    }*/
}
