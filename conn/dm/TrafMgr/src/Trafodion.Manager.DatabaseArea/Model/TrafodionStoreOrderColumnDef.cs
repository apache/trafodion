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
    /// A column definition for the store order
    /// </summary>
    public class TrafodionStoreOrderColumnDef : TrafodionObject
    {

        // Member variables
        private TrafodionTable _sqlMxTable;
        private bool _theSystemAddedColumn;
        private bool _isAscending;
        private int _positionInRow;

        /// <summary>
        /// Creates a new instance of a TrafodionStoreOrderColumnDef
        /// </summary>
        /// <param name="aTrafodionTable">the TrafodionTable</param>
        /// <param name="anExternalName">the External Name</param>
        /// <param name="aPositionInRow">the row position</param>
        /// <param name="anOrdering">the Ordering type</param>
        /// <param name="aSystemAddedColumn">whether the column was system added</param>       
        public TrafodionStoreOrderColumnDef(TrafodionTable aTrafodionTable, string anExternalName, int aPositionInRow, string anOrdering, string aSystemAddedColumn)
            :base(anExternalName, aTrafodionTable.UID)
        {
            ExternalName = anExternalName;
            _sqlMxTable = aTrafodionTable;
            _isAscending = anOrdering.Trim().Equals("A");
            _theSystemAddedColumn = aSystemAddedColumn.Trim().Equals("Y");
            _positionInRow = aPositionInRow;
        }

        /// <summary>
        /// Returns the TrafodionTable
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return _sqlMxTable; }            
        }

        /// <summary>
        /// Returns the ordering of the TrafodionStoreOrderColumnDef
        /// </summary>
        public bool IsAscending
        {
            get { return _isAscending; }
        }

        /// <summary>
        /// the system added column
        /// </summary>
        public bool TheSystemAddedColumn
        {
            get { return _theSystemAddedColumn; }
        }

        /// <summary>
        /// Returns the position in row of the TrafodionStoreOrderColumnDef
        /// </summary>
        public int PositionInRow
        {
            get { return _positionInRow; }
        }

        /// <summary>
        /// Returns the position in row with a 1-based index instead of 0-based
        /// </summary>
        public int FormattedPositionInRow
        {
            get { return _positionInRow + 1; }
        }

        /// <summary>
        /// Returns the connection from the Store Order Column Def
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return TrafodionTable.GetConnection();
        }

        /// <summary>
        /// Returns the connection definition from the Store Order Column Def
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TrafodionTable.ConnectionDefinition; }
        }


    }
}
