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
using System.Linq;
using System.Text;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UserManagement.Controls.Tree
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
            aContextMenuStrip.Items.Add(new ToolStripSeparator()); 
            base.AddToContextMenu(aContextMenuStrip);

            //Remove refresh option from context menu. refresh is handled in the roles widget 
            foreach(ToolStripItem item in aContextMenuStrip.Items)
            {
                if (item.Text.Equals(global::Trafodion.Manager.Properties.Resources.RefreshMenuText))
                {
                    aContextMenuStrip.Items.Remove(item);
                    break;
                }
            }
        }

        private ToolStripMenuItem GetAddRoleMenu(TreeNode node)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem("Create ...");
            menuItem.Tag = node;
            menuItem.Click += new EventHandler(addRoleMenuItem_Click);
            return menuItem;
        }

        void addRoleMenuItem_Click(object sender, EventArgs e)
        {
            CreateRoleUserControl createRoleControl = new CreateRoleUserControl(this.TheConnectionDefinition);

            Utilities.LaunchManagedWindow("Create Role(s)", createRoleControl, this.TheConnectionDefinition, createRoleControl.Size, true);

            createRoleControl.OnSuccessImpl += DoRefresh;
           
        }

        private void DoRefresh()
        {
            this.DoRefresh(null);
        }

    }
}
