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
using System.ComponentModel;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using System.Collections;
using System.Collections.Generic;
using System.Drawing;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// Allows for the configuration of a given system monitor client.
    /// </summary>
    public partial class SystemMonitorConfigurationControl : UserControl, ICloneToWindow
    {
        #region Fields
        private ConnectionDefinition _connectionDefinition;
        private SystemMonitorConfigurationDefinition _activeConfigDefinition = new SystemMonitorConfigurationDefinition();
        private Dictionary<SystemMetric, UserControl> _graphDictionary = new Dictionary<SystemMetric, UserControl>();

        // event declaration 
        public delegate void ChangingHandler(object sender, SystemMonitorConfigurationDefinition args);
        public event ChangingHandler ConfigurationChanged;

        #endregion

        #region Properties
        public Dictionary<SystemMetric, UserControl> GraphDictionary
        {
            get { return _graphDictionary; }
        }

        public SystemMonitorConfigurationDefinition ActiveConfigDefinition
        {
            get { return _activeConfigDefinition; }
            set { 
                _activeConfigDefinition = value;
            }
        }


        /// <summary>
        /// Read only property that supplies a suitable base title for the managed window.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return Properties.Resources.SystemMonitor;
            }
        }

        
        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set
            {
                if (value == null)
                    return;

                _connectionDefinition = value;
            }
        }
        
        #endregion

        /// <summary>
        /// Creates a new SystemMonitorConfigurationControl.
        /// </summary>
        public SystemMonitorConfigurationControl()
        {
            InitializeComponent();
            Resize += SystemMonitorConfigurationControl_Resize;
            HideMetricsControls();
        }

        void SystemMonitorConfigurationControl_Resize(object sender, EventArgs e)
        {
            if (this.Width < 350)
            {
                sysStatTopBottom_flowLayoutPanel.FlowDirection = FlowDirection.TopDown;
                sysStatFlowLayout_flowLayoutPanel.FlowDirection = FlowDirection.TopDown;
                sysStatRedText_oneGuiLabel.Text = "* Requires an active \nODBC connection";
            }
            else
            {
                sysStatFlowLayout_flowLayoutPanel.FlowDirection = FlowDirection.LeftToRight;
                this.sysStatTopBottom_flowLayoutPanel.FlowDirection = FlowDirection.LeftToRight;

                sysStatRedText_oneGuiLabel.Text = "* Requires an active ODBC connection";
            }
        }

       /// <summary>
        /// Creates a new SystemMonitorControl using another one. It will copy all of the
        /// information from the given control and add itself as a listener to changes made
        /// to the given control.
        /// </summary>
        /// <param name="SystemMonitorControl">The control that will be copied and listened to.</param>
        public SystemMonitorConfigurationControl(SystemMonitorConfigurationControl aSystemMonitorConfigControl)
            : this()
        {
            this.ConnectionDefn = aSystemMonitorConfigControl.ConnectionDefn;
        }

        public void BuildSettingObjects()
        {
            //Miscelaneous settings
            this._activeConfigDefinition.Aggregate = this.miscAggSegs_radioButton.Checked;
            this._activeConfigDefinition.SwapBGonDisconnect = this.miscToggleBG_checkBox.Checked;
            this._activeConfigDefinition.ShowSegSeparators = this.miscShowSep_checkBox.Checked;
            this._activeConfigDefinition.TimelineMaxRange = Int32.Parse(this.miscTimelineMax_textBox.Text);

            //System Status settings
            //this._activeConfigDefinition.ShowStatDisks = this.sysStatDisk_checkBox.Checked;
            //this._activeConfigDefinition.ShowStatConnectivity = this.sysStatConn_checkBox.Checked;
            //this._activeConfigDefinition.ShowStatTransactions = this.sysStatTrans_checkBox.Checked;
            //this._activeConfigDefinition.ShowStatAlerts = this.sysStatAlarm_checkbox.Checked;
            //this._activeConfigDefinition.ShowStatusOnTop = this.sysStatTop_radioButton.Checked;

            //Connection settings
            this._activeConfigDefinition.NSMServerConfigDef.PortNumber = Int32.Parse(this.portNum_oneGuiMaskedTextBox.Text);
            this._activeConfigDefinition.NSMServerConfigDef.PortNumberINC = Int32.Parse(this.portNumINC_oneGuiMaskedTextBox.Text);
            this._activeConfigDefinition.NSMServerConfigDef.UseIncPort = this.portNumINC_Checkbox.Checked;
            this._activeConfigDefinition.NSMServerConfigDef.RefreshRate = Int32.Parse(this.refreshRate_MaskedTextBox.Text);

            //Build the settings object for MAX THRESHOLD VALUES
            this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.CPUBusy, this.maxValCPUBSY_MaskedTextBox.Text);
            //this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.CPUQueueLength, this.maxCPUQL_MaskedTextBox.Text);
            //this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.DiskIO, this.maxValDiskIO_MaskedTextBox.Text);
            //this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.Dispatch, this.maxDispatch_MaskedTextBox.Text);
            this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.FreeMemory, this.maxValFreeMem_MaskedTextBox.Text);
            //this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.Swap, this.maxValSwap_MaskedTextBox.Text);
            //this._activeConfigDefinition.NSMServerConfigDef.SetThresholdForMetric(MetricTypes.CacheHits, this.maxCache_MaskedTextBox.Text);

            //Misc color
            this._activeConfigDefinition.ColorMouseOver = this.colMiscMouse_button.BackColor;
            this._activeConfigDefinition.ColorBarBackground = this.colBGBar_button.BackColor;
            this._activeConfigDefinition.ColorDisconnected = this.colBGDiscon_button.BackColor;
            this._activeConfigDefinition.ColorCPUDown = this.colMiscCPUDown_button.BackColor;
            this._activeConfigDefinition.ColorThresholdExceeded = this.colMiscThreshExceeded_button.BackColor;

            this._activeConfigDefinition.ColorTimelineBack = this.colBGTimeline_button.BackColor;

            //Build the settings object for BAR COLOR
            this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CPUBusy).BarBrushColor = new SolidBrush(this.colCPUBSY_ColButton.BackColor);
            //this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CPUQueueLength).BarBrushColor = new SolidBrush(this.colCPUQL_button.BackColor);
            //this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.DiskIO).BarBrushColor = new SolidBrush(this.colDiskIO_ColButton.BackColor);
            //this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.Dispatch).BarBrushColor = new SolidBrush(this.colDispatch_Button.BackColor);
            this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.FreeMemory).BarBrushColor = new SolidBrush(this.colFreeMem_button.BackColor);
            //this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.Swap).BarBrushColor = new SolidBrush(this.colSwap_button.BackColor);
            //this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CacheHits).BarBrushColor = new SolidBrush(this.colCacheHits_ColButton.BackColor);
       
            this._activeConfigDefinition.AddMetric(MetricTypes.CPUBusy,this.metricCPUBSY_checkBox.Checked);
            //this._activeConfigDefinition.AddMetric(MetricTypes.CPUQueueLength,this.metricCPUQL_checkBox.Checked);
            //this._activeConfigDefinition.AddMetric(MetricTypes.DiskIO, this.metricDiskIO_checkBox.Checked);
            //this._activeConfigDefinition.AddMetric(MetricTypes.Dispatch,this.metricDispatch_checkBox.Checked);
            this._activeConfigDefinition.AddMetric(MetricTypes.FreeMemory,this.metricFreeMem_checkBox.Checked);
            //this._activeConfigDefinition.AddMetric(MetricTypes.Swap, this.metricSwap_checkBox.Checked);
            //this._activeConfigDefinition.AddMetric(MetricTypes.CacheHits, this.metricCache_checkBox1.Checked);

            //bool systemStatusEnabled = (this._activeConfigDefinition.ShowStatDisks || this._activeConfigDefinition.ShowStatConnectivity || this._activeConfigDefinition.ShowStatTransactions || this._activeConfigDefinition.ShowStatAlerts);
            //this._activeConfigDefinition.AddMetric(MetricTypes.SystemStatus, systemStatusEnabled); 

        }

        public void RebuildControlObjects()
        {

            if (this._activeConfigDefinition.MetricGUISettings.Count < 2)
            {
                this._activeConfigDefinition.InitializeMetricSettings();
            }

            //Miscelaneous settings
            this.miscAggSegs_radioButton.Checked = this._activeConfigDefinition.Aggregate;
            this.miscToggleBG_checkBox.Checked = this._activeConfigDefinition.SwapBGonDisconnect;
            this.miscShowSep_checkBox.Checked = this._activeConfigDefinition.ShowSegSeparators;
            this.miscTimelineMax_textBox.Text = this._activeConfigDefinition.TimelineMaxRange.ToString();

            //System Status settings
            //this.sysStatDisk_checkBox.Checked = this._activeConfigDefinition.ShowStatDisks;
            //this.sysStatConn_checkBox.Checked = this._activeConfigDefinition.ShowStatConnectivity;
            //this.sysStatTrans_checkBox.Checked = this._activeConfigDefinition.ShowStatTransactions;
            //this.sysStatAlarm_checkbox.Checked = this._activeConfigDefinition.ShowStatAlerts;//GetGUISettingsForMetric(MetricTypes.SystemStatus).IsEnabled;

            
            
            
            this.sysStatTop_radioButton.Checked = this._activeConfigDefinition.ShowStatusOnTop;

            //Connection settings
            this.portNum_oneGuiMaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.PortNumber.ToString();
            this.refreshRate_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.RefreshRate.ToString();

            if (null == this._activeConfigDefinition.NSMServerConfigDef.UseIncPort)
            {
                this._activeConfigDefinition.NSMServerConfigDef.UseIncPort = false;
                this._activeConfigDefinition.NSMServerConfigDef.PortNumberINC = 0;
            }

            this.portNumINC_Checkbox.Checked = this._activeConfigDefinition.NSMServerConfigDef.UseIncPort;
            this.portNumINC_oneGuiMaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.PortNumberINC.ToString();
            this.portNumINC_oneGuiMaskedTextBox.Enabled = this.portNumINC_Checkbox.Checked; 
 
            //Misc color
            this.colMiscMouse_button.BackColor = this._activeConfigDefinition.ColorMouseOver;
            this.colBGBar_button.BackColor = this._activeConfigDefinition.ColorBarBackground;
            this.colBGDiscon_button.BackColor = this._activeConfigDefinition.ColorDisconnected;
            this.colBGTimeline_button.BackColor = this._activeConfigDefinition.ColorTimelineBack;

            this.colMiscCPUDown_button.BackColor = this._activeConfigDefinition.ColorCPUDown;
            this.colMiscThreshExceeded_button.BackColor = this._activeConfigDefinition.ColorThresholdExceeded;

            this.colCPUBSY_ColButton.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CPUBusy).BarBrushColor).Color;
            //this.colCPUQL_button.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CPUQueueLength).BarBrushColor).Color;
            //this.colDiskIO_ColButton.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.DiskIO).BarBrushColor).Color;
            //this.colDispatch_Button.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.Dispatch).BarBrushColor).Color;
            this.colFreeMem_button.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.FreeMemory).BarBrushColor).Color;
            //this.colSwap_button.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.Swap).BarBrushColor).Color;
            //this.colCacheHits_ColButton.BackColor = new Pen(this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CacheHits).BarBrushColor).Color;

            //Build the settings object for MAX THRESHOLD VALUES
            this.maxValCPUBSY_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.CPUBusy).ToString();
            //this.maxCPUQL_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.CPUQueueLength).ToString();
            //this.maxValDiskIO_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.DiskIO).ToString();
            //this.maxDispatch_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.Dispatch).ToString();
            this.maxValFreeMem_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.FreeMemory).ToString();
            //this.maxValSwap_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.Swap).ToString();
            //this.maxCache_MaskedTextBox.Text = this._activeConfigDefinition.NSMServerConfigDef.GetThresholdForMetric(MetricTypes.CacheHits).ToString();

            this.metricCPUBSY_checkBox.Checked =this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CPUBusy).IsEnabled;
            //this.metricDiskIO_checkBox.Checked = this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.DiskIO).IsEnabled;
            //this.metricCache_checkBox1.Checked = this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CacheHits).IsEnabled;
            //this.metricDispatch_checkBox.Checked = this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.Dispatch).IsEnabled;
            //this.metricSwap_checkBox.Checked = this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.Swap).IsEnabled;
            this.metricFreeMem_checkBox.Checked = this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.FreeMemory).IsEnabled;
            //this.metricCPUQL_checkBox.Checked = this._activeConfigDefinition.GetGUISettingsForMetric(MetricTypes.CPUQueueLength).IsEnabled;
            
        
        }


        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            SystemMonitorConfigurationControl SystemMonitorConfigurationControl = new SystemMonitorConfigurationControl(this);
            return SystemMonitorConfigurationControl;
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            Resize -= SystemMonitorConfigurationControl_Resize;
        }

        public void showControl()
        {
            this.RebuildControlObjects();
            this.BringToFront();
            this.Visible = true;
            this.Show();
        }

        private void oneGuiButton1_Click(object sender, EventArgs e)
        {
            _systemMonitorToolTip.RemoveAll();
            BuildSettingObjects();

            //If someone is listening to the ConfigurationChanged event
            //let them know the settings have changed.
            if (ConfigurationChanged != null)
                ConfigurationChanged(this, this._activeConfigDefinition);
        }

        private void ColorButton_Click(object sender, EventArgs e)
        {
            ColorDialog MyDialog = new ColorDialog();
            // Keeps the user from selecting a custom color.
            MyDialog.AllowFullOpen = true;
            // Allows the user to get help. (The default is false.)
            MyDialog.ShowHelp = true;
            // Sets the initial color select to the current text color.
            MyDialog.Color = Color.Gray;
            MyDialog.ShowHelp = false;
            //MyDialog.ShowDialog();

            // Update the text box color if the user clicks OK 
            if (MyDialog.ShowDialog() == DialogResult.OK)
                ((Button)sender).BackColor = MyDialog.Color;
        }

        void textBox_KeyPressValidate(object sender, System.Windows.Forms.KeyPressEventArgs e)
        {
            char deleteChar = (char)Keys.Delete;
            bool deletePressed = (e.KeyChar == '\b');
            if (!System.Text.RegularExpressions.Regex.IsMatch(e.KeyChar.ToString(), "\\d+") && !deletePressed)
                e.Handled = true;

            if ((((TextBox)sender).SelectedText.Length >= ((TextBox)sender).TextLength || ((TextBox)sender).TextLength == 1) && deletePressed)
            {
                ((TextBox)sender).Text = "0";
                e.Handled = true;
            }
        }

        void textBox_PasteValidate(object sender, System.EventArgs e)
        {
            ((System.Windows.Forms.TextBox)sender).Text = System.Text.RegularExpressions.Regex.Replace(((System.Windows.Forms.TextBox)sender).Text, @"\D", "");
        }

        private void button25_Click(object sender, EventArgs e)
        {
            _systemMonitorToolTip.RemoveAll();
            //If someone is listening to the ConfigurationChanged event
            //let them know the settings have changed.
            if (ConfigurationChanged != null)
                ConfigurationChanged(this, this._activeConfigDefinition);
        }

        private void portNumINC_Checkbox_CheckedChanged(object sender, EventArgs e)
        {
            this.portNumINC_oneGuiMaskedTextBox.Enabled = this.portNumINC_Checkbox.Checked;
            if (!this.portNumINC_Checkbox.Checked)
            {
                errorProvider1.SetError(portNumINC_oneGuiMaskedTextBox, string.Empty);
                string inputnum = portNumINC_oneGuiMaskedTextBox.Text;
                if (string.IsNullOrEmpty(inputnum))
                {
                    portNumINC_oneGuiMaskedTextBox.Text = "0";
                }
                applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
            }
        }


        private void scrollPanel_oneGuiPanel_Scroll(object sender, ScrollEventArgs e)
        {
            _systemMonitorToolTip.RemoveAll();
        }


        private void portNum_oneGuiMaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            PortNumberStatus portNumberStatus = new PortNumberStatus(inputNum);
            string errorMessage = "Please enter a valid 'Port Number' between " + portNumberStatus.MinValue +
                " and " + portNumberStatus.MaxValue + ".";
            if (!portNumberStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
            
        }

        private void refreshRate_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            RefreshRateStatus refreshRateStatus = new RefreshRateStatus(inputNum);
            string errorMessage = "Please enter a valid 'Refresh Rate' number between " + refreshRateStatus.MinValue +
                " and " + refreshRateStatus.MaxValue + ".";
            if (!refreshRateStatus.IsValidBeforeReset)
            {
                _lblSeconds.Left = refreshRate_MaskedTextBox.Left + refreshRate_MaskedTextBox.Width + 15;
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                _lblSeconds.Left = refreshRate_MaskedTextBox.Left + refreshRate_MaskedTextBox.Width + 5;
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void portNumINC_oneGuiMaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            FixedIncomingStatus fixedIncomingStatus = new FixedIncomingStatus(inputNum);
            string errorMessage = "Please enter a valid 'Fixed Incoming' number between " + fixedIncomingStatus.MinValue +
                " and " + fixedIncomingStatus.MaxValue + ".";
            if (!fixedIncomingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxValCPUBSY_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            CPUBusySettingStatus cpuBusySettingStatus = new CPUBusySettingStatus(inputNum);
            string errorMessage = "Please enter a valid '% CPU Busy' number between " + cpuBusySettingStatus.MinValue +
                " and " + cpuBusySettingStatus.MaxValue + ".";
            if (!cpuBusySettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxValDiskIO_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            Other100SettingStatus other100SettingStatus = new Other100SettingStatus(inputNum);
            string errorMessage = "Please enter a valid 'Disk I/O' number between " + other100SettingStatus.MinValue +
                " and " + other100SettingStatus.MaxValue + ".";
            if (!other100SettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxCache_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            Other100SettingStatus other100SettingStatus = new Other100SettingStatus(inputNum);
            string errorMessage = "Please enter a valid 'Cache Hits' number between " + other100SettingStatus.MinValue +
                " and " + other100SettingStatus.MaxValue + ".";
            if (!other100SettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxDispatch_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            Other100SettingStatus other100SettingStatus = new Other100SettingStatus(inputNum);
            string errorMessage = "Please enter a valid 'Dispatch' number between " + other100SettingStatus.MinValue +
                " and " + other100SettingStatus.MaxValue + ".";
            if (!other100SettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxValSwap_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            Other100SettingStatus other100SettingStatus = new Other100SettingStatus(inputNum);
            string errorMessage = "Please enter a valid 'Swap' number between " + other100SettingStatus.MinValue +
                " and " + other100SettingStatus.MaxValue + ".";
            if (!other100SettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxValFreeMem_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            Other100SettingStatus other100SettingStatus = new Other100SettingStatus(inputNum);
            string errorMessage = "Please enter a valid 'Free Memory' number between " + other100SettingStatus.MinValue +
                " and " + other100SettingStatus.MaxValue + ".";
            if (!other100SettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private void maxCPUQL_MaskedTextBox_TextChanged(object sender, EventArgs e)
        {
            string inputNum = ((TrafodionMaskedTextBox)sender).Text.Trim();
            Other100SettingStatus other100SettingStatus = new Other100SettingStatus(inputNum);
            string errorMessage = "Please enter a valid 'CPU Queue Length' number between " + other100SettingStatus.MinValue +
                " and " + other100SettingStatus.MaxValue + ".";
            if (!other100SettingStatus.IsValidBeforeReset)
            {
                errorProvider1.SetError((Control)sender, errorMessage);
            }
            else
            {
                errorProvider1.SetError((Control)sender, String.Empty);
            }
            applyConfig_oneGuiButton.Enabled = !ErrorExistsInErrorProvider();
        }

        private bool ErrorExistsInErrorProvider()
        {
            // 1-5
            if (errorProvider1.GetError(portNum_oneGuiMaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(refreshRate_MaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(portNumINC_oneGuiMaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(maxValCPUBSY_MaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(maxValDiskIO_MaskedTextBox) != string.Empty)
                return true;

            // 6-10
            if (errorProvider1.GetError(maxCache_MaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(maxDispatch_MaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(maxValSwap_MaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(maxValFreeMem_MaskedTextBox) != string.Empty)
                return true;
            if (errorProvider1.GetError(maxCPUQL_MaskedTextBox) != string.Empty)
                return true;

            return false;
        }

        private void TrafodionMaskedTextBox_Enter(object sender, EventArgs e)
        {
            this.BeginInvoke(
                new SetMaskedTextBoxSelectAllDelegate(SetMaskedTextBoxSelectAll), new object[] 
                { (MaskedTextBox)sender });
        }

        private delegate void SetMaskedTextBoxSelectAllDelegate(MaskedTextBox txtbox);

        private void SetMaskedTextBoxSelectAll(MaskedTextBox txtbox)
        {
            txtbox.SelectAll();
        }

        //In Trafodion platform, only CPU and memory counters are displayed;
        //Also hide system counters: disk, alerts etc...
        private void HideMetricsControls() 
        {            
            this.flowLayoutPanel3.Visible = false;
            this.sysStatAlarm_checkbox.Checked = false;
            this.sysStatConn_checkBox.Checked = false;
            this.sysStatDisk_checkBox.Checked = false;
            this.sysStatTrans_checkBox.Checked = false;


            this.flowLayoutPanel7.Visible = false;
            this.flowLayoutPanel8.Visible = false;
            this.flowLayoutPanel9.Visible = false;
            this.flowLayoutPanel10.Visible = false;
            this.flowLayoutPanel12.Visible = false;

            this.metricCache_checkBox1.Checked = false;
            this.metricCPUQL_checkBox.Checked = false;
            this.metricDispatch_checkBox.Checked = false;
            this.metricSwap_checkBox.Checked = false;
            this.metricDiskIO_checkBox.Checked = false;

        }
    }
}
