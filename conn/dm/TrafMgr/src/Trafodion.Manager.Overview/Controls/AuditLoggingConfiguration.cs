#region Copyright info
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
#endregion Copyright info

using System;
using System.ComponentModel;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using System.Collections.Generic;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// Security Audit Logging Configuration
    /// </summary>
    public partial class AuditLoggingConfiguration : TrafodionForm
    {

        #region enums
        enum AuditLogsConfigOperation { Default, Reset, UpdateConfig };
        #endregion

        #region Member Variables

        ConnectionDefinition _connectionDefinition;
        TrafodionProgressUserControl _progressControl = null;
        BackgroundWorker _backgroundWorker = null;
        AuditLogsConfigDataModel _auditLogsConfigurationBackup = new AuditLogsConfigDataModel();
        AuditLogsConfigDataModel _auditLogsConfigurationUpdate = new AuditLogsConfigDataModel();

        private TrafodionChangeTracker _theChangeTracker = null;
        private AuditLogsConfigOperation _mode = AuditLogsConfigOperation.UpdateConfig;
        private DataTable _resultsAuditLogsConfiguration = new DataTable();
        private bool _IsChanged = false;
        private bool _isLoaded = false;
        #endregion

        #region Properties
        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _connectionDefinition;
            }
            set
            {
                _connectionDefinition = value;
            }
        }
        #endregion

        #region Contructor

        public AuditLoggingConfiguration()
        {
            InitializeComponent();
            //_theLogTypeComboBox.Visible = false; //For futuer use;
        }

        public AuditLoggingConfiguration(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            _connectionDefinition = aConnectionDefinition;
           // _bannerControl.ConnectionDefinition = ConnectionDefinition;
            this.Load += new System.EventHandler(this.ShowProgressBar);
            _isLoaded = false;
        }
        #endregion

        #region Private Events

        /// <summary>
        /// Constrain audit logging options enable/disable
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theEnableChkbox_CheckBoxCheckedChanged(object sender, EventArgs e)
        {
            EnableAuditLoggingOptions(_theEnableChkbox.Checked);
        }

        /// <summary>
        /// Show progress bar and try to fetch data
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ShowProgressBar(object sender, EventArgs e)
        {
            AuditLogsConfigModel auditLogsConfigModel = AuditLogsConfigModel.FindAuditLogsConfigModel(ConnectionDefinition);

            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.AuditLogsConfigLoadingMsg, auditLogsConfigModel, "DisplaySecurityAuditConfiguration", null);

            // Hide/disable some controls to avoid unexpect actions     
            this._progressPanel.Visible = true;
            this._progressPanel.Height = _theAuditLogsConfigPanel.Height;
            this._theAuditLogsConfigPanel.Visible = false;
            this._theLogTypePanel.Visible = false;
            this._progressBar.Visible = false;
            this._buttonsPanel.Visible = true;
            _theRestoreTrafodionButton.Enabled = false;
            _theResetTrafodionButton.Enabled = false;
            _theClearTrafodionButton.Enabled = false;
            _theOKTrafodionButton.Enabled = false;
            _theCloseTrafodionButton.Enabled = true;

            _progressPanel.Controls.Clear();
            _progressControl = new TrafodionProgressUserControl(args);
            _progressControl.ProgressCompletedEvent += DisplayAuditLogsConfiguration_Completed;
            _progressControl.Dock = DockStyle.Fill;
            _progressPanel.Controls.Add(_progressControl);
        }

        private void DisplayAuditLogsConfiguration_Completed(object sender, TrafodionProgressCompletedArgs e)
        {

            this._progressControl.ProgressCompletedEvent -= DisplayAuditLogsConfiguration_Completed;
            if (_progressControl.Error != null)
            {
                //Display error message to the label.
                Label errorLabel = new Label();
                errorLabel.Dock = DockStyle.Fill;
                errorLabel.Text = _progressControl.Error.Message;
                errorLabel.ForeColor = System.Drawing.Color.Red;
                _progressPanel.Controls.Clear();
                _progressPanel.Controls.Add(errorLabel);
                return;
            }
                        
            SetThresholdTooltips();
            //initialize the log type.
            InitLogType();
            InitLogFailsAction();
            _resultsAuditLogsConfiguration = (DataTable)e.ReturnValue;
            GetDataModel(Convert.ToInt32(_theLogTypeComboBox.SelectedValue));
            populateUIFromConfigModel(_auditLogsConfigurationBackup);
            //_isLoaded = true;
            _progressPanel.Visible = false;
            _theLogTypePanel.Visible = true;
            _theAuditLogsConfigPanel.Visible = true;
            _theClearTrafodionButton.Enabled = false;
            _theOKTrafodionButton.Enabled = false;
            _theRestoreTrafodionButton.Enabled = true;
            _theResetTrafodionButton.Enabled = true;
            //if (ConnectionDefinition.ServerVersion <= ConnectionDefinition.SERVER_VERSION.SQ140)
            //{
            HideOptionsForM9();
            //}
            AddChangeTracker();
        }

       

        private void _theOKTrafodionButton_Click(object sender, EventArgs e)
        {
            //if (!validatingThreshold())
            //{
            //    MessageBox.Show(Properties.Resources.AuditLogSpaceUsageValidateMessage,"Invalid Space Usage input",MessageBoxButtons.OK,MessageBoxIcon.Error);
            //    return;
            //}
            _mode = AuditLogsConfigOperation.UpdateConfig;
            DoWorkBackground();            
        }

        private void _theRestoreTrafodionButton_Click(object sender, EventArgs e)
        {
            DialogResult userInput = MessageBox.Show(Properties.Resources.AuditLogsRestoreWarningMsg, "Audit Logging Configuration", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
            if (userInput == DialogResult.OK)
            {
                _mode = AuditLogsConfigOperation.Default;
                DoWorkBackground();
            }
        }

        private void _theClearTrafodionButton_Click(object sender, EventArgs e)
        {
            Clear();
        }

        private void _theResetTrafodionButton_Click(object sender, EventArgs e)
        {
            DialogResult userInput = MessageBox.Show(Properties.Resources.AuditLogsResetDefaultWarningMsg, "Audit Logging Configuration", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
            if (userInput == DialogResult.OK)
            {
                _mode = AuditLogsConfigOperation.Reset;
                DoWorkBackground();
            }
        }

        /// <summary>
        /// Event handler for change detected.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeTracker_OnChangeDetected(object sender, System.EventArgs e)
        {            
            //Update UI to Model,then sync button status.
            UpdateButtons();
        }

        /// <summary>
        /// only number can be input
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theRefreshTimesTextbox_KeyPress(object sender, KeyPressEventArgs e)
        {
            e.Handled = !char.IsDigit(e.KeyChar) && !char.IsControl(e.KeyChar);
        }

        private void _theHelpTrafodionButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.AuditLogConfiguration);
        }

        private void _theLogTypeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_theLogTypeComboBox.SelectedValue.Equals(AuditLogsConfigDataModel.LOG_TYPE_SAR_CODE))
            {
                _theLogToAlternateRadiobox.Text = Properties.Resources.AuditLogAlternateSARLabel;
            }
            else if (_theLogTypeComboBox.SelectedValue.Equals(AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_CODE))
            {
                _theLogToAlternateRadiobox.Text = Properties.Resources.AuditLogAlternateSyslogLabel;
            }

            if (_isLoaded)
            {
                GetDataModel(Convert.ToInt32(_theLogTypeComboBox.SelectedValue));
                Clear();
            }

        }

        private void _theLogTypeComboBox_SelectedIndexChanging(object sender, CancelEventArgs e)
        {
            if (_theOKTrafodionButton.Enabled)
            {
                DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(),
                    Properties.Resources.AuditLogCommitChangesWarningMessage,
                Properties.Resources.Confirm, MessageBoxButtons.YesNoCancel);

                if (result == DialogResult.Yes)
                {
                    //Commit the changes.
                    _mode = AuditLogsConfigOperation.UpdateConfig;
                    DoWorkBackground();
                }
                else if (result == DialogResult.No)
                {
                    //do nothing  
                }
                else if (result == DialogResult.Cancel)
                {
                    e.Cancel = true;
                }
            }
        }

        #endregion Events

        #region Private Methods

        //private bool validatingThreshold()
        //{
        //    if (_theSpaceUsageNotifyNumUpDown.Value > _theSpaceUsageWarnNumUpDown.Value ||
        //        _theSpaceUsageWarnNumUpDown.Value > _theSpaceUsageDialoutNumUpDown.Value ||
        //        _theSpaceUsageNotifyNumUpDown.Value > _theSpaceUsageDialoutNumUpDown.Value)
        //    {
        //        return false;
        //    }
        //    return true;
        //}

        private void HideOptionsForM9()
        {
            _theLogTypeComboBox.Enabled = false;
            _theLogTypePanel.Visible = false;
            _theLoggingFailActionLabel.Visible = false;
            
            _theAuditLogsFailActionLabelPanel.Visible = false;
            _theEnableChkbox.Height = _theEnableChkbox.Height - _theAuditLogsFailActionLabelPanel.Height;
            
            //_theSpaceUsagePanel.Visible = false;
            //_theAuditLogsConfigPanel.Height = _theAuditLogsConfigPanel.Height - _theAuditLogAlternateGroup.Height;
            this.Height -= _theAuditLogAlternateGroup.Height;
            _theAuditLogsConfigChangesLogFailActionDropdown.Visible = false;
            _theAuthSucessFailActionDropdown.Visible = false;
            _theAuthFailuresActionDropdown.Visible = false;
            _thePrivilegeChangesFailuresActionDropdown.Visible = false;
            _theUserMgmtChangesLogFailActionDropdown.Visible = false;
            _theObjectChangesLogFailActionDropdown.Visible = false;
            _theAuthorViolationsLogFailActionDropdown.Visible = false;
            _theAuditLogAlternateGroup.Visible = false;
            
        }

        /// <summary>
        /// Discarding user input
        /// </summary>
        private void Clear()
        {            
            populateUIFromConfigModel(_auditLogsConfigurationBackup);
            UpdateButtons();
         }

        /// <summary>
        /// Monitoring changes.
        /// </summary>
        private void AddChangeTracker()
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
            _theChangeTracker = new TrafodionChangeTracker(_theAuditLogsConfigPanel);
            _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(ChangeTracker_OnChangeDetected);
            _theChangeTracker.EnableChangeEvents = true;
        }

        private void DoWorkBackground()
        {
            //Disable buttons
            this._theClearTrafodionButton.Enabled = false;
            this._theRestoreTrafodionButton.Enabled = false;
            this._theResetTrafodionButton.Enabled = false;
            this._theOKTrafodionButton.Enabled = false;
            this._theCloseTrafodionButton.Enabled = false;

            //Initiate the upload as a background operation           
            initBackgroundWorker();
            _backgroundWorker.RunWorkerAsync();
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            CancelAsync();
            RemoveEventHandlers();
        }

        /// <summary>
        /// Cancels any currently running background work
        /// </summary>
        private void CancelAsync()
        {
            if (_backgroundWorker != null && _backgroundWorker.IsBusy)
            {
                _backgroundWorker.CancelAsync();
            }
        }

        private void RemoveEventHandlers()
        {
            _theLogTypeComboBox.SelectedIndexChanged -= _theLogTypeComboBox_SelectedIndexChanged;
            _theLogTypeComboBox.SelectedIndexChanging -= _theLogTypeComboBox_SelectedIndexChanging;
            _theChangeTracker.OnChangeDetected -= ChangeTracker_OnChangeDetected;
        }

        private void InitLogType()
        {
            _theLogTypeComboBox.Items.Clear();
            List<DropdownItemObject> logTypeList = new List<DropdownItemObject>();
            logTypeList.Add(new DropdownItemObject(AuditLogsConfigDataModel.LOG_TYPE_SAR_DESC, AuditLogsConfigDataModel.LOG_TYPE_SAR_CODE));
            logTypeList.Add(new DropdownItemObject(AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_DESC, AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_CODE));
            _theLogTypeComboBox.DataSource = logTypeList;
            _theLogTypeComboBox.ValueMember = "Code";
            _theLogTypeComboBox.DisplayMember = "Desc";
            _theLogTypeComboBox.SelectedIndex = 0;
            _theLogTypeComboBox.SelectedIndexChanged += _theLogTypeComboBox_SelectedIndexChanged;
            _theLogTypeComboBox.SelectedIndexChanging += _theLogTypeComboBox_SelectedIndexChanging;
        }



        private void InitLogFailsAction()
        {

            List<DropdownItemObject> logFailsActionList = new List<DropdownItemObject>();
            logFailsActionList.Add(new DropdownItemObject(AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_DESC1, AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1));
            logFailsActionList.Add(new DropdownItemObject(AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_DESC2, AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE2));
            logFailsActionList.Add(new DropdownItemObject(AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_DESC3, AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE3));

            _theAuditLogsConfigChangesLogFailActionDropdown.Items.Clear();
            _theAuditLogsConfigChangesLogFailActionDropdown.DataSource=logFailsActionList;
            _theAuditLogsConfigChangesLogFailActionDropdown.ValueMember="Code";
            _theAuditLogsConfigChangesLogFailActionDropdown.DisplayMember="Desc";

            _theAuthSucessFailActionDropdown.Items.Clear();
            _theAuthSucessFailActionDropdown.BindingContext = new BindingContext();
            _theAuthSucessFailActionDropdown.DataSource=logFailsActionList;
            _theAuthSucessFailActionDropdown.ValueMember="Code";
            _theAuthSucessFailActionDropdown.DisplayMember="Desc";

            _theAuthFailuresActionDropdown.Items.Clear();
            _theAuthFailuresActionDropdown.BindingContext = new BindingContext();
            _theAuthFailuresActionDropdown.DataSource=logFailsActionList;
            _theAuthFailuresActionDropdown.ValueMember="Code";
            _theAuthFailuresActionDropdown.DisplayMember = "Desc";

            _thePrivilegeChangesFailuresActionDropdown.Items.Clear();
            _thePrivilegeChangesFailuresActionDropdown.BindingContext = new BindingContext();
            _thePrivilegeChangesFailuresActionDropdown.DataSource=logFailsActionList;
            _thePrivilegeChangesFailuresActionDropdown.ValueMember = "Code";
            _thePrivilegeChangesFailuresActionDropdown.DisplayMember = "Desc";

            _theUserMgmtChangesLogFailActionDropdown.Items.Clear();
            _theUserMgmtChangesLogFailActionDropdown.BindingContext = new BindingContext();
            _theUserMgmtChangesLogFailActionDropdown.DataSource=logFailsActionList;
            _theUserMgmtChangesLogFailActionDropdown.ValueMember = "Code";
            _theUserMgmtChangesLogFailActionDropdown.DisplayMember = "Desc";

            _theObjectChangesLogFailActionDropdown.Items.Clear();
            _theObjectChangesLogFailActionDropdown.BindingContext = new BindingContext();
            _theObjectChangesLogFailActionDropdown.DataSource=logFailsActionList;
            _theObjectChangesLogFailActionDropdown.ValueMember = "Code";
            _theObjectChangesLogFailActionDropdown.DisplayMember = "Desc";

            _theAuthorViolationsLogFailActionDropdown.Items.Clear();
            _theAuthorViolationsLogFailActionDropdown.BindingContext = new BindingContext();
            _theAuthorViolationsLogFailActionDropdown.DataSource=logFailsActionList;
            _theAuthorViolationsLogFailActionDropdown.ValueMember = "Code";
            _theAuthorViolationsLogFailActionDropdown.DisplayMember = "Desc";

        }

 
        /// <summary>
        /// Assign server data to client data model
        /// </summary>
        /// <param name="logType"></param>
        private void GetDataModel(int logTypeCode)
        {
             
            if (_resultsAuditLogsConfiguration != null && _resultsAuditLogsConfiguration.Rows.Count > 0)
            {
                _auditLogsConfigurationBackup.AuditLoggingEnable = GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_LOGGING_ENABLE_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.RefreshConfigTime = Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.REFRESH_CONFIG_TIME_NUMBER - 1]["CONFIG_VALUE"]);

                _auditLogsConfigurationBackup.AuditConfigurationChanges = 
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_CONFIGURATION_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditConfigurationChangesFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_CONFIGURATION_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.AuditAuthenticationSuccess =
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_AUTHENTICATION_SUCCESS_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditAuthenticationSuccessFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_AUTHENTICATION_SUCCESS_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.AuditAuthenticationFailure =
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_AUTHENTICATION_FAILURE_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditAuthenticationFailureFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_AUTHENTICATION_FAILURE_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.AuditObjectChanges =
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_OBJECT_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditObjectChangesFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_OBJECT_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.AuditPrivilegeChanges =
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_PRIVILEGE_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditPrivilegeChangesFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_PRIVILEGE_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.AuditSecurityViolations =
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_SECURITY_VIOLATIONS_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditSecurityViolationsFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_SECURITY_VIOLATIONS_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.AuditUserMgmtChanges =
                    GetBoolFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_USER_MGMT_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());
                _auditLogsConfigurationBackup.AuditUserMgmtChangesFailAction =
                    GetSingleValueFromString(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_USER_MGMT_CHANGES_NUMBER - 1]["CONFIG_VALUE"].ToString());

                _auditLogsConfigurationBackup.TableAging = 
                    Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.TABLE_AGING_NUMBER - 1]["CONFIG_VALUE"]);
                _auditLogsConfigurationBackup.SpaceUsageNotifyThreshold = 
                    Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.SPACE_USAGE_NOTIFY_THRESHOLD_NUMBER - 1]["CONFIG_VALUE"]);
                _auditLogsConfigurationBackup.SpaceUsageWarnThreshold = 
                    Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.SPACE_USAGE_WARN_THRESHOLD_NUMBER - 1]["CONFIG_VALUE"]);
                _auditLogsConfigurationBackup.SpaceUsageDialoutThreshold = 
                    Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.SPACE_USAGE_DIALOUT_THRESHOLD_NUMBER - 1]["CONFIG_VALUE"]);
                _auditLogsConfigurationBackup.AuditLogTarget = 1;
                    //Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_LOG_TARGET - 1]["CONFIG_VALUE"]);
                _auditLogsConfigurationBackup.AuditLogAlternate = 0;
                    //Convert.ToInt32(_resultsAuditLogsConfiguration.Rows[AuditLogsConfigDataModel.AUDIT_LOG_ALTERNATE - 1]["CONFIG_VALUE"]);

                if (_auditLogsConfigurationBackup.AuditLogTarget.ToString().Equals(AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_CODE))
                {
                    _auditLogsConfigurationBackup.LogType = AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_DESC;
                }
                else
                {
                    _auditLogsConfigurationBackup.LogType = AuditLogsConfigDataModel.LOG_TYPE_SAR_DESC;
                }                
            }
        }

        /// <summary>
        /// status message setting.
        /// </summary>
        /// <param name="statusMessage"></param>
        private void ReportStatusStrip(String statusMessage)
        {
            _statusLabel.Text = statusMessage;
            _progressBar.Visible = !string.IsNullOrEmpty(statusMessage);
        }

        /// <summary>
        /// constrain Enable or Disable audit logging options 
        /// </summary>
        private void EnableAuditLoggingOptions(bool enable)
        {
            _theAuditLogsConfigChangesChkbox.Checked = enable;
            _theAuthSucessChkbox.Checked = enable;
            _theAuthFailuresChkbox.Checked = enable;
            _thePrivilegeChangesChkbox.Checked = enable;
            _theUserMgmtChangesChkbox.Checked = enable;
            _theObjectChangesChkbox.Checked = enable;
            _theAuthorViolationsChkbox.Checked = enable;
        }
      

        /// <summary>
        /// Set tooltips for the threshold values.
        /// </summary>
        private void SetThresholdTooltips()
        {
            _theToolTip.SetToolTip(_theTableAgingNumUpDown, Properties.Resources.AuditLogTableAgingTooltip);
            _theToolTip.SetToolTip(_theRefreshConfigNumUpDown, Properties.Resources.AuditLogRefreshTooltip);
            //_theToolTip.SetToolTip(_theSpaceUsageNotifyNumUpDown, Properties.Resources.AuditLogSpaceUsageNotifyTooltip);
            //_theToolTip.SetToolTip(_theSpaceUsageWarnNumUpDown, Properties.Resources.AuditLogSpaceUsageWarnTooltip);
            //_theToolTip.SetToolTip(_theSpaceUsageDialoutNumUpDown, Properties.Resources.AuditLogSpaceUsageDialoutTooltip);
        }

        /// <summary>
        /// Save client user input to client datamodel.
        /// </summary>
        private void UpdateButtons()
        {
            _auditLogsConfigurationUpdate = populateConfigModelFromUI();
            _IsChanged = !_auditLogsConfigurationBackup.Equals(_auditLogsConfigurationUpdate);
            UpdateButtons(_IsChanged);
        }

        /// <summary>
        /// update controls status
        /// </summary>
        private void UpdateButtons(bool modelChanged)
        {
            _theClearTrafodionButton.Enabled = modelChanged;
            _theOKTrafodionButton.Enabled = modelChanged;
            _theAuditLogsConfigChangesLogFailActionDropdown.Enabled = _theAuditLogsConfigChangesChkbox.Checked;
            _theAuthSucessFailActionDropdown.Enabled = _theAuthSucessChkbox.Checked;
            _theAuthFailuresActionDropdown.Enabled = _theAuthFailuresChkbox.Checked;            
            _theObjectChangesLogFailActionDropdown.Enabled =_theObjectChangesChkbox.Checked ;
            _thePrivilegeChangesFailuresActionDropdown.Enabled = _thePrivilegeChangesChkbox.Checked;
            _theAuthorViolationsLogFailActionDropdown.Enabled =_theAuthorViolationsChkbox.Checked;
            _theUserMgmtChangesLogFailActionDropdown.Enabled = _theUserMgmtChangesChkbox.Checked;            
            _theCloseTrafodionButton.Enabled = true;
            _theRestoreTrafodionButton.Enabled = true;
            _theResetTrafodionButton.Enabled = true;
        }
        
        /// <summary>
        /// Helper method to populate the model from the UI
        /// </summary>
        /// <returns>Client data model</returns>
        private AuditLogsConfigDataModel populateConfigModelFromUI()
        {
            _auditLogsConfigurationUpdate.AuditLogTarget = Convert.ToInt32(_theLogTypeComboBox.SelectedValue);
            _auditLogsConfigurationUpdate.AuditLoggingEnable = _theEnableChkbox.Checked;
            _auditLogsConfigurationUpdate.RefreshConfigTime = Convert.ToInt32(_theRefreshConfigNumUpDown.Value);
            
            _auditLogsConfigurationUpdate.AuditConfigurationChanges = _theAuditLogsConfigChangesChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditConfigurationChangesFailAction = _theAuditLogsConfigChangesLogFailActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.AuditAuthenticationSuccess = _theAuthSucessChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditAuthenticationSuccessFailAction = _theAuthSucessFailActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.AuditAuthenticationFailure = _theAuthFailuresChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditAuthenticationFailureFailAction = _theAuthFailuresActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.AuditObjectChanges = _theObjectChangesChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditObjectChangesFailAction = _theObjectChangesLogFailActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.AuditPrivilegeChanges = _thePrivilegeChangesChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditPrivilegeChangesFailAction = _thePrivilegeChangesFailuresActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.AuditSecurityViolations = _theAuthorViolationsChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditSecurityViolationsFailAction = _theAuthorViolationsLogFailActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.AuditUserMgmtChanges = _theUserMgmtChangesChkbox.Checked;
            _auditLogsConfigurationUpdate.AuditUserMgmtChangesFailAction = _theUserMgmtChangesLogFailActionDropdown.SelectedValue.ToString();

            _auditLogsConfigurationUpdate.TableAging = Convert.ToInt32(_theTableAgingNumUpDown.Value);
            //_auditLogsConfigurationUpdate.SpaceUsageNotifyThreshold = Convert.ToInt32(_theSpaceUsageNotifyNumUpDown.Value);
            //_auditLogsConfigurationUpdate.SpaceUsageWarnThreshold = Convert.ToInt32(_theSpaceUsageWarnNumUpDown.Value);
            //_auditLogsConfigurationUpdate.SpaceUsageDialoutThreshold = Convert.ToInt32(_theSpaceUsageDialoutNumUpDown.Value);
            
            _auditLogsConfigurationUpdate.AuditLogAlternate = 0;
            if (_theLogToAlternateRadiobox.Checked)
            {
                if (_theLogTypeComboBox.SelectedValue.ToString().Equals(AuditLogsConfigDataModel.LOG_TYPE_SAR_CODE))
                {
                    _auditLogsConfigurationUpdate.AuditLogAlternate=Convert.ToInt32(AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_CODE);
                }
                else if (_theLogTypeComboBox.SelectedValue.ToString().Equals(AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_CODE))
                {
                    _auditLogsConfigurationUpdate.AuditLogAlternate = Convert.ToInt32(AuditLogsConfigDataModel.LOG_TYPE_SAR_CODE);
                }
                 
            }
            _auditLogsConfigurationUpdate.LogType = string.Empty; 
            if (_auditLogsConfigurationUpdate.AuditLogTarget.ToString().Equals(AuditLogsConfigDataModel.LOG_TYPE_SAR_CODE))
            {
                _auditLogsConfigurationUpdate.LogType = AuditLogsConfigDataModel.LOG_TYPE_SAR_DESC;
            }
            else if (_auditLogsConfigurationUpdate.AuditLogTarget.ToString().Equals(AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_CODE))
            {
                _auditLogsConfigurationUpdate.LogType = AuditLogsConfigDataModel.LOG_TYPE_SYSLOG_DESC;
            }

            return _auditLogsConfigurationUpdate;
        }

        /// <summary>
        /// Display model to UI.
        /// </summary>
        /// <param name="anAuditLogsConfiguration">Data model</param>
        private void populateUIFromConfigModel(AuditLogsConfigDataModel anAuditLogsConfiguration)
        {
            _theEnableChkbox.Checked = anAuditLogsConfiguration.AuditLoggingEnable;
            _theRefreshConfigNumUpDown.Value = anAuditLogsConfiguration.RefreshConfigTime;

            _theAuditLogsConfigChangesChkbox.Checked = anAuditLogsConfiguration.AuditConfigurationChanges;

            _theAuditLogsConfigChangesLogFailActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditConfigurationChangesFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 : 
                anAuditLogsConfiguration.AuditConfigurationChangesFailAction;

            _theAuthSucessChkbox.Checked = anAuditLogsConfiguration.AuditAuthenticationSuccess;
            _theAuthSucessFailActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 : 
                anAuditLogsConfiguration.AuditAuthenticationSuccessFailAction;

            _theAuthFailuresChkbox.Checked = anAuditLogsConfiguration.AuditAuthenticationFailure;
            _theAuthFailuresActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditAuthenticationFailureFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 :
                anAuditLogsConfiguration.AuditAuthenticationFailureFailAction ;

            _theObjectChangesChkbox.Checked = anAuditLogsConfiguration.AuditObjectChanges;
            _theObjectChangesLogFailActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditObjectChangesFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 : 
                anAuditLogsConfiguration.AuditObjectChangesFailAction;

            _thePrivilegeChangesChkbox.Checked = anAuditLogsConfiguration.AuditPrivilegeChanges;
            _thePrivilegeChangesFailuresActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditPrivilegeChangesFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 : 
                anAuditLogsConfiguration.AuditPrivilegeChangesFailAction;

            _theAuthorViolationsChkbox.Checked = anAuditLogsConfiguration.AuditSecurityViolations;
            _theAuthorViolationsLogFailActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditSecurityViolationsFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 :
                anAuditLogsConfiguration.AuditSecurityViolationsFailAction ;

            _theUserMgmtChangesChkbox.Checked = anAuditLogsConfiguration.AuditUserMgmtChanges;
            _theUserMgmtChangesLogFailActionDropdown.SelectedValue =
                anAuditLogsConfiguration.AuditUserMgmtChangesFailAction.Equals(string.Empty) ? 
                AuditLogsConfigDataModel.LOG_FAIL_ACTION_OPT_CODE1 : 
                anAuditLogsConfiguration.AuditUserMgmtChangesFailAction;

            _theTableAgingNumUpDown.Value = anAuditLogsConfiguration.TableAging;
            //_theSpaceUsageNotifyNumUpDown.Value = anAuditLogsConfiguration.SpaceUsageNotifyThreshold;
            //_theSpaceUsageWarnNumUpDown.Value = anAuditLogsConfiguration.SpaceUsageWarnThreshold;
            //_theSpaceUsageDialoutNumUpDown.Value = anAuditLogsConfiguration.SpaceUsageDialoutThreshold;

            _theLogTypeComboBox.SelectedValue = anAuditLogsConfiguration.AuditLogTarget.ToString();
            if (anAuditLogsConfiguration.AuditLogAlternate == 0)
            {
                _theNotLogToAlternateRadiobox.Checked = true;
                _theLogToAlternateRadiobox.Checked = false;
            }
            else
            {
                _theNotLogToAlternateRadiobox.Checked = false;
                _theLogToAlternateRadiobox.Checked = true;
            }

        }

        /// <summary>
        /// transfer string value to bool
        /// </summary>
        /// <param name="input">Y or N</param>
        /// <returns>True or False</returns>
        private bool GetBoolFromString(string input)
        {
            if (input.Trim().Length > 0 && input.Trim().Substring(0, 1).ToUpper().Equals("Y"))
            {
                return true;
            }
            return false;
        }

 
        /// <summary>
        /// Get a single status from a string
        /// the second character indicates what action to take if the logging operation fails.
        /// </summary>
        /// <param name="input">a string</param>
        /// <returns>the second character</returns>
        private string GetSingleValueFromString(string input)
        {
            if (input.Trim().Length > 1 )
            {
                return input.Substring(1, 1).ToUpper();
            }
            return string.Empty;
        }


        #endregion Private Methods

        #region BackgroundWorker
        /// <summary>
        /// initialize a background worker.
        /// </summary>
        private void initBackgroundWorker()
        {
            _backgroundWorker = new BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(_backgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(_backgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(_backgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// Do work of updating Audit Logging Configuration
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            BackgroundWorker worker = sender as BackgroundWorker;

            try
            {
                HandleUpdateAuditLogsConfig(worker, e);                
            }
            catch (Exception ex)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }
                throw ex;
            }
        }

        /// <summary>
        /// update status to UI progress bar.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            string msg = string.Empty;
            switch (e.ProgressPercentage)
            {
                case 0:
                    {
                        ReportStatusStrip("Applying the changes...");
                        break;
                    }
                case 50:
                    {
                        ReportStatusStrip("Reload the data...");
                        break;
                    }
                case 100:
                    {
                        ReportStatusStrip(string.Empty);
                        break;
                    }
                default:
                    break;

            }
        }

        /// <summary>
        /// worker completes process.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            ReportStatusStrip(string.Empty);

            if (e.Error != null)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), e.Error.Message,
                    "Audit Log Configuration",
                    MessageBoxButtons.OK, MessageBoxIcon.Error); 
                //restore button status
                UpdateButtons();
            }
            else
            {                
                _resultsAuditLogsConfiguration = (DataTable)e.Result;
                GetDataModel(Convert.ToInt32(_theLogTypeComboBox.SelectedValue));
                populateUIFromConfigModel(_auditLogsConfigurationBackup);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), Properties.Resources.AuditLogsConfigUpdateSuccessfullyMsg,
                   "Audit Log Configuration",
                   MessageBoxButtons.OK, MessageBoxIcon.Information); 
            }

            this._theRestoreTrafodionButton.Enabled = true;
            this._theResetTrafodionButton.Enabled = true;
            this._theCloseTrafodionButton.Enabled = true;
        }

       /// <summary>
       /// Process updating jobs
       /// </summary>
       /// <param name="worker"></param>
       /// <param name="e"></param>
        private void HandleUpdateAuditLogsConfig(BackgroundWorker worker, DoWorkEventArgs e)
        {
            AuditLogsConfigOperation operation = _mode;
            AuditLogsConfigModel model=AuditLogsConfigModel.FindAuditLogsConfigModel(ConnectionDefinition);
            DataTable dtResult = null;
            switch (operation)
            {
                case AuditLogsConfigOperation.Default:
                    {
                        worker.ReportProgress(0);
                        model.RestoreSecurityAuditConfiguration(_auditLogsConfigurationBackup.LogType);
                        worker.ReportProgress(50);
                        e.Result = model.DisplaySecurityAuditConfiguration();
                        worker.ReportProgress(100);
                        break;
                    }
                case AuditLogsConfigOperation.Reset:
                    {
                        worker.ReportProgress(0);
                        model.ResetSecurityAuditConfiguration(_auditLogsConfigurationBackup.LogType);
                        worker.ReportProgress(50);
                        e.Result = model.DisplaySecurityAuditConfiguration();
                        worker.ReportProgress(100);
                        break;
                    }
                case AuditLogsConfigOperation.UpdateConfig:
                    {
                        worker.ReportProgress(0);
                        model.AlterSecurityAuditConfiguration(_auditLogsConfigurationBackup.AssemblyAlterAuditConfigCommand(_auditLogsConfigurationUpdate).ToString());
                        worker.ReportProgress(50);
                        e.Result = model.DisplaySecurityAuditConfiguration();
                        worker.ReportProgress(100);
                        break;
                    }
                
                default:
                    {
                        break;
                    }
            }            
          
        }

        #endregion
    }
}
