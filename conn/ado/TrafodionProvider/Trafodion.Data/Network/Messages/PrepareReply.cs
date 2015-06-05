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
    internal class PrepareReply: INetworkReply
    {
        public ReturnCode returnCode;
        public SqlWarningOrError[] errorList;
        public QueryType queryType;
        public int stmtHandle;
        public int estimatedCost;

        public int inputDescLength;
        public int inputRowLength;
        public Descriptor[] inputDesc;

        public int outputDescLength;
        public int outputRowLength;
        public Descriptor[] outputDesc;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            returnCode = (ReturnCode)ds.ReadInt32();

            if (returnCode != ReturnCode.Success)
            {
                errorList = SqlWarningOrError.ReadListFromDataStream(ds, enc);
            }

            if (returnCode == ReturnCode.Success|| returnCode == ReturnCode.SuccessWithInfo)
            {
                queryType = (QueryType) ds.ReadInt32();
                stmtHandle = ds.ReadInt32();
                estimatedCost = ds.ReadInt32();

                inputDescLength = ds.ReadInt32();
                if (inputDescLength > 0)
                {
                    inputRowLength = ds.ReadInt32();
                    inputDesc = Descriptor.ReadListFromDataStream(ds, enc);
                }
                else
                {
                    inputDesc = new Descriptor[0];
                }

                outputDescLength = ds.ReadInt32();
                if (outputDescLength > 0)
                {
                    outputRowLength = ds.ReadInt32();
                    outputDesc = Descriptor.ReadListFromDataStream(ds, enc);
                }
                else
                {
                    outputDesc = new Descriptor[0];
                }
            }
        }
    }
}
