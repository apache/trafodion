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
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class DatabaseUserPanel : UserControl, IsValidateable
    {
        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        private bool showPlatformPanel = false;
        User _User = null;
        User.EditMode _Mode = User.EditMode.Create;
        ConnectionDefinition _theConnectionDefinition;
        Policies _thePolicies = null;

        public DatabaseUserPanel()
        {
            InitializeComponent();            
            _theUserNameLabel.Text = "User Name";
            _setUI();
            _theUserRolePanel.Mode = Mode;
            _theUserNameText.MaxLength = ConnectionDefinition.USER_NAME_MAX_LENGTH;
        }


        public DatabaseUserPanel(User aUser, User.EditMode mode) : this()
        {
            User = aUser;
            Mode = mode;
            showPlatformPanel = ((aUser.UserType == User.UserTypeEnum.PlatformDBUser) || (aUser.UserType == User.UserTypeEnum.PlatformUser));
            //_setUI();
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
                if (_theUserRolePanel != null)
                {
                    _theUserRolePanel.ConnectionDefinition = _theConnectionDefinition;
                }
            }
        }
        public void SetupDefaultRoles()
        {
            User tempUser = new User(_theConnectionDefinition);
            Object[] parameters = new Object[] { _theConnectionDefinition.RoleName, 
                ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike)) ? ("New" + DateTime.Now.Ticks) : User.UserName, 
                ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike)) ? "ADD" : "ALTER"};
            TrafodionProgressArgs args = new TrafodionProgressArgs("Looking up possible default roles for user...", tempUser, "GetRolesForOperation", parameters);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();
            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error obtaining list of default roles",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                List<string> result = (List<string>)progressDialog.ReturnValue;
                _theUserRolePanel.SetDefaultRoles(result);
            }
        }

        public void SetUserFromDB(string aUserName)
        {
            User aUser = new User(_theConnectionDefinition, aUserName);
            User = aUser.GetUser(aUserName);
            showPlatformPanel = ((User.UserType == User.UserTypeEnum.PlatformDBUser) || (User.UserType == User.UserTypeEnum.PlatformUser));
            _setUI();

        }

        public User.EditMode Mode
        {
            get { return _Mode; }
            set 
            { 
                _Mode = value;
                _theUserRolePanel.Mode = _Mode;
                _thePlatformUserProperties.Mode = _Mode;
                _theSelectorPanel.Visible = (_Mode == User.EditMode.Create);
                _theUserNameText.ReadOnly = !((Mode == User.EditMode.Create) || (Mode == User.EditMode.CreateLike));
                if (Mode == User.EditMode.Create)
                {
                    ShowPlatformPanel = SecurityAreaOptions.GetOptions().DefaultAddDatabaseUserLocally;
                }
            }
        }

        public bool ShowPlatformPanel
        {
            get { return showPlatformPanel; }
            set 
            { 
                showPlatformPanel = value;
                _setUI();
            }
        }

        public User User
        {
            get { return _User; }
            set 
            { 
                _User = value;
                PopulateUiFromUser(_User);
            }
        }

        public bool AddUser()
        {
            User user = new User(_theConnectionDefinition);
            LongOperationHandler _theStatusHandler = new LongOperationHandler();
            _theStatusHandler.StatusDialog = new LongOperationStatusDialog("Add User Status", "Add User in Progress...");
            user.OnSecurityBackendOperation += _theStatusHandler.OnSecurityBackendOperation;
            //Populate the user object
            PopulateUserFromUI(user);

            //Validate the user object

            //Add the user
            DataTable status = user.AddUser(user);

            //Close the add status dialog
            user.OnSecurityBackendOperation -= _theStatusHandler.OnSecurityBackendOperation;
            
            //Display the final dialog if needed
            bool allOperationsSuccessful = user.AreAllOperationsSuccessful(status);
            bool addUserOperationsSuccessful = user.AreAllOperationsSuccessful(status, "Add user");
            showMultiMessageDialog("Add user", status, allOperationsSuccessful, addUserOperationsSuccessful);
            
            //Refresh the right pane if needed
            if (addUserOperationsSuccessful)
            {
                if (OnSuccessImpl != null)
                {
                    OnSuccessImpl();
                }
            }

            //return status
            return addUserOperationsSuccessful;
        }
 

        public bool UpdateUser()
        {
            User user = new User(_theConnectionDefinition);
            LongOperationHandler _theStatusHandler = new LongOperationHandler();
            _theStatusHandler.StatusDialog = new LongOperationStatusDialog("Alter User Status", "Alter User in Progress...");
            user.OnSecurityBackendOperation += _theStatusHandler.OnSecurityBackendOperation;
            //Populate the user object
            PopulateUserFromUI(user);

            //Validate the user object

            //Alter the user
            DataTable status = user.AlterUser(_User, user);

            //Close the add status dialog
            user.OnSecurityBackendOperation -= _theStatusHandler.OnSecurityBackendOperation;
            
            //Display the final dialog if needed
            bool allOperationsSuccessful = user.AreAllOperationsSuccessful(status);
            bool alterUserOperationsSuccessful = user.AreAllOperationsSuccessful(status, "Alter user");
            showMultiMessageDialog("Alter user", status, allOperationsSuccessful, alterUserOperationsSuccessful);

            //Refresh the right pane if needed
            if (alterUserOperationsSuccessful)
            {
                if (OnSuccessImpl != null)
                {
                    OnSuccessImpl();
                }
            }

            //return status
            return alterUserOperationsSuccessful;
        }


        public  List<string> IsValid()
        {
            List<string> ret = new List<string>();
            if (_theUserNameText.Text.Trim().Length == 0)
            {
                ret.Add("The User Name is required");
            }
            ret.AddRange(_theUserRolePanel.IsValid(_Mode));
            if (_theCreatePlatformUserCheck.Checked)
            {
                ret.AddRange(_thePlatformUserProperties.IsValid(_Mode));
            }

            //return ((_theUserNameText.Text.Trim().Length > 0) 
            //    && (_theUserRolePanel.IsValid(_Mode))
            //    && (_theCreatePlatformUserCheck.Checked ? (_thePlatformUserProperties.IsValid(_Mode)) : true));
            return ret;
        }


        //Helper method to display the multi message dialog
        private void showMultiMessageDialog(string operation, DataTable status, bool allOperationsSuccessful, bool mainOperationSuccess)
        {
            if (! allOperationsSuccessful)
            {
                string message = (mainOperationSuccess) ? "{0} succeeded with errors." : "{0}  failed.";

                TrafodionMultipleMessageDialog multiMessageDialog = new TrafodionMultipleMessageDialog(string.Format(message, operation),
                    status,
                    (mainOperationSuccess) ? System.Drawing.SystemIcons.Warning : System.Drawing.SystemIcons.Error);
                multiMessageDialog.ShowDialog();
            }
        }

        private void PopulateUserFromUI(User aUser)
        {
            aUser.UserName = this._theUserNameText.Text.Trim();
            aUser.UserType = (_theCreatePlatformUserCheck.Checked) ? User.UserTypeEnum.PlatformDBUser : User.UserTypeEnum.DBUser;

            //Populate the roles from the User Roles Panel
            _theUserRolePanel.PopulateUserRoles(aUser);

            //Populate the platform user properties if needed
            if (_thePlatformUserProperties.Visible)
            {
                _thePlatformUserProperties.PopulatePlatformUserProperties(aUser);
            }            
        }

        private void PopulateUiFromUser(User aUser)
        {
            this._theUserNameText.Text = aUser.UserName.Trim();
            
            _theUserRolePanel.PopulateUiFromUser(aUser);

            _thePlatformUserProperties.PopulateUiFromUser(aUser);
        }

        private void _setUI()
        {
            _theSelectorPanel.Visible = (_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike);
            _theCreatePlatformUserCheck.Checked = showPlatformPanel;
            _thePlatformUserProperties.Visible = showPlatformPanel;
            _thePlatformUserProperties.ShowAdditionalProperties(false);
        }

        private void _theCreatePlatformUserCheck_Click(object sender, EventArgs e)
        {
            showPlatformPanel = _theCreatePlatformUserCheck.Checked;
            LookUpDefaultPolicies();
        }

        public void LookUpDefaultPolicies()
        {
            if (_theCreatePlatformUserCheck.Checked)
            {
                _thePlatformUserProperties.SetDefaultPolicyValues(_theConnectionDefinition);
            }
            _setUI();

        }

    }
}
