//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using System.Text;
using System.IO;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemSummaryConfigurationUserControl : TrafodionForm
    {

        #region Fields

        private LiveFeedConnection aLiveFeedConnection;
        private DataTable cachedChartConfigDataTable = new DataTable();
        private DataTable cachedHealthStatesDataTable = new DataTable();
        private string _originalPortNumber = null;
        private string _originalSessionTimer = null;
        private bool _initialized = false;
        private string originalAlarmSoundFile = string.Empty;
        private string lastSelectedAlarmSoundFolder = string.Empty;
        
        // event declaration 
        public delegate void ChangingHandler(object sender, EventArgs args);
        public event ChangingHandler ConfigurationChanged;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Message - the diagonse message for the dialog
        /// </summary>
        public string Message
        {
            get { return _theMessagePanel.Text.Trim(); }
            set
            {
                _theMessagePanel.Text = value;
                _theMessagePanel.Visible = !string.IsNullOrEmpty(value);
                btnApply.Enabled = string.IsNullOrEmpty(value);
            }
        }

        private string SelectedAlarmSoundFile
        {
            get { return (string)ddlAlarmSounds.SelectedValue; }
            set { ddlAlarmSounds.SelectedValue = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="argLiveFeedConnection"></param>
        public SystemSummaryConfigurationUserControl(LiveFeedConnection argLiveFeedConnection) 
        {
            InitializeComponent();
            CenterToScreen();
            this.aLiveFeedConnection = argLiveFeedConnection;
            _originalPortNumber = argLiveFeedConnection.ConnectionDefn.LiveFeedPort;
            _originalSessionTimer = argLiveFeedConnection.ConnectionDefn.LiveFeedRetryTimer;
            if (_originalPortNumber.Equals("-1") || string.IsNullOrEmpty(_originalPortNumber))
            {
                tbxPortNumberComboBox.SelectedIndex = 0;
            }
            else
            {
                tbxPortNumberComboBox.Text = _originalPortNumber;
            }

            tbxSessionRetryTimer.Text = argLiveFeedConnection.ConnectionDefn.LiveFeedRetryTimer;
            
            //cached the changes before last apply, so that users could revert;  
            cachedChartConfigDataTable = (SystemMetricChartConfigModel.Instance.TheSystemMetricsColorTables[argLiveFeedConnection.ConnectionDefn.Name]).Copy();
            cachedHealthStatesDataTable = (SystemMetricChartConfigModel.Instance.TheHealthStatesTables[argLiveFeedConnection.ConnectionDefn.Name]).Copy();

            this.tbxPortNumberComboBox.TextChanged += tbxPort_TextChanged;
            this.tbxSessionRetryTimer.TextChanged += tbxPort_TextChanged;

            Message = "";

            InitializeThresholdAndCheckBoxStates();
            _initialized = true;
            Message = DoValidate();
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To initialize all metrics and Health/States
        /// </summary>
        private void InitializeThresholdAndCheckBoxStates()
        {
            InitializeThresholdTextBoxValues();
            //Initialize100PercentSettingTextBoxValues();
            InitializeMetricsCheckBoxStates();
            InitializeHealthStatesCheckBoxStates();
            InitializeAlarmSounds();
        }

        //To improve, detect what is the name of the checkbox in the same row, and pass the value insead of hard coding
        #region Change Color Events
        private void coreBusyButton_Click(object sender, EventArgs e)
        {            
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Core);
        }

        private void diskIOButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Disk);
        }

        private void tseButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Tse);
        }

        private void loadAvgButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Load_Avg);
        }

        private void contextSwitchButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Virtual_Memory);
        }

        private void networkRcvButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Network_Rcv);
        }

        private void networkXmitButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Network_Txn);
        }

        private void memUsedButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Memory);
        }

        private void swapUsedButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.Swap);
        }

        private void fileSystemButton_Click(object sender, EventArgs e)
        {
            ConfigureChartColor(SystemMetricModel.SystemMetrics.File_System);
        }

        #endregion
        private bool HasMetricColorChange
        {
            set;
            get;
        }
        private bool HasMetricChange
        {
            get
            {
                bool hasMetricChange = false;
                foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
                {
                    CheckBox cbxMetric = (CheckBox)this.Controls.Find("cbx" + metric.ToString(), true)[0];
                    bool displayStatus = SystemMetricChartConfigModel.Instance.GetSystemMetricDisplayStatus(this.aLiveFeedConnection.ConnectionDefn.Name, metric);
                    if (cbxMetric.Checked != displayStatus)
                    {
                        hasMetricChange = true;
                        break;
                    }
                }

                if (!hasMetricChange)
                {
                    foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
                    {
                        TextBox tbxThreshold = (TextBox)this.Controls.Find("tbx" + metric.ToString(), true)[0];
                        int threshold = SystemMetricChartConfigModel.Instance.GetSystemMetricThreshold(this.aLiveFeedConnection.ConnectionDefn.Name, metric);
                        if (tbxThreshold.Text != threshold.ToString())
                        {
                            hasMetricChange = true;
                            break;
                        }
                    }
                }

                return hasMetricChange;
            }
        }

        private bool HasHealthStateVisibilityChange
        {
            get
            {
                bool hasHealthStateVisibilityChange = false;
                foreach (SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
                {
                    CheckBox cbxHealthState = (CheckBox)this.Controls.Find("cbx" + h.ToString(), true)[0];
                    bool displayStatus = SystemMetricChartConfigModel.Instance.GetHealthStatesLayerDisplayStatus(this.aLiveFeedConnection.ConnectionDefn.Name, h);
                    if (cbxHealthState.Checked != displayStatus)
                    {
                        hasHealthStateVisibilityChange = true;
                        break;
                    }
                }

                return hasHealthStateVisibilityChange;
            }
        }

        private void ConfigureChartColor(SystemMetricModel.SystemMetrics systemMetrics) 
        {
            ChartColorPropertyGrid chartpropertyGrid1 = new ChartColorPropertyGrid();
            //DataTable chartColorDataTable = SystemMetricChartColorModel.Instance.TheSystemMetricsColorTables[argLiveFeedConnection.ConnectionDefn.Name];            
            ChartColorModelForPropertyGrid model = new ChartColorModelForPropertyGrid(systemMetrics, cachedChartConfigDataTable);
            chartpropertyGrid1.PropertyObject = model;
            
            if (chartpropertyGrid1.ShowDialog() == DialogResult.OK)
            {
                bool hasColorChange = false;
                model = (ChartColorModelForPropertyGrid)chartpropertyGrid1.PropertyObject;
                hasColorChange = SystemMetricChartConfigModel.Instance.UpdateSystemMetricColors(cachedChartConfigDataTable, systemMetrics,
                model.BackColor, model.ForeColor, model.GridlineColor, model.CursorColor, model.ExceedThresholdColor, 
                model.MaximumLineForeColor, model.MinimumLineForeColor,model.AverageLineForeColor);
                if (hasColorChange) HasMetricColorChange = true;
            }
            
        }        

        /// <summary>
        /// Event handler for apply button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnApply_Click(object sender, EventArgs e)
        {
            AlarmSoundHelper.AlarmSoundFile = SelectedAlarmSoundFile;
            AlarmSoundHelper.AlarmSoundFolder = this.lastSelectedAlarmSoundFolder;

            if (HasMetricColorChange || HasMetricChange || HasHealthStateVisibilityChange)
            {
                ConfigQpidPortNumber(this.tbxPortNumberComboBox.Text);
                ConfigQpidSessionRetryTimer(this.tbxSessionRetryTimer.Text);
                //ConfigureChartMetric100PercentSetting();
                ConfigureChartMetricThreshold();
                ConfigureMetricsDisplayStatus();
                ConfigureHealthStatesDisplayStatus();
                SystemMetricChartConfigModel.Instance.TheSystemMetricsColorTables[aLiveFeedConnection.ConnectionDefn.Name] = cachedChartConfigDataTable;
                SystemMetricChartConfigModel.Instance.TheHealthStatesTables[aLiveFeedConnection.ConnectionDefn.Name] = cachedHealthStatesDataTable;
                SystemMetricChartConfigModel.Instance.PersistSystemMetricColorsAndHealthStates();

                if (ConfigurationChanged != null)
                {
                    ConfigurationChanged(this, e);
                }
                HasMetricColorChange = false;
            }

            this.Close();
        }

        /// <summary>
        /// Event handler for reset button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnReset_Click(object sender, EventArgs e)
        {
            if (_originalPortNumber.Equals("-1") || string.IsNullOrEmpty(_originalPortNumber))
            {
                tbxPortNumberComboBox.SelectedIndex = 0;
            }
            else
            {
                tbxPortNumberComboBox.Text = _originalPortNumber;
            }

            tbxSessionRetryTimer.Text = _originalSessionTimer;

            cachedChartConfigDataTable = SystemMetricChartConfigModel.Instance.TheSystemMetricsColorTables[aLiveFeedConnection.ConnectionDefn.Name].Copy();
            cachedHealthStatesDataTable = (SystemMetricChartConfigModel.Instance.TheHealthStatesTables[aLiveFeedConnection.ConnectionDefn.Name]).Copy();

            if (ConfigurationChanged != null)
            {
                ConfigurationChanged(this, e);
            }
            HasMetricColorChange = false;
            InitializeThresholdAndCheckBoxStates();
            SetAlarmSound(this.originalAlarmSoundFile);
        }

        /// <summary>
        /// Event handler for close button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnClose_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        /// <summary>
        /// Update all metrics' display status
        /// </summary>
        private void ConfigureMetricsDisplayStatus()
        {
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                CheckBox cbx = (CheckBox)this.Controls.Find("cbx" + metric.ToString(), true)[0];
                SystemMetricChartConfigModel.Instance.UpdateSystemMetricDisplayStatus(cachedChartConfigDataTable, metric, cbx.Checked);               
            }
        }

        /// <summary>
        /// Update all Health/State display status
        /// </summary>
        private void ConfigureHealthStatesDisplayStatus()
        {
            foreach (SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
            {
                CheckBox cbx = (CheckBox)this.Controls.Find("cbx" + h.ToString(), true)[0];
                SystemMetricChartConfigModel.Instance.UpdateHealthStatesLayerDisplayStatus(cachedHealthStatesDataTable, h, cbx.Checked);
            }
        }

        #region InitilizeValues
        /// <summary>
        /// Initialize all of the threshold texboxes
        /// </summary>
        private void InitializeThresholdTextBoxValues()
        {
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                TextBox tbxThreshold = (TextBox)this.Controls.Find("tbx" + metric.ToString(), true)[0];
                int threshold = SystemMetricChartConfigModel.Instance.GetSystemMetricThreshold(this.aLiveFeedConnection.ConnectionDefn.Name, metric);
                tbxThreshold.Text = threshold.ToString();
                tbxThreshold.Tag = SystemMetricModel.GetOverallSummaryTitle(metric);
            }
            
        }
        
        /// <summary>
        /// To initialize all of the metric checkboxes
        /// </summary>
        private void InitializeMetricsCheckBoxStates() 
        {
            bool all = true;
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                CheckBox cbxMetric = (CheckBox)this.Controls.Find("cbx" + metric.ToString(), true)[0];
                bool displayStatus = SystemMetricChartConfigModel.Instance.GetSystemMetricDisplayStatus(this.aLiveFeedConnection.ConnectionDefn.Name, metric);
                cbxMetric.Checked = displayStatus;
                cbxMetric.Text = SystemMetricModel.GetOverallSummaryTitle(metric);
                all &= displayStatus;
            }
            cbxAllMetrics.Checked = all;
        }

        /// <summary>
        /// to initialize all of the Health/State checkboxes
        /// </summary>
        private void InitializeHealthStatesCheckBoxStates() 
        {
            bool all = true;
            foreach (SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
            {
                CheckBox cbxHealthState = (CheckBox)this.Controls.Find("cbx" + h.ToString(), true)[0];
                bool displayStatus = SystemMetricChartConfigModel.Instance.GetHealthStatesLayerDisplayStatus(this.aLiveFeedConnection.ConnectionDefn.Name, h);
                cbxHealthState.Checked = displayStatus;
                all &= displayStatus;
            }

            cbxAllHealthState.Checked = all;
        }

        private void InitializeAlarmSounds()
        {
            ddlAlarmSounds.DisplayMember = "Value";
            ddlAlarmSounds.ValueMember = "Key";

            if (!AlarmSoundHelper.Validate())
            {
                this.Shown += (sender, arg) =>
                {
                    MessageBox.Show("Last selected sound has been reset to (None), because the sound file has been removed or moved.", "Sound is reset", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    ddlAlarmSounds.Focus();
                };
            }

            SetAlarmSound(AlarmSoundHelper.AlarmSoundFile);
            this.originalAlarmSoundFile = AlarmSoundHelper.AlarmSoundFile;
        }

        private void SetAlarmSound(string alarmSoundFile)
        {
            string alarmSoundFolder = AlarmSoundHelper.GetAlarmSoundFolder(alarmSoundFile);
            ddlAlarmSounds.DataSource = AlarmSoundHelper.GetAlarmSounds(alarmSoundFolder);
            SelectedAlarmSoundFile = alarmSoundFile;

            btnTestAlarm.Enabled = ddlAlarmSounds.SelectedIndex > 0;
            this.lastSelectedAlarmSoundFolder = alarmSoundFolder;
        }

        #endregion

        /// <summary>
        /// Configure all of the metrics' thresholds
        /// </summary>
        private void ConfigureChartMetricThreshold() 
        {
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics))) 
            {
                //Find the threshold txtbox controls
                TextBox tbxThreshold = (TextBox)this.Controls.Find("tbx" + metric.ToString(), true)[0];
                int argThreshold = Int32.Parse(tbxThreshold.Text.Trim());
                SystemMetricChartConfigModel.Instance.UpdateSystemMetricThreshold(cachedChartConfigDataTable, metric, argThreshold);
            } 
        }

        /// <summary>
        /// This checking is to make sure at least one checkbox is checked. 
        /// </summary>
        /// <returns></returns>
        private string DoValidateCheckBoxes()
        {
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                CheckBox cbxm = (CheckBox)this.Controls.Find("cbx" + metric.ToString(), true)[0];
                if (cbxm.Checked)
                    return null;
            }

            foreach (SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
            {
                CheckBox cbxh = (CheckBox)this.Controls.Find("cbx" + h.ToString(), true)[0];
                if (cbxh.Checked)
                    return null;
            }

            return "Requires at least one Metric or one Health/State being selected.";
        }

        #region metric checkbox CheckedChanged events
        private void cbxCore_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Core, cbxCore.Checked);
            coreBusyButton.Enabled = cbxCore.Checked;
            Message = DoValidate();
        }

        private void cbxDisk_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Disk, cbxDisk.Checked);
            diskIOButton.Enabled = cbxDisk.Checked;
            if (!cbxDisk.Checked)
            {
                tseButton.Enabled = cbxTse.Checked = false;
            }

            Message = DoValidate();
        }

        private void cbxTse_CheckedChanged(object sender, EventArgs e)
        {
            tseButton.Enabled = cbxTse.Checked;
            if (cbxTse.Checked)
            {
                diskIOButton.Enabled = cbxDisk.Checked = true;
            }

            Message = DoValidate();
        }

        private void cbxLoadAvg_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Load_Avg, cbxLoad_Avg.Checked);
            loadAvgButton.Enabled = cbxLoad_Avg.Checked;
            Message = DoValidate();
        }

        private void cbxVirtualMemory_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Virtual_Memory, cbxVirtual_Memory.Checked);
            contextSwitchButton.Enabled = cbxVirtual_Memory.Checked;
            Message = DoValidate();
        }

        private void cbxNetworkRcv_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Network_Rcv, cbxNetwork_Rcv.Checked);
            networkRcvButton.Enabled = cbxNetwork_Rcv.Checked;
            Message = DoValidate();
        }

        private void cbxNetworkTxn_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Network_Txn, cbxNetwork_Txn.Checked);
            networkXmitButton.Enabled = cbxNetwork_Txn.Checked;
            Message = DoValidate();
        }

        private void cbxMemory_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Memory, cbxMemory.Checked);
            memUsedButton.Enabled = cbxMemory.Checked;
            Message = DoValidate();
        }

        private void cbxSwap_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.Swap, cbxSwap.Checked);
            swapUsedButton.Enabled = cbxSwap.Checked;
            Message = DoValidate();
        }

        private void cbxFileSystem_CheckedChanged(object sender, EventArgs e)
        {
            //ConfigureChartDisplayStatus(SystemMetricModel.SystemMetrics.File_System, cbxFile_System.Checked);
            fileSystemButton.Enabled = cbxFile_System.Checked;
            Message = DoValidate();
        }

        #endregion

        private void ConfigQpidPortNumber(string argPortNumber) 
        {
            if (_originalPortNumber.CompareTo(argPortNumber) != 0)
            {
                this.aLiveFeedConnection.ConnectionDefn.LiveFeedPort = argPortNumber;
            }
        }

        private void ConfigQpidSessionRetryTimer(string argSessionRetryTimer)
        {
            if (_originalSessionTimer.CompareTo(argSessionRetryTimer) != 0)
            {
                this.aLiveFeedConnection.ConnectionDefn.LiveFeedRetryTimer = argSessionRetryTimer;
            }
        }

        /// <summary>
        /// To validate all of the textboxes.
        /// </summary>
        /// <returns></returns>
        private string DoValidate()
        {
            if (!_initialized)
            {
                return null;
            }

            int port = 0;
            int sessionMonitorTimer = 0;
            string message = null;
            bool apply = false;

            // If port number has been changed...
            if (tbxPortNumberComboBox.Text.CompareTo(_originalPortNumber) != 0)
            {
                try
                {
                    if (tbxPortNumberComboBox.Text.StartsWith("Default"))
                    {
                        port = -1;
                    }
                    else
                    {
                        port = int.Parse(tbxPortNumberComboBox.Text.Trim());
                        if (port <= 0 || port > 65535)
                        {
                            message = "Invalid port number, please enter a number between 1 and 65535.";
                        }
                    }
                }
                catch (Exception ex)
                {
                    message = "Invalid port number, please enter a number between 1 and 65535.";
                }

                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }

                apply = true;
            }

            // If the retry timer has been changed ...
            if (tbxSessionRetryTimer.Text.Trim().CompareTo(_originalSessionTimer) != 0)
            {
                try
                {
                    sessionMonitorTimer = int.Parse(tbxSessionRetryTimer.Text.Trim());
                    if (sessionMonitorTimer < 0)
                    {
                        message = string.Format("Invalid session timeout value, please enter a number between 0 and {0}.", int.MaxValue);
                    }
                }
                catch (Exception ex)
                {
                    message = string.Format("Invalid session timeout value, please enter a number between 0 and {0}.", int.MaxValue);
                }

                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }

                apply = true;
            }

            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                //Find the 100% setting txtbox controls
               // TrafodionTextBox settingTextBox = (TrafodionTextBox)this.Controls.Find("sbx" + metric.ToString(), true)[0];
                TrafodionTextBox thresholdTextBox = (TrafodionTextBox)this.Controls.Find("tbx" + metric.ToString(), true)[0];
               // int arg100PercentSetting = 0;
                int argThreshold = 0;
               // string arg = "100% setting";
                //double yMaxValue = SystemMetricModel.GetOverallSummaryYValueMax(metric);
                //if (yMaxValue == -1)
                //{
                //    yMaxValue = int.MaxValue;
                //}

                try
                {
                    //arg100PercentSetting = int.Parse(settingTextBox.Text.Trim());
                    //if (arg100PercentSetting < 1)
                    //{
                    //    message = string.Format("Invalid {0} 100% setting value, please enter a number between 1 and {1}.", settingTextBox.Tag, yMaxValue);
                    //}
                    //else if (arg100PercentSetting > yMaxValue)
                    //{
                    //    message = string.Format("{0} 100% setting value exceeds its maximum.", thresholdTextBox.Tag);
                    //}

                    //if (!string.IsNullOrEmpty(message))
                    //{
                    //    return message;
                    //}

                    //arg = "threshold";
                    argThreshold = int.Parse(thresholdTextBox.Text.Trim());
                    if (argThreshold < 0)
                    {
                        message = string.Format("Invalid {0} threshold value, please enter a number greater than 0.", thresholdTextBox.Tag);
                    }
                    else if (argThreshold > 100)
                    {
                        string[] arr = { "%Avg Node Busy","%Mem Used","%Swap Used","%FileSys Used"};

                        var results = Array.FindAll(arr, s => s.Equals(thresholdTextBox.Tag));
                        if (results.GetLength(0) > 0)
                        {
                            message = string.Format("{0} threshold value can't greater than 100.", thresholdTextBox.Tag);
                        }
                    }
                }
                catch (Exception ex)
                {
                    message = string.Format("Invalid {0} threshold value, please enter an integer value.", thresholdTextBox.Tag);
                    //if (arg.Equals("threshold"))
                    //{
                    //    message = string.Format("Invalid {0} {1} value, please enter a number between 0 and {2}.", settingTextBox.Tag, arg, yMaxValue);
                    //}
                    //else
                    //{
                    //    message = string.Format("Invalid {0} {1} value, please enter a number between 1 and {2}.", settingTextBox.Tag, arg, yMaxValue);
                    //}
                }

                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }
            }

            if (string.IsNullOrEmpty(message))
            {
                message = DoValidateCheckBoxes();
            }

            return message;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tbxPort_TextChanged(object sender, EventArgs e)
        {
            Message = DoValidate();
        }


        private void tbxMetric_TextChanged(object sender, EventArgs e)
        {
            Message = DoValidate();
        }

        /// <summary>
        /// Event handler for any of the CheckBox checked changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxHealthStateCheckedChanged(object sender, EventArgs e)
        {
            /*
             * Fix defect 4641:
             * Unless all validations are passed, the Apply button can be set to enabled.
             * Involve two validations:
             *  1. DoValidateCheckBoxes()       - For check box
             *  2. DoValidate()                 - For text box
             */
            string message = string.Empty;
            string messageCheckBoxValication = DoValidateCheckBoxes();
            if (!string.IsNullOrEmpty(messageCheckBoxValication))
            {
                message = (messageCheckBoxValication.Trim());
            }
            else
            {
                string messageTextBoxValication = DoValidate();
                if (!string.IsNullOrEmpty(messageTextBoxValication))
                {
                    message = messageTextBoxValication;
                }
            }

            Message = message;
        }

        /// <summary>
        /// Event handler for Check/UnCheck All button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxAllHealthState_Click(object sender, EventArgs e)
        {
            foreach (SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
            {
                CheckBox cbx = (CheckBox)this.Controls.Find("cbx" + h.ToString(), true)[0];
                cbx.Checked = cbxAllHealthState.Checked;
            }
        }

        /// <summary>
        /// Event handler for Check/UnCheck All Metrics button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cbxAllMetrics_Click(object sender, EventArgs e)
        {
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                CheckBox cbx = (CheckBox)this.Controls.Find("cbx" + metric.ToString(), true)[0];
                cbx.Checked = cbxAllMetrics.Checked;
            }
        }

        /// <summary>
        /// When help button is clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnHelp_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SystemMonitorConfiguration);
        }

        #endregion Private methods

        private void btnTestAlarm_Click(object sender, EventArgs e)
        {
            string alarmSoundFile = SelectedAlarmSoundFile;
            if (!File.Exists(alarmSoundFile))
            {
                MessageBox.Show("The sound file cannot be found: " + Environment.NewLine
                                + alarmSoundFile + Environment.NewLine
                                + "Please select another one.");
                return;
            }

            AlarmSoundHelper.Test(alarmSoundFile);
        }

        private void btnBrowseAlarm_Click(object sender, EventArgs e)
        {
            OpenFileDialog alarmSoundDialog = new OpenFileDialog();
            alarmSoundDialog.Title = Properties.Resources.BrowseAlarmSoundFile;
            alarmSoundDialog.Filter = AlarmSoundHelper.AlarmSoundFilter;
            alarmSoundDialog.InitialDirectory = this.lastSelectedAlarmSoundFolder;
            alarmSoundDialog.CheckPathExists = true;

            if (alarmSoundDialog.ShowDialog() == DialogResult.OK)
            {
                string selectedAlarmSoundFile = alarmSoundDialog.FileName;
                string selectedAlarmSoundFolder = AlarmSoundHelper.GetAlarmSoundFolder(selectedAlarmSoundFile);
                this.lastSelectedAlarmSoundFolder = selectedAlarmSoundFolder;
                ddlAlarmSounds.DataSource = AlarmSoundHelper.GetAlarmSounds(selectedAlarmSoundFolder);
                SelectedAlarmSoundFile = selectedAlarmSoundFile;
            }
        }

        private void ddlAlarmSounds_SelectedIndexChanged(object sender, EventArgs e)
        {
            btnTestAlarm.Enabled = ddlAlarmSounds.SelectedIndex > 0;
        }
    }
}
