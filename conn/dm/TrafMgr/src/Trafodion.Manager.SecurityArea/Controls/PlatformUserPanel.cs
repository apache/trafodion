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
using Trafodion.Manager.Framework;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class PlatformUserPanel : UserControl, IsValidateable
    {
        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        ConnectionDefinition _theConnectionDefinition;
        User _User = null;
        User.EditMode _Mode = User.EditMode.Create;

        public PlatformUserPanel()
        {
            InitializeComponent();
            _setUI();
        }


        public PlatformUserPanel(User aUser, User.EditMode mode)
            : this()
        {
            User = aUser;
            Mode = mode;
        }

        public User.EditMode Mode
        {
            get { return _Mode; }
            set
            {
                _Mode = value;
                _theUserNameAndRolePanel.Mode = _Mode;
                _thePlatformUserProperties.Mode = _Mode;
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
        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public void SetupDefaultRoles()
        {
            if ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike))
            {
                User tempUser = new User(_theConnectionDefinition);
                Object[] parameters = new Object[] { _theConnectionDefinition.RoleName, 
                 "New" + DateTime.Now.Ticks, "ADDP"};
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
                    _theUserNameAndRolePanel.SetDefaultRoles(result);
                }
            }
        }

        public void SetupDefaultPolicy()
        {
            _thePlatformUserProperties.SetDefaultPolicyValues(_theConnectionDefinition);
        }

        public void SetUserFromDB(string aUserName)
        {
            User aUser = new User(_theConnectionDefinition, aUserName);
            User = aUser.GetUser(aUserName);
            _setUI();

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

            //Add the user
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

        public List<string> IsValid()
        {
            List<string> ret = new List<string>();
            ret.AddRange(_theUserNameAndRolePanel.IsValid(_Mode));
            ret.AddRange(_thePlatformUserProperties.IsValid(_Mode));
            return ret;
            //return ((_theUserNameAndRolePanel.UserName.Trim().Length > 0)
            //    && ((_theUserNameAndRolePanel.Role != null) && (_theUserNameAndRolePanel.Role.Trim().Length > 0))
            //    && (_thePlatformUserProperties.IsValid(_Mode)));
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

        private void PopulateUserFromUI(User aUser)
        {
            aUser.UserName = _theUserNameAndRolePanel.UserName;
            aUser.UserType = User.UserTypeEnum.PlatformUser;
            aUser.DefaultRole = _theUserNameAndRolePanel.Role;
             _thePlatformUserProperties.PopulatePlatformUserProperties(aUser);
        }


        private void PopulateUiFromUser(User aUser)
        {
            _theUserNameAndRolePanel.User = aUser;
            _thePlatformUserProperties.User = aUser;
        }
        private void _setUI()
        {
            _theUserNameAndRolePanel.Mode = Mode;
        }


    }
}
