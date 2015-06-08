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
    internal enum GetObjRefError: int
    {
        Success = 0,
        ASParamError = 1,
	    ASTimeout = 2,
	    ASNoSrvrHdl = 3,
	    ASTryAgain = 4,
	    ASNotAvailable = 5,
	    DSNotAvailable = 6,
	    PortNotAvailable = 7,
	    InvalidUser = 8,
	    LogonUserFailure = 9,
        Unknown1 = -27, //check on these last two -- why are they here?
        Unknown2 = -29
    }

    internal class GetObjRefReply: INetworkReply
    {
        public GetObjRefError error;
        public int errorDetail;
        public string errorText;

        public string serverObjRef;
        public int dialogueId;
        public string datasource;
        public byte[] userSid;
        public Version [] serverVersion;
        public Boolean securityEnabled;
        public int serverNode;
        public int processId;
        public byte[] timestamp;
        public string cluster;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            error = (GetObjRefError) ds.ReadInt32();
            errorDetail = ds.ReadInt32();
            errorText = enc.GetString(ds.ReadString(), enc.Transport);

            if (error == GetObjRefError.Success)
            {
                serverObjRef = enc.GetString(ds.ReadString(), enc.Transport);
                dialogueId = ds.ReadInt32();
                datasource = enc.GetString(ds.ReadString(), enc.Transport);
                userSid = ds.ReadStringBytes();

                serverVersion = Version.ReadListFromDataStream(ds, enc);

                // read in isoMapping, not stored anywhere
                if ((serverVersion[0].BuildId & BuildOptions.Charset) > 0)
                    ds.ReadInt32();

                if ((serverVersion[0].BuildId & BuildOptions.PasswordSecurity) > 0)
                {
                    securityEnabled = true;

                    serverNode = ds.ReadInt32();
                    processId = ds.ReadInt32();
                    timestamp = ds.ReadBytes(8);
                    cluster = enc.GetString(ds.ReadString(), enc.Transport);
                }
                else
                {
                    securityEnabled = false;
                }
            }
        }
    }
}
