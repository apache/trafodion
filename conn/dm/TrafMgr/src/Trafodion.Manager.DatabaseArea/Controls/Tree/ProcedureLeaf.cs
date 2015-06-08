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

using Trafodion.Manager.DatabaseArea.Model;
using System.Windows.Forms;
using System;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
	/// <summary>
	/// Summary description for ProcedureLeaf.
	/// </summary>
    public class ProcedureLeaf : DatabaseTreeNode
	{
		public ProcedureLeaf(TrafodionProcedure aTrafodionProcedure)
            :base(aTrafodionProcedure)
		{
            ImageKey = DatabaseTreeView.DB_SPJ_ICON;
            SelectedImageKey = DatabaseTreeView.DB_SPJ_ICON;
        }

        public TrafodionProcedure TrafodionProcedure
		{
            get { return (TrafodionProcedure)this.TrafodionObject; }

		}

        override public string LongerDescription
		{
			get
			{
                return "Procedure " + TrafodionProcedure.VisibleAnsiName;
			}
		}

        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            ToolStripMenuItem dropProcedureMenuItem = new ToolStripMenuItem(Properties.Resources.DropProcedure);
            dropProcedureMenuItem.Tag = this;
            dropProcedureMenuItem.Click += new System.EventHandler(dropProcedureMenuItem_Click);

            aContextMenuStrip.Items.Add(dropProcedureMenuItem);
            
        }

        void dropProcedureMenuItem_Click(object sender, System.EventArgs e)
        {
            if (MessageBox.Show(Utilities.GetForegroundControl(), String.Format(Properties.Resources.DropProcedureConfirm, TrafodionProcedure.ExternalName), Properties.Resources.Confirm, MessageBoxButtons.YesNo) != DialogResult.Yes)
            {
                return;
            }
            try
            {
                this.TrafodionProcedure.Drop();
            }
            catch (Exception ex)
            {
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message,
                    Properties.Resources.DropProcedureError, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
	}
}
