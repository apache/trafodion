#region Copyright info
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
#endregion Copyright info

using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class TextEventUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private static readonly string  TextEventsConfigName = "Text_Event_Widget";
        UniversalWidgetConfig           _widgetConfig = null;
        GenericUniversalWidget          TextEventsWidget;
        ConnectionDefinition            _theConnectionDefinition;
        String                          _theTitle = "Text Events";
        ArrayList                       _commands = new ArrayList();
        TextEventsDataHandler           _dataDisplayHandler = null;
        TextEventsDataProvider           _dataProvider = null;
        DataTable                       _dataStore = null;
        ToolStrip                       _theToolStrip = null;
        Form _theParentForm = null;
        EventDetails _theEventDetails;
        public delegate void UpdateStatus(object obj, DataProviderEventArgs e);
        TrafodionProgressUserControl _progressUserControl;
        EventHandler<TrafodionProgressCompletedArgs> _progressHandler;

        private DataTable _initErrorTable = new DataTable();         
 
        #endregion Fields

        #region Properties

        public EventDetails TheEventDetails
        {
            get { return _theEventDetails; }
        }
        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                _theConnectionDefinition = value;
            }
        }

        public DataProvider DataProvider
        {
            get { return _dataProvider; }
            set { _dataProvider = value as TextEventsDataProvider; }
        }

        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get 
            {
                return _widgetConfig; 
            }
            set { _widgetConfig = value; }
        }

        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _dataDisplayHandler; }
            set { }
        }

        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        public DataTable DataStore
        {
            get { return _dataStore; }
            set { _dataStore = value; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }

        public string WindowTitle
        {
            get { return _theTitle; }
        }

        #endregion Properties


        #region Constructors
        //dummy constructor to satisfy the designer
        public TextEventUserControl()
        {
            InitializeComponent();
            this.Load += new EventHandler(TextEventUserControl_Load);
        }

        public TextEventUserControl(ConnectionDefinition aConnectionDefinition):this()
        {
            ConnectionDefinition = aConnectionDefinition;
            _theEventDetails = new EventDetails(aConnectionDefinition);
            InitializeFilterControls();
        }

        public TextEventUserControl(TextEventUserControl aTextEventUserControl)
            : this(aTextEventUserControl.ConnectionDefn)
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
            TextEventUserControl theTextEventUserControl = new TextEventUserControl(this);
            return theTextEventUserControl;
        }

        public void PersistConfiguration()
        {

        }
        public void ShowFilterMessage()
        {
            string filterString =  _theFilterPanel.GetFilterDisplayString(_dataDisplayHandler);
            ShowFilterMessage(filterString);
        }

        public void ShowFilterMessage(string message)
        {
            if ((message == null) || (message.Trim().Length == 0))
            {
                _theFilterListPanel.Visible = false;
                this._theFilterListPanel.Controls.Remove(_theToolStrip);
                this.TextEventsWidget.ReAttachToolStrip();

            }
            else
            {
                _theFilterListPanel.Visible = true;
                _theToolStrip = TextEventsWidget.GetDetachedToolStrip();
                _theToolStrip.Dock = DockStyle.Top;
                this._theFilterListPanel.Controls.Add(_theToolStrip);
                _theFiltertext.Text = "Filter criteria: \n" + message;
            }
        }

        #endregion Public methods


        #region Private methods

        private void InitializeFilterControls()
        {
            _initErrorTable = new DataTable();
            _initErrorTable.Columns.Add("Filter Name");
            _initErrorTable.Columns.Add("Error Text");

            _theContentPanel.Visible = false;
            _theProgressPanel.Visible = true;

            //try to get it from one gui context
            ArrayList components = TrafodionContext.Instance.GetComponents(_theConnectionDefinition.Name);
           /* if (components == null || !TrafodionContext.Instance.IsComponentListPermanent(_theConnectionDefinition.Name))
            {
                //try to fetch from the db and set it to the context
                Object[] parameters = new Object[] { };
                TrafodionProgressArgs args = new TrafodionProgressArgs("Initializing the Event viewer. Getting subsystems filter information from server...", _theEventDetails, "GetComponentsFromDB", parameters);
                _progressUserControl = new TrafodionProgressUserControl(args);
                _progressUserControl.Dock = DockStyle.Top;
                _progressHandler = Component_ProgressCompletedEvent;
                _progressUserControl.ProgressCompletedEvent += _progressHandler;
                _theProgressPanel.Controls.Clear();
                _theProgressPanel.Controls.Add(_progressUserControl);
            }
            else*/
            {
                _theEventDetails.SubSystems = components;
                InitializeSeveritiesList();
            }
        }


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
                    TrafodionContext.Instance.SetComponentListPermanent(_theConnectionDefinition.Name);
                }
            }
            InitializeSeveritiesList();
        }

        private void InitializeSeveritiesList()
        {
            //try to get it from one gui context
           ArrayList severityList = TrafodionContext.Instance.GetSeverities(_theConnectionDefinition.Name);
           /* if (severityList == null || !TrafodionContext.Instance.IsSeverityListPermanent(_theConnectionDefinition.Name))
            {
                //try to fetch from the db and set it to the context
                Object[] parameters = new Object[] { };
                TrafodionProgressArgs args = new TrafodionProgressArgs("Initializing the Event viewer. Getting severities filter information from server...", _theEventDetails, "GetSeveritiesFromDB", parameters);
                _progressUserControl = new TrafodionProgressUserControl(args);
                _progressUserControl.Dock = DockStyle.Top;
                _progressHandler = Severities_ProgressCompletedEvent;
                _progressUserControl.ProgressCompletedEvent += _progressHandler;
                _theProgressPanel.Controls.Clear();
                _theProgressPanel.Controls.Add(_progressUserControl);
            }
            else*/
            {
                _theProgressPanel.Visible = false;
                _theContentPanel.Visible = true;
                _theEventDetails.Severities = severityList;
                CompleteInitialization();
            }
        }

        private void Severities_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            _theProgressPanel.Controls.Clear();
            _theProgressPanel.Visible = false;
            _theContentPanel.Visible = true;
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
                    TrafodionContext.Instance.SetServerityListPermanent(_theConnectionDefinition.Name);
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

        private void CompleteInitialization()
        {
            _theFilterPanel.TheEventDetails = _theEventDetails;
            _theFilterPanel.ConnectionDefinition = _theConnectionDefinition;
            _theFilterPanel.PopulateUIFromPersistence(EventFilterModel.EventFilterPersistenceKey);
            ShowWidgets();
            ShowFilters(true);
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
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
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void InvokeHandleFetchCancelled(Object obj, EventArgs e) 
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchCancelled), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }


        //Keep a reference of the parent when the control gets created
        private void TextEventUserControl_Load(object sender, EventArgs e)
        {
            _theParentForm = this.Parent as Form;
            if (_theParentForm != null)
            {
                _theParentForm.FormClosing += new FormClosingEventHandler(_theParentForm_FormClosing);
            }
        }

        //Stop the timer and the provider when the control exists
        private void _theParentForm_FormClosing(object sender, FormClosingEventArgs e)
        {
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

        private void ShowWidgets()
        {
            _theFilterPanel.OnFilterChangedImpl = this.OnFilterChangedImpl;
            this._theGraphEventSplitContainer.Panel1Collapsed = true;

            //Create a UW using the configuration
            _widgetConfig = WidgetRegistry.GetConfigFromPersistence(TextEventsConfigName);
            if (_widgetConfig == null)
            {
                _widgetConfig = new UniversalWidgetConfig();
                _widgetConfig.Name = TextEventsConfigName;
                _widgetConfig.Title = "Text Events";
                _widgetConfig.ShowProperties = false;
                _widgetConfig.ShowToolBar = true;
                _widgetConfig.ShowChart = false;
                _widgetConfig.ShowRowCount = true;
                _widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            TextEventDataProviderConfig dbConfig = new TextEventDataProviderConfig();
            if (_widgetConfig.DataProviderConfig == null)
            {
            	List<string> defaultVisibleColumns = new List<string>
                    {
                    EventFilterModel.EVENT_TIME,
                    EventFilterModel.EVENT_ID,
                    EventFilterModel.PROCESS_ID,
                    EventFilterModel.SUBSYSTEM,
                    EventFilterModel.SEVERITY,
                    EventFilterModel.MESSAGE,
                    EventFilterModel.NODE_ID,
                    EventFilterModel.PROCESS_NAME,
                    EventFilterModel.TOKENIZED_EVENT_TABLE
                    };
				dbConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

                _widgetConfig.DataProviderConfig = dbConfig;
                _widgetConfig.DataProviderConfig.RefreshRate = 30;
            }
            else 
            {
                dbConfig = (TextEventDataProviderConfig)_widgetConfig.DataProviderConfig;
            }
            _widgetConfig.DataProviderConfig.ColumnMappings = new List<ColumnMapping>()
            {
                    new ColumnMapping(EventFilterModel.EVENT_TIME, EventFilterModel.EVENT_TIME, 200),
                    new ColumnMapping(EventFilterModel.EVENT_ID, EventFilterModel.EVENT_ID, 80),
                    new ColumnMapping(EventFilterModel.PROCESS_ID, EventFilterModel.PROCESS_ID, 80),
                    new ColumnMapping(EventFilterModel.SUBSYSTEM, EventFilterModel.SUBSYSTEM, 80),
                    new ColumnMapping(EventFilterModel.SEVERITY, EventFilterModel.SEVERITY, 80),
                    new ColumnMapping(EventFilterModel.ROLE_NAME, EventFilterModel.ROLE_NAME, 80),
                    new ColumnMapping(EventFilterModel.MESSAGE, EventFilterModel.MESSAGE, 400),
                    new ColumnMapping(EventFilterModel.NODE_ID, EventFilterModel.NODE_ID, 80),
                    new ColumnMapping(EventFilterModel.PROCESS_NAME, EventFilterModel.PROCESS_NAME, 80),
                    new ColumnMapping(EventFilterModel.TOKENIZED_EVENT_TABLE, EventFilterModel.TOKENIZED_EVENT_TABLE, 80)
            };
            //show help button
            _widgetConfig.ShowHelpButton = true;
            _widgetConfig.HelpTopic = HelpTopics.UseEventViewer;

            //make sure timer is not paused
            _widgetConfig.DataProviderConfig.TimerPaused = false;

            //set the connection definition and the data provider
            dbConfig.ConnectionDefinition = _theConnectionDefinition;
            _dataProvider = (TextEventsDataProvider)dbConfig.GetDataProvider();
            
            //initialize the filter model of the data provider from the filter panel
            _dataProvider.EventFilterModel = _theFilterPanel.FilterModel;

            //Create the Universal widget to diaplay the data
            TextEventsWidget = new GenericUniversalWidget();
            TextEventsWidget.UniversalWidgetConfiguration = _widgetConfig;

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)TextEventsWidget.DataDisplayControl).LineCountFormat = "Fetching events from server...";
            _dataDisplayHandler = new TextEventsDataHandler(this);
            _dataDisplayHandler.ServerTimeZone = ConnectionDefn.ServerTimeZoneName;
            _dataProvider.ColumnsToFilterOn = _dataDisplayHandler.Columns;
            TextEventsWidget.DataDisplayControl.DataDisplayHandler = _dataDisplayHandler;
            TextEventsWidget.UniversalWidgetConfiguration = _widgetConfig;
            TextEventsWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(TextEventsWidget);

            //Add event handlers to deal with data provider events
            AddHandlers();

            ((TabularDataDisplayControl)TextEventsWidget.DataDisplayControl).DataGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(DataGrid_CellClick);

            //Keep DataGrid data when error encountered. 
            ((TabularDataDisplayControl)TextEventsWidget.DataDisplayControl).KeepDataGridOnError = true;

            //Show for all events
            TextEventsWidget.StartDataProvider();

            //Add Disposed handler to clean up, connection
            this.Disposed += new EventHandler(TextEventsCanvas_Disposed);
        }

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


        private void TextEventsCanvas_Disposed(object sender, EventArgs e)
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
            }
        }

        private void Shutdown()
        {
        }

        //gets executed when the Apply filter button is pressed
        private void OnFilterChangedImpl(EventFilterModel aFilterModel)
        {
            if (_dataProvider.FetchInProgress)
            {
                _dataProvider.Stop();
            }
            _dataProvider.EventFilterModel = aFilterModel;
            // Always wants the refresh first
            _dataProvider.StartTimer(); 
            _dataProvider.Start();
            ShowFilterMessage(aFilterModel.GetFormattedFilterString(_dataDisplayHandler));
        }

        //associates the data provider handlers
        private void AddHandlers()
        {
            if (TextEventsWidget != null && TextEventsWidget.DataProvider != null)
            {
                //Associate the event handlers
                TextEventsWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                TextEventsWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                TextEventsWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
                TextEventsWidget.DataProvider.OnFetchCancelled += InvokeHandleFetchCancelled;
            }
        }

        //removes the data provider handlers
        private void RemoveHandlers()
        {
            if (TextEventsWidget != null && TextEventsWidget.DataProvider != null)
            {
                //Remove the event handlers
                TextEventsWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                TextEventsWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                TextEventsWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
                TextEventsWidget.DataProvider.OnFetchCancelled -= InvokeHandleFetchCancelled;
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(true);

            // Turned off the refresh if the customer end time is up.
            if (_dataProvider.EventFilterModel.TimeRange == TimeRangeHandler.Range.CustomRange &&
                !_dataProvider.EventFilterModel.CurrentTime &&
                DateTime.Now >= _dataProvider.EventFilterModel.TheEndTime)
            {
                _dataProvider.StopTimer();
            }
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(true);

            // Turned off the refresh if the customer end time is up.
            if (_dataProvider.EventFilterModel.TimeRange == TimeRangeHandler.Range.CustomRange &&
                !_dataProvider.EventFilterModel.CurrentTime &&
                DateTime.Now >= _dataProvider.EventFilterModel.TheEndTime)
            {
                _dataProvider.StopTimer();
            }
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(false);
        }

        private void HandleFetchCancelled(Object obj, EventArgs e)
        {
            _theFilterPanel.EnableDisableButtons(true);
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

        private void ShowFilters(bool showFlag)
        {
            _theFilterToolStrip.Visible = !showFlag;
            _theTexteventSplitContainer.Panel2Collapsed = !showFlag;
        }
        #endregion Private methods
    }
}
