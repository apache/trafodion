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
    public partial class AlterUserRole : UserControl
    {
        #region Member Variables
        User _User = null;
        ConnectionDefinition _theConnectionDefinition;
        User.EditMode _Mode = User.EditMode.Create;
        #endregion

        #region Constructors
        public AlterUserRole()
        {
            InitializeComponent();
            this._theRoleLabel.Text = "Default Role";
            this._theNameLabel.Text = "User Name";
        }

        
        public AlterUserRole(ConnectionDefinition aConnectionDefinition, User aUser) : this()
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
                    _theRoleCombo.Items.Add(role);
                }
                _theRoleCombo.SelectedItem = role;
            }
        }


        public User.EditMode Mode
        {
            get { return _Mode; }
            set
            {
                _Mode = value;
                _theUserNameText.ReadOnly = (_Mode == User.EditMode.Update);
                _theRoleCombo.Enabled = (_Mode == User.EditMode.Update);
            }
        }
        #endregion

        #region Public methods
        public bool UpdateUserDefaultRole()
        {
            User user = new User(_User);
            
            //Populate the user object
            PopulateUserFromUI(user);

            try
            {
                user.SetDefaultRole(user.UserName, user.DefaultRole);
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(this.ParentForm, string.Format("Set default role failed for user {0}", _User.UserName),
                    "Set default role failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return false;
            }
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
