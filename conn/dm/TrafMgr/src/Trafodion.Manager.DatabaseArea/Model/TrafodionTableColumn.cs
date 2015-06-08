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
    /// A column that resides in a table. It contains special information that only
    /// pertains to a table column such as primary key information.
    /// </summary>
    public class TrafodionTableColumn : TrafodionColumn
    {
        private TrafodionPrimaryKeyColumnDef theTrafodionPrimaryKeyColumn = null;
        private TableColumnHistogramStats _histogramStatistics = null;
        private TableColumnSampledStats _sampledStatistics = null;

        #region Properties

        /// <summary>
        /// The table to which this column belongs.
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return TrafodionSchemaObject as TrafodionTable; }
            set { TrafodionSchemaObject = value; }
        }

        /// <summary>
        /// The column definition which describes this column. This
        /// will return null if the column is not a primary key column.
        /// </summary>
        public TrafodionPrimaryKeyColumnDef TheTrafodionPrimaryKeyColumnDef
        {
            get { return theTrafodionPrimaryKeyColumn; }
            set { theTrafodionPrimaryKeyColumn = value; }
        }

        /// <summary>
        /// Indicates if this column is in the table's primary key.
        /// </summary>
        public bool InPrimaryKey
        {
            get
            {
                TrafodionTable.LoadPrimaryKeyColumns();
                return (TheTrafodionPrimaryKeyColumnDef != null);
            }
        }

        /// <summary>
        /// The statistics for this column
        /// </summary>
        public TableColumnHistogramStats HistogramStatistics
        {
            get { return _histogramStatistics; }
            set 
            { 
                _histogramStatistics = value;
                //clear out the sampled stats any time histogram stats changes
                //this will force us to recompute the sampled stats
                _sampledStatistics = null; 
            }
        }

        public TableColumnSampledStats SampledStatistics
        {
            get 
            {
                if (_sampledStatistics == null)
                {
                    _sampledStatistics = new TableColumnSampledStats(this);
                    //_sampledStatistics.LoadSampledTableColumnStats();
                    _sampledStatistics.ComputeStatsUsingSample();
                }
                return _sampledStatistics; 
            }
            set { _sampledStatistics = value; }
        }

        #endregion

        /// <summary>
        /// Constructs a new table column.
        /// </summary>
        public TrafodionTableColumn()
        {
        }

        /// <summary>
        /// Returns the string to indicate if the column is a primary key column.
        /// </summary>
        /// <returns></returns>
        public string FormattedInPrimaryKey()
        {
            return InPrimaryKey ? "X" : "";
        }
    }
}
