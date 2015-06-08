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
#define SQL_PLAN

using System;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;
using System.IO;
using System.Reflection;


namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class MonitorWorkloadCanvas : WMSWorkloadCanvas
    {
        #region Constants
        private const int NORMAL_ROW_HEIGHT = 19;
        private const int PREVIEW_ROW_HEIGHT = 38;
        private static Size ChildrenWindowSize = new Size(800, 600);
        #endregion

        #region Static Members
        private static readonly string MonitorWorkloadConfigName = "WMS_MonitorWorkloadConfig";
        private static readonly string ParentChildQueriesConfigName = "WMS_ParentChildQueriesConfig";
        private static readonly string STATUS_QUERIES_ALL_MERGED = "STATUS QUERIES ALL MERGED";
        private static readonly string STATUS_QUERIES_CHILDREN = "STATUS QUERY {0} CHILDREN";
        private static readonly string STATUS_QUERY_QID_MERGED = "STATUS QUERY MXID01001001188212123570257198296000000001007Dummy MERGED";
        #endregion

        #region Members
        private bool _isParentChildQueries = false;
        private string _selectedParentChildQueryID = null;
        private Size _preferredSize = Size.Empty;
        private Persistence.PersistenceHandler _persistenceHandler = null;
        private ConnectionDefinition _theConnectionDefinition = null;
        private UniversalWidgetConfig MonitorWorkloadConfig = null;
        private GenericUniversalWidget MonitorWorkloadWidget = null;
        private TrafodionIGrid _monitorWorkloadIGrid = null;        
        private TrafodionIGrid _previousMonitorWorkloadIGrid = null;
        private string _theTitle = Properties.Resources.LiveWorkloads;
        private TrafodionIGridToolStripMenuItem _workloadDetailMenuItem = null;
        private TrafodionIGridToolStripMenuItem _displayParentChildQueriesMenuItem = null;
        private TrafodionIGridToolStripMenuItem _displaySQLTextMenuItem = null;
#if SQL_PLAN
        private TrafodionIGridToolStripMenuItem _displaySQLPlanMenuItem = null;
#endif
        private TrafodionIGridToolStripMenuItem _cancelSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _holdSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _releaseSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _reprepareSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _loadTextSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _loadPlanSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _warnInfoMenuItem = null;
        private TrafodionIGridToolStripMenuItem _rulesAssociatedMenuItem = null;
        private TrafodionIGridToolStripMenuItem _loadHistoryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _reorgProgressMenuItem = null;
        private TrafodionIGridToolStripMenuItem _queryProgressVisualizerMenuItem = null;
#if REPO_INFO
        private TrafodionIGridToolStripMenuItem _repositoryInfoMenuItem = null;
#endif
        private TrafodionIGridToolStripMenuItem _processDetailMenuItem = null;
        private TrafodionIGridToolStripMenuItem _pstateMenuItem = null;
        private TrafodionIGridToolStripMenuItem _childrenProcessesMenuItem = null;
        private DataTable _previousDataTable = null;
        private MonitorWorkloadOptions _monitorWorkloadOptions = null;
        private int _iGridNormalRowHeight = NORMAL_ROW_HEIGHT;
        private int _iGridRowHeightWithPreview = PREVIEW_ROW_HEIGHT;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid3RowTextColCellStyle;
        private iGRow _highlightedQuery = null;
        ToolStripButton _sqlPreviewButton = new ToolStripButton();
        ToolStripButton _loadTriageButton = new ToolStripButton();
        ToolStripButton _loadSessionButton = new ToolStripButton();
        ToolStripButton _clientRuleButton = new ToolStripButton();
        ToolStripButton _loadHistoryButton = new ToolStripButton();
        private string clientRuleFolderName = ClientRuleOptions.GetOptions().ClientRuleFolderName;
        /// <summary>
        /// Two Events - Get Session and Load Triage
        /// The events are registered in TabbedWorkloadUserControlWrapper, 
        /// and actually execute command is in TriageGridUserControl
        /// </summary>
        public delegate void GetSession();
        public event GetSession GetSessionEvent;

        public delegate void loadQueriesToTriageSpace();
        public event loadQueriesToTriageSpace LoadQueriesToTriageSpaceEvent;
        public delegate void UpdateStatus(Object obj, EventArgs e);
        
        

        #endregion

        #region Properties
        public MonitorWorkloadOptions MonitorWorkloadOptions
        {
            get { return _monitorWorkloadOptions; }
            set { _monitorWorkloadOptions = value; }
        }

        public DataTable PreviousDataTable
        {
            get { return _previousDataTable; }
            set { _previousDataTable = value; }
        }

        public GenericUniversalWidget TheMonitorWorkloadWidget
        {
            get { return MonitorWorkloadWidget; }
        }

        public TrafodionIGrid MonitorWorkloadIGrid
        {
            get { return _monitorWorkloadIGrid; }
        }

        public bool ClientRulesEnabled 
        {
            get 
            {
                if (this.ConnectionDefn == null)
                    return false;
                else
                    return true;
            }
        }

        private string QueryCommand
        {
            get
            {
                return this._isParentChildQueries ? string.Format(STATUS_QUERIES_CHILDREN, SelectedParentChildQueryID) : STATUS_QUERIES_ALL_MERGED;
            }
        }

        private string SelectedParentChildQueryID
        {
            get
            {
                return this._selectedParentChildQueryID;
            }
            set
            {
                if (value != null)
                {
                    this._isParentChildQueries = true;
                    this._selectedParentChildQueryID = value;
                }
            }
        }

        #endregion

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set
            {
                if (_theConnectionDefinition != null && MonitorWorkloadWidget != null)
                {
                    MonitorWorkloadWidget.DataProvider.Stop();
                }

                if (_theConnectionDefinition != null && value != null && !_theConnectionDefinition.Equals(value))
                {
                    _monitorWorkloadIGrid.Rows.Clear();
                }

                _theConnectionDefinition = value;

                if (_theConnectionDefinition != null)
                {
                    if (MonitorWorkloadWidget != null)
                    {
                        MonitorWorkloadWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;
                        ((MonitorWorkloadDataProvider)MonitorWorkloadWidget.DataProvider).ConnectionDefinition = _theConnectionDefinition;
                        MonitorWorkloadWidget.DataProvider.Start();
                    }
                    else
                    {
                        ShowWidgets();
                    }
                }

            }
        }

        public MonitorWorkloadCanvas(ConnectionDefinition connectionDefinition)
           : this(connectionDefinition, null, Size.Empty)
        { 
        }

        public MonitorWorkloadCanvas(ConnectionDefinition connectionDefinition, string selectedParentChildQueryID, Size preferredSize)
        {
            InitializeComponent();

            this.SelectedParentChildQueryID = selectedParentChildQueryID;
            this._preferredSize = preferredSize;
            _theConnectionDefinition = connectionDefinition;
            computeRowHeightForSQLPreview();
            ShowWidgets();
        }

        void MyDispose(bool disposing)
        {
            if (disposing)
            {
                RemoveHandlers();

                if (MonitorWorkloadWidget != null)
                {
                    MonitorWorkloadWidget.DataProvider.Stop();
                }
            }
        }

        void DisplayErrorPanel(string errorMessage)
        {
            Controls.Clear();

            TrafodionLabel errorLabel = new TrafodionLabel();
            errorLabel.Text = errorMessage;
            errorLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            errorLabel.Dock = DockStyle.Fill;

            Controls.Add(errorLabel);
        }

        private string getDisplayColumnName(string colName)
        {
            return TrafodionIGridUtils.ConvertUnderlineToBreak(colName);
        }

        public void ShowWidgets()
        {
            string persistenceFileName = this._isParentChildQueries ? ParentChildQueriesConfigName : MonitorWorkloadConfigName;
            //Create the configuration. If one is persisted, we use that otherwise we create one
            UniversalWidgetConfig tempConfig = WidgetRegistry.GetConfigFromPersistence(persistenceFileName);

            _monitorWorkloadOptions = MonitorWorkloadOptions.GetOptions();

            _previousMonitorWorkloadIGrid = new TrafodionIGrid();

            DatabaseDataProviderConfig dbConfig = null;
            List<ColumnMapping> persistedColumnMappings = null;
            if (tempConfig == null)
            {
                MonitorWorkloadConfig = WidgetRegistry.GetDefaultDBConfig();
                MonitorWorkloadConfig.Name = persistenceFileName;
                MonitorWorkloadConfig.Title = _theTitle;
                dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.CommandTimeout = WORKLOAD_EXEC_TIMEOUT;
                dbConfig.OpenCommand = "WMSOPEN";
                dbConfig.CloseCommand = "WMSCLOSE";
                dbConfig.RefreshRate = WORKLOAD_REFRESH_RATE;               

            }
            else
            {
                MonitorWorkloadConfig = tempConfig;

                if (tempConfig.DataProviderConfig != null)
                {
                    persistedColumnMappings = tempConfig.DataProviderConfig.ColumnMappings;
                }
            }

            dbConfig = (DatabaseDataProviderConfig)MonitorWorkloadConfig.DataProviderConfig;
            dbConfig.SQLText = QueryCommand;
            dbConfig.TimerPaused = false;

            List<string> defaultVisibleColumns = new List<string>();
            defaultVisibleColumns.Add("WARN_LEVEL");
            defaultVisibleColumns.Add("QUERY_STATE");
            defaultVisibleColumns.Add("QUERY_SUBSTATE");
            defaultVisibleColumns.Add("TOTAL_QUERY_TIME");
            defaultVisibleColumns.Add("ELAPSED_TIME");
            defaultVisibleColumns.Add("WAIT_TIME");
            defaultVisibleColumns.Add("HOLD_TIME");
            defaultVisibleColumns.Add("SERVICE_NAME");
            defaultVisibleColumns.Add("COMPUTER_NAME");
            defaultVisibleColumns.Add("ROLE_NAME");
            defaultVisibleColumns.Add("APPLICATION_NAME");
            defaultVisibleColumns.Add("DATASOURCE");
            defaultVisibleColumns.Add("EST_COST");
            defaultVisibleColumns.Add("EST_CPU_TIME");
            defaultVisibleColumns.Add("EST_IO_TIME");
            defaultVisibleColumns.Add("EST_MSG_TIME");
            defaultVisibleColumns.Add("EST_IDLE_TIME");
            defaultVisibleColumns.Add("EST_TOTAL_TIME");
            defaultVisibleColumns.Add("EST_CARDINALITY");
            defaultVisibleColumns.Add("EST_TOTAL_MEM");
            defaultVisibleColumns.Add("COMP_START_TIME");
            defaultVisibleColumns.Add("COMP_END_TIME");
            defaultVisibleColumns.Add("EXEC_START_TIME");
            defaultVisibleColumns.Add("EXEC_END_TIME");
            defaultVisibleColumns.Add("EXEC_STATE");
            defaultVisibleColumns.Add("PROCESS_NAME");
            defaultVisibleColumns.Add("QUERY_PRIORITY");
            defaultVisibleColumns.Add("TRANSACTION_ID");
            defaultVisibleColumns.Add("ACCESSED_ROWS");
            defaultVisibleColumns.Add("USED_ROWS");
            defaultVisibleColumns.Add("EST_ACCESSED_ROWS");
            defaultVisibleColumns.Add("EST_USED_ROWS");
            defaultVisibleColumns.Add("MESSAGE_COUNT");
            defaultVisibleColumns.Add("MESSAGE_BYTES");
            defaultVisibleColumns.Add("STATS_BYTES");
            defaultVisibleColumns.Add("DISK_IOS");
            defaultVisibleColumns.Add("LOCK_WAITS");
            defaultVisibleColumns.Add("LOCK_ESCALATIONS");

            defaultVisibleColumns.Add("PROCESSOR_USAGE_PER_SEC");
            defaultVisibleColumns.Add("TOTAL_PROCESSOR_TIME");
            defaultVisibleColumns.Add("LAST_INTERVAL_PROCESSOR_TIME");
            defaultVisibleColumns.Add("DELTA_PROCESSOR_TIME");
            defaultVisibleColumns.Add("SQL_CPU_TIME_SEC");
            defaultVisibleColumns.Add("PROCESS_BUSYTIME_SEC");
            defaultVisibleColumns.Add("OPEN_TIME_SEC");
            defaultVisibleColumns.Add("PROCESS_CREATE_TIME_SEC");

            defaultVisibleColumns.Add("TOTAL_MEM_ALLOC");
            defaultVisibleColumns.Add("MAX_MEM_USED");
            defaultVisibleColumns.Add("NUM_SQL_PROCESSES");
            defaultVisibleColumns.Add("PROCESSES_CREATED");
            defaultVisibleColumns.Add("QUERY_ID");
            defaultVisibleColumns.Add("QUERY_TEXT");

            defaultVisibleColumns.Add("CMP_NUMBER_OF_BMOS");
            defaultVisibleColumns.Add("CMP_OVERFLOW_MODE");
            defaultVisibleColumns.Add("CMP_OVERFLOW_SIZE");

            defaultVisibleColumns.Add("OVF_FILE_COUNT");
            defaultVisibleColumns.Add("OVF_SPACE_ALLOCATED");
            defaultVisibleColumns.Add("OVF_SPACE_USED");
            defaultVisibleColumns.Add("OVF_BLOCK_SIZE");
            defaultVisibleColumns.Add("OVF_WRITE_READ_COUNT");
            defaultVisibleColumns.Add("OVF_WRITE_COUNT");
            defaultVisibleColumns.Add("OVF_BUFFER_BLOCKS_WRITTEN");
            defaultVisibleColumns.Add("OVF_BUFFER_BYTES_WRITTEN");
            defaultVisibleColumns.Add("OVF_READ_COUNT");
            defaultVisibleColumns.Add("OVF_BUFFER_BLOCKS_READ");
            defaultVisibleColumns.Add("OVF_BUFFER_BYTES_READ");

            MonitorWorkloadConfig.DataProviderConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping("WARN_LEVEL", getDisplayColumnName("Warning_Level"), 80));
            columnMappings.Add(new ColumnMapping("QUERY_STATE", getDisplayColumnName("Query_State"), 80));
            columnMappings.Add(new ColumnMapping("QUERY_SUBSTATE", getDisplayColumnName("Query_SubState"), 80));
            columnMappings.Add(new ColumnMapping("TOTAL_QUERY_TIME", getDisplayColumnName("Total_Query_Time"), 80));
            columnMappings.Add(new ColumnMapping("ELAPSED_TIME", getDisplayColumnName("Elapsed_Time"), 80));
            columnMappings.Add(new ColumnMapping("WAIT_TIME", getDisplayColumnName("Wait_Time"), 80));
            columnMappings.Add(new ColumnMapping("HOLD_TIME", getDisplayColumnName("Hold_Time"), 80));
            columnMappings.Add(new ColumnMapping("SERVICE_NAME", getDisplayColumnName("Service_Name"), 150));
            columnMappings.Add(new ColumnMapping("COMPUTER_NAME", getDisplayColumnName("Computer_Name"), 150));
            columnMappings.Add(new ColumnMapping("ROLE_NAME", getDisplayColumnName("Role_Name"), 150));
            columnMappings.Add(new ColumnMapping("APPLICATION_NAME", getDisplayColumnName("Application_Name"), 150));
            columnMappings.Add(new ColumnMapping("DATASOURCE", getDisplayColumnName("Datasource"), 150));
            columnMappings.Add(new ColumnMapping("EST_COST", getDisplayColumnName("Estimated_Cost"), 80));
            columnMappings.Add(new ColumnMapping("EST_CPU_TIME", getDisplayColumnName("Estimated_CPU_Time"), 80));
            columnMappings.Add(new ColumnMapping("EST_IO_TIME", getDisplayColumnName("Estimated_IO_Time"), 80));
            columnMappings.Add(new ColumnMapping("EST_MSG_TIME", getDisplayColumnName("Estimated_Message_Time"), 80));
            columnMappings.Add(new ColumnMapping("EST_IDLE_TIME", getDisplayColumnName("Estimated_Idle_Time"), 80));
            columnMappings.Add(new ColumnMapping("EST_TOTAL_TIME", getDisplayColumnName("Estimated_Total_Time"), 80));
            columnMappings.Add(new ColumnMapping("EST_CARDINALITY", getDisplayColumnName("Estimated_Cardinality"), 80));
            columnMappings.Add(new ColumnMapping("EST_TOTAL_MEM", getDisplayColumnName("Estimated_Total_Memory"), 80));
            columnMappings.Add(new ColumnMapping("COMP_START_TIME", getDisplayColumnName("Compile_Start_Time"), 150));
            columnMappings.Add(new ColumnMapping("COMP_END_TIME", getDisplayColumnName("Compile_End_Time"), 150));
            columnMappings.Add(new ColumnMapping("EXEC_START_TIME", getDisplayColumnName("Execution_Start_Time"), 150));
            columnMappings.Add(new ColumnMapping("EXEC_END_TIME", getDisplayColumnName("Execution_End_Time"), 150));
            columnMappings.Add(new ColumnMapping("EXEC_STATE", getDisplayColumnName("Execution State"), 80));
            columnMappings.Add(new ColumnMapping("PROCESS_NAME", getDisplayColumnName("Process_Name"), 150));
            columnMappings.Add(new ColumnMapping("QUERY_PRIORITY", getDisplayColumnName("Query_Priority"), 80));
            columnMappings.Add(new ColumnMapping("TRANSACTION_ID", getDisplayColumnName("Transaction_ID"), 150));
            columnMappings.Add(new ColumnMapping("ACCESSED_ROWS", getDisplayColumnName("Accessed_Rows"), 80));
            columnMappings.Add(new ColumnMapping("USED_ROWS", getDisplayColumnName("Used_Rows"), 80));
            columnMappings.Add(new ColumnMapping("EST_ACCESSED_ROWS", getDisplayColumnName("Estimated_Accessed_Rows"), 80));
            columnMappings.Add(new ColumnMapping("EST_USED_ROWS", getDisplayColumnName("Estimated_Used_Rows"), 80));
            columnMappings.Add(new ColumnMapping("MESSAGE_COUNT", getDisplayColumnName("Message_Count"), 80));
            columnMappings.Add(new ColumnMapping("MESSAGE_BYTES", getDisplayColumnName("Message_Bytes"), 80));
            columnMappings.Add(new ColumnMapping("STATS_BYTES", getDisplayColumnName("Statistics_Bytes"), 80));
            columnMappings.Add(new ColumnMapping("DISK_IOS", getDisplayColumnName("Disk_IOs"), 80));
            columnMappings.Add(new ColumnMapping("LOCK_WAITS", getDisplayColumnName("Lock_Waits"), 80));
            columnMappings.Add(new ColumnMapping("LOCK_ESCALATIONS", getDisplayColumnName("Lock_Escalations"), 80));
            columnMappings.Add(new ColumnMapping("QUERY_START_TIME", getDisplayColumnName("Query_Start_Time"), 80));
            columnMappings.Add(new ColumnMapping("START_TS", getDisplayColumnName("Submit_Time"), 80));
            columnMappings.Add(new ColumnMapping("PARENT_QUERY_ID", getDisplayColumnName("Parent_Query_ID"), 80));
            columnMappings.Add(new ColumnMapping("PROCESSOR_USAGE_PER_SEC", getDisplayColumnName("Processor Usage_Per_Second"), 80));
            columnMappings.Add(new ColumnMapping("TOTAL_PROCESSOR_TIME", getDisplayColumnName("Total_Processor_Time"), 80));
            columnMappings.Add(new ColumnMapping("LAST_INTERVAL_PROCESSOR_TIME", getDisplayColumnName("Last Interval_Processor_Time"), 80));
            columnMappings.Add(new ColumnMapping("DELTA_PROCESSOR_TIME", getDisplayColumnName("Delta_Processor_Time"), 80));
            columnMappings.Add(new ColumnMapping("SQL_CPU_TIME_SEC", getDisplayColumnName("SQL_CPU Time_Seconds"), 80));
            columnMappings.Add(new ColumnMapping("PROCESS_BUSYTIME_SEC", getDisplayColumnName("Process_BusyTime_Seconds"), 80));
            columnMappings.Add(new ColumnMapping("OPEN_TIME_SEC", getDisplayColumnName("Open_Time_Seconds"), 80));
            columnMappings.Add(new ColumnMapping("PROCESS_CREATE_TIME_SEC", getDisplayColumnName("Process_CreateTime_Seconds"), 80));

            columnMappings.Add(new ColumnMapping("TOTAL_MEM_ALLOC", getDisplayColumnName("Total_Memory_Allocated"), 80));
            columnMappings.Add(new ColumnMapping("MAX_MEM_USED", getDisplayColumnName("Total_Memory_Used"), 80));
            columnMappings.Add(new ColumnMapping("NUM_SQL_PROCESSES", getDisplayColumnName("Number_Sql_Processes"), 80));
            columnMappings.Add(new ColumnMapping("PROCESSES_CREATED", getDisplayColumnName("Process_Created"), 80));
            columnMappings.Add(new ColumnMapping("QUERY_ID", getDisplayColumnName("Query_ID"), 150));
            columnMappings.Add(new ColumnMapping("QUERY_TEXT", getDisplayColumnName("Query_Text"), 200));

            columnMappings.Add(new ColumnMapping("CMP_NUMBER_OF_BMOS", getDisplayColumnName("Compilation_Number_Of_BMOS"), 80));
            columnMappings.Add(new ColumnMapping("CMP_OVERFLOW_MODE", getDisplayColumnName("Compilation_Overflow_Mode"), 80));
            columnMappings.Add(new ColumnMapping("CMP_OVERFLOW_SIZE", getDisplayColumnName("Compilation_Overflow Size_(KB)"), 80));

            columnMappings.Add(new ColumnMapping("OVF_FILE_COUNT", getDisplayColumnName("Overflow_File_Count"), 80));
            columnMappings.Add(new ColumnMapping("OVF_SPACE_ALLOCATED", getDisplayColumnName("Overflow_Space_Allocated_(KB)"), 80));
            columnMappings.Add(new ColumnMapping("OVF_SPACE_USED", getDisplayColumnName("Overflow_Space_Used_(KB)"), 80));
            columnMappings.Add(new ColumnMapping("OVF_BLOCK_SIZE", getDisplayColumnName("Overflow_Block_Size"), 80));
            columnMappings.Add(new ColumnMapping("OVF_WRITE_READ_COUNT", getDisplayColumnName("Overflow_Write_Read_Count"), 80));
            columnMappings.Add(new ColumnMapping("OVF_WRITE_COUNT", getDisplayColumnName("Overflow_Write_Count"), 80));
            columnMappings.Add(new ColumnMapping("OVF_BUFFER_BLOCKS_WRITTEN", getDisplayColumnName("Overflow_Buffer_Blocks_Written"), 80));
            columnMappings.Add(new ColumnMapping("OVF_BUFFER_BYTES_WRITTEN", getDisplayColumnName("Overflow_Buffer_Bytes_Written_(KB)"), 80));
            columnMappings.Add(new ColumnMapping("OVF_READ_COUNT", getDisplayColumnName("Overflow_Read_Count"), 80));
            columnMappings.Add(new ColumnMapping("OVF_BUFFER_BLOCKS_READ", getDisplayColumnName("Overflow_Buffer_Blocks_Read"), 80));
            columnMappings.Add(new ColumnMapping("OVF_BUFFER_BYTES_READ", getDisplayColumnName("Overflow_Buffer_Bytes_Read_(KB)"), 80));
            columnMappings.Add(new ColumnMapping("SQL_CPU_OFFENDER_INTERVAL_SECS", getDisplayColumnName("CPU Offender_Interval (secs)"), 80));
            columnMappings.Add(new ColumnMapping("SQL_TSE_OFFENDER_INTERVAL_SECS", getDisplayColumnName("TSE Offender_Interval (secs)"), 80));
            columnMappings.Add(new ColumnMapping("SQL_SLOW_OFFENDER_INTERVAL_SECS", getDisplayColumnName("Slow Offender_Interval (secs)"), 80));

            MonitorWorkloadConfig.DataProviderConfig.ColumnMappings = columnMappings;
            //ColumnMapping.Synchronize(MonitorWorkloadConfig.DataProviderConfig.ColumnMappings, persistedColumnMappings);

            //Make this data provider's timer to continue after encountered an error. 
            MonitorWorkloadConfig.DataProviderConfig.TimerContinuesOnError = true;

            //Make this data provider's timer to continue after encountered an error. 
            MonitorWorkloadConfig.DataProviderConfig.TimerContinuesOnError = true;

            //Set the connection definition if available
            MonitorWorkloadConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;
            MonitorWorkloadConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.Show;
            MonitorWorkloadConfig.ShowExportButtons = true;
            MonitorWorkloadConfig.ShowHelpButton = true;

            if (_isParentChildQueries)
            {
                MonitorWorkloadConfig.HelpTopic = HelpTopics.ParentChildQuery;
            }
            else
            {
                MonitorWorkloadConfig.HelpTopic = HelpTopics.LiveView;
            }

            //Create a UW using the configuration             
            MonitorWorkloadWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)MonitorWorkloadWidget.DataDisplayControl).LineCountFormat = "Queries";
            MonitorWorkloadWidget.DataProvider = new MonitorWorkloadDataProvider(_theConnectionDefinition, MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig);
            MonitorWorkloadWidget.UniversalWidgetConfiguration = MonitorWorkloadConfig;

            //Add the widget to the canvas
            //GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            //_theCanvas.AddWidget(MonitorWorkloadWidget, MonitorWorkloadConfig.Name, MonitorWorkloadConfig.Title, gridConstraint, -1);
            MonitorWorkloadWidget.Dock = DockStyle.Fill;
            _theCanvas.Controls.Add(MonitorWorkloadWidget);


            //Add custom toolstrip buttons
            AddToolStripButtons();

            //Add popup menu items to the table
            TabularDataDisplayControl dataDisplayControl = MonitorWorkloadWidget.DataDisplayControl as TabularDataDisplayControl;

            if (dataDisplayControl != null)
            {
                _workloadDetailMenuItem = new TrafodionIGridToolStripMenuItem();
                _workloadDetailMenuItem.Text = "Workload Detail...";
                _workloadDetailMenuItem.Enabled = false;
                _workloadDetailMenuItem.Click += new EventHandler(workloadDetailMenuItem_Click);
                dataDisplayControl.AddMenuItem(_workloadDetailMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                if (File.Exists("Plugins\\Trafodion.Manager.QPV.dll"))
                {
                    _queryProgressVisualizerMenuItem = new TrafodionIGridToolStripMenuItem();
                    _queryProgressVisualizerMenuItem.Text = "Visualize Query Progress...";
                    _queryProgressVisualizerMenuItem.Enabled = true;
                    _queryProgressVisualizerMenuItem.Click += new EventHandler(_queryProgressVisualizerMenuItem_Click);
                    dataDisplayControl.AddMenuItem(_queryProgressVisualizerMenuItem);
                }

                if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    if (!_isParentChildQueries)
                    {
                        _displayParentChildQueriesMenuItem = new TrafodionIGridToolStripMenuItem();
                        _displayParentChildQueriesMenuItem.Text = "Display Parent/Child Queries...";
                        _displayParentChildQueriesMenuItem.Enabled = false;
                        _displayParentChildQueriesMenuItem.Click += new EventHandler(DisplayParentChildQueriesMenuItem_Click);
                        dataDisplayControl.AddMenuItem(_displayParentChildQueriesMenuItem);
                    }
                }

                _displaySQLTextMenuItem = new TrafodionIGridToolStripMenuItem();
                _displaySQLTextMenuItem.Text = "Display Full SQL Text...";
                _displaySQLTextMenuItem.Enabled = false;
                _displaySQLTextMenuItem.Click += new EventHandler(displaySQLTextMenuItem_Click);
                dataDisplayControl.AddMenuItem(_displaySQLTextMenuItem);

#if SQL_PLAN
                _displaySQLPlanMenuItem = new TrafodionIGridToolStripMenuItem();
                _displaySQLPlanMenuItem.Text = "Display SQL Plan...";
                _displaySQLPlanMenuItem.Enabled = false;
                _displaySQLPlanMenuItem.Click += new EventHandler(displaySQLPlanMenuItem_Click);
                dataDisplayControl.AddMenuItem(_displaySQLPlanMenuItem);
#endif
                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());
				
                _cancelSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _cancelSelectedQueryMenuItem.Text = "Cancel Selected Queries...";
                _cancelSelectedQueryMenuItem.Enabled = false;
                _cancelSelectedQueryMenuItem.Click += new EventHandler(cancelSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_cancelSelectedQueryMenuItem);

                _holdSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _holdSelectedQueryMenuItem.Text = "Hold Selected Queries...";
                _holdSelectedQueryMenuItem.Enabled = false;
                _holdSelectedQueryMenuItem.Click += new EventHandler(holdSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_holdSelectedQueryMenuItem);

                _releaseSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _releaseSelectedQueryMenuItem.Text = "Release Selected Queries...";
                _releaseSelectedQueryMenuItem.Enabled = false;
                _releaseSelectedQueryMenuItem.Click += new EventHandler(releaseSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_releaseSelectedQueryMenuItem);

                _reprepareSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _reprepareSelectedQueryMenuItem.Text = "Reprepare Selected Query...";
                _reprepareSelectedQueryMenuItem.Enabled = false;
                _reprepareSelectedQueryMenuItem.Click += new EventHandler(reprepareSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_reprepareSelectedQueryMenuItem);

                _loadHistoryMenuItem = new TrafodionIGridToolStripMenuItem();
                _loadHistoryMenuItem.Text = "Get History for Selected Queries...";
                _loadHistoryMenuItem.Enabled = false;
                _loadHistoryMenuItem.Click += new EventHandler(_loadHistoryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_loadHistoryMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                //_loadTextSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                //_loadTextSelectedQueryMenuItem.Text = "Load Text for Selected Query...";
                //_loadTextSelectedQueryMenuItem.Enabled = false;
                //_loadTextSelectedQueryMenuItem.Click += new EventHandler(loadTextSelectedQueryMenuItem_Click);
                //dataDisplayControl.AddMenuItem(_loadTextSelectedQueryMenuItem);

                //_loadPlanSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                //_loadPlanSelectedQueryMenuItem.Text = "Load Plan for Selected Query...";
                //_loadPlanSelectedQueryMenuItem.Enabled = false;
                //_loadPlanSelectedQueryMenuItem.Click += new EventHandler(loadPlanSelectedQueryMenuItem_Click);
                //dataDisplayControl.AddMenuItem(_loadPlanSelectedQueryMenuItem);

                //dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                _warnInfoMenuItem = new TrafodionIGridToolStripMenuItem();
                _warnInfoMenuItem.Text = "Warn Info...";
                _warnInfoMenuItem.Enabled = false;
                _warnInfoMenuItem.Click += new EventHandler(warnInfoMenuItem_Click);
                dataDisplayControl.AddMenuItem(_warnInfoMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                _reorgProgressMenuItem = new TrafodionIGridToolStripMenuItem();
                _reorgProgressMenuItem.Text = Properties.Resources.ReorgProgressMenuItemText;
                _reorgProgressMenuItem.Enabled = false;
                _reorgProgressMenuItem.Click += new EventHandler(reorgProgressMenuItem_Click);
                dataDisplayControl.AddMenuItem(_reorgProgressMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                _rulesAssociatedMenuItem = new TrafodionIGridToolStripMenuItem();
                _rulesAssociatedMenuItem.Text = "Rules Associated...";
                _rulesAssociatedMenuItem.Enabled = false;
                _rulesAssociatedMenuItem.Click += new EventHandler(rulesAssociatedMenuItem_Click);
                dataDisplayControl.AddMenuItem(_rulesAssociatedMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

#if REPO_INFO
                _repositoryInfoMenuItem = new TrafodionIGridToolStripMenuItem();
                _repositoryInfoMenuItem.Text = "Repository Info...";
                _repositoryInfoMenuItem.Enabled = false;
                _repositoryInfoMenuItem.Click += new EventHandler(repositoryInfoMenuItem_Click);
                dataDisplayControl.AddMenuItem(_repositoryInfoMenuItem);
#endif

                _processDetailMenuItem = new TrafodionIGridToolStripMenuItem();
                _processDetailMenuItem.Text = "Master Process Detail...";
                _processDetailMenuItem.Enabled = false;
                _processDetailMenuItem.Click += new EventHandler(processDetailMenuItem_Click);
                dataDisplayControl.AddMenuItem(_processDetailMenuItem);

                _pstateMenuItem = new TrafodionIGridToolStripMenuItem();
                _pstateMenuItem.Text = "Master Pstate...";
                _pstateMenuItem.Enabled = false;
                _pstateMenuItem.Click += new EventHandler(pstateMenuItem_Click);
                dataDisplayControl.AddMenuItem(_pstateMenuItem);

                _childrenProcessesMenuItem = new TrafodionIGridToolStripMenuItem();
                _childrenProcessesMenuItem.Text = "Children Processes...";
                _childrenProcessesMenuItem.Enabled = false;
                _childrenProcessesMenuItem.Click += new EventHandler(childrenProcessesMenuItem_Click);
                dataDisplayControl.AddMenuItem(_childrenProcessesMenuItem);
            }

            //Add event handlers to deal with data provider events
            AddHandlers();

            //Associate the custom data display handler for the TabularDisplay panel
            MonitorWorkloadWidget.DataDisplayControl.DataDisplayHandler = new MyMonitorDataHandler(this);
            
            _monitorWorkloadIGrid = ((TabularDataDisplayControl)MonitorWorkloadWidget.DataDisplayControl).DataGrid;

            if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
            {
                _monitorWorkloadIGrid.RowHeader.Visible = true;
                _monitorWorkloadIGrid.CustomDrawRowHdrForeground += new iGCustomDrawRowHdrForegroundEventHandler(Grid_CustomDrawRowHdrForeground);
            }

            _monitorWorkloadIGrid.CellMouseDown += new iGCellMouseDownEventHandler(_monitorWorkloadIGrid_CellMouseDown);
            _monitorWorkloadIGrid.SelectionChanged += new EventHandler(_monitorWorkloadIGrid_SelectionChanged);
            _monitorWorkloadIGrid.DoubleClickHandler = this._monitorWorkloadIGrid_DoubleClick;

            List<string> alwaysHiddenColumnNames = new List<string>();
            alwaysHiddenColumnNames.Add("ForegroundColor");
            alwaysHiddenColumnNames.Add("BackgroundColor");
            alwaysHiddenColumnNames.Add("ViolatorColor");
            alwaysHiddenColumnNames.Add("ViolatorNames");
            alwaysHiddenColumnNames.Add("ElapsedTimeTicks");
            alwaysHiddenColumnNames.Add("WaitTimeTicks");
            _monitorWorkloadIGrid.AlwaysHiddenColumnNames.AddRange(alwaysHiddenColumnNames);

            //Start fetching data from WMS
            //MonitorWorkloadWidget.StartDataProvider();
            ClientQueryRuler.Instance.LoadRulesFromFolder();
            ClientQueryRuler.Instance.Refresh();

        }

        private bool IsParentQueryRow(iGRow gridRow)
        {
            bool isParentQueryRow = false;

            string parentQueryId = (string)gridRow.Cells["PARENT_QUERY_ID"].Value;
            int totalChildCount = (int)gridRow.Cells["TOTAL_CHILD_COUNT"].Value;
            if (parentQueryId != null)
            {
                if ((parentQueryId.Trim().Length == 0 || string.Compare(parentQueryId, "NONE", true) == 0)
                    && totalChildCount > 0)
                {
                    isParentQueryRow = true;
                }
            }

            return isParentQueryRow;
        }

        private Color GetConfiguredColor(string queryState)
        {
            Color color = Color.Blue;

            if (queryState.Equals(WmsCommand.QUERY_STATE_EXECUTING))
            {
                color = _monitorWorkloadOptions.ExecutingColor;
            }
            else if (queryState.Equals(WmsCommand.QUERY_STATE_WAITING))
            {
                color = _monitorWorkloadOptions.WaitingColor;
            }
            else if (queryState.Equals(WmsCommand.QUERY_STATE_HOLDING))
            {
                color = _monitorWorkloadOptions.HoldingColor;
            }
            else if (queryState.Equals(WmsCommand.QUERY_STATE_SUSPENDED))
            {
                color = _monitorWorkloadOptions.SuspendedColor;
            }
            else if (queryState.Equals(WmsCommand.QUERY_STATE_REJECTED))
            {
                color = _monitorWorkloadOptions.RejectedColor;
            }
            else if (queryState.Equals(WmsCommand.QUERY_STATE_COMPLETED))
            {
                color = _monitorWorkloadOptions.CompletedColor;
            }

            return color;
        }

        void Grid_CustomDrawRowHdrForeground(object sender, iGCustomDrawRowHdrForegroundEventArgs e)
        {
            iGrid grid = (iGrid)sender;
            iGRow gridRow = grid.Rows[e.RowIndex];
            if (gridRow != null && IsParentQueryRow(gridRow))
            {
                string queryState = gridRow.Cells[WmsCommand.COL_QUERY_STATE].Text;
                Color foreColor = GetConfiguredColor(queryState);
                e.Graphics.DrawString("P", new Font(Font.FontFamily, Font.Size, FontStyle.Bold), new SolidBrush(foreColor), new PointF(e.GlyphAreaBounds.X, e.GlyphAreaBounds.Y));
            }
            e.DoDefault = false;
        }

        void _loadHistoryMenuItem_Click(object sender, EventArgs e)
        {
            _loadHistoryButton.PerformClick();
        }

        void _monitorWorkloadIGrid_SelectionChanged(object sender, EventArgs e)
        {
            //Enable the load session and load triage buttons only if atleast one row is selected.
            _loadSessionButton.Enabled = _loadTriageButton.Enabled = _loadHistoryButton.Enabled = (_monitorWorkloadIGrid.SelectedRowIndexes.Count > 0);
        }

        void computeRowHeightForSQLPreview()
        {
            iGrid3RowTextColCellStyle = new iGCellStyle();
			this.iGrid3RowTextColCellStyle.ContentIndent = new iGIndent(3, 3, 3, 3);
			this.iGrid3RowTextColCellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
			this.iGrid3RowTextColCellStyle.Font = new System.Drawing.Font("Tahoma", 8.25F, FontStyle.Regular);
			this.iGrid3RowTextColCellStyle.ForeColor = Color.Gray;

            TrafodionIGrid tempGrid = new TrafodionIGrid();
            TenTec.Windows.iGridLib.iGCol sqlTextCol = tempGrid.Cols.Add("SQL_TEXT", "SQL_TEXT");

            TenTec.Windows.iGridLib.iGRow row = tempGrid.Rows.Add();
			iGCell cell = row.Cells[0];

            cell.Value = "SET SCHEMA  TRAFODION.USER_SCHEMA";

            tempGrid.RowTextVisible = true;
            row.RowTextCell.Style = tempGrid.GroupRowLevelStyles[0];

            row.RowTextCell.Value = " First Line #1 " + Environment.NewLine + " Second Line #2 ";
			row.AutoHeight();
			this._iGridRowHeightWithPreview = (int)(1.15 * row.Height);

            tempGrid.RowTextVisible = false;
			row.AutoHeight();
			this._iGridNormalRowHeight = (int)(1.15 * row.Height);
        }

        void _monitorWorkloadIGrid_DoubleClick(int row)
        {
            invokeWorkloadDetail(row);
        }

        void _monitorWorkloadIGrid_CellMouseDown(object sender, iGCellMouseDownEventArgs e)
        {
            if (e.RowIndex >= 0 && e.Button == MouseButtons.Right)
            {
                iGRow row = _monitorWorkloadIGrid.Rows[e.RowIndex];
                iGRowCellCollection coll = row.Cells;

                if (e.ColIndex >= 0)
                {
                    row.Cells[e.ColIndex].Selected = true;
                }
                string query_id = (string)coll[WmsCommand.COL_QUERY_ID].Value;
                string query_state = (string)coll[WmsCommand.COL_QUERY_STATE].Value;
                bool isQueryIdValid = query_id.Length > 0;
                _reorgProgressMenuItem.Enabled 
                    = _workloadDetailMenuItem.Enabled 
                    = _loadHistoryMenuItem.Enabled
                    = isQueryIdValid;

                if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    if (!_isParentChildQueries)
                    {
                        _displayParentChildQueriesMenuItem.Enabled = isQueryIdValid;
                    }
                }

                _displaySQLTextMenuItem.Enabled = isQueryIdValid;
#if SQL_PLAN
                _displaySQLPlanMenuItem.Enabled = isQueryIdValid;
#endif
                _warnInfoMenuItem.Enabled = isQueryIdValid;
                _rulesAssociatedMenuItem.Enabled = isQueryIdValid;

                //We are not enabling these emnu items till WMS is capable of providing the 
                //details from the backend
                _processDetailMenuItem.Enabled = isQueryIdValid;
                _pstateMenuItem.Enabled = isQueryIdValid;
                _childrenProcessesMenuItem.Enabled = isQueryIdValid;

                _cancelSelectedQueryMenuItem.Enabled =
                    ! (query_state.Equals(WmsCommand.QUERY_STATE_COMPLETED) || query_state.Equals(WmsCommand.QUERY_STATE_REJECTED));

                _reprepareSelectedQueryMenuItem.Enabled= query_state.Equals(WmsCommand.QUERY_STATE_HOLDING)
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_REPREPARE.ToString());

                _holdSelectedQueryMenuItem.Enabled= (query_state.Equals(WmsCommand.QUERY_STATE_EXECUTING) || query_state.Equals(WmsCommand.QUERY_STATE_WAITING))
                    && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());

                if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    _releaseSelectedQueryMenuItem.Enabled = (query_state.Equals(WmsCommand.QUERY_STATE_HOLDING) || query_state.Equals(WmsCommand.QUERY_STATE_SUSPENDED) || query_state.Equals(WmsCommand.QUERY_STATE_WAITING))
                        && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());
                }
                else
                {
                    _releaseSelectedQueryMenuItem.Enabled = (query_state.Equals(WmsCommand.QUERY_STATE_HOLDING) || query_state.Equals(WmsCommand.QUERY_STATE_SUSPENDED))
                        && this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());
                }

                if (File.Exists("Plugins\\Trafodion.Manager.QPV.dll") && _queryProgressVisualizerMenuItem != null)
                {
                    _queryProgressVisualizerMenuItem.Visible = false;
                    if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
                    {
                        _queryProgressVisualizerMenuItem.Visible = true;
                        _queryProgressVisualizerMenuItem.Enabled = isQueryIdValid;
                    }

                }

            }
        }

        private void AddToolStripButtons()
        {
            if (!this._isParentChildQueries)
            {
                ToolStripButton monitorOptionsButton = new ToolStripButton();
                monitorOptionsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
                monitorOptionsButton.Image = (System.Drawing.Image)Properties.Resources.ConfigureIcon;
                monitorOptionsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                monitorOptionsButton.Name = "monitorOptionsButton";
                monitorOptionsButton.Size = new System.Drawing.Size(23, 22);
                monitorOptionsButton.Text = "Options";
                monitorOptionsButton.Click += new EventHandler(monitorOptionsButton_Click);
                MonitorWorkloadWidget.AddToolStripItem(monitorOptionsButton);
            }

            _clientRuleButton.Text = "Configure Client Thresholds";
            _clientRuleButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            _clientRuleButton.Image = Trafodion.Manager.Properties.Resources.AlterIcon;
            _clientRuleButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _clientRuleButton.Name = "_clientRuleButton";
            _clientRuleButton.Click += new EventHandler(_clientRuleButton_Click);
            //_clientRuleButton.Enabled = false;
            MonitorWorkloadWidget.AddToolStripItem(_clientRuleButton);

            _sqlPreviewButton.Text = "Preview SQL";
            _sqlPreviewButton.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
            _sqlPreviewButton.CheckOnClick = true;
            _sqlPreviewButton.Checked = _monitorWorkloadOptions.SQLPreview;
            _sqlPreviewButton.Image = _sqlPreviewButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            _sqlPreviewButton.ImageScaling = ToolStripItemImageScaling.None;
            _sqlPreviewButton.CheckedChanged += new EventHandler(_sqlPreviewButton_CheckedChanged);
            MonitorWorkloadWidget.AddToolStripItem(_sqlPreviewButton);

            //Add a separator
            ToolStripSeparator toolStripSeparator1 = new ToolStripSeparator();
            toolStripSeparator1.Name = "LoadTriageGetSessionSeparator";
            toolStripSeparator1.Size=new System.Drawing.Size(6,25);
            toolStripSeparator1.Visible = true;
            MonitorWorkloadWidget.AddToolStripItem(toolStripSeparator1);

            if (!_isParentChildQueries)
            {
                ToolStripButton monitorQueryStatesButton = new ToolStripButton();
                monitorQueryStatesButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
                monitorQueryStatesButton.Image = (System.Drawing.Image)Properties.Resources.QueryStats;
                monitorQueryStatesButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                monitorQueryStatesButton.Name = "monitorQueryStatesButton";
                monitorQueryStatesButton.Size = new System.Drawing.Size(23, 22);
                monitorQueryStatesButton.Text = "WMS Statistics";
                monitorQueryStatesButton.ToolTipText = "View the cumulative WMS statistics";
                monitorQueryStatesButton.Click += new EventHandler(monitorQueryStatesButton_Click);
                MonitorWorkloadWidget.AddToolStripItem(monitorQueryStatesButton);

                //Add Get Session Button
                _loadSessionButton = new ToolStripButton();
                _loadSessionButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
                _loadSessionButton.Image = (System.Drawing.Image)Trafodion.Manager.Properties.Resources.GetSessionIcon;
                _loadSessionButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                _loadSessionButton.Name = "getSessionButton";
                _loadSessionButton.Size = new System.Drawing.Size(23, 22);
                _loadSessionButton.Text = "Load Session";
                _loadSessionButton.ToolTipText = "Load the Triage Space with the entire session for the selected queries";
                _loadSessionButton.Enabled = false;
                _loadSessionButton.Click += new EventHandler(getSessionButton_Click);
                MonitorWorkloadWidget.AddToolStripItem(_loadSessionButton);

                //Add Load Triage Button
                _loadTriageButton = new ToolStripButton();
                _loadTriageButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
                _loadTriageButton.Image = (System.Drawing.Image)Trafodion.Manager.Properties.Resources.LoadTriageIcon;
                _loadTriageButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                _loadTriageButton.Name = "loadTriageButton";
                _loadTriageButton.Size = new System.Drawing.Size(23, 22);
                _loadTriageButton.Text = "Load Triage";
                _loadTriageButton.ToolTipText = "Load the Triage Space with the selected queries";
                _loadTriageButton.Click += new EventHandler(loadTriageButton_Click);
                _loadTriageButton.Enabled = false;
                MonitorWorkloadWidget.AddToolStripItem(_loadTriageButton);
            }

            //Add Load History Button
            _loadHistoryButton = new ToolStripButton();
            _loadHistoryButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _loadHistoryButton.Image = (System.Drawing.Image)Trafodion.Manager.Properties.Resources.HistoryIcon;
            _loadHistoryButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _loadHistoryButton.Name = "loadHistoryButton";
            _loadHistoryButton.Size = new System.Drawing.Size(23, 22);
            _loadHistoryButton.Text = "Get History";
            _loadHistoryButton.ToolTipText = "Get past occurrences of the selected queries";
            _loadHistoryButton.Click += new EventHandler(_loadHistoryButton_Click);
            _loadHistoryButton.Enabled = false;
            MonitorWorkloadWidget.AddToolStripItem(_loadHistoryButton);
            
            if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
            {
                ToolStripButton getTransButton = new ToolStripButton();
                getTransButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
                getTransButton.Image = (System.Drawing.Image)Properties.Resources.GetTrans;
                getTransButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                getTransButton.Name = "getTransButton";
                getTransButton.Size = new System.Drawing.Size(23, 22);
                getTransButton.Text = "Get Trans";
                getTransButton.ToolTipText = Properties.Resources.GetTransactionToolTip;
                getTransButton.Click += new EventHandler(getTransButton_Click);
                MonitorWorkloadWidget.AddToolStripItem(getTransButton);
            }
        }

        void _loadHistoryButton_Click(object sender, EventArgs e)
        {
            if (this._monitorWorkloadIGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int index in this._monitorWorkloadIGrid.SelectedRowIndexes)
                {
                    string queryText = _monitorWorkloadIGrid.Cells[index, WmsCommand.COL_QUERY_TEXT].Value as string;
                    if (string.IsNullOrEmpty(queryText)) //If query text is null or blank, no details to fetch.
                        return;

                    string title = "Workload History : " + queryText;
                    if (!string.IsNullOrEmpty(title))
                    {
                        title = title.Replace("\n", "");
                        title = title.Replace("\r", "");
                    }
                    ManagedWindow workloadHistory = WindowsManager.GetManagedWindow(TrafodionForm.TitlePrefix + this.ConnectionDefn.Name + " : " + title);
                    if (workloadHistory != null)
                    {
                        workloadHistory.BringToFront();
                    }
                    else
                    {
                        WorkloadHistoryControl whc = new WorkloadHistoryControl(ConnectionDefn, queryText);
                        WindowsManager.PutInWindow(new Size(800, 600), whc, title, this.ConnectionDefn);
                    } 
                }

            }            
        }

        void _sqlPreviewButton_CheckedChanged(object sender, EventArgs e)
        {
            if (!_isParentChildQueries)
            {
                _monitorWorkloadOptions.SQLPreview = _sqlPreviewButton.Checked;
            }

            _sqlPreviewButton.Image = _sqlPreviewButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            showHidePreviewSQL(_sqlPreviewButton.Checked);
        }

        void monitorOptionsButton_Click(object sender, EventArgs e)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            MonitorWorkloadFilter workloadFilter = new MonitorWorkloadFilter(_monitorWorkloadOptions);
            DialogResult result = workloadFilter.ShowDialog();
            if (result == DialogResult.OK)
            {
                dbConfig.SQLText = QueryCommand;
                _sqlPreviewButton.Checked = _monitorWorkloadOptions.SQLPreview;
                this.MonitorWorkloadWidget.StartDataProvider();
            }
        }

        void _clientRuleButton_Click(object sender, EventArgs e) 
        {            
            ClientQueryRuler.Instance.OpenRuleManager();
        }


        void monitorQueryStatesButton_Click(object sender, EventArgs e)
        {
            //if (_theConnectionDefinition != null && _theConnectionDefinition.IsWmsAdminRole)
            if (_theConnectionDefinition != null && _theConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
            {
                DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
                ManagedWindow wmsStats = WindowsManager.GetManagedWindow(TrafodionForm.TitlePrefix + dbConfig.ConnectionDefinition.Name + " : " + "Cumulative WMS Statistics");
                if (wmsStats != null)
                {
                    wmsStats.BringToFront();
                    wmsStats.Refresh();
                }
                else
                {
                    MonitorWorkloadQueryStats dlg = new MonitorWorkloadQueryStats(this, dbConfig.ConnectionDefinition);
                    WindowsManager.PutInWindow(new Size(800, 600), dlg, "Cumulative WMS Statistics", dbConfig.ConnectionDefinition);
                }
            }
            else
            {
                MessageBox.Show("You do not have the required component privilege to view WMS statistics. Please contact your administrator.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        void getTransButton_Click(object sender, EventArgs e)
        {
            TransactionUserControl parentChildQueriesUserControl = new TransactionUserControl(_theConnectionDefinition);
            Utilities.LaunchManagedWindow(Properties.Resources.TitleTransactionsOfAllNodes, parentChildQueriesUserControl, _theConnectionDefinition, TransactionUserControl.IdealWindowSize, true);
        }

        void getSessionButton_Click(object sender, EventArgs e) 
        {
            GetSessionEvent();
        }

        //The Event is registered in TabbedWorkloadUserControlWrapper
        //And the real method is in TriageGridUserControl
        void loadTriageButton_Click(object sender, EventArgs e) 
        {
            LoadQueriesToTriageSpaceEvent();
        }

        private void AddHandlers()
        {
            if (MonitorWorkloadWidget != null && MonitorWorkloadWidget.DataProvider != null)
            {
                //Associate the event handlers
                //MonitorWorkloadWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                MonitorWorkloadWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                MonitorWorkloadWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }

        private void RemoveHandlers()
        {
            if (MonitorWorkloadWidget != null && MonitorWorkloadWidget.DataProvider != null)
            {
                //Remove the event handlers
                //MonitorWorkloadWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                MonitorWorkloadWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                MonitorWorkloadWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
            SetTitle(e as DataProviderEventArgs);
        }

        private void showHidePreviewSQL(bool previewSQLText)
        {
            if (previewSQLText)
            {
                _monitorWorkloadIGrid.RowMode = true;
                _monitorWorkloadIGrid.RowTextVisible = true;
                _monitorWorkloadIGrid.RowTextStartColNear = 1;
            }
            else
            {
                _monitorWorkloadIGrid.RowMode = false;
                _monitorWorkloadIGrid.RowTextVisible = false;
            }

            foreach (iGRow row in _monitorWorkloadIGrid.Rows)
            {
                if (previewSQLText)
                {
                    row.Height = this._iGridRowHeightWithPreview;
                    string sqlPreview = row.Cells[WmsCommand.COL_QUERY_TEXT].Value.ToString();
                    sqlPreview = sqlPreview.Replace("\r\n", " ");
                    sqlPreview = sqlPreview.Replace("\r", " ");
                    sqlPreview = sqlPreview.Replace("\n", " ");
                    row.RowTextCell.Value = sqlPreview;
                    row.RowTextCell.Style = iGrid3RowTextColCellStyle;
                    row.RowTextCell.ForeColor = this._monitorWorkloadOptions.SQLPreviewColor;
                }
                else
                {
                    row.Height = _iGridNormalRowHeight;
                }
            }
        }

        public void HighlightQuery(String queryId, String queryStartTime)
        {
            if (null != this._highlightedQuery)
            {
                //Set 'original' background color to restore to
                Color originalBackCol = Color.White;

                //if there is no background color setting assigned by the Rules filter
                try
                {
                    if (this.ClientRulesEnabled && !this._highlightedQuery.Cells["BackgroundColor"].Text.Equals(null))
                    {
                        string backCol = ((string)this._highlightedQuery.Cells["BackgroundColor"].Text);
                        if (backCol != null && backCol != "")
                            originalBackCol = Color.FromArgb(int.Parse(backCol));
                    }

                }
                catch (Exception)
                {
                }

                //Update the background color, then re-apply cell specific color settings
                TriageGridDataDisplayHandler.ChangeDataRowBackColor(this._highlightedQuery, originalBackCol);
                TriageGridDataDisplayHandler.CheckChangeCellRowColor(this._highlightedQuery);
            }

            this._highlightedQuery = null;


            if (null == queryId)
                return;


            int cnt = _monitorWorkloadIGrid.Rows.Count;
            for (int i = 0; i < _monitorWorkloadIGrid.Rows.Count; i++)
            {
                String qId = _monitorWorkloadIGrid.Rows[i].Cells["QUERY_ID"].Value.ToString();
                String qStartTime = _monitorWorkloadIGrid.Rows[i].Cells[WmsCommand.COL_QUERY_START_TIME].Value.ToString();
                if (qId.Trim().Equals(queryId.Trim()) && qStartTime.Trim().Equals(queryStartTime.Trim()))
                {
                    this._highlightedQuery = _monitorWorkloadIGrid.Rows[i];
                    break;
                }
            }

            if (null != this._highlightedQuery)
            {
                TriageGridDataDisplayHandler.ChangeDataRowBackColor(this._highlightedQuery, Color.Gold);
                _monitorWorkloadIGrid.SetCurRow(-1);
                _monitorWorkloadIGrid.SetCurRow(this._highlightedQuery.Index);
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "End Workload Monitor Fetch", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            } 
            string query = SetTitle(e as DataProviderEventArgs);
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;           

            UpdateWorkloadGridDisplay();

            //Reset the load triage and load session buttons.
            bool enabled = _monitorWorkloadIGrid.SelectedRowIndexes.Count > 0;
            if (!_isParentChildQueries)
            {
                _loadTriageButton.Enabled = _loadSessionButton.Enabled = enabled;
            }

            _loadHistoryButton.Enabled = enabled;

            if (_monitorWorkloadOptions.HighLightChanges && 
                _previousMonitorWorkloadIGrid != null && _previousMonitorWorkloadIGrid.Rows.Count > 0)
            {
                highlightChanges(_previousMonitorWorkloadIGrid, _monitorWorkloadIGrid);
            }
            if (_previousDataTable != null)
            {
                _previousMonitorWorkloadIGrid.FillWithData(_previousDataTable);
            }

            string[] watchedQueries = GetWatchedQueryList(dbConfig.ConnectionDefinition);
            foreach (string queryId in watchedQueries)
            {
                string[] ids = queryId.Split('@');
                string qid = ids[0];
                string start_ts = (ids.Length > 1) ? ids[1] : "";


                WMSQueryDetailUserControl queryInfo = GetWatchedQueryWindow(dbConfig.ConnectionDefinition, qid, start_ts);
                if (queryInfo != null)
                {
                    DataTable dataTable = ExtractQueryDetails(qid, start_ts);
                    if (dataTable.Rows.Count > 0)
                    {
                        queryInfo.LoadData(dataTable);
                    }
                }
            }

            string[] watchedReorgQueries = GetWatchedReorgQueryList(dbConfig.ConnectionDefinition);
            foreach (string queryId in watchedReorgQueries)
            {
                string[] ids = queryId.Split('@');
                string qid = ids[0];
                string start_ts = (ids.Length > 1) ? ids[1] : "";

                ReorgProgressUserControl reorgWindow = GetWatchedReorgProgressWindow(dbConfig.ConnectionDefinition, qid, start_ts);
                if (reorgWindow != null)
                {
                    reorgWindow.DoRefresh();
                }
            }
        }

        private int getRowFromPreviousIGrid(TrafodionIGrid previousIGrid, string search_qid, string search_start_ts)
        {
            for (int r = 0; r < previousIGrid.Rows.Count; r++)
            {
                string qid = previousIGrid.Cells[r, WmsCommand.COL_QUERY_ID].Value.ToString();
                string start_ts = previousIGrid.Cells[r, WmsCommand.COL_QUERY_START_TIME].Value.ToString();
                if (qid.Equals(search_qid) && start_ts.Equals(search_start_ts))
                {
                    return r;
                }
            }

            return -1;
        }

        private void highlightChanges(TrafodionIGrid previousIGrid, TrafodionIGrid currentIGrid)
        {
            for (int r = 0; r < currentIGrid.Rows.Count; r++)
            {
                string qid = currentIGrid.Cells[r, WmsCommand.COL_QUERY_ID].Text;
                string query_state = currentIGrid.Cells[r, WmsCommand.COL_QUERY_STATE].Text;
                string start_ts = currentIGrid.Cells[r, WmsCommand.COL_QUERY_START_TIME].Text;

                if (!_monitorWorkloadOptions.QueryStates.Contains(query_state))
                {
                    continue;
                }

                int rowIndex = getRowFromPreviousIGrid(previousIGrid, qid, start_ts);
                if (rowIndex != -1)
                {
                    int count = previousIGrid.Cols.Count;
                    for (int c = 0; c < currentIGrid.Cols.Count; c++)
                    {
                        //don't overwrite WARN_LEVEL and QUERY_STATE column colors
                        string colName = currentIGrid.Cols[c].Text.ToString();
                        if (colName.Equals("WARN" + Environment.NewLine + "LEVEL") ||
                            colName.Equals("QUERY" + Environment.NewLine + "STATE"))
                        {
                            continue;
                        }

                        if (currentIGrid.Cols[c].Visible)
                        {
                            if (!currentIGrid.Cells[r, c].Value.Equals(previousIGrid.Cells[rowIndex, c].Value))
                            {
                                currentIGrid.Cells[r, c].ForeColor = _monitorWorkloadOptions.HighLightChangesColor;
                            }
                        }
                    }
                }
            }
        }

        //sets the title with the query that got executed
        private string SetTitle(DataProviderEventArgs evtArgs)
        {
            if (evtArgs != null)
            {
                SimpleReportDefinition reportDef = evtArgs.GetEventProperty("report_definition") as SimpleReportDefinition;
                if (reportDef != null)
                {
                    string query = reportDef.GetProperty(ReportDefinition.ACTUAL_QUERY) as String;
                    MonitorWorkloadWidget.SetTitle(_theTitle + String.Format(" - ({0})", query));
                    return query;
                }
            }
            else
            {
                MonitorWorkloadWidget.SetTitle(_theTitle);
            }

            return null;
        }

        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        private void HandleFetchingData(Object obj, EventArgs e)
        {
            SetTitle(e as DataProviderEventArgs);
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "Begin Workload Monitor Fetch", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            }
        }

        private void invokeReorgProgress(int row)
        {
            if (row < 0)
                return;

            try
            {                
                string query_id = _monitorWorkloadIGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                if (string.IsNullOrEmpty(query_id)) //If query id null or blank, no details to fetch.
                    return;
                               

                string title = string.Format(Properties.Resources.TitleReorgProgress, query_id);
                                
                ReorgProgressUserControl reorgProgressWindow = GetWatchedReorgWindow(ConnectionDefn, query_id);


                if (reorgProgressWindow != null)
                {
                    string systemIdentifier = (_theConnectionDefinition != null) ? _theConnectionDefinition.Name + " : " : "";
                    string windowTitle = TrafodionForm.TitlePrefix + systemIdentifier + title;
                    if (WindowsManager.Exists(windowTitle))
                    {
                        WindowsManager.Restore(windowTitle);
                        WindowsManager.BringToFront(windowTitle);
                    }
                }
                else
                {
                    Size size = new Size(827, 520); 

                    ReorgProgressUserControl reorgProgress = new ReorgProgressUserControl(this,_theConnectionDefinition, query_id);                    
                    AddQueryToWatchReorg(reorgProgress);

                    Utilities.LaunchManagedWindow(title, reorgProgress, true, _theConnectionDefinition, size, false);
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(),
                    "Unable to obtain reorge progress information for the selected row due to the following exception - " + ex.Message,
                    Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }


        private void invokeWorkloadDetail(int row)
        {
            if (row < 0)
                return;

            try
            {
                DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
                string query_id = _monitorWorkloadIGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                if (string.IsNullOrEmpty(query_id)) //If query id null or blank, no details to fetch.
                    return;

                string mxosrvrStartTime = "";
                if (_monitorWorkloadIGrid.Cols.KeyExists(WmsCommand.COL_QUERY_START_TIME))
                {
                    mxosrvrStartTime = _monitorWorkloadIGrid.Cells[row, WmsCommand.COL_QUERY_START_TIME].Value as string;
                }

                string title = "";
                if (!string.IsNullOrEmpty(mxosrvrStartTime))
                    title = string.Format(Properties.Resources.TitleQueryDetails, string.Format("{0}@{1}", query_id, mxosrvrStartTime));
                else
                    title = string.Format(Properties.Resources.TitleQueryDetails, string.Format("{0}", query_id));

                DataTable dataTable = ExtractQueryDetails(query_id, mxosrvrStartTime);

                //// Just pass the selected row to show query details
                //DataTable sourceDataTable = MonitorWorkloadWidget.DataProvider.GetDataTable();
                //DataTable dataTable = sourceDataTable.Clone();
                //dataTable.Rows.Add(sourceDataTable.Rows[row].ItemArray);
                WMSQueryDetailUserControl queryInfo = GetWatchedQueryWindow(dbConfig.ConnectionDefinition, query_id, mxosrvrStartTime);

                if (dataTable.Rows.Count > 0)
                {
                    if (queryInfo != null)
                    {
                        queryInfo.LoadData(dataTable);
                        string systemIdentifier = (_theConnectionDefinition != null) ? _theConnectionDefinition.Name + " : " : "";
                        string windowTitle = TrafodionForm.TitlePrefix + systemIdentifier + title;
                        if (WindowsManager.Exists(windowTitle))
                        {
                            WindowsManager.Restore(windowTitle);
                            WindowsManager.BringToFront(windowTitle);
                        }
                    }
                    else
                    {
                        WMSQueryDetailUserControl queryDetails = new WMSQueryDetailUserControl(this, _theConnectionDefinition, query_id, mxosrvrStartTime, dataTable);
                        AddQueryToWatch(queryDetails);
                        Utilities.LaunchManagedWindow(title, queryDetails, true, _theConnectionDefinition, WMSQueryDetailUserControl.IdealWindowSize, false);
                    }
                }
                else
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Query statistics not available for the selected query in WMS. It may be that the query has already completed.", Properties.Resources.LiveWorkloads, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), 
                    "Unable to obtain query details information for the selected row due to the following exception - " + ex.Message, 
                    Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private DataTable ExtractQueryDetails(string queryId, string mxosrvrStartTime)
        {
            DataTable dataTable = new DataTable();
            DataRow[] rows = null;

            // Synch the access to MonitorWorkloadDataProvider' DataTable, because some thread else may change it.
            lock (((MonitorWorkloadDataProvider)MonitorWorkloadWidget.DataProvider).SyncRootForFetchData)
            {
                DataTable sourceDataTable = MonitorWorkloadWidget.DataProvider.GetDataTable();
                if (sourceDataTable != null)
                {
                    dataTable = sourceDataTable.Clone();
                }
                if (sourceDataTable.Columns.Contains(WmsCommand.COL_QUERY_START_TIME))
                {
                    rows = sourceDataTable.Select(WmsCommand.COL_QUERY_ID + " = '" + queryId + "' AND " + WmsCommand.COL_QUERY_START_TIME + " = '" + mxosrvrStartTime + "'");
                }
                else
                {
                    rows = sourceDataTable.Select(WmsCommand.COL_QUERY_ID + " = '" + queryId + "'");
                }
            }

            foreach (DataRow row in rows)
            {
                dataTable.Rows.Add(row.ItemArray);
            }

            return dataTable;
        }


        private void invokeWorkloadDetail(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    invokeWorkloadDetail(eventObj.Row);
                }
            }
        }

        private void invokeReorgProgressDetail(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    invokeReorgProgress(eventObj.Row);
                }
            }
        }

        void workloadDetailMenuItem_Click(object sender, EventArgs events)
        {
            invokeWorkloadDetail(sender, events);
        }

        void _queryProgressVisualizerMenuItem_Click(object sender, EventArgs events)
        {
            try
            {
                Assembly qpvAssembly = null;
                foreach (Assembly currAssembly in AppDomain.CurrentDomain.GetAssemblies())
                {
                    if (currAssembly.FullName.Contains("Trafodion.Manager.QPV"))
                    {
                        qpvAssembly = currAssembly;
                        break;
                    }
                }
                if (qpvAssembly == null)
                {
                    qpvAssembly = Assembly.LoadFrom("Plugins\\Trafodion.Manager.QPV.dll");
                }
                if (qpvAssembly != null)
                {
                    Type type = qpvAssembly.GetType("Trafodion.Manager.QPV.QueryProgressIndicatorUserControl");
                    TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
                    TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                    string query_id = _monitorWorkloadIGrid.Cells[eventObj.Row, WmsCommand.COL_QUERY_ID].Value as string;
                    //pass MonitorWorkloadCanvas, ConnectionDefinition, queryId to form constructor
                    ConstructorInfo ci = type.GetConstructor(new Type[3] { typeof(MonitorWorkloadCanvas), typeof(ConnectionDefinition), typeof(string) });
                    object[] argVals = new object[] { this, this.ConnectionDefn, query_id };
                    UserControl QPVControl = (UserControl)ci.Invoke(argVals);
                    Utilities.LaunchManagedWindow("Query Progress Visualizer", QPVControl, true, this.ConnectionDefn, new Size(800, 650), false);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        void DisplayParentChildQueriesMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridEventObject eventObject = ((TrafodionIGridToolStripMenuItem)sender).TrafodionIGridEventObject;
            iGRow eventRow = eventObject.TheGrid.Rows[eventObject.Row];

            string queryId = (string)eventRow.Cells[WmsCommand.COL_QUERY_ID].Value;
            string windowTitle = string.Format(Properties.Resources.TitleParentChildQueries, queryId);
            ParentChildQueriesUserControl parentChildQueriesUserControl = new ParentChildQueriesUserControl(_theConnectionDefinition, queryId, this.ParentForm.ClientSize);
            Utilities.LaunchManagedWindow(windowTitle, parentChildQueriesUserControl, _theConnectionDefinition, ChildrenWindowSize, true);
        }

        void displaySQLTextMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_text = iGrid.Cells[row, WmsCommand.COL_QUERY_TEXT].Value as string;
                    int length = (int)iGrid.Cells[row, "QUERY_TEXT_LEN"].Value;
                    string mxosrvrStartTime = "";
                    if (iGrid.Cols.KeyExists(WmsCommand.COL_QUERY_START_TIME))
                    {
                        mxosrvrStartTime = iGrid.Cells[row, WmsCommand.COL_QUERY_START_TIME].Value as string;
                    }
                    string title = string.Format(Properties.Resources.TitleQueryText, query_id);
                    WorkloadPlanControl wpc = new WorkloadPlanControl(_theConnectionDefinition, query_id, mxosrvrStartTime, query_text, length, false,"",false);
                    Utilities.LaunchManagedWindow(title, wpc, _theConnectionDefinition, ChildrenWindowSize, true);
                }
            }
        }

#if SQL_PLAN
        void displaySQLPlanMenuItem_Click(object sender, EventArgs events)
        {
            //DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_text = iGrid.Cells[row, WmsCommand.COL_QUERY_TEXT].Value as string;
                    int length = (int)iGrid.Cells[row, "QUERY_TEXT_LEN"].Value;
                    string mxosrvrStartTime = "";
                    if (iGrid.Cols.KeyExists(WmsCommand.COL_QUERY_START_TIME))
                    {
                        mxosrvrStartTime = iGrid.Cells[row, WmsCommand.COL_QUERY_START_TIME].Value as string;
                    }
                    string dataSource = "";
                    if (iGrid.Cols.KeyExists("DATASOURCE"))
                    {
                        dataSource = iGrid.Cells[row, "DATASOURCE"].Value as string;
                    }
                    string title = string.Format(Properties.Resources.TitleQueryPlan, query_id);
                    WorkloadPlanControl wpc = new WorkloadPlanControl(_theConnectionDefinition, query_id, mxosrvrStartTime, query_text, length, true, dataSource, true);
                    Utilities.LaunchManagedWindow(title, wpc, _theConnectionDefinition, ChildrenWindowSize, true);
                }
            }
        }
#endif

        void cancelSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            bool queryCancelled = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");

            if (MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to cancel the selected queries?", 
                    "Confirm Cancel", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                Cursor = Cursors.WaitCursor;
                foreach (int rowIndex in _monitorWorkloadIGrid.SelectedRowIndexes)
                {
                    string query_id = _monitorWorkloadIGrid.Cells[rowIndex, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_state = _monitorWorkloadIGrid.Cells[rowIndex, WmsCommand.COL_QUERY_STATE].Value as string;
                    //skip the completed queries
                    if (query_state.Equals(WmsCommand.QUERY_STATE_COMPLETED) || query_state.Equals(WmsCommand.QUERY_STATE_REJECTED))
                        continue;
                    try
                    {
                        Queries.ManageQuery(dbConfig.ConnectionDefinition, query_id, WmsCommand.QUERY_ACTION.CANCEL_QUERY);
                        queryCancelled = true;
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { query_id, ex.Message });
                    }
                }
                Cursor = Cursors.Default;

                if (errorTable.Rows.Count > 0)
                {
                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("One or more queries were not cancelled", errorTable, System.Drawing.SystemIcons.Warning);
                    mmd.ShowDialog();
                }
                if (queryCancelled)
                {
                    dbConfig.SQLText = QueryCommand;
                    this.MonitorWorkloadWidget.StartDataProvider();
                }
            }
        }

        void holdSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            bool queryHeld = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");

            if (MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to place the selected queries on hold?",
                    "Confirm Hold", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                Connection connection = null;
                Cursor = Cursors.WaitCursor;
                foreach (int rowIndex in _monitorWorkloadIGrid.SelectedRowIndexes)
                {
                    string query_id = _monitorWorkloadIGrid.Cells[rowIndex, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_state = _monitorWorkloadIGrid.Cells[rowIndex, WmsCommand.COL_QUERY_STATE].Value as string;
                    
                    //Skip queries that have completed or already on hold
                    if (query_state.Equals(WmsCommand.QUERY_STATE_COMPLETED) || query_state.Equals(WmsCommand.QUERY_STATE_REJECTED) || query_state.Equals(WmsCommand.QUERY_STATE_HOLDING) || query_state.Equals(WmsCommand.QUERY_STATE_SUSPENDED))
                        continue;

                    try
                    {
                        Queries.ManageQuery(dbConfig.ConnectionDefinition, query_id, WmsCommand.QUERY_ACTION.HOLD_QUERY);
                        queryHeld = true;
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { query_id, ex.Message });
                    }
                }
                Cursor = Cursors.Default;

                if (errorTable.Rows.Count > 0)
                {
                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("One or more queries were not placed on hold", errorTable, System.Drawing.SystemIcons.Warning);
                    mmd.ShowDialog();
                }

                if (queryHeld)
                {
                    dbConfig.SQLText = QueryCommand;
                    this.MonitorWorkloadWidget.StartDataProvider();
                }
            }
        }

        void releaseSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            bool queryReleased = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");

            if (MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to release the selected queries?",
                    "Confirm Release", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                Connection connection = null;
                Cursor = Cursors.WaitCursor;
                foreach (int rowIndex in _monitorWorkloadIGrid.SelectedRowIndexes)
                {
                    string query_id = _monitorWorkloadIGrid.Cells[rowIndex, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_state = _monitorWorkloadIGrid.Cells[rowIndex, WmsCommand.COL_QUERY_STATE].Value as string;

                    //Skip queries that have completed or not on hold
                    if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        if (query_state.Equals(WmsCommand.QUERY_STATE_COMPLETED) || query_state.Equals(WmsCommand.QUERY_STATE_REJECTED) || (!query_state.Equals(WmsCommand.QUERY_STATE_HOLDING) && !query_state.Equals(WmsCommand.QUERY_STATE_SUSPENDED) && !query_state.Equals(WmsCommand.QUERY_STATE_WAITING)))
                            continue;
                    }
                    else
                    {
                        if (query_state.Equals(WmsCommand.QUERY_STATE_COMPLETED) || query_state.Equals(WmsCommand.QUERY_STATE_REJECTED) || (!query_state.Equals(WmsCommand.QUERY_STATE_HOLDING) && !query_state.Equals(WmsCommand.QUERY_STATE_SUSPENDED)))
                            continue;
                    }

                    try
                    {
                        Queries.ManageQuery(dbConfig.ConnectionDefinition, query_id, WmsCommand.QUERY_ACTION.RELEASE_QUERY);
                        queryReleased = true;
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { query_id, ex.Message });
                    }
                }
                Cursor = Cursors.Default;

                if (errorTable.Rows.Count > 0)
                {
                    TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("One or more queries were not placed on hold", errorTable, System.Drawing.SystemIcons.Warning);
                    mmd.ShowDialog();
                }

                if (queryReleased)
                {
                    dbConfig.SQLText = QueryCommand;
                    this.MonitorWorkloadWidget.StartDataProvider();
                }
            }
        }

        void reprepareSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_state = iGrid.Cells[row, WmsCommand.COL_QUERY_STATE].Value as string;
                    bool enable = query_state.Equals(WmsCommand.QUERY_STATE_HOLDING) ? true : false;
                    string action = "Reprepare Query";
                    string sql = action + " " + query_id;
                    MonitorSQLCommand monitorCmd = new MonitorSQLCommand(this, dbConfig.ConnectionDefinition, action, sql, enable);
                    DialogResult result = monitorCmd.ShowDialog();
                }
            }
        }

        void loadTextSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                    string action = "Load Query";
                    string sql = action + " " + query_id + " TEXT";
                    MonitorSQLCommand monitorCmd = new MonitorSQLCommand(this, dbConfig.ConnectionDefinition, action, sql, false);
                    DialogResult result = monitorCmd.ShowDialog();
                }
            }
        }

        void loadPlanSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                    string action = "Load Query";
                    string sql = action + " " + query_id + " PLAN";
                    MonitorSQLCommand monitorCmd = new MonitorSQLCommand(this, dbConfig.ConnectionDefinition, action, sql, false);
                    DialogResult result = monitorCmd.ShowDialog();
                }
            }
        }

        void warnInfoMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        string query_id = eventObj.TheGrid.Cells[eventObj.Row, WmsCommand.COL_QUERY_ID].Value as string;
                        string title = string.Format(Properties.Resources.TitleWarningInfo, query_id);
                        WMSQueryWarningUserControl warningInfo = new WMSQueryWarningUserControl(this, _theConnectionDefinition, query_id);
                        Utilities.LaunchManagedWindow(title, warningInfo, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to obtain warning information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        void reorgProgressMenuItem_Click(object sender, EventArgs events)
        {
            invokeReorgProgressDetail(sender, events);
        }
        
        void rulesAssociatedMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        TrafodionIGrid iGrid = eventObj.TheGrid;
                        int row = eventObj.Row;
                        string service_name = iGrid.Cells[row, "SERVICE_NAME"].Value as string;
                        string title = string.Format(Properties.Resources.TitleRulesAssociated, service_name);
                        WMSRulesAssociatedUserControl rules = new WMSRulesAssociatedUserControl(this, _theConnectionDefinition, service_name);
                        Utilities.LaunchManagedWindow(title, rules, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to obtain rules information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }
    
#if REPO_INFO
        void repositoryInfoMenuItem_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, "QUERY_ID"].Value as string;
                    WMSRepositoryInfo repositoryInfo = new WMSRepositoryInfo(this, dbConfig.ConnectionDefinition, query_id);
                    repositoryInfo.Show();
                }
            }
        }
