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


namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Column definition for a Sql Primary key column
    /// </summary>
    public class TrafodionPrimaryKeyColumnDef
    {

        /// <summary>
        /// Creates a new instance of PrimaryKeyTrafodionColumn.
        /// </summary>
        /// <param name="aTrafodionColumn"></param>
        /// <param name="anOrdering"></param>
        /// <param name="aClusteringKeySeqNum"></param>
        /// <param name="aSystemAddedColumn"></param>
        /// <param name="aColumnNumber"></param>
        public TrafodionPrimaryKeyColumnDef(TrafodionTableColumn aTrafodionTableColumn, string anOrdering, int aClusteringKeySeqNum, string aSystemAddedColumn, int aColumnNumber)
            : this(aTrafodionTableColumn, 0, aColumnNumber, anOrdering, 0, aClusteringKeySeqNum, aSystemAddedColumn)
        {
        }

        /// <summary>
        /// Creates a new instance of PrimaryKeyTrafodionColumn
        /// </summary>
        /// <param name="aTrafodionColumn"></param>
        /// <param name="aPositionInRow"></param>
        /// <param name="aColumnNumber"></param>
        /// <param name="anOrdering"></param>
        /// <param name="aPartKeySeqNum"></param>
        /// <param name="aClusteringKeySeqNum"></param>
        /// <param name="aSystemAddedColumn"></param>
        public TrafodionPrimaryKeyColumnDef(TrafodionTableColumn aTrafodionTableColumn, int aPositionInRow, int aColumnNumber, string anOrdering, int aPartKeySeqNum, int aClusteringKeySeqNum, string aSystemAddedColumn)
        {
            _sqlMxTableColumn = aTrafodionTableColumn;

            _sqlMxTableColumn.TheTrafodionPrimaryKeyColumnDef = this;

            _positionInRow = aPositionInRow;
            _columnNumber = aColumnNumber;
            _isAscending = anOrdering.Trim().Equals("A");
            _partKeySeqNum = aPartKeySeqNum;
            _clusteringKeySeqNum = aClusteringKeySeqNum;
            _isSystemAddedColumn = aSystemAddedColumn.Trim().Equals("Y");
        }

        /// <summary>
        /// Creates a new instance of primary key column definition from 
        /// an existing primary key column definition
        /// </summary>
        /// <param name="aTrafodionColumn">The reference to the Sql column object</param>
        /// <param name="aPrimaryKeyTrafodionColumn">An existing primary key column definition</param>
        public TrafodionPrimaryKeyColumnDef(TrafodionTableColumn aTrafodionTableColumn, TrafodionPrimaryKeyColumnDef aPrimaryKeyTrafodionColumn)
        {
            _sqlMxTableColumn = aTrafodionTableColumn;

            _sqlMxTableColumn.TheTrafodionPrimaryKeyColumnDef = this;

            _positionInRow = aPrimaryKeyTrafodionColumn.ThePositionInRow;
            _columnNumber = aPrimaryKeyTrafodionColumn.TheColumnNumber;
            _isAscending = aPrimaryKeyTrafodionColumn.IsAscending;
            _partKeySeqNum = aPrimaryKeyTrafodionColumn.ThePartKeySeqNum;
            _clusteringKeySeqNum = aPrimaryKeyTrafodionColumn.TheClusteringKeySeqNum;
            _isSystemAddedColumn = aPrimaryKeyTrafodionColumn.IsSystemAddedColumn;
        }

        TrafodionTableColumn _sqlMxTableColumn;
        /// <summary>
        /// A reference to the sql column object
        /// </summary>
        public TrafodionTableColumn TheTrafodionTableColumn
        {
            get { return _sqlMxTableColumn; }
        }

        int _positionInRow;

        /// <summary>
        /// The position in the row
        /// </summary>
        public int ThePositionInRow
        {
            get { return _positionInRow; }
        }

        int _columnNumber;
        /// <summary>
        /// The column number to represent the order in the primary key definition
        /// </summary>
        public int TheColumnNumber
        {
            get { return _columnNumber; }
        }

        bool _isAscending;
        /// <summary>
        /// Sort order of the primary key column
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

        int _clusteringKeySeqNum;
        /// <summary>
        /// The clusterin key sequence number
        /// </summary>
        public int TheClusteringKeySeqNum
        {
            get { return _clusteringKeySeqNum; }
        }

        bool _isSystemAddedColumn;

        /// <summary>
        /// Indicates if the column was added by system or user specified it 
        /// explicitly to the primary key definition when the table was created
        /// </summary>
        public bool IsSystemAddedColumn
        {
            get { return _isSystemAddedColumn; }
        }

    }
}
