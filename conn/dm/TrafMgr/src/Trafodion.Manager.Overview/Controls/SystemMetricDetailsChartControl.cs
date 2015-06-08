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
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Connections;
using System.Linq;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// Class for display detailed system metric charts
    /// </summary>
    public partial class SystemMetricDetailsChartControl : UserControl, IDataDisplayControl
    {
        #region Fields

        private ConnectionDefinition _theConnectionDefinition = null;
        private Hashtable _theThresholdColor = new Hashtable();
        private SystemMetricModel.SystemMetricDisplays _theSystemMetricDisplay;
        private DataTable _theMetricDetailsTable = null;
        private string _theToolTipFormat = "";
        private bool _theZeroHasBeenUpdated = false;
        private bool _theUseShorterIDLabel = false;
        private ContextMenu _theWidgetContextMenu = null;
        private ChartArea _theHitChartArea = null;

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
        /// Property: DataProvider - the data provider used by this widget
        /// </summary>
        public DataProvider DataProvider
        {
            get;
            set;
        }

        /// <summary>
        /// Proerpty: UniversalWidgetConfiguration - the configuration of the widget
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get;
            set;
        }

        /// <summary>
        /// Property: DataDisplayHandler - the data display handler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get;
            set;
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
        /// Constructor
        /// </summary>
        /// <param name="aConnectonDefinition"></param>
        /// <param name="aMetricDisplay"></param>
        public SystemMetricDetailsChartControl(ConnectionDefinition aConnectonDefinition, SystemMetricModel.SystemMetricDisplays aMetricDisplay)
        {
            _theConnectionDefinition = aConnectonDefinition;
            _theSystemMetricDisplay = aMetricDisplay;
            
            InitializeComponent();
            CreateTheChart();
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Persistent configruation - this is not used. But, for need to be here to fulfill the requirement of the interface
        /// </summary>
        public void PersistConfiguration()
        {
        }

        /// <summary>
        /// Update the data table
        /// </summary>
        /// <param name="aDataTable"></param>
        public void UpdateChart(DataTable aDataTable, SystemMetricModel.SystemMetrics metric)
        {
            DataView dv0 = new DataView(aDataTable);
            _theChart.Invalidate();
            Color configBackColor = SystemMetricChartConfigModel.Instance.GetSystemMetricBackColor(ConnectionDefn.Name, metric);
            
            foreach (Series series in _theChart.Series)
            {
                series.Points.DataBindXY(dv0, series.XValueMember, dv0, series.YValueMembers);
            }

            for (int i = 1; i < _theChart.ChartAreas.Count; i++)
            {
                _theChart.ChartAreas[i].AlignWithChartArea = _theChart.ChartAreas[0].Name;                
            }
            for (int i = 0; i < _theChart.ChartAreas.Count; i++)
            {
                _theChart.ChartAreas[i].BackColor = configBackColor;
            }
            

            _theChart.Update();
        }

        public void ClearChartSeries()
        {
            try // Sometimes accessing Series property of Chart will throw exception.
            {
                if (this._theChart == null || this._theChart.Series == null || this._theChart.Series.Count == 0) return;

                foreach (Series chartSeries in this._theChart.Series)
                {
                    if (chartSeries != null)
                    {
                        chartSeries.Points.Clear();
                        this._theChart.ChartAreas[chartSeries.ChartArea].BackColor = Color.Gray;
                    }
                }
             
            }
            catch { }
        }
        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                _theChart.PrePaint -= _theChart_PrePaint;
                _theChart.MouseClick -= Chart_MouseClick;
            }
        }

        /// <summary>
        /// To create a MS chart for the system metric details
        /// </summary>
        private void CreateTheChart()
        {
            switch (_theSystemMetricDisplay)
            {
                case SystemMetricModel.SystemMetricDisplays.CoreMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.CoreMetricDetailsTable;
                    _theToolTipFormat = " Core: #VALX\n{0}: #VALY";
                    _theUseShorterIDLabel = true;
                    break;

                case SystemMetricModel.SystemMetricDisplays.MemoryMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.MemoryMetricDetailsTable;
                    _theToolTipFormat = SystemMetricModel.SystemMetricToolTipFormat;
                    _theUseShorterIDLabel = true; // only display node ID
                    break;

                case SystemMetricModel.SystemMetricDisplays.DiskMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.DiskMetricDetailsTable;
                    _theToolTipFormat = " Device: #VALX\n{0}: #VALY";
                    _theUseShorterIDLabel = false;
                    break;

                case SystemMetricModel.SystemMetricDisplays.TseMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.TseMetricDetailsTable;
                    _theToolTipFormat = " Device: #VALX\n{0}: #VALY";
                    _theUseShorterIDLabel = false;
                    break;

                case SystemMetricModel.SystemMetricDisplays.FileSystemMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.FileSystemMetricDetailsTable;
                    _theToolTipFormat = " File Sys: #VALX\n{0}: #VALY";
                    _theUseShorterIDLabel = false;
                    break;

                case SystemMetricModel.SystemMetricDisplays.LoadAvgMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.LoadAvgMetricDetailsTable;
                    _theToolTipFormat = SystemMetricModel.SystemMetricToolTipFormat;
                    _theUseShorterIDLabel = true; // only display node ID
                    break;

                case SystemMetricModel.SystemMetricDisplays.VMMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.VMMetricDetailsTable.Copy();
                    //Starting from M8, "Swap In" and "Swap Out" fields are not displayed
                    if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ130) 
                    {                        
                        //DataRow[] drSwapIn = _theMetricDetailsTable.Select("title='Swap In' or title='Swap Out'");
                        DataRow[] rows = (from t in _theMetricDetailsTable.AsEnumerable().Cast<DataRow>()
                                          where t.Field<string>("title") == "Swap In" || t.Field<string>("title") == "Swap Out"
                                          select t).ToArray();
                        
                        foreach (var item in rows)
                        {
                            _theMetricDetailsTable.Rows.Remove(item);
                        }
                    }
                    _theToolTipFormat = SystemMetricModel.SystemMetricToolTipFormat;
                    _theUseShorterIDLabel = true; // only display node ID
                    break;

                case SystemMetricModel.SystemMetricDisplays.NetworkMetricDetails:
                    _theMetricDetailsTable = SystemMetricModel.NetworkMetricDetailsTable;
                    _theToolTipFormat = " Interface: #VALX\n{0}: #VALY";
                    _theUseShorterIDLabel = true;
                    break;

                default:
                    return;
            }


            string lastChartAreaName = "";
            ChartArea chartArea;

            //Clean up first. 
            _theChart.ChartAreas.Clear();
            _theChart.Titles.Clear();
            _theChart.Legends.Clear();
            _theChart.Series.Clear();
            _theChart.Annotations.Clear();

            int chartPosOffset = (_theUseShorterIDLabel) ? 4 : 25;
            int metricCount = _theMetricDetailsTable.Rows.Count;
            //Create a chart area for each of the system metrics in the chart config
            for (int i = 0; i < metricCount; i++)
            {
                SystemMetricModel.SystemMetrics metric = (SystemMetricModel.SystemMetrics)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.colorTableKey.ToString()];

                Color metricBackColor = SystemMetricChartConfigModel.Instance.GetSystemMetricBackColor(ConnectionDefn.Name, metric);
                Color metricColor = SystemMetricChartConfigModel.Instance.GetSystemMetricColor(ConnectionDefn.Name, metric);
                Color metricGridLineColor = SystemMetricChartConfigModel.Instance.GetSystemMetricGridLineColor(ConnectionDefn.Name, metric);
                Color metricCursorColor = SystemMetricChartConfigModel.Instance.GetSystemMetricCursorColor(ConnectionDefn.Name, metric);
                Color metricThresholdColor = SystemMetricChartConfigModel.Instance.GetSystemMetricThresholdColor(ConnectionDefn.Name, metric);

                // Remember, the metric name is the sub categorary
                string metricName = (string)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.displayName.ToString()];
                string metricTitle = (string)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.title.ToString()];
                string xValueMember = (string)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.xValueMember.ToString()];
                ChartValueType xValueType = (ChartValueType)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.xValueType.ToString()];
                string yValueMember = (string)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.yValueMember.ToString()];
                ChartValueType yValueType = (ChartValueType)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.yValueType.ToString()];
                double yValueMin = (double)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.yValueMin.ToString()];
                double yValueMax = (double)_theMetricDetailsTable.Rows[i][SystemMetricModel.SystemMetricTableColumns.yValueMax.ToString()];
                if (yValueMax == -1)
                {
                    yValueMax = double.NaN;
                }

                string toolTipFormat = string.Format(_theToolTipFormat, metricTitle);


                if (metric == SystemMetricModel.SystemMetrics.Tse)
                {
                    metricTitle = SystemMetricModel.GetTseMetricTitle();
                    yValueMember = SystemMetricModel.SelectedTseMetric;
                }

                chartArea = CreateAChartArea(metricName, metricCount, metricBackColor, metricColor, metricGridLineColor, metricCursorColor, metricThresholdColor, 
                                             metricTitle, xValueMember, xValueType, yValueMember, yValueType, yValueMin, yValueMax, toolTipFormat);

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
                chartArea.Position.X = 4;
                chartArea.Position.Y = chartPosOffset + i * (100 / metricCount - 1);
                chartArea.Position.Width = 92;
                chartArea.Position.Height = (100 - chartPosOffset) / metricCount - 1;

                _theChart.ChartAreas.Add(chartArea);
            }

            //Create a chart area to display the Node ID
            chartArea = new ChartArea();
            chartArea.Name = string.Format("ChartArea_{0}", "NodeID");
            //chartArea.BackColor = SystemMetricChartConfigModel.DefaultMetricBackColor; //Color.Black;
            chartArea.BorderColor = Color.White;
            chartArea.BorderDashStyle = ChartDashStyle.Solid;
            chartArea.AxisX.Interval = 1;
            chartArea.AxisX.IsStartedFromZero = false;
            chartArea.AxisX.IsLabelAutoFit = true;
            chartArea.AxisX.LabelAutoFitStyle = LabelAutoFitStyles.LabelsAngleStep90 |LabelAutoFitStyles.DecreaseFont;
            chartArea.AxisX.MajorGrid.LineColor = SystemMetricChartConfigModel.DefaultMetricGridLineColor; //Color.YellowGreen;
            chartArea.AxisX.MajorGrid.Interval = 1;
            chartArea.AxisX.MajorGrid.LineDashStyle = ChartDashStyle.Dot;
            chartArea.AxisX.MajorTickMark.TickMarkStyle = TickMarkStyle.None;
            chartArea.Position.X = 1;
            chartArea.Position.Y = 1;
            chartArea.Position.Width = 92;
            chartArea.Position.Height = chartPosOffset;
            chartArea.InnerPlotPosition.Height = 1;
            chartArea.InnerPlotPosition.Width = 100;

            if (!string.IsNullOrEmpty(lastChartAreaName))
            {
                chartArea.AlignWithChartArea = _theChart.ChartAreas[0].Name;
                chartArea.AlignmentOrientation = AreaAlignmentOrientations.Vertical;
                chartArea.AlignmentStyle = AreaAlignmentStyles.All;
            }

            Series series = new Series();
            series.Name = string.Format("Series_{0}", "ID");
            series.BorderColor = Color.Transparent;
            series.Color = SystemMetricChartConfigModel.DefaultMetricColor; // Color.LimeGreen;
            series.ChartArea = chartArea.Name;
            series.YValueMembers = "zero";
            series.YValueType = ChartValueType.Double;
            series.XValueMember = (string)_theMetricDetailsTable.Rows[0][SystemMetricModel.SystemMetricTableColumns.xValueMember.ToString()];
            series.XValueType = (ChartValueType)_theMetricDetailsTable.Rows[0][SystemMetricModel.SystemMetricTableColumns.xValueType.ToString()];
            _theChart.Series.Add(series);
            _theChart.ChartAreas.Add(chartArea);

            this.CreateContextMenu();
            
            SetBarWidth(_theChart, 60);
            _theChart.PrePaint += new EventHandler<ChartPaintEventArgs>(_theChart_PrePaint);
            _theChart.MouseClick += Chart_MouseClick;
        }

        /// <summary>
        /// Create context menu
        /// </summary>
        private void CreateContextMenu()
        {
            _theWidgetContextMenu = new ContextMenu();
            _theWidgetContextMenu.MenuItems.Add("Set Threshold", new EventHandler(SetThreshold_MouseClick));
            _theWidgetContextMenu.MenuItems.Add("Clear Threshold", new EventHandler(RemoveThreshold_MouseClick));
        }

        /// <summary>
        /// Set threshold
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void SetThreshold_MouseClick(object sender, EventArgs e)
        {
            if (_theHitChartArea != null &&
                !_theHitChartArea.Name.Equals("ChartArea_NodeID"))
            {
                MouseEventArgs me = _theWidgetContextMenu.Tag as MouseEventArgs;
                if (me != null)
                {
                    var pos = me.Location;
                    _theHitChartArea.CursorY.SetCursorPixelPosition(new PointF(pos.X, pos.Y), true);
                }
            }
        }

        /// <summary>
        /// To clear threshold
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void RemoveThreshold_MouseClick(object sender, EventArgs e)
        {
            if (_theHitChartArea != null)
            {
                _theHitChartArea.CursorY.Position = double.NaN;
            }
        }

        /// <summary>
        /// Create a ChartArea for the chart
        /// </summary>
        /// <param name="metricName"></param>
        /// <param name="totalMetrics"></param>
        /// <param name="metricBackColor"></param>
        /// <param name="metricColor"></param>
        /// <param name="metricGridLineColor"></param>
        /// <param name="metricCursorColor"></param>
        /// <param name="metricThresholdColor"></param>
        /// <param name="metricTitle"></param>
        /// <param name="xValueMember"></param>
        /// <param name="xValueType"></param>
        /// <param name="yValueMember"></param>
        /// <param name="yValueType"></param>
        /// <param name="yValueMin"></param>
        /// <param name="yValueMax"></param>
        /// <param name="toolTipFormat"></param>
        /// <returns></returns>
        private ChartArea CreateAChartArea(string metricName, int totalMetrics,
                                     Color metricBackColor, Color metricColor, Color metricGridLineColor, Color metricCursorColor, Color metricThresholdColor,
                                     string metricTitle, string xValueMember, ChartValueType xValueType, string yValueMember, ChartValueType yValueType, double yValueMin, double yValueMax, 
                                     string toolTipFormat)
        {
            ChartArea chartArea = new ChartArea();
            chartArea.Name = string.Format("{0}", metricName);
            chartArea.BackColor = metricBackColor;
            chartArea.BorderDashStyle = ChartDashStyle.Solid;
            chartArea.AxisX.Interval = 1;
            chartArea.AxisX.IsLabelAutoFit = true;
            chartArea.AxisX.CustomLabels.Add(new CustomLabel());
            chartArea.AxisX.MajorGrid.LineColor = metricGridLineColor;
            chartArea.AxisX.MajorGrid.Interval = 1;
            chartArea.AxisX.MajorGrid.LineDashStyle = ChartDashStyle.Dot;
            chartArea.AxisX.MajorTickMark.TickMarkStyle = TickMarkStyle.None;
            chartArea.AxisX2.MajorTickMark.Enabled = false;
            chartArea.AxisX2.MinorTickMark.Enabled = false;

            chartArea.AxisY.MajorGrid.Enabled = true;
            chartArea.AxisY.MajorGrid.LineColor = metricGridLineColor;
            chartArea.AxisY.MajorGrid.LineDashStyle = ChartDashStyle.Dot;
            chartArea.AxisY.MajorGrid.LineWidth = 1;
            chartArea.AxisY.MajorTickMark.Enabled = false;
            chartArea.AxisY2.MajorTickMark.Enabled = false;
            chartArea.AxisY2.MinorTickMark.Enabled = false;

            chartArea.AxisY.Minimum = yValueMin;
            chartArea.AxisY.IsLabelAutoFit = true;

            //if (yValueMax > 1000)
            //{
            //    chartArea.AxisY.Maximum = yValueMax / 1000;
            //    chartArea.AxisY.LabelStyle.Format = "{0}K";
            //}

            if (yValueMax != -1)
            {
                chartArea.AxisY.Maximum = yValueMax;
            }

            chartArea.CursorY.LineColor = metricCursorColor;
            chartArea.CursorY.LineWidth = 2;
            //chartArea.CursorY.IsUserEnabled = true;
            chartArea.CursorY.Position = double.NaN;

            chartArea.CursorX.IsUserSelectionEnabled = true;

            //Now, create a new chart series for the new area
            Series chartSeries = new Series();
            chartSeries.Name = string.Format("Series_{0}", metricName);
            chartSeries.BorderColor = Color.Transparent;
            chartSeries.Color = metricColor;
            chartSeries.ChartArea = chartArea.Name;
            chartSeries.YValueMembers = yValueMember;
            chartSeries.YValueType = yValueType;
            chartSeries.XValueMember = xValueMember;
            chartSeries.XValueType = xValueType;
            chartSeries.ToolTip = toolTipFormat;
            _theChart.Series.Add(chartSeries);

            //Create a new title for the new area
            Title title = new Title();
            title.Name = title.Text = metricTitle;
            title.DockedToChartArea = chartArea.Name;
            title.Docking = Docking.Left;
            title.DockingOffset = 0;
            title.IsDockedInsideChartArea = false;
            title.Alignment = ContentAlignment.MiddleCenter;
            title.TextOrientation = TextOrientation.Auto;
            title.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, GraphicsUnit.Point, 0);
            _theChart.Titles.Add(title);
            _theThresholdColor.Add(chartArea.Name, metricThresholdColor);

            return chartArea;
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
                foreach (DataPoint dp in series.Points)
                {
                    ChartArea area = _theChart.ChartAreas.FindByName(series.ChartArea);
                    if (area != null && dp.YValues[0] >= area.CursorY.Position)
                    {
                        dp.Color = (Color)_theThresholdColor[area.Name];
                        dp.BackHatchStyle = ChartHatchStyle.DarkUpwardDiagonal;
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
                var result = targetChart.HitTest(pos.X, pos.Y, ChartElementType.PlottingArea);
                if (result.ChartArea != null &&
                    !result.ChartArea.Name.Equals("ChartArea_NodeID"))
                {
                    _theHitChartArea = result.ChartArea;
                    if (result.ChartArea.CursorY.Position.Equals(double.NaN))
                    {
                        _theWidgetContextMenu.MenuItems[0].Visible = true;
                        _theWidgetContextMenu.MenuItems[1].Visible = false;
                        _theWidgetContextMenu.Tag = e;
                    }
                    else
                    {
                        _theWidgetContextMenu.MenuItems[0].Visible = false;
                        _theWidgetContextMenu.MenuItems[1].Visible = true;
                        _theWidgetContextMenu.Tag = e;
                    }

                    this._theWidgetContextMenu.Show(targetChart, e.Location);
                }
                else
                {
                    _theHitChartArea = null;
                }
            }
        }

        /// <summary>
        /// This method is to set the width of databar, so it won't look too wide when connecting single node.
        /// there are other properties to control the width, like MinPixelPointWidth and
        /// PixelPointWidth to set an exact value
        /// </summary>
        /// <param name="aChart"></param>
        /// <param name="width"></param>
        private void SetBarWidth(Chart aChart, int width)
        {
            foreach (Series s in aChart.Series)
            {                
                s["MaxPixelPointWidth"] = width.ToString();
            }
        }

        #endregion Private methods
    }
}
