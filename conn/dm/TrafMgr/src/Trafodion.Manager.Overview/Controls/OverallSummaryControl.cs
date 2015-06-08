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
using System.Collections;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
//using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Controls;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// User control to display the overall system summary
    /// </summary>
    public partial class OverallSummaryControl : UserControl, IDataDisplayControl
    {
        #region Fields

        public delegate void OnShowOffenderClick(ShowOffenderEventArgs args);
        public event OnShowOffenderClick OnShowOffenderClickImpl;

        public delegate void OnShowSQLOffenderClick();
        public event OnShowSQLOffenderClick OnShowSQLOffenderClickImpl;

        public event EventHandler OnSelectedTseMetricChanged;
        
        public const string OverallSystemMetricTooltip = " Node: #VALX\n{0}: #VALY\nat {1}";

        // private DataTable healthDataTable = new DataTable();
/*        private SystemMetricChartControl _theChart1 = null;
        private SystemMetricChartControl _theChart2 = null;
        private List<SystemMetricModel.SystemMetrics> _theConfig1 = null;
        private List<SystemMetricModel.SystemMetrics> _theConfig2 = null;
        SystemHealthStatesUserControl _theHealthStates = null;*/
        WidgetContainer widgetContainerH = null;

        private System.Drawing.Size _theCurrentSize;
        public delegate void NewDataArrivalHandler(object sender, NewDataEventArgs e);
        public event NewDataArrivalHandler OnNewDataArrival;

        private ConnectionDefinition _theConnectionDefinition = null;
        private DataProvider _theDataProvider = null;
        private UniversalWidgetConfig _theWidgetConfig = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        private MenuItem _tseContextMenuItems = null;
        //private SystemMetricChartConfig _theOverallSummaryChartConfig = null;
        private List<SystemMetricModel.SystemMetrics> _theOverallSummaryMetrics = null;
        private SystemMetricModel.SystemMetricDisplays _theMetricDisplay;
        private String _metricName=string.Empty;
        private int _metricNodeID = -1;
        private ContextMenu _theWidgetContextMenu = null;
        private ContextMenu _theHealthCheckContextMenu = null;
        private bool _showMetrics = false;
        private bool _showHealthState = false;
        private Hashtable _theMetricsUpdated = new Hashtable();
        private Size _childrenWindowSize = new Size(800, 600);

        private const string _theStatesAvailableTooltip = "Click for details.";
        private const string OverallSummaryControlCanvasPersistenceKey = "OverallSummaryCanvasPersistenceKey_v1";

        private const int NULL_VERSION = -1;
        ConnectionDefinition.SERVER_VERSION _serverVersion = (ConnectionDefinition.SERVER_VERSION)NULL_VERSION;
        private readonly object _realtimeTseSyncRoot = new object();
        private List<int> _diskIoDownNodes = new List<int>();

        #endregion 

        #region Properties

        /// <summary>
        /// Property: ConnectonDefn - the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }


        /// <summary>
        /// Property: DataProvider - the data provider used by this widget
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
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

        /// <summary>
        /// Property: ShowMetrics - to display metric widget
        /// </summary>
        public bool ShowMetrics
        {
            get { return _showMetrics; }
        }

        /// <summary>
        /// Property: ShowHealthState - to display health & state widget
        /// </summary>
        public bool ShowHealthState
        {
            get { return _showHealthState; }
        }

        public int DiskIoNodeCount
        {
            get;
            set;
        }

        public bool IsTseConnectionTimeOut
        {
            get;
            set;
        }


        private List<int> DiskIoDownNodes
        {
            get
            {
                List<int> diskIoDownNodes = new List<int>();
                diskIoDownNodes.AddRange(this._diskIoDownNodes);
                return diskIoDownNodes;
            }
            set
            {
                this._diskIoDownNodes.Clear();
                _diskIoDownNodes.AddRange(value);
            }
        }

        public DataTable RealtimeTseDataTable
        {
            get;
            set;
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// The constructor
        /// </summary>
        //public OverallSummaryControl(SystemMetricChartConfig aOverallSummaryChartConfig)
        public OverallSummaryControl(ConnectionDefinition aConnectionDefinition, List<SystemMetricModel.SystemMetrics> aOverallSummaryMetrics, bool aShowHealthState)
        {
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            _theOverallSummaryMetrics = aOverallSummaryMetrics;
            _showMetrics = (_theOverallSummaryMetrics.Count > 0);
            _showHealthState = aShowHealthState;

            if (string.IsNullOrEmpty(this.ConnectionDefn.PlatformReleaseVersion))
            {
                ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
            }
            else
            {
                this._serverVersion = this.ConnectionDefn.ServerVersion;
            }
        }
        

        ~OverallSummaryControl()
        {
            ConnectionDefinition.Changed -= new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Persistent configruation - this is not used. But, for need to be here to fulfill the requirement of the interface
        /// </summary>
        public void PersistConfiguration()
        {
        }
        
        public void ClearChartSeries(string systemMetric)
        {
            SystemMetricChartControl[] chartControls = new SystemMetricChartControl[] { this._theChart1, this._theChart2 };
            foreach (SystemMetricChartControl chartControl in chartControls)
            {
                if (chartControl != null)
                {
                    chartControl.ClearChartSeries(systemMetric);
                }
            }
        }

        public void ClearHealthState(string healthLayer)
        {
            if (_theHealthStates != null)
            {
                _theHealthStates.ClearHealthState(healthLayer);
            }
        }

        #endregion Public methods

        #region Private methods

        void ConnectionDefinition_Changed(object sender, ConnectionDefinition connectionDefinition, ConnectionDefinition.Reason reason)
        {
            if (connectionDefinition.Name == this.ConnectionDefn.Name
                && reason == ConnectionDefinition.Reason.PlatformReleaseVersion)
            {
                this._serverVersion = connectionDefinition.ServerVersion;

                if (this.IsHandleCreated)
                {
                    this.Invoke(new MethodInvoker(() => SetTseChartVisibility(this._serverVersion)));
                }
                else
                {
                    this.HandleCreated += (objSender, e) => this.Invoke(new MethodInvoker(() => SetTseChartVisibility(this._serverVersion)));
                }
            }
        }

        private void SetTseChartVisibility(ConnectionDefinition.SERVER_VERSION serverVersion)
        {
            if (serverVersion != (ConnectionDefinition.SERVER_VERSION)NULL_VERSION
                && serverVersion < ConnectionDefinition.SERVER_VERSION.SQ151)
            {
                SystemMetricChartControl chartControl = null;
                List<SystemMetricModel.SystemMetrics> chartConfig = null;
                if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Disk))
                {
                    chartControl = _theChart1;
                    chartConfig = _theConfig1;
                }
                else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Disk))
                {
                    chartControl = _theChart2;
                    chartConfig = _theConfig2;
                }

                if (chartControl.Chart != null)
                {
                    string tseChartAreaName = SystemMetricModel.GetChartAreaName(SystemMetricModel.SystemMetrics.Tse);
                    ChartArea tseChartArea = chartControl.Chart.ChartAreas.FindByName(tseChartAreaName);
                    if (tseChartArea != null)
                    {
                        float currentHeight = tseChartArea.Position.Height;
                        int metricCount = chartConfig.Count - 1;
                        float metricHeight = (96 / metricCount - 1) - 1;
                        int i = 0;
                        foreach (SystemMetricModel.SystemMetrics metric in chartConfig)
                        {
                            string chartAreaName = SystemMetricModel.GetChartAreaName(metric);
                            ChartArea chartArea = chartControl.Chart.ChartAreas.FindByName(chartAreaName);
                            if ( chartArea != null && 0 != string.Compare( tseChartAreaName, chartArea.Name, true))
                            {
                                chartArea.Position.Y = 4 + i * (100 / metricCount - 1);
                                chartArea.Position.Height = metricHeight;
                                i ++;
                            }
                        }

                        tseChartArea.Visible = false;
                    }
                }
            }
        }

        /// <summary>
        /// To fire up data arrival event.
        /// </summary>
        /// <param name="e"></param>
        private void FireNewDataArrival(NewDataEventArgs e)
        {
            if (OnNewDataArrival != null)
            {
                OnNewDataArrival(this, e);
            }
        }

        /// <summary>
        /// To load the initial system metrics
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OverallSummaryControl_Load(object sender, EventArgs e)
        {
            CreateSystemMetricsWidget(_theOverallSummaryMetrics);
            SetTseChartVisibility(this._serverVersion);
        }

        /// <summary>
        /// To create system metric widgets
        /// </summary>
        /// <param name="aMetrics"></param>
        private void CreateSystemMetricsWidget(List<SystemMetricModel.SystemMetrics> aMetrics)
        {
            // this._theCanvas.ThePersistenceKey = OverallSummaryControlCanvasPersistenceKey;
            GridLayoutManager gridLayoutManager = new GridLayoutManager(10, 6);
            gridLayoutManager.CellSpacing = 4;
            _theCanvas.LayoutManager = gridLayoutManager;

            if (ShowMetrics)
            {
                _theConfig1 = new List<SystemMetricModel.SystemMetrics>();
                _theConfig2 = new List<SystemMetricModel.SystemMetrics>();

                int metricCount = aMetrics.Count;
                
                if (metricCount > 5)
                {
                    // We need two charts; also try to put network rcv and txn in chart2
                    for (int i = 0; i < metricCount; i++)
                    {
                        SystemMetricModel.SystemMetrics m = aMetrics[i];
                        switch (m)
                        {
                            case SystemMetricModel.SystemMetrics.Core:
                            case SystemMetricModel.SystemMetrics.Memory:
                            case SystemMetricModel.SystemMetrics.Swap:
                            case SystemMetricModel.SystemMetrics.File_System:
                            case SystemMetricModel.SystemMetrics.Load_Avg:
                                if (_theConfig1.Count > (metricCount - 1) / 2)
                                {
                                    _theConfig2.Add(m);
                                }
                                else
                                {
                                    _theConfig1.Add(m);
                                }
                                break;

                            case SystemMetricModel.SystemMetrics.Disk:
                            case SystemMetricModel.SystemMetrics.Tse:
                            case SystemMetricModel.SystemMetrics.Network_Rcv:
                            case SystemMetricModel.SystemMetrics.Network_Txn:
                            case SystemMetricModel.SystemMetrics.Virtual_Memory:
                                if (_theConfig2.Count > (metricCount - 1) / 2)
                                {
                                    _theConfig1.Add(m);
                                }
                                else
                                {
                                    _theConfig2.Add(m);
                                }
                                break;
                        }
                    }

                    _theChart1 = new SystemMetricChartControl(ConnectionDefn, _theConfig1);
                    _theChart2 = new SystemMetricChartControl(ConnectionDefn, _theConfig2);

                    GridConstraint gridConstraint = (ShowHealthState) ? new GridConstraint(0, 0, 8, 3) : new GridConstraint(0, 0, 10, 3);
                    WidgetContainer widgetContainer = new WidgetContainer(_theCanvas, _theChart1, "MetricWidget_1");
                    widgetContainer.Name = "MetricWidget_1";
                    widgetContainer.Title = "";
                    widgetContainer.AllowDelete = false;
                    _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);

                    gridConstraint = (ShowHealthState) ? new GridConstraint(0, 3, 8, 3) : new GridConstraint(0, 3, 10, 3);
                    widgetContainer = new WidgetContainer(_theCanvas, _theChart2, "MetricWidget_2");
                    widgetContainer.Name = "MetricWidget_2";
                    widgetContainer.Title = "";
                    widgetContainer.AllowDelete = false;
                    _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);

                    _theChart1.Chart.MouseDoubleClick += ChartMouseDoubleClick;
                    _theChart2.Chart.MouseDoubleClick += ChartMouseDoubleClick;
                    _theChart1.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                    _theChart2.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                }
                else
                {
                    // Just put everything in one chart
                    _theChart1 = new SystemMetricChartControl(ConnectionDefn, aMetrics);

                    GridConstraint gridConstraint = (ShowHealthState) ? new GridConstraint(0, 0, 8, 6) : new GridConstraint(0, 0, 10, 6);
                    WidgetContainer widgetContainer = new WidgetContainer(_theCanvas, _theChart1, "MetricWidget_1");
                    widgetContainer.Name = "MetricWidget_1";
                    widgetContainer.Title = "";
                    widgetContainer.AllowDelete = false;
                    _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);
                    _theChart1.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                    _theChart1.Chart.MouseDoubleClick += ChartMouseDoubleClick;
                }

                this.CreateContextMenu();
            }

            if (ShowHealthState)
            {
                // Health and States      
                GridConstraint gridConstraintH = (ShowMetrics) ? new GridConstraint(8, 0, 2, 6) : new GridConstraint(0, 0, 2, 6);

                //healthStates = new SystemHealthStatesUserControl();
                DataTable healthStatesDataTable = SystemMetricChartConfigModel.Instance.GetHealthStatesTable(ConnectionDefn.Name);
                _theHealthStates = new SystemHealthStatesUserControl(ConnectionDefn);
                _theHealthStates.AccessLayer.MouseClickLight += accessStatusLight_MouseClickLight;
                _theHealthStates.DatabaseLayer.MouseClickLight += databaseStatusLight_MouseClickLight;
                _theHealthStates.FoundationLayer.MouseClickLight += foundationStatusLight_MouseClickLight;
                _theHealthStates.OSLayer.MouseClickLight += osStatusLight_MouseClickLight;
                _theHealthStates.ServerLayer.MouseClickLight += serverStatusLight_MouseClickLight;
                _theHealthStates.StorageLayer.MouseClickLight += storageStatusLight_MouseClickLight;

                widgetContainerH = new WidgetContainer(_theCanvas, _theHealthStates, "Health/State");
                widgetContainerH.Name = "Health/State";
                widgetContainerH.Title = "Health/State";
                widgetContainerH.AllowDelete = false;
                widgetContainerH.AllowMaximize = true;
                widgetContainerH.Resizable = widgetContainerH.Moveable = true;
                this._theCanvas.AddWidget(widgetContainerH, gridConstraintH, -1);
            }

            this._theCanvas.Dock = DockStyle.Fill;
            this._theCanvas.InitializeCanvas();
            this._theCurrentSize = this.Size;
        }

        /// <summary>
        /// Chart1 mouse click event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ChartMouseDoubleClick(object sender, MouseEventArgs e)
        {
            Chart targetChart = (Chart)sender;
            var pos = e.Location;
            GetHitChartMetricDetails(targetChart, pos);
            ShowMetricDetails();
        }

        /// <summary>
        /// Test to see if the hit is in a chart area.
        /// </summary>
        /// <param name="aChart"></param>
        /// <param name="pos"></param>
        private void GetHitChartMetricDetails(Chart aChart, Point pos)
        {
            
            _metricName = string.Empty;
            _metricNodeID = -1;
            var results = aChart.HitTest(pos.X, pos.Y, false, new ChartElementType[] { ChartElementType.PlottingArea, ChartElementType.DataPoint });

            if (results.Length > 0)
            {
                var result = results[0];
                if (result.ChartElementType == ChartElementType.DataPoint || 
                    result.ChartElementType == ChartElementType.PlottingArea)
                {
                    _metricName = result.ChartArea.Name;

                    if (_metricName.Equals("ChartArea_NodeID"))
                    {
                        _metricName = string.Empty;
                        return;
                    }
                    
                    if (result.ChartElementType == ChartElementType.DataPoint)
                    {
                        _metricNodeID = result.PointIndex;
                    }

                    SystemMetricModel.SystemMetrics metric = (SystemMetricModel.SystemMetrics)Enum.Parse(typeof(SystemMetricModel.SystemMetrics), _metricName); ;
                    
                    switch (metric)
                    {
                        case SystemMetricModel.SystemMetrics.Core:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.CoreMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.Memory:
                        case SystemMetricModel.SystemMetrics.Swap:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.MemoryMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.File_System:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.FileSystemMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.Load_Avg:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.LoadAvgMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.Disk:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.DiskMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.Tse:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.TseMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.Network_Rcv:
                        case SystemMetricModel.SystemMetrics.Network_Txn:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.NetworkMetricDetails;
                            break;

                        case SystemMetricModel.SystemMetrics.Virtual_Memory:
                            _theMetricDisplay = SystemMetricModel.SystemMetricDisplays.VMMetricDetails;
                            break;

                        default:
                            _metricName = string.Empty;
                            break;
                    }
                }
            }
        }

        private void ShowTransactions()
        {
            if (_metricName.Equals(string.Empty)) return;
            string windowTitle = _metricNodeID == -1 ? Properties.Resources.TitleTransactionsOfAllNodes : string.Format(Properties.Resources.TitleTransactionsOfSpecifiedNode, _metricNodeID);

            TransactionUserControl parentChildQueriesUserControl = new TransactionUserControl(_metricNodeID, _theConnectionDefinition);
            Utilities.LaunchManagedWindow(windowTitle, parentChildQueriesUserControl, _theConnectionDefinition, TransactionUserControl.IdealWindowSize, true);
        }

        /// <summary>
        /// Show detailed 
        /// </summary>
        private void ShowMetricDetails()
        {
            if (_metricName.Equals(string.Empty)) return;

            string title = "";

            switch (_theMetricDisplay)
            {
                case SystemMetricModel.SystemMetricDisplays.CoreMetricDetails:
                    title = _metricName;

                    if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140) 
                    {
                        if (_metricNodeID == -1)
                        {
                            MessageBox.Show(Properties.Resources.CoreBusyDrillDownWarning, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                            return;
                        }
                    }
                    //In M8, message is node level and aggregration has been done on server side, so drilldown is not supported
                    else                        
                    {
                        MessageBox.Show(string.Format(Properties.Resources.DrillDownNotSupported, "%Node Busy", "core"), "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }                    
                    break;

                case SystemMetricModel.SystemMetricDisplays.MemoryMetricDetails:
                case SystemMetricModel.SystemMetricDisplays.SwapMetricDetails:
                    title = _metricName;
                    break;

                case SystemMetricModel.SystemMetricDisplays.FileSystemMetricDetails:                    
                    title = _metricName;
                    if (_metricNodeID == -1)
                    {
                        MessageBox.Show(Properties.Resources.FileDrillDownWarning, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }
                    break;

                case SystemMetricModel.SystemMetricDisplays.LoadAvgMetricDetails:
                    title = "Load Average";
                    break;

                case SystemMetricModel.SystemMetricDisplays.VMMetricDetails:
                    title = "Virtual Memory";
                    break;

                case SystemMetricModel.SystemMetricDisplays.NetworkMetricDetails:
                    title = "Network";
                    break;

                case SystemMetricModel.SystemMetricDisplays.DiskMetricDetails:
                    title = _metricName;
                    //In M8 message aggregration is done on server side so client drill down is not supported. M7 or earlier release it is OK.
                    if (this.ConnectionDefn.ServerVersion < ConnectionDefinition.SERVER_VERSION.SQ151)
                    {
                        MessageBox.Show(string.Format(Properties.Resources.DrillDownNotSupported, "Disk IOs", "disk"), "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }
                    else
                    {
                        if (_metricNodeID == -1)
                        {
                            MessageBox.Show(Properties.Resources.DiskDrillDownWarning, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                            return;
                        }
                    }
                    break;

                case SystemMetricModel.SystemMetricDisplays.TseMetricDetails:
                    title = "TSE " + SystemMetricModel.GetTseMetricText();
                    if (_metricNodeID == -1)
                    {
                        MessageBox.Show(Properties.Resources.TseDrillDownWarning, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                        return;
                    }
                    break;
            }

            DetailedSummaryControl control = new DetailedSummaryControl(ConnectionDefn, _theMetricDisplay, this, _metricNodeID, this.DiskIoNodeCount);
            bool windowLaunched = Utilities.LaunchManagedWindow(string.Format("Detailed {0} Metrics for {1}", title, (_metricNodeID == -1) ? "All Nodes" : string.Format("Node {0}", _metricNodeID)), 
                control, _theConnectionDefinition, new Size(this.Width - 5, this.Height - 5), true);
            if (!windowLaunched)
            {
                control.Dispose();
            }

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void Chart_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                Chart targetChart = (Chart)sender;
                var pos = e.Location;
                GetHitChartMetricDetails(targetChart, pos);
                if (_metricName.Equals(string.Empty)) return;

                bool isOdbcConnectionReady = (ConnectionDefn.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded);
                SystemMetricModel.SystemMetrics metric = (SystemMetricModel.SystemMetrics)Enum.Parse(typeof(SystemMetricModel.SystemMetrics), _metricName);
                switch (metric)
                {
                    case SystemMetricModel.SystemMetrics.Core:
                    case SystemMetricModel.SystemMetrics.Memory:
                    case SystemMetricModel.SystemMetrics.Swap:
                    case SystemMetricModel.SystemMetrics.Disk:
                    case SystemMetricModel.SystemMetrics.Tse:
                        _theWidgetContextMenu.MenuItems[0].Visible = true;
                        _theWidgetContextMenu.MenuItems[0].Enabled = isOdbcConnectionReady;
                        _theWidgetContextMenu.MenuItems[1].Visible = true;
                        _theWidgetContextMenu.MenuItems[1].Enabled = isOdbcConnectionReady;
                        _theWidgetContextMenu.MenuItems[2].Visible = true;
                        _theWidgetContextMenu.MenuItems[3].Visible = true;
                        _theWidgetContextMenu.MenuItems[3].Enabled = isOdbcConnectionReady;
                        _theWidgetContextMenu.MenuItems[4].Visible = true;
                        _theWidgetContextMenu.MenuItems[4].Enabled = isOdbcConnectionReady;
                        break;

                    default:
                        _theWidgetContextMenu.MenuItems[0].Visible = false;
                        _theWidgetContextMenu.MenuItems[1].Visible = false;
                        _theWidgetContextMenu.MenuItems[2].Visible = true;
                        _theWidgetContextMenu.MenuItems[3].Visible = true;
                        _theWidgetContextMenu.MenuItems[3].Enabled = isOdbcConnectionReady;
                        _theWidgetContextMenu.MenuItems[4].Visible = true;
                        _theWidgetContextMenu.MenuItems[4].Enabled = isOdbcConnectionReady;
                        break;
                }

                if (metric == SystemMetricModel.SystemMetrics.Tse)
                {
                    if (this._serverVersion != (ConnectionDefinition.SERVER_VERSION)NULL_VERSION)
                    {
                        bool isTseAvailable = this._serverVersion >= ConnectionDefinition.SERVER_VERSION.SQ151;
                        _tseContextMenuItems.Visible = _tseContextMenuItems.Enabled = isTseAvailable;
                        if (isTseAvailable)
                        {
                            string currentSelectedTseMetric = SystemMetricModel.SelectedTseMetric;
                            foreach (MenuItem tseMenuItem in _tseContextMenuItems.MenuItems)
                            {
                                tseMenuItem.Checked = 0 == string.Compare(currentSelectedTseMetric, (string)tseMenuItem.Tag, true);
                            }
                        }
                    }
                }
                else
                {
                    _tseContextMenuItems.Visible = false;
                }

                this._theWidgetContextMenu.Show(targetChart, e.Location);
            }
        }

        public void ResetLayout()
        {             
            _theCanvas.Locked = false;
            _theCanvas.ResetWidgetLayout();
        }

        public string LockLayout()
        {
            _theCanvas.LockMenuItem.PerformClick();
            return _theCanvas.LockMenuItem.Text.Trim();
        }

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_theChart1 != null)
                {
                    _theChart1.MouseDoubleClick -= ChartMouseDoubleClick;
                    _theChart1.MouseClick -= Chart_MouseClick;
                }
                if (_theChart2 != null)
                {
                    _theChart2.MouseDoubleClick -= ChartMouseDoubleClick;
                    _theChart2.MouseDoubleClick -= Chart_MouseClick;
                }
                if (_theHealthStates != null)
                {
                    _theHealthStates.AccessLayer.MouseClickLight -= accessStatusLight_MouseClickLight;
                    _theHealthStates.DatabaseLayer.MouseClickLight -= databaseStatusLight_MouseClickLight;
                    _theHealthStates.FoundationLayer.MouseClickLight -= foundationStatusLight_MouseClickLight;
                    _theHealthStates.OSLayer.MouseClickLight -= osStatusLight_MouseClickLight;
                    _theHealthStates.ServerLayer.MouseClickLight -= serverStatusLight_MouseClickLight;
                    _theHealthStates.StorageLayer.MouseClickLight -= storageStatusLight_MouseClickLight;
                }

                // this._theCanvas.SaveToPersistence();
            }
        }

        /// <summary>
        /// To update core busy chart with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateCoreBusy(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            SystemMetricChartControl chart;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Core))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Core))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate cores");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricCoreBusy, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricCoreBusyMin, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricCoreBusyMax, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            if (ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                int numNodes = (int)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.total_nodes.ToString()];

                for (int i = 0; i < numNodes; i++)
                {
                    DataRow r = table.NewRow();
                    double busy = 0;
                    double busyMin = 0;
                    double busyMax = 0;
                    try
                    {
                        busy = (double)aDataTable.Compute(string.Format("AVG({0})", SystemMetricModel.CoreMetricDataTableColumns.avg_core_total.ToString()),
                            string.Format("{0} = {1}", SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString(), i));

                        busyMin = (double)aDataTable.Compute(string.Format("MIN({0})", SystemMetricModel.CoreMetricDataTableColumns.avg_core_total.ToString()),
                            string.Format("{0} = {1}", SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString(), i));

                        busyMax = (double)aDataTable.Compute(string.Format("MAX({0})", SystemMetricModel.CoreMetricDataTableColumns.avg_core_total.ToString()),
                            string.Format("{0} = {1}", SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString(), i));
                    }
                    catch (Exception ex)
                    { }

                    r[SystemMetricModel.SystemMetricNodeID] = (busy < 0) ? "na" : string.Format("{0}", i);
                    r[SystemMetricModel.SystemMetricCoreBusy] = (busy < 0) ? double.NaN : busy;
                    r[SystemMetricModel.SystemMetricCoreBusyMin] = (busyMin < 0) ? double.NaN : busyMin;
                    r[SystemMetricModel.SystemMetricCoreBusyMax] = (busyMax < 0) ? double.NaN : busyMax;
                    r[SystemMetricModel.SystemMetricZero] = 0;
                    table.Rows.Add(r);
                }
            }
            else 
            {
                foreach (DataRow dr in aDataTable.Rows)
                {
                    DataRow row = table.NewRow();
                    double busy = 0;
                    try
                    {
                        busy = (double)dr[SystemMetricModel.CoreMetricDataTableColumns.avg_core_total.ToString()];
                    }
                    catch (Exception ex)
                    { }

                    row[SystemMetricModel.SystemMetricNodeID] = busy < 0 ? "na" : dr[SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString()];
                    row[SystemMetricModel.SystemMetricCoreBusy] = (busy < 0) ? double.NaN : busy;
                    row[SystemMetricModel.SystemMetricZero] = 0;
                    table.Rows.Add(row);
                }
            }

            DataView dv0 = new DataView(table);
            chart.Chart.Invalidate();

            // Series Busy
            string coreSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetrics.Core.ToString());
            chart.Chart.Series[coreSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricCoreBusy);
            chart.Chart.Series[coreSeriesName].ToolTip =
                string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Core), Utilities.StandardizeDateTime(currentTS));
                        
            /* 
             * Support "%Node Busy Min/Max" series since M9
             */
            if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                string coreMinSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricCoreBusyMin);
                chart.Chart.Series[coreMinSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricCoreBusyMin);
                chart.Chart.Series[coreMinSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.SystemMetricCoreBusyMinToolTip, Utilities.StandardizeDateTime(currentTS));

                string coreMaxSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricCoreBusyMax);
                chart.Chart.Series[coreMaxSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricCoreBusyMax);
                chart.Chart.Series[coreMaxSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.SystemMetricCoreBusyMaxToolTip, Utilities.StandardizeDateTime(currentTS));
            }

            UpdateChartArea(chart, SystemMetricModel.SystemMetrics.Core, dv0);
            chart.AllignCharts();
            chart.Chart.Update();
        }

        /// <summary>
        /// Update the Disk IOs chartarea with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateDiskIOs(DataTable aDataTable)
        {
            if ( aDataTable != null && aDataTable.Rows.Count > 0 )
            {
                this.DiskIoNodeCount = (int)aDataTable.Rows[0][SystemMetricModel.DiskMetricDataTableColumns.total_nodes.ToString()];
            }

            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;


            SystemMetricChartControl chart;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Disk))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Disk))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate disks");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricDiskIO, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricDiskIOMin, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricDiskIOMax, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            List<int> downNodes = new List<int>();
            if (ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
            {
                int numNodes = (int)aDataTable.Rows[0][SystemMetricModel.DiskMetricDataTableColumns.total_nodes.ToString()];

                for (int i = 0; i < numNodes; i++)
                {
                    DataRow r = table.NewRow();
                    double readsWrites = 0;
                    double readsWritesMin = 0;
                    double readsWritesMax = 0;
                    try
                    {
                        readsWrites = (double)aDataTable.Compute(string.Format("AVG({0})", SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes.ToString()),
                            string.Format("{0} = {1}", SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString(), i));

                        readsWritesMin = (double)aDataTable.Compute(string.Format("MIN({0})", SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes.ToString()),
                                                                 string.Format("{0} = {1}", SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString(), i));

                        readsWritesMax = (double)aDataTable.Compute(string.Format("MAX({0})", SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes.ToString()),
                                                                 string.Format("{0} = {1}", SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString(), i));
                    }
                    catch (Exception ex)
                    { }

                    if (readsWrites < 0) downNodes.Add(i);
                    r[SystemMetricModel.SystemMetricNodeID] = (readsWrites < 0) ? "na" : string.Format("{0}", i);
                    r[SystemMetricModel.SystemMetricDiskIO] = (readsWrites < 0) ? double.NaN : readsWrites;
                    r[SystemMetricModel.SystemMetricDiskIOMin] = (readsWritesMin < 0) ? double.NaN : readsWritesMin;
                    r[SystemMetricModel.SystemMetricDiskIOMax] = (readsWritesMax < 0) ? double.NaN : readsWritesMax;
                    r[SystemMetricModel.SystemMetricZero] = 0;
                    table.Rows.Add(r);
                }
            }
            else 
            {
                foreach (DataRow dr in aDataTable.Rows)
                {
                    DataRow row = table.NewRow();
                    double readswrites = 0;
                    try
                    {
                        readswrites = (double)dr[SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes.ToString()];
                    }
                    catch (Exception ex)
                    { }

                    if (readswrites < 0)
                    {
                        try
                        {
                            int downNodeId = int.Parse((string)dr[SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString()]);
                            downNodes.Add(downNodeId);
                        }
                        catch { }
                    }
                    row[SystemMetricModel.SystemMetricNodeID] = readswrites < 0 ? "na" : dr[SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString()];
                    row[SystemMetricModel.SystemMetricDiskIO] = (readswrites < 0) ? double.NaN : readswrites;
                    row[SystemMetricModel.SystemMetricZero] = 0;
                    table.Rows.Add(row);
                }
            }

            DiskIoDownNodes = downNodes;

            DataView dv0 = new DataView(table);
            chart.Chart.Invalidate();
            string diskSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetrics.Disk.ToString());
            chart.Chart.Series[diskSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricDiskIO);
            chart.Chart.Series[diskSeriesName].ToolTip =
                string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Disk), Utilities.StandardizeDateTime(currentTS));

            // Support "Disk IO Max" series since M10SP1
            if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
            {
                string readsWritesMinSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricDiskIOMin);
                chart.Chart.Series[readsWritesMinSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricDiskIOMin);
                chart.Chart.Series[readsWritesMinSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.SystemMetricDiskIOMinToolTip, Utilities.StandardizeDateTime(currentTS));

                string readsWritesMaxSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricDiskIOMax);
                chart.Chart.Series[readsWritesMaxSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricDiskIOMax);
                chart.Chart.Series[readsWritesMaxSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.SystemMetricDiskIOMaxToolTip, Utilities.StandardizeDateTime(currentTS));
            }

            UpdateChartArea(chart, SystemMetricModel.SystemMetrics.Disk, dv0);
            chart.AllignCharts();
            chart.Chart.Update();
        }
        
        /// <summary>
        /// Update the TSE chartarea with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateTseSkew(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            SystemMetricChartControl chart = null;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Tse))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Tse))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate TSEs");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricTse, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricTseMin, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricTseMax, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            string selectedTseMetricColumnName = SystemMetricModel.GetTseMetricColumnName();
            int numNodes = (int)aDataTable.Rows[0][SystemMetricModel.TseMetricDataTableColumns.total_nodes.ToString()];
            List<int> downNodes = DiskIoDownNodes;
            for (int i = 0; i < numNodes; i++)
            {
                DataRow r = table.NewRow();
                double tse = 0;
                double tseMin = 0;
                double tseMax = 0;
                try
                {
                    tse = (double)aDataTable.Compute(string.Format("AVG({0})", selectedTseMetricColumnName),
                        string.Format("{0} = {1}", SystemMetricModel.TseMetricDataTableColumns.node_id.ToString(), i));

                    tseMin = (double)aDataTable.Compute(string.Format("MIN({0})", selectedTseMetricColumnName),
                                                             string.Format("{0} = {1}", SystemMetricModel.TseMetricDataTableColumns.node_id.ToString(), i));

                    tseMax = (double)aDataTable.Compute(string.Format("MAX({0})", selectedTseMetricColumnName),
                                                             string.Format("{0} = {1}", SystemMetricModel.TseMetricDataTableColumns.node_id.ToString(), i));
                }
                catch (Exception ex)
                { }

                bool isNodeDown = downNodes.Contains(i);
                r[SystemMetricModel.SystemMetricNodeID] = isNodeDown ? "na" : string.Format("{0}", i);
                r[SystemMetricModel.SystemMetricTse] = isNodeDown ? double.NaN : tse;
                r[SystemMetricModel.SystemMetricTseMin] = isNodeDown ? double.NaN : tseMin;
                r[SystemMetricModel.SystemMetricTseMax] = isNodeDown ? double.NaN : tseMax;
                r[SystemMetricModel.SystemMetricZero] = 0;
                table.Rows.Add(r);
            }

            lock (this._realtimeTseSyncRoot)
            {
                DataView dv0 = new DataView(table);
                chart.Chart.Invalidate();
                string tseSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricTse);
                chart.Chart.Series[tseSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricTse);
                chart.Chart.Series[tseSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Tse), Utilities.StandardizeDateTime(currentTS));

                string tseMinSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricTseMin);
                chart.Chart.Series[tseMinSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricTseMin);
                chart.Chart.Series[tseMinSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.SystemMetricTseMinToolTip, Utilities.StandardizeDateTime(currentTS));

                string tseMaxSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetricTseMax);
                chart.Chart.Series[tseMaxSeriesName].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricTseMax);
                chart.Chart.Series[tseMaxSeriesName].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.SystemMetricTseMaxToolTip, Utilities.StandardizeDateTime(currentTS));

                UpdateChartArea(chart, SystemMetricModel.SystemMetrics.Tse, dv0);
                chart.AllignCharts();
                chart.Chart.Update();
            }
        }

        /// <summary>
        /// Update the Load Avg chartarea with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateLoadAvg(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            SystemMetricChartControl chart;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Load_Avg))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Load_Avg))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate load avg");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricOneMinAvg, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow row = table.NewRow();
                double loadvg = 0;
                try
                {
                    loadvg = (double)dr[SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg.ToString()];
                }
                catch(Exception e)
                {
                }
                row[SystemMetricModel.SystemMetricNodeID] = dr[SystemMetricModel.LoadAvgMetricDataTableColumns.node_name.ToString()];
                row[SystemMetricModel.SystemMetricOneMinAvg] = loadvg < 0 ? double.NaN : loadvg;
                row[SystemMetricModel.SystemMetricZero] = 0;
                table.Rows.Add(row);
            }

            DataView dv0 = new DataView(table);
            chart.Chart.Invalidate();
            chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Load_Avg.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricOneMinAvg);
            chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Load_Avg.ToString())].ToolTip =
                string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Load_Avg), Utilities.StandardizeDateTime(currentTS));
            UpdateChartArea(chart, SystemMetricModel.SystemMetrics.Load_Avg, dv0);
            chart.AllignCharts();
            chart.Chart.Update();
        }

        /// <summary>
        /// Update the memory chartareas with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateMemory(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate memory");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricMemoryUsed, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricSwapUsed, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));


            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow row = table.NewRow();
                row[SystemMetricModel.SystemMetricNodeID] = dr[SystemMetricModel.MemoryMetricDataTableColumns.node_name.ToString()];
                double memUsed = 0;
                try
                {
                    memUsed = (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.memory_used.ToString()];
                }
                catch (Exception ex)
                { } 
                double swapUsed = 0;
                try
                {
                    swapUsed = (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.swap_used.ToString()];
                }
                catch (Exception ex)
                { }

                row[SystemMetricModel.SystemMetricMemoryUsed] = memUsed < 0 ? double.NaN : memUsed;
                row[SystemMetricModel.SystemMetricSwapUsed] = swapUsed < 0 ? double.NaN : swapUsed;
                row[SystemMetricModel.SystemMetricZero] = 0;
                table.Rows.Add(row);
            }

            DataView dv0 = new DataView(table);
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Memory))
            {
                _theChart1.Chart.Invalidate();
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Memory.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricMemoryUsed);
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Memory.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Memory), Utilities.StandardizeDateTime(currentTS));
                UpdateChartArea(_theChart1, SystemMetricModel.SystemMetrics.Memory, dv0);
                _theChart1.AllignCharts();
                _theChart1.Chart.Update();
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Memory))
            {
                _theChart2.Chart.Invalidate();
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Memory.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricMemoryUsed);
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Memory.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Memory), Utilities.StandardizeDateTime(currentTS));
                UpdateChartArea(_theChart2, SystemMetricModel.SystemMetrics.Memory, dv0);
                _theChart2.AllignCharts();
                _theChart2.Chart.Update();
            }

            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Swap))
            {
                _theChart1.Chart.Invalidate();
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Swap.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricSwapUsed);
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Swap.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Swap), Utilities.StandardizeDateTime(currentTS));
                UpdateChartArea(_theChart1, SystemMetricModel.SystemMetrics.Swap, dv0);
                _theChart1.AllignCharts();
                _theChart1.Chart.Update();
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Swap))
            {
                _theChart2.Chart.Invalidate();
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Swap.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Swap), Utilities.StandardizeDateTime(currentTS));
                UpdateChartArea(_theChart2, SystemMetricModel.SystemMetrics.Swap, dv0);
                _theChart2.AllignCharts();
                _theChart2.Chart.Update();
            }
        }

        /// <summary>
        /// Update network chartarea with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateNetwork(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate nets");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricRcvPackets, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricTxnPackets, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            //int numNodes = (int)aDataTable.Compute("MAX(NodeID)", "") + 1;
            int numNodes = (int)aDataTable.Rows[0][SystemMetricModel.NetworkMetricDataTableColumns.total_nodes.ToString()]; 

            for (int i = 0; i < numNodes; i++)
            {
                DataRow r = table.NewRow();
                double rcvs = 0;
                double txns = 0;
                try
                {
                    //rcvs = (double)aDataTable.Compute("MAX([Rcv Packets])", string.Format("NodeID = {0}", i));
                    //txns = (double)aDataTable.Compute("MAX([Txn Packets])", string.Format("NodeID = {0}", i));
                    rcvs = (double)aDataTable.Compute(string.Format("MAX({0})", SystemMetricModel.NetworkMetricDataTableColumns.rcv_packets.ToString()),
                                         string.Format("{0} = {1}", SystemMetricModel.NetworkMetricDataTableColumns.node_id.ToString(), i));
                }
                catch (Exception ex)
                { }

                try
                {
                    txns = (double)aDataTable.Compute(string.Format("MAX({0})", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets.ToString()),
                     string.Format("{0} = {1}", SystemMetricModel.NetworkMetricDataTableColumns.node_id.ToString(), i));
                }
                catch (Exception e) { }

                r[SystemMetricModel.SystemMetricNodeID] = (rcvs < 0) ? "na" : string.Format("{0}", i);
                r[SystemMetricModel.SystemMetricRcvPackets] = (rcvs < 0) ? double.NaN : rcvs;
                r[SystemMetricModel.SystemMetricTxnPackets] = (txns < 0) ? double.NaN : txns;
                r[SystemMetricModel.SystemMetricZero] = 0;
                table.Rows.Add(r);
            }

            DataView dv0 = new DataView(table);

            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Network_Rcv))
            {
                _theChart1.Chart.Invalidate();
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Rcv.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricRcvPackets);
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Rcv.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Network_Rcv), Utilities.StandardizeDateTime(currentTS));
                UpdateChartArea(_theChart1, SystemMetricModel.SystemMetrics.Network_Rcv, dv0);
                _theChart1.AllignCharts();
                _theChart1.Chart.Update();
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Network_Rcv))
            {
                _theChart2.Chart.Invalidate();
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Rcv.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricRcvPackets);
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Rcv.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Network_Rcv), Utilities.StandardizeDateTime(currentTS));                
                UpdateChartArea(_theChart2, SystemMetricModel.SystemMetrics.Network_Rcv, dv0);
                _theChart2.AllignCharts();
                _theChart2.Chart.Update();
            }

            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Network_Txn))
            {
                _theChart1.Chart.Invalidate();
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Txn.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricTxnPackets);
                _theChart1.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Txn.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Network_Txn), Utilities.StandardizeDateTime(currentTS));                
                UpdateChartArea(_theChart1, SystemMetricModel.SystemMetrics.Network_Txn, dv0);
                _theChart1.AllignCharts();
                _theChart1.Chart.Update();
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Network_Txn))
            {
                _theChart2.Chart.Invalidate();
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Txn.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricTxnPackets);
                _theChart2.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Network_Txn.ToString())].ToolTip =
                    string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Network_Txn), Utilities.StandardizeDateTime(currentTS));
                UpdateChartArea(_theChart2, SystemMetricModel.SystemMetrics.Network_Txn, dv0);
                _theChart2.AllignCharts();
                _theChart2.Chart.Update();
            }
        }

        /// <summary>
        /// Update the virtual memory chartarea with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void Updatevirtualmem(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            SystemMetricChartControl chart;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Virtual_Memory))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Virtual_Memory))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate vm");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricContextSwitches, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            foreach (DataRow dr in aDataTable.Rows)
            {
                DataRow row = table.NewRow();
                double ctxswitch = 0;
                try
                {
                    ctxswitch = (double)dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches.ToString()];
                }
                catch (Exception ex)
                {
                }
                row[SystemMetricModel.SystemMetricNodeID] = dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.node_name.ToString()];
                row[SystemMetricModel.SystemMetricContextSwitches] = ctxswitch < 0 ? double.NaN : ctxswitch;
                row[SystemMetricModel.SystemMetricZero] = 0;
                table.Rows.Add(row);
            }

            DataView dv0 = new DataView(table);

            chart.Chart.Invalidate();
            chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Virtual_Memory.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricContextSwitches);
            chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.Virtual_Memory.ToString())].ToolTip =
                string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Virtual_Memory), Utilities.StandardizeDateTime(currentTS));
            UpdateChartArea(chart, SystemMetricModel.SystemMetrics.Virtual_Memory, dv0);
            chart.AllignCharts();
            chart.Chart.Update();
        }

        /// <summary>
        /// Update File System chartarea for the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateFileSystem(DataTable aDataTable)
        {
            if (!ShowMetrics || aDataTable == null || aDataTable.Rows.Count <= 0)
                return;

            SystemMetricChartControl chart;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.File_System))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.File_System))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DateTime currentTS = (DateTime)aDataTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
            DataTable table = new DataTable("aggregate filesys");
            table.Columns.Add(SystemMetricModel.SystemMetricNodeID, typeof(string));
            table.Columns.Add(SystemMetricModel.SystemMetricPercentConsumed, typeof(double));
            table.Columns.Add(SystemMetricModel.SystemMetricZero, typeof(int));

            //int numNodes = Convert.ToInt32(aDataTable.Compute("MAX(NodeID)", "")) + 1;
            int numNodes = (int)aDataTable.Rows[0][SystemMetricModel.FileSysMetricDataTableColumns.total_nodes.ToString()]; 

            for (int i = 0; i < numNodes; i++)
            {
                DataRow r = table.NewRow();
                double percent = 0;
                try
                {
                    //percent = (double)aDataTable.Compute("MAX(percent_consumed)", string.Format("NodeID = {0}", i));
                    percent = (double)aDataTable.Compute(string.Format("MAX({0})", SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed.ToString()),
                     string.Format("{0} = {1}", SystemMetricModel.FileSysMetricDataTableColumns.node_id.ToString(), i));
                }
                catch (Exception ex)
                { }

                r[SystemMetricModel.SystemMetricNodeID] = (percent < 0) ? "na" : string.Format("{0}", i);
                r[SystemMetricModel.SystemMetricPercentConsumed] = (percent < 0) ? double.NaN : percent;
                r[SystemMetricModel.SystemMetricZero] = 0;
                table.Rows.Add(r);
            }

            DataView dv0 = new DataView(table);
            chart.Chart.Invalidate();
            chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.File_System.ToString())].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricPercentConsumed);
            chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetrics.File_System.ToString())].ToolTip =
                string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.File_System), Utilities.StandardizeDateTime(currentTS));  
            UpdateChartArea(chart, SystemMetricModel.SystemMetrics.File_System, dv0);
            chart.AllignCharts();
            chart.Chart.Update();
        }

        /// <summary>
        /// Update the health & stats lights
        /// </summary>
        /// <param name="key"></param>
        /// <param name="aStats"></param>
        public void UpdateHealthStates(string key, Object aStats, bool isNewDataComing)
        {
            if (!ShowHealthState)
                return;

            DataTable table = LiveFeedStatsTransformer.TransformHealthStatesProtobufToLayerDataTable(key, aStats);
            DataRow dr = table.Rows[0];
            int previousState = 0;
            switch (key)
            {
                case LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer:
                    previousState = _theHealthStates.AccessLayer.State;
                    _theHealthStates.AccessLayer.State = (int)dr[LiveFeedStatsTransformer.HealthStatesLayerDataTableColumns.layer_current_score.ToString()];
                    _theHealthStates.AccessLayer.ToolTipText = _theStatesAvailableTooltip;
                    _theHealthStates.AccessLayer.TagObject = aStats;
                    Alarm(isNewDataComing, _theHealthStates.AccessLayer, previousState);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer:
                    previousState = _theHealthStates.DatabaseLayer.State;
                    _theHealthStates.DatabaseLayer.State = (int)dr[LiveFeedStatsTransformer.HealthStatesLayerDataTableColumns.layer_current_score.ToString()];
                    _theHealthStates.DatabaseLayer.ToolTipText = _theStatesAvailableTooltip;
                    _theHealthStates.DatabaseLayer.TagObject = aStats;
                    Alarm(isNewDataComing, _theHealthStates.DatabaseLayer, previousState);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer:
                    previousState = _theHealthStates.FoundationLayer.State;
                    _theHealthStates.FoundationLayer.State = (int)dr[LiveFeedStatsTransformer.HealthStatesLayerDataTableColumns.layer_current_score.ToString()];
                    _theHealthStates.FoundationLayer.ToolTipText = _theStatesAvailableTooltip;
                    _theHealthStates.FoundationLayer.TagObject = aStats;
                    Alarm(isNewDataComing, _theHealthStates.FoundationLayer, previousState);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer:
                    previousState = _theHealthStates.OSLayer.State;
                    _theHealthStates.OSLayer.State = (int)dr[LiveFeedStatsTransformer.HealthStatesLayerDataTableColumns.layer_current_score.ToString()];
                    _theHealthStates.OSLayer.ToolTipText = _theStatesAvailableTooltip; ;
                    _theHealthStates.OSLayer.TagObject = aStats;
                    Alarm(isNewDataComing, _theHealthStates.OSLayer, previousState);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer:
                    previousState = _theHealthStates.ServerLayer.State;
                    _theHealthStates.ServerLayer.State = (int)dr[LiveFeedStatsTransformer.HealthStatesLayerDataTableColumns.layer_current_score.ToString()];
                    _theHealthStates.ServerLayer.ToolTipText = _theStatesAvailableTooltip;
                    _theHealthStates.ServerLayer.TagObject = aStats;
                    Alarm(isNewDataComing, _theHealthStates.ServerLayer, previousState);
                    break;

                case LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer:
                    previousState = _theHealthStates.StorageLayer.State;
                    _theHealthStates.StorageLayer.State = (int)dr[LiveFeedStatsTransformer.HealthStatesLayerDataTableColumns.layer_current_score.ToString()];
                    _theHealthStates.StorageLayer.ToolTipText = _theStatesAvailableTooltip;
                    _theHealthStates.StorageLayer.TagObject = aStats;
                    Alarm(isNewDataComing, _theHealthStates.StorageLayer, previousState);
                    break;

                default:
                    break;
            }
        }

        private void Alarm(bool isNewDataComing, TrafodionStatusLightUserControl statusControl, int previousState)
        {
            if (!isNewDataComing) return;

            if (statusControl.IsAbnormal(previousState))
            {
                Utilities.FlashWindowEx(this.ParentForm);

                if (AlarmSoundHelper.IsAlarmOn)
                {
                    AlarmSoundHelper.Alarm();
                }
            }
        }

        /// <summary>
        /// By design, the node ID is only updated in the beginning. 
        /// </summary>
        /// <param name="chart"></param>
        /// <param name="metric"></param>
        /// <param name="dv0"></param>
        /*private void UpdateChartArea(SystemMetricChartControl chart, SystemMetricModel.SystemMetrics metric, DataView dv0)
        {
            // Update Node ID Chart Area
            if (!_theMetricsUpdated.ContainsKey(metric))
            {
                chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetricNodeID)].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricZero);
                _theMetricsUpdated.Add(metric, 1);
            }
            else if ((int)_theMetricsUpdated[metric] < 2)
            {
                chart.Chart.Series[string.Format("Series_{0}", SystemMetricModel.SystemMetricNodeID)].Points.DataBindXY(dv0, SystemMetricModel.SystemMetricNodeID, dv0, SystemMetricModel.SystemMetricZero);
                //_theMetricsUpdated[metric] = 2;
            }

            // Change back color to indicate the chart area is active
            string seriesName = SystemMetricChartControl.GetChartSeriesName(metric.ToString());
            Series updatedSeries = chart.Chart.Series.FindByName(seriesName);
            if (updatedSeries != null) 
            {
                Color configBackColor = SystemMetricChartConfigModel.Instance.GetSystemMetricBackColor(ConnectionDefn.Name, metric);
                chart.ChartAreas[updatedSeries.ChartArea].BackColor = configBackColor;
            }
        }

        private int GetHealthDataCategory(DataTable argDataTable, string categoryName) 
        {
            int state = 0;

            DataRow[] foundRows = argDataTable.Select("Layer='" + categoryName + "'", "State DESC");
            if (0 < foundRows.Length) 
            {
                state = int.Parse(foundRows[0][2].ToString());
            }

            return state;
        }

        private DataTable getSubjectAreaData(string argLayer, DataTable argDataTable) 
        {
            DataTable subjectAreaDataTable = argDataTable.Clone();
            DataRow[] foundRows = argDataTable.Select("Layer='" + argLayer + "'");
            
            foreach (DataRow row in foundRows)
            {                
                subjectAreaDataTable.ImportRow(row);                
            }

            return subjectAreaDataTable;
        }

        private void accessStatusLight_MouseClickLight(object sender, EventArgs e)
        {
            Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl light = (Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl)sender;
            PopupSubjectAreaDialog(LiveFeedRoutingKeyMapper.PUBS_health_state_access_layer, light.TagObject);
            if (_theHealthCheckContextMenu != null && light.TagObject == null && ((MouseEventArgs)e).Button == MouseButtons.Right)
            {
                this._theHealthCheckContextMenu.Show(_theHealthStates.AccessLayer, ((MouseEventArgs)e).Location);
            }
            
        }

                
        private void databaseStatusLight_MouseClickLight(object sender, EventArgs e)
        {
            Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl light = (Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl)sender;
            PopupSubjectAreaDialog(LiveFeedRoutingKeyMapper.PUBS_health_state_database_layer, light.TagObject);
            if (_theHealthCheckContextMenu != null && light.TagObject == null && ((MouseEventArgs)e).Button == MouseButtons.Right)
            {
                this._theHealthCheckContextMenu.Show(_theHealthStates.DatabaseLayer, ((MouseEventArgs)e).Location);
            }
        }
        
        private void foundationStatusLight_MouseClickLight(object sender, EventArgs e)
        {
            Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl light = (Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl)sender;
            PopupSubjectAreaDialog(LiveFeedRoutingKeyMapper.PUBS_health_state_foundation_layer, light.TagObject);
            if (_theHealthCheckContextMenu != null && light.TagObject == null && ((MouseEventArgs)e).Button == MouseButtons.Right)
            {
                this._theHealthCheckContextMenu.Show(_theHealthStates.FoundationLayer, ((MouseEventArgs)e).Location);
            }
        }

        private void osStatusLight_MouseClickLight(object sender, EventArgs e)
        {
            Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl light = (Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl)sender;
            PopupSubjectAreaDialog(LiveFeedRoutingKeyMapper.PUBS_health_state_os_layer, light.TagObject);
            if (_theHealthCheckContextMenu != null && light.TagObject == null && ((MouseEventArgs)e).Button == MouseButtons.Right)
            {
                this._theHealthCheckContextMenu.Show(_theHealthStates.OSLayer, ((MouseEventArgs)e).Location);
            }
        }

        private void serverStatusLight_MouseClickLight(object sender, EventArgs e)
        {
            Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl light = (Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl)sender;
            PopupSubjectAreaDialog(LiveFeedRoutingKeyMapper.PUBS_health_state_server_layer, light.TagObject);
            if (_theHealthCheckContextMenu != null && light.TagObject == null && ((MouseEventArgs)e).Button == MouseButtons.Right)
            {
                this._theHealthCheckContextMenu.Show(_theHealthStates.ServerLayer, ((MouseEventArgs)e).Location);
            }
        }

        private void storageStatusLight_MouseClickLight(object sender, EventArgs e)
        {
            Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl light = (Trafodion.Manager.Framework.Controls.TrafodionStatusLightUserControl)sender;
            PopupSubjectAreaDialog(LiveFeedRoutingKeyMapper.PUBS_health_state_storage_layer, light.TagObject);
            if (_theHealthCheckContextMenu != null && light.TagObject == null && ((MouseEventArgs)e).Button == MouseButtons.Right)
            {
                this._theHealthCheckContextMenu.Show(_theHealthStates.StorageLayer, ((MouseEventArgs)e).Location);
            }
        }
        
        private void PopupSubjectAreaDialog(string aPublication, object aStats)
        {
            if (aStats != null)
            {
                DateTime currentTS;
                DataTable theDataTable = LiveFeedStatsTransformer.TransformHealthStatesProtobufToSubjectDataTable(aPublication, aStats, out currentTS);
                HealthPopupDialog popUpDialog = new HealthPopupDialog(aPublication, theDataTable, currentTS);
                popUpDialog.ShowDialog();
            }
            else if (_theHealthCheckContextMenu==null)
            {
                if (this._serverVersion != (ConnectionDefinition.SERVER_VERSION)NULL_VERSION)
                {
                    if (this._serverVersion >=ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        _theHealthCheckContextMenu = new ContextMenu();
                        _theHealthCheckContextMenu.MenuItems.Add(Properties.Resources.CheckLiveFeedInfraMenuCaption, new EventHandler(LiveFeedInfrastructureHealthCheck_MouseClick));
                    }
                }
            }
        }

        public void ResetTseSkew()
        {
            SystemMetricChartControl chartControl = null;
            if (_theChart1.Metrics.Contains(SystemMetricModel.SystemMetrics.Tse))
            {
                chartControl = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(SystemMetricModel.SystemMetrics.Tse))
            {
                chartControl = _theChart2;
            }

            if (chartControl != null && chartControl.Chart != null)
            {
                string tseChartAreaName = SystemMetricModel.GetChartAreaName(SystemMetricModel.SystemMetrics.Tse);
                ChartArea tseChartArea = chartControl.Chart.ChartAreas.FindByName(tseChartAreaName);
                if (tseChartArea != null)
                {
                    Title tseChartTitle = chartControl.Chart.Titles.FindByName(SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Tse));
                    tseChartTitle.Text = tseChartTitle.ToolTip = SystemMetricModel.GetTseMetricTitle();
                }
            }

            if (!this.IsTseConnectionTimeOut)
            {
                if (this.RealtimeTseDataTable != null && this.RealtimeTseDataTable.Rows.Count > 0)
                {
                    UpdateTseSkew(this.RealtimeTseDataTable);
                }
            }
        }*/
 
        private void CreateContextMenu()
        {
            _theWidgetContextMenu = new ContextMenu();
            _theWidgetContextMenu.MenuItems.Add("Show System &Offender", new EventHandler(ShowSystemOffender_MouseClick));
            
            MenuItem showSQLOffenderMenuItem = _theWidgetContextMenu.MenuItems.Add("Show &SQL Offender", new EventHandler(ShowSQLSystemOffender_MouseClick));

            _theWidgetContextMenu.MenuItems.Add("Show Metric &Details", new EventHandler(ShowDetailMetrics_MouseClick));

            MenuItem showTransactionsMenuItem = _theWidgetContextMenu.MenuItems.Add("Show &Transactions", new EventHandler(ShowTransactions_MouseClick));
            showTransactionsMenuItem.Enabled = false;

            MenuItem checkLiveFeedInfraMenuItem = _theWidgetContextMenu.MenuItems.Add(Properties.Resources.CheckLiveFeedInfraMenuCaption, new EventHandler(LiveFeedInfrastructureHealthCheck_MouseClick));
            checkLiveFeedInfraMenuItem.Enabled = false;

            _theWidgetContextMenu.MenuItems.Add("-");

            _tseContextMenuItems = _theWidgetContextMenu.MenuItems.Add(SystemMetricModel.SystemMetricTseTitle);
            string selectedTseMetric = SystemMetricModel.SelectedTseMetric;
            foreach (string metric in SystemMetricModel.TseMetrics.Keys)
            {
                MenuItem subTseMenuItem = _tseContextMenuItems.MenuItems.Add(SystemMetricModel.TseMetrics[metric], TseMetric_MouseClick);
                subTseMenuItem.Tag = metric;
            }
            _tseContextMenuItems.Enabled = false;
            
            _theWidgetContextMenu.Popup += (sender, e) =>
            {
                if (this._serverVersion != (ConnectionDefinition.SERVER_VERSION)NULL_VERSION )
                {
                    if (this._serverVersion < ConnectionDefinition.SERVER_VERSION.SQ151)
                    {
                        if (showSQLOffenderMenuItem.Visible)
                            showSQLOffenderMenuItem.Visible = false;

                        showTransactionsMenuItem.Visible = false;
                    }
					
					if (this._serverVersion < ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        checkLiveFeedInfraMenuItem.Visible = false;
                    }
                    else
                    {
                        checkLiveFeedInfraMenuItem.Enabled = true;
                    }
                }
            };
        }

        private void ShowSystemOffender_MouseClick(object sender, EventArgs e)
        {
            if (OnShowOffenderClickImpl != null)
            {
                ShowOffenderEventArgs args;
                if(_theMetricDisplay == SystemMetricModel.SystemMetricDisplays.CoreMetricDetails
                    || _theMetricDisplay == SystemMetricModel.SystemMetricDisplays.TseMetricDetails)
                {
                    args = new ShowOffenderEventArgs(ShowOffenderEventArgs.CommandType.CPU);
                }
                else
                {
                    args = new ShowOffenderEventArgs(ShowOffenderEventArgs.CommandType.MEM);
                }
                
                OnShowOffenderClickImpl(args);
            }
        }

        private void ShowSQLSystemOffender_MouseClick(object sender, EventArgs e)
        {
            if (OnShowSQLOffenderClickImpl != null)
            {
                OnShowSQLOffenderClickImpl();
            }
        }

        private void ShowDetailMetrics_MouseClick(object sender, EventArgs e)
        {
            ShowMetricDetails();
        }

        private void LiveFeedInfrastructureHealthCheck_MouseClick(object sender, EventArgs e)
        {
            HealthPopupDialog popUpDialog = new HealthPopupDialog();
            popUpDialog.LiveFeedInfrastructureCheck();
        }

        private void ShowTransactions_MouseClick(object sender, EventArgs e)
        {
            bool isODBCConnected = this.ConnectionDefn.TheState == ConnectionDefinition.State.TestSucceeded;
            if (!isODBCConnected)
            {
                ConnectionDefinitionDialog connectionDialog = new ConnectionDefinitionDialog(false);
                connectionDialog.Edit(this.ConnectionDefn);
                isODBCConnected = this.ConnectionDefn.TheState == ConnectionDefinition.State.TestSucceeded;
            }

            if (isODBCConnected)
            {
                ShowTransactions();
            }
            else
            {
                MessageBox.Show(Properties.Resources.ODBCNeededForHealthDrillDown, "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }            
        }

        private void TseMetric_MouseClick(object sender, EventArgs e)
        {
            MenuItem selectedMenuItem = (MenuItem)sender;
            string selectedTseMetric = (string)selectedMenuItem.Tag;
            SystemMetricModel.SelectedTseMetric = selectedTseMetric;
            ResetTseSkew();
            if (OnSelectedTseMetricChanged != null)
            {
                OnSelectedTseMetricChanged(null, null);
            }

            foreach (MenuItem tseMenuItem in selectedMenuItem.Parent.MenuItems)
            {
                if (tseMenuItem.Checked)
                {
                    tseMenuItem.Checked = false;
                    break;
                }
            }
            selectedMenuItem.Checked = true;
        }     


        #endregion Private methods
    }

    public class NewDataEventArgs : EventArgs
    {

        private DataTable _dataTable = null;

        /// <summary>
        /// Property: DataTable
        /// </summary>
        public DataTable DataTable
        {
            get { return _dataTable; }
            set { _dataTable = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="originalState"></param>
        /// <param name="currentState"></param>
        public NewDataEventArgs(DataTable aDataTable)
        {
            _dataTable = aDataTable;
        }
    }
}
