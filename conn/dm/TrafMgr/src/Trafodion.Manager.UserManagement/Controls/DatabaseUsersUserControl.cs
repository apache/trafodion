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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.UserManagement.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.UserManagement.Controls
{
    public partial class DatabaseUsersUserControl : UserControl, IDataDisplayControl, ICloneToWindow
    {
        #region Static Members
        private static readonly string DbUsersWidgetConfigName = "UserMgmt_DbUsersWidgetConfig";
        #endregion

        #region Fields
        UniversalWidgetConfig _theDbUsersWidgetConfig = null;
        GenericUniversalWidget _theDbUsersWidget = null;
        DatabaseUsersDataHandler _theDbUsersDataDisplayHandler = null;
        ConnectionDefinition _theConnectionDefinition;
        TrafodionIGrid _theDbUsersGrid = null;
        String _theTitle = Properties.Resources.DatabaseUsersTitle;
        Connection _conn = null;
        private delegate void HandleEvent(object obj, EventArgs e);         

        #endregion Fields

        #region Properties

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set 
            {
                if (_theConnectionDefinition != null)
                {
                    _theDbUsersWidget.DataProvider.Stop();
                }
                _theConnectionDefinition = value;
                if ((_theDbUsersWidgetConfig != null) && (_theDbUsersWidgetConfig.DataProviderConfig != null))
                {
                    if (_theDbUsersGrid != null)
                    {
                        _theDbUsersGrid.Rows.Clear();
                    }
                    _theDbUsersWidgetConfig.DataProviderConfig.ConnectionDefinition = value;
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
            get { return _theDbUsersWidgetConfig; }
            set { _theDbUsersWidgetConfig = value; }
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

        public DatabaseUsersUserControl()
        {
            InitializeComponent();
            _theUnRegisterButton.Enabled = false;
            _sqlPrivilegesButton.Enabled = false;
            _theAlterUserButton.Enabled = false;
        }

        public DatabaseUsersUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefn = aConnectionDefinition;
            ShowWidgets();
        }

        public DatabaseUsersUserControl(DatabaseUsersUserControl aDatabaseUsersUserControl)
            : this(aDatabaseUsersUserControl.ConnectionDefn)
        {
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Refresh the grid display
        /// </summary>
        public override void Refresh()
        {
            DoRefresh();
        }

        /// <summary>
        /// To clone a self.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            DatabaseUsersUserControl theDatabaseUsersUserControl = new DatabaseUsersUserControl(this);
            return theDatabaseUsersUserControl;
        }

        public void PersistConfiguration()
        {
        }
        #endregion Public methods

        #region Private methods

        private string GetSQLText()
        {
            StringBuilder sbSql = new StringBuilder();
            sbSql.Append(" SElECT 	USER_NAME,");
            sbSql.Append("		USER_ID,");
            sbSql.Append("		LOGON_ROLE_ID,");
            sbSql.Append("		LOGON_ROLE_ID_NAME, ");
            sbSql.Append("		EXTERNAL_USER_NAME, ");
            sbSql.Append("		CREATE_TIME, ");
            sbSql.Append("		REDEF_TIME, ");
            sbSql.Append("		IS_VALID_USER, ");
            //Only show user immutable for M10 and above
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                sbSql.Append("  IS_AUTO_REGISTERED AS IMMUTABLE, ");
            }
            //Only show authentication option for M8 and above
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ131)
            {
                sbSql.Append("  AUTHENTICATION_TYPE, ");
            }            
            sbSql.Append("		ROLE_NAME, ");
            sbSql.Append("		ROLE_ID, ");
            sbSql.Append("		GRANTOR_USER_ID,");
            sbSql.Append("		GRANTOR_USER_NAME, ");
            sbSql.Append("		GRANTEE_ID ");
            sbSql.Append(" FROM TRAFODION_INFORMATION_SCHEMA.USER_INFO U LEFT JOIN TRAFODION_INFORMATION_SCHEMA.USER_ROLE_INFO R ");
            sbSql.Append(" ON U.USER_ID=R.GRANTEE_ID ");
            sbSql.Append(" ORDER BY U.USER_NAME FOR READ UNCOMMITTED ACCESS;");
            return sbSql.ToString();
        }

        private void ShowWidgets()
        {
            // Remove all current contents and add the alerts widget
            _theWidgetPanel.Controls.Clear();
            //Load the Users widget config from persistence. 
            //if it does not exist or there is an error reading the persistence, create
            //a default Users widget config
            _theDbUsersWidgetConfig = WidgetRegistry.GetConfigFromPersistence(DbUsersWidgetConfigName);
            if (_theDbUsersWidgetConfig == null)
            {
                _theDbUsersWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _theDbUsersWidgetConfig.Name = DbUsersWidgetConfigName;
                _theDbUsersWidgetConfig.Title = _theTitle;
                _theDbUsersWidgetConfig.ShowProperties = false;
                _theDbUsersWidgetConfig.ShowToolBar = true;
                _theDbUsersWidgetConfig.ShowChart = false;
                _theDbUsersWidgetConfig.ShowTimerSetupButton = false;
                _theDbUsersWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            _theDbUsersWidgetConfig.ShowHelpButton = true;
            _theDbUsersWidgetConfig.HelpTopic = HelpTopics.UnderstandDBUsersPane;

            DatabaseDataProviderConfig _dbConfig = _theDbUsersWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;

            _dbConfig.SQLText = GetSQLText();

            if (_dbConfig.ColumnMappings == null)
            {
                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("Database User Name", "Database User Name", 200));
                columnMappings.Add(new ColumnMapping("User ID", "User ID", 60));
                columnMappings.Add(new ColumnMapping("Directory-Service User Name", "Directory-Service User Name", 200));
                columnMappings.Add(new ColumnMapping("Creation Time", "Creation Time", 185));
                columnMappings.Add(new ColumnMapping("Redefinition Time", "Redefinition Time", 185));
                columnMappings.Add(new ColumnMapping("Valid User", "Valid User", 80));
                columnMappings.Add(new ColumnMapping("Immutable", "Immutable", 80));
                columnMappings.Add(new ColumnMapping("Authentication", "Authentication", 95));
                columnMappings.Add(new ColumnMapping("Primary Role", "Primary Role", 128));
                columnMappings.Add(new ColumnMapping("Granted Roles", "Granted Roles", 180));

                _dbConfig.ColumnMappings = columnMappings;
            }
            List<string> listDefaultCols = new List<string> { "Database User Name", "User ID", "Directory-Service User Name", 
                                                                "Creation Time", "Redefinition Time","Valid User","Immutable",
                                                                "Authentication","Granted Roles","Primary Role"};
           
            _dbConfig.DefaultVisibleColumnNames = listDefaultCols;
            _theDbUsersWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

            _theDbUsersWidget = new GenericUniversalWidget();
            _theDbUsersWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_theDbUsersWidget.DataDisplayControl).LineCountFormat = "Database Users";
            _theDbUsersDataDisplayHandler = new DatabaseUsersDataHandler(this);
            _theDbUsersWidget.DataDisplayControl.DataDisplayHandler = _theDbUsersDataDisplayHandler;
            _theDbUsersWidget.UniversalWidgetConfiguration = _theDbUsersWidgetConfig;
            _theDbUsersWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_theDbUsersWidget);

            _theDbUsersGrid = ((TabularDataDisplayControl)_theDbUsersWidget.DataDisplayControl).DataGrid;
            _theDbUsersGrid.SelectionChanged += _theDSGrid_SelectionChanged;
            //_theDbUsersGrid.AutoResizeCols = true;

            //Set selected rows color while losed focus.
            _theDbUsersGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _theDbUsersGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;

            UserMgmtSystemModel systemModel = UserMgmtSystemModel.FindSystemModel(ConnectionDefn);
            if (systemModel.IsAdminUser)
            {
                TrafodionIGridToolStripMenuItem registerMenuItem = new TrafodionIGridToolStripMenuItem();
                registerMenuItem.Text = Properties.Resources.Register;
                registerMenuItem.Click += new EventHandler(registerMenuItem_Click);
                _theDbUsersGrid.AddContextMenu(registerMenuItem);

                TrafodionIGridToolStripMenuItem unregisterMenuItem = new TrafodionIGridToolStripMenuItem();
                unregisterMenuItem.Text = Properties.Resources.UnRegister;
                unregisterMenuItem.Click += new EventHandler(unregisterMenuItem_Click);
                _theDbUsersGrid.AddContextMenu(unregisterMenuItem);

                TrafodionIGridToolStripMenuItem alterUserMenuItem = new TrafodionIGridToolStripMenuItem();
                alterUserMenuItem.Text = Properties.Resources.AlterUser;
                alterUserMenuItem.Click += new EventHandler(alterUserMenuItem_Click);
                _theDbUsersGrid.AddContextMenu(alterUserMenuItem);

                TrafodionIGridToolStripMenuItem viewSQLPrivMenuItem = new TrafodionIGridToolStripMenuItem();
                viewSQLPrivMenuItem.Text = Properties.Resources.ShowSQLPrivilegesDialogTitle;
                viewSQLPrivMenuItem.Click += new EventHandler(viewSQLPrivMenuItem_Click);
                _theDbUsersGrid.AddContextMenu(viewSQLPrivMenuItem);
            }

            _theDbUsersGrid.DoubleClickHandler = UserEdit_Handler;   
            _theDbUsersGrid.RowMode = true;

            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_theDbUsersWidget.DataDisplayControl).ShowExportButtons = false;

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theDbUsersWidget.StartDataProvider();

            this.Disposed += new EventHandler(DatabaseUsersUserControl_Disposed);

        }

        void viewSQLPrivMenuItem_Click(object sender, EventArgs e)
        {
            _sqlPrivilegesButton.PerformClick();
        }

        void unregisterMenuItem_Click(object sender, EventArgs e)
        {
            _theUnRegisterButton.PerformClick();
        }

        void registerMenuItem_Click(object sender, EventArgs e)
        {
            _theRegisterButton.PerformClick();
        }

        void alterUserMenuItem_Click(object sender, EventArgs e)
        {
            _theAlterUserButton.PerformClick();
        }
        void  DatabaseUsersUserControl_Disposed(object sender, EventArgs e)
        {
            _theDbUsersGrid.SelectionChanged -= _theDSGrid_SelectionChanged;
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
            if (_theDbUsersWidget != null && _theDbUsersWidget.DataProvider != null)
            {
                //Associate the event handlers
                _theDbUsersWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _theDbUsersWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theDbUsersWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;

            }
        }

        private void RemoveHandlers()
        {
            if (_theDbUsersWidget != null && _theDbUsersWidget.DataProvider != null)
            {
                //Remove the event handlers
                _theDbUsersWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _theDbUsersWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theDbUsersWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UserManagement,
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UserManagement,
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.UserManagement,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Refresh the Grid.
        /// </summary>
        private void DoRefresh()
        {
            DatabaseDataProviderConfig _dbConfig = _theDbUsersWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            _dbConfig.SQLText = GetSQLText();
            _theDbUsersWidget.DataProvider.RefreshData();
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

        private void DatabaseUsersUserControl_Load(object sender, EventArgs e)
        {

        }

        private void _theUnRegisterButton_Click(object sender, EventArgs e)
        {
            if ((_theDbUsersGrid == null) || (_theDbUsersGrid.SelectedRows.Count == 0))
            {
                return;
            }
            List<string> usersToDelete = new List<string>();
            foreach (iGRow row in _theDbUsersGrid.SelectedRows)
            {
                usersToDelete.Add(((string)row.Cells[0].Value).Trim());
            }

            if (usersToDelete.Count > 0)
            {
                DialogResult userInput = MessageBox.Show(string.Format(Properties.Resources.UnRegisterMsg1, usersToDelete.Count), "Un-Register Users", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
                if (userInput == DialogResult.OK)
                {
                    UserMgmtSystemModel userSystemModel = UserMgmtSystemModel.FindSystemModel(ConnectionDefn);
                    //User user = new User();
                    DataTable resultsTable = new DataTable();
                    Object[] parameters = new Object[] { usersToDelete };
                    TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.UnRegisterMsg2, userSystemModel, "UnRegisterUsers", parameters);
                    TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                    progressDialog.ShowDialog();
                    if (progressDialog.Error != null)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error un-registering user(s)",
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

                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(string.Format(infoMsg, "Unregistering the user(s)"), resultsTable, iconType);
                    mmd.ShowDialog();                   

                    //Refresh the right pane if needed
                    if (successCount > 0)
                    {
                        DoRefresh();
                    }
                }
            }
        }

        private void _theRegisterButton_Click(object sender, EventArgs e)
        {

            RegisterUserUserControl registerUserControl = new RegisterUserUserControl(ConnectionDefn);
            registerUserControl.OnSuccessImpl += DoRefresh;

            Utilities.LaunchManagedWindow("Register User(s)", registerUserControl, ConnectionDefn, registerUserControl.Size, true);
        }

        
        void _theDSGrid_SelectionChanged(object sender, EventArgs e)
        {
            enableDisableButtons();
        }

        private void enableDisableButtons()
        {
            if (_theDbUsersGrid.SelectedRowIndexes.Count > 0)
            {
                _theUnRegisterButton.Enabled = true;
                _sqlPrivilegesButton.Enabled = true;
                _theAlterUserButton.Enabled = true;
            }
            else
            {
                _theUnRegisterButton.Enabled = false;
                _sqlPrivilegesButton.Enabled = false;
                _theAlterUserButton.Enabled = false;
            }
        }
        /// <summary>
        /// Double click event for the Grid, which will bring up the Edit dialog.
        /// </summary>
        /// <param name="rowIndex"></param>
        private void UserEdit_Handler(int rowIndex)
        {
            iGRow row = _theDbUsersGrid.Rows[rowIndex];
            string directoryServiceUserName = row.Cells["Directory-Service User Name"].Value.ToString();
            string userName = row.Cells["Database User Name"].Value.ToString();
            string validUser = row.Cells["Valid User"].Value.ToString();

            string immuteUser = string.Empty;
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                immuteUser=row.Cells["Immutable"].Value.ToString();
            }
            string authentication = "";
            //Only show remote authentication option for M8 and above
            if (_theConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ131)
            {
                authentication = row.Cells["Authentication"].Value.ToString();
            }
            AlterUserUserControl alterUserControl = new AlterUserUserControl(ConnectionDefn, directoryServiceUserName, userName, validUser, authentication, immuteUser);
            alterUserControl.OnSuccessImpl += DoRefresh;
            System.Drawing.Size size = alterUserControl.Size;
            size.Height += 30; //account for the banner control
            Utilities.LaunchManagedWindow("Alter User: " + userName, alterUserControl, ConnectionDefn, size, true);
        }
        private void sqlPrivilegesButton_Click(object sender, EventArgs e)
        {
            List<string> userNames = new List<string>();
            foreach (iGRow row in _theDbUsersGrid.Rows)
            {
                userNames.Add(row.Cells["Database User Name"].Value.ToString());
            }

            int selectedIndex = _theDbUsersGrid.SelectedRowIndexes.Count > 0 ? _theDbUsersGrid.SelectedRowIndexes[0] : -1;
            AllSQLPrivilegesUserControl privilegesControl = new AllSQLPrivilegesUserControl(WorkMode.UserMode, ConnectionDefn, userNames, selectedIndex);
            Utilities.LaunchManagedWindow(Properties.Resources.ShowSQLPrivilegesDialogTitle + " - User Name: " + _theDbUsersGrid.Rows[selectedIndex].Cells["Database User Name"].Value, privilegesControl, ConnectionDefn, privilegesControl.Size, true);
        }

        private void _theAlterUserButton_Click(object sender, EventArgs e)
        {
            if (_theDbUsersGrid.SelectedRows.Count > 0)
            {
                UserEdit_Handler(_theDbUsersGrid.SelectedRowIndexes[0]);
            }

        }
        #endregion Private methods
        
    }

   #region DatabaseUsersDataHandler Class 

    public class DatabaseUsersDataHandler : TabularDataDisplayHandler
    {
        #region Fields
        private DatabaseUsersUserControl _theDatabaseUsersUserControl;
        private ArrayList _Columns = new ArrayList();
        public string[] columns = new string[] { "Database User Name", 
                                                "User ID", 
                                                "Directory-Service User Name", 
                                                "Creation Time", 
                                                "Redefinition Time",
                                                //"Auto Registered",
                                                "Valid User",
                                                "Immutable",
                                                "Authentication",
                                                "Granted Roles",
                                                "Primary Role"
                                                 };
        public enum ColumnPos { UserName = 0, 
                                                UserID, 
                                                ExternalName,
                                                CreateTime,
                                                RedefineTime,
                                                //AutoRegistered,
                                                ValidUser,
                                                Immutable,
                                                Authentication,
                                                GrantedRoles,
                                                DefaultRole
                                };

        private const string AUTH_OPTION_ENTERPRISE = "Enterprise";
        private const string AUTH_OPTION_CLUSTER = "Cluster";
        private const string AUTH_OPTION_REMOTE = "Remote";

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors
        public DatabaseUsersDataHandler(DatabaseUsersUserControl aDatabaseUsersUserControl)
        {
            _theDatabaseUsersUserControl = aDatabaseUsersUserControl;
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
                    tempUser = new User();
                    populateUser(tempUser, row);
                }
                else if (tempUser.UserName.Equals(row["USER_NAME"]))
                {
                    //We are dealing with the same usr                    
                    tempUser.AddAdditionalRole(row["ROLE_NAME"].ToString());
                }
                else
                {
                    //The row has a new user, so we add the existing user to the table
                    //
                    addRowToTable(tempUser, dataTable);
                    tempUser = new User();
                    populateUser(tempUser, row);
                }
            }
            //deal with the last row
            if (tempUser != null)
            {
                addRowToTable(tempUser, dataTable);
            }

            if (_theDatabaseUsersUserControl.ConnectionDefn.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ131)
            {
                dataTable.Columns.Remove("Authentication");
            }
            if (_theDatabaseUsersUserControl.ConnectionDefn.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                dataTable.Columns.Remove("Immutable");
            } 
            base.DoPopulate(aConfig, dataTable, aDataGrid);
            string gridHeaderText = string.Format(Properties.Resources.UserGridTitle, aDataGrid.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);
        }

        //helper method to populate a user object from the data row passed
        private void populateUser(User user, DataRow aRow)
        {
            user.UserName = aRow["USER_NAME"] as string;
            user.UserId = Convert.ToInt32(aRow["USER_ID"]);
            user.ExternalUserName = aRow["EXTERNAL_USER_NAME"] as string;
            user.CreateTime = getDate(aRow["CREATE_TIME"]);
            user.RedefTime = getDate(aRow["REDEF_TIME"]);
            //user.IsAutoRegistered = Utilities.YesNo((aRow["IS_AUTO_REGISTERED"] as string).Trim());
            user.IsValidUser =  Utilities.YesNo((aRow["IS_VALID_USER"] as string).Trim());
            if (aRow.Table.Columns.Contains("IMMUTABLE"))
            {
                user.Immutable =Utilities.YesNo((aRow["IMMUTABLE"] as string).Trim());
            }
            else
            {
                user.Immutable = string.Empty;
            }
            if (aRow.Table.Columns.Contains("AUTHENTICATION_TYPE"))
            {
                user.AuthenticationMode =getAuthType(aRow["AUTHENTICATION_TYPE"] as string);
            }
            else
            {
                user.AuthenticationMode = string.Empty;
            }
            if ((aRow["ROLE_NAME"] as string) != null)
            {
                user.AddAdditionalRole(((string)aRow["ROLE_NAME"]).Trim());
            }
            user.LogonRoleName = aRow["LOGON_ROLE_ID_NAME"] as string;
        }
        private string getAuthType(string authType)
        {
            if (_theDatabaseUsersUserControl.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                if (AUTH_OPTION_REMOTE.Equals(authType, System.StringComparison.InvariantCultureIgnoreCase))
                {
                    return AUTH_OPTION_CLUSTER;
                }
                else
                {
                    return AUTH_OPTION_ENTERPRISE;
                }
            }
            return authType;
        }
        private string getDate(Object objTime)
        {
            string strReturn = "";
            try
            {
                //check date validation and set blank if it's invalid.
                long timeVar = Convert.ToInt64(objTime);
                strReturn = (new JulianTimestamp(timeVar)).ToString();
            }
            catch 
            {
                strReturn = "";
            }
            return strReturn;
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
                    value = user.UserName.Trim();
                }
                else if (columnName.Equals(columns[(int)ColumnPos.UserID]))
                {
                    value = user.UserId.ToString().Trim();
                }
                else if (columnName.Equals(columns[(int)ColumnPos.ExternalName]))
                {
                    value = user.ExternalUserName.Trim();
                }
                else if (columnName.Equals(columns[(int)ColumnPos.CreateTime]))
                {
                    value = user.CreateTime;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.RedefineTime]))
                {
                    value = user.RedefTime;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.ValidUser]))
                {
                    value = user.IsValidUser;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.Immutable]))
                {
                    value = user.Immutable;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.Authentication]))
                {
                    value = user.AuthenticationMode;
                }
                else if (columnName.Equals(columns[(int)ColumnPos.GrantedRoles]))
                {
                    value = User.GetAdditionalRoleString(user.AdditionalRoles);
                }
                else if (columnName.Equals(columns[(int)ColumnPos.DefaultRole]))
                {
                    value = user.LogonRoleName;
                }
                row[columnName] = value;

            }
            aDataTable.Rows.Add(row);
        }

        #endregion Private methods
    }

    #endregion DatabaseUsersDataHandler Class
}
