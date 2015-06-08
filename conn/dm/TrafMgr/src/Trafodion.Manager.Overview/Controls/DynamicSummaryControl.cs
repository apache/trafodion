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
using System.Linq;
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Controls;



namespace Trafodion.Manager.OverviewArea.Controls
{
    public partial class DynamicSummaryControl : UserControl, IDataDisplayControl
    {
        private List<SystemMetricModel.SystemMetrics> _theDynamicSystemSummaryMetrics = null;

        private ConnectionDefinition _theConnectionDefinition = null;
        private DataProvider _theDataProvider = null;
        private UniversalWidgetConfig _theWidgetConfig = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        private bool _showMetrics = false;

        DynamicChartControl _theChart1 = null;
        DynamicChartControl _theChart2 = null;
        private List<SystemMetricModel.SystemMetrics> _theConfig1 = null;
        private List<SystemMetricModel.SystemMetrics> _theConfig2 = null;
        private Dictionary<string, DataTable> _theSnapshotTables = new Dictionary<string, DataTable>();
        private ToolStripButton _theResetLayoutButton = new ToolStripButton();
        private ToolStripButton _theLockLayoutButton = new ToolStripButton();
        private DynamicChartModel _theDynamicChartModel = null;
        private ContextMenu _contextMenu = null;

        public event EventHandler OnSelectedTseMetricChanged;

        private const int NULL_VERSION = -1;
        ConnectionDefinition.SERVER_VERSION _serverVersion = (ConnectionDefinition.SERVER_VERSION)NULL_VERSION;
        
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



      

        #endregion Properties


        #region Constructor

        /// <summary>
        /// The constructor
        /// </summary>
        public DynamicSummaryControl(ConnectionDefinition aConnectionDefinition, List<SystemMetricModel.SystemMetrics> aDynamicSummaryMetrics)
        {
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            _theDynamicSystemSummaryMetrics = aDynamicSummaryMetrics;
            _showMetrics = (aDynamicSummaryMetrics.Count > 0);
            _theDynamicChartModel = DynamicChartModel.FindSystemModel(aConnectionDefinition);

            if (string.IsNullOrEmpty(this.ConnectionDefn.PlatformReleaseVersion))
            {
                ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
            }
            else
            {
                this._serverVersion = this.ConnectionDefn.ServerVersion;
            }
        }

        #endregion Constructor

        public void ReConfigDynamicMetricControl(List<SystemMetricModel.SystemMetrics> aMetrics)
        {
            _theCanvas.Controls.Clear();
            CreateDynamicMetricsWidget(aMetrics);
            RecoverOldData(aMetrics);
        }

        public void RecoverOldData(List<SystemMetricModel.SystemMetrics> aMetrics)
        {
            for (int i = 0; i < aMetrics.Count; i++)
            {
                SystemMetricModel.SystemMetrics m = aMetrics[i];
                RecoverAMetricData(m);
            }

        }
        
        public void ResetTseSkew()
        {
            DynamicChartControl chartControl = null;
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
                chartControl.ResetTseSkew();
                RecoverAMetricData(SystemMetricModel.SystemMetrics.Tse);
            }
        }

