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
using System.Windows.Forms;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.UserManagement.Controls.Tree
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
 
            aContextMenuStrip.Items.Add(new ToolStripSeparator());

            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            //Remove refresh option from context menu. refresh is handled in the database users widget 
            foreach (ToolStripItem item in aContextMenuStrip.Items)
            {
                if (item.Text.Equals(global::Trafodion.Manager.Properties.Resources.RefreshMenuText))
                {
                    aContextMenuStrip.Items.Remove(item);
                    break;
                }
            }
        }

        private ToolStripMenuItem GetAddUserMenu(TreeNode node)
        {
            ToolStripMenuItem menuItem = new ToolStripMenuItem("Register ...");
            menuItem.Tag = node;
            menuItem.Click += new EventHandler(addUserMenuItem_Click);
            return menuItem;
        }

        void addUserMenuItem_Click(object sender, EventArgs e)
        {
            RegisterUserUserControl registerUserControl = new RegisterUserUserControl(this.TheConnectionDefinition);
            registerUserControl.OnSuccessImpl += DoRefresh;

            Utilities.LaunchManagedWindow("Register User(s)", registerUserControl, this.TheConnectionDefinition, registerUserControl.Size, true);

        }

        private void DoRefresh()
        {
            this.DoRefresh(null);
        }

    }
}
