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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using System.Drawing;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class PlatformCountersUserControl : UserControl
    {
        #region private member variables

        int _lastRefreshRate = 30;
        DataTable _wmsPlatformMetrics;
        ConnectionDefinition _connectionDefinition;

        #endregion private member variables

        /// <summary>
        /// Constructs the WMS platform counters control
        /// </summary>
        public PlatformCountersUserControl()
        {
            InitializeComponent();

            //The data table is used to store the historical platform metrics
            _wmsPlatformMetrics = new DataTable();
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.TIME_COLUMN_NAME, typeof(DateTime));
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.PERCENT_NODE_BUSY_COLUMN_NAME, typeof(double));
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.MAX_NODE_BUSY_COLUMN_NAME, typeof(double));
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.PERCENT_MEM_USAGE_COLUMN_NAME, typeof(double));
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.MAX_MEM_USAGE_COLUMN_NAME, typeof(double));            
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.COUNT_EXEC_QUERIES_COLUMN_NAME, typeof(double));
            _wmsPlatformMetrics.Columns.Add(WMSPlatformCounterHistory.MAX_EXEC_QUERIES_COLUMN_NAME, typeof(double));                
            
            Reset();
        }

        /// <summary>
        /// Loads the counters with new updated information from WMS
        /// </summary>
        /// <param name="wmsSystem"></param>
        /// <param name="refreshRate"></param>
        public void LoadCounters(SummaryCountsDataProvider summaryCountsDataProvider)
        {
            _lastRefreshRate = summaryCountsDataProvider.RefreshRate; //used by the platform counter details screen.

            if (summaryCountsDataProvider != null)
            {
                _connectionDefinition = summaryCountsDataProvider.ConnectionDefinition;
                
                //Update the counter values
                _nodeBusyCounterText.Text = String.Format("{0:N2}", summaryCountsDataProvider.WmsStats.CpuBusy);
                _memUsageCounterText.Text = String.Format("{0:N2}", summaryCountsDataProvider.WmsStats.MemUsage);
                _excQueriesCounterText.Text = String.Format("{0}", summaryCountsDataProvider.WmsStats.CurExec);

                //Colorize Exe Queries Label to indicate it is exceeding threshold or not
                if (summaryCountsDataProvider.WmsStats.CurExec >= summaryCountsDataProvider.WmsStats.MaxExecQueries
                    && summaryCountsDataProvider.WmsStats.MaxExecQueries != 0)
                    _excQueriesCounterText.ForeColor = Color.Red;
                else
                    _excQueriesCounterText.ForeColor = Color.Green;

                _countersDetailsButton.Enabled = true;

                //Add a new row for the current metrics to the historical data table.
                DataRow aRow = _wmsPlatformMetrics.NewRow();
                aRow[WMSPlatformCounterHistory.TIME_COLUMN_NAME] = DateTime.UtcNow + _connectionDefinition.ServerGMTOffset; 
                aRow[WMSPlatformCounterHistory.PERCENT_NODE_BUSY_COLUMN_NAME] = summaryCountsDataProvider.WmsStats.CpuBusy;
                aRow[WMSPlatformCounterHistory.MAX_NODE_BUSY_COLUMN_NAME] = summaryCountsDataProvider.WmsStats.MaxCpuBusy;
                aRow[WMSPlatformCounterHistory.PERCENT_MEM_USAGE_COLUMN_NAME] = summaryCountsDataProvider.WmsStats.MemUsage;
                aRow[WMSPlatformCounterHistory.MAX_MEM_USAGE_COLUMN_NAME] = summaryCountsDataProvider.WmsStats.MaxMemUsage;                
                aRow[WMSPlatformCounterHistory.COUNT_EXEC_QUERIES_COLUMN_NAME] = summaryCountsDataProvider.WmsStats.CurExec;
                aRow[WMSPlatformCounterHistory.MAX_EXEC_QUERIES_COLUMN_NAME] = summaryCountsDataProvider.WmsStats.MaxExecQueries;
                
                _wmsPlatformMetrics.Rows.Add(aRow);

                


                //Purge the old metric rows from data table, if the number of rows has exceeded the max entries allowed.
                MonitorWorkloadOptions options = MonitorWorkloadOptions.GetOptions();
                if (_wmsPlatformMetrics.Rows.Count > options.MaxGraphPoints)
                {
                    DataRow[] rows = _wmsPlatformMetrics.Select("", string.Format("{0} DESC", WMSPlatformCounterHistory.TIME_COLUMN_NAME));
                    if (rows.Length > options.MaxGraphPoints)
                    {
                        for (int idx = rows.Length - 1; idx >= options.MaxGraphPoints; idx--)
                        {
                            rows[idx].Delete();
                        }

                        _wmsPlatformMetrics.AcceptChanges();
                    }
                }

                //If the platform counter details window is open for the current system, update the details screen with the new metrics.
                ManagedWindow platformCounterDetails = WindowsManager.GetManagedWindow(TrafodionForm.TitlePrefix + summaryCountsDataProvider.ConnectionDefinition.Name + " : " + "Platform Counter History");
                if (platformCounterDetails != null)
                {
                    foreach (Control control in platformCounterDetails.Controls)
                    {
                        if (control is WMSPlatformCounterHistory)
                        {
                            ((WMSPlatformCounterHistory)control).UpdateMetrics(_wmsPlatformMetrics, _lastRefreshRate);
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Reset the platform counter values
        /// </summary>
        public void Reset()
        {
            _nodeBusyCounterText.Text = "";
            _memUsageCounterText.Text = "";
            _excQueriesCounterText.Text = "";
            _wmsPlatformMetrics.Rows.Clear();
            _countersDetailsButton.Enabled = false;
        }

        /// <summary>
        /// Displays the historical trend of the platform counter metrics
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _countersDetailsButton_Click(object sender, EventArgs e)
        {
            //Check if a detail window is already open for the current system
            string windowTitle = TrafodionForm.TitlePrefix + _connectionDefinition.Name + " : " + "Platform Counter History";
            ManagedWindow platformCounterDetails = WindowsManager.GetManagedWindow(windowTitle);
            if (platformCounterDetails != null)
            {
                //detail window already exists. update the data and bring it to front.
                foreach (Control control in platformCounterDetails.Controls)
                {
                    if (control is WMSPlatformCounterHistory)
                    {
                        ((WMSPlatformCounterHistory)control).UpdateMetrics(_wmsPlatformMetrics, _lastRefreshRate);
                        break;
                    }
                }

                if (WindowsManager.Exists(windowTitle))
                {
                    WindowsManager.Restore(windowTitle);
                    WindowsManager.BringToFront(windowTitle);
                }
            }
            else
            {
                //create a new window for the platform counter details and display
                WMSPlatformCounterHistory platformCounterHistory = new WMSPlatformCounterHistory();
                ManagedWindow historyWindow = (ManagedWindow)WindowsManager.PutInWindow(new System.Drawing.Size(800, 600), 
                    platformCounterHistory, "Platform Counter History", _connectionDefinition);

                platformCounterHistory.UpdateMetrics(_wmsPlatformMetrics, _lastRefreshRate);
            }
        }
    }
}
