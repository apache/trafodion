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
using System.Text;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Describes the privileges that apply to a schema.
    /// </summary>
    public class SchemaPrivilege : Privilege, IComparable<SchemaPrivilege>
    {

        #region Fields

        private List<string> _dmlPrivileges = new List<string>();
        private List<string> _ddlPrivileges = new List<string>();
        private List<string> _utilityPrivileges = new List<string>();

        private bool _isAdministrator = false;

        #endregion

        /// <summary>
        /// Creates a new set of schema privileges based on a privilege type.
        /// </summary>
        /// <param name="type">
        /// A string representation of a privilege. It must be one of the privilege types
        /// defined for either this or the SchemaObjectPrivilege class.
        /// </param>
        /// <param name="grantable">
        /// Indicates that the user has the ability to grantable privileges to other users.
        /// </param>
        /// <param name="grantor">The user's id which granted these permissions.</param>
        /// <param name="grantorType">The grantor's user type.</param>
        /// <param name="grantee">The user's id which was granted the permissions.</param>
        /// <param name="granteeType">The grantee's user type.</param>
        public SchemaPrivilege(string type, bool grantable, int grantor, string grantorName, string grantorType,
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
        public void AddPrivilegesByTypeString(string type)
        {
            if (TrafodionPrivilegeTypeHelper.IsSchemaPrivilege(type))
            {
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
        /// Does the grantee has Alter All privilege?
        /// </summary>
        /// <returns></returns>
        public bool HasAlterAllPrivileges()
        {
            return _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER) ||
                (
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_TABLE) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_TRIGGER) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_MV) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_MV_GROUP) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_VIEW) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_SYNONYM) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_ALTER_LIBRARY)
                );
        }

        /// <summary>
        /// Does the grantee has Create All privilege?
        /// </summary>
        /// <returns></returns>
        public bool HasCreateAllPrivileges()
        {
            return _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE) ||
                (
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_TABLE) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_TRIGGER) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_MV) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_MV_GROUP) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_VIEW) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_PROCEDURE) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_SYNONYM) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_CREATE_LIBRARY)
                );
        }

        /// <summary>
        /// Does the grantee has Drop All privilege?
        /// </summary>
        /// <returns></returns>
        public bool HasDropAllPrivileges()
        {
            return _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP) ||
                (
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_TABLE) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_TRIGGER) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_MV) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_MV_GROUP) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_VIEW) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_PROCEDURE) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_SYNONYM) &&
                    _ddlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DROP_LIBRARY)
                );
        }

        public bool HasAllDDLPrivileges()
        {
            return HasAlterAllPrivileges() && HasCreateAllPrivileges() && HasDropAllPrivileges();
        }

        /// <summary>
        /// Check if the grantee has all DML privileges
        /// </summary>
        /// <returns></returns>
        public bool HasAllDMLPrivileges()
        {
            return  _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_SELECT) &&
                    _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_INSERT) &&
                    _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_UPDATE) &&
                    _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_DELETE) &&
                    _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_EXECUTE) &&
                    _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_REFERENCE) &&
                    _dmlPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_USAGE);
        }

        /// <summary>
        /// Check if the grantee has ALL_UTILITY privilege
        /// </summary>
        /// <returns></returns>
        public bool HasAllUtilityPrivileges()
        {
            return _utilityPrivileges.Contains(TrafodionPrivilegeTypeHelper.TYPE_REPLICATE);
        }

        /// <summary>
        /// Return string for displaying schema priv types for schema tab only
        /// </summary>
        /// <returns></returns>
        public string DisplayPrivileges()
        {
            return DisplayPrivileges("");
        }

        /// <summary>
        /// Check if the specified privilege exists in any kind of privilleges list
        /// </summary>
        /// <param name="privilegeType"></param>
        /// <returns></returns>
        public bool HasPrivilege(string privilegeType)
        {
            return  _ddlPrivileges.Contains(privilegeType) ||
                    _dmlPrivileges.Contains(privilegeType) ||
                    _utilityPrivileges.Contains(privilegeType);
        }

        /// <summary>
        /// Return the displayable string for other object tabs
        /// </summary>
        /// <returns></returns>
        public string DisplayPrivileges(string aObjectType)
        {

            // Walk through the privilege list and return a single string.
            string ddlPrivList = "";
            string dmlPrivList = "";
            string utilityPrivList = "";

            if (_isAdministrator)
            {
                return (Properties.Resources.AllPrivileges);
            }

            if (HasAllDDLPrivileges())
            {
                ddlPrivList = Properties.Resources.AllDDL;
            }
            else
            {
                // Now, for the DDL privilege first
                bool first = true;
                _ddlPrivileges.Sort();
                foreach (string priv in _ddlPrivileges)
                {
                    switch (aObjectType)
                    {
                        case "": // schema object type
                            {
                                if (HasAlterAllPrivileges() && TrafodionPrivilegeTypeHelper.IsAlterPrivilege(priv))
                                {
                                    // skip alter specific object type if alter all is present
                                    continue;
                                }

                                if (HasCreateAllPrivileges() && TrafodionPrivilegeTypeHelper.IsCreatePrivilege(priv))
                                {
                                    // skip create specific object type if create all is present
                                    continue;
                                }

                                if (HasDropAllPrivileges() && TrafodionPrivilegeTypeHelper.IsDropPrivilege(priv))
                                {
                                    // skip drop specific object type if drop all is present
                                    continue;
                                }
                            }
                            break;

                        case TrafodionTable.ObjectType:
                            {
                                if (!TrafodionPrivilegeTypeHelper.IsTablePrivilege(priv))
                                {
                                    continue;
                                }

                                if (HasAlterAllPrivileges() && TrafodionPrivilegeTypeHelper.IsAlterPrivilege(priv))
                                {
                                    // skip alter specific object type if alter all is present
                                    continue;
                                }
                            }
                            break;

                        case TrafodionMaterializedView.ObjectType:
                            {
                                if (!TrafodionPrivilegeTypeHelper.IsMVPrivilege(priv))
                                {
                                    continue;
                                }

                                if (HasAlterAllPrivileges() && TrafodionPrivilegeTypeHelper.IsAlterPrivilege(priv))
                                {
                                    // skip alter specific object type if alter all is present
                                    continue;
                                }
                            }
                            break;

                        case TrafodionView.ObjectType:
                            {
                                if (!TrafodionPrivilegeTypeHelper.IsViewPrivilege(priv))
                                {
                                    continue;
                                }

                                if (HasAlterAllPrivileges() && TrafodionPrivilegeTypeHelper.IsAlterPrivilege(priv))
                                {
                                    // skip alter specific object type if alter all is present
                                    continue;
                                }
                            }
                            break;

                        case TrafodionProcedure.ObjectType:
                            {
                                if (!TrafodionPrivilegeTypeHelper.IsProcedurePrivilege(priv))
                                {
                                    continue;
                                }
                            }
                            break;

                        default:
                            continue;
                    }

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
            }


            if (HasAllDMLPrivileges())
            {
                dmlPrivList = Properties.Resources.AllDML;
            }
            else 
            {
                // The DML privilege will follow
                bool firstPriv = true;
                _dmlPrivileges.Sort();
                foreach (string priv in _dmlPrivileges)
                {
                    switch (aObjectType)
                    {
                        case "": // schema object type
                            break;

                        case TrafodionTable.ObjectType:
                        case TrafodionMaterializedView.ObjectType:
                        case TrafodionView.ObjectType:
                            if (!TrafodionPrivilegeTypeHelper.IsCommonPrivilege(priv))
                            {
                                continue;
                            }
                            break;

                        case TrafodionProcedure.ObjectType:
                            if (!TrafodionPrivilegeTypeHelper.IsProcedurePrivilege(priv))
                            {
                                continue;
                            }
                            break;

                        default:
                            continue;
                    }

                    if (firstPriv)
                    {
                        dmlPrivList += String.Format("{0}", TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(priv));
                        firstPriv = false;
                    }
                    else
                    {
                        dmlPrivList += String.Format(", {0}", TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(priv));
                    }
                }
            }

            /*
             * Utility Privilege
             */
            if (HasAllUtilityPrivileges())
            {
                utilityPrivList = Properties.Resources.AllUtility;
            }
            else
            {
                _utilityPrivileges.Sort();
                for (int i = 0; i < _utilityPrivileges.Count; i++)
                {
                    string utilityPrivilege = TrafodionPrivilegeTypeHelper.DisplayPrivilegeType(_utilityPrivileges[i]);
                    string formatString = i == 0 ? "{0}" : ", {0}";
                    utilityPrivList += String.Format("{0}", utilityPrivilege);
                }
            }

            return TrafodionPrivilegeTypeHelper.BuildPrivilegeSequence(ddlPrivList, dmlPrivList, utilityPrivList);
        }

        /// <summary>
        /// CompareTo to fullfil IComparable. 
        /// Note: This does not compare the priv list, it only compare the grantee/grantor/grant opt. 
        /// </summary>
        /// <param name="schemaPriv"></param>
        /// <returns></returns>
        public int CompareTo(SchemaPrivilege schemaPriv)
        {
            // First, compare basic privilege info.
            return CompareTo(schemaPriv as Privilege);
        }

        public List<string> GetPrivilegesForGrantee(string granteeName)
        {
            List<string> privileges = new List<string>();
            if (this.GranteeName.Equals(granteeName))
            {
                privileges.AddRange(_ddlPrivileges.ToArray());
                privileges.AddRange(_dmlPrivileges.ToArray());
                privileges.AddRange(_utilityPrivileges.ToArray());
                if (_isAdministrator)
                    privileges.Add(TrafodionPrivilegeTypeHelper.TYPE_ADMINISTRATOR);
            }
            return privileges;
        }
    }

}