#endif

        void processDetailMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        TrafodionIGrid iGrid = eventObj.TheGrid;
                        int row = eventObj.Row;
                        string prcess_name = iGrid.Cells[row, "PROCESS_NAME"].Value as string;
                        string title = string.Format(Properties.Resources.TitleMasterProcessDetails, prcess_name);
                        WMSProcessDetailsUserControl process = new WMSProcessDetailsUserControl(title, _theConnectionDefinition, prcess_name, true);
                        Utilities.LaunchManagedWindow(title, process, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to obtain process information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        void pstateMenuItem_Click(object sender, EventArgs events)
        {

            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        TrafodionIGrid iGrid = eventObj.TheGrid;
                        int row = eventObj.Row;
                        string prcess_name = iGrid.Cells[row, "PROCESS_NAME"].Value as string;
                        string title = string.Format(Properties.Resources.TitleMasterPstate, prcess_name);
                        WMSPStateUserControl pstate = new WMSPStateUserControl(title, _theConnectionDefinition, prcess_name, true);
                        Utilities.LaunchManagedWindow(title, pstate, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to obtain Pstate information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        void childrenProcessesMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        TrafodionIGrid iGrid = eventObj.TheGrid;
                        int row = eventObj.Row;
                        string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                        string process_name = iGrid.Cells[row, "PROCESS_NAME"].Value as string;
                        string title = string.Format(Properties.Resources.TitleChildrenProcesses, process_name);
                        WMSChildrenProcessesUserControl children = new WMSChildrenProcessesUserControl(this, _theConnectionDefinition, query_id, process_name);
                        Utilities.LaunchManagedWindow(title, children, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to obtain children processes information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
        }

        private void UpdateWorkloadGridDisplay()
        {
            if (this._sqlPreviewButton.Checked)
            {
                _monitorWorkloadIGrid.RowMode = true;
                _monitorWorkloadIGrid.RowTextVisible = true;
                _monitorWorkloadIGrid.RowTextStartColNear = 1;
            }
            else
            {
                _monitorWorkloadIGrid.RowMode = false;
                _monitorWorkloadIGrid.RowTextVisible = false;
            }

            

            for (int row = 0; row < _monitorWorkloadIGrid.Rows.Count; row++)
            {
                string warnLevel = _monitorWorkloadIGrid.Rows[row].Cells["WARN_LEVEL"].Text;
                if (warnLevel.Equals(""))
                {
                    warnLevel = "NOWARN";
                }

                if (warnLevel.Equals("HIGH"))
                {
                    _monitorWorkloadIGrid.Rows[row].BackColor = _monitorWorkloadOptions.WarnHighColor;
                }
                else if (warnLevel.Equals("MEDIUM"))
                {
                    _monitorWorkloadIGrid.Rows[row].BackColor = _monitorWorkloadOptions.WarnMediumColor;
                }
                else if (warnLevel.Equals("LOW"))
                {
                    _monitorWorkloadIGrid.Rows[row].BackColor = _monitorWorkloadOptions.WarnLowColor;
                }
                else if (warnLevel.Equals("NOWARN"))
                {
                    _monitorWorkloadIGrid.Rows[row].BackColor = _monitorWorkloadOptions.NoWarnColor;
                }

                string queryState = _monitorWorkloadIGrid.Rows[row].Cells[WmsCommand.COL_QUERY_STATE].Text;
                _monitorWorkloadIGrid.Rows[row].ForeColor = GetConfiguredColor(queryState);

                bool match = _monitorWorkloadOptions.WarnLevels.Contains(warnLevel);
                _monitorWorkloadIGrid.Rows[row].Visible = (_monitorWorkloadIGrid.Rows[row].Visible && match) ? true : false;

                bool match2 = _monitorWorkloadOptions.QueryStates.Contains(queryState);
                _monitorWorkloadIGrid.Rows[row].Visible = (_monitorWorkloadIGrid.Rows[row].Visible && match2) ? true : false;

                string serviceName = _monitorWorkloadIGrid.Rows[row].Cells[Queries.SERVICE_NAME].Text;
                if (serviceName.Equals(Queries.HPS_MANAGEABILITY))
                {
	                _monitorWorkloadIGrid.Rows[row].Visible = (_monitorWorkloadIGrid.Rows[row].Visible && _monitorWorkloadOptions.ShowManageabilityQueries) ? true : false;
                }

                if (this._sqlPreviewButton.Checked)
                {
                    _monitorWorkloadIGrid.Rows[row].Height = this._iGridRowHeightWithPreview;
                    string sqlPreview = _monitorWorkloadIGrid.Rows[row].Cells[WmsCommand.COL_QUERY_TEXT].Value.ToString();
                    sqlPreview = sqlPreview.Replace("\r\n", " ");
                    sqlPreview = sqlPreview.Replace("\r", " ");
                    sqlPreview = sqlPreview.Replace("\n", " ");
                    _monitorWorkloadIGrid.Rows[row].RowTextCell.Value = sqlPreview;
                    _monitorWorkloadIGrid.Rows[row].RowTextCell.Style = iGrid3RowTextColCellStyle;
                    _monitorWorkloadIGrid.Rows[row].RowTextCell.ForeColor = this._monitorWorkloadOptions.SQLPreviewColor;
                }
                else
                {
                    _monitorWorkloadIGrid.Rows[row].Height = _iGridNormalRowHeight;
                }

                //for (int a = 0; a < _monitorWorkloadIGrid.Rows[row].Cells.Count; a++ )
                //{
                //    Debug.WriteLine(_monitorWorkloadIGrid.Rows[row].Cells[a].ColKey + " : " + _monitorWorkloadIGrid.Rows[row].Cells[a].Value);
                //}
                

                if (this.ClientRulesEnabled)
                {
                    //Check for Threshold Color indicators on this row
                    String BackCol = _monitorWorkloadIGrid.Rows[row].Cells["BackgroundColor"].Value.ToString();
                    if (BackCol != "")
                        ChangeDataRowBackgroundColor(_monitorWorkloadIGrid.Rows[row], Color.FromArgb(int.Parse(BackCol)));

                    String ForeCol = _monitorWorkloadIGrid.Rows[row].Cells["ForegroundColor"].Value.ToString();
                    if (ForeCol != "")
                        ChangeDataRowColor(_monitorWorkloadIGrid.Rows[row], Color.FromArgb(int.Parse(ForeCol)));

                    //then apply the color settings for each cell.
                    CheckChangeCellRowColor(_monitorWorkloadIGrid.Rows[row]);                    
                }
            }
            //Remove threshold columns, so that they won't show up in Show/Hide columns            
            //if (_monitorWorkloadIGrid.DataBindings.Count > 0) 
            //{
            //    DataTable dt1 = (DataTable)_monitorWorkloadIGrid.DataBindings[0].DataSource;
            //    WMSUtils.removeThresholdRulesColumns(dt1);
            //    _monitorWorkloadIGrid.FillWithData(dt1);
            //}
            
        }        


        private void ChangeDataRowBackgroundColor(iGRow row, Color c)
        {
            int len = row.Cells.Count;

            try
            {
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].BackColor = c;

            }
            catch (Exception)
            {
            }

        }

        private void CheckChangeCellRowColor(iGRow row)
        {
            if (!this.ClientRulesEnabled)
                return;

            try
            {
                string CellCols = row.Cells["ViolatorColor"].Value.ToString();
                if (CellCols != "")
                {
                    string CellCol = "";
                    String CellNames = row.Cells["ViolatorNames"].Value.ToString();
                    string[] CellNamesArray = CellNames.Split('|');
                    string[] ColorArray = CellCols.Split('|');

                    for (int i = 0; i < ColorArray.Length; i++)  //foreach (string nameArrays in CellNamesArray)
                    {
                        string nameArrays = CellNamesArray[i];
                        CellCol = ColorArray[i];
                        string[] CellNameArray = nameArrays.Split(',');
                        foreach (string name in CellNameArray)
                        {
                            if (name != "")
                                ChangeCellRowColor(row, name, Color.FromArgb(int.Parse(CellCol)));
                        }
                    }
                }

            }
            catch (Exception)
            {
            }

        }

        private void ChangeCellRowColor(iGRow row, string name, Color c)
        {
            try
            {
                row.Cells[name].BackColor = c;

            }
            catch (Exception)
            {
            }
        }

        private void ChangeDataRowColor(iGRow row, Color c)
        {
            int len = row.Cells.Count;

            try
            {
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].ForeColor = c;

            }
            catch (Exception)
            {
            }

        }

        private void ChangeDataRowBackColor(iGRow row, Color c)
        {
            try
            {
                int len = row.Cells.Count;
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].BackColor = c;
            }
            catch (Exception)
            {
            }
        }

        private void setMonitorVisibility(ref TrafodionIGrid aDataGrid)
        {
            for (int r = 0; r < aDataGrid.Rows.Count; r++)
            {
                string warnLevel = aDataGrid.Rows[r].Cells["WARN_LEVEL"].Text;
                if (warnLevel.Equals(""))
                {
                    warnLevel = "NOWARN";
                }
                bool match = _monitorWorkloadOptions.WarnLevels.Contains(warnLevel);
                aDataGrid.Rows[r].Visible = (aDataGrid.Rows[r].Visible && match) ? true : false;

                string queryState = aDataGrid.Rows[r].Cells[WmsCommand.COL_QUERY_STATE].Text;
                bool match2 = _monitorWorkloadOptions.QueryStates.Contains(queryState);
                aDataGrid.Rows[r].Visible = (aDataGrid.Rows[r].Visible && match2) ? true : false;
            }
        }

        private void setMonitorColor(ref TrafodionIGrid aDataGrid)
        {
            for (int r = 0; r < aDataGrid.Rows.Count; r++)
            {
                string warnLevel = aDataGrid.Rows[r].Cells["WARN_LEVEL"].Text;
                if (warnLevel.Equals("HIGH"))
                {
                    aDataGrid.Rows[r].BackColor = _monitorWorkloadOptions.WarnHighColor;
                }
                else if (warnLevel.Equals("MEDIUM"))
                {
                    aDataGrid.Rows[r].BackColor = _monitorWorkloadOptions.WarnMediumColor;
                }
                else if (warnLevel.Equals("LOW"))
                {
                    aDataGrid.Rows[r].BackColor = _monitorWorkloadOptions.WarnLowColor;
                }
                else if (warnLevel.Equals(""))
                {
                    //aDataGrid.Rows[r].BackColor = _monitorWorkloadOptions.NoWarnColor;
                }

                string queryState = aDataGrid.Rows[r].Cells[WmsCommand.COL_QUERY_STATE].Text;

                if (queryState.Equals(WmsCommand.QUERY_STATE_EXECUTING))
                {
                    aDataGrid.Rows[r].ForeColor = _monitorWorkloadOptions.ExecutingColor;
                }
                else if (queryState.Equals(WmsCommand.QUERY_STATE_WAITING))
                {
                    aDataGrid.Rows[r].ForeColor = _monitorWorkloadOptions.WaitingColor;
                }
                else if (queryState.Equals(WmsCommand.QUERY_STATE_HOLDING))
                {
                    aDataGrid.Rows[r].ForeColor = _monitorWorkloadOptions.HoldingColor;
                }
                else if (queryState.Equals(WmsCommand.QUERY_STATE_SUSPENDED))
                {
                    aDataGrid.Rows[r].ForeColor = _monitorWorkloadOptions.SuspendedColor;
                }
                else if (queryState.Equals(WmsCommand.QUERY_STATE_REJECTED))
                {
                    aDataGrid.Rows[r].ForeColor = _monitorWorkloadOptions.RejectedColor;
                }
                else if (queryState.Equals(WmsCommand.QUERY_STATE_COMPLETED))
                {
                    aDataGrid.Rows[r].ForeColor = _monitorWorkloadOptions.CompletedColor;
                }
            }
        }

    }

    public class MyMonitorDataHandler : TabularDataDisplayHandler
    {
        private MonitorWorkloadCanvas _monitorWorkloadCanvas = null;
        private iGCellStyle igridCellStyleForInt = null;
        private iGCellStyle igridCellStyleForDouble = null;
        
        public MyMonitorDataHandler(MonitorWorkloadCanvas aMonitorWorkloadCanvas)
        {
            _monitorWorkloadCanvas = aMonitorWorkloadCanvas;
            InitializeIGridCellStyle();
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable, TrafodionIGrid aDataGrid)
        {
            DataTable newDataTable = new DataTable();
            foreach (DataColumn dc in aDataTable.Columns)
            {
                if (dc.ColumnName.Equals("ELAPSED_TIME") || dc.ColumnName.Equals("WAIT_TIME") || dc.ColumnName.Equals("HOLD_TIME"))
                    newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
                else if (dc.ColumnName.Equals("COMP_START_TIME") || dc.ColumnName.Equals("COMP_END_TIME"))
                    newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
                else if (dc.ColumnName.Equals("EXEC_START_TIME") || dc.ColumnName.Equals("EXEC_END_TIME"))
                    newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
                else if (dc.ColumnName.Equals("FIRST_ROW_RETURNED_TIME"))
                    newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
                else if (dc.ColumnName.Equals("CMP_LAST_UPDATED") || dc.ColumnName.Equals("EXEC_LAST_UPDATED"))
                    newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
                else if (dc.ColumnName.Equals("SQL_CPU_TIME"))
                    newDataTable.Columns.Add("SQL_CPU_TIME_SEC", System.Type.GetType("System.Double"));
                else if (dc.ColumnName.Equals("PROCESS_BUSYTIME"))
                    newDataTable.Columns.Add("PROCESS_BUSYTIME_SEC", System.Type.GetType("System.Double"));
                else if (dc.ColumnName.Equals("OPEN_TIME"))
                    newDataTable.Columns.Add("OPEN_TIME_SEC", System.Type.GetType("System.Double"));
                else if (dc.ColumnName.Equals("PROCESS_CREATE_TIME"))
                    newDataTable.Columns.Add("PROCESS_CREATE_TIME_SEC", System.Type.GetType("System.Double"));
                else
                    newDataTable.Columns.Add(dc.ColumnName, dc.DataType);
            }
            newDataTable.Columns.Add(new DataColumn("PROCESSOR_USAGE_PER_SEC", System.Type.GetType("System.Double")));
            newDataTable.Columns.Add(new DataColumn("TOTAL_PROCESSOR_TIME", System.Type.GetType("System.String")));
            newDataTable.Columns.Add(new DataColumn("LAST_INTERVAL_PROCESSOR_TIME", System.Type.GetType("System.String")));
            newDataTable.Columns.Add(new DataColumn("DELTA_PROCESSOR_TIME", System.Type.GetType("System.String")));
            newDataTable.Columns.Add(new DataColumn("TOTAL_QUERY_TIME", System.Type.GetType("System.String")));

            foreach (DataRow r in aDataTable.Rows)
            {
                string queryID = r[WmsCommand.COL_QUERY_ID].ToString();
                string start_ts = r[WmsCommand.COL_QUERY_START_TIME].ToString();
                DataRow newDR = newDataTable.NewRow();
                for (int i = 0; i < aDataTable.Columns.Count; i++)
                {
                    try
                    {
                        string colName = aDataTable.Columns[i].ToString();
                        //info from Compiler
                        if (colName.Equals("EST_COST"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_COST"].ToString()));
                        else if (colName.Equals("EST_CPU_TIME"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_CPU_TIME"].ToString()));
                        else if (colName.Equals("EST_IO_TIME"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_IO_TIME"].ToString()));
                        else if (colName.Equals("EST_MSG_TIME"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_MSG_TIME"].ToString()));
                        else if (colName.Equals("EST_IDLE_TIME"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_IDLE_TIME"].ToString()));
                        else if (colName.Equals("EST_TOTAL_TIME"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_TOTAL_TIME"].ToString()));
                        else if (colName.Equals("EST_CARDINALITY"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_CARDINALITY"].ToString()));
                        else if (colName.Equals("EST_TOTAL_MEM"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["EST_TOTAL_MEM"].ToString()));
                        else if (colName.Equals("CMP_ROWS_ACCESSED_FULL_SCAN"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["CMP_ROWS_ACCESSED_FULL_SCAN"].ToString()));
                        else if (colName.Equals("CMP_EID_ROWS_ACCESSED"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["CMP_EID_ROWS_ACCESSED"].ToString()));
                        else if (colName.Equals("CMP_EID_ROWS_USED"))
                            newDR[i] = String.Format("{0:F}", Double.Parse(r["CMP_EID_ROWS_USED"].ToString()));
                        //info from RMS
                        else if (colName.Equals("COMP_START_TIME"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["COMP_START_TIME"]);
                        else if (colName.Equals("COMP_END_TIME"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["COMP_END_TIME"]);
                        else if (colName.Equals("EXEC_START_TIME"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["EXEC_START_TIME"]);
                        else if (colName.Equals("EXEC_END_TIME"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["EXEC_END_TIME"]);
                        else if (colName.Equals("ELAPSED_TIME"))
                            newDR[i] = WMSUtils.FormatTimeFromMilliseconds(Int64.Parse(r["ELAPSED_TIME"].ToString()) / 1000);
                        else if (colName.Equals("WAIT_TIME"))
                            newDR[i] = WMSUtils.formatInt2Time(Int64.Parse(r["WAIT_TIME"].ToString()));
                        else if (colName.Equals("HOLD_TIME"))
                            newDR[i] = WMSUtils.formatInt2Time(Int64.Parse(r["HOLD_TIME"].ToString()));
                        else if (colName.Equals("FIRST_ROW_RETURNED_TIME"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["FIRST_ROW_RETURNED_TIME"]);
                        else if (colName.Equals("CMP_LAST_UPDATED"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["CMP_LAST_UPDATED"]);
                        else if (colName.Equals("EXEC_LAST_UPDATED"))
                            newDR[i] = WMSUtils.convertJulianTimeStamp(r["EXEC_LAST_UPDATED"]);
                        else if (colName.Equals("SQL_CPU_TIME"))
                            newDR[i] = String.Format("{0:#,0.000}", Double.Parse(r["SQL_CPU_TIME"].ToString()) / 1000000);
                        else if (colName.Equals("PROCESS_BUSYTIME"))
                            newDR[i] = String.Format("{0:#,0.000}", Double.Parse(r["PROCESS_BUSYTIME"].ToString()) / 1000000);
                        else if (colName.Equals("OPEN_TIME"))
                            newDR[i] = String.Format("{0:#,0.000}", Double.Parse(r["OPEN_TIME"].ToString()) / 1000000);
                        else if (colName.Equals("PROCESS_CREATE_TIME"))
                            newDR[i] = String.Format("{0:#,0.000}", Double.Parse(r["PROCESS_CREATE_TIME"].ToString()) / 1000000);
                        else
                            newDR[i] = r[i];
                    }
                    catch (Exception ex)
                    {
                        newDR[i] = r[i]; 
                    }
                }

                updateDerivedCounters(queryID, start_ts, r, newDR);
                newDataTable.Rows.Add(newDR);                
            }            

            if (this._monitorWorkloadCanvas.ClientRulesEnabled && newDataTable != null)
            {
                ClientQueryRuler.Instance.SetupOperatingTable(this._monitorWorkloadCanvas.ConnectionDefn, newDataTable);
                if (newDataTable.Rows.Count > 0)
                {
                    ClientQueryRuler.Instance.EvaluateEnabledRules(this._monitorWorkloadCanvas.ConnectionDefn, true);
                    ClientQueryRuler.Instance.RuleViolatorPopup.RuleViolatorCellClick += new EventHandler(RuleViolatorPopup_RuleViolatorCellClick);
                }
            }

            base.DoPopulate(aConfig, newDataTable, aDataGrid);
            StyleGrid(aDataGrid);

            string gridHeaderText = string.Format(
                                    (newDataTable.Rows.Count > 1) ? Properties.Resources.MultipleWorkLoadCount :
                                                                    Properties.Resources.SingleWorkLoadCount,
                                    newDataTable.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);            

            _monitorWorkloadCanvas.PreviousDataTable = newDataTable.Copy();
        }

        private void updateDerivedCounters(string queryID, string start_ts, DataRow dr, DataRow newDR)
        {
            try
            {
                try
                {
                    Int64 elapsed_time = Int64.Parse(dr["ELAPSED_TIME"].ToString()) / 1000;
                    Int64 wait_time = Int64.Parse(dr["WAIT_TIME"].ToString()) * 1000;
                    Int64 hold_time = Int64.Parse(dr["HOLD_TIME"].ToString()) * 1000;
                    Int64 total_time = elapsed_time + wait_time + hold_time;
                    newDR["TOTAL_QUERY_TIME"] = WMSUtils.FormatTimeFromMilliseconds(total_time);
                }
                catch (Exception)
                { }

                double totalCpuTimeMicroSecs = Double.Parse(dr["SQL_CPU_TIME"].ToString()) +
                                       Double.Parse(dr["PROCESS_BUSYTIME"].ToString()) +
                                       Double.Parse(dr["OPEN_TIME"].ToString()) +
                                       Double.Parse(dr["PROCESS_CREATE_TIME"].ToString());

                Int64 elapsedTime = Int64.Parse(dr["ELAPSED_TIME"].ToString()) / 1000000;
                double processorUsageSec = 0.0;
                if (elapsedTime > 0)
                {
                    processorUsageSec = (totalCpuTimeMicroSecs / (1000 * 1000)) / elapsedTime;
                }
                newDR["PROCESSOR_USAGE_PER_SEC"] = String.Format("{0:0.000}", processorUsageSec);

                TimeSpan cpuTimeSpan = TimeSpan.FromMilliseconds(totalCpuTimeMicroSecs / 1000);

                DateTime startTime = DateTime.Now.Subtract(cpuTimeSpan);
                newDR["TOTAL_PROCESSOR_TIME"] = WMSUtils.getFormattedElapsedTime(startTime, DateTime.Now);

                if (_monitorWorkloadCanvas.PreviousDataTable == null)
                {
                    newDR["LAST_INTERVAL_PROCESSOR_TIME"] = "00:00:00.000";
                    newDR["DELTA_PROCESSOR_TIME"] = "00:00:00.000";
                }
                else
                {
                    DataRow prevStatsRow = null;
                    DataRow[] theRows = new DataRow[0];
                    //fetch the DataRow of the previous DataTable
                    try
                    {
                        string filter = WmsCommand.COL_QUERY_ID + " = '" + queryID + "' AND QUERY_START_TIME = '" + start_ts + "'";
                        theRows = _monitorWorkloadCanvas.PreviousDataTable.Select(filter);
                        if (theRows.Length > 0)
                        {
                            prevStatsRow = theRows[0];
                        }
                    }
                    catch (Exception)
                    { }

                    if (prevStatsRow != null)
                    {
                        //get the last interval processor time of the previous DataTable
                        double previousTotalCPUTimeMicroSecs = Double.Parse(prevStatsRow["SQL_CPU_TIME"].ToString()) +
                                               Double.Parse(prevStatsRow["PROCESS_BUSYTIME"].ToString()) +
                                               Double.Parse(prevStatsRow["OPEN_TIME"].ToString()) +
                                               Double.Parse(prevStatsRow["PROCESS_CREATE_TIME"].ToString());

                        try
                        {
                            DateTime now = DateTime.Now;
                            DateTime startTime2 = now.Subtract(TimeSpan.FromMilliseconds(previousTotalCPUTimeMicroSecs / 1000));
                            newDR["LAST_INTERVAL_PROCESSOR_TIME"] = WMSUtils.getFormattedElapsedTime(startTime2, now);
                        }
                        catch (Exception)
                        { }

                        //get the delta processor time of the previous DataTable
                        try
                        {
                            String deltaCPUTime = "00:00:00.000";
                            double cpuTimeMicroSec = totalCpuTimeMicroSecs - previousTotalCPUTimeMicroSecs;
                            if (cpuTimeMicroSec > 0.0)
                            {
                                TimeSpan timeDiff = TimeSpan.FromMilliseconds(cpuTimeMicroSec / 1000);
                                deltaCPUTime = String.Format("{0:00}", timeDiff.Hours) + ":" +
                                               String.Format("{0:00}", timeDiff.Minutes) + ":" +
                                               String.Format("{0:00}", timeDiff.Seconds) + "." +
                                               String.Format("{0:000}", timeDiff.Milliseconds);
                                newDR["DELTA_PROCESSOR_TIME"] = deltaCPUTime;
                            }
                        }
                        catch (Exception)
                        { }
                    }
                }
            }
            catch (Exception ex)
            {

            }
        }

        private void RuleViolatorPopup_RuleViolatorCellClick(object sender, EventArgs e) 
        {
            if (e is Trafodion.Manager.WorkloadArea.Controls.ClientRuleViolators.ViolatorCellClickEventArgs) 
            {
                Trafodion.Manager.WorkloadArea.Controls.ClientRuleViolators.ViolatorCellClickEventArgs violatorCellClickEventArgs = 
                    e as Trafodion.Manager.WorkloadArea.Controls.ClientRuleViolators.ViolatorCellClickEventArgs;
                if (violatorCellClickEventArgs.IsLiveView == true) 
                {
                    this._monitorWorkloadCanvas.HighlightQuery(violatorCellClickEventArgs.QueryID, violatorCellClickEventArgs.QueryStartTime);                  
                }
            }
        }

        private void InitializeIGridCellStyle()
        {
            igridCellStyleForInt = new iGCellStyle();
            igridCellStyleForDouble = new iGCellStyle();
            igridCellStyleForInt.FormatString = "{0:N0}";
            igridCellStyleForDouble.FormatString="{0:N2}";

        }

        private void SetColumnStyle(iGrid igrid, string colName, iGCellStyle aStyle)
        {
            if (igrid.Cols.KeyExists(colName) == true)
                igrid.Cols[colName].CellStyle = aStyle;
        }

        private void StyleGrid(iGrid igrid)
        {
            if (igrid == null || igrid.Cols.Count == 0)
                return;  
            
            SetColumnStyle(igrid, "EST_COST", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_CPU_TIME", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_IO_TIME", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_MSG_TIME", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_IDLE_TIME", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_TOTAL_TIME", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_CARDINALITY", igridCellStyleForInt);
            SetColumnStyle(igrid, "EST_TOTAL_MEM", igridCellStyleForDouble);
            SetColumnStyle(igrid, "EST_ACCESSED_ROWS", igridCellStyleForInt);
            SetColumnStyle(igrid, "EST_USED_ROWS", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_ROWS_ACCESSED_FULL_SCAN", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_EID_ROWS_ACCESSED", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_EID_ROWS_USED", igridCellStyleForInt);            
            SetColumnStyle(igrid, "EST_RESRC_USAGE", igridCellStyleForInt);
            //SetColumnStyle(igrid, "CMP_AFFINITY_NUM", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_DOP", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_TXN_NEEDED", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_MANDATORY_X_PROD", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_MISSING_STATS", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_NUM_JOINS", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_FULL_SCAN_ON_TABLE", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_HIGH_EID_MAX_BUF_USAGE", igridCellStyleForInt);
            SetColumnStyle(igrid, "ACCESSED_ROWS", igridCellStyleForInt);
            SetColumnStyle(igrid, "USED_ROWS", igridCellStyleForInt);
            SetColumnStyle(igrid, "ROWS_RETURNED", igridCellStyleForInt);
            SetColumnStyle(igrid, "MESSAGE_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "MESSAGE_BYTES", igridCellStyleForInt);
            SetColumnStyle(igrid, "STATS_BYTES", igridCellStyleForInt);
            SetColumnStyle(igrid, "REQ_MESSAGE_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "REQ_MESSAGE_BYTES", igridCellStyleForInt);
            SetColumnStyle(igrid, "REPLY_MESSAGE_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "REPLY_MESSAGE_BYTES", igridCellStyleForInt);
            SetColumnStyle(igrid, "DISK_IOS", igridCellStyleForInt);
            SetColumnStyle(igrid, "LOCK_WAITS", igridCellStyleForInt);
            SetColumnStyle(igrid, "LOCK_ESCALATIONS", igridCellStyleForInt);
            SetColumnStyle(igrid, "OPENS", igridCellStyleForInt);
            SetColumnStyle(igrid, "AQR_NUM_RETRIES", igridCellStyleForInt);
            SetColumnStyle(igrid, "DELAY_BEFORE_AQR", igridCellStyleForInt);
            SetColumnStyle(igrid, "NUM_ROWS_IUD", igridCellStyleForInt);
            SetColumnStyle(igrid, "SQL_SPACE_ALLOC", igridCellStyleForInt);
            SetColumnStyle(igrid, "SQL_SPACE_USED", igridCellStyleForInt);
            SetColumnStyle(igrid, "SQL_HEAP_ALLOC", igridCellStyleForInt);
            SetColumnStyle(igrid, "SQL_HEAP_USED", igridCellStyleForInt);
            SetColumnStyle(igrid, "SQL_CPU_TIME", igridCellStyleForInt);
            SetColumnStyle(igrid, "EID_SPACE_ALLOC", igridCellStyleForInt);
            SetColumnStyle(igrid, "EID_SPACE_USED", igridCellStyleForInt);
            SetColumnStyle(igrid, "EID_HEAP_ALLOC", igridCellStyleForInt);
            SetColumnStyle(igrid, "EID_HEAP_USED", igridCellStyleForInt);
            SetColumnStyle(igrid, "TOTAL_MEM_ALLOC", igridCellStyleForInt);
            SetColumnStyle(igrid, "MAX_MEM_USED", igridCellStyleForInt);
            SetColumnStyle(igrid, "NUM_SQL_PROCESSES", igridCellStyleForInt);
            SetColumnStyle(igrid, "PROCESSES_CREATED", igridCellStyleForInt);
            SetColumnStyle(igrid, "AGGR_TOTAL_QUERIES", igridCellStyleForInt);
            SetColumnStyle(igrid, "AGGR_SECS_SINCE_LAST_UPDATE", igridCellStyleForInt);
            SetColumnStyle(igrid, "AGGR_SECS_TOTAL_TIME", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_NUMBER_OF_BMOS", igridCellStyleForInt);
            SetColumnStyle(igrid, "CMP_OVERFLOW_SIZE", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_FILE_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_SPACE_ALLOCATED", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_SPACE_USED", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_BLOCK_SIZE", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_WRITE_READ_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_WRITE_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_BUFFER_BLOCKS_WRITTEN", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_BUFFER_BYTES_WRITTEN", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_READ_COUNT", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_BUFFER_BLOCKS_READ", igridCellStyleForInt);
            SetColumnStyle(igrid, "OVF_BUFFER_BYTES_READ", igridCellStyleForInt);
            SetColumnStyle(igrid, "NUM_NODES", igridCellStyleForInt);
            SetColumnStyle(igrid, "UDR_PROCESS_BUSY_TIME", igridCellStyleForInt);
            
        }

        
    }

}
