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
using System.Linq;
using System.Data;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class RoleDetailsDialog : TrafodionForm
    {
        ConnectionDefinition _connectionDefinition;
        DataTable _dataTable;
        bool _grantRevokePerformed = false;

        public RoleDetailsDialog(ConnectionDefinition aConnectionDefinition, DataTable aDataTable, int rowIndex)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            _dataTable = aDataTable;
            _roleNamesComboBox.DataSource = aDataTable;
            _roleNamesComboBox.DisplayMember = aDataTable.Columns[0].ColumnName;
            _roleNamesComboBox.SelectedIndex = -1;

            _roleNamesComboBox.SelectedIndexChanged += new EventHandler(_roleNamesComboBox_SelectedIndexChanged);
            _roleNamesComboBox.SelectedIndex = rowIndex;
            _usersForRoleControl.GrantRevokeEvent += new EventHandler(_usersForRoleControl_GrantRevokeEvent);

            CenterToParent();
        }

        void _usersForRoleControl_GrantRevokeEvent(object sender, EventArgs e)
        {
            _grantRevokePerformed = true;
            Cursor = Cursors.WaitCursor;
            int rowIndex = _roleNamesComboBox.SelectedIndex;
            string roleName = _dataTable.Rows[rowIndex].ItemArray[0] as string;
            try
            {
                Role role = new Role(roleName, _connectionDefinition);
                role.GetRoleDetails();
                _grantCountTextBox.Text = role.GrantCount.ToString();
                _defaultRoleCountTextBox.Text = role.DefaultRoleGrantCount.ToString();
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Error fetching role details : " + ex.Message,
                    global::Trafodion.Manager.Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);

            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        private void _roleNamesComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            int rowIndex = _roleNamesComboBox.SelectedIndex;
            string roleName = _dataTable.Rows[rowIndex].ItemArray[0] as string;
            _grantCountTextBox.Text = ((long)_dataTable.Rows[rowIndex].ItemArray[1]).ToString();
            _defaultRoleCountTextBox.Text = ((long)_dataTable.Rows[rowIndex].ItemArray[2]).ToString();
            string ownerName = _dataTable.Rows[rowIndex].ItemArray[3] as string;
            _createdByTextBox.Text = string.IsNullOrEmpty(ownerName) ? "" : ownerName.Trim();
            long createTime = (long)_dataTable.Rows[rowIndex].ItemArray[4];
            _createdTimeTextBox.Text = Trafodion.Manager.Framework.Utilities.FormattedJulianTimestamp(createTime, "Unavailable");
            _usersForRoleControl.InitializeWidget(roleName, _connectionDefinition);
        }

        private void _sqlPrivilegesButton_Click(object sender, EventArgs e)
        {
            // Define the query expression.
            IEnumerable<string> roleNames =
                from DataRow row in _dataTable.Rows
                select row[0] as string;
            roleNames.ToList();

            int rowIndex = _roleNamesComboBox.SelectedIndex;
            RoleSQLPrivilegesUserControl privilegesControl = new RoleSQLPrivilegesUserControl(_connectionDefinition, roleNames.ToList(),_roleNamesComboBox.SelectedIndex);
            WindowsManager.PutInWindow(privilegesControl.Size, privilegesControl, Properties.Resources.RoleSQLPrivileges, _connectionDefinition);
        }

        public bool GrantOrRevokePerformed
        {
            get { return _grantRevokePerformed; }
        }
    }
}
