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
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.SecurityArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.SecurityArea.Controls
{
    /// <summary>
    /// Class to display all of the configured directory server entries.
    /// </summary>
    public partial class DirectoryServersUserControl : UserControl, IDataDisplayControl, ICloneToWindow
    {

        #region Fields

        /// <summary>
        /// Column names for displaying configured servers.
        /// </summary>
        public static string[] ColumnNames = { "Domain_Name", "Usage_Priority", "Host_Name", "Host_Port Number", "LDAP_Version", "Search_User DN", "Encryption", "Last_Updated" };

        /// <summary>
        /// Enum defined for Grid column
        /// </summary>
        public enum GridColumns { DOMAIN_NAME =0, USAGE_PRI, LDAPHOST, LDAPPORT, LDAPVERSION, SEARCHUSERDN, LDAPENABLESSL, TIME_STAMP };
        public static readonly string SELECT_LDAP_SERVER_SQL_STATEMENT = 
                "SELECT DOMAIN_NAME, USAGE_PRI, LDAPHOST, LDAPPORT, LDAPVERSION, SEARCHUSERDN, LDAPENABLESSL, TIME_STAMP FROM MANAGEABILITY.CONFIG_MANAGEMENT.CONFIG WHERE DOMAIN_NAME != 'DEFAULT' ORDER BY USAGE_PRI DESC FOR READ UNCOMMITTED ACCESS";

        // Now, they are all private.
        private static readonly string _theDirectoryServersConfigName = "_theDSWidgetConfig";
        private UniversalWidgetConfig _theDSWidgetConfig = null;
        private GenericUniversalWidget _theDSWidget = null;
        private DirectoryServersDataHandler _theDSDataDisplayHandler = null;
        private ConnectionDefinition _theConnectionDefinition;
        private TrafodionIGrid _theDSGrid = null;
        private String _theTitle = Properties.Resources.DirectoryServers;
        private ArrayList _commands = new ArrayList();
        private Connection _conn = null;
        private Color _defaultValueFontColor = Color.RoyalBlue;
        private delegate void HandleEvent(object obj, EventArgs e);     

        #endregion Fields

        #region Properties

        /// <summary>
        /// ConnectionDefn: the connection definition for the control
        /// </summary>
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

        /// <summary>
        /// DataProvider: the data provider used by this control
        /// </summary>
        public DataProvider DataProvider
        {
            get;
            set;
        }

        /// <summary>
        /// UniversalWidgetConfiguration: the widget config for this user control
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theDSWidgetConfig; }
            set { _theDSWidgetConfig = value; }
        }

        /// <summary>
        /// DataDisplayHandler: the data display handler used by the widget
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get;
            set;
        }

        /// <summary>
        /// DrillDownManager: if there is any
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        /// <summary>
        /// WindowTitle: the window title used for popup
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

        /// <summary>
        /// DefaultValueFontColor: the font color used to denote Default values
        /// </summary>
        public Color DefaultValueFontColor
        {
            get { return _defaultValueFontColor; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The default constructor
        /// </summary>
        public DirectoryServersUserControl()
        {
            InitializeComponent();
            ResetControls();
        }

        /// <summary>
        /// Constructor: 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public DirectoryServersUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefn = aConnectionDefinition;
            ShowWidgets();
            this._addButton.Enabled = !SystemSecurity.FindSystemModel(aConnectionDefinition).IsViewOnly;
        }

        /// <summary>
        /// Constructor: for cloning 
        /// </summary>
        /// <param name="aDirectoryServersUserControl"></param>
        public DirectoryServersUserControl(DirectoryServersUserControl aDirectoryServersUserControl)
            : this(aDirectoryServersUserControl.ConnectionDefn)
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
            DirectoryServersUserControl theDirectoryServersUserControl = new DirectoryServersUserControl(this);
            return theDirectoryServersUserControl;
        }

        /// <summary>
        /// Persist any configuraiton in this user control.
        /// </summary>
        public void PersistConfiguration()
        {
        }

        /// <summary>
        /// Refresh the grid display
        /// </summary>
        public void Refresh()
        {
            DoRefresh();
        }

        /// <summary>
        /// Reset all controls 
        /// </summary>
        public void ResetControls()
        {
            // Currently, only some of the buttons need to be disable.
            _deleteButton.Enabled = _editButton.Enabled = _addLikeButton.Enabled = false;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Create, setup and show the widgets
        /// </summary>
        private void ShowWidgets()
        {
            // Remove all current contents and add the alerts widget
            _theWidgetPanel.Controls.Clear();

            _theDSWidgetConfig = WidgetRegistry.GetDefaultDBConfig();
            _theDSWidgetConfig.Name = _theTitle;
            _theDSWidgetConfig.Title = _theTitle;
            _theDSWidgetConfig.ShowProperties = false;
            _theDSWidgetConfig.ShowChart = false;
            _theDSWidgetConfig.ShowToolBar = true;
            _theDSWidgetConfig.ShowTimerSetupButton = false;
            _theDSWidgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;

            DatabaseDataProviderConfig _dbConfig = _theDSWidgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            _dbConfig.SQLText = SELECT_LDAP_SERVER_SQL_STATEMENT;

            _theDSWidgetConfig.DataProviderConfig.ConnectionDefinition = ConnectionDefn;

            _theDSWidget = new GenericUniversalWidget();
            _theDSWidget.DataProvider = new DatabaseDataProvider(_dbConfig);

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_theDSWidget.DataDisplayControl).LineCountFormat = 
                String.Format(Properties.Resources.SingleDirectoryServerCount, 0);
            _theDSDataDisplayHandler = new DirectoryServersDataHandler(this);
            _theDSWidget.DataDisplayControl.DataDisplayHandler = _theDSDataDisplayHandler;
            _theDSWidget.UniversalWidgetConfiguration = _theDSWidgetConfig;
            _theDSWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_theDSWidget);

            //Setup the iGrid properties:
            _theDSGrid = ((TabularDataDisplayControl)_theDSWidget.DataDisplayControl).DataGrid;
            _theDSGrid.CellClick += new iGCellClickEventHandler(DSGrid_CellClick);
            _theDSGrid.SelectionChanged += new EventHandler(DSGrid_SelectionChanged);
            _theDSGrid.DoubleClickHandler = DirectoryServerDetails_Handler;
            _theDSGrid.RowMode = true;

            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_theDSWidget.DataDisplayControl).ShowExportButtons = false;
            Control exportButtonsControl = _theDSGrid.GetButtonControl();
            exportButtonsControl.Dock = DockStyle.Right;
            _theExportButtonPanel.Controls.Clear();
            _theExportButtonPanel.Controls.Add(exportButtonsControl);

            ////Associate the tool strip buttons to the Universal widget
            //ToolStripButton moveUpButton = new ToolStripButton(Properties.Resources.uparrow);
            //moveUpButton.Click += new System.EventHandler(moveUpButton_Click);
            //moveUpButton.ToolTipText = "Move up";
            //_theDSWidget.AddToolStripItem(moveUpButton);

            //ToolStripButton moveDownButton = new ToolStripButton(Properties.Resources.downarrow);
            //moveDownButton.Click += new System.EventHandler(moveDownButton_Click);
            //moveDownButton.ToolTipText = "Move down";
            //_theDSWidget.AddToolStripItem(moveDownButton);

            //ToolStripButton saveButton = new ToolStripButton(Properties.Resources.filesave);
            //saveButton.ToolTipText = "Apply";
            //_theDSWidget.AddToolStripItem(saveButton);

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Start it.
            _theDSWidget.StartDataProvider();

            this.Disposed += new EventHandler(DirectoryServersUserControl_Disposed);

        }

        /// <summary>
        /// Event handler for iGrid selection changes.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void DSGrid_SelectionChanged(object sender, EventArgs e)
        {
            AdjustButtons();
        }

        // Not used for now.
        void moveUpButton_Click(object sender, System.EventArgs e)
        {
            if (_theDSGrid.CurRow.Index > 0)
            {
                _theDSGrid.CurRow.Move(_theDSGrid.CurRow.Index - 1);
            }
        }

        // Not used for now.
        void moveDownButton_Click(object sender, System.EventArgs e)
        {
            if (_theDSGrid.CurRow.Index + 2 <= _theDSGrid.Rows.Count)
            {
                _theDSGrid.CurRow.Move(_theDSGrid.CurRow.Index + 2);
            }

        }
        /// <summary>
        /// Event handler when cell clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DSGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            AdjustButtons();
        }

        /// <summary>
        /// Adjust the visibility of all buttons.
        /// </summary>
        private void AdjustButtons()
        {
            if (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly)
            {
                if (_theDSGrid.CurRow != null && _theDSGrid.CurRow.Index >= 0 && _theDSGrid.SelectedRows.Count == 1)
                {
                    _deleteButton.Enabled = true;
                    _editButton.Enabled = true;
                    _addLikeButton.Enabled = true;
                }
                else
                {
                    _deleteButton.Enabled = false;
                    _editButton.Enabled = false;
                    _addLikeButton.Enabled = false;
                }
            }
        }

        /// <summary>
        /// Event handler when control displosed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void  DirectoryServersUserControl_Disposed(object sender, EventArgs e)
        {
            if (_theDSGrid != null)
            {
                _theDSGrid.CellClick -= DSGrid_CellClick;
                _theDSGrid.SelectionChanged -= DSGrid_SelectionChanged;
            }
        }

        /// <summary>
        /// Cleanup event handlers when the control is displosed.
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

        /// <summary>
        /// Add event handlers for handling data
        /// </summary>
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

        /// <summary>
        /// Remove event handlers for data handling
        /// </summary>
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

        private void DirectoryServersUserControl_Load(object sender, EventArgs e)
        {
        }

        /// <summary>
        /// Refresh the Grid.
        /// </summary>
        private void DoRefresh()
        {
            _theDSWidget.DataProvider.RefreshData();
        }

        /// <summary>
        /// Double click event for the Grid, which will bring up the Edit dialog.
        /// </summary>
        /// <param name="rowIndex"></param>
        private void DirectoryServerDetails_Handler(int rowIndex)
        {
            if (_theDSGrid != null &&  (!SystemSecurity.FindSystemModel(_theConnectionDefinition).IsViewOnly))
            {
                DirectoryServer ds = new DirectoryServer(ConnectionDefn, _theDSGrid.Rows[rowIndex]);
                ConfigureDirectoryServerDialog dialog = new ConfigureDirectoryServerDialog(ConnectionDefn, ds, true);
                DialogResult result = dialog.ShowDialog();
                if (result == DialogResult.Yes)
                {
                    DoRefresh();
                }
            }
        }

        /// <summary>
        /// Event handler for Add button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _addButton_Click(object sender, EventArgs e)
        {
            ConfigureDirectoryServerDialog theDialog = new ConfigureDirectoryServerDialog(ConnectionDefn, false);
            if (theDialog.Initialized)
            {
                // Don't even show it if it is not properly initialized. In this case, the dialog should have
                // reported error messages.
                DialogResult result = theDialog.ShowDialog();
                if (result == DialogResult.Yes)
                {
                    DoRefresh();
                }
            }
        }

        /// <summary>
        /// Event handler for Edit button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _editButton_Click(object sender, EventArgs e)
        {
            if (_theDSGrid == null || _theDSGrid.CurRow == null)
            {
                string message = String.Format(Properties.Resources.ErrorNoEntryIsSelected, Properties.Resources.DirectoryServer);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                message, 
                                String.Format(Properties.Resources.OperationErrorTitle,
                                              Properties.Resources.EditServer),
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
                return;
            }

            if (_theDSGrid.SelectedRows.Count > 1)
            {
                string message = String.Format(Properties.Resources.ErrorOnlyOneServerPlease, Properties.Resources.Edited);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                message,
                                String.Format(Properties.Resources.OperationErrorTitle,
                                              Properties.Resources.EditServer),
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
                return;
            }

            DirectoryServer server = null;

            try
            {
                server = new DirectoryServer(ConnectionDefn, _theDSGrid.CurRow);
            }
            catch (Exception ex)
            {
                string message = String.Format(Properties.Resources.ErrorOperationFailed,
                                        Properties.Resources.LookupServer,
                                        ex.Message);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                message,
                                String.Format(Properties.Resources.OperationErrorTitle,
                                              Properties.Resources.EditServer),
                                MessageBoxButtons.OK,
                                MessageBoxIcon.Error);
                return;
            }

            ConfigureDirectoryServerDialog dialog = new ConfigureDirectoryServerDialog(ConnectionDefn, server, true);
            if (dialog.Initialized)
            {
                DialogResult result = dialog.ShowDialog();
                if (result == DialogResult.Yes)
                {
                    DoRefresh();
                }
            }
        }

        /// <summary>
        /// Event handler for Delete button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _deleteButton_Click(object sender, EventArgs e)
        {
            string message = "";
            DialogResult result;

            if (_theDSGrid == null || _theDSGrid.CurRow == null)
            {
                message = String.Format(Properties.Resources.ErrorNoEntryIsSelected, Properties.Resources.DirectoryServer);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                message,
                                String.Format(Properties.Resources.OperationErrorTitle,
                                              Properties.Resources.DropServer),
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
                return;
            }

            if (_theDSGrid.SelectedRows.Count > 1)
            {
                message = String.Format(Properties.Resources.ErrorOnlyOneServerPlease, Properties.Resources.Dropped);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                message,
                                String.Format(Properties.Resources.OperationErrorTitle,
                                              Properties.Resources.DropServer),
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
                return;
            }

            if (_theDSGrid.Rows.Count < 2)
            {
                message = Properties.Resources.RemoveLastServerWarning;
                result = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                         message,
                                         String.Format(Properties.Resources.OperationErrorTitle,
                                                       Properties.Resources.DropServer), 
                                         MessageBoxButtons.YesNo, 
                                         MessageBoxIcon.Question);
                if (result != DialogResult.Yes)
                {
                    return;
                }
            }

            string domainName = _theDSGrid.CurRow.Cells[(int)GridColumns.DOMAIN_NAME].Text.Trim();
            short usagePriority = short.Parse(_theDSGrid.CurRow.Cells[(int)GridColumns.USAGE_PRI].Text.Trim());
            string hostName = _theDSGrid.CurRow.Cells[ColumnNames[(int)GridColumns.LDAPHOST]].Text.Trim();
            string portNumber = _theDSGrid.CurRow.Cells[ColumnNames[(int)GridColumns.LDAPPORT]].Text.Trim();

            message = Properties.Resources.DropServerWarning +
                      "\n\n" +
                      String.Format(Properties.Resources.SelectedServerMessage, domainName, usagePriority, hostName, portNumber) +
                      "\n\n";

            result = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                     message, 
                                     String.Format(Properties.Resources.OperationWarningTitle, 
                                                   Properties.Resources.DropServer),
                                     MessageBoxButtons.YesNo, 
                                     MessageBoxIcon.Question);
            
            // User's confirmation? 
            if (result == DialogResult.Yes)
            {
                DirectoryServer server = null;
                try
                {
                    server = new DirectoryServer(ConnectionDefn, _theDSGrid.CurRow);
                }
                catch (Exception ex)
                {
                    message = String.Format(Properties.Resources.ErrorOperationFailed,
                                            Properties.Resources.LookupServer,
                                            ex.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle,
                                                  Properties.Resources.DropServer),
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                    return;
                }

                TrafodionProgressArgs args = new TrafodionProgressArgs(Properties.Resources.RemovingServer,
                                                 server,
                                                 "Drop",
                                                 new Object[0]);
                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    message = String.Format(Properties.Resources.ErrorOperationFailed, 
                                            Properties.Resources.DropServer, 
                                            progressDialog.Error.Message);
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    message,
                                    String.Format(Properties.Resources.OperationErrorTitle, 
                                                  Properties.Resources.DropServer),
                                    MessageBoxButtons.OK, 
                                    MessageBoxIcon.Error);
                }
                else
                {
                    // If everything goes well, refresh the grid display.
                    DoRefresh();
                }
            }
        }

        /// <summary>
        /// Event handler for Add Like button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _addLikeButton_Click(object sender, EventArgs e)
        {
            if (_theDSGrid == null || _theDSGrid.CurRow == null)
            {
                return;
            }

            DirectoryServer server = null;
            try
            {
                server = new DirectoryServer(ConnectionDefn, _theDSGrid.CurRow);
            }
            catch (Exception ex)
            {
                string message = String.Format(Properties.Resources.ErrorOperationFailed,
                                        Properties.Resources.LookupServer,
                                        ex.Message);
                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                message,
                                String.Format(Properties.Resources.OperationErrorTitle, 
                                              Properties.Resources.CreateDirectoryServer),
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Error);
                return;
            }

            ConfigureDirectoryServerDialog theDialog = new ConfigureDirectoryServerDialog(ConnectionDefn, server, false);
            if (theDialog.Initialized)
            {
                DialogResult result = theDialog.ShowDialog();
                if (result == DialogResult.Yes)
                {
                    // done.
                    DoRefresh();
                }
            }
        }

        #endregion Private methods
    }

   #region DirectoryServersDataHandler Class 

    /// <summary>
    /// Data handler for displaying directory servers. 
    /// </summary>
    public class DirectoryServersDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private DirectoryServersUserControl _theDirectoryServersUserControl;

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDirectoryServersUserControl"></param>
        public DirectoryServersDataHandler(DirectoryServersUserControl aDirectoryServersUserControl)
        {
            _theDirectoryServersUserControl = aDirectoryServersUserControl;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To override the default do populate method
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
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

        /// <summary>
        /// Special handling of the data. 
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            if (null == aDataTable)
            {
                return;
            }

            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.DOMAIN_NAME], typeof(string));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.USAGE_PRI], typeof(short));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.LDAPHOST], typeof(string));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.LDAPPORT], typeof(string));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.LDAPVERSION], typeof(string));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.SEARCHUSERDN], typeof(string));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.LDAPENABLESSL], typeof(string));
            dataTable.Columns.Add(DirectoryServersUserControl.ColumnNames[(int)DirectoryServersUserControl.GridColumns.TIME_STAMP], typeof(string));

            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow row = dataTable.NewRow();
                row[(int)DirectoryServersUserControl.GridColumns.DOMAIN_NAME] = dr["DOMAIN_NAME"] as string;
                row[(int)DirectoryServersUserControl.GridColumns.USAGE_PRI] = (short)dr["USAGE_PRI"];

                // The rest are nullable columns
                try { row[(int)DirectoryServersUserControl.GridColumns.LDAPHOST] = dr["LDAPHOST"] as string; }
                catch (Exception) { row[(int)DirectoryServersUserControl.GridColumns.LDAPHOST] = ""; }

                try { row[(int)DirectoryServersUserControl.GridColumns.LDAPPORT] = ((int)dr["LDAPPORT"]).ToString(); }
                catch (Exception) { row[(int)DirectoryServersUserControl.GridColumns.LDAPPORT] = ""; }

                try { row[(int)DirectoryServersUserControl.GridColumns.LDAPVERSION] = ((int)dr["LDAPVERSION"]).ToString(); }
                catch (Exception) { row[(int)DirectoryServersUserControl.GridColumns.LDAPVERSION] = ""; }

                try { row[(int)DirectoryServersUserControl.GridColumns.SEARCHUSERDN] = dr["SEARCHUSERDN"] as string; }
                catch (Exception) { row[(int)DirectoryServersUserControl.GridColumns.SEARCHUSERDN] = ""; }

                try 
                {
                    DirectoryServer.ENCRYPTION_SUPPORT encrption = (DirectoryServer.ENCRYPTION_SUPPORT)(short)dr["LDAPENABLESSL"];
                    row[(int)DirectoryServersUserControl.GridColumns.LDAPENABLESSL] = encrption.ToString();
                }
                catch (Exception) { row[(int)DirectoryServersUserControl.GridColumns.LDAPENABLESSL] = ""; }

                // Not nullable column
                row[(int)DirectoryServersUserControl.GridColumns.TIME_STAMP] = 
                    Trafodion.Manager.Framework.Utilities.FormattedJulianTimestamp((long)dr["TIME_STAMP"]);
                dataTable.Rows.Add(row);
            }

            base.DoPopulate(aConfig, dataTable, aDataGrid);

            string gridHeaderText = string.Format(
                (dataTable.Rows.Count > 1) ? Properties.Resources.MultipleDirectoryServerCount : 
                                             Properties.Resources.SingleDirectoryServerCount, 
                dataTable.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);
            if (dataTable.Rows.Count > 0)
            {
                aDataGrid.ResizeGridColumns(dataTable, 7, 20);
            }

            _theDirectoryServersUserControl.ResetControls();
        }
        #endregion Private methods
    }

    #endregion DirectoryServersDataHandler Class
}
