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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// This user control displays the histogram statistics for a given table
    /// </summary>
    public partial class TableHistogramStatsControl : UserControl
    {
        /// <summary>
        /// Constructs the control to display the histogram statitics for the table
        /// </summary>
        /// <param name="aTrafodionTable">The table model for which the histogram needs to be displayed</param>
        public TableHistogramStatsControl(TrafodionTable aTrafodionTable)
        {
            InitializeComponent();
            _viewSampledButton.Enabled = false;
            _viewSampledButton.Text = Properties.Resources.ViewSampledStatistics + "...";
            _tableStatsDataGridView.AddCountControlToParent(Properties.Resources.StatsDataGridHeader, DockStyle.Top);
            _tableStatsDataGridView.AddButtonControlToParent(DockStyle.Bottom);
            _tableStatsDataGridView.TrafodionTable = aTrafodionTable;

            //Add listener to selections on the datagrid so the View Sample button can be enabled/disabled
            _tableStatsDataGridView.SelectionChanged += new EventHandler(TableStatsDataGridView_SelectionChanged);

            //Display the Table Row count in the header
            headerLabel.Text = String.Format(Properties.Resources.HistogramStatsCardinalityHeader, aTrafodionTable.RowCount);

            // Create the ToolTip and associate with the Form container.
            ToolTip toolTip = new ToolTip();

            // Set up the delays for the ToolTip.
            toolTip.AutoPopDelay = 4000;
            toolTip.InitialDelay = 500;
            toolTip.ReshowDelay = 500;
            toolTip.ShowAlways = true;

            // Set up the ToolTip text for the Button.
            toolTip.SetToolTip(_viewSampledButton, Properties.Resources.SampleStatsButtonToolTip);
        }

        /// <summary>
        /// If any rows are selected in the histogram datagrid view, enable the View Sample Stats button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TableStatsDataGridView_SelectionChanged(object sender, EventArgs e)
        {
            if (_tableStatsDataGridView.SelectedRows.Count > 0)
            {
                _viewSampledButton.Enabled = true;
            }
            else
            {
                _viewSampledButton.Enabled = false;
            }
        }

        /// <summary>
        /// If the View Sample button is clicked, launch the TableSampledStatsControl as a managed window
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ViewSampledButton_Click(object sender, EventArgs e)
        {
            if (_tableStatsDataGridView.SelectedRows.Count > 0)
            {
                ColumnNameLink.ShowDetails(_tableStatsDataGridView.SelectedRows[0]);
            }
        }
    }
}
