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
using System.Windows.Forms;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class SecurityPoliciesLoggingUserControl : UserControl
    {
        #region Fields

        private bool _changesPending = false;
        private Policies _thePolicies = null;
        private Policies.POLICY_SETTING _theCurrentSetting = Policies.POLICY_SETTING.SETTING_CUSTOMER;

        #endregion Fields

        #region Properties

        /// <summary>
        /// IsChangePending: Is there any change pending?
        /// </summary>
        public bool IsChangePending
        {
            get { return _changesPending; }
            set { _changesPending = value; }
        }

        /// <summary>
        /// CurrentSetting: What is the current value setting?
        /// </summary>
        public Policies.POLICY_SETTING CurrentSetting
        {
            get { return _theCurrentSetting; }
            set { _theCurrentSetting = value; }
        }

        /// <summary>
        /// LogUserManagement: Log all user management operations?
        /// </summary>
        public bool LogUserManagement
        {
            get { return _logUserManagement.Checked; }
            set { _logUserManagement.Checked = value; }
        }

        /// <summary>
        /// LogUserManagementRequired: Logging successful is required for all user management operations?
        /// </summary>
        public bool LogUserManagementRequired
        {
            get { return _logUserManagementRequired.Checked; }
            set 
            {
                if (LogUserManagement)
                {
                    _logUserManagementRequired.Checked = value;
                }
                else
                {
                    _logUserManagementRequired.Checked = false;
                }
            }
        }

        /// <summary>
        /// LogChangePassword: Log all change passwords? 
        /// </summary>
        public bool LogChangePassword
        {
            get { return _logChangePassword.Checked; }
            set { _logChangePassword.Checked = value; }
        }

        /// <summary>
        /// LogChangePasswordRequired: Successful logging is required for all change passwords. 
        /// </summary>
        public bool LogChangePasswordRequired
        {
            get { return _logChangePasswordRequired.Checked; }
            set 
            {
                if (LogChangePassword)
                {
                    _logChangePasswordRequired.Checked = value;
                }
                else
                {
                    _logChangePasswordRequired.Checked = false;
                }
            }
        }

        /// <summary>
        /// LogDatabaseLogonOK: Log all successful database logon? 
        /// </summary>
        public bool LogDatabaseLogonOK
        {
            get { return _logDatabaseLogonOK.Checked; }
            set { _logDatabaseLogonOK.Checked = value; }
        }

        /// <summary>
        /// LogDatabaseLogonOKRequired: Successful logging is required for all database logon. 
        /// </summary>
        public bool LogDatabaseLogonOKRequired
        {
            get { return _logDatabaseLogonOKRequired.Checked; }
            set 
            {
                if (LogDatabaseLogonOK)
                {
                    _logDatabaseLogonOKRequired.Checked = value;
                }
                else
                {
                    _logDatabaseLogonOKRequired.Checked = false;
                }
            }
        }

        /// <summary>
        /// LogPlatformLogonOK: Log all successful platform logon? 
        /// </summary>
        public bool LogPlatformLogonOK
        {
            get { return _logPlatformLogonOK.Checked; }
            set { _logPlatformLogonOK.Checked = value; }
        }

        ///// <summary>
        ///// LogPlatformLogonOKRequired: Successful logging is required for all platform logon. 
        ///// </summary>
        //public bool LogPlatformLogonOKRequired
        //{
        //    get { return _logPlatformLogonOKRequired.Checked; }
        //    set 
        //    {
        //        if (LogPlatformLogonOK)
        //        {
        //            _logPlatformLogonOKRequired.Checked = value;
        //        }
        //        else
        //        {
        //            _logPlatformLogonOKRequired.Checked = false;
        //        }
        //    }
        //}

        /// <summary>
        /// LogDatabaseLogonFailure: Log all database logon failure?
        /// </summary>
        public bool LogDatabaseLogonFailure
        {
            get { return _logDatabaseLogonFailure.Checked; }
            set { _logDatabaseLogonFailure.Checked = value; }
        }

        /// <summary>
        /// LogPlatformLogonFailure: Log all platform logon failure?
        /// </summary>
        public bool LogPlatformLogonFailure
        {
            get { return _logPlatformLogonFailure.Checked; }
            set { _logPlatformLogonFailure.Checked = value; }
        }

        /// <summary>
        /// LogFileAgesInDays: Log file will age in days
        /// </summary>
        public int LogFileAgesInDays
        {
            get { return (int)_logFileAgesInDaysSpinner.Value; }
            set { _logFileAgesInDaysSpinner.Value = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="thePolicies"></param>
        public SecurityPoliciesLoggingUserControl(Policies thePolicies)
        {
            InitializeComponent();
            AddToolTips();
            _thePolicies = thePolicies;
            LoadAttributes(_thePolicies);
            _changesPending = false;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refresh all attributes. 
        /// Note: the caller needs to refresh the model object first. 
        /// </summary>
        public void Reload()
        {
            Reload(_thePolicies, Policies.POLICY_SETTING.SETTING_CUSTOMER);
        }

        /// <summary>
        /// Reload value settings.  
        /// </summary>
        /// <param name="aPolicies"></param>
        /// <param name="aSetting"></param>
        public void Reload(Policies aPolicies, Policies.POLICY_SETTING aSetting)
        {
            LoadAttributes(aPolicies);
            CurrentSetting = aSetting;
            _changesPending = false;
        }

        /// <summary>
        /// Update the policies model object with user's input.
        /// </summary>
        public void Update()
        {
            if (_changesPending || CurrentSetting == Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT || CurrentSetting == Policies.POLICY_SETTING.SETTING_MOSE_SECURE)
            {
                SetAttributes(_thePolicies);
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Add tool tips.
        /// </summary>
        private void AddToolTips()
        {
            this._toolTip.SetToolTip(this._logUserManagement, Utilities.TrafodionTextWrapper("Specifies whether to log all user-management operations.", 50));
            this._toolTip.SetToolTip(this._logUserManagementRequired, Utilities.TrafodionTextWrapper("Specifies whether to abort a user-management operation if logging fails.", 50));
            this._toolTip.SetToolTip(this._logChangePassword, Utilities.TrafodionTextWrapper("Specifies whether to log all change-password operations.", 50));
            this._toolTip.SetToolTip(this._logChangePasswordRequired, Utilities.TrafodionTextWrapper("Specifies whether to abort a change-password operation if logging fails.", 50));
            this._toolTip.SetToolTip(this._logDatabaseLogonOK, Utilities.TrafodionTextWrapper("Specifies whether to log all database-user logins.", 50));
            this._toolTip.SetToolTip(this._logDatabaseLogonOKRequired, Utilities.TrafodionTextWrapper("Specifies whether to abort a database-user login if logging fails.", 50));
            this._toolTip.SetToolTip(this._logPlatformLogonOK, Utilities.TrafodionTextWrapper("Specifies whether to log all platform-user logins.", 50));
            //this._toolTip.SetToolTip(this._logPlatformLogonOKRequired, Utilities.TrafodionTextWrapper("Specifies whether to abort a platform-user login if logging fails.", 50));
            this._toolTip.SetToolTip(this._logDatabaseLogonFailure, Utilities.TrafodionTextWrapper("Specifies whether to log all database-user failed login attempts.", 50));
            this._toolTip.SetToolTip(this._logPlatformLogonFailure, Utilities.TrafodionTextWrapper("Specifies whether to log all platform-user failed login attempts.", 50));
            this._toolTip.SetToolTip(this._logFileAgesInDaysSpinner, Utilities.TrafodionTextWrapper("Specifies the number of days in which the logging records will be kept. The valid values range from 7 to 1827.", 50));
        }

        ///// <summary>
        ///// Whenever an item is checked, we have to remember it in case the focus is off. 
        ///// </summary>
        ///// <param name="sender"></param>
        ///// <param name="e"></param>
        //private void Object_Click(object sender, System.EventArgs e)
        //{
        //    //_changesPending = true;
        //}

        /// <summary>
        /// If the value is changed, the "Required" checkbox need to be enabled/disabled.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _logUserManagement_CheckChanged(object sender, System.EventArgs e)
        {
            if (_logUserManagement.Checked)
            {
                _logUserManagementRequired.Enabled = true;
            }
            else
            {
                // Turn it off to prevent confusions
                LogUserManagementRequired = false;
                _logUserManagementRequired.Enabled = false;
            }
        }

        /// <summary>
        /// If the value is changed, the "Required" checkbox need to be enabled/disabled.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _logChangePassword_CheckChanged(object sender, System.EventArgs e)
        {
            if (_logChangePassword.Checked)
            {
                _logChangePasswordRequired.Enabled = true;
            }
            else
            {
                // Turn it off to prevent confusions
                LogChangePasswordRequired = false;
                _logChangePasswordRequired.Enabled = false;
            }
        }

        /// <summary>
        /// If the value is changed, the "Required" checkbox need to be enabled/disabled.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _logDatabaseLogonOK_CheckChanged(object sender, System.EventArgs e)
        {
            if (_logDatabaseLogonOK.Checked)
            {
                _logDatabaseLogonOKRequired.Enabled = true;
            }
            else
            {
                // Turn it off to prevent confusions
                LogDatabaseLogonOKRequired = false;
                _logDatabaseLogonOKRequired.Enabled = false;
            }

        }

        ///// <summary>
        ///// If value is changed, the "Required" checkbox need to be enabled/disabled.
        ///// </summary>
        ///// <param name="sender"></param>
        ///// <param name="e"></param>
        //private void _logPlatformLogonOK_CheckChanged(object sender, System.EventArgs e)
        //{
        //    if (_logPlatformLogonOK.Checked)
        //    {
        //        _logPlatformLogonOKRequired.Enabled = true;
        //    }
        //    else
        //    {
        //        // Turn it off to prevent confusions
        //        LogPlatformLogonOKRequired = false;
        //        _logPlatformLogonOKRequired.Enabled = false;
        //    }
        //}

        /// <summary>
        /// Load all of the attributes from the given model object.
        /// </summary>
        /// <param name="policies"></param>
        private void LoadAttributes(Policies policies)
        {
            LogUserManagement = policies.LogUserManagement;
            LogUserManagementRequired = policies.LogUserManagementRequired;

            LogChangePassword = policies.LogChangePassword;
            LogChangePasswordRequired = policies.LogChangePasswordRequired;

            LogDatabaseLogonOK = policies.LogDatabaseLogonOK;
            LogDatabaseLogonOKRequired = policies.LogDatabaseLogonOKRequired;

            LogPlatformLogonOK = policies.LogPlatformLogonOK;
            //LogPlatformLogonOKRequired = policies.LogPlatformLogonOKRequired;

            LogDatabaseLogonFailure = policies.LogDatabaseLogonFailure;
            LogPlatformLogonFailure = policies.LogPlatformLogonFailure;

            LogFileAgesInDays = policies.LogFileAgesInDays;
        }

        /// <summary>
        /// Set model attributes from the GUI input.
        /// </summary>
        /// <param name="policies"></param>
        private void SetAttributes(Policies policies)
        {
            policies.LogUserManagement = LogUserManagement;
            policies.LogUserManagementRequired = LogUserManagementRequired;

            policies.LogChangePassword = LogChangePassword;
            policies.LogChangePasswordRequired = LogChangePasswordRequired;

            policies.LogDatabaseLogonOK = LogDatabaseLogonOK;
            policies.LogDatabaseLogonOKRequired = LogDatabaseLogonOKRequired;

            policies.LogPlatformLogonOK = LogPlatformLogonOK;
            //policies.LogPlatformLogonOKRequired = LogPlatformLogonOKRequired;

            policies.LogDatabaseLogonFailure = LogDatabaseLogonFailure;
            policies.LogPlatformLogonFailure = LogPlatformLogonFailure;

            policies.LogFileAgesInDays = LogFileAgesInDays;
        }

        #endregion Private methods
    }
}
