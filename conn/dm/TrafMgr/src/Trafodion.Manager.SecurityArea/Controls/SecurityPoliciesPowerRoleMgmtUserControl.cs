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
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// System related security policies
    /// </summary>
    public partial class SecurityPoliciesPowerRoleMgmtUserControl : UserControl
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
        /// PasswordRequiredToResetPowerRoles: Current password required to reset power users password.
        /// </summary>
        public bool PasswordRequiredToResetPowerRoles
        {
            get { return _pwdRequiredToResetPowerRolesCheckBox.Checked; }
            set { _pwdRequiredToResetPowerRolesCheckBox.Checked = value; }
        }

        /// <summary>
        /// PasswordRequiredToResetSuper: Current password required to reset super ID's password.
        /// </summary>
        public bool PasswordRequiredToResetSuper
        {
            get { return _pwdRequiredToResetSuperCheckBox.Checked; }
            set { _pwdRequiredToResetSuperCheckBox.Checked = value; }
        }

        /// <summary>
        /// RolePasswordForDatabaseUsers: Role password required for Power database users?
        /// </summary>
        public bool RolePasswordForDatabaseUsers
        {
            get { return _powerDatabaseUsersRadioButton.Checked; }
            set 
            {
                _powerDatabaseUsersRadioButton.Checked = value;
                _noneDatabaseUsersRadioButton.Checked = !value;
            }
        }

        /// <summary>
        /// RolePasswordForPlatformUsers: Setting for role password required for platform users.
        /// </summary>
        public Policies.ROLE_PWD_FOR_PLATOFRM RolePasswordForPlatformUsers
        {
            get 
            {
                if (_allPlatformUsersRadioButton.Checked)
                {
                    return Policies.ROLE_PWD_FOR_PLATOFRM.A;
                }
                else if (_powerPlatformUsersRadioButton.Checked)
                {
                    return Policies.ROLE_PWD_FOR_PLATOFRM.P;
                }
                else
                {
                    return Policies.ROLE_PWD_FOR_PLATOFRM.N;
                }
            }
            set 
            {
                switch (value)
                {
                    case Policies.ROLE_PWD_FOR_PLATOFRM.A:
                        _allPlatformUsersRadioButton.Checked = true;
                        break;
                    case Policies.ROLE_PWD_FOR_PLATOFRM.P:
                        _powerPlatformUsersRadioButton.Checked = true;
                        break;
                    default:
                        _nonePlatformUsersRadioButton.Checked = true;
                        break;
                }
            }
        }
            
        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor with a model poicicies object.
        /// </summary>
        /// <param name="thePolicies"></param>
        public SecurityPoliciesPowerRoleMgmtUserControl(Policies thePolicies)
        {
            InitializeComponent();
            AddToolTips();
            _thePolicies = thePolicies;
            Reload();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refresh all attruibutes. 
        /// Note: this will trigger a connection to the server to fetch the current attribute. 
        /// Note: The caller should have refresh the model object.
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

        private void AddToolTips()
        {
            this._toolTip.SetToolTip(this._pwdRequiredToResetPowerRolesCheckBox, Utilities.TrafodionTextWrapper("Specifies whether the old password is required when changing the passwords for power roles (ROLE.SECMGR, ROLE.MGR, or ROLE.DBA).", 50));
            this._toolTip.SetToolTip(this._pwdRequiredToResetSuperCheckBox, Utilities.TrafodionTextWrapper("Specifies whether the old password is required when changing the password for the SUPER.SUPER.", 50));
            this._toolTip.SetToolTip(this._powerDatabaseUsersRadioButton, Utilities.TrafodionTextWrapper("Specifies that all power database users are rquired to supply two passwords at login: the first for the user, and the second for the role of the user.", 50));
            this._toolTip.SetToolTip(this._noneDatabaseUsersRadioButton, Utilities.TrafodionTextWrapper("Specifies that no database users are required to supply an additional role password at login.", 50));
            this._toolTip.SetToolTip(this._allPlatformUsersRadioButton, Utilities.TrafodionTextWrapper("Specifies that all platform users are required to supply an additional role password at login.", 50));
            this._toolTip.SetToolTip(this._powerPlatformUsersRadioButton, Utilities.TrafodionTextWrapper("Specifies that only the platform power users are required to supply an additional role password at login.", 50));
            this._toolTip.SetToolTip(this._nonePlatformUsersRadioButton, Utilities.TrafodionTextWrapper("Specifies that no platform users are required to supply an additional role password at login.", 50));
        }

        /// <summary>
        /// Load attributes from the model policies. 
        /// </summary>
        /// <param name="policies"></param>
        private void LoadAttributes(Policies policies)
        {
            RolePasswordForDatabaseUsers = policies.RolePasswordForDatabase;
            RolePasswordForPlatformUsers = policies.RolePasswordForPlatform;
            PasswordRequiredToResetPowerRoles = policies.PwdReqForPowerRoleReset;
            PasswordRequiredToResetSuper = policies.PwdReqForSuperReset;
        }

        /// <summary>
        /// Set policies attributes from user's input. 
        /// </summary>
        /// <param name="policies"></param>
        private void SetAttributes(Policies policies)
        {
            policies.RolePasswordForDatabase = RolePasswordForDatabaseUsers;
            policies.RolePasswordForPlatform = RolePasswordForPlatformUsers;
            policies.PwdReqForPowerRoleReset = PasswordRequiredToResetPowerRoles;
            policies.PwdReqForSuperReset = PasswordRequiredToResetSuper;
        }
        #endregion Private methods
    }
}
