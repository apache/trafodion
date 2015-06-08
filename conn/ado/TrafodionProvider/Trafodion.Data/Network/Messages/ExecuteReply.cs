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
    internal class ExecuteReply: INetworkReply
    {
        public ReturnCode returnCode;
	    public SqlWarningOrError[] errorList;
	    public long rowsAffected;
	    public QueryType queryType;
	    public int estimatedCost;
	    public byte[] data;

	    public int numResultSets;
	    public Descriptor[][] outputDesc;
	    public string [] stmtLabels;

	    public int rowLength;
	    public int paramCount;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
		    returnCode = (ReturnCode) ds.ReadInt32();

            errorList = SqlWarningOrError.ReadListFromDataStream(ds, enc);

		    int outputDescLength = ds.ReadInt32();
            if (outputDescLength > 0)
            {
                outputDesc = new Descriptor[1][];

                rowLength = ds.ReadInt32();
                outputDesc[0] = Descriptor.ReadListFromDataStream(ds, enc);
            }
            else
            {
                outputDesc = new Descriptor[0][];
            }

		    rowsAffected = ds.ReadUInt32();
		    queryType = (QueryType)ds.ReadInt32();
		    estimatedCost = ds.ReadInt32();

		    // 64 bit rowsAffected
		    // this is a horrible hack because we cannot change the protocol yet
		    // rowsAffected should be made a regular 64 bit value when possible
		    rowsAffected |= ((long) estimatedCost) << 32;

            data = ds.ReadBytes();

		    numResultSets = ds.ReadInt32();

		    if (numResultSets > 0) {
			    outputDesc = new Descriptor[numResultSets][];
			    stmtLabels = new String[numResultSets];

			    for (int i = 0; i < numResultSets; i++) {
				    ds.ReadInt32(); // int stmt_handle

                    stmtLabels[i] = enc.GetString(ds.ReadString(), enc.Transport);

				    ds.ReadInt32(); // long stmt_label_charset
				    outputDescLength = ds.ReadInt32();

                    if (outputDescLength > 0)
                    {
                        paramCount = ds.ReadInt32();
                        outputDesc[i] = Descriptor.ReadListFromDataStream(ds, enc);
                    }
                    else
                    {
                        outputDesc[i] = new Descriptor[0];
                    }

				    ds.ReadString(); //proxy syntax
			    }
		    }

            ds.ReadString(); //single proxy syntax
        }
    }
}
