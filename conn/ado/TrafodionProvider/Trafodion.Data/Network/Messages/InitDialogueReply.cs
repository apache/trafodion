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
    internal enum InitDialogueError : int
    {
        Success = 0,
        ParamError = 1,
	    InvalidConnection = 2,
	    SQLError = 3,
	    SQLInvalidHandle = 4,
	    SQLNeedData = 5,
	    InvalidUser = 6
    }

    //static final int SQL_PASSWORD_EXPIRING = 8857;
	//static final int SQL_PASSWORD_GRACEPERIOD = 8837;

    internal class InitDialogueReply: INetworkReply
    {
        public InitDialogueError error;
        public int errorDetail;
        public string errorText;

        public OutConnectionContext outConnectionContext;

        public ErrorDesc [] errorDesc;

        public void ReadFromDataStream(DataStream ds, TrafodionDBEncoder enc)
        {
            error = (InitDialogueError)ds.ReadInt32();
            errorDetail = ds.ReadInt32();

            outConnectionContext = new OutConnectionContext();

            switch (error)
            {
                case InitDialogueError.Success:
                    outConnectionContext.ReadFromDataStream(ds, enc);
                    break;
                case InitDialogueError.InvalidUser:
                    //TODO: need to check detail for expiring/grace
                    errorDesc = ErrorDesc.ReadListFromDataStream(ds, enc);
                    outConnectionContext.ReadFromDataStream(ds, enc);
                    break;
                case InitDialogueError.SQLError:
                    errorDesc = ErrorDesc.ReadListFromDataStream(ds, enc);
                    outConnectionContext.ReadFromDataStream(ds, enc);
                    break;
                case InitDialogueError.ParamError:
                    errorText = enc.GetString(ds.ReadString(), enc.Transport);
                    break;
                //TODO: shouldnt there be more here?  need to look up in the server which others return text
            }
        }
    }
}
