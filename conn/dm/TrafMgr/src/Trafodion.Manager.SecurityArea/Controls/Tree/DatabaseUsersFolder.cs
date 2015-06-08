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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.SecurityArea.Controls;
using Trafodion.Manager.SecurityArea.Model;

namespace Trafodion.Manager.SecurityArea.Controls.Tree
{
    class DatabaseUsersFolder : NavigationTreeFolder
    {
        public override string LongerDescription
        {
            get { return Properties.Resources.DatabaseUsers; }
        }

        public override string ShortDescription
        {
            get { return Properties.Resources.DatabaseUsers; }
        }

        public DatabaseUsersFolder()
        {
            Text = LongerDescription;
            Nodes.Clear();
        }

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        /// <summary>
        /// Add context munu according to logon roles.
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        override public void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            aContextMenuStrip.Items.Add(GetAddUserMenu(this));
            //aContextMenuStrip.Items.Add(GetAddLikeUserMenu(this));
            aContextMenuStrip.Items.Add(GetAddMultipleUserMenu(this));
            //aContextMenuStrip.Items.Add(GrantRevokeRoleUserMenu(this));
            
            aContextMenuStrip.Items.Add(new ToolStripSeparator());

            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);
        }

        private ToolStripMenuItem GetAddUserMenu(TreeNode node)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem("Add Database User...");
            menuItem.Tag = node;
            menuItem.Click += new EventHandler(addUserMenuItem_Click);
            return menuItem;
        }

        void addUserMenuItem_Click(object sender, EventArgs e)
        {
            ManageUserDialog dialog = new ManageUserDialog();
            DatabaseUserPanel dbUserPanel = new DatabaseUserPanel();
            dbUserPanel.Mode = User.EditMode.Create;
            dbUserPanel.OnSuccessImpl += DoRefresh;
            dialog.OnOk += dbUserPanel.AddUser;
            dbUserPanel.ConnectionDefinition = TheConnectionDefinition;
            dbUserPanel.SetupDefaultRoles();
            dbUserPanel.LookUpDefaultPolicies();
            dialog.ShowControl(dbUserPanel, "Add Database User");
            dialog.OnOk -= dbUserPanel.AddUser;
            dbUserPanel.OnSuccessImpl -= DoRefresh;

        }

        private ToolStripMenuItem GetAddMultipleUserMenu(TreeNode node)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem("Add Multiple Database Users...");
            menuItem.Tag = node;
            menuItem.Click += new EventHandler(addMultipleUserMenuItem_Click);
            return menuItem;
        }

        void addMultipleUserMenuItem_Click(object sender, EventArgs e)
        {
            AddUsersPanel userPanel = new AddUsersPanel();
            ManageUserDialog dialog = new ManageUserDialog();
            userPanel.OnSuccessImpl += DoRefresh;
            dialog.OnOk += userPanel.AddUsers;
            userPanel.ConnectionDefinition = TheConnectionDefinition;
            userPanel.SetupDefaultRoles();
            dialog.ShowControl(userPanel, "Add Multiple Database Users");
            dialog.OnOk -= userPanel.AddUsers;
            userPanel.OnSuccessImpl -= DoRefresh;
        }

        private void DoRefresh()
        {
            base.DoRefresh(null);
        }

    }
}
