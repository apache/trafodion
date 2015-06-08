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
using System.Linq;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Base class for schema level objects
    /// </summary>
    abstract public class TrafodionSchemaObject : TrafodionObject, IHasTrafodionCatalog, IHasTrafodionSchema, IHasTrafodionPrivileges
    {
        #region Fields

        private TrafodionSchema _sqlMxSchema;
        private long _createTime;
        private long _redefTime;
        private string _securityClass;
        private string _owner;
        private List<SchemaObjectPrivilege> _schemaObjectPrivileges = null;
        private List<TrafodionDivisionByColumnDef> _sqlMxDivisionByColumnDefs = null;

        #endregion

        /// <summary>
        /// Constructs a Schema level Object
        /// </summary>
        /// <param name="aTrafodionSchema">Parent Schema</param>
        /// <param name="anInternalName">Internal name of the schema level object</param>
        /// <param name="aUID">UID of the object</param>
        /// <param name="aCreateTime">Create time</param>
        /// <param name="aRedefTime">Redefinition time</param>
        /// <param name="aSecurityClass">Security class</param>
        /// <param name="anOwner">Owner of the object</param>
        public TrafodionSchemaObject(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(anInternalName, aUID)
        {
            _sqlMxSchema = aTrafodionSchema;
            _createTime = aCreateTime;
            _redefTime = aRedefTime;
            _securityClass = aSecurityClass;
            _owner = anOwner;
        }

        /// <summary>
        /// Lets the model reset its state
        /// </summary>
        public override void Refresh()
        {
            base.Refresh();
            _schemaObjectPrivileges = null;
            _sqlMxDivisionByColumnDefs = null;
        }

        /// <summary>
        /// List of store order column defs on this table
        /// </summary>
        public List<TrafodionDivisionByColumnDef> TheTrafodionDivisionByColumnDefs
        {
            get
            {
                if (_sqlMxDivisionByColumnDefs == null)
                {
                    _sqlMxDivisionByColumnDefs = new TrafodionDivisionByColumnDefsLoader().Load(this);
                }
                return _sqlMxDivisionByColumnDefs;
            }
        }

        #region IHasTrafodionCatalog

        /// <summary>
        /// The catalog that this object belongs to
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return _sqlMxSchema.TheTrafodionCatalog; }
        }

        #endregion



        #region IHasTrafodionSchema

        /// <summary>
        /// The schema that this object belongs to
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get { return _sqlMxSchema; }
        }

        #endregion

        /// <summary>
        /// User who created the object
        /// </summary>
        public string Owner
        {
            get { return _owner; }
        }

        /// <summary>
        /// DateTime when the object was created
        /// </summary>
        public long TheCreateTime
        {
            get { return _createTime; }
        }
        /// <summary>
        /// DateTime when the object was modified
        /// </summary>
        public long TheRedefTime
        {
            get { return _redefTime; }
        }
        /// <summary>
        /// Connection definition associated with this object
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return _sqlMxSchema.ConnectionDefinition; }
        }
        /// <summary>
        /// Indicates if the object is a metadata object. Subclasses should override this property
        /// </summary>
        virtual public bool IsMetadataObject
        {
            get { return false; }
        }
        /// <summary>
        /// Security class
        /// </summary>
        public string TheSecurityClass
        {
            get { return _securityClass; }
        }
        /// <summary>
        /// Returns a connection
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return TheTrafodionSchema.GetConnection();
        }
        /// <summary>
        /// Object type e.g. "BT" for base table 
        /// </summary>
        abstract public string SchemaObjectType
        {
            get;
        }
        /// <summary>
        /// Object name space e.g. "TA" for Table-value-object: table, view etc.
        /// </summary>
        abstract public string SchemaObjectNameSpace
        {
            get;
        }
        /// <summary>
        /// The displayable object type for the schema object. e.g. "Table" for sql tables.
        /// </summary>
        abstract public string DisplayObjectType
        {
            get;
        }
        /// <summary>
        /// Real 3 part ansi name
        /// </summary>
        override public string RealAnsiName
        {
            get
            {
                // Real is always a three part name
                return _sqlMxSchema.TheTrafodionCatalog.ExternalName + "." + _sqlMxSchema.ExternalName + "." + ExternalName;
            }
        }
        /// <summary>
        /// If trafodion user, 2 part ansi name or else 3 part ansi name
        /// </summary>
        override public string VisibleAnsiName
        {
            get
            {
                return _sqlMxSchema.TheTrafodionCatalog.ExternalName + "." + _sqlMxSchema.ExternalName + "." + ExternalName;
            }
        }

        public void ClearPrivileges()
        {
            if (_schemaObjectPrivileges != null)
            {
                _schemaObjectPrivileges.Clear();
                _schemaObjectPrivileges = null;
            }
        }

        /// <summary>
        /// The privileges of this schema object.
        /// </summary>
        virtual public List<SchemaObjectPrivilege> Privileges
        {
            get
            {
                if (_schemaObjectPrivileges == null)
                {
                    _schemaObjectPrivileges = PrivilegesLoader.LoadSchemaObjectPrivileges(this);
                }
                return _schemaObjectPrivileges;
            }
        }
        /// <summary>
        /// Checks if the given user has the specificed privilege type on this object
        /// </summary>
        /// <param name="userName">User name</param>
        /// <param name="privilegeType">Privilege Type</param>
        /// <returns>True or False</returns>
        public bool DoesUserHaveObjectPrivilege(string userName, string privilegeType)
        {
            SchemaObjectPrivilege[] userObjectPrivileges = (from priv in Privileges where priv.DMLPrivileges.Contains(privilegeType) && (priv.GranteeName.Equals(userName) || priv.GranteeName.ToUpper().Equals("PUBLIC")) select priv).Distinct().ToArray();
            return userObjectPrivileges.Length > 0;
        }

        #region FORMATTED
        /// <summary>
        /// Formatted create time for display
        /// </summary>
        /// <returns>Create time-stamp as a string </returns>
        public JulianTimestamp FormattedCreateTime()
        {
            return new JulianTimestamp(_createTime);
        }
        /// <summary>
        /// Formatted redefinition time for display
        /// </summary>
        /// <returns>Create redefinition-stamp as a string</returns>
        public JulianTimestamp FormattedRedefTime()
        {
            return new JulianTimestamp(_redefTime);
        }

        #endregion

        #region IHasTrafodionPrivileges Members


        public void GrantRevoke(TrafodionObject.PrivilegeAction action, ConnectionDefinition aConnectionDefinition, string grantRevokeCommandString, out DataTable sqlWarnings)
        {
            sqlWarnings = new DataTable();
            if (SupportsPrivileges(this))
            {
#if DEBUG
                Console.WriteLine(grantRevokeCommandString);
#endif
                Trafodion.Manager.DatabaseArea.Model.Queries.ExecuteGrantRevoke(aConnectionDefinition, grantRevokeCommandString, out sqlWarnings);
                if (_schemaObjectPrivileges != null)
                {
                    _schemaObjectPrivileges.Clear();
                    _schemaObjectPrivileges = null;
                    if (this is IHasTrafodionColumns)
                    {
                        ((IHasTrafodionColumns)this).ResetColumnPrivileges();
                    }
                }
                if (action == PrivilegeAction.GRANT)
                {
                    OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.PrivilegeGranted, this));
                }
                else
                {
                    OnModelChangedEvent(new TrafodionModelChangeEventArgs(ChangeEvent.PrivilegeRevoked, this));
                }
            }
        }

        #endregion

        #region TrafodionDivisionByColumnDefsLoader

        class TrafodionDivisionByColumnDefsLoader : TrafodionObjectsLoader<TrafodionSchemaObject, TrafodionDivisionByColumnDef>
        {
            /// <summary>
            /// Returns the ODBC Reader that contains the result of a query to get the list of Division By Keys
            /// </summary>
            /// <param name="aConnection">an ODBC Connection</param>
            /// <param name="aTrafodionSchemaObject">a TrafodionSchemaObject</param>
            /// <returns>returns an ODBCDataReader with data from the query</returns>
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionSchemaObject aTrafodionSchemaObject)
            {
                return Queries.ExecuteSelectDivisionByColumnDefs(aConnection, aTrafodionSchemaObject);
            }

            /// <summary>
            /// Overrides the load method to division by key  definitions to the list
            /// </summary>
            /// <param name="aList">the list of Division By Key Definitions</param>
            /// <param name="aTrafodionSchemaObject">the TrafodionSchemaObject that is supplying data</param>
            /// <param name="aReader">the Reader that contains the data to be loaded into the list</param>
            override protected void LoadOne(List<TrafodionDivisionByColumnDef> aList, TrafodionSchemaObject aTrafodionSchemaObject, OdbcDataReader aReader)
            {
                aList.Add(new TrafodionDivisionByColumnDef(aTrafodionSchemaObject,
                    aReader.GetInt32(0), //sequence number or position
                    aReader.GetInt32(1),  //column number
                    aReader.GetString(2).TrimEnd(), //column name
                    aReader.GetString(3).Trim(),//ordering
                    aReader.GetString(4).Trim(),//expression
                    aReader.GetString(5).Trim() //is system added indicator
                    ));
            }
        }

        #endregion TrafodionDivisionByColumnDefsLoader

    }
}
