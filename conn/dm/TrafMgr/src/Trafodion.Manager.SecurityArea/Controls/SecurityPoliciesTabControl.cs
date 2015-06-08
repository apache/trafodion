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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// User control to display security policies.
    /// </summary>
    public partial class SecurityPoliciesTabControl : UserControl, IPendingChange
    {
        #region Fields

        // This is to remember where we have been left off. 
        private static int _currentSelectedIndex = 0;
        private ConnectionDefinition _theConnectionDefinition = null;
        private string _theTitle = Properties.Resources.Policies;
        private SystemSecurity _theSystemSecurity = null;
        private Policies _thePolicies = null;
        private SecurityPoliciesSystemUserControl _theSecurityPoliciesSystemUserControl = null;
        private SecurityPoliciesLoggingUserControl _theSecurityPoliciesLoggingUserControl = null;
        private SecurityPoliciesPasswordUserControl _theSecurityPoliciesPasswordUserControl = null;
        private SecurityPoliciesPowerRoleMgmtUserControl _theSecurityPoliciesPowerRoleMgmtUserControl = null;
        private TrafodionChangeTracker _theSystemUserControlChangeTracker = null;
        private TrafodionChangeTracker _theLoggingUserControlChangeTracker = null;
        private TrafodionChangeTracker _thePasswordUserControlChangeTracker = null;
        private TrafodionChangeTracker _thePowerRoleMgmtUserControlChangeTracker = null;

        // what is the current policy setting?
        private Policies.POLICY_SETTING _theCurrentSetting = Policies.POLICY_SETTING.SETTING_CUSTOMER;

        // whether or not the setup has completed?
        private bool _setupCompleted = false;

        // is error message currently shown? 
        private bool _errorMessageShown = false;

        #endregion Fields

        #region Properties

        /// <summary>
        /// CurrentSelectedIndex: the current selected tab page
        /// </summary>
        public int CurrentSelectedIndex
        {
            get { return _currentSelectedIndex; }
            set
            {
                _currentSelectedIndex = value;
                if (value != _currentSelectedIndex)
                {
                    _currentSelectedIndex = value;
                    _theTabControl.SelectedIndex = _currentSelectedIndex;
                }
            }
        }

        /// <summary>
        /// ConnectionDefn: the connection definition for the control
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Policies: the policies model object
        /// </summary>
        public Policies Policies
        {
            get { return _thePolicies; }
            set { _thePolicies = value; }
        }

        /// <summary>
        /// SystemSecurity: the system security model object 
        /// </summary>
        public SystemSecurity SystemSecurity
        {
            get { return _theSystemSecurity; }
            set { _theSystemSecurity = value; }
        }

        /// <summary>
        /// CurrentSetting: the current value settings
        /// </summary>
        public Policies.POLICY_SETTING CurrentSetting
        {
            get { return _theCurrentSetting; }
            set { _theCurrentSetting = value; }
        }

        /// <summary>
        /// WindowTitle: the window title used for popup window
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Default constructor
        /// </summary>
        /// <param name="theSystemSecurity"></param>
        public SecurityPoliciesTabControl(SystemSecurity theSystemSecurity)
        {
            InitializeComponent();
            _theSystemSecurity = theSystemSecurity;
            _theConnectionDefinition = _theSystemSecurity.ConnectionDefinition;

            this._theTabControl.SelectedIndexChanged += new EventHandler(SecurityPoliciesTabControl_SelectedIndexChanged);
            _theTabControl.SelectedIndex = _currentSelectedIndex;
            _setupCompleted = false;
            _loadDefaultButton.Enabled = false;
            _loadMostSecureButton.Enabled = false;

            _thePolicies = new Policies(ConnectionDefn, Policies.POLICY_SETTING.SETTING_CUSTOMER);
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.RetrieveSecurityPolicies, 
                                                             _thePolicies, 
                                                             "LoadPolicies", 
                                                             new Object[0]);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();

            if (progressDialog.Error != null)
            {
                SecurityShowExceptionUserControl showException =
                            new SecurityShowExceptionUserControl(String.Format(Properties.Resources.ErrorOperationFailed,
                                                                               Properties.Resources.RetrieveSecurityPolicies,
                                                                               progressDialog.Error.Message));
                AddControl(showException, Properties.Resources.Message);
                _errorMessageShown = true;
            }
            else
            {
                SetupWidgets();
            }
            if (SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                _loadDefaultButton.Enabled = _loadMostSecureButton.Enabled = _applyButton.Enabled = false;
            }
        }

        /// <summary>
        /// Constructor: for cloning 
        /// </summary>
        /// <param name="aDirectoryServersUserControl"></param>
        public SecurityPoliciesTabControl(SecurityPoliciesTabControl aSecurityPoliciesTabControl)
            : this(aSecurityPoliciesTabControl.SystemSecurity)
        {
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To clone a self.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            SecurityPoliciesTabControl theSecurityPoliciesTabControl = new SecurityPoliciesTabControl(this);
            return theSecurityPoliciesTabControl;
        }

        /// <summary>
        /// Persist any configuraiton in this user control.
        /// </summary>
        public void PersistConfiguration()
        {
        }

        /// <summary>
        /// Refresh the grid display
        /// </summary>
        public override void Refresh()
        {
            DoRefresh();
        }

        /// <summary>
        /// Is there any change pending?
        /// </summary>
        public bool IsPending()
        {
            return (_theSecurityPoliciesLoggingUserControl.IsChangePending ||
                    _theSecurityPoliciesPasswordUserControl.IsChangePending ||
                    _theSecurityPoliciesSystemUserControl.IsChangePending ||
                    _theSecurityPoliciesPowerRoleMgmtUserControl.IsChangePending);
        }

        /// <summary>
        /// Return the change pending object.
        /// </summary>
        /// <returns></returns>
        public PendingChangeObject GetPendingChangeObject()
        {
            bool isChangePending = (_theSecurityPoliciesLoggingUserControl.IsChangePending ||
                                    _theSecurityPoliciesPasswordUserControl.IsChangePending ||
                                    _theSecurityPoliciesSystemUserControl.IsChangePending ||
                                    _theSecurityPoliciesPowerRoleMgmtUserControl.IsChangePending);
            PendingChangeObject pending = new PendingChangeObject(isChangePending, Properties.Resources.PolicyChangePendingWarning);
            return pending;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Event handler for refresh button.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _refreshButton_Click(object sender, EventArgs e)
        {
            DoRefresh();
        }

        /// <summary>
        /// Event handler for loading the factory default button. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _loadDefaultButton_Click(object sender, EventArgs e)
        {
            DoLoadFactoryDefaultSettings();
        }

        /// <summary>
        /// Event handler for loading the most secure button.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _loadMostSecureButton_Click(object sender, EventArgs e)
        {
            DoLoadMostSecureSettings();
        }

        /// <summary>
        /// Event handler for apply change button. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _applyButton_Click(object sender, EventArgs e)
        {

            String message = Properties.Resources.ChangePolicyWarningMessage;
            DialogResult result = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                                  message, 
                                                  String.Format(Properties.Resources.OperationWarningTitle,
                                                                Properties.Resources.ChangeSecurityPolicy),
                                                  MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            
            // User's confirmation? 
            if (result == DialogResult.Yes)
            {
                try
                {
                    //DoUpdate();
                    string method = "UpdateAll";
                    switch (CurrentSetting)
                    {
                        case Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT:
                            if (!IsPending())
                            {
                                // User has not changed any factory default settings. 
                                method = "ResetAllToDefault";
                            }
                            break;

                        case Policies.POLICY_SETTING.SETTING_MOSE_SECURE:
                            if (!IsPending())
                            {
                                // User has not changed any factory default settings. 
                                method = "ResetAllToMostSecure";
                            }
                            break;

                        default:
                            if (!IsPending())
                            {
                                DoRefresh();
                                return;
                            }
                            break;
                    }

                    // Update the policies with the user's input. 
                    _theSecurityPoliciesLoggingUserControl.Update();
                    _theSecurityPoliciesPasswordUserControl.Update();

                    if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
                    {
                        _theSecurityPoliciesSystemUserControl.Update();
                    }
                    _theSecurityPoliciesPowerRoleMgmtUserControl.Update();

                    TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.UpdateSecurityPolicies,
                                                                     _thePolicies,
                                                                     method,
                                                                     new Object[0]);
                    TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                    progressDialog.ShowDialog();

                    if (progressDialog.Error != null)
                    {
                        string errorMessage = String.Format(Properties.Resources.ErrorOperationFailed,
                                                                                       Properties.Resources.UpdateSecurityPolicies,
                                                                                       progressDialog.Error.Message);
                        if (_thePolicies.ReconnectRequired)
                        {
                            errorMessage = "Not all policies were updated. For details see the following error message.\n" +
                                           "Some of the updated policies require that you reconnect. Please disconnect from the system and reconnect.\n\n" +
                                           progressDialog.Error.Message;
                        }
                        SecurityShowExceptionUserControl showException =
                                    new SecurityShowExceptionUserControl(errorMessage);
                        AddControl(showException, Properties.Resources.Message);
                        _errorMessageShown = true;
                    }
                    else
                    {
                        if (_thePolicies.ReconnectRequired)
                        {
                            MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                "The policies were updated successfully. \n" + "Some of the updated policies require that you reconnect. Please disconnect from the system and reconnect.",
                                "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                        }
                        DoRefresh();
                    }
                }
                catch (Exception ex)
                {
                    message = String.Format(Properties.Resources.ErrorOperationFailed, 
                                            Properties.Resources.ChangeSecurityPolicy,
                                            ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle, 
                                                  Properties.Resources.ChangeSecurityPolicy),
                                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

        }

        /// <summary>
        /// Add only one control to the tab controls.  Usually, this is the error message display control.
        /// </summary>
        /// <param name="aControl"></param>
        /// <param name="Title"></param>
        private void AddControl(UserControl aControl, string Title)
        {
            _theTabControl.Controls.Clear();
            AddToTabControl(aControl, Title);
        }

        /// <summary>
        /// Set up all widgets
        /// </summary>
        private void SetupWidgets()
        {
            _theSecurityPoliciesPasswordUserControl = new SecurityPoliciesPasswordUserControl(_thePolicies);
            _theSecurityPoliciesSystemUserControl = new SecurityPoliciesSystemUserControl(_thePolicies);
            _theSecurityPoliciesLoggingUserControl = new SecurityPoliciesLoggingUserControl(_thePolicies);
            _theSecurityPoliciesPowerRoleMgmtUserControl = new SecurityPoliciesPowerRoleMgmtUserControl(_thePolicies);
            AddTabPages();
            _setupCompleted = true;
            _loadDefaultButton.Enabled = true;
            _loadMostSecureButton.Enabled = true;
            AddChangeTrackers();
        }

        /// <summary>
        /// Add change trackers to monitor changes.
        /// </summary>
        private void AddChangeTrackers()
        {
            AddSystemUserControlChangeTracker();
            AddPowerRoleMgmtUserControlChangeTracker();
            AddLoggingUserControlChangeTracker();
        }

        /// <summary>
        /// Add change tracker for system policy.
        /// </summary>
        private void AddSystemUserControlChangeTracker()
        {
            if (_theSystemUserControlChangeTracker != null)
            {
                _theSystemUserControlChangeTracker.RemoveChangeHandlers();
            }
            _theSystemUserControlChangeTracker =
                new TrafodionChangeTracker(_theSecurityPoliciesSystemUserControl);
            _theSystemUserControlChangeTracker.OnChangeDetected +=
                new TrafodionChangeTracker.ChangeDetected(_theSystemUserControlChangeTracker_OnChangeDetected);
            _theSystemUserControlChangeTracker.EnableChangeEvents = true;
        }

        /// <summary>
        /// System change tracker event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theSystemUserControlChangeTracker_OnChangeDetected(object sender, EventArgs e)
        {
            _theSecurityPoliciesSystemUserControl.IsChangePending = true;
        }

        /// <summary>
        /// Add a change tracker for power role mgmt policy.
        /// </summary>
        private void AddPowerRoleMgmtUserControlChangeTracker()
        {
            if (_thePowerRoleMgmtUserControlChangeTracker != null)
            {
                _thePowerRoleMgmtUserControlChangeTracker.RemoveChangeHandlers();
            }
            _thePowerRoleMgmtUserControlChangeTracker = 
                new TrafodionChangeTracker(_theSecurityPoliciesPowerRoleMgmtUserControl);
            _thePowerRoleMgmtUserControlChangeTracker.OnChangeDetected += 
                new TrafodionChangeTracker.ChangeDetected(_thePowerRoleMgmtUserControlChangeTracker_OnChangeDetected);
            _thePowerRoleMgmtUserControlChangeTracker.EnableChangeEvents = true;
        }

        /// <summary>
        /// Event handler for power role mgmt change tracker.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _thePowerRoleMgmtUserControlChangeTracker_OnChangeDetected(object sender, EventArgs e)
        {
            _theSecurityPoliciesPowerRoleMgmtUserControl.IsChangePending = true;
        }

        /// <summary>
        /// Add a change tracker for logging policy.
        /// </summary>
        private void AddLoggingUserControlChangeTracker()
        {
            if (_theLoggingUserControlChangeTracker != null)
            {
                _theLoggingUserControlChangeTracker.RemoveChangeHandlers();
            }
            _theLoggingUserControlChangeTracker =
                new TrafodionChangeTracker(_theSecurityPoliciesLoggingUserControl);
            _theLoggingUserControlChangeTracker.OnChangeDetected +=
                new TrafodionChangeTracker.ChangeDetected(_theLoggingUserControlChangeTracker_OnChangeDetected);
            _theLoggingUserControlChangeTracker.EnableChangeEvents = true;
        }

        /// <summary>
        /// The event handler for logging policy change tracker.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theLoggingUserControlChangeTracker_OnChangeDetected(object sender, EventArgs e)
        {
            _theSecurityPoliciesLoggingUserControl.IsChangePending = true;
        }

        /// <summary>
        /// Enable/Disable all change trackers. 
        /// </summary>
        /// <param name="enable"></param>
        private void EnableChangeTrackers(bool enable)
        {
            _theSystemUserControlChangeTracker.EnableChangeEvents = enable;
            _thePowerRoleMgmtUserControlChangeTracker.EnableChangeEvents = enable;
            _theLoggingUserControlChangeTracker.EnableChangeEvents = enable;
        }


        /// <summary>
        /// Add all tab pages into the tab control.
        /// </summary>
        private void AddTabPages()
        {
            _theTabControl.Controls.Clear();

            //If user does not have view only privileges, then show the system policies.
            if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                AddToTabControl(_theSecurityPoliciesSystemUserControl, Properties.Resources.SystemPolicyTitle);
            }
            else
            {
                //If user only has read privileges,disable the controls, so user cannot modify any values in the UI.
                Utilities.DisableControl(_theSecurityPoliciesPasswordUserControl);
                Utilities.DisableControl(_theSecurityPoliciesLoggingUserControl);
                Utilities.DisableControl(_theSecurityPoliciesPowerRoleMgmtUserControl);
            }
            AddToTabControl(_theSecurityPoliciesPasswordUserControl, Properties.Resources.PasswordPolicyTitle);
            AddToTabControl(_theSecurityPoliciesLoggingUserControl, Properties.Resources.LoggingPolicyTitle);
            AddToTabControl(_theSecurityPoliciesPowerRoleMgmtUserControl, Properties.Resources.PowerRoleMgmtTitle);
            _errorMessageShown = false;
        }

        /// <summary>
        /// To refresh all of the policies from the server. 
        /// </summary>
        private void DoRefresh()
        {
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.RetrieveSecurityPolicies,
                                                             _thePolicies,
                                                             "Refresh",
                                                             new Object[0]);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();

            if (progressDialog.Error != null)
            {
                SecurityShowExceptionUserControl showException =
                            new SecurityShowExceptionUserControl(String.Format(Properties.Resources.ErrorOperationFailed,
                                                                               Properties.Resources.RetrieveSecurityPolicies,
                                                                               progressDialog.Error.Message));
                AddControl(showException, Properties.Resources.Message);
                _errorMessageShown = true;
            }
            else
            {
                if (_setupCompleted)
                {
                    EnableChangeTrackers(false);
                    if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
                    {
                        _theSecurityPoliciesSystemUserControl.Reload();
                    }
                    _theSecurityPoliciesPasswordUserControl.Reload();
                    _theSecurityPoliciesLoggingUserControl.Reload();
                    _theSecurityPoliciesPowerRoleMgmtUserControl.Reload();
                    CurrentSetting = Policies.POLICY_SETTING.SETTING_CUSTOMER;

                    // If the error message is currently shown, need to add all pages in. 
                    if (_errorMessageShown)
                    {
                        AddTabPages();
                    }

                    EnableChangeTrackers(true);
                }
                else
                {
                    SetupWidgets();
                }
            }
        }

        /// <summary>
        /// To load all of the policies from the default factory settings.
        /// </summary>
        private void DoLoadFactoryDefaultSettings()
        {
            if (!_setupCompleted)
            {
                return;
            }

            if (SystemSecurity.FactoryDefaultPolicies == null)
            {
                SystemSecurity.FactoryDefaultPolicies = 
                    new Policies(ConnectionDefn, Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT);

                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.RetrieveDefaultSecurityPolicies,
                                                     SystemSecurity.FactoryDefaultPolicies,
                                                     "LoadPolicies",
                                                     new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    SecurityShowExceptionUserControl showException =
                                new SecurityShowExceptionUserControl(String.Format(Properties.Resources.ErrorOperationFailed,
                                                                                   Properties.Resources.RetrieveDefaultSecurityPolicies,
                                                                                   progressDialog.Error.Message));
                    AddControl(showException, Properties.Resources.Message);
                    SystemSecurity.FactoryDefaultPolicies = null;
                    _errorMessageShown = true;
                    return;
                }
            }

            // If the factory policies exist, do not refresh it since they would not be changed.
            EnableChangeTrackers(false);
            if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                _theSecurityPoliciesSystemUserControl.Reload(SystemSecurity.FactoryDefaultPolicies,
                                                             Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT);
            }
            _theSecurityPoliciesPasswordUserControl.Reload(SystemSecurity.FactoryDefaultPolicies, 
                                                         Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT);
            _theSecurityPoliciesLoggingUserControl.Reload(SystemSecurity.FactoryDefaultPolicies, 
                                                         Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT);
            _theSecurityPoliciesPowerRoleMgmtUserControl.Reload(SystemSecurity.FactoryDefaultPolicies, 
                                                         Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT);
            CurrentSetting = Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT;

            // If the error message is currently shown, need to add all pages in. 
            if (_errorMessageShown)
            {
                AddTabPages();
            }

            EnableChangeTrackers(true);
        }

        /// <summary>
        /// To load all of the policies from the most strong settings.
        /// </summary>
        private void DoLoadMostSecureSettings()
        {
            if (!_setupCompleted)
            {
                return;
            }

            if (SystemSecurity.MostSecurePolicies == null)
            {
                SystemSecurity.MostSecurePolicies =
                    new Policies(ConnectionDefn, Policies.POLICY_SETTING.SETTING_MOSE_SECURE);

                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.RetrieveMostStrongSecurityPolicies,
                                                     SystemSecurity.MostSecurePolicies,
                                                     "LoadPolicies",
                                                     new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    SecurityShowExceptionUserControl showException =
                                new SecurityShowExceptionUserControl(String.Format(Properties.Resources.ErrorOperationFailed,
                                                                                   Properties.Resources.RetrieveMostStrongSecurityPolicies,
                                                                                   progressDialog.Error.Message));
                    AddControl(showException, Properties.Resources.Message);
                    SystemSecurity.MostSecurePolicies = null;
                    _errorMessageShown = true;
                    return;
                }
            }

            // If the factory policies exist, do not refresh it since the settings would not be changed.
            EnableChangeTrackers(false);
            if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                _theSecurityPoliciesSystemUserControl.Reload(SystemSecurity.MostSecurePolicies,
                                                             Policies.POLICY_SETTING.SETTING_MOSE_SECURE);
            }
            _theSecurityPoliciesPasswordUserControl.Reload(SystemSecurity.MostSecurePolicies, 
                                                           Policies.POLICY_SETTING.SETTING_MOSE_SECURE);
            _theSecurityPoliciesLoggingUserControl.Reload(SystemSecurity.MostSecurePolicies, 
                                                          Policies.POLICY_SETTING.SETTING_MOSE_SECURE);
            _theSecurityPoliciesPowerRoleMgmtUserControl.Reload(SystemSecurity.MostSecurePolicies, 
                                                          Policies.POLICY_SETTING.SETTING_MOSE_SECURE);
            CurrentSetting = Policies.POLICY_SETTING.SETTING_MOSE_SECURE;

            // If the error message is currently shown, need to add all pages in. 
            if (_errorMessageShown)
            {
                AddTabPages();
            }
            EnableChangeTrackers(true);
        }

        /// <summary>
        /// To remember the tab selection has been changed.  
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void SecurityPoliciesTabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            CurrentSelectedIndex = _theTabControl.SelectedIndex;
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            // Create the tab page with the user control dock filled
            TabPage theTabPage = new TrafodionTabPage(aTabText);

            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);

            _theTabControl.TabPages.Add(theTabPage);
        }

        #endregion Private methods
    }
}
