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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.ConnectivityArea.Controls;
using Trafodion.Manager.Framework;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// The user control that's used to display a datagrid of NDCS services
    /// </summary>
    public partial class NDCSServicesUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private NDCSSystem _NdcsSystem;
        private ConnectivityObjectsIGrid<NDCSService> ndcsServicesDataGrid;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Get and set the NdcsSystem
        /// </summary>
        public NDCSSystem NdcsSystem
        {
            get { return _NdcsSystem; }
            set { _NdcsSystem = value; }
        }

        #endregion Properties

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        public NDCSServicesUserControl(NDCSSystem aNdcsSystem)
        {
            NdcsSystem = aNdcsSystem;
            InitializeComponent();
            ResetButtons();

            // Hide hold and release for non-services users
            if (!aNdcsSystem.ConnectionDefinition.IsAdminUser)
            {
                _holdServiceButton.Visible = false;
                _releaseServiceButton.Visible = false;
            }
            // Hide start and stop service buttons
            if (NdcsSystem.ConnectionDefinition.ServerVersion <= ConnectionDefinition.SERVER_VERSION.R23)
            {
                _startServiceButton.Visible = false;
                _stopServiceButton.Visible = false;
            }

            RefreshTable();
        }

        /// <summary>
        /// Alter the row that was double-clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void NdcsServicesDataGrid_DoubleClick(object sender, EventArgs e)
        {
            if (ndcsServicesDataGrid.CurRow != null && ndcsServicesDataGrid.CurRow.Index >= 0)
            {
                DoAlter();
            }
        }

        /// <summary>
        /// Enable the buttons based on row selections
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ndcsServicesDataGrid_SelectionChanged(object sender, EventArgs e)
        {
            //Some of the buttons should only be enabled if at least 1 row is selected
            if (ndcsServicesDataGrid.SelectedRows.Count > 0)
            {
                iGRow row = ndcsServicesDataGrid.SelectedRows[0];
                iGCell cell = row.Cells[Properties.Resources.ServiceState];
                _alterServiceButton.Enabled = true;
                if (NdcsSystem.ConnectionDefinition.ServerVersion > ConnectionDefinition.SERVER_VERSION.R23)
                {
                    _startServiceButton.Enabled = false;
                    _stopServiceButton.Enabled = false;
                    _deleteServiceButton.Enabled = false;
                    if (cell.Value.Equals("TEST"))//NDCSService.ACTIVE))
                    {
                        _stopServiceButton.Enabled = true;
                    }
                    else if (cell.Value.Equals("TEST"))//NDCSService.STOPPED))
                    {
                        _startServiceButton.Enabled = true;
                        _deleteServiceButton.Enabled = true;
                    }
                }
                else
                {
                    _deleteServiceButton.Enabled = true;
                }

                if (cell.Value.Equals("TBD")) //NDCSService.ACTIVE))
                {
                    _holdServiceButton.Enabled = true;
                    _releaseServiceButton.Enabled = false;
                }
                else if (cell.Value.Equals("TBD")) //NDCSService.HOLD))
                {
                    _holdServiceButton.Enabled = false;
                    _releaseServiceButton.Enabled = true;
                }
                else if (cell.Value.Equals("TBD")) //NDCSService.STOPPED))
                {
                    _holdServiceButton.Enabled = false;
                    _releaseServiceButton.Enabled = false;
                }
            }
            else
            {
                ResetButtons();
            }
        }

        /// <summary>
        /// Resets buttons to their initial state
        /// </summary>
        private void ResetButtons()
        {
            _alterServiceButton.Enabled = false;
            _deleteServiceButton.Enabled = false;
            _startServiceButton.Enabled = false;
            _stopServiceButton.Enabled = false;
            _holdServiceButton.Enabled = false;
            _releaseServiceButton.Enabled = false;
        }

        /// <summary>
        /// Creates a new table and adds it to the display
        /// </summary>
        private void RefreshTable()
        {
            ResetButtons();

            // Add columns
            DataTable dataTable = new DataTable();
        }

        private void addServiceButton_Click(object sender, EventArgs ea)
        {
            //try
            //{
            //    AddServiceDialog theAddServiceDialog = new AddServiceDialog(WmsSystem);
            //    if (theAddServiceDialog.ShowDialog() == DialogResult.OK)
            //    {
            //        RefreshTable();
            //        ((ServicesFolder)wmsConfigurationUserControl.WmsTreeView.SelectedNode).refreshTree();
            //    }
            //}
            //catch (System.Data.Odbc.OdbcException oe)
            //{
            //    // Got an ODBC erorr. Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            //}
            //catch (Exception e)
            //{

            //    // Got some other exception.  Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            //}
        }

        /// <summary>
        /// Alters the selected service
        /// </summary>
        private void DoAlter()
        {
            //try
            //{
            //    int theRowIndex = ndcsServicesDataGrid.SelectedRows[0].Index;
            //    WmsService theService = ndcsServicesDataGrid.Cells[theRowIndex, 0].Value as WmsService;
            //    AlterServiceDialog theAlterServiceDialog = new AlterServiceDialog(WmsSystem, theService);
            //    if (theAlterServiceDialog.ShowDialog() == DialogResult.OK)
            //    {
            //        RefreshTable();
            //        ndcsServicesDataGrid.SetCurRow(theRowIndex);
            //    }

            //}
            //catch (System.Data.Odbc.OdbcException oe)
            //{
            //    // Got an ODBC erorr. Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            //}
            //catch (Exception e)
            //{

            //    // Got some other exception.  Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            //}
        }

        private void _alterServiceButton_Click(object sender, EventArgs ea)
        {
            DoAlter();
        }

        private void _deleteServiceButton_Click(object sender, EventArgs ea)
        {
            //if (MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ConfirmDeleteWMS, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo) == DialogResult.Yes)
            //{
            //    try
            //    {
            //        int theRowIndex = ndcsServicesDataGrid.SelectedRows[0].Index;
            //        WmsService theService = ndcsServicesDataGrid.Cells[theRowIndex, 0].Value as WmsService;
            //        theService.Delete(false);
            //        RefreshTable();
            //        ((ServicesFolder)wmsConfigurationUserControl.WmsTreeView.SelectedNode).refreshTree();
            //    }
            //    catch (System.Data.Odbc.OdbcException oe)
            //    {
            //        // Got an ODBC erorr. Show it.
            //        MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            //    }
            //    catch (Exception e)
            //    {

            //        // Got some other exception.  Show it.
            //        MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            //    }
            //}
        }


        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            //return new ServicesUserControl(wmsConfigurationUserControl, WmsSystem);
            return null;
        }

        /// <summary>
        /// Returns the window title
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.TabPageLabel_Services + " - " + NdcsSystem.ConnectionDefinition.Name; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsSystem.ConnectionDefinition; }
        }

        #endregion

        private void _startServiceButton_Click(object sender, EventArgs ea)
        {
            //try
            //{
            //    int theRowIndex = ndcsServicesDataGrid.SelectedRows[0].Index;
            //    WmsService theService = ndcsServicesDataGrid.Cells[theRowIndex, 0].Value as WmsService;
            //    theService.Start();
            //    RefreshTable();
            //    ndcsServicesDataGrid.SetCurRow(theRowIndex);
            //}
            //catch (System.Data.Odbc.OdbcException oe)
            //{
            //    // Got an ODBC erorr. Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            //}
            //catch (Exception e)
            //{
            //    // Got some other exception.  Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            //}
        }

        private void _stopServiceButton_Click(object sender, EventArgs ea)
        {
            //try
            //{
            //    int theRowIndex = ndcsServicesDataGrid.SelectedRows[0].Index;
            //    WmsService theService = ndcsServicesDataGrid.Cells[theRowIndex, 0].Value as WmsService;
            //    theService.Stop();
            //    RefreshTable();
            //    ndcsServicesDataGrid.SetCurRow(theRowIndex);
            //}
            //catch (System.Data.Odbc.OdbcException oe)
            //{
            //    // Got an ODBC erorr. Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            //}
            //catch (Exception e)
            //{
            //    // Got some other exception.  Show it.
            //    MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            //}
        }
    }
}
