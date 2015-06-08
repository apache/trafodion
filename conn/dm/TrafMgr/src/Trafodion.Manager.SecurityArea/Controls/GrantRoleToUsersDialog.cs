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
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class GrantRoleToUsersDialog : TrafodionForm
    {
        TrafodionChangeTracker _theChangeTracker;
        UserSelectionPanel userSelectionPanel;
        ConnectionDefinition _connectionDefinition;
        string _roleName;
        bool _roleGranted = false;

        public bool RoleGranted
        {
            get { return _roleGranted; }
        }

        public GrantRoleToUsersDialog(string roleName, ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _roleName = roleName;
            _connectionDefinition = aConnectionDefinition;
            userSelectionPanel = new UserSelectionPanel(roleName, aConnectionDefinition);
            userSelectionPanel.Dock = DockStyle.Fill;
            _usersPanel.Controls.Add(userSelectionPanel);
            _roleNameTextBox.Text = roleName;
            CenterToParent();
            //Add a tracker to track changes
            AddChangeTracker();
        }

        public void DoValidate()
        {
            IsValidateable validateable = userSelectionPanel as IsValidateable;

            if (validateable != null)
            {
                List<string> messages = validateable.IsValid();
                _grantButton.Enabled = (messages.Count == 0);
            }
            else
            {
                _grantButton.Enabled = true;
            }
        }

        private void AddChangeTracker()
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
            if (userSelectionPanel != null)
            {
                _theChangeTracker = new TrafodionChangeTracker(userSelectionPanel);
                _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(_theChangeTracker_OnChangeDetected);
                _theChangeTracker.EnableChangeEvents = true;
            }
        }

        private void _theChangeTracker_OnChangeDetected(object sender, EventArgs e)
        {
            DoValidate();
        }

        private void _grantButton_Click(object sender, EventArgs e)
        {
            Role role = new Role(_roleName, _connectionDefinition);

            TrafodionProgressArgs args = new TrafodionProgressArgs("Granting role to selected users...", role, "GrantMultiple", new Object[1] { userSelectionPanel.AdditionalUsers});
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();

            if (progressDialog.Error == null && progressDialog.ReturnValue is DataTable)
            {
                DataTable errorTable = (DataTable)progressDialog.ReturnValue;
                if (errorTable.Rows.Count > 0)
                {
                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.DeleteRoleErrorHeaderText, errorTable, System.Drawing.SystemIcons.Warning);
                    mmd.ShowDialog();
                }
                if (errorTable.Rows.Count != userSelectionPanel.AdditionalUsers.Count)
                {
                    _roleGranted = true;
                }
            }

            if (_roleGranted)
            {
                DialogResult = DialogResult.OK;
                Close();
            }
        }

        private void GrantRoleToUsersDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
        }
    }
}
