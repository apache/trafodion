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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// Live Feed event viewer
    /// </summary>
    public partial class LiveFeedEventUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private static readonly string  LiveFeedEventsConfigName = "LiveFeed_Event_Widget_tst4";
        private UniversalWidgetConfig _widgetConfig = null;
        private LiveFeedUniversalWidget LiveFeedEventsWidget;
        private ConnectionDefinition _theConnectionDefinition;
        private LiveFeedConnection  _theLiveFeedConneciton = null;
        private string _theTitle = "Live Feed Events";
        private LiveFeedEventsDataHandler _dataDisplayHandler = null;
        private LiveFeedEventsDataProvider _dataProvider = null;
        private DataTable _dataStore = null;
        private ToolStrip _theToolStrip = null;
        private Form _theParentForm = null;
        private EventDetails _theEventDetails;
        public delegate void UpdateStatus(object obj, DataProviderEventArgs e);
        private TrafodionProgressUserControl _progressUserControl;
        private EventHandler<TrafodionProgressCompletedArgs> _progressHandler;
        private DataTable _initErrorTable = new DataTable();
        private ToolStripButton _configServerButton = new ToolStripButton();
        private bool _initialized = false;
 
        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: TheEventDetails - the event details
        /// </summary>
        public EventDetails TheEventDetails
        {
            get { return _theEventDetails; }
        }

        /// <summary>
        /// Property: ConnectionDefinition - the system definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: LiveFeedConnection - the live feed connection being used
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            get { return _theLiveFeedConneciton; }
            set { _theLiveFeedConneciton = value; }
        }

        /// <summary>
        /// Property: DataProvider - the data provider used
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _dataProvider; }
            set { _dataProvider = value as LiveFeedEventsDataProvider; }
        }

        /// <summary>
        /// Property: UniversalWidgetConfiguration - the widget configuration 
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _widgetConfig; }
            set { _widgetConfig = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - the display handler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _dataDisplayHandler; }
            set { }
        }

        /// <summary>
        /// Property: DrillDownManager - for interface
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        /// <summary>
        /// Property: WindowTitle - the title for the window
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

        /// <summary>
        /// Property: Initialized - indicated the user control is properly initialized
        /// </summary>
        public bool Initialized
        {
            get { return _initialized; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// dummy constructor to satisfy the designer
        /// </summary>
        public LiveFeedEventUserControl()
        {
            InitializeComponent();
            _theFilterPanel.ForLiveEvents = true;
            this.Load += new EventHandler(LiveFeedEventUserControl_Load);
        }

        /// <summary>
        /// Constructor used mostly
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public LiveFeedEventUserControl(ConnectionDefinition aConnectionDefinition):this()
        {
            ConnectionDefn = aConnectionDefinition;
            _theEventDetails = new EventDetails(aConnectionDefinition);
            InitializeFilterControls();
        }

        /// <summary>
        /// Copy constructor 
        /// </summary>
        /// <param name="aLiveFeedEventUserControl"></param>
        public LiveFeedEventUserControl(LiveFeedEventUserControl aLiveFeedEventUserControl)
            : this(aLiveFeedEventUserControl.ConnectionDefn)
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
            LiveFeedEventUserControl theLiveFeedEventUserControl = new LiveFeedEventUserControl(this);
            return theLiveFeedEventUserControl;
        }

        /// <summary>
        /// not used, but interface requires it
        /// </summary>
        public void PersistConfiguration()
        {

        }

        /// <summary>
        /// showing the filter messages
        /// </summary>
        public void ShowFilterMessage()
        {
            string filterString =  _theFilterPanel.GetLiveFeedFilterDisplayString(_dataDisplayHandler);
            ShowFilterMessage(filterString);
        }

        /// <summary>
        /// showing the filter messages 
        /// </summary>
        /// <param name="message"></param>
        public void ShowFilterMessage(string message)
        {
            if ((message == null) || (message.Trim().Length == 0))
            {
                _theFilterListPanel.Visible = false;
                this._theFilterListPanel.Controls.Remove(_theToolStrip);
                this.LiveFeedEventsWidget.ReAttachToolStrip();

            }
            else
            {
                _theFilterListPanel.Visible = true;
                _theToolStrip = LiveFeedEventsWidget.GetDetachedToolStrip();
                _theToolStrip.Dock = DockStyle.Top;
                this._theFilterListPanel.Controls.Add(_theToolStrip);
                _theFiltertext.Text = "Filter criteria: \n" + message;
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Initialize filter controls
        /// </summary>
        private void InitializeFilterControls()
        {
            if (this.ConnectionDefn.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded &&
                this.ConnectionDefn.TheState != ConnectionDefinition.State.TestSucceeded)
            {
                if (!DoConnect())
                {
                    return;
                }
            }

            _initialized = true;

            _initErrorTable = new DataTable();
            _initErrorTable.Columns.Add("Filter Name");
            _initErrorTable.Columns.Add("Error Text");

            _theContentPanel.Visible = false;
            _theProgressPanel.Visible = true;
            ShowFilters(false);

            //try to get it from one gui context
            ArrayList components = TrafodionContext.Instance.GetComponents(_theConnectionDefinition.Name);
            if (components == null)
            {
                //try to fetch from the db and set it to the context
                Object[] parameters = new Object[] { };
                TrafodionProgressArgs args = new TrafodionProgressArgs("Initializing the Event viewer. Getting subsystems filter information from server...", _theEventDetails, "GetComponentList", parameters);
                _progressUserControl = new TrafodionProgressUserControl(args);
                _progressUserControl.Dock = DockStyle.Fill;
                _progressHandler = Component_ProgressCompletedEvent;
                _progressUserControl.ProgressCompletedEvent += _progressHandler;
                _theProgressPanel.Controls.Clear();
                _theProgressPanel.Controls.Add(_progressUserControl);
            }
            else
            {
                _theEventDetails.SubSystems = components;
                InitializeSeveritiesList();
            }
        }

        private bool DoConnect()
        {
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);
            theConnectionDefinitionDialog.Edit(this.ConnectionDefn);
  
            if (this.ConnectionDefn.TheState == ConnectionDefinition.State.TestSucceeded ||
                this.ConnectionDefn.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Process completed event for getting components
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Component_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            _theProgressPanel.Controls.Clear();
            _progressUserControl.ProgressCompletedEvent -= Component_ProgressCompletedEvent;

            if (e.Error != null)
            {
                _theEventDetails.SubSystems = new ArrayList();
                _initErrorTable.Rows.Add(new object[] { "Subsystems", e.Error.Message });
            }
            else
            {
                ArrayList components = (ArrayList)_progressUserControl.ReturnValue;
                _theEventDetails.SubSystems = components;
                if (components.Count > 0)
                {
                    TrafodionContext.Instance.SetComponents(_theConnectionDefinition.Name, components);
                }
            }
            InitializeSeveritiesList();
        }

        /// <summary>
        /// To initialize severity list
        /// </summary>
        private void InitializeSeveritiesList()
        {
            //try to get it from one gui context
            ArrayList severityList = TrafodionContext.Instance.GetSeverities(_theConnectionDefinition.Name);
            if (severityList == null)
            {
                //try to fetch from the db and set it to the context
                Object[] parameters = new Object[] { };
                TrafodionProgressArgs args = new TrafodionProgressArgs("Initializing the Event viewer. Getting severities filter information from server...", _theEventDetails, "GetSeverityList", parameters);
                _progressUserControl = new TrafodionProgressUserControl(args);
                _progressUserControl.Dock = DockStyle.Fill;
                _progressHandler = Severities_ProgressCompletedEvent;
                _progressUserControl.ProgressCompletedEvent += _progressHandler;
                _theProgressPanel.Controls.Clear();
                _theProgressPanel.Controls.Add(_progressUserControl);
            }
            else
            {
                _theProgressPanel.Visible = false;
                _theContentPanel.Visible = true;
                ShowFilters(true);
                _theEventDetails.Severities = severityList;
                CompleteInitialization();
            }
        }

        /// <summary>
        /// event handler for the progress completed event for getting severity list
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Severities_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            _theProgressPanel.Controls.Clear();
            _theProgressPanel.Visible = false;
            _theContentPanel.Visible = true;
            ShowFilters(true);
            _progressUserControl.ProgressCompletedEvent -= Severities_ProgressCompletedEvent;
            if (e.Error != null)
            {
                _theEventDetails.Severities = new ArrayList();
                _initErrorTable.Rows.Add(new object[] { "Severities", e.Error.Message });
            }
            else
            {
                ArrayList severities = (ArrayList)_progressUserControl.ReturnValue;
                _theEventDetails.Severities = severities;
                if (severities.Count > 0)
                {
                    TrafodionContext.Instance.SetSeverities(_theConnectionDefinition.Name, severities);
                }
             }

            CompleteInitialization();

            if (_initErrorTable.Rows.Count > 0)
            {
                if (_initErrorTable.Rows.Count > 1)
                {
                    TrafodionMultipleMessageDialog ommd = new TrafodionMultipleMessageDialog("Failed to initialize filter settings", _initErrorTable, System.Drawing.SystemIcons.Warning);
                    ommd.ShowDialog();
                }
                else
                {
                    MessageBox.Show( string.Format("Warning. Failed to retrieve list of {0}. You will not be able to use the {0} filter. \n\n{1}", _initErrorTable.Rows[0][0], _initErrorTable.Rows[0][1]),
                        "Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
        }

        /// <summary>
        /// Complete the entire initialization
        /// </summary>
        private void CompleteInitialization()
        {
            _theFilterPanel.TheEventDetails = _theEventDetails;
            _theFilterPanel.ConnectionDefinition = _theConnectionDefinition;
            _theFilterPanel.PopulateUIFromPersistence(EventFilterModel.LiveEventFilterPersistenceKey);
            try
            {
                ShowWidgets();
                ShowFilters(true);
                ShowFilterMessage();
            }
            catch (Exception ex)
            {
                _theWidgetPanel.Controls.Clear();

                TrafodionTextBox errorTextBox = new TrafodionTextBox();
                errorTextBox.WordWrap = errorTextBox.ReadOnly = errorTextBox.Multiline = true;
                errorTextBox.Text = "Error initializing Live Event Viewer : " + Environment.NewLine + Environment.NewLine + ex.Message;
                errorTextBox.Dock = DockStyle.Fill;

                _theWidgetPanel.Controls.Add(errorTextBox);
            }
        }

        /// <summary>
        /// Invoker for the error handler
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview, 
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Invoker for new data arrival handler
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview, 
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Invoker for fetching data handler
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Handler for Loaded event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void LiveFeedEventUserControl_Load(object sender, EventArgs e)
        {
            //Keep a reference of the parent when the control gets created
            _theParentForm = this.Parent as Form;
            if (_theParentForm != null)
            {
                _theParentForm.FormClosing += new FormClosingEventHandler(_theParentForm_FormClosing);
            }
        }

        /// <summary>
        /// Form closing event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theParentForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            //Stop the timer and the provider when the control exists
            if(_dataProvider != null)
            {
                _dataProvider.StopTimer();
                _dataProvider.Stop();
            }

            if (_progressUserControl != null && _progressHandler != null)
            {
                _progressUserControl.ProgressCompletedEvent -= _progressHandler;
                _progressUserControl.Dispose();
            }
        }

        /// <summary>
        /// To create all widgets
        /// </summary>
        private void ShowWidgets()
        {
            _theFilterPanel.OnFilterChangedImpl = this.OnFilterChangedImpl;
            this._theGraphEventSplitContainer.Panel1Collapsed = true;

            //Create a UW using the configuration
            _widgetConfig = WidgetRegistry.GetConfigFromPersistence(LiveFeedEventsConfigName);
            if (_widgetConfig == null)
            {
                _widgetConfig = new UniversalWidgetConfig();
                _widgetConfig.Name = LiveFeedEventsConfigName;
                _widgetConfig.Title = "Live Events";
                _widgetConfig.ShowProperties = false;
                _widgetConfig.ShowToolBar = true;
                _widgetConfig.ShowChart = false;
                _widgetConfig.ShowRowCount = true;
                _widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }

            LiveFeedEventDataProviderConfig dpConfig;

            if (_widgetConfig.DataProviderConfig == null)
            {
                dpConfig = new LiveFeedEventDataProviderConfig();

                List<string> defaultVisibleColumns = new List<string>
                    {
                    EventFilterModel.EVENT_TIME,
                    EventFilterModel.EVENT_ID,
                    EventFilterModel.PROCESS_ID,
                    EventFilterModel.SUBSYSTEM,
                    EventFilterModel.SEVERITY,
                    EventFilterModel.ROLE_NAME,
                    EventFilterModel.MESSAGE,
                    EventFilterModel.NODE_ID,
                    EventFilterModel.PROCESS_NAME
                    };
                dpConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

                List<ColumnMapping> columnMappings = new List<ColumnMapping>
                {
                    new ColumnMapping(EventFilterModel.EVENT_TIME, EventFilterModel.EVENT_TIME, 200),
                    new ColumnMapping(EventFilterModel.EVENT_ID, EventFilterModel.EVENT_ID, 80),
                    new ColumnMapping(EventFilterModel.PROCESS_ID, EventFilterModel.PROCESS_ID, 80),
                    new ColumnMapping(EventFilterModel.SUBSYSTEM, EventFilterModel.SUBSYSTEM, 80),
                    new ColumnMapping(EventFilterModel.SEVERITY, EventFilterModel.SEVERITY, 80),
                    new ColumnMapping(EventFilterModel.ROLE_NAME, EventFilterModel.ROLE_NAME, 80),
                    new ColumnMapping(EventFilterModel.MESSAGE, EventFilterModel.MESSAGE, 400),
                    new ColumnMapping(EventFilterModel.NODE_ID, EventFilterModel.NODE_ID, 80),
                    new ColumnMapping(EventFilterModel.PROCESS_NAME, EventFilterModel.PROCESS_NAME, 80)
                };
                dpConfig.ColumnMappings = columnMappings;

                _widgetConfig.DataProviderConfig = dpConfig;
            }
            else
            {
                dpConfig = (LiveFeedEventDataProviderConfig)_widgetConfig.DataProviderConfig; 
            }

            dpConfig.ConnectionDefinition = _theConnectionDefinition;
            dpConfig.Configure(LiveFeedRoutingKeyMapper.PUBS_common_text_event);

            dpConfig.RefreshRate = 0;
            dpConfig.KeepCacheOnStop = true;

            //show help button
            _widgetConfig.ShowHelpButton = true;
            _widgetConfig.HelpTopic = HelpTopics.UseEventViewer;

            _widgetConfig.ShowTimerSetupButton = false;
            _widgetConfig.ShowRefreshButton = false;
            _widgetConfig.ShowRefreshTimerButton = true;

            //set the connection definition and the data provider
            _dataProvider = dpConfig.GetDataProvider() as LiveFeedEventsDataProvider;
            
            //initialize the filter model of the data provider from the filter panel
            _dataProvider.EventFilterModel = _theFilterPanel.FilterModel;

            //Create the Universal widget to diaplay the data
            LiveFeedEventsWidget = new LiveFeedUniversalWidget();
            LiveFeedEventsWidget.RowCount = dpConfig.CacheSize;

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)LiveFeedEventsWidget.DataDisplayControl).LineCountFormat = "Waiting for events from the server...";
            LiveFeedEventsWidget.UniversalWidgetConfiguration = _widgetConfig;
            _dataDisplayHandler = new LiveFeedEventsDataHandler(this);
            _dataDisplayHandler.ServerTimeZone = ConnectionDefn.ServerTimeZoneName;

            LiveFeedEventsWidget.DataDisplayControl.DataDisplayHandler = _dataDisplayHandler;

            LiveFeedEventsWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(LiveFeedEventsWidget);

            //AddToolStripButtons();

            //Add event handlers to deal with data provider events
            AddHandlers();

            ((TabularDataDisplayControl)LiveFeedEventsWidget.DataDisplayControl).DataGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(DataGrid_CellClick);

            //Keep DataGrid data when error encountered. 
            ((TabularDataDisplayControl)LiveFeedEventsWidget.DataDisplayControl).KeepDataGridOnError = true;

            //Show for all events
            LiveFeedEventsWidget.StartDataProvider();
            LiveFeedEventsWidget.RowCountTextBox.KeyPress += new KeyPressEventHandler(RowCountTextBox_KeyPress);
            LiveFeedEventsWidget.RowCountTextBox.Leave += new EventHandler(RowCountTextBox_Leave);

            //Add Disposed handler to clean up, connection
            this.Disposed += new EventHandler(LiveFeedEventsCanvas_Disposed);
            this._dataProvider.LiveFeedConnection.BrokerConfiguration.OnBrokerChanged += new LiveFeedBrokerConfiguration.LiveFeedBrokerChanged(BrokerConfiguration_OnBrokerChanged);
        }

        /// <summary>
        /// Handle events when the broker configuration has been changed by others.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="eArgs"></param>
        private void BrokerConfiguration_OnBrokerChanged(object sender, LiveFeedBrokerChangedEventArgs eArgs)
        {
            if (eArgs.ChangedReason == LiveFeedBrokerChangedEventArgs.Reason.SessionRetryTimer)
            {
                _dataProvider.ConfiguredRefreshRate = _dataProvider.LiveFeedConnection.BrokerConfiguration.SessionRetryTimer;
            }
            else
            {
                _dataProvider.Stop();
                _dataProvider.ClearCache();
                ((TabularDataDisplayControl)LiveFeedEventsWidget.DataDisplayControl).DataGrid.Clear();
                _dataProvider.Start();
            }
        }

        ///// <summary>
        ///// Add additional strip buttons
        ///// </summary>
        //private void AddToolStripButtons()
        //{
        //    _configServerButton.Text = "Configure Live Feed Server";
        //    _configServerButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
        //    _configServerButton.Image = Trafodion.Manager.Properties.Resources.AlterIcon;
        //    _configServerButton.ImageTransparentColor = System.Drawing.Color.Magenta;
        //    _configServerButton.Name = "_configureLiveFeedButton";
        //    _configServerButton.Click += new EventHandler(_configServerButton_Click);
        //    LiveFeedEventsWidget.AddToolStripItem(_configServerButton);

        //    //Add a separator
        //    ToolStripSeparator toolStripSeparator1 = new ToolStripSeparator();
        //    toolStripSeparator1.Name = "_configureLiveFeedSeparator";
        //    toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
        //    toolStripSeparator1.Visible = true;
        //    LiveFeedEventsWidget.AddToolStripItem(toolStripSeparator1);
        //}

        /// <summary>
        /// Event handler for config server button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        //void _configServerButton_Click(object sender, EventArgs e)
        //{
        //    LiveFeedBrokerConfigDialog dialog = new LiveFeedBrokerConfigDialog(_dataProvider.LiveFeedConnection);
        //    DialogResult result = dialog.ShowDialog();
        //    if (result == DialogResult.OK)
        //    {
        //        _dataProvider.ConfiguredRefreshRate = _dataProvider.LiveFeedConnection.BrokerConfiguration.SessionRetryTimer;
        //        _dataProvider.Stop();
        //        _dataProvider.ClearCache();
        //        ((TabularDataDisplayControl)LiveFeedEventsWidget.DataDisplayControl).DataGrid.Clear();
        //        _dataProvider.Start();
        //    }
        //}

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
            int count = LiveFeedEventsWidget.RowCount;
            if (count > 0)
            {
                CachedLiveFeedDataProviderConfig clfConfig = _dataProvider.DataProviderConfig as CachedLiveFeedDataProviderConfig;
                if (clfConfig != null && clfConfig.CacheSize != count)
                {
                    clfConfig.CacheSize = count;
                    _dataProvider.ReSizeCache();
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void DataGrid_CellClick(object sender, TenTec.Windows.iGridLib.iGCellClickEventArgs e)
        {
            TenTec.Windows.iGridLib.iGrid grid = sender as TenTec.Windows.iGridLib.iGrid;
            if (grid != null)
            {
                String columnName = grid.Cols[e.ColIndex].Text as string;
                object value = grid.Cells[e.RowIndex, e.ColIndex].Value;
                _theFilterPanel.SetFilterValues(columnName, value);
            }
        }

        /// <summary>
        /// Event handler for disposed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void LiveFeedEventsCanvas_Disposed(object sender, EventArgs e)
        {
            Shutdown();
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
                if (LiveFeedEventsWidget != null)
                {
                    LiveFeedEventsWidget.RowCountTextBox.KeyPress -= RowCountTextBox_KeyPress;
                    LiveFeedEventsWidget.RowCountTextBox.Leave -= RowCountTextBox_Leave;
                }
                if (_dataProvider != null)
                {
                    Shutdown();
                }
            }
        }

        /// <summary>
        /// To shut down
        /// </summary>
        private void Shutdown()
        {
            _dataProvider.Stop();
            this._dataProvider.LiveFeedConnection.BrokerConfiguration.OnBrokerChanged -= BrokerConfiguration_OnBrokerChanged;

        }

        /// <summary>
        /// gets executed when the Apply filter button is pressed
        /// </summary>
        /// <param name="aFilterModel"></param>
        private void OnFilterChangedImpl(EventFilterModel aFilterModel)
        {
            _dataProvider.Stop();
            _dataProvider.ClearCache();
            ((TabularDataDisplayControl)LiveFeedEventsWidget.DataDisplayControl).DataGrid.Clear();
            _dataProvider.EventFilterModel = aFilterModel;
            _dataProvider.Start();
            ShowFilterMessage(aFilterModel.GetLiveFeedFormattedFilterString(_dataDisplayHandler));
        }

        /// <summary>
        /// To associate all data provider handlers
        /// </summary>
        private void AddHandlers()
        {
            if (LiveFeedEventsWidget != null && LiveFeedEventsWidget.DataProvider != null)
            {
                //Associate the event handlers
                LiveFeedEventsWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                LiveFeedEventsWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                LiveFeedEventsWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
            }
        }

        /// <summary>
        /// removes the data provider handlers
        /// </summary>
        private void RemoveHandlers()
        {
            if (LiveFeedEventsWidget != null && LiveFeedEventsWidget.DataProvider != null)
            {
                //Remove the event handlers
                LiveFeedEventsWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                LiveFeedEventsWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                LiveFeedEventsWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(true);
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(true);
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(false);
        }
        private void _theShowEventFilterButton_Click(object sender, EventArgs e)
        {
            ShowFilters(true);
        }

        private void _theCloseFilterPanelButton_Click(object sender, EventArgs e)
        {
            ShowFilters(false);
        }

        private void _theHelpToolStripButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseEventFilter);
        }

        /// <summary>
        /// to show filters
        /// </summary>
        /// <param name="showFlag"></param>
        private void ShowFilters(bool showFlag)
        {
            _theFilterToolStrip.Visible = !showFlag;
            _theLiveFeedEventSplitContainer.Panel2Collapsed = !showFlag;
        }
        #endregion Private methods
    }
}
