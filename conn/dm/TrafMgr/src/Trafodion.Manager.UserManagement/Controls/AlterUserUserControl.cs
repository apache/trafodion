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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UserManagement.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class AlterUserUserControl : UserControl
    {
        #region Fields
        private RoleSelectionPanel roleSelectionPanel = null;
        public delegate void OnSuccess();
        public event OnSuccess OnSuccessImpl;

        private bool showPlatformPanel = false;
        private bool showControlBox = false;
        private bool showMenuStrip = false;
        private int dialogHeight = 0;
        private ConnectionDefinition _theConnectionDefinition;
        private ArrayList _arrayOrginalRoles = new ArrayList();
        private ArrayList _arrayGrantingRoles = new ArrayList();
        private ArrayList _arrayRevokingRoles = new ArrayList();
        private string _userName = string.Empty;
        private string _defaultRole = "";
        private String _UpdatedDefaultRole="";
        private bool _isValidUser = false;
        private bool _isImmutableUser = false;
        private string _auth_Type = string.Empty;
        private string _strDirUserName = "";

        private const string DIR_SERVER_USERNAME = "Directory-Service User Name (*)";
        private const string DB_USERNAME = "Database User Name";

        private const string COL_KEY_ROLENAME="ROLE_NAME";
        private const string COL_KEY_DEFAULTROLE="DEFAULT_ROLE";
        private const string COL_TEXT_ROLENAME="Role Name";
        private const string COL_TEXT_DEFAULTROLE = "Primary Role";
        private const string DEFAULT_ROLE_NONE = "NONE";

        private const string AUTH_OPTION_LOCAL = "Local";
        private const string AUTH_OPTION_REMOTE = "Remote";
        private const string AUTH_OPTION_ENTERPRISE = "Enterprise";
        private const string AUTH_OPTION_CLUSTER = "Cluster";

        private List<string> _additionalRoles= new List<string>();
        private DataTable _dtOrginalComponentPrivileges = new DataTable();
        TrafodionProgressUserControl _progressControl = null;
        
        private const string COL_COMPONENT = "Component";
        private const string COL_PRIVILEGES = "Privilege";
        private const string COL_WITHGRANTOPTION = "With Grant Option";
        private const string COL_DESCRIPTION = "Description";
        
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

        public string DefaultRole
        {
            get
            {
                return _defaultRole;
            }
            set
            {
                _defaultRole = (value == null) ? "" : value.Trim();              
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
        #endregion

        #region Constructor
        public AlterUserUserControl(ConnectionDefinition aConnectionDefinition, string directoryServiceUserName, string userName, string validUser, string authentication,string immuteUser)
        {
            InitializeComponent();            

            #region Setup users grid properties
            ConnectionDefinition = aConnectionDefinition;
            _strDirUserName = directoryServiceUserName;
            this._userName = userName;           
            _dirServiceUserNameTextBox.Text = directoryServiceUserName;
            _dbUserNameTextBox.Text = userName;
            _isValidUser = validUser.Equals("Yes");
            _isImmutableUser = immuteUser.Equals("Yes");
            _theValidUser.Checked = _isValidUser;
            _theImmutableUser.Checked = _isImmutableUser;
            //Set authentication option
            _theDropdownAuthenticationType.DropDownStyle = ComboBoxStyle.DropDownList;            

            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _theDropdownAuthenticationType.Items.Add(AUTH_OPTION_ENTERPRISE);
                _theDropdownAuthenticationType.Items.Add(AUTH_OPTION_CLUSTER);
                if (AUTH_OPTION_CLUSTER.Equals(authentication, System.StringComparison.InvariantCultureIgnoreCase))
                {
                    _theDropdownAuthenticationType.Text = AUTH_OPTION_CLUSTER;
                    _auth_Type = AUTH_OPTION_CLUSTER;
                }
                else
                {
                    _theDropdownAuthenticationType.Text = AUTH_OPTION_ENTERPRISE;
                    _auth_Type = AUTH_OPTION_ENTERPRISE;
                }

                _theImmutableUser.Visible = true;
                _toolTip.SetToolTip(_theImmutableUser, Properties.Resources.ImmutableUserTooltips);
                _headerPanel.Height = 131;
            }
            else
            {
                _theDropdownAuthenticationType.Items.Add(AUTH_OPTION_LOCAL);
                _theDropdownAuthenticationType.Items.Add(AUTH_OPTION_REMOTE);
                if (AUTH_OPTION_REMOTE.Equals(authentication, System.StringComparison.InvariantCultureIgnoreCase))
                {
                    _theDropdownAuthenticationType.Text = AUTH_OPTION_REMOTE;
                    _auth_Type = AUTH_OPTION_REMOTE;
                }
                else
                {
                    _theDropdownAuthenticationType.Text = AUTH_OPTION_LOCAL;
                    _auth_Type = AUTH_OPTION_LOCAL;
                }
                _theImmutableUser.Visible = false;
                _headerPanel.Height = 109;
            }   

            _rolesGrid.Cols.Add(COL_KEY_ROLENAME, COL_TEXT_ROLENAME);
            iGCol defaultRolCol = _rolesGrid.Cols.Add(COL_KEY_DEFAULTROLE, COL_TEXT_DEFAULTROLE);
            defaultRolCol.CellStyle.Type = iGCellType.Check;
            defaultRolCol.CellStyle.SingleClickEdit = iGBool.True;
            _rolesGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(_rolesGrid_AfterCommitEdit);
            _rolesGrid.SelectionChanged += new EventHandler(_rolesGrid_SelectionChanged);
            _rolesGrid.AllowColumnFilter = false;
            _dirServiceUserNameTextBox.TextChanged += new EventHandler(_dirServiceUserNameTextBox_TextChanged);
            _theValidUser.CheckedChanged += new EventHandler(_theValidUser_CheckedChanged);
            _theImmutableUser.CheckedChanged += new EventHandler(_theImmutableUser_CheckedChanged);
            _theDropdownAuthenticationType.SelectedIndexChanged += new EventHandler(_theDropdownAuthenticationType_SelectedIndexChanged);
            #endregion

            this.Load += new EventHandler(AlterUserUserControl_Loaded);
        }

      


        
        #endregion
        
        #region Events

        /// <summary>
        /// Show progress bar and try to fetch data
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AlterUserUserControl_Loaded(object sender, EventArgs e)
        {
            // Hide/disable some controls to avoid unexpect actions
            this._headerPanel.Visible = false;
            this._contentPanel.Visible = false;
            this._resetButton.Enabled = false;
            this._applyButton.Enabled = false;
            this.cancelButton.Enabled = false;

            // Backup dialog setting, and hide menu bar and control box
            this.showControlBox = this.ParentForm.ControlBox;
            this.ParentForm.ControlBox = false;
            HideMenuStrip();

            // Show progress bar when trying to fetching user & role data
            UserMgmtSystemModel userModel = UserMgmtSystemModel.FindSystemModel(_theConnectionDefinition);
            Object[] parameters = new Object[] { this._userName };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.AlterRolesLoadingMsg, userModel, "GetUserRolesAndPrivileges", parameters);
            _progressPanel.Controls.Clear();
            _progressControl = new TrafodionProgressUserControl(args);
            _progressControl.ProgressCompletedEvent += GetUserRolesAndPrivileges_Completed;
            _progressControl.Dock = DockStyle.Fill;
            _progressPanel.Controls.Add(_progressControl);

        }

        private void GetUserRolesAndPrivileges_Completed(object sender, TrafodionProgressCompletedArgs e)
        {
            this._progressControl.ProgressCompletedEvent -= GetUserRolesAndPrivileges_Completed;

            // Restore the dialog's state
            this.ParentForm.ControlBox = this.showControlBox;
            RestoreMenuStrip();

            if (_progressControl.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), _progressControl.Error.Message, "Error drop role(s)",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);

                return;
            }

            // Restore the controls' state
            this._progressPanel.Visible = false;
            this._headerPanel.Visible = true;
            this._contentPanel.Visible = true;
            this.cancelButton.Enabled = true;

            //Only show remote authentication option for M10 and above
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ131)
            {
                lblAuthType.Visible = _theDropdownAuthenticationType.Visible = true;                
            }
            else
            {
                _headerPanel.Height = _headerPanel.Height - _theDropdownAuthenticationType.Height;
                lblAuthType.Visible = _theDropdownAuthenticationType.Visible = false;                
            }

            #region Load Roles for a user from server
            Hashtable htResult = (Hashtable)_progressControl.ReturnValue;

            DataTable resultsRolesTable = new DataTable();
            DataTable resultsPrivilegesTable = new DataTable();

            if (htResult.Contains("ROLES"))
            {
                resultsRolesTable = (DataTable)htResult["ROLES"];
            }

            if (resultsRolesTable != null && resultsRolesTable.Rows.Count > 0)
            {
                foreach (DataRow dr in resultsRolesTable.Rows)
                {
                    this._arrayOrginalRoles.Add(dr[COL_KEY_ROLENAME].ToString());
                }
            }

            _rolesGrid.AutoResizeCols = true;
            _rolesGrid.ReadOnly = false;

            _rolesGrid.FillWithData(resultsRolesTable, true);

            TransferDataTableToListData(resultsRolesTable);
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

            #region   Component Privilege
            _theComponentPrivilegesUserControl.CaptionForSelectionDialog = "Alter User";
            _theComponentPrivilegesUserControl.ConnectionDefinition = _theConnectionDefinition;
            _theComponentPrivilegesUserControl.GranteeName = this._userName;
            _theComponentPrivilegesUserControl.OnChangedImpl += new ComponentPrivilegesUserControl.OnChanged(_theComponentPrivilegesUserControl_OnChangedImpl);
            if (htResult.Contains("PRIVILEGES"))
            {
                resultsPrivilegesTable = (DataTable)htResult["PRIVILEGES"];
                //Save a copy of privileges for the user.use for compare to check update.   
                _dtOrginalComponentPrivileges = resultsPrivilegesTable.Copy();
                _theComponentPrivilegesUserControl.OriginalComponentPrivilegesTable = _dtOrginalComponentPrivileges;
                _theComponentPrivilegesUserControl.ShowComponentsPrivileges(resultsPrivilegesTable);
            }
            #endregion
               
            UpdateControls();
        }

        private void _theComponentPrivilegesUserControl_OnChangedImpl()
        {
            UpdateControls();
        }

        private void _theImmutableUser_CheckedChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }
        private void _theValidUser_CheckedChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        void _theDropdownAuthenticationType_SelectedIndexChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void _dirServiceUserNameTextBox_TextChanged(object sender, EventArgs e)
        {
            UpdateControls();
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
            UpdateControls();
        }
        
        private void _okButton_Click(object sender, EventArgs e)
        {
            if (AlterUser())
            {
                Close();
            }
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseAlterUsersDialog);
        }

        private void addRoleToolStripButton_Click(object sender, EventArgs e)
        {
            showRolesDialog(RoleSelectionPanel.RoleSelectionMode.Additional);
            UpdateControls();
        }

        private void deleteRoleToolStripButton_Click(object sender, EventArgs e)
        {
            DeleteRow();
        }

        private void _resetButton_Click(object sender, EventArgs e)
        {
            _UpdatedDefaultRole = string.Empty;
            _grantedByComboBox.Text = string.Empty;
            _dirServiceUserNameTextBox.Text = _strDirUserName;
            _theValidUser.Checked = _isValidUser;
            _theImmutableUser.Checked = _isImmutableUser;
            _theDropdownAuthenticationType.Text = _auth_Type;
            _additionalRoles.Clear();
            for (int i = 0; i < _arrayOrginalRoles.Count; i++)
            {
                _additionalRoles.Add(_arrayOrginalRoles[i].ToString());
            }
            BindListDataToGrid();            
            //Bind Privilges            
            DataTable dtPrivileges = _dtOrginalComponentPrivileges.Copy();
            _theComponentPrivilegesUserControl.ClearPrivilegeCommand();
            _theComponentPrivilegesUserControl.OriginalComponentPrivilegesTable = _dtOrginalComponentPrivileges;
            _theComponentPrivilegesUserControl.ShowComponentsPrivileges(dtPrivileges);
            UpdateControls();
        }

     
        
        public bool AlterUser()
        {
            string userName = _dbUserNameTextBox.Text.Trim();           
            
            string dirServerUserName ="";            
            //if dir user name has been changed
            if (!_dirServiceUserNameTextBox.Text.Trim().Equals(_strDirUserName))
            {
                dirServerUserName = _dirServiceUserNameTextBox.Text.Trim();
            }

            string strValidUser = "";
            if (!_isValidUser.Equals(_theValidUser.Checked))
            {
                if (_theValidUser.Checked)
                {
                    strValidUser = "ONLINE";
                }
                else
                {
                    strValidUser = "OFFLINE";
                }
            }

            string strRemoteAuth = "";
            if (!_auth_Type.Equals(_theDropdownAuthenticationType.Text))
            {
                if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    if (AUTH_OPTION_ENTERPRISE.Equals(_theDropdownAuthenticationType.Text, System.StringComparison.InvariantCultureIgnoreCase))
                    {
                        strRemoteAuth = "ENTERPRISE AUTHENTICATION";
                    }
                    else if (AUTH_OPTION_CLUSTER.Equals(_theDropdownAuthenticationType.Text, System.StringComparison.InvariantCultureIgnoreCase))
                    {
                        strRemoteAuth = "CLUSTER AUTHENTICATION";
                    }
                }
                else
                {
                    if (AUTH_OPTION_REMOTE.Equals(_theDropdownAuthenticationType.Text, System.StringComparison.InvariantCultureIgnoreCase))
                    {
                        strRemoteAuth = "REMOTE AUTHENTICATION";
                    }
                    else
                    {
                        strRemoteAuth = "LOCAL AUTHENTICATION";
                    }
                }
                
            }

            string strImmutableUser = "";
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150 && !_isImmutableUser.Equals(_theImmutableUser.Checked))
            {
                if (_theImmutableUser.Checked)
                {
                    strImmutableUser = "SET IMMUTABLE";
                }
                else
                {
                    strImmutableUser = "RESET IMMUTABLE";
                }
            }



            string grantedByClause = GetGrantedByClause();

            List<string> listRevokePrivilegesCmd = new List<string>();
            listRevokePrivilegesCmd = _theComponentPrivilegesUserControl.GetRevokePrivilegesCommand(grantedByClause);

            List<string> listAlterPrivilegesCmd = new List<string>();
            listAlterPrivilegesCmd = _theComponentPrivilegesUserControl.GetAlterPrivilegesCommand(grantedByClause);

            UserMgmtSystemModel userSystemModel = UserMgmtSystemModel.FindSystemModel(ConnectionDefinition);

            string[] userMapping = { userName, _UpdatedDefaultRole, dirServerUserName, strValidUser, strRemoteAuth, strImmutableUser};

            DataTable resultsTable = new DataTable();
            Object[] parameters = new Object[] { _arrayGrantingRoles, _arrayRevokingRoles, userMapping, listRevokePrivilegesCmd,listAlterPrivilegesCmd };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.AlterUserProcessing, userSystemModel, "AlterUserRolesAndPrivileges", parameters);
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

            int successCount = userSystemModel.GetSuccessRowCount(resultsTable);
            int failureCount = userSystemModel.GetFailureRowCount(resultsTable);
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

            TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(string.Format(infoMsg, "Altering the user(s)"), resultsTable, iconType);
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

        private void Close()
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        private void _rolesGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        private void _theRemoveAllRoleStripButton_Click(object sender, EventArgs e)
        {
            _rolesGrid.Rows.Clear();
            _additionalRoles.Clear();
            _UpdatedDefaultRole = DEFAULT_ROLE_NONE;
            UpdateControls();
        }

        private void _grantedByComboBox_TextChanged(object sender, EventArgs e)
        {
            if (_grantedByComboBox.Text.Trim().Length > 0)
            {
                _resetButton.Enabled = true;
            }
        }
        #endregion

        #region  Private Methods



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
        /// Delete a row
        /// </summary>
        private void DeleteRow()
        {

            if (_rolesGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in _rolesGrid.SelectedRowIndexes)
                {
                    string roleName = _rolesGrid.Rows[_rolesGrid.SelectedRowIndexes[0]].Cells[COL_KEY_ROLENAME].Value.ToString();
                    _rolesGrid.Rows.RemoveAt(_rolesGrid.SelectedRowIndexes[0]);
                    _additionalRoles.Remove(roleName);
                    //if user removes the selected Primary role
                    if (roleName.Equals(_UpdatedDefaultRole))
                    {
                        _UpdatedDefaultRole = DEFAULT_ROLE_NONE;
                    }
                }
            }
            
            UpdateControls();
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
            dialog.ShowControl(roleSelectionPanel, "Alter User : Select Role");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (roleSelectionPanel.AdditionalRoles.Count > 0)
                {
                   AdditionalRoles= roleSelectionPanel.AdditionalRoles;
                }

            }
        }
        
        /// <summary>
        /// Update user input and buttons status.
        /// </summary>
        private void UpdateControls()
        {
            deleteRoleToolStripButton.Enabled = _rolesGrid.SelectedRowIndexes.Count > 0;
            _arrayGrantingRoles=new ArrayList();
            _arrayRevokingRoles=new ArrayList();
            ArrayList tempUnChangedList=new ArrayList();
            foreach (iGRow igr in this._rolesGrid.Rows)
            {
                string tmpRole=igr.Cells["ROLE_NAME"].Value.ToString();
                if (!_arrayOrginalRoles.Contains(tmpRole))
                {
                    _arrayGrantingRoles.Add(tmpRole);
                }
                else
                {
                    tempUnChangedList.Add(tmpRole);
                }
            }

            for (int i = 0; i < _arrayOrginalRoles.Count; i++)
            {
                if (!tempUnChangedList.Contains(_arrayOrginalRoles[i].ToString()))
                {
                    _arrayRevokingRoles.Add(_arrayOrginalRoles[i].ToString());
                }
            }

            if (_dirServiceUserNameTextBox.Text.Trim().Equals(string.Empty))
            {
                this._applyButton.Enabled = false;
            }
            else
            {
                this._applyButton.Enabled = _arrayGrantingRoles.Count > 0
                    || _arrayRevokingRoles.Count > 0
                    || (!_UpdatedDefaultRole.Equals(string.Empty) && !_UpdatedDefaultRole.Equals(_defaultRole))
                    || !_theValidUser.Checked.Equals(_isValidUser)
                    || !_theImmutableUser.Checked.Equals(_isImmutableUser)
                    || !_theDropdownAuthenticationType.Text.Equals(_auth_Type)
                    || !_dirServiceUserNameTextBox.Text.Trim().Equals(_strDirUserName)
                    || _theComponentPrivilegesUserControl.PrivilegesChanged;
            }

            this._resetButton.Enabled = this._applyButton.Enabled || _grantedByComboBox.Text.Trim().Length > 0;
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
                if (strRole.Equals(_defaultRole))
                {
                    dr[COL_KEY_DEFAULTROLE] = true;
                }
                else
                {
                    dr[COL_KEY_DEFAULTROLE] = false;
                }
                dataTable.Rows.Add(dr);
            }
            _rolesGrid.FillWithData(dataTable, true);
        }


        private void TransferDataTableToListData(DataTable dtData)
        {
            _additionalRoles = new List<string>();
            string roleName = "";
            _defaultRole = "";
            foreach (DataRow dr in dtData.Rows)
            {
                roleName = dr[COL_KEY_ROLENAME].ToString();
                _additionalRoles.Add(roleName);

                if (Convert.ToBoolean(dr[COL_KEY_DEFAULTROLE]))
                {
                    _defaultRole = roleName;
                }
            }
            if (_defaultRole.Equals(""))
            {
                _defaultRole = DEFAULT_ROLE_NONE;
            }
        }

        #endregion
    }
}
