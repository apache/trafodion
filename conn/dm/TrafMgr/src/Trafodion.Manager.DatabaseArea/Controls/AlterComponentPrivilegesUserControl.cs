//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class AlterComponentPrivilegesUserControl : UserControl, ICloneToWindow
    {
        #region Private member variables

        ConnectionDefinition _connectionDefinition;
        static readonly string CompPrivPersistenceKey = "AlterComponentPrivileges";
        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _granteesWidget;
        TrafodionIGrid _granteesGrid;
        //TrafodionSystem _sqlMxSystem;
        private string _defaultComponentName;

        ToolStripButton addGranteeButton=null;
        ToolStripButton removeGranteeButton=null;
        GranteeSelectionPanel granteeSelectionPanel = null;
        DataTable _dtGrantees = null;
        private const string COL_GRANTEE = "Grantee Name";
        private const string COL_GRANTEETYPE = "Type";
        public const string STR_COMPONENT_ALL = "ALL";
        bool _IsLoaded = false;

        //Flag of new grantee added and hasn't committed
        bool _HasNewGranteeAdded = false;

        #endregion Private member variables

        #region Public Properties

        public bool HasNewGranteeAdded
        {
            get
            {
                return _HasNewGranteeAdded;
            }
        }

        public bool HasPrivilegesChanged
        {
            get
            {
                if (_componentPrivilegesUserControl.PrivilegesChanged || _HasNewGranteeAdded)
                    return true;
                else return false;
            }
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set 
            {
                //When a new connection is set, stop the data provider and reset the data provider to use the new connection
                if (_connectionDefinition != null)
                {
                    _granteesWidget.DataProvider.Stop();
                    _granteesGrid.Rows.Clear();
                    _componentPrivilegesUserControl.ClearPrivileges();
                }
                _connectionDefinition = value;
                if (_connectionDefinition != null)
                {
                    _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _granteesWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _granteesWidget.StartDataProvider();
                }

                // Refresh Granted By user/role list                 
                _componentPrivilegesUserControl.GrantedByDataSource = TrafodionSystem.FindTrafodionSystem(value).RolesForCurrentUser;

                /*
                * Compatibility for M6 & M7-
                * Only show the Grantor combo box for M7
                */
                SetGrantedByVisibility();
            }
        }

        /// <summary>
        /// Interaction property for select Grantee dialog.
        /// </summary>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public List<DataRow> Grantees
        {
            get
            {
                if (_dtGrantees == null)
                {
                    _dtGrantees = new DataTable();
                    _dtGrantees.Columns.Add(COL_GRANTEE);
                    _dtGrantees.Columns.Add(COL_GRANTEETYPE);
                }
                return _dtGrantees.AsEnumerable().ToList();
            }
            set
            {
                List<DataRow> needAddToGridRows = new List<DataRow>();
                bool isExistedInGrid = false;
                foreach (DataRow dr in value)
                {
                    isExistedInGrid = false;
                    foreach (DataRow gridDr in _dtGrantees.Rows)
                    {
                        if (dr[COL_GRANTEE].ToString().Equals(gridDr[COL_GRANTEE].ToString()) &&
                            dr[COL_GRANTEETYPE].ToString().Equals(gridDr[COL_GRANTEETYPE].ToString()))
                        {
                            isExistedInGrid = true;
                            break;
                        }
                    }

                    if (!isExistedInGrid)
                    {
                        needAddToGridRows.Add(dr);
                    }
                }

                iGRow lastAddedRow = null;

                foreach (DataRow dr in needAddToGridRows)
                {
                    iGRow row = _granteesGrid.Rows.Add();
                    row.Cells[COL_GRANTEE].Value = dr[COL_GRANTEE].ToString();
                    row.Cells[COL_GRANTEETYPE].Value = dr[COL_GRANTEETYPE].ToString();
                    row.BackColor = System.Drawing.Color.LightGreen;

                    DataRow newRow = _dtGrantees.NewRow();
                    newRow[COL_GRANTEE] = dr[COL_GRANTEE].ToString();
                    newRow[COL_GRANTEETYPE] = dr[COL_GRANTEETYPE].ToString();
                    _dtGrantees.Rows.Add(newRow);
                    lastAddedRow = row;
                    _HasNewGranteeAdded = true;
                }
                if (lastAddedRow != null)
                {
                    _granteesGrid.PerformAction(iGActions.DeselectAllRows);                    
                    lastAddedRow.Selected = true;
                    _granteesGrid.PerformAction(iGActions.GoLastRow);
                }
            }
        }
        #endregion Public Properties

        #region Constructors/Destructor

        public AlterComponentPrivilegesUserControl()
        {
            InitializeComponent();
        }

        public AlterComponentPrivilegesUserControl(ConnectionDefinition aConnectionDefinition, string componentName)
            :this()
        {
            _connectionDefinition = aConnectionDefinition;
            //_sqlMxSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
            _defaultComponentName = componentName;
            _componentPrivilegesUserControl.ConnectionDefinition = aConnectionDefinition;
            _componentPrivilegesUserControl.Component = componentName;
            _componentPrivilegesUserControl.OnComponentPrivilegesChangedImpl += new 
                ComponentPrivilegesUserControl.OnComponentPrivilegesChanged(_componentPrivilegesUserControl_OnComponentPrivilegesChangedImpl);
            _applyButton.Enabled = false;
            _resetGuiButton.Enabled = false;
            LoadComponents();
            SetupGranteeWidget();
        }

        void _componentPrivilegesUserControl_OnComponentPrivilegesChangedImpl()
        {
            UpdateControls();
        }

        /// <summary>
        /// Do cleanup on dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (_granteesWidget != null)
            {
                _granteesWidget.DataProvider.Stop();
                ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).Dispose();
                _granteesWidget.Dispose();
            }
        }

        #endregion Constructors/Destructors

        #region private functions

        /// <summary>
        /// Load all components 
        /// </summary>
        public void LoadComponents()
        {
            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
            if (_sqlMxSystem.Components.Count > 0)
            {
                string[] componentNames = (from component in _sqlMxSystem.Components select component.ComponentName).Distinct().ToArray();
                _componentComboBox.Items.Clear();
                _componentComboBox.Items.Add(STR_COMPONENT_ALL);
                _componentComboBox.Items.AddRange(componentNames);
            }
            _componentComboBox.SelectedIndexChanged += _componentComboBox_SelectedIndexChanged;
            _componentComboBox.SelectedIndexChanging += _componentComboBox_SelectedIndexChanging;
            if (!string.IsNullOrEmpty(_defaultComponentName))
            {
                _componentComboBox.SelectedItem = _defaultComponentName;
            }

            _componentPrivilegesUserControl.GrantedByDataSource = _sqlMxSystem.RolesForCurrentUser;

            /*
            * Compatibility for M6 & M7-
            * Only show the Grantor combo box for M7
            */
            _componentPrivilegesUserControl.SetGrantedByVisibility(this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120);
        }


        /// <summary>
        /// Update buttons status 
        /// </summary>
        void UpdateControls()
        {
            if (addGranteeButton != null)
            {
                addGranteeButton.Enabled = true;
            }
            if (removeGranteeButton != null)
            {
                removeGranteeButton.Enabled = _granteesGrid.Rows.Count > 0 && _granteesGrid.SelectedRows.Count > 0;
            }

            if (addGranteeButton != null)
            {
                addGranteeButton.Enabled = _IsLoaded;
            }
            
            _applyButton.Enabled = _granteesGrid.Rows.Count > 0 && 
                                _granteesGrid.SelectedRows.Count > 0 &&
                                _componentPrivilegesUserControl.PrivilegesChanged;
            _resetGuiButton.Enabled = _componentPrivilegesUserControl.PrivilegesChanged;
        }

        /// <summary>
        /// Sets up the grantees widget
        /// </summary>
        void SetupGranteeWidget()
        {
            //Read the widget config from persistence
            _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(CompPrivPersistenceKey);

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _universalWidgetConfig.Name = CompPrivPersistenceKey;
                _universalWidgetConfig.DataProviderConfig.TimerPaused = false;
                _universalWidgetConfig.ShowProviderStatus = false;
                _universalWidgetConfig.ShowProperties = false;
                _universalWidgetConfig.ShowToolBar = true;
                _universalWidgetConfig.ShowChart = false;
                _universalWidgetConfig.ShowTimerSetupButton = false;
                _universalWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
                _universalWidgetConfig.ShowExportButtons = false;
            }
            _universalWidgetConfig.ShowHelpButton = false;

            _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;

            //Create the Grantees Widget
            _granteesWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).LineCountFormat = "Grantees";
            _granteesWidget.DataProvider = new ComponentPrivilegesDataProvider();
            _granteesWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            _granteesWidget.DataProvider.OnBeforeFetchingData += new DataProvider.BeforeFetchingData(DataProvider_OnBeforeFetchingData);
            _granteesWidget.DataProvider.OnFetchCancelled += new DataProvider.ErrorEncountered(DataProvider_OnFetchCancelled);

            //Set the widget configuration 
            _granteesWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _granteesWidget.Dock = DockStyle.Fill;
            _usersGroupBox.Controls.Add(_granteesWidget);

            //Associate the custom data display handler for the TabularDisplay panel
            _granteesWidget.DataDisplayControl.DataDisplayHandler = new CompPrivDataHandler(this);

            //Initialize the Alerts iGrid
            _granteesGrid = ((TabularDataDisplayControl)_granteesWidget.DataDisplayControl).DataGrid;
            _granteesGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
            _granteesGrid.AutoResizeCols = true;
            _granteesGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.One;
            _granteesGrid.RowMode = true;
            _granteesGrid.AllowColumnFilter = false;//don't need Show/Hide column config dialog
            _granteesGrid.SelectionChanged += _granteesGrid_SelectionChanged;

            //Set selected rows color while losed focus.
            _granteesGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _granteesGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;            

            addGranteeButton = new ToolStripButton();
            addGranteeButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            addGranteeButton.Image = global::Trafodion.Manager.Properties.Resources.EditAddIcon;
            addGranteeButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            addGranteeButton.Name = "addGrantee";
            addGranteeButton.Size = new System.Drawing.Size(23, 22);
            addGranteeButton.Text = "Add Grantee(s)";
            addGranteeButton.Click += new EventHandler(addGranteeButton_Click);
            _granteesWidget.AddToolStripItem(addGranteeButton);
            addGranteeButton.Enabled = false;
            
            removeGranteeButton = new ToolStripButton();
            removeGranteeButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            removeGranteeButton.Image = global::Trafodion.Manager.Properties.Resources.EditRemoveIcon;
            removeGranteeButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            removeGranteeButton.Name = "removeGrantee";
            removeGranteeButton.Size = new System.Drawing.Size(23, 22);
            removeGranteeButton.Text = "Remove Grantee(s)";
            removeGranteeButton.Click += new EventHandler(removeGranteeButton_Click);
            _granteesWidget.AddToolStripItem(removeGranteeButton);
            removeGranteeButton.Enabled = false;
        }

        private void DataProvider_OnFetchCancelled(object sender, DataProviderEventArgs e)
        {
            removeGranteeButton.Enabled = _granteesGrid.Rows.Count > 0 && _granteesGrid.SelectedRows.Count > 0;
            if (removeGranteeButton.Enabled)
            {
                _componentPrivilegesUserControl.ResetAllPrivileges();
            }
            else
            {
                _componentPrivilegesUserControl.ClearPrivileges();
            }
        }

        private void DataProvider_OnBeforeFetchingData(object sender, DataProviderEventArgs e)
        {
            removeGranteeButton.Enabled = false;
            if (_componentPrivilegesUserControl.PrivilegesChanged)
            {
                DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(),
                      string.Format(Properties.Resources.CommitChangesWarningMessage, "grantee"),
                  Properties.Resources.Confirm, MessageBoxButtons.YesNo);

                if (result == DialogResult.Yes)
                {
                    //Commit the changes.
                    CommitChanges(_componentPrivilegesUserControl.ListCommandString);
                }
            }
            _componentPrivilegesUserControl.ClearPrivileges();
            _resetGuiButton.Enabled = _applyButton.Enabled = false;
        }

        /// <summary>
        /// Commit user changes 
        /// </summary>
        /// <param name="cmd">Revoke/Grant command string that has been built.</param>
        /// <returns></returns>
        private bool CommitChanges(List<string> cmd)
        {
            if (cmd == null || cmd.Count == 0)
            {
                _HasNewGranteeAdded = false;
                return false;
            }
            ComponentPrivilegeModel comModel = ComponentPrivilegeModel.FindSystemModel(ConnectionDefinition);
            DataTable resultsTable = new DataTable();
            Object[] parameters = new Object[] { cmd };
            TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.ApplyChangesProcessingMessage, comModel, "GrantRevokeComponentPrivileges", parameters);
            TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
            progressDialog.ShowDialog();
            if (progressDialog.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error on commiting to the server",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                resultsTable = (DataTable)progressDialog.ReturnValue;
            }

            int successCount = comModel.GetSuccessRowCount(resultsTable);
            int failureCount = comModel.GetFailureRowCount(resultsTable);
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

            TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(
                string.Format(infoMsg, "Granting or revoking the component privilege(s)"), resultsTable, iconType);
            mmd.ShowDialog();
            _componentPrivilegesUserControl.ResetAllPrivileges();
            _HasNewGranteeAdded = false;
            return successCount > 0;
        }

        /// <summary>
        /// Remove a row from grantee datatable by grantee name and type.
        /// </summary>
        /// <param name="granteeName"></param>
        /// <param name="type"></param>
        private bool removeGranteeFromDataTable(string granteeName,string type)
        {
            List<DataRow> rowsToRemove = new List<DataRow>();
            foreach (DataRow dr in _dtGrantees.Rows)
            {
                if (dr[COL_GRANTEE].ToString().Equals(granteeName) &&
                    dr[COL_GRANTEETYPE].ToString().Equals(type))
                {
                    rowsToRemove.Add(dr);
                    break;
                }
            }

            foreach (var dr in rowsToRemove)
            {
                _dtGrantees.Rows.Remove(dr);
            }
            return true;
        }
        #endregion

        #region Events Handling

        private void _componentComboBox_SelectedIndexChanging(object sender, CancelEventArgs e)
        {
            if (_componentPrivilegesUserControl.PrivilegesChanged || _HasNewGranteeAdded)
            {
                DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(),
                    string.Format(Properties.Resources.CommitChangesWarningMessage, "component"),
                Properties.Resources.Confirm, MessageBoxButtons.YesNoCancel);

                if (result == DialogResult.Yes)
                {
                    //Commit the changes.
                    CommitChanges(_componentPrivilegesUserControl.ListCommandString);
                }
                else if (result == DialogResult.No)
                {
                    _componentPrivilegesUserControl.ResetAllPrivileges();
                    _HasNewGranteeAdded = false;
                }
                else if (result == DialogResult.Cancel)
                {
                    e.Cancel = true;
                }
            }

        }

        /// <summary>
        /// Display related component privilges for the grantee.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _componentComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {

            _componentPrivilegesUserControl.Component = _componentComboBox.SelectedItem.ToString();
            _componentPrivilegesUserControl.GranteeName = string.Empty;
            if (_IsLoaded)
            {
                LoadData();
                if (_granteesGrid.SelectedRowIndexes.Count > 0)
                {
                    ShowUserPrivileges(_granteesGrid.SelectedRowIndexes[0]);
                }
            }


        }

        /// <summary>
        /// Remove grantee with revoking all privilges on it.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void removeGranteeButton_Click(object sender, EventArgs e)
        {
            List<string> revokePrivilges = _componentPrivilegesUserControl.getRevokeAllPrivilegesCommand(_componentComboBox.SelectedItem.ToString());
            //In case of new added user without commit,then it would be discarded silently.
            if (revokePrivilges != null && revokePrivilges.Count > 0)
            {
                if (MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.RevokeGranteeConfirmMessage,
                        Properties.Resources.Confirm, MessageBoxButtons.YesNo) != DialogResult.Yes)
                {
                    return;
                }
                //Commit the revoking all privilges on that user.
                if (CommitChanges(revokePrivilges))
                {
                    _granteesWidget.DataProvider.Start();
                }
                else//failed to commit changes.
                {
                    return;
                }
            }
            else
            {
                //A new added grantee that will be removed again.reset the flag.
                _HasNewGranteeAdded = false;
            }
            //Remove grantee.
            int iRowIndex = _granteesGrid.SelectedRowIndexes[0];
            foreach (int index in _granteesGrid.SelectedRowIndexes)
            {
                string granteeName = _granteesGrid.Rows[_granteesGrid.SelectedRowIndexes[0]].Cells[0].Value.ToString();
                string granteeType=_granteesGrid.Rows[_granteesGrid.SelectedRowIndexes[0]].Cells[1].Value.ToString();
                _granteesGrid.Rows.RemoveAt(_granteesGrid.SelectedRowIndexes[0]);
                removeGranteeFromDataTable(granteeName, granteeType);
            }
            //Automatically select the upper row.
            if (iRowIndex >= 1)
            {
                _granteesGrid.Rows[iRowIndex - 1].Selected = true;
            }
            else
            {
                if (_granteesGrid.Rows.Count>0)   _granteesGrid.Rows[0].Selected = true;
            }
        }

        /// <summary>
        /// Add user/role name to the grantee grid.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void addGranteeButton_Click(object sender, EventArgs e)
        {
            showSelectionGranteeDialog();
        }

        /// <summary>
        /// Check if "Public" is in granted grantees
        /// </summary>
        /// <param name="grantedGrantees"></param>
        /// <returns></returns>
        private bool IsPublicInGrantees(List<DataRow> grantedGrantees)
        {
            bool isPublicInGrantees = false;
            const string PUBLIC_GRANTEE = "PUBLIC";
            foreach (DataRow granteeRow in grantedGrantees)
            {
                bool isPublicGrantee = 0 == string.Compare(PUBLIC_GRANTEE, (string)granteeRow[COL_GRANTEE], true);
                bool isPublicGranteeType = 0 == string.Compare(PUBLIC_GRANTEE, Privilege.UserType.Public.ToString(), true);
                if (isPublicGrantee && isPublicGranteeType)
                {
                    isPublicInGrantees = true;
                    break;
                }
            }

            return isPublicInGrantees;
        }

        /// <summary>
        /// Display role selection dialog.
        /// </summary>
        /// <param name="granteeName"></param>
        private void showSelectionGranteeDialog()
        {
            List<DataRow> grantedGrantees = Grantees;
            bool isPublicInGrantees = IsPublicInGrantees(grantedGrantees);
            if (granteeSelectionPanel == null)
            {
                granteeSelectionPanel = new GranteeSelectionPanel(_connectionDefinition, isPublicInGrantees);
            }
            else
            {
                //Assign new connection to dialog.in case of user switch different system.
                granteeSelectionPanel.ConnectionDefinition = _connectionDefinition;
                granteeSelectionPanel.IsPublicInGrantees = isPublicInGrantees;
            }
            granteeSelectionPanel.SelectedGrantees = grantedGrantees;

            ManageGranteeDialog dialog = new ManageGranteeDialog();
            dialog.ShowControl(granteeSelectionPanel, "Select Grantee");
            if (dialog.DialogResult == DialogResult.OK)
            {
                if (granteeSelectionPanel.SelectedGrantees.Count > 0)
                {
                    Grantees = granteeSelectionPanel.SelectedGrantees;
                }

            }
        }
        
        /// <summary>
        /// Handles the row selection in the grantees grid. When you select a different grantee, 
        /// the component privileges control displays the privileges for the newly selected grantee
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _granteesGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (_componentPrivilegesUserControl.PrivilegesChanged)
            {
                DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(),
                      string.Format(Properties.Resources.CommitChangesWarningMessage, "grantee"),
                  Properties.Resources.Confirm, MessageBoxButtons.YesNo);


                if (result == DialogResult.Yes)
                {
                    //Commit the changes.
                    CommitChanges(_componentPrivilegesUserControl.ListCommandString);
                } 
            }

            if (_granteesGrid.SelectedRowIndexes.Count > 0)
            {                
                ShowUserPrivileges(_granteesGrid.SelectedRowIndexes[0]);
            }            
            UpdateControls();
        }

        /// <summary>
        /// Display the component privileges for the grantee selected in the grantee grid
        /// </summary>
        /// <param name="rowIndex"></param>
        private void ShowUserPrivileges(int rowIndex)
        {
            string granteeName = _granteesGrid.Rows[rowIndex].Cells[COL_GRANTEE].Value.ToString();

            /*
             * Check if the Grantee Type of current selection is public, and set IsPublicGranteeTypey property
            */


            /*
            * Compatibility for M6 & M7-
            * For M6, IsWithGrantOptionEnabled should be set by checking if the selected object is "User"
            * For M7, IsPublicGranteeType should be set by checking if the selected object is "Public"
            * Since M7, IsWithGrantOptionEnabled will be deprecated, and IsPublicGranteeType will be used.
            */
            string granteeType = (string)_granteesGrid.Rows[rowIndex].Cells[COL_GRANTEETYPE].Value;
            if (this.ConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _componentPrivilegesUserControl.IsWithGrantOptionEnabled = 0 == string.Compare(granteeType, "User", true);
            }
            else
            {
                _componentPrivilegesUserControl.IsPublicGranteeType = 0 == string.Compare(granteeType, "Public", true);
            }

            _componentPrivilegesUserControl.ConnectionDefinition = _connectionDefinition;
            _componentPrivilegesUserControl.ShowPrivilegesToGrid(granteeName,true);
        }

        /// <summary>
        /// Resets the edits that user might have made to the component privileges
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _resetGuiButton_Click(object sender, EventArgs e)
        {

            if (MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ResetComPrivilegesConfirmMessage,
                Properties.Resources.Confirm, MessageBoxButtons.YesNo) != DialogResult.Yes)
            {
                return;
            }

            //Reset component privileges for selected grantee
            _componentPrivilegesUserControl.ResetAllPrivileges();           
        }

        /// <summary>
        /// Apply all changes and commit them to the server.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _applyButton_Click(object sender, EventArgs e)
        {
            if (CommitChanges(_componentPrivilegesUserControl.ListCommandString))
            {
                _granteesWidget.DataProvider.Start();
            }

        }
        /// <summary>
        /// load grantee list and update all buttons' status.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AlterComponentPrivilegesUserControl_Load(object sender, EventArgs e)
        {
            if (_granteesWidget != null && _granteesWidget.DataProvider != null)
            {
                UpdateControls();
                _granteesWidget.DataProvider.Start();
            }
        }

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ComponentPrivileges);
        }
        #endregion

        #region Public Methods

        /// <summary>
        /// Set GRANTED BY Visibility
        /// </summary>
        public void SetGrantedByVisibility()
        {
            /*
            * Compatibility for M6 & M7-
            * Only show the Grantor combo box for M7
            */
            _componentPrivilegesUserControl.SetGrantedByVisibility(this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120);
        }

        /// <summary>
        /// Loads the component privileges into the grantees grid and the component privileges user control 
        /// </summary>
        public void LoadData()
        {
            TrafodionSystem _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
            _IsLoaded = false; 
           
            _dtGrantees = new DataTable();
            _dtGrantees.Columns.Add(COL_GRANTEE);
            _dtGrantees.Columns.Add(COL_GRANTEETYPE);

            if (_sqlMxSystem.ComponentPrivileges.Count > 0)
            {
                //First load the list of unique grantees into the grantees grid.                
                var allGranteeNamesAndType = _sqlMxSystem.ComponentPrivileges.Select(priv => new {priv.ComponentUID,priv.GranteeName, priv.GranteeType }).Distinct().ToArray();

                string componentName = _componentComboBox.SelectedItem.ToString();
                var granteeNameAndType = allGranteeNamesAndType.Where(
                    a => (componentName.Equals(_sqlMxSystem.GetComponentName(a.ComponentUID)) || componentName.Equals("ALL"))).Select(
                    priv => new { priv.GranteeName, priv.GranteeType }).Distinct().ToArray();

               // string componentName;
                foreach (var gt in granteeNameAndType)
                {
                    DataRow dr=_dtGrantees.NewRow();
                    dr[COL_GRANTEE]=gt.GranteeName;
                    dr[COL_GRANTEETYPE]=gt.GranteeType;
                    _dtGrantees.Rows.Add(dr);
                }
            }
            _granteesGrid.FillWithData(_dtGrantees);

            if (_granteesGrid.Rows.Count > 0)
            {
                bool hasContainGrantee = false;
                if (string.IsNullOrEmpty(_componentPrivilegesUserControl.GranteeName))
                {
                    //Select the first row, which causes the corresponding grantee's privileges to show up in the component privilege user control
                    _granteesGrid.Rows[0].Selected = true;
                }
                else
                {
                    //in case of refreshing the grantee list,keep the selected grantee name unchanged.
                    foreach (iGRow row in _granteesGrid.Rows)
                    {
                        if (row.Cells[0].Value.ToString().Equals(_componentPrivilegesUserControl.GranteeName))
                        {
                            hasContainGrantee = true;
                            row.Selected = true;
                            break;
                        }
                    }

                    if (!hasContainGrantee)
                    {
                        _granteesGrid.Rows[0].Selected = true;
                    }
                }
            }

            _IsLoaded = true;
            UpdateControls();
        }

        /// <summary>
        /// Apply all changes function for external call.
        /// </summary>
        public void ApplyChanges()
        {
            //Commit the changes.
            if (_componentPrivilegesUserControl!=null)            
                CommitChanges(_componentPrivilegesUserControl.ListCommandString);
        }

        /// <summary>
        /// Reset user changes function for external call.
        /// </summary>
        public void ResetChanges()
        {
            if (_componentPrivilegesUserControl != null)
                _componentPrivilegesUserControl.ResetAllPrivileges();
               _HasNewGranteeAdded = false;
        }

        #endregion Public Methods      

    
    
        #region ICloneToWindow Members

        public Control Clone()
        {
            return new AlterComponentPrivilegesUserControl(this._connectionDefinition, _defaultComponentName);
        }

        public string WindowTitle
        {
            get { return Properties.Resources.Privileges; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
        }

        #endregion
    }

    /// <summary>
    /// Custom handler to load the components and grantee information
    /// </summary>
    public class CompPrivDataHandler : TabularDataDisplayHandler
    {
        private AlterComponentPrivilegesUserControl _alterPrivilegesControl;

        public CompPrivDataHandler(AlterComponentPrivilegesUserControl alterPrivilegesControl)
        {
            _alterPrivilegesControl = alterPrivilegesControl;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            _alterPrivilegesControl.ResetChanges();
            _alterPrivilegesControl.LoadData();
        }

    }
}
