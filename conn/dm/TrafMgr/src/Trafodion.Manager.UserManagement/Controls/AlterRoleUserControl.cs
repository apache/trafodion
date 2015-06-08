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
using System.Drawing;
using System.Linq;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UserManagement.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class AlterRoleUserControl : UserControl
    {
        #region Fields

        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        ConnectionDefinition _connectionDefinition;
        private ArrayList _arrayOrginalUsers = new ArrayList();
        private ArrayList _arrayGrantingUsers = new ArrayList();
        private ArrayList _arrayRevokingUsers = new ArrayList();

        //contains Roles list datatable
        DataTable _dataTable;
        private string _roleName;
        private int _rowIndex;
        private UserSelectionPanel userSelectionPanel = null;
        private const string COL_KEY_USERNAME = "USER_NAME";  
        private const string COL_TEXT_USERNAME = "Database User Name";
        private List<string> _additionalUsers = new List<string>();
        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_DESCRIPTION = "Description";

        private DataTable _dtOrginalComponentPrivileges = new DataTable();
        private bool isInitializing = false;
        private bool showControlBox = false;
        private bool showMenuStrip = false;

        TrafodionProgressUserControl _progressControl = null;

        #endregion

        #region Properties
        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _connectionDefinition;
            }
            set
            {
                _connectionDefinition = value;
            }
        }
        
        public List<string> AdditionalUsers
        {
            get
            {
                return _additionalUsers;
            }

            set
            {
                if (value != null)
                {
                    _additionalUsers = value;
                    BindListDataToGrid();
                    UpdateControls();
                }
            }
        }
        #endregion

        #region Constructor
        public AlterRoleUserControl(ConnectionDefinition aConnectionDefinition, DataTable aDataTable, int rowIndex)
        {
            InitializeComponent();
            isInitializing = true;
            _connectionDefinition = aConnectionDefinition;
            _dataTable = aDataTable;
            this._rowIndex = rowIndex;

            _roleNameTextBox.Text = aDataTable.Rows[rowIndex][0].ToString();
            _grantedByComboBox.Text = string.Empty;
            _roleName = _dataTable.Rows[rowIndex].ItemArray[0] as string;
            _grantCountTextBox.Text = ((long)_dataTable.Rows[rowIndex].ItemArray[2]).ToString();
            _defaultRoleCountTextBox.Text = ((long)_dataTable.Rows[rowIndex].ItemArray[3]).ToString();
            string ownerName = _dataTable.Rows[rowIndex].ItemArray[4] as string;
            _createdByTextBox.Text = string.IsNullOrEmpty(ownerName) ? "" : ownerName.Trim();
            long createTime = (long)_dataTable.Rows[rowIndex].ItemArray[5];
            _createdTimeTextBox.Text = Trafodion.Manager.Framework.Utilities.FormattedJulianTimestamp(createTime, "Unavailable");

            _usersGrid.Cols.Add(COL_KEY_USERNAME, COL_TEXT_USERNAME);
            //_usersGrid.Cols.Add(COL_KEY_USERID, COL_TEXT_USERID);
           // _usersGrid.Cols.Add(COL_KEY_EXTERNALUSERNAME, COL_TEXT_EXTERNALUSERNAME);
            _usersGrid.SelectionChanged += new EventHandler(_usersGrid_SelectionChanged);
            _usersGrid.AllowColumnFilter = false;


            this.Load += new EventHandler(ShowProgressBar);
        }
        #endregion

        #region Events        

        /// <summary>
        /// Show progress bar and try to fetch data
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ShowProgressBar(object sender, EventArgs e)
        {
            Role roleModel = Role.FindSystemModel(ConnectionDefinition);
            Object[] parameters = new Object[] { _roleName };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.LoadingRoleDetailsDialogMsg, roleModel, "GetRoleUsersAndPrivileges", parameters);


            // Hide/disable some controls to avoid unexpect actions     
            this._progressPanel.Visible = true;
            this._theSplitContainer.Visible = false;
            this._resetButton.Enabled = false;
            this._applyButton.Enabled = false;
            this.cancelButton.Enabled = false;
            if (this.isInitializing)
            {
                this._headerPanel.Visible = false;
            }

            // Backup dialog setting, and hide menu bar and control box
            this.showControlBox = this.ParentForm.ControlBox;
            this.ParentForm.ControlBox = false;
            HideMenuStrip();

            _progressPanel.Controls.Clear();
            _progressControl = new TrafodionProgressUserControl(args);
            _progressControl.ProgressCompletedEvent += GetGrantedUsersAndPrivileges_Completed;
            _progressControl.Dock = DockStyle.Fill;
            _progressPanel.Controls.Add(_progressControl);
        }



        private void GetGrantedUsersAndPrivileges_Completed(object sender, TrafodionProgressCompletedArgs e)
        {
            if (this.isInitializing) 
            {
                this.isInitializing = false;

                /*
                * Compatibility for M6 & M7-
                * Only show the Grantor combo box for M7
                */
                if (this.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    InitializeGrantor();
                }
                else
                {
                    gbxGrantor.Visible = false;
                }
            }

            this._progressControl.ProgressCompletedEvent -= GetGrantedUsersAndPrivileges_Completed;

            // Restore the dialog's state
            this.ParentForm.ControlBox = this.showControlBox;
            RestoreMenuStrip();

            if (_progressControl.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), _progressControl.Error.Message, "Error finding user(s)",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);

                return;
            }

            Hashtable htResult = (Hashtable)e.ReturnValue;

            DataTable resultsUsersTable = new DataTable();
            DataTable resultsPrivilegesTable = new DataTable();

            if (htResult.Contains("USERS"))
            {
                resultsUsersTable = (DataTable)htResult["USERS"];
            }

            _arrayOrginalUsers = new ArrayList();
            if (resultsUsersTable != null && resultsUsersTable.Rows.Count > 0)
            {
                foreach (DataRow dr in resultsUsersTable.Rows)
                {
                    this._arrayOrginalUsers.Add(dr[COL_KEY_USERNAME].ToString());
                }
            }
            _usersGrid.FillWithData(resultsUsersTable, true);
            _usersGrid.AutoResizeCols = true;
            TransferDataTableToListData(resultsUsersTable);

            #region   Component Privilege
            _theComponentPrivilegesUserControl.CaptionForSelectionDialog = "Alter Role";
            _theComponentPrivilegesUserControl.ConnectionDefinition = _connectionDefinition;
            _theComponentPrivilegesUserControl.GranteeName = _roleName;
            _theComponentPrivilegesUserControl.OnChangedImpl += new ComponentPrivilegesUserControl.OnChanged(_theComponentPrivilegesUserControl_OnChangedImpl);
            if (htResult.Contains("PRIVILEGES"))
            {
                resultsPrivilegesTable = (DataTable)htResult["PRIVILEGES"];
                //Save a copy of privileges for the user.use for compare to check update.
                _dtOrginalComponentPrivileges = resultsPrivilegesTable.Copy();
                _theComponentPrivilegesUserControl.OriginalComponentPrivilegesTable = _dtOrginalComponentPrivileges;



                /*
                * Compatibility for M6 & M7-
                * For M6, it should call ShowComponentsPrivileges(resultsPrivilegesTable, false)
                * For M7, it should call ShowComponentsPrivileges(resultsPrivilegesTable)
                * In the future, it should only call ShowComponentsPrivileges(resultsPrivilegesTable),
                * And remove this one ShowComponentsPrivileges(resultsPrivilegesTable, false)
                */
                if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    _theComponentPrivilegesUserControl.ShowComponentsPrivileges(resultsPrivilegesTable, false);
                }
                else
                {
                    _theComponentPrivilegesUserControl.ShowComponentsPrivileges(resultsPrivilegesTable);
                }
            }
            #endregion

            // Restore the controls' state
            this._progressPanel.Visible = false;
            this._headerPanel.Visible = true;
            this._theSplitContainer.Visible = true;
            this.cancelButton.Enabled = true;

            UpdateControls();
        }

        /// <summary>
        /// Users selection changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _usersGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        /// <summary>
        /// Cancel button Click will return to window.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        /// <summary>
        /// Show help page about this function.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseAlterRoleDialog);
        }
        /// <summary>
        /// Click add user button will show up a select users dialog.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void addUserToolStripButton_Click(object sender, EventArgs e)
        {
            showSelectUsersDialog();
        }
        /// <summary>
        /// delete selected row(s).
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void deleteUserToolStripButton_Click(object sender, EventArgs e)
        {
            DeleteRow();
        }

        private void _applyButton_Click(object sender, EventArgs e)
        {
            if (AlterRole())
            {
                Close();
            }
        }

        private void _theRemoveAllRoleStripButton_Click(object sender, EventArgs e)
        {
            _usersGrid.Rows.Clear();
            _additionalUsers.Clear();
            UpdateControls();
        }

        private void _resetButton_Click(object sender, EventArgs e)
        {
            _additionalUsers.Clear();
            _grantedByComboBox.Text = string.Empty;
            for (int i = 0; i < _arrayOrginalUsers.Count; i++)
            {
                _additionalUsers.Add(_arrayOrginalUsers[i].ToString());
            }
            BindListDataToGrid();
            //Bind Privilges            
            DataTable dtPrivileges = _dtOrginalComponentPrivileges.Copy();
            _theComponentPrivilegesUserControl.ClearPrivilegeCommand();
            _theComponentPrivilegesUserControl.OriginalComponentPrivilegesTable = _dtOrginalComponentPrivileges;
            
            /*
            * Compatibility for M6 & M7-
            * For M6, it should call ShowComponentsPrivileges(resultsPrivilegesTable, false)
            * For M7, it should call ShowComponentsPrivileges(resultsPrivilegesTable)
            * In the future, it should only call ShowComponentsPrivileges(resultsPrivilegesTable),
            * And remove this one ShowComponentsPrivileges(resultsPrivilegesTable, false)
            */
            if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _theComponentPrivilegesUserControl.ShowComponentsPrivileges(dtPrivileges, false);
            }
            else
            {
                _theComponentPrivilegesUserControl.ShowComponentsPrivileges(dtPrivileges);
            }

            UpdateControls();
        }

        private void _sqlPrivilegesButton_Click(object sender, EventArgs e)
        {
            // Define the query expression.
            IEnumerable<string> roleNames =
                from DataRow row in _dataTable.Rows
                select row[0] as string;
            roleNames.ToList();
            AllSQLPrivilegesUserControl privilegesControl = new AllSQLPrivilegesUserControl(WorkMode.RoleMode, _connectionDefinition, roleNames.ToList(), _rowIndex);
            Utilities.LaunchManagedWindow(Properties.Resources.ShowSQLPrivilegesDialogTitle + " - Role Name: " + roleNames.ToList()[_rowIndex], privilegesControl, _connectionDefinition, privilegesControl.Size, true);
        }

        private void _grantedByComboBox_TextChanged(object sender, EventArgs e)
        {
            if (_grantedByComboBox.Text.Trim().Length > 0)
            {
                _resetButton.Enabled = true;
            }
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Initialize the Grantor combo box
        /// Load all roles of current user as grantors
        /// </summary>
        private void InitializeGrantor()
        {
            TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(this._connectionDefinition);
            _grantedByComboBox.DataSource = sqlMxSystem.RolesForCurrentUser;
            _grantedByComboBox.SelectedIndex = -1;
            _toolTip.SetToolTip(_grantedByComboBox, global::Trafodion.Manager.Properties.Resources.GrantedByToolTip);
        }

        /// <summary>
        /// Get GRANTED BY clause
        /// </summary>
        /// <returns></returns>
        private string GetGrantedByClause()
        {
            const string GRANTED_BY = " GRANTED BY {0} ";
            string grantor = Utilities.ExternalUserName(_grantedByComboBox.Text.Trim());
            if (grantor.Length > 0)
            {
                return string.Format(GRANTED_BY, grantor);
            }
            else
            {
                return string.Empty;
            }
        }

        /// <summary>
        /// Hide MenuStrip to avoid unexpect actions when showing the progress bar
        /// </summary>
        private void HideMenuStrip()
        {
            foreach (Control ctrl in this.ParentForm.Controls)
            {
                MenuStrip menuStrip = ctrl as MenuStrip;
                if (menuStrip != null)
                {
                    this.showMenuStrip = menuStrip.Visible;
                    menuStrip.Visible = false;
                    break;
                }
            }
        }

        /// <summary>
        /// Restore the MenuStrip to its original state
        /// </summary>
        private void RestoreMenuStrip()
        {
            foreach (Control ctrl in this.ParentForm.Controls)
            {
                MenuStrip menuStrip = ctrl as MenuStrip;
                if (menuStrip != null)
                {
                    menuStrip.Visible = this.showMenuStrip;
                    break;
                }
            }
        }

        /// <summary>
        /// Update user input and buttons status.
        /// </summary>
        private void UpdateControls()
        {
            deleteUserToolStripButton.Enabled = _usersGrid.SelectedRowIndexes.Count > 0;
            _arrayGrantingUsers = new ArrayList();
            _arrayRevokingUsers = new ArrayList();
            ArrayList tempUnChangedList = new ArrayList();
            foreach (iGRow igr in this._usersGrid.Rows)
            {
                string tmpRole = igr.Cells["USER_NAME"].Value.ToString();
                if (!_arrayOrginalUsers.Contains(tmpRole) && !_arrayGrantingUsers.Contains(tmpRole))
                {
                    _arrayGrantingUsers.Add(tmpRole);
                }
                else
                {
                    tempUnChangedList.Add(tmpRole);
                }
            }

            for (int i = 0; i < _arrayOrginalUsers.Count; i++)
            {
                if (!tempUnChangedList.Contains(_arrayOrginalUsers[i].ToString()) && !_arrayRevokingUsers.Contains(_arrayOrginalUsers[i].ToString()))
                {
                    _arrayRevokingUsers.Add(_arrayOrginalUsers[i].ToString());
                }
            }
            this._applyButton.Enabled = _arrayGrantingUsers.Count > 0
                || _arrayRevokingUsers.Count > 0
                || _theComponentPrivilegesUserControl.PrivilegesChanged;
            this._resetButton.Enabled = this._applyButton.Enabled || _grantedByComboBox.Text.Trim().Length > 0;

        }

        private void _theComponentPrivilegesUserControl_OnChangedImpl()
        {
            UpdateControls();
        }

        private void TransferDataTableToListData(DataTable dtData)
        {
            _additionalUsers = new List<string>();
            string userName = "";
            foreach (DataRow dr in dtData.Rows)
            {
                userName = dr[COL_KEY_USERNAME].ToString();
                _additionalUsers.Add(userName);
            }
        }

        /// <summary>
        /// Display list data with grid on list data being set.
        /// </summary>
        private void BindListDataToGrid()
        {
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(COL_KEY_USERNAME, System.Type.GetType("System.String"));
            foreach (string strUser in _additionalUsers)
            {
                DataRow dr = dataTable.NewRow();
                dr[COL_KEY_USERNAME] = strUser;
               
                dataTable.Rows.Add(dr);
            }
            _usersGrid.FillWithData(dataTable, true);
        }

        /// <summary>
        /// Close the dialog.
        /// </summary>
        private void Close()
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }


        /// <summary>
        /// Delete a row
        /// </summary>
        private void DeleteRow()
        {

            if (_usersGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _usersGrid.SelectedRowIndexes)
                {
                    string userName = _usersGrid.Rows[_usersGrid.SelectedRowIndexes[0]].Cells[COL_KEY_USERNAME].Value.ToString();
                    _usersGrid.Rows.RemoveAt(_usersGrid.SelectedRowIndexes[0]);
                    _additionalUsers.Remove(userName);
                }
            }

            UpdateControls();
        }

        /// <summary>
        /// Show selecting user dialog for click Add users button.
        /// </summary>
        private void showSelectUsersDialog()
        {
            if (userSelectionPanel == null)
            {
                userSelectionPanel = new UserSelectionPanel(this._roleName,ConnectionDefinition);
            }

            userSelectionPanel.AdditionalUsers = AdditionalUsers;

            ManageUserDialog dialog = new ManageUserDialog();
            dialog.ShowControl(userSelectionPanel, "Alter Role : Select User");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (userSelectionPanel.AdditionalUsers.Count > 0)
                {
                    AdditionalUsers = userSelectionPanel.AdditionalUsers;
                }
            }
        }
 
        public bool AlterRole()
        {

            Role roleModel = Role.FindSystemModel(ConnectionDefinition);
            string grantedByClause = GetGrantedByClause();
            
            DataTable resultsTable = new DataTable();
            Object[] parameters = new Object[] { this._arrayGrantingUsers, 
                this._arrayRevokingUsers, 
                this._roleName, 
                _theComponentPrivilegesUserControl.GetRevokePrivilegesCommand(grantedByClause), 
                _theComponentPrivilegesUserControl.GetAlterPrivilegesCommand(grantedByClause) };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.AlterRoleProcessing, roleModel, "AlterRoleUsersAndPrivileges", parameters);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();
            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error registering user",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                resultsTable = (DataTable)progressDialog.ReturnValue;
            }

            int successCount = roleModel.GetSuccessRowCount(resultsTable);
            int failureCount = roleModel.GetFailureRowCount(resultsTable);

            string infoMsg = "";
            Icon iconType = System.Drawing.SystemIcons.Information;
            if (successCount > 0 && failureCount == 0)
            {
                iconType = System.Drawing.SystemIcons.Information;
                infoMsg = Properties.Resources.OperationCompleted;
            }
            else if (successCount == 0 && failureCount > 0)
            {
                iconType = System.Drawing.SystemIcons.Error;
                infoMsg = Properties.Resources.OperationFailed;
            }
            else
            {
                iconType = System.Drawing.SystemIcons.Warning;
                infoMsg = Properties.Resources.SomeOperationsFailded;
            }

            TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(string.Format(infoMsg, "Altering the role(s)"), resultsTable, iconType);
            mmd.ShowDialog();                         

            if (successCount > 0)
            {
                if (OnSuccessImpl != null)
                {
                    OnSuccessImpl();
                }
            }
            //return status, so dialog can be closed.
            return (successCount > 0);
        }
        #endregion

    }
}
