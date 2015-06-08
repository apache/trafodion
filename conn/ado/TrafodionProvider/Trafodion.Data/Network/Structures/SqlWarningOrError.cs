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
    /// Represents a message returned by the SQL engine.
    /// </summary>
    internal class SqlWarningOrError: INetworkReply
    {
        public int rowId;
	    public int sqlCode;
	    public string text;
	    public string sqlState;

	    public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            rowId = ds.ReadInt32();
            sqlCode = ds.ReadInt32();
            text = enc.GetString(ds.ReadString(), enc.Transport);
            sqlState = enc.GetString(ds.ReadBytes(5), enc.Transport);
            ds.ReadByte(); // null term
	    }

        public static SqlWarningOrError [] ReadListFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            int totalErrorLength = ds.ReadInt32();
            SqlWarningOrError[] errorList;

            if (totalErrorLength > 0)
            {
                errorList = new SqlWarningOrError[ds.ReadInt32()];
                for (int i = 0; i < errorList.Length; i++)
                {
                    errorList[i] = new SqlWarningOrError();
                    errorList[i].ReadFromDataStream(ds, enc);
                }
            }
            else
            {
                errorList = new SqlWarningOrError[0];
            }

            return errorList;
        }
    }
}
