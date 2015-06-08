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
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{

    public partial class SnapshotMetricControl : UserControl, IDataDisplayControl
    {
        #region Fields

        public delegate void OnShowOffenderClick(ShowOffenderEventArgs args);
        public event OnShowOffenderClick OnShowOffenderClickImpl;

        public const string OverallSystemMetricTooltip = " Node: #VALX\n{0}: #VALY\nat {1}";

        private SystemMetricChartControl _theChart1 = null;
        private SystemMetricChartControl _theChart2 = null;
        private List<SystemMetricModel.SystemMetrics> _theConfig1 = null;
        private List<SystemMetricModel.SystemMetrics> _theConfig2 = null;
        SystemHealthStatesUserControl _theHealthStates = null;

        private System.Drawing.Size _theCurrentSize;
        public delegate void NewDataArrivalHandler(object sender, NewDataEventArgs e);
        public event NewDataArrivalHandler OnNewDataArrival; 


        private ConnectionDefinition _theConnectionDefinition = null;
        private DataProvider _theDataProvider = null;
        private UniversalWidgetConfig _theWidgetConfig = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        private List<SystemMetricModel.SystemMetrics> _theSnapMetrics = null;
        private SystemMetricModel.SystemMetricDisplays _theMetricDisplay;
        private String _metricName=string.Empty;
        private int _metricNodeID = -1;
        private ContextMenu _theWidgetContextMenu = null;
        private bool _showMetrics = false;
        private Hashtable _theMetricsUpdated = new Hashtable();
        Dictionary<string, DataTable> _theSnapshotTables = null;
        private const string _theStatesAvailableTooltip = "Click for details.";
        private const string OverallSummaryControlCanvasPersistenceKey = "OverallSummaryCanvasPersistenceKey_v1";

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



  

        #endregion Properties

        #region Constructor

        /// <summary>
        /// The constructor
        /// </summary>
        public SnapshotMetricControl(ConnectionDefinition aConnectionDefinition, List<SystemMetricModel.SystemMetrics> aMetrics, Dictionary<string, DataTable> aSnapshotTables)
        {
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            _theSnapMetrics = aMetrics;
            _theSnapshotTables = aSnapshotTables;
            _showMetrics = (_theSnapMetrics.Count > 0);

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
            //SystemMetricChartControl[] chartControls = new SystemMetricChartControl[] { this._theChart1, this._theChart2 };
            //foreach (SystemMetricChartControl chartControl in chartControls)
            //{
            //    if (chartControl != null)
            //    {
            //        chartControl.ClearChartSeries(systemMetric);
            //    }
            //}
        }


        #endregion Public methods

        #region Private methods


        /// <summary>
        /// To load the initial system metrics
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void SnapshotMetricControl_Load(object sender, EventArgs e)
        {
            CreateSystemMetricsWidget(_theSnapMetrics);
            UpdateMetricData();
        }

        private void UpdateMetricData()
        {
            if (_theSnapshotTables == null && _theSnapshotTables.Count==0) return;
            DataTable table = null;
            for (int i = 0; i < _theSnapMetrics.Count; i++)
            {
                table = new DataTable();
                SystemMetricModel.SystemMetrics m = _theSnapMetrics[i];
                switch (m)
                {
                    case SystemMetricModel.SystemMetrics.Core:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.Core.ToString()];
                        UpdateCoreBusy(table);
                        break;
                    case SystemMetricModel.SystemMetrics.Memory:
                    case SystemMetricModel.SystemMetrics.Swap:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.Memory.ToString()];
                        break;
                    case SystemMetricModel.SystemMetrics.File_System:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.File_System.ToString()];
                        break;
                    case SystemMetricModel.SystemMetrics.Disk:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.Disk.ToString()];
                        break;
                    case SystemMetricModel.SystemMetrics.Load_Avg:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.Load_Avg.ToString()];
                        break;
                    case SystemMetricModel.SystemMetrics.Network_Rcv:
                    case SystemMetricModel.SystemMetrics.Network_Txn:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.Network_Rcv.ToString()];
                        break;
                    case SystemMetricModel.SystemMetrics.Virtual_Memory:
                        table = _theSnapshotTables[SystemMetricModel.SystemMetrics.Virtual_Memory.ToString()];
                        break;
                }

            }
        }


        /// <summary>
        /// To update core busy chart with the supplied data
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateCoreBusy(DataTable aDataTable)
        {
            if ( aDataTable == null || aDataTable.Rows.Count <= 0)
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
            DataView dv0 = new DataView(aDataTable);
            chart.Chart.Invalidate();

            // Series Busy
            string coreSeriesName = SystemMetricChartControl.GetChartSeriesName(SystemMetricModel.SystemMetrics.Core.ToString());
            chart.Chart.Series[coreSeriesName].Points.DataBindXY(dv0, Trafodion.Manager.OverviewArea.Models.SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString(),
                dv0, Trafodion.Manager.OverviewArea.Models.SystemMetricModel.CoreMetricDataTableColumns.avg_core_total.ToString());
            chart.Chart.Series[coreSeriesName].ToolTip =
                string.Format(OverallSystemMetricTooltip, SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Core), Utilities.StandardizeDateTime(currentTS));


            UpdateChartArea(chart, SystemMetricModel.SystemMetrics.Core, dv0);
            chart.AllignCharts();
            chart.Chart.Update();
        }

        /// <summary>
        /// By design, the node ID is only updated in the beginning. 
        /// </summary>
        /// <param name="chart"></param>
        /// <param name="metric"></param>
        /// <param name="dv0"></param>
        private void UpdateChartArea(SystemMetricChartControl chart, SystemMetricModel.SystemMetrics metric, DataView dv0)
        {
            // Change back color to indicate the chart area is active
            string seriesName = SystemMetricChartControl.GetChartSeriesName(metric.ToString());
            Series updatedSeries = chart.Chart.Series.FindByName(seriesName);
            if (updatedSeries != null)
            {
                Color configBackColor = SystemMetricChartConfigModel.Instance.GetSystemMetricBackColor(ConnectionDefn.Name, metric);
                chart.ChartAreas[updatedSeries.ChartArea].BackColor = configBackColor;
            }
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

            if (true)
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
                            case SystemMetricModel.SystemMetrics.Load_Avg:
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

                    _theChart1.Chart.MouseDoubleClick += ChartMouseDoubleClick;
                    _theChart2.Chart.MouseDoubleClick += ChartMouseDoubleClick;
                    _theChart1.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                    _theChart2.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                }
                else
                {
                    // Just put everything in one chart
                    _theChart1 = new SystemMetricChartControl(ConnectionDefn, aMetrics);

                    GridConstraint gridConstraint =  new GridConstraint(0, 0, 10, 6);
                    WidgetContainer widgetContainer = new WidgetContainer(_theCanvas, _theChart1, "MetricWidget_1");
                    widgetContainer.Name = "MetricWidget_1";
                    widgetContainer.Title = "";
                    widgetContainer.AllowDelete = false;
                    _theCanvas.AddWidget(widgetContainer, gridConstraint, -1);
                    _theChart1.Chart.MouseClick += new MouseEventHandler(Chart_MouseClick);
                    _theChart1.Chart.MouseDoubleClick += ChartMouseDoubleClick;
                }

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

                SystemMetricModel.SystemMetrics metric = (SystemMetricModel.SystemMetrics)Enum.Parse(typeof(SystemMetricModel.SystemMetrics), _metricName);
                switch (metric)
                {
                    case SystemMetricModel.SystemMetrics.Core:
                    case SystemMetricModel.SystemMetrics.Memory:
                    case SystemMetricModel.SystemMetrics.Swap:
                        _theWidgetContextMenu.MenuItems[0].Visible = true;
                        _theWidgetContextMenu.MenuItems[0].Enabled = (ConnectionDefn.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded);
                        _theWidgetContextMenu.MenuItems[1].Visible = true;
                        break;                            

                    default:
                        _theWidgetContextMenu.MenuItems[0].Visible = false;
                        _theWidgetContextMenu.MenuItems[1].Visible = true;
                        break;
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
             

                // this._theCanvas.SaveToPersistence();
            }
        }

        #endregion Private methods
    }

   
}
