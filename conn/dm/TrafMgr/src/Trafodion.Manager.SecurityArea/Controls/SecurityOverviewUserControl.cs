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
using Trafodion.Manager.Connections.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class SecurityOverviewUserControl : UserControl
    {
        #region Fields

        ConnectionDefinition _theConnectionDefinition = null;
        SystemSecurity _systemSecurity;
        User _theUser = null;
        TrafodionProgressUserControl _progressControl = null;

        #endregion Fields

        #region Properties

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
        }

        public String UserName
        {
            get { return _theUserNameTextBox.Text.Trim(); }
            set { _theUserNameTextBox.Text = value; }
        }

        public String DefaultRole
        {
            get { return (string)_theDefaultRoleCombo.SelectedItem; }
            set 
            {
                if (! _theDefaultRoleCombo.Items.Contains(value))
                {
                    _theDefaultRoleCombo.Items.Add(value);
                }
                _theDefaultRoleCombo.SelectedItem = value;
            }
        }

        #endregion Properties

        #region Constructors

        public SecurityOverviewUserControl(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            _theUserInfoGroupBox.Visible = false;
        }

        #endregion Constructors

        void SetupOverview()
        {
            User tempUser = new User(ConnectionDefinition, ConnectionDefinition.UserName);
            TrafodionProgressArgs args = new TrafodionProgressArgs("Obtaining user information", tempUser, "GetUserWithDetails", new Object[] { ConnectionDefinition.UserName, false });
            
            //Clear the config privileges lookup control and event handler
            _progressPanel.Controls.Clear();
            if (_progressControl != null)
            {
                _progressControl.ProgressCompletedEvent -= ConfigPrivilegesLookup_Completed;
            }

            _progressControl = new TrafodionProgressUserControl(args);
            _progressControl.ProgressCompletedEvent += UserInfoLookup_Completed;
            _progressPanel.Controls.Add(_progressControl);
        }


        void UserInfoLookup_Completed(object sender, TrafodionProgressCompletedArgs e)
        {
            if (_progressControl.Error != null)
            {
                _progressPanel.Controls.Clear();
                MessageBox.Show(Utilities.GetForegroundControl(), _progressControl.Error.Message, "Error obtaining user details",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                _theChangeRoleButton.Enabled = _theChangePasswordButton.Enabled = _theDefaultRoleCombo.Enabled = false;
                UserName = ConnectionDefinition.UserName;
                return;
            }
            else
            {
                _progressPanel.Controls.Clear();
                _theUser = (User)_progressControl.ReturnValue;
            }

            _theUserInfoGroupBox.Visible = true;

            //set the roles combo
            _theDefaultRoleCombo.Items.Add(_theUser.DefaultRole);
            foreach (string role in _theUser.AdditionalRoles)
            {
                _theDefaultRoleCombo.Items.Add(role);
            }
            UserName = ConnectionDefinition.UserName;
            DefaultRole = _theUser.DefaultRole;
            _theChangeRoleButton.Enabled = (((_theUser.UserType == User.UserTypeEnum.DBUser) || (_theUser.UserType == User.UserTypeEnum.PlatformDBUser)) && (_theUser.AdditionalRoles.Count > 0));
            _theDefaultRoleCombo.Enabled = _theChangeRoleButton.Enabled;
            _theChangePasswordButton.Enabled = ((_theUser.UserType == User.UserTypeEnum.PlatformUser) || (_theUser.UserType == User.UserTypeEnum.PlatformDBUser));

            UnregisterProgressEventHandler();
        }

        private void UnregisterProgressEventHandler()
        {
            _progressPanel.Controls.Clear();
            _progressControl.ProgressCompletedEvent -= UserInfoLookup_Completed;
        }

        private void _theChangeRoleButton_Click(object sender, EventArgs e)
        {
            User aUser = new User(ConnectionDefinition, ConnectionDefinition.UserName);
            try
            {
                aUser.SetDefaultRole(_theUser.UserName, DefaultRole);
                MessageBox.Show(Utilities.GetForegroundControl(), string.Format("Default role changed successfully to {0}", DefaultRole), "Default role change successful",
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), string.Format("Error encountered while saving default role - {0}", ex.Message), "Error saving default role",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }          

        }

        private void _theChangePasswordButton_Click(object sender, EventArgs e)
        {
            ChangePasswordDialog dialog = new ChangePasswordDialog(ConnectionDefinition, true);
            dialog.ShowDialog();
        }

        private void _theRefreshButton_Click(object sender, EventArgs e)
        {
            Refresh();
        }

        private void Refresh()
        {

        }

        private void SecurityOverviewUserControl_Load(object sender, EventArgs e)
        {
            //if (_theConnectionDefinition.ServerVersion == ConnectionDefinition.SERVER_VERSION.R25)
            //{
                _systemSecurity = SystemSecurity.FindSystemModel(_theConnectionDefinition);
                if (!_systemSecurity.IsSecPrivilegesLoaded)
                {
                    _theUserInfoGroupBox.Visible = false;
                    TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.LookupSecConfigPrivProgressText, _systemSecurity, "LoadSecConfigPrivileges", new Object[0]);
                    _progressControl = new TrafodionProgressUserControl(args);
                    _progressControl.ProgressCompletedEvent += ConfigPrivilegesLookup_Completed;
                    _progressPanel.Controls.Clear();
                    _progressPanel.Controls.Add(_progressControl);
                }
                else
                {
                    SetupOverview();
                }
            //}
            //else
            //{
            //    SecurityShowExceptionUserControl exceptionControl = new SecurityShowExceptionUserControl(Properties.Resources.SecurityFunctionsOnlyOnR25Text);
            //    Controls.Clear();
            //    exceptionControl.Dock = DockStyle.Fill;
            //    Controls.Add(exceptionControl);
            //}
        }

        void ConfigPrivilegesLookup_Completed(object sender, TrafodionProgressCompletedArgs e)
        {
            if (_progressControl.Error != null)
            {
                SecurityShowExceptionUserControl exceptionControl = new SecurityShowExceptionUserControl(Properties.Resources.ErrorLoadingSecurityConfigPrivileges + Environment.NewLine + Environment.NewLine + _progressControl.Error.Message);
                Controls.Clear();
                exceptionControl.Dock = DockStyle.Fill;
                Controls.Add(exceptionControl);
            }
            else
            {
                _progressPanel.Controls.Clear();
                _systemSecurity.FirePrivilegesLoadedEvent();
                SetupOverview();
            }
        }
    }
}
