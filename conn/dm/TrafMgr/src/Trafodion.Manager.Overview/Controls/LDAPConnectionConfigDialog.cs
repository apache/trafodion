//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.ComponentModel;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// LDAP configuration Tool dialog
    /// </summary>
    public partial class LDAPConnectionConfigDialog : TrafodionForm
    {
        const string STR_Unencrypted = "Unencrypted";
        const string STR_SSL = "SSL";
        const string STR_TLS = "TLS";
        const string PORT_Unencrypted_OR_TLS = "389";
        const string PORT_SSL = "636";
        const string OPTION_YES = "Yes";
        const string OPTION_NO = "No";
        const string OPTION_ENTERPRISE = "Enterprise";
        const string OPTION_CLUSTER = "Cluster";

        private LDAPConfigFileHandler handler;
        private ConnectionDefinition connectionDefinition;
        private String savedCerLocation = string.Empty;
        private string savedCer = string.Empty;
        //the cer which has been loaded
        private string backupCerLocation = string.Empty;
        private string backupCer = string.Empty;
        private Hashtable _operationParamaters = new Hashtable();

        private bool isAsyncRunning = false;
        private bool isCertificateContentChangeHandled = true;
        private TrafodionPanel asyncPanel = null;
        private TrafodionProgressUserControl asyncProgressor = null;

        LDAPConfigObject _theLDAPConfigObjectUpdate = null;
        LDAPConfigObject _theLDAPConfigObjectSaved = null;
        private TrafodionChangeTracker _theChangeTracker = null;
        private bool _isLoaded = false;
        private bool _hasError = false;
        private bool _isPreM10Version = false;
        private string _dbRootDefaultRole = string.Empty;
        private bool _isFirstLoad = true;
        private bool isOperationCancelled = false;
        private TabPage currentTabPage = null;
        private bool disableEnterprise, disableCluster;
        private Dictionary<string, string> localErrorMap = new Dictionary<string, string>();
        private Dictionary<string, string> remoteErrorMap = new Dictionary<string, string>();
        private Dictionary<string, string> defaultErrorMap = new Dictionary<string, string>();

        private int RefreshTime
        {
            get
            {
                return Int32.Parse(this.refreshTimeNumericUpDown.Value.ToString());
            }
        }

        public LDAPConnectionConfigDialog(ConnectionDefinition theConnectionDefinition)
        {
            InitializeComponent();
            InitializeControl();
            handler = new LDAPConfigFileHandler(theConnectionDefinition);
            this.connectionDefinition = theConnectionDefinition;
            InitializeBackgoundWorker();
            InitializeData();
            AddChangeTracker();
            saveButton.Enabled = false;
            refreshButton.Enabled = false;
            this.FormClosing += MyFormClosing;

            this.configAttributeSettingControl.OnRowsChangedImpl += configAttributeSettingControl_OnRowsChangedImpl;
            this.configAttributeSettingControl1.OnRowsChangedImpl += configAttributeSettingControl_OnRowsChangedImpl;

            this.Text = Trafodion.Manager.Properties.Resources.ConfigureLDAP;
            this.configurationToolTip.SetToolTip(this.hostNameTextBox, Properties.Resources.HostNameSeparator);
            this.configurationToolTip.SetToolTip(this.hostNameTextBox1, Properties.Resources.HostNameSeparator);
            this.configurationToolTip.SetToolTip(this.loadServerCerButton, Properties.Resources.ToolTiploadServerCerButton);
            this.configurationToolTip.SetToolTip(this.browseButton, Properties.Resources.ToolTipBrowseButton);
        }

        private void configAttributeSettingControl_OnRowsChangedImpl()
        {
            UpdateSaveButton();
        }

        private void HighlightCertificateChange(bool isChanged)
        {
            certificateRichTextBox.ForeColor = isChanged ? System.Drawing.Color.Blue : System.Drawing.SystemColors.WindowText;
        }
        
        private void InitializeControl()
        {
            string[] defaultSections = { OPTION_ENTERPRISE, OPTION_CLUSTER };
            defaultSectionComboBox.Items.AddRange(defaultSections);

            string[] preserveConnections = { OPTION_YES, OPTION_NO };
            connectionComboBox.Items.AddRange(preserveConnections);
            connectionComboBox1.Items.AddRange(preserveConnections);

            string[] encryptTypes = { STR_Unencrypted, STR_SSL, STR_TLS };
            encryptTypeComboBox.Items.AddRange(encryptTypes);
            encryptTypeComboBox1.Items.AddRange(encryptTypes);

            this.asyncPanel = new TrafodionPanel();
            this.defaultsGroupBox.Controls.Add(this.asyncPanel);
            this.asyncPanel.Anchor = cerLocationTextBox.Anchor;
            this.asyncPanel.Location = certificateRichTextBox.Location;
            this.asyncPanel.Size = certificateRichTextBox.Size;
            this.asyncPanel.Padding = new Padding(0);
            this.asyncPanel.Margin = new Padding(0);
            this.asyncPanel.Visible = false;
        }

        private void InitializeData()
        {
            _operationParamaters.Clear();
            _operationParamaters.Add(KeyStore.Operation, LDAPConfigFileHandler.ConfigOperation.List);

            setControlsStatus(false);
            this.saveButton.Enabled = this.refreshButton.Enabled = false;
            this.toolStripStatusLabel1.Text = Properties.Resources.ConfigLoading;

            this.backgroundWorker.RunWorkerAsync(_operationParamaters);
        }

        /// <summary>
        /// enable or disable the controls on the panel
        /// </summary>
        /// <param name="enable"></param>
        private void setControlsStatus(bool enable)
        {
            sectionGroupBox.Enabled = enable;
            sectionTabControl.Enabled = enable;
            defaultsGroupBox.Enabled = enable;
            //this.saveButton.Enabled = this.refreshButton.Enabled = enable;
            toolStripProgressBar1.Visible = !enable;
            toolStripStatusLabel1.Visible = !enable;
        }

        /// <summary>
        /// display the values
        /// </summary>
        /// <param name="configContent"></param>
        /// <param name="cerContent"></param>
        /// <param name="LDAPRCContent"></param>
        private void populateUIfromModel(String configContent, String cerContent, String LDAPRCContent)
        {
            _theLDAPConfigObjectUpdate = new LDAPConfigObject(this.connectionDefinition);
            _theLDAPConfigObjectUpdate.FillObject(configContent, cerContent, LDAPRCContent);

            //Do a copy for object to use for compairing or reset.
            _theLDAPConfigObjectSaved = new LDAPConfigObject(this.connectionDefinition);
            _theLDAPConfigObjectSaved.defaultSection = _theLDAPConfigObjectUpdate.defaultSection != null ?
                (LDAPConfigDefaultSection)_theLDAPConfigObjectUpdate.defaultSection.Clone() : null;
            _theLDAPConfigObjectSaved.remote = _theLDAPConfigObjectUpdate.remote != null ?
                (LDAPConfigSection)_theLDAPConfigObjectUpdate.remote.Clone() : null;
            _theLDAPConfigObjectSaved.local = _theLDAPConfigObjectUpdate.local != null ?
                (LDAPConfigSection)_theLDAPConfigObjectUpdate.local.Clone() : null;

            this.configAttributeSettingControl.Clear();
            this.configAttributeSettingControl1.Clear();

            //update local value
            if (_theLDAPConfigObjectUpdate.local == null)
            {
                hideLocalSection();
            }
            else
            {
                this.localCheckBox.Checked = true;
                if (sectionTabControl.Contains(tabPageCluster))
                    sectionTabControl.Controls.Remove(tabPageCluster);
                if (!sectionTabControl.Contains(tabPageEnterprise))
                    sectionTabControl.Controls.Add(this.tabPageEnterprise);
                if (this.remoteCheckBox.Checked)
                    sectionTabControl.Controls.Add(tabPageCluster);
                //hostname
                this.hostNameTextBox.Text=String.Empty;
                for (int i = 0; i < _theLDAPConfigObjectUpdate.local.LdapHostname.Count; i++)
                {
                    if (i + 1 < _theLDAPConfigObjectUpdate.local.LdapHostname.Count)
                        this.hostNameTextBox.AppendText(_theLDAPConfigObjectUpdate.local.LdapHostname[i] + ", ");
                    else
                        this.hostNameTextBox.AppendText(_theLDAPConfigObjectUpdate.local.LdapHostname[i] + " ");
                }
                //type
                if (_theLDAPConfigObjectUpdate.local.LDAPSSL == 0)
                    this.encryptTypeComboBox.Text = STR_Unencrypted;
                else if (_theLDAPConfigObjectUpdate.local.LDAPSSL == 1)
                    this.encryptTypeComboBox.Text = STR_SSL;
                else if (_theLDAPConfigObjectUpdate.local.LDAPSSL == 2)
                    this.encryptTypeComboBox.Text = STR_TLS;
                //port
                if (Convert.ToInt32(_theLDAPConfigObjectUpdate.local.LdapPort) < 1 || Convert.ToInt32(_theLDAPConfigObjectUpdate.local.LdapPort) > 65535)
                {
                    if (this.encryptTypeComboBox.Text.Equals(STR_Unencrypted) || this.encryptTypeComboBox.Text.Equals(STR_TLS))
                        this.portTextBox.Text = PORT_Unencrypted_OR_TLS;
                    else if (this.encryptTypeComboBox.Text.Equals(STR_SSL))
                        this.portTextBox.Text = PORT_SSL;
                    else
                        this.portTextBox.Text = "1";
                }
                else
                    this.portTextBox.Text = _theLDAPConfigObjectUpdate.local.LdapPort;

                this.searchDNTextBox.Text = _theLDAPConfigObjectUpdate.local.LDAPSearchDN;
                passwdMaskedTextBox.Text = _theLDAPConfigObjectUpdate.local.LDAPSearchPWD;
                confirmPasswdMaskedTextBox.Text = _theLDAPConfigObjectUpdate.local.LDAPSearchPWD;
                connectionComboBox.Text = _theLDAPConfigObjectUpdate.local.PreserveConnection ? OPTION_YES : OPTION_NO;

                this.configAttributeSettingControl.populateGrid(_theLDAPConfigObjectUpdate.local.attributes);

                if (_theLDAPConfigObjectUpdate.local.LDAPNetworkTimeout < networkTimeoutNumericUpDown.Minimum || _theLDAPConfigObjectUpdate.local.LDAPNetworkTimeout > networkTimeoutNumericUpDown.Maximum)
                    networkTimeoutNumericUpDown.Value = 30;
                else networkTimeoutNumericUpDown.Value = _theLDAPConfigObjectUpdate.local.LDAPNetworkTimeout;

                if (_theLDAPConfigObjectUpdate.local.LDAPTimelimit < timelimitNumericUpDown.Minimum || _theLDAPConfigObjectUpdate.local.LDAPTimelimit > timelimitNumericUpDown.Maximum)
                    timelimitNumericUpDown.Value = 30;
                else
                    timelimitNumericUpDown.Value = _theLDAPConfigObjectUpdate.local.LDAPTimelimit;

                if (_theLDAPConfigObjectUpdate.local.LDAPTimeout < ldapTimeoutNumericUpDown.Minimum || _theLDAPConfigObjectUpdate.local.LDAPTimeout > ldapTimeoutNumericUpDown.Maximum)
                    ldapTimeoutNumericUpDown.Value = 30;
                else
                    ldapTimeoutNumericUpDown.Value = _theLDAPConfigObjectUpdate.local.LDAPTimeout;

                if (_theLDAPConfigObjectUpdate.local.RetryCount < retryCountNumericUpDown.Minimum || _theLDAPConfigObjectUpdate.local.RetryCount > retryCountNumericUpDown.Maximum)
                    retryCountNumericUpDown.Value = 5;
                else
                    retryCountNumericUpDown.Value = _theLDAPConfigObjectUpdate.local.RetryCount;

                if (_theLDAPConfigObjectUpdate.local.RetryDelay < retryDelayNumericUpDown.Minimum || _theLDAPConfigObjectUpdate.local.RetryDelay > retryDelayNumericUpDown.Maximum)
                    retryDelayNumericUpDown.Value = 2;
                else
                    retryDelayNumericUpDown.Value = _theLDAPConfigObjectUpdate.local.RetryDelay;
            }


            //update remote value
            if (_theLDAPConfigObjectUpdate.remote == null)
            {
                hideRemoteSection();
            }
            else
            {
                this.remoteCheckBox.Checked = true;
                if (!sectionTabControl.Contains(tabPageCluster))
                    sectionTabControl.Controls.Add(this.tabPageCluster);
                //hostname
                this.hostNameTextBox1.Text = String.Empty;
                for (int i = 0; i < _theLDAPConfigObjectUpdate.remote.LdapHostname.Count; i++)
                {
                    if (i + 1 < _theLDAPConfigObjectUpdate.remote.LdapHostname.Count)
                        this.hostNameTextBox1.AppendText(_theLDAPConfigObjectUpdate.remote.LdapHostname[i] + " , ");
                    else
                        this.hostNameTextBox1.AppendText(_theLDAPConfigObjectUpdate.remote.LdapHostname[i] + " ");
                }

                //type
                if (_theLDAPConfigObjectUpdate.remote.LDAPSSL == 0)
                    this.encryptTypeComboBox1.Text = STR_Unencrypted;
                else if (_theLDAPConfigObjectUpdate.remote.LDAPSSL == 1)
                    this.encryptTypeComboBox1.Text = STR_SSL;
                else if (_theLDAPConfigObjectUpdate.remote.LDAPSSL == 2)
                    this.encryptTypeComboBox1.Text = STR_TLS;

                //port
                if (Convert.ToInt32(_theLDAPConfigObjectUpdate.remote.LdapPort) < 1 || Convert.ToInt32(_theLDAPConfigObjectUpdate.remote.LdapPort) > 65535)
                {
                    if (this.encryptTypeComboBox1.Text.Equals(STR_Unencrypted) || this.encryptTypeComboBox1.Text.Equals(STR_TLS))
                        this.portTextBox1.Text = PORT_Unencrypted_OR_TLS;
                    else if (this.encryptTypeComboBox1.Text.Equals(STR_SSL))
                        this.portTextBox1.Text = PORT_SSL;
                    else
                        this.portTextBox1.Text = "1";
                }
                else
                    this.portTextBox1.Text = _theLDAPConfigObjectUpdate.remote.LdapPort.ToString();

                this.searchDNTextBox1.Text = _theLDAPConfigObjectUpdate.remote.LDAPSearchDN;
                passwdMaskedTextBox1.Text = _theLDAPConfigObjectUpdate.remote.LDAPSearchPWD;
                confirmPasswdMaskedTextBox1.Text = _theLDAPConfigObjectUpdate.remote.LDAPSearchPWD;
                connectionComboBox1.Text = _theLDAPConfigObjectUpdate.remote.PreserveConnection ? OPTION_YES : OPTION_NO;

                if (_theLDAPConfigObjectUpdate.remote.LDAPNetworkTimeout < networkTimeoutNumericUpDown1.Minimum || _theLDAPConfigObjectUpdate.remote.LDAPNetworkTimeout > networkTimeoutNumericUpDown1.Maximum)
                    networkTimeoutNumericUpDown1.Value = 30;
                else networkTimeoutNumericUpDown1.Value = _theLDAPConfigObjectUpdate.remote.LDAPNetworkTimeout;

                if (_theLDAPConfigObjectUpdate.remote.LDAPTimelimit < timelimitNumericUpDown1.Minimum || _theLDAPConfigObjectUpdate.remote.LDAPTimelimit > timelimitNumericUpDown1.Maximum)
                    timelimitNumericUpDown1.Value = 30;
                else
                    timelimitNumericUpDown1.Value = _theLDAPConfigObjectUpdate.remote.LDAPTimelimit;

                if (_theLDAPConfigObjectUpdate.remote.LDAPTimeout < ldapTimeoutNumericUpDown1.Minimum || _theLDAPConfigObjectUpdate.remote.LDAPTimeout > ldapTimeoutNumericUpDown1.Maximum)
                    ldapTimeoutNumericUpDown1.Value = 30;
                else
                    ldapTimeoutNumericUpDown1.Value = _theLDAPConfigObjectUpdate.remote.LDAPTimeout;

                if (_theLDAPConfigObjectUpdate.remote.RetryCount < retryCountNumericUpDown1.Minimum || _theLDAPConfigObjectUpdate.remote.RetryCount > retryCountNumericUpDown1.Maximum)
                    retryCountNumericUpDown1.Value = 5;
                else
                    retryCountNumericUpDown1.Value = _theLDAPConfigObjectUpdate.remote.RetryCount;

                if (_theLDAPConfigObjectUpdate.remote.RetryDelay < retryDelayNumericUpDown1.Minimum || _theLDAPConfigObjectUpdate.remote.RetryDelay > retryDelayNumericUpDown1.Maximum)
                    retryDelayNumericUpDown1.Value = 2;
                else
                    retryDelayNumericUpDown1.Value = _theLDAPConfigObjectUpdate.remote.RetryDelay;

                this.configAttributeSettingControl1.populateGrid(_theLDAPConfigObjectUpdate.remote.attributes);
            }

            
            if (_theLDAPConfigObjectUpdate.defaultSection.refreshTime < refreshTimeNumericUpDown.Minimum || _theLDAPConfigObjectUpdate.defaultSection.refreshTime > refreshTimeNumericUpDown.Maximum)
                this.refreshTimeNumericUpDown.Value = 30;
            else
                this.refreshTimeNumericUpDown.Value = _theLDAPConfigObjectUpdate.defaultSection.refreshTime;
            this.refreshTimeCheckBox.Checked = !(this.refreshTimeNumericUpDown.Value == 0);
            this.refreshTimeNumericUpDown.Enabled = !(this.refreshTimeNumericUpDown.Value == 0);

            SetDefaultSection(_theLDAPConfigObjectUpdate.defaultSection);

            if (_theLDAPConfigObjectUpdate.local != null)
                this.localErrorMap = _theLDAPConfigObjectUpdate.local.errorMap;
            if (_theLDAPConfigObjectUpdate.remote != null)
                this.remoteErrorMap = _theLDAPConfigObjectUpdate.remote.errorMap;
            if (_theLDAPConfigObjectUpdate.defaultSection != null)
                this.defaultErrorMap = _theLDAPConfigObjectUpdate.defaultSection.errorMap;

            if (currentTabPage != null)
            {
                if (this.sectionTabControl.Contains(currentTabPage))
                    sectionTabControl.SelectedTab = currentTabPage;
                currentTabPage = null;
            } 
        }

        /// <summary>
        /// Initialize the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            //Before do that, first prepare for the case which user has canceled the request.
            backgroundWorker.WorkerReportsProgress = true;
            backgroundWorker.WorkerSupportsCancellation = true;
            backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }
        
        #region BackgroundWorker event
        /// <summary>
        /// Background Method that is asynchronously invoked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender, System.ComponentModel.DoWorkEventArgs e)
        {
            isOperationCancelled = false;
            Hashtable argument = e.Argument as Hashtable;
            LDAPConfigFileHandler.ConfigOperation configOperation = (LDAPConfigFileHandler.ConfigOperation)argument[KeyStore.Operation];

            Hashtable result = new Hashtable();
            result.Add(KeyStore.Operation, configOperation);

            if (configOperation == LDAPConfigFileHandler.ConfigOperation.List)
            {
                string configContent, LDAPRCContent, cerContent, defaultRole;
                handler.LoadConfigAndDefaultRole(out configContent, out cerContent, out LDAPRCContent);
                if (this._isFirstLoad)
                {
                    this.handler.canRemoveSection(out disableEnterprise, out disableCluster, out _dbRootDefaultRole);
                    _isFirstLoad = false;
                }

                result.Add(KeyStore.ConfigContent, configContent);
                result.Add(KeyStore.LDAPRCContent, LDAPRCContent);
                result.Add(KeyStore.CerContent, cerContent);
                //result.Add(KeyStore.DBRootDefaultRole, defaultRole);
            }
            else if (configOperation == LDAPConfigFileHandler.ConfigOperation.Update)
            {
                string configContent, cerPath, cerContent;
                configContent = argument[KeyStore.ConfigContent] as string;
                cerPath = argument[KeyStore.CerPath] as string;
                cerContent = argument[KeyStore.CerContent] as string;

                this.handler.updateLDAPConfig(configContent, cerPath, cerContent);
            }

            e.Result = result;
        }

        /// <summary>
        /// Handle completion events from the background worker
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(object sender, System.ComponentModel.RunWorkerCompletedEventArgs e)
        {
            this.toolStripStatusLabel1.Text = String.Empty;
            bool isWindowClosed = false;

            if (this.IsDisposed || isOperationCancelled)
                return;
            
            if (this.backgroundWorker.CancellationPending || e.Cancelled)
            {
                this._operationParamaters.Clear();
                return;
            }

            try
            {
                LDAPConfigFileHandler.ConfigOperation operation = (LDAPConfigFileHandler.ConfigOperation)this._operationParamaters[KeyStore.Operation];
                
                if (e.Error == null)
                {
                    Hashtable result = e.Result as Hashtable;
                    _hasError = false;
                    if (operation == LDAPConfigFileHandler.ConfigOperation.List && result != null)
                    {
                        string configContent, cerContent, LDAPRCContent;
                        configContent = result[KeyStore.ConfigContent] as string;
                        LDAPRCContent = result[KeyStore.LDAPRCContent] as string;
                        cerContent = result[KeyStore.CerContent] as string;
                        //_dbRootDefaultRole = result[KeyStore.DBRootDefaultRole] as string;
                        populateUIfromModel(configContent, cerContent, LDAPRCContent);
                        _isPreM10Version = this._theLDAPConfigObjectUpdate.isPreM10Version;
                        _isLoaded = true;
                        this.saveButton.Enabled = false;
                        string errorMsg = string.Empty;
                        const string SEPARATOR = " : ";
                        const string SPACE = "      ";
                        if (this.defaultErrorMap.Count != 0)
                        {
                            string value;
                            errorMsg += "Defaults configuration\n";
                            foreach (string key in defaultErrorMap.Keys)
                            {
                                this.defaultErrorMap.TryGetValue(key, out value);
                                errorMsg += SPACE + key + SEPARATOR + value + "\n";
                            }
                        }
                        if (this.localErrorMap.Count != 0)
                        {
                            string value;
                            errorMsg += "Enterprise configuration\n";
                            foreach (string key in localErrorMap.Keys)
                            {
                                this.localErrorMap.TryGetValue(key, out value);
                                errorMsg += SPACE + key + SEPARATOR + value + "\n";
                            }
                        }
                        if (this.remoteErrorMap.Count != 0)
                        {
                            string value;
                            errorMsg += "Cluster configuration\n";
                            foreach (string key in remoteErrorMap.Keys)
                            {
                                this.remoteErrorMap.TryGetValue(key, out value);
                                errorMsg += SPACE + key + SEPARATOR + value + "\n";
                            }
                        }
                        if (!string.IsNullOrEmpty(errorMsg))
                        {
                            this.saveButton.Enabled = true;
                            _hasError = true;
                            errorMsg = Properties.Resources.WrongConfigFile + errorMsg;
                            MessageBox.Show(errorMsg, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        }
                        if (_isPreM10Version)
                            this.saveButton.Enabled = true;
                    }
                    else if (operation == LDAPConfigFileHandler.ConfigOperation.Update && result != null)
                    {
                        this._theLDAPConfigObjectSaved = this._theLDAPConfigObjectUpdate;
                        this.backupCerLocation = this._theLDAPConfigObjectSaved.defaultSection.cerLocation;
                        this.backupCer = this._theLDAPConfigObjectSaved.defaultSection.cerContent;
                        MessageBox.Show(Properties.Resources.ConfigFileSavedWarning, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Information);
                        saveButton.Enabled = this._isPreM10Version = false;
                    }

                    this.isCertificateContentChangeHandled = true;
                    HighlightCertificateChange(false);
                }
                else
                {
                    //enable save button when there is error
                    _hasError = true;
                    this.saveButton.Enabled = true;
                    if (operation == LDAPConfigFileHandler.ConfigOperation.List)
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), string.Format(Properties.Resources.FailToReadConfigFile, e.Error.Message),
                                   Properties.Resources.LoadLDAPConfiguration, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        this.Close();
                        isWindowClosed = true;
                    }
                    else if (operation == LDAPConfigFileHandler.ConfigOperation.Update)
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), string.Format(Properties.Resources.FailToSaveConfigFile, e.Error.Message),
                                   Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        if (e.Error.ToString().Contains("Permission denied"))
                        {
                            cerLocationTextBox.Focus();
                        }
                    }
                    else
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), e.Error.Message, Trafodion.Manager.Properties.Resources.ConfigureLDAP,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
            catch (Exception ex)
            {
                saveButton.Enabled = true;
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message, Trafodion.Manager.Properties.Resources.ConfigureLDAP,
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                if (!isWindowClosed)
                {
                    setControlsStatus(true);
                    restrictSections();
                    refreshButton.Enabled = true;
                }
            }
        }

        /// <summary>
        /// Handles any progress events from the background worker
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_ProgressChanged(object sender, System.ComponentModel.ProgressChangedEventArgs e)
        { 
            //this.toolStripStatusLabel.Text = " " + e.UserState as string + "...";
        }

        /// <summary>
        /// Cleanup before form closing
        /// </summary>
        private void MyFormClosing(object sender, FormClosingEventArgs e)
        {
            CancelAsync();
        }

        /// <summary>
        /// Cancels any currently running background work
        /// </summary>
        private void CancelAsync()
        {
            if (backgroundWorker != null && backgroundWorker.IsBusy)
            {
                isOperationCancelled = true;
                backgroundWorker.CancelAsync();
            }
        }

        #endregion

        # region local control event

        private void networkTimeoutNumericUpDown_Leave(object sender, EventArgs e)
        {
            if (networkTimeoutNumericUpDown.Value.Equals(0))
                networkTimeoutNumericUpDown.Value = -1;
        }

        private void ldapTimeoutNumericUpDown_Leave(object sender, EventArgs e)
        {
            if (ldapTimeoutNumericUpDown.Value.Equals(0))
                ldapTimeoutNumericUpDown.Value = -1;
        }

        private void localCheckBox_Click(object sender, EventArgs e)
        {
            if (!localCheckBox.Checked)
            {
                if (!remoteCheckBox.Checked)
                {
                    MessageBox.Show(Properties.Resources.RemoveLastSection, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    localCheckBox.Checked = true;
                    return;
                }
                else if ("LOCAL".Equals(_dbRootDefaultRole.ToUpper()) || "ENTERPRISE".Equals(_dbRootDefaultRole.ToUpper()))
                {
                    MessageBox.Show(Properties.Resources.RemoveSectionWithDB__ROOT, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    localCheckBox.Checked = true;
                    return;
                }

                DialogResult dr = MessageBox.Show(string.Format(Properties.Resources.SectionRemove, "Enterprise"), Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (dr == DialogResult.No)
                {
                    localCheckBox.Checked = true;
                    return;
                }
                hideLocalSection();
            }
            else
            {
                if (sectionTabControl.Contains(tabPageCluster))
                    sectionTabControl.Controls.Remove(tabPageCluster);
                if (!sectionTabControl.Contains(tabPageEnterprise))
                    sectionTabControl.Controls.Add(this.tabPageEnterprise);
                if (this.remoteCheckBox.Checked)
                    sectionTabControl.Controls.Add(tabPageCluster);
                SetCertificateAvailablitly(this.encryptTypeComboBox.Text == STR_Unencrypted);
            }
        } 

        /// <summary>
        /// hide local section controls
        /// </summary>
        private void hideLocalSection()
        {
            //clear all the field and make them unvisible
            this.localCheckBox.Checked = false;
            hostNameTextBox.ResetText();
            encryptTypeComboBox.SelectedIndex = 0;
            searchDNTextBox.Text = String.Empty;
            passwdMaskedTextBox.Text = String.Empty;
            confirmPasswdMaskedTextBox.Text = String.Empty;
            networkTimeoutNumericUpDown1.Value = 30;
            ldapTimeoutNumericUpDown.Value = 30;
            timelimitNumericUpDown.Value = 30;
            retryDelayNumericUpDown.Value = 2;
            retryCountNumericUpDown.Value = 5;
            connectionComboBox.Text = OPTION_NO;
            configAttributeSettingControl.Clear();
            if (sectionTabControl.Controls.Contains(tabPageEnterprise))
                sectionTabControl.Controls.Remove(this.tabPageEnterprise);
        }


        # endregion local control event
        
        #region remote control event
        private void remoteCheckBox_Click(object sender, EventArgs e)
        {
            if (!remoteCheckBox.Checked)
            {
                if (!localCheckBox.Checked)
                {
                    MessageBox.Show(Properties.Resources.RemoveLastSection, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    remoteCheckBox.Checked = true;
                    return;
                }
                else if ("REMOTE".Equals(_dbRootDefaultRole.ToUpper()) || "CLUSTER".Equals(_dbRootDefaultRole.ToUpper()))
                {
                    MessageBox.Show(Properties.Resources.RemoveSectionWithDB__ROOT, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    remoteCheckBox.Checked = true;
                    return;
                }

                DialogResult dr = MessageBox.Show(string.Format(Properties.Resources.SectionRemove, "Cluster"), Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (dr == DialogResult.No)
                {
                    remoteCheckBox.Checked = true;
                    return;
                }
                hideRemoteSection();
            }
            else
            {
                if (!sectionTabControl.Contains(tabPageCluster))
                    sectionTabControl.Controls.Add(this.tabPageCluster);
                SetCertificateAvailablitly(this.encryptTypeComboBox1.Text == STR_Unencrypted);
            }
        }

        private void refreshTimeCheckBox_Click(object sender, EventArgs e)
        {
            if (this.refreshTimeCheckBox.Checked)
                this.refreshTimeNumericUpDown.Enabled = true;
            else
            {
                if (DialogResult.No == MessageBox.Show(Properties.Resources.DisableRefreshTime, "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning))
                {
                    this.refreshTimeCheckBox.Checked = true;
                    return;
                }
                this.refreshTimeNumericUpDown.Enabled = false;
                this.refreshTimeNumericUpDown.Value = 0;
            }
        }

        private void networkTimeoutNumericUpDown1_Leave(object sender, EventArgs e)
        {
            if (networkTimeoutNumericUpDown1.Value.Equals(0))
                networkTimeoutNumericUpDown1.Value = -1;
        }

        private void ldapTimeoutNumericUpDown1_Leave(object sender, EventArgs e)
        {
            if (ldapTimeoutNumericUpDown1.Value.Equals(0))
                ldapTimeoutNumericUpDown1.Value = -1;
        }

        /// <summary>
        /// set values to default for remote section controls
        /// </summary>
        private void hideRemoteSection()
        {
            //clear all the field and make them unvisible
            this.remoteCheckBox.Checked = false;
            hostNameTextBox1.ResetText();
            encryptTypeComboBox1.SelectedIndex = 0;
            searchDNTextBox1.Text = String.Empty;
            passwdMaskedTextBox1.Text = String.Empty;
            confirmPasswdMaskedTextBox1.Text = String.Empty;
            networkTimeoutNumericUpDown1.Value = 30;
            ldapTimeoutNumericUpDown1.Value = 30;
            timelimitNumericUpDown1.Value = 30;
            retryDelayNumericUpDown1.Value = 2;
            retryCountNumericUpDown1.Value = 5;
            connectionComboBox1.Text = OPTION_NO;
            configAttributeSettingControl1.Clear();
            if (sectionTabControl.Controls.Contains(tabPageCluster))
                sectionTabControl.Controls.Remove(this.tabPageCluster);
        }
             

        #endregion remote control event
        
        private void refreshButton_Click(object sender, EventArgs e)
        {
            if (saveButton.Enabled && 
                DialogResult.No == MessageBox.Show(Properties.Resources.DiscardChange, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.YesNo, MessageBoxIcon.Question))
                return;
            _isLoaded = false;
            currentTabPage = this.sectionTabControl.SelectedTab;
            InitializeData();
        }


        private void saveButton_Click(object sender, EventArgs e)
        {
            populateModelFromUI();
            if (!validateInput())
            {
                return;
            }

            _operationParamaters.Clear();
            _operationParamaters.Add(KeyStore.Operation, LDAPConfigFileHandler.ConfigOperation.Update);
            _operationParamaters.Add(KeyStore.ConfigContent, _theLDAPConfigObjectUpdate.LDAPConfigToString());
            _operationParamaters.Add(KeyStore.CerPath, _theLDAPConfigObjectUpdate.defaultSection.cerLocation);
            _operationParamaters.Add(KeyStore.CerContent, this._theLDAPConfigObjectUpdate.defaultSection.cerContent);

            setControlsStatus(false);
            this.saveButton.Enabled = this.refreshButton.Enabled = false;
            this.toolStripStatusLabel1.Text = Properties.Resources.ConfigSaving;
            backgroundWorker.RunWorkerAsync(_operationParamaters);
        }


        private ArrayList getAttributesFromGrid(iGRowCollection attributes)
        {
            ArrayList arrayList = new ArrayList();
            //List<string> listDirectorybase = new List<string>();
            List<string> listUniqueIdentifier = new List<string>();

            foreach (TenTec.Windows.iGridLib.iGRow row in attributes)
            {
                string name = row.Cells[0].Text.Trim();
                string value = row.Cells[1].Text.Trim();
                //if (name.Equals(LDAPConfigSection.Key_DirectoryBase))
                //{
                //    listDirectorybase.Add(value);
                //}
                //else 
                if (name.Equals(LDAPConfigSection.Key_UniqueIdentifier))
                {
                    listUniqueIdentifier.Add(value);
                }    
            }

            //arrayList.Add(listDirectorybase);
            arrayList.Add(listUniqueIdentifier);
            return arrayList;
        }

             

        bool isValidHostName(string hostName)
        {
            bool isValid = Regex.IsMatch(hostName, "^[a-z0-9A-Z]([a-z0-9A-Z-.](?!\\.\\.)){0,61}[a-z0-9A-Z]$");
            return isValid;
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            isOperationCancelled = true;
            if (_theLDAPConfigObjectSaved != null && (!_theLDAPConfigObjectSaved.Equals(_theLDAPConfigObjectUpdate) || _hasError) &&
                DialogResult.No == MessageBox.Show(Properties.Resources.CloseWindow, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.YesNo, MessageBoxIcon.Question))
                return;
            this.CancelAsync();
            this.Close();
        }

        /// <summary>
        /// browse pem file and load to certificate content textbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void browseButton_Click(object sender, EventArgs e)
        {
            //Display a folder browser dialog
            OpenFileDialog fileDialog = new OpenFileDialog();
            fileDialog.Title = Properties.Resources.CerFileDialogueTitle;
            fileDialog.Filter = Properties.Resources.CerFileFilter;

            if (fileDialog.ShowDialog() == DialogResult.OK)
            {
                string loadedContent = System.IO.File.ReadAllText(fileDialog.FileName);
                this.savedCer = this.certificateRichTextBox.Text = loadedContent;
                this.isCertificateContentChangeHandled = false;
                HighlightCertificateChange(true);
            }
        }

        private void loadServerCerButton_Click(object sender, EventArgs e)
        {
            if (!this.isAsyncRunning)
            {
                LoadCertification();
            }
        }

        private void cerLocationTextBox_TextChanged(object sender, EventArgs e)
        {
            SaveCerInfo();

            // Nothing to do if action has been done due to certificate content change
            if (this.isCertificateContentChangeHandled)
            {
                bool isCerLocationChanged = this.cerLocationTextBox.Text.Trim() != this.backupCerLocation.Trim();
                HighlightCertificateChange(isCerLocationChanged);
            }
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UpdateLDAPConfig);
        }

        private class KeyStore
        {
            public const string Operation = "Operation";
            public const string ConfigContent = "ConfigContent";
            public const string LDAPRCContent = "LDAPRCContent";
            public const string CerPath = "CerPath";
            public const string CerContent = "CerContent";
            public const string DBRootDefaultRole = "DefaultRole";
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
            _theChangeTracker = new TrafodionChangeTracker(_theTrafodionPanelContent);
            _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(ChangeTracker_OnChangeDetected);
            _theChangeTracker.EnableChangeEvents = true;
        }

        /// <summary>
        /// Event handler for change detected.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeTracker_OnChangeDetected(object sender, System.EventArgs e)
        {
            if (_isLoaded)
            {
                UpdateSaveButton();
            }
        }

        #region Certificate 
        
        private void SetDefaultSection(LDAPConfigDefaultSection defaultSection)
        {
            defaultSectionComboBox.Text = defaultSection.defaultSectionName;
            this.savedCerLocation = this.backupCerLocation = defaultSection.cerLocation;
            this.savedCer = this.backupCer = defaultSection.cerContent;

            foreach (TabPage tabPage in sectionTabControl.TabPages)
            {
                if (0 == string.Compare(tabPage.Text, (string)defaultSectionComboBox.SelectedItem, true))
                {
                    sectionTabControl.SelectedTab = tabPage;
                    SetCertificateAvailablitly(tabPage);
                    break;
                }
            }
        }

        private bool IsValidCerLocation()
        {
            bool isValid = true;

            //check certificate location
            if (!cerLocationTextBox.ReadOnly)
            {
                string cerFileName = cerLocationTextBox.Text.Trim();
                if (cerFileName.Length == 0)
                {
                    isValid = false;
                    MessageBox.Show(Properties.Resources.CerLocationEmpty, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    this.cerLocationTextBox.Focus();
                }
                else if (!Utilities.IsValidServerFileName(cerFileName))
                {
                    isValid = false;
                    MessageBox.Show(Properties.Resources.CerLocationInvalid, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this.cerLocationTextBox.Focus();
                }
            }

            return isValid;
        }

        private void LoadCertification()
        {
            string currentCerLocation = cerLocationTextBox.Text.Trim();
            string lastCerLocation = this.savedCerLocation;

            SaveCerInfo();

            if (this.isAsyncRunning || cerLocationTextBox.ReadOnly || !IsValidCerLocation())
            {
                return;
            }

            LoadCertificationAsync();
        }

        private void LoadCertificationAsync()
        {
            _isLoaded = false;//Certification is loading by thread, prevent from process of change handler
            SetCheckCertificateAvailability(false);
            cerLocationTextBox.Text = this.savedCerLocation;

            Object[] parameters = new Object[] { this.connectionDefinition };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.LoadCertificate, this, "CheckCertificate", parameters);
            this.asyncProgressor = new TrafodionProgressUserControl(args);
            this.asyncProgressor.ProgressCompletedEvent += CheckCertificate_ProgressCompleted;
            this.asyncProgressor.AdaptCompactSize();
            this.asyncProgressor.Dock = DockStyle.Fill;
            this.asyncProgressor.Margin = new Padding(0);
            this.asyncProgressor.Padding = new Padding(0);
            this.asyncPanel.Controls.Add(this.asyncProgressor);

            this.FormClosing += (sender, e) =>
            {
                asyncProgressor.ProgressCompletedEvent -= CheckCertificate_ProgressCompleted;
            };
        }

        public string CheckCertificate(ConnectionDefinition connectionDefinition)
        {
            string certificateContent = string.Empty;
            String cerContent;
            this.handler.viewCertifcate(this.cerLocationTextBox.Text.Trim(), out cerContent);

            return cerContent;
        }

        private void CheckCertificate_ProgressCompleted(object sender, TrafodionProgressCompletedArgs arg)
        {
            SetCheckCertificateAvailability(true);

            if (arg.Error != null)
            {
                MessageBox.Show(arg.Error.Message, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                UpdateSaveButton();
                return;
            }

            _isLoaded = true;//Certification is loaded
            string certificateContent = (string)arg.ReturnValue;
            this.backupCer = this.savedCer = certificateRichTextBox.Text = certificateContent;
            this.backupCerLocation = this.cerLocationTextBox.Text;
            this.isCertificateContentChangeHandled = true;
            HighlightCertificateChange(false);
            UpdateSaveButton();
        }

        private void SetCheckCertificateAvailability(bool available)
        {
            this.isAsyncRunning = !available;
            this.asyncPanel.Visible = !available;

            this.loadServerCerButton.Enabled = available;
            browseButton.Enabled = available;
            refreshButton.Enabled = available;
            saveButton.Enabled = available;
            certificateRichTextBox.Visible = available;
            encryptTypeComboBox.Enabled = encryptTypeComboBox1.Enabled = available;
            localCheckBox.Enabled = remoteCheckBox.Enabled = available;
            if (available)
                restrictSections();
            SetCertificateAvailablitly(!available);
            if (available)
            {
                this.asyncPanel.Controls.Remove(this.asyncProgressor);
            }
        }

        private void SetPort(TrafodionTextBox txtPort, string encryptionType)
        {
            switch (encryptionType)
            {
                case STR_Unencrypted:
                case STR_TLS:
                    txtPort.Text = PORT_Unencrypted_OR_TLS;
                    break;

                case STR_SSL:
                    txtPort.Text = PORT_SSL;
                    break;
            }
        }

        private void SetCertificateAvailablitly(TabPage tabPage)
        {            
            if (sectionTabControl.SelectedTab == tabPageEnterprise)
            {
                SetCertificateAvailablitly((string)encryptTypeComboBox.SelectedItem == STR_Unencrypted);
            }
            else if (sectionTabControl.SelectedTab == tabPageCluster)
            {
                SetCertificateAvailablitly((string)encryptTypeComboBox1.SelectedItem == STR_Unencrypted);
            }
        }

        private void SetCertificateAvailablitly(bool isReadOnly)
        {
            if ( this.isAsyncRunning )
            {
                isReadOnly = true;
            }

            this.loadServerCerButton.Enabled = browseButton.Enabled = !isReadOnly;
            cerLocationTextBox.ReadOnly = isReadOnly;
            cerLocationTextBox.BackColor = isReadOnly ? System.Drawing.Color.LightGray : System.Drawing.SystemColors.Window;
            certificateRichTextBox.Text = isReadOnly ? string.Empty : this.savedCer;
            cerLocationTextBox.Text = isReadOnly ? string.Empty : this.savedCerLocation;
        }

        private void SaveCerInfo()
        {
            if ( !cerLocationTextBox.ReadOnly )
            {
                this.savedCerLocation = cerLocationTextBox.Text.Trim();
                this.savedCer = certificateRichTextBox.Text.Trim();
            }
        }
        private void UpdateSaveButton()
        {
            populateModelFromUI();
            saveButton.Enabled = !_theLDAPConfigObjectSaved.Equals(_theLDAPConfigObjectUpdate) || _hasError || this._isPreM10Version;
        }

        private bool validateInput()
        {
            if (_theLDAPConfigObjectUpdate.local!=null && !validateLocalConfigSection(_theLDAPConfigObjectUpdate.local))
            {
                return false;
            }

            if (_theLDAPConfigObjectUpdate.remote!=null && !validateRemoteConfigSection(_theLDAPConfigObjectUpdate.remote))
            {
                return false;
            }

            if (!validateDefaultSection(_theLDAPConfigObjectUpdate.defaultSection))
            {
                return false;
            }
            return true;
        }

        private bool validateAttribute(iGRowCollection attributes,int idxTab)
        {
            LDAPConfigSection ldapObject = new LDAPConfigSection();
            iGRow errorRow = null;
            string errorMsg = null;

            foreach (TenTec.Windows.iGridLib.iGRow row in attributes)
            {
                string name = row.Cells[0].Text.Trim();
                string value = row.Cells[1].Text.Trim();

                if (value == String.Empty)
                {
                    errorMsg = string.Format(Properties.Resources.EmptyAlert, name);
                    errorRow = row;
                    break;
                }

                if (name.Equals(LDAPConfigSection.Key_UniqueIdentifier))
                {
                    if (ldapObject.UniqueIdentifier.Contains(value))
                    {
                        errorRow = row;
                        break;
                    }
                    else
                        ldapObject.UniqueIdentifier.Add(value);
                }    
            }

            if (errorRow == null && ldapObject.UniqueIdentifier.Count == 0)
            {
                errorMsg = Properties.Resources.UniqueIdentifierRequired;
                errorRow = null;
            }

            if (errorRow != null || errorMsg != null)
            {
                if (errorMsg == null)
                    errorMsg = errorRow.Cells[0].Value + Properties.Resources.HasDuplicateValue;

                MessageBox.Show(errorMsg, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                //focus into the error row
                if (errorRow != null)
                {
                    sectionTabControl.SelectedIndex = idxTab;
                    errorRow.Grid.PerformAction(iGActions.DeselectAllRows);
                    errorRow.Grid.PerformAction(iGActions.DeselectAllCells);
                    errorRow.Grid.SetCurCell(errorRow.Index, 1);
                    errorRow.Grid.RequestEditCurCell();
                }
                ldapObject = null;
                return false;
            }

            return true;
        }

        private bool validateLocalConfigSection(LDAPConfigSection ldapLocalSection)
        {
            if (!validateAttribute(this.configAttributeSettingControl.getGridData(),0))
            {
                sectionTabControl.SelectedIndex = 0;
                return false;
            }

            if (ldapLocalSection.LdapHostname.Count == 0)
            {
                MessageBox.Show(Properties.Resources.HostnameEmpty, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 0;
                hostNameTextBox.Focus();
                return false;
            }
            else
            {
                foreach (string str in ldapLocalSection.LdapHostname)
                {
                    if (!isValidHostName(str))
                    {
                        MessageBox.Show(Properties.Resources.HostnameInvalid, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        sectionTabControl.SelectedIndex = 0;
                        hostNameTextBox.Focus();
                        return false;
                    }
                }

                if (Utilities.containsDuplicateString(ldapLocalSection.LdapHostname))
                {
                    MessageBox.Show(Properties.Resources.HostnameDuplicate, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    sectionTabControl.SelectedIndex = 0;
                    hostNameTextBox.Focus();
                    return false;
                }
            }

            //check port
            try
            {
                if (String.IsNullOrEmpty(ldapLocalSection.LdapPort))
                {
                    MessageBox.Show("LDAP Port cannot be empty.", Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    sectionTabControl.SelectedIndex = 0;
                    this.portTextBox.Focus();
                    return false;
                }

                int port = Int32.Parse(ldapLocalSection.LdapPort);
                if (port < 1 || port > 65535)
                    throw new Exception("The port is invalid.");
            }
            catch
            {
                MessageBox.Show(Properties.Resources.PortInvalid, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 0;
                this.portTextBox.Focus();
                return false;
            }          


            //check password
            if (this.passwdMaskedTextBox.Text.Equals(this.confirmPasswdMaskedTextBox.Text))
                ldapLocalSection.LDAPSearchPWD = this.passwdMaskedTextBox.Text;
            else
            {
                MessageBox.Show(Properties.Resources.PasswordDifferent, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 0;
                passwdMaskedTextBox.Focus();
                return false; 
            }


            //check searchDN
            if (!string.IsNullOrEmpty(ldapLocalSection.LDAPSearchPWD) && string.IsNullOrEmpty(ldapLocalSection.LDAPSearchDN))
            {
                MessageBox.Show(Properties.Resources.SearchDNNull, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 0;
                searchDNTextBox.Focus();
                return false;
            }
            return true;
        }

        private bool validateRemoteConfigSection(LDAPConfigSection ldapRemoteSection)
        {
            if (!validateAttribute(this.configAttributeSettingControl1.getGridData(),1))
            {
                sectionTabControl.SelectedIndex = 1;
                return false;
            }

            if (ldapRemoteSection.LdapHostname.Count == 0)
            {
                MessageBox.Show(Properties.Resources.HostnameEmpty, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 1;
                hostNameTextBox1.Focus();
                return false;
            }
            else
            {
                foreach (string str in ldapRemoteSection.LdapHostname)
                {
                    if (!isValidHostName(str))
                    {
                        MessageBox.Show(Properties.Resources.HostnameInvalid, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        sectionTabControl.SelectedIndex = 1;
                        hostNameTextBox1.Focus();
                        return false;
                    }
                }

                if (Utilities.containsDuplicateString(ldapRemoteSection.LdapHostname))
                {
                    MessageBox.Show(Properties.Resources.HostnameDuplicate, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    sectionTabControl.SelectedIndex = 1;
                    hostNameTextBox1.Focus();
                    return false;
                }
            }

            //check port
            if (String.IsNullOrEmpty(ldapRemoteSection.LdapPort))
            {
                MessageBox.Show("LDAP Port cannot be empty.", Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 1;
                this.portTextBox1.Focus();
                return false;
            }
            try
            {
                int port = Int32.Parse(ldapRemoteSection.LdapPort);
                if (port < 1 || port > 65535)
                    throw new Exception("The port is invalid.");
            }
            catch
            {
                MessageBox.Show(Properties.Resources.PortInvalid, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 1;
                this.portTextBox1.Focus();
                return false;
            }


            //check password
            if (this.passwdMaskedTextBox1.Text.Equals(this.confirmPasswdMaskedTextBox1.Text))
                ldapRemoteSection.LDAPSearchPWD = this.passwdMaskedTextBox1.Text;
            else
            {
                MessageBox.Show(Properties.Resources.PasswordDifferent, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 1;
                passwdMaskedTextBox1.Focus();
                return false;
            }


            //check searchDN
            if (!string.IsNullOrEmpty(ldapRemoteSection.LDAPSearchPWD) && string.IsNullOrEmpty(ldapRemoteSection.LDAPSearchDN))
            {
                MessageBox.Show(Properties.Resources.SearchDNNull, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                sectionTabControl.SelectedIndex = 1;
                searchDNTextBox1.Focus();
                return false;
            }
            return true;
        }

        private bool validateDefaultSection(LDAPConfigDefaultSection defaultSection)
        {
            if (this.defaultSectionComboBox.Text.Equals(LDAPConfigObject.ENTERPRISE) && !this.localCheckBox.Checked ||
             this.defaultSectionComboBox.Text.Equals(LDAPConfigObject.CLUSTER) && !this.remoteCheckBox.Checked)
            {
                MessageBox.Show(Properties.Resources.SelectNonexistentSection, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                this.defaultSectionComboBox.Focus();
                return false;
            }
            else if (this.defaultSectionComboBox.Text.Equals(String.Empty))
            {
                MessageBox.Show(Properties.Resources.SpecifyDefaultSection, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                this.defaultSectionComboBox.Focus();
                return false;
            }
            //check certificate path and content if one section is encrypted
            if (this.encryptTypeComboBox.Text.Equals(STR_SSL) || this.encryptTypeComboBox.Text.Equals(STR_TLS) ||
                this.encryptTypeComboBox1.Text.Equals(STR_SSL) || this.encryptTypeComboBox1.Text.Equals(STR_TLS))
            {
                // get tabpage with encrypted section
                TabPage encryptedTabPage = null;
                if (!this.cerLocationTextBox.ReadOnly)
                    encryptedTabPage = this.sectionTabControl.SelectedTab;
                else if (this.encryptTypeComboBox.Text.Equals(STR_SSL) || this.encryptTypeComboBox.Text.Equals(STR_TLS))
                    encryptedTabPage = this.tabPageEnterprise;
                else encryptedTabPage = this.tabPageCluster;

                // check certificate location
                if (this.savedCerLocation == null || this.savedCerLocation.Trim().Length == 0)
                {
                    MessageBox.Show(Properties.Resources.CerLocationEmpty, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    this.sectionTabControl.SelectedTab = encryptedTabPage;
                    this.cerLocationTextBox.Focus();
                    return false;
                }
                else if (!Utilities.IsValidServerFileName(savedCerLocation))
                {
                    MessageBox.Show(Properties.Resources.CerLocationInvalid, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    this.sectionTabControl.SelectedTab = encryptedTabPage;
                    this.cerLocationTextBox.Focus();
                    return false;
                }
                //check certificate content
                if (this.savedCer == null || this.savedCer.Trim().Length == 0)
                {
                    MessageBox.Show(Properties.Resources.CertificateContentEmpty, Trafodion.Manager.Properties.Resources.ConfigureLDAP, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    this.sectionTabControl.SelectedTab = encryptedTabPage;
                    this.certificateRichTextBox.Focus();
                    return false;
                }
            }

            return true;
        }

        private void restrictSections()
        {
            if ((_dbRootDefaultRole.ToUpper().Equals("LOCAL") || _dbRootDefaultRole.ToUpper().Equals("ENTERPRISE")) && this.localCheckBox.Checked)
            {
                this.localCheckBox.Enabled = false;
            }
            else if ((_dbRootDefaultRole.ToUpper().Equals("REMOTE") || _dbRootDefaultRole.ToUpper().Equals("CLUSTER")) && this.remoteCheckBox.Checked)
            {
                this.remoteCheckBox.Enabled = false;
            }
            else
            {
                this.localCheckBox.Enabled = true;
                this.remoteCheckBox.Enabled = true;
            }
            this.localCheckBox.Enabled = this.localCheckBox.Enabled && !this.disableEnterprise || !this.localCheckBox.Checked;
            this.remoteCheckBox.Enabled = this.remoteCheckBox.Enabled && !this.disableCluster || !this.remoteCheckBox.Checked;
        }
        private void populateModelFromUI()
        {
            LDAPConfigSection ldapLocalSection = null;
            LDAPConfigSection ldapRemoteSection = null;
            LDAPConfigDefaultSection ldapDefaultSection = new LDAPConfigDefaultSection();
            LDAPConfigObject ldapConfigTemp = new LDAPConfigObject(connectionDefinition);

            #region Save Local Section
            if (this.localCheckBox.Checked)
            {
                ldapLocalSection = new LDAPConfigSection();
                //extract data from local grid                
                ArrayList arrayAttributes = this.getAttributesFromGrid(this.configAttributeSettingControl.getGridData());
                //ldapLocalSection.DirectoryBase = (List<string>)arrayAttributes[0];
                ldapLocalSection.UniqueIdentifier = (List<string>)arrayAttributes[0];
                ldapLocalSection.Section = LDAPConfigObject.ENTERPRISE;

                //host name
                string[] hostnames = this.hostNameTextBox.Text.Split(new string[] { "\r\n", ",", " " }, StringSplitOptions.RemoveEmptyEntries);
                ldapLocalSection.LdapHostname.AddRange(hostnames);

                //port
                ldapLocalSection.LdapPort = portTextBox.Text.Trim();

                //searchDN
                string[] searchDNs = this.searchDNTextBox.Text.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                ldapLocalSection.LDAPSearchDN = string.Join(",", searchDNs);

                //search password
                ldapLocalSection.LDAPSearchPWD = this.passwdMaskedTextBox.Text;

                ldapLocalSection.LDAPNetworkTimeout = (int)this.networkTimeoutNumericUpDown.Value;
                ldapLocalSection.LDAPTimelimit = (int)this.timelimitNumericUpDown.Value;
                ldapLocalSection.LDAPTimeout = (int)this.ldapTimeoutNumericUpDown.Value;
                ldapLocalSection.PreserveConnection = this.connectionComboBox.Text.Equals(OPTION_YES);
                ldapLocalSection.RetryCount = (int)this.retryCountNumericUpDown.Value;
                ldapLocalSection.RetryDelay = (int)this.retryDelayNumericUpDown.Value;

                if (this.encryptTypeComboBox.Text.Equals(STR_Unencrypted))
                    ldapLocalSection.LDAPSSL = 0;
                else if (this.encryptTypeComboBox.Text.Equals(STR_SSL))
                    ldapLocalSection.LDAPSSL = 1;
                else if (this.encryptTypeComboBox.Text.Equals(STR_TLS))
                    ldapLocalSection.LDAPSSL = 2;
            }
            #endregion Sav Local Section

            #region Save Remote Section
            if (this.remoteCheckBox.Checked)
            {
                ldapRemoteSection = new LDAPConfigSection();
                //extract data from local grid                
                ArrayList arrayAttributes = this.getAttributesFromGrid(this.configAttributeSettingControl1.getGridData());
                //ldapRemoteSection.DirectoryBase = (List<string>)arrayAttributes[0];
                ldapRemoteSection.UniqueIdentifier = (List<string>)arrayAttributes[0];
                ldapRemoteSection.Section = LDAPConfigObject.CLUSTER;

                //host name
                string[] hostnames = this.hostNameTextBox1.Text.Split(new string[] { "\r\n", ",", " " }, StringSplitOptions.RemoveEmptyEntries);
                ldapRemoteSection.LdapHostname.AddRange(hostnames);

                //port
                ldapRemoteSection.LdapPort = portTextBox1.Text;

                //searchDN
                string[] searchDNs = this.searchDNTextBox1.Text.Split(new string[] { "," }, StringSplitOptions.RemoveEmptyEntries);
                ldapRemoteSection.LDAPSearchDN = string.Join(",", searchDNs);

                //search password
                ldapRemoteSection.LDAPSearchPWD = this.passwdMaskedTextBox1.Text;

                ldapRemoteSection.LDAPNetworkTimeout = (int)this.networkTimeoutNumericUpDown1.Value;
                ldapRemoteSection.LDAPTimelimit = (int)this.timelimitNumericUpDown1.Value;
                ldapRemoteSection.LDAPTimeout = (int)this.ldapTimeoutNumericUpDown1.Value;
                ldapRemoteSection.PreserveConnection = this.connectionComboBox1.Text.Equals(OPTION_YES);
                ldapRemoteSection.RetryCount = (int)this.retryCountNumericUpDown1.Value;
                ldapRemoteSection.RetryDelay = (int)this.retryDelayNumericUpDown1.Value;

                if (this.encryptTypeComboBox1.Text.Equals(STR_Unencrypted))
                    ldapRemoteSection.LDAPSSL = 0;
                else if (this.encryptTypeComboBox1.Text.Equals(STR_SSL))
                    ldapRemoteSection.LDAPSSL = 1;
                else if (this.encryptTypeComboBox1.Text.Equals(STR_TLS))
                    ldapRemoteSection.LDAPSSL = 2;
            }
            #endregion Save Remote Section

            #region Default Section
            ldapDefaultSection.refreshTime = RefreshTime;
            ldapDefaultSection.defaultSectionName = this.defaultSectionComboBox.Text;
            //restore level and location when they are not enabled
            if (!cerLocationTextBox.ReadOnly)
            {
                ldapDefaultSection.cerLocation = this.cerLocationTextBox.Text.Trim();
                ldapDefaultSection.cerContent = this.certificateRichTextBox.Text;
            }
            else
            {
                if (this.encryptTypeComboBox.Text.Equals(STR_Unencrypted) && this.encryptTypeComboBox1.Text.Equals(STR_Unencrypted))
                    ldapDefaultSection.cerLocation = ldapDefaultSection.cerContent = string.Empty;
                else
                {
                    ldapDefaultSection.cerLocation = this.savedCerLocation;
                    ldapDefaultSection.cerContent = this.savedCer;
                }
            }
            
            #endregion Default Section

            ldapConfigTemp.local = ldapLocalSection;
            if (_theLDAPConfigObjectSaved.local != null)
            {
                ldapConfigTemp.local.TheOtherUndefinedEntries = _theLDAPConfigObjectSaved.local.TheOtherUndefinedEntries;
            }
            
            ldapConfigTemp.remote = ldapRemoteSection;
            if (_theLDAPConfigObjectSaved.remote != null)
            {
                ldapConfigTemp.remote.TheOtherUndefinedEntries = _theLDAPConfigObjectSaved.remote.TheOtherUndefinedEntries;
            }

            ldapConfigTemp.defaultSection = ldapDefaultSection;
            if (_theLDAPConfigObjectSaved.defaultSection != null)
            {
                ldapConfigTemp.defaultSection.TheOtherUndefinedEntries = _theLDAPConfigObjectSaved.defaultSection.TheOtherUndefinedEntries;
            }

            _theLDAPConfigObjectUpdate=ldapConfigTemp;
        }

        #endregion

        #region Certificate Event

        private void encryptTypeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            TrafodionComboBox comboBox = (TrafodionComboBox)sender;
            string encryptionType = (string)comboBox.SelectedItem;
            bool isCurrentTabActive = false;
            if (comboBox == encryptTypeComboBox)
            {
                SetPort(portTextBox, encryptionType);
                isCurrentTabActive = sectionTabControl.SelectedTab == tabPageEnterprise;
            }
            else if (comboBox == encryptTypeComboBox1)
            {
                SetPort(portTextBox1, encryptionType);
                isCurrentTabActive = sectionTabControl.SelectedTab == tabPageCluster;
            }

            // Only save and set certificate when current tab is active, ignoring the event when binging default data to combo box
            if (isCurrentTabActive)
            {
                SaveCerInfo();
                SetCertificateAvailablitly(encryptionType == STR_Unencrypted);
            }
        }

        void sectionTabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            SetCertificateAvailablitly(sectionTabControl.SelectedTab);
        }

        private void sectionTabControl_Selecting(object sender, TabControlCancelEventArgs e)
        {
            if (this.isAsyncRunning)
            {
                e.Cancel = true;
            }
        }

        private void cerLocationTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (Char)Keys.Enter && !this.isAsyncRunning)
            {
                LoadCertification();
                e.Handled = true;
            }
        }

        #endregion
    }
}
