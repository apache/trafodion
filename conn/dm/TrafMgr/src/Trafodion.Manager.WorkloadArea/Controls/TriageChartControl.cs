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
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.DatabaseArea.Model;


namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class TriageChartControl : UserControl
    {
        private TriageHelper _theTriageHelper = null;
        private string _type;

        Chart _theChart = new Chart();
        public TriageHelper TriageHelper
        {
            get { return _theTriageHelper; }
            set { _theTriageHelper = value; }
        }
        public String WidgetType
        {
            get { return this._type; }
            set { this._type = value; }
        }

        public TriageChartControl()
        {
            InitializeComponent();

            //Initial settings for the graph
            InitializeGraphControl();
        }

        private void InitializeGraphControl()
        {
            _theChart.Dock = DockStyle.Fill;
            _theChart.Location = new Point(10, 10);
            // Leave a small margin around the outside of the control
            _theChart.Size = new Size(chartPanel.ClientRectangle.Width - 10,
                                    chartPanel.ClientRectangle.Height - 10);
            this._theChart.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.DashDotDot;
            this._theChart.BorderSkin.SkinStyle = System.Windows.Forms.DataVisualization.Charting.BorderSkinStyle.Emboss;
            _theChart.Name = "_theChart";
            _theChart.ChartAreas.Add(GetChartArea());
            SetAxesTitle();
            chartPanel.Controls.Add(_theChart);
        }

            // Set BarBase to the YAxis for horizontal bars
        private ChartArea GetChartArea()
        {
            ChartArea chartArea = new ChartArea();
            chartArea.Name = "Default";
            return chartArea;
        }
        private Series GetSeries()
        {
            Series series = new Series();
            series.ChartArea = "Default";
            series.Name = "DefaultSeries";
            series.Legend = "Testing";

            // show the horizontal scroll bar
            series.ChartType = SeriesChartType.RangeColumn;
            series.YValueMembers = "y-axis";
            series.XValueMember = "x-axis";

            return series;
        }
            // Horizontal pan and zoom not allowed

        private void SetAxesTitle()
        {
            _theChart.ChartAreas["Default"].AxisX.Title = "Query";
            _theChart.ChartAreas["Default"].AxisY.Title = "Elapsed Time";
            _theChart.ChartAreas["Default"].CursorY.IsUserSelectionEnabled= true;
            _theChart.ChartAreas["Default"].CursorX.IsUserSelectionEnabled = true;
            _theChart.Titles.Add("");
        }

            // Fill the chart background with a color gradient
        private void doCleanupOfChart(System.Windows.Forms.DataVisualization.Charting.Chart aChart)
        {
            aChart.ChartAreas.Clear();
            aChart.Series.Clear();
            aChart.Legends.Clear();

            aChart.Titles.Clear();
            aChart.DataBindings.Clear();
        }

        /// <summary>
        /// Draws the chart to display the bar graph for each interval
        /// </summary>
        /// <param name="_boundaryStatsDataGridView"></param>
        public void DrawChart(DataGridView _boundaryStatsDataGridView)
        {
            doCleanupOfChart(_theChart);
            _theChart.ChartAreas.Add(GetChartArea());
            SetAxesTitle();
            _theChart.Series.Add(GetSeries());
            // Create data points for the BarItem
            DataTable data = new DataTable();
            data.Columns.Add("x-axis", typeof(int));
            data.Columns.Add("y-axis", typeof(long));
            foreach (DataGridViewRow row in _boundaryStatsDataGridView.Rows)
            {
                int y = (Int32)row.Cells[0].Value; //interval
                long x = (long)row.Cells[6].Value; //cardinality
                data.Rows.Add(new object[] { y, x });
            }
            _theChart.DataSource = data;
            // Fill the bar item with the values


            // Create TextObj's to provide labels for each bar
            //BarItem.CreateBarLabels(graphPane, false, "f0");
            _theChart.DataBind();

        }
        public void DrawChart(DataTable _boundaryStatsDataGridView)
        {
            doCleanupOfChart(_theChart);
            _theChart.ChartAreas.Add(GetChartArea());
            SetAxesTitle();
            _theChart.Series.Add(GetSeries());
            // Create data points for the BarItem
            DataTable data = new DataTable();
            data.Columns.Add("x-axis", typeof(int));
            data.Columns.Add("y-axis", typeof(long));
            foreach (DataGridViewRow row in _boundaryStatsDataGridView.Rows)
            {
                int y = (Int32)row.Cells[0].Value; //interval
                long x = (long)row.Cells[6].Value; //cardinality
                data.Rows.Add(new object[] { y, x });
            }
            _theChart.DataSource = data;
            // Fill the bar item with the values


            // Create TextObj's to provide labels for each bar
            //BarItem.CreateBarLabels(graphPane, false, "f0");
            _theChart.DataBind();

        }
        public void UpdateGraphWidget(DataTable filteredDataTable)
        {
            try
            {
                Hide();
                if(filteredDataTable != null)
                DrawChart(filteredDataTable);

            }
            finally
            {
                Show();
            }

        }

    }
}
