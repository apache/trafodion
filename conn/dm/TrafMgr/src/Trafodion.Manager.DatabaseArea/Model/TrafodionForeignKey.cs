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

using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Foreign Key Constraint
    /// </summary>
    public class TrafodionForeignKey : TrafodionTableConstraint
    {
        // Member Variables
        private TrafodionTable _foreignTable;
        private List<ForeignKeyLocalColumn> _theForeignKeyLocalColumns = new List<ForeignKeyLocalColumn>();
        private List<ForeignKeyForeignColumn> _theForeignKeyForeignColumns = new List<ForeignKeyForeignColumn>();
        private bool _enforced;

        /// <summary>
        /// Constructs a Foreign key constraint
        /// </summary>
        /// <param name="anInternalName">Constraint name</param>
        /// <param name="anUID">UID</param>
        /// <param name="aTrafodionTable">Table for which this primary key is defined</param>
        public TrafodionForeignKey(string anInternalName, long anUID, bool aEnforced, TrafodionTable aTrafodionTable)
            : base(anInternalName, anUID, aTrafodionTable)
        {
          _enforced = aEnforced;
          
        }

        /// <summary>
        /// Get Enforced attribute associated with this foreign key
        /// </summary>
        public bool Enforced
        {
            get { return _enforced; }
            
        }

        /// <summary>
        /// Get and set the foreign table associated with this foreign key
        /// </summary>
        public TrafodionTable ForeignTable
        {
            get { return _foreignTable; }
            set { _foreignTable = value; }
        }

        /// <summary>
        /// Get and set the list of local foreign key columns
        /// </summary>
        public List<ForeignKeyLocalColumn> TheForeignKeyLocalColumns
        {
            get { return _theForeignKeyLocalColumns; }
            set { _theForeignKeyLocalColumns = value; }
        }

        /// <summary>
        /// Get and set the list of columns from the foreign table
        /// </summary>
        public List<ForeignKeyForeignColumn> TheForeignKeyForeignColumns
        {
            get { return _theForeignKeyForeignColumns; }
            set { _theForeignKeyForeignColumns = value; }
        }

        /// <summary>
        /// Stores the local foreign key column
        /// </summary>
        public class ForeignKeyLocalColumn
        {
            private string _columnName;
            private int _columnNumber;

            /// <summary>
            /// Constructor for the local foreign key column
            /// </summary>
            /// <param name="aColumnName">the name of the column</param>
            /// <param name="aColumnNumber">the number of the column</param>
            public ForeignKeyLocalColumn(string aColumnName, int aColumnNumber)
            {
                _columnName = aColumnName;
                _columnNumber = aColumnNumber;
            }

            /// <summary>
            /// Returns the number of the column
            /// </summary>
            public int ColumnNumber
            {
                get { return _columnNumber; }
            }

            /// <summary>
            /// Returns the name of the column
            /// </summary>
            public string ColumnName
            {
                get { return _columnName; }
            }

        }

        /// <summary>
        /// Stores the foreign column in the foreign key
        /// </summary>
        public class ForeignKeyForeignColumn
        {
            private string _columnName;
            private int _columnNumber;

            /// <summary>
            /// Constructor for the foreign key column
            /// </summary>
            /// <param name="aColumnName"></param>
            /// <param name="aColumnNumber"></param>
            public ForeignKeyForeignColumn(string aColumnName, int aColumnNumber)
            {
                _columnName = aColumnName;
                _columnNumber = aColumnNumber;
            }

            /// <summary>
            /// Returns the column number
            /// </summary>
            public int ColumnNumber
            {
                get { return _columnNumber; }
            }

            /// <summary>
            /// Returns the column name
            /// </summary>
            public string ColumnName
            {
                get { return _columnName; }
            }

        }


    }

}
