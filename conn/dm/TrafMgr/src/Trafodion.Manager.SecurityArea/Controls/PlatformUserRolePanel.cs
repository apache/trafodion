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
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class PlatformUserRolePanel : UserControl
    {
        #region Member Variables
        User _User = null;
        ConnectionDefinition _theConnectionDefinition;
        User.EditMode _Mode = User.EditMode.Create;
        #endregion

        #region Constructors
        public PlatformUserRolePanel()
        {
            InitializeComponent();
            this._theRoleLabel.Text = "User Role";
            this._theNameLabel.Text = "User Name";

            _theUserNameText.MaxLength = ConnectionDefinition.USER_NAME_MAX_LENGTH;
            _theRoleCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        }

        
        public PlatformUserRolePanel(ConnectionDefinition aConnectionDefinition, User aUser) : this()
        {
            ConnectionDefinition = aConnectionDefinition;
            User = aUser;
        }
        #endregion

        #region Member variables
        public User User
        {
            get 
            {
                _User.UserName = UserName;
                _User.DefaultRole = Role;
                return _User; 
            }
            set 
            { 
                _User = value;
                UserName = _User.UserName;
                Role = _User.DefaultRole;
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
        public string UserName
        {
            get
            {
                return this._theUserNameText.Text.Trim().ToUpper();
            }
            set
            {
                _theUserNameText.Text = (value != null) ? value.Trim().ToUpper() : "";
            }
        }

        public string Role
        {
            get
            {
                return (string)_theRoleCombo.SelectedItem;
            }

            set
            {
                string role = (value != null) ? value.Trim().ToUpper() : "";
                if (!_theRoleCombo.Items.Contains(role))
                {
                    if (_Mode == User.EditMode.Update)
                    {
                        _theRoleCombo.Items.Add(role);
                        _theRoleCombo.SelectedItem = role;
                    }
                }
                else
                {
                    _theRoleCombo.SelectedItem = role;
                }
            }
        }


        public User.EditMode Mode
        {
            get { return _Mode; }
            set
            {
                _Mode = value;
                _theUserNameText.ReadOnly = (_Mode == User.EditMode.Update);
                _theRoleCombo.Enabled = (_Mode != User.EditMode.Update);
            }
        }
        #endregion

        #region Public methods
        public bool UpdateUser()
        {
            User user = new User(_User);
            
            //Populate the user object
            PopulateUserFromUI(user);

            //Validate the user object

            //Alter the user
            DataTable status = user.AlterUser(_User, user);

            //Display the final dialog if needed
            bool allOperationsSuccessful = user.AreAllOperationsSuccessful(status);

            //Refresh the right pane if needed
            if (allOperationsSuccessful)
            {
            }
            else
            {
                MessageBox.Show(this.ParentForm, string.Format("Alter role failed for user {0}", _User.UserName),
                    "Alter role failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            //return status
            return allOperationsSuccessful;
        }

        public List<string> IsValid(User.EditMode aMode)
        {
            List<string> ret = new List<string>();
            if (UserName.Trim().Length == 0)
            {
                ret.Add("The User name is required");
            }
            if ((Role == null) || (Role.Trim().Length == 0))
            {
                ret.Add("The role is required");
            }
            return ret;
        }

        public void SetDefaultRoles(List<string> roles)
        {
            string currentRole = Role;

            _theRoleCombo.Items.Clear();
            foreach (string role in roles)
            {
                _theRoleCombo.Items.Add(role);
            }

            if ((currentRole != null) && (currentRole.Trim().Length > 0))
            {
                if (_theRoleCombo.Items.Contains(currentRole))
                {
                    _theRoleCombo.SelectedItem = currentRole;
                }
            }
        }
        #endregion

        #region Private methods
        private void PopulateUserFromUI(User aUser)
        {
            aUser.UserName = UserName;
            aUser.DefaultRole = Role;
        }
       #endregion
    }
}
