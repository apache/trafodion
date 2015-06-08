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
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSQueryDetailUserControl : UserControl, IMenuProvider
    {
        #region Fields

        private const string WMSQueryDetailPersistenceKey = "WMSQueryDetailPKey_" + WMSQueryDetailInfoUtils.MetricVersion;
        private const string WMSQueryMetricsPersistenceKey = "WMSQueryMetricsPKey_" + WMSQueryDetailInfoUtils.MetricVersion;
        private const string WMSConnectionMetricsPersistenceKey = "WMSConnectionMetricsPKey_" + WMSQueryDetailInfoUtils.MetricVersion;
        private const string WMSTimeBasedMetricsPersistenceKey = "WMSTimeBasedMetricsPKey_" + WMSQueryDetailInfoUtils.MetricVersion;
        private const string WMSCompileTimeMetricsPersistenceKey = "WMSCompileTimeMetricsPKey_" + WMSQueryDetailInfoUtils.MetricVersion;
        private const string WMSRunTimeMetricsPersistenceKey = "WMSRunTimeMetricsPKey_" + WMSQueryDetailInfoUtils.MetricVersion;

        private static Size ChildrenWindowSize = new Size(800, 600);

        private bool m_hasFullQueryText = false;
        private WMSWorkloadCanvas m_parent = null;
        private ConnectionDefinition m_theConnectionDefinition = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private DataTable m_dataTable = null;
        private DataTable m_previousDataTable = null;
        private string m_title = "";
        private UniversalWidgetConfig m_config1 = null;
        private GenericUniversalWidget m_widget1 = null;
        WidgetContainer m_widgetContainer1 = null;
        private WMSQueryMetricsUserControl m_timeBasedMetrics = null;
        private WMSQueryMetricsUserControl m_connectionFacts = null;
        private WMSQueryMetricsUserControl m_overflowMetrics = null;
        private WMSQueryMetricsUserControl m_compileTimeMetrics = null;
        private WMSQueryMetricsUserControl m_runTimeMetrics = null;
        private WMSQueryMetricsUserControl m_rateBasedMetrics = null;
        private SqlStatementTextBox m_sqlTextBox;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);

        private List<string> m_selectedMetricsTimeBased = null;
        private List<string> m_selectedMetricsConnection = null;
        private List<string> m_selectedMetricsCompileTime = null;
        private List<string> m_selectedMetricsRunTime = null;
        private string _previewText = "";
        private Dictionary<string, object> m_queryMetricsPersistenceStore;

        public static readonly Size IdealWindowSize = new Size(1350, 700);

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: HasFullQueryText - has gotten the full query text already
        /// </summary>
        public bool HasFullQueryText
        {
            get { return m_hasFullQueryText; }
            set { m_hasFullQueryText = value; }
        }

        /// <summary>
        /// Property: QueryId - the query ID
        /// </summary>
        public string QueryId
        {
            get { return m_qid; }
            set { m_qid = _theQueryIdTextBox.Text = value; }
        }

        /// <summary>
        /// Property: QueryState - the current query state
        /// </summary>
        public string QueryState
        {
            get { return _theStateTextBox.Text; }
            set { _theStateTextBox.Text = value; }
        }

        /// <summary>
        /// Property: QuerySubState - the current query substate
        /// </summary>
        public string QuerySubState
        {
            get { return _theSubStateTextBox.Text; }
            set { _theSubStateTextBox.Text = value; }
        }

        /// <summary>
        /// Property: WarningLevel - the current warning level
        /// </summary>
        public string WarningLevel
        {
            get { return _theWarnLevelTextBox.Text; }
            set { _theWarnLevelTextBox.Text = value; }
        }

        /// <summary>
        /// Property: ConnectionDefinition - the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefinition
        {
            get { return m_theConnectionDefinition; }
        }

        /// <summary>
        /// Property: QueryStartTS - the start time of the query
        /// </summary>
        public string QueryStartTime
        {
            get { return m_start_ts; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="cd"></param>
        /// <param name="qid"></param>
        /// <param name="start_ts"></param>
        /// <param name="dataTable"></param>
        public WMSQueryDetailUserControl(WMSWorkloadCanvas parent, ConnectionDefinition cd, string qid, string start_ts, DataTable dataTable)
        {
            InitializeComponent();
            m_parent = parent;
            m_theConnectionDefinition = cd;
            m_qid = qid;
            m_start_ts = start_ts;
            m_dataTable = dataTable;
            if (parent is OffenderWorkloadCanvas)
                m_title = Properties.Resources.TitleWMSOffender;
            else if (parent is MonitorWorkloadCanvas)
                m_title = Properties.Resources.TitleMonitorWorkload;

            ShowWidgets();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To reset the SQL whiteboard layout
        /// </summary>
        public void ResetLayout()
        {
            this._widgetCanvas.ResetWidgetLayout();
        }

        /// <summary>
        /// To load the new data 
        /// </summary>
        /// <param name="aDataTable"></param>
        public void LoadData(DataTable aDataTable)
        {
            LoadNewData(aDataTable);
        }

        /// <summary>
        /// Reset data 
        /// </summary>
        public void ResetData()
        {
            ResetAllMetrics();
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To construct all widgets
        /// </summary>
        private void ShowWidgets()
        {
            this._widgetCanvas.ThePersistenceKey = WMSQueryDetailPersistenceKey;
            LoadQueryMetricsPersistenceStore();

            QueryId = m_qid;
            QueryState = (m_dataTable.Rows[0]["QUERY_STATE"] as string).Trim();
            QuerySubState = (m_dataTable.Rows[0]["QUERY_SUBSTATE"] as string).Trim();
            WarningLevel = (m_dataTable.Rows[0]["WARN_LEVEL"] as string).Trim();

            string text = m_dataTable.Rows[0]["QUERY_TEXT"] as string;
            int length = (int)m_dataTable.Rows[0]["QUERY_TEXT_LEN"]; 

            if (text != null)
            {
                text = text.Trim();
            }
            else
            {
                text = "";
            }
            _previewText = text;

            string title = (text.Length >= length) ? Properties.Resources.TitleFullQueryText : Properties.Resources.TitlePreviewQueryText;

            Color initialFontColor = Color.MidnightBlue;

            GridLayoutManager gridLayoutManager = new GridLayoutManager(12, 4);
            gridLayoutManager.CellSpacing = 4;
            this._widgetCanvas.LayoutManager = gridLayoutManager;

            // Create the first widget for query text preview
            DatabaseDataProviderConfig dbConfig = new DatabaseDataProviderConfig();
            dbConfig.ConnectionDefinition = m_theConnectionDefinition;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText;
            m_dbDataProvider.QueryID = m_qid;
            m_dbDataProvider.START_TS = m_start_ts;

            m_config1 = new UniversalWidgetConfig();
            m_config1.ShowToolBar = false;
            m_config1.ShowProviderToolBarButton = false;
            m_config1.ShowProviderStatus = false;
            m_config1.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            m_config1.DataProviderConfig = dbConfig;
            m_config1.Name = title;
            m_config1.Title = title;

            m_widget1 = new GenericUniversalWidget();
            m_widget1.DataProvider = m_dbDataProvider;
            m_widget1.UniversalWidgetConfiguration = m_config1;

            m_sqlTextBox = new SqlStatementTextBox();
            m_sqlTextBox.Text = text;
            m_sqlTextBox.WordWrap = true;
            m_sqlTextBox.ReadOnly = true;
            m_sqlTextBox.Dock = DockStyle.Fill;

            GenericDataDisplayControl gddc = new GenericDataDisplayControl(m_sqlTextBox);
            m_widget1.DataDisplayControl = gddc;
            m_widget1.DataProvider = m_dbDataProvider;

            GridConstraint gridConstraint = new GridConstraint(0, 0, 2, 4);
            m_widgetContainer1 = new WidgetContainer(_widgetCanvas, m_widget1, title);
            m_widgetContainer1.Name = Properties.Resources.TitlePreviewQueryText;
            m_widgetContainer1.AllowDelete = false;
            this._widgetCanvas.AddWidget(m_widgetContainer1, gridConstraint, -1);

            // Create the 2nd widget for monitoring the connection metrics
            DataRow dr = getDataRow(m_dataTable);

            List<string> undefinedColumns;
            DataTable connectionMetricData = GetConnectionFacts(dr, out undefinedColumns);
            List<string> availableMetricsConnection = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.Connection);
            // Add undefined columns into the available metrics
            availableMetricsConnection.AddRange(undefinedColumns);
            // Add undefined columns into the selected metrics, except duplicated ones
            undefinedColumns.ForEach(
                column =>
                {
                    if (!m_selectedMetricsConnection.Contains(column)
                        && !availableMetricsConnection.Contains(column))
                    {
                        m_selectedMetricsConnection.Add(column);
                    }
                });
            EliminateColumnsNotAvailable(availableMetricsConnection, m_selectedMetricsConnection);
            m_connectionFacts = new WMSQueryMetricsUserControl(WMSQueryDetailInfoUtils.Metrics.Connection, availableMetricsConnection, m_selectedMetricsConnection);
            m_connectionFacts.LoadInitialData(connectionMetricData, initialFontColor);
            m_connectionFacts.Dock = DockStyle.Fill;

            gridConstraint = new GridConstraint(2, 0, 10, 1);
            WidgetContainer widgetContainer = new WidgetContainer(_widgetCanvas, m_connectionFacts, Properties.Resources.TitleConnectionMetrics);
            widgetContainer.Name = Properties.Resources.TitleConnectionMetrics;
            widgetContainer.AllowDelete = false;
            this._widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // Create the 3rd widget for monitoring the timebased metrics
            List<string> availableMetricsTimeBased = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.TimeBased);
            EliminateColumnsNotAvailable(availableMetricsTimeBased, m_selectedMetricsTimeBased);           
            m_timeBasedMetrics = new WMSQueryMetricsUserControl(WMSQueryDetailInfoUtils.Metrics.TimeBased, availableMetricsTimeBased, m_selectedMetricsTimeBased);
            m_timeBasedMetrics.LoadInitialData(GetTimeBasedMetrics(dr, null), initialFontColor);
            m_timeBasedMetrics.Dock = DockStyle.Fill;

            m_timeBasedMetrics.Margin = m_timeBasedMetrics.Padding = new Padding(0);
            gridConstraint = new GridConstraint(2, 1, 10, 1);
            widgetContainer = new WidgetContainer(_widgetCanvas, m_timeBasedMetrics, Properties.Resources.TitleTimeBasedMetrics);
            widgetContainer.Name = Properties.Resources.TitleTimeBasedMetrics;
            widgetContainer.AllowDelete = false;
            this._widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // Create the 4th widget for monitoring the compile-time metrics
            List<string> availableMetricsCompileTime = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.CompileTime);
            EliminateColumnsNotAvailable(availableMetricsCompileTime, m_selectedMetricsCompileTime);
            m_compileTimeMetrics = new WMSQueryMetricsUserControl(WMSQueryDetailInfoUtils.Metrics.CompileTime, availableMetricsCompileTime, m_selectedMetricsCompileTime);
            m_compileTimeMetrics.LoadInitialData(GetCompileTimeMetrics(dr), initialFontColor);
            m_compileTimeMetrics.Dock = DockStyle.Fill;

            gridConstraint = new GridConstraint(2, 2, 10, 1);
            widgetContainer = new WidgetContainer(_widgetCanvas, m_compileTimeMetrics, Properties.Resources.TitleCompileTimeMetrics);
            widgetContainer.Name = Properties.Resources.TitleCompileTimeMetrics;
            widgetContainer.AllowDelete = false;
            this._widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // Create the 5th widget for monitoring the runtime metrics
            List<string> availableMetricsRunTime = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.RunTime);
            EliminateColumnsNotAvailable(availableMetricsRunTime, m_selectedMetricsRunTime);
            m_runTimeMetrics = new WMSQueryMetricsUserControl(WMSQueryDetailInfoUtils.Metrics.RunTime, availableMetricsRunTime, m_selectedMetricsRunTime);
            m_runTimeMetrics.LoadInitialData(GetRunTimeMetrics(dr), initialFontColor);
            m_runTimeMetrics.Dock = DockStyle.Fill;

            gridConstraint = new GridConstraint(2, 3, 10, 1);
            widgetContainer = new WidgetContainer(_widgetCanvas, m_runTimeMetrics, Properties.Resources.TitleRunTimeMetrics);
            widgetContainer.Name = Properties.Resources.TitleRunTimeMetrics;
            widgetContainer.AllowDelete = false;
            this._widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            EnableDisableButtons();

            _refreshButton.Click += _refreshButton_Click;
            _getReposButton.Click += GetRepositoryInfoButton_Click;
            _getSqlPlanButton.Click += GetSQLPlanButton_Click;
            _getSqlTextButton.Click += GetSQLTextButton_Click;
            _cancelButton.Click += CancelButton_Click;

            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered += InvokeHandleError;
            Persistence.PersistenceHandlers += PersistencePersistenceHandlers;
            this._widgetCanvas.InitializeCanvas();

            // We need to reset the text title back to preview 
            m_widgetContainer1.Text = title;

            if (title.Equals(Properties.Resources.TitleFullQueryText))
            {
                //_getSqlTextButton.Enabled = false;
                HasFullQueryText = true;
            }
        }

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                Persistence.PersistenceHandlers -= PersistencePersistenceHandlers;
                m_dbDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                m_dbDataProvider.OnErrorEncountered -= InvokeHandleError;

                if (this.m_parent != null)
                {
                    this.m_parent.RemoveQueryFromWatch(this);
                }

                if (m_dbDataProvider != null)
                {
                    m_dbDataProvider.Stop();
                    m_dbDataProvider = null;
                }

                this.m_widget1.Dispose();
                this.m_connectionFacts.Dispose();
                this.m_timeBasedMetrics.Dispose();
                this.m_compileTimeMetrics.Dispose();
                this.m_runTimeMetrics.Dispose();
                this._widgetCanvas.Dispose();
            }
            base.Dispose(disposing);
        }

        private void EliminateColumnsNotAvailable(List<string> availableColumns, List<string> selectedColumns)
        {
            List<string> columnsToBeRemoved = new List<string>();
            foreach (string selectedColumn in selectedColumns)
            {
                if (!availableColumns.Contains(selectedColumn))
                {
                    columnsToBeRemoved.Add(selectedColumn);
                }
            }
            foreach (string column in columnsToBeRemoved)
            {
                selectedColumns.Remove(column);
            }
        }

        /// <summary>
        /// Load all metric configurations from persistence store
        /// </summary>
        private void LoadQueryMetricsPersistenceStore()
        {
            m_queryMetricsPersistenceStore = Persistence.Get(WMSQueryMetricsPersistenceKey) as Dictionary<string, object>;
            if (m_queryMetricsPersistenceStore == null)
            {
                // Set default values
                m_queryMetricsPersistenceStore = new Dictionary<string, object>();
                m_selectedMetricsTimeBased = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.TimeBased);
                m_selectedMetricsConnection = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.Connection);
                m_selectedMetricsCompileTime = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.CompileTime);
                m_selectedMetricsRunTime = WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.RunTime);
            }
            else
            {
                object obj = null;
                m_queryMetricsPersistenceStore.TryGetValue(WMSQueryDetailInfoUtils.Metrics.TimeBased.ToString(), out obj);
                    m_selectedMetricsTimeBased = (obj == null) ? 
                        WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.TimeBased) : obj as List<string>;

                m_queryMetricsPersistenceStore.TryGetValue(WMSQueryDetailInfoUtils.Metrics.Connection.ToString(), out obj);
                m_selectedMetricsConnection = (obj == null) ? 
                        WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.Connection) : obj as List<string>;

                m_queryMetricsPersistenceStore.TryGetValue(WMSQueryDetailInfoUtils.Metrics.CompileTime.ToString(), out obj);
                m_selectedMetricsCompileTime = (obj == null) ?
                        WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.CompileTime) : obj as List<string>;

                m_queryMetricsPersistenceStore.TryGetValue(WMSQueryDetailInfoUtils.Metrics.RunTime.ToString(), out obj);
                m_selectedMetricsRunTime = (obj == null) ?
                        WMSQueryDetailInfoUtils.GetMetricDisplayNames(WMSQueryDetailInfoUtils.Metrics.RunTime) : obj as List<string>;
            }
        }

        /// <summary>
        /// New data arrival invoker
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(m_dbDataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Event handler for new data arrived from the data provider
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            switch (m_dbDataProvider.FetchRepositoryDataOption)
            {
                case WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText:
                    if (m_dbDataProvider.QueryText != null && m_dbDataProvider.QueryText.Length > 0)
                    {
                        m_sqlTextBox.Text = m_dbDataProvider.QueryText + Environment.NewLine;
                        m_widgetContainer1.Text = Properties.Resources.TitleFullQueryText;
                        _getSqlTextButton.Enabled = false;
                        HasFullQueryText = true;
                    }
                    break;

                case WMSOffenderDatabaseDataProvider.FetchDataOption.Option_CancelQuery:
                    MessageBox.Show(Utilities.GetForegroundControl(), 
                                    "The query was cancelled successfully", 
                                    m_title, 
                                    MessageBoxButtons.OK, 
                                    MessageBoxIcon.Information);

                    //Perform a refresh; Note: the query may not have been cancelled at this time. 
                    PerformRefresh();
                    EnableDisableButtons();
                    break;

                case WMSOffenderDatabaseDataProvider.FetchDataOption.Option_QueryDetails:
                    LoadNewData(m_dbDataProvider.DatabaseDataTable);
                    break;
            }
        }

        /// <summary>
        /// The invoker on data provider error
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(m_dbDataProvider_OnErrorEncountered), new object[] { obj, (DataProviderEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// Event handler for provider error
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void m_dbDataProvider_OnErrorEncountered(object sender, DataProviderEventArgs e)
        {
            if (m_dbDataProvider.FetchRepositoryDataOption == WMSOffenderDatabaseDataProvider.FetchDataOption.Option_CancelQuery)
            {
                if (e.Exception.Message.Contains("not found"))
                {
                    _refreshButton.Enabled = _cancelButton.Enabled = _getPerTableStats.Enabled = false;
                    _theStateTextBox.Text = "COMPLETED";
                    _theSubStateTextBox.Text = string.Empty;
                    m_compileTimeMetrics.ResetData();
                    m_connectionFacts.ResetData();
                    m_timeBasedMetrics.ResetData();
                    m_runTimeMetrics.ResetData();
                    MessageBox.Show("Cannot cancel the query. The query is no longer available in WMS.",
                                    "Warning",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Warning);
                }
                else
                {
                    MessageBox.Show(string.Format("Cancel query failed with the following exception: {0}", e.Exception.Message),
                                    "Cancel Query Failed",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);

                    // Reenable the button
                    _cancelButton.Enabled = true;
                }
            }
            else if (m_dbDataProvider.FetchRepositoryDataOption == WMSOffenderDatabaseDataProvider.FetchDataOption.Option_QueryDetails)
            {
                if (e.Exception.Message.Contains("not found"))
                {
                    _refreshButton.Enabled = _cancelButton.Enabled = _getPerTableStats.Enabled = false;
                    _theStateTextBox.Text = "COMPLETED";
                    _theSubStateTextBox.Text = string.Empty;
                    m_compileTimeMetrics.ResetData();
                    m_connectionFacts.ResetData();
                    m_timeBasedMetrics.ResetData();
                    m_runTimeMetrics.ResetData();
                    MessageBox.Show("The query is no longer available in WMS. Use the Repo info button to look up the query details from repository.",
                                    "Warning",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Warning);
                }
                else
                {
                    MessageBox.Show(string.Format("Fetch query details failed with the following exception: {0}", e.Exception.Message),
                                    "Fetch Query Details Failed",
                                    MessageBoxButtons.OK,
                                    MessageBoxIcon.Error);
                }
            }
        }

        /// <summary>
        /// Event handler for Refresh button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _refreshButton_Click(object sender, EventArgs e)
        {
            //getWMSInfo(true);
            //EnableDisableButtons();
            PerformRefresh();
        }

        private void PerformRefresh()
        {
            if (!m_dbDataProvider.FetchInProgress)
            {
                m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_QueryDetails;
                m_dbDataProvider.QueryID = m_qid;
                m_dbDataProvider.START_TS = m_start_ts;

                m_dbDataProvider.Start();
            }
            else
            {
                ReportFetchInProgress();
            }
        }

        void ReportFetchInProgress()
        {
            MessageBox.Show("Another fetch is in progress. Please retry later.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);

        }

        /// <summary>
        /// Load the newly arrived data to all of the metrics
        /// </summary>
        /// <param name="aDataTable"></param>
        private void LoadNewData(DataTable aDataTable)
        {
            if (aDataTable == null)
            {
                return;
            }

            DataRow dr = getDataRow(aDataTable);
            DataRow oldRow = getDataRow(m_dataTable);

            if (null != dr)
            {
                // Update the query state info
                QueryState = (dr["QUERY_STATE"] as string).Trim();
                QuerySubState = (dr["QUERY_SUBSTATE"] as string).Trim();
                WarningLevel = (dr["WARN_LEVEL"] as string).Trim();

                Color initialFontColor = (m_parent is MonitorWorkloadCanvas) ? MonitorWorkloadOptions.GetOptions().HighLightChangesColor : Color.MidnightBlue;
                Color unchangedColor = Color.MidnightBlue;
                Color changedColor;

                if (m_parent is MonitorWorkloadCanvas)
                {
                    if (MonitorWorkloadOptions.GetOptions().HighLightChanges)
                    {
                        changedColor = MonitorWorkloadOptions.GetOptions().HighLightChangesColor;
                    }
                    else
                    {
                        changedColor = unchangedColor;
                    }
                }
                else
                {
                    changedColor = Color.Blue;
                }

                List<string> undefinedColumns;
                DataTable connectionMetricData = GetConnectionFacts(dr, out undefinedColumns);
                m_connectionFacts.LoadNewData(connectionMetricData, unchangedColor, changedColor);
                m_timeBasedMetrics.LoadNewData(GetTimeBasedMetrics(dr, oldRow), unchangedColor, changedColor);
                m_compileTimeMetrics.LoadNewData(GetCompileTimeMetrics(dr), unchangedColor, changedColor);
                m_runTimeMetrics.LoadNewData(GetRunTimeMetrics(dr), unchangedColor, changedColor);

                EnableDisableButtons();
            }

            m_dataTable = aDataTable;
        }

        /// <summary>
        /// Reset all metrics
        /// </summary>
        private void ResetAllMetrics()
        {
            m_connectionFacts.ResetData();
            m_timeBasedMetrics.ResetData();
            m_compileTimeMetrics.ResetData();
            m_runTimeMetrics.ResetData();
        }

        /// <summary>
        /// Facility utilities for adding metric columns
        /// </summary>
        /// <param name="aDataTable"></param>
        private void AddColumns(DataTable aDataTable)
        {
            aDataTable.Columns.Add(WMSQueryMetricsUserControl.DATATABLE_COL_NAME, typeof(string));
            aDataTable.Columns.Add(WMSQueryMetricsUserControl.DATATABLE_COL_VALUE, typeof(object));
            aDataTable.Columns.Add(WMSQueryMetricsUserControl.DATATABLE_COL_FORMAT, typeof(string));
        }

        /// <summary>
        /// Extra Time-Based metrics from the current WMS data row
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private DataTable GetTimeBasedMetrics(DataRow newRow, DataRow oldRow)
        {
            DataTable table = new DataTable();
            if (null != newRow)
            {
                AddColumns(table);

                foreach (WMSQueryDetailInfoUtils.TimeBasedMetrics myType in Enum.GetValues(typeof(WMSQueryDetailInfoUtils.TimeBasedMetrics)))
                {
                    //Previous M10,we don't have these 3 columns
                    if (m_theConnectionDefinition.ServerVersion <= ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        if (myType == WMSQueryDetailInfoUtils.TimeBasedMetrics.SQL_CPU_OFFENDER_INTERVAL_SECS ||
                            myType == WMSQueryDetailInfoUtils.TimeBasedMetrics.SQL_TSE_OFFENDER_INTERVAL_SECS ||
                            myType == WMSQueryDetailInfoUtils.TimeBasedMetrics.SQL_SLOW_OFFENDER_INTERVAL_SECS)
                        {
                            continue;
                        }
                    }
                    string name = WMSQueryDetailInfoUtils.GetDisplayName(WMSQueryDetailInfoUtils.Metrics.TimeBased, (int)myType);

                    table.Rows.Add(new object[] { 
                            WMSQueryDetailInfoUtils.GetDisplayName(WMSQueryDetailInfoUtils.Metrics.TimeBased, (int)myType), 
                            WMSQueryDetailInfoUtils.ConvertMetrics(WMSQueryDetailInfoUtils.Metrics.TimeBased, (int)myType, newRow, oldRow), 
                            WMSQueryDetailInfoUtils.GetFormat(WMSQueryDetailInfoUtils.Metrics.TimeBased, (int)myType)                           
                            });
                }
            }

            return table;
        }

        /// <summary>
        /// Extra the connection metrics from the current WMS data row
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private DataTable GetConnectionFacts(DataRow dr, out List<string> undefinedColumns)
        {
            DataTable table = new DataTable();
            undefinedColumns = new List<string>();
            if (null != dr)
            {
                AddColumns(table);

                // Add rows according to defined columns
                foreach (WMSQueryDetailInfoUtils.ConnectionFacts myType in Enum.GetValues(typeof(WMSQueryDetailInfoUtils.ConnectionFacts)))
                {
                    table.Rows.Add(new object[] { 
                            WMSQueryDetailInfoUtils.GetDisplayName(WMSQueryDetailInfoUtils.Metrics.Connection, (int)myType), 
                            WMSQueryDetailInfoUtils.ConvertMetrics(WMSQueryDetailInfoUtils.Metrics.Connection, (int)myType, dr), 
                            WMSQueryDetailInfoUtils.GetFormat(WMSQueryDetailInfoUtils.Metrics.Connection, (int)myType) 
                            });
                }

                // Add rows according to undefined columns
                undefinedColumns = GetUndefinedColumns(dr.Table.Columns);
                foreach (string undefinedColumn in undefinedColumns)
                {
                    table.Rows.Add(
                            undefinedColumn,                            // Use the columns name by default 
                            Convert.ToString(dr[undefinedColumn]),      // Use the cell value converted to string by default
                            string.Empty                                // Use empty string as formatting string by default
                            );                    
                }
            }

            return table;
        }

        /// <summary>
        /// From the provided data columns, 
        /// extract the column names which cannot be found in the defined columns
        /// </summary>
        /// <param name="dataColumns"></param>
        /// <returns></returns>
        private List<string> GetUndefinedColumns(DataColumnCollection dataColumns)
        {
            /*
             * Get all metric columns defined in the code
            */
            string[] connectionMetricColumns = Enum.GetNames(typeof(WMSQueryDetailInfoUtils.ConnectionFacts));
            string[] timeBasedMetricColumns = Enum.GetNames(typeof(WMSQueryDetailInfoUtils.TimeBasedMetrics));
            string[] compileTimeMetricColumns = Enum.GetNames(typeof(WMSQueryDetailInfoUtils.CompileTimeMetrics));
            string[] runTimeMetricColumns = Enum.GetNames(typeof(WMSQueryDetailInfoUtils.RunTimeMetrics));

            /*
             * Combine all the defined metric columns into one
            */
            List<string> allDefinedColumns = new List<string>();
            allDefinedColumns.AddRange(connectionMetricColumns);
            allDefinedColumns.AddRange(timeBasedMetricColumns);
            allDefinedColumns.AddRange(compileTimeMetricColumns);
            allDefinedColumns.AddRange(runTimeMetricColumns);


            /*
             * Walk through the provided data columns, and extract the ones which cannot be found in the defined columns
            */
            List<string> undefinedColumns = new List<string>();
            if (dataColumns != null && dataColumns.Count > 0)
            {
                foreach (DataColumn dataColumn in dataColumns)
                {
                    // Indicates if the data column can be found in the combined defined columns
                    bool isDataColumnFoundInDefinedColumns = allDefinedColumns.Exists(columnName => 0 == string.Compare(columnName, dataColumn.ColumnName, true));

                    // If the data column cannot be found, it will be treated as undefined column
                    if (!isDataColumnFoundInDefinedColumns)
                    {
                        undefinedColumns.Add(dataColumn.ColumnName);
                    }
                }
            }

            return undefinedColumns;
        }

        /// <summary>
        /// Extra the Compile-Time metrics from the current WMS data row
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private DataTable GetCompileTimeMetrics(DataRow dr)
        {
            DataTable table = new DataTable();
            if (null != dr)
            {
                AddColumns(table);

                foreach (WMSQueryDetailInfoUtils.CompileTimeMetrics myType in Enum.GetValues(typeof(WMSQueryDetailInfoUtils.CompileTimeMetrics)))
                {
                    table.Rows.Add(new object[] { 
                            WMSQueryDetailInfoUtils.GetDisplayName(WMSQueryDetailInfoUtils.Metrics.CompileTime, (int)myType), 
                            WMSQueryDetailInfoUtils.ConvertMetrics(WMSQueryDetailInfoUtils.Metrics.CompileTime, (int)myType, dr), 
                            WMSQueryDetailInfoUtils.GetFormat(WMSQueryDetailInfoUtils.Metrics.CompileTime, (int)myType) 
                            });
                }
            }

            return table;
        }

        /// <summary>
        /// Extra the Run-Time metrics from the current WMS data row
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private DataTable GetRunTimeMetrics(DataRow dr)
        {
            DataTable table = new DataTable();
            if (null != dr)
            {
                AddColumns(table);

                foreach (WMSQueryDetailInfoUtils.RunTimeMetrics myType in Enum.GetValues(typeof(WMSQueryDetailInfoUtils.RunTimeMetrics)))
                {
                    if (m_theConnectionDefinition.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        if (myType == WMSQueryDetailInfoUtils.RunTimeMetrics.TOTAL_CHILD_COUNT ||
                            myType == WMSQueryDetailInfoUtils.RunTimeMetrics.ACTIVE_CHILD_COUNT)
                            continue;
                    }
                    table.Rows.Add(new object[] { 
                            WMSQueryDetailInfoUtils.GetDisplayName(WMSQueryDetailInfoUtils.Metrics.RunTime, (int)myType), 
                            WMSQueryDetailInfoUtils.ConvertMetrics(WMSQueryDetailInfoUtils.Metrics.RunTime, (int)myType, dr), 
                            WMSQueryDetailInfoUtils.GetFormat(WMSQueryDetailInfoUtils.Metrics.RunTime, (int)myType)                           
                            });
                }
            }

            return table;
        }

        /// <summary>
        /// Get the current datarow 
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <returns></returns>
        private DataRow getDataRow(DataTable aDataTable)
        {
            if (aDataTable.Rows.Count > 0)
            {
                foreach (DataRow r in aDataTable.Rows)
                {
                    string qid = r[WmsCommand.COL_QUERY_ID].ToString();
                    string start_ts = "";
                    if (aDataTable.Columns.Contains(WmsCommand.COL_QUERY_START_TIME))
                    {
                        start_ts = r[WmsCommand.COL_QUERY_START_TIME].ToString();
                    }
                    if (qid.Equals(m_qid) && start_ts.Equals(m_start_ts))
                    {
                        return r;
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Event handler for Get Repos Info button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetRepositoryInfoButton_Click(object sender, EventArgs e)
        {
            string title = string.Format(Properties.Resources.TitleRepositoryInfo, m_qid);
            WMSRepositoryInfoUserControl reposInfo = new WMSRepositoryInfoUserControl(m_parent, m_theConnectionDefinition, m_qid, m_start_ts, Properties.Resources.QueryStatistics);
            Utilities.LaunchManagedWindow(title, reposInfo, m_theConnectionDefinition, ChildrenWindowSize, true);
        }

        /// <summary>
        /// Event handler for Get SQL Text button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetSQLTextButton_Click(object sender, EventArgs e)
        {
            if (!m_dbDataProvider.FetchInProgress)
            {
                m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText;
                m_dbDataProvider.QueryID = m_qid;
                m_dbDataProvider.START_TS = m_start_ts;
                m_dbDataProvider.Start();
            }
            else
            {
                ReportFetchInProgress();
            }
        }

        /// <summary>
        /// Event handler for Get SQL Plan button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetSQLPlanButton_Click(object sender, EventArgs e)
        {
            string title = string.Format(Properties.Resources.TitleQueryPlan, m_qid);
            WorkloadPlanControl wpc = null;
            int length = (int)m_dataTable.Rows[0]["QUERY_TEXT_LEN"];
            string dataSource = "";
            if (m_dataTable.Columns.Contains("DATASOURCE"))
            {
                dataSource = m_dataTable.Rows[0]["DATASOURCE"] as string;
            }
            if (this.HasFullQueryText)
            {
                wpc = new WorkloadPlanControl(m_theConnectionDefinition, m_qid, m_start_ts, m_sqlTextBox.Text.Trim(), length, true, dataSource,true);
            }
            else
            {
                wpc = new WorkloadPlanControl(m_theConnectionDefinition, m_qid, m_start_ts, _previewText, length, true, dataSource,true);
            }
            Utilities.LaunchManagedWindow(title, wpc, m_theConnectionDefinition, ChildrenWindowSize, true);
        }

        /// <summary>
        /// Event handler for cancel query
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CancelButton_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to cancel the query?", m_title, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_CancelQuery;
                m_dbDataProvider.QueryID = m_qid;
                m_dbDataProvider.START_TS = m_start_ts;

                m_dbDataProvider.Start();

                _cancelButton.Enabled = false;
            }
        }

        /// <summary>
        /// to manage show/hide buttons
        /// </summary>
        private void EnableDisableButtons()
        {
            if (QueryState.Equals("COMPLETED") || QueryState.Equals("REJECTED"))
            {
                _refreshButton.Enabled = false;
                _cancelButton.Enabled = false;
            }
            else
            {
                _refreshButton.Enabled = true;
                _cancelButton.Enabled = true;
            }
            if (QueryState.Equals("EXECUTING"))
            {
                _getPerTableStats.Enabled = true;
            }
            else
            {
                _getPerTableStats.Enabled = false;
            }

            SetWarningLight();
        }

        /// <summary>
        /// Set warning light accordingly
        /// </summary>
        private void SetWarningLight()
        {
            if (string.IsNullOrEmpty(WarningLevel) || WarningLevel.Equals(WMSUtils.WARN_LEVEL_NOWARN))
            {
                _theWarnIndPictureBox.Image = this._theWarnIndImageList.Images[0];
                _getWarningButton.Enabled = false;
            }
            else if (WarningLevel.Equals(WMSUtils.WARN_LEVEL_LOW))
            {
                _theWarnIndPictureBox.Image = this._theWarnIndImageList.Images[1];
                _getWarningButton.Enabled = true;
            }
            else if (WarningLevel.Equals(WMSUtils.WARN_LEVEL_MEDIUM))
            {
                _theWarnIndPictureBox.Image = this._theWarnIndImageList.Images[2];
                _getWarningButton.Enabled = true;
            }
            else if (WarningLevel.Equals(WMSUtils.WARN_LEVEL_HIGH))
            {
                _theWarnIndPictureBox.Image = this._theWarnIndImageList.Images[3];
                _getWarningButton.Enabled = true;
            }
        }

        /// <summary>
        /// Event handler for get warning button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetWarningButton_Click(object sender, EventArgs e)
        {
            string title = string.Format(Properties.Resources.TitleWarningInfo, m_qid);
            WMSQueryWarningUserControl warningInfo = new WMSQueryWarningUserControl(m_parent, m_theConnectionDefinition, m_qid);
            Utilities.LaunchManagedWindow(title, warningInfo, m_theConnectionDefinition, ChildrenWindowSize, true);
        }

        /// <summary>
        /// Event handler for help button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetHelpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.WorkloadDetail);
        }

        /// <summary>
        /// Event handler for Get session stats
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetSessionStatsButton_Click(object sender, EventArgs e)
        {
            string title = string.Format(Properties.Resources.TitleSessionStats, m_qid);
            WMSRepositoryInfoUserControl reposInfo = new WMSRepositoryInfoUserControl(m_parent, m_theConnectionDefinition, m_qid, m_start_ts, Properties.Resources.SessionStatistics);
            Utilities.LaunchManagedWindow(title, reposInfo, m_theConnectionDefinition, ChildrenWindowSize, true);
        }

        /// <summary>
        /// Event handler for Get Table stats
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void GetTableStatsButton_Click(object sender, EventArgs e)
        {
            string title = string.Format(Properties.Resources.TitlePerTableStats, m_qid);
            WMSPerTableStatsControl stats = new WMSPerTableStatsControl(m_parent, m_theConnectionDefinition, m_qid, m_start_ts);
            Utilities.LaunchManagedWindow(title, stats, m_theConnectionDefinition, ChildrenWindowSize, true);
        }

        private void _theWarnIndPictureBox_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "The color of the circle maps to a warn level:\r\n" +
                "\tGreen\t= NOWARN\r\n" +
                "\tYellow\t= LOW\r\n" +
                "\tOrange\t= MEDIUM\r\n" +
                "\tRed\t= HIGH\r\n";
            this._theToolTip.IsBalloon = true;
            this._theToolTip.AutoPopDelay = 30 * 1000;
            this._theToolTip.SetToolTip(_theWarnIndPictureBox, caption);
        }


        /// <summary>
        /// Event handler for peristence persist event
        /// </summary>
        /// <param name="aDictionary"></param>
        /// <param name="aReportDefinitionsOperation"></param>
        private void PersistencePersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aReportDefinitionsOperation)
        {
            switch (aReportDefinitionsOperation)
            {
                case Persistence.PersistenceOperation.Load:
                    {
                        // do nothing at this time.
                        break;
                    }
                case Persistence.PersistenceOperation.Save:
                    {
                        SaveQueryMetricsPersistenceStore(aDictionary);
                        break;
                    }
                default:
                    {
                        break;
                    }
            }
        }

        /// <summary>
        /// To save query metrics configuration to persistence store
        /// </summary>
        /// <param name="aDictionary"></param>
        private void SaveQueryMetricsPersistenceStore(Dictionary<string, object> aDictionary)
        {
            if (m_queryMetricsPersistenceStore == null)
            {
                m_queryMetricsPersistenceStore = new Dictionary<string, object>();
            }

            m_queryMetricsPersistenceStore[WMSQueryDetailInfoUtils.Metrics.TimeBased.ToString()] = m_timeBasedMetrics.SelectedMetrics;
            m_queryMetricsPersistenceStore[WMSQueryDetailInfoUtils.Metrics.Connection.ToString()] =  m_connectionFacts.SelectedMetrics;
            m_queryMetricsPersistenceStore[WMSQueryDetailInfoUtils.Metrics.CompileTime.ToString()] = m_compileTimeMetrics.SelectedMetrics;
            m_queryMetricsPersistenceStore[WMSQueryDetailInfoUtils.Metrics.RunTime.ToString()] = m_runTimeMetrics.SelectedMetrics;
            aDictionary[WMSQueryMetricsPersistenceKey] = m_queryMetricsPersistenceStore;
        }
 
        #endregion Private methods

        #region IMenuProvider implementation
        /// <summary>
        /// Implementing the IMenuProvider interface
        /// </summary>
        /// <returns></returns>
        public Trafodion.Manager.Framework.Controls.TrafodionMenuStrip GetMenuItems(ImmutableMenuStripWrapper aMenuStripWrapper)
        {
            //get the menu items from the canvas
            TrafodionToolStripMenuItem theResetLayoutMenuItem = _widgetCanvas.ResetLayoutMenuItem;
            TrafodionToolStripMenuItem theLockStripMenuItem = _widgetCanvas.LockMenuItem;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator1 = new TrafodionToolStripSeparator();
            System.Windows.Forms.ToolStripSeparator toolStripSeparator2 = new TrafodionToolStripSeparator();

            //Obtain the index of the exit menu because we want to insert the
            //menus just above the exit menu
            int exitIndex = aMenuStripWrapper.getMenuIndex(global::Trafodion.Manager.Properties.Resources.MenuExit);

            //Set the properties of the menu items correctly
            theResetLayoutMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theResetLayoutMenuItem.MergeIndex = exitIndex;
            theLockStripMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theLockStripMenuItem.MergeIndex = exitIndex;
            toolStripSeparator1.MergeAction = System.Windows.Forms.MergeAction.Insert;
            toolStripSeparator1.MergeIndex = exitIndex;
            toolStripSeparator2.MergeAction = System.Windows.Forms.MergeAction.Insert;
            toolStripSeparator2.MergeIndex = exitIndex;


            //Create the same menu structure as we have for main
            ToolStripMenuItem fileMenuItem = new ToolStripMenuItem();
            fileMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator1,
            theResetLayoutMenuItem,
            theLockStripMenuItem
            });

            //Appropriately set the menu name and text for the file menu
            fileMenuItem.MergeAction = System.Windows.Forms.MergeAction.MatchOnly;
            fileMenuItem.Name = global::Trafodion.Manager.Properties.Resources.MenuFile;
            fileMenuItem.Text = global::Trafodion.Manager.Properties.Resources.MenuFile;


            //Create the menu strip
            Trafodion.Manager.Framework.Controls.TrafodionMenuStrip menus = new Trafodion.Manager.Framework.Controls.TrafodionMenuStrip();
            menus.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
                fileMenuItem});

            return menus;
        }

        #endregion

        private void _getHistoryToolStripButton_Click(object sender, EventArgs e)
        {
            if (!string.IsNullOrEmpty(_previewText))
            {
                string title = "Workload History : " + _previewText;
                if (!string.IsNullOrEmpty(title))
                {
                    title = title.Replace("\n", "");
                    title = title.Replace("\r", "");
                }
                ManagedWindow workloadHistory = WindowsManager.GetManagedWindow(TrafodionForm.TitlePrefix + this.m_theConnectionDefinition.Name + " : " + title);
                if (workloadHistory != null)
                {
                    workloadHistory.BringToFront();
                }
                else
                {
                    WorkloadHistoryControl whc = new WorkloadHistoryControl(m_theConnectionDefinition, _previewText);
                    WindowsManager.PutInWindow(new Size(800, 600), whc, title, this.m_theConnectionDefinition);
                }
            }
        }
    }
}
