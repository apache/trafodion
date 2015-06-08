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
    public partial class UsersForRoleControl : UserControl, IDataDisplayControl, ICloneToWindow
    {

        #region Fields

        UniversalWidgetConfig _usersWidgetConfig = null;
        GenericUniversalWidget _usersWidget = null;
        UserForRoleGridDataHandler _usersWidgetDisplayHandler = null;
        ConnectionDefinition _connectionDefinition;
        TrafodionIGrid _usersGrid = null;
        String _title = "Users";
        ArrayList _commands = new ArrayList();
        string _roleName = "";
        public const string USER_TYPE_COL_NAME = "USERTYPE";
        public const string ISDEFAULT_COL_NAME = "ISDEFAULT";
        Trafodion.Manager.Framework.TrafodionDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionDateTimeFormatProvider();
        bool _isInitialized = false;
        public event EventHandler GrantRevokeEvent;
        private delegate void HandleEvent(object obj, EventArgs e);  

        #endregion Fields

        #region Properties

        public Trafodion.Manager.Framework.TrafodionDateTimeFormatProvider DateFormatProvider
        {
            get { return _dateFormatProvider; }
            set { _dateFormatProvider = value; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set 
            {
                _connectionDefinition = value;
                if ((_usersWidgetConfig != null) && (_usersWidgetConfig.DataProviderConfig != null))
                {
                    _usersWidgetConfig.DataProviderConfig.ConnectionDefinition = value;
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
            get { return _usersWidgetConfig; }
            set { _usersWidgetConfig = value; }
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
            get { return _title; }
        }

        #endregion Properties

        #region Constructors

        public UsersForRoleControl()
        {
            InitializeComponent();
            _isInitialized = false;
            _title = Properties.Resources.Users;
            _revokeButton.Enabled = false;
        }

        public UsersForRoleControl(string roleName, ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefn = aConnectionDefinition;
            _roleName = roleName;
            _isInitialized = false;
            ShowWidgets();
        }

        public UsersForRoleControl(UsersForRoleControl aUsersForRoleControl)
            : this()
        {
            _isInitialized = false;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To clone a self.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            UsersForRoleControl theUsersForRoleControl = new UsersForRoleControl(this);
            return theUsersForRoleControl;
        }

        public void PersistConfiguration()
        {

        }

        public void InitializeWidget(string roleName, ConnectionDefinition aConnectionDefinition)
        {
            ConnectionDefn = aConnectionDefinition;
            _roleName = roleName;
            ShowWidgets();
        }

        #endregion Public methods

        #region Private methods

        private void ShowWidgets()
        {
            if (!_isInitialized)
            {
                // Remove all current contents and add the alerts widget
                _theWidgetPanel.Controls.Clear();

                _usersWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _usersWidgetConfig.Name = _title;
                _usersWidgetConfig.Title = _title;
                _usersWidgetConfig.ShowProperties = false;
                _usersWidgetConfig.ShowToolBar = true;
                _usersWidgetConfig.ShowChart = false;
                _usersWidgetConfig.ShowTimerSetupButton = false;
                _usersWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;

                DatabaseDataProviderConfig _dbConfig = _usersWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
                _dbConfig.SQLText = string.Format("SELECT USERNAME, USERTYPE, CREATETIME, CREATEDBY, ISDEFAULT " +
                                        "FROM MANAGEABILITY.TRAFODION_SECURITY.USER_INFO " +
                                        "WHERE ROLE = '{0}' " +
                                        "ORDER BY USERNAME FOR READ UNCOMMITTED ACCESS", _roleName.Trim());

                _dbConfig.CommandTimeout = 0;
                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("USERNAME", "User Name", 50));
                columnMappings.Add(new ColumnMapping("USERTYPE", "User Type", 50));
                columnMappings.Add(new ColumnMapping("CREATETIME", "Create Time", 50));
                columnMappings.Add(new ColumnMapping("CREATEDBY", "Created By", 50));
                columnMappings.Add(new ColumnMapping("ISDEFAULT", "Is Default Role", 50));

                _dbConfig.ColumnMappings = columnMappings;

                //_dbConfig.TimerPaused = true;
                _usersWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

                _usersWidget = new GenericUniversalWidget();
                _usersWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

                //Set the display properties of the widget and add it to the control
                ((TabularDataDisplayControl)_usersWidget.DataDisplayControl).LineCountFormat = Properties.Resources.Users;
                _usersWidgetDisplayHandler = new UserForRoleGridDataHandler(this);
                _usersWidget.DataDisplayControl.DataDisplayHandler = _usersWidgetDisplayHandler;
                _usersWidget.UniversalWidgetConfiguration = _usersWidgetConfig;
                _usersWidget.Dock = DockStyle.Fill;
                _theWidgetPanel.Controls.Add(_usersWidget);

                _usersGrid = ((TabularDataDisplayControl)_usersWidget.DataDisplayControl).DataGrid;
                _usersGrid.DoubleClickHandler = UserDetails_Handler;

                _usersGrid.RowSelectionInCellMode = iGRowSelectionInCellModeTypes.MultipleRows;
                _usersGrid.SelectionChanged += new EventHandler(_usersGrid_SelectionChanged);

                //Disable the export buttons so it does not show up within the universal widget panel
                //But get the export buttons from the grid and add them to their own panel
                ((TabularDataDisplayControl)_usersWidget.DataDisplayControl).ShowExportButtons = false;
                Control exportButtonsControl = _usersGrid.GetButtonControl();
                exportButtonsControl.Dock = DockStyle.Right;
                _theExportButtonPanel.Controls.Clear();
                _theExportButtonPanel.Controls.Add(exportButtonsControl);

                //Add event handlers to deal with data provider events
                AddHandlers();

                //Start it.
                _usersWidget.StartDataProvider();

                _isInitialized = true;
            }
            else
            {
                _usersWidget.DataProvider.Stop();
                _usersGrid.UpdateCountControlText(Properties.Resources.Users);

                _usersGrid.Clear();
                ((DatabaseDataProviderConfig)_usersWidget.DataProvider.DataProviderConfig).SQLText = string.Format("SELECT USERNAME, USERTYPE, CREATETIME, CREATEDBY, ISDEFAULT " +
                                        "FROM MANAGEABILITY.TRAFODION_SECURITY.USER_INFO " +
                                        "WHERE ROLE = '{0}' " +
                                        "ORDER BY USERNAME FOR READ UNCOMMITTED ACCESS", _roleName.Trim());
                _usersWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;
                _usersWidget.DataProvider.Start();
            }
        }

        void _usersGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
        }

        public void UpdateControls()
        {
            _revokeButton.Enabled = (_usersGrid != null && _usersGrid.SelectedRowIndexes.Count > 0);
        }

        void  UsersForRoleControl_Disposed(object sender, EventArgs e)
        {
 	        
        }

        void OnGrantRevokeEvent()
        {
            EventHandler handler = GrantRevokeEvent;
            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        void UserDetails_Handler(int rowIndex)
        {
            string userName = _usersGrid.Rows[rowIndex].Cells["USERNAME"].Value as string;
            if (!string.IsNullOrEmpty(userName))
            {
                ManageUserDialog dialog = new ManageUserDialog();
                DatabaseUserPanel dbUserPanel = new DatabaseUserPanel();
                dbUserPanel.Mode = User.EditMode.Update;
                dbUserPanel.OnSuccessImpl += DoRefresh;
                dialog.OnOk += dbUserPanel.UpdateUser;
                dbUserPanel.ConnectionDefinition = ConnectionDefn;
                dbUserPanel.SetUserFromDB(userName);
                dbUserPanel.SetupDefaultRoles();
                dialog.ShowControl(dbUserPanel, "Edit User");
                dialog.OnOk -= dbUserPanel.UpdateUser;
                dbUserPanel.OnSuccessImpl -= DoRefresh;
            }
        }

        /// <summary>
        /// Refresh the Grid.
        /// </summary>
        private void DoRefresh()
        {
            this._usersWidget.DataProvider.RefreshData();
            UpdateControls();
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
            if (_usersWidget != null && _usersWidget.DataProvider != null)
            {
                //Associate the event handlers
                _usersWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _usersWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _usersWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }

        private void RemoveHandlers()
        {
            if (_usersWidget != null && _usersWidget.DataProvider != null)
            {
                //Remove the event handlers
                _usersWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _usersWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _usersWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
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


        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            UpdateControls();
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
        }

        private void _revokeButton_Click(object sender, EventArgs e)
        {
            bool roleRevoked = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");
            
            Role _role = new Role(_roleName, _connectionDefinition);

            if (MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Do you really want to revoke the role for the selected users?", Properties.Resources.Confirm, MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                foreach (int rowIndex in _usersGrid.SelectedRowIndexes)
                {
                    Cursor = Cursors.WaitCursor;
                    string userName = _usersGrid.Rows[rowIndex].Cells[0].Value as string;
                    try
                    {
                        _role.Revoke(userName);
                        roleRevoked = true;
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { userName, ex.Message });
                    }
                    Cursor = Cursors.Default;
                }
            }
            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("The role could not be revoked for the following users", errorTable, System.Drawing.SystemIcons.Warning);
                mmd.ShowDialog();
            }
            if (roleRevoked)
            {
                OnGrantRevokeEvent();
                DoRefresh();
            }
        }

        private void _grantButton_Click(object sender, EventArgs e)
        {
            GrantRoleToUsersDialog grd = new GrantRoleToUsersDialog(_roleName, _connectionDefinition);
            if(grd.ShowDialog() == DialogResult.OK)
            {
                if (grd.RoleGranted)
                {
                    OnGrantRevokeEvent();
                    DoRefresh();
                }
            }
        }
        #endregion Private methods

    }

    #region UserForRoleGridDataHandler Class

    public class UserForRoleGridDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private UsersForRoleControl _theUsersForRoleControl;
        private ArrayList _Columns = new ArrayList();

        #endregion Fields

        #region Constructors

        public UserForRoleGridDataHandler(UsersForRoleControl aUsersForRoleControl)
        {
            _theUsersForRoleControl = aUsersForRoleControl;
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

        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if (null == aDataTable)
            {
                return;
            }

            DataTable dataTable = new DataTable();

            foreach (DataColumn dc in aDataTable.Columns)
            {
                if (dc.ColumnName.Equals(UsersForRoleControl.USER_TYPE_COL_NAME))
                {
                    dataTable.Columns.Add(dc.ColumnName, typeof(string));
                }
                else
                {
                    dataTable.Columns.Add(dc.ColumnName, dc.DataType);
                }
            }

            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow row = dataTable.NewRow();
                foreach (DataColumn dc in aDataTable.Columns)
                {
                    if (dc.ColumnName.Equals(UsersForRoleControl.USER_TYPE_COL_NAME))
                    {
                        row[dc.ColumnName] = User.GetUserTypeString((User.UserTypeEnum)dr[dc.ColumnName]);
                    }
                    else
                    {
                        if (dc.ColumnName.Equals(UsersForRoleControl.ISDEFAULT_COL_NAME))
                        {
                            string isDefault = dr[dc.ColumnName] as string;
                            row[dc.ColumnName] = (isDefault.Trim().Equals("Y") ? "Yes" : "No");
                        }
                        else
                        {
                            row[dc.ColumnName] = dr[dc.ColumnName];
                        }
                    }
                }
                dataTable.Rows.Add(row);
            }

            base.DoPopulate(aConfig, dataTable, aDataGrid);

            foreach (iGCol column in aDataGrid.Cols)
            {
                if (column.CellStyle.ValueType == typeof(System.DateTime))
                {
                    column.CellStyle.FormatProvider = _theUsersForRoleControl.DateFormatProvider;
                    column.CellStyle.FormatString = "{0}";
                }
            }

            string gridHeaderText = string.Format(Properties.Resources.UsersForRoleHeaderText, dataTable.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);
            if (dataTable.Rows.Count > 0)
            {
                aDataGrid.ResizeGridColumns(dataTable, 7, 20);
            }

            _theUsersForRoleControl.UpdateControls();
        }
        #endregion Private methods
    }

   #endregion RolesGridDataHandler Class
}
