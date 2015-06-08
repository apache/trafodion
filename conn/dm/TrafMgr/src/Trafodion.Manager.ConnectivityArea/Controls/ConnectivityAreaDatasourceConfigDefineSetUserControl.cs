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
 
    public partial class ConnectivityAreaDatasourceConfigDefineSetUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        NDCSDataSource _NdcsDatasource;
        private bool valuesChanged = false;
        private ConnectivityAreaDatasourceConfigurationUserControl _linkToParent;
        public ConnectivityAreaDatasourceConfigurationUserControl LinkToParent
        {
            set { _linkToParent = value; 
            }
        }
        ConnectivityObjectsIGrid<NDCSDataSource> SetsiGrid;
        ConnectivityObjectsIGrid<NDCSDataSource> DefinesiGrid;

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
        public ConnectivityAreaDatasourceConfigDefineSetUserControl(NDCSDataSource aNdcsDataSource)
        {
            InitializeComponent();
            this._NdcsDatasource = aNdcsDataSource;

            // Defines are not supported in SQ.
            this.splitContainer1.Panel1Collapsed = true;

            //NdcsSystem = aNdcsSystem;
            SetInitialValues();
            SetupIGrid();
            PopulateContols();
        }

        public void PopulateContols()
        {
            PopulateSets();
        }

        public void SetupIGrid()
        {
            SetupSetsIGrid();
        }

        private void PopulateSets()
        {
            int setsSelectedIndex = -1;
            if (null != this.SetsiGrid.SelectedRows && this.SetsiGrid.SelectedRows.Count > 0)
                setsSelectedIndex = this.SetsiGrid.SelectedRows[0].Index;

            this.SetsiGrid.Rows.Clear();
            if (null != this._NdcsDatasource.Sets)
            {

                foreach (Set theSet in this._NdcsDatasource.Sets)
                {
                    try
                    {
                        TenTec.Windows.iGridLib.iGRow row = this.SetsiGrid.Rows.Add();
                        row.Cells[0].Value = theSet.Name;
                        row.Cells[1].Value = theSet.Value;
                        row.Tag = theSet;
                    }
                    catch (Exception e)
                    {
                        //Error Set policy
                    }
                }

                SetsiGrid.Cols.AutoWidth();

                //Make sure that the previously selected row is re-selected
                if (setsSelectedIndex >= 0 && this.SetsiGrid.Rows.Count > 0)
                {
                    if (setsSelectedIndex >= this.SetsiGrid.Rows.Count)
                        setsSelectedIndex = this.SetsiGrid.Rows.Count - 1;

                    this.SetsiGrid.Rows[setsSelectedIndex].Selected = true;
                }
            }

            this.SetsiGrid.UpdateCountControlText("There are {0} configured SETs");
        }

        private void PopulateDefines()
        {
            int definesSelectedIndex = -1;
            if (null != this.DefinesiGrid.SelectedRows && this.DefinesiGrid.SelectedRows.Count > 0)
                definesSelectedIndex = this.DefinesiGrid.SelectedRows[0].Index;

            this.DefinesiGrid.Rows.Clear();
            if (null != this._NdcsDatasource.Defines)
            {

                foreach (Define theDefine in this._NdcsDatasource.Defines)
                {
                    try
                    {
                        TenTec.Windows.iGridLib.iGRow row = this.DefinesiGrid.Rows.Add();
                        row.Cells[0].Value = theDefine.Name;
                        row.Cells[1].Value = theDefine.Attribute;
                        row.Tag = theDefine;
                    }
                    catch (Exception e)
                    {
                        //Error Set policy
                    }
                }

                DefinesiGrid.Cols.AutoWidth();
                //Make sure that the previously selected row is re-selected
                if (definesSelectedIndex >= 0 && this.DefinesiGrid.Rows.Count > 0)
                {
                    if (definesSelectedIndex >= this.DefinesiGrid.Rows.Count)
                        definesSelectedIndex = this.DefinesiGrid.Rows.Count - 1;

                    this.DefinesiGrid.Rows[definesSelectedIndex].Selected = true;
                }
            }

            this.DefinesiGrid.UpdateCountControlText("There are {0} configured DEFINEs");
        }

        private void SetupSetsIGrid()
        {
            SetsiGrid = new ConnectivityObjectsIGrid<NDCSDataSource>();

            //Configure the "sets" IGrid
            DataTable setsDataTable = new DataTable();
            setsDataTable.Columns.Add("Name", typeof(System.String));
            setsDataTable.Columns.Add("Value", typeof(System.String));
            SetsiGrid.BeginUpdate();
            SetsiGrid.FillWithData(setsDataTable);
            SetsiGrid.ResizeGridColumns(setsDataTable);
            SetsiGrid.EndUpdate();
            SetsiGrid.Dock = DockStyle.Fill;
            SetsiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            SetsiGrid.SelectionChanged += new EventHandler(SetsiGrid_SelectionChanged);

            this.sets_TrafodionPanel.Controls.Add(SetsiGrid);
            SetsiGrid.Cols.AutoWidth();

            SetsiGrid.AddButtonControlToParent(DockStyle.Bottom);

            this.SetsiGrid.AddCountControlToParent("There are {0} configured SETs", DockStyle.Top);

            SetsiGrid.DoubleClickHandler = new TrafodionIGrid.DoubleClickDelegate(DoubleClick_SetsiGrid);
        }

        private void SetupDefinesIGrid()
        {
            DefinesiGrid = new ConnectivityObjectsIGrid<NDCSDataSource>();

            //Configure the "define" IGrid
            DataTable definesDataTable = new DataTable();
            definesDataTable.Columns.Add("Name", typeof(System.String));
            definesDataTable.Columns.Add("Attributes", typeof(System.String));
            DefinesiGrid.BeginUpdate();
            DefinesiGrid.FillWithData(definesDataTable);
            DefinesiGrid.ResizeGridColumns(definesDataTable);
            DefinesiGrid.EndUpdate();
            DefinesiGrid.Dock = DockStyle.Fill;
            DefinesiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            DefinesiGrid.SelectionChanged += new EventHandler(DefinesiGrid_SelectionChanged);

            this.defines_TrafodionPanel.Controls.Add(DefinesiGrid);
            DefinesiGrid.Cols.AutoWidth();

            DefinesiGrid.AddButtonControlToParent(DockStyle.Bottom);

            this.DefinesiGrid.AddCountControlToParent("There are {0} configured DEFINEs", DockStyle.Top);

            DefinesiGrid.DoubleClickHandler = new TrafodionIGrid.DoubleClickDelegate(DoubleClick_DefinesiGrid);
        }

        private void DoubleClick_SetsiGrid(int row)
        {
            ModifySet(row);
        }

        private void DoubleClick_DefinesiGrid(int row)
        {
            ModifyDefine(row);
        }

        void DefinesiGrid_SelectionChanged(object sender, EventArgs e)
        {
            this.definesRemove_TrafodionButton.Enabled = (DefinesiGrid.SelectedRows.Count >= 1);
            this.definesModify_TrafodionButton.Enabled = DefinesiGrid.SelectedRows.Count.Equals(1);
        }

        void SetsiGrid_SelectionChanged(object sender, EventArgs e)
        {
            this.setsRemove_TrafodionButton.Enabled = (SetsiGrid.SelectedRows.Count >= 1);
            this.setsModify_TrafodionButton.Enabled = SetsiGrid.SelectedRows.Count.Equals(1);
        }


        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            this.setsModify_TrafodionButton.Enabled = false;
            this.setsRemove_TrafodionButton.Enabled = false;
            this.definesRemove_TrafodionButton.Enabled = false;
            this.definesModify_TrafodionButton.Enabled = false;

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
            return new ConnectivityAreaDatasourceConfigDefineSetUserControl(_NdcsDatasource);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get 
            { 
                //return Properties.Resources.TabPageLabel_DefinesSets + " - " + NdcsSystem.ConnectionDefinition.Name;
                return String.Format("{0} - {1}", Properties.Resources.TabPageLabel_Sets,
                                     this.ConnectionDefn.Name);
            }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsDatasource.ConnectionDefinition; }
        }

        #endregion

        private void definesAdd_TrafodionButton_Click(object sender, EventArgs e)
        {
            ConnectivityAreaConfigBasicAddDialog defineAddDialog = new ConnectivityAreaConfigBasicAddDialog("Add DEFINE", 1);
            DialogResult result = defineAddDialog.New();
            if (result == DialogResult.OK)
            {
                //Handle inserting the define
                Define newDefine = defineAddDialog.UserControl.TheDefine;
               
                if (null == this._NdcsDatasource.Defines)
                    this._NdcsDatasource.Defines = new List<Define>(); 
                
                this._NdcsDatasource.Defines.Add(newDefine);
                ValueChanged();
                PopulateContols();

            }
        }

        private void setsAdd_TrafodionButton_Click(object sender, EventArgs e)
        {
            ConnectivityAreaConfigBasicAddDialog setAddDialog = new ConnectivityAreaConfigBasicAddDialog("Add SET", 0);
            DialogResult result = setAddDialog.New();
            if (result == DialogResult.OK)
            {
                //Handle inserting the set
                Set newSet = setAddDialog.UserControl.TheSet;
                if (null == this._NdcsDatasource.Sets)
                    this._NdcsDatasource.Sets = new List<Set>();

                Set existingSet = _NdcsDatasource.FindSet(newSet.Name);
                if (existingSet == null)
                {
                    this._NdcsDatasource.Sets.Add(newSet);
   
                }
                else
                {
                    DialogResult dr = MessageBox.Show("A duplicate SET was found. Would you like to overwrite the old value?", "Duplicate found", MessageBoxButtons.YesNo);
                    if (dr == DialogResult.Yes)
                    {
                        existingSet.Value = newSet.Value;
                    }
                    else
                    {
                        return;
                    }
                }
                ValueChanged();
                PopulateContols();
            }
        }


        private void definesModify_TrafodionButton_Click(object sender, EventArgs e)
        {
            
            if (this.DefinesiGrid.SelectedRows.Count <= 0)
                return;

            ModifyDefine(DefinesiGrid.SelectedRows[0].Index);
        }

        private void ModifyDefine(int row)
        {
            ConnectivityAreaConfigBasicAddDialog defineAddDialog = new ConnectivityAreaConfigBasicAddDialog("Edit Define", 1);
            Define defineToEdit = (Define)this.DefinesiGrid.Rows[row].Tag;

            DialogResult result = defineAddDialog.Edit(defineToEdit);
            if (result == DialogResult.OK)
            {
                Define newDefine = defineAddDialog.UserControl.TheDefine;
                defineToEdit.Attribute = newDefine.Attribute;
                defineToEdit.Name = newDefine.Name;

                ValueChanged();
                PopulateContols();
                //Handle the modify
            }

        }

        private void setsModify_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.SetsiGrid.SelectedRows.Count <= 0)
                return;

            ModifySet(SetsiGrid.SelectedRows[0].Index);
        }

        private void ModifySet(int row)
        {
            ConnectivityAreaConfigBasicAddDialog setsAddDialog = new ConnectivityAreaConfigBasicAddDialog("Edit Set", 0);
            Set setToEdit = (Set)this.SetsiGrid.Rows[row].Tag;
            DialogResult result = setsAddDialog.Edit(setToEdit);
            if (result == DialogResult.OK)
            {
                Set newSet = setsAddDialog.UserControl.TheSet;
                setToEdit.Name = newSet.Name;
                setToEdit.Value = newSet.Value;

                //Handle the modify

                ValueChanged();
                PopulateContols();
            }
        }

        private void definesRemove_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.DefinesiGrid.SelectedRows.Count <= 0)
                return;

            bool removeSucceeded = true;
            foreach (TenTec.Windows.iGridLib.iGRow row in this.DefinesiGrid.SelectedRows)
            {
                if (!this._NdcsDatasource.Defines.Remove(((Define)row.Tag)))
                {
                    removeSucceeded = false;
                    break;
                }
            }

            if (!removeSucceeded)
                MessageBox.Show("Error removing Define");

            ValueChanged();
            this.PopulateContols();
        }


        public bool CheckAndCommit()
        {
            //Perform final consistency validation

            //Commit values from GUI Controls --
            //Overwrite existing values in the Datasource object.
            this.valuesChanged = false;
            return true;
        }

        private void ValueChanged()
        {
            this.valuesChanged = true;
            if (null != this._linkToParent)
            {
                this._linkToParent.EnableApplyButton();
            }
        }

        private void setsRemove_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.SetsiGrid.SelectedRows.Count <= 0)
                return;

            bool removeSucceeded = true;
            foreach (TenTec.Windows.iGridLib.iGRow row in this.SetsiGrid.SelectedRows)
            {
                if (!this._NdcsDatasource.Sets.Remove(((Set)row.Tag)))
                {
                    removeSucceeded = false;
                    break;
                }
            }

            if (!removeSucceeded)
                MessageBox.Show("Error removing Define");

            ValueChanged();
            this.PopulateContols();
        }
    }
}
