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
    public partial class RolesUserControl : UserControl, IDataDisplayControl, ICloneToWindow
    {
        #region Static Members
        private static readonly string RolesWidgetConfigName = "UserMgmt_RolesWidgetConfig";
        #endregion

        #region Fields
        UniversalWidgetConfig _theRolesWidgetConfig = null;
        GenericUniversalWidget _theRolesWidget = null;
        RolesDataHandler _theRolesDataDisplayHandler = null;
        ConnectionDefinition _theConnectionDefinition;
        TrafodionIGrid _theRolesGrid = null;
        String _theTitle = Properties.Resources.RolesTitle;
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
                    _theRolesWidget.DataProvider.Stop();
                }
                _theConnectionDefinition = value;
                if ((_theRolesWidgetConfig != null) && (_theRolesWidgetConfig.DataProviderConfig != null))
                {
                    if (_theRolesGrid != null)
                    {
                        _theRolesGrid.Rows.Clear();
                    }
                    _theRolesWidgetConfig.DataProviderConfig.ConnectionDefinition = value;
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
            get { return _theRolesWidgetConfig; }
            set { _theRolesWidgetConfig = value; }
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

        public RolesUserControl()
        {
            InitializeComponent();
            _theDropButton.Enabled = false;
            _theAlterRoleButton.Enabled = false;
            _sqlPrivilegesButton.Enabled = false;
        }

        public RolesUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefn = aConnectionDefinition;
            ShowWidgets();
        }

        public RolesUserControl(RolesUserControl aRolesUserControl)
            : this(aRolesUserControl.ConnectionDefn)
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
            //Load the Roles widget config from persistence.
            //if it does not exist or there is an error reading the persistence, create
            //a default Roles widget config
            _theRolesWidgetConfig = WidgetRegistry.GetConfigFromPersistence(RolesWidgetConfigName);
            if (_theRolesWidgetConfig == null)
            {
                _theRolesWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
                _theRolesWidgetConfig.Name = RolesWidgetConfigName;
                _theRolesWidgetConfig.Title = _theTitle;
                _theRolesWidgetConfig.ShowProperties = false;
                _theRolesWidgetConfig.ShowToolBar = true;
                _theRolesWidgetConfig.ShowChart = false;
                _theRolesWidgetConfig.ShowTimerSetupButton = false;
                _theRolesWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            _theRolesWidgetConfig.ShowHelpButton = true;
            _theRolesWidgetConfig.HelpTopic = HelpTopics.UnderstandRolesPane;

            DatabaseDataProviderConfig _dbConfig = _theRolesWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;

            StringBuilder sbSql = new StringBuilder();
            sbSql.Append("SELECT	R.ROLE_NAME,");
            sbSql.Append("		R.ROLE_ID,");
            sbSql.Append("		(SELECT COUNT(*) ");
            sbSql.Append("		FROM	TRAFODION_INFORMATION_SCHEMA.ROLE_INFO S,");
            sbSql.Append("				TRAFODION_INFORMATION_SCHEMA.USER_ROLE_INFO X ");
            sbSql.Append("		WHERE R.ROLE_NAME = S.ROLE_NAME");
            sbSql.Append("		AND X.ROLE_NAME = S.ROLE_NAME) AS GRANTCOUNT,");
            sbSql.Append("		(SELECT COUNT(*) ");
            sbSql.Append("		FROM	TRAFODION_INFORMATION_SCHEMA.ROLE_INFO S,");
            sbSql.Append("				TRAFODION_INFORMATION_SCHEMA.USER_INFO X ");
            sbSql.Append("		WHERE R.ROLE_NAME = S.ROLE_NAME");
            sbSql.Append("			AND X.LOGON_ROLE_ID_NAME = S.ROLE_NAME) AS DEFAULTROLECOUNT,");
            sbSql.Append("		R.ROLE_CREATOR_NAME,");
            sbSql.Append("		R.CREATE_TIME,");
            sbSql.Append("		R.REDEF_TIME");
            sbSql.Append(" FROM TRAFODION_INFORMATION_SCHEMA.ROLE_INFO R ");
            sbSql.Append(" ORDER BY R.ROLE_NAME FOR READ UNCOMMITTED ACCESS;");
            _dbConfig.SQLText = sbSql.ToString();

            if (_dbConfig.ColumnMappings == null)
            {
                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("ROLE_NAME", "Role Name", 128));
                columnMappings.Add(new ColumnMapping("ROLE_ID", "Role ID", 60));
                columnMappings.Add(new ColumnMapping("GRANTCOUNT", "Grant Count",60));
                columnMappings.Add(new ColumnMapping("DEFAULTROLECOUNT", "Primary Role Count", 60));
                columnMappings.Add(new ColumnMapping("ROLE_CREATOR_NAME", "Created By", 128));
                columnMappings.Add(new ColumnMapping("CREATE_TIME", "Creation Time", 200));
                columnMappings.Add(new ColumnMapping("REDEF_TIME", "Redefinition Time", 200));

                _dbConfig.ColumnMappings = columnMappings;
            }
            //_dbConfig.TimerPaused = true;
            _theRolesWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

            _theRolesWidget = new GenericUniversalWidget();
            _theRolesWidget.DataProvider = new DatabaseDataProvider(_theRolesWidgetConfig.DataProviderConfig);

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_theRolesWidget.DataDisplayControl).LineCountFormat = "Database Roles";
            _theRolesDataDisplayHandler = new RolesDataHandler(this);
            _theRolesWidget.DataDisplayControl.DataDisplayHandler = _theRolesDataDisplayHandler;
            _theRolesWidget.UniversalWidgetConfiguration = _theRolesWidgetConfig;
            _theRolesWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_theRolesWidget);

            _theRolesGrid = ((TabularDataDisplayControl)_theRolesWidget.DataDisplayControl).DataGrid;
            _theRolesGrid.SelectionChanged += _theDSGrid_SelectionChanged;
            //Set selected rows color while losed focus.
            _theRolesGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _theRolesGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;

            UserMgmtSystemModel systemModel = UserMgmtSystemModel.FindSystemModel(ConnectionDefn);
            if (systemModel.IsAdminUser)
            {
                
                _theRolesGrid.DoubleClickHandler = RoleDetails_Handler;

                TrafodionIGridToolStripMenuItem createRoleMenuItem = new TrafodionIGridToolStripMenuItem();
                createRoleMenuItem.Text = Properties.Resources.CreateRole;
                createRoleMenuItem.Click += new EventHandler(createRoleMenuItem_Click);
                _theRolesGrid.AddContextMenu(createRoleMenuItem);

                TrafodionIGridToolStripMenuItem dropRoleMenuItem = new TrafodionIGridToolStripMenuItem();
                dropRoleMenuItem.Text = Properties.Resources.DropRole;
                dropRoleMenuItem.Click += new EventHandler(dropRoleMenuItem_Click);
                _theRolesGrid.AddContextMenu(dropRoleMenuItem);

                TrafodionIGridToolStripMenuItem alterRoleMenuItem = new TrafodionIGridToolStripMenuItem();
                alterRoleMenuItem.Text = Properties.Resources.ViewRoleDetailsMenuText;
                alterRoleMenuItem.Click += new EventHandler(alterRoleMenuItem_Click);
                _theRolesGrid.AddContextMenu(alterRoleMenuItem);

                TrafodionIGridToolStripMenuItem viewSQLPrivMenuItem = new TrafodionIGridToolStripMenuItem();
                viewSQLPrivMenuItem.Text = Properties.Resources.ShowSQLPrivilegesDialogTitle;
                viewSQLPrivMenuItem.Click += new EventHandler(viewSQLPrivMenuItem_Click);
                _theRolesGrid.AddContextMenu(viewSQLPrivMenuItem);

            }
            
            //[TBD]Let the usual row detail to be shown at this time.  But, when the user becomes editable, we need to re-visit this
            //     to see if we want to handle double click differently. 
            //_theDSGrid.DoubleClickHandler = UserEdit_Handler;   
            _theRolesGrid.RowMode = true;

            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_theRolesWidget.DataDisplayControl).ShowExportButtons = false;

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theRolesWidget.StartDataProvider();            
            this.Disposed += new EventHandler(RolesUserControl_Disposed);

        }

        void alterRoleMenuItem_Click(object sender, EventArgs e)
        {
            _theAlterRoleButton.PerformClick();
        }

        void viewSQLPrivMenuItem_Click(object sender, EventArgs e)
        {
            _sqlPrivilegesButton.PerformClick();
        }

        void dropRoleMenuItem_Click(object sender, EventArgs e)
        {
            _theDropButton.PerformClick();
        }

        void createRoleMenuItem_Click(object sender, EventArgs e)
        {
            _theCreateButton.PerformClick();
        }

        void RoleDetails_Handler(int rowIndex)
        {
            UserMgmtSystemModel systemModel = UserMgmtSystemModel.FindSystemModel(ConnectionDefn);
            if (systemModel.IsAdminUser)
            {
                string roleName = _theRolesGrid.Rows[rowIndex].Cells[0].Value as string;
                DataTable dataTable = _theRolesWidget.DataProvider.GetDataTable();
                if (dataTable != null && dataTable.Rows.Count > 0)
                {
                    for (int i = 0; i < dataTable.Rows.Count; i++)
                    {
                        if (dataTable.Rows[i].ItemArray[0].Equals(roleName))
                        {
                            rowIndex = i;
                            break;
                        }
                    }
                    AlterRoleUserControl alterRoleUserControl = new AlterRoleUserControl(ConnectionDefn, dataTable, rowIndex);
                    alterRoleUserControl.OnSuccessImpl += DoRefresh;
                    System.Drawing.Size size = alterRoleUserControl.Size;
                    size.Height += 30; //account for the banner control
                    Utilities.LaunchManagedWindow(Properties.Resources.AlterRole + ": " + roleName, alterRoleUserControl, ConnectionDefn, size, true);
                }

            }
        }

        void RolesUserControl_Disposed(object sender, EventArgs e)
        {
            _theRolesGrid.SelectionChanged -= _theDSGrid_SelectionChanged;
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
            if (_theRolesWidget != null && _theRolesWidget.DataProvider != null)
            {
                //Associate the event handlers
                _theRolesWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _theRolesWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _theRolesWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;

            }
        }

        private void RemoveHandlers()
        {
            if (_theRolesWidget != null && _theRolesWidget.DataProvider != null)
            {
                //Remove the event handlers
                _theRolesWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _theRolesWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theRolesWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
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
            _theRolesWidget.DataProvider.RefreshData();
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

        private void RolesUserControl_Load(object sender, EventArgs e)
        {

        }

        void _theDSGrid_SelectionChanged(object sender, EventArgs e)
        {
            enableDisableButtons();
        }

        private void enableDisableButtons()
        {
            _theAlterRoleButton.Enabled = (_theRolesGrid.SelectedRowIndexes.Count == 1);
            if (_theRolesGrid.SelectedRowIndexes.Count > 0)
            {
                _theDropButton.Enabled = true;
                _theAlterRoleButton.Enabled = true;
                _sqlPrivilegesButton.Enabled = true;
            }
            else
            {
                _theDropButton.Enabled = false;
                _theAlterRoleButton.Enabled = false;
                _sqlPrivilegesButton.Enabled = false;
            }
        }


        #endregion Private methods

        private void sqlPrivilegesButton_Click(object sender, EventArgs e)
        {
            List<string> roleNames = new List<string>();
            foreach(iGRow row in _theRolesGrid.Rows)
            {
                roleNames.Add(row.Cells["ROLE_NAME"].Value.ToString());
            }

            int selectedIndex = _theRolesGrid.SelectedRowIndexes.Count > 0 ? _theRolesGrid.SelectedRowIndexes[0] : -1;
            AllSQLPrivilegesUserControl privilegesControl = new AllSQLPrivilegesUserControl(WorkMode.RoleMode,ConnectionDefn, roleNames, selectedIndex);
            Utilities.LaunchManagedWindow(Properties.Resources.ShowSQLPrivilegesDialogTitle + " - Role Name: " + _theRolesGrid.Rows[selectedIndex].Cells["ROLE_NAME"].Value, privilegesControl, ConnectionDefn, privilegesControl.Size, true);
        }

        private void _theCreateButton_Click(object sender, EventArgs e)
        {
            CreateRoleUserControl createRoleControl = new CreateRoleUserControl(ConnectionDefn);
            createRoleControl.OnSuccessImpl += DoRefresh;

            Utilities.LaunchManagedWindow("Create Role(s)", createRoleControl, ConnectionDefn, createRoleControl.Size, true);

        }

        private void _theDropButton_Click(object sender, EventArgs e)
        {
            if ((_theRolesGrid == null) || (_theRolesGrid.SelectedRows.Count == 0))
            {
                return;
            }
            List<string> rolesToDelete = new List<string>();
            foreach (iGRow row in _theRolesGrid.SelectedRows)
            {
                rolesToDelete.Add(((string)row.Cells[0].Value).Trim());
            }

            int dropCout = rolesToDelete.Count;
            if (dropCout > 0)
            {
                bool isCascade = false;
                DialogResult dropDialogResult;

                dropDialogResult = MessageBox.Show(string.Format(Properties.Resources.DropRoleMsg1, rolesToDelete.Count), "Drop Role(s)", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                if (dropDialogResult == DialogResult.Yes)
                {
                    Role roleSystemModel = Role.FindSystemModel(ConnectionDefn);              
                    DataTable resultsTable = new DataTable();
                    Object[] parameters = new Object[] { rolesToDelete, isCascade };
                    TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.DropRoleMsg2, roleSystemModel, "DropRoles", parameters);
                    TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                    progressDialog.ShowDialog();
                    if (progressDialog.Error != null)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), progressDialog.Error.Message, "Error drop role(s)",
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

                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog(string.Format(infoMsg, "Dropping the role(s)"), resultsTable, iconType);
                    mmd.ShowDialog();                         

                    //Refresh the right pane if needed
                    if (successCount > 0)
                    {
                        DoRefresh();
                    }
                }
            }
        }

        private void _theAlterRoleButton_Click(object sender, EventArgs e)
        {
            if (_theRolesGrid.SelectedRowIndexes.Count > 0)
            {
                RoleDetails_Handler(_theRolesGrid.SelectedRowIndexes[0]);
            }
        }

}



    #region RolesDataHandler Class

    public class RolesDataHandler : TabularDataDisplayHandler
    {
        private RolesUserControl _theRolesUserControl;

        public RolesDataHandler(RolesUserControl aRolesUserControl)
        {
            _theRolesUserControl = aRolesUserControl;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            lock (this)
            {
                populate(aConfig, aDataTable, aDataGrid);
            }
        }

        //Popultes the datatable using the data returned by the data provider and displays it in the grid
        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if (null == aDataTable)
            {
                return;
            }

            DataTable newDataTable = new DataTable();
            
            foreach (DataColumn dc in aDataTable.Columns)
            {
                if (dc.ColumnName.Equals("CREATE_TIME") || dc.ColumnName.Equals("REDEF_TIME") )
                    newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
                else
                    newDataTable.Columns.Add(dc.ColumnName, dc.DataType);
            }
            
            //format columns
            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow newDR = newDataTable.NewRow();
                for (int i = 0; i < aDataTable.Columns.Count; i++)
                {
                    string colName = aDataTable.Columns[i].ToString();
                    if (colName.Equals("CREATE_TIME"))
                    {
                        try
                        {
                            //check date validation and set blank if it's invalid.
                            long timeVar = Convert.ToInt64(dr["CREATE_TIME"]);
                            newDR[i] = (new JulianTimestamp(timeVar)).ToString();
                        }
                        catch (Exception ex)
                        {
                            newDR[i] = "";
                        }
                    }
                    else if (colName.Equals("REDEF_TIME"))
                    {
                        try
                        {
                            //check date validation and set blank if it's invalid.
                            long timeVar = Convert.ToInt64(dr["REDEF_TIME"]);
                            newDR[i] = (new JulianTimestamp(timeVar)).ToString();
                        }
                        catch (Exception ex)
                        {
                            newDR[i] = "";
                        }
                    }
                    else
                    {
                        newDR[i] = dr[i];
                    }
                }
                newDataTable.Rows.Add(newDR);
            }

            base.DoPopulate(aConfig, newDataTable, aDataGrid);

            string gridHeaderText = string.Format(Properties.Resources.RoleGridTitle, aDataGrid.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);

        }
    }

    #endregion RolesDataHandler Class
}
