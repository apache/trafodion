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
    /// Describes how the user will authenticate.
    /// </summary>
    internal enum UserDescType: uint
    {
        AUTHENTICATED_USER_TYPE = 1,
        UNAUTHENTICATED_USER_TYPE = 2,      //regular user
        PASSWORD_ENCRYPTED_USER_TYPE = 3,
        SID_ENCRYPTED_USER_TYPE = 4
    }

    /// <summary>
    /// A user's authentication information.
    /// </summary>
    internal class UserDescription: INetworkMessage
    {
        public UserDescType UserDescType;
        public byte [] UserSid;
        public string DomainName;
        public string UserName;
        public byte [] Password;

        private byte[] _domainName;
        private byte[] _userName;

        public void WriteToDataStream(DataStream ds)
        {
            ds.WriteUInt32((uint)UserDescType);

            ds.WriteString(UserSid);
            ds.WriteString(_domainName);
            ds.WriteString(_userName);
            ds.WriteString(Password);
        }

        public int PrepareMessageParams(TrafodionDBEncoder enc)
        {
            int len = 20; //1*4 Int32, 4*4 string len

            if (UserSid.Length > 0)
            {
                len += UserSid.Length + 1;
            }

            _domainName = enc.GetBytes(DomainName, enc.Transport);
            if (_domainName.Length > 0)
            {
                len += _domainName.Length + 1;
            }

            _userName = enc.GetBytes(UserName, enc.Transport);
            if (_userName.Length > 0)
            {
                len += _userName.Length + 1;
            }

            if (Password.Length > 0)
            {
                len += Password.Length + 1;
            }

            return len;
        }
    }
}
