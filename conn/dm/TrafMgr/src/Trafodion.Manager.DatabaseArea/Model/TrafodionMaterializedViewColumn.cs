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
    /// A column that resides in a MV. It contains special information that only
    /// pertains to a MV
    /// </summary>
    public class TrafodionMaterializedViewColumn : TrafodionColumn
    {
        private MVColumnHistogramStats _histogramStatistics = null;
        private MVColumnSampledStats _sampledStatistics = null;
        
        #region Properties

        /// <summary>
        /// The View to which this column belongs.
        /// </summary>
        public TrafodionMaterializedView TrafodionMaterializedView
        {
            get { return TrafodionSchemaObject as TrafodionMaterializedView; }
            set { TrafodionSchemaObject = value; }
        }

        /// <summary>
        /// The statistics for this column
        /// </summary>
        public MVColumnHistogramStats HistogramStatistics
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

        public MVColumnSampledStats SampledStatistics
        {
            get
            {
                if (_sampledStatistics == null)
                {
                    _sampledStatistics = new MVColumnSampledStats(this);
                    _sampledStatistics.ComputeStatsUsingSample();
                }
                return _sampledStatistics;
            }
            set { _sampledStatistics = value; }
        }

        #endregion

        /// <summary>
        /// Constructs a new View column.
        /// </summary>
        public TrafodionMaterializedViewColumn()
        {

        }

        public TrafodionObject GetUsedTable(int columnNum)
        {
            return TrafodionMaterializedView.GetUsedTable(columnNum); 
        }
    }
}
