
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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class ServicesCountersUserControl : UserControl
    {
        #region member variables

        private WmsSystem _wmsSystem;
        ConnectionDefinition _connectionDefinition;
        DataTable _servicesDataTable;
        WmsStatsStruct _wmsStats;
        int _lastRefreshRate = 30;

        #endregion member variables

        /// <summary>
        /// Constructs the Services counters control
        /// </summary>
        public ServicesCountersUserControl()
        {
            InitializeComponent();
            Reset();
        }

        /// <summary>
        /// Loads the counters with new updated information from WMS
        /// </summary>
        /// <param name="summaryCountsDataProvider"></param>
        public void LoadCounters(SummaryCountsDataProvider summaryCountsDataProvider)
        {
            if (summaryCountsDataProvider != null)
            {
                // Synchronize the access to "summaryCountsDataProvider.ServicesDataTable",  because it's possible there's another thread try to change its value
                lock (summaryCountsDataProvider.SyncRootForServiceDataTable)
                {
                    _lastRefreshRate = summaryCountsDataProvider.RefreshRate;
                    _wmsStats = summaryCountsDataProvider.WmsStats;

                    _connectionDefinition = summaryCountsDataProvider.ConnectionDefinition;
                    if (summaryCountsDataProvider.ServicesDataTable != null && summaryCountsDataProvider.ServicesDataTable.Rows.Count > 0)
                    {
                        _totalCounterText.Text = summaryCountsDataProvider.ServicesDataTable.Rows.Count.ToString();
                        DataRow[] activeStateRows = summaryCountsDataProvider.ServicesDataTable.Select("STATE = 'ACTIVE'");

                        _activeCounterText.Text = activeStateRows.Length.ToString();
                        string cpuThreshold = "(MAX_CPU_BUSY > 0  AND  MAX_CPU_BUSY < " + summaryCountsDataProvider.WmsStats.CpuBusy + ")";
                        string memThreshold = "(MAX_MEM_USAGE > 0  AND  MAX_MEM_USAGE < " + summaryCountsDataProvider.WmsStats.MemUsage + ")";
                        string overflowThreshold = "(MAX_SSD_USAGE > 0 AND MAX_SSD_USAGE < " + summaryCountsDataProvider.WmsStats.SSDUsage + ")";

                        DataRow[] overThresholdRows = new DataRow[0];
                        if (_connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
                        {
                            string curExecThreshold = "(MAX_EXEC_QUERIES > 0 AND MAX_EXEC_QUERIES <= CUR_EXEC)";
                            string avgESPNumberThreshold = "(MAX_AVG_ESPS > 0 AND MAX_AVG_ESPS < " + summaryCountsDataProvider.WmsStats.AvgESPNumber + ")";
                            if (_connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                            {
                                List<DataRow> cpuDataRows = new List<DataRow>();
                                foreach (DataRow row in summaryCountsDataProvider.ServicesDataTable.Rows)
                                {
                                    int cpuBusy = -1;
                                    try
                                    {
                                        cpuBusy = int.Parse(row["MAX_CPU_BUSY"].ToString());
                                    }
                                    catch (Exception ex)
                                    {
                                        cpuBusy = -1;
                                    }

                                    if (cpuBusy > 0 && cpuBusy < summaryCountsDataProvider.WmsStats.CpuBusy)
                                    {
                                        cpuDataRows.Add(row);
                                    }
                                }

                                DataRow[] otherThresholdRows = summaryCountsDataProvider.ServicesDataTable.Select(memThreshold + "  OR  "
                                        + overflowThreshold + "  OR  " + curExecThreshold + "  OR  " + avgESPNumberThreshold);
                                foreach (DataRow row in otherThresholdRows)
                                {
                                    if (!cpuDataRows.Contains(row))
                                        cpuDataRows.Add(row);
                                }
                                //Before copy to datarow,re dim the length for the DataRows array.
                                overThresholdRows = new DataRow[cpuDataRows.Count];
                                cpuDataRows.CopyTo(overThresholdRows);
                            }
                            else
                            {
                                overThresholdRows = summaryCountsDataProvider.ServicesDataTable.Select(cpuThreshold + "  OR  " + memThreshold + "  OR  "
                                        + overflowThreshold + "  OR  " + curExecThreshold + "  OR  " + avgESPNumberThreshold);
                            }
                        }
                        else
                        {
                            overThresholdRows = summaryCountsDataProvider.ServicesDataTable.Select(cpuThreshold + "  OR  " + memThreshold + "  OR  "
                               + overflowThreshold);
                        }


                        _thresholdCounterText.Text = overThresholdRows.Length.ToString();
                        _servicesDataTable = summaryCountsDataProvider.ServicesDataTable;
                        _countersDetailsButton.Enabled = true;

                        //If the platform counter details window is open for the current system, update the details screen with the new metrics.
                        ManagedWindow servicesMonitorWindow = WindowsManager.GetManagedWindow(TrafodionForm.TitlePrefix + summaryCountsDataProvider.ConnectionDefinition.Name + " : " + "WMS Service Status");
                        if (servicesMonitorWindow != null)
                        {
                            foreach (Control control in servicesMonitorWindow.Controls)
                            {
                                if (control is ServiceStatusMonitorControl)
                                {
                                    ((ServiceStatusMonitorControl)control).UpdateMetrics(_wmsStats, _servicesDataTable);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Reset the metrics in the control
        /// </summary>
        public void Reset()
        {
            _totalCounterText.Text = "";
            _activeCounterText.Text = "";
            _thresholdCounterText.Text = "";
            _countersDetailsButton.Enabled = false;
        }

        /// <summary>
        /// Displays the service counter details
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _countersDetailsButton_Click(object sender, System.EventArgs e)
        {
            //Check if a detail window is already open for the current system
            string windowTitle = TrafodionForm.TitlePrefix + _connectionDefinition.Name + " : " + "WMS Service Status";
            ManagedWindow servicesMonitorWindow = WindowsManager.GetManagedWindow(windowTitle);
            if (servicesMonitorWindow != null)
            {
                //detail window already exists. update the data and bring it to front.
                foreach (Control control in servicesMonitorWindow.Controls)
                {
                    if (control is ServiceStatusMonitorControl)
                    {
                        ((ServiceStatusMonitorControl)control).UpdateMetrics(_wmsStats, _servicesDataTable);
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
                ServiceStatusMonitorControl servicesMonitorControl = new ServiceStatusMonitorControl(_connectionDefinition);
                ManagedWindow historyWindow = (ManagedWindow)WindowsManager.PutInWindow(new System.Drawing.Size(1080, 737),
                    servicesMonitorControl, "WMS Service Status", _connectionDefinition);

                servicesMonitorControl.UpdateMetrics(_wmsStats, _servicesDataTable);
            }
        }
    }
}
