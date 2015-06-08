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
using System.Collections.Generic;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class GrantRevokeControl : UserControl
    {    
        #region private member variables

        private GrantRevokeTreeView _databaseTreeView = null;
        private TrafodionObject _initalTrafodionObject = null;
        TrafodionProgressUserControl _progressControl = null;
        ConnectionDefinition _connectionDefinition = null;

        TrafodionLabel noCommonPrivilegSetLabel = new TrafodionLabel();
        TrafodionLabel noObjectSelectedLabel = new TrafodionLabel();
        SchemaPrivilegeOptionsControl _schemaPrivilegeOptionsControl = null;
        TablePrivilegesOptionControl _tablePrivilegesOptionControl = null;
        RoutinePrivilegeOptionsControl _procedurePrivilegesOptionControl = null;
        LibraryPrivilegeOptionsControl _libraryPrivilegeOptionsControl = null;
        IPrivilegeOptionsProvider _currentPrivilegesOptionProvider = null; 
        List<TrafodionObject> _currentSelectedObjects = new List<TrafodionObject>();

        string grantSchemaSQLText = "GRANT {0} ON SCHEMA {1} TO {2} {3} {4}";
        string grantSQLText = "GRANT {0} ON {1} TO {2} {3} {4}";
        string revokeSchemaSQLText = "REVOKE {0} {1} ON SCHEMA {2} FROM {3} {4} {5}";
        string revokeSQLText = "REVOKE {0} {1} ON {2} FROM {3} {4} {5}";
        string grantLibrarySQLText = "GRANT {0} ON LIBRARY {1} TO {2} {3} {4}";
        string revokeLibrarySQLText = "REVOKE {0} {1} ON LIBRARY {2} FROM {3} {4} {5}";

        #endregion private member variables

        #region public properties

        public List<TrafodionObject> CurrentSelectedObjects
        {
            get { return _currentSelectedObjects; }
        }

        public TrafodionObject.PrivilegeAction Action
        {
            get
            {
                return (_grantRadioButton.Checked ? TrafodionObject.PrivilegeAction.GRANT : TrafodionObject.PrivilegeAction.REVOKE);
            }
        }

        public TrafodionSystem TheTrafodionSystem
        {
            get { return TrafodionSystem.FindTrafodionSystem(_connectionDefinition); }
        }

        #endregion public properties

        #region public methods

        public GrantRevokeControl()
        {
            InitializeComponent();
            _databaseTreeView = new GrantRevokeTreeView(null);
            _databaseTreeView.Dock = DockStyle.Fill;
            _databaseTreeView.CheckAndAddListener();
            outerSplitContainer.Panel1.Controls.Add(_databaseTreeView);
            _databaseTreeView.SetAndPopulateRootFolders();
            applyButton.Enabled = false;
        }

        public GrantRevokeControl(TrafodionObject aTrafodionObject)
        {
            InitializeComponent();

            noCommonPrivilegSetLabel.Text = Properties.Resources.GrantRevokeConflictingObjectTypes;
            noObjectSelectedLabel.Text = Properties.Resources.GrantRevokeNoObjectSelected;
            privilegesPanel.Controls.Clear();
            noObjectSelectedLabel.Dock = DockStyle.Fill;
            privilegesPanel.Controls.Add(noObjectSelectedLabel);

            _initalTrafodionObject = aTrafodionObject;

            TrafodionCatalog _currentTrafodionCatalog = null;
            if (_initalTrafodionObject is TrafodionCatalog)
            {
                _currentTrafodionCatalog = (TrafodionCatalog)_initalTrafodionObject;
            }
            if (_initalTrafodionObject is TrafodionSchema)
            {
                _currentTrafodionCatalog = ((TrafodionSchema)_initalTrafodionObject).TheTrafodionCatalog;
            }
            if (_initalTrafodionObject is TrafodionSchemaObject)
            {
                _currentTrafodionCatalog = ((TrafodionSchemaObject)_initalTrafodionObject).TheTrafodionCatalog;
            }
            _connectionDefinition = _initalTrafodionObject.ConnectionDefinition;
            _databaseTreeView = new GrantRevokeTreeView(_currentTrafodionCatalog);
            _databaseTreeView.Dock = DockStyle.Fill;
            objectsGroupBox.Controls.Add(_databaseTreeView);
            _databaseTreeView.AfterCheck += _databaseTreeView_AfterCheck;
            _databaseTreeView.SetAndPopulateRootFolders();
            _databaseTreeView.SelectTrafodionObject(_initalTrafodionObject);
            if (_databaseTreeView.SelectedNode != null)
            {
                _databaseTreeView.SelectedNode.Checked = true;
            }

            granteeListPanel.Visible = false;
            actionTypeGroupBox.Visible = false;
            applyButton.Enabled = false;

            TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_initalTrafodionObject.ConnectionDefinition);
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.FetchDatabaseUsersMessage, sqlMxSystem, "LoadRoleAndUserList", new object[0]);

            //Clear the config privileges lookup control and event handler
            progressPanel.Controls.Clear();
            _progressControl = new TrafodionProgressUserControl(args);
            _progressControl.ProgressCompletedEvent += GranteeListLookup_Completed;
            _progressControl.Dock = DockStyle.Fill;
            progressPanel.Controls.Add(_progressControl);
        }

        #endregion public methods

        #region private methods

        void _databaseTreeView_AfterCheck(object sender, TreeViewEventArgs e)
        {
            if (e.Node != null)
            {
                TrafodionObject currentSelectedObject = null;
                if (e.Node is DatabaseTreeFolder)
                {
                    currentSelectedObject = ((DatabaseTreeFolder)e.Node).TrafodionObject;
                }
                else if (e.Node is DatabaseTreeNode)
                {
                    currentSelectedObject = ((DatabaseTreeNode)e.Node).TrafodionObject;
                }
                bool matchFound = false;
                if (e.Node.Checked)
                {
                    if(_currentSelectedObjects.Count > 0)
                    {
                        _currentSelectedObjects.Add(currentSelectedObject);
                        matchFound = isMatchSameTypeObject(_currentSelectedObjects);                   
                        if (!matchFound)
                        {
                            privilegesPanel.Controls.Clear();
                            _currentPrivilegesOptionProvider = null;
                            noCommonPrivilegSetLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
                            noCommonPrivilegSetLabel.Dock = DockStyle.Fill;
                            privilegesPanel.Controls.Add(noCommonPrivilegSetLabel);
                        }
                        else
                        {
                            _currentPrivilegesOptionProvider.UpdateSelections();
                        }
                    }
                    else
                    {
                        _currentSelectedObjects.Add(currentSelectedObject);
                        DisplaySuitablePrivilegeControl(currentSelectedObject);
                    }
                }
                else
                {
                    if (_currentSelectedObjects.Contains(currentSelectedObject))
                    {
                        _currentSelectedObjects.Remove(currentSelectedObject);
                    }
                                     
                    if (_currentSelectedObjects.Count > 0)
                    {
                        matchFound = isMatchSameTypeObject(_currentSelectedObjects);
                        if (!matchFound)
                        {
                            privilegesPanel.Controls.Clear();
                            _currentPrivilegesOptionProvider = null;
                            noCommonPrivilegSetLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
                            noCommonPrivilegSetLabel.Dock = DockStyle.Fill;
                            privilegesPanel.Controls.Add(noCommonPrivilegSetLabel);
                        }
                        else
                        {
                            DisplaySuitablePrivilegeControl(_currentSelectedObjects[0]);
                        }
                    }
                    else
                    {
                        privilegesPanel.Controls.Clear();
                        _currentPrivilegesOptionProvider = null;
                        noObjectSelectedLabel.Dock = DockStyle.Fill;
                        privilegesPanel.Controls.Add(noObjectSelectedLabel);
                    }
                }
                //reset the control status
                UpdateControls();
            }
        }

        /// <summary>
        /// check all selected objects type
        /// </summary>
        /// <param name="lstSelectObjects"></param>
        /// <returns></returns>
        private bool isMatchSameTypeObject(List<TrafodionObject> lstSelectObjects)
        {
            bool isMatch = false;
            if (lstSelectObjects.Count == 1)
            {
                return true;
            } 
            for (int idx=1; idx < lstSelectObjects.Count; idx++)
            {
                isMatch = false;
                if (lstSelectObjects[idx - 1] is TrafodionSchema && lstSelectObjects[idx] is TrafodionSchema)
                {
                    isMatch = true;
                }
                if (lstSelectObjects[idx - 1] is TrafodionProcedure && lstSelectObjects[idx] is TrafodionProcedure)
                {
                    isMatch = true;
                }
                if (lstSelectObjects[idx - 1] is TrafodionLibrary && lstSelectObjects[idx] is TrafodionLibrary)
                {
                    isMatch = true;
                }
                if (lstSelectObjects[idx - 1] is TrafodionTable ||
                    lstSelectObjects[idx - 1] is TrafodionMaterializedView ||
                    lstSelectObjects[idx - 1] is TrafodionView)
                {
                    if (lstSelectObjects[idx] is TrafodionTable ||
                        lstSelectObjects[idx] is TrafodionMaterializedView ||
                        lstSelectObjects[idx] is TrafodionView)
                    {
                        isMatch = true;
                    }
                }
                if (!isMatch) return false;
            }

            return true;
        }

        void DisplaySuitablePrivilegeControl(TrafodionObject currentSelectedObject)
        {
            if (currentSelectedObject is TrafodionSchema)
            {
                if (_schemaPrivilegeOptionsControl == null)
                {
                    _schemaPrivilegeOptionsControl = new SchemaPrivilegeOptionsControl(this, currentSelectedObject.ConnectionDefinition.ServerVersion);
                }
                privilegesPanel.Controls.Clear();
                _schemaPrivilegeOptionsControl.Dock = DockStyle.Fill;
                _currentPrivilegesOptionProvider = _schemaPrivilegeOptionsControl;
                privilegesPanel.Controls.Add(_schemaPrivilegeOptionsControl);
            }
            else
                if (currentSelectedObject is TrafodionTable ||
                    currentSelectedObject is TrafodionMaterializedView ||
                    currentSelectedObject is TrafodionView)
                {
                    if (_tablePrivilegesOptionControl == null)
                    {
                        _tablePrivilegesOptionControl = new TablePrivilegesOptionControl(this, currentSelectedObject.ConnectionDefinition.ServerVersion);
                    }
                    privilegesPanel.Controls.Clear();
                    _tablePrivilegesOptionControl.Dock = DockStyle.Fill;
                    _currentPrivilegesOptionProvider = _tablePrivilegesOptionControl;
                    _tablePrivilegesOptionControl.UpdateSelections();
                    privilegesPanel.Controls.Add(_tablePrivilegesOptionControl);
                }
                else
                    if (currentSelectedObject is TrafodionProcedure)
                    {
                        if (_procedurePrivilegesOptionControl == null)
                        {
                            _procedurePrivilegesOptionControl = new RoutinePrivilegeOptionsControl(this);
                        }
                        privilegesPanel.Controls.Clear();
                        _procedurePrivilegesOptionControl.Dock = DockStyle.Fill;
                        _currentPrivilegesOptionProvider = _procedurePrivilegesOptionControl;
                        privilegesPanel.Controls.Add(_procedurePrivilegesOptionControl);
                    }
                    else
                        if (currentSelectedObject is TrafodionLibrary)
                        {
                            if (_libraryPrivilegeOptionsControl == null)
                            {
                                _libraryPrivilegeOptionsControl = new LibraryPrivilegeOptionsControl(this);
                            }
                            privilegesPanel.Controls.Clear();
                            _libraryPrivilegeOptionsControl.Dock = DockStyle.Fill;
                            _currentPrivilegesOptionProvider = _libraryPrivilegeOptionsControl;
                            privilegesPanel.Controls.Add(_libraryPrivilegeOptionsControl);
                        }

        }

        void GranteeListLookup_Completed(object sender, TrafodionProgressCompletedArgs e)
        {
            if (_progressControl.Error != null)
            {
                progressPanel.Controls.Clear();
                MessageBox.Show(Utilities.GetForegroundControl(), _progressControl.Error.Message, Properties.Resources.FetDatabaseUsersFailureMessage,
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            bool loadSuccess = (bool)_progressControl.ReturnValue;
            if (loadSuccess)
            {
                LoadUsersToListView();
                granteeListBox.Enabled = false;


                /*
                * Compatibility for M6 & M7-
                * Only show the Grantor combo box for M7
                */
                if (this._initalTrafodionObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    InitializeGrantor();
                }
                else
                {
                    _grantedByComboBox.Visible = false;
                    _grantedByLabel.Visible = false;
                }
            }
            progressPanel.Controls.Clear();
            progressPanel.Visible = false;
            _progressControl.ProgressCompletedEvent -= GranteeListLookup_Completed;

            granteeListPanel.Visible = true;
            actionTypeGroupBox.Visible = true;
            selUsersRadioButton.Checked = true;
            granteeListBox.Enabled = true;

            UpdateControls();
        }

        private void resetButton_Click(object sender, System.EventArgs e)
        {
            if (_currentPrivilegesOptionProvider != null)
            {
                _currentPrivilegesOptionProvider.Reset();
            }
            selUsersRadioButton.Checked = true;
            granteeListBox.Enabled = true;
            granteeListBox.ClearCheckedItems();
            granteeListBox.ClearSelected();
        }

        private void helpButton_Click(object sender, System.EventArgs e)
        {
            if (privilegesPanel.Controls.Contains(noObjectSelectedLabel) || privilegesPanel.Controls.Contains(noCommonPrivilegSetLabel))
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Grant_Revoke_Privileges);
            }
            if (_currentPrivilegesOptionProvider is SchemaPrivilegeOptionsControl)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Grant_Schema_Privs);
            }
            else if (_currentPrivilegesOptionProvider is TablePrivilegesOptionControl)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Grant_Object_Privs);
            }
            else if (_currentPrivilegesOptionProvider is RoutinePrivilegeOptionsControl)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Grant_Procedure_Privs);
            }
            else if (_currentPrivilegesOptionProvider is LibraryPrivilegeOptionsControl)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.Grant_Library_Privs);
            }
        }

        private void cancelButton_Click(object sender, System.EventArgs e)
        {
            if (Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        private void applyButton_Click(object sender, System.EventArgs e)
        {
            if (_currentPrivilegesOptionProvider != null)
            {
                string validityString = _currentPrivilegesOptionProvider.IsValid();
                if(!string.IsNullOrEmpty(validityString))
                {
                    MessageBox.Show(validityString, Properties.Resources.Error);
                    return;
                }
                else
                {
                    if (_grantRadioButton.Checked)
                    {
                        DoGrant();
                    }
                    else
                    {
                        DoRevoke();
                    }
                }
            }
        }

        void DoGrant()
        {
            string opObjectName = "";
            if (selRolesRadioButton.Checked)
            {
                opObjectName = "selected Role(s)";
            }
            else if (selUsersRadioButton.Checked)
            {
                opObjectName = "selected User(s)";
            }
            else
            {
                opObjectName = "Public";
            }

            if (MessageBox.Show(String.Format(Properties.Resources.GrantConfirmationMessage,opObjectName), 
                Properties.Resources.Confirm, MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                string privilegeSQLClause = "";
                string userList = GetSelectedUsers();
                if (_currentPrivilegesOptionProvider != null)
                {
                    privilegeSQLClause = _currentPrivilegesOptionProvider.GetSQLPrivilegeClause();
                }
                
                DataTable errorTable = new DataTable();
                errorTable.Columns.Add("Object Name");
                errorTable.Columns.Add("Error Description");
                Cursor = Cursors.WaitCursor;

                string grantedByClause = string.Empty;
                if(!string.IsNullOrEmpty(_grantedByComboBox.Text.Trim()))
                {
                    grantedByClause = "BY " + Utilities.ExternalUserName(_grantedByComboBox.Text.Trim().ToUpper());
                }
                DataTable warningsTable = new DataTable();

                foreach (TrafodionObject currentSelectedObject in _currentSelectedObjects)
                {
                    string grantCommandString = "";
                    if (currentSelectedObject is TrafodionSchema)
                    {
                        grantCommandString = string.Format(grantSchemaSQLText,
                                    privilegeSQLClause,
                                    ((TrafodionSchema)currentSelectedObject).VisibleAnsiName,
                                    userList,
                                    (withGrantOptionCheckBox.Checked ? "WITH GRANT OPTION" : string.Empty),
                                    grantedByClause);
                    }
                    else
                    if (currentSelectedObject is TrafodionLibrary)
                    {
                        grantCommandString = string.Format(grantLibrarySQLText,
                                    privilegeSQLClause,
                                    ((TrafodionLibrary)currentSelectedObject).VisibleAnsiName,
                                    userList,
                                    (withGrantOptionCheckBox.Checked ? "WITH GRANT OPTION" : string.Empty),
                                    grantedByClause);
                    }
                    else
                    {
                    grantCommandString = string.Format(grantSQLText,
                                        privilegeSQLClause,
                                        ((TrafodionSchemaObject)currentSelectedObject).VisibleAnsiName,
                                        userList,
                                        (withGrantOptionCheckBox.Checked ? "WITH GRANT OPTION" : string.Empty),
                                        grantedByClause);
                    }

                    try
                    {
                        if (currentSelectedObject is IHasTrafodionPrivileges)
                        {
                            ((IHasTrafodionPrivileges)currentSelectedObject).GrantRevoke(TrafodionObject.PrivilegeAction.GRANT, _connectionDefinition, grantCommandString, out warningsTable);
                        }
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { currentSelectedObject.VisibleAnsiName, ex.Message });
                    }
                }
                Cursor = Cursors.Default;

                if (errorTable.Rows.Count == 0 && warningsTable.Rows.Count == 0)
                {
                    MessageBox.Show("Grant was successful", Properties.Resources.Info);
                }
                else
                {
                    if (errorTable.Rows.Count == 1)
                    {
                        MessageBox.Show(string.Format("{0} {1} \n\n {2}", Properties.Resources.SingleGrantFailureMessage,
                                            errorTable.Rows[0].ItemArray[0], errorTable.Rows[0].ItemArray[1]),
                                            Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else if (errorTable.Rows.Count > 1)
                    {
                        TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.MultiGrantFailureMessage, errorTable, System.Drawing.SystemIcons.Error);
                        mmd.ShowDialog();
                    }
                    if (warningsTable.Rows.Count == 1)
                    {
                        MessageBox.Show(string.Format("{0}\n\n{1}", "Grant generated warnings.",
                                            warningsTable.Rows[0].ItemArray[0]),
                                            Properties.Resources.Warning, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                    else if (warningsTable.Rows.Count > 1)
                    {
                        TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("Grant generated warnings.", warningsTable, System.Drawing.SystemIcons.Warning);
                        mmd.ShowDialog();
                    }
                }
            }
        }

        void DoRevoke()
        {
            string opObjectName = "";
            if (selRolesRadioButton.Checked)
            {
                opObjectName = "selected Role(s)";
            }
            else if (selUsersRadioButton.Checked)
            {
                opObjectName = "selected User(s)";
            }
            else
            {
                opObjectName = "Public";
            }

            if (MessageBox.Show(String.Format(Properties.Resources.RevokeConfirmationMessage,opObjectName), 
                Properties.Resources.Confirm, MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                string privilegeSQLClause = "";
                string userList = GetSelectedUsers();
                if (_currentPrivilegesOptionProvider != null)
                {
                    privilegeSQLClause = _currentPrivilegesOptionProvider.GetSQLPrivilegeClause();
                }

                DataTable errorTable = new DataTable();
                errorTable.Columns.Add("Object Name");
                errorTable.Columns.Add("Error Description");

                DataTable warningsTable = new DataTable();
                string grantedByClause = string.Empty;
                if (!string.IsNullOrEmpty(_grantedByComboBox.Text.Trim()))
                {
                    grantedByClause = " BY " + _grantedByComboBox.Text.Trim().ToUpper();
                }

                Cursor = Cursors.WaitCursor;
                foreach (TrafodionObject currentSelectedObject in _currentSelectedObjects)
                {
                    string revokeCommandString = string.Empty;
                    if (currentSelectedObject is TrafodionSchema)
                    {
                        revokeCommandString = string.Format(revokeSchemaSQLText,
                                    (withGrantOptionCheckBox.Checked ? "GRANT OPTION FOR" : string.Empty),
                                    privilegeSQLClause,
                                    ((TrafodionSchema)currentSelectedObject).VisibleAnsiName,
                                    userList,
                                    cascadeCheckBox.Checked ? "CASCADE" : string.Empty,
                                    grantedByClause);
                    }
                    else
                    if (currentSelectedObject is TrafodionLibrary)
                    {
                        revokeCommandString = string.Format(revokeLibrarySQLText,
                                    (withGrantOptionCheckBox.Checked ? "GRANT OPTION FOR" : string.Empty),
                                    privilegeSQLClause,
                                    ((TrafodionLibrary)currentSelectedObject).VisibleAnsiName,
                                    userList,
                                    cascadeCheckBox.Checked ? "CASCADE" : string.Empty,
                                    grantedByClause);
                    }
                    else
                    {
                        revokeCommandString = string.Format(revokeSQLText,
                                    (withGrantOptionCheckBox.Checked ? "GRANT OPTION FOR" : string.Empty),
                                    privilegeSQLClause,
                                    ((TrafodionSchemaObject)currentSelectedObject).VisibleAnsiName,
                                    userList,
                                    cascadeCheckBox.Checked ? "CASCADE" : string.Empty,
                                    grantedByClause);
                    }

                    try
                    {
                        if (currentSelectedObject is IHasTrafodionPrivileges)
                        {
                            ((IHasTrafodionPrivileges)currentSelectedObject).GrantRevoke(TrafodionObject.PrivilegeAction.REVOKE, _connectionDefinition, revokeCommandString, out warningsTable);
                        }
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { currentSelectedObject.VisibleAnsiName, ex.Message });
                    }
                }
                Cursor = Cursors.Default;
                if (errorTable.Rows.Count == 0 && warningsTable.Rows.Count == 0)
                {
                    MessageBox.Show("Revoke was successful", Properties.Resources.Info);
                }
                else
                {
                    if (errorTable.Rows.Count == 1)
                    {
                        MessageBox.Show(string.Format("{0} {1} \n\n {2}", Properties.Resources.MultiRevokeFailureMessage,
                                            errorTable.Rows[0].ItemArray[0], errorTable.Rows[0].ItemArray[1]),
                                            Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                    else if (errorTable.Rows.Count > 1)
                    {
                        TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.MultiRevokeFailureMessage, errorTable, System.Drawing.SystemIcons.Error);
                        mmd.ShowDialog();
                    }
                    if (warningsTable.Rows.Count == 1)
                    {
                        MessageBox.Show(string.Format("{0}\n\n{1}", "Revoke generated warnings.",
                                            warningsTable.Rows[0].ItemArray[0]),
                                            Properties.Resources.Warning, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                    else if (warningsTable.Rows.Count > 1)
                    {
                        TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("Revoke generated warnings.", warningsTable, System.Drawing.SystemIcons.Warning);
                        mmd.ShowDialog();
                    }
                }
            }

        }

        /// <summary>
        /// Initialize the Grantor combo box
        /// Load all roles of current user as grantors
        /// </summary>
        private void InitializeGrantor()
        {
            TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(this._initalTrafodionObject.ConnectionDefinition);
            _grantedByComboBox.DataSource = sqlMxSystem.RolesForCurrentUser;
            _grantedByComboBox.SelectedIndex = -1;
            _toolTip.SetToolTip(_grantedByComboBox, global::Trafodion.Manager.Properties.Resources.GrantedByToolTip);
        }

        string GetSelectedUsers()
        {
            if (selUsersRadioButton.Checked)
            {
                List<string> selectedUsers = new List<string>();
                for (int i = 0; i < granteeListBox.CheckedItems.Count; i++)
                {
                    selectedUsers.Add(Utilities.ExternalUserName(granteeListBox.CheckedItems[i].ToString()));
                }
                return string.Join(", ", selectedUsers.ToArray());
            }
            else if (selRolesRadioButton.Checked)
            {
                List<string> selectedRoles = new List<string>();
                for (int i = 0; i < granteeListBox.CheckedItems.Count; i++)
                {
                    selectedRoles.Add(Utilities.ExternalUserName(granteeListBox.CheckedItems[i].ToString()));
                }
                return string.Join(", ", selectedRoles.ToArray());
            }
            else
            {
                return "PUBLIC";
            }
        }

        private void SetApplyButton()
        {
            /*
             * The Grantee is valid only if:
             * "Public" is checked, or
             * there's at least one User/Role selected.
            */
            bool hasUserOrRoleSelected = (selUsersRadioButton.Checked || selRolesRadioButton.Checked) && granteeListBox.CheckedItems.Count > 0;
            bool isPublicChecked = _publicRadioButton.Checked;
            bool isGranteeValid = isPublicChecked || hasUserOrRoleSelected;

            /*
             * The Apply button is enabled only if:
             * there's at least one object is selected in the tree, and 
             * Grantee is valid(Refer to above isGranteeValid).
             */
            bool hasObjectSelected = _currentSelectedObjects.Count > 0;
            applyButton.Enabled = hasObjectSelected && isGranteeValid;
        }

        void UpdateControls()
        {
            SetApplyButton();

            applyButton.Text = _grantRadioButton.Checked ? Properties.Resources.GrantButtonText : Properties.Resources.RevokeButtonText;
            withGrantOptionCheckBox.Text = _grantRadioButton.Checked ? "With Grant Option" : "Only revoke Grant Option on selected privileges";
            cascadeCheckBox.Visible = revokeRadioButton.Checked;
            if (_grantRadioButton.Checked)
            {
                cascadeCheckBox.Checked = false;
            }

            /*
            * Compatibility for M6 & M7-
            * For M6, With Grant Option check box is always visible
            * For M7, If "Public" is checked, With Grant Option check box should be hidden
            */
            if (this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                withGrantOptionCheckBox.Visible = !_publicRadioButton.Checked;

                if (_publicRadioButton.Checked)
                {
                    withGrantOptionCheckBox.Checked = false;
                }
            }
        }

        private void revokeRadioButton_CheckedChanged(object sender, System.EventArgs e)
        {
            UpdateControls();
        }

        private void _grantRadioButton_CheckedChanged(object sender, System.EventArgs e)
        {
            UpdateControls();
        }

        private void _publicRadioButton_CheckedChanged(object sender, EventArgs e)
        {
            granteeListBox.ClearCheckedItems();            
            granteeListBox.Enabled = false;
            UpdateControls();
        }

        private void granteeListBox_ItemCheck(object sender, ItemCheckEventArgs e)
        {
            if (e.NewValue == CheckState.Checked)
            {
                if (granteeListBox.Tag.Equals("ROLE"))
                {
                    selRolesRadioButton.Checked = true;
                }
                else if (granteeListBox.Tag.Equals("USER"))
                {
                    selUsersRadioButton.Checked = true;
                }
            }
        }


        private void selUsersRadioButton_CheckedChanged(object sender, EventArgs e)
        {
            if (selUsersRadioButton.Checked)
            {
                LoadUsersToListView();                
            }
            UpdateControls();
        }

        private void selRolesRadioButton_CheckedChanged(object sender, EventArgs e)
        {
            if (selRolesRadioButton.Checked)
            {
                LoadRolesToListView();                
            }
            UpdateControls();
        }

        private void _grantRadioButton_Click(object sender, EventArgs e)
        {
            withGrantOptionCheckBox.Checked = false;
        }

        private void revokeRadioButton_Click(object sender, EventArgs e)
        {
            withGrantOptionCheckBox.Checked = false;
        }

        private void LoadUsersToListView()
        {
            granteeListBox.Enabled = true;
            granteeListBox.Items.Clear();
            granteeListBox.Tag = "USER";
            if (TheTrafodionSystem.RoleAndUserList != null && TheTrafodionSystem.RoleAndUserList.Count == 2)
            {
                foreach (string[] role in (List<string[]>)TheTrafodionSystem.RoleAndUserList[0])
                {
                    granteeListBox.Items.Add(role[0]);
                }
            }
        }

        private void LoadRolesToListView()
        {
            granteeListBox.Enabled = true;
            granteeListBox.Items.Clear();
            granteeListBox.Tag = "ROLE";
            if (TheTrafodionSystem.RoleAndUserList != null && TheTrafodionSystem.RoleAndUserList.Count == 2)
            {
                foreach (string[] userName in (List<string[]>)TheTrafodionSystem.RoleAndUserList[1])
                {
                    granteeListBox.Items.Add(userName[0]);
                }
            }
        }
        #endregion private methods

        private void granteeListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            SetApplyButton();
        }
    }
}
