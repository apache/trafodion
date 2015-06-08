#region Copyright info
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
    public partial class AuditLogUserControl : UserControl, ICloneToWindow
    {
        #region Fields
        private static readonly string AuditLogsConfigName = "Audit_Logs_Widget";
        UniversalWidgetConfig _widgetConfig = null;
        GenericUniversalWidget _auditLogsWidget;
        ConnectionDefinition _theConnectionDefinition;
        String _theTitle = "Audit Logs";
        ArrayList _commands = new ArrayList();
        AuditLogsDataHandler _dataDisplayHandler = null;
        AuditLogsDataProvider _dataProvider = null;
        DataTable _dataStore = null;
        ToolStrip _theToolStrip = null;
        Form _theParentForm = null;
        AuditLogDetails _theAuditLogDetails;
        public delegate void UpdateStatus(object obj, DataProviderEventArgs e);
        TrafodionProgressUserControl _progressUserControl;
        EventHandler<TrafodionProgressCompletedArgs> _progressHandler;
        ToolStripButton _messagePreviewButton = new ToolStripButton();
        ToolStripButton _auditlogConfigButton = new ToolStripButton();
   
         #endregion Fields

        #region Properties

        public AuditLogDetails TheAuditLogDetails
        {
            get { return _theAuditLogDetails; }
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
            set { _dataProvider = value as AuditLogsDataProvider; }
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

        public GenericUniversalWidget AuditLogsWidget
        {
            get { return _auditLogsWidget; }
            set { _auditLogsWidget = value; }
        }

        public bool ShowMessagePreview()
        {
            return _messagePreviewButton.Checked;
        }

        #endregion Properties

        #region Constructors

        public AuditLogUserControl()
        {
            InitializeComponent();
            this.Load += new EventHandler(AuditLogUserControl_Load);
        }



        public AuditLogUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefinition = aConnectionDefinition;
            _theAuditLogDetails = new AuditLogDetails(aConnectionDefinition);
            InitializeFilterControls();
        }

        public AuditLogUserControl(AuditLogUserControl anAuditLogUserControl)
            : this(anAuditLogUserControl.ConnectionDefn)
        {
        }

        #endregion Constructors

        private void AuditLogUserControl_Load(object sender, EventArgs e)
        {
            _theParentForm = this.Parent as Form;
            if (_theParentForm != null)
            {
                _theParentForm.FormClosing += new FormClosingEventHandler(_theParentForm_FormClosing);
            }
        }

        private void InitializeFilterControls()
        {
            _theContentPanel.Visible = true;
            _theFilterPanel.TheAuditLogDetails = _theAuditLogDetails;
            _theFilterPanel.ConnectionDefinition = _theConnectionDefinition;
            _theFilterPanel.PopulateUIFromPersistence(AuditLogFilterModel.AuditLogFilterPersistenceKey);
            _theApplyFilterButton.Enabled = false;
            _theResetButton.Enabled = false;
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


        //Stop the timer and the provider when the control exists
        private void _theParentForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (_dataProvider != null)
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
        /// To clone a self.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            AuditLogUserControl theAuditLogUserControl = new AuditLogUserControl(this);
            return theAuditLogUserControl;
        }

        public void PersistConfiguration()
        {

        }
        public void ShowFilterMessage()
        {
            string filterString = _theFilterPanel.GetFilterDisplayString(_dataDisplayHandler);
            ShowFilterMessage(filterString);
        }

        public void ShowFilterMessage(string message)
        {
            if ((message == null) || (message.Trim().Length == 0))
            {
                _theFilterListPanel.Visible = false;
                this._theFilterListPanel.Controls.Remove(_theToolStrip);
                this._auditLogsWidget.ReAttachToolStrip();
            }
            else
            {
                _theFilterListPanel.Visible = true;
                _theToolStrip = _auditLogsWidget.GetDetachedToolStrip();
                _theToolStrip.Dock = DockStyle.Top;
                this._theFilterListPanel.Controls.Add(_theToolStrip);
                _theFiltertext.Text = "Filter criteria: \n" + message;
            }
        }


        private void ShowWidgets()
        {
            _theFilterPanel.OnFilterChangedImpl = this.OnFilterChangedImpl;
            this._theGraphEventSplitContainer.Panel1Collapsed = true;

            //Create a UW using the configuration
            _widgetConfig = WidgetRegistry.GetConfigFromPersistence(AuditLogsConfigName);
            if (_widgetConfig == null)
            {
                _widgetConfig = new UniversalWidgetConfig();
                _widgetConfig.Name = AuditLogsConfigName;
                _widgetConfig.Title = "Audit Logs";
                _widgetConfig.ShowProperties = false;
                _widgetConfig.ShowToolBar = true;
                _widgetConfig.ShowChart = false;
                _widgetConfig.ShowRowCount = true;                
            }
            _widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.Show;

            AuditLogDataProviderConfig dbConfig = new AuditLogDataProviderConfig();
            if (_widgetConfig.DataProviderConfig == null)
            {
                List<string> defaultVisibleColumns = new List<string>
                    {
                        AuditLogFilterModel.AUDITLOG_TIME,
                        AuditLogFilterModel.AUDITTYPE,                        
                        AuditLogFilterModel.INTERNALUSERNAME,
                        AuditLogFilterModel.EXTERNALUSERNAME,
                        AuditLogFilterModel.TRANSACTION_ID,
                        AuditLogFilterModel.SESSION_ID,
                        AuditLogFilterModel.SQLCODE,
                        AuditLogFilterModel.OUTCOME,
                        AuditLogFilterModel.MESSAGE
                        
                    };
                dbConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

                _widgetConfig.DataProviderConfig = dbConfig;
                _widgetConfig.DataProviderConfig.RefreshRate = 60;
                _widgetConfig.DataProviderConfig.ColumnSortObjects = null;
            }
            else
            {
                dbConfig = (AuditLogDataProviderConfig)_widgetConfig.DataProviderConfig;
            }
            dbConfig.ColumnMappings = new List<ColumnMapping>()
            {
                new ColumnMapping(AuditLogFilterModel.AUDITLOG_TIME, AuditLogFilterModel.AUDITLOG_TIME, 200),
                new ColumnMapping(AuditLogFilterModel.AUDITTYPE, AuditLogFilterModel.AUDITTYPE, 120),
                new ColumnMapping(AuditLogFilterModel.INTERNALUSERNAME, AuditLogFilterModel.INTERNALUSERNAME, 120),
                new ColumnMapping(AuditLogFilterModel.EXTERNALUSERNAME, AuditLogFilterModel.EXTERNALUSERNAME, 120),
                new ColumnMapping(AuditLogFilterModel.USER_ID, AuditLogFilterModel.USER_ID, 70),
                new ColumnMapping(AuditLogFilterModel.TRANSACTION_ID, AuditLogFilterModel.TRANSACTION_ID, 60),
                new ColumnMapping(AuditLogFilterModel.SESSION_ID, AuditLogFilterModel.SESSION_ID, 120),
                new ColumnMapping(AuditLogFilterModel.SQLCODE, AuditLogFilterModel.SQLCODE, 80),
                new ColumnMapping(AuditLogFilterModel.OUTCOME, AuditLogFilterModel.OUTCOME, 120),
                new ColumnMapping(AuditLogFilterModel.MESSAGE, AuditLogFilterModel.MESSAGE, 120)
            };
            //show help button
            _widgetConfig.ShowHelpButton = true;
            _widgetConfig.HelpTopic = HelpTopics.UseAuditLogViewer;

            //make sure timer is not paused
            _widgetConfig.DataProviderConfig.TimerPaused = false;

            //set the connection definition and the data provider
            dbConfig.ConnectionDefinition = _theConnectionDefinition;
            _dataProvider = (AuditLogsDataProvider)dbConfig.GetDataProvider();

            //initialize the filter model of the data provider from the filter panel
            _dataProvider.AuditLogFilterModel = _theFilterPanel.FilterModel;

            //Create the Universal widget to diaplay the data
            _auditLogsWidget = new GenericUniversalWidget();
            _auditLogsWidget.UniversalWidgetConfiguration = _widgetConfig;

            _messagePreviewButton.Text = "Preview Message";
            _messagePreviewButton.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
            _messagePreviewButton.CheckOnClick = true;
            _messagePreviewButton.ImageScaling = ToolStripItemImageScaling.None;
            _messagePreviewButton.Checked = false;
            _messagePreviewButton.Image = _messagePreviewButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            _messagePreviewButton.CheckedChanged += new EventHandler(_messagePreviewButton_CheckedChanged);
            _auditLogsWidget.AddToolStripItem(_messagePreviewButton);


            _auditlogConfigButton.Text = global::Trafodion.Manager.Properties.Resources.SecurityAuditLogsConfig;
            _auditlogConfigButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            _auditlogConfigButton.Image = Trafodion.Manager.Properties.Resources.ConfigureIcon;
            _auditlogConfigButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _auditlogConfigButton.Name = "_auditlogConfigButton";
            _auditlogConfigButton.Click += new EventHandler(_auditlogConfigButton_Click);
            _auditLogsWidget.AddToolStripItem(_auditlogConfigButton);

            if (ConnectionDefinition.ComponentPrivilegeExists("AUDIT_LOGGING", "UPDATE_CONFIGURATION"))
            {
                _auditlogConfigButton.Enabled = true;
            }
            else
            {
                _auditlogConfigButton.Enabled = false;
            }

            _auditLogsWidget.AddToolStripItem(new ToolStripSeparator());

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_auditLogsWidget.DataDisplayControl).LineCountFormat = "Fetching audit logs from server...";
            _dataDisplayHandler = new AuditLogsDataHandler(this);
            _dataDisplayHandler.ServerTimeZone = ConnectionDefn.ServerTimeZoneName;
            _dataProvider.ColumnsToFilterOn = _dataDisplayHandler.Columns;
            _auditLogsWidget.DataDisplayControl.DataDisplayHandler = _dataDisplayHandler;
            _auditLogsWidget.UniversalWidgetConfiguration = _widgetConfig;
            _auditLogsWidget.Dock = DockStyle.Fill;
            _theWidgetPanel.Controls.Add(_auditLogsWidget);

            //Add event handlers to deal with data provider events
            AddHandlers();

            ((TabularDataDisplayControl)_auditLogsWidget.DataDisplayControl).DataGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(DataGrid_CellClick);
            

            //Keep DataGrid data when error encountered. 
            ((TabularDataDisplayControl)_auditLogsWidget.DataDisplayControl).KeepDataGridOnError = true;

            //Show for all events
            _auditLogsWidget.StartDataProvider();
            //Add Disposed handler to clean up, connection
            this.Disposed += new EventHandler(AuditLogsCanvas_Disposed);
        }


        void _auditlogConfigButton_Click(object sender, EventArgs e)
        {
            if (ConnectionDefinition != null)
            {
                AuditLoggingConfiguration auditLoggingConfiguration = new AuditLoggingConfiguration(ConnectionDefinition);
                auditLoggingConfiguration.ShowDialog();
            }
        }
        void _messagePreviewButton_CheckedChanged(object sender, EventArgs e)
        {
            // Show/Hide Preview Message 
            _messagePreviewButton.Image = _messagePreviewButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            AuditLogsDataHandler displayHandler = _auditLogsWidget.DataDisplayControl.DataDisplayHandler as AuditLogsDataHandler;
            displayHandler.showHidePreviewMessage(_messagePreviewButton.Checked);
        }

        void DataGrid_CellClick(object sender, TenTec.Windows.iGridLib.iGCellClickEventArgs e)
        {
            TenTec.Windows.iGridLib.iGrid grid = sender as TenTec.Windows.iGridLib.iGrid;
            if (grid != null)
            {
                String columnName = grid.Cols[e.ColIndex].Text as string;
                object value = grid.Cells[e.RowIndex, e.ColIndex].Value;
                if (!string.IsNullOrEmpty(columnName))
                _theFilterPanel.SetFilterValues(columnName, value);
            }
        }

        private void AuditLogsCanvas_Disposed(object sender, EventArgs e)
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
        private void OnFilterChangedImpl(AuditLogFilterModel aFilterModel)
        {
            if (_dataProvider.FetchInProgress)
            {
                _dataProvider.Stop();
            }
            _dataProvider.AuditLogFilterModel = aFilterModel;
            // Always wants the refresh first
            _dataProvider.StartTimer();
            _dataProvider.Start();
            ShowFilterMessage(aFilterModel.GetFormattedFilterString(_dataDisplayHandler));
        }

        //associates the data provider handlers
        private void AddHandlers()
        {
            if (_auditLogsWidget != null && _auditLogsWidget.DataProvider != null)
            {
                //Associate the event handlers
                _auditLogsWidget.DataProvider.OnErrorEncountered += InvokeHandleError;
                _auditLogsWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _auditLogsWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
                _auditLogsWidget.DataProvider.OnFetchCancelled += InvokeHandleFetchCancelled;
            }
        }

        //removes the data provider handlers
        private void RemoveHandlers()
        {
            if (_auditLogsWidget != null && _auditLogsWidget.DataProvider != null)
            {
                //Remove the event handlers
                _auditLogsWidget.DataProvider.OnErrorEncountered -= InvokeHandleError;
                _auditLogsWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _auditLogsWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData;
                _auditLogsWidget.DataProvider.OnFetchCancelled -= InvokeHandleFetchCancelled;
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {
            _messagePreviewButton.Enabled = false;
            _theFilterPanel.EnableDisableButtons(true);

            // Turned off the refresh if the customer end time is up.
            if (_dataProvider.AuditLogFilterModel.TimeRange == TimeRangeHandler.Range.CustomRange &&
                !_dataProvider.AuditLogFilterModel.CurrentTime &&
                DateTime.Now >= _dataProvider.AuditLogFilterModel.TheEndTime)
            {
                _dataProvider.StopTimer();
            }
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            _messagePreviewButton.Enabled = true;
            _theFilterPanel.EnableDisableButtons(true);

            // Turned off the refresh if the customer end time is up.
            if (_dataProvider.AuditLogFilterModel.TimeRange == TimeRangeHandler.Range.CustomRange &&
                !_dataProvider.AuditLogFilterModel.CurrentTime &&
                DateTime.Now >= _dataProvider.AuditLogFilterModel.TheEndTime)
            {
                _dataProvider.StopTimer();
            }
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            _messagePreviewButton.Enabled = false;
            _theFilterPanel.EnableDisableButtons(false);
        }

        private void HandleFetchCancelled(Object obj, EventArgs e)
        {
            _messagePreviewButton.Enabled = true;
            _theFilterPanel.EnableDisableButtons(true);
        }

        private void _theShowAuditLogFilterButton_Click(object sender, EventArgs e)
        {
            ShowFilters(true);
        }

        private void _theCloseFilterPanelButton_Click(object sender, EventArgs e)
        {
            ShowFilters(false);
        }

        private void _theHelpToolStripButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseAuditLogFilter);
        }

        private void ShowFilters(bool showFlag)
        {
            _theFilterToolStrip.Visible = !showFlag;
            _theTexteventSplitContainer.Panel2Collapsed = !showFlag;
        }

        private void _theResetButton_Click(object sender, EventArgs e)
        {
            _theFilterPanel.Reset();
        }

        private void _theFilterPanel_OnUpdateButtonsImpl()
        {
            _theApplyFilterButton.Enabled = (_theFilterPanel.TheModelChanged && _theFilterPanel.TheButtonsEnabled);
            _theResetButton.Enabled = _theFilterPanel.TheButtonsEnabled;
        }

        private void _theApplyFilterButton_Click(object sender, EventArgs e)
        {
            _theFilterPanel.ApplyFilter();
        }
    }
}
