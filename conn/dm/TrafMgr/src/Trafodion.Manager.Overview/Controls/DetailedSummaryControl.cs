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
using System.Data;
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.LiveFeedFramework.Controls;
using System.Linq;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// User control to display the detailed a system metric summary
    /// </summary>
    public partial class DetailedSummaryControl : UserControl, IDataDisplayControl
    {
        #region Fields

        private DataTable _dataTable = new DataTable();   
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);

        Timer _theTimer = new Timer();

        int maxNodes = 32;
        int CoresPerNode = 8;
        private Random random = new Random();
        private System.Drawing.Size _theCurrentSize;
        private ConnectionDefinition _theConnectionDefinition = null;
        private LiveFeedConnection _LiveFeedConnection = null;

        private UniversalWidgetConfig _theWidgetConfig = null;
        private LiveFeedUniversalWidget _theWidget = null;
        private CachedLiveFeedProtoBufDataProvider _theLiveFeedDataProvider = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        //private SystemMetricChartConfig _theOverallSummaryChartConfig = null;
        private List<SystemMetricModel.SystemMetrics> _theOverallSummaryMetrics = null;
        private SystemMetricModel.SystemMetricDisplays _theSystemMetricDisplay;
        private OverallSummaryControl _myParent = null;
        private SystemMetricDetailsChartControl _theMetricDetailsChartControl = null;
        private int _theNodeID = -1;
        private ToolStripLabel _theTimestampLabel = new ToolStripLabel();
        private int _diskIoNodeCount = 0;
        private Timer _liveFeedStatusMonitorTimer = null;
        private const int LIVE_FEED_STATUS_MONITORING_INTERVAL = 30 * 1000; // 30000 milliseconds
        private const int SYSTEM_METRICS_WAITING_SECOND = 30;
        private const int SYSTEM_METRICS_TSE_WAITING_SECOND = 3 * 60;       // 3 minutes
        private const int METRICS_HEALTH_RETRY_THRESHOLD = 15 * 60;           // 15 minutes
        private Dictionary<string, DateTime> _systemMetricsLastRefreshTime = null;
        private DataTable _routingKeyMapping = null;
        private const string COLUMN_ROUTING_KEY = "RoutingKey";
        private const string COLUMN_METRIC_HEALTH = "MetricHealth";
        private const string COLUMN_REFRESH_TIME_DICTIONARY = "RefreshTimeDictionary";
        private SystemMetricModel.SystemMetrics _theSystemMetric;
        #endregion 

        #region Properties
        /// <summary>
        /// Property: ConnectonDefn - the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }

        public DataTable ChartCounterDataTable 
        {
            private set { _dataTable = value; }
            get {return _dataTable;}
        }

        /// <summary>
        /// Property: DataProvider - the data provider used by this widget
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _theLiveFeedDataProvider; }
            set { }
        }

        /// <summary>
        /// Proerpty: UniversalWidgetConfiguration - the configuration of the widget
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theWidgetConfig; }
            set { _theWidgetConfig = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - the data display handler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - not used at this time
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        #endregion Properties

        #region Constructor

        public DetailedSummaryControl(ConnectionDefinition aConnectionDefinition, SystemMetricModel.SystemMetricDisplays aSystemMetricDisplay, OverallSummaryControl aParent, int aNodeID, int diskIoNodeCount)
        {
            _theNodeID = aNodeID;
            this._diskIoNodeCount = diskIoNodeCount;
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            _theSystemMetricDisplay = aSystemMetricDisplay;

            InitializeLiveFeedStatusMonitorTimer();
            ShowWidgets();

            _myParent = aParent;
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Persistent configruation - this is not used. But, for need to be here to fulfill the requirement of the interface
        /// </summary>
        public void PersistConfiguration()
        {
        }

        #endregion Public methods

        #region Private methods
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

        
        }

        private void InitializeLastRefreshTime()
        {
            this._systemMetricsLastRefreshTime = new Dictionary<string, DateTime>();
            
            foreach (string systemMetric in Enum.GetNames(typeof(SystemMetricModel.SystemMetrics)))
            {
                this._systemMetricsLastRefreshTime.Add(systemMetric, DateTime.MinValue);
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

                    int waitingSecond = SYSTEM_METRICS_WAITING_SECOND;
                    if (systemMetric == SystemMetricModel.SystemMetrics.Tse.ToString())
                    {
                        waitingSecond = SYSTEM_METRICS_TSE_WAITING_SECOND;
                    }

                    if (elapsedSecond > waitingSecond)
                    {
                        _theMetricDetailsChartControl.ClearChartSeries();
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
            var metricLastArrival = (from a in _systemMetricsLastRefreshTime select a.Value).Max();
       

            if (metricLastArrival != DateTime.MinValue)
            {
                if ((System.DateTime.Now - metricLastArrival).TotalSeconds > METRICS_HEALTH_RETRY_THRESHOLD)
                    metricTimeout = true;
                else
                    metricTimeout = false;
            }

            if (metricTimeout)
            {
                _theLiveFeedDataProvider.Stop();
                _theLiveFeedDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _theLiveFeedDataProvider.Start();
                _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            }

        }

        //void _myParent_OnNewDataArrival(object sender, NewDataEventArgs e)
        //{
        //    if (_theMetricDetailsChartControl != null)
        //    {
        //        _theMetricDetailsChartControl.UpdateChart(e.DataTable);
        //    }
        //}

        private void ShowWidgets()
        {
        
            GridLayoutManager gridLayoutManager = new GridLayoutManager(1, 1);
            gridLayoutManager.CellSpacing = 4;
            _theCanvas.LayoutManager = gridLayoutManager;

            _theMetricDetailsChartControl = new SystemMetricDetailsChartControl(ConnectionDefn, _theSystemMetricDisplay);

            // Create the first widget for monitoring the connector
            _theWidgetConfig = new UniversalWidgetConfig();
            CachedLiveFeedDataProviderConfig qpidConfig = new CachedLiveFeedDataProviderConfig();
            qpidConfig.TheDataFormat = LiveFeedDataProviderConfig.LiveFeedDataFormat.ProtoBuf;
            qpidConfig.ConnectionDefinition = this.ConnectionDefn;

            string publication = "";
            switch (_theSystemMetricDisplay)
            {
                case SystemMetricModel.SystemMetricDisplays.CoreMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_core_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.DiskMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_disk_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.TseMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_tse_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.FileSystemMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_filesys_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.LoadAvgMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_loadavg_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.MemoryMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_memory_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.NetworkMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_network_metrics_asm;
                    break;

                case SystemMetricModel.SystemMetricDisplays.VMMetricDetails:
                    publication = LiveFeedRoutingKeyMapper.PUBS_performance_virtualmem_metrics_asm;
                    break;

                default:
                    break;
            }


            //qpidConfig.Configure(publication, new string[] { string.Format("performance_stat.linuxcounters.instance.public.gpb.core_metrics_shortbusy_assembled.header.info_node_id  = {0}", _theNodeID) });

            qpidConfig.Configure(publication);
            _theWidgetConfig.DataProviderConfig = qpidConfig;
            _theWidgetConfig.Name = "";
            _theWidgetConfig.Title = "";
            //_theWidgetConfig.ShowRefreshButton = false;
            //_theWidgetConfig.ShowProviderToolBarButton = false;
            _theWidgetConfig.ShowTimerSetupButton = false;
            _theWidgetConfig.ShowExportButtons = false;
            _theWidgetConfig.ShowRefreshTimerButton = false;
            _theWidgetConfig.ShowHelpButton = true;
            
            switch (_theSystemMetricDisplay)
            {
                case SystemMetricModel.SystemMetricDisplays.CoreMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedCoreMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Core;
                    break;

                case SystemMetricModel.SystemMetricDisplays.DiskMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedDiskMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Disk;
                    break;

                case SystemMetricModel.SystemMetricDisplays.TseMetricDetails:
                    string helpTopic = string.Empty;
                    switch((TseMetric) Enum.Parse(typeof(TseMetric), SystemMetricModel.SelectedTseMetric ))
                    {
                        case TseMetric.requests:
                            helpTopic = "TSE-Request-Count-Skew.html";
                            break;

                        case TseMetric.service_time:
                            helpTopic = "TSE-Service-Time-Skew.html";
                            break;

                        case TseMetric.ase_service_time:
                            helpTopic = "ASE-Service-Time-Skew.html";
                            break;

                        case TseMetric.request_io_wait_time:
                            helpTopic = "TSE-Request-IO-Wait-Time-Skew.html";
                            break;

                        case TseMetric.ready_list_count:
                            helpTopic = "TSE-Ready-List-Count-Skew.html";
                            break;
                    }
                    _theWidgetConfig.HelpTopic = helpTopic;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Tse;
                    break;

                case SystemMetricModel.SystemMetricDisplays.FileSystemMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedFileSystemMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.File_System;
                    break;

                case SystemMetricModel.SystemMetricDisplays.LoadAvgMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedLoadAverageMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Load_Avg;
                    break;

                case SystemMetricModel.SystemMetricDisplays.MemoryMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedMemoryAndSwapMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Memory;
                    break;

                case SystemMetricModel.SystemMetricDisplays.NetworkMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedNetworkMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Network_Rcv;
                    break;

                case SystemMetricModel.SystemMetricDisplays.VMMetricDetails:
                    _theWidgetConfig.HelpTopic = HelpTopics.DetailedVirtualMemoryMetrics;
                    _theSystemMetric = SystemMetricModel.SystemMetrics.Virtual_Memory;
                    break;

                default:
                    break;
            }
            _theWidget = new LiveFeedUniversalWidget();
            _theWidget.UniversalWidgetConfiguration = _theWidgetConfig;
            _theLiveFeedDataProvider = (CachedLiveFeedProtoBufDataProvider)_theWidget.DataProvider;
            _LiveFeedConnection = _theLiveFeedDataProvider.LiveFeedConnection;

            _theMetricDetailsChartControl = new SystemMetricDetailsChartControl(ConnectionDefn, _theSystemMetricDisplay);
            _theWidget.DataDisplayControl = _theMetricDetailsChartControl;
            _theMetricDetailsChartControl.DataProvider = _theWidget.DataProvider;

            //Add the widget to the canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer container1 = _theCanvas.AddWidget(_theWidget, _theWidgetConfig.Name, _theWidgetConfig.Title, gridConstraint, -1);
            container1.Padding = new System.Windows.Forms.Padding(1, 0, 1, 1);
            container1.Moveable = container1.Resizable = container1.AllowMaximize = container1.AllowDelete = container1.AllowDrop = false;

            AddToolStripButtons();

            _theLiveFeedDataProvider.Start();
            _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

        }

        private void AddToolStripButtons()
        {
            _theTimestampLabel.Text = "Waiting for server data...";
            _theTimestampLabel.Name = "_theTimestampLabel";
            _theWidget.AddToolStripItem(_theTimestampLabel);

            //Add a separator
            ToolStripSeparator toolStripSeparator1 = new ToolStripSeparator();
            toolStripSeparator1.Name = "_theResetLayoutButtonSeparator";
            toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            toolStripSeparator1.Visible = true;
            _theWidget.AddToolStripItem(toolStripSeparator1);
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
            DateTime currentTS; 

            lock (dataStats)
            {

                foreach (string key in dataStats.Keys)
                {
                    List<object> stats = dataStats[key];
                    if (stats.Count > 0)
                    {
                        RecordLastRefreshTime(key);

                        table = SystemMetricModel.TransformSystemMetricProtoBuf(this.ConnectionDefn, key, stats[stats.Count - 1], _theNodeID, out currentTS, this._diskIoNodeCount);
                        _theMetricDetailsChartControl.UpdateChart(table,_theSystemMetric);
                        _theTimestampLabel.Text = string.Format("Last Reported Time: {0}", Utilities.StandardizeDateTime(currentTS));
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
                if (DataProvider != null)
                {
                    DataProvider.Stop();
                }
                _theLiveFeedDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            }
        }
       
        #endregion Private methods
    }
}
