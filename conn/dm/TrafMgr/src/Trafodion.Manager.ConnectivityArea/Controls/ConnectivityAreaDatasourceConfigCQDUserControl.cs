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
using System.Data;
using System.Collections.Generic;


namespace Trafodion.Manager.ConnectivityArea.Controls
{
 
    public partial class ConnectivityAreaDatasourceConfigCQDUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        NDCSDataSource _NdcsDatasource;
        private bool valuesChanged = false;
        private ConnectivityObjectsIGrid<Trafodion.Manager.ConnectivityArea.Model.NDCSObject> ConnectivityiGrid;
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
        public ConnectivityAreaDatasourceConfigCQDUserControl(NDCSDataSource aNdcsSDataSource)
        {
            this._NdcsDatasource = aNdcsSDataSource;
            InitializeComponent();
            SetupIGrid();

            SetInitialValues();
            PopulateControls();
        }


        public void PopulateControls()
        {
            int selectedIndex = -1;
            if (null != this.ConnectivityiGrid.SelectedRows && this.ConnectivityiGrid.SelectedRows.Count > 0)
                selectedIndex = this.ConnectivityiGrid.SelectedRows[0].Index;

            this.ConnectivityiGrid.Rows.Clear();
            if (null != this._NdcsDatasource.CQDs)
            {

                foreach (CQD theCQD in this._NdcsDatasource.CQDs)
                {
                    try
                    {
                        TenTec.Windows.iGridLib.iGRow row = this.ConnectivityiGrid.Rows.Add();
                        row.Cells[0].Value = theCQD.Attribute;
                        row.Cells[1].Value = theCQD.Value;
                        row.Tag = theCQD;

                    }
                    catch (Exception e)
                    {
                        //Error displaying CQD
                    }
                }
            }

            ConnectivityiGrid.UpdateCountControlText("There are {0} active CQDs");
            ConnectivityiGrid.Cols.AutoWidth();
            
            if (selectedIndex >= 0 && this.ConnectivityiGrid.Rows.Count > 0)
            {
                if (selectedIndex >= this.ConnectivityiGrid.Rows.Count)
                    selectedIndex = this.ConnectivityiGrid.Rows.Count - 1;

                this.ConnectivityiGrid.Rows[selectedIndex].Selected = true;
            }
        }

        public void SetupIGrid()
        {

            //
            // ConnectivityiGrid
            // 
            this.ConnectivityiGrid = new ConnectivityObjectsIGrid<Trafodion.Manager.ConnectivityArea.Model.NDCSObject>();

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add("CQD Name", typeof(System.String));
            dataTable.Columns.Add("Value", typeof(System.String));

            ConnectivityiGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            ConnectivityiGrid.SelectionChanged += new EventHandler(ConnectivityiGrid_SelectionChanged);
            ConnectivityiGrid.BeginUpdate();
            ConnectivityiGrid.FillWithData(dataTable);
            ConnectivityiGrid.ResizeGridColumns(dataTable);
            ConnectivityiGrid.EndUpdate();

            ConnectivityiGrid.Dock = DockStyle.Fill;
            ConnectivityiGrid.RowMode = true;

            TrafodionPanel1.Controls.Add(ConnectivityiGrid);
            ConnectivityiGrid.AddButtonControlToParent(DockStyle.Bottom);
            ConnectivityiGrid.AddCountControlToParent("There are {0} active CQDs", DockStyle.Top);

            ConnectivityiGrid.DoubleClickHandler = new TrafodionIGrid.DoubleClickDelegate(DoubleClick_ConnectivityiGrid);

        }

        private void DoubleClick_ConnectivityiGrid(int row)
        {
            ModifyCQD(row);
        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            this.cqdModify_TrafodionButton.Enabled = false;
            this.cqdRemove_TrafodionButton.Enabled = false;            

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
            return new ConnectivityAreaDatasourceConfigCQDUserControl(this._NdcsDatasource);

        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.TabPageLabel_CQD + " - " + this._NdcsDatasource.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this._NdcsDatasource.ConnectionDefinition; }
        }

        #endregion

        private void cqdAdd_TrafodionButton_Click(object sender, EventArgs e)
        {
            ConnectivityAreaConfigCQDAddDialog cqdAddDialog = new ConnectivityAreaConfigCQDAddDialog();
            DialogResult result = cqdAddDialog.New();
            if (result == DialogResult.OK)
            {
                CQD newCQD = cqdAddDialog.CQD;
                if (null == this._NdcsDatasource.CQDs)
                {
                    this._NdcsDatasource.CQDs = new List<CQD>();
                }

                CQD existingCQD = null;
                if ((existingCQD = CQDExists(newCQD)) != null)
                {
                    DialogResult dr = MessageBox.Show("A duplicate CQD was found. Would you like to overwrite the old value?", "Duplicate found", MessageBoxButtons.YesNo);
                    if (dr == DialogResult.Yes)
                    {
                        existingCQD.Value = newCQD.Value;
                    }
                    else
                    {
                        return;
                    }
                }
                else
                {
                    this._NdcsDatasource.CQDs.Add(newCQD);
                }

                ValueChanged();
                PopulateControls();
                //Handle inserting the CQD
            }
        }


        private CQD CQDExists(CQD cqdToCheck)
        {
            if (null == this._NdcsDatasource.CQDs)
                return null;

            foreach (CQD theCQD in this._NdcsDatasource.CQDs)
            {
                //Compare existing CQD to new CQD
                if (theCQD.Attribute == cqdToCheck.Attribute)
                    return theCQD;
            }
            return null;
        }

        private void cqdModify_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.ConnectivityiGrid.SelectedRows.Count <= 0)
                return;

            ModifyCQD(ConnectivityiGrid.SelectedRows[0].Index);
        }

        void ModifyCQD(int row)
        {
            ConnectivityAreaConfigCQDAddDialog cqdAddDialog = new ConnectivityAreaConfigCQDAddDialog();
            CQD cqdToEdit = (CQD)this.ConnectivityiGrid.Rows[row].Tag;
            DialogResult result = cqdAddDialog.Edit(cqdToEdit);
            if (result == DialogResult.OK)
            {
                //Handle the modify
                CQD newCQD = cqdAddDialog.CQD;
                cqdToEdit.Attribute = newCQD.Attribute;
                cqdToEdit.SystemDefault = newCQD.SystemDefault;
                cqdToEdit.Value = newCQD.Value;

                ValueChanged();
                PopulateControls();
            }

        }

        void ConnectivityiGrid_SelectionChanged(object sender, EventArgs e)
        {
            this.cqdRemove_TrafodionButton.Enabled = (ConnectivityiGrid.SelectedRows.Count >= 1);
            this.cqdModify_TrafodionButton.Enabled = ConnectivityiGrid.SelectedRows.Count.Equals(1);
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

        private void cqdRemove_TrafodionButton_Click(object sender, EventArgs e)
        {
            if (this.ConnectivityiGrid.SelectedRows.Count <= 0)
                return;

            bool removeSucceeded = true;
            foreach (TenTec.Windows.iGridLib.iGRow row in this.ConnectivityiGrid.SelectedRows)
            {
                if (!this.NdcsDatasource.CQDs.Remove(((CQD)row.Tag)))
                {
                    removeSucceeded = false;
                    break;
                }
            }

            if (!removeSucceeded)
                MessageBox.Show("Error removing CQD");

            ValueChanged();
            this.PopulateControls();
        }
    }
}
