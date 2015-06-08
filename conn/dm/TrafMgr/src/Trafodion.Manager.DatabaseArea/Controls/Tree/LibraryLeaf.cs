//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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

using Trafodion.Manager.DatabaseArea.Model;
using System.Windows.Forms;
using System;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
	/// <summary>
    /// Node class for library that won't contains jar classes.
	/// </summary>
    public class LibraryLeaf : DatabaseTreeNode
	{
        public LibraryLeaf(TrafodionLibrary sqlMxLibrary)
            : base(sqlMxLibrary)
		{
            ImageKey = DatabaseTreeView.DB_LIBRARY_ICON;
            SelectedImageKey = DatabaseTreeView.DB_LIBRARY_ICON;
        }

        public TrafodionLibrary TrafodionLibrary
		{
            get { return (TrafodionLibrary)this.TrafodionObject; }

		}

        override public string LongerDescription
		{
			get
			{
                return Properties.Resources.Library+" " + TrafodionLibrary.VisibleAnsiName;
			}
		}

        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            if (TheConnectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS", "CREATE_LIBRARY"))
            {
                if (this.Parent != null)
                {
                    if (this.Parent.Parent != null && this.Parent.Parent is SchemaFolder)
                    {
                        aContextMenuStrip.Items.Add(((SchemaFolder)this.Parent.Parent).GetShowBrowseLibraryToolMenuItem(this));
                    }
                }

                ToolStripMenuItem alterLibraryMenuItem = new ToolStripMenuItem(Properties.Resources.AlterLibrary + "...");
                alterLibraryMenuItem.Tag = this;
                alterLibraryMenuItem.Click += new System.EventHandler(alterLibraryMenuItem_Click);

                aContextMenuStrip.Items.Add(alterLibraryMenuItem);

                ToolStripMenuItem dropLibraryMenuItem = new ToolStripMenuItem(Properties.Resources.DropLibrary);
                dropLibraryMenuItem.Tag = this;
                dropLibraryMenuItem.Click += new System.EventHandler(dropLibraryMenuItem_Click);

                aContextMenuStrip.Items.Add(dropLibraryMenuItem);
            }
            
        }

        /// <summary>
        /// Event handler for the alter Library menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void alterLibraryMenuItem_Click(object sender, EventArgs e)
        {
            //"You cannot alter a system library."
            if (this.TrafodionLibrary.IsMetadataObject)
            {
                MessageBox.Show(Properties.Resources.CannotAlterLibrary, Properties.Resources.Warning, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                CreateLibraryUserControl cld = new CreateLibraryUserControl(this.TrafodionLibrary);
                cld.ShowDialog();
            }
        }

        /// <summary>
        /// Drop library
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void dropLibraryMenuItem_Click(object sender, System.EventArgs e)
        {
            try
            {
                DropConfirmDialog dlg = new DropConfirmDialog(Properties.Resources.DropLibrary,
                    string.Format(Properties.Resources.DropLibraryConfirm,this.TrafodionLibrary.ExternalName),
                    Trafodion.Manager.Properties.Resources.Question,
                    Properties.Resources.DropLibraryOptionDescription, false);
                if (dlg.ShowDialog() == DialogResult.Yes)
                {
                    this.TrafodionLibrary.Drop(dlg.OptionValue);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message,
                    Properties.Resources.DropLibraryError, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            
        }
	}
}
