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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class RepoSymptomDetails : UserControl
    {
        #region Fields
        static readonly string RepoSymptomDetailsPersistenceKey1 = "RepoSymptomDetailsPersistence1";
        static readonly string RepoSymptomDetailsPersistenceKey2 = "RepoSymptomDetailsPersistence2";

        UniversalWidgetConfig _config1;
        UniversalWidgetConfig _config2;
        GenericUniversalWidget _symptomEventsWidget;
        GenericUniversalWidget _symptomHealthWidget;
        TrafodionIGrid _symptomEventsGrid;
        TrafodionIGrid _symptomHealthGrid;

        DatabaseDataProvider _symptomEventsDataProvider;
        DatabaseDataProvider _symptomHealthDataProvider;

        ToolStripButton _refreshButton;
        ToolStripButton _stopButton;

        public delegate void HandleEvent(Object obj, EventArgs e);

        ConnectionDefinition _connectionDefinition;
        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();
        string _createTime;
        string _creatorHostId;
        string _creatorProcessId;

        private TrafodionIGridToolStripMenuItem _viewEventDrillDownMenuItem;
        private TrafodionIGridToolStripMenuItem _viewHealthDrillDownMenuItem;

        #region Alerts Columns

        public static readonly string SYMPTOM_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";
        public static readonly string SYMPTOM_COMPONENT_COL_NAME = "COMPONENT_NAME";
        public static readonly string SYMPTOM_EVENT_ID_COL_NAME = "EVENT_ID";
        public static readonly string SYMPTOM_SEVERITY_NAME_COL_NAME = "SEVERITY_NAME";
        public static readonly string SYMPTOM_PROCESS_NAME_COL_NAME = "PROCESS_NAME";
        public static readonly string SYMPTOM_TEXT_COL_NAME = "TEXT";
        public static readonly string SYMPTOM_PROCESS_ID_COL_NAME = "PROCESS_ID";
        public static readonly string SYMPTOM_THREAD_ID_COL_NAME = "THREAD_ID";
        public static readonly string SYMPTOM_NODE_ID_COL_NAME = "NODE_ID";
        public static readonly string SYMPTOM_PNID_ID_COL_NAME = "PNID_ID";
        public static readonly string SYMPTOM_HOST_ID_COL_NAME = "HOST_ID";
        public static readonly string SYMPTOM_IP_ADDRESS_ID_COL_NAME = "IP_ADDRESS_ID";

        #endregion Alerts Columns

        #region Event Symptom Columns
        //internal static readonly string EVENT_INSTANCE_ID_COL_NAME = "INSTANCE_ID";
        //internal static readonly string EVENT_TENANT_ID_COL_NAME = "TENANT_ID";
        //internal static readonly string EVENT_CALENDAR_DATE_LCT_COL_NAME = "CALENDAR_DATE_LCT";
        //internal static readonly string EVENT_CALENDAR_DATE_UTC_COL_NAME = "CALENDAR_DATE_UTC";
        internal static readonly string EVENT_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";
        //internal static readonly string EVENT_GEN_TS_UTC_COL_NAME = "GEN_TS_UTC";
        //internal static readonly string EVENT_UPSERT_TS_LCT_COL_NAME = "UPSERT_TS_LCT";
        //internal static readonly string EVENT_UPSERT_TS_UTC_COL_NAME = "UPSERT_TS_UTC";
        //internal static readonly string EVENT_COMPONENT_ID_COL_NAME = "COMPONENT_ID";
        internal static readonly string EVENT_COMPONENT_NAME_COL_NAME = "COMPONENT_NAME";
        internal static readonly string EVENT_PROCESS_ID_COL_NAME = "PROCESS_ID";
        internal static readonly string EVENT_THREAD_ID_COL_NAME = "THREAD_ID";
        internal static readonly string EVENT_NODE_ID_COL_NAME = "NODE_ID";
        internal static readonly string EVENT_PNID_ID_COL_NAME = "PNID_ID";
        internal static readonly string EVENT_HOST_ID_COL_NAME = "HOST_ID";
        internal static readonly string EVENT_IP_ADDRESS_ID_COL_NAME = "IP_ADDRESS_ID";
        //internal static readonly string EVENT_SEQUENCE_NUMBER_COL_NAME = "SEQUENCE_NUMBER";
        internal static readonly string EVENT_PROCESS_NAME_COL_NAME = "PROCESS_NAME";
        internal static readonly string EVENT_EVENT_ID_COL_NAME = "EVENT_ID";
        //internal static readonly string EVENT_SEVERITY_COL_NAME = "SEVERITY";
        internal static readonly string EVENT_SEVERITY_NAME_COL_NAME = "SEVERITY_NAME";
        //internal static readonly string EVENT_ROLE_NAME_COL_NAME = "ROLE_NAME";
        internal static readonly string EVENT_TEXT_COL_NAME = "TEXT";
        internal static readonly string EVENT_TOKENIZED_EVENT_TABLE_COL_NAME = "TOKENIZED_EVENT_TABLE";
        //internal static readonly string EVENT_PROBLEM_CREATE_TS_LCT_COL_NAME = "PROBLEM_CREATE_TS_LCT";
        //internal static readonly string EVENT_PROBLEM_CREATOR_HOST_ID_COL_NAME = "PROBLEM_CREATOR_HOST_ID";
        //internal static readonly string EVENT_PROBLEM_CREATOR_PROCESS_ID_COL_NAME = "PROBLEM_CREATOR_PROCESS_ID";

        #endregion Events Symptom Columns
        internal static readonly string HEALTH_GEN_TS_LCT_COL_NAME = "GEN_TS_LCT";
        internal static readonly string HEALTH_COMPONENT_NAME_COL_NAME = "COMPONENT_NAME";
        internal static readonly string HEALTH_PUBLICATION_TYPE_COL_NAME = "PUBLICATION_TYPE";
        internal static readonly string HEALTH_CHECK_INTERVAL_SEC_COL_NAME = "CHECK_INTERVAL_SEC";
        internal static readonly string HEALTH_LOGICAL_OBJECT_TYPE_NAME_COL_NAME = "LOGICAL_OBJECT_TYPE_NAME";
        internal static readonly string HEALTH_LOGICAL_OBJECT_NAME_COL_NAME = "LOGICAL_OBJECT_NAME";
        internal static readonly string HEALTH_LOGICAL_OBJECT_PATH_COL_NAME = "LOGICAL_OBJECT_PATH";
        internal static readonly string HEALTH_LOGICAL_OBJECT_QUAL_1_COL_NAME = "LOGICAL_OBJECT_QUAL_1";
        internal static readonly string HEALTH_LOGICAL_OBJECT_QUAL_2_COL_NAME = "LOGICAL_OBJECT_QUAL_2";
        internal static readonly string HEALTH_CURRENT_HEALTH_NAME_COL_NAME = "CURRENT_HEALTH_NAME";
        internal static readonly string HEALTH_PREVIOUS_HEALTH_NAME_COL_NAME = "PREVIOUS_HEALTH_NAME";
        internal static readonly string HEALTH_HEALTH_CHANGE_TS_LCT_COL_NAME = "HEALTH_CHANGE_TS_LCT";

        public const string EVENT_VIEW = "PROBLEM_INSTANCE_EVENTMANYTOMANYBRIEF_1";
        public const string EVENT_VIEW_DEPRECATED = "PROBLEM_INSTANCE_SYMPTOMEVENT_2";

        public const string HEALTH_VIEW = "PROBLEM_INSTANCE_HEALTHMANYTOMANYBRIEF_1";
        public const string HEALTH_VIEW_DEPRECATED = "PROBLEM_INSTANCE_HEALTHSYMPTOMHEALTH_2";    

        #region  Health/State Symptom Columns
        internal static readonly string HEALTH_BASE_TABLE_NAME_COL_NAME = "BASE_TABLE_NAME";


        #endregion Health/State Symptom Columns

        #endregion Fields

        #region Properties

        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }

        private string EventView
        {
            get
            {
                return this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140
                    ? EVENT_VIEW : EVENT_VIEW_DEPRECATED;
            }
        }

        private string HealthView
        {
            get
            {
                return this._connectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140
                    ? HEALTH_VIEW : HEALTH_VIEW_DEPRECATED;
            }
        }

        /// <summary>
        /// The connection definition associated with this alerts widget instance
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
        }

        #endregion Properties


        #region Constructor
        public RepoSymptomDetails(ConnectionDefinition aConnectionDefinition, DataTable dataTable)
        {
            InitializeComponent();
            _connectionDefinition = aConnectionDefinition;
            SetupComponents(dataTable);
        }

        #endregion Constructor

        #region Public Methods


        #endregion Public Methods

        #region Private Methods
        private void SetupComponents(DataTable aDataTable) 
        {
            SetupAlertDetails(aDataTable);
            SetupSymptomEventsWidget();
            SetupSymptomHealthWidget();

            LocateRefreshButton();
            LocateStopButton();

            UpdateExportButtonsStatus();
        }

        private void SetupAlertDetails(DataTable aDataTable) 
        {
            _createTime = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CREATE_TS_LCT_COL_NAME].ToString();
            _creatorHostId = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CREATOR_HOST_ID_COL_NAME].ToString();
            _creatorProcessId = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CREATOR_PROCESS_ID_COL_NAME].ToString();
            _createTimeTextBox.Text = _createTime;
            _processIDTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_PROCESS_ID_COL_NAME].ToString();
            _componentNameTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_COMPONENT_NAME_COL_NAME].ToString();
            _processNameTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_PROCESS_NAME_COL_NAME].ToString();
            _closeTimeTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_CLOSE_TS_LCT_COL_NAME].ToString();
            if (aDataTable.Columns.Contains(SystemAlertsUserControl.ALERT_TYPE_DESCRIPTION_COL_NAME))
            {
                _typeDescriptionTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_TYPE_DESCRIPTION_COL_NAME].ToString();
            }
            else
            {
                _typeDescriptionTextBox.Text = "";
            }
            _severityTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_SEVERITY_NAME_COL_NAME].ToString();
            _statusTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_STATUS_COL_NAME].ToString();
            _lastUpdateTimeTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_LAST_UPDATE_TS_LCT_COL_NAME].ToString();
            _descriptionTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_DESCRIPTION_COL_NAME].ToString();
            _resourceNameTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_RESOURCE_COL_NAME].ToString();
            _resourceTypeTextBox.Text = aDataTable.Rows[0][SystemAlertsUserControl.ALERT_RESOURCE_TYPE_COL_NAME].ToString();
        }

        private void SetupSymptomEventsWidget() 
        {
            //Read the alert options from persistence. 
            //If the alert options does not exist or there is an error reading the persistence, create
            //a default alerts option
            _config1 = WidgetRegistry.GetConfigFromPersistence(RepoSymptomDetailsPersistenceKey1);
            if (_config1 == null)
            {
                _config1 = WidgetRegistry.GetDefaultDBConfig();
                _config1.Name = RepoSymptomDetailsPersistenceKey1;
                _config1.DataProviderConfig.TimerPaused = false;
                _config1.ShowProviderStatus = false;
                _config1.Title = "Event Symptoms";
                _config1.ShowProperties = false;
                _config1.ShowToolBar = true;
                _config1.ShowChart = false;
                _config1.ShowTimerSetupButton = false;                
                _config1.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
                _config1.ShowExportButtons = false;
            }
            _config1.ShowHelpButton = true;
            _config1.HelpTopic = HelpTopics.AlertSymptomEvents;
            _config1.DataProviderConfig.DefaultVisibleColumnNames = InitializationEventDefaultColumns();
            _config1.DataProviderConfig.ColumnMappings = InitializationEventColumnMappings();

            DateTime createDateTime = DateTime.Now;
            DateTime.TryParse(_createTime, out createDateTime);

            DatabaseDataProviderConfig _dbConfig = _config1.DataProviderConfig as DatabaseDataProviderConfig;
            string sqlText = string.Format("SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.{0} " +
                                "WHERE PROBLEM_CREATE_TS_LCT = TIMESTAMP '{1}' " +
                                "AND PROBLEM_CREATOR_HOST_ID = {2} " +
                                "AND PROBLEM_CREATOR_PROCESS_ID = {3} " +
                                "FOR READ UNCOMMITTED ACCESS", EventView, createDateTime.ToString("yyyy-MM-dd HH:mm:ss.FFFFFF"), _creatorHostId.Trim(), _creatorProcessId.Trim());

            _dbConfig.SQLText = sqlText;
            _dbConfig.CommandTimeout = 0;
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            _config1.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            if (_config1.DataProviderConfig.ColumnSortObjects == null)
            {
                _config1.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            }
                       
            //Create the Alert Symptoms Widget
            _symptomEventsWidget = new GenericUniversalWidget();

            _symptomEventsDataProvider = new DatabaseDataProvider(_dbConfig);
            _symptomEventsWidget.DataProvider = _symptomEventsDataProvider;
            ((TabularDataDisplayControl)_symptomEventsWidget.DataDisplayControl).LineCountFormat = "Event Symptoms";

            //Set the widget configuration 
            _symptomEventsWidget.UniversalWidgetConfiguration = _config1;

            _symptomEventsWidget.Dock = DockStyle.Fill;
            _symptomEventsWidget.DataDisplayControl.DataDisplayHandler = new SymptomDetailsDataHandler(this);

            //Initialize the Alerts iGrid
            _symptomEventsGrid = ((TabularDataDisplayControl)_symptomEventsWidget.DataDisplayControl).DataGrid;
            _symptomEventsGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);


            // Remove all current contents and add the alerts widget
            this.TrafodionSplitContainerSymptoms.Panel1.Controls.Clear();
            //Half Half devide the container, but since the first iGrid contains toolbar, add 20 pixels for it.
            this.TrafodionSplitContainerSymptoms.SplitterDistance = TrafodionSplitContainerSymptoms.Height / 2 + 20;

            this.TrafodionSplitContainerSymptoms.Panel1.Controls.Add(_symptomEventsWidget);

            _viewEventDrillDownMenuItem = new TrafodionIGridToolStripMenuItem();
            _viewEventDrillDownMenuItem.Text = Properties.Resources.ViewDrillDownMenu;
            _viewEventDrillDownMenuItem.Click += new EventHandler(viewEventDrillDownMenuItem_Click);
            _symptomEventsGrid.AddContextMenu(_viewEventDrillDownMenuItem);
            _symptomEventsGrid.CellMouseDown += new iGCellMouseDownEventHandler(_symptomEventsGrid_RightMouseDown);

            _symptomEventsGrid.DoubleClickHandler = DoubleClickSymptomEventsGridHandler;

            _symptomEventsDataProvider.OnNewDataArrived += InvokeEventsHandleNewDataArrived;
            _symptomEventsDataProvider.Start();

        }

        private void SetupSymptomHealthWidget()
        {
            //Read the alert options from persistence. 
            //If the alert options does not exist or there is an error reading the persistence, create
            //a default alerts option
            _config2 = WidgetRegistry.GetConfigFromPersistence(RepoSymptomDetailsPersistenceKey2);
            if (_config2 == null)
            {
                _config2 = WidgetRegistry.GetDefaultDBConfig();
                _config2.Name = RepoSymptomDetailsPersistenceKey2;
                _config2.DataProviderConfig.TimerPaused = false;
                _config2.ShowProviderStatus = false;
                _config2.Title = "Symptom Health";
                _config2.ShowProperties = false;
                _config2.ShowToolBar = false;
                _config2.ShowChart = false;
                _config2.ShowTimerSetupButton = false;
                _config2.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }

            _config2.DataProviderConfig.DefaultVisibleColumnNames = InitializationHealthDefaultColumns();
            _config2.DataProviderConfig.ColumnMappings = InitializationHealthColumnMappings();


            DateTime createDateTime = DateTime.Now;
            DateTime.TryParse(_createTime, out createDateTime);

            DatabaseDataProviderConfig _dbConfig = _config2.DataProviderConfig as DatabaseDataProviderConfig;
            string sqlText = string.Format("SELECT * FROM MANAGEABILITY.INSTANCE_REPOSITORY.{0} " +
                                "WHERE PROBLEM_CREATE_TS_LCT = TIMESTAMP '{1}' " +
                                "AND PROBLEM_CREATOR_HOST_ID = {2} " +
                                "AND PROBLEM_CREATOR_PROCESS_ID = {3} " +
                                "FOR READ UNCOMMITTED ACCESS", HealthView, createDateTime.ToString("yyyy-MM-dd HH:mm:ss.FFFFFF"), _creatorHostId.Trim(), _creatorProcessId.Trim());
            
            _dbConfig.SQLText = sqlText;
            _dbConfig.CommandTimeout = 0;
            _dbConfig.ConnectionDefinition = _connectionDefinition;

            _config2.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            if (_config2.DataProviderConfig.ColumnSortObjects == null)
            {
                _config2.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            }
            
            //Create the Alert Symptoms Widget
            _symptomHealthWidget = new GenericUniversalWidget();

            _symptomHealthDataProvider = new DatabaseDataProvider(_dbConfig);
            _symptomHealthWidget.DataProvider = _symptomHealthDataProvider;
            ((TabularDataDisplayControl)_symptomHealthWidget.DataDisplayControl).LineCountFormat = "Health/State Symptoms";

            //Set the widget configuration 
            _symptomHealthWidget.UniversalWidgetConfiguration = _config2;

            _symptomHealthWidget.Dock = DockStyle.Fill;
            _symptomHealthWidget.DataDisplayControl.DataDisplayHandler = new SymptomDetailsDataHandler(this);

            //Initialize the Alerts iGrid
            _symptomHealthGrid = ((TabularDataDisplayControl)_symptomHealthWidget.DataDisplayControl).DataGrid;
            _symptomHealthGrid.DefaultCol.ColHdrStyle.Font = new Font("Tahoma", 8.25F, FontStyle.Bold);


            // Remove all current contents and add the alerts widget         
            this.TrafodionSplitContainerSymptoms.Panel2.Controls.Clear();
            this.TrafodionSplitContainerSymptoms.Panel2.Controls.Add(_symptomHealthWidget);

            _viewHealthDrillDownMenuItem = new TrafodionIGridToolStripMenuItem();
            _viewHealthDrillDownMenuItem.Text = Properties.Resources.ViewDrillDownMenu;
            _viewHealthDrillDownMenuItem.Click += new EventHandler(viewHealthDrillDownMenuItem_Click);
            _symptomHealthGrid.AddContextMenu(_viewHealthDrillDownMenuItem);
            _symptomHealthGrid.CellMouseDown += new iGCellMouseDownEventHandler(_symptomHealthGrid_RightMouseDown);
            _symptomHealthGrid.DoubleClickHandler = DoubleClickSymptomHealthGridHandler;
			
            _symptomHealthDataProvider.OnNewDataArrived += InvokeHealthHandleNewDataArrived;
            _symptomHealthDataProvider.Start();
        }

        private List<string> InitializationEventDefaultColumns()
        {
            List<string> defaultColumns = new List<string>();
            defaultColumns.Add(EVENT_GEN_TS_LCT_COL_NAME);
            defaultColumns.Add(EVENT_COMPONENT_NAME_COL_NAME);
            defaultColumns.Add(EVENT_EVENT_ID_COL_NAME);
            defaultColumns.Add(EVENT_SEVERITY_NAME_COL_NAME);
            defaultColumns.Add(EVENT_PROCESS_NAME_COL_NAME);
            defaultColumns.Add(EVENT_TEXT_COL_NAME);
            defaultColumns.Add(EVENT_PROCESS_ID_COL_NAME);
            defaultColumns.Add(EVENT_THREAD_ID_COL_NAME);
            defaultColumns.Add(EVENT_NODE_ID_COL_NAME);
            defaultColumns.Add(EVENT_PNID_ID_COL_NAME);
            defaultColumns.Add(EVENT_HOST_ID_COL_NAME);
            defaultColumns.Add(EVENT_IP_ADDRESS_ID_COL_NAME);
            return defaultColumns;
        }

        private List<string> InitializationHealthDefaultColumns()
        {
            List<string> defaultColumns = new List<string>();
            defaultColumns.Add(HEALTH_GEN_TS_LCT_COL_NAME);
            defaultColumns.Add(HEALTH_COMPONENT_NAME_COL_NAME);
            defaultColumns.Add(HEALTH_PUBLICATION_TYPE_COL_NAME);
            defaultColumns.Add(HEALTH_CHECK_INTERVAL_SEC_COL_NAME);
            defaultColumns.Add(HEALTH_LOGICAL_OBJECT_TYPE_NAME_COL_NAME);
            defaultColumns.Add(HEALTH_LOGICAL_OBJECT_NAME_COL_NAME);
            defaultColumns.Add(HEALTH_LOGICAL_OBJECT_PATH_COL_NAME);
            defaultColumns.Add(HEALTH_LOGICAL_OBJECT_QUAL_1_COL_NAME);
            defaultColumns.Add(HEALTH_LOGICAL_OBJECT_QUAL_2_COL_NAME);
            defaultColumns.Add(HEALTH_CURRENT_HEALTH_NAME_COL_NAME);
            defaultColumns.Add(HEALTH_PREVIOUS_HEALTH_NAME_COL_NAME);
            defaultColumns.Add(HEALTH_HEALTH_CHANGE_TS_LCT_COL_NAME);
            return defaultColumns;
        }

        private List<ColumnMapping> InitializationEventColumnMappings()
        {
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();            
            
            columnMappings.Add(new ColumnMapping(EVENT_GEN_TS_LCT_COL_NAME, "Gen Time LCT", 120));           
            columnMappings.Add(new ColumnMapping(EVENT_PROCESS_ID_COL_NAME, "Process ID", 200));
            columnMappings.Add(new ColumnMapping(EVENT_THREAD_ID_COL_NAME, "Thread ID", 120));
            columnMappings.Add(new ColumnMapping(EVENT_NODE_ID_COL_NAME, "Node ID", 120));
            columnMappings.Add(new ColumnMapping(EVENT_PNID_ID_COL_NAME, "PNID ID", 120));
            columnMappings.Add(new ColumnMapping(EVENT_HOST_ID_COL_NAME, "Host ID", 120));
            columnMappings.Add(new ColumnMapping(EVENT_IP_ADDRESS_ID_COL_NAME, "IP Address ID", 120));
            columnMappings.Add(new ColumnMapping(EVENT_PROCESS_NAME_COL_NAME, "Process Name", 120));
            columnMappings.Add(new ColumnMapping(EVENT_COMPONENT_NAME_COL_NAME, "Component Name", 120));
            columnMappings.Add(new ColumnMapping(EVENT_EVENT_ID_COL_NAME, "Event ID", 120));
            columnMappings.Add(new ColumnMapping(EVENT_SEVERITY_NAME_COL_NAME, "Severity Name", 120));
            columnMappings.Add(new ColumnMapping(EVENT_TEXT_COL_NAME, "Text", 120));
            
            return columnMappings;
        }

        private List<ColumnMapping> InitializationHealthColumnMappings()
        {
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();

            columnMappings.Add(new ColumnMapping(HEALTH_GEN_TS_LCT_COL_NAME, "Gen Time LCT", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_COMPONENT_NAME_COL_NAME, "Component Name", 200));
            columnMappings.Add(new ColumnMapping(HEALTH_PUBLICATION_TYPE_COL_NAME, "Publication Type", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_CHECK_INTERVAL_SEC_COL_NAME, "Check Interval Seconds", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_LOGICAL_OBJECT_TYPE_NAME_COL_NAME, "Logic Object Type Name", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_LOGICAL_OBJECT_NAME_COL_NAME, "Logic Object Name", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_LOGICAL_OBJECT_PATH_COL_NAME, "Logic Object Path", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_LOGICAL_OBJECT_QUAL_1_COL_NAME, "Logical Object Qualifier 1", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_LOGICAL_OBJECT_QUAL_2_COL_NAME, "Logical Object Qualifier 2", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_CURRENT_HEALTH_NAME_COL_NAME, "Current Health Name", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_PREVIOUS_HEALTH_NAME_COL_NAME, "Previous Health Name", 120));
            columnMappings.Add(new ColumnMapping(HEALTH_HEALTH_CHANGE_TS_LCT_COL_NAME, "Health Change Time LCT", 120));

            return columnMappings;
        }

        


        private void InvokeEventsHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(EventsDataProvider_OnNewDataArrived), new object[] { obj, e });    
                    _symptomEventsGrid.UpdateCountControlText(string.Format("There are {0} event symptoms", _symptomEventsGrid.Rows.Count));
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void EventsDataProvider_OnNewDataArrived(object sender, EventArgs e) 
        {
            if (_symptomHealthWidget.StatusLabel.Text.Contains("Fetch"))
            {
                SetToolStripButtonFetchingStatus(true);                
            }
            else 
            {
                SetToolStripButtonFetchingStatus(false);
            }
            UpdateExportButtonsStatus();
        }

        private void InvokeHealthHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvent(HealthDataProvider_OnNewDataArrived), new object[] { obj, e });
                    _symptomHealthGrid.UpdateCountControlText(string.Format("There are {0} health/state symptoms", _symptomHealthGrid.Rows.Count));
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HealthDataProvider_OnNewDataArrived(object sender, EventArgs e) 
        {
            if (_symptomEventsWidget.StatusLabel.Text.Contains("Fetch"))
            {
                SetToolStripButtonFetchingStatus(true);                
            }
            else 
            {
                SetToolStripButtonFetchingStatus(false);                
            }
            UpdateExportButtonsStatus();
        }

        public void SetToolStripButtonFetchingStatus(bool fetchStatus) 
        {
            _stopButton.Enabled = fetchStatus;
            _refreshButton.Enabled = !fetchStatus;
        }

        private void _refreshButton_Click(object sender, EventArgs e)
        {
            _symptomEventsDataProvider.Start();
            _symptomHealthDataProvider.Start();

            UpdateExportButtonsStatus();
        }

        private void _stopButton_Click(object sender, EventArgs e) 
        {
            _symptomEventsDataProvider.Stop();
            _symptomHealthDataProvider.Stop();

            UpdateExportButtonsStatus();
        }


        private void LocateRefreshButton()
        {
            try
            {
                var toolStrip = (TrafodionToolStrip)this._symptomEventsWidget.Controls.Find("_theToolStrip", true)[0];
                _refreshButton = (ToolStripButton)toolStrip.Items.Find("_theRefreshButton", true)[0];
                _refreshButton.Click += _refreshButton_Click;
            }
            catch (Exception)
            {
                
            }
        }

        private void LocateStopButton()
        {
            try
            {
                var toolStrip = (TrafodionToolStrip)this._symptomEventsWidget.Controls.Find("_theToolStrip", true)[0];
                _stopButton = (ToolStripButton)toolStrip.Items.Find("_theStopQuery", true)[0];
                _stopButton.Click += _stopButton_Click;
            }
            catch (Exception)
            {

            }
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_symptomEventsWidget != null)
                {
                    if (_symptomEventsDataProvider != null)
                    {
                        _symptomEventsDataProvider.OnNewDataArrived -= InvokeEventsHandleNewDataArrived;
                    }

                    if (_refreshButton != null)
                    {
                        _refreshButton.Click -= _refreshButton_Click;
                    }

                    if (_stopButton != null)
                    {
                        _stopButton.Click -= _stopButton_Click;
                    }
                }

                if (_symptomHealthWidget != null)
                {
                    if (_symptomHealthDataProvider != null)
                    {
                        _symptomHealthDataProvider.OnNewDataArrived -= InvokeHealthHandleNewDataArrived;
                    }
                }
            }
        }

        private void _closeButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        void _symptomEventsGrid_RightMouseDown(object sender, iGCellMouseDownEventArgs e)
        {
            if (e.Button == MouseButtons.Right && sender is TrafodionIGrid)
            {
                String cellValue = this._symptomEventsGrid.Rows[e.RowIndex].Cells[EVENT_TOKENIZED_EVENT_TABLE_COL_NAME].Value as String;
                if (String.IsNullOrEmpty(cellValue))
                    _viewEventDrillDownMenuItem.Enabled = false;
                else if (cellValue.Trim() == "")
                    _viewEventDrillDownMenuItem.Enabled = false;
                else
                    _viewEventDrillDownMenuItem.Enabled = true;
            }
        }

        void _symptomHealthGrid_RightMouseDown(object sender, iGCellMouseDownEventArgs e)
        {
            if (e.Button == MouseButtons.Right && sender is TrafodionIGrid)
            {
                String cellValue = this._symptomHealthGrid.Rows[e.RowIndex].Cells[HEALTH_BASE_TABLE_NAME_COL_NAME].Value as String;
                if (String.IsNullOrEmpty(cellValue))
                    _viewHealthDrillDownMenuItem.Enabled = false;
                else if (cellValue.Trim() == "")
                    _viewHealthDrillDownMenuItem.Enabled = false;
                else
                    _viewHealthDrillDownMenuItem.Enabled = true;
            }
        }

        void DoubleClickSymptomEventsGridHandler(int rowIndex)
        {
            String cellValue = this._symptomEventsGrid.Rows[rowIndex].Cells[EVENT_TOKENIZED_EVENT_TABLE_COL_NAME].Value as String;
            if (String.IsNullOrEmpty(cellValue))
                _symptomEventsGrid.ShowRowDetails(rowIndex);
            else if (cellValue.Trim() == "")
                _symptomEventsGrid.ShowRowDetails(rowIndex);
            else
                viewEventSymptomDetails(rowIndex);      
        }

        void DoubleClickSymptomHealthGridHandler(int rowIndex)
        {
            String cellValue = this._symptomHealthGrid.Rows[rowIndex].Cells[HEALTH_BASE_TABLE_NAME_COL_NAME].Value as String;
            if (String.IsNullOrEmpty(cellValue))
                _symptomHealthGrid.ShowRowDetails(rowIndex);
            else if (cellValue.Trim() == "")
                _symptomHealthGrid.ShowRowDetails(rowIndex);
            else
                viewHealthSymptomDetails(rowIndex);
        }

        private void viewEventDrillDownMenuItem_Click(object sender, EventArgs e)
        {
            if (sender is TrafodionIGridToolStripMenuItem)
            {
                int rowIndex = ((TrafodionIGridToolStripMenuItem)sender).TrafodionIGridEventObject.Row;
                viewEventSymptomDetails(rowIndex);
            }
        }

        private void viewHealthDrillDownMenuItem_Click(object sender, EventArgs e)
        {
            if (sender is TrafodionIGridToolStripMenuItem)
            {
                int rowIndex = ((TrafodionIGridToolStripMenuItem)sender).TrafodionIGridEventObject.Row;
                viewHealthSymptomDetails(rowIndex);
            }
        }

        void viewEventSymptomDetails(int rowIndex)
        {
            DataTable table = new DataTable();
            foreach (iGCol col in _symptomEventsGrid.Cols)
            {
                table.Columns.Add(col.Key);
            }

            DataRow dr = table.NewRow();
            for (int i = 0; i < _symptomEventsGrid.Rows[rowIndex].Cells.Count; i++)
                dr[i] = _symptomEventsGrid.Rows[rowIndex].Cells[i].Text;
            table.Rows.Add(dr);

            EventsSymptomDetailControl eventdetails = new EventsSymptomDetailControl(this._connectionDefinition, table);
            //Add height of size to contain the TrafodionManager banner
            Size aSize = eventdetails.Size;
            aSize.Height += 100;
            WindowsManager.PutInWindow(aSize, eventdetails, "Event Symptom", this._connectionDefinition);
        }

        void viewHealthSymptomDetails(int rowIndex)
        {
            DataTable table = new DataTable();
            foreach (iGCol col in _symptomHealthGrid.Cols)
            {
                table.Columns.Add(col.Key);
            }

            DataRow dr = table.NewRow();
            for (int i = 0; i < _symptomHealthGrid.Rows[rowIndex].Cells.Count; i++ )
                dr[i] = _symptomHealthGrid.Rows[rowIndex].Cells[i].Text;
            table.Rows.Add(dr);

            HealthSymptomDetailControl healthdetails = new HealthSymptomDetailControl(this._connectionDefinition, table);
            //Add height of size to contain the TrafodionManager banner
            Size aSize = healthdetails.Size;
            aSize.Height += 100;
            WindowsManager.PutInWindow(aSize, healthdetails, "Health/State Symptom", this._connectionDefinition);
        }
        #endregion Private Methods

        private void _clipboardButton_Click(object sender, EventArgs e)
        {
            TrafodionIGrid grid = GetIGridWithContents();
            if (grid != null) 
            {
                grid.ExportToClipboard();
            }            
        }

        private void _explorerButton_Click(object sender, EventArgs e)
        {
            TrafodionIGrid grid = GetIGridWithContents();
            if (grid != null)
            {
                grid.ExportToBrowser();
            }    
        }

        private void _spreadsheetButton_Click(object sender, EventArgs e)
        {
            TrafodionIGrid grid = GetIGridWithContents();
            if (grid != null)
            {
                grid.ExportToSpreadsheet();
            } 
        }

        private void _fileButton_Click(object sender, EventArgs e)
        {
            TrafodionIGrid grid = GetIGridWithContents();
            if (grid != null)
            {
                grid.ExportToFile();
            } 
        }

        private TrafodionIGrid GetIGridWithContents() 
        {
            //According to symptom detail design, at most one type of symptoms can contain data.
            if (_symptomEventsGrid != null && _symptomEventsGrid.Rows.Count > 0)
            {
                return _symptomEventsGrid;
            }
            else if (_symptomHealthGrid != null && _symptomHealthGrid.Rows.Count > 0)
            {
                return _symptomHealthGrid;
            }
            else return null;
        }
                

        private void UpdateExportButtonsStatus()
        {
            if ((_symptomEventsWidget != null && _symptomEventsWidget.StatusLabel.Text.Contains("Fetch")) || (_symptomHealthWidget != null && _symptomHealthWidget.StatusLabel.Text.Contains("Fetch")))
            {
                _clipboardButton.Enabled = _fileButton.Enabled = _explorerButton.Enabled = _spreadsheetButton.Enabled = false;
            }
            else 
            {
                _clipboardButton.Enabled = _fileButton.Enabled = _explorerButton.Enabled = _spreadsheetButton.Enabled = true;
            }
        }

        

        

        

    }



  
   

    public class SymptomDetailsDataHandler : TabularDataDisplayHandler
    {
        #region private member variables

        private RepoSymptomDetails _repoSymptomDetails;

        #endregion private member variables

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aSystemAlertsUserControl"></param>
        public SymptomDetailsDataHandler(RepoSymptomDetails aRepoSymptomDetailsEventsControl)
        {
            _repoSymptomDetails = aRepoSymptomDetailsEventsControl;
        }

        #region UniversalWidget DataDisplayHandler methods

        /// <summary>
        /// Populate the Alerts Grid
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            //Populate the grid with the data table passed
            if (aDataTable != null)
            {
                aDataGrid.Clear();
                TrafodionIGridUtils.PopulateGrid(aConfig, aDataTable, aDataGrid);

                //Enable status and notes columns for editing
                //Other columns remain readonly
                foreach (iGCol column in aDataGrid.Cols)
                {
                    if (column.CellStyle.ValueType == typeof(System.DateTime))
                    {
                        column.CellStyle.FormatProvider = _repoSymptomDetails.TheDateTimeFormatProvider;
                        column.CellStyle.FormatString = "{0}";
                    }
                }
                TrafodionIGridUtils.updateIGridColumnHeadings(aDataGrid);
                //aDataGrid.UpdateCountControlText(string.Format("There are {0} symptom events", aDataGrid.Rows.Count));
                aDataGrid.ResizeGridColumns(aDataTable, 7, 20);

                if (aConfig.DataProviderConfig.ColumnSortObjects != null && aConfig.DataProviderConfig.ColumnSortObjects.Count == 0)
                {
                    if (aDataGrid.Cols.KeyExists("GEN_TS_LCT"))
                    {
                        aConfig.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(aDataGrid.Cols["GEN_TS_LCT"].Index, 0, (int)iGSortOrder.Descending));
                    }
                }
                TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);

                aDataGrid.EndUpdate();
                aDataGrid.Cols.AutoWidth();
            }
        }
        #endregion UniversalWidget DataDisplayHandler methods
    }

}
