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
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class ChangeRolePasswordDialog : TrafodionForm
    {
        #region private member variables

        private Role _role;
        bool _isInitialized = false;

        public bool IsInitialized
        {
            get { return _isInitialized; }
            set { _isInitialized = value; }
        }

        #endregion private member variables

        public ChangeRolePasswordDialog(Role aRole)
        {
            InitializeComponent();
            _role = aRole;
            _roleNameTextBox.Text = aRole.Name;
            _confirmPasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;
            _newPasswordTextBox.MaxLength = ConnectionDefinition.PASSWORD_MAX_LENGTH;
            _roleNameTextBox.MaxLength = ConnectionDefinition.ROLE_NAME_MAX_LENGTH;

            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.LookupPasswordAttributesForRoleText, aRole, "GetPasswordAttributes", new Object[0]);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();

            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), string.Format(Properties.Resources.RolePasswordExpirationLoadFailure, progressDialog.Error.Message), Properties.Resources.ErrorMessageTitle,
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                string expiryDate = aRole.ExpirationDate;
                expiryDate = string.IsNullOrEmpty(expiryDate) ? "" : expiryDate.Trim();

                _passwordExpirationPolicyControl.ExpiryDays = aRole.ExpirationDays;
                _passwordExpirationPolicyControl.ExpiryDate = expiryDate;
                _passwordExpirationPolicyControl.NoExpiry = (aRole.ExpirationDays < 1 && string.IsNullOrEmpty(expiryDate));
        

                SystemSecurity _systemSecurityModel = SystemSecurity.FindSystemModel(aRole.ConnectionDefinition);
                Policies _policies = _systemSecurityModel.Policies;

                args = new TrafodionProgressArgs(Properties.Resources.RetrieveSecurityPolicies, _policies, "GetAttributes", new Object[0]);
                progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), string.Format(Properties.Resources.RolePasswordPolicyLoadFailure, progressDialog.Error.Message), Properties.Resources.ErrorMessageTitle,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                else
                {
                    if (aRole.Name == "SUPER.SUPER")
                    {
                        _oldPasswordLabel.Enabled = _oldPasswordTextBox.Enabled = _policies.PwdReqForSuperReset;
                    }
                    else if (aRole.Name == "ROLE.DBA" || aRole.Name == "ROLE.SECMGR" || aRole.Name == "ROLE.MGR")
                    {
                        _oldPasswordLabel.Enabled = _oldPasswordTextBox.Enabled = _policies.PwdReqForPowerRoleReset;
                    }
                }
            }
            _isInitialized = true;
            CenterToParent();
            UpdateControls();
        }

        private void _okButton_Click(object sender, System.EventArgs e)
        {
            if(_oldPasswordTextBox.Enabled && string.IsNullOrEmpty(_oldPasswordTextBox.Text.Trim()))
            {
                if (_newPasswordTextBox.Text.Trim().Length > 0 || _confirmPasswordTextBox.Text.Trim().Length > 0)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.OldPasswordRequiredText, Properties.Resources.ErrorMessageTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }

            if(_newPasswordTextBox.Text.Trim().Equals(_confirmPasswordTextBox.Text.Trim()))
            {
                try
                {
                    Connection aConnection = new Connection(_role.ConnectionDefinition);
                    Role alteredRole = new Role(_role);
                    alteredRole.Password = aConnection.EncryptPassword(_newPasswordTextBox.Text.Trim());
                    if (_passwordExpirationPolicyControl.NoExpiry)
                    {
                        alteredRole.ExpirationDate = "";
                        alteredRole.ExpirationDays = 0;
                    }
                    else
                    {
                        alteredRole.ExpirationDays = _passwordExpirationPolicyControl.ExpiryDays;
                        alteredRole.ExpirationDate = _passwordExpirationPolicyControl.ExpiryDate;
                    }
                    int attrVector = Role.GetAttributeVectorForAlter(_role, alteredRole, _oldPasswordTextBox.Text.Trim());
                    if (attrVector > 0)
                    {
                        Cursor = Cursors.WaitCursor;
                        alteredRole.Alter(_role, _oldPasswordTextBox.Text.Trim());
                        Cursor = Cursors.Default;

                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ChangeRolesPasswordSuccessText, "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
                        Close();
                    }
                    else
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.RolePasswordNoChangeDetected, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.ErrorMessageTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                finally
                {
                    Cursor = Cursors.Default;
                }
            }
            else
            {
                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.PasswordDontMatch, Properties.Resources.ErrorMessageTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void UpdateControls()
        {
            //_okButton.Enabled = (_newPasswordTextBox.Text.Trim().Length > 0 && _confirmPasswordTextBox.Text.Trim().Length > 0);
        }

        private void _newPasswordTextBox_TextChanged(object sender, System.EventArgs e)
        {
            UpdateControls();
        }

        private void _confirmPasswordTextBox_TextChanged(object sender, System.EventArgs e)
        {
            UpdateControls();
        }
    }
}
