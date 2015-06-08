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

using System.Data.Odbc;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// A model object that holds the histogram statistics for the MV column
    /// </summary>
    public class MVColumnHistogramStats : MVColumnStats
    {
        #region Fields

        private string _minValue;
        private string _maxValue;
        private long _totalUEC;
        private long _rowCount;
        private long _statsTime;

        #endregion Fields

        /// <summary>
        /// Constructs the model for the Sql MV column
        /// </summary>
        /// <param name="aTrafodionTableColumn"></param>
        public MVColumnHistogramStats(TrafodionMaterializedViewColumn aTrafodionMVColumn)
            : base(aTrafodionMVColumn)
        {
        }

        /// <summary>
        /// Stores the stats attributes from database into the member variables 
        /// </summary>
        /// <param name="odbcDataReader"></param>
        public void SetAttributes(OdbcDataReader odbcDataReader)
        {
            _numberOfNulls = odbcDataReader.GetInt32(1);
            _minValue = Trafodion.Manager.Framework.Utilities.RemoveLeadingAndTailingParenthesis(odbcDataReader.GetString(2));
            _maxValue = Trafodion.Manager.Framework.Utilities.RemoveLeadingAndTailingParenthesis(odbcDataReader.GetString(3));
            if (_sqlMxMVColumn.TrafodionMaterializedView.TheTrafodionSchema.Version >= 2300)
            {
                _skew = odbcDataReader.GetDouble(4);
                _totalUEC = odbcDataReader.GetInt64(5);
                _rowCount = odbcDataReader.GetInt64(6);
                _statsTime = odbcDataReader.GetInt64(7);
            }
            else
            {
                _totalUEC = odbcDataReader.GetInt64(4);
                _rowCount = odbcDataReader.GetInt64(5);
                _statsTime = odbcDataReader.GetInt64(6);
            }
        }

        #region Properties

        /// <summary>
        /// Mininum value for this column
        /// </summary>
        public string MinValue
        {
            get { return _minValue; }
        }

        /// <summary>
        /// Maximum value for this column
        /// </summary>
        public string MaxValue
        {
            get { return _maxValue; }
        }

        /// <summary>
        /// Total unique entry count for this column
        /// </summary>
        public long TotalUEC
        {
            get { return _totalUEC; }
        }

        /// <summary>
        /// Number of rows that contain values for this column
        /// </summary>
        public long RowCount
        {
            get { return _rowCount; }
        }

        /// <summary>
        /// The last time update stats was done on this column
        /// </summary>
        public long StatsTime
        {
            get { return _statsTime; }
        }

        #endregion Properties
    }
}
