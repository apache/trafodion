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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User control that's used to edit system configuration settings
    /// </summary>
    public partial class EditSystemConfigurationUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        WmsSystem _wmsSystem;

        static string[] TRACE_OBJECTS_R23 =
        {
            "OFF",
            "ALL",
            "QSMGR",
            "QSCOM",
            "QSSTATS",
            "QSSYNC",
            "QSSYNCAS"
        };

        static string[] TRACE_OBJECTS_R24 =
        {
            "OFF",
            "ALL",
            "QSMGR",
            "QSCOM",
            "QSRULE",
            "QSSTATS",
            "QSSYNC",
            "QSSYNCAS"
        };

        public const int MAX_TRACE_FILENAME_LENGTH = 50;
        public const int MAX_TRACE_PATH_LENGTH = 256;
        public const string TRACE_OFF = "OFF";

        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public WmsSystem WmsSystem
        {
            get { return _wmsSystem; }
            set 
            {
                if (_wmsSystem != null)
                {
                    _wmsSystem.WmsModelEvent -= _wmsSystem_WmsModelEvent;
                }
                _wmsSystem = value; 
                if(_wmsSystem != null)
                {
                    _wmsSystem.WmsModelEvent += _wmsSystem_WmsModelEvent;
                    InitializeFromSystemModel(_wmsSystem);
                }
            }
        }

        private string CancelQueryIfClientDisappears
        {
            get
            {
                return (string)ddlCancelQueryIfClientDisappears.SelectedItem;
            }
            set
            {
                ddlCancelQueryIfClientDisappears.SelectedItem = value;
            }
        }

        private string CheckQueryEstimatedResourceUse
        {
            get
            {
                return (string)ddlCheckQueryEstimatedResourceUse.SelectedItem;
            }
            set
            {
                ddlCheckQueryEstimatedResourceUse.SelectedItem = value;
            }
        }

        #endregion Properties

        public EditSystemConfigurationUserControl()
        {
            InitializeComponent();
        }
        /// <summarzzzy>
        /// Constructor for the Edit System Configuration User Control
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public EditSystemConfigurationUserControl(WmsSystem aWmsSystem)
            :this()
        {
            InitializeControl();    
            WmsSystem = aWmsSystem;
        }

        void _wmsSystem_WmsModelEvent(object sender, WmsObject.WmsModelEventArgs e)
        {
            RefreshWMSInfo();
        }

        private void InitializeControl()
        {
            ddlCancelQueryIfClientDisappears.Items.AddRange(new string[] { WmsCommand.SWITCH_ON, WmsCommand.SWITCH_OFF });
            ddlCheckQueryEstimatedResourceUse.Items.AddRange(new string[] { WmsCommand.SWITCH_ON, WmsCommand.SWITCH_OFF });
            
            _traceFileNameTextBox.MaxLength = MAX_TRACE_FILENAME_LENGTH;
            _traceFilePathTextBox.MaxLength = MAX_TRACE_PATH_LENGTH;
        }

        /// <summary>
        /// Backward compatibility supprot
        /// </summary>
        private void SetBackwardCompatibility()
        {
            // For M8 or later
            lblMaxExecQuery.Visible = nudMaxExecQuery.Visible 
                = lblMaxESP.Visible = nudMaxESP.Visible 
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130;
            
            // For M9 or later
            grbCanaryQuery.Visible 
                = lblCancelQuery.Visible = ddlCancelQueryIfClientDisappears.Visible
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140;

            // For M10 or later
            lblCheckQueryEstimatedResourceUse.Visible = ddlCheckQueryEstimatedResourceUse.Visible
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150;

            // For M11 or later
            _maxTransactionRollbackLabel.Visible = _maxTransactionRollbackUpDown.Visible
                = lblCanaryTimeout.Visible = nudCanaryTimeout.Visible
                = this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ160;

            // Hide MAX SSD Usage
            _maxOverflowUsageLabel.Visible = _maxOverflowUsageUpDown.Visible = false;

#warning : below should be removed once Canary Timeout is ready
            _maxTransactionRollbackLabel.Visible = _maxTransactionRollbackUpDown.Visible = false;
            lblCanaryTimeout.Visible = nudCanaryTimeout.Visible = false;
        }

        private void InitializeButtonStatus() 
        {
            this._refreshButton.Enabled = this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString());
            this._applyButton.Enabled = this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString());

            this._startButton.Enabled = _statusTextBox.Text.Equals(WmsCommand.STOPPED_STATE) && this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_START.ToString());
            this._stopButton.Enabled = !_statusTextBox.Text.Equals(WmsCommand.STOPPED_STATE) && this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STOP.ToString());

            this._holdButton.Enabled = _statusTextBox.Text.Equals(WmsCommand.ACTIVE_STATE) && this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());
            this._releaseButton.Enabled = _statusTextBox.Text.Equals(WmsCommand.HOLD_STATE) && this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());            

            this._exportButton.Enabled = this.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString());
        }

        private void InitializeFromSystemModel(WmsSystem aWmsSystem)
        {
            SetBackwardCompatibility();     

            _statusTextBox.Text = aWmsSystem.State;
            _maxCPUBusyUpDown.Value = aWmsSystem.MaxCpuBusy;
            _maxMemoryUsageUpDown.Value = aWmsSystem.MaxMemUsage;
            _statsIntervalUpDown.Value = aWmsSystem.StatsInterval;
            nudMaxExecQuery.Value = aWmsSystem.MaxExecQueries;
            nudMaxESP.Value = aWmsSystem.MaxEsps;

            // For M9 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                nudCanaryInterval.Value = aWmsSystem.CanaryIntervalMinutes;
                nudCanaryExec.Value = aWmsSystem.CanaryExecSeconds;
                txtCanaryQuery.Text = aWmsSystem.CanaryQuery;

                CancelQueryIfClientDisappears = aWmsSystem.CancelQueryIfClientDisappears;
            }

            // For M10 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                CheckQueryEstimatedResourceUse = aWmsSystem.CheckQueryEstimatedResourceUse;
            }

