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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UserManagement.Controls.Tree;
using Trafodion.Manager.UserManagement.Model;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class UserMgmtRightPaneControl : TrafodionRightPaneControl
    {
        private DatabaseUsersUserControl _databaseUsersUserControl = null;
        private DefaultRoleUserControl _defaultRoleUserControl = null;
        private RolesUserControl _rolesUserControl = null;
        private TrafodionTextBox _nonAdminUserErrorBox;

        #region public properties

        public UserMgmtNavigator TheUserMgmtNavigator
        {
            get { return TheNavigationUserControl as UserMgmtNavigator; }
        }

        #endregion public properties

        public UserMgmtRightPaneControl()
            :base()
        {
            InitializeComponent();
        }

        public UserMgmtRightPaneControl(UserMgmtNavigator aUserMgmtNavigator)
            : base(aUserMgmtNavigator)
        {
            InitializeComponent();
        }


        public override void HandleNavigationTreeNodeSelect(NavigationTreeNode aNavigationTreeNode)
        {
            try
            {
                ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;

                //else // The following all simply use a TrafodionTabControl instead of a derived one
                {
                    if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                    {
                        AddControl(new FixSystemUserControl(theConnectionDefinition));
                    }
                    else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                    {
                        AddControl(new MySystemsUserControl(true));
                    }
                    else if (aNavigationTreeNode is SystemConnectionFolder)
                    {
                            if (_defaultRoleUserControl == null)
                            {
                            _defaultRoleUserControl = new DefaultRoleUserControl(theConnectionDefinition);
                            }
                            else
                            {
                                _defaultRoleUserControl.ConnectionDefinition = theConnectionDefinition;
                                _defaultRoleUserControl.Refresh();
                            }
                        AddControl(_defaultRoleUserControl);
                        if (!aNavigationTreeNode.IsExpanded)
                        {
                            aNavigationTreeNode.ExpandAll();
                        }
                    }
                    else if (aNavigationTreeNode is DatabaseUsersFolder)
                    {
                        if (UserMgmtSystemModel.FindSystemModel(theConnectionDefinition).IsAdminUser)
                        {

                            if (_databaseUsersUserControl == null)
                            {
                                _databaseUsersUserControl = new DatabaseUsersUserControl(theConnectionDefinition);
                            }
                            else
                            {
                                _databaseUsersUserControl.ConnectionDefn = theConnectionDefinition;
                                _databaseUsersUserControl.Refresh();
                            }
                            AddControl(_databaseUsersUserControl);
                        }
                        else
                        {
                            if (_nonAdminUserErrorBox == null)
                            {
                                _nonAdminUserErrorBox = new TrafodionTextBox();
                                _nonAdminUserErrorBox.Multiline = true;
                                _nonAdminUserErrorBox.Dock = System.Windows.Forms.DockStyle.Fill;
                                _nonAdminUserErrorBox.ReadOnly = true;
                                _nonAdminUserErrorBox.Text = Properties.Resources.NonAdminUserErrorText;
                            }
                            AddControl(_nonAdminUserErrorBox);

                        }
                    }
                    else if (aNavigationTreeNode is RolesFolder)
                    {
                        if (UserMgmtSystemModel.FindSystemModel(theConnectionDefinition).IsAdminUser)
                        {

                            if (_rolesUserControl == null)
                            {
                                _rolesUserControl = new RolesUserControl(theConnectionDefinition);
                            }
                            else
                            {
                                _rolesUserControl.ConnectionDefn = theConnectionDefinition;
                                _rolesUserControl.Refresh();
                            }
                            AddControl(_rolesUserControl);
                        }
                        else
                        {
                            if (_nonAdminUserErrorBox == null)
                            {
                                _nonAdminUserErrorBox = new TrafodionTextBox();
                                _nonAdminUserErrorBox.Multiline = true;
                                _nonAdminUserErrorBox.Dock = System.Windows.Forms.DockStyle.Fill;
                                _nonAdminUserErrorBox.ReadOnly = true;
                                _nonAdminUserErrorBox.Text = Properties.Resources.NonAdminUserErrorText;
                            }
                            AddControl(_nonAdminUserErrorBox);

                        }
                    }
                }
            }
            catch (Exception anException)
            {
                AddControl(anException);
            }
        }

        public override TrafodionRightPaneControl DoClone()
        {
            return new UserMgmtRightPaneControl();
        }
    }
}
