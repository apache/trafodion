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
    /// Column definition for a Sql Hash key column
    /// </summary>
    public class TrafodionHashKeyColumnDef : TrafodionObject
    {
        /// <summary>
        /// Creates a new instance of HashKeyTrafodionColumn
        /// </summary>
        /// <param name="aTrafodionTable"></param>
        /// <param name="anExternalName"></param>
        /// <param name="aPositionInRow"></param>
        /// <param name="aPartKeySeqNum"></param>
        /// <param name="anOrdering"></param>
        public TrafodionHashKeyColumnDef(TrafodionTable aTrafodionTable, string anExternalName, int aColumnNumber,
            int aPositionInRow, int aPartKeySeqNum, string anOrdering)
            :base(anExternalName, aTrafodionTable.UID)
        {
            _sqlMxTable = aTrafodionTable;
            // SK: needed if the Columns tab is going to show Hash Key columns
            //_sqlMxTableColumn.TheTrafodionHashKeyColumnDef = this;

            _columnNumber = aColumnNumber;
            _partKeySeqNum = aPartKeySeqNum;
            _positionInRow = aPositionInRow;
            _isAscending = anOrdering.Trim().Equals("A");
            // SK: this is not needed since Hash Key always added by user
            //_isSystemAddedColumn = aSystemAddedColumn.Trim().Equals("Y");
        }

        /// <summary>
        /// </summary>
        /// <param name="aTrafodionTable">The reference to the Sql table object</param>
        /// <param name="aHashKeyTrafodionColumn">An existing hash key column definition</param>
        public TrafodionHashKeyColumnDef(TrafodionTable aTrafodionTable, TrafodionHashKeyColumnDef aHashKeyTrafodionColumn)
        {
            ExternalName = aHashKeyTrafodionColumn.ExternalName;
            _sqlMxTable = aTrafodionTable;
            //_sqlMxTableColumn.TheTrafodionHashKeyColumnDef = this;

            _partKeySeqNum = aHashKeyTrafodionColumn.ThePartKeySeqNum;
            _positionInRow = aHashKeyTrafodionColumn.ThePositionInRow;
            _isAscending = aHashKeyTrafodionColumn.IsAscending;
            //_clusteringKeySeqNum = aHashKeyTrafodionColumn.TheClusteringKeySeqNum;
            //_isSystemAddedColumn = aHashKeyTrafodionColumn.IsSystemAddedColumn;
        }

        TrafodionTable _sqlMxTable;
        /// <summary>
        /// A reference to the sql table object
        /// </summary>
        public TrafodionTable TheTrafodionTable
        {
            get { return _sqlMxTable; }
        }

        int _columnNumber;
        /// <summary>
        /// The Position within row of base table (first column is number 0)
        /// </summary>
        public int TheColumnNum
        {
            get { return _columnNumber; }
        }

        /// <summary>
        /// Returns the column position in row with a 1-based index instead of 0-based
        /// </summary>
        public int FormattedColumnNumber
        {
            get { return _columnNumber + 1; }
        }


        int _positionInRow;

        /// <summary>
        /// The position in the row
        /// </summary>
        public int ThePositionInRow
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

        bool _isAscending;
        /// <summary>
        /// Sort order of the hash key column
        /// </summary>
        public bool IsAscending
        {
            get { return _isAscending; }
        }

        int _partKeySeqNum;
        /// <summary>
        /// The paritioning key sequence number
        /// </summary>
        public int ThePartKeySeqNum
        {
            get { return _partKeySeqNum; }
        }

        //int _clusteringKeySeqNum;
        /// <summary>
        /// The clusterin key sequence number
        /// </summary>
        //public int TheClusteringKeySeqNum
        //{
        //    get { return _clusteringKeySeqNum; }
        //}

        //bool _isSystemAddedColumn;

        /// <summary>
        /// Indicates if the column was added by system or user specified it 
        /// explicitly to the hash key definition when the table was created
        /// </summary>
        //public bool IsSystemAddedColumn
        //{
        //    get { return _isSystemAddedColumn; }
        //}

        /// <summary>
        /// Returns the connection from the Store Order Column Def
        /// </summary>
        /// <returns></returns>
        override public Connection GetConnection()
        {
            return TheTrafodionTable.GetConnection();
        }

        /// <summary>
        /// Returns the connection definition from the Store Order Column Def
        /// </summary>
        override public ConnectionDefinition ConnectionDefinition
        {
            get { return TheTrafodionTable.ConnectionDefinition; }
        }

    }
}