        /// <summary>
        /// Persistent configruation - this is not used. But, for need to be here to fulfill the requirement of the interface
        /// </summary>
        public void PersistConfiguration()
        {
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


        private void CreateContextMenu()
        {
            _contextMenu = new ContextMenu();
            string selectedTseMetric = SystemMetricModel.SelectedTseMetric;
            foreach (string metric in SystemMetricModel.TseMetrics.Keys)
            {
                MenuItem tseMetricMenuItem = _contextMenu.MenuItems.Add(SystemMetricModel.TseMetrics[metric], TseMetric_MouseClick);
                tseMetricMenuItem.Tag = metric;
            }

            _contextMenu.Popup +=
               (sender, e) =>
               {
                   string currentSelectedTseMetric = SystemMetricModel.SelectedTseMetric;
                   foreach (MenuItem tseMenuItem in _contextMenu.MenuItems)
                   {
                       tseMenuItem.Checked = 0 == string.Compare( currentSelectedTseMetric, (string)tseMenuItem.Tag, true );
                   }
               };
        }

        private void DynamicSumControl_Load(object sender, EventArgs e)
        {
            CreateDynamicMetricsWidget(_theDynamicSystemSummaryMetrics);
            SetTseChartVisibility(this._serverVersion);
            RecoverOldData(_theDynamicSystemSummaryMetrics);
        }
        
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
                DynamicChartControl chartControl = null;
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
                        int metricCount = chartConfig.Count - 1;
                        float bottomLabelHeight = 2;
                        float metricHeight = (100 - bottomLabelHeight) / metricCount - 1.5f;
                        int i = 0;
                        foreach (SystemMetricModel.SystemMetrics metric in chartConfig)
                        {
                            string chartAreaName = SystemMetricModel.GetChartAreaName(metric);
                            ChartArea chartArea = chartControl.Chart.ChartAreas.FindByName(chartAreaName);
                            if (chartArea != null && 0 != string.Compare(tseChartAreaName, chartArea.Name, true))
                            {
                                chartArea.Position.Y = i * (100 / metricCount);
                                chartArea.Position.Height = (metric != chartConfig[chartConfig.Count - 1]) ? metricHeight : metricHeight + bottomLabelHeight;
                                i++;
                            }
                        }

                        tseChartArea.Visible = false;
                    }
                }
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


        void Chart_MouseClick(object sender, MouseEventArgs e)
        {
            if (this._serverVersion == (ConnectionDefinition.SERVER_VERSION)NULL_VERSION
                || this._serverVersion < ConnectionDefinition.SERVER_VERSION.SQ151)
                return;

            if (e.Button == MouseButtons.Right)
            {
                Chart targetChart = (Chart)sender;
                HitTestResult[] hitTestResults = targetChart.HitTest(e.Location.X, e.Location.Y, false, new ChartElementType[] { ChartElementType.PlottingArea, ChartElementType.DataPoint });
                if (hitTestResults.Length > 0)
                {
                    string tseChartAreaName = SystemMetricModel.GetChartAreaName(SystemMetricModel.SystemMetrics.Tse);
                    if ( 0 == string.Compare( tseChartAreaName, hitTestResults[0].ChartArea.Name, true))
                    {
                        this._contextMenu.Show(targetChart, e.Location);
                    }
                }
            }
        }

        /// <summary>
        /// To create system metric widgets
        /// </summary>
        /// <param name="aMetrics"></param>
        private void CreateDynamicMetricsWidget(List<SystemMetricModel.SystemMetrics> aMetrics)
        {
            GridLayoutManager gridLayoutManager = new GridLayoutManager(10, 6);
            gridLayoutManager.CellSpacing = 4;
            _theCanvas.LayoutManager = gridLayoutManager;

            if (aMetrics.Count==0) return;
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

                _theChart1 = new DynamicChartControl(ConnectionDefn, _theConfig1);
                _theChart2 = new DynamicChartControl(ConnectionDefn, _theConfig2);
                _theChart1.OnDynamicChartScaleViewChanging += ScaleAllChart;
                _theChart2.OnDynamicChartScaleViewChanging += ScaleAllChart;
                _theChart1.OnDynamicChartScaleViewChanged += _theChart_OnDynamicChartScaleViewChanged;
                _theChart2.OnDynamicChartScaleViewChanged += _theChart_OnDynamicChartScaleViewChanged;

                GridConstraint gridConstraint = new GridConstraint(0, 0, 10, 3);
                WidgetContainer widgetContainer = new WidgetContainer(_theCanvas, _theChart1, "MetricWidget_1");
                widgetContainer.Name = "MetricWidget_1";
                widgetContainer.Title = "";
                widgetContainer.AllowDelete = false;
                _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);

                gridConstraint = new GridConstraint(0, 3, 10, 3);
                widgetContainer = new WidgetContainer(_theCanvas, _theChart2, "MetricWidget_2");
                widgetContainer.Name = "MetricWidget_2";
                widgetContainer.Title = "";
                widgetContainer.AllowDelete = false;
                _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);

                _theChart1.Chart.GetToolTipText += new EventHandler<ToolTipEventArgs>(Chart_GetToolTipText);
                _theChart2.Chart.GetToolTipText += new EventHandler<ToolTipEventArgs>(Chart_GetToolTipText);
                _theChart1.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                _theChart2.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
            }
            else
            {
                // Just put everything in one chart
                _theChart1 = new DynamicChartControl(ConnectionDefn, aMetrics);

                GridConstraint gridConstraint = new GridConstraint(0, 0, 10, 6);
                WidgetContainer widgetContainer = new WidgetContainer(_theCanvas, _theChart1, "MetricWidget_1");
                widgetContainer.Name = "MetricWidget_1";
                widgetContainer.Title = "";
                widgetContainer.AllowDelete = false;
                _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);
                _theChart1.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                _theChart1.OnDynamicChartScaleViewChanging += ScaleAllChart;
                _theChart1.Chart.GetToolTipText += new EventHandler<ToolTipEventArgs>(Chart_GetToolTipText);
            }
            this._theCanvas.Dock = DockStyle.Fill;
            this._theCanvas.InitializeCanvas();

            CreateContextMenu();
        }

        private void _theChart_OnDynamicChartScaleViewChanged(string chartName)
        {
            if (_theChart1.Name.Equals(chartName))
            {
                
                _theChart2.Zoom(_theChart1.ZoomedPosition, _theChart1.ZoomedSize);
                _theChart2.SetLabel(_theChart1.ZoomedSize);
                _theChart1.SetLabel(_theChart1.ZoomedSize);
            }
            else if (_theChart2.Name.Equals(chartName))
            {
                _theChart1.Zoom(_theChart2.ZoomedPosition, _theChart2.ZoomedSize);
                _theChart2.SetLabel(_theChart2.ZoomedSize);
                _theChart1.SetLabel(_theChart2.ZoomedSize);
            }
        }

        private void ScaleAllChart(string chartName,string scaleChartAreaName)
        {
            if (_theChart1.Name.Equals(chartName))
            {
                _theChart1.AlignChartScaleView(scaleChartAreaName);
                _theChart2.AlignChartScaleView(_theChart2.Chart.ChartAreas[0].Name);
            }
            else if (_theChart2.Name.Equals(chartName))
            {
                _theChart2.AlignChartScaleView(scaleChartAreaName);
                _theChart1.AlignChartScaleView(_theChart1.Chart.ChartAreas[0].Name);
            }
            
            


        }

        private void Chart_GetToolTipText(object sender, ToolTipEventArgs e)
        {
            if (e.HitTestResult.ChartElementType == ChartElementType.DataPoint)
            {
                DataPoint point = e.HitTestResult.Series.Points[e.HitTestResult.PointIndex];
                e.Text = GetTooltipsByDateTime(e.HitTestResult.ChartArea.Name, DateTime.FromOADate(point.XValue));
            }
        }

        private DataRow FindADataRow(DataTable metricHistoryTable, DateTime dateTimeValue)
        {
            DataRow drReturn = null;
            foreach (DataRow dr in metricHistoryTable.Rows)
            {
                if (Math.Abs(((DateTime)dr[SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()] - dateTimeValue).TotalSeconds) < 2)
                {
                    drReturn = dr;
                    break;
                }
            }
            return drReturn;
        }

        private string FormatTooltips(SystemMetricModel.SystemMetrics metric, double max, double min, double avg, DateTime dtime)
        {
            string tooltips = string.Empty;
            switch (metric)
            {
                case SystemMetricModel.SystemMetrics.Core:
                case SystemMetricModel.SystemMetrics.Load_Avg:
                case SystemMetricModel.SystemMetrics.File_System:
                case SystemMetricModel.SystemMetrics.Memory:
                case SystemMetricModel.SystemMetrics.Swap:
                    {
                        tooltips = SystemMetricModel.GetOverallSummaryTitle(metric)
                            + ": \n Avg: " + avg.ToString("F2")
                            + "% \n Min: " + min.ToString("F2")
                            + "% \n Max: " + max.ToString("F2")
                            + "%\n " + dtime.ToString("yyyy-MM-dd HH:mm:ss");
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Tse:
                    {
                        tooltips = SystemMetricModel.GetOverallSummaryTitle(metric)
                                   + ": \n Avg {0}: " + avg.ToString("F")
                                   + "\n Min {0}: " + min.ToString("F")
                                   + "\n Max {0}: " + max.ToString("F")
                                   + "\n " + dtime.ToString("yyyy-MM-dd HH:mm:ss");

                        tooltips = string.Format(tooltips, SystemMetricModel.GetTseMetricText());
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Disk:
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                case SystemMetricModel.SystemMetrics.Network_Rcv:
                case SystemMetricModel.SystemMetrics.Network_Txn:
                    {
                        tooltips = SystemMetricModel.GetOverallSummaryTitle(metric)
                                   + ": \n Avg: " + avg.ToString("F")
                                   + "\n Min: " + min.ToString("F")
                                   + "\n Max: " + max.ToString("F")
                                   + "\n " + dtime.ToString("yyyy-MM-dd HH:mm:ss");
                    }
                    break;              
                default:
                    break;
            }
            return tooltips;
        }

        private string GetTooltipsByDateTime(String areaName,DateTime dateTimeValue)
        {
            DataTable dt = new DataTable();
            DataRow resultDataRow = null;
            string tooltips = string.Empty;
            if(areaName.Equals(SystemMetricModel.SystemMetrics.Core.ToString()))
            {                
                resultDataRow = FindADataRow(_theDynamicChartModel.CoreMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Core,
                        (double)resultDataRow[SystemMetricModel.CoreMetricDataTableColumns.max_core.ToString()],
                        (double)resultDataRow[SystemMetricModel.CoreMetricDataTableColumns.min_core.ToString()],
                        (double)resultDataRow[SystemMetricModel.CoreMetricDataTableColumns.avg_core.ToString()],
                        dateTimeValue);
                }
            }
            else if (areaName.Equals(SystemMetricModel.SystemMetrics.Memory.ToString()))
            {            
                    resultDataRow = FindADataRow(_theDynamicChartModel.MemoryMetricDynamicTable, dateTimeValue);
                    if (resultDataRow != null)
                    {
                        tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Memory,
                        (double)resultDataRow[SystemMetricModel.MemoryMetricDataTableColumns.max_memory_used.ToString()],
                        (double)resultDataRow[SystemMetricModel.MemoryMetricDataTableColumns.min_memory_used.ToString()],
                        (double)resultDataRow[SystemMetricModel.MemoryMetricDataTableColumns.avg_memory_used.ToString()],
                        dateTimeValue);
                    }
            }
            else if (areaName.Equals(SystemMetricModel.SystemMetrics.Swap.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.MemoryMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Swap,
                        (double)resultDataRow[SystemMetricModel.MemoryMetricDataTableColumns.max_swap_used.ToString()],
                        (double)resultDataRow[SystemMetricModel.MemoryMetricDataTableColumns.min_swap_used.ToString()],
                        (double)resultDataRow[SystemMetricModel.MemoryMetricDataTableColumns.avg_swap_used.ToString()],
                        dateTimeValue);
                }
            }
            else if (areaName.Equals(SystemMetricModel.SystemMetrics.File_System.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.FileSystemMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.File_System,
                        (double)resultDataRow[SystemMetricModel.FileSysMetricDataTableColumns.max_percent_consumed.ToString()],
                        (double)resultDataRow[SystemMetricModel.FileSysMetricDataTableColumns.min_percent_consumed.ToString()],
                        (double)resultDataRow[SystemMetricModel.FileSysMetricDataTableColumns.avg_percent_consumed.ToString()],
                        dateTimeValue);
                }
            }
             else if (areaName.Equals(SystemMetricModel.SystemMetrics.Load_Avg.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.LoadAvgMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Load_Avg,
                    (double)resultDataRow[SystemMetricModel.LoadAvgMetricDataTableColumns.max_one_min_avg.ToString()],
                    (double)resultDataRow[SystemMetricModel.LoadAvgMetricDataTableColumns.min_one_min_avg.ToString()],
                    (double)resultDataRow[SystemMetricModel.LoadAvgMetricDataTableColumns.avg_one_min_avg.ToString()],
                    dateTimeValue);
                }
            }
             else if (areaName.Equals(SystemMetricModel.SystemMetrics.Disk.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.DiskMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Disk,
                        (double)resultDataRow[SystemMetricModel.DiskMetricDataTableColumns.max_reads_and_writes.ToString()],
                        (double)resultDataRow[SystemMetricModel.DiskMetricDataTableColumns.min_reads_and_writes.ToString()],
                        (double)resultDataRow[SystemMetricModel.DiskMetricDataTableColumns.avg_reads_and_writes.ToString()],
                        dateTimeValue);
                }
            }
            else if (areaName.Equals(SystemMetricModel.SystemMetrics.Tse.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.TseMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    double avgTse;
                    double minTse;
                    double maxTse;
                    SystemMetricModel.GetTseMetricValue(resultDataRow, out avgTse, out minTse, out maxTse);
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Tse, maxTse, minTse, avgTse, dateTimeValue);
                }
            }
            else if (areaName.Equals(SystemMetricModel.SystemMetrics.Network_Rcv.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.NetworkMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Network_Rcv,
                        (double)resultDataRow[SystemMetricModel.NetworkMetricDataTableColumns.max_rcv_packets.ToString()],
                        (double)resultDataRow[SystemMetricModel.NetworkMetricDataTableColumns.min_rcv_packets.ToString()],
                        (double)resultDataRow[SystemMetricModel.NetworkMetricDataTableColumns.avg_rcv_packets.ToString()],
                        dateTimeValue);
                }
            }
              else if (areaName.Equals(SystemMetricModel.SystemMetrics.Network_Txn.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.NetworkMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Network_Txn,
                        (double)resultDataRow[SystemMetricModel.NetworkMetricDataTableColumns.max_txn_packets.ToString()],
                        (double)resultDataRow[SystemMetricModel.NetworkMetricDataTableColumns.min_txn_packets.ToString()],
                        (double)resultDataRow[SystemMetricModel.NetworkMetricDataTableColumns.avg_txn_packets.ToString()],
                        dateTimeValue);
                }
            }
            else if (areaName.Equals(SystemMetricModel.SystemMetrics.Virtual_Memory.ToString()))
            {
                resultDataRow = FindADataRow(_theDynamicChartModel.VMMetricDynamicTable, dateTimeValue);
                if (resultDataRow != null)
                {
                    tooltips = FormatTooltips(SystemMetricModel.SystemMetrics.Virtual_Memory,
                    (double)resultDataRow[SystemMetricModel.VirtualMemoryMetricDataTableColumns.max_context_switches.ToString()],
                    (double)resultDataRow[SystemMetricModel.VirtualMemoryMetricDataTableColumns.min_context_switches.ToString()],
                    (double)resultDataRow[SystemMetricModel.VirtualMemoryMetricDataTableColumns.avg_context_switches.ToString()],
                    dateTimeValue);
                }

            }
            
            return tooltips;
        }

        public void UpdateDynamicChart(SystemMetricModel.SystemMetrics aMetric, DateTime xDateTime, double yValueMin, double yValueMax, double yValueAvg)
        {

            DynamicChartControl chart;
            if (_theChart1.Metrics.Contains(aMetric))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(aMetric))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }
            chart.AddData(aMetric, xDateTime, yValueMin, yValueMax, yValueAvg);
            chart.Chart.Invalidate();
        }
               
        public void RecoverAMetricData(SystemMetricModel.SystemMetrics aMetric)
        {
            DynamicChartControl chart;
            if (_theChart1.Metrics.Contains(aMetric))
            {
                chart = _theChart1;
            }
            else if (_theChart2 != null && _theChart2.Metrics.Contains(aMetric))
            {
                chart = _theChart2;
            }
            else
            {
                return;
            }

            DataTable historicDataTable = FindSystemMetricDataTable(aMetric);
            chart.Chart.Invalidate();
            chart.RecoverData(aMetric, historicDataTable);
            chart.AlignChartArea();
            chart.Chart.Update();
        }

        private DataTable FindSystemMetricDataTable(SystemMetricModel.SystemMetrics aMetric)
        {
            switch (aMetric)
            {
                case SystemMetricModel.SystemMetrics.Core:
                    return _theDynamicChartModel.CoreMetricDynamicTable;                    
                case SystemMetricModel.SystemMetrics.Memory:
                case SystemMetricModel.SystemMetrics.Swap:
                    return _theDynamicChartModel.MemoryMetricDynamicTable;                    
                case SystemMetricModel.SystemMetrics.File_System:
                    return _theDynamicChartModel.FileSystemMetricDynamicTable;
                case SystemMetricModel.SystemMetrics.Load_Avg:
                    return _theDynamicChartModel.LoadAvgMetricDynamicTable;
                case SystemMetricModel.SystemMetrics.Disk:
                    return _theDynamicChartModel.DiskMetricDynamicTable;
                case SystemMetricModel.SystemMetrics.Tse:
                    return _theDynamicChartModel.TseMetricDynamicTable;
                case SystemMetricModel.SystemMetrics.Network_Rcv:
                case SystemMetricModel.SystemMetrics.Network_Txn:
                    return _theDynamicChartModel.NetworkMetricDynamicTable;
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                    return _theDynamicChartModel.VMMetricDynamicTable;
                default:
                    break;
            }
            return null;
        }


        public void StoreHistoricalDataTable(SystemMetricModel.SystemMetrics aMetric, DataTable table)
        {
            switch (aMetric)
            {
                case SystemMetricModel.SystemMetrics.Core:
                    SaveData(SystemMetricModel.SystemMetrics.Core, _theDynamicChartModel.CoreMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.Memory:
                case SystemMetricModel.SystemMetrics.Swap:
                    SaveData(SystemMetricModel.SystemMetrics.Memory, _theDynamicChartModel.MemoryMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.File_System:
                    SaveData(SystemMetricModel.SystemMetrics.File_System, _theDynamicChartModel.FileSystemMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.Load_Avg:
                    SaveData(SystemMetricModel.SystemMetrics.Load_Avg, _theDynamicChartModel.LoadAvgMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.Disk:
                    SaveData(SystemMetricModel.SystemMetrics.Disk, _theDynamicChartModel.DiskMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.Tse:
                    SaveData(SystemMetricModel.SystemMetrics.Tse, _theDynamicChartModel.TseMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.Network_Rcv:
                case SystemMetricModel.SystemMetrics.Network_Txn:
                    SaveData(SystemMetricModel.SystemMetrics.Network_Txn, _theDynamicChartModel.NetworkMetricDynamicTable, table);
                    break;
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                    SaveData(SystemMetricModel.SystemMetrics.Virtual_Memory, _theDynamicChartModel.VMMetricDynamicTable, table);
                    break;
                default:
                    break;
            }
        }

        private void SaveData(SystemMetricModel.SystemMetrics metric,DataTable totalTable, DataTable newTable)
        {
            DataRow dr = null;
            DateTime timestamp = (DateTime)newTable.Rows[0]["gen_time_ts_lct"];
            SystemMetricModel.totalNodes = (int)newTable.Rows[0]["total_nodes"];

            DateTime filterTime = timestamp.AddMinutes(-1 * SystemMetricModel.lengthMinutes);

            if (totalTable.Rows.Count > SystemMetricModel.MaximumDataCount && (DateTime)totalTable.Rows[0]["gen_time_ts_lct"] < filterTime)
            {
                DataRow[] rows = totalTable.Select("gen_time_ts_lct" + "<" + "#" + filterTime.AddSeconds(-0.1).ToString("M/dd/yy HH:mm:ss.fff") + "#");
                foreach (DataRow row in rows)
                {
                    totalTable.Rows.Remove(row);
                }
            }
            switch (metric)
            {
                case SystemMetricModel.SystemMetrics.Core:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.CoreMetricDataTableColumns.max_core.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.CoreMetricDataTableColumns.avg_core_total + ")", SystemMetricModel.CoreMetricDataTableColumns.avg_core_total+">=0");
                    dr[SystemMetricModel.CoreMetricDataTableColumns.min_core.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.CoreMetricDataTableColumns.avg_core_total + ")", SystemMetricModel.CoreMetricDataTableColumns.avg_core_total + ">=0");
                    dr[SystemMetricModel.CoreMetricDataTableColumns.avg_core.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.CoreMetricDataTableColumns.avg_core_total + ")", SystemMetricModel.CoreMetricDataTableColumns.avg_core_total + ">=0") / newTable.Rows.Count;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Core,
                        (DateTime)dr[SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.CoreMetricDataTableColumns.min_core.ToString()],
                        (double)dr[SystemMetricModel.CoreMetricDataTableColumns.max_core.ToString()],
                        (double)dr[SystemMetricModel.CoreMetricDataTableColumns.avg_core.ToString()]);
                    break;
                case SystemMetricModel.SystemMetrics.Memory:
                case SystemMetricModel.SystemMetrics.Swap:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.MemoryMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.max_memory_used.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.MemoryMetricDataTableColumns.memory_used + ")", SystemMetricModel.MemoryMetricDataTableColumns.memory_used + ">=0");
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.min_memory_used.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.MemoryMetricDataTableColumns.memory_used + ")", SystemMetricModel.MemoryMetricDataTableColumns.memory_used + ">=0");
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.avg_memory_used.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.MemoryMetricDataTableColumns.memory_used + ")", SystemMetricModel.MemoryMetricDataTableColumns.memory_used + ">=0") / SystemMetricModel.totalNodes;
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.max_swap_used.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.MemoryMetricDataTableColumns.swap_used + ")", SystemMetricModel.MemoryMetricDataTableColumns.swap_used + ">=0");
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.min_swap_used.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.MemoryMetricDataTableColumns.swap_used + ")", SystemMetricModel.MemoryMetricDataTableColumns.swap_used + ">=0");
                    dr[SystemMetricModel.MemoryMetricDataTableColumns.avg_swap_used.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.MemoryMetricDataTableColumns.swap_used + ")", SystemMetricModel.MemoryMetricDataTableColumns.swap_used + ">=0") / SystemMetricModel.totalNodes;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Memory,
                        (DateTime)dr[SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.min_memory_used.ToString()],
                        (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.max_memory_used.ToString()],
                        (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.avg_memory_used.ToString()]);

                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Swap,
                        (DateTime)dr[SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.min_swap_used.ToString()],
                        (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.max_swap_used.ToString()],
                        (double)dr[SystemMetricModel.MemoryMetricDataTableColumns.avg_swap_used.ToString()]);

                    break;
                case SystemMetricModel.SystemMetrics.File_System:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.FileSysMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.FileSysMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.FileSysMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.FileSysMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.FileSysMetricDataTableColumns.max_percent_consumed.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed + ")", SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed + ">=0");
                    dr[SystemMetricModel.FileSysMetricDataTableColumns.min_percent_consumed.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed + ")", SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed + ">=0");
                    dr[SystemMetricModel.FileSysMetricDataTableColumns.avg_percent_consumed.ToString()] = (double)newTable.Compute("Avg(" + SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed + ")", SystemMetricModel.FileSysMetricDataTableColumns.percent_consumed + ">=0") / SystemMetricModel.totalNodes;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.File_System,
                        (DateTime)dr[SystemMetricModel.FileSysMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.FileSysMetricDataTableColumns.min_percent_consumed.ToString()],
                        (double)dr[SystemMetricModel.FileSysMetricDataTableColumns.max_percent_consumed.ToString()],
                        (double)dr[SystemMetricModel.FileSysMetricDataTableColumns.avg_percent_consumed.ToString()]);

                    
                    break;
                case SystemMetricModel.SystemMetrics.Load_Avg:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.LoadAvgMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.LoadAvgMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.LoadAvgMetricDataTableColumns.max_one_min_avg.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg + ")", SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg + ">=0");
                    dr[SystemMetricModel.LoadAvgMetricDataTableColumns.min_one_min_avg.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg + ")", SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg + ">=0");
                    dr[SystemMetricModel.LoadAvgMetricDataTableColumns.avg_one_min_avg.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg + ")", SystemMetricModel.LoadAvgMetricDataTableColumns.one_min_avg + ">=0") / SystemMetricModel.totalNodes;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Load_Avg,
                        (DateTime)dr[SystemMetricModel.LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.LoadAvgMetricDataTableColumns.min_one_min_avg.ToString()],
                        (double)dr[SystemMetricModel.LoadAvgMetricDataTableColumns.max_one_min_avg.ToString()],
                        (double)dr[SystemMetricModel.LoadAvgMetricDataTableColumns.avg_one_min_avg.ToString()]);
                    break;
                case SystemMetricModel.SystemMetrics.Disk:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.DiskMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.DiskMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.DiskMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.DiskMetricDataTableColumns.max_reads_and_writes.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes + ")", SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes + ">=0");
                    dr[SystemMetricModel.DiskMetricDataTableColumns.min_reads_and_writes.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes + ")", SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes + ">=0");
                    dr[SystemMetricModel.DiskMetricDataTableColumns.avg_reads_and_writes.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes + ")", SystemMetricModel.DiskMetricDataTableColumns.reads_and_writes + ">=0") / SystemMetricModel.totalNodes;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Disk,
                        (DateTime)dr[SystemMetricModel.DiskMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.DiskMetricDataTableColumns.min_reads_and_writes.ToString()],
                        (double)dr[SystemMetricModel.DiskMetricDataTableColumns.max_reads_and_writes.ToString()],
                        (double)dr[SystemMetricModel.DiskMetricDataTableColumns.avg_reads_and_writes.ToString()]);
                    break;
                case SystemMetricModel.SystemMetrics.Tse:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.TseMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.TseMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.TseMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.TseMetricDataTableColumns.gen_time_ts_lct.ToString()];

                    dr[SystemMetricModel.TseMetricDataTableColumns.max_service_time.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.TseMetricDataTableColumns.service_time + ")", SystemMetricModel.TseMetricDataTableColumns.service_time + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.min_service_time.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.TseMetricDataTableColumns.service_time + ")", SystemMetricModel.TseMetricDataTableColumns.service_time + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.service_time.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.TseMetricDataTableColumns.service_time + ")", SystemMetricModel.TseMetricDataTableColumns.service_time + ">=0") / SystemMetricModel.totalNodes;

                    dr[SystemMetricModel.TseMetricDataTableColumns.max_requests.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.TseMetricDataTableColumns.requests + ")", SystemMetricModel.TseMetricDataTableColumns.requests + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.min_requests.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.TseMetricDataTableColumns.requests + ")", SystemMetricModel.TseMetricDataTableColumns.requests + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.requests.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.TseMetricDataTableColumns.requests + ")", SystemMetricModel.TseMetricDataTableColumns.requests + ">=0") / SystemMetricModel.totalNodes;

                    dr[SystemMetricModel.TseMetricDataTableColumns.max_ase_service_time.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.TseMetricDataTableColumns.ase_service_time + ")", SystemMetricModel.TseMetricDataTableColumns.ase_service_time + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.min_ase_service_time.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.TseMetricDataTableColumns.ase_service_time + ")", SystemMetricModel.TseMetricDataTableColumns.ase_service_time + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.ase_service_time.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.TseMetricDataTableColumns.ase_service_time + ")", SystemMetricModel.TseMetricDataTableColumns.ase_service_time + ">=0") / SystemMetricModel.totalNodes;

                    dr[SystemMetricModel.TseMetricDataTableColumns.max_request_io_wait_time.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time + ")", SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.min_request_io_wait_time.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time + ")", SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time + ")", SystemMetricModel.TseMetricDataTableColumns.request_io_wait_time + ">=0") / SystemMetricModel.totalNodes;

                    dr[SystemMetricModel.TseMetricDataTableColumns.max_ready_list_count.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.TseMetricDataTableColumns.ready_list_count + ")", SystemMetricModel.TseMetricDataTableColumns.ready_list_count + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.min_ready_list_count.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.TseMetricDataTableColumns.ready_list_count + ")", SystemMetricModel.TseMetricDataTableColumns.ready_list_count + ">=0");
                    dr[SystemMetricModel.TseMetricDataTableColumns.ready_list_count.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.TseMetricDataTableColumns.ready_list_count + ")", SystemMetricModel.TseMetricDataTableColumns.ready_list_count + ">=0") / SystemMetricModel.totalNodes;
                    totalTable.Rows.Add(dr);

                    double avgTse;
                    double minTse;
                    double maxTse;
                    SystemMetricModel.GetTseMetricValue(dr, out avgTse, out minTse, out maxTse);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Tse,
                        (DateTime)dr[SystemMetricModel.TseMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        minTse,
                        maxTse,
                        avgTse);
                    break;
                case SystemMetricModel.SystemMetrics.Network_Rcv:
                case SystemMetricModel.SystemMetrics.Network_Txn:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.NetworkMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.NetworkMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.NetworkMetricDataTableColumns.max_txn_packets.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ")", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ">=0");
                    dr[SystemMetricModel.NetworkMetricDataTableColumns.max_rcv_packets.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.NetworkMetricDataTableColumns.rcv_packets + ")", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ">=0");

                    dr[SystemMetricModel.NetworkMetricDataTableColumns.min_txn_packets.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ")", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ">=0");
                    dr[SystemMetricModel.NetworkMetricDataTableColumns.min_rcv_packets.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.NetworkMetricDataTableColumns.rcv_packets + ")", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ">=0");

                    dr[SystemMetricModel.NetworkMetricDataTableColumns.avg_txn_packets.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ")", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ">=0") / newTable.Rows.Count;
                    dr[SystemMetricModel.NetworkMetricDataTableColumns.avg_rcv_packets.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.NetworkMetricDataTableColumns.rcv_packets + ")", SystemMetricModel.NetworkMetricDataTableColumns.txn_packets + ">=0") / newTable.Rows.Count;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Network_Rcv,
                        (DateTime)dr[SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.NetworkMetricDataTableColumns.min_rcv_packets.ToString()],
                        (double)dr[SystemMetricModel.NetworkMetricDataTableColumns.max_rcv_packets.ToString()],
                        (double)dr[SystemMetricModel.NetworkMetricDataTableColumns.avg_rcv_packets.ToString()]);

                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Network_Txn,
                        (DateTime)dr[SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()],
                        (double)dr[SystemMetricModel.NetworkMetricDataTableColumns.min_txn_packets.ToString()],
                        (double)dr[SystemMetricModel.NetworkMetricDataTableColumns.max_txn_packets.ToString()],
                        (double)dr[SystemMetricModel.NetworkMetricDataTableColumns.avg_txn_packets.ToString()]);
                    break;
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                    dr = totalTable.NewRow();
                    dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.node_id.ToString()] = newTable.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.node_id.ToString()];
                    dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()] = newTable.Rows[0][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()];
                    dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.max_context_switches.ToString()] = (double)newTable.Compute("MAX(" + SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches + ")", SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches + ">=0");
                    dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.min_context_switches.ToString()] = (double)newTable.Compute("MIN(" + SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches + ")", SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches + ">=0");
                    dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.avg_context_switches.ToString()] = (double)newTable.Compute("Sum(" + SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches + ")", SystemMetricModel.VirtualMemoryMetricDataTableColumns.context_switches + ">=0") / SystemMetricModel.totalNodes;
                    totalTable.Rows.Add(dr);
                    UpdateDynamicChart(SystemMetricModel.SystemMetrics.Virtual_Memory,
                     (DateTime)dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()],
                     (double)dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.min_context_switches.ToString()],
                     (double)dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.max_context_switches.ToString()],
                     (double)dr[SystemMetricModel.VirtualMemoryMetricDataTableColumns.avg_context_switches.ToString()]);
                    break;
                default:
                    break;
            }
          
        }

        private void MergeAndRemoveData(DataTable totalTable, DataTable newTable)
        {

            DateTime timestamp = (DateTime)newTable.Rows[0]["gen_time_ts_lct"];
            DateTime filterTime = timestamp.AddMinutes(-1 * SystemMetricModel.lengthMinutes);

            if (totalTable.Rows.Count > 0 && (DateTime)totalTable.Rows[0]["gen_time_ts_lct"] < filterTime)
            {
                DataRow[] rows = totalTable.Select("gen_time_ts_lct" + "<" + "#" + filterTime.AddSeconds(-0.1).ToString("M/dd/yy HH:mm:ss.fff") + "#");
                foreach (DataRow row in rows)
                {
                    totalTable.Rows.Remove(row);
                }
            }
            string[] selectedColumns = (from dc in totalTable.Columns.Cast<DataColumn>() select dc.ColumnName).ToArray();

            List<string> alist = selectedColumns.ToList();
            if (alist.Contains("gen_time_ts_lct"))
            {
                alist.Remove("gen_time_ts_lct");
            }
            selectedColumns = alist.ToArray();

            string columns = string.Join(",", selectedColumns);
            DataTable selectedColumnsDT = new DataView(newTable).ToTable(false, selectedColumns);
            DataTable groupTable = GetGroupedBy(selectedColumnsDT, columns, "node_id", "Avg");

            DataColumn col = new DataColumn("gen_time_ts_lct", typeof(DateTime));
            col.DefaultValue = timestamp;

            groupTable.Columns.Add(col);
            groupTable.Columns["gen_time_ts_lct"].SetOrdinal(1);

            totalTable.Merge(groupTable);
            selectedColumnsDT.Clear();
            groupTable.Clear();
            newTable.Clear();
        }

        /// <summary>
        /// The group by clause for Data Table
        /// </summary>
        /// <param name="dt">The main data table to be filtered with the Group By.</param>
        /// <param name="columnNamesInDt">The Fields sent seperated by commas. EG "Field1,Field2,Field3"</param>
        /// <param name="groupByColumnNames">The column name which should be used for group by.</param>
        /// <param name="typeOfCalculation">The calculation type like Sum, Avg Or Count.</param>
        /// <returns></returns>
        private DataTable GetGroupedBy(DataTable dt, string columnNamesInDt, string groupByColumnNames, string typeOfCalculation)
        {
            //Return its own if the column names are empty
            if (columnNamesInDt == string.Empty || groupByColumnNames == string.Empty)
            {
                return dt;
            }

            //Once the columns are added find the distinct rows and group it bu the numbet
            DataTable _dt = dt.DefaultView.ToTable(true, groupByColumnNames);
            //The column names in data table
            string[] _columnNamesInDt = columnNamesInDt.Split(',');

            for (int i = 0; i < _columnNamesInDt.Length; i = i + 1)
            {
                if (_columnNamesInDt[i] != groupByColumnNames)
                {
                    _dt.Columns.Add(_columnNamesInDt[i], dt.Columns[_columnNamesInDt[i]].DataType);
                }
            }

            //Gets the collection and send it back
            for (int i = 0; i < _dt.Rows.Count; i = i + 1)
            {
                for (int j = 0; j < _columnNamesInDt.Length; j = j + 1)
                {
                    if (_columnNamesInDt[j] != groupByColumnNames)
                    {
                        _dt.Rows[i][j] = dt.Compute(typeOfCalculation + "(" + _columnNamesInDt[j] + ")"
                            , groupByColumnNames + " = '" + _dt.Rows[i][groupByColumnNames].ToString() + "'");
                    }
                }
            }
            return _dt;
        }
        
        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                ConnectionDefinition.Changed -= new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);

                if (_theChart1 != null)
                {
                    _theChart1.MouseClick -= Chart_MouseClick;
                    _theChart1.Dispose();
                }
                if (_theChart2 != null)
                {
                    _theChart2.MouseDoubleClick -= Chart_MouseClick;
                    _theChart2.Dispose();
                }
               
            }
        }


        private void _theResetLayoutButton_Click(object sender, EventArgs e)
        {
            
            this.ResetLayout();
            _theLockLayoutButton.Text = "Lock Layout";
            _theLockLayoutButton.Image = Trafodion.Manager.Properties.Resources.Locked;
        }

        private void _theLockLayoutButton_Click(object sender, EventArgs e)
        {
            string buttonText = this.LockLayout();
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
    }
}
