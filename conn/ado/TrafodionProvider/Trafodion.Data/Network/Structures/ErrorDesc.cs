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
    /// Represents an error returned by NDCS.
    /// </summary>
    internal class ErrorDesc: INetworkReply
    {
        public int rowId;
        public int errorDiagnosticId;
        public int sqlCode;
        public String sqlState;
        public String errorText;
        public int operationAbortId;
        public int errorCodeType;
        public String Param1;
        public String Param2;
        public String Param3;
        public String Param4;
        public String Param5;
        public String Param6;
        public String Param7;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            rowId = ds.ReadInt32();
            errorDiagnosticId = ds.ReadInt32();
            sqlCode = ds.ReadInt32();
            sqlState = System.Text.ASCIIEncoding.ASCII.GetString(ds.ReadBytes(6)); //TODO: is ASCII ok?  what happened to the NULL?
            errorText = enc.GetString(ds.ReadString(), enc.Transport);
            operationAbortId = ds.ReadInt32();
            errorCodeType = ds.ReadInt32();

            Param1 = enc.GetString(ds.ReadString(), enc.Transport);
            Param2 = enc.GetString(ds.ReadString(), enc.Transport);
            Param3 = enc.GetString(ds.ReadString(), enc.Transport);
            Param4 = enc.GetString(ds.ReadString(), enc.Transport);
            Param5 = enc.GetString(ds.ReadString(), enc.Transport);
            Param6 = enc.GetString(ds.ReadString(), enc.Transport);
            Param7 = enc.GetString(ds.ReadString(), enc.Transport);
        }

        public static ErrorDesc[] ReadListFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            int len = ds.ReadInt32();
            ErrorDesc [] errorDesc = new ErrorDesc[len];
            for (int i = 0; i < len; i++)
            {
                errorDesc[i] = new ErrorDesc();
                errorDesc[i].ReadFromDataStream(ds, enc);
            }

            return errorDesc;
        }
    }
}
