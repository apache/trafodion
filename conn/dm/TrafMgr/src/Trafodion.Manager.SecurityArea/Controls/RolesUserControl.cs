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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.SecurityArea.Controls
{
    public partial class RolesUserControl : UserControl, IDataDisplayControl, ICloneToWindow
    {

        #region Fields

        private static readonly string _rolesListConfigName = "RolesWidgetConfig";
        UniversalWidgetConfig _rolesWidgetConfig = null;
        GenericUniversalWidget _rolesWidget = null;
        RolesGridDataHandler _rolesWidgetDisplayHandler = null;
        ConnectionDefinition _connectionDefinition;
        TrafodionIGrid _rolesGrid = null;
        String _title = "Roles";
        SystemSecurity _systemSecurity;
        private delegate void HandleEvent(object obj, EventArgs e);  

        #endregion Fields

        #region Properties

        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set 
            {
                _connectionDefinition = value;
                if ((_rolesWidgetConfig != null) && (_rolesWidgetConfig.DataProviderConfig != null))
                {
                    _rolesWidgetConfig.DataProviderConfig.ConnectionDefinition = value;
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
            get { return _rolesWidgetConfig; }
            set { _rolesWidgetConfig = value; }
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

        public RolesUserControl()
        {
            InitializeComponent();
        }

        public RolesUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefn = aConnectionDefinition;
            _systemSecurity = SystemSecurity.FindSystemModel(aConnectionDefinition);

            if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.ROLE_MGMT) || _systemSecurity.RolePasswordChangeOnly)
            {
                if (!_systemSecurity.IsPasswordChangeableRolesLoaded)
                {
                    TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.LookPasswordPrivProgressText, _systemSecurity, "LoadPasswordChangeableRoles", new Object[0]);
                    TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                    progressDialog.ShowDialog();

                    if (progressDialog.Error != null)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, Properties.Resources.ErrorMessageTitle,
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return;
                    }
                }
            }

            ShowWidgets();
        }

        public RolesUserControl(RolesUserControl aRolesUserControl)
            : this(aRolesUserControl.ConnectionDefn)
        {

        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To clone a self.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            RolesUserControl theRolesUserControl = new RolesUserControl(this);
            return theRolesUserControl;
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
            _rolesWidgetConfig = WidgetRegistry.GetConfigFromPersistence(_rolesListConfigName);
            if (_rolesWidgetConfig == null)
            {
                _rolesWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _rolesWidgetConfig.Name = _title;
                _rolesWidgetConfig.Title = _title;
                _rolesWidgetConfig.ShowProperties = false;
                _rolesWidgetConfig.ShowToolBar = true;
                _rolesWidgetConfig.ShowChart = false;
                _rolesWidgetConfig.ShowTimerSetupButton = false;
                _rolesWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;

                DatabaseDataProviderConfig _dbConfig = _rolesWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
                _dbConfig.SQLText = "SELECT TRIM(ROLENAME) AS ROLENAME, GRANTCOUNT, DEFAULTROLECOUNT, ROLEOWNER, CREATETIME FROM MANAGEABILITY.TRAFODION_SECURITY.ROLE_INFO ORDER BY ROLENAME FOR READ UNCOMMITTED ACCESS";
                _dbConfig.CommandTimeout = 0;

            }
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping("ROLENAME", "Role Name", 50));
            columnMappings.Add(new ColumnMapping("GRANTCOUNT", "Grant Count", 50));
            columnMappings.Add(new ColumnMapping("DEFAULTROLECOUNT", "Default Role Count", 50));
            columnMappings.Add(new ColumnMapping("ROLEOWNER", "Created By", 50));
            columnMappings.Add(new ColumnMapping("CREATETIME", "Create Time", 50));

            _rolesWidgetConfig.DataProviderConfig.ColumnMappings = columnMappings;


            //_dbConfig.TimerPaused = true;
            _rolesWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

            _rolesWidget = new GenericUniversalWidget();
            _rolesWidget.DataProvider = new DatabaseDataProvider(_rolesWidgetConfig.DataProviderConfig);

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_rolesWidget.DataDisplayControl).LineCountFormat = Properties.Resources.Roles;
            _rolesWidgetDisplayHandler = new RolesGridDataHandler(this);
            _rolesWidget.DataDisplayControl.DataDisplayHandler = _rolesWidgetDisplayHandler;
            _rolesWidget.UniversalWidgetConfiguration = _rolesWidgetConfig;
            _rolesWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_rolesWidget);

            _rolesGrid = ((TabularDataDisplayControl)_rolesWidget.DataDisplayControl).DataGrid;
            _rolesGrid.DoubleClickHandler = RoleDetails_Handler;
            _rolesGrid.SelectionChanged += new EventHandler(_rolesGrid_SelectionChanged);

            if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.ROLE_MGMT))
            {
                TrafodionIGridToolStripMenuItem viewDetailsMenuItem = new TrafodionIGridToolStripMenuItem();
                viewDetailsMenuItem.Text = Properties.Resources.ViewRoleDetailsMenuText;
                viewDetailsMenuItem.Click += new EventHandler(viewDetailsMenuItem_Click);
                _rolesGrid.AddContextMenu(viewDetailsMenuItem);
            }


            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_rolesWidget.DataDisplayControl).ShowExportButtons = false;
            Control exportButtonsControl = _rolesGrid.GetButtonControl();
            exportButtonsControl.Dock = DockStyle.Right;
            _theExportButtonPanel.Controls.Clear();
            _theExportButtonPanel.Controls.Add(exportButtonsControl);

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _rolesWidget.StartDataProvider();
            UpdateControls();

        }

        void _rolesGrid_SelectionChanged(object sender, EventArgs e)
        {
            UpdateControls();
            if (_rolesGrid.SelectedRowIndexes.Count == 1)
            {
                string roleName = _rolesGrid.Rows[_rolesGrid.SelectedRowIndexes[0]].Cells[0].Value as string;
                _changePasswordButton.Enabled = _systemSecurity.IsChangePasswordAllowed(roleName);
            }
        }

        /// <summary>
        /// Handle the update context menu click. Used for bulk updates
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void viewDetailsMenuItem_Click(object sender, EventArgs e)
        {
            if (_rolesGrid.SelectedRowIndexes.Count > 0)
            {
                RoleDetails_Handler(_rolesGrid.SelectedRowIndexes[0]);
            }
        }

        public void UpdateControls()
        {
            _changePasswordButton.Enabled = false;
            if (_systemSecurity.IsViewOnly || _systemSecurity.RolePasswordChangeOnly)
            {
                _addButton.Enabled = _deleteButton.Enabled = false;
            }
            else
            {
                _deleteButton.Enabled = _rolesGrid.SelectedRowIndexes.Count >= 1;
            }
        }

        void RoleDetails_Handler(int rowIndex)
        {
            if (_systemSecurity.IsConfigFunctionAllowed(SystemSecurity.ROLE_MGMT))
            {
                string roleName = _rolesGrid.Rows[rowIndex].Cells[0].Value as string;
                DataTable dataTable = _rolesWidget.DataProvider.GetDataTable();
                for(int i=0; i< dataTable.Rows.Count; i++)
                {
                    if (dataTable.Rows[i].ItemArray[0].Equals(roleName))
                    {
                        rowIndex = i;
                        break;
                    }
                }

                RoleDetailsDialog rd = new RoleDetailsDialog(_connectionDefinition, dataTable, rowIndex);
                rd.ShowDialog();
                if (rd.GrantOrRevokePerformed)
                {
                    _rolesWidget.DataProvider.RefreshData();
                }
            }
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
            if (_rolesWidget != null && _rolesWidget.DataProvider != null)
            {
                //Associate the event handlers
                _rolesWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _rolesWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _rolesWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }

        private void RemoveHandlers()
        {
            if (_rolesWidget != null && _rolesWidget.DataProvider != null)
            {
                //Remove the event handlers
                _rolesWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _rolesWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _rolesWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Security, ex.StackTrace);
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
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
        }

        #endregion Private methods


        private void _addButton_Click(object sender, EventArgs e)
        {
            AddRoleDialog addRoleDialog = new AddRoleDialog(_connectionDefinition);
            if (addRoleDialog.ShowDialog() == DialogResult.OK)
            {
                if (addRoleDialog.RoleGotAdded)
                {
                    _rolesWidget.DataProvider.RefreshData();
                }
            }
        }

        private void _deleteButton_Click(object sender, EventArgs e)
        {
            bool roleDeleted = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");

            if (MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), Properties.Resources.ConfirmRoleDeleteMessage, Properties.Resources.Confirm, MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                foreach (int rowIndex in _rolesGrid.SelectedRowIndexes)
                {
                    Cursor = Cursors.WaitCursor;
                    string roleName = _rolesGrid.Rows[rowIndex].Cells[0].Value as string;
                    try
                    {
                        Role roleModel = new Role(roleName.Trim(), _connectionDefinition);
                        roleModel.Delete();
                        roleDeleted = true;
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { roleName, ex.Message});
                    }
                    Cursor = Cursors.Default;
                }
            }
            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(Properties.Resources.DeleteRoleErrorHeaderText, errorTable, System.Drawing.SystemIcons.Warning);
                mmd.ShowDialog();
            }
            if (roleDeleted)
            {
                _rolesWidget.DataProvider.RefreshData();
            }
        }

        private void _changePasswordButton_Click(object sender, EventArgs e)
        {
            if(_rolesGrid.SelectedRowIndexes.Count == 1)
            {
                Role role = new Role(_rolesGrid.Rows[_rolesGrid.SelectedRowIndexes[0]].Cells[0].Value as string, _connectionDefinition);
                ChangeRolePasswordDialog crp = new ChangeRolePasswordDialog(role);
                if (crp.IsInitialized)
                {
                    crp.ShowDialog();
                }
            }
        }
    }

    #region RolesGridDataHandler Class

    public class RolesGridDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private RolesUserControl _theRolesUserControl;
        private object _locker;

        #endregion Fields

        #region Constructors

        public RolesGridDataHandler(RolesUserControl aRolesUserControl)
        {
            _theRolesUserControl = aRolesUserControl;
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
                if (dc.ColumnName.Equals("CREATETIME", StringComparison.OrdinalIgnoreCase))
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
                    if (dc.ColumnName.Equals("CREATETIME", StringComparison.OrdinalIgnoreCase))
                    {
                        row[dc.ColumnName] = Trafodion.Manager.Framework.Utilities.FormattedJulianTimestamp((long)dr[dc.ColumnName], "Unavailable");
                    }
                    else
                    {
                        row[dc.ColumnName] = dr[dc.ColumnName];
                    }
                }
                dataTable.Rows.Add(row);
            }

            base.DoPopulate(aConfig, dataTable, aDataGrid);

            string gridHeaderText = string.Format(Properties.Resources.RolesRowHeaderText, dataTable.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);
            if (dataTable.Rows.Count > 0)
            {
                aDataGrid.ResizeGridColumns(dataTable, 7, 20);
            }

            _theRolesUserControl.UpdateControls();
        }
        #endregion Private methods
    }

   #endregion RolesGridDataHandler Class
}
