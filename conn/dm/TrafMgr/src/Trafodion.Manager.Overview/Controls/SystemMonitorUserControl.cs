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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Controls;
using System.Linq;
using System.Drawing;

namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class SystemMonitorUserControl : UserControl, ICloneToWindow
    {   
        public const string TRACE_SUB_AREA_NAME = "System Monitor";

        private ConnectionDefinition _theConnectionDefinition=null;
        private LiveFeedConnection _LiveFeedConnection = null;        

        private SystemSummaryConfigurationUserControl configurationUserControl = null;
        private CachedLiveFeedProtoBufDataProvider _theLiveFeedDataProvider = null;
        private CachedLiveFeedDataProviderConfig _theQpidConfig = null;

        SystemMonitorTabControl _theSystemMonitorTabControl = null;

        public delegate void OnSystemMonitorShowOffenderClick(ShowOffenderEventArgs args);
        public event OnSystemMonitorShowOffenderClick OnSystemMonitorShowOffenderClickImpl;

        public delegate void OnSystemMonitorShowSQLOffenderClick();
        public event OnSystemMonitorShowSQLOffenderClick OnSystemMonitorShowSQLOffenderClickImpl;

        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private delegate void HandleLiveFeedConnectionEvents(Object obj, LiveFeedConnectionEventArgs e);     
        private OffenderWorkloadCanvas _offenderWorkloadCanvas;
        private SQLOffenderCanvas _sqlOffenderWorkloadCanvas;

        private const int LIVE_FEED_STATUS_MONITORING_INTERVAL = 30 * 1000; // 30000 milliseconds
        private const int SYSTEM_METRICS_WAITING_SECOND = 30;
        private const int SYSTEM_METRICS_TSE_WAITING_SECOND = 2 * 60;       // 2 minutes
        private const int HEALTH_LAYER_WAITING_SECOND = 10 * 60;            // 10 minutes
        private const int METRICS_HEALTH_RETRY_THRESHOLD = 15 * 60;           // 15 minutes
        private const string COLUMN_ROUTING_KEY = "RoutingKey";
        private const string COLUMN_METRIC_HEALTH = "MetricHealth";
        private const string COLUMN_REFRESH_TIME_DICTIONARY = "RefreshTimeDictionary";
        private Dictionary<string, DateTime> _systemMetricsLastRefreshTime = null;
        private Dictionary<string, DateTime> _healthLayerLastRefreshTime = null;
        private DataTable _routingKeyMapping = null;
        private Timer _liveFeedStatusMonitorTimer = null;
        private bool _toShowMetricWidget = false;
        private bool _toShowHealthStateWidget = false;
        private Dictionary<string, Object> _theCachedHealthStatesBuffer = new Dictionary<string, object>();

        private List<SystemMetricModel.SystemMetrics> _theOverallSummaryMetrics = null;

        private string _theTitle = null;
        private UniversalWidgetConfig _config1 = null;
        private LiveFeedUniversalWidget _widget1 = null;
        private UniversalWidgetConfig _config2 = null;
        private LiveFeedUniversalWidget _widget2 = null;
        private ToolStripButton _theResetLayoutButton = new ToolStripButton();
        private ToolStripButton _theLockLayoutButton = new ToolStripButton();
        private ToolStripButton _configServerButton = new ToolStripButton();
        

        #region Properties

        /// <summary>
        /// Property: ShowMetricWidget - to display metric widget
        /// </summary>
        public bool ShowMetricWidget
        {
            get { return _toShowMetricWidget; }
        }

        /// <summary>
        /// Property: ShowHealthStateWidget - to display health and state widget
        /// </summary>
        public bool ShowHealthStateWidget
        {
            get { return _toShowHealthStateWidget; }
        }

        /// <summary>
        /// Property: The connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }
              

        /// <summary>
        /// Property: The window title
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

     
        #endregion Properties


        public SystemMonitorUserControl(ConnectionDefinition aConnectionDefinition)
        {
            Initialize(aConnectionDefinition, "System Monitor", null);
            
        }

        /// <summary>
        /// Constructor for cloning
        /// </summary>
        /// <param name="aSystemMonitorUserControl"></param>
        public SystemMonitorUserControl(SystemMonitorUserControl aSystemMonitorUserControl)
        {
            Initialize(null, null, aSystemMonitorUserControl);
        }

        private void Initialize(ConnectionDefinition connectionDefinition, string title, SystemMonitorUserControl cloneSelf)
        {
            if (cloneSelf != null)
            {
                this.OnSystemMonitorShowOffenderClickImpl -= cloneSelf_OnSystemMonitorShowOffenderClickImpl;
                this._theConnectionDefinition = cloneSelf.ConnectionDefn;
                this._theTitle = cloneSelf.WindowTitle;
                this.OnSystemMonitorShowOffenderClickImpl += cloneSelf_OnSystemMonitorShowOffenderClickImpl;
            }
            else
            {
                this._theConnectionDefinition = connectionDefinition;
                this._theTitle = title;                
            }

            InitializeComponent();
            InitializeLiveFeedStatusMonitorTimer();
            ShowWidgets();
        }

        void cloneSelf_OnSystemMonitorShowOffenderClickImpl(ShowOffenderEventArgs args)
        {
            _offenderWorkloadCanvas = new OffenderWorkloadCanvas(ConnectionDefn);
            _offenderWorkloadCanvas.HandleMouseClickCPU(args);
            WindowsManager.PutInWindow(this.Size, _offenderWorkloadCanvas, _offenderWorkloadCanvas.WindowTitle, ConnectionDefn);
        }

        void cloneSelf_OnSystemMonitorShowSQLOffenderClickImpl()
        {
            _sqlOffenderWorkloadCanvas = new SQLOffenderCanvas(ConnectionDefn);
            WindowsManager.PutInWindow(this.Size, _sqlOffenderWorkloadCanvas, _sqlOffenderWorkloadCanvas.WindowTitle, ConnectionDefn);
        }

        private void InitializeLiveFeedStatusMonitorTimer()
        {
            InitializeLastRefreshTime();
            InitializeRoutingKeyMapping();
            this._liveFeedStatusMonitorTimer = new Timer();
            this._liveFeedStatusMonitorTimer.Interval = LIVE_FEED_STATUS_MONITORING_INTERVAL;
            this._liveFeedStatusMonitorTimer.Tick += ((e, sender) =>
            {
                CheckAndUpdateLiveFeedControl();
                RetryLiveFeedConnection();
            });
            this._liveFeedStatusMonitorTimer.Start();
        }

        private void InitializeRoutingKeyMapping()
        {
            this._routingKeyMapping = new DataTable();
            this._routingKeyMapping.Columns.Add(COLUMN_ROUTING_KEY, typeof(string));
            this._routingKeyMapping.Columns.Add(COLUMN_METRIC_HEALTH, typeof(string));
            this._routingKeyMapping.Columns.Add(COLUMN_REFRESH_TIME_DICTIONARY, typeof(Dictionary<string, DateTime>));

            // Mapping for SystemMetrics
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Core.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Disk.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Tse.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Memory.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Swap.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Load_Avg.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Virtual_Memory.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Network_Rcv.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.Network_Txn.ToString(),
                this._systemMetricsLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm.ToString(),
                SystemMetricModel.SystemMetrics.File_System.ToString(),
                this._systemMetricsLastRefreshTime);

            // Mapping for HealthLayer
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer.ToString(),
                SystemMetricModel.HealthLayer.Access.ToString(),
                this._healthLayerLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer.ToString(),
                SystemMetricModel.HealthLayer.Database.ToString(),
                this._healthLayerLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer.ToString(),
                SystemMetricModel.HealthLayer.Foundation.ToString(),
                this._healthLayerLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer.ToString(),
                SystemMetricModel.HealthLayer.OS.ToString(),
                this._healthLayerLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer.ToString(),
                SystemMetricModel.HealthLayer.Server.ToString(),
                this._healthLayerLastRefreshTime);
            _routingKeyMapping.Rows.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer.ToString(),
                SystemMetricModel.HealthLayer.Storage.ToString(),
                this._healthLayerLastRefreshTime);
        }


        private void InitializeLastRefreshTime()
        {
            this._systemMetricsLastRefreshTime = new Dictionary<string, DateTime>();
            this._healthLayerLastRefreshTime = new Dictionary<string, DateTime>();
            foreach (string systemMetric in Enum.GetNames(typeof(SystemMetricModel.SystemMetrics)))
            {
                this._systemMetricsLastRefreshTime.Add(systemMetric, DateTime.MinValue);
            }

            foreach (string healthLayer in Enum.GetNames(typeof(SystemMetricModel.HealthLayer)))
            {
                this._healthLayerLastRefreshTime.Add(healthLayer, DateTime.MinValue);
            }
        }

        private void RecordLastRefreshTime(string routingKey)
        {
            DateTime now = DateTime.Now;
            foreach (DataRow drMapping in _routingKeyMapping.Rows)
            {
                if ((string)drMapping[COLUMN_ROUTING_KEY] == routingKey)
                {
                    Dictionary<string, DateTime> dicLastRefreshTime = (Dictionary<string, DateTime>)drMapping[COLUMN_REFRESH_TIME_DICTIONARY];
                    string keyMetricHealthy = (string)drMapping[COLUMN_METRIC_HEALTH];
                    dicLastRefreshTime[keyMetricHealthy] = now;
                }
            }
        }

        private void CheckAndUpdateLiveFeedControl()
        {
            if (this._LiveFeedConnection != null && this._LiveFeedConnection.CurrentState != LiveFeedConnection.LiveFeedConnectionState.Started) return;

            // Clear System Metrics
            foreach (string systemMetric in Enum.GetNames(typeof(SystemMetricModel.SystemMetrics)))
            {
                DateTime lastRefreshTime = this._systemMetricsLastRefreshTime[systemMetric];

                if (DateTime.MinValue != lastRefreshTime)
                {
                    double elapsedSecond = (DateTime.Now - lastRefreshTime).TotalSeconds;
                    bool isTimeOut = elapsedSecond > SYSTEM_METRICS_WAITING_SECOND;
                    if (systemMetric == SystemMetricModel.SystemMetrics.Tse.ToString())
                    {
                        isTimeOut = elapsedSecond > SYSTEM_METRICS_TSE_WAITING_SECOND;
                        _theSystemMonitorTabControl.OverallSummary.IsTseConnectionTimeOut = isTimeOut;
                    }

                    if (isTimeOut)
                    {
                        _theSystemMonitorTabControl.OverallSummary.ClearChartSeries(systemMetric);
                    }
                }
            }

            // Reset Health/State
            foreach (string healthLayer in Enum.GetNames(typeof(SystemMetricModel.HealthLayer)))
            {
                DateTime lastRefreshTime = this._healthLayerLastRefreshTime[healthLayer];

                if (DateTime.MinValue != lastRefreshTime)
                {
                    double elapsedSecond = (DateTime.Now - lastRefreshTime).TotalSeconds;
                    if (elapsedSecond > HEALTH_LAYER_WAITING_SECOND)
                    {
                        _theSystemMonitorTabControl.OverallSummary.ClearHealthState(healthLayer);
                    }
                }
            }
        }

        private void RetryLiveFeedConnection()
        {
            if (_theLiveFeedDataProvider != null && _theLiveFeedDataProvider.LiveFeedConnection != null
                && _theLiveFeedDataProvider.LiveFeedConnection.CurrentState != LiveFeedConnection.LiveFeedConnectionState.Started)
                return;

            bool metricTimeout = false;
            bool healthTimeout = false;
            var metricLastArrival = (from a in _systemMetricsLastRefreshTime select a.Value).Max();
            var healthLastArrival = (from a in _healthLayerLastRefreshTime select a.Value).Max();

            if (metricLastArrival != DateTime.MinValue && ShowMetricWidget)
            {
                if ((System.DateTime.Now - metricLastArrival).TotalSeconds > METRICS_HEALTH_RETRY_THRESHOLD)
                    metricTimeout = true;
                else
                    metricTimeout = false;
            }

            if (healthLastArrival != DateTime.MinValue && ShowHealthStateWidget)
            {
                if ((System.DateTime.Now - healthLastArrival).TotalSeconds > METRICS_HEALTH_RETRY_THRESHOLD)
                    healthTimeout = true;
                else
                    healthTimeout = false;
            }

            bool retry = false;
            retry = (metricTimeout && healthTimeout);
            if (retry)
            {
                _theLiveFeedDataProvider.Stop();
                _theLiveFeedDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theLiveFeedDataProvider.Start();
                _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            }

        }

        /// <summary>
        /// Create all of the widgets
        /// </summary>
        private void ShowWidgets()
        {
            GridLayoutManager gridLayoutManager = new GridLayoutManager(1, 1);
            gridLayoutManager.CellSpacing = 4;
            this._theCanvas.LayoutManager = gridLayoutManager;

            try
            {
                // Create the first widget for monitoring the connector
                _config1 = new UniversalWidgetConfig();
                _theQpidConfig = new CachedLiveFeedDataProviderConfig();
                _theQpidConfig.TheDataFormat = LiveFeedDataProviderConfig.LiveFeedDataFormat.ProtoBuf;
                _theQpidConfig.ConnectionDefinition = this.ConnectionDefn;


                _theQpidConfig.Configure(GetSubscriptions());
                _config1.DataProviderConfig = _theQpidConfig;
                _config1.Name = "SystemMonitorPersistence";
                _config1.Title = "";
                //_config1.ShowRefreshButton = false;
                //_config1.ShowProviderToolBarButton = false;
                _config1.ShowTimerSetupButton = false;
                _config1.ShowExportButtons = false;
                _config1.ShowRefreshTimerButton = false;
                _config1.ShowHelpButton = true;
                _config1.HelpTopic = HelpTopics.SystemMonitor;
                _widget1 = new LiveFeedUniversalWidget();
                _widget1.UniversalWidgetConfiguration = _config1;
                _theLiveFeedDataProvider = (CachedLiveFeedProtoBufDataProvider)_widget1.DataProvider;
                _LiveFeedConnection = _theLiveFeedDataProvider.LiveFeedConnection;
                _LiveFeedConnection.BrokerConfiguration.OnBrokerChanged += new LiveFeedBrokerConfiguration.LiveFeedBrokerChanged(BrokerConfiguration_OnBrokerChanged);

                _theOverallSummaryMetrics = new List<SystemMetricModel.SystemMetrics>();

                foreach (DataRow dr in SystemMetricChartConfigModel.Instance.GetSystemMetricColorTable(this.ConnectionDefn.Name).Rows)
                {
                    bool displayStatus = (bool)dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColDisplayStatus.ToString()];
                    SystemMetricModel.SystemMetrics metric = (SystemMetricModel.SystemMetrics)dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetric.ToString()];
                    if (displayStatus)
                        _theOverallSummaryMetrics.Add(metric);
                }

                _theSystemMonitorTabControl = new SystemMonitorTabControl(_theConnectionDefinition, _theOverallSummaryMetrics, ShowHealthStateWidget);
                _theSystemMonitorTabControl.DataProvider = _widget1.DataProvider;
                _widget1.DataDisplayControl = _theSystemMonitorTabControl;
                _theSystemMonitorTabControl.OnShowOffenderClickImpl += _theSystemMonitorTabControl_OnShowOffenderClickImpl;
                _theSystemMonitorTabControl.OnShowSQLOffenderClickImpl += _theSystemMonitorTabControl_OnShowSQLOffenderClickImpl;

                //Add the widget to the canvas
                GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
                WidgetContainer container1 = _theCanvas.AddWidget(_widget1, _config1.Name, _config1.Title, gridConstraint, -1);
                container1.Padding = new System.Windows.Forms.Padding(1, 0, 1, 1);
                container1.Moveable = container1.Resizable = container1.AllowMaximize = container1.AllowDelete = container1.AllowDrop = false;

                AddToolStripButtons();

                _theLiveFeedDataProvider.Start();
                _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

            }
            catch (Exception ex)
            {
                _theCanvas.Controls.Clear();

                TrafodionTextBox errorTextBox = new TrafodionTextBox();
                errorTextBox.WordWrap = errorTextBox.ReadOnly = errorTextBox.Multiline = true;
                errorTextBox.Text = "Error initializing System Monitor : " + Environment.NewLine + Environment.NewLine + ex.Message  + Environment.NewLine + ex.StackTrace;
                errorTextBox.Dock = DockStyle.Fill;

                _theCanvas.Controls.Add(errorTextBox);
            }
        }

        void _theSystemMonitorTabControl_OnShowSQLOffenderClickImpl()
        {
            if (OnSystemMonitorShowSQLOffenderClickImpl != null)
            {
                OnSystemMonitorShowSQLOffenderClickImpl();
            }
        }

        private void _theSystemMonitorTabControl_OnShowOffenderClickImpl(ShowOffenderEventArgs args)
        {
            if (OnSystemMonitorShowOffenderClickImpl != null)
            {
                OnSystemMonitorShowOffenderClickImpl(args);
            }
        }

        /// <summary>
        /// Add additional strip buttons
        /// </summary>
        private void AddToolStripButtons()
        {
            _theResetLayoutButton.Text = "Reset Layout";
            _theResetLayoutButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _theResetLayoutButton.Image = Trafodion.Manager.Properties.Resources.UndoIcon;
            _theResetLayoutButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _theResetLayoutButton.Name = "_theResetLayoutButton";
            _theResetLayoutButton.Click += new EventHandler(_theResetLayoutButton_Click);
            _widget1.AddToolStripItem(_theResetLayoutButton);

            _theLockLayoutButton.Text = "Lock Layout";
            _theLockLayoutButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _theLockLayoutButton.Image = Trafodion.Manager.Properties.Resources.Locked;
            _theLockLayoutButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _theLockLayoutButton.Name = "_theLockLayoutButton";
            _theLockLayoutButton.Click += new EventHandler(_theLockLayoutButton_Click);
            _widget1.AddToolStripItem(_theLockLayoutButton);

            //Add a separator
            ToolStripSeparator toolStripSeparator1 = new ToolStripSeparator();
            toolStripSeparator1.Name = "_theResetLayoutButtonSeparator";
            toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            toolStripSeparator1.Visible = true;
            _widget1.AddToolStripItem(toolStripSeparator1);

            _configServerButton.Text = "Configure System Monitor";
            _configServerButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            _configServerButton.Image = Trafodion.Manager.Properties.Resources.AlterIcon;
            _configServerButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _configServerButton.Name = "_configureLiveFeedButton";
            _configServerButton.Click += new EventHandler(_configServerButton_click);
            _widget1.AddToolStripItem(_configServerButton);

            //Add a separator
            ToolStripSeparator toolStripSeparator2 = new ToolStripSeparator();
            toolStripSeparator2.Name = "_configureLiveFeedSeparator";
            toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
            toolStripSeparator2.Visible = true;
            _widget1.AddToolStripItem(toolStripSeparator2);
        }

        private void _configServerButton_click(object sender, EventArgs e)
        {
            configurationUserControl = new SystemSummaryConfigurationUserControl(_theLiveFeedDataProvider.LiveFeedConnection);
            configurationUserControl.ConfigurationChanged += new SystemSummaryConfigurationUserControl.ChangingHandler(ConfigurationChangeHandler);
            configurationUserControl.ShowDialog();
        }

        private void ConfigurationChangeHandler(object sender, EventArgs e)
        {
            int intTabIndex = _theSystemMonitorTabControl.TheMonitoringTabCtl.SelectedIndex;
            //Dispose and Create Again
            _theOverallSummaryMetrics.Clear();
            _theLiveFeedDataProvider.Stop();
            _theLiveFeedDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            _theQpidConfig.ReConfigure(GetSubscriptions());

            DataTable chartColorDataModelTable = SystemMetricChartConfigModel.Instance.TheSystemMetricsColorTables[_theConnectionDefinition.Name];
            foreach (DataRow dr in chartColorDataModelTable.Rows)
            {
                bool displayStatus = (bool)dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColDisplayStatus.ToString()];
                if (displayStatus)
                {
                    int mValue = (int)dr[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetric.ToString()];
                    SystemMetricModel.SystemMetrics metric = (SystemMetricModel.SystemMetrics)Enum.ToObject(typeof(SystemMetricModel.SystemMetrics), mValue);
                    _theOverallSummaryMetrics.Add(metric);
                }
            }

            _theSystemMonitorTabControl.OnShowOffenderClickImpl -= _theSystemMonitorTabControl_OnShowOffenderClickImpl;
            _theSystemMonitorTabControl.OnShowSQLOffenderClickImpl -= _theSystemMonitorTabControl_OnShowSQLOffenderClickImpl;
            _theSystemMonitorTabControl.Dispose();

            _theSystemMonitorTabControl = new SystemMonitorTabControl(_theConnectionDefinition, _theOverallSummaryMetrics, ShowHealthStateWidget);
            _theSystemMonitorTabControl.DataProvider = _widget1.DataProvider;
            _widget1.DataDisplayControl = _theSystemMonitorTabControl;
            _theSystemMonitorTabControl.OnShowOffenderClickImpl += _theSystemMonitorTabControl_OnShowOffenderClickImpl;
            _theSystemMonitorTabControl.OnShowSQLOffenderClickImpl += _theSystemMonitorTabControl_OnShowSQLOffenderClickImpl;

            _theSystemMonitorTabControl.DynamicSummaryControl.ReConfigDynamicMetricControl(_theOverallSummaryMetrics);

            _theSystemMonitorTabControl.TheMonitoringTabCtl.SelectedIndex = intTabIndex;
            _theLiveFeedDataProvider.Start();
            _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

            //Now, try to restore any of the cached health & states.
            foreach (string key in _theCachedHealthStatesBuffer.Keys)
            {
                object stats = _theCachedHealthStatesBuffer[key];
                _theSystemMonitorTabControl.OverallSummary.UpdateHealthStates(key, stats, false);
            }
        }

        void _overallSummary_OnShowOffenderClickImpl(ShowOffenderEventArgs args)
        {
            if (OnSystemMonitorShowOffenderClickImpl != null)
            {
                OnSystemMonitorShowOffenderClickImpl(args);
            }
        }
        private void _theResetLayoutButton_Click(object sender, EventArgs e)
        {
            _theSystemMonitorTabControl.ResetLayout();
            _theLockLayoutButton.Text = "Lock Layout";
            _theLockLayoutButton.Image = Trafodion.Manager.Properties.Resources.Locked;
        }

        private void _theLockLayoutButton_Click(object sender, EventArgs e)
        {
            string buttonText = _theSystemMonitorTabControl.LockLayout();
            if (buttonText.Equals(Trafodion.Manager.Properties.Resources.MenuLock))
            {
                _theLockLayoutButton.Text = "Lock Layout";
                _theLockLayoutButton.Image = Trafodion.Manager.Properties.Resources.Locked;
            }
            else
            {
                _theLockLayoutButton.Text = "Unlock Layout";
                _theLockLayoutButton.Image = Trafodion.Manager.Properties.Resources.Unlocked;
            }
        }
        /// <summary>
        /// Get all necessary subscriptions
        /// </summary>
        /// <returns></returns>
        private string[] GetSubscriptions()
        {
            bool memory = false;
            bool network = false;
            _toShowMetricWidget = false;
            _toShowHealthStateWidget = false;

            List<string> publicationNames = new List<string>();
            foreach (SystemMetricModel.SystemMetrics metric in Enum.GetValues(typeof(SystemMetricModel.SystemMetrics)))
            {
                if (SystemMetricChartConfigModel.Instance.GetSystemMetricDisplayStatus(ConnectionDefn.Name, metric))
                {
                    _toShowMetricWidget = true;
                    switch (metric)
                    {
                        case SystemMetricModel.SystemMetrics.Core:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm);
                            break;

                        case SystemMetricModel.SystemMetrics.Disk:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm);
                            break;

                        case SystemMetricModel.SystemMetrics.Tse:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm);
                            break;

                        case SystemMetricModel.SystemMetrics.File_System:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm);
                            break;

                        case SystemMetricModel.SystemMetrics.Load_Avg:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm);
                            break;

                        case SystemMetricModel.SystemMetrics.Memory:
                        case SystemMetricModel.SystemMetrics.Swap:
                            if (!memory)
                            {
                                publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm);
                                memory = true;
                            }
                            break;

                        case SystemMetricModel.SystemMetrics.Network_Rcv:
                        case SystemMetricModel.SystemMetrics.Network_Txn:
                            if (!network)
                            {
                                publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm);
                                network = true;
                            }
                            break;

                        case SystemMetricModel.SystemMetrics.Virtual_Memory:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm);
                            break;
                    }
                }
            }

            foreach (SystemMetricModel.HealthLayer h in Enum.GetValues(typeof(SystemMetricModel.HealthLayer)))
            {
                if (SystemMetricChartConfigModel.Instance.GetHealthStatesLayerDisplayStatus(ConnectionDefn.Name, h))
                {
                    _toShowHealthStateWidget = true;
                    switch (h)
                    {
                        case SystemMetricModel.HealthLayer.Access:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer);
                            break;

                        case SystemMetricModel.HealthLayer.Database:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer);
                            break;

                        case SystemMetricModel.HealthLayer.Foundation:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer);
                            break;

                        case SystemMetricModel.HealthLayer.OS:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer);
                            break;

                        case SystemMetricModel.HealthLayer.Server:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer);
                            break;

                        case SystemMetricModel.HealthLayer.Storage:
                            publicationNames.Add(LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer);
                            break;
                    }
                }
            }

            return publicationNames.ToArray();
        }

        void BrokerConfiguration_OnBrokerChanged(object sender, LiveFeedBrokerChangedEventArgs eArgs)
        {
            if (eArgs.ChangedReason == LiveFeedBrokerChangedEventArgs.Reason.SessionRetryTimer)
            {
                _theLiveFeedDataProvider.ConfiguredRefreshRate = _theLiveFeedDataProvider.LiveFeedConnection.BrokerConfiguration.SessionRetryTimer;
            }
            else
            {
                _theLiveFeedDataProvider.Stop();
                _theLiveFeedDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theLiveFeedDataProvider.Start();
                _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
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
                    Invoke(new HandleEvents(DataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
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
        /// The new data arrival handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {

            Dictionary<string, List<object>> dataStats = _theLiveFeedDataProvider.GetStats();
            DataTable table = null;

            lock (dataStats)
            {

                foreach (string key in dataStats.Keys)
                {
                    DateTime currentTS;
                    List<object> stats = dataStats[key];
                    if (stats.Count > 0)
                    {
                        try
                        {
                            RecordLastRefreshTime(key);
                            switch (key)
                            {
                                case LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.UpdateCoreBusy(table);                                            
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Core, table);                                            
                                        }

                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.UpdateDiskIOs(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Disk, table);

                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS, _theSystemMonitorTabControl.OverallSummary.DiskIoNodeCount);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.RealtimeTseDataTable = table.Copy();
                                            _theSystemMonitorTabControl.OverallSummary.UpdateTseSkew(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Tse, table);
                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.UpdateMemory(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Memory, table);
                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.UpdateLoadAvg(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Load_Avg, table);
                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.Updatevirtualmem(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Virtual_Memory, table);
                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.UpdateNetwork(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.Network_Rcv, table);
                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm:
                                    {
                                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], -1, out currentTS);
                                        if (table != null && table.Rows.Count > 0)
                                        {
                                            _theSystemMonitorTabControl.OverallSummary.UpdateFileSystem(table);
                                            _theSystemMonitorTabControl.DynamicSummaryControl.StoreHistoricalDataTable(SystemMetricModel.SystemMetrics.File_System, table);
                                        }
                                    }
                                    break;

                                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                                    _theSystemMonitorTabControl.OverallSummary.UpdateHealthStates(key, stats[stats.Count - 1], true);

                                    //Save it to the cache for future use.
                                    if (_theCachedHealthStatesBuffer.ContainsKey(key))
                                    {
                                        _theCachedHealthStatesBuffer[key] = stats[stats.Count - 1];
                                    }
                                    else
                                    {
                                        _theCachedHealthStatesBuffer.Add(key, stats[stats.Count - 1]);
                                    }

                                    break;
                            }
                        }
                        catch (Exception ex)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.Overview, TRACE_SUB_AREA_NAME,
                                               String.Format("Transformation encountered exception - {0}",
                                                            ex.Message));
                        }
                    }
                }
            }

            dataStats.Clear();
        }

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_theLiveFeedDataProvider != null)
                {
                    _theLiveFeedDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                }
                if (_LiveFeedConnection != null && _LiveFeedConnection.BrokerConfiguration != null)
                {
                    _LiveFeedConnection.BrokerConfiguration.OnBrokerChanged -= BrokerConfiguration_OnBrokerChanged;
                }
                if (_theSystemMonitorTabControl != null)
                {
                    _theSystemMonitorTabControl.OnShowOffenderClickImpl -= _theSystemMonitorTabControl_OnShowOffenderClickImpl;
                    _theSystemMonitorTabControl.OnShowSQLOffenderClickImpl -= _theSystemMonitorTabControl_OnShowSQLOffenderClickImpl;
                }
                Shutdown();
            }
        }

        /// <summary>
        /// To clean everything up in this Canvas.  
        /// </summary>
        private void Shutdown()
        {
            if (_widget1 != null && _widget1.DataProvider != null)
            {
                _widget1.DataProvider.Stop();
            }
        }

        #region Public methods

        /// <summary>
        /// The interface method for ICloneWindow.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            SystemMonitorUserControl theSystemMonitorUserControl = new SystemMonitorUserControl(this);
            return theSystemMonitorUserControl;
        }



        #endregion Public methods

 
    }
}
