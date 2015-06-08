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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.SecurityArea.Model;
namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class UserRolePanel : UserControl
    {
        private RoleSelectionPanel roleSelectionPanel = null;
        private RolesIntillisense rolesIntillisense = null;
        ConnectionDefinition _theConnectionDefinition;
        List<string> _additionalRoles = new List<string>();
        string _defaultRole = null;
        User.EditMode _Mode = User.EditMode.Create;

        public UserRolePanel()
        {
            InitializeComponent();
            //rolesIntillisense = new RolesIntillisense(_theAdditionalRoles);
            //this._theAdditionalRolePanel.Controls.Add(rolesIntillisense.TheAutoCompleteListBox);
            _theDefaultRole.MaxLength = ConnectionDefinition.ROLE_NAME_MAX_LENGTH;
        }


        public string DefaultRole
        {
            get { return _theDefaultRole.Text.Trim().ToUpper(); }
            set 
            {
                string currentDefaultRole = (value == null) ? "" : value.Trim().ToUpper();
                if (currentDefaultRole.Length > 0)
                {
                    if (! _theDefaultRole.Items.Contains(currentDefaultRole))
                    {
                        _theDefaultRole.Items.Add(currentDefaultRole);                       
                    }
                    _theDefaultRole.SelectedItem = currentDefaultRole;
                }
                FilterOutNonAllowedRoles();
                _theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
            }
        }

        public List<string> AdditionalRoles
        {
            get 
            {
                _additionalRoles.Clear();
                loadAdditionalRolesFromText();
                return _additionalRoles; 
            }

            set 
            {
                if (value != null)
                {
                    _additionalRoles = new List<string>();
                    foreach (string role in (List<string>)value)
                    {
                        _additionalRoles.Add(role);
                    }
                    _theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
                }                
            }
        }


        private void AddAdditionalRole(string aRole)
        {
            if (_additionalRoles == null)
            {
                _additionalRoles = new List<string>();
            }
            if (aRole != null)
            {
                string role = aRole.Trim().ToUpper();
                if (_additionalRoles.Contains(role))
                {
                    _additionalRoles.Remove(role);
                }
                _additionalRoles.Add(role);
            }
            FilterOutNonAllowedAdditionalRoles();
            //_theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
        }

        public void SetDefaultRoles(List<string> roles)
        {
            string currentDefaultRole = DefaultRole;

            _theDefaultRole.Items.Clear();
            foreach (string role in roles)
            {
                _theDefaultRole.Items.Add(role);                
            }

            //Set the default role
            if ((currentDefaultRole != null) && (currentDefaultRole.Trim().Length > 0))
            {
                if (_theDefaultRole.Items.Contains(currentDefaultRole))
                {
                    _theDefaultRole.SelectedItem = currentDefaultRole;
                }
                else
                {
                    if (_Mode == User.EditMode.Update)
                    {
                        _theDefaultRole.Text = currentDefaultRole;
                    }
                }
            }

            //Filter out any roles that cannot be displayed
            FilterOutNonAllowedRoles();
            _theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
        }

        private void FilterOutNonAllowedRoles()
        {
            //filter invalid default roles
            FilterOutNonAllowedDefaultRole();

            //filter invalid additional roles
            FilterOutNonAllowedAdditionalRoles();
        }
        private void FilterOutNonAllowedDefaultRole()
        {
            if (_theDefaultRole.Items.Count > 0)
            {
                //filter default roles
                string currentDefaultRole = DefaultRole;
                if (_Mode == User.EditMode.CreateLike)
                {
                    if ((currentDefaultRole != null) && (currentDefaultRole.Length > 0))
                    {
                        if (!_theDefaultRole.Items.Contains(currentDefaultRole))
                        {
                            _theDefaultRole.Text = "";
                        }
                    }
                }
            }
        }
        private void FilterOutNonAllowedAdditionalRoles()
        {
            if (_theDefaultRole.Items.Count > 0)
            {
                //Filter additional roles that are not in the list
                if (_Mode == User.EditMode.CreateLike || _Mode == User.EditMode.Create)
                {
                    List<string> filteredAdditionalRoles = new List<string>();
                    foreach (string role in _additionalRoles)
                    {
                        if (_theDefaultRole.Items.Contains(role))
                        {
                            filteredAdditionalRoles.Add(role);
                        }
                    }
                    _additionalRoles = new List<string>();
                    foreach (string role in filteredAdditionalRoles)
                    {
                        _additionalRoles.Add(role);
                    }
                    //_theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
                }
            }
        }

        public User.EditMode Mode
        {
            get { return _Mode; }
            set
            {
                _Mode = value;
                _theDefaultRoleButton.Enabled = ((_Mode == User.EditMode.Create) || (_Mode == User.EditMode.CreateLike));
                if ((_Mode == User.EditMode.Update) || ((_Mode == User.EditMode.GrantRevokeRole)))
                {
                    _theDefaultRole.DropDownStyle = ComboBoxStyle.DropDownList;
                }
                FilterOutNonAllowedRoles();
                _theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
            }
        }

        public bool ShowAdditionalRole
        {
            get { return this._theAdditionalRolePanel.Visible; }
            set 
            { 
                this._theAdditionalRolePanel.Visible = value;
            }
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
                if (roleSelectionPanel != null)
                {
                    roleSelectionPanel.ConnectionDefinition = _theConnectionDefinition;
                }
            }
        }

        public void PopulateUserRoles(User aUser)
            {
            aUser.DefaultRole = this.DefaultRole;
            List<string> additionalRoles = this.AdditionalRoles;
            if (additionalRoles != null)
            {
                foreach (string role in additionalRoles)
                {
                    aUser.AddAdditionalRole(role);
                }
            }
        }

        public void PopulateUiFromUser(User aUser)
        {
            DefaultRole = aUser.DefaultRole;
            AdditionalRoles = aUser.AdditionalRoles;
        }

        public List<string> IsValid(User.EditMode aMode)
        {
            List<string> ret = new List<string>();
            if (_theDefaultRole.Text.Trim().Length == 0)
            {
                ret.Add("The default role is required");
            }

            if (DefaultRole.Length > ConnectionDefinition.ROLE_NAME_MAX_LENGTH)
            {
                ret.Add(string.Format("Default role \"{0}\" cannot be more than {1} characters long", DefaultRole, ConnectionDefinition.ROLE_NAME_MAX_LENGTH));
            }

            List<string> additionalRoles = AdditionalRoles;
            foreach (string additionalRole in additionalRoles)
            {
                if (additionalRole.Length > ConnectionDefinition.ROLE_NAME_MAX_LENGTH)
                {
                    ret.Add(string.Format("Additional role \"{0}\" cannot be more than {1} characters long", additionalRole, ConnectionDefinition.ROLE_NAME_MAX_LENGTH));
                }
            }
            return ret;
        }

        private void loadAdditionalRolesFromText()
        {
            string additionalRoles = _theAdditionalRoles.Text.Trim().ToUpper();
            _additionalRoles.Clear();
            if (additionalRoles.Length > 0)
            {
                string[] splitRoles = additionalRoles.Split(new char[] { ',' });
                foreach (string role in splitRoles)
                {
                    if (role.Trim().Length > 0)
                    {
                        AddAdditionalRole(role.Trim());
                    }
                }
            }
        }

        private void _theDefaultRoleButton_Click(object sender, EventArgs e)
        {
            showRolesDialog(RoleSelectionPanel.RoleSelectionMode.Default);
        }

        private void _theAdditionalRolesButton_Click(object sender, EventArgs e)
        {
            showRolesDialog(RoleSelectionPanel.RoleSelectionMode.Additional);
        }

        private void showRolesDialog(RoleSelectionPanel.RoleSelectionMode aSelectionMode)
        {
            if (roleSelectionPanel == null)
            {
                roleSelectionPanel = new RoleSelectionPanel(_theConnectionDefinition);
            }
            roleSelectionPanel.SelectionMode = aSelectionMode;
            roleSelectionPanel.Mode = Mode;
            roleSelectionPanel.DefaultRole = DefaultRole;
            roleSelectionPanel.AdditionalRoles = AdditionalRoles;

            ManageUserDialog dialog = new ManageUserDialog();
            dialog.ShowControl(roleSelectionPanel, "Select User Role");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (roleSelectionPanel.AdditionalRoles.Count > 0)
                {
                    _theAdditionalRoles.Text = User.GetAdditionalRoleString(roleSelectionPanel.AdditionalRoles);
                }
                if (roleSelectionPanel.DefaultRole.Length > 0)
                {
                    _theDefaultRole.Text = roleSelectionPanel.DefaultRole;
                }
            }
        }

        private void sanitizeRoles()
        {
            _theDefaultRole.Text = _theDefaultRole.Text.Trim().ToUpper();
            string defaultRole = DefaultRole;
            loadAdditionalRolesFromText();
            //if (_additionalRoles.Contains(defaultRole))
            //{
            //    _additionalRoles.Remove(defaultRole);
            //}
            _theAdditionalRoles.Text = User.GetAdditionalRoleString(_additionalRoles);
        }

        private void _theAdditionalRoles_Leave(object sender, EventArgs e)
        {
            sanitizeRoles();
        }

        private void _theDefaultRole_Leave(object sender, EventArgs e)
        {
            sanitizeRoles();
        }
    }
}
