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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Collections.Generic;

using System.Data;
namespace Trafodion.Manager.ConnectivityArea.Controls
{
 
    public partial class ConnectivityAreaDatasourceConfigControlledTablesUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        NDCSDataSource _NdcsDatasource;
        private bool valuesChanged = false;
        ConnectivityObjectsIGrid<NDCSDataSource> ControlledTableiGrid;
        ConnectivityAreaConfigControlledTablesAddDialog ctAddDialog;
        private ConnectivityAreaDatasourceConfigurationUserControl _linkToParent;
        public ConnectivityAreaDatasourceConfigurationUserControl LinkToParent
        {
            set
            {
                _linkToParent = value;
            }
        }

        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSDataSource NdcsDatasource
        {
            get { return _NdcsDatasource; }
            set { _NdcsDatasource = value; }
        }

        public bool ValuesChanged
        {
            get { return valuesChanged; }
            set { valuesChanged = value; }
        }

        #endregion Properties
        public ConnectivityAreaDatasourceConfigControlledTablesUserControl(NDCSDataSource aNdcsDatasource)
        {
            this._NdcsDatasource = aNdcsDatasource;

            InitializeComponent();
            //NdcsSystem = aNdcsSystem;
            SetInitialValues();
            SetupIGrid();

            SetupBrowseDialog();

            PopulateControls();
        }

        public void PopulateControls()
        {
            this.ControlledTableiGrid.Rows.Clear();
            if (null != this._NdcsDatasource.ControlledTables)
            {
                int selectedIndex = -1;
                if (null != this.ControlledTableiGrid.SelectedRows && this.ControlledTableiGrid.SelectedRows.Count > 0)
                    selectedIndex = this.ControlledTableiGrid.SelectedRows[0].Index;

                foreach (ControlledTable theTable in this._NdcsDatasource.ControlledTables)
                {
                    try
                    {
                        TenTec.Windows.iGridLib.iGRow row = this.ControlledTableiGrid.Rows.Add();
                        row.Cells[0].Value = theTable.Name;
                        row.Cells[1].Value = theTable.IfLocked;
                        row.Cells[2].Value = theTable.Mdam;

                        if (theTable.Priority != ControlledTable.UNDEFINED_PRIORITY)
                            row.Cells[3].Value = theTable.Priority;

                        row.Cells[4].Value = theTable.SimilarityCheck;
                        row.Cells[5].Value = theTable.TableLock;

                        if (theTable.Timeout != ControlledTable.UNDEFINED_TIMEOUT)
                        {
                            row.Cells[6].Value = (theTable.Timeout == ControlledTable.NO_TIMEOUT) ? ControlledTable.NOTIMEOUT : ((theTable.Timeout == ControlledTable.WILLNOTWAIT_TIMEOUT) ? ControlledTable.WILLNOTWAIT : theTable.Timeout + "");
                        }

                        row.Tag = theTable;

                    }
                    catch (Exception e)
                    {
                        //Error displaying CQD
                    }
                }

                //Make sure that the previously selected row is re-selected
                if (selectedIndex >= 0 && this.ControlledTableiGrid.Rows.Count > 0)
                {
                    if (selectedIndex >= this.ControlledTableiGrid.Rows.Count)
                        selectedIndex = this.ControlledTableiGrid.Rows.Count - 1;

                    this.ControlledTableiGrid.Rows[selectedIndex].Selected = true;
                }
            }

            ControlledTableiGrid.UpdateCountControlText("There are {0} Controlled Tables");
            ControlledTableiGrid.Cols.AutoWidth();

        }

        public void SetupBrowseDialog()
        {
            
        }


        public void SetupIGrid()
        {
            if (null != ControlledTableiGrid)
                return;

            ControlledTableiGrid = new ConnectivityObjectsIGrid<NDCSDataSource>();

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add("Name", typeof(System.String));
            dataTable.Columns.Add("If-Locked", typeof(System.String));
            dataTable.Columns.Add("MDAM", typeof(System.String));
            dataTable.Columns.Add("Priority", typeof(System.Int16));
            dataTable.Columns.Add("Similarity Check", typeof(System.String));
            dataTable.Columns.Add("Table Lock", typeof(System.String));
            dataTable.Columns.Add("Timeout", typeof(System.Double));

            ControlledTableiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            ControlledTableiGrid.SelectionChanged += new EventHandler(ControlledTableiGrid_SelectionChanged);
            ControlledTableiGrid.BeginUpdate();
            ControlledTableiGrid.FillWithData(dataTable);
            ControlledTableiGrid.ResizeGridColumns(dataTable);
            ControlledTableiGrid.EndUpdate();
            ControlledTableiGrid.Dock = DockStyle.Fill;
            TrafodionPanel1.Controls.Add(ControlledTableiGrid);
            ControlledTableiGrid.AddButtonControlToParent(DockStyle.Bottom);
            ControlledTableiGrid.AddCountControlToParent("There are {0} Controlled Tables", DockStyle.Top);
            ControlledTableiGrid.DoubleClickHandler = new TrafodionIGrid.DoubleClickDelegate(DoubleClick_ControlledTableiGrid);
        }

        private void DoubleClick_ControlledTableiGrid(int row)
        {
            ModifyControlledTable(row);
        }

