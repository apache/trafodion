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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary> 
    public partial class NDCSPermissionsUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        NDCSSystem _theNDCSSystem = null;
        private ConnectivityObjectsIGrid<NDCSUser> ConnectivityNDCSUsersDataGrid = new ConnectivityObjectsIGrid<NDCSUser>();

        #endregion Fields

        #region Properties

        /// <summary>
        /// Gets the underlying system
        /// </summary>
        public NDCSSystem NdcsSystem
        {
            get { return _theNDCSSystem; }
        }

        #endregion Properties

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNdcsSystem"></param>
        public NDCSPermissionsUserControl(NDCSSystem aNdcsSystem)
        {
            InitializeComponent();
            _theNDCSSystem = aNdcsSystem;
            SetInitialValues();
            UpdateControls();
        }

        void UpdateControls()
        {
            _revokeButton.Enabled = this.ConnectivityNDCSUsersDataGrid.SelectedRows.Count > 0;
        }

        private void _refreshButton_Click(object sender, EventArgs e)
        {
            RefreshAll();
        }

        private void RefreshAll()
        {
            NdcsSystem.RefreshUsers();
            RefreshDataGrid();
        }

        private void RefreshDataGrid()
        {
            Cursor.Current = Cursors.WaitCursor;
            try
            {

                //Save sort order
                ArrayList sortObject = Trafodion.Manager.Framework.Utilities.GetSortObject(ConnectivityNDCSUsersDataGrid);

                ConnectivityNDCSUsersDataGrid.Rows.Clear();
                Populate();

                //re-apply sort
                Trafodion.Manager.Framework.Utilities.ApplySort(this.ConnectivityNDCSUsersDataGrid, sortObject);

            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
            UpdateControls();
        }
        
        internal void Populate()
        {
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(Properties.Resources.NDCSOperator, typeof(NDCSUser));
            dataTable.Columns.Add(Properties.Resources.Grantor, typeof(string));

            try
            {
                foreach (NDCSUser user in NdcsSystem.NDCSUsers)
                {
                    dataTable.Rows.Add(new object[] {user, user.GrantorRoleName});
                }
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
                return;
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
                return;
            }

            ConnectivityNDCSUsersDataGrid.FillWithData(dataTable);
            ConnectivityNDCSUsersDataGrid.ResizeGridColumns(dataTable);

            ConnectivityNDCSUsersDataGrid.ObjectLinkColumnNumber = 0;
            ConnectivityNDCSUsersDataGrid.Dock = DockStyle.Fill;
            ConnectivityNDCSUsersDataGrid.TreeView = null;

            string _timestamp = Utilities.CurrentFormattedDateTime;
            string _strformat;

            {
                _strformat = string.Format(Properties.Resources.RefreshTimestampCaption, _timestamp) +
                    string.Format(Properties.Resources.OperatorPrivilegeCount, dataTable.Rows.Count);

                ConnectivityNDCSUsersDataGrid.UpdateCountControlText(_strformat);
            }

        }

        /// <summary>
        /// Stores the initial settings
        /// </summary>
        private void SetInitialValues()
        {
            this._tablePanel.Controls.Clear();
            this._tablePanel.Controls.Add(ConnectivityNDCSUsersDataGrid);
            ConnectivityNDCSUsersDataGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            ConnectivityNDCSUsersDataGrid.SelectionChanged += new EventHandler(ConnectivityNDCSUsersDataGrid_SelectionChanged);
            ConnectivityNDCSUsersDataGrid.AutoResizeCols = true;

            // make the initial top status panel here
            this.ConnectivityNDCSUsersDataGrid.AddCountControlToParent("   ", DockStyle.Top);

            Control theDataGridSaveControl = this.ConnectivityNDCSUsersDataGrid.GetButtonControl();
            theDataGridSaveControl.Dock = DockStyle.Fill;
            this._bottomRightPanel.Controls.Add(theDataGridSaveControl);
        }

        void ConnectivityNDCSUsersDataGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void _grantButton_Click(object sender, EventArgs e)
        {
            NDCSUser aNewUSer = new NDCSUser(_theNDCSSystem);
            EditNDCSUserDialog addDialog = new EditNDCSUserDialog(aNewUSer);
            if (addDialog.ShowDialog() == DialogResult.OK)
            {
                RefreshAll();
            }
        }

        private void _revokeButton_Click(object sender, EventArgs e)
        {
            if (ConnectivityNDCSUsersDataGrid.SelectedRows.Count > 0)
            {
                List<NDCSUser> selectedUsers = new List<NDCSUser>();
                foreach (TenTec.Windows.iGridLib.iGRow row in ConnectivityNDCSUsersDataGrid.SelectedRows)
                {
                    NDCSUser ndcsUser = row.Cells[0].Value as NDCSUser;
                    if (ndcsUser != null)
                    {
                        selectedUsers.Add(ndcsUser);
                    }
                }
                
                StringBuilder skipList = new StringBuilder();
                int nUsers = 0;
                foreach (NDCSUser ndcsUser in selectedUsers)
                {
                    nUsers++;
                    if (nUsers <= 20)
                        skipList.AppendLine(ndcsUser.Name);
                }

                if (20 < nUsers)
                    skipList.Append(Environment.NewLine + String.Format(Properties.Resources.DeleteMoreThan20Users,(nUsers - 20)));

                DialogResult result = MessageBox.Show(Properties.Resources.ConfirmRevokePrivilege + 
                                                      Environment.NewLine +
                                                      skipList + Environment.NewLine,
                                                    Properties.Resources.Revoke, MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                if (result == DialogResult.Yes)
                {
                    foreach (TenTec.Windows.iGridLib.iGRow row in ConnectivityNDCSUsersDataGrid.SelectedRows)
                    {
                        NDCSUser ndcsUser = row.Cells[0].Value as NDCSUser;
                        if (ndcsUser != null)
                        {
                            try
                            {
                                this.NdcsSystem.DeleteUser(ndcsUser);
                            }
                            catch (Exception ex)
                            {
                                MessageBox.Show(ex.Message, Properties.Resources.FailedRevokePrivilege, MessageBoxButtons.OK, MessageBoxIcon.Error);
                            }
                        }
                    }
                    RefreshAll();
                }
            }
        }

        #region ICloneToWindow

        /// <summary>
        /// Clones the privileges panel into a new window
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            NDCSPermissionsUserControl permissionsControl = new NDCSPermissionsUserControl(NdcsSystem);
            permissionsControl.Populate();
            return permissionsControl;
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        public string WindowTitle
        {
            get { return Properties.Resources.NDCSPrivileges; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return NdcsSystem.ConnectionDefinition; }
        }

        #endregion ICloneToWindow
    }
}
