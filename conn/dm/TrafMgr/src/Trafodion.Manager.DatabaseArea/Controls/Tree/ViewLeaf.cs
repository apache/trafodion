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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using System.Windows.Forms;
using System;
using Trafodion.Manager.Framework;
using System.Collections.Generic;
using System.Data;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for ViewLeaf.
    /// </summary>
    public class ViewLeaf : DatabaseTreeNode
    {
        public ViewLeaf(TrafodionView aTrafodionView)
            : base(aTrafodionView)
        {
            ImageKey = DatabaseTreeView.DB_VIEW_ICON;
            SelectedImageKey = DatabaseTreeView.DB_VIEW_ICON;
        }

        public TrafodionView TrafodionView
        {
            get { return (TrafodionView)this.TrafodionObject; }
        }

        override public string LongerDescription
        {
            get
            {
                return "View " + TrafodionView.VisibleAnsiName;
            }
        }

        /// <summary>
        /// Add ValidateView menu to the context menu list
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);
            if (this.TheConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                aContextMenuStrip.Items.Add(GetValidateViewMenuItem(this));
            }
        }

        /// <summary>
        /// Static method to create a Validate View context menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public ToolStripMenuItem GetValidateViewMenuItem(TreeNode node)
        {
            ToolStripMenuItem validateViewMenuItem = new ToolStripMenuItem(Properties.Resources.ValidateView);
            validateViewMenuItem.Tag = node;
            validateViewMenuItem.Click += new EventHandler(validateViewMenuItem_Click);
            if (node != null)
            {
                TrafodionView sqlMxView = ((ViewLeaf)node).TrafodionView;
                validateViewMenuItem.Enabled = (sqlMxView != null && sqlMxView.Valid_Def.Equals("N"));
            }
            return validateViewMenuItem;
        }

        /// <summary>
        /// Event handler for the Validate View menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void validateViewMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionView theTrafodionView = null;
            Object sourceNode = ((ToolStripMenuItem)sender).Tag;
            if (sourceNode is DatabaseTreeNode)
            {
                theTrafodionView = ((DatabaseTreeNode)sourceNode).TrafodionObject as TrafodionView;
            }

            if (theTrafodionView != null)
            {
                try
                {
                    DropConfirmDialog confirmDialog = new DropConfirmDialog(Properties.Resources.ValidateView, Properties.Resources.ValidateViewConfirmation, Trafodion.Manager.Properties.Resources.Question, "Cascade", true, Properties.Resources.ValidateViewConfirmationNote);
                    confirmDialog.ShowDialog();
                    DataTable warningTable = new DataTable();
                    if (confirmDialog.DialogResult == DialogResult.Yes)
                    {
                        bool isCascade = confirmDialog.OptionValue;
                        Object[] parameters = new Object[] { isCascade, theTrafodionView, warningTable };
                        TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.ValidateView, this, "ValidateViewOperation", parameters);                        
                        TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                        progressDialog.ShowDialog();

                        if (progressDialog.Error != null)
                        {
                            MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error on validating view",
                                MessageBoxButtons.OK, MessageBoxIcon.Error);
                        }
                        else
                        {
                            warningTable = progressDialog.ReturnValue as DataTable;
                            int warningCount = warningTable == null ? 0 : warningTable.Rows.Count;
                            if (warningCount == 0)
                            {
                                theTrafodionView.FireChangedEvent();
                                MessageBox.Show("The validation operation has completed.", "Validate View", MessageBoxButtons.OK, MessageBoxIcon.Information);
                            }
                            else 
                            {
                                //Suppose we can get only one warning, for one validation command
                                MessageBox.Show(Utilities.GetForegroundControl(), warningTable.Rows[0].ItemArray[0].ToString(), "Warning on validating view", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                            }
                        } 
                    }
                    
                }
                catch (System.Data.Odbc.OdbcException oe)
                {
                    // Got an ODBC erorr. Show it.
                    MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCException, MessageBoxButtons.OK);
                }
                catch (Exception ex)
                {
                    // Got some other exception.  Show it.
                    MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.OperationFailed, MessageBoxButtons.OK);
                }
            }
            else
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Please select a view", Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

        }

        /// <summary>
        /// The operation to ValidateView, which will be used in TrafodionProgressDialog
        /// </summary>
        /// <param name="isCascade"></param>
        /// <param name="theTrafodionView"></param>
        public DataTable ValidateViewOperation(bool isCascade, TrafodionView theTrafodionView, DataTable warningDataTable)
        { 
            theTrafodionView.ValidateView(isCascade, out warningDataTable);
            return warningDataTable;
        }

        /// <summary>
        /// Handles the View Validation events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        override public void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {
            if (e.EventId == TrafodionObject.ChangeEvent.ViewValidated && this.Parent is ViewsFolder)
            {
                ((ViewsFolder)Parent).TrafodionObject_ModelChangedEvent(sender, e);
            }
        }
    }
}
