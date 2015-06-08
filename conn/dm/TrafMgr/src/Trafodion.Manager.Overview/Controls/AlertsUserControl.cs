//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// The Alerts Widget. Displays alerts details
    /// </summary>
    public partial class AlertsUserControl : UserControl, ICloneToWindow
    {
        #region private member variables

        static readonly string AlertsPersistenceKey = "SQAlertOptionsPersistence2";
        Persistence.PersistenceHandler _persistenceHandler = null;

        int alertsWidgetRowCount = 1000;
        UniversalWidgetConfig _universalWidgetConfig;
        GenericUniversalWidget _alertsWidget;
        TrafodionIGrid _alertsGrid;
        iGDropDownList _alertStatusDropDownList = new iGDropDownList();
        iGColPattern _alertLevelColPattern = new iGColPattern();
        iGColPattern _alertStatusColPattern = new iGColPattern();
        iGColPattern _alertNotesColPattern = new iGColPattern();
        ConnectionDefinition _connectionDefinition;
        private TrafodionIGridToolStripMenuItem _viewSymptomEventsMenuItem;
        private TrafodionIGridToolStripMenuItem _bulkUpdateMenuItem;

        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();
        AlertOptionsModel _alertOptions;
        Dictionary<int, AlertsPrimaryKey> _modifiedAlerts = new Dictionary<int, AlertsPrimaryKey>();

        static bool _isFirstTimeEdit = true; //Is the alerts grid being edited for the first time ?
        private bool _isEditInProgress = false;
        private RepoAlertDataProvider _theRepoAlertDataProvider = null;
        object _oldStatusCellValue;
        object _oldNotesCellValue;
        private const string TRACE_SUB_AREA_NAME = "System Alerts";
        public delegate void HandleEvent(Object obj, EventArgs e);

        private LiveFeedConnection _theLiveFeedConnection = null;
        private CachedLiveFeedDataProviderConfig _theQpidConfig = null;        
        private LiveAlertDataProvider _theLiveAlertDataProvider = null;
        private delegate void LiveFeed_HandleEvent(Object obj, DataProviderEventArgs e);        
        
        private DataTable alertwidgeDataTable = new DataTable();  
        ToolStripButton _refreshButton;

        TrafodionProgressUserControl _progressUserControl;
        EventHandler<TrafodionProgressCompletedArgs> _progressHandler;
        private bool initializationInProgress = true;

        public bool InitializationInProgress
        {
            get { return initializationInProgress; }            
        }

        private AlertsWidgetDataHandler2 alertDisplayHandler;        

        public enum InitializationStates
        {
            NO_ODBC = 1,
            NO_REPOSITORY_VIEW = 2,
            NO_ACCESS_PRIVILEGE = 3,
            REPOSITORY_AND_LIVE = 4
        };

        private int initializationState = 1;
        public int InitializationState
        {
            get { return initializationState; }
            set { initializationState = value; }
        }

        #endregion private member variables

        #region Public Properties

        #region COLUMN NAMES
        public static readonly string ALERT_CREATE_TS_LCT_COL_NAME = "CREATE_TS_LCT";
        public static readonly string ALERT_CREATOR_PROCESS_ID_COL_NAME = "CREATOR_PROCESS_ID";
        public static readonly string ALERT_CREATOR_HOST_ID_COL_NAME = "CREATOR_HOST_ID";
        public static readonly string ALERT_LAST_UPDATE_TS_LCT_COL_NAME = "LAST_UPDATE_TS_LCT";
        public static readonly string ALERT_CLOSE_TS_LCT_COL_NAME = "CLOSE_TS_LCT";
        public static readonly string ALERT_COMPONENT_NAME_COL_NAME = "PROBLEM_COMPONENT_NAME";
        public static readonly string ALERT_PROCESS_NAME_COL_NAME = "PROBLEM_PROCESS_NAME";
        public static readonly string ALERT_PROCESS_ID_COL_NAME = "PROBLEM_PROCESS_ID";
        public static readonly string ALERT_IP_ADDRESS_ID_COL_NAME = "PROBLEM_IP_ADDRESS_ID";
        public static readonly string ALERT_RESOURCE_COL_NAME = "RESOURCE_NAME";
        public static readonly string ALERT_RESOURCE_TYPE_COL_NAME = "RESOURCE_TYPE";
        public static readonly string ALERT_SEVERITY_COL_NAME = "SEVERITY";
        public static readonly string ALERT_SEVERITY_NAME_COL_NAME = "SEVERITY_NAME";
        public static readonly string ALERT_DESCRIPTION_COL_NAME = "DESCRIPTION";
        public static readonly string ALERT_STATUS_COL_NAME = "STATUS";
        public static readonly string ALERT_CREATE_TS_UTC_COL_NAME = "CREATE_TS_UTC";
        public static readonly string ALERT_NOTES_COL_NAME = "NOTES";
        public static readonly string ALERT_TYPE_COL_NAME = "TYPE";
        public static readonly string ALERT_TYPE_DESCRIPTION_COL_NAME = "TYPE_DESCRIPTION";
        public static readonly string ALERT_STATUS_OPEN = "OPEN";
        public static readonly string ALERT_STATUS_USER_CLOSED = "USERCLOSED";
        public static readonly string ALERT_STATUS_OP_CLOSED = "OPCLOSED";
        public static readonly string ALERT_STATUS_AUTO_CLOSED = "AUTOCLOSED";
        public static readonly string ALERT_STATUS_ACK = "ACKNOWLEDGED";
        public static readonly string ALERT_STATUS_UNKNOWN = "UNKNOWN";

        public static readonly string ALERT_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";

        #endregion COLUMN NAMES

        public iGColPattern AlertLevelColPattern
        {
            get { return _alertLevelColPattern; }
        }
        public iGColPattern AlertStatusColPattern
        {
            get { return _alertStatusColPattern; }
        }
        public iGColPattern AlertNotesColPattern
        {
            get { return _alertNotesColPattern; }
        }
        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }

        /// <summary>
        /// Dictionary of the primary keys of the modified alerts
        /// </summary>
        public Dictionary<int, AlertsPrimaryKey> ModifiedAlerts
        {
            get { return _modifiedAlerts; }
            set { _modifiedAlerts = value; }
        }

        /// <summary>
        /// Identifies if the widget is in edit mode
        /// </summary>
        public bool IsEditInProgress
        {
            get { return _isEditInProgress; }
            set { _isEditInProgress = value; }
        }

        /// <summary>
        /// The connection definition associated with this alerts widget instance
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
        }

        /// <summary>
        /// The options model for the Alerts widget
        /// </summary>
        public AlertOptionsModel AlertOptions
        {
            get { return _alertOptions; }
        }

        /// <summary>
        /// Alerts data provider
        /// </summary>
        public RepoAlertDataProvider TheDataProvider
        {
            get { return _theRepoAlertDataProvider; }
        }

        /// <summary>
        /// List of the default visible column names.
        /// This list is overriden when the user customizes the visible column list
        /// </summary>
        public List<string> DefaultVisibleColumns
        {
            get
            {
                return new List<string>
                    {   
                        ALERT_CREATE_TS_LCT_COL_NAME, ALERT_LAST_UPDATE_TS_LCT_COL_NAME, ALERT_CLOSE_TS_LCT_COL_NAME, 
                        ALERT_STATUS_COL_NAME, ALERT_SEVERITY_NAME_COL_NAME, ALERT_RESOURCE_COL_NAME, ALERT_RESOURCE_TYPE_COL_NAME, 
                        ALERT_TYPE_COL_NAME, ALERT_TYPE_DESCRIPTION_COL_NAME, ALERT_DESCRIPTION_COL_NAME, ALERT_NOTES_COL_NAME, 
                        ALERT_COMPONENT_NAME_COL_NAME, ALERT_PROCESS_ID_COL_NAME, ALERT_IP_ADDRESS_ID_COL_NAME, ALERT_PROCESS_NAME_COL_NAME  
                    };
            }
        }

        /// <summary>
        /// The drop down list object that contains the Alerts status
        /// </summary>
        public iGDropDownList AlertStatusDropDownList
        {
            get { return _alertStatusDropDownList; }
        }

        /// <summary>
        /// Enables/Disables the apply button
        /// </summary>
        public bool ApplyEnabled
        {
            get { return _applyButton.Enabled; }
            set { _applyButton.Enabled = value; }
        }

        #endregion Public Properties

        #region SQL queries

        /// <summary>
        /// Query to fetch all open or acknowledged alerts
        /// </summary>
        string _openAlertsQuery = "SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.PROBLEM_INSTANCE_1 "
                                   + "WHERE STATUS IN ('OPEN', 'ACKNOWLEDGED') "
                                   + "{0} ORDER BY LAST_UPDATE_TS_LCT DESC "
                                   + "FOR READ UNCOMMITTED ACCESS";
        private TrafodionLabel TrafodionLabel1;

        /// <summary>
        /// Query to fetch all alerts including closed alerts
        /// </summary>
        string _allAlertsQuery = "SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.PROBLEM_INSTANCE_1 {0} ORDER BY LAST_UPDATE_TS_LCT DESC FOR READ UNCOMMITTED ACCESS";

        #endregion SQL queries

        #region Constructors/Destructors

        /// <summary>
        /// Constructs the user control
        /// </summary>
        /// <param name="aConnectionDefinition">Connection definition used by the widget</param>
        public AlertsUserControl(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;

            InitializeProgressControl();
        }


        public void CompleteInitialization() 
        {
            //ReadAlertOptionsFromPersistence();
            ////Instantiate the data provider.     
            //InitializeDataProvider(initializationState);

            //Clear Progress Bar and display controls
            _theProgressPanel.Controls.Clear();
            _progressUserControl.ProgressCompletedEvent -= Component_ProgressCompletedEvent;
            _theProgressPanel.Visible = false;
            _theContentPanel.Visible = true;

            SetupComponents();

            initializationInProgress = false;
        }

        public void InitializeProgressControl() 
        {
            _theContentPanel.Visible = false;
            _theProgressPanel.Visible = true;

            Object[] parameters = new Object[] { ConnectionDefn };

            TrafodionProgressArgs args = new TrafodionProgressArgs("Initializing Alert Widget...", this, "CheckInitializationState", parameters);
            _progressUserControl = new TrafodionProgressUserControl(args);

            _progressUserControl.Dock = DockStyle.Top;
            _progressHandler = Component_ProgressCompletedEvent;
            _progressUserControl.ProgressCompletedEvent += _progressHandler;
            _theProgressPanel.Controls.Clear();
            _theProgressPanel.Controls.Add(_progressUserControl);

        }


        private void Component_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {

            if (e.Error != null)
            {                
                MessageBox.Show("Initialization Alert Widget Failed. Please exit and reopen TrafodionManager.",
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                _theProgressPanel.Controls.Clear();
            }
            else 
            {
                CompleteInitialization();
            }
        }

        //public void Initializa() 
        //{
        //    AlertsInitializingUserControl ac = new AlertsInitializingUserControl();
        //    ac.Anchor = AnchorStyles.None;
        //    ac.Left = (widgetPanel.ClientSize.Width - ac.Width) / 2;
        //    ac.Top = (widgetPanel.ClientSize.Height - ac.Height) / 2;
        //    widgetPanel.Controls.Add(ac);
        //}

        public void InitializeDataProvider(int state)
        {
            switch (state)
            {
                case 1:
                case 2:
                case 3:
                    InitializeLiveAlertDataProvider();
                    break;
                case 4:
                    InitializeRepoAlertDataProvider();
                    InitializeLiveAlertDataProvider();
                    break;
            }
        }



        /// <summary>
        /// Constructor called by the clone method
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aDataProvider"></param>
        /// <param name="alertsLight"></param>
        //public SystemAlertsUserControl2(ConnectionDefinition aConnectionDefinition, RepoAlertsDataProvider aDataProvider)
        //{
        //    InitializeComponent();
        //    _connectionDefinition = aConnectionDefinition;
        //    _theRepoAlertDataProvider = aDataProvider;

        //    //Check to see if alerts view exists
        //    if (TheDataProvider.ViewExists)
        //    {
        //        //If view exists, check if current logged on user has privileges to view the alerts view
        //        if (TheDataProvider.UserCanView)
        //        {
        //            SetupComponents();

        //            //User has privileges to view alerts. Proceed to display alerts
        //            this._alertsWidget.DataDisplayControl.DataDisplayHandler.DoPopulate(_universalWidgetConfig,
        //                            TheDataProvider.GetDataTable(), _alertsGrid);
        //        }
        //        else
        //        {
        //            //User does not have privileges to view alerts. Display an error message.
        //            DisplayErrorPanel(Properties.Resources.NoPrivilegeToViewAlerts);
        //        }
        //    }
        //    else
        //    {
        //        //Alerts view does not exist. It is possible we are connected to an older release. Display info message
        //        DisplayErrorPanel(Properties.Resources.AlertsFunctionNotSupported);
        //    }
        //}

        /// <summary>
        /// Do cleanup on dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Save options to persistence
                SavePersistence();

                if (_alertsWidget != null)
                {
                    if (_theRepoAlertDataProvider != null)
                    {
                        _theRepoAlertDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                        //_theRepoAlertDataProvider.OnErrorEncountered -= InvokeHandleError;
                        _alertsWidget.DataProvider.OnBeforeFetchingData -= InvokeHandleBeforeFetchingData;
                    }
                    if (_theLiveAlertDataProvider != null)
                    {
                        _theLiveAlertDataProvider.OnNewDataArrived -= LiveAlert_InvokeHandleNewDataArrived;
                        _theLiveFeedConnection.BrokerConfiguration.OnBrokerChanged -= LiveFeed_BrokerConfiguration_OnBrokerChanged;
                    }

                    _alertsWidget.RowCountTextBox.KeyPress -= new KeyPressEventHandler(RowCountTextBox_KeyPress);
                    _alertsWidget.RowCountTextBox.Leave -= new EventHandler(RowCountTextBox_Leave);
                    //((TabularDataDisplayControl)_alertsWidget.DataDisplayControl).Dispose();
                    _alertsWidget.Dispose();
                }

                //un-register listener to persistence
                Persistence.PersistenceHandlers -= _persistenceHandler;
            }
        }
        #endregion Constructors/Destructors

        #region private methods

        private void InitializeRepoAlertDataProvider()
        {
            _theRepoAlertDataProvider = new RepoAlertDataProvider();
            _theRepoAlertDataProvider.DataProviderConfig.ConnectionDefinition = ConnectionDefn;
            _theRepoAlertDataProvider.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            if (_universalWidgetConfig != null && _universalWidgetConfig.DataProviderConfig != null)
            {
                if (_universalWidgetConfig.DataProviderConfig.ColumnMappings != null)
                {
                    _theRepoAlertDataProvider.DataProviderConfig.ColumnMappings = new List<ColumnMapping>(_universalWidgetConfig.DataProviderConfig.ColumnMappings);
                }

                if (_universalWidgetConfig.DataProviderConfig.ColumnSortObjects != null)
                {
                    _theRepoAlertDataProvider.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>(_universalWidgetConfig.DataProviderConfig.ColumnSortObjects);
                }

                if (_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames != null)
                {
                    _theRepoAlertDataProvider.DataProviderConfig.CurrentVisibleColumnNames = new List<string>(_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames);
                }

                if (_universalWidgetConfig.DataProviderConfig.PrefetchColumnNameList != null)
                {
                    _theRepoAlertDataProvider.DataProviderConfig.PrefetchColumnNameList = new List<string>(_universalWidgetConfig.DataProviderConfig.PrefetchColumnNameList);
                }
                else
                {
                    if (_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames != null)
                    {
                        _theRepoAlertDataProvider.DataProviderConfig.PrefetchColumnNameList = new List<string>(_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames);
                    }
                }
            }

            _theRepoAlertDataProvider.OnBeforeFetchingData += InvokeHandleBeforeFetchingData;
            SetSqlQueryText();

            //The data provider would do some system checks the very first time
            _theRepoAlertDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            //_theRepoAlertDataProvider.OnErrorEncountered += InvokeHandleError;

            _theRepoAlertDataProvider.IsValidationRun = false;
            //_theRepoAlertDataProvider.Start();

        }

        private void InitializeLiveAlertDataProvider()
        {
            _theQpidConfig = new CachedLiveFeedDataProviderConfig();
            _theQpidConfig.TheDataFormat = CachedLiveFeedDataProviderConfig.LiveFeedDataFormat.ProtoBuf;
            _theQpidConfig.ConnectionDefinition = this.ConnectionDefn;
            if (_universalWidgetConfig != null && _universalWidgetConfig.DataProviderConfig != null)
            {
                if (_universalWidgetConfig.DataProviderConfig.ColumnMappings != null)
                {
                    _theQpidConfig.ColumnMappings = new List<ColumnMapping>(_universalWidgetConfig.DataProviderConfig.ColumnMappings);
                }

                if (_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames != null)
                {
                    _theQpidConfig.ColumnSortObjects = new List<ColumnSortObject>(_universalWidgetConfig.DataProviderConfig.ColumnSortObjects);
                }

                if (_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames != null)
                {
                    _theQpidConfig.CurrentVisibleColumnNames = new List<string>(_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames);
                }

                if (_universalWidgetConfig.DataProviderConfig.PrefetchColumnNameList != null)
                {
                    _theQpidConfig.PrefetchColumnNameList = new List<string>(_universalWidgetConfig.DataProviderConfig.PrefetchColumnNameList);
                }
                else
                {
                    if (_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames != null)
                    {
                        _theQpidConfig.PrefetchColumnNameList = new List<string>(_universalWidgetConfig.DataProviderConfig.CurrentVisibleColumnNames);
                    }
                }
            }

            List<string> publications = new List<string>() { LiveAlertDataProvider.problem_open_routing_key, LiveAlertDataProvider.problem_close_routing_key };
            _theQpidConfig.Configure(publications.ToArray());
            _theLiveAlertDataProvider = new LiveAlertDataProvider(_theQpidConfig);

            _theLiveFeedConnection = _theLiveAlertDataProvider.LiveFeedConnection;

            _theLiveAlertDataProvider.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            _theLiveFeedConnection.BrokerConfiguration.OnBrokerChanged += new LiveFeedBrokerConfiguration.LiveFeedBrokerChanged(LiveFeed_BrokerConfiguration_OnBrokerChanged);
            
            _theLiveAlertDataProvider.OnNewDataArrived += LiveAlert_InvokeHandleNewDataArrived;

            //if (initializationState != 1)
            //{
            //    Connection connection = new Connection(this.ConnectionDefn);
            //    _theLiveAlertDataProvider.ConstrucDimensionTablesfromDB(connection);
            //}
        }        
        
        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(AlertsDataProvider_OnNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(AlertsDataProvider_OnErrorEncountered), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview, ex.StackTrace);
            }
        }
        //private override _alert


        void AlertsDataProvider_OnErrorEncountered(object sender, EventArgs eArg)
        {
            DataProviderEventArgs e = (DataProviderEventArgs)eArg;
            string errorMessage = "Error initializing Alerts widget\n\n" + e.Exception.Message;
            if (e.Exception is OdbcException)
            {
                OdbcException oe = e.Exception as OdbcException;
                bool error4082Found = false; //table does not exist
                foreach (OdbcError error in oe.Errors)
                {
                    if (error.NativeError == -4082)
                        error4082Found = true;
                }

                // Does the schema (1003) or table (4082) not exist?
                if (oe.Errors.Count > 0 && error4082Found)
                {
                    errorMessage = Properties.Resources.AlertsFunctionNotSupported;
                }
            }
            DisplayErrorPanel(errorMessage);
        }

        /// <summary>
        /// Handle new data returned by the dataprovider
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void AlertsDataProvider_OnNewDataArrived(object sender, EventArgs e)
        {
            DataTable tempNewComingAlertDataTable = _theRepoAlertDataProvider.GetDataTable();
            if (tempNewComingAlertDataTable == null || tempNewComingAlertDataTable.Rows.Count == 0) 
            {
                if (_alertsGrid.Rows.Count == 0) 
                {
                    //Clear Grid Header
                    string gridHeaderText = string.Format(Properties.Resources.AlertsCountAfterFetch, 0, "");
                    this._alertsGrid.UpdateCountControlText(gridHeaderText);
                }
                return;
            }
                

            if (_alertsGrid.Cols.Count == 0)
            {
                tempNewComingAlertDataTable = GetTopNAlertsDataTable(tempNewComingAlertDataTable, GetAlertsWidgetMaxRowCount());
            }
            else
            {
                DataTable alertWidgetDataTable = tempNewComingAlertDataTable.Clone();
                alertwidgeDataTable = _alertsGrid.ExportToDataTable(alertWidgetDataTable);
                DataTable liveAlert = FilterLiveAlertFromAllAlerts(alertwidgeDataTable);
                tempNewComingAlertDataTable = LiveMergeRepo(liveAlert, tempNewComingAlertDataTable);
                //alertDisplayHandler.PopulateAlert(_universalWidgetConfig, tempNewComingAlertDataTable, _alertsGrid, true);
            }
            alertDisplayHandler.PopulateAlert(_universalWidgetConfig, tempNewComingAlertDataTable, _alertsGrid, true);
        }
               
        private DataTable FilterLiveAlertFromAllAlerts(DataTable sourceDataTable)
        {
            DataTable result = sourceDataTable.Clone();
            int count = sourceDataTable.Rows.Count;
            if (count > 0)
            {                
                string conditions = "[UPSERT_TS_LCT] is NULL";
                result = FilterDataTable(sourceDataTable, conditions);
            }
            return result;
        }

        //private DataTable FilterDataTableWithTime(DataTable sourceDataTable)
        //{
        //    //sourceDataTable is already sorted;
        //    int count = sourceDataTable.Rows.Count;
        //    if (count > 0)
        //    {
        //        DateTime filterTime = (DateTime)sourceDataTable.Rows[count - 1][ALERT_LAST_UPDATE_TS_LCT_COL_NAME];
        //        string conditions = ALERT_LAST_UPDATE_TS_LCT_COL_NAME + " >= " + "'" + filterTime.ToString(TheDateTimeFormatProvider) + "'";
        //        return FilterDataTable(sourceDataTable, conditions);
        //    }
        //    else
        //    {
        //        return sourceDataTable;
        //    }
        //}


        /// <summary>
        /// Displays an error panel
        /// </summary>
        /// <param name="errorMessage">The error message to be displayed inside the panel</param>
        void DisplayErrorPanel(string errorMessage)
        {
            Controls.Clear();

            TrafodionTextBox errorTextBox = new TrafodionTextBox();
            errorTextBox.WordWrap = errorTextBox.ReadOnly = errorTextBox.Multiline = true;
            errorTextBox.Text = errorMessage;
            errorTextBox.Dock = DockStyle.Fill;

            Controls.Add(errorTextBox);
        }

        public void CheckInitializationState(ConnectionDefinition conn)
        {
            if (conn.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
            {
                initializationState = (int)InitializationStates.REPOSITORY_AND_LIVE;
            }
            else
                initializationState = (int)InitializationStates.NO_ODBC;

            ReadAlertOptionsFromPersistence();

            //Instantiate the data provider.     
            InitializeDataProvider(initializationState);
        }

        #region RowCount

        /// <summary>
        /// Event handler for mouse leaving row count box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void RowCountTextBox_Leave(object sender, EventArgs e)
        {
            RowCountTextBox_TextChanged();
        }

        /// <summary>
        /// Event handler for key press in row count box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void RowCountTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            // Wait until the Enter key to process the change.
            if (e.KeyChar == (Char)Keys.Enter)
            {
                RowCountTextBox_TextChanged();
            }
        }

        /// <summary>
        /// Processing row count changed
        /// </summary>
        private void RowCountTextBox_TextChanged()
        {
            int count = _alertsWidget.RowCount;
            if (count > 0)
            {
                //alertwidgeDataTable.DefaultView.Sort = ALERT_LAST_UPDATE_TS_LCT_COL_NAME + " DESC";
                //alertwidgeDataTable = alertwidgeDataTable.DefaultView.ToTable();

                //alertwidgeDataTable = GetTopNAlertsDataTable(alertwidgeDataTable, count);
                //alertDisplayHandler.PopulateAlert(_universalWidgetConfig, alertwidgeDataTable, _alertsGrid, true);
                GetTopNRowsofIGrid();
                if (alertDisplayHandler != null && _alertsGrid.Cols.Count > 0)
                {
                    alertDisplayHandler.CalculateAlertCount(_alertsGrid);
                }  
            }
        }

        public DataTable GetTopNAlertsDataTable(DataTable alertDataTable, int count)
        {
            if (alertDataTable.Rows.Count > count)
            {
                alertDataTable.DefaultView.Sort = ALERT_LAST_UPDATE_TS_LCT_COL_NAME + " DESC";
                alertDataTable = alertDataTable.DefaultView.ToTable();
                alertDataTable = GetTopNRows(alertDataTable, count);
            }

            return alertDataTable;
        }

        public DataTable GetTopNRows(DataTable dt, int n)
        {
            if (dt.Rows.Count > n)
            {
                DataTable result = dt.Clone();
                for (int i = 0; i < n; i++)
                {
                    result.ImportRow(dt.Rows[i]);
                }
                return result;
            }
            else
                return dt;
        }


        #endregion RowCount




        /// <summary>
        /// Sets up the Alerts Widget
        /// </summary>
        void SetupComponents()
        {
            //Register to listen to persistence events, so you can save options, when notified.
            _persistenceHandler = new Persistence.PersistenceHandler(Persistence_PersistenceHandlers);
            Persistence.PersistenceHandlers += _persistenceHandler;

            if (_universalWidgetConfig == null)
            {
                //Create the Universal widget configuration
                _universalWidgetConfig = new UniversalWidgetConfig();
                //_universalWidgetConfig.DataProviderConfig = _theRepoAlertDataProvider.DataProviderConfig;

                _universalWidgetConfig.Name = AlertsPersistenceKey;
            }

            if (initializationState == 4)
            {
                //Add the possible status strings to the alerts status drop down list
                //User can only either acknowledge or operator close an alert.
                _alertStatusDropDownList.Items.Add(ALERT_STATUS_ACK);
                _alertStatusDropDownList.Items.Add(ALERT_STATUS_USER_CLOSED);
                _alertStatusDropDownList.Items.Add(ALERT_STATUS_OPEN);
                CreateIGridColumnPatterns();
            }
            if (initializationState == 4)
                _universalWidgetConfig.DataProviderConfig = _theRepoAlertDataProvider.DataProviderConfig;
            else
                _universalWidgetConfig.DataProviderConfig = _theLiveAlertDataProvider.DataProviderConfig;

            _universalWidgetConfig.DataProviderConfig.DefaultVisibleColumnNames = this.DefaultVisibleColumns;

            List<ColumnMapping> persistedColumnMappings = _universalWidgetConfig.DataProviderConfig.ColumnMappings;
            _universalWidgetConfig.DataProviderConfig.ColumnMappings = InitializationColumnMappings();
            ColumnMapping.Synchronize(_universalWidgetConfig.DataProviderConfig.ColumnMappings, persistedColumnMappings);

            _universalWidgetConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            _universalWidgetConfig.ShowExportButtons = false;
            _universalWidgetConfig.ShowHelpButton = true;
            _universalWidgetConfig.ShowRefreshTimerButton = false;
            _universalWidgetConfig.ShowTimerSetupButton = false;
            _universalWidgetConfig.ShowProviderStatus = false;
            _universalWidgetConfig.ShowRowCount = true;

            if (initializationState == 4)
                _universalWidgetConfig.ShowRefreshButton = true;
            else
                _universalWidgetConfig.ShowRefreshButton = false;
            
            _universalWidgetConfig.HelpTopic = HelpTopics.SystemAlerts;

            
            //Create the Alerts Widget
            _alertsWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)_alertsWidget.DataDisplayControl).LineCountFormat = Properties.Resources.AlertsWaiting;

            if (initializationState == 4)
                _alertsWidget.DataProvider = _theRepoAlertDataProvider;
            else
            {
                _alertsWidget.DataProvider = _theLiveAlertDataProvider;                
            }
            //Set the widget configuration 
            _alertsWidget.UniversalWidgetConfiguration = _universalWidgetConfig;

            _alertsWidget.Dock = DockStyle.Fill;

            //_alertsWidget.RowCountTextBox
            _alertsWidget.RowCount = alertsWidgetRowCount;
            _alertsWidget.RowCountTextBox.KeyPress += new KeyPressEventHandler(RowCountTextBox_KeyPress);
            _alertsWidget.RowCountTextBox.Leave += new EventHandler(RowCountTextBox_Leave);

            // Remove all current contents and add the alerts widget
            widgetPanel.Controls.Clear();
            widgetPanel.Controls.Add(_alertsWidget);

            //Associate the custom data display handler for the TabularDisplay panel
            alertDisplayHandler = new AlertsWidgetDataHandler2(this);
            _alertsWidget.DataDisplayControl.DataDisplayHandler = alertDisplayHandler;
            
            //Associate the alerts options tool strip buttons to the Universal widget
            ToolStripButton optionsButton = new ToolStripButton(Properties.Resources.AlterIcon);
            optionsButton.Click += new System.EventHandler(optionsButton_Click);
            optionsButton.ToolTipText = Properties.Resources.AlertsOptionTooltip;
            _alertsWidget.AddToolStripItem(optionsButton);

            //Initialize the Alerts iGrid
            _alertsGrid = ((TabularDataDisplayControl)_alertsWidget.DataDisplayControl).DataGrid;
            _alertsGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
            _alertsGrid.RequestEdit += new iGRequestEditEventHandler(_alertsGrid_RequestEdit);
            _alertsGrid.BeforeCommitEdit += new iGBeforeCommitEditEventHandler(_alertsGrid_BeforeCommitEdit);
            _alertsGrid.AfterCommitEdit += new iGAfterCommitEditEventHandler(_alertsGrid_AfterCommitEdit);
            _alertsGrid.SelectionMode = iGSelectionMode.MultiExtended;
            _alertsGrid.CancelEdit += new iGCancelEditEventHandler(_alertsGrid_CancelEdit);
            _alertsGrid.EllipsisButtonClick += new iGEllipsisButtonClickEventHandler(_alertsGrid_EllipsisButtonClick);
            if (initializationState == 4) 
            {
                _alertsGrid.DoubleClickHandler = DoubleClickHandler;
            }
            
            //_alertsGrid.CellMouseDown += new iGCellMouseDownEventHandler(_alertsGrid_CellMouseDown);

            //Disable the export buttons so it does not show up within the universal widget panel
            //But get the export buttons from the grid and add them to their own panel
            ((TabularDataDisplayControl)_alertsWidget.DataDisplayControl).ShowExportButtons = false;
            Control exportButtonsControl = _alertsGrid.GetButtonControl();
            exportButtonsControl.Dock = DockStyle.Right;
            exportButtonsPanel.Controls.Clear();
            exportButtonsPanel.Controls.Add(exportButtonsControl);

            //If user has update privileges, then enable the update context menu and the apply button
            //if (TheDataProvider.UserCanUpdate)
            if (initializationState == 4)
            {
                _applyButton.Visible = true;

                //Update Alert(s)
                _bulkUpdateMenuItem = new TrafodionIGridToolStripMenuItem();
                _bulkUpdateMenuItem.Text = Properties.Resources.UpdateAlertsMenu;
                _bulkUpdateMenuItem.Click += new EventHandler(bulkUpdateMenuItem_Click);
                _alertsGrid.AddContextMenu(_bulkUpdateMenuItem);

                //View Symptoms
                _viewSymptomEventsMenuItem = new TrafodionIGridToolStripMenuItem();
                _viewSymptomEventsMenuItem.Text = Properties.Resources.ViewSymptomMenu;
                _viewSymptomEventsMenuItem.Click += new EventHandler(viewSymptomEventsMenuItem_Click);
                _alertsGrid.AddContextMenu(_viewSymptomEventsMenuItem);
            }

            _theLiveAlertDataProvider.Start();
            if (initializationState == 4)
            {
                _alertsWidget.DataProvider.Start();
            }

            LocateRefreshButton();
        }


        private void LocateRefreshButton() 
        {            
            try
            {
                var toolStrip = (TrafodionToolStrip)this.Controls.Find("_theToolStrip", true)[0];
                _refreshButton = (ToolStripButton)toolStrip.Items.Find("_theRefreshButton", true)[0];
                if (_refreshButton != null)
                {
                    //RemoveClickEvent(_refreshButton);                    
                    //_refreshButton.Click += _refreshButton_Click;

                    if (initializationState == 4)
                    {
                        _refreshButton.Visible = true;
                    }
                    else
                    {                        
                        //when an update fails we need the refresh button to reset the grid
                        //otherwise the modified rows in the grid will still be highlighted in green color.
                        _refreshButton.Visible = false;
                    }

                }
            }
            catch (Exception)
            {                
                //Keep Silence...
            }
            
        }

        private int GetAlertsWidgetMaxRowCount()
        {
            int alertsWidgetMaxRowCount = alertsWidgetRowCount;
            Int32.TryParse(_alertsWidget.RowCountTextBox.Text.Trim(), out alertsWidgetMaxRowCount);
            return alertsWidgetMaxRowCount;
        }

        //private void _refreshButton_Click(object sender, EventArgs e) 
        //{
        //    foreach (iGRow row in _alertsGrid.Rows)
        //    {
        //        row.BackColor = Color.White;
        //    }
        //    //_isEditInProgress = false;
        //    if (_theRepoAlertDataProvider != null)
        //    {
        //        _theRepoAlertDataProvider.DataProviderConfig.MaxRowCount = _alertsWidget.RowCount;
        //        this._theRepoAlertDataProvider.RefreshData();
        //    }
        //}

        private void ReadAlertOptionsFromPersistence()
        {
            //Read the alert options from persistence. 
            //If the alert options does not exist or there is an error reading the persistence, create
            //a default alerts option
            try
            {
                _alertOptions = Trafodion.Manager.Framework.Persistence.Get(AlertsPersistenceKey) as AlertOptionsModel;
                _universalWidgetConfig = WidgetRegistry.GetConfigFromPersistence(AlertsPersistenceKey);

                if (_alertOptions == null)
                {
                    _alertOptions = new AlertOptionsModel();
                }
                else
                {
                    //Set AlertOptions to default value; or we don't need persistent it;
                    bool fetchMode = _alertOptions.FetchOpenAlertsOnly;
                    _alertOptions = new AlertOptionsModel();
                    _alertOptions.FetchOpenAlertsOnly = fetchMode;
                }
            }
            catch (Exception ex)
            {
                _alertOptions = new AlertOptionsModel();
            }
        }

        private List<ColumnMapping> InitializationColumnMappings()
        {
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping(ALERT_CREATE_TS_LCT_COL_NAME, "Create Time LCT", 120));
            columnMappings.Add(new ColumnMapping(ALERT_LAST_UPDATE_TS_LCT_COL_NAME, "Last Update Time LCT", 120));
            columnMappings.Add(new ColumnMapping(ALERT_CLOSE_TS_LCT_COL_NAME, "Close Time LCT", 120));
            columnMappings.Add(new ColumnMapping(ALERT_STATUS_COL_NAME, "Status", 120));
            columnMappings.Add(new ColumnMapping(ALERT_SEVERITY_NAME_COL_NAME, "Severity Name", 120));
            columnMappings.Add(new ColumnMapping(ALERT_RESOURCE_COL_NAME, "Resource Name", 120));
            columnMappings.Add(new ColumnMapping(ALERT_RESOURCE_TYPE_COL_NAME, "Resource Type", 120));
            columnMappings.Add(new ColumnMapping(ALERT_TYPE_COL_NAME, "Type", 120));
            columnMappings.Add(new ColumnMapping(ALERT_TYPE_DESCRIPTION_COL_NAME, "Type Description", 120));
            columnMappings.Add(new ColumnMapping(ALERT_DESCRIPTION_COL_NAME, "Description", 200));
            columnMappings.Add(new ColumnMapping(ALERT_NOTES_COL_NAME, "Notes", 200));
            columnMappings.Add(new ColumnMapping(ALERT_COMPONENT_NAME_COL_NAME, "Problem Component Name", 120));
            columnMappings.Add(new ColumnMapping(ALERT_PROCESS_ID_COL_NAME, "Problem Process ID", 120));
            columnMappings.Add(new ColumnMapping(ALERT_PROCESS_NAME_COL_NAME, "Problem Process Name", 120));
            columnMappings.Add(new ColumnMapping(ALERT_IP_ADDRESS_ID_COL_NAME, "Problem IP Address ID", 120));
            columnMappings.Add(new ColumnMapping(ALERT_SEVERITY_COL_NAME, "Severity", 120));
            columnMappings.Add(new ColumnMapping(ALERT_CREATE_TS_UTC_COL_NAME, "Create Time UTC", 120));
            columnMappings.Add(new ColumnMapping(ALERT_CREATOR_PROCESS_ID_COL_NAME, "Creator Process ID", 120));
            columnMappings.Add(new ColumnMapping(ALERT_CREATOR_HOST_ID_COL_NAME, "Creator Host ID", 120));
            return columnMappings;
        }


        void _alertsGrid_RequestEdit(object sender, iGRequestEditEventArgs e)
        {
            if (e.ColIndex == _alertsGrid.Cols[ALERT_STATUS_COL_NAME].Index || e.ColIndex == _alertsGrid.Cols[ALERT_NOTES_COL_NAME].Index) 
            {
                _oldStatusCellValue = _alertsGrid.Rows[e.RowIndex].Cells[ALERT_STATUS_COL_NAME].Value as string;
                _oldNotesCellValue = _alertsGrid.Rows[e.RowIndex].Cells[ALERT_NOTES_COL_NAME].Value as string; 
            }

        }

        void _alertsGrid_CancelEdit(object sender, iGCancelEditEventArgs e)
        {
            if (e.ColIndex == _alertsGrid.Cols[ALERT_STATUS_COL_NAME].Index)
            {
                _alertsGrid.Cells[e.RowIndex, e.ColIndex].Value = _oldStatusCellValue;
            }
            else if (e.ColIndex == _alertsGrid.Cols[ALERT_NOTES_COL_NAME].Index)
            {
                _alertsGrid.Cells[e.RowIndex, e.ColIndex].Value = _oldNotesCellValue;
            }
        }

        private void InvokeHandleBeforeFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(DataProvider_OnFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
                throw new FetchVetoException();
            }
        }

        void DataProvider_OnFetchingData(object sender, EventArgs e)
        {
            if (_isEditInProgress)
            {
                if (MessageBox.Show(Utilities.GetForegroundControl(), "A refresh operation has been requested and you have unsaved changes. \nDo you want to continue ? If you choose Yes, unsaved changes will be discarded.",
                    Properties.Resources.Confirm, MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.No)
                {
                    throw new FetchVetoException();
                }
                else
                {
                    //User selected yes and agreed to lose unsaved changes.
                    _isEditInProgress = false;
                    _modifiedAlerts.Clear();
                }
            }
        }

        /// <summary>
        /// Sets the sql query text in the data provider config.
        /// Alerts widget supports viewing open alerts or all alerts.
        /// Also include/exclude dashboard alerts
        /// Based on the alert options, the appropriate query is set
        /// </summary>
        void SetSqlQueryText()
        {

            string rangeClause = "";
            
            rangeClause = _alertOptions.GetFilterSQL();
            _alertOptions.ServerGMTOffset = this.ConnectionDefn.ServerGMTOffset;

            if (_alertOptions.FetchOpenAlertsOnly)
            {
                if (!string.IsNullOrEmpty(rangeClause))
                {                    
                    rangeClause = " AND " + rangeClause;
                }                
                ((DatabaseDataProviderConfig)_theRepoAlertDataProvider.DataProviderConfig).SQLText =
                     string.Format(_openAlertsQuery, rangeClause);
            }
            else
            {
                if (!string.IsNullOrEmpty(rangeClause))
                {                    
                    rangeClause = " WHERE " + rangeClause;
                }                
                ((DatabaseDataProviderConfig)_theRepoAlertDataProvider.DataProviderConfig).SQLText =
                   string.Format(_allAlertsQuery, rangeClause);
            }
        }

        void viewSymptomEventsMenuItem_Click(object sender, EventArgs e)
        {
            if (sender is TrafodionIGridToolStripMenuItem)
            {
                int rowIndex = ((TrafodionIGridToolStripMenuItem)sender).TrafodionIGridEventObject.Row;
                viewSymptomEvents(rowIndex);
            }
        }

        void DoubleClickHandler(int rowIndex)
        {
            // Handle Live Feed Only differently
            if (_alertOptions.TimeRange == TimeRangesHandler.Range.LiveFeedOnly)
            {
                _alertsGrid.ShowRowDetails(rowIndex);
            }
            else
            {
                viewSymptomEvents(rowIndex);
            }
        }

        void viewSymptomEvents(int rowIndex)
        {
            DataTable table = new DataTable();
            foreach (iGCol col in _alertsGrid.Cols)
            {
                table.Columns.Add(col.Key);
            }

            ArrayList cellValues = new ArrayList();
            foreach (iGCell cell in _alertsGrid.Rows[rowIndex].Cells)
            {
                cellValues.Add(cell.Text);
            }
            table.Rows.Add(cellValues.ToArray());

            if (ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130)
            {
                RepoSymptomDetails details = new RepoSymptomDetails(this._connectionDefinition, table);
                //Add height of size to contain the TrafodionManager banner
                Size aSize = details.Size;
                aSize.Height += 100;
                WindowsManager.PutInWindow(aSize, details, "Symptoms", this._connectionDefinition);
            }
            else 
            {
                AlertSymptomEventsControl events = new AlertSymptomEventsControl(this._connectionDefinition, table);
                WindowsManager.PutInWindow(events.Size, events, "Symptom Events", this._connectionDefinition);
            }

            
        }
        /// <summary>
        /// Handle the update context menu click. Used for bulk updates
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void bulkUpdateMenuItem_Click(object sender, EventArgs e)
        {
            bool updateSucceeded = true;

            //Go into Edit mode. This pauses the timer
            BeginAlertsEdit(true);
            string existingNotes = "";
            List<int> selectedRowIndexes = new List<int>();

            if (_alertsGrid.RowMode)
            {
                foreach (iGRow row in _alertsGrid.SelectedRows)
                {
                    selectedRowIndexes.Add(row.Index);
                }
            }
            else
            {
                foreach (iGCell cell in _alertsGrid.SelectedCells)
                {
                    if (selectedRowIndexes.Contains(cell.RowIndex))
                        continue;

                    selectedRowIndexes.Add(cell.RowIndex);
                }
            }

            if (selectedRowIndexes.Count == 1)
            {
                existingNotes = _alertsGrid.Rows[selectedRowIndexes[0]].Cells[ALERT_NOTES_COL_NAME].Value as string;
            }

            //Display the update alerts dialog
            AlarmStatusUpdateDialog asud = new AlarmStatusUpdateDialog(this.ConnectionDefn, existingNotes);
            if (asud.ShowDialog() == DialogResult.OK)
            {
                //Get the updated notes and status
                string updatedStatus = asud.AlarmStatus.Trim();
                string updatedNotes = asud.AlarmNotes.Trim();

                foreach (int rowIndex in selectedRowIndexes)
                {
                    MarkModifiedRows(rowIndex, updatedStatus, updatedNotes);
                }

                //If there are modified rows, update the modified rows in the backend.
                if (_modifiedAlerts.Count > 0)
                {
                    updateSucceeded = UpdateStatus();
                }
            }

            if (updateSucceeded)
            {
                //If update succeeded, exit the edit mode and start the timer again
                EndAlertsEdit();
                if (_alertOptions.TimeRange != TimeRangesHandler.Range.LiveFeedOnly) 
                {
                    Start();
                }                
            }
        }

        /// <summary>
        /// Checks to see if the specified row has updated status and/or notes and if so
        /// marks them as modified
        /// </summary>
        /// <param name="RowIndex"></param>
        /// <param name="updatedStatus"></param>
        /// <param name="updatedNotes"></param>
        void MarkModifiedRows(int RowIndex, string updatedStatus, string updatedNotes)
        {
            //If the row index is already marked as modified, continue
            if (_modifiedAlerts.ContainsKey(RowIndex))
                return;

            iGRow row = _alertsGrid.Rows[RowIndex];

            //Get the current status and notes
            string oldStatus = row.Cells[ALERT_STATUS_COL_NAME].Value as string;
            string oldNotes = row.Cells[ALERT_NOTES_COL_NAME].Value as string;

            //Only alerts with status open/acknowledged can be updated
            if (oldStatus != null /*&& 
                (oldStatus.Trim().Equals(SystemAlertsUserControl.ALERT_STATUS_OPEN) ||
                 oldStatus.Trim().Equals(SystemAlertsUserControl.ALERT_STATUS_ACK) ||
                 oldStatus.Trim().Equals(SystemAlertsUserControl.ALERT_STATUS_USER_CLOSED))*/
                )
            {
                //If the old status and old notes are the same as the new status/notes
                //then nothings changed, ignore this row
                if (oldNotes != null && oldNotes.Trim().Equals(updatedNotes) && oldStatus.Trim().Equals(updatedStatus))
                    return;

                if ((oldStatus.Trim().Equals(SystemAlertsUserControl.ALERT_STATUS_AUTO_CLOSED) ||
                    oldStatus.Trim().Equals(updatedStatus))
                    &&
                    (string.IsNullOrEmpty(updatedNotes) ||
                    (oldNotes == null && updatedNotes == null) ||
                    (oldNotes != null && oldNotes.Trim().Equals(updatedNotes)))
                  )
                {
                    //Notes has not changed but user has tried to change status of an autoclosed problem.
                    //this is not allowed, so return.
                    return;
                }

                //Mark the row as being modified
                if (!_modifiedAlerts.ContainsKey(RowIndex))
                {
                    _modifiedAlerts.Add(RowIndex, CreateAlertsPrimaryKey(RowIndex, true));
                }

                if (!oldStatus.Trim().Equals(SystemAlertsUserControl.ALERT_STATUS_AUTO_CLOSED))
                {
                    //set the new status in the grid
                    row.Cells[ALERT_STATUS_COL_NAME].Value = updatedStatus;
                }

                //set the new notes in the grid
                row.Cells[ALERT_NOTES_COL_NAME].Value = updatedNotes;
            }
        }

        /// <summary>
        /// Apply button click. Used to update individual alerts
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _applyButton_Click(object sender, EventArgs e)
        {
            //If apply button is enabled, then the widget is already in edit mode.
            bool updateResult = UpdateStatus();            

            if (updateResult)
            {
                //If update succeeds then end edit mode and start the timer again
                EndAlertsEdit();
                if (_alertOptions.TimeRange != TimeRangesHandler.Range.LiveFeedOnly)
                {
                    Start();
                }
            }
            else 
            {
                //If update failed then the widget continues to be in edit mode allowing the user
                //to correct the problem
            }            
        }

        

        string EscapeEmbeddedSingleQuote(string inputString)
        {
            return inputString.Replace("'", "''");
        }

        DataRow GetOriginalAlertDataRow(AlertsPrimaryKey updateParam)
        {
//            DataTable dataTable = _theRepoAlertDataProvider.GetDataTable();
            DataTable dataTable = new DataTable();
            _alertsGrid.ExportToDataTable(dataTable);

            if (dataTable != null && dataTable.Rows.Count > 0)
            {
                foreach (DataRow row in dataTable.Rows)
                {
                    if (
                        (updateParam.CreateTime == (DateTime)row["Create Time LCT"]) &&
                        (updateParam.CreateHostId == (long)row["Creator Host ID"]) &&
                        (updateParam.CreateProcessId == (int)row["Creator Process ID"])
                       )
                    {
                        return row;
                    }
                }
            }
            return null;
        }

        /// <summary>
        /// Updates the modified alerts in the backend
        /// </summary>
        /// <returns></returns>
        bool UpdateStatus()
        {
            Connection connection = new Connection(this.ConnectionDefn);
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Create Time LCT");
            errorTable.Columns.Add("Creator Host ID");
            errorTable.Columns.Add("Creator Process ID");
            errorTable.Columns.Add("Error Text");

            //Run through the list of rows marked as modified and update them

            List<int> successfulCompletedAlerts = new List<int>();
            List<int> closedAlerts = new List<int>();

            foreach (KeyValuePair<int, AlertsPrimaryKey> modifiedAlert in _modifiedAlerts)
            {
                AlertsPrimaryKey primaryKey = _modifiedAlerts[modifiedAlert.Key];

                string status = _alertsGrid.Rows[modifiedAlert.Key].Cells[ALERT_STATUS_COL_NAME].Value.ToString().Trim();
                string notes = _alertsGrid.Rows[modifiedAlert.Key].Cells[ALERT_NOTES_COL_NAME].Value != null ?
                    _alertsGrid.Rows[modifiedAlert.Key].Cells[ALERT_NOTES_COL_NAME].Value.ToString().Trim() : null;

                string originalStatus = primaryKey.OriginalStatus;
                string originalNotes = primaryKey.OriginalNotes==null?null:primaryKey.OriginalNotes.Trim();
                if (originalStatus == null || (!string.IsNullOrEmpty(status) && status.Equals(originalStatus)))
                {
                    status = null; //status not been edited by the user.
                }

                if (originalNotes != null && !string.IsNullOrEmpty(notes) && notes.Equals(originalNotes))
                {
                    notes = null; //notes not been edited by the user.
                }

                if (string.IsNullOrEmpty(status) && string.IsNullOrEmpty(notes))
                {
                    errorTable.Rows.Add(new object[] { primaryKey.CreateTime.ToString(TheDateTimeFormatProvider), primaryKey.CreateHostId, primaryKey.CreateProcessId, "No change detected in the status or note text for this alert. Notes cannot be empty or null." });
                    continue;
                }                

                try
                {
                    string updateQueryText = "";


                    if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        updateQueryText = "CALL TRAFODION.SP.UPDATE_PROBLEM_2 (CAST(? AS TIMESTAMP(6)),?,?,?,?)";
                    }
                    else 
                    {
                        updateQueryText = "CALL TRAFODION.SP.UPDATE_PROBLEM (CAST(? AS TIMESTAMP(6)),?,?,?,?)";
                    }

                    OdbcCommand updateCommand = new OdbcCommand(updateQueryText, connection.OpenOdbcConnection);
                    updateCommand.CommandType = System.Data.CommandType.StoredProcedure;
                    OdbcParameter param1 = updateCommand.Parameters.Add("@CreateTsLct", OdbcType.Text);
                    param1.Direction = System.Data.ParameterDirection.Input;
                    param1.Value = primaryKey.CreateTime.ToString("yyyy-MM-dd HH':'mm':'ss.FFFFFF");

                    OdbcParameter param2 = new OdbcParameter();
                    if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        param2 = updateCommand.Parameters.Add("@CreatorHostId", OdbcType.BigInt);
                    }
                    else 
                    {
                        param2 = updateCommand.Parameters.Add("@CreatorHostId", OdbcType.Int);
                    }
                   
                    param2.Direction = System.Data.ParameterDirection.Input;
                    param2.Value = primaryKey.CreateHostId;

                    OdbcParameter param3 = updateCommand.Parameters.Add("@CreatorProcessId", OdbcType.Int);
                    param3.Direction = System.Data.ParameterDirection.Input;
                    param3.Value = primaryKey.CreateProcessId;

                    OdbcParameter param4 = updateCommand.Parameters.Add("@Status", OdbcType.Text);
                    param4.Direction = System.Data.ParameterDirection.Input;
                    if (string.IsNullOrEmpty(status))
                    {
                        param4.Value = DBNull.Value;
                    }
                    else
                    {
                        param4.Value = status;
                    }

                    OdbcParameter param5 = updateCommand.Parameters.Add("@Notes", OdbcType.Text);
                    param5.Direction = System.Data.ParameterDirection.Input;
                    if (string.IsNullOrEmpty(notes))
                    {
                        param5.Value = DBNull.Value;
                    }
                    else
                    {
                        param5.Value = notes;
                    }

                    int rows = Utilities.ExecuteNonQuery(updateCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Monitoring, TRACE_SUB_AREA_NAME, true);
                    updateCommand.Dispose();

                    //If the row has been marked for edit, clear the background since the update is now completed
                    _alertsGrid.Rows[modifiedAlert.Key].BackColor = System.Drawing.Color.White;

                    successfulCompletedAlerts.Add(modifiedAlert.Key);

                    if (!string.IsNullOrEmpty(status) && status.Contains("CLOSED")) 
                    {
                        if (_alertOptions.FetchOpenAlertsOnly == true) 
                        {
                            closedAlerts.Add(modifiedAlert.Key);
                        }
                    }

                }
                catch (Exception ex)
                {
                    errorTable.Rows.Add(new object[] { primaryKey.CreateTime, primaryKey.CreateHostId, primaryKey.CreateProcessId, ex.Message });
                }
            }
            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("Failed to update one or more alerts", errorTable, System.Drawing.SystemIcons.Error);                
                mmd.ShowDialog();
            }
            if (connection != null)
            {
                connection.Close();
            }

            foreach (var item in successfulCompletedAlerts)
            {  
                _modifiedAlerts.Remove(item);
            }

            //Remove closed alerts from grid
            closedAlerts.Sort();
            for (int i = 0; i < closedAlerts.Count; i++)
            {
                _alertsGrid.Rows.RemoveAt(closedAlerts[i] - i);
            }

            //Since all rows updated successfully, clear the modified list
            if (_modifiedAlerts.Count == 0)
            {
                return true;
            }
            else 
            {
                return false;
            }            
        }

        


        /// <summary>
        /// Place the widget in edit mode
        /// </summary>
        void BeginAlertsEdit(bool bulkUpdate)
        {
            if (TheDataProvider != null)
            {
                //Pause the data provider
                TheDataProvider.StopTimer();
            }

            //Enable the apply button
            ApplyEnabled = true;

            //Mark that edit is in progress
            _isEditInProgress = true;

            //Display an informational message once during the current TrafodionManager session
            if (_isFirstTimeEdit)
            {
                _isFirstTimeEdit = false;

                if (!bulkUpdate)
                {
                    //The message informs the user that the UI is now in an edit mode
                    MessageBox.Show(Properties.Resources.AlertsUpdateRequested,
                        Properties.Resources.UpdateAlertsMenu, MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
        }

        /// <summary>
        /// End the edit mode
        /// </summary>
        void EndAlertsEdit()
        {
            _isEditInProgress = false;
            ApplyEnabled = false;    
        }

        /// <summary>
        /// Allows editing an individual note when the ellipsis button is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _alertsGrid_EllipsisButtonClick(object sender, iGEllipsisButtonClickEventArgs e)
        {
            if (_alertsGrid.Cols[e.ColIndex].Key.Equals(ALERT_NOTES_COL_NAME))
            {
                //Place the widget in edit mode, if it's not in edit mode already
                if (!_isEditInProgress)
                {
                    BeginAlertsEdit(false);
                }

                //Display a notes editor
                AlarmsNotesEditor ane = new AlarmsNotesEditor(this.ConnectionDefn, _alertsGrid.Rows[e.RowIndex].Cells[ALERT_NOTES_COL_NAME].Value as string);
                if (ane.ShowDialog() == DialogResult.OK)
                {
                    string updatedNotes = ane.AlarmNotes.Trim();
                    string oldNotes = _alertsGrid.Rows[e.RowIndex].Cells[ALERT_NOTES_COL_NAME].Value as string;

                    //If the current and new notes are the same, nothing's changed
                    if (oldNotes != null && oldNotes.Trim().Equals(updatedNotes))
                    {
                        //If there are no other modified rows, end the edit mode.
                        if (_modifiedAlerts.Count < 1)
                        {
                            EndAlertsEdit();
                        }
                    }
                    else
                    {
                        //Update the alert with the new note and mark the row as modified.                       
                        _oldNotesCellValue = oldNotes;
                        _oldStatusCellValue = (string)_alertsGrid.Rows[e.RowIndex].Cells[ALERT_STATUS_COL_NAME].Value;
                        _alertsGrid.Rows[e.RowIndex].Cells[ALERT_NOTES_COL_NAME].Value = updatedNotes;
                        _alertsGrid.Rows[e.RowIndex].BackColor = System.Drawing.Color.YellowGreen;
                        CompleteEdit(e.ColIndex, e.RowIndex);
                        
                    }
                }
            }
        }

        private AlertsPrimaryKey CreateAlertsPrimaryKey(int RowIndex, bool storeOriginalValues)
        {
            DateTime createTime = (DateTime)_alertsGrid.Rows[RowIndex].Cells[ALERT_CREATE_TS_LCT_COL_NAME].Value;
            long creatorHostId = (long)_alertsGrid.Rows[RowIndex].Cells[ALERT_CREATOR_HOST_ID_COL_NAME].Value;
            int creatorProcessId = (int)_alertsGrid.Rows[RowIndex].Cells[ALERT_CREATOR_PROCESS_ID_COL_NAME].Value;

            AlertsPrimaryKey aKey = new AlertsPrimaryKey(createTime, creatorHostId, creatorProcessId);

            if(storeOriginalValues)
            {
                aKey.OriginalNotes = _alertsGrid.Rows[RowIndex].Cells[ALERT_NOTES_COL_NAME].Value.ToString();
                aKey.OriginalStatus = _alertsGrid.Rows[RowIndex].Cells[ALERT_STATUS_COL_NAME].Value.ToString();
            }           

            return aKey;
        }        

        /// <summary>
        /// Store the current value of the cell before the edit is committed
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _alertsGrid_BeforeCommitEdit(object sender, iGBeforeCommitEditEventArgs e)
        {

        }

        /// <summary>
        /// After the edit is committed, check the new value against the old value
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void _alertsGrid_AfterCommitEdit(object sender, iGAfterCommitEditEventArgs e)
        {
            CompleteEdit(e.ColIndex, e.RowIndex);
        }

        void CompleteEdit(int colIndex, int rowIndex)
        {
            if (colIndex == _alertsGrid.Cols[ALERT_NOTES_COL_NAME].Index || colIndex == _alertsGrid.Cols[ALERT_STATUS_COL_NAME].Index)
            {
                if (object.Equals(_oldStatusCellValue, _alertsGrid.Rows[rowIndex].Cells[ALERT_STATUS_COL_NAME].Value as string)
                    && object.Equals(_oldNotesCellValue, _alertsGrid.Rows[rowIndex].Cells[ALERT_NOTES_COL_NAME].Value as string))
                {                    
                    //Neither status nor notes column is changed, do nothing;
                    //TBD set old values to null?                    
                }
                else 
                {
                    AlertsPrimaryKey updateParams;
                    if (!_modifiedAlerts.ContainsKey(rowIndex))
                    {
                        updateParams = CreateAlertsPrimaryKey(rowIndex, false);
                        updateParams.OriginalNotes = _oldNotesCellValue == null ? null : _oldNotesCellValue.ToString();
                        updateParams.OriginalStatus = _oldStatusCellValue == null ? null : _oldStatusCellValue.ToString();

                        _modifiedAlerts.Add(rowIndex, updateParams);
                        
                        _alertsGrid.Rows[rowIndex].BackColor = System.Drawing.Color.YellowGreen;

                        if (!_isEditInProgress)
                        {
                            BeginAlertsEdit(false);
                        }
                    }
                    else
                    {
                        updateParams = _modifiedAlerts[rowIndex];
                        if (object.Equals(updateParams.OriginalNotes, _alertsGrid.Rows[rowIndex].Cells[ALERT_NOTES_COL_NAME].Value as string)
                            && object.Equals(updateParams.OriginalStatus, _alertsGrid.Rows[rowIndex].Cells[ALERT_STATUS_COL_NAME].Value as string))
                        {
                            _modifiedAlerts.Remove(rowIndex);
                            _alertsGrid.Rows[rowIndex].BackColor = System.Drawing.Color.White;
                        }

                        if (_modifiedAlerts.Count == 0) 
                        {
                            EndAlertsEdit();
                        }

                    }

                }
                
            }

        }

        /// <summary>
        /// Handler to save persistence
        /// </summary>
        /// <param name="aDictionary"></param>
        /// <param name="aPersistenceOperation"></param>
        void Persistence_PersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
        {
            //When framework notifies the save persistence event, do your part
            if (aPersistenceOperation == Persistence.PersistenceOperation.Save)
            {
                SavePersistence();
            }
        }

        /// <summary>
        /// Persist the alert options
        /// </summary>
        void SavePersistence()
        {
            try
            {
                Trafodion.Manager.Framework.Persistence.Put(AlertsPersistenceKey, _alertOptions);
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Display the options dialog
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void optionsButton_Click(object sender, System.EventArgs e)
        {
            AlertOptionsModel originalAlertOption = _alertOptions.Clone();
            AlertsConfigDialog aop = new AlertsConfigDialog(_alertOptions, initializationState, this.ConnectionDefn);
            aop.StartPosition = FormStartPosition.CenterParent;
            if (aop.ShowDialog(this.ParentForm) == DialogResult.OK)
            {
                //Get the modified options from the option dialog.
                _alertOptions = aop.AlertOptions;

                //Save the persistence                
                try
                {
                    Trafodion.Manager.Framework.Persistence.Put(AlertsPersistenceKey, _alertOptions);
                }
                catch (Exception)
                {
                }

                //Apply Filter
                if (initializationState == 1 || initializationState == 2 || initializationState == 3 ||
                    (_alertOptions.TimeRange==TimeRangesHandler.Range.LiveFeedOnly&&originalAlertOption.TimeRange==TimeRangesHandler.Range.LiveFeedOnly))
                {
                    FilterGridWithFetchOption(_alertsGrid, _alertOptions.FetchOpenAlertsOnly);
                }
                else 
                {
                    _alertsGrid.Rows.Clear();
                    _isEditInProgress = false;
                    ApplyEnabled = false;

                    if (_alertOptions.TimeRange == TimeRangesHandler.Range.LiveFeedOnly)
                    {
                        // live feed only selected, update is not allowed
                        _bulkUpdateMenuItem.Enabled = false;
                        _viewSymptomEventsMenuItem.Enabled = false;
                    }
                    else
                    {
                        // make sure the two menu items are enabled in case they were disabled previously
                        _bulkUpdateMenuItem.Enabled = true;
                        _viewSymptomEventsMenuItem.Enabled = true;
                    }

                    //Clear Grid Header
                    string gridHeaderText = string.Format(Properties.Resources.AlertsCountAfterFetch, 0, "");
                    this._alertsGrid.UpdateCountControlText(gridHeaderText);
                }                

                
                if (initializationState == 4)
                {
                    ManageDataProviders(_alertOptions);
                }
                
            }
            else if (aop.DialogResult == DialogResult.Cancel) 
            {
                _alertOptions = aop.AlertOptions;
            }
        }

        private void FilterGridWithFetchOption(TrafodionIGrid theGrid, bool fetchOpenAlertsOnly) 
        {
            if (fetchOpenAlertsOnly)
            {
                List<int> toRemoved = new List<int>();
                foreach (iGRow aRow in theGrid.Rows)
                {
                    string status = aRow.Cells[ALERT_STATUS_COL_NAME].Value.ToString();
                    if (status.ToUpper().Equals("OPEN") || status.ToUpper().Equals("ACKNOWLEDGED"))
                    {
                        //OpenAlerts will be kept.
                    }
                    else
                    {
                        toRemoved.Add(aRow.Index);
                    }
                }

                toRemoved.Sort();
                for (int i = 0; i < toRemoved.Count; i++)
                {
                    _alertsGrid.Rows.RemoveAt(toRemoved[i] - i);
                }
            }                      
        }




        private void ManageDataProviders(AlertOptionsModel theAlertOption)
        {
            if (theAlertOption.TimeRange == TimeRangesHandler.Range.LiveFeedOnly)
            {
                _theLiveAlertDataProvider.Stop();
                _theLiveAlertDataProvider.Start();
                turnOnOffAlertGridReadOnly(true);

                if (_theRepoAlertDataProvider != null)
                {
                    _theRepoAlertDataProvider.Stop();
                }
                if (_refreshButton != null) 
                {
                    _refreshButton.Visible = false;
                }                
            }
            else
            {
                SetSqlQueryText();
                if (_refreshButton != null)
                {
                    _refreshButton.Visible = true;
                }
                _theRepoAlertDataProvider.Stop();
                _theRepoAlertDataProvider.Start();

                turnOnOffAlertGridReadOnly(false);

                _theLiveAlertDataProvider.Stop();
                if (theAlertOption.IncludeLiveFeed)
                {
                    _theLiveAlertDataProvider.Start();
                }
                        
            }

        }

        private void turnOnOffAlertGridReadOnly(bool readOnly) 
        {
            if (readOnly)
            {                
                if(_alertsGrid.Cols.KeyExists(ALERT_STATUS_COL_NAME))
                {
                    _alertsGrid.Cols[ALERT_STATUS_COL_NAME].CellStyle.ReadOnly = iGBool.True;
                    _alertsGrid.Cols[ALERT_STATUS_COL_NAME].CellStyle.TypeFlags = ((iGCellTypeFlags)((iGCellTypeFlags.HideComboButton | iGCellTypeFlags.NoTextEdit)));
                }

                if (_alertsGrid.Cols.KeyExists(ALERT_NOTES_COL_NAME))
                {
                    _alertsGrid.Cols[ALERT_NOTES_COL_NAME].CellStyle.ReadOnly = iGBool.True;
                    _alertsGrid.Cols[ALERT_NOTES_COL_NAME].CellStyle.TypeFlags = ((iGCellTypeFlags)((iGCellTypeFlags.HideComboButton | iGCellTypeFlags.NoTextEdit)));
                }
            }
            else 
            {
                if (_alertsGrid.Cols.KeyExists(ALERT_STATUS_COL_NAME)) 
                {
                    _alertsGrid.Cols[ALERT_STATUS_COL_NAME].CellStyle.ReadOnly = iGBool.False;
                    _alertsGrid.Cols[ALERT_STATUS_COL_NAME].CellStyle.TypeFlags = iGCellTypeFlags.NotSet;
                }

                if (_alertsGrid.Cols.KeyExists(ALERT_NOTES_COL_NAME)) 
                {
                    _alertsGrid.Cols[ALERT_NOTES_COL_NAME].CellStyle.ReadOnly = iGBool.False;
                    _alertsGrid.Cols[ALERT_NOTES_COL_NAME].CellStyle.TypeFlags = iGCellTypeFlags.HasEllipsisButton;
                }                
            }
        
        }



        /// <summary>
        /// Starts the data provider
        /// </summary>
        void Start()
        {
            if (_connectionDefinition != null && _connectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                if ((_alertsWidget.DataProvider != null) && (_alertsWidget.DataProvider.DataProviderConfig != null))
                {
                    _alertsWidget.DataProvider.Stop();
                    _alertsWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
                    _alertsWidget.DataProvider.Start();
                }
            }
        }

        /// <summary>
        /// Finds the current offset of the LCT and the UTC timezones
        /// </summary>
        /// <returns></returns>
        TimeSpan GetLCTTimeZoneOffset()
        {
            DateTime maxDateTimeLCT = new DateTime(1990, 1, 1);
            DateTime maxDateTimeUCT = new DateTime(1990, 1, 1);
            DateTime dateTimeUCT = new DateTime(1990, 1, 1);

            //Run through the alerts and find the maximum create UTC date
            //Use this row to compute the offset between the server side LCT and UTC timezones.
            foreach (iGRow row in _alertsGrid.Rows)
            {
                dateTimeUCT = (DateTime)row.Cells[ALERT_CREATE_TS_UTC_COL_NAME].Value;

                if (maxDateTimeUCT == null || maxDateTimeUCT < dateTimeUCT)
                {
                    maxDateTimeUCT = dateTimeUCT;
                    maxDateTimeLCT = (DateTime)row.Cells[ALERT_CREATE_TS_LCT_COL_NAME].Value;
                }
            }
            if (maxDateTimeUCT == null || maxDateTimeLCT == null)
                return TimeSpan.Zero;

            TimeSpan tsp = maxDateTimeLCT - maxDateTimeUCT;
            return tsp;
        }

        #endregion private methods

        #region Live Feed dataprovider methods

        private void LiveFeed_BrokerConfiguration_OnBrokerChanged(object sender, LiveFeedBrokerChangedEventArgs eArgs)
        {
            if (eArgs.ChangedReason == LiveFeedBrokerChangedEventArgs.Reason.SessionRetryTimer)
            {
                _theLiveAlertDataProvider.ConfiguredRefreshRate = _theLiveAlertDataProvider.LiveFeedConnection.BrokerConfiguration.SessionRetryTimer;
            }
            else
            {
                _theLiveAlertDataProvider.Stop();
                //_theLiveAlertDataProvider.OnNewDataArrived -= LiveAlert_InvokeHandleNewDataArrived;
                _theLiveAlertDataProvider.Start();
                //_theLiveAlertDataProvider.OnNewDataArrived += LiveAlert_InvokeHandleNewDataArrived;
            }
        }

        private void LiveAlert_InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                    Invoke(new LiveFeed_HandleEvent(LiveAlertDataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
            }
            catch (Exception ex)
            {
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }


        private void LiveAlertDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            //Coming new alert;      
            DataTable tempNewComingAlertDataTable = _theLiveAlertDataProvider.GetDataTable();
            if (tempNewComingAlertDataTable == null || tempNewComingAlertDataTable.Rows.Count == 0)
                return;

            //Check the date before processing the raw data
            bool passedEndDate = IsLiveDataProviderMeetsEndDate(tempNewComingAlertDataTable);

            if (_alertsGrid.Cols.Count == 0)
            {
                tempNewComingAlertDataTable = ApplyFetchOption(tempNewComingAlertDataTable);
                tempNewComingAlertDataTable = GetTopNAlertsDataTable(tempNewComingAlertDataTable, GetAlertsWidgetMaxRowCount());
                alertDisplayHandler.PopulateAlert(_universalWidgetConfig, tempNewComingAlertDataTable, _alertsGrid, true);
            }
            else
            {
                _alertsGrid.BeginUpdate();
                AlertGridMergeLive(tempNewComingAlertDataTable);
                alertDisplayHandler.CalculateAlertCount(_alertsGrid);
                _alertsGrid.EndUpdate();
            }

            //Stop Live Data Provider
            if (passedEndDate) 
            {
                _theLiveAlertDataProvider.Stop(); ;
            }
        }

        private bool IsLiveDataProviderMeetsEndDate(DataTable liveAlertDataTable) 
        {
            bool returnValue = false;
            if(_alertOptions.TimeRange==TimeRangesHandler.Range.CustomRange)
            {
                DataRow[] rows = liveAlertDataTable.Select(ALERT_GEN_TS_LCT_COL_NAME + "=" + "Max(" + ALERT_GEN_TS_LCT_COL_NAME + ")");
                if ((DateTime)rows[0][ALERT_GEN_TS_LCT_COL_NAME] > _alertOptions.TheEndTime) 
                {
                    returnValue = true;
                }
            }
            return returnValue;
        }



        private DataTable ApplyFetchOption(DataTable aDataTable)
        {
            if (_alertOptions.FetchOpenAlertsOnly)
            {                
                return FilterDataTable(aDataTable, "STATUS IN ('OPEN', 'ACKNOWLEDGED')");
            }
            else
            {
                return aDataTable;
            }
        }

        private DataTable FilterDataTable(DataTable sourceDataTable, string conditions)
        {
            DataRow[] selectedRows = sourceDataTable.Select(conditions);
            DataTable selectedTable = sourceDataTable.Clone();
            foreach (DataRow dr in selectedRows)
            {
                selectedTable.ImportRow(dr);
            }
            return selectedTable;
        }
        
        private void AlertGridMergeLive(DataTable newComingDataTable)
        {
            List<DataRow> liveToRemoved = new List<DataRow>();

            List<int> alertGridToRemoved = new List<int>();

            foreach (DataRow drNew in newComingDataTable.Rows)
            {
                foreach (iGRow alertGridRow in _alertsGrid.Rows)
                {
                    if (drNew[ALERT_CREATE_TS_LCT_COL_NAME].Equals(alertGridRow.Cells[ALERT_CREATE_TS_LCT_COL_NAME].Value) &&
                            drNew[ALERT_CREATOR_HOST_ID_COL_NAME].Equals(alertGridRow.Cells[ALERT_CREATOR_HOST_ID_COL_NAME].Value) &&
                            drNew[ALERT_CREATOR_PROCESS_ID_COL_NAME].Equals(alertGridRow.Cells[ALERT_CREATOR_PROCESS_ID_COL_NAME].Value))
                    {
                        DateTime liveTime = (DateTime)drNew[ALERT_GEN_TS_LCT_COL_NAME];
                        DateTime alertTime = (DateTime)alertGridRow.Cells[ALERT_GEN_TS_LCT_COL_NAME].Value;

                        if (IsReopenAlert(drNew, alertGridRow))
                        {
                            drNew[ALERT_NOTES_COL_NAME] = alertGridRow.Cells[ALERT_NOTES_COL_NAME].Value;
                        }
                        if (liveTime > alertTime)
                        {
                            alertGridToRemoved.Add(alertGridRow.Index);
                            //Now we check the alert options Open or Close
                            //If the new coming alert doesn't apply alert options, remove it;
                            if (!ApplyFetchOption(drNew))
                            {
                                liveToRemoved.Add(drNew);
                            }
                        }
                        else
                        {
                            //Something wrong of data quality with New Coming Data;
                            liveToRemoved.Add(drNew);
                        }
                        break;
                    }
                }
            }


            alertGridToRemoved.Sort();
            for (int i = 0; i < alertGridToRemoved.Count; i++)
            {
                _alertsGrid.Rows.RemoveAt(alertGridToRemoved[i] - i);
            }            

            foreach (var dr in liveToRemoved)
            {
                newComingDataTable.Rows.Remove(dr);
            }

            List<iGRow> addedRows = new List<iGRow>();
            foreach (DataRow dr in newComingDataTable.Rows)
            {
                if (ApplyFetchOption(dr))
                {
                    iGRow row = _alertsGrid.AddRow(dr.ItemArray);
                    addedRows.Add(row);
                }
            }

            //If connected, add cell style to added rows 
            if (initializationState == 4)
            {
                StyleIGRows(addedRows);
            }

            //Sort and Top N
            GetTopNRowsofIGrid();
        }
        

        private void StyleIGRows(List<iGRow> iGRows)
        {
            foreach (var iGRow in iGRows)
            {
                string alertStatus = iGRow.Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].Value.ToString().Trim();
                if (string.IsNullOrEmpty(alertStatus) || alertStatus.Trim().Equals(ALERT_STATUS_AUTO_CLOSED) ||
                           alertStatus.Trim().Equals(ALERT_STATUS_OP_CLOSED)
                           )
                {
                    iGRow.Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].ReadOnly = iGBool.True;
                    iGRow.Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].Style.TypeFlags =
                                            ((iGCellTypeFlags)((iGCellTypeFlags.HideComboButton | iGCellTypeFlags.NoTextEdit)));
                }

            }

        }

        private void GetTopNRowsofIGrid()
        {
            int maxRowCount = GetAlertsWidgetMaxRowCount();
            if (_alertsGrid.Rows.Count > maxRowCount)
            {
                List<ColumnSortObject> csos = _universalWidgetConfig.DataProviderConfig.ColumnSortObjects;

                if (_alertsGrid.Cols.KeyExists(AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME))
                {
                    _alertsGrid.SortObject.Clear();
                    ColumnSortObject cso = new ColumnSortObject(_alertsGrid.Cols[AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME].Index, 0, (int)iGSortOrder.Descending);
                    _alertsGrid.SortObject.Add(cso.ColIndex, (iGSortOrder)cso.SortOrder);
                    _alertsGrid.Sort();

                    int countToReomved = _alertsGrid.Rows.Count - maxRowCount;
                    _alertsGrid.Rows.RemoveRange(maxRowCount, countToReomved);

                    //If the original sort is not ALERT_LAST_UPDATE_TS_LCT_COL_NAME, restore it and sort
                    if (csos != null && csos[0].ColIndex != cso.ColIndex)
                    {
                        _alertsGrid.SortObject.Clear();
                        foreach (ColumnSortObject csobject in csos)
                        {
                            _alertsGrid.SortObject.Add(csobject.ColIndex, (iGSortOrder)csobject.SortOrder);
                        }
                        _alertsGrid.Sort();
                    }
                }
            }
            else
            {
                _alertsGrid.Sort();
            }
        }


        private bool ApplyFetchOption(DataRow newRow)
        {
            if (_alertOptions.FetchOpenAlertsOnly)
            {
                string status = newRow[ALERT_STATUS_COL_NAME].ToString();
                if (status.ToUpper().Equals("OPEN") || status.ToUpper().Equals("ACKNOWLEDGED"))
                    return true;
                else return false;
            }
            else
            {
                return true;
            }
        }


        private DataTable LiveMergeRepo(DataTable liveDT, DataTable repoDT)
        {
            DataTable result = repoDT.Clone();
            if (repoDT.Rows.Count == 0)
            {
                result = liveDT;
            }
            else if (liveDT.Rows.Count==0)
            {
                result = repoDT;
            }
            else
            {
                List<DataRow> liveToRemoved = new List<DataRow>();
                List<DataRow> repoToRemoved = new List<DataRow>();
                foreach (DataRow drLive in liveDT.Rows)
                {
                    foreach (DataRow drRepo in repoDT.Rows)
                    {
                        if (drLive[ALERT_CREATE_TS_LCT_COL_NAME].Equals(drRepo[ALERT_CREATE_TS_LCT_COL_NAME]) &&
                            drLive[ALERT_CREATOR_HOST_ID_COL_NAME].Equals(drRepo[ALERT_CREATOR_HOST_ID_COL_NAME]) &&
                            drLive[ALERT_CREATOR_PROCESS_ID_COL_NAME].Equals(drRepo[ALERT_CREATOR_PROCESS_ID_COL_NAME]))
                        {
                            DateTime liveTime = (DateTime)drLive[ALERT_GEN_TS_LCT_COL_NAME];
                            DateTime repoTime = (DateTime)drRepo[ALERT_GEN_TS_LCT_COL_NAME];

                            if (IsReopenAlert(drLive, drRepo))
                            {
                                drLive[ALERT_NOTES_COL_NAME] = drRepo[ALERT_NOTES_COL_NAME];
                            }
                            if (liveTime > repoTime)
                            {
                                repoToRemoved.Add(drRepo);
                            }
                            else
                            {
                                liveToRemoved.Add(drLive);
                            }
                        }
                    }
                }
                foreach (var dr in repoToRemoved)
                {
                    repoDT.Rows.Remove(dr);
                }
                foreach (var dr in liveToRemoved)
                {
                    liveDT.Rows.Remove(dr);
                }

                try
                {                   
                    repoDT.Merge(liveDT);
                    repoDT = GetTopNAlertsDataTable(repoDT, GetAlertsWidgetMaxRowCount());
                    result = repoDT;
                }
                catch (Exception ex)
                {
                }
            }
            return result;
        }


        private bool IsReopenAlert(DataRow drLive, iGRow drGrid)
        {
            string newAlertStatus = drLive[ALERT_STATUS_COL_NAME].ToString();
            string oldAlertStatus = drGrid.Cells[ALERT_STATUS_COL_NAME].Value.ToString();

            if (newAlertStatus.Contains("OPEN") && (oldAlertStatus.Contains("CLOSE")))
            {
                return true;
            }
            else
                return false;

        }


        private bool IsReopenAlert(DataRow drLive, DataRow drRepo)
        {
            string newAlertStatus = drLive[ALERT_STATUS_COL_NAME].ToString();
            string oldAlertStatus = drRepo[ALERT_STATUS_COL_NAME].ToString();

            if (newAlertStatus.Contains("OPEN") && (oldAlertStatus.Contains("CLOSE")))
            {
                return true;
            }
            else
                return false;

        }
              

        #endregion Live Data dataprovider methods



        #region IClone methods

        /// <summary>
        /// Clones the alerts user control into a new window
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            AlertsUserControl alertsControl = new AlertsUserControl(ConnectionDefn);
            return alertsControl;
        }

        /// <summary>
        /// Title for the cloned window
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return Properties.Resources.Alerts;
            }
        }

        #endregion IClone methods


        #region Custom Column Styles

        /// <summary>
        /// Creates the iGrid column pattern for the Alert type column
        /// Alert type displays an image
        /// </summary>
        /// <returns></returns>
        void CreateIGridColumnPatterns()
        {
            iGCellStyle statusStyle = new iGCellStyle();
            statusStyle.DropDownControl = AlertStatusDropDownList;
            statusStyle.Flags = iGCellFlags.DisplayText;
            statusStyle.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            statusStyle.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            statusStyle.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            statusStyle.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            statusStyle.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.NoTextEdit;

            _alertStatusColPattern = new iGColPattern();
            _alertStatusColPattern.CellStyle = statusStyle;
            _alertStatusColPattern.Text = "Status";
            _alertStatusColPattern.Key = AlertsUserControl.ALERT_STATUS_COL_NAME;
            _alertStatusColPattern.MinWidth = 60;
            _alertStatusColPattern.Width = 108;
            _alertStatusColPattern.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);

            iGCellStyle notesStyle = new iGCellStyle();
            notesStyle.Flags = iGCellFlags.DisplayText;
            notesStyle.EmptyStringAs = TenTec.Windows.iGridLib.iGEmptyStringAs.Null;
            notesStyle.Enabled = TenTec.Windows.iGridLib.iGBool.True;
            notesStyle.Selectable = TenTec.Windows.iGridLib.iGBool.True;
            notesStyle.Type = TenTec.Windows.iGridLib.iGCellType.Text;
            notesStyle.TypeFlags = TenTec.Windows.iGridLib.iGCellTypeFlags.HasEllipsisButton;
            notesStyle.ValueType = typeof(string);
            notesStyle.TextFormatFlags = iGStringFormatFlags.LineLimit;

            _alertNotesColPattern = new iGColPattern();
            _alertNotesColPattern.CellStyle = notesStyle;
            _alertNotesColPattern.Text = "Notes";
            _alertNotesColPattern.Key = AlertsUserControl.ALERT_NOTES_COL_NAME;
            _alertNotesColPattern.MinWidth = 60;
            _alertNotesColPattern.Width = 108;
            _alertNotesColPattern.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);
        }

        #endregion Custom Column Styles



    }

    /// <summary>
    /// Custom data display handler for the Alerts widget
    /// </summary>
    public class AlertsWidgetDataHandler2 : TabularDataDisplayHandler
    {
        #region private member variables

        private AlertsUserControl _systemAlertsUserControl;

        #endregion private member variables

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aSystemAlertsUserControl"></param>
        public AlertsWidgetDataHandler2(AlertsUserControl aSystemAlertsUserControl)
        {
            _systemAlertsUserControl = aSystemAlertsUserControl;
        }

        #region UniversalWidget DataDisplayHandler methods

        /// <summary>
        /// Populate the Alerts Grid
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        /// 
        public void PopulateAlert(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid, bool reloadData)
        {
            if (_systemAlertsUserControl.InitializationState != 4||(_systemAlertsUserControl.InitializationState==4&&_systemAlertsUserControl.AlertOptions.TimeRange==TimeRangesHandler.Range.LiveFeedOnly))
            {
                PopulateWhenNotConnected(aConfig, aDataTable, aDataGrid, reloadData);
            }
            else
            {
                PopulateWhenConnected(aConfig, aDataTable, aDataGrid, reloadData);
            }
        }


        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {

        }

        private void PopulateWhenNotConnected(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid, bool reloadData)
        {
            if (aDataTable != null)
            {
                _systemAlertsUserControl.ApplyEnabled = false;
                _systemAlertsUserControl.IsEditInProgress = false;
                aDataGrid.Clear();
                aDataGrid.BeginUpdate();
                if (reloadData)
                {
                    TrafodionIGridUtils.PopulateGrid(aConfig, aDataTable, aDataGrid);
                }
                aDataGrid.ReadOnly = true;


                foreach (iGCol column in aDataGrid.Cols)
                {
                    if (column.CellStyle.ValueType == typeof(System.DateTime))
                    {
                        //Display the alert create time in LCT using TrafodionManager standard datetime format
                        if (column.Key.Equals(AlertsUserControl.ALERT_CREATE_TS_LCT_COL_NAME) ||
                            column.Key.Equals(AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME) ||
                            column.Key.Equals(AlertsUserControl.ALERT_CLOSE_TS_LCT_COL_NAME))
                        {
                            column.CellStyle.FormatProvider = _systemAlertsUserControl.TheDateTimeFormatProvider;
                            column.CellStyle.FormatString = "{0}";
                        }
                        else
                        {
                            column.CellStyle.FormatProvider = _systemAlertsUserControl.TheDateTimeFormatProvider;
                            column.CellStyle.FormatString = "{0}";
                        }
                    }
                }
                aDataGrid.ResizeGridColumns(aDataTable, 7, 20);
                if (aConfig.DataProviderConfig.ColumnSortObjects != null && aConfig.DataProviderConfig.ColumnSortObjects.Count == 0)
                {
                    if (aDataGrid.Cols.KeyExists(AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME))
                    {
                        aConfig.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(aDataGrid.Cols[AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME].Index, 0, (int)iGSortOrder.Descending));
                    }
                }


                CalculateAlertCount(aDataGrid);


                TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);
                aDataGrid.EndUpdate();
            }
        }


        private void PopulateWhenConnected(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid, bool reloadData)
        {
            //Populate the grid with the data table passed
            //int countOfOpenEmergencies = 0;
            //int countOfOpenImmediates = 0;
            //int countOfOpenCriticals = 0;
            //int countOfOpenErrors = 0;
            //int totalEmergencies = 0;
            //int totalImmediates = 0;
            //int totalCriticals = 0;
            //int totalErrors = 0;
            if (aDataTable != null)
            {
                _systemAlertsUserControl.ApplyEnabled = false;
                _systemAlertsUserControl.IsEditInProgress = false;
                //Reset the list of modified rows, since we just refresh the grid
                _systemAlertsUserControl.ModifiedAlerts.Clear();

                aDataGrid.BeginUpdate();
                if (reloadData)
                {
                    aDataGrid.Clear();
                    TrafodionIGridUtils.PopulateGrid(aConfig, aDataTable, aDataGrid);
                }

                //aDataGrid.FillWithData(aDataTable);

                iGCol alertLevelColumn = null;
                iGCol severityColumn = null;

                //Set the Alert level icon, based on the alert severity
                if (aDataGrid.Cols.KeyExists(AlertsUserControl.ALERT_SEVERITY_NAME_COL_NAME))
                {
                    severityColumn = aDataGrid.Cols[AlertsUserControl.ALERT_SEVERITY_NAME_COL_NAME];
                }

                if (severityColumn != null)
                {
                    //Make the alert level column, the first column
                    aDataGrid.ReadOnly = false;

                    //Enable status and notes columns for editing
                    //Other columns remain readonly
                    foreach (iGCol column in aDataGrid.Cols)
                    {
                        if (column.Key.Equals(AlertsUserControl.ALERT_STATUS_COL_NAME) || column.Key.Equals(AlertsUserControl.ALERT_NOTES_COL_NAME))
                        {
                            column.CellStyle.ReadOnly = iGBool.False;
                            column.MinWidth = 60;
                        }
                        else
                        {
                            column.CellStyle.ReadOnly = iGBool.True;
                        }

                        if (column.CellStyle.ValueType == typeof(System.DateTime))
                        {
                            //Display the alert create time in LCT using TrafodionManager standard datetime format
                            if (column.Key.Equals(AlertsUserControl.ALERT_CREATE_TS_LCT_COL_NAME) ||
                                column.Key.Equals(AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME) ||
                                column.Key.Equals(AlertsUserControl.ALERT_CLOSE_TS_LCT_COL_NAME))
                            {
                                column.CellStyle.FormatProvider = _systemAlertsUserControl.TheDateTimeFormatProvider;
                                column.CellStyle.FormatString = "{0}";
                            }
                            else
                            {
                                column.CellStyle.FormatProvider = _systemAlertsUserControl.TheDateTimeFormatProvider;
                                column.CellStyle.FormatString = "{0}";
                            }
                        }
                    }
                    CalculateAlertCount(aDataGrid);
                    ApplyStatusStyle(aDataGrid);
                }

                aDataGrid.ResizeGridColumns(aDataTable, 7, 20);

                //If the user has update privileges, make the status column to be a drop down list 
                //and make the notes column be a editable column with ellipsis
                //else these 2 columns will be readonly text fields
                if (_systemAlertsUserControl.InitializationState == 4)
                {
                    if (aDataGrid.Cols.KeyExists(AlertsUserControl.ALERT_STATUS_COL_NAME))
                    {
                        aDataGrid.Cols[AlertsUserControl.ALERT_STATUS_COL_NAME].Pattern = _systemAlertsUserControl.AlertStatusColPattern;
                    }
                    if (aDataGrid.Cols.KeyExists(AlertsUserControl.ALERT_NOTES_COL_NAME))
                    {
                        aDataGrid.Cols[AlertsUserControl.ALERT_NOTES_COL_NAME].Pattern = _systemAlertsUserControl.AlertNotesColPattern;
                    }
                }
                if (aConfig.DataProviderConfig.ColumnSortObjects != null && aConfig.DataProviderConfig.ColumnSortObjects.Count == 0)
                {
                    if (aDataGrid.Cols.KeyExists(AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME))
                    {
                        aConfig.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(aDataGrid.Cols[AlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME].Index, 0, (int)iGSortOrder.Descending));
                    }
                }
                TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);
                aDataGrid.EndUpdate();
            }
        }


        public void CalculateAlertCount(Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            //Populate the grid with the data table passed
            int countOfOpenEmergencies = 0;
            int countOfOpenImmediates = 0;
            int countOfOpenCriticals = 0;
            int countOfOpenErrors = 0;
            int countOfOpenWarnings = 0;
            int totalEmergencies = 0;
            int totalImmediates = 0;
            int totalCriticals = 0;
            int totalErrors = 0;
            int totalWarnings = 0;

            //Now for each alert, set the appropriate alert level icon based on the alert severity mapping
            for (int i = 0; i < aDataGrid.Rows.Count; i++)
            {
                string alertStatus = aDataGrid.Rows[i].Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].Value.ToString().Trim();
                string severity = aDataGrid.Cells[i, AlertsUserControl.ALERT_SEVERITY_NAME_COL_NAME].Value.ToString().Trim();

                if (!string.IsNullOrEmpty(severity))
                {
                    if (!string.IsNullOrEmpty(alertStatus) &&
                        (alertStatus.Trim().Equals(AlertsUserControl.ALERT_STATUS_AUTO_CLOSED) ||
                            alertStatus.Trim().Equals(AlertsUserControl.ALERT_STATUS_OP_CLOSED) ||
                            alertStatus.Trim().Equals(AlertsUserControl.ALERT_STATUS_USER_CLOSED))
                        )
                    {
                        //Found a closed alert
                        if (severity.Equals(AlertOptionsModel.AlertType.CRITICAL.ToString(), StringComparison.InvariantCultureIgnoreCase))
                        {
                            totalCriticals++;
                        }
                        else
                            if (severity.Equals(AlertOptionsModel.AlertType.EMERGENCY.ToString(), StringComparison.InvariantCultureIgnoreCase))
                            {
                                totalEmergencies++;
                            }
                            else
                                if (severity.Equals(AlertOptionsModel.AlertType.IMMEDIATE.ToString(), StringComparison.InvariantCultureIgnoreCase))
                                {
                                    totalImmediates++;
                                }
                                else
                                    if (severity.Equals(AlertOptionsModel.AlertType.ERROR.ToString(), StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        totalErrors++;
                                    }
                                    else if (severity.Equals(AlertOptionsModel.AlertType.WARNING.ToString(), StringComparison.InvariantCultureIgnoreCase)) 
                                    {
                                        totalWarnings++;
                                    }
                    }
                    else
                    {
                        if (severity.Equals(AlertOptionsModel.AlertType.CRITICAL.ToString(), StringComparison.InvariantCultureIgnoreCase))
                        {
                            countOfOpenCriticals++;
                            totalCriticals++;
                        }
                        else
                            if (severity.Equals(AlertOptionsModel.AlertType.EMERGENCY.ToString(), StringComparison.InvariantCultureIgnoreCase))
                            {
                                countOfOpenEmergencies++;
                                totalEmergencies++;
                            }
                            else
                                if (severity.Equals(AlertOptionsModel.AlertType.IMMEDIATE.ToString(), StringComparison.InvariantCultureIgnoreCase))
                                {
                                    countOfOpenImmediates++;
                                    totalImmediates++;
                                }
                                else
                                    if (severity.Equals(AlertOptionsModel.AlertType.ERROR.ToString(), StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        countOfOpenErrors++;
                                        totalErrors++;
                                    }
                                    else if (severity.Equals(AlertOptionsModel.AlertType.WARNING.ToString(), StringComparison.InvariantCultureIgnoreCase))
                                    {
                                        countOfOpenWarnings++;
                                        totalWarnings++;
                                    }
                    }
                }


            }

            int totalOpenAlerts = countOfOpenEmergencies + countOfOpenCriticals + countOfOpenImmediates + countOfOpenErrors + countOfOpenWarnings;
            int totalAlerts = totalEmergencies + totalCriticals + totalImmediates + totalErrors + totalWarnings;
            List<string> headerTexts = new List<string>();
            if (countOfOpenEmergencies > 0)
                headerTexts.Add(string.Format("{0} Emergency", countOfOpenEmergencies));
            if (countOfOpenCriticals > 0)
                headerTexts.Add(string.Format("{0} Critical", countOfOpenCriticals));
            if (countOfOpenImmediates > 0)
                headerTexts.Add(string.Format("{0} Immediate", countOfOpenImmediates));
            if (countOfOpenErrors > 0)
                headerTexts.Add(string.Format("{0} Error", countOfOpenErrors));
            if (countOfOpenWarnings > 0)
                headerTexts.Add(string.Format("{0} Warning", countOfOpenWarnings));

            string gridHeaderText = string.Format(Properties.Resources.AlertsCountAfterFetch,
                totalOpenAlerts, totalOpenAlerts > 0 ? "(" + string.Join(", ", headerTexts.ToArray()) + ")" : "");

            aDataGrid.UpdateCountControlText(gridHeaderText);
            TrafodionIGridUtils.updateIGridColumnHeadings(aDataGrid);
        }

        private void ApplyStatusStyle(TrafodionIGrid aDataGrid) 
        {
            for (int i = 0; i < aDataGrid.Rows.Count; i++)
            {
                string alertStatus = aDataGrid.Rows[i].Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].Value.ToString().Trim();
                string severity = aDataGrid.Cells[i, AlertsUserControl.ALERT_SEVERITY_NAME_COL_NAME].Value.ToString().Trim();

                if (string.IsNullOrEmpty(alertStatus) || alertStatus.Trim().Equals(AlertsUserControl.ALERT_STATUS_AUTO_CLOSED) ||
                            alertStatus.Trim().Equals(AlertsUserControl.ALERT_STATUS_OP_CLOSED)
                            )
                {
                    aDataGrid.Rows[i].Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].ReadOnly = iGBool.True;
                    aDataGrid.Rows[i].Cells[AlertsUserControl.ALERT_STATUS_COL_NAME].Style.TypeFlags =
                                            ((iGCellTypeFlags)((iGCellTypeFlags.HideComboButton | iGCellTypeFlags.NoTextEdit)));                    
                }
            }
        }

        #endregion UniversalWidget DataDisplayHandler methods
    }

}
