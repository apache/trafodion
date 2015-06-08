//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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

using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Model
{
    /// <summary>
    /// Model for a WMS Admin Role
    /// </summary>
    public class NDCSUser : NDCSObject
    {
        public const string USER_PRIVILEGE = "USER";
        public const string OPERATOR_PRIVILEGE = "OPERATOR";

        #region Private member variables

        private NDCSSystem _ndcsSystem;
        private string _roleName;
        private string _privilegeType;
        private string _grantorRoleName;

        #endregion Private Fields

        #region Public properties

        /// <summary>
        /// System in which the service is defined
        /// </summary>
        public NDCSSystem NDCSSystem
        {
            get { return _ndcsSystem; }
        }

        /// <summary>
        /// Name of the service
        /// </summary>
        override public string Name
        {
            get { return _roleName; }
            set { _roleName = value; }
        }

        public string DelimitedName
        {
            get
            {
                return NDCSName.GetDelimitedName(Name);
            }
        }

        public string DelimitedType
        {
            get
            {
                return NDCSName.GetDelimitedName(PrivilegeType);
            }
        }

        public string PrivilegeType
        {
            get { return _privilegeType; }
            set { _privilegeType = value; }
        }

        /// <summary>
        /// Identifies if this admin role is system created
        /// </summary>
        public bool IsOperator
        {
            get
            {
                if (_privilegeType.Equals(OPERATOR_PRIVILEGE))
                    return true;
                else
                    return false;
            }
            set
            {
                _privilegeType = (value ? OPERATOR_PRIVILEGE : USER_PRIVILEGE);
            }

        }       
        /// <summary>
        /// There is nothing to be returned.
        /// </summary>
        public override string DDLText
        {
            get
            {
                return "";
            }
        }

        /// <summary>
        /// The role that granted the privilege to the user
        /// </summary>
        public string GrantorRoleName
        {
            get { return _grantorRoleName; }
        }

        /// <summary>
        /// Connection definition associated with this NDCSService
        /// </summary>
        public override ConnectionDefinition ConnectionDefinition
        {
            get { return NDCSSystem.ConnectionDefinition; }
        }
        
        #endregion Public properties

        public NDCSUser(NDCSSystem ndcsSystem)
        {
            _ndcsSystem = ndcsSystem;
        }
        /// <summary>
        /// Constructs a new WmsAdminRole instance
        /// </summary>
        /// <param name="ndcsSystem"></param>
        /// <param name="aName"></param>
        /// <param name="aType"></param>
        public NDCSUser(NDCSSystem ndcsSystem, string aName, string aType, string aGrantorRoleName)
        {
            _ndcsSystem = ndcsSystem;
            _roleName = aName;
            _privilegeType = aType;
            _grantorRoleName = aGrantorRoleName;
        }

        /// <summary>
        /// Refreshes the admin role attributes
        /// </summary>
        override public void Refresh()
        {
            NDCSSystem.RefreshUsers();
        }

        /// <summary>
        /// Alters the ndcs user
        /// </summary>
        public void Alter()
        {
            OdbcCommand anOpenCommand = null;
            try
            {
                anOpenCommand = this.NDCSSystem.GetCommand();
                int result = Queries.ExecuteAlterUser(anOpenCommand, this);
            }
            finally
            {
                this.NDCSSystem.CloseCommand(anOpenCommand);
            }
        }

        /// <summary>
        /// Returns the admin role name
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
 	         return Name;
        }
    }
}
