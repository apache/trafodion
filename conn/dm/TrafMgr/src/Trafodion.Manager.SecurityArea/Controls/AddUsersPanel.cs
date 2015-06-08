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
    public partial class AddUsersPanel : UserControl, IsValidateable
    {
        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        ConnectionDefinition _theConnectionDefinition;
        User.EditMode _Mode = User.EditMode.Create;

        public AddUsersPanel()
        {
            InitializeComponent();
            _theUserNameLabel.Text = "User Names";
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


        public bool AddUsers()
        {
            User user = new User(_theConnectionDefinition);
            LongOperationHandler _theStatusHandler = new LongOperationHandler();
            _theStatusHandler.StatusDialog = new LongOperationStatusDialog("Add Users Status", "Add Users in Progress...");
            user.OnSecurityBackendOperation += _theStatusHandler.OnSecurityBackendOperation;

            //Populate the user object
            List<User> users = PopulateUsersFromUI();

            //Validate the user object

            //Add the user
            DataTable status = user.AddMultipleDBUsers(users);

            //Close the add status dialog
            user.OnSecurityBackendOperation -= _theStatusHandler.OnSecurityBackendOperation;

            //Display the final dialog if needed
            bool allOperationsSuccessful = user.AreAllOperationsSuccessful(status);
            int addFailureCount = user.GetOperationFailureCount(status, "Add user");

            //bool addUserOperationsSuccessful = user.AreAllOperationsSuccessful(status, "Add user");
            //If we see the the number of "Add user" failures are less than the number of users we were trying to add,
            //we know that some users have been added successfully.
            bool addUserOperationsSuccessful = (addFailureCount < users.Count);
            showMultiMessageDialog("Add users", status, allOperationsSuccessful, addUserOperationsSuccessful);

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


        public List<string> IsValid()
        {
            List<string> ret = new List<string>();
            if (!(_theUsers.Text.Trim().Length > 0))
            {
                ret.Add("Please provide one or more user names seprarated by comma");
            }
            ret.AddRange(_theUserRolePanel.IsValid(_Mode));
            return ret;
            //return ((_theUsers.Text.Trim().Length > 0)
            //    && (_theUserRolePanel.IsValid(_Mode)));
        }

        private List<User> PopulateUsersFromUI()
        {
            List<User> users = new List<User>();
            string[] userNames = _theUsers.Text.Trim().ToUpper().Split(new char[] { ',' });
            foreach (string name in userNames)
            {
                string userName = name.Trim();
                if (userName.Length > 0)
                {
                    User user = new User(_theConnectionDefinition, userName);
                    user.UserType = User.UserTypeEnum.DBUser;

                    //Populate the roles from the User Roles Panel
                    _theUserRolePanel.PopulateUserRoles(user);

                    users.Add(user);
                }
            }
            return users;
        }
        public void SetupDefaultRoles()
        {
            User tempUser = new User(_theConnectionDefinition);
            Object[] parameters = new Object[] { _theConnectionDefinition.RoleName, 
                "New" + DateTime.Now.Ticks, 
                "ADD"};
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
        //Helper method to display the multi message dialog
        private void showMultiMessageDialog(string operation, DataTable status, bool allOperationsSuccessful, bool mainOperationSuccess)
        {
            if (!allOperationsSuccessful)
            {
                string message = (mainOperationSuccess) ? "{0} succeeded with errors." : "{0} failed.";

                TrafodionMultipleMessageDialog multiMessageDialog = new TrafodionMultipleMessageDialog(string.Format(message, operation),
                    status,
                    (mainOperationSuccess) ? System.Drawing.SystemIcons.Warning : System.Drawing.SystemIcons.Error);
                multiMessageDialog.ShowDialog();
            }
        }

    }
}
