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
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    public partial class EditNDCSUserDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {

        #region Private member variables

        private NDCSUser _ndcsUser;

        #endregion Private member variables

        #region Public Properties

        #endregion Public Properties

        public EditNDCSUserDialog(NDCSUser anNDCSUser)
        {
            InitializeComponent();
            _ndcsUser = anNDCSUser;
            Text = Properties.Resources.GrantNDCSPrivilege;
            _grantButton.Text = Properties.Resources.GrantButtonText;
            _grantButton.Enabled = false;
            CenterToParent();
        }

        private void roleNameTextBox_TextChanged(object sender, EventArgs e)
        {
            _grantButton.Enabled = (roleNameTextBox.Text.Trim().Length > 0);
        }

        private void _grantButton_Click(object sender, EventArgs e)
        {
            _ndcsUser.Name = roleNameTextBox.Text.Trim();
            _ndcsUser.PrivilegeType = NDCSUser.OPERATOR_PRIVILEGE;
            try
            {
                _ndcsUser.NDCSSystem.AddUser(_ndcsUser);
                DialogResult = DialogResult.OK;
                Close();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, Properties.Resources.FailedGrantPrivilege, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void roleNameTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Enter)
            {
                if (_grantButton.Enabled)
                {
                    _grantButton.PerformClick();
                }
            }
        }
    }
}
