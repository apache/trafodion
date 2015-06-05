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
    internal class FetchReply: INetworkReply
    {
        public ReturnCode returnCode;
        public SqlWarningOrError[] errorList;
        public long rowsAffected;
        public int dataFormat;
        public int dataOffset;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            returnCode = (ReturnCode)ds.ReadInt32();

            if (returnCode != ReturnCode.Success && returnCode != ReturnCode.NoDataFound)
            {
                errorList = SqlWarningOrError.ReadListFromDataStream(ds, enc);
            }
            else
            {
                errorList = new SqlWarningOrError[0];
            }

            rowsAffected = ds.ReadUInt32();
            dataFormat = ds.ReadInt32();

            ds.ReadInt32(); // ignore length since we save entire buffer
            dataOffset = ds.Position;

            /*if (returnCode == ReturnCode.Success || returnCode == ReturnCode.SuccessWithInfo)
            {
                data = ds.ReadBytes();
            }*/
        }
    }
}
