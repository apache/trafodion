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
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// A model object holds the sampled statistics for a MV column
    /// </summary>
    public class MVColumnSampledStats : MVColumnStats
    {

        #region Fields

        public const double SAMPLING_THRESHOLD = 100000.00;

        private List<FrequentValue> _frequentValues = null;
        private List<SampledInterval> _sampledIntervals = null;
        private double _sampledPercent = 0.0;
        public const int NUMBER_OF_FREQ_VALUES = 10;

        #endregion Fields

        /// <summary>
        /// Constructs the sampled statistics model for the sql MV column
        /// </summary>
        /// <param name="aTrafodionMVColumn"></param>
        public MVColumnSampledStats(TrafodionMaterializedViewColumn aTrafodionMVColumn)
            :base(aTrafodionMVColumn)
        {
        }

        /// <summary>
        /// Determines the sample sql clause to include in the sql query
        /// </summary>
        /// <returns></returns>
        private string GetSampleClause()
        {
            string sampleClause = "";
            if (_sqlMxMVColumn.TrafodionMaterializedView.RowCount >= SAMPLING_THRESHOLD)
            {
                //If number of rows in table is >= SAMPLING_THRESHOLD, compute the sample percent
                _sampledPercent = SAMPLING_THRESHOLD / _sqlMxMVColumn.TrafodionMaterializedView.RowCount;
                if (_sampledPercent < 0.0000000001)
                {
                    _sampledPercent = 0.0000000001;
                }
                sampleClause = " SAMPLE RANDOM " + FormattedSampledPercent + " PERCENT ";
            }
            else
            {
                //If number of rows in table is < SAMPLING_THRESHOLD, use all rows, dont sample
                _sampledPercent = 100.00;
                sampleClause = "";
            }

            return sampleClause;
        }

        /// <summary>
        /// Computes the statistics by sampling the data in the table
        /// </summary>
        public void ComputeStatsUsingSample()
        {
            Connection connection = null;
            OdbcConnection aOdbcConnection = null;

            try
            {
                connection = _sqlMxMVColumn.GetConnection();
                aOdbcConnection = connection.OpenOdbcConnection;

                //The sequence of queries have to be executed in one session (one open connection)
                //since we are using a volatile table

                #region CreateAndLoadTempTable

                //Name for the temporary volatile table that will hold the sampled data
                string tempTableName = "TRAFMGR_" + DateTime.Now.Ticks;

                //Create the volatile table and load sample data from the user table into the volatile table.
                int result = Queries.ExecuteCreateTempStatsTable(aOdbcConnection, tempTableName, _sqlMxMVColumn.TrafodionMaterializedView, ColumnName, GetSampleClause());

                #endregion CreateAndLoadTempTable

                //If temp table creation was successful, do other computations
                if (result >= 0)
                {
                    #region ComputeSkewAndRowcount

                    //If temp table creation was successful, now compute the skew and number of not nulls
                    //for this column from the sampled data
                    OdbcDataReader odbcDataReader = Queries.ExecuteSelectSampledTableColumnStats(aOdbcConnection, tempTableName, ColumnName);

                    if (odbcDataReader.Read())
                    {
                        //If number of partitions is 1 then skew is 0.
                        if (_sqlMxMVColumn.TrafodionMaterializedView.Partitions.Count > 1)
                            _skew = odbcDataReader.GetDouble(0);

                        _numberOfNulls = odbcDataReader.GetInt32(1);
                    }

                    #endregion ComputeSkewAndRowcount

                    #region ComputeFrequentValues

                    //Compute the top 10 frequent values for this column from the sampled data
                    odbcDataReader = Queries.ExecuteSelectTableColumnFrequentValues(aOdbcConnection, tempTableName, ColumnName, NUMBER_OF_FREQ_VALUES);
                    _frequentValues = new List<FrequentValue>();

                    while (odbcDataReader.Read())
                    {
                        _frequentValues.Add(new FrequentValue(odbcDataReader.GetString(0), odbcDataReader.GetInt32(1)));
                    }

                    #endregion ComputeFrequentValues

                    #region GetHistogramIntervals

                    //Read the histogram intervals table and load all the interval boundaries, their uec and rowcounts
                    _sampledIntervals = new List<SampledInterval>();

                    odbcDataReader = Queries.ExecuteSelectTableColumnStatIntervals(aOdbcConnection, _sqlMxMVColumn.TrafodionMaterializedView, _sqlMxMVColumn.TheColumnNumber);

                    while (odbcDataReader.Read())
                    {
                        _sampledIntervals.Add(new SampledInterval(odbcDataReader.GetInt32(0),
                                odbcDataReader.GetString(1), odbcDataReader.GetInt64(2), odbcDataReader.GetInt64(3)));
                    }

                    #endregion GetHistogramIntervals

                    #region ComputeStatsForIntervalBoundaries

                    //Now for each of the interval boundary, compute the uec and rowcount using the sampled data
                    for (int i = 1; i < _sampledIntervals.Count; i++)
                    {
                        SampledInterval sampledInterval = _sampledIntervals[i];
                        if (!String.IsNullOrEmpty(sampledInterval.BoundaryValue) && !String.IsNullOrEmpty(_sampledIntervals[i - 1].BoundaryValue))
                        {
                            odbcDataReader = Queries.ExecuteSelectStatsForIntervals(aOdbcConnection, tempTableName, ColumnName,
                                             _sampledIntervals[i - 1].BoundaryValue, sampledInterval.BoundaryValue);

                            while (odbcDataReader.Read())
                            {
                                sampledInterval.SampledUEC = odbcDataReader.GetInt32(0);
                                sampledInterval.SampledRowCount = odbcDataReader.GetInt32(1);
                            }
                        }
                    }

                    #endregion ComputeStatsForIntervalBoundaries
                }
            }
            finally
            {
                if (connection != null)
                {
                    connection.Close();
                }
            }
        }

        #region Properties

        /// <summary>
        /// The sample percent used to compute the statistics
        /// </summary>
        public string FormattedSampledPercent
        {
            get { return _sampledPercent.ToString("###.##########"); }
        }

        /// <summary>
        /// List of frequent values for this column
        /// </summary>
        public List<FrequentValue> FrequentValues
        {
            get 
            {
                if (_frequentValues == null)
                {
                    ComputeStatsUsingSample();
                }
                return _frequentValues; 
            }
        }

        /// <summary>
        /// List of the sampled intervals for this column
        /// </summary>
        public List<SampledInterval> SampledIntervals
        {
            get 
            {
                if (_sampledIntervals == null)
                {
                    ComputeStatsUsingSample();
                }
                return _sampledIntervals; 
            }
        }

        #endregion Properties

        /// <summary>
        /// A struct that holds the column value and count for each value
        /// </summary>
        public struct FrequentValue
        {
            public string ColumnValue;
            public long Count;

            public FrequentValue(string colValue, long count)
            {
                ColumnValue = colValue;
                Count = count;
            }
        }

        /// <summary>
        /// This class holds the sampled stats for a specific interval boundary
        /// </summary>
        public class SampledInterval
        {
            #region Fields

            private int _intervalNumber;
            private string _boundaryValue;
            private long _UEC;
            private long _rowCount;
            private long _sampledUEC;
            private long _sampledRowCount;

            #endregion Fields

            /// <summary>
            /// Constructor
            /// </summary>
            /// <param name="seqNumber"></param>
            /// <param name="value"></param>
            /// <param name="uEC"></param>
            /// <param name="rowCount"></param>
            public SampledInterval(int seqNumber, string value, long uEC, long rowCount)
            {
                _intervalNumber = seqNumber;
                _boundaryValue = Trafodion.Manager.Framework.Utilities.RemoveLeadingAndTailingParenthesis(value);
                _UEC = uEC;
                _rowCount = rowCount;
                _sampledUEC = 0;
                _sampledRowCount = 0;
            }

            #region Properties

            /// <summary>
            /// Interval number
            /// </summary>
            public int IntervalNumber
            {
                get { return _intervalNumber; }
            }

            /// <summary>
            /// Boundary value for this interval
            /// </summary>
            public string BoundaryValue
            {
                get { return _boundaryValue; }
            }

            /// <summary>
            /// UEC for this interval from histogram intervals
            /// </summary>
            public long UEC
            {
                get { return _UEC; }
            }

            /// <summary>
            /// Rowcount for this interval from histogram intervals
            /// </summary>
            public long RowCount
            {
                get { return _rowCount; }
            }

            /// <summary>
            /// UEC computed for this interval from the sampled data
            /// </summary>
            public long SampledUEC
            {
                get { return _sampledUEC; }
                set { _sampledUEC = value; }
            }

            /// <summary>
            /// Rowcount computed for this interval from the sampled data
            /// </summary>
            public long SampledRowCount
            {
                get { return _sampledRowCount; }
                set { _sampledRowCount = value; }
            }

            #endregion Properties
        }
    }
}
