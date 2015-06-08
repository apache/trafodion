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
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.OverviewArea.Models;
using System.Collections;
using System.Collections.Generic;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// User control which display system metric in charts
    /// </summary>
    public partial class SystemMetricChartControl : UserControl
    {

        #region Fields

        private const int NULL_VERSION = -1;
        private Hashtable _theThresholdColor = new Hashtable();
        private List<SystemMetricModel.SystemMetrics> _theMetrics = null;
        private ConnectionDefinition _theConnectionDefinition = null;
        ConnectionDefinition.SERVER_VERSION _serverVersion = (ConnectionDefinition.SERVER_VERSION)NULL_VERSION;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ConnectonDefn - the connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }

        /// <summary>
        /// Property: Chart - the MS Chart itself
        /// </summary>
        public Chart Chart
        {
            get { return _theChart; }
        }

        /// <summary>
        /// Property: ChartAreas - the MS Chart ChartAreas
        /// </summary>
        public ChartAreaCollection ChartAreas
        {
            get { return _theChart.ChartAreas; }
        }

        /// <summary>
        /// Property: Metrics - the list of metrics in the chart
        /// </summary>
        public List<SystemMetricModel.SystemMetrics> Metrics
        {
            get { return _theMetrics; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor 
        /// </summary>
        /// <param name="aSystemMetricChartConfig"></param>
        /// <param name="aDataTable"></param>
        public SystemMetricChartControl(ConnectionDefinition aConnectionDefinition, List<SystemMetricModel.SystemMetrics> aMetrics)
        {
            _theMetrics = aMetrics;
            _theConnectionDefinition = aConnectionDefinition;

            if (string.IsNullOrEmpty(this.ConnectionDefn.PlatformReleaseVersion))
            {
                ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
            }
            else
            {
                this._serverVersion = this.ConnectionDefn.ServerVersion;
                this.Load += new EventHandler((sender, args) => { ConnectionDefinition_Changed(null, this.ConnectionDefn, ConnectionDefinition.Reason.PlatformReleaseVersion); });                
            }

            InitializeComponent();
            CreateChartAreas();
        }

        ~SystemMetricChartControl()
        {
            ConnectionDefinition.Changed -= new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);
        }

        #endregion Constructors

        #region Public methods

        public void AllignCharts()
        {
            for (int i = 1; i < _theChart.ChartAreas.Count; i++)
            {
                _theChart.ChartAreas[i].AlignWithChartArea = _theChart.ChartAreas[0].Name;
            }
        }

        public static string GetChartSeriesName(string systemMetric)
        {
            return string.Format("Series_{0}", systemMetric);
        }
        
        public void ClearChartSeries(string systemMetric)
        {
            try // Sometimes accessing Series property of Chart will throw exception.
            {
                if (this._theChart == null || this._theChart.Series == null || this._theChart.Series.Count == 0) return;

                string chartAreaName = SystemMetricModel.GetChartAreaName(systemMetric);
                string nodeIdSeriesName = SystemMetricModel.GetChartNodeIdSeriesName();
                foreach (Series series in this._theChart.Series)
                {
                    if ( 0 != string.Compare( nodeIdSeriesName, series.Name, true)
                        && 0 == string.Compare( chartAreaName, series.ChartArea, true))
                    {
                        series.Points.Clear();
                        this._theChart.ChartAreas[series.ChartArea].BackColor = Color.Gray;
                    }
                }
            }
            catch{}
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Create chart areas according to the chart config
        /// </summary>
        /// <param name="aDataTable"></param>
        private void CreateChartAreas()
        {
            string lastChartAreaName = "";
            ChartArea chartArea;

            //Clean up first. 
            _theChart.ChartAreas.Clear();
            _theChart.Titles.Clear();
            _theChart.Legends.Clear();
            _theChart.Series.Clear();
            _theChart.Annotations.Clear();
            _theChart.GetToolTipText += new EventHandler<ToolTipEventArgs>(GetToolTipText);

            int metricCount = _theMetrics.Count;
            //Create a chart area for each of the system metrics in the chart config
            for (int i = 0; i < metricCount; i++)
            {
                SystemMetricModel.SystemMetrics metric = _theMetrics[i];
                Color metricBackColor = SystemMetricChartConfigModel.Instance.GetSystemMetricBackColor(ConnectionDefn.Name, metric);
                Color metricColor = SystemMetricChartConfigModel.Instance.GetSystemMetricColor(ConnectionDefn.Name, metric);
                Color metricGridLineColor = SystemMetricChartConfigModel.Instance.GetSystemMetricGridLineColor(ConnectionDefn.Name, metric);
                Color metricCursorColor = SystemMetricChartConfigModel.Instance.GetSystemMetricCursorColor(ConnectionDefn.Name, metric);
                Color metricThresholdColor = SystemMetricChartConfigModel.Instance.GetSystemMetricThresholdColor(ConnectionDefn.Name, metric);
                int metricThreshold = SystemMetricChartConfigModel.Instance.GetSystemMetricThreshold(ConnectionDefn.Name, metric);
                chartArea = CreateAChartArea(metric, metricCount, metricBackColor, metricColor, metricGridLineColor, metricCursorColor, metricThresholdColor, metricThreshold);

                //Alignments
                if (!string.IsNullOrEmpty(lastChartAreaName))
                {
                    chartArea.AlignWithChartArea = lastChartAreaName;
                    chartArea.AlignmentOrientation = AreaAlignmentOrientations.Vertical;
                    chartArea.AlignmentStyle = AreaAlignmentStyles.All;
                }

                lastChartAreaName = chartArea.Name;

                //Set the height and Width of each chearArea, the value is percent.
                //theChartArea.Position.Width=90 means it will take 90% of all width
                chartArea.Position.X = 0;
                chartArea.Position.Y = 4 + i * (100 / metricCount - 1);
                chartArea.Position.Width = 92;
                chartArea.Position.Height = (96 / metricCount - 1) - 1;
                _theChart.ChartAreas.Add(chartArea);
            }
            

            //Create a chart area to display the Node ID
            chartArea = new ChartArea();
            chartArea.Name = string.Format("ChartArea_{0}", "NodeID");
            chartArea.BackColor = SystemMetricChartConfigModel.DefaultMetricBackColor;
            chartArea.BorderDashStyle = ChartDashStyle.Solid;
            chartArea.AxisX.Interval = 1;
            chartArea.AxisX.IsLabelAutoFit = true;
            chartArea.AxisX.MajorGrid.LineColor = SystemMetricChartConfigModel.DefaultMetricGridLineColor;
            chartArea.AxisX.MajorGrid.Interval = 1;
            chartArea.AxisX.MajorGrid.LineDashStyle = ChartDashStyle.Dot;
            chartArea.AxisX.MajorTickMark.TickMarkStyle = TickMarkStyle.None;
            chartArea.Position.X = 0;
            chartArea.Position.Y = 0;
            chartArea.Position.Width = 95;
            chartArea.Position.Height = 4;
            chartArea.InnerPlotPosition.Height = 1;
            chartArea.InnerPlotPosition.Width = 100;

            if (!string.IsNullOrEmpty(lastChartAreaName))
            {
                chartArea.AlignWithChartArea = lastChartAreaName;
                chartArea.AlignmentOrientation = AreaAlignmentOrientations.Vertical;
                chartArea.AlignmentStyle = AreaAlignmentStyles.All;
            }

            Series series = new Series();
            series.Name = string.Format("Series_{0}", "NodeID");
            series.BorderColor = Color.Transparent;
            series.Color = SystemMetricChartConfigModel.DefaultMetricColor; // Color.LimeGreen;
            series.ChartArea = chartArea.Name;
            series.YValueMembers = SystemMetricModel.SystemMetricZero;
            series.YValueType = ChartValueType.Double;
            series.XValueMember = "NodeID";

            _theChart.Series.Add(series);
            _theChart.ChartAreas.Insert(0, chartArea);
            
            //Set the width of databar, so it won't look too wide when connecting single node
            SetBarWidth(_theChart, 60);
            _theChart.PrePaint += new EventHandler<ChartPaintEventArgs>(_theChart_PrePaint);            
            _theChart.CursorPositionChanging+=new EventHandler<CursorEventArgs>(_theChart_CursorPositionChanging);
        }

        void GetToolTipText(object sender, ToolTipEventArgs e)
        {
            // Merge two tool tips into one (Node Busy AVG & Node Busy Max)
            if (e.HitTestResult.ChartElementType == ChartElementType.DataPoint)
            {
                if (this._serverVersion != (ConnectionDefinition.SERVER_VERSION)NULL_VERSION)
                {
                    string chartAreaName = e.HitTestResult.ChartArea.Name;
                    Chart chart = (Chart)sender;
                    if (this._serverVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        SetToolTip(e, chart.Series, chartAreaName,
                            SystemMetricModel.SystemMetrics.Core, SystemMetricModel.SystemMetricCoreBusyMin, SystemMetricModel.SystemMetricCoreBusyMax,
                            SystemMetricModel.SystemMetricToolTipMergedCoreBusyAvgMinMax);
                    }

                    if (this._serverVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
                    {
                        SetToolTip(e, chart.Series, chartAreaName, 
                            SystemMetricModel.SystemMetrics.Disk, SystemMetricModel.SystemMetricDiskIOMin, SystemMetricModel.SystemMetricDiskIOMax,
                            SystemMetricModel.SystemMetricToolTipMergedDiskIOAvgMinMax);

                        SetToolTip(e, chart.Series, chartAreaName,
                            SystemMetricModel.SystemMetrics.Tse, SystemMetricModel.SystemMetricTseMin, SystemMetricModel.SystemMetricTseMax,
                            SystemMetricModel.SystemMetricToolTipMergedTseAvgMinMax);
                    }
                }
            }
        }

        private void SetToolTip(ToolTipEventArgs e, SeriesCollection series, 
            string chartAreaName, SystemMetricModel.SystemMetrics targetMetric, string minSeriesKey, string maxSeriesKey, 
            string toolTipFormat)
        {
            string targetAreaName = SystemMetricModel.GetChartAreaName(targetMetric);
            if (0 == string.Compare(chartAreaName, targetAreaName, true))
            {
                Series avgSeries = series.FindByName(GetChartSeriesName(targetMetric.ToString()));
                Series minSeries = series.FindByName(GetChartSeriesName(minSeriesKey));
                Series maxSeries = series.FindByName(GetChartSeriesName(maxSeriesKey));
                if (avgSeries != null && minSeries != null && maxSeries != null)
                {
                    if (e.HitTestResult.PointIndex >= avgSeries.Points.Count || e.HitTestResult.PointIndex < 0) return;

                    DataPoint avgDataPoint = avgSeries.Points[e.HitTestResult.PointIndex];
                    DataPoint minDataPoint = minSeries.Points[e.HitTestResult.PointIndex];
                    DataPoint maxDataPoint = maxSeries.Points[e.HitTestResult.PointIndex];
                    e.Text = string.Format(toolTipFormat, e.HitTestResult.PointIndex,
                        string.Format("{0:0.00}", avgDataPoint.YValues[0]),
                        string.Format("{0:0.00}", minDataPoint.YValues[0]),
                        string.Format("{0:0.00}", maxDataPoint.YValues[0]),
                        ExtractTimeStamp(avgDataPoint.ToolTip),
                        SystemMetricModel.GetTseMetricText());
                }
            }
        }

        private string ExtractTimeStamp(string tooltip)
        {
            string timeStamp = string.Empty;
            string[] tooltipLines = tooltip.Split(new string[] { "\n" }, StringSplitOptions.None);
            if (tooltipLines.Length > 0)
            {
                timeStamp = tooltipLines[tooltipLines.Length - 1];
            }

            return timeStamp;
        }

        private void _theChart_CursorPositionChanging(object sender, CursorEventArgs e)
        {
            if (e.Axis.AxisName == AxisName.Y) 
            {
                string metricName=e.ChartArea.Name;
                SystemMetricModel.SystemMetrics m=(SystemMetricModel.SystemMetrics)Enum.Parse(typeof(SystemMetricModel.SystemMetrics), metricName);
                int thresholdValue = 0;
                try
                {
                    thresholdValue=Convert.ToInt32(e.NewPosition);
                }
                catch(Exception ex)
                {

                }
                    
                DataTable configDataTable=SystemMetricChartConfigModel.Instance.GetSystemMetricColorTable(this.ConnectionDefn.Name);
                SystemMetricChartConfigModel.Instance.UpdateSystemMetricThreshold(configDataTable, m, thresholdValue);

            }
        }
        
        private void SetBarWidth(Chart aChart, int width) 
        {
            foreach (Series s in aChart.Series) 
            {
                //This property is to set the width of databar, also there is MinPixelPointWidth, and 
                //PixelPointWidth to set an exact value
                s["MaxPixelPointWidth"] = width.ToString();
            }
        }

        private ChartArea CreateAChartArea(SystemMetricModel.SystemMetrics metric, int totalMetrics, 
                                     Color metricBackColor, Color metricColor, Color metricGridLineColor, 
                                     Color metricCursorColor, Color metricThresholdColor, int metricThreshold)
        {
            ChartArea chartArea = new ChartArea();
            chartArea.Name = SystemMetricModel.GetChartAreaName(metric);
            chartArea.BackColor = metricBackColor; //Color.Black;
            chartArea.BorderDashStyle = ChartDashStyle.Solid;
            chartArea.AxisX.Interval = 1;
            chartArea.AxisX.IsLabelAutoFit = true;
            chartArea.AxisX.CustomLabels.Add(new CustomLabel());
            chartArea.AxisX.MajorGrid.LineColor = metricGridLineColor; //Color.YellowGreen;
            chartArea.AxisX.MajorGrid.Interval = 1;
            chartArea.AxisX.MajorGrid.LineDashStyle = ChartDashStyle.Dot;
            chartArea.AxisX.MajorTickMark.TickMarkStyle = TickMarkStyle.None;
            chartArea.AxisX2.MajorTickMark.Enabled = false;
            chartArea.AxisX2.MinorTickMark.Enabled = false;

            chartArea.AxisY.MajorGrid.Enabled = true;
            chartArea.AxisY.MajorGrid.LineColor = metricGridLineColor; //Color.YellowGreen;
            chartArea.AxisY.MajorGrid.LineDashStyle = ChartDashStyle.Dot;
            chartArea.AxisY.MajorGrid.LineWidth = 1;
            chartArea.AxisY.MajorTickMark.Enabled = false;
            chartArea.AxisY2.MajorTickMark.Enabled = false;
            chartArea.AxisY2.MinorTickMark.Enabled = false;

            chartArea.AxisY.IsLabelAutoFit = true;
            chartArea.AxisY.CustomLabels.Add(new CustomLabel());
            if (metric == SystemMetricModel.SystemMetrics.Core ||
                metric == SystemMetricModel.SystemMetrics.Memory ||
                metric == SystemMetricModel.SystemMetrics.Swap ||
                metric == SystemMetricModel.SystemMetrics.File_System)
            {
                chartArea.AxisY.Minimum = 0D;
                chartArea.AxisY.Maximum = 100D;
            }
            else
            {
                chartArea.AxisY.Minimum = Double.NaN;
                chartArea.AxisY.Maximum = Double.NaN;
            }

            chartArea.CursorY.LineColor = metricCursorColor; //Color.RoyalBlue;
            chartArea.CursorY.LineWidth = 2;

            //Using Cursor Value as Threshold
            if(metricThreshold!=0)
            {                
                chartArea.CursorY.Position = metricThreshold;
            }

            if (metric == SystemMetricModel.SystemMetrics.Core)
            {
                CreateSerie(chartArea, SystemMetricModel.SystemMetricCoreBusyMax, metric, metricColor);
                CreateSerie(chartArea, metric.ToString(), metric, metricColor);
                CreateSerie(chartArea, SystemMetricModel.SystemMetricCoreBusyMin, metric, metricColor);
            }
            else if (metric == SystemMetricModel.SystemMetrics.Disk)
            {
                CreateSerie(chartArea, SystemMetricModel.SystemMetricDiskIOMax, metric, metricColor);
                CreateSerie(chartArea, metric.ToString(), metric, metricColor);
                CreateSerie(chartArea, SystemMetricModel.SystemMetricDiskIOMin, metric, metricColor);
            }
            else if (metric == SystemMetricModel.SystemMetrics.Tse)
            {
                CreateSerie(chartArea, SystemMetricModel.SystemMetricTseMax, metric, metricColor);
                CreateSerie(chartArea, metric.ToString(), metric, metricColor);
                CreateSerie(chartArea, SystemMetricModel.SystemMetricTseMin, metric, metricColor);
            }
            else
            {
                CreateSerie(chartArea, metric.ToString(), metric, metricColor);
            }

            _theThresholdColor.Add(chartArea.Name, metricThresholdColor);

            return chartArea;
        }

        private void CreateSerie(ChartArea chartArea, string systemMetric, SystemMetricModel.SystemMetrics metric, Color metricColor)
        {
            Series chartSeries = new Series();
            chartSeries.Name = GetChartSeriesName(systemMetric);
            chartSeries.BorderColor = Color.Transparent;
            chartSeries.Color = metricColor; //Color.LimeGreen;
            chartSeries.ChartArea = chartArea.Name;
            chartSeries.YValueMembers = SystemMetricModel.GetOverallSummaryYValueMember(metric);
            chartSeries.YValueType = SystemMetricModel.GetOverallSummaryYValueType(metric);
            chartSeries.XValueMember = SystemMetricModel.GetOverallSummaryXValueMember(metric);
            chartSeries.XValueType = SystemMetricModel.GetOverallSummaryXValueType(metric);
            _theChart.Series.Add(chartSeries);

            switch (systemMetric)
            {
                case SystemMetricModel.SystemMetricCoreBusyMin:
                    chartSeries["DrawSideBySide"] = "False";
                    chartSeries.BorderWidth = 0;
                    chartSeries.Color = SystemMetricModel.MinColor;
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.SystemMetricCoreBusyMinToolTip);
                    break;
                case SystemMetricModel.SystemMetricCoreBusyMax:
                    chartSeries["DrawSideBySide"] = "False";
                    chartSeries.BorderWidth = 0;
                    chartSeries.Color = SystemMetricModel.MaxColor;
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.SystemMetricCoreBusyMaxToolTip);
                    break;
                case SystemMetricModel.SystemMetricDiskIOMin:
                    chartSeries["DrawSideBySide"] = "False";
                    chartSeries.BorderWidth = 0;
                    chartSeries.Color = SystemMetricModel.MinColor;
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.SystemMetricDiskIOMinToolTip);
                    break;
                case SystemMetricModel.SystemMetricDiskIOMax:
                    chartSeries["DrawSideBySide"] = "False";
                    chartSeries.BorderWidth = 0;
                    chartSeries.Color = SystemMetricModel.MaxColor;
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.SystemMetricDiskIOMaxToolTip);
                    break;
                case SystemMetricModel.SystemMetricTseMin:
                    chartSeries["DrawSideBySide"] = "False";
                    chartSeries.BorderWidth = 0;
                    chartSeries.Color = SystemMetricModel.MinColor;
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.SystemMetricTseMinToolTip);
                    break;
                case SystemMetricModel.SystemMetricTseMax:
                    chartSeries["DrawSideBySide"] = "False";
                    chartSeries.BorderWidth = 0;
                    chartSeries.Color = SystemMetricModel.MaxColor;
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.SystemMetricTseMaxToolTip);
                    break;

                default:
                    chartSeries.ToolTip = string.Format(SystemMetricModel.SystemMetricToolTipFormat, SystemMetricModel.GetOverallSummaryTitle(metric));

                    // Set empty point
                    chartSeries.EmptyPointStyle.Color = Color.Red;
                    chartSeries.EmptyPointStyle.BorderWidth = 8;
                    chartSeries.EmptyPointStyle.MarkerSize = 12;
                    chartSeries.EmptyPointStyle.MarkerStyle = MarkerStyle.Triangle;
                    _theChart.Series[chartSeries.Name]["EmptyPointValue"] = "Zero";

                    // Create a new title for the new area
                    Title title = new Title();
                    title.Name = SystemMetricModel.GetOverallSummaryTitle(metric);
                    title.Text = SystemMetricModel.GetOverallSummaryTitle(metric);
                    title.DockedToChartArea = chartArea.Name;
                    title.Docking = Docking.Right;
                    title.DockingOffset = 0;
                    title.IsDockedInsideChartArea = false;
                    title.Alignment = ContentAlignment.MiddleCenter;
                    title.TextOrientation = TextOrientation.Auto;
                    title.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, GraphicsUnit.Point, 0);
                    title.ToolTip = SystemMetricModel.GetOverallSummaryTitle(metric);
                    _theChart.Titles.Add(title);

                    if (metric == SystemMetricModel.SystemMetrics.Core 
                        || metric == SystemMetricModel.SystemMetrics.Disk
                        || metric == SystemMetricModel.SystemMetrics.Tse)
                    {
                        string toolTip = string.Empty;
                        if (metric == SystemMetricModel.SystemMetrics.Core)
                        {
                            toolTip = SystemMetricModel.SystemMetricCoreBusyTitle;
                        }
                        else if (metric == SystemMetricModel.SystemMetrics.Disk)
                        {
                            toolTip = SystemMetricModel.SystemMetricDiskIOTitle;
                        }
                        else if (metric == SystemMetricModel.SystemMetrics.Tse)
                        {
                            toolTip = SystemMetricModel.GetTseMetricTitle();
                        }
                        title.ToolTip = title.Text = toolTip;

                        chartSeries["DrawSideBySide"] = "False";
                        chartSeries.BorderWidth = 0;
                    }
                    break;
            }
        }        

        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition connectionDefinition, ConnectionDefinition.Reason reason)
        {
            if (connectionDefinition.Name == this.ConnectionDefn.Name
                && reason == ConnectionDefinition.Reason.PlatformReleaseVersion)
            {
                this._serverVersion = connectionDefinition.ServerVersion;
            }
        }

        /// <summary>
        /// Event handler for prepaint event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theChart_PrePaint(object sender, ChartPaintEventArgs e)
        {
            RePaintCharts(e.Chart);
        }

        /// <summary>
        /// Repaint the charts for thresholds
        /// </summary>
        /// <param name="aChart"></param>
        private void RePaintCharts(Chart aChart)
        {
            for (int i = 0; i < aChart.Series.Count; i++)
            {
                Series series = aChart.Series[i];
                ChartArea area = _theChart.ChartAreas.FindByName(series.ChartArea);
                //if (area != null)
                //{
                //    area.AxisX.StripLines.Clear();
                //}
                for (int j = 0; j < series.Points.Count; j++)
                {                    
                    DataPoint dp = series.Points[j];
                    if (area != null && dp.YValues[0] >= area.CursorY.Position)
                    {
                        dp.Color = (Color)_theThresholdColor[area.Name];
                        dp.BackHatchStyle = ChartHatchStyle.DarkUpwardDiagonal;
                    }

                    //if (dp.IsEmpty)
                    //{
                    //    Console.WriteLine(string.Format("empty point, x = {0}, y ={1}", dp.XValue, dp.YValues[0]));
                    //    StripLine sl = new StripLine();
                    //    sl.IntervalOffset = j + .75;
                    //    sl.StripWidth = .5;
                    //    sl.BackColor = Color.FromArgb(64, Color.Orange);
                    //    area.AxisX.StripLines.Add(sl);
                    //}
                }
            }
        }

        #endregion Private methods
    }
}
