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
using System.Data;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.SecurityArea.Model
{
    public class Role : SecurityObject
    {
        #region Private variables
        
        string _name;
        string _owner;
        DateTime _createTimestamp;
        string _password;
        long _grantCount;
        long _defaultRoleGrantCount;
        int _expirationDays;
        string _expirationDate;

        //the bits for the bit vector
        public const int Old_Password_Bit = 1 << 0;
        public const int Password_Bit = 1 << 1;
        public const int ExpirationDate_Bit = 1 << 2;
        public const int ExpirationDays_Bit = 1 << 3;


        #endregion Private variables

        #region Public properties

        public string Name
        {
            get { return _name; }
        }

        public string Owner
        {
            get { return _owner; }
            set { _owner = value; }
        }

        public String FormattedCreateTimestamp
        {
            get
            {
                return _createTimestamp == null ? "Not Available" : Trafodion.Manager.Framework.Utilities.GetFormattedDateTime(_createTimestamp);
            }
        }
        public DateTime CreateTimestamp
        {
            get { return _createTimestamp; }
            set { _createTimestamp = value; }
        }

        public long GrantCount
        {
            get { return _grantCount; }
            set { _grantCount = value; }
        }

        public long DefaultRoleGrantCount
        {
            get { return _defaultRoleGrantCount; }
            set { _defaultRoleGrantCount = value; }
        }

        public string Password
        {
            get { return _password; }
            set { _password = value; }
        }

        public int ExpirationDays
        {
            get { return _expirationDays; }
            set { _expirationDays = value; }
        }

        public string ExpirationDate
        {
            get { return _expirationDate; }
            set { _expirationDate = value; }
        }

        #endregion Public properties

        public Role(string aName, ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            _name = aName;
        }

        public Role(Role sourceRole)
            : base(sourceRole.ConnectionDefinition)
        {
            _name = sourceRole.Name;
            _owner = sourceRole.Owner;
            _grantCount = sourceRole.GrantCount;
            _defaultRoleGrantCount = sourceRole.DefaultRoleGrantCount;
            _createTimestamp = sourceRole.CreateTimestamp;
            _password = sourceRole.Password;
            _expirationDate = sourceRole.ExpirationDate;
            _expirationDays = sourceRole.ExpirationDays; 
        }

        public void Add()
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteAddRole(CurrentConnection, _name);
            }
            finally
            {
                CloseConnection();
            }
        }

        public void Delete()
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteDeleteRole(CurrentConnection, _name);
            }
            finally
            {
                CloseConnection();
            }
        }

        public void Alter(Role originalRole, string oldPassword)
        {
            if (!GetConnection())
                return;

            try
            {
                int attrVector = GetAttributeVectorForAlter(originalRole, this, oldPassword);

                Queries.ExecuteAlterRole(CurrentConnection, this, attrVector, oldPassword);
            }
            finally
            {
                CloseConnection();
            }
        }

        public void Grant(string userName)
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteGrantRole(CurrentConnection, userName, Name);

            }
            finally
            {
                CloseConnection();
            }
        }

        public DataTable GrantMultiple(List<string> userNames)
        {
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("User Name");
            errorTable.Columns.Add("Message");

            if (!GetConnection())
                return errorTable;

            try
            {
                foreach (string userName in userNames)
                {
                    try
                    {
                        Queries.ExecuteGrantRole(CurrentConnection, userName, Name);
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { userName, ex.Message });
                    }
                }
            }
            finally
            {
                CloseConnection();
            }

            return errorTable;
        }

        public void Revoke(string userName)
        {
            if (!GetConnection())
                return;

            try
            {
                Queries.ExecuteRevokeRole(CurrentConnection, userName, Name);

            }
            finally
            {
                CloseConnection();
            }

        }

        public void GetRoleDetails()
        {
            if (!GetConnection())
                return;
            
            OdbcDataReader reader = null;
            try
            {
                reader = Queries.GetRoleDetails(CurrentConnection, Name);
                if (reader.Read())
                {
                    _grantCount = reader.GetInt32(0);
                    _defaultRoleGrantCount = reader.GetInt32(1);
                }
            }
            finally
            {
                if (reader != null)
                {
                    reader.Close();
                }
                CloseConnection();
            }
        }

        /// <summary>
        /// Gets the current password attributes
        /// </summary>
        public void GetPasswordAttributes()
        {
            if (!GetConnection())
                return;

            try
            {
                string expiryDaysString = "";
                Queries.ExecuteGetSysAttributeForRole(CurrentConnection, this);
            }
            finally
            {
                CloseConnection();
            }
        }


        /// <summary>
        /// Computes the attribute vector for the parameters of alter
        /// </summary>
        /// <param name="originalUser"></param>
        /// <param name="changedUser"></param>
        /// <returns></returns>
        public static int GetAttributeVectorForAlter(Role originalRole, Role alteredRole, string oldPassword)
        {
            int attrVector = 0;

            attrVector = (oldPassword.Length > 0) ? (attrVector | Old_Password_Bit) : attrVector;
            attrVector = (alteredRole.Password.Length > 0) ? (attrVector | Password_Bit) : attrVector;
            attrVector = (originalRole.ExpirationDays != alteredRole.ExpirationDays) ? (attrVector | ExpirationDays_Bit) : attrVector;
            attrVector = User.AreDatesDifferent(originalRole.ExpirationDate, alteredRole.ExpirationDate) ? (attrVector | ExpirationDate_Bit) : attrVector;

            return attrVector;
        }

    }
}