        void ControlledTableiGrid_SelectionChanged(object sender, EventArgs e)
        {
                this.ctRemoveControlledTable_TrafodionButton.Enabled = (ControlledTableiGrid.SelectedRows.Count >= 1);
                this.ctModifyControlledTable_TrafodionButton.Enabled = ControlledTableiGrid.SelectedRows.Count.Equals(1);            
        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            this.ctRemoveControlledTable_TrafodionButton.Enabled = false;
            this.ctModifyControlledTable_TrafodionButton.Enabled = false;


            valuesChanged = false;
            //Set dummy values
        }

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            return new ConnectivityAreaDatasourceConfigControlledTablesUserControl(_NdcsDatasource);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.TabPageLabel_ControlledTables + " - " + this._NdcsDatasource.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this._NdcsDatasource.ConnectionDefinition; }
        }

        #endregion

        private void addControlledTable_TrafodionButton_Click(object sender, EventArgs e)
        {

            ctAddDialog = new ConnectivityAreaConfigControlledTablesAddDialog();
            DialogResult dr = ctAddDialog.New();
            if (dr == DialogResult.OK)
            {
                
                if (null == this._NdcsDatasource.ControlledTables)
                {
                    this._NdcsDatasource.ControlledTables = new List<ControlledTable>();
                }

                ControlledTable existingTable = _NdcsDatasource.FindControlledTable(ctAddDialog.ControlledTable.Name);
                if (existingTable == null)
                {
                    this._NdcsDatasource.ControlledTables.Add(ctAddDialog.ControlledTable);
                }
                else
                {
                    DialogResult drm = MessageBox.Show("A duplicate Controlled Table entry was found. Would you like to overwrite the old value?", "Duplicate found", MessageBoxButtons.YesNo);
                    if (drm == DialogResult.Yes)
                    {
                        ControlledTable newTable = ctAddDialog.ControlledTable;
                        existingTable.Name = newTable.Name;
                        existingTable.IfLocked = newTable.IfLocked;
                        existingTable.Mdam = newTable.Mdam;
                        existingTable.Priority = newTable.Priority;
                        existingTable.SimilarityCheck = newTable.SimilarityCheck;
                        existingTable.TableLock = newTable.TableLock;
                        existingTable.Timeout = newTable.Timeout;
                    }
                    else
                    {
                        return;
                    }
                }

                ValueChanged();
                PopulateControls();
            }
        }

        private void ctModifyControlledTable_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.ControlledTableiGrid.SelectedRows.Count <= 0)
                return;
            
            ModifyControlledTable(ControlledTableiGrid.SelectedRows[0].Index);
        }

        void ModifyControlledTable(int row)
        {
            ConnectivityAreaConfigControlledTablesAddDialog ctModifyDialog = new ConnectivityAreaConfigControlledTablesAddDialog();
            ControlledTable ctToEdit = (ControlledTable)this.ControlledTableiGrid.Rows[row].Tag;
            DialogResult dr = ctModifyDialog.Edit(ctToEdit);

            if (dr == DialogResult.OK)
            {

                ControlledTable newTable = ctModifyDialog.ControlledTable;
                ctToEdit.Name = newTable.Name;
                ctToEdit.IfLocked = newTable.IfLocked;
                ctToEdit.Mdam = newTable.Mdam;
                ctToEdit.Priority = newTable.Priority;
                ctToEdit.SimilarityCheck = newTable.SimilarityCheck;
                ctToEdit.TableLock = newTable.TableLock;
                ctToEdit.Timeout = newTable.Timeout;

                ValueChanged();
                this.PopulateControls();
            }
        }

        private void ctRemoveControlledTable_TrafodionButton_Click(object sender, EventArgs e)
        {

            if (this.ControlledTableiGrid.SelectedRows.Count <= 0)
                return;

            //Prompt: "Are you sure you want to remove this item?"
            string promptText = "";
            string captionText = "";

            if (this.ControlledTableiGrid.SelectedRows.Count == 1)
            {
                promptText = "Are you sure you want to remove the selected item?";
                captionText = "Remove Controlled Table";
            }
            else
            {
                promptText = "Are you sure you want to remove the selected items?";
                captionText = "Remove Controlled Tables";
            }

           DialogResult dr = MessageBox.Show(promptText, captionText, MessageBoxButtons.YesNo);
           bool removeSucceeded = true;

           if (dr == DialogResult.Yes)
            {
               foreach(TenTec.Windows.iGridLib.iGRow row in  this.ControlledTableiGrid.SelectedRows)
               {
                   if (!this._NdcsDatasource.ControlledTables.Remove(((ControlledTable)row.Tag)))
                   {
                       removeSucceeded = false;
                       break;
                   }
               }

               if (!removeSucceeded)
                   MessageBox.Show("Error removing Controlled Table");

               ValueChanged();
                this.PopulateControls();    
            }
        }

        private void ValueChanged()
        {
            this.valuesChanged = true;
            if (null != this._linkToParent)
            {
                this._linkToParent.EnableApplyButton();
            }
        }

        public bool CheckAndCommit()
        {
            //Perform final consistency validation
            //Commit values from GUI Controls --
            //Overwrite existing values in the Datasource object.
            this.valuesChanged = false;
            return true;
        }


    }
}
