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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UserManagement.Model;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class DefaultRoleUserControl : UserControl
    {
        #region Fields

        ConnectionDefinition _theConnectionDefinition = null;
        TrafodionProgressUserControl _progressControl = null;
        private string _originalPrimaryRole = string.Empty;
        #endregion Fields

        #region Properties
        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _theConnectionDefinition;
            }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public String UserName
        {
            get { return _theUserNameTextBox.Text.Trim().ToUpper(); }
            set { _theUserNameTextBox.Text = value; }
        }

        public String PrimaryRole
        {
            get
            {
                if (_thePrimaryRoleCombo.SelectedItem != null)
                {
                    return (string)_thePrimaryRoleCombo.SelectedItem;
                }
                else
                {
                    return string.Empty;
                }
            }
            set
            {
                _thePrimaryRoleCombo.SelectedItem = value;
            }
        }

        #endregion Properties

        #region Constructors
        public DefaultRoleUserControl(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            _theUserInfoGroupBox.Visible = false;
        }
        #endregion

        #region Events

        /// <summary>
        /// Load all roles for the user.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DefaultRoleUserControl_Load(object sender, EventArgs e)
        {
           SetupOverview();     
        }

        /// <summary>
        /// Setting roles combo for the user.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void UserInfoLookup_Completed(object sender, TrafodionProgressCompletedArgs e)
        {
            DataTable dtRoleList;
            if (_progressControl.Error != null)
            {
                _progressPanel.Controls.Clear();
                MessageBox.Show(Utilities.GetForegroundControl(), _progressControl.Error.Message, "Error obtaining user details",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);

                UserName = _theConnectionDefinition.DatabaseUserName;
                return;
            }
            else
            {
                _progressPanel.Controls.Clear();
                dtRoleList = (DataTable)_progressControl.ReturnValue;
            }

            _theUserInfoGroupBox.Visible = true;
            UserName = _theConnectionDefinition.DatabaseUserName;
            _thePrimaryRoleCombo.Items.Clear();
            _thePrimaryRoleCombo.Items.Add(string.Empty);
            if (dtRoleList != null)
            {
                //set the roles combo            
                foreach (DataRow dr in dtRoleList.Rows)
                {
                    _thePrimaryRoleCombo.Items.Add(dr["ROLE_NAME"]);
                }
            }
            string primaryRoleName = "";
            if (dtRoleList != null && dtRoleList.Rows.Count > 0)
            {
                primaryRoleName = dtRoleList.Rows[0]["LOGON_ROLE_ID_NAME"] as string;
                _originalPrimaryRole = primaryRoleName;
            }
            if (primaryRoleName!= null && !primaryRoleName.Equals("NONE"))
            {
                PrimaryRole = primaryRoleName;
            }
            else
            {
                _thePrimaryRoleCombo.SelectedIndex = 0;//Select the first item of "NONE"
            }
            _theChangeRoleButton.Enabled = false;
            _thePrimaryRoleCombo.Enabled = true;
  
            UnregisterProgressEventHandler();
        }
        /// <summary>
        /// Unregister Event.
        /// </summary>
        private void UnregisterProgressEventHandler()
        {
            _progressPanel.Controls.Clear();
            _progressControl.ProgressCompletedEvent -= UserInfoLookup_Completed;
        }

        /// <summary>
        /// Process changing Primary role.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theChangeRoleButton_Click(object sender, EventArgs e)
        {
            
            UserMgmtSystemModel userMgmtSystemModel = UserMgmtSystemModel.FindSystemModel(_theConnectionDefinition);
            try
            {
                bool isSpecifiedRole = ConnectionDefinition.UserSpecifiedRole.Equals(string.Empty) ? false : true;
                if (!isSpecifiedRole && MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.AlterPrimaryRoleWarningMsg, "Change Primary Role",
                        MessageBoxButtons.YesNo,MessageBoxIcon.Warning) != DialogResult.Yes)
                {
                    return;
                }
                string strDefaultRole = PrimaryRole.Equals(string.Empty) ? "NONE" : PrimaryRole;
                DataTable resultsTable = userMgmtSystemModel.SetPrimaryRole(UserName, strDefaultRole);
                int successCount = userMgmtSystemModel.GetSuccessRowCount(resultsTable);

                if (successCount > 0)
                {
                    //reset primary role
                    _originalPrimaryRole = PrimaryRole;
                    _theChangeRoleButton.Enabled = false;
                    MessageBox.Show(Utilities.GetForegroundControl(), string.Format("Primary role changed successfully to {0}", strDefaultRole), "Primary role change successful",
                        MessageBoxButtons.OK);

                    if (!isSpecifiedRole)
                    {
                        // Clear the password for one connection definition
                        string tmp = _theConnectionDefinition.Password;
                        _theConnectionDefinition.ClearPassword();
                        _theConnectionDefinition.Password = tmp;
                        //Reset the primary role
                        _theConnectionDefinition.UserSpecifiedRole = PrimaryRole;
                        _theConnectionDefinition.DoTest(true);                     
                    }
                }
                else
                {
                    string errorMessage = resultsTable.Rows[0][Role.MessageColumName] as string;
                    MessageBox.Show(Utilities.GetForegroundControl(), string.Format("Failed to change Primary role {0}.\n\n{1}", PrimaryRole, errorMessage), "Primary role change  Failed",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), string.Format("Error encountered while saving Primary role - {0}", ex.Message), "Error saving Primary role",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }          

        }
        /// <summary>
        /// Enable change role button once user changing primary role.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _thePrimaryRoleCombo_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (!PrimaryRole.Equals(_originalPrimaryRole))
            {
                _theChangeRoleButton.Enabled = true;
            }
            else
            {
                _theChangeRoleButton.Enabled = false;
            }
        }

        #endregion

        #region Public Function
        public void Refresh()
        {
            if (_theConnectionDefinition != null)
            {
                SetupOverview();
            }
        }
        #endregion

        #region Private Function
        /// <summary>
        /// Getting roles list for the user.
        /// </summary>
        private void SetupOverview()
        {
            _theUserInfoGroupBox.Visible = false;
            UserMgmtSystemModel userMgmtSystemModel = UserMgmtSystemModel.FindSystemModel(_theConnectionDefinition);
            TrafodionProgressArgs args = new TrafodionProgressArgs("Obtaining roles list for the current user.", userMgmtSystemModel, "GetRoleListByUserName", new Object[] { _theConnectionDefinition.DatabaseUserName });

            _progressPanel.Controls.Clear();

            _progressControl = new TrafodionProgressUserControl(args);
            _progressControl.Dock = DockStyle.Top;
            _progressControl.ProgressCompletedEvent += UserInfoLookup_Completed;
            _progressPanel.Controls.Add(_progressControl);
        }
        #endregion

       
    }
}
