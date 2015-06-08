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
using System.Collections.Generic;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class SecurityPoliciesUserControl : UserControl
    {
        #region Fields

        private bool _changesPending = false;

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        public SecurityPoliciesUserControl()
        {
            InitializeComponent();
            _numberOfPwdCriteria.SelectedIndex = 0;
            _changesPending = false;
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        private void Object_Click(object sender, System.EventArgs e)
        {
            _changesPending = true;
        }

        private void _logUserManagement_Click(object sender, System.EventArgs e)
        {
            _changesPending = true;
            if (_logUserManagement.Checked)
            {
                _logUserManagementRequired.Enabled = true;
            }
            else
            {
                _logUserManagementRequired.Enabled = false;
            }
        }

        private void _logChangePassword_Click(object sender, System.EventArgs e)
        {
            _changesPending = true;
            if (_logChangePassword.Checked)
            {
                _logChangePasswordRequired.Enabled = true;
            }
            else
            {
                _logChangePasswordRequired.Enabled = false;
            }
        }

        private void _logDatabaseLogonOK_Click(object sender, System.EventArgs e)
        {
            _changesPending = true;
            if (_logDatabaseLogonOK.Checked)
            {
                _logDatabaseLogonOKRequired.Enabled = true;
            }
            else
            {
                _logDatabaseLogonOKRequired.Enabled = false;
            }
        }

        private void _logPlatformLogonOK_Click(object sender, System.EventArgs e)
        {
            _changesPending = true;
            if (_logPlatformLogonOK.Checked)
            {
                _logPlatformLogonOKRequired.Enabled = true;
            }
            else
            {
                _logPlatformLogonOKRequired.Enabled = false;
            }
        }

        private void _numberOfPwdCriteria_SelectedIndexChanged(object sender, System.EventArgs e)
        {
            _changesPending = true;
        }

        private void _numberOfPwdCriteria_Click(object sender, System.EventArgs e)
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

        private void _refreshButton_Click(object sender, System.EventArgs e)
        {
            _changesPending = false;
        }

        private void _resetButton_Click(object sender, System.EventArgs e)
        {
            _changesPending = false;
        }

        private void _applyButton_Click(object sender, System.EventArgs e)
        {

        }
        #endregion Private methods
    }
}
