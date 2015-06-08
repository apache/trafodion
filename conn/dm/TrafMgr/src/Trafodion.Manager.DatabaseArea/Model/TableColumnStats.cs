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
    /// A model object that holds statistics for a table column
    /// </summary>
    public class TableColumnStats
    {
        #region Fields

        protected int _numberOfNulls = 0;
        protected double _skew = 0.0;
        protected TrafodionTableColumn _sqlMxTableColumn = null;

        #endregion Fields

        /// <summary>
        /// Constructs the model for the Sql Table column
        /// </summary>
        /// <param name="aTrafodionTableColumn"></param>
        public TableColumnStats(TrafodionTableColumn aTrafodionTableColumn)
        {
            _sqlMxTableColumn = aTrafodionTableColumn;
        }

        #region Properties

        /// <summary>
        /// Number of null values for this column
        /// </summary>
        public int NumberOfNulls
        {
            get { return _numberOfNulls; }
        }

        /// <summary>
        /// Skew in distribution of the column values
        /// </summary>
        public double Skew
        {
            get { return _skew; }
        }

        /// <summary>
        /// The table column model for this column
        /// </summary>
        public TrafodionTableColumn TrafodionTableColumn
        {
            get { return _sqlMxTableColumn; }
        }

        /// <summary>
        /// The name of the column
        /// </summary>
        public string ColumnName
        {
            get { return TrafodionTableColumn.ExternalName; }
        }

        /// <summary>
        /// The datatype of the column
        /// </summary>
        public string DataType
        {
            get { return TrafodionTableColumn.FormattedDataType(); }
        }

        #endregion Properties

    }
}
