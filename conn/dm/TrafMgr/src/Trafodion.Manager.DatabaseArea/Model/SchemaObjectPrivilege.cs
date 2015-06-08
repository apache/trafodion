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

using System;
using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Describes the privileges that apply to objects that reside under a schema.
    /// </summary>
    public class SchemaObjectPrivilege : Privilege, IComparable<SchemaObjectPrivilege>
    {

        #region Fields

        private List<string> _ddlPrivileges = new List<string>();
        private List<string> _dmlPrivileges = new List<string>();
        private List<string> _utilityPrivileges = new List<string>();

        private bool _isAdministrator = false;

        #endregion

        #region Properties

        public List<string> DDLPrivileges
        {
            get { return _ddlPrivileges; }
        }

        public List<string> DMLPrivileges
        {
            get { return _dmlPrivileges; }
        }

        public List<string> UtilityPrivileges
        {
            get { return _utilityPrivileges; }
        }

        #endregion

        /// <summary>
        /// Creates a new set of object privileges based on a privilege type.
        /// </summary>
        /// A string representation of a privilege. It must be one of the privilege types
        /// defined for this class.
        /// <param name="type">The privilege type.</param>
        /// <param name="grantable">The user's grantable permission.</param>
        /// <param name="grantor">The user's id which granted these permissions.</param>
        /// <param name="grantorType">The usertype of the grantee.</param>
        /// <param name="grantee">The user's id which was granted the permissions.</param>
        /// <param name="granteeType">The usertype of the grantor.</param>
        public SchemaObjectPrivilege(string type, bool grantable, int grantor, string grantorName, string grantorType,
            int grantee, string granteeName, string granteeType)
            : base(grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType)
        {
            AddPrivilegesByTypeString(type);
        }

        /// <summary>
        /// Adds privileges based on the type of privilege specified.
        /// </summary>
        /// <param name="type">
        /// A string representation of a privilege. It must be one of the privilege types
        /// defined for this class.
        /// </param>
        virtual public void AddPrivilegesByTypeString(string type)
        {
            if (TrafodionPrivilegeTypeHelper.IsObjectPrivilege(type))
            {
                // The priv type is defined, so add it to the list
                if (TrafodionPrivilegeTypeHelper.IsDDLPrivilege(type))
                {
                    _ddlPrivileges.Add(type);
                }
                else if (TrafodionPrivilegeTypeHelper.IsDMLPrivilege(type))
                {
                    _dmlPrivileges.Add(type);
                }
                else if (TrafodionPrivilegeTypeHelper.IsUtilityPrivilege(type))
                {
                    _utilityPrivileges.Add(type);
                }
                else if (TrafodionPrivilegeTypeHelper.TYPE_ADMINISTRATOR.Equals(type, StringComparison.OrdinalIgnoreCase))
                {
                    _isAdministrator = true;
                }
            }
        }

        /// <summary>
        /// Compares this SchemaObjectPrivilege to another instance.
        /// NOTE: warning! This comparision does not compare the privilege list a grantee has.
        /// </summary>
        /// <param name="schemaObjPriv">The privilege to compare with this instance.</param>
        /// <returns>
        /// Less than zero means this object is less than priv.
        /// Zero means that this object is the same a priv.
        /// Greater than zero means that this object is greater than priv.
        /// </returns>
        public int CompareTo(SchemaObjectPrivilege schemaObjPriv)
        {
            // First, compare basic privilege info.
            return CompareTo(schemaObjPriv as Privilege);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public string DisplayPrivileges()
        {
            // Walk through the privilege list and return a single string.
            string ddlPrivList = "";
            string dmlPrivList = "";
            string utilityPrivList = "";

            if (_isAdministrator)
            {
                return (Properties.Resources.AllPrivileges);
            }

            // Now, for the DDL privilege first
            bool first = true;
            _ddlPrivileges.Sort();
            foreach (string priv in _ddlPrivileges)
            {
                if (first)
                {
                    ddlPrivList += String.Format("{0}", TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(priv));
                    first = false;
                }
                else
                {
                    ddlPrivList += String.Format(", {0}", TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(priv));
                }
            }

            // The DML privilege will follow
            first = true;
            _dmlPrivileges.Sort();
            foreach (string priv in _dmlPrivileges)
            {
                if (first)
                {
                    dmlPrivList += String.Format("{0}", TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(priv));
                    first = false;
                }
                else
                {
                    dmlPrivList += String.Format(", {0}", TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(priv));
                }
            }

            // Utility Privileges
            _utilityPrivileges.Sort();
            for (int i = 0; i < _utilityPrivileges.Count; i++)
            {
                string utilityPrivilege = TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(_utilityPrivileges[i]);
                string formatString = i == 0 ? "{0}" : ", {0}";
                utilityPrivList += String.Format("{0}", utilityPrivilege);
            }


            return TrafodionPrivilegeTypeHelper.BuildPrivilegeSequence(ddlPrivList, dmlPrivList, utilityPrivList);
        }
    }
}
