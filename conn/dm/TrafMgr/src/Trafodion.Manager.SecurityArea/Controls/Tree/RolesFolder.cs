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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using System;

namespace Trafodion.Manager.SecurityArea.Controls.Tree
{
    class RolesFolder : NavigationTreeFolder
    {
        public override string LongerDescription
        {
            get { return Properties.Resources.Roles; }
        }

        public override string ShortDescription
        {
            get { return Properties.Resources.Roles; }
        }

        public RolesFolder()
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

        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            aContextMenuStrip.Items.Add(GetAddRoleMenu(this));
            base.AddToContextMenu(aContextMenuStrip);
        }

        private ToolStripMenuItem GetAddRoleMenu(TreeNode node)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem("Add Role...");
            menuItem.Tag = node;
            menuItem.Click += new EventHandler(addRoleMenuItem_Click);
            return menuItem;
        }

        void addRoleMenuItem_Click(object sender, EventArgs e)
        {
            AddRoleDialog addRoleDialog = new AddRoleDialog(this.TheConnectionDefinition);
            if (addRoleDialog.ShowDialog() == DialogResult.OK)
            {
                if (addRoleDialog.RoleGotAdded)
                {
                    this.DoRefresh(null);
                }
            }
        }
    }
}
