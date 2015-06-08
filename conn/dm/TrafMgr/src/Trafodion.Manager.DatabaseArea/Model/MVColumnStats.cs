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
    /// A model object that holds statistics for a MV column
    /// </summary>
    public class MVColumnStats
    {
        #region Fields

        protected int _numberOfNulls = 0;
        protected double _skew = 0.0;
        protected TrafodionMaterializedViewColumn _sqlMxMVColumn = null;

        #endregion Fields

        /// <summary>
        /// Constructs the model for the Sql MV column
        /// </summary>
        /// <param name="aTrafodionMVColumn"></param>
        public MVColumnStats(TrafodionMaterializedViewColumn aTrafodionMVColumn)
        {
            _sqlMxMVColumn = aTrafodionMVColumn;
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
        public TrafodionMaterializedViewColumn TrafodionMaterializedViewColumn
        {
            get { return _sqlMxMVColumn; }
        }

        /// <summary>
        /// The name of the column
        /// </summary>
        public string ColumnName
        {
            get { return TrafodionMaterializedViewColumn.ExternalName; }
        }

        /// <summary>
        /// The datatype of the column
        /// </summary>
        public string DataType
        {
            get { return TrafodionMaterializedViewColumn.FormattedDataType(); }
        }

        #endregion Properties

    }
}
