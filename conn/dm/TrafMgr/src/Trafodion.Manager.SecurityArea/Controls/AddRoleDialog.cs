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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class AddRoleDialog : TrafodionForm
    {
        bool _roleGotAdded = false;
        private ConnectionDefinition _connectionDefinition;

        public bool RoleGotAdded
        {
            get { return _roleGotAdded; }
        }

        public AddRoleDialog(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            this.CenterToParent();
        }

        private void _roleNameTextBox_TextChanged(object sender, EventArgs e)
        {
            _okButton.Enabled = _roleNameTextBox.Text.Trim().Length > 0;
        }

        private void _okButton_Click(object sender, EventArgs e)
        {
            _roleGotAdded = false;
            bool addFailed = false;

            string[] roleNames = _roleNameTextBox.Text.Trim().Split(new string[] { ","}, StringSplitOptions.RemoveEmptyEntries);
            if (roleNames.Length < 1)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), Properties.Resources.NoValidRoleNamesErrorText, 
                    global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);

                return;
            }
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add("Role Name");
            dataTable.Columns.Add("Message");

            foreach (string roleName in roleNames)
            {
                if (roleName.Length > ConnectionDefinition.ROLE_NAME_MAX_LENGTH)
                {
                    dataTable.Rows.Add(new string[] { roleName, string.Format(Properties.Resources.RoleNameExceedsLengthErrorText,ConnectionDefinition.ROLE_NAME_MAX_LENGTH)});
                }
            }

            if (dataTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.OneOrMorInvalidRoleNames, dataTable, System.Drawing.SystemIcons.Error);
                mmd.ShowDialog();
                return;
            }

            dataTable.Rows.Clear();

            Cursor = Cursors.WaitCursor;

            foreach (string roleName in roleNames)
            {
                Role role = new Role(roleName, _connectionDefinition);
                try
                {
                    role.Add();
                    _roleGotAdded = true;
                    dataTable.Rows.Add(roleName, Properties.Resources.RoleAddSuccessText);
                }
                catch (Exception ex)
                {
                    addFailed = true;
                    dataTable.Rows.Add(roleName, ex.Message);
                }
            }
            Cursor = Cursors.Default;

            if (addFailed)
            {
                if (dataTable.Rows.Count > 1)
                {
                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.RoleAddMultipleFailureText, dataTable,
                                                                            System.Drawing.SystemIcons.Error);
                    mmd.ShowDialog();
                }
                else
                {
                    if (dataTable.Rows.Count == 1)
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                            string.Format(Properties.Resources.RoleAddOneFailureText, dataTable.Rows[0].ItemArray[0], dataTable.Rows[0].ItemArray[1]),
                            Properties.Resources.ErrorMessageTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }

            if (_roleGotAdded)
            {
                DialogResult = DialogResult.OK;
            }
        }
    }
}
