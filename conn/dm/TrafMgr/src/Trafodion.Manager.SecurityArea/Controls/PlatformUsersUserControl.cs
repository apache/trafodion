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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class PlatformUsersUserControl : UserControl, IDataDisplayControl, ICloneToWindow
    {

        #region Fields

        private static readonly string _theDirectoryServersConfigName = "_thePFUserWidgetConfig";
        UniversalWidgetConfig _theDSWidgetConfig = null;
        GenericUniversalWidget _theDSWidget = null;
        PlatformUsersDataHandler _theDSDataDisplayHandler = null;
        ConnectionDefinition _theConnectionDefinition;
        TrafodionIGrid _theDSGrid = null;
        String _theTitle = "Platform Users";
        ArrayList _commands = new ArrayList();
        Connection _conn = null;
        private delegate void HandleEvent(object obj, EventArgs e);  

        #endregion Fields

        #region Properties

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set 
            {
                _theConnectionDefinition = value;
                if ((_theDSWidgetConfig != null) && (_theDSWidgetConfig.DataProviderConfig != null))
                {
                    _theDSWidgetConfig.DataProviderConfig.ConnectionDefinition = value;
                }
            }
        }

        public DataProvider DataProvider
        {
            get;
            set;
        }

        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theDSWidgetConfig; }
            set { _theDSWidgetConfig = value; }
        }

        public IDataDisplayHandler DataDisplayHandler
        {
            get;
            set;
        }

        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        public string WindowTitle
        {
            get { return _theTitle; }
        }

        #endregion Properties

        #region Constructors

        public PlatformUsersUserControl()
        {
            InitializeComponent();
            _deleteButton.Enabled = false;
            _editButton.Enabled = false;
            _addLikeButton.Enabled = false;

        }

        public PlatformUsersUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefn = aConnectionDefinition;
            ShowWidgets();
            this._addButton.Enabled = !SystemSecurity.FindSystemModel(aConnectionDefinition).IsViewOnly;
        }

        public PlatformUsersUserControl(PlatformUsersUserControl aPlatformUsersUserControl)
            : this(aPlatformUsersUserControl.ConnectionDefn)
        {
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refresh the grid display
        /// </summary>
        public void Refresh()
        {
            DoRefresh();
        }

        /// <summary>
        /// To clone a self.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            PlatformUsersUserControl thePlatformUsersUserControl = new PlatformUsersUserControl(this);
            return thePlatformUsersUserControl;
        }

        public void PersistConfiguration()
        {
        }
        #endregion Public methods

        #region Private methods

        private void ShowWidgets()
        {
            // Remove all current contents and add the alerts widget
            _theWidgetPanel.Controls.Clear();

            _theDSWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
            _theDSWidgetConfig.Name = _theTitle;
            _theDSWidgetConfig.Title = _theTitle;
            _theDSWidgetConfig.ShowProperties = false;
            _theDSWidgetConfig.ShowToolBar = true;
            _theDSWidgetConfig.ShowChart = false;
            _theDSWidgetConfig.ShowTimerSetupButton = false;
            _theDSWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;

            DatabaseDataProviderConfig _dbConfig = _theDSWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            _dbConfig.SQLText = "Select USERNAME as EXTERNAL_NAME, ISDEFAULT as IS_PRIMARY, ROLE as ROLE_NAME, CREATEDBY as CREATING_USER, USERTYPE as USER_TYPE, CREATETIME from MANAGEABILITY.TRAFODION_SECURITY.USER_INFO WHERE USERTYPE IN (2)  ORDER BY USERNAME ASC  FOR BROWSE ACCESS";
            //_dbConfig.SQLText = "SELECT EXTERNAL_NAME, IS_PRIMARY, ROLE_NAME, CREATING_USER, USER_TYPE  FROM MANAGEABILITY.USER_MANAGEMENT.USERS WHERE USER_TYPE IN (2) ORDER BY EXTERNAL_NAME ASC FOR BROWSE ACCESS";
            //_dbConfig.TimerPaused = true;
            _theDSWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

            _theDSWidget = new GenericUniversalWidget();
            _theDSWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_theDSWidget.DataDisplayControl).LineCountFormat = "Platform Users";
            _theDSDataDisplayHandler = new PlatformUsersDataHandler(this);
            _theDSWidget.DataDisplayControl.DataDisplayHandler = _theDSDataDisplayHandler;
            _theDSWidget.UniversalWidgetConfiguration = _theDSWidgetConfig;
            _theDSWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_theDSWidget);

            _theDSGrid = ((TabularDataDisplayControl)_theDSWidget.DataDisplayControl).DataGrid;
            _theDSGrid.SelectionChanged += _theDSGrid_SelectionChanged;
            _theDSGrid.DoubleClickHandler = UserEdit_Handler;
            _theDSGrid.RowMode = true;

            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_theDSWidget.DataDisplayControl).ShowExportButtons = false;
            Control exportButtonsControl = _theDSGrid.GetButtonControl();
            exportButtonsControl.Dock = DockStyle.Right;
            _theExportButtonPanel.Controls.Clear();
            _theExportButtonPanel.Controls.Add(exportButtonsControl);

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theDSWidget.StartDataProvider();

            this.Disposed += new EventHandler(PlatformUsersUserControl_Disposed);
        }

        void _theDSGrid_SelectionChanged(object sender, EventArgs e)
        {
            enableDisableButtons();
        }

        private void enableDisableButtons()
        {
            _deleteButton.Enabled = false;
            _editButton.Enabled = false;
            _addLikeButton.Enabled = false;
            if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                if ((_theDSGrid.SelectedRows.Count > 0) && (_theDSGrid.SelectedRows[0].Index >= 0))
                {
                    _deleteButton.Enabled = true;
                    if (_theDSGrid.SelectedRows.Count == 1)
                    {
                        _editButton.Enabled = true;
                        _addLikeButton.Enabled = true;
                    }
                }
            }
        }

        void  PlatformUsersUserControl_Disposed(object sender, EventArgs e)
        {
 	        
        }

        /// <summary>
        /// Cleanup
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Remove the event handlers
                this.RemoveHandlers();
            }
        }
        
        private void AddHandlers()
        {
            if (_theDSWidget != null && _theDSWidget.DataProvider != null)
            {
                //Associate the event handlers
                _theDSWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _theDSWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDSWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;

            }
        }

        private void RemoveHandlers()
        {
            if (_theDSWidget != null && _theDSWidget.DataProvider != null)
            {
                //Remove the event handlers
                _theDSWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _theDSWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theDSWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Refresh the Grid.
        /// </summary>
        private void DoRefresh()
        {
            _theDSWidget.DataProvider.RefreshData();
        }
        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
            enableDisableButtons();
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            enableDisableButtons();
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            enableDisableButtons();
        }

        #endregion Private methods

        private void PlatformUsersUserControl_Load(object sender, EventArgs e)
        {

        }

        private void _addButton_Click(object sender, EventArgs e)
        {
            ManageUserDialog dialog = new ManageUserDialog();
            PlatformUserPanel userPanel = new PlatformUserPanel();
            userPanel.Mode = User.EditMode.Create;
            userPanel.OnSuccessImpl += DoRefresh;
            dialog.OnOk += userPanel.AddUser;
            userPanel.ConnectionDefinition = ConnectionDefn;
            userPanel.SetupDefaultRoles();
            userPanel.SetupDefaultPolicy();
            dialog.ShowControl(userPanel, "Add Platform User");
            dialog.OnOk -= userPanel.AddUser;
            userPanel.OnSuccessImpl -= DoRefresh;
        }

        private void _editButton_Click(object sender, EventArgs e)
        {
            if (_theDSGrid == null || _theDSGrid.CurRow == null)
            {
                return;
            }

            int idx = _theDSGrid.CurRow.Index;
            OnEditRequest(idx);
        }

        private void OnEditRequest(int idx)
        {
            string userName = ((string)_theDSGrid.CurRow.Cells["User Name"].Value).Trim();

            ManageUserDialog dialog = new ManageUserDialog();
            PlatformUserPanel userPanel = new PlatformUserPanel();
            userPanel.Mode = User.EditMode.Update;
            userPanel.OnSuccessImpl += DoRefresh;
            dialog.OnOk += userPanel.UpdateUser;
            userPanel.ConnectionDefinition = ConnectionDefn;
            userPanel.SetupDefaultPolicy();
            userPanel.SetUserFromDB(userName);
            dialog.ShowControl(userPanel, "Edit User");
            dialog.OnOk -= userPanel.UpdateUser;
            userPanel.OnSuccessImpl -= DoRefresh;
        }

        private void _deleteButton_Click(object sender, EventArgs e)
        {
            if ((_theDSGrid == null) || (_theDSGrid.SelectedRows.Count == 0))
            {
                return;
            }
            List<string> usersToDelete = new List<string>();
            foreach (iGRow row in _theDSGrid.SelectedRows)
            {
                usersToDelete.Add(((string)row.Cells["User Name"].Value).Trim());
            }

            if (usersToDelete.Count > 0)
            {
                DialogResult userInput = MessageBox.Show(string.Format("Are you sure you want to delete {0} user(s)?", usersToDelete.Count), "Delete Users", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
                if (userInput == DialogResult.OK)
                {
                    User user = new User(ConnectionDefn);
                    LongOperationHandler _theStatusHandler = new LongOperationHandler();
                    _theStatusHandler.StatusDialog = new LongOperationStatusDialog("Delete Users Status", "Delete User in Progress...");
                    user.OnSecurityBackendOperation += _theStatusHandler.OnSecurityBackendOperation;


                    DataTable status = user.DeleteUsers(usersToDelete);

                    //Close the add status dialog
                    user.OnSecurityBackendOperation -= _theStatusHandler.OnSecurityBackendOperation;

                    //Display the final dialog if needed
                    bool allOperationsSuccessful = user.AreAllOperationsSuccessful(status);
                    bool deleteUserOperationsSuccessful = user.AreAllOperationsSuccessful(status, "Delete user");

                    showMultiMessageDialog("Delete users", status, allOperationsSuccessful, deleteUserOperationsSuccessful);

                    //Refresh the right pane if needed
                    if (deleteUserOperationsSuccessful)
                    {
                        DoRefresh();
                    }
                }
            }
        }

        //Helper method to display the multi message dialog
        private void showMultiMessageDialog(string operation, DataTable status, bool allOperationsSuccessful, bool mainOperationSuccess)
        {
            if (!allOperationsSuccessful)
            {
                string message = (mainOperationSuccess) ? "{0} succeeded with errors." : "{0} failed.";

                TrafodionMultipleMessageDialog multiMessageDialog = new TrafodionMultipleMessageDialog(string.Format(message, operation),
                    status,
                    (mainOperationSuccess) ? System.Drawing.SystemIcons.Warning : System.Drawing.SystemIcons.Error);
                multiMessageDialog.ShowDialog();
            }
        }

        private void _addLikeButton_Click(object sender, EventArgs e)
        {
            if (_theDSGrid == null || _theDSGrid.CurRow == null)
            {
                return;
            }

            int idx = _theDSGrid.CurRow.Index;
            string userName = ((string)_theDSGrid.CurRow.Cells["User Name"].Value).Trim();

            ManageUserDialog dialog = new ManageUserDialog();
            PlatformUserPanel userPanel = new PlatformUserPanel();
            userPanel.Mode = User.EditMode.CreateLike;
            userPanel.OnSuccessImpl += DoRefresh;
            dialog.OnOk += userPanel.AddUser;
            userPanel.ConnectionDefinition = ConnectionDefn;
            userPanel.SetupDefaultRoles();
            userPanel.SetupDefaultPolicy();
            userPanel.SetUserFromDB(userName);
            dialog.ShowControl(userPanel, "Add Platform User Like");
            dialog.OnOk -= userPanel.AddUser;
            userPanel.OnSuccessImpl -= DoRefresh;
        }


        /// <summary>
        /// Event handler when cell clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        //private void DSGrid_CellClick(object sender, iGCellClickEventArgs e)
        //{
        //    _deleteButton.Enabled = false;
        //    _editButton.Enabled = false;
        //    _addLikeButton.Enabled = false;
        //    if (e.RowIndex >= 0)
        //    {
        //        _deleteButton.Enabled = true;
        //        if (_theDSGrid.SelectedRows.Count == 1)
        //        {
        //            _editButton.Enabled = true;
        //            _addLikeButton.Enabled = true;
        //        }
        //    }
        //}

        /// <summary>
        /// Double click event for the Grid, which will bring up the Edit dialog.
        /// </summary>
        /// <param name="rowIndex"></param>
        private void UserEdit_Handler(int rowIndex)
        {
            if (_theDSGrid != null &&(!SystemSecurity.FindSystemModel(this._theConnectionDefinition).IsViewOnly))
            {
                OnEditRequest(rowIndex);
            }
        }
    }

   #region PlatformUsersDataHandler Class 

    public class PlatformUsersDataHandler : TabularDataDisplayHandler
    {
        #region Fields
        private PlatformUsersUserControl _thePlatformUsersUserControl;
        private ArrayList _Columns = new ArrayList();
        string[] columns = new string[] { "User Name", "Role", "Created By" };
        enum ColumnPos { UserName = 0, DefaultRole, CreatedBy };
        #endregion Fields


        #region Properties


        #endregion Properties


        #region Constructors

        public PlatformUsersDataHandler(PlatformUsersUserControl aPlatformUsersUserControl)
        {
            _thePlatformUsersUserControl = aPlatformUsersUserControl;
        }

        #endregion Constructors


        #region Public methods
        
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            lock (this)
            {
                populate(aConfig, aDataTable, aDataGrid);
            }
        }

        #endregion Public methods


        #region Private methods

        //Popultes the datatable using the data returned by the data provider and displays it in the grid
        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if (null == aDataTable)
            {
                return;
            }

            DataTable dataTable = new DataTable();
            foreach (string columnName in columns)
            {
                dataTable.Columns.Add(columnName, typeof(string));
            }

            User tempUser = null;
            foreach (DataRow row in aDataTable.Rows)
            {                
                if (tempUser == null)
                {
                    tempUser = new User(aConfig.DataProviderConfig.ConnectionDefinition);
                    populateUser(tempUser, row);
                }
                else if (tempUser.UserName.Equals(row["EXTERNAL_NAME"]))
                {
                    //We are dealing with the same usr
                    addRoleToUser(tempUser, row);
                }
                else
                {
                    //The row has a new user, so we add the existing user to the table
                    //
                    addRowToTable(tempUser, dataTable);
                    tempUser = new User(aConfig.DataProviderConfig.ConnectionDefinition);
                    populateUser(tempUser, row);
                }
            }
            //deal with the last row
            if (tempUser != null)
            {
                addRowToTable(tempUser, dataTable);
            }

            base.DoPopulate(aConfig, dataTable, aDataGrid);

            string gridHeaderText = string.Format("{0} Platform users configured", dataTable.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);
            if (dataTable.Rows.Count > 0)
            {
                //aDataGrid.ResizeGridColumns(dataTable);
                aDataGrid.ResizeGridColumns(dataTable, 7, 20);
            }
        }

        //sets the default or additional role of the user
        private void addRoleToUser(User user, DataRow aRow)
        {
            string isPrimary = ((string)aRow["IS_PRIMARY"]).Trim();
            if (isPrimary.Equals("Y", StringComparison.CurrentCultureIgnoreCase))
            {
                user.DefaultRole = ((string)aRow["ROLE_NAME"]).Trim();
            }
            else
            {
                user.AddAdditionalRole(((string)aRow["ROLE_NAME"]).Trim());
            }
        }

        //helper method to populate a user object from the data row passed
        private void populateUser(User user, DataRow aRow)
        {
            user.UserName = aRow["EXTERNAL_NAME"] as string;
            addRoleToUser(user, aRow);
            user.CreatingUser = aRow["CREATING_USER"] as string;
            user.UserType = (User.UserTypeEnum)aRow["USER_TYPE"];
        }

        //helper method to add a user object to the datatable passed
        private void addRowToTable(User user, DataTable aDataTable)
        {
            DataRow row = aDataTable.NewRow();
            foreach (string columnName in columns)
            {
                string value = "";
                if (columnName.Equals(columns[(int)ColumnPos.UserName]))
                {
                    value = user.UserName;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.DefaultRole]))
                {
                    value = user.DefaultRole;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.CreatedBy]))
                {
                    value = user.CreatingUser;
                }
                row[columnName] = value;

            }
            aDataTable.Rows.Add(row);
        }
        #endregion Private methods
    }

    #endregion PlatformUsersDataHandler Class
}
