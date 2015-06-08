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
using System.Windows.Forms;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class ManageUserDialog : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        public delegate bool OnOkClicked();
        public event OnOkClicked OnOk;

        TrafodionChangeTracker _theChangeTracker;
        UserControl _theUserControl = null;

        public ManageUserDialog()
        {
            InitializeComponent();
            CenterToParent();
        }

        public void ShowControl(UserControl aUserControl, string aCaption)
        {
            _theUserControl = aUserControl;
            this.Text = aCaption;

            this._theControlPanel.Controls.Clear();
            aUserControl.Dock = DockStyle.Fill;
            this._theControlPanel.Controls.Add(aUserControl);            

            //Add a tracker to track changes
            AddChangeTracker();
            
            //This will ensure that the Ok button is enabled only if the data is valid
            DoValidate();
            CenterToParent();
            ShowDialog(ParentForm);
        }

        public void DoValidate()
        {
            RegisterUserUserControl validateable = _theUserControl as RegisterUserUserControl;

            if (validateable != null)
            {
                List<string> messages = validateable.IsValid();
                if (messages.Count > 0)
                {
                    _theOkButton.Enabled = false;
                    _theMessagePanel.Text = messages[0];
                    _theMessagePanel.Visible = true;
                }
                else
                {
                    _theOkButton.Enabled = true;
                    _theMessagePanel.Text = "";
                    _theMessagePanel.Visible = false;
                }
            }
            else
            {
                _theOkButton.Enabled = true;
                _theMessagePanel.Text = "";
                _theMessagePanel.Visible = false;
            }
        }

        private void AddChangeTracker()
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
            _theChangeTracker = new TrafodionChangeTracker(_theUserControl);
            _theChangeTracker.OnChangeDetected += new TrafodionChangeTracker.ChangeDetected(_theChangeTracker_OnChangeDetected);
            _theChangeTracker.EnableChangeEvents = true;
        }

        private void _theChangeTracker_OnChangeDetected(object sender, EventArgs e)
        {
            DoValidate();
        }


        private void _theOkButton_Click(object sender, EventArgs e)
        {

            if (OnOk != null)
            {
                bool succeeded = OnOk();
                if (succeeded)
                {
                    DialogResult = DialogResult.OK;
                    this.Close();
                }
                else
                {
                    //setting this value to indicate error
                    //DialogResult = DialogResult.Abort;
                    return;
                }
            }
            else
            {
                DialogResult = DialogResult.OK;
            }
            this.Close();
        }

        private void _theCancelButton_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void ManageUserDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (_theChangeTracker != null)
            {
                _theChangeTracker.RemoveChangeHandlers();
            }
        }

        private void theHelpButton_Click(object sender, EventArgs e)
        {
            if (_theUserControl != null && _theUserControl is UserSelectionPanel)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SelectUser);
            }
            else
            if (_theUserControl != null && _theUserControl is RoleSelectionPanel)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SelectRole);
            }
            else
            if(_theUserControl != null && _theUserControl is ComponentPrivilegesSelectionPanel)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SelectCompPrivileges);
            }
            else
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Register_Users_Dialog_Box);
            }
        }
    }
}
