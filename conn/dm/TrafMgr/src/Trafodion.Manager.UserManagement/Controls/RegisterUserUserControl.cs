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
    public partial class RegisterUserUserControl : UserControl
    {
        #region Fields
        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        private bool showPlatformPanel = false;
        private ConnectionDefinition _theConnectionDefinition;

        private const string DIR_SERVER_USERNAME = "Directory-Service User Name (*)";
        private const string DB_USERNAME = "Database User Name";
        private const string ROW_NUMBER = "Row #";
        private const string AUTH_TYPE = "Authentication";
        private const string USER_IMMUTABLE = "Immutable";
        private MenuItem _theAddRowMenu = null;
        private MenuItem _theDeleteRowMenu = null;

        private const string COL_KEY_ROLENAME = "ROLE_NAME";
        private const string COL_KEY_DEFAULTROLE = "DEFAULT_ROLE";
        private const string COL_TEXT_ROLENAME = "Role Name";
        private const string COL_TEXT_DEFAULTROLE = "Primary Role";
        private const string DEFAULT_ROLE_NONE = "NONE";
        private const string AUTH_OPTION_LOCAL = "Local";
        private const string AUTH_OPTION_REMOTE = "Remote";
        private const string AUTH_OPTION_ENTERPRISE = "Enterprise";
        private const string AUTH_OPTION_CLUSTER = "Cluster";
        private List<string> _listGrantingRoles = new List<string>();
        private RoleSelectionPanel roleSelectionPanel = null;
        private List<string> _additionalRoles = new List<string>();
        iGDropDownList _authenOptionDropDownList = new iGDropDownList();
        iGColPattern _authenOptionColPattern = new iGColPattern();
        //private string _defaultRole = "";
        private string _UpdatedDefaultRole = "";
        private int _iCurrentEditCellColIndex = -1;
        private int _iCurrentEditCellRowIndex = -1;
        private string _sCellText = "";

        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_DESCRIPTION = "Description";
        private List<string> _listPrivilegesCmd = new List<string>();

        #endregion

        #region Properties
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

        public List<string> AdditionalRoles
        {
            get
            {
                return _additionalRoles;
            }

            set
            {
                if (value != null)
                {
                    _additionalRoles = value;
                    BindListDataToGrid();
                }
            }
        }

        private void BindListDataToGrid()
        {
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(COL_KEY_ROLENAME, System.Type.GetType("System.String"));
            dataTable.Columns.Add(COL_KEY_DEFAULTROLE, System.Type.GetType("System.Boolean"));
            foreach (string strRole in _additionalRoles)
            {
                DataRow dr = dataTable.NewRow();
                dr[COL_KEY_ROLENAME] = strRole;
                dr[COL_KEY_DEFAULTROLE] = false; //init it as non-default value
                dataTable.Rows.Add(dr);
            }
            _rolesGrid.FillWithData(dataTable, true);
        }
        #endregion

        #region Constructor

        public RegisterUserUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();

            ConnectionDefinition = aConnectionDefinition;
             theUsersMappingGrid.BeginUpdate();
             //theUsersMappingGrid.AutoWidthColMode = TenTec.Windows.iGridLib.iGAutoWidthColMode.HeaderAndCells;
             theUsersMappingGrid.ReadOnly = false;
             theUsersMappingGrid.RowMode = false ;
             theUsersMappingGrid.SelectionMode = iGSelectionMode.MultiExtended;
             theUsersMappingGrid.Cols.Add(ROW_NUMBER, ROW_NUMBER);
             theUsersMappingGrid.Cols.Add(DIR_SERVER_USERNAME, DIR_SERVER_USERNAME);
             theUsersMappingGrid.Cols.Add(DB_USERNAME, DB_USERNAME);
             //Add the authentication type drop down list     
             if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
             {
                 _authenOptionDropDownList.Items.Add(AUTH_OPTION_ENTERPRISE);
                 _authenOptionDropDownList.Items.Add(AUTH_OPTION_CLUSTER);
             }
             else
             {
                 _authenOptionDropDownList.Items.Add(AUTH_OPTION_LOCAL);
                 _authenOptionDropDownList.Items.Add(AUTH_OPTION_REMOTE);
             }

             CreateIGridColumnPatterns();

             iGCol authCol = theUsersMappingGrid.Cols.Add(AUTH_TYPE, AUTH_TYPE);
             authCol.Pattern = _authenOptionColPattern;
             if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
             {
                 iGCol userImmutable = theUsersMappingGrid.Cols.Add(USER_IMMUTABLE, USER_IMMUTABLE);
                 userImmutable.ColHdrStyle.TextAlign = iGContentAlignment.MiddleCenter;
                 userImmutable.CellStyle.ImageAlign = iGContentAlignment.MiddleCenter;
                 userImmutable.CellStyle.Type = iGCellType.Check;
                 userImmutable.CellStyle.SingleClickEdit = iGBool.True;
                 userImmutable.MinWidth = userImmutable.MaxWidth = 80;
             }
             theUsersMappingGrid.RowHeader.Visible = true;
             theUsersMappingGrid.RowHeader.UseXPStyles = true;
             theUsersMappingGrid.CustomDrawCellForeground += new iGCustomDrawCellEventHandler(theUsersMappingGrid_CustomDrawCellForeground);

             theUsersMappingGrid.Cols[ROW_NUMBER].CellStyle.CustomDrawFlags = iGCustomDrawFlags.Foreground;
             theUsersMappingGrid.Cols[ROW_NUMBER].MinWidth = 60;
             theUsersMappingGrid.Cols[ROW_NUMBER].MaxWidth = 60;
             theUsersMappingGrid.Cols[ROW_NUMBER].CellStyle.ReadOnly = iGBool.True;
             theUsersMappingGrid.Cols[ROW_NUMBER].IncludeInSelect = false;
             theUsersMappingGrid.Cols[ROW_NUMBER].AllowMoving = false;
             theUsersMappingGrid.Cols[DIR_SERVER_USERNAME].ColHdrStyle.ForeColor = Color.Blue;
             theUsersMappingGrid.Cols[DIR_SERVER_USERNAME].Width = Convert.ToInt32(theUsersMappingGrid.Width * 0.4);
             theUsersMappingGrid.Cols[DB_USERNAME].Width = Convert.ToInt32(theUsersMappingGrid.Width * 0.4);
             authCol.MinWidth = authCol.MaxWidth = 130;
                
             theUsersMappingGrid.FrozenArea.ColCount = 2;
             theUsersMappingGrid.SearchAsType.Mode = iGSearchAsTypeMode.None;

            theUsersMappingGrid.KeyDown += new KeyEventHandler(theUsersMappingGrid_KeyDown);

            _theAddRowMenu = new MenuItem("Add Row");
            _theAddRowMenu.Click += AddRowMenu_Click;
            _theDeleteRowMenu = new MenuItem("Delete Row(s)");
            _theDeleteRowMenu.Click += DeleteRowMenu_Click;
            theUsersMappingGrid.ContextMenu = new ContextMenu();
            theUsersMappingGrid.ContextMenu.MenuItems.Add(_theAddRowMenu);
            theUsersMappingGrid.ContextMenu.MenuItems.Add(_theDeleteRowMenu);
            theUsersMappingGrid.ContextMenu.Popup += ContextMenu_Popup;
            theUsersMappingGrid.Disposed += new EventHandler(theUsersMappingGrid_Disposed);
            theUsersMappingGrid.DoubleClickHandler = HandleDoubleClient;
            theUsersMappingGrid.SelectionChanged += new EventHandler(theUsersMappingGrid_SelectionChanged);
            theUsersMappingGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(theUsersMappingGrid_AfterCommitEdit);
            theUsersMappingGrid.TextBoxTextChanged += new iGTextBoxTextChangedEventHandler(theUsersMappingGrid_TextBoxTextChanged);
            theUsersMappingGrid.AutoResizeCols = true;
            if (_theConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ131)
            {
                authCol.Visible = false;
            }
            AddRow();
            theUsersMappingGrid.EndUpdate();
            UpdateControls();

            _rolesGrid.Cols.Add(COL_KEY_ROLENAME, COL_TEXT_ROLENAME);
            iGCol defaultRolCol = _rolesGrid.Cols.Add(COL_KEY_DEFAULTROLE, COL_TEXT_DEFAULTROLE);
            defaultRolCol.CellStyle.Type = iGCellType.Check;
            defaultRolCol.CellStyle.SingleClickEdit = iGBool.True;
            defaultRolCol.ColHdrStyle.TextAlign = iGContentAlignment.MiddleCenter;
            defaultRolCol.CellStyle.ImageAlign = iGContentAlignment.MiddleCenter;
            defaultRolCol.MinWidth = defaultRolCol.MaxWidth = 100;
            _rolesGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(_rolesGrid_AfterCommitEdit);
            _rolesGrid.SelectionChanged += new EventHandler(_rolesGrid_SelectionChanged);
            _rolesGrid.AutoResizeCols = true;
            _rolesGrid.AllowColumnFilter = false;
            _rolesGrid.ReadOnly = false;
            _rolesGrid.SelectionMode = iGSelectionMode.MultiExtended;
            
            #region   Component Privilege
            _theComponentPrivilegesUserControl.CaptionForSelectionDialog = "Register User";
            _theComponentPrivilegesUserControl.ConnectionDefinition = _theConnectionDefinition;
            _theComponentPrivilegesUserControl.GranteeName = "";            
            DataTable privilegesTable = new DataTable();
            privilegesTable.Columns.Add(COL_COMPONENT, typeof(string));
            privilegesTable.Columns.Add(COL_PRIVILEGES, typeof(string));
            privilegesTable.Columns.Add(COL_WITHGRANTOPTION, typeof(Boolean));
            privilegesTable.Columns.Add(COL_DESCRIPTION, typeof(string));
            _theComponentPrivilegesUserControl.ShowComponentsPrivileges(privilegesTable);
            #endregion


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
        

        #endregion

        #region Events
        private void theUsersMappingGrid_TextBoxTextChanged(object sender, iGTextBoxTextChangedEventArgs e)
        {
            theUsersMappingGrid.CurCell.ValueType = Type.GetType("System.String");
            this._sCellText = theUsersMappingGrid.TextBox.Text;
            this._iCurrentEditCellColIndex = theUsersMappingGrid.CurCell.ColIndex;
            this._iCurrentEditCellRowIndex = theUsersMappingGrid.CurCell.RowIndex;
        }

        private void _rolesGrid_SelectionChanged(object sender, EventArgs e)
        {
            updateRoleControls();
        }

        private void _rolesGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            if (e.ColIndex == _rolesGrid.Cols[COL_KEY_DEFAULTROLE].Index)
            {
                if ("true".Equals(_rolesGrid.Rows[e.RowIndex].Cells[e.ColIndex].Value.ToString(), System.StringComparison.InvariantCultureIgnoreCase))
                {
                    foreach (iGRow row in _rolesGrid.Rows)
                    {
                        if (e.RowIndex == row.Index)
                            continue;
                        _rolesGrid.Rows[row.Index].Cells[e.ColIndex].Value = false;
                    }
                    _UpdatedDefaultRole = _rolesGrid.Rows[e.RowIndex].Cells[COL_KEY_ROLENAME].Value.ToString();
                }
                else
                {
                    _UpdatedDefaultRole = DEFAULT_ROLE_NONE;
                }
            }
            updateRoleControls();
        }
        /// <summary>
        /// Update controls status after selection changed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theUsersMappingGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }
        /// <summary>
        /// Update controls status after commit edit.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theUsersMappingGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            UpdateControls();
        }
        /// <summary>
        /// Draw foreground for the Grid.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theUsersMappingGrid_CustomDrawCellForeground(object sender, iGCustomDrawCellEventArgs e)
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
        private void theUsersMappingGrid_Disposed(object sender, EventArgs e)
        {
            if (_theDeleteRowMenu != null)
            {
                _theDeleteRowMenu.Click -= DeleteRowMenu_Click;
            }

            if (_theAddRowMenu != null)
            {
                _theAddRowMenu.Click -= AddRowMenu_Click;
            }

            if (theUsersMappingGrid != null && theUsersMappingGrid.ContextMenu != null)
            {
                theUsersMappingGrid.ContextMenu.Popup -= ContextMenu_Popup;
            }
        }

        /// <summary>
        /// Enable/disable delete item jsut before the menu is popped. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ContextMenu_Popup(object sender, EventArgs e)
        {
            if (null == theUsersMappingGrid.CurCell)
            {
                _theDeleteRowMenu.Enabled = false;
            }
            else
            {
                _theDeleteRowMenu.Enabled = theUsersMappingGrid.SelectedRowIndexes.Count > 0;
            }
        }
        /// <summary>
        /// Process key pressing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theUsersMappingGrid_KeyDown(object sender, KeyEventArgs e)
        {
            if (null != theUsersMappingGrid.CurCell)
            {
                if (theUsersMappingGrid.CurCell.Col.Text.Equals(DIR_SERVER_USERNAME) && (e.KeyCode == Keys.Tab))
                {
                    theUsersMappingGrid.PerformAction(iGActions.GoNextCol);
                }
                if (theUsersMappingGrid.CurCell.Col.Text.Equals(DB_USERNAME) && (e.KeyCode == Keys.Tab))
                {
                    theUsersMappingGrid.BeginUpdate();
                    iGRow row = theUsersMappingGrid.Rows.Add();
                    row.Cells[DIR_SERVER_USERNAME].Selected = true;
                    theUsersMappingGrid.EndUpdate();
                }
                else if (e.KeyCode == Keys.Delete)
                {
                    if (theUsersMappingGrid.Rows.Count == 1)
                    {
                        theUsersMappingGrid.Rows.RemoveAt(theUsersMappingGrid.CurCell.RowIndex);
                        theUsersMappingGrid.Rows.Add();
                    }
                    else
                    {
                        theUsersMappingGrid.Rows.RemoveAt(theUsersMappingGrid.CurCell.RowIndex);
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
            DeleteUserRow();
        }
       /// <summary>
       /// Add new row.
       /// </summary>
       /// <param name="sender"></param>
       /// <param name="e"></param>
        private void _addRowButton_Click(object sender, EventArgs e)
        {
             AddRow();
             //Restore information for not saved user input.
             if (_iCurrentEditCellRowIndex >= 0 && _iCurrentEditCellColIndex >= 0 && !_sCellText.Equals(string.Empty))
             {
                 theUsersMappingGrid.Cells[_iCurrentEditCellRowIndex, _iCurrentEditCellColIndex].Value = _sCellText;
                 UpdateControls();
             }
        }

        /// <summary>
        /// Delete a row
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _deleteRowButton_Click(object sender, EventArgs e)
        {
            DeleteUserRow();
            ResetUserInputInfo();
        }
        /// <summary>
        /// Submit adding user
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _okButton_Click(object sender, EventArgs e)
        {
            if (AddUser())
            {
                Close();
            }
        }
        /// <summary>
        /// Cancel edit and close window.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }
        /// <summary>
        /// click it to open Help document
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Register_Users_Dialog_Box);
        }
        /// <summary>
        /// Clear all rows.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theRemoveAllStripButton_Click(object sender, EventArgs e)
        {
            ClearAllUserRows();
            ResetUserInputInfo();
        }
        #endregion

        #region private Methods

        /// <summary>
        /// Creates the iGrid column pattern for the authentication type column
        /// </summary>
        /// <returns></returns>
        void CreateIGridColumnPatterns()
        {
            iGCellStyle authenOptionStyle = new iGCellStyle();
            authenOptionStyle.DropDownControl = _authenOptionDropDownList;
            authenOptionStyle.Flags = iGCellFlags.DisplayText;
            authenOptionStyle.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            authenOptionStyle.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            authenOptionStyle.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            authenOptionStyle.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            authenOptionStyle.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.NoTextEdit;

            _authenOptionColPattern = new iGColPattern();
            _authenOptionColPattern.CellStyle = authenOptionStyle;
            _authenOptionColPattern.Text = AUTH_TYPE;
            _authenOptionColPattern.Key = AUTH_TYPE;
            _authenOptionColPattern.MinWidth = 60;
            _authenOptionColPattern.Width = 108;
            _authenOptionColPattern.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);


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
        /// Update user input and buttons status.
        /// </summary>
        private void UpdateControls()
        {
            errorLabel.Text = "";
            errorLabel.Visible = false;
            bool nonEmptyRowFound = false;
            for (int i = 0; i < theUsersMappingGrid.Rows.Count; i++)
            {
                string dirServerUserName = theUsersMappingGrid.Rows[i].Cells[DIR_SERVER_USERNAME].Value as string;
                string dbUserName = theUsersMappingGrid.Rows[i].Cells[DB_USERNAME].Value as string;
               
                if (string.IsNullOrEmpty(dirServerUserName) && !string.IsNullOrEmpty(dbUserName))
                {
                    errorLabel.Visible = true;
                    errorLabel.Text = "The Directory-Service User Name in row (" + (i + 1) + ") is required";
                    break;
                }
                else
                {
                    if (!string.IsNullOrEmpty(dirServerUserName))
                    {
                        nonEmptyRowFound = true;
                    }
                }
            }

            _okButton.Enabled = nonEmptyRowFound && string.IsNullOrEmpty(errorLabel.Text);
            _theDeleteUserStripButton.Enabled = theUsersMappingGrid.SelectedRowIndexes.Count > 0;
            _theRemoveAllUserStripButton.Enabled = theUsersMappingGrid.Rows.Count > 0;
        }
         
        /// <summary>
        /// Add user by calling user management Model.
        /// </summary>
        /// <returns></returns>
        private bool AddUser()
        {
            UserMgmtSystemModel userSystemModel = UserMgmtSystemModel.FindSystemModel(ConnectionDefinition);
            ArrayList userList = new ArrayList();
            for (int i = 0; i < theUsersMappingGrid.Rows.Count; i++)
            {
                object userNameCellValue = theUsersMappingGrid.Rows[i].Cells[DIR_SERVER_USERNAME].Value;
                if (userNameCellValue == null)
                {
                    // Skip the rows whose User Name cell is empty
                    continue; 
                }

                string dirServerUserName = userNameCellValue.ToString();
                if (!string.IsNullOrEmpty(dirServerUserName))
                {
                    string userName = string.Empty;
                    if (theUsersMappingGrid.Rows[i].Cells[DB_USERNAME].Value != null)
                    {
                        userName = theUsersMappingGrid.Rows[i].Cells[DB_USERNAME].Value.ToString();
                    }
                    string logonRole = "";
                    if (string.IsNullOrEmpty(userName))
                    {
                        userName = dirServerUserName;
                    }
                    if (string.IsNullOrEmpty(_UpdatedDefaultRole))
                    {
                        logonRole = "NONE";
                    }
                    else
                    {
                        logonRole = _UpdatedDefaultRole;
                    }
                    string authMode = "";
                    if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ131)
                    {
                        if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                        {
                            if (theUsersMappingGrid.Rows[i].Cells[AUTH_TYPE].Value != null &&
                                AUTH_OPTION_CLUSTER.Equals(theUsersMappingGrid.Rows[i].Cells[AUTH_TYPE].Value.ToString(), System.StringComparison.InvariantCultureIgnoreCase))
                            {
                                authMode = "CLUSTER AUTHENTICATION";
                            }
                            else
                            {
                                authMode = "ENTERPRISE AUTHENTICATION";
                            }
                        }
                        else
                        {
                            if (theUsersMappingGrid.Rows[i].Cells[AUTH_TYPE].Value != null &&
                                AUTH_OPTION_REMOTE.Equals(theUsersMappingGrid.Rows[i].Cells[AUTH_TYPE].Value.ToString(), System.StringComparison.InvariantCultureIgnoreCase))
                            {
                                authMode = "REMOTE AUTHENTICATION";
                            }
                            else
                            {
                                authMode = "LOCAL AUTHENTICATION";
                            }
                        }
                    }
                    string immutable = "";
                    if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        if (theUsersMappingGrid.Rows[i].Cells[USER_IMMUTABLE].Value != null &&
                           (bool)theUsersMappingGrid.Rows[i].Cells[USER_IMMUTABLE].Value == true)
                        {
                            immutable = "IMMUTABLE";
                        }
                    }
                    string[] userMapping = { dirServerUserName, userName, logonRole, authMode,immutable };
                    userList.Add(userMapping);
                }
            }

            
            //Add the user
            DataTable resultsTable = new DataTable();
            Object[] parameters = new Object[] { userList, _listGrantingRoles,getGrantPrivilegesCommand() };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.RegisterMsg1, userSystemModel, "RegisterUsers", parameters);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();
            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error registering user(s)",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                resultsTable = (DataTable)progressDialog.ReturnValue;
            }

            int successCount = userSystemModel.GetSuccessRowCount(resultsTable);
            int failureCount=userSystemModel.GetFailureRowCount(resultsTable);
            string infoMsg = "";
            Icon iconType = System.Drawing.SystemIcons.Information;
            if (successCount > 0 && failureCount==0)
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

            TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(string.Format(infoMsg,"Registering the user(s)"), resultsTable, iconType);
            mmd.ShowDialog();

            //Refresh the right pane if needed
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
        /// Add new row
        /// </summary>
        private void AddRow()
        {
            iGRow row = theUsersMappingGrid.Rows.Add();
            theUsersMappingGrid.CurRow = row;
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                row.Cells[AUTH_TYPE].Value =AUTH_OPTION_ENTERPRISE;
            }
            else
            {
                row.Cells[AUTH_TYPE].Value = AUTH_OPTION_LOCAL;
            }
            theUsersMappingGrid.Invalidate();
            theUsersMappingGrid.Update();
        }

        /// <summary>
        /// Delete a row.
        /// </summary>
        private void DeleteUserRow()
        {
            if (theUsersMappingGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in theUsersMappingGrid.SelectedRowIndexes)
                {
                    theUsersMappingGrid.Rows.RemoveAt(theUsersMappingGrid.SelectedRowIndexes[0]);
                }
                if (theUsersMappingGrid.Rows.Count == 0)
                {
                    theUsersMappingGrid.Rows.Add();
                }
                //Clear user input cell value that not be saved.
                _sCellText = string.Empty;
            }
            UpdateControls();
        }

        /// <summary>
        /// Clear all rows.
        /// </summary>
        private void ClearAllUserRows()
        {
            theUsersMappingGrid.Rows.Clear();

            if (theUsersMappingGrid.Rows.Count == 0)
            {
                theUsersMappingGrid.Rows.Add();
            }

            UpdateControls();
        }

        /// <summary>
        /// Close current dialog.
        /// </summary>
        private void Close()
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }
        

 
        private void showRolesDialog(RoleSelectionPanel.RoleSelectionMode aSelectionMode)
        {
            if (roleSelectionPanel == null)
            {
                roleSelectionPanel = new RoleSelectionPanel(_theConnectionDefinition);
            }
            roleSelectionPanel.SelectionMode = aSelectionMode;

            roleSelectionPanel.AdditionalRoles = AdditionalRoles;

            ManageUserDialog dialog = new ManageUserDialog();
            dialog.ShowControl(roleSelectionPanel, "Register User : Select Role");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (roleSelectionPanel.AdditionalRoles.Count > 0)
                {
                    AdditionalRoles = roleSelectionPanel.AdditionalRoles;
                }

            }
        }
        private void deleteRoleToolStripButton_Click(object sender, EventArgs e)
        {
            DeleteRoleRow();
            updateRoleControls();
        }
        /// <summary>
        /// Delete a row
        /// </summary>
        private void DeleteRoleRow()
        {

            if (_rolesGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _rolesGrid.SelectedRowIndexes)
                {
                    string roleName = _rolesGrid.Rows[_rolesGrid.SelectedRowIndexes[0]].Cells[COL_KEY_ROLENAME].Value.ToString();
                    //Primary role has been removed
                    if ("true".Equals(_rolesGrid.Rows[_rolesGrid.SelectedRowIndexes[0]].Cells[COL_KEY_DEFAULTROLE].Value.ToString(), System.StringComparison.InvariantCultureIgnoreCase))
                    {
                        _UpdatedDefaultRole = "";
                    }
                    _rolesGrid.Rows.RemoveAt(_rolesGrid.SelectedRowIndexes[0]);
                    _additionalRoles.Remove(roleName);
                }
            }

            
        }
        private void addRoleToolStripButton_Click(object sender, EventArgs e)
        {
            showRolesDialog(RoleSelectionPanel.RoleSelectionMode.Additional);
            updateRoleControls();
        }

        private void _theRemoveAllRoleStripButton_Click(object sender, EventArgs e)
        {
            this._rolesGrid.Rows.Clear();
            this._additionalRoles.Clear();
            updateRoleControls();
        }

        private void updateRoleControls()
        {
            _listGrantingRoles = new List<string>();

            foreach (iGRow igr in this._rolesGrid.Rows)
            {
                string tmpRole = igr.Cells[COL_KEY_ROLENAME].Value.ToString();
                //If role has been checked as logon role,system should have grant this role to user by default.
                if (!_listGrantingRoles.Contains(tmpRole) && !Convert.ToBoolean(igr.Cells[COL_KEY_DEFAULTROLE].Value))
                {
                    _listGrantingRoles.Add(tmpRole);
                }
            }
            this.deleteRoleToolStripButton.Enabled =_rolesGrid.SelectedRows.Count>0;            
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
                listReturn.AddRange(buildSQLCommand(htGrantPrivilege, "GRANT","<GRANTEE_LIST>"));
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
        private List<string> buildSQLCommand(Hashtable htCommand, string commandType,string grantee)
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

        #region Public Methods
        /// <summary>
        /// Validate user input.
        /// </summary>
        /// <returns></returns>
        public List<string> IsValid()
        {
            List<string> ret = new List<string>();

            for (int i = 0; i < theUsersMappingGrid.Rows.Count; i++)
            {
                string dirServerUserName = theUsersMappingGrid.Rows[i].Cells[DIR_SERVER_USERNAME].Value as string;
                string dbUserName = theUsersMappingGrid.Rows[i].Cells[DB_USERNAME].Value as string;
                //string logonRoleName = theUsersMappingGrid.Rows[i].Cells[LOGON_ROLE].Value as string;
                if (string.IsNullOrEmpty(dirServerUserName) && !string.IsNullOrEmpty(dbUserName))
                {
                    ret.Add("The Directory-Service User Name in row (" + (i + 1) + ") is required");
                }
            }
            return ret;
        }
        #endregion
    }
}