#warning : below should be restored once ready
            //// For M11 or later
            //if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ160)
            //{
            //    _maxTransactionRollbackUpDown.Value = aWmsSystem.MaxTransactionRollbackMinutes;
            //    nudCanaryTimeout.Value = aWmsSystem.CanaryTimeoutSeconds;
            //}
            
            _ruleIntervalUpDown.Value = aWmsSystem.RuleInterval;
            _execTimeoutUpDown.Value = aWmsSystem.ExecTimeout;
            _waitTimeoutUpDown.Value = aWmsSystem.WaitTimeout;
            _holdTimeoutUpDown.Value = aWmsSystem.HoldTimeout;
            _maxRowsFetchedUpDown.Text = aWmsSystem.MaxRowsFetched.ToString();
            _maxOverflowUsageUpDown.Value = aWmsSystem.MaxSSDUsage;
            //Do not show trace options. These are not used for Trafodion
            HideTraceOptions();

            //Initialize Button Status according to system state and WMS component privilege, 
            //InitializeFromSystemModel command is called every time whenever this control is first created or added again
            InitializeButtonStatus();
        }
        /// <summary>
        /// Hides controls that are not available in R2.3 systems
        /// </summary>
        private void HideAttributes(bool visible)
        {
            _ruleIntervalLabel.Visible = visible;
            _execTimeoutLabel.Visible = visible;
            _waitTimeoutLabel.Visible = visible;
            _holdTimeoutLabel.Visible = visible;
            _maxRowsFetchedLabel.Visible = visible;
            _ruleIntervalUpDown.Visible = visible;
            _execTimeoutUpDown.Visible = visible;
            _waitTimeoutUpDown.Visible = visible;
            _holdTimeoutUpDown.Visible = visible;
            _maxRowsFetchedUpDown.Visible = visible;
            _timeOutGroupBox.Visible = visible;

            _maxOverflowUsageLabel.Visible = visible;
            _maxOverflowUsageUpDown.Visible = visible;

            thresholdsGroupBox.SetBounds(thresholdsGroupBox.Bounds.X, thresholdsGroupBox.Bounds.Y,_statusGroupBox.Size.Width, thresholdsGroupBox.Bounds.Height);
        }

        /// <summary>
        /// Hides controls that are only for services users
        /// </summary>
        private void HideTraceOptions()
        {
            traceGroupBox.Visible = false;
            _traceFileNameLabel.Visible = false;
            _traceFileNameTextBox.Visible = false;
            _traceFilePathLabel.Visible = false;
            _traceFilePathTextBox.Visible = false;
            _traceObjectComboBox.Visible = false;
            _traceObjectLabel.Visible = false;
        }

        private void _resetButton_Click(object sender, EventArgs e)
        {
            _wmsSystem.Refresh();
            RefreshWMSInfo();
        }


        private void RefreshWMSInfo()
        {
            try
            {
                ShowWMSInfo();
                InitializeButtonStatus();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error fetching WMS system configuration. \n\n" +
                                "Problem: \t Unable to get the WMS system configuration. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Fetch WMS configuration", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void ShowWMSInfo()
        {
            _statusTextBox.Text = _wmsSystem.State;
            _maxCPUBusyUpDown.Value = _wmsSystem.MaxCpuBusy;
            _maxMemoryUsageUpDown.Value = _wmsSystem.MaxMemUsage;
            _statsIntervalUpDown.Value = _wmsSystem.StatsInterval;
            _maxRowsFetchedUpDown.Text = _wmsSystem.MaxRowsFetched.ToString();
            _execTimeoutUpDown.Value = _wmsSystem.ExecTimeout;
            _waitTimeoutUpDown.Value = _wmsSystem.WaitTimeout;
            _holdTimeoutUpDown.Value = _wmsSystem.HoldTimeout;
            _ruleIntervalUpDown.Value = _wmsSystem.RuleInterval;
            _ruleIntervalQueryExecTimeUpDown.Value = _wmsSystem.RuleIntervalQueryExecTime;
            _maxOverflowUsageUpDown.Value = _wmsSystem.MaxSSDUsage;
            nudMaxExecQuery.Value = _wmsSystem.MaxExecQueries;
            nudMaxESP.Value = _wmsSystem.MaxEsps;

            // For M9 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                nudCanaryInterval.Value = _wmsSystem.CanaryIntervalMinutes;
                nudCanaryExec.Value = _wmsSystem.CanaryExecSeconds;
                txtCanaryQuery.Text = _wmsSystem.CanaryQuery;

                CancelQueryIfClientDisappears = _wmsSystem.CancelQueryIfClientDisappears;
            }

            // For M10 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                CheckQueryEstimatedResourceUse = _wmsSystem.CheckQueryEstimatedResourceUse;
            }

#warning : below should be restored once Canary Timeout is ready
            //// For M11 or later
            //if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ160)
            //{
            //    _maxTransactionRollbackUpDown.Value = _wmsSystem.MaxTransactionRollbackMinutes;
            //    nudCanaryTimeout.Value = _wmsSystem.CanaryTimeoutSeconds;
            //}

            if (this._wmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
            {
                _traceObjectComboBox.Text = _wmsSystem.TraceObject;
                _traceFilePathTextBox.Text = _wmsSystem.TraceFilePath;
                _traceFileNameTextBox.Text = _wmsSystem.TraceFileName;
            }
        }



        private bool isTraceValid()
        {
            bool valid = true;

            if (WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
            {
                if (!_traceObjectComboBox.Text.Equals(TRACE_OFF))
                {
                    if (_traceFilePathTextBox.Text.Trim().Length == 0 || _traceFileNameTextBox.Text.Trim().Length == 0)
                    {
                        valid = false;
                    }
                }
            }

            return valid;
        }

        private void _applyButton_Click(object sender, EventArgs e)
        {            
            WmsSystem savedCopy = (WmsSystem)_wmsSystem.Clone();

            try
            {
                //Read information from the UI controls into the model
                GetInfoFromControls();

                this.Cursor = Cursors.WaitCursor;
                _wmsSystem.Alter();
                _wmsSystem.Refresh();
                RefreshWMSInfo();
                MessageBox.Show("The WMS system configuration has been altered successfully.",
                                TrafodionForm.TitlePrefix + "Alter WMS configuration", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                //If the alter failed, reset the attributes from the backup
                Utilities.CopyProperties(savedCopy, ref _wmsSystem);

                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to alter the WMS system configuration. Details = " + ex.Message);

                MessageBox.Show("Failed to alter WMS system configuration. \n\n" +
                                "Problem: \t Unable to alter the WMS system configuration. \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Alter WMS configuration", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                this.Cursor = Cursors.Default;
            }
        }

        private void GetInfoFromControls()
        {
            _wmsSystem.MaxCpuBusy = int.Parse(_maxCPUBusyUpDown.Text);
            _wmsSystem.MaxMemUsage = int.Parse(_maxMemoryUsageUpDown.Text);
            _wmsSystem.StatsInterval = int.Parse(_statsIntervalUpDown.Text);

            _wmsSystem.MaxRowsFetched = Int64.Parse(_maxRowsFetchedUpDown.Text);
            _wmsSystem.ExecTimeout = int.Parse(_execTimeoutUpDown.Text);
            _wmsSystem.WaitTimeout = int.Parse(_waitTimeoutUpDown.Text);
            _wmsSystem.HoldTimeout = int.Parse(_holdTimeoutUpDown.Text);
            _wmsSystem.RuleInterval = int.Parse(_ruleIntervalUpDown.Text);
            _wmsSystem.RuleIntervalQueryExecTime = int.Parse(_ruleIntervalQueryExecTimeUpDown.Text);
            _wmsSystem.MaxSSDUsage = int.Parse(_maxOverflowUsageUpDown.Text);
            _wmsSystem.MaxExecQueries = int.Parse(nudMaxExecQuery.Text);
            _wmsSystem.MaxEsps = int.Parse(nudMaxESP.Text);

            // For M9 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                _wmsSystem.CanaryIntervalMinutes = int.Parse(nudCanaryInterval.Text);
                _wmsSystem.CanaryExecSeconds = int.Parse(nudCanaryExec.Text);
                _wmsSystem.CanaryQuery = txtCanaryQuery.Text;

                _wmsSystem.CancelQueryIfClientDisappears = CancelQueryIfClientDisappears;
            }

            // For M10 or later
            if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _wmsSystem.CheckQueryEstimatedResourceUse = CheckQueryEstimatedResourceUse;
            }

#warning : below should be restored once Canary Timeout is ready
            //// For M11 or later
            //if (this._wmsSystem.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ160)
            //{
            //    _wmsSystem.MaxTransactionRollbackMinutes = int.Parse(_maxTransactionRollbackUpDown.Text);
            //    _wmsSystem.CanaryTimeoutSeconds = int.Parse(nudCanaryTimeout.Text);
            //}
        }

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new EditSystemConfigurationUserControl(WmsSystem);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.EditSystemConfig + " - " + WmsSystem.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return WmsSystem.ConnectionDefinition; }
        }

        #endregion

        private void _releaseButton_Click(object sender, EventArgs e)
        {
            try
            {
                _wmsSystem.ReleaseWMS();
            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to release WMS from hold state. Details = " + ex.Message);

                MessageBox.Show("Failed to release WMS from hold state. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Release WMS", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void _holdButton_Click(object sender, EventArgs e)
        {
            try
            {
                DialogResult result = MessageBox.Show(Properties.Resources.ConfirmHoldWMS,
                                                      TrafodionForm.TitlePrefix + Properties.Resources.HoldWMS, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    _wmsSystem.HoldWMS();
                }
            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to hold WMS. Details = " + ex.Message);

                MessageBox.Show("Failed to hold WMS. \n\nDetails: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Hold WMS", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void _stopButton_Click(object sender, EventArgs e)
        {
            try
            {
                WMSCommandStopConfirmationDialog wmsStopDialog = new WMSCommandStopConfirmationDialog(true);

                if (DialogResult.Yes == wmsStopDialog.ShowDialog())
                {
                    _wmsSystem.StopWMS(wmsStopDialog.StopImmediately);
                }
            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to stop WMS. Details = " + ex.Message);

                MessageBox.Show("Failed to stop WMS. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Stop WMS", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void _startButton_Click(object sender, EventArgs e)
        {
            try
            {
                _wmsSystem.StartWMS();
            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to start the WMS system. Details = " + ex.Message);

                MessageBox.Show("Failed to start WMS. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Start WMS", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void _statusTextBox_TextChanged(object sender, EventArgs e)
        {
            InitializeButtonStatus();            
        }

        private void _exportButton_Click(object sender, EventArgs e)
        {
            if (WmsSystem != null)
            {
                //Pass the source wmssystem from which the menu was clicked, to the ExportConfigControl constructor
                ExportConfigControl exportConfigControl = new ExportConfigControl(WmsSystem);

                //Place the ExportConfigControl user control into a managed window
                Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(exportConfigControl.Size, exportConfigControl, Properties.Resources.ExportConfiiguration + " - " + WmsSystem.ConnectionDefinition.Name, WmsSystem.ConnectionDefinition);
            }

        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.WMSSystemConfiguration);
        }

        private void btnReset_Click(object sender, EventArgs e)
        {
            try
            {
                _wmsSystem.ResetCanaryQuerySqlText();
                txtCanaryQuery.Text = _wmsSystem.CanaryQuery;
            }
            catch (Exception ex)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Failed to reset Canary Query SQL text. Details = " + ex.Message);

                MessageBox.Show("Failed to reset Canary Query SQL text. \n\n" +
                                "Problem: \t Unable to reset Canary Query SQL text (ALTER WMS CANARY_QUERY RESET). \n\n" +
                                "Solution: \t Please check error details for recovery information. \n\n" +
                                "Details: " + "\t " + ex.Message + "\n\n",
                                TrafodionForm.TitlePrefix + "Reset Canary Query SQL Text", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void _maxTransactionRollbackUpDown_ValueChanged(object sender, EventArgs e)
        {
            // Skip 1 and 2, which are not valid value for Transaction Rollback Timeout
            NumericUpDown nudTransactionRollback = (NumericUpDown)sender;
            if (nudTransactionRollback.Value > 0 && nudTransactionRollback.Value <2)
            {
                nudTransactionRollback.Value = 3;
            }
            else if (nudTransactionRollback.Value >= 2 && nudTransactionRollback.Value < 3)
            {
                nudTransactionRollback.Value = 0;
            }
        } 

    }
}
