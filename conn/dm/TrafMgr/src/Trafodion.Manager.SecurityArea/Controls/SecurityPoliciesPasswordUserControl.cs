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
    /// <summary>
    /// User control to handle Password policies. 
    /// </summary>
    public partial class SecurityPoliciesPasswordUserControl : UserControl
    {
        #region Fields

        private bool _changesPending = false;
        private Policies _thePolicies = null;
        private Policies.POLICY_SETTING _theCurrentSetting = Policies.POLICY_SETTING.SETTING_CUSTOMER;
        private TrafodionChangeTracker _theChangeTracker = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// IsChangePending: Is there any change pending?
        /// </summary>
        public bool IsChangePending
        {
            get { return _changesPending; }
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
        /// MinLength: Minimum password length
        /// </summary>
        public int MinLength
        {
            get { return (int)_pwdMinLengthSpinner.Value; }
            set { _pwdMinLengthSpinner.Value = value; }
        }

        /// <summary>
        /// History: The depth of the password history to remember
        /// </summary>
        public int History
        {
            get { return (int)_pwdHistorySpinner.Value; }
            set { _pwdHistorySpinner.Value = value; }
        }

        /// <summary>
        /// GracePeriod: The grace period after the password has expired in days. 
        /// </summary>
        public int GracePeriod
        {
            get { return (int)_pwdGracePeriodSpinner.Value; }
            set { _pwdGracePeriodSpinner.Value = value; }
        }

        /// <summary>
        /// CanChangeWithin: Password can be changed within the days before the password is expired. 
        /// </summary>
        public int CanChangeWithin
        {
            get { return (int)_pwdCanChangeSpinner.Value; }
            set { _pwdCanChangeSpinner.Value = value; }
        }

        /// <summary>
        /// MaxLogonAttempts: Maximum logon attempts are allowed before a delay is applied.  
        /// </summary>
        public int MaxLogonAttempts
        {
            get { return (int)_pwdMaxLogonSpinner.Value; }
            set { _pwdMaxLogonSpinner.Value = value; }
        }

        /// <summary>
        /// LogonFailedDelay: The logon delay in seconds. 
        /// </summary>
        public int LogonFailedDelay
        {
            get { return (int)_pwdLogonDelaySpinner.Value; }
            set { _pwdLogonDelaySpinner.Value = value; }
        }

        /// <summary>
        /// NumberOfPasswordCriteria: Number of password criteria required for password quality. 
        /// Note: this has be set after all of the password required options are set. Else,
        ///       the combobox list will not be calculated correctly.
        /// </summary>
        public int NumberOfPasswordCriteria
        {
            get { return _numberOfPwdCriteria.SelectedIndex; }
            set 
            {
                MakeOverNumberOfCriteriaComboBox();
                if (value < 0)
                {
                    _numberOfPwdCriteria.SelectedIndex = 0;
                }
                else if (value > _numberOfPwdCriteria.Items.Count - 1)
                {
                    _numberOfPwdCriteria.SelectedIndex = _numberOfPwdCriteria.Items.Count - 1;
                }
                else
                {
                    _numberOfPwdCriteria.SelectedIndex = value;
                }
            }
        }

        /// <summary>
        /// PasswordUppercaseRequired: Upper case letters are required in passwords. 
        /// </summary>
        public bool PasswordUppercaseRequired
        {
            get { return _pwdUpperRequired.Checked; }
            set { _pwdUpperRequired.Checked = value; }
        }

        /// <summary>
        /// PasswordLowercaseRquired: Lower case letters are required in passwords.
        /// </summary>
        public bool PasswordLowercaseRquired
        {
            get { return _pwdLowerRequired.Checked; }
            set { _pwdLowerRequired.Checked = value; }
        }

        /// <summary>
        /// PasswordNumberRequired: Numeric characters are required in passwords.
        /// </summary>
        public bool PasswordNumberRequired
        {
            get { return _pwdNumberRequired.Checked; }
            set { _pwdNumberRequired.Checked = value; }
        }

        /// <summary>
        /// PasswordSpeicalRequired: Special characters are required in passwords. 
        /// </summary>
        public bool PasswordSpeicalRequired
        {
            get { return _pwdSpecialRequired.Checked; }
            set { _pwdSpecialRequired.Checked = value; }
        }

        /// <summary>
        /// DefaultNoExpiry: Password will not expire?
        /// </summary>
        public bool DefaultNoExpiry
        {
            get { return _thePasswordExpirationPolicyControl.NoExpiry; }
            set { _thePasswordExpirationPolicyControl.NoExpiry = value; }
        }

        /// <summary>
        /// DefaultExpiryDays: Password will be expired in days
        /// </summary>
        public int DefaultExpiryDays
        {
            get { return _thePasswordExpirationPolicyControl.ExpiryDays; }
            set { _thePasswordExpirationPolicyControl.ExpiryDays = value; }
        }

        /// <summary>
        /// DefaultExpiryDate: Password expiration date. 
        /// </summary>
        public string DefaultExpiryDate
        {
            get { return _thePasswordExpirationPolicyControl.ExpiryDate; }
            set { _thePasswordExpirationPolicyControl.ExpiryDate = value; }
        }

        /// <summary>
        /// PasswordNoUserName: no user name as password
        /// </summary>
        public bool PasswordNoUserName
        {
            get { return _pwdQualNoUserNameCheckBox.Checked; }
            set { _pwdQualNoUserNameCheckBox.Checked = value; }
        }

        /// <summary>
        /// PasswordNoRepeatChars: no repeat chars in password
        /// </summary>
        public bool PasswordNoRepeatChars
        {
            get { return _pwdQualNoRepeatCharsCheckBox.Checked; }
            set { _pwdQualNoRepeatCharsCheckBox.Checked = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Given the policies model object and create the user control.
        /// </summary>
        /// <param name="thePolicies"></param>
        public SecurityPoliciesPasswordUserControl(Policies thePolicies)
        {
            InitializeComponent();
            AddToolTips();
            _thePolicies = thePolicies;
            _thePasswordExpirationPolicyControl.GroupboxText = "Default Password Expiration Policy";
            _numberOfPwdCriteria.SelectedIndex = 0;
            Reload();

            //Add a tracker to track changes
            AddChangeTracker();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refresh all of the attributes.
        /// Note: this causes direct connect to server to retrieve all current attributes.
        /// Note: the caller need to refresh the policies model object. 
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
            if (_theChangeTracker != null)
            {
                _theChangeTracker.EnableChangeEvents = false;
            }

            LoadAttributes(aPolicies);
            CurrentSetting = aSetting;
            _changesPending = false;

            if (_theChangeTracker != null)
            {
                _theChangeTracker.EnableChangeEvents = true;
            }
        }

        public void Update()
        {
            if (_changesPending || CurrentSetting == Policies.POLICY_SETTING.SETTING_FACTORY_DEFAULT || CurrentSetting == Policies.POLICY_SETTING.SETTING_MOSE_SECURE)
            {
                SetAttributes(_thePolicies);
            }
        }

        #endregion Public methods

        #region Private methods

        private void AddToolTips()
        {
            this._toolTip.SetToolTip(this._pwdHistorySpinner, Utilities.TrafodionTextWrapper("Records a specified number of previously used passwords for each user and does not allow a user to change his or her password to any password in this history. The valid values range from 0 to 60.", 50));
            this._toolTip.SetToolTip(this._pwdGracePeriodSpinner, Utilities.TrafodionTextWrapper("Specifies the number of days after password expiration during which users are allowed to change their expired passwords during login. The valid values range from 0 to 365.", 50));
            this._toolTip.SetToolTip(this._pwdCanChangeSpinner, Utilities.TrafodionTextWrapper("Specifies the number of days prior to expiration that a user can change the password.  Valid values range from 0 to 365.", 50));
            this._toolTip.SetToolTip(this._pwdMaxLogonSpinner, Utilities.TrafodionTextWrapper("Specifies the maximum number of failed login attempts for a single user ID before the system freezes the user ID or causes a timeout to occur. The valid values range from 0 to 60.", 50));
            this._toolTip.SetToolTip(this._pwdLogonDelaySpinner, Utilities.TrafodionTextWrapper("Specifies the timeout for a user ID if the Max Login Attempts value is exceeded. The valid values range from 0 to 86400.", 50));
            this._toolTip.SetToolTip(this._pwdQualNoUserNameCheckBox, Utilities.TrafodionTextWrapper("Specifies whether the password can contain the user name.", 50));
            this._toolTip.SetToolTip(this._pwdQualNoRepeatCharsCheckBox, Utilities.TrafodionTextWrapper("Specifies whether the password can contain more than two consecutive instances of the same character.", 50));
            this._toolTip.SetToolTip(this._pwdMinLengthSpinner, Utilities.TrafodionTextWrapper("Specifies the minimum acceptable length of a password. The valid values range from 6 to 64.", 50));
            this._toolTip.SetToolTip(this._numberOfPwdCriteria, Utilities.TrafodionTextWrapper("Specifies the minimum quality criteria that must be met when a password is set or changed. The valid values range from 0 to 4.", 50));
            this._toolTip.SetToolTip(this._pwdUpperRequired, Utilities.TrafodionTextWrapper("Specifies whether a password must have at least one uppercase character.", 50));
            this._toolTip.SetToolTip(this._pwdLowerRequired, Utilities.TrafodionTextWrapper("Specifies whether a password must have at least one lowercase character.", 50));
            this._toolTip.SetToolTip(this._pwdNumberRequired, Utilities.TrafodionTextWrapper("Specifies whether a password must have at least one numeric character.", 50));
            this._toolTip.SetToolTip(this._pwdSpecialRequired, Utilities.TrafodionTextWrapper("Specifies whether a password must have at least one special character.", 50));
        }

        private void AddChangeTracker()
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
            _theChangeTracker = new TrafodionChangeTracker(_thePasswordExpirationPolicyControl);
            _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(_theChangeTracker_OnChangeDetected);
            _theChangeTracker.EnableChangeEvents = true;
        }

        private void _theChangeTracker_OnChangeDetected(object sender, System.EventArgs e)
        {
            _changesPending = true;
        }

        /// <summary>
        /// Remember something has been changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Object_Changed(object sender, System.EventArgs e)
        {
            _changesPending = true;
        }

        /// <summary>
        /// Remember the password criteria has been changed. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _numberOfPwdCriteria_SelectedIndexChanged(object sender, System.EventArgs e)
        {
            //if (_numberOfPwdCriteria.SelectedIndex < 0)
            //{
            //    _numberOfPwdCriteria.SelectedIndex = 0;
            //}
            //else if (_numberOfPwdCriteria.SelectedIndex > _numberOfPwdCriteria.Items.Count)
            //{
            //    _numberOfPwdCriteria.SelectedIndex = _numberOfPwdCriteria.Items.Count-1;
            //}

            _changesPending = true;
        }

        /// <summary>
        /// Event handler for the password required option changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PwdQualRequirement_Changed(object sender, System.EventArgs e)
        {
            // Mark the change first. 
            _changesPending = true;

            // Save the original selection.
            int origCriteria = _numberOfPwdCriteria.SelectedIndex;

            MakeOverNumberOfCriteriaComboBox();

            // Restoring the original selection but make sure it does not go out of bounds.
            if (origCriteria > _numberOfPwdCriteria.Items.Count-1)
            {
                _numberOfPwdCriteria.SelectedIndex = _numberOfPwdCriteria.Items.Count-1;
            }
            else
            {
                _numberOfPwdCriteria.SelectedIndex = origCriteria;
            }
        }

        /// <summary>
        /// Re-create the number of criteria combox with the current option settings.
        /// </summary>
        private void MakeOverNumberOfCriteriaComboBox()
        {
            _numberOfPwdCriteria.Items.Clear();
            _numberOfPwdCriteria.Items.Add("0");
            int items = 0;
            if (_pwdUpperRequired.Checked)
            {
                items++;
                _numberOfPwdCriteria.Items.Add(items.ToString());
            }
            if (_pwdLowerRequired.Checked)
            {
                items++;
                _numberOfPwdCriteria.Items.Add(items.ToString());
            }
            if (_pwdNumberRequired.Checked)
            {
                items++;
                _numberOfPwdCriteria.Items.Add(items.ToString());
            }
            if (_pwdSpecialRequired.Checked)
            {
                items++;
                _numberOfPwdCriteria.Items.Add(items.ToString());
            }
        }

        /// <summary>
        /// Load all of the attributes from the given model object. 
        /// </summary>
        /// <param name="policies"></param>
        private void LoadAttributes(Policies policies)
        {
            MinLength = policies.PwdQualMinLength;
            History = policies.PwdHistory;
            GracePeriod = policies.PwdCtrlGracePeriod;
            CanChangeWithin = policies.PwdCanChangeWithin;
            MaxLogonAttempts = policies.PwdAuthFailsBeforeDelay;
            LogonFailedDelay = policies.PwdAuthFailDelayInSecs;
            PasswordUppercaseRequired = policies.PwdQualReqUpper;
            PasswordLowercaseRquired = policies.PwdQualReqLower;
            PasswordNumberRequired = policies.PwdQualReqNumber;
            PasswordSpeicalRequired = policies.PwdQualReqSpecChar;

            // Watch out that the number of criteria needs to be set after the four 
            // password required options.
            NumberOfPasswordCriteria = policies.PwdQualReqCriteria;

            DefaultNoExpiry = (!policies.DefaultPasswordExpiry);
            if (policies.DefaultPasswordExpiry)
            {
                DefaultExpiryDays = policies.PwdDefaultExprDays;
                DefaultExpiryDate = policies.PwdDefaultExprDate;
            }
            else
            {
                DefaultExpiryDate = "";
                DefaultExpiryDays = 0;
            }

            PasswordNoUserName = policies.PwdQualNoUserName;
            PasswordNoRepeatChars = policies.PwdQualNoRepeatChars;
        }

        /// <summary>
        /// Setting the model object with the attribute input from GUI
        /// Note: the model object will perform comparison to formulate updated vectors.
        /// </summary>
        /// <param name="policies"></param>
        private void SetAttributes(Policies policies)
        {
            policies.PwdQualMinLength = MinLength;
            policies.PwdHistory = History;
            policies.PwdCtrlGracePeriod = GracePeriod;
            policies.PwdCanChangeWithin = CanChangeWithin;
            policies.PwdAuthFailsBeforeDelay = MaxLogonAttempts;
            policies.PwdAuthFailDelayInSecs = LogonFailedDelay;
            policies.PwdQualReqCriteria = NumberOfPasswordCriteria;
            policies.PwdQualReqUpper = PasswordUppercaseRequired;
            policies.PwdQualReqLower = PasswordLowercaseRquired;
            policies.PwdQualReqNumber = PasswordNumberRequired;
            policies.PwdQualReqSpecChar = PasswordSpeicalRequired;

            if (DefaultNoExpiry)
            {
                policies.PwdDefaultExprDays = 0;
                policies.PwdDefaultExprDate = "";
            }
            else
            {
                policies.PwdDefaultExprDays = DefaultExpiryDays;
                policies.PwdDefaultExprDate = DefaultExpiryDate;
            }
           
            policies.PwdQualNoUserName = PasswordNoUserName;
            policies.PwdQualNoRepeatChars = PasswordNoRepeatChars;
        }

        #endregion Private methods
    }
}
