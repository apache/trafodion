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
using System.Windows.Forms;
using System;

namespace Trafodion.Manager.SecurityArea.Controls.Tree
{
    class DirectoryServerFolder : NavigationTreeFolder
    {
        public override string LongerDescription
        {
            get { return Properties.Resources.DirectoryServers; }
        }

        public override string ShortDescription
        {
            get { return Properties.Resources.DirectoryServers; }
        }

        public DirectoryServerFolder()
        {
            Text = LongerDescription;
            Nodes.Clear();
        }

        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            ToolStripMenuItem theAddServerMenuItem = new ToolStripMenuItem(Properties.Resources.AddServer);
            theAddServerMenuItem.Click += new EventHandler(theAddServerMenuItem_Click);
            aContextMenuStrip.Items.Add(theAddServerMenuItem);
        }

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        private void theAddServerMenuItem_Click(object sender, EventArgs e)
        {
            ConfigureDirectoryServerDialog theDialog = new ConfigureDirectoryServerDialog(this.TheConnectionDefinition, false);
            if (theDialog.ShowDialog() == DialogResult.Yes)
            {
                this.DoRefresh(null);
            }
        }
    }
}
