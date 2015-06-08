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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class ServiceStatusMonitorControl : UserControl, IMainToolBarConsumer
    {
        #region private member variables

        private const string WMS_SERVICE_STATUS_GRID_CONFIG_ID = "GridConfig_WmsServiceStatus";
        private DateTime _logLastRolloverAt = DateTime.Now;
        public const string LOG_FILE_FILTER = "Log files (*.log)|*.log|All files (*.*)|*.*";
        private String _logFileName = "";
        private DataTable _servicesDataTable = null;
        bool _logWMSStatus = false;
        WmsStatsStruct _wmsStats;
        List<string> _servicesOverThresholdList = new List<string>();
        bool _logTurnedON = false;
        private ConnectionDefinition _theConn;

        #endregion private member variables

        public ServiceStatusMonitorControl(ConnectionDefinition aConnectionDefinition)
        {            
            InitializeComponent();
            _theConn = aConnectionDefinition;
            _servicesDataGrid.CreateGridConfig(WMS_SERVICE_STATUS_GRID_CONFIG_ID);
            _servicesDataGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _servicesDataGrid.AddCountControlToParent("There are {0} services", DockStyle.Top);
            _servicesDataGrid.AddButtonControlToParent(DockStyle.Bottom);

            if (aConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _servicesDataGrid.AlwaysHiddenColumnNames.Add(WmsCommand.COL_MAX_SSD_USAGE);
            }
        }

        public void UpdateMetrics(WmsStatsStruct wmsStats, DataTable servicesDataTable)
        {
            _servicesDataTable = servicesDataTable;
            _totalServices.Text = servicesDataTable.Rows.Count.ToString();
            DataRow[] activeStateRows = servicesDataTable.Select("STATE = 'ACTIVE'");
            _activeServices.Text = activeStateRows.Length.ToString();

            string cpuThreshold = "(MAX_CPU_BUSY > 0  AND  MAX_CPU_BUSY < " + wmsStats.CpuBusy + ")";
            string memThreshold = "(MAX_MEM_USAGE > 0  AND  MAX_MEM_USAGE < " + wmsStats.MemUsage + ")";
            string overflowThreshold = "(MAX_SSD_USAGE > 0  AND  MAX_SSD_USAGE < " + wmsStats.SSDUsage + ")";

            DataRow[] overThresholdRows = new DataRow[0];
            if (_theConn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
            {
                string curExecThreshold = "(MAX_EXEC_QUERIES > 0 AND MAX_EXEC_QUERIES <= CUR_EXEC )";
                string avgESPNumberThreshold = "(MAX_AVG_ESPS > 0 AND MAX_AVG_ESPS < " + +wmsStats.AvgESPNumber + ")";
                //overThresholdRows = servicesDataTable.Select(cpuThreshold + "  OR  " + memThreshold + "  OR  " + overflowThreshold + "  OR  " + curExecThreshold + "  OR  " + avgESPNumberThreshold);
                if (_theConn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    List<DataRow> cpuDataRows = new List<DataRow>();
                    foreach (DataRow row in servicesDataTable.Rows)
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

                        if (cpuBusy > 0 && cpuBusy < wmsStats.CpuBusy)
                        {
                            cpuDataRows.Add(row);
                        }
                    }

                    DataRow[] otherThresholdRows = servicesDataTable.Select(memThreshold + "  OR  "
                            + overflowThreshold + "  OR  " + curExecThreshold + "  OR  " + avgESPNumberThreshold);
                    foreach (DataRow row in otherThresholdRows)
                    {
                        if (!cpuDataRows.Contains(row))
                            cpuDataRows.Add(row);
                    }
                    overThresholdRows = new DataRow[cpuDataRows.Count];
                    cpuDataRows.CopyTo(overThresholdRows);
                }
                else
                {
                    overThresholdRows = servicesDataTable.Select(cpuThreshold + "  OR  " + memThreshold + "  OR  "
                            + overflowThreshold + "  OR  " + curExecThreshold + "  OR  " + avgESPNumberThreshold);
                }
            }
            else 
            {
                overThresholdRows = servicesDataTable.Select(cpuThreshold + "  OR  " + memThreshold + "  OR  " + overflowThreshold);
            }
            _servicesOverThreshold.Text = overThresholdRows.Length.ToString();

            _servicesDataGrid.FillWithDataConfig(servicesDataTable);

            _thresholdExceededReason.Text = string.Format("Last Sample:   Node Busy = {0}% , Memory Usage = {1}% and Exec Queries = {2} ", wmsStats.CpuBusy, wmsStats.MemUsage, wmsStats.CurExec);
            
            Color backgroundColor = Color.Gainsboro;
            Color foregroundColor = _servicesOverThreshold.ForeColor;
			Font  highlightFont = new Font("Tahoma", 10.25F);

            _servicesOverThresholdList = new List<string>();
            foreach (DataRow row in overThresholdRows)
            {
                _servicesOverThresholdList.Add(row["SERVICE_NAME"].ToString());
            }

            for (int idx = 0; idx < _servicesDataGrid.Rows.Count; idx++)
            {
                iGRow aRow = _servicesDataGrid.Rows[idx];

                try
                {
                    String serviceName = aRow.Cells["SERVICE_NAME"].Value.ToString();
                    if (_servicesOverThresholdList.Contains(serviceName))
                    {
                        Hashtable cellsToColorize_ht = new Hashtable();
                        try
                        {
                            double configuredMaxCpuBusy = double.Parse(aRow.Cells["MAX_CPU_BUSY"].Value.ToString());
                            if ((0 < configuredMaxCpuBusy) && (configuredMaxCpuBusy < wmsStats.CpuBusy))
                                cellsToColorize_ht.Add("MAX_CPU_BUSY", "MAX_CPU_BUSY");

                            double configuredMaxMemUsage = double.Parse(aRow.Cells["MAX_MEM_USAGE"].Value.ToString());
                            if ((0 < configuredMaxMemUsage) && (configuredMaxMemUsage < wmsStats.MemUsage))
                                cellsToColorize_ht.Add("MAX_MEM_USAGE", "MAX_MEM_USAGE");

                            double configuredMaxOverflowUsage = double.Parse(aRow.Cells["MAX_SSD_USAGE"].Value.ToString());
                            if ((0 < configuredMaxOverflowUsage) && (configuredMaxOverflowUsage < wmsStats.SSDUsage))
                                cellsToColorize_ht.Add("MAX_SSD_USAGE", "MAX_SSD_USAGE");

                            if (_theConn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
                            {
                                double configuredMaxcurExec = double.Parse(aRow.Cells["MAX_EXEC_QUERIES"].Value.ToString());
                                double curExec = double.Parse(aRow.Cells["CUR_EXEC"].Value.ToString());
                                if ((0 < configuredMaxcurExec) && (configuredMaxcurExec <= curExec))
                                    cellsToColorize_ht.Add("MAX_EXEC_QUERIES", "MAX_EXEC_QUERIES");

                                double configuredMaxavgEsps = double.Parse(aRow.Cells["MAX_AVG_ESPS"].Value.ToString());
                                if ((0 < configuredMaxavgEsps) && (configuredMaxavgEsps < wmsStats.AvgESPNumber))
                                    cellsToColorize_ht.Add("MAX_AVG_ESPS", "MAX_AVG_ESPS");
                            }

                        }
                        catch (Exception)
                        {
                        }

                        if (0 < cellsToColorize_ht.Count)
                            cellsToColorize_ht.Add("SERVICE_NAME", "SERVICE_NAME");

                        for (int cellIdx = 0; cellIdx < aRow.Cells.Count; cellIdx++)
                        {
                            aRow.Cells[cellIdx].BackColor = backgroundColor;
                            if (cellsToColorize_ht.ContainsKey(aRow.Cells[cellIdx].Col.Key))
                            {
                                aRow.Cells[cellIdx].ForeColor = foregroundColor;
                                aRow.Cells[cellIdx].Font = highlightFont;
                                aRow.AutoHeight();
                            }
                        }
                    }
                }
                catch (Exception)
                {
                }
            }

            if (_logTurnedON)
            {
                logServiceLevelInformation();
            }
        }

        private bool rollOverLogFileIfNeeded(DateTime updateTime)
        {
            bool wasFileRolledOver = false;

            /**
             *  Check if its a new day and if so rollover the log file.
            */
            if (_logLastRolloverAt.Day != updateTime.Day)
            {
                try
                {
                    DateTime yesterday = updateTime.Subtract(TimeSpan.FromDays(1.0));
                    String fileNameSuffix = yesterday.ToString("MMM_dd_yyyy");

                    String dirName = Path.GetDirectoryName(this._logFileName);
                    if ((null == dirName) || (0 >= dirName.Length))
                        dirName = ".";

                    String fileName = Path.GetFileNameWithoutExtension(this._logFileName);
                    String fileExt = Path.GetExtension(this._logFileName);

                    String rollOverFileName = dirName + Path.DirectorySeparatorChar + fileName +
                                              "-" + fileNameSuffix + "." + fileExt;

                    /**
                     *  Update the time before we move the file -- in case there's no file from
                     *  the previous day.
                     */
                    this._logLastRolloverAt = updateTime;
                    wasFileRolledOver = true;
                    File.Move(this._logFileName, rollOverFileName);

                }
                catch (Exception)
                {
                }

            }

            return wasFileRolledOver;
        }

        private void writeLogHeadings(StreamWriter writer, DataTable serviceLevels_dt,
                                      Hashtable hiddenCols_ht)
        {
            try
            {
                String colsList;
                if (_theConn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
                {
                    colsList = "Monitor DateTime, CPU Busy %, Memory Usage %, Overflow Usage %, " +
                                     "Exec Queries, Avg ESPs, Threshold Violations, ";
                }
                else 
                {
                    colsList = "Monitor DateTime, CPU Busy %, Memory Usage %, Overflow Usage %, Threshold Violations, ";
                }

                if (0 < serviceLevels_dt.Columns.Count)
                    colsList += serviceLevels_dt.Columns[0].ColumnName;

                for (int idx = 1; idx < serviceLevels_dt.Columns.Count; idx++)
                {
                    String colName = serviceLevels_dt.Columns[idx].ColumnName;
                    if (!hiddenCols_ht.ContainsKey(colName))
                        colsList += ", " + colName;

                }

                writer.WriteLine(colsList);
            }
            catch (Exception)
            {
            }

        }

        private void logServiceLevelInformation()
        {
            if ((null == this._logFileName) || (0 >= this._logFileName.Length))
                return;

            StreamWriter writer = null;

            try
            {
                DateTime now = DateTime.Now;

                bool didRollover = rollOverLogFileIfNeeded(now);

                String numFormat = "N0";

                Hashtable hideColumns_ht = new Hashtable();
                //if (!showSQLDefaults)
                //    hideColumns_ht.Add("SQL_DEFAULTS", "SQL_DEFAULTS");

                writer = System.IO.File.AppendText(this._logFileName);

                if (didRollover)
                    writeLogHeadings(writer, _servicesDataTable, hideColumns_ht);


                writer.WriteLine("---------------------------------------------------------------------------");
                String dataLine = "\"" + now.ToString() + "\"" + ", " + _wmsStats.CpuBusy + ", " + _wmsStats.MemUsage + ", "
                                    + _wmsStats.SSDUsage + ", ";
                if (_theConn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
                {
                    dataLine += _wmsStats.CurExec + ", " + _wmsStats.AvgESPNumber + ", ";
                 }
                writer.WriteLine(dataLine + this._servicesOverThresholdList.Count);

                foreach (DataRow dr in _servicesDataTable.Rows)
                {
                    String serviceInfoLine = dataLine;
                    if (0 < _servicesDataTable.Columns.Count)
                    {
                        String serviceName = dr["SERVICE_NAME"].ToString();
                        if (_servicesOverThresholdList.Contains(serviceName))
                            serviceInfoLine += "[*], ";
                        else
                            serviceInfoLine += ", ";


                        serviceInfoLine += dr[0].ToString();
                    }

                    for (int idx = 1; idx < _servicesDataTable.Columns.Count; idx++)
                    {
                        String colName = _servicesDataTable.Columns[idx].ColumnName;
                        if (!hideColumns_ht.ContainsKey(colName))
                            serviceInfoLine += ", " + dr[idx].ToString();

                    }

                    writer.WriteLine(serviceInfoLine);
                }

            }
            catch (Exception)
            {
            }
            finally
            {
                if (null != writer)
                    writer.Close();

            }

        }

        private void _logButton_Click(object sender, EventArgs e)
        {

            this._logWMSStatus = !this._logWMSStatus;
            if (this._logWMSStatus)
                this._logWMSStatus = logWMSStatusToFile();

            String logButtonText = "  &Log To File";
            Image logButtonImg = global::Trafodion.Manager.Properties.Resources.LogIcon;

            if (this._logWMSStatus)
            {
                logButtonText = " &Stop Logging";
                logButtonImg = global::Trafodion.Manager.Properties.Resources.StopIcon;
            }
            else
            {
                _logTurnedON = false;
            }
            _logButton.Text = logButtonText;
            _logButton.Image = logButtonImg;

            if (!this._logWMSStatus)
                MessageBox.Show(Utilities.GetForegroundControl(), "\nLogging WMS Services status information is OFF.\n\n",
                               "Logging is OFF", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        private bool logWMSStatusToFile()
        {
            SaveFileDialog logToFileDialog = new SaveFileDialog();
            logToFileDialog.Title = "Log WMS Services status information to file ...";
            logToFileDialog.Filter = LOG_FILE_FILTER;
            logToFileDialog.InitialDirectory = Utilities.FileDialogLocation();
            String theFileName = "";

            DialogResult result = logToFileDialog.ShowDialog();

            if (result == DialogResult.OK)
            {
                _logTurnedON = true;
                theFileName = logToFileDialog.FileName;
                Trafodion.Manager.Framework.Utilities.FileDialogLocation(theFileName);

                Hashtable hideColumns_ht = new Hashtable();
                if (!this._servicesDataTable.Columns.Contains(WmsCommand.COL_SQL_DEFAULTS))
                    hideColumns_ht.Add(WmsCommand.COL_SQL_DEFAULTS, WmsCommand.COL_SQL_DEFAULTS);

                FileStream fs = null;
                try
                {
                    fs = File.Open(theFileName, FileMode.Create);
                    StreamWriter writer = new StreamWriter(fs);
                    writeLogHeadings(writer, this._servicesDataTable, hideColumns_ht);
                    this._logFileName = theFileName;
                    writer.Close();
                }
                catch (Exception e)
                {
                }
                finally
                {
                    if (null != fs)
                        fs.Close();

                }
                logServiceLevelInformation();
            }
            return _logTurnedON;
        }

        #region IMainToolBarConsumer implementation

        /// <summary>
        /// Implementating the IMainToolBarConsumer interface, which the consumer could elect buttons to show and modify 
        /// the Help button to invoke context sensitive help topic.
        /// </summary>
        /// <param name="aMainToolBar"></param>
        public void CustomizeMainToolBarItems(Trafodion.Manager.Framework.MainToolBar aMainToolBar)
        {
            // Now, turn on all of the tool strip buttons for PCFTool
            aMainToolBar.TheSystemToolToolStripItem.Visible = true;
            aMainToolBar.TheSystemsToolStripSeparator.Visible = true;
            aMainToolBar.TheSQLWhiteboardToolStripItem.Visible = true;
            aMainToolBar.TheNCIToolStripItem.Visible = true;
            aMainToolBar.TheMetricMinerToolStripItem.Visible = true;
            aMainToolBar.TheOptionsToolStripItem.Visible = true;
            aMainToolBar.TheToolsStripSeparator.Visible = true;
            aMainToolBar.TheWindowManagerToolStripItem.Visible = true;
            aMainToolBar.TheWindowManagerStripSeparator.Visible = true;
            aMainToolBar.TheHelpToolStripItem.Visible = true;

            ///Customize the help topic if it is desired.
            aMainToolBar.UnRegisterDefaultHelpEventHandler();
            aMainToolBar.TheHelpToolStripItem.Alignment = ToolStripItemAlignment.Right;
            aMainToolBar.TheHelpToolStripItem.Click += new EventHandler(TheHelpToolStripItem_Click);
        }

        /// <summary>
        /// The event handler for the context sensitive 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheHelpToolStripItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.WMSServiceStatusHistory);
        }

        #endregion IMainToolBarConsumer
    }
}
