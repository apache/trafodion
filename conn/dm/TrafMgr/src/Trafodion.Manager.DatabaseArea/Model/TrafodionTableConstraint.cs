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

using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// A Table Constraint
    /// </summary>
    public class TrafodionTableConstraint : TrafodionObject
    {
        /// <summary>
        /// Constructs a table constraint
        /// </summary>
        /// <param name="anInternalName">Name of the constraint</param>
        /// <param name="anUID">UID of the constraint</param>
        /// <param name="aTrafodionTable">Table on which this constraint is defined</param>
        public TrafodionTableConstraint(string anInternalName, long anUID, TrafodionTable aTrafodionTable)
            :base(anInternalName, anUID)
        {
            _sqlMxTable = aTrafodionTable;
        }

        private TrafodionObject _sqlMxTable;
        /// <summary>
        /// The table on which this constraint is defined
        /// </summary>
        public TrafodionObject TrafodionTable
        {
            get { return _sqlMxTable; }
        }
        /// <summary>
        /// Gets a connection object
        /// </summary>
        /// <returns></returns>
        public override Connection GetConnection()
        {
            return TrafodionTable.GetConnection();
        }

        private char _type;
        /// <summary>
        /// The Type of the constraint
        /// </summary>
        public char Type
        {
            get { return _type; }
            set { _type = value; }
        }

        private string _objectType;
        /// <summary>
        /// The object type of the constraint
        /// </summary>
        public string ObjectType
        {
            get { return _objectType; }
            set { _objectType = value; }
        }

        private bool _canDropPrimaryKey;
        /// <summary>
        /// Indicates if the constraint can be dropped
        /// </summary>
        public bool CanDropPrimaryKey
        {
            get { return _canDropPrimaryKey; }
            set { _canDropPrimaryKey = value; }
        }
       
        private long _indexUID;
        /// <summary>
        /// Index UID of this constraint
        /// </summary>
        public long IndexUID
        {
            get { return _indexUID; }
            set { _indexUID = value; }
        }
        /// <summary>
        /// The connection definition associated with this constraint
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TrafodionTable.ConnectionDefinition; }
        }
    }
}
