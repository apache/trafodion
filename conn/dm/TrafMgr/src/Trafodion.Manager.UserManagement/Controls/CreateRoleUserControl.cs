//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2015 Hewlett-Packard Development Company, L.P.
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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UserManagement.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class CreateRoleUserControl : UserControl
    {
        #region Fields
        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        private bool showPlatformPanel = false;
        private ConnectionDefinition _theConnectionDefinition;

        private const string ROLE_NAME = "Role Name";
        private const string ROW_NUMBER = "Row #";
        private const string COL_KEY_USERNAME = "USER_NAME";
        private const string COL_TEXT_USERNAME = "Database User Name";
        
        private MenuItem _theAddRowMenu = null;
        private MenuItem _theDeleteRowMenu = null;

        private int _iCurrentEditCellColIndex = -1;
        private int _iCurrentEditCellRowIndex = -1;
        private string _sCellText = "";
        private UserSelectionPanel userSelectionPanel = null;
        private List<string> _additionalUsers = new List<string>();

        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_DESCRIPTION = "Description";
        private List<string> _listPrivilegesCmd = new List<string>();

        #endregion

        #region properties
        public ConnectionDefinition ConnectionDefinition
        {
            get
            {
                return _theConnectionDefinition;
            }
            set
            {
                _theConnectionDefinition = value;
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
                    updateUserControls();
                }
            }
        }

        #endregion

        #region Constructors
        public CreateRoleUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();

            ConnectionDefinition = aConnectionDefinition;
            theRolesMappingGrid.BeginUpdate();
            theRolesMappingGrid.AutoResizeCols = true;
            //theRolesMappingGrid.AutoWidthColMode = TenTec.Windows.iGridLib.iGAutoWidthColMode.HeaderAndCells;
            theRolesMappingGrid.ReadOnly = false;
            theRolesMappingGrid.RowMode = false;
            theRolesMappingGrid.SelectionMode = iGSelectionMode.MultiExtended;
            theRolesMappingGrid.Cols.Add(ROW_NUMBER, ROW_NUMBER);
            theRolesMappingGrid.Cols.Add(ROLE_NAME, ROLE_NAME);  
            
            theRolesMappingGrid.RowHeader.Visible = true;
            theRolesMappingGrid.RowHeader.UseXPStyles = true;
            theRolesMappingGrid.CustomDrawCellForeground += new iGCustomDrawCellEventHandler(theRolesMappingGrid_CustomDrawCellForeground);

            theRolesMappingGrid.Cols[ROW_NUMBER].CellStyle.CustomDrawFlags = iGCustomDrawFlags.Foreground;
            theRolesMappingGrid.Cols[ROW_NUMBER].Width = 50;
            theRolesMappingGrid.Cols[ROW_NUMBER].CellStyle.ReadOnly = iGBool.True;
            theRolesMappingGrid.Cols[ROW_NUMBER].IncludeInSelect = false;
            theRolesMappingGrid.Cols[ROW_NUMBER].AllowMoving = false;
            //theRolesMappingGrid.Cols[ROLE_NAME].ColHdrStyle.ForeColor = Color.Blue;
            //theRolesMappingGrid.Cols[ROLE_NAME].Width = Convert.ToInt32(theRolesMappingGrid.Width*0.8);
            theRolesMappingGrid.FrozenArea.ColCount = 1;
            theRolesMappingGrid.SearchAsType.Mode = iGSearchAsTypeMode.None;

            theRolesMappingGrid.KeyDown += new KeyEventHandler(theRolesMappingGrid_KeyDown);

            _theAddRowMenu = new MenuItem("Add Row");
            _theAddRowMenu.Click += AddRowMenu_Click;
            _theDeleteRowMenu = new MenuItem("Delete Row(s)");
            _theDeleteRowMenu.Click += DeleteRowMenu_Click;
            theRolesMappingGrid.ContextMenu = new ContextMenu();
            theRolesMappingGrid.ContextMenu.MenuItems.Add(_theAddRowMenu);
            theRolesMappingGrid.ContextMenu.MenuItems.Add(_theDeleteRowMenu);
            theRolesMappingGrid.ContextMenu.Popup += ContextMenu_Popup;
            theRolesMappingGrid.Disposed += new EventHandler(theRolesMappingGrid_Disposed);
            theRolesMappingGrid.DoubleClickHandler = HandleDoubleClient;
            theRolesMappingGrid.SelectionChanged += new EventHandler(theRolesMappingGrid_SelectionChanged);
            theRolesMappingGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(theRolesMappingGrid_AfterCommitEdit);

            theRolesMappingGrid.TextBoxTextChanged += new iGTextBoxTextChangedEventHandler(theRolesMappingGrid_TextBoxTextChanged);
            this.Resize += new System.EventHandler(this.CreateRoleUserControl_Resize);

            AddRow();
            theRolesMappingGrid.EndUpdate();
            UpdateControls();

            _theUsersGrid.Cols.Add(COL_KEY_USERNAME, COL_TEXT_USERNAME);
            _theUsersGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(_theUsersGrid_AfterCommitEdit);
            _theUsersGrid.SelectionChanged += new EventHandler(_theUsersGrid_SelectionChanged);
            _theUsersGrid.AutoResizeCols = true;
            _theUsersGrid.ReadOnly = true;
            _theUsersGrid.AllowColumnFilter = false;
            _theUsersGrid.SelectionMode = iGSelectionMode.MultiExtended;

            #region   Component Privilege
            _theComponentPrivilegesUserControl.CaptionForSelectionDialog = "Create Role";
            _theComponentPrivilegesUserControl.ConnectionDefinition = _theConnectionDefinition;
            _theComponentPrivilegesUserControl.GranteeName = "";            
            DataTable privilegesTable = new DataTable();
            privilegesTable.Columns.Add(COL_COMPONENT, typeof(string));
            privilegesTable.Columns.Add(COL_PRIVILEGES, typeof(string));
            privilegesTable.Columns.Add(COL_WITHGRANTOPTION, typeof(Boolean));
            privilegesTable.Columns.Add(COL_DESCRIPTION, typeof(string));


            /*
            * Compatibility for M6 & M7-
            * For M6, it should call ShowComponentsPrivileges(resultsPrivilegesTable, false)
            * For M7, it should call ShowComponentsPrivileges(resultsPrivilegesTable)
            * In the future, it should only call ShowComponentsPrivileges(resultsPrivilegesTable),
            * And remove this one ShowComponentsPrivileges(resultsPrivilegesTable, false)
            */
            if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _theComponentPrivilegesUserControl.ShowComponentsPrivileges(privilegesTable, false);
                gbxGrantor.Visible = false;
            }
            else
            {
                _theComponentPrivilegesUserControl.ShowComponentsPrivileges(privilegesTable);
                InitializeGrantor();
            }

            #endregion
        }
        

        #endregion

        #region Events
        /// <summary>
        /// Save user input cell information,after button click add Row will restore input info.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theRolesMappingGrid_TextBoxTextChanged(object sender, iGTextBoxTextChangedEventArgs e)
        {
            theRolesMappingGrid.CurCell.ValueType = Type.GetType("System.String");
            this._sCellText = theRolesMappingGrid.TextBox.Text.ToString();
            this._iCurrentEditCellColIndex = theRolesMappingGrid.CurCell.ColIndex;
            this._iCurrentEditCellRowIndex = theRolesMappingGrid.CurCell.RowIndex;
        }

        /// <summary>
        /// update controls after Selection Changed 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theRolesMappingGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        /// <summary>
        /// update controls after Edited
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theRolesMappingGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            UpdateControls();
        }

        /// <summary>
        /// update controls after Selection Changed 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theUsersGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateUserControls();
        }

        /// <summary>
        /// update controls after Edited
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theUsersGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            updateUserControls();
        }

        /// <summary>
        /// Draw the row numbers for the grid control.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theRolesMappingGrid_CustomDrawCellForeground(object sender, iGCustomDrawCellEventArgs e)
        {
            if (e.RowIndex >= 0)
            {
                int rowIndex = e.RowIndex + 1;
                if (e.ColIndex == 0)
                {
                    // Draw the row numbers.
                    e.Graphics.FillRectangle(SystemBrushes.Control, e.Bounds);
                    e.Graphics.DrawString(
                        rowIndex.ToString(),
                        Font,
                        SystemBrushes.ControlText,
                        new Rectangle(e.Bounds.X + 2, e.Bounds.Y, e.Bounds.Width - 4, e.Bounds.Height));
                }
            }
        }

        private void HandleDoubleClient(int rowIndex)
        {
            //No need for double click handling
        }

        /// <summary>
        /// To unregister event handlers
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theRolesMappingGrid_Disposed(object sender, EventArgs e)
        {
            if (_theDeleteRowMenu != null)
            {
                _theDeleteRowMenu.Click -= DeleteRowMenu_Click;
            }

            if (_theAddRowMenu != null)
            {
                _theAddRowMenu.Click -= AddRowMenu_Click;
            }

            if (theRolesMappingGrid != null && theRolesMappingGrid.ContextMenu != null)
            {
                theRolesMappingGrid.ContextMenu.Popup -= ContextMenu_Popup;
            }
        }

        /// <summary>
        /// Enable/disable delete item jsut before the menu is popped. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ContextMenu_Popup(object sender, EventArgs e)
        {
            if (null == theRolesMappingGrid.CurCell)
            {
                _theDeleteRowMenu.Enabled = false;
            }
            else
            {
                _theDeleteRowMenu.Enabled = theRolesMappingGrid.SelectedRowIndexes.Count > 0; 
            }
        }
        /// <summary>
        /// Press Tab key to add new row,DEL key to del rows.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theRolesMappingGrid_KeyDown(object sender, KeyEventArgs e)
        {
            if (null != theRolesMappingGrid.CurCell)
            {
                if (theRolesMappingGrid.CurCell.RowIndex<theRolesMappingGrid.Rows.Count-1)
                {                   
                }
                else if (theRolesMappingGrid.CurCell.Col.Text.Equals(ROLE_NAME) && (e.KeyCode == Keys.Tab))
                {
                    theRolesMappingGrid.BeginUpdate();
                    iGRow row = theRolesMappingGrid.Rows.Add();
                    row.Cells[ROLE_NAME].Selected = true;
                    theRolesMappingGrid.EndUpdate();
                }
                else if (e.KeyCode == Keys.Delete)
                {
                    if (theRolesMappingGrid.Rows.Count == 1)
                    {
                        theRolesMappingGrid.Rows.RemoveAt(theRolesMappingGrid.CurCell.RowIndex);
                        theRolesMappingGrid.Rows.Add();
                    }
                    else
                    {
                        theRolesMappingGrid.Rows.RemoveAt(theRolesMappingGrid.CurCell.RowIndex);
                    }
                }
            }
        }

        /// <summary>
        /// Event handler for add context menu
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AddRowMenu_Click(object sender, EventArgs e)
        {
            AddRow();
        }

        /// <summary>
        /// Event handler for delete context menu
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DeleteRowMenu_Click(object sender, EventArgs e)
        {
            DeleteRow();
        }

        /// <summary>
        /// submit roles creating
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _okButton_Click(object sender, EventArgs e)
        {
            if (AddRoles())
            {
                if (OnSuccessImpl != null)
                {
                    OnSuccessImpl();
                }
                Close();
            }
        }

        /// <summary>
        /// press Cancel to return window.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        /// <summary>
        /// Show Help for Creating roles.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseCrRoleDB);
        }

        /// <summary>
        /// process Add strip button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theAddRoleStripButton_Click(object sender, EventArgs e)
        {
            AddRow();
            //Restore information for not saved user input.
            if (_iCurrentEditCellRowIndex >= 0 && _iCurrentEditCellColIndex >= 0 && !_sCellText.Equals(string.Empty))
            {
                theRolesMappingGrid.Cells[_iCurrentEditCellRowIndex, _iCurrentEditCellColIndex].Value = _sCellText;
            }
        }
        /// <summary>
        /// process Remove strip button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theDeleteRoleStripButton_Click(object sender, EventArgs e)
        {
            DeleteRow();
            ResetUserInputInfo();
        }
        /// <summary>
        /// process Remove All strip button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theRemoveAllStripButton_Click(object sender, EventArgs e)
        {
            ClearAllRows();
            ResetUserInputInfo();
        }

        private void CreateRoleUserControl_Resize(object sender, EventArgs e)
        {
            if (theRolesMappingGrid.Cols.KeyExists(ROW_NUMBER))
            {
                theRolesMappingGrid.Cols[ROW_NUMBER].Width = 50;
            }
        }

        private void addUserToolStripButton_Click(object sender, EventArgs e)
        {
            showSelectUsersDialog();
        }



        private void RemoveUserToolStripButton_Click(object sender, EventArgs e)
        {
            DeleteUserRow();
            updateUserControls();
        }

        private void _theRemoveAllUsersStripButton_Click(object sender, EventArgs e)
        {
            this._theUsersGrid.Rows.Clear();
            this._additionalUsers.Clear();
            updateUserControls();
        }
        #endregion

        #region private functions
        
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
        /// Initialize the Grantor combo box
        /// Load all roles of current user as grantors
        /// </summary>
        private void InitializeGrantor()
        {
            TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(this._theConnectionDefinition);
            _grantedByComboBox.DataSource = sqlMxSystem.RolesForCurrentUser;
            _grantedByComboBox.SelectedIndex = -1;
            _toolTip.SetToolTip(_grantedByComboBox, global::Trafodion.Manager.Properties.Resources.GrantedByToolTip);
        }

        /// <summary>
        /// Delete a row
        /// </summary>
        private void DeleteUserRow()
        {
            if (_theUsersGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _theUsersGrid.SelectedRowIndexes)
                {
                    string userName = _theUsersGrid.Rows[_theUsersGrid.SelectedRowIndexes[0]].Cells[COL_KEY_USERNAME].Value.ToString();
                    _theUsersGrid.Rows.RemoveAt(_theUsersGrid.SelectedRowIndexes[0]);
                    _additionalUsers.Remove(userName);
                }
            }
        }

        /// <summary>
        /// Show selecting user dialog for click Add users button.
        /// </summary>
        private void showSelectUsersDialog()
        {
            if (userSelectionPanel == null)
            {
                userSelectionPanel = new UserSelectionPanel(ConnectionDefinition);
            }

            userSelectionPanel.AdditionalUsers = AdditionalUsers;

            ManageUserDialog dialog = new ManageUserDialog();
            dialog.ShowControl(userSelectionPanel, "Create Role : Select User");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (userSelectionPanel.AdditionalUsers.Count > 0)
                {
                    AdditionalUsers = userSelectionPanel.AdditionalUsers;
                }
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
            _theUsersGrid.FillWithData(dataTable, true);
        }

        /// <summary>
        /// Used for reset user input information
        /// </summary>
        private void ResetUserInputInfo()
        {
            _iCurrentEditCellColIndex = -1;
            _iCurrentEditCellRowIndex = -1;
            _sCellText = "";
        }

        /// <summary>
        /// Update input role name and operation buttons status.
        /// </summary>
        private void UpdateControls()
        {
            errorLabel.Text = "";
            bool nonEmptyRowFound = false;
            for (int i = 0; i < theRolesMappingGrid.Rows.Count; i++)
            {
                string roleName = theRolesMappingGrid.Rows[i].Cells[ROLE_NAME].Value as string;
                if (string.IsNullOrEmpty(roleName) )
                {
                    errorLabel.Text = "The Role Name in row (" + (i + 1) + ") is required";
                    break;
                }
                else
                {
                    if (!string.IsNullOrEmpty(roleName))
                    {
                        nonEmptyRowFound = true;
                    }
                }
            }
            _okButton.Enabled = nonEmptyRowFound && string.IsNullOrEmpty(errorLabel.Text);
            _theDeleteRoleStripButton.Enabled = theRolesMappingGrid.SelectedRowIndexes.Count > 0;
            _theRemoveAllStripButton.Enabled = theRolesMappingGrid.Rows.Count > 0;

        }

        /// <summary>
        /// Update user grid control status
        /// </summary>
        private void updateUserControls()
        {
            RemoveUserToolStripButton.Enabled = _theUsersGrid.SelectedRows.Count > 0;
        }
        /// <summary>
        /// Create role by the Role management model
        /// </summary>
        /// <returns></returns>
        private bool AddRoles()
        {
            Role roleSystemModel = Role.FindSystemModel(ConnectionDefinition);
        
            System.Collections.ArrayList roleList = new System.Collections.ArrayList();
            for (int i = 0; i < theRolesMappingGrid.Rows.Count; i++)
            {
                string roleName = theRolesMappingGrid.Rows[i].Cells[ROLE_NAME].Value as string;
                if (!string.IsNullOrEmpty(roleName))
                {
                    roleList.Add(roleName);
                }
                else
                {
                    theRolesMappingGrid.CurRow = theRolesMappingGrid.Rows[i];
                    MessageBox.Show("The Role Name in row (" + (i + 1) + ") is required", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return false;
                }
            }

            //Add the role
            DataTable resultsTable = new DataTable();
            Object[] parameters = new Object[] { roleList, AdditionalUsers,getGrantPrivilegesCommand() };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.CreateRoleMsg1, roleSystemModel, "CreateRoles", parameters);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();
            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error Creating Role(s)",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                resultsTable = (DataTable)progressDialog.ReturnValue;
            }

            int successCount = roleSystemModel.GetSuccessRowCount(resultsTable);
            int failureCount = roleSystemModel.GetFailureRowCount(resultsTable);
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

            TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(string.Format(infoMsg, "Creating the role(s)"), resultsTable, iconType);
            mmd.ShowDialog();     
   
            //Refresh the right panel if needed
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
        /// <summary>
        /// Validation input.
        /// </summary>
        /// <returns></returns>
        public List<string> IsValid()
        {
            List<string> ret = new List<string>();

            for (int i = 0; i < theRolesMappingGrid.Rows.Count; i++)
            {               
                string roleName = theRolesMappingGrid.Rows[i].Cells[ROLE_NAME].Value as string;
                if (!string.IsNullOrEmpty(roleName))
                {
                    ret.Add("The Role Name in row (" + (i + 1) + ") is required");
                }
            }
            return ret;
        }
        
        /// <summary>
        /// Add new row
        /// </summary>
        private void AddRow()
        {
            iGRow row = theRolesMappingGrid.Rows.Add();
            theRolesMappingGrid.CurRow = row;
            theRolesMappingGrid.Invalidate();
            theRolesMappingGrid.Update();
        }
        /// <summary>
        /// Delete a row
        /// </summary>
        private void DeleteRow()
        {
           
            if (theRolesMappingGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in theRolesMappingGrid.SelectedRowIndexes)
                {
                    theRolesMappingGrid.Rows.RemoveAt(theRolesMappingGrid.SelectedRowIndexes[0]);
                    
                }
                if (theRolesMappingGrid.Rows.Count == 0)
                {
                    theRolesMappingGrid.Rows.Add();
                }
                //Clear user input cell value that not be saved.
                _sCellText = string.Empty;
            }
            UpdateControls();
        }
        /// <summary>
        /// Clear all rows and update relevant controls.
        /// </summary>
        private void ClearAllRows()
        {
            theRolesMappingGrid.Rows.Clear();

            if (theRolesMappingGrid.Rows.Count == 0)
            {
                theRolesMappingGrid.Rows.Add();
            }

            UpdateControls();
        }
        /// <summary>
        /// Close this window.
        /// </summary>
        private void Close()
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        /// <summary>
        /// Look up component privilges grid to get grant privilges command.
        /// </summary>
        /// <returns></returns>
        private List<string> getGrantPrivilegesCommand()
        {
            List<string> listReturn = new List<string>();

            Hashtable htGrantPrivilege = new Hashtable();
            Hashtable htGrantWithGrantOption = new Hashtable();

            _theComponentPrivilegesUserControl.GetPrivilegeData(htGrantPrivilege, htGrantWithGrantOption);

            #region Build Command string.
            if (htGrantPrivilege.Count > 0)
            {
                listReturn.AddRange(buildSQLCommand(htGrantPrivilege, "GRANT", "<GRANTEE_LIST>"));
            }

            if (htGrantWithGrantOption.Count > 0)
            {
                listReturn.AddRange(buildSQLCommand(htGrantWithGrantOption, "GRANTWITH", "<GRANTEE_LIST>"));
            }


            #endregion

            return listReturn;
        }//end function

        /// <summary>
        /// Build Grant component privilges command.
        /// </summary>
        /// <param name="htCommand"></param>
        /// <param name="commandType"></param>
        /// <param name="grantee"></param>
        /// <returns></returns>
        private List<string> buildSQLCommand(Hashtable htCommand, string commandType, string grantee)
        {
            List<string> listReturn = new List<string>();
            string grantedByClause = GetGrantedByClause();
            if (htCommand != null)
            {
                bool hasCommand = false;
                StringBuilder cmd = new StringBuilder();
                List<string> privileges;
                string strComp;
                foreach (DictionaryEntry de in htCommand)
                {
                    hasCommand = false;
                    cmd = new StringBuilder();
                    strComp = de.Key.ToString();
                    privileges = (List<string>)de.Value;
                    if (privileges != null && privileges.Count > 0)
                    {
                        hasCommand = true;
                        #region BuildCommand

                        cmd.Append(" GRANT COMPONENT PRIVILEGE ");

                        foreach (string priv in privileges)
                        {
                            cmd.Append(priv);
                            cmd.Append(",");
                        }
                        cmd.Remove(cmd.Length - 1, 1);
                        cmd.Append(" ON ");
                        cmd.Append(strComp);
                        switch (commandType)
                        {
                            case "GRANT":
                                cmd.Append(" TO ");
                                cmd.Append(grantee);
                                break;
                            case "GRANTWITH":
                                cmd.Append(" TO ");
                                cmd.Append(grantee);
                                cmd.Append(" WITH GRANT OPTION ");
                                break;

                        }

                        cmd.Append(grantedByClause);

                        #endregion
                    }

                    if (hasCommand)
                    {
                        listReturn.Add(cmd.ToString());
                    }

                }
            }
            return listReturn;
        }

        #endregion


    }
}
