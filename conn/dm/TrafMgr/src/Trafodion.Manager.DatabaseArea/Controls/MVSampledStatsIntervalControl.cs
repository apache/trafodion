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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class MVSampledStatsIntervalControl : UserControl
    {
        public MVSampledStatsIntervalControl()
        {
            InitializeComponent();
            _boundaryStatsDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        public void InitializeControl()
        {
            _boundaryStatsDataGridView.Columns.Clear();
            _boundaryStatsDataGridView.Rows.Clear();
            _boundaryStatsDataGridView.Columns.Add("IntervalNumber", Properties.Resources.IntervalNumber);
            _boundaryStatsDataGridView.Columns.Add("MinBoundary", Properties.Resources.MinBoundary);
            _boundaryStatsDataGridView.Columns.Add("MaxBoundary", Properties.Resources.MaxBoundary);
            _boundaryStatsDataGridView.Columns.Add("StatUEC", Properties.Resources.HistogramUEC);
            _boundaryStatsDataGridView.Columns.Add("UEC", Properties.Resources.SampledUEC);
            _boundaryStatsDataGridView.Columns.Add("Cardinality", Properties.Resources.HistogramCardinality);
            _boundaryStatsDataGridView.Columns.Add("SampleCardinality", Properties.Resources.SampledCardinality);
            _boundaryStatsDataGridView.Columns[0].DefaultCellStyle.Format = "N0"; //IntervalNumber
            _boundaryStatsDataGridView.Columns[3].DefaultCellStyle.Format = "N0"; //StatUEC
            _boundaryStatsDataGridView.Columns[4].DefaultCellStyle.Format = "N0"; //UEC
            _boundaryStatsDataGridView.Columns[5].DefaultCellStyle.Format = "N0"; //Cardinality
            _boundaryStatsDataGridView.Columns[6].DefaultCellStyle.Format = "N0"; //SampleCardinality
        }

        public void Populate(TrafodionMaterializedViewColumn sqlMxMVColumn)
        {

            List<MVColumnSampledStats.SampledInterval> sampledIntervals = sqlMxMVColumn.SampledStatistics.SampledIntervals;
            if (sampledIntervals != null)
            {
                //Ignore row 0 from the intervals table.
                for (int i = 1; i < sampledIntervals.Count; i++)
                {
                    _boundaryStatsDataGridView.Rows.Add(new object[] {
                        sampledIntervals[i].IntervalNumber,
                        (i-1 == 0 ? "" : sampledIntervals[i-1].BoundaryValue),
                        sampledIntervals[i].BoundaryValue,
                        sampledIntervals[i].UEC,
                        sampledIntervals[i].SampledUEC,
                        sampledIntervals[i].RowCount,
                        sampledIntervals[i].SampledRowCount
                    });
                }
            }
        }

        public DataGridView BoundaryDataGridView
        {
            get { return _boundaryStatsDataGridView; }
        }
    }
}
