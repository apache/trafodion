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
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using System;
using Trafodion.Manager.Framework;
using System.Data;
using TenTec.Windows.iGridLib;
using System.Drawing;

namespace Trafodion.Manager.DatabaseArea.Controls
{    
    /// <summary>
    /// A panel that displays a list of sql views
    /// </summary>
    public class TrafodionViewListPanel : TrafodionSchemaObjectListPanel<TrafodionView>
    {
        private TrafodionButton validateViewButton = new TrafodionButton();

        public TrafodionViewListPanel(DatabaseObjectsControl databaseObjectsControl, string headerText, TrafodionObject parentTrafodionObject, 
                    List<TrafodionView> sqlMxObjects, string title)
            :base(databaseObjectsControl, headerText, parentTrafodionObject, sqlMxObjects, title)
        {
            
        }

        /// <summary>
        /// Overrides to create a custom grid to hold the list of views
        /// </summary>
        protected override void CreateGrid()
        {
            base.CreateGrid();
            this.grid.SelectionChanged += new EventHandler(Grid_SelectionChanged);
        }

        protected override void AddAdditionalButton(TrafodionIGridButtonsUserControl buttonsControl)
        {
            //Add Validate Button
            validateViewButton.Text = "&Validate View";
            validateViewButton.Name = "btnValidateView";
            validateViewButton.Size = new System.Drawing.Size(111, 23);
            validateViewButton.Dock = DockStyle.Left;
            validateViewButton.Enabled = false;
            validateViewButton.Click += new EventHandler(ValidateViewButton_Click);
            buttonsControl.Controls.Add(validateViewButton);
        }

        protected override void AddGridColumn()
        {
            this.grid.Cols.Add("Name", Properties.Resources.Name);
            this.grid.Cols.Add("Owner", Properties.Resources.Owner);            
            this.grid.Cols.Add("State", Properties.Resources.ValidDef);
            this.grid.Cols.Add("MetadataUID", Properties.Resources.MetadataUID);
            this.grid.Cols.Add("CreationTime", Properties.Resources.CreationTime);
            this.grid.Cols.Add("RedefinitionTime", Properties.Resources.RedefinitionTime);
        }

        protected override object[] ExtractValues(TrafodionSchemaObject sqlMxSchemaObject)
        {
            TrafodionView sqlMxView = (TrafodionView)sqlMxSchemaObject;
            return new object[] 
                {                            
                    sqlMxView.ExternalName, 
                    sqlMxView.Owner,
                    TrafodionView.DisplayValidState(sqlMxView.Valid_Def),
                    sqlMxView.UID, 
                    sqlMxView.FormattedCreateTime(),
                    sqlMxView.FormattedRedefTime()
                };
        }

        private void Grid_SelectionChanged(object sender, EventArgs e) 
        {
            foreach (int selectedRowIndex in this.grid.SelectedRowIndexes)
            {
                TrafodionView sqlMxView = (TrafodionView)this.grid.Rows[selectedRowIndex].Cells[0].AuxValue;
                if (sqlMxView.Valid_Def.Trim().Equals("N"))
                {
                    validateViewButton.Enabled = true;
                    break;
                }
                else 
                {
                    validateViewButton.Enabled = false;
                }
            }
        } 

        private void ValidateViewButton_Click(object sender, EventArgs e) 
        {
            List<TrafodionView> sqlMxViews = new List<TrafodionView>();
            foreach (int selectedRowIndex in this.grid.SelectedRowIndexes)
            {
                TrafodionView sqlMxView = (TrafodionView)this.grid.Rows[selectedRowIndex].Cells[0].AuxValue;
                if (sqlMxView.Valid_Def.Trim().Equals("N"))
                {
                    sqlMxViews.Add(sqlMxView);
                }
            }
            if (sqlMxViews.Count == 0)
                return; //Bye
            try
            {
                DropConfirmDialog confirmDialog = new DropConfirmDialog(Properties.Resources.ValidateView, Properties.Resources.ValidateViewConfirmation, Trafodion.Manager.Properties.Resources.Question, "Cascade", true, Properties.Resources.ValidateViewConfirmationNote);
                confirmDialog.ShowDialog();

                DataTable warningsTable = new DataTable();

                if (confirmDialog.DialogResult == DialogResult.Yes)
                {
                    bool isCascade = confirmDialog.OptionValue;
                    Object[] parameters = new Object[] { isCascade, sqlMxViews, warningsTable };
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
                        warningsTable = progressDialog.ReturnValue as DataTable;
                        int warningCount = warningsTable == null ? 0 : warningsTable.Rows.Count;
                        if (warningCount == 1)
                        {
                            MessageBox.Show(Utilities.GetForegroundControl(), warningsTable.Rows[0].ItemArray[0].ToString(), "Warning on validating view", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        }
                        else if(warningCount>1)
                        {
                            TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.MultiGrantFailureMessage, warningsTable, System.Drawing.SystemIcons.Error);
                            mmd.ShowDialog();
                        }
                        if (this.Parent != null && this.Parent.Parent != null)
                        {
                            if (this.Parent.Parent is SchemaTabControl)
                                sqlMxViews[0].TheTrafodionSchema.FireChangedEventAtSchemaLevel();
                            else
                                sqlMxViews[0].TheTrafodionSchema.FireChangedEventAtViewLevel();
                        }
                        if(warningCount==0) //No error, and no warning, tell them we are successful.
                            MessageBox.Show("The validation operation has completed.", "Validate View", MessageBoxButtons.OK, MessageBoxIcon.Information);
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

        public DataTable ValidateViewOperation(bool isCascade, List<TrafodionView> theTrafodionViews, DataTable multiSqlWarnings)
        {
            //If we come here, theTrafodionViews.Count cannot be 0 because count check was performed before. 
            return theTrafodionViews[0].TheTrafodionSchema.ValidateViews(theTrafodionViews, isCascade, multiSqlWarnings);
        }


        /// <summary>
        /// Clone this window
        /// </summary>
        /// <returns></returns>
        public override Control Clone()
        {
            //Override this method, so the custom panel is cloned instead of the base panel
            TrafodionViewListPanel sqlMxObjectListPanel = new TrafodionViewListPanel(null, TheHeaderText, TheParentTrafodionObject, TheTrafodionObjects, TheTitle);
            sqlMxObjectListPanel.validateViewButton.Visible = false;
            return sqlMxObjectListPanel;
        }
    }
}
