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
    /// The privileges applied to a column.
    /// </summary>
    public class ColumnPrivilege : Privilege, IComparable<ColumnPrivilege> //, IComparable<SchemaObjectPrivilege>
    {
        /// <summary>
        /// A string that represents a reference permission.
        /// </summary>
        public const string REFERENCE_TYPE = TrafodionPrivilegeTypeHelper.TYPE_REFERENCE;

        /// <summary>
        /// A string that represents a reference permission.
        /// </summary>
        public const string UPDATE_TYPE = TrafodionPrivilegeTypeHelper.TYPE_UPDATE;


        /// <summary>
        /// A string that represents a select permission.
        /// </summary>
        public const string SELECT_TYPE = TrafodionPrivilegeTypeHelper.TYPE_SELECT;

        /// <summary>
        /// A string that represents a insert permission.
        /// </summary>
        public const string INSERT_TYPE = TrafodionPrivilegeTypeHelper.TYPE_INSERT;

        private string _columnName = "";
        private TrafodionColumn.ColumnClass _columnClass = TrafodionColumn.ColumnClass.UserClass;
        private int _columnNumber = -1;
        private bool _reference = false;
        private bool _update = false;
        private bool _insert = false;
        private bool _select = false;

        #region Properties

        /// <summary>
        /// The name of the column to which this privilege applies.
        /// </summary>
        public string ColumnName
        {
            get { return _columnName; }
        }

        /// <summary>
        /// The column's class type.
        /// </summary>
        public TrafodionColumn.ColumnClass ColumnClass
        {
            get { return _columnClass; }
        }

        /// <summary>
        /// The column's number.
        /// </summary>
        public int ColumnNumber
        {
            get { return _columnNumber; }
        }

        /// <summary>
        /// Indicates if the user has the reference privilege.
        /// </summary>
        public bool Reference
        {
            get { return _reference; }
            protected set { _reference = value; }
        }

        /// <summary>
        /// Indicates if the user has update privilege.
        /// </summary>
        public bool Update
        {
            get { return _update; }
            protected set { _update = value; }
        }

        /// <summary>
        /// Indicates if the user has insert privilege.
        /// </summary>
        public bool Insert
        {
            get { return _insert; }
            protected set { _insert = value; }
        }

        /// <summary>
        /// Indicates if the user has select privilege.
        /// </summary>
        public bool Select
        {
            get { return _select; }
            protected set { _select = value; }
        }
        #endregion

        /// <summary>
        /// Creates a new set of column privileges based on a privilege type.
        /// </summary>
        /// <param name="columnName">The name of the column.</param>
        /// <param name="columnClass">The column's class.</param>
        /// <param name="columnNumber">The column's number.</param>
        /// <param name="type">
        /// A string representation of a privilege. It must be one of the privilege types
        /// defined for either this or the SchemaObjectPrivilege class.
        /// </param>
        /// <param name="grantable">Indicates that the user has the ability to grantable privileges to other users.</param>
        /// <param name="grantor">The user's id which granted these permissions.</param>
        /// <param name="grantorType">The grantor's user type.</param>
        /// <param name="grantee">The user's id which was granted the permissions.</param>
        /// <param name="granteeType">The grantee's user type.</param>
        public ColumnPrivilege(string columnName, TrafodionColumn.ColumnClass columnClass, int columnNumber, string type, bool grantable, int grantor, string grantorName, string grantorType,
            int grantee, string granteeName, string granteeType)
            : base(grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType)
        {
            _columnName = columnName;
            _columnClass = columnClass;
            _columnNumber = columnNumber;

            if (type.Equals(REFERENCE_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _reference = true;
            }
            else if (type.Equals(UPDATE_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _update = true;
            }
            else if (type.Equals(INSERT_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _insert = true;
            }
            else if (type.Equals(SELECT_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _select = true;
            }
        }

        /// <summary>
        /// Creates a new set of column privileges based on individual privilege settings.
        /// </summary>
        /// <param name="columnName">The name of the column.</param>
        /// <param name="reference">Indicates if the user has reference permission.</param>
        /// <param name="update">Indicates if the user has update permission.</param>
        /// <param name="grantable">Indicates that the user has the ability to grantable privileges to other users.</param>
        /// <param name="grantor">The user's id which granted these permissions.</param>
        /// <param name="grantorType">The grantor's user type.</param>
        /// <param name="grantee">The user's id which was granted the permissions.</param>
        /// <param name="granteeType">The grantee's user type.</param>
        public ColumnPrivilege(string columnName, bool reference, bool update, bool insert, bool select, bool grantable, int grantor,
            string grantorName, string grantorType, int grantee, string granteeName, string granteeType)
            : base(grantable, grantor, grantorName, grantorType, grantee, granteeName, granteeType)
        {
            _columnName = columnName;
            _reference = reference;
            _update = update;
            _insert = insert;
            _select = select;
        }

        /// <summary>
        /// Adds privileges based on the type of privilege specified.
        /// <seealso cref="REFERENCE_TYPE"/>
        /// <seealso cref="UPDATE_TYPE"/>
        /// </summary>
        /// <param name="type">
        /// A string representation of a privilege. It must be one of the privilege types
        /// defined for this class.
        /// </param>
        public void AddPrivilegesByTypeString(string type)
        {
            if (type.Equals(REFERENCE_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _reference = true;
            }
            else if (type.Equals(UPDATE_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _update = true;
            }
            else if (type.Equals(INSERT_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _insert = true;
            }
            else if (type.Equals(SELECT_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                _select = true;
            }
        }

        public bool DoesPrivilegExist(string type)
        {
            if (type.Equals(REFERENCE_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                return _reference;
            }
            else if (type.Equals(UPDATE_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                return _update;
            }
            else if (type.Equals(INSERT_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                return _insert;
            }
            else if (type.Equals(SELECT_TYPE, StringComparison.OrdinalIgnoreCase))
            {
                return _select;
            }
            return false;
        }
        /// <summary>
        /// Compares this ColumnPrivilege to another instance.
        /// </summary>
        /// <param name="colPriv">The privilege to compare with this instance.</param>
        /// <returns>
        /// Less than zero means this object is less than priv.
        /// Zero means that this object is the same a priv.
        /// Greater than zero means that this object is greater than priv.
        /// </returns>
        public int CompareTo(ColumnPrivilege colPriv)
        {
            // First, compare basic privilege info.
            int order = CompareTo(colPriv as Privilege);
            if (order != 0)
            {
                return order;
            }

            // Second, compare column number
            order = ColumnNumber.CompareTo(colPriv.ColumnNumber);
            if (order != 0)
            {
                return order;
            }

            // Third, compare reference.
            if (Reference != colPriv.Reference)
            {
                order = (Reference ? -1 : 1);
                return order;
            }

            // Then, compare update.
            if (Update != colPriv.Update)
            {
                order = (Update ? -1 : 1);
                return order;
            }

            // Then, compare insert.
            if (Insert != colPriv.Insert)
            {
                order = (Insert ? -1 : 1);
                return order;
            }

            // Lastly, compare select.
            if (Select != colPriv.Select)
            {
                order = (Select ? -1 : 1);
                return order;
            }

            // We're the same!
            return 0;
        }
    }
}
