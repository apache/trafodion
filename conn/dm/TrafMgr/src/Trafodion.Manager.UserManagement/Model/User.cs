//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
//

using System.Collections.Generic;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UserManagement.Model
{
    public class User
    {
        private string _userName;
        private int _userId;
        private string _externalUserName;
        private string _createTime;
        private string _redefTime;
        private string _isImmutable;
        private string _isValidUser;
        private string _authenticationMode;
        private string _grantedRoles;
        private string _logonRoleName;
        List<string> _additionalRoles = new List<string>();

        #region user model properties

        public string UserName
        {
            get { return _userName; }
            set { _userName = value; }
        }
        public int UserId
        {
            get { return _userId; }
            set { _userId = value; }
        } 
        public string ExternalUserName
        {
            get { return _externalUserName; }
            set { _externalUserName = value; }
        }
        public string CreateTime
        {
            get { return _createTime; }
            set { _createTime = value; }
        }
        public string RedefTime
        {
            get { return _redefTime; }
            set { _redefTime = value; }
        }
        public string Immutable
        {
            get { return _isImmutable; }
            set { _isImmutable = value; }
        }
        public string IsValidUser
        {
            get { return _isValidUser; }
            set { _isValidUser = value; }
        }
        public string AuthenticationMode
        {
            get { return _authenticationMode; }
            set { _authenticationMode = value; }
        }
        public string GrantedRoles
        {
            get { return _grantedRoles; }
            set { _grantedRoles = value; }
        }
        public string LogonRoleName
        {
            get { return _logonRoleName; }
            set { _logonRoleName = value; }
        }

        public List<string> AdditionalRoles
        {
            get { return _additionalRoles; }
            set { _additionalRoles = value; }
        }

        public string DelimitedUserName
        {
            get { return Utilities.ExternalUserName(_userName); }
        }

        public string DelimitedExternalUserName
        {
            get { return Utilities.ExternalUserName(_externalUserName); }
        }
       #endregion

        public void AddAdditionalRole(string aRole)
        {
            if (_additionalRoles == null)
            {
                _additionalRoles = new List<string>();
            }
            //TODO: deal with case sensitivity
            if (!_additionalRoles.Contains(aRole))
            {
                _additionalRoles.Add(aRole);
            }
        }
        public static string GetAdditionalRoleString(List<string> roles)
        {
            string value = "";
            if (roles != null)
            {
                int roleCount = roles.Count;
                for (int i = 0; i < roleCount; i++)
                {
                    value += (i < (roleCount - 1)) ? roles[i] + ", " : roles[i];
                }
            }
            return value;
        }
    }


}
