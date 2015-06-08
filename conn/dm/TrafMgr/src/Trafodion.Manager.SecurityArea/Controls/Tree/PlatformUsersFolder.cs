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
    class PlatformUsersFolder : NavigationTreeFolder
    {
        public override string LongerDescription
        {
            get { return Properties.Resources.PlatformUsers; }
        }

        public override string ShortDescription
        {
            get { return Properties.Resources.PlatformUsers; }
        }

        public PlatformUsersFolder()
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
            aContextMenuStrip.Items.Add(new ToolStripSeparator());

            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);
        }

        private ToolStripMenuItem GetAddUserMenu(TreeNode node)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem("Add Platform User...");
            menuItem.Tag = node;
            menuItem.Click += new EventHandler(addUserMenuItem_Click);
            return menuItem;
        }

        void addUserMenuItem_Click(object sender, EventArgs e)
        {
            ManageUserDialog dialog = new ManageUserDialog();
            PlatformUserPanel userPanel = new PlatformUserPanel();
            userPanel.OnSuccessImpl += DoRefresh;
            dialog.OnOk += userPanel.AddUser;
            userPanel.Mode = User.EditMode.Create;
            userPanel.ConnectionDefinition = TheConnectionDefinition;
            userPanel.SetupDefaultRoles();
            userPanel.SetupDefaultPolicy();
            dialog.ShowControl(userPanel, "Add Platform User");
            dialog.OnOk -= userPanel.AddUser;
            userPanel.OnSuccessImpl -= DoRefresh;
        }


        private void DoRefresh()
        {
            base.DoRefresh(null);
        }
    }
}
