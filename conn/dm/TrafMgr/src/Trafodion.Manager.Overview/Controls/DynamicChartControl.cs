// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.Framework.Connections;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Threading;

namespace Trafodion.Manager.OverviewArea
{
    public delegate void DynamicChartScaleViewChangingHandler(string chartName, string scaleChartAreaName);
    public delegate void DynamicChartScaleViewChangedHandler(string chartName);
    public partial class DynamicChartControl : UserControl
    {
        #region Fields
        
        private List<SystemMetricModel.SystemMetrics> _theMetrics = null;
        private ConnectionDefinition _theConnectionDefinition = null;
        private double _zoomStartX = double.MaxValue;
        private double _zoomEndX = 0D;

        #endregion Fields

        

        #region Constructor
        public DynamicChartControl()
        {
            InitializeComponent();
        }


        public DynamicChartControl(ConnectionDefinition aConnectionDefinition, List<SystemMetricModel.SystemMetrics> aMetrics) 
        {
            _theMetrics = aMetrics;            
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            Name = aMetrics[0].ToString();
            CreateChartAreas();            
        }

    
        #endregion Constructor

        public event DynamicChartScaleViewChangingHandler OnDynamicChartScaleViewChanging;
        public event DynamicChartScaleViewChangedHandler OnDynamicChartScaleViewChanged;

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
        /// Property: Metrics - the list of metrics in the chart
        /// </summary>
        public List<SystemMetricModel.SystemMetrics> Metrics
        {
            get { return _theMetrics; }
        }

        #endregion Properties



        #region Public methods

        public void ResetTseSkew()
        {
            string tseChartAreaName = SystemMetricModel.GetChartAreaName(SystemMetricModel.SystemMetrics.Tse);
            ChartArea tseChartArea = this.Chart.ChartAreas.FindByName(tseChartAreaName);
            if (tseChartArea != null)
            {
                Title tseChartTitle = this.Chart.Titles.FindByName(SystemMetricModel.GetOverallSummaryTitle(SystemMetricModel.SystemMetrics.Tse));
                tseChartTitle.Text = tseChartTitle.ToolTip = SystemMetricModel.GetTseMetricTitle();

                foreach (Series series in this.Chart.Series)
                {
                    if ( 0 == string.Compare( series.ChartArea, tseChartAreaName, true ))
                    {
                        series.Points.Clear();
                    }
                }
            }
        }

        #endregion Public methods

        #region Private methods
        private void CreateChartAreas() 
        {    
            _theChart.ChartAreas.Clear();
            _theChart.Titles.Clear();
            _theChart.Legends.Clear();
            _theChart.Series.Clear();
            _theChart.Annotations.Clear();

            int metricCount = _theMetrics.Count;

            //Create ChartArea
            for (int i = metricCount-1; i >= 0; i--)
            {
                SystemMetricModel.SystemMetrics metric = _theMetrics[i];

                Color metricBackColor = SystemMetricChartConfigModel.Instance.GetSystemMetricBackColor(ConnectionDefn.Name, metric);
                Color metricColor = SystemMetricChartConfigModel.Instance.GetSystemMetricColor(ConnectionDefn.Name, metric);
                Color metricGridLineColor = SystemMetricChartConfigModel.Instance.GetSystemMetricGridLineColor(ConnectionDefn.Name, metric);
                Color metricCursorColor = SystemMetricChartConfigModel.Instance.GetSystemMetricCursorColor(ConnectionDefn.Name, metric);
                Color metricThresholdColor = SystemMetricChartConfigModel.Instance.GetSystemMetricThresholdColor(ConnectionDefn.Name, metric);
                ChartArea chartArea = CreateAChartArea(metric, metricBackColor, metricColor, metricGridLineColor, metricCursorColor, metricThresholdColor);

                if (i != metricCount - 1)
                {
                    chartArea.AxisX.CustomLabels.Add(new CustomLabel());
                }
                else
                {
                    //XAsix
                    chartArea.AxisX.IsLabelAutoFit = false;
                    chartArea.AxisX.Interval = 2;
                    chartArea.AxisX.IntervalType = DateTimeIntervalType.Minutes;
                    chartArea.AxisX.MinorGrid.Enabled = false;
                    chartArea.AxisX.MajorGrid.Enabled = false;
                    chartArea.AxisX.MajorTickMark.Enabled = false;
                    chartArea.AxisX.MinorTickMark.Enabled = false;
                    chartArea.AxisX.Enabled = AxisEnabled.True;
                    chartArea.AxisX.IsMarginVisible = false;
                    chartArea.AxisX.LabelStyle.Format = "HH:mm";
                    chartArea.AxisX.LabelStyle.Font = new Font("Arial", 8, FontStyle.Bold);
                }
                float bottomLabelHeight = 2;
                float metricHeight = (100 - bottomLabelHeight) / metricCount - 1.5f;
                chartArea.Position.X = 0;
                chartArea.Position.Y = i * (100 / metricCount);
                chartArea.Position.Width = 95;
                chartArea.Position.Height = (i != metricCount - 1) ? metricHeight : (metricHeight + bottomLabelHeight);
                

                // Enable range selection and zooming UI by default
                //chartArea.CursorX.IsUserEnabled = true;
                chartArea.CursorX.IsUserSelectionEnabled = true;
                chartArea.CursorX.Interval = 2D;
                chartArea.CursorX.IntervalType = DateTimeIntervalType.Seconds;
                chartArea.CursorX.SelectionColor = System.Drawing.Color.IndianRed;
                chartArea.AxisX.ScaleView.SmallScrollMinSize = 2D;
                chartArea.AxisX.ScaleView.SmallScrollMinSizeType = DateTimeIntervalType.Seconds;
                chartArea.AxisX.ScaleView.SmallScrollSizeType = DateTimeIntervalType.Seconds;
                chartArea.AxisX.ScaleView.SmallScrollSize = 2D;
                chartArea.AxisX.ScaleView.Zoomable = true;

                //set scrollbar style
                chartArea.AxisX.ScrollBar.IsPositionedInside = true;
                chartArea.AxisX.ScrollBar.ButtonStyle = ScrollBarButtonStyles.All;
                chartArea.AxisX.ScrollBar.BackColor = Color.LightGray;
                chartArea.AxisX.ScrollBar.ButtonColor = Color.Gray;
                chartArea.AxisX.ScrollBar.LineColor = Color.Black;


                if (!_theChart.ChartAreas.Contains(chartArea))
                {
                    _theChart.ChartAreas.Add(chartArea);                  
                }

            }

            AlignChartArea();

            _theChart.AxisViewChanging += new EventHandler<ViewEventArgs>(Chart_AxisViewChanging);
            _theChart.AxisViewChanged += new EventHandler<ViewEventArgs>(Chart_AxisViewChanged);

            for (int i = 0; i < _theChart.ChartAreas.Count; i++)
            {
                _theChart.ChartAreas[i].AlignmentStyle = AreaAlignmentStyles.All;// AreaAlignmentStyles.All;//AreaAlignmentStyles.PlotPosition | AreaAlignmentStyles.Cursor | AreaAlignmentStyles.AxesView;
                _theChart.ChartAreas[i].AxisY.ScaleView.Zoomable = true;
            }

            //Create Series            
            for (int i = 0; i < _theMetrics.Count; i++) 
            {
                SystemMetricModel.SystemMetrics metric = _theMetrics[i];                
                Color metricMaxLineColor = SystemMetricChartConfigModel.Instance.GetSystemMetricMaxLineColor(ConnectionDefn.Name, metric);
                Color metricMinLineColor = SystemMetricChartConfigModel.Instance.GetSystemMetricMinLineColor(ConnectionDefn.Name, metric);
                Color metricAvgLineColor = SystemMetricChartConfigModel.Instance.GetSystemMetricAverageLineColor(ConnectionDefn.Name, metric);
                ChartArea theChartArea = _theChart.ChartAreas[metric.ToString()];
                CreateSeries(metric, theChartArea, metricMaxLineColor,metricMinLineColor, metricAvgLineColor);
            }


        }

        private void Chart_AxisViewChanging(object sender, ViewEventArgs e)
        {            
            if (OnDynamicChartScaleViewChanging != null)
            {
                OnDynamicChartScaleViewChanging(Name,e.ChartArea.Name);
            }
        }

        private void Chart_AxisViewChanged(object sender, ViewEventArgs e)
        {
            if (OnDynamicChartScaleViewChanged != null)
            {
                OnDynamicChartScaleViewChanged(Name);
            }
        }

        public void AlignChartScaleView(string alignChartArea)
        {
            if (_theChart.ChartAreas.FindByName(alignChartArea) != null)
            {
                for (int i = 0; i < _theChart.ChartAreas.Count; i++)
                {
                    if (_theChart.ChartAreas[i].Name != alignChartArea)
                    {
                        _theChart.ChartAreas[i].AlignWithChartArea = alignChartArea;
                    }
                }
            }
        }

        public double ZoomedPosition
        {
            get
            {
                return _theChart.ChartAreas[0].AxisX.ScaleView.Position;
            }
        }

        public double ZoomedSize
        {
            get
            {
                return _theChart.ChartAreas[0].AxisX.ScaleView.Size;
            }
        }

        public void Zoom(double position,double size)
        {
            _theChart.ChartAreas[0].AxisX.ScaleView.Position = position;
            _theChart.ChartAreas[0].AxisX.ScaleView.Size = size;
        }

        public void SetLabel(double size)
        {
            _theChart.ChartAreas[0].AxisX.IsLabelAutoFit = false;
            
            if (size.Equals(double.NaN))
            {
                _theChart.ChartAreas[0].AxisX.Interval = 2;
                _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Minutes;
                _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm";
            }
            else
            {
                int level = (int)Math.Round(size * 10000);
                //90seconds 
                if (level <= 10)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 10;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 20)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 20;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 30)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 30;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 40)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 40;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 50)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 50;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 60)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 60;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 70)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 70;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                }
                else if (level <= 80)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 1;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Minutes;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm";
                }
                else if (level <= 100)
                {
                    _theChart.ChartAreas[0].AxisX.Interval = 1.5D;
                    _theChart.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Minutes;
                    _theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm";
                }
            }
            _theChart.ChartAreas[0].AxisX.LabelStyle.Font = new Font("Arial", 8, FontStyle.Bold);
            _theChart.Invalidate();
        }



        private int GetSeriesCount(ChartArea aChartArea) 
        {
            int i = 0;
            foreach (var item in _theChart.Series)
            {
                if (item.ChartArea == aChartArea.Name)
                    i++;
            }
            return i;
        }

        private int GetTotalUnit(List<string> chartAreaNames) 
        {
            int unit = 0;
            for (int i = 0; i < chartAreaNames.Count; i++)
            {
                ChartArea area = _theChart.ChartAreas[chartAreaNames[i]];
                int seriesCount = GetSeriesCount(area);
                unit += seriesCount;
            }
            return unit;        
        }



        private int GetMetricCount(List<SystemMetricModel.SystemMetrics> metrics) 
        {
            int count = metrics.Count;

            foreach (List<SystemMetricModel.SystemMetrics> aList in SystemMetricModel.shareChartMetrics)
            {
                bool flag = false;
                foreach (var item in aList)
                {
                    if (metrics.Contains(item))
                        flag = true;
                }
                if (flag)
                    count--;
            }

            return count;
        }

        public void AddData(SystemMetricModel.SystemMetrics metric, DateTime xDateTime, double yValueMin, double yValueMax, double yValueAvg) 
        {
            AddNewData(_theChart.Series[string.Format("Series_{0}_MAX", metric.ToString())],
                _theChart.Series[string.Format("Series_{0}_MIN", metric.ToString())],
                _theChart.Series[string.Format("Series_{0}_AVG", metric.ToString())], 
                xDateTime, yValueMin, yValueMax, yValueAvg);            
        }

        public void RecoverData(SystemMetricModel.SystemMetrics metric, DataTable historicTable)
        {
            //Remove timeout data
            if (historicTable.Rows.Count > 0)
            {                
                DateTime serverTime = DateTime.UtcNow + ConnectionDefn.ServerGMTOffset;
                DateTime filterTime = serverTime.AddMinutes(-1 * SystemMetricModel.lengthMinutes);

                if ((DateTime)historicTable.Rows[0]["gen_time_ts_lct"] < filterTime)
                {
                    DataRow[] rows = historicTable.Select("gen_time_ts_lct" + "<" + "#" + filterTime.AddSeconds(-0.1).ToString("M/dd/yy HH:mm:ss.fff") + "#");
                    foreach (DataRow row in rows)
                    {
                        historicTable.Rows.Remove(row);
                    }
                }
            }

            for (int iRow = 0; iRow < historicTable.Rows.Count; iRow++)
            {
                AddOldData(metric, _theChart.Series[string.Format("Series_{0}_MAX", metric.ToString())],
                    _theChart.Series[string.Format("Series_{0}_MIN", metric.ToString())],
                    _theChart.Series[string.Format("Series_{0}_AVG", metric.ToString())], historicTable, iRow);
            }
        }

        public void AlignChartArea()
        {
            string lastChartAreaName = string.Empty;
            for (int i = 0; i < _theChart.ChartAreas.Count; i++)
            {
                if (!string.IsNullOrEmpty(lastChartAreaName))
                {
                    _theChart.ChartAreas[i].AlignWithChartArea = lastChartAreaName;
                    _theChart.ChartAreas[i].AlignmentOrientation = AreaAlignmentOrientations.Vertical;
                    _theChart.ChartAreas[i].AlignmentStyle = AreaAlignmentStyles.All;// AreaAlignmentStyles.All;//AreaAlignmentStyles.PlotPosition | AreaAlignmentStyles.Cursor | AreaAlignmentStyles.AxesView;
                }

                lastChartAreaName = _theChart.ChartAreas[i].Name;
            }
        }


        private int GetSeriesNumberOfChartArea(ChartArea aChartArea) 
        {
            int i = 0;
            foreach (var series in _theChart.Series)
            {
                if (series.ChartArea.Equals(aChartArea.Name)) 
                {
                    i++;
                }
            }
            return i;
        }

        

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if (keyData == (Keys.A | Keys.B | Keys.C))
            {
                DynamicChartModel model = DynamicChartModel.FindSystemModel(_theConnectionDefinition);
                object a = model.CoreMetricDynamicTable;
                object b = model.MemoryMetricDynamicTable;
                object c = model.DiskMetricDynamicTable;
                object d = model.FileSystemMetricDynamicTable;
                object e = model.LoadAvgMetricDynamicTable;
                object f = model.NetworkMetricDynamicTable;
                object g = model.VMMetricDynamicTable;

                List<object> alist = new List<object>();
                alist.Add(a);
                alist.Add(b);
                alist.Add(c);
                alist.Add(d);
                alist.Add(e);
                alist.Add(f);
                alist.Add(g);

                List<string> stringList = new List<string>()
                {
                    "Core", "Memory", "Disk", "File", "Load_Avg", "Network", "VM"           
                };

                double total = 0D;
                string result = "";
                for (int i = 0; i < alist.Count; i++)
                {
                    var item = alist[i];
                    Stream s = new MemoryStream();
                    BinaryFormatter formatter = new BinaryFormatter();
                    formatter.Serialize(s, item);
                    long size = 0;
                    size = s.Length;
                    total += s.Length / 1024;

                    result += stringList[i] + " Size " + s.Length / 1024 + "KB" + "\r\n";
                }

                MessageBox.Show(result + "Total: " + total / 1024 + "MB");

                return true;
            }
            if (keyData == (Keys.A | Keys.B | Keys.D))
            {
                DynamicChartModel model = DynamicChartModel.FindSystemModel(_theConnectionDefinition);
                string a ="CoreMetricDynamicTable:"+ model.CoreMetricDynamicTable.Rows.Count.ToString()+"\n";
                string b = "MemoryMetricDynamicTable:" + model.MemoryMetricDynamicTable.Rows.Count + "\n";
                string c = "DiskMetricDynamicTable:" + model.DiskMetricDynamicTable.Rows.Count + "\n";
                string d = "FileSystemMetricDynamicTable:" + model.FileSystemMetricDynamicTable.Rows.Count + "\n";
                string e = "LoadAvgMetricDynamicTable:" + model.LoadAvgMetricDynamicTable.Rows.Count + "\n";
                string f = "NetworkMetricDynamicTable:" + model.NetworkMetricDynamicTable.Rows.Count + "\n";
                string g = "VMMetricDynamicTable:" + model.VMMetricDynamicTable.Rows.Count;


                MessageBox.Show("Rows count:"+a+b+c+d+e+f+g);

                return true;
            }
            return base.ProcessCmdKey(ref msg, keyData);
        }




        private void AddNewData(Series chartSeriesMax, Series chartSeriesMin, Series chartSeriesAVG, DateTime xDateTime, double yValueMin, double yValueMax, double yValueAvg)
        {
            double xValue = xDateTime.ToOADate();
            chartSeriesMax.Points.AddXY(xValue, yValueMax);
            chartSeriesMin.Points.AddXY(xValue, yValueMin);
            chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
            SetChartAreaXAxis(chartSeriesMax,chartSeriesAVG,chartSeriesMin, xDateTime);
        }

        private void AddOldData(SystemMetricModel.SystemMetrics metric, Series chartSeriesMax, Series chartSeriesMin, Series chartSeriesAVG, DataTable table, int rowIndex)
        {
            switch (metric)
            {
                case SystemMetricModel.SystemMetrics.Core:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.CoreMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();
                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.CoreMetricDataTableColumns.avg_core.ToString()];
                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.CoreMetricDataTableColumns.min_core.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.CoreMetricDataTableColumns.max_core.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);

                        //SetChartAreaYAxis(chartSeries, 0, 100);
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Memory:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();
                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.avg_memory_used.ToString()];

                        //chartSeries.Points.AddXY(xValue, yValue);

                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.min_memory_used.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.max_memory_used.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                        //SetChartAreaYAxis(chartSeries, 0, 100);    
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Swap:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        ////Sum(avg_core_total)/nodeCount;
                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.avg_swap_used.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);
                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.min_swap_used.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.MemoryMetricDataTableColumns.max_swap_used.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                        //SetChartAreaYAxis(chartSeries, 0, 100);    
                    }
                    break;
                case SystemMetricModel.SystemMetrics.File_System:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.FileSysMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();
                        ////Sum(avg_core_total)/nodeCount;
                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.FileSysMetricDataTableColumns.avg_percent_consumed.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);

                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.FileSysMetricDataTableColumns.min_percent_consumed.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.FileSysMetricDataTableColumns.max_percent_consumed.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                        //SetChartAreaYAxis(chartSeries, 0, 100);    
                    }

                    break;
                case SystemMetricModel.SystemMetrics.Load_Avg:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.LoadAvgMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.LoadAvgMetricDataTableColumns.avg_one_min_avg.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);

                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.LoadAvgMetricDataTableColumns.min_one_min_avg.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.LoadAvgMetricDataTableColumns.max_one_min_avg.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Disk:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.DiskMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.DiskMetricDataTableColumns.avg_reads_and_writes.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);

                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.DiskMetricDataTableColumns.min_reads_and_writes.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.DiskMetricDataTableColumns.max_reads_and_writes.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                    }

                    break;

                case SystemMetricModel.SystemMetrics.Tse:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.TseMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        double yValueAvg;
                        double yValueMin;
                        double yValueMax;
                        SystemMetricModel.GetTseMetricValue(table.Rows[rowIndex], out yValueAvg, out yValueMin, out yValueMax);

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                    }
                    break;

                case SystemMetricModel.SystemMetrics.Network_Rcv:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.avg_rcv_packets.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);
                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.min_rcv_packets.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.max_rcv_packets.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);

                    }
                    break;
                case SystemMetricModel.SystemMetrics.Network_Txn:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.avg_txn_packets.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);
                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.min_txn_packets .ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.NetworkMetricDataTableColumns.max_txn_packets .ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                    {
                        DateTime xDataTime = ((DateTime)table.Rows[rowIndex][SystemMetricModel.VirtualMemoryMetricDataTableColumns.gen_time_ts_lct.ToString()]);
                        double xValue = xDataTime.ToOADate();

                        double yValueAvg = (double)table.Rows[rowIndex][SystemMetricModel.VirtualMemoryMetricDataTableColumns.avg_context_switches.ToString()];
                        //chartSeries.Points.AddXY(xValue, yValue);
                        double yValueMin = (double)table.Rows[rowIndex][SystemMetricModel.VirtualMemoryMetricDataTableColumns.min_context_switches.ToString()];
                        double yValueMax = (double)table.Rows[rowIndex][SystemMetricModel.VirtualMemoryMetricDataTableColumns.max_context_switches.ToString()];

                        chartSeriesMax.Points.AddXY(xValue, yValueMax);
                        chartSeriesMin.Points.AddXY(xValue, yValueMin);
                        chartSeriesAVG.Points.AddXY(xValue, yValueAvg);
                        SetChartAreaXAxis(chartSeriesMax, chartSeriesAVG, chartSeriesMin, xDataTime);
                    }
                    break;
                default:
                    break;
            }

        }

        private void SetChartAreaXAxis(Series aSeriesMax, Series aSeriesAvg, Series aSeriesMin, DateTime aDataTime) 
        {
            ChartArea currentChartArea = _theChart.ChartAreas[aSeriesMax.ChartArea];

            if (aSeriesMax.Points.Count == 0)
            {
                currentChartArea.AxisX.Minimum = aDataTime.ToOADate();
                currentChartArea.AxisX.Maximum = aDataTime.AddMinutes(SystemMetricModel.lengthMinutes).ToOADate();
            }
            else
            {
                Double removeBefore = aDataTime.AddMinutes(-1 * SystemMetricModel.lengthMinutes).ToOADate();
                while (aSeriesMax.Points[0].XValue < removeBefore)
                {
                    aSeriesMax.Points.RemoveAt(0);
                    aSeriesAvg.Points.RemoveAt(0);
                    aSeriesMin.Points.RemoveAt(0);
                }

                currentChartArea.AxisX.Minimum = aSeriesMax.Points[0].XValue;
                currentChartArea.AxisX.Maximum = DateTime.FromOADate(aSeriesMax.Points[0].XValue).AddMinutes(SystemMetricModel.lengthMinutes).ToOADate();
            }
        }


        private ChartArea CreateAChartArea(SystemMetricModel.SystemMetrics metric, 
            Color metricBackColor, Color metricColor, Color metricGridLineColor,
                                     Color metricCursorColor, Color metricThresholdColor)
        {
            ChartArea chartArea = new ChartArea();
            chartArea.Name = string.Format("{0}", metric.ToString());
            chartArea.BackColor = metricBackColor;
            chartArea.BorderDashStyle = ChartDashStyle.Solid;

            chartArea.AxisX.Interval = 1;
            chartArea.AxisX.IsLabelAutoFit = true;
            //chartArea.AxisX.CustomLabels.Add(new CustomLabel());
            chartArea.AxisX.MajorGrid.LineColor = metricGridLineColor;
            chartArea.AxisX.MajorGrid.Interval = 1;
            chartArea.AxisX.MajorGrid.LineDashStyle = ChartDashStyle.Solid;
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

            chartArea.AxisY.IsLabelAutoFit = true;
            chartArea.AxisY.CustomLabels.Add(new CustomLabel());

            if (metric.ToString().Equals(SystemMetricModel.SystemMetrics.Core.ToString()) ||
             metric.ToString().Equals(SystemMetricModel.SystemMetrics.Memory.ToString()) ||
             metric.ToString().Equals(SystemMetricModel.SystemMetrics.Swap.ToString()) ||
             metric.ToString().Equals(SystemMetricModel.SystemMetrics.File_System.ToString()))
            {
                chartArea.AxisY.Minimum = 0D;
                chartArea.AxisY.Maximum = 100D;
            }
            else
            {
                chartArea.AxisY.Minimum = Double.NaN;
                chartArea.AxisY.Maximum = Double.NaN;
            }

            Title title = new Title();
            string metricTitle = SystemMetricModel.GetOverallSummaryTitle(metric);
            title.Name = metricTitle;
            if (metric == SystemMetricModel.SystemMetrics.Tse)
            {
                metricTitle = SystemMetricModel.GetTseMetricTitle();
            }

            title.Text = title.ToolTip = metricTitle;

            title.DockedToChartArea = chartArea.Name;
            title.Docking = Docking.Right;
            title.DockingOffset = 0;
            title.IsDockedInsideChartArea = false;
            title.Alignment = ContentAlignment.TopCenter;
            title.TextOrientation = TextOrientation.Auto;
            title.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, GraphicsUnit.Point, 0);
            _theChart.Titles.Add(title);

            return chartArea;
        }

        private ChartArea CreateOrUseExsitingChartArea(SystemMetricModel.SystemMetrics metric, out bool IsUsingExistingChartArea) 
        {
            ChartArea chartArea;
            IsUsingExistingChartArea = false;
            switch (metric)
            {
                case SystemMetricModel.SystemMetrics.Core:
                case SystemMetricModel.SystemMetrics.File_System:
                case SystemMetricModel.SystemMetrics.Load_Avg:
                case SystemMetricModel.SystemMetrics.Disk:
                case SystemMetricModel.SystemMetrics.Tse:
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                    {
                        chartArea = new ChartArea();
                        chartArea.Name = metric.ToString();                        
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Memory:
                case SystemMetricModel.SystemMetrics.Swap:
                    {
                        string aName = "MemorySwap";

                        if (_theChart.ChartAreas.FindByName(aName) == null)
                        {
                            chartArea = new ChartArea();
                            chartArea.Name = aName;
                        }
                        else
                        {
                            chartArea = _theChart.ChartAreas[aName];
                            IsUsingExistingChartArea = true;
                        }
                    }
                    break;

                case SystemMetricModel.SystemMetrics.Network_Rcv:
                case SystemMetricModel.SystemMetrics.Network_Txn:
                    {
                        string aName = "Network";
                        if (_theChart.ChartAreas.FindByName(aName) == null)
                        {
                            chartArea = new ChartArea();
                            chartArea.Name = aName;
                        }
                        else
                        {
                            chartArea = _theChart.ChartAreas[aName];
                            IsUsingExistingChartArea = true;
                        }
                    }
                    break;
                default:
                    chartArea = new ChartArea();
                    chartArea.Name = metric.ToString();
                    break;
            }
            return chartArea;
        }

        private void CreateSeries(SystemMetricModel.SystemMetrics metric, ChartArea aChartArea, Color maxColor, Color minColor, Color avgColor) 
        {
            Series chartSeries = new Series();
            chartSeries.Name = GetChartMaxSeriesName(metric.ToString());
            chartSeries.ChartType = SeriesChartType.FastLine;            
            chartSeries.BorderWidth = 2;
            chartSeries.Color = maxColor;
            chartSeries.ChartArea = aChartArea.Name;
            chartSeries.XValueType = ChartValueType.DateTime;
            //chartSeries.EmptyPointStyle.BorderWidth = 0;
            //chartSeries.EmptyPointStyle.MarkerStyle = MarkerStyle.None;
            _theChart.Series.Add(chartSeries);

            Series chartSeriesMin = new Series();
            chartSeriesMin.Name = GetChartMinSeriesName(metric.ToString());
            chartSeriesMin.ChartType = SeriesChartType.FastLine;
            chartSeriesMin.BorderWidth = 2;
            chartSeriesMin.Color = minColor;
            chartSeriesMin.ChartArea = aChartArea.Name;
            chartSeriesMin.XValueType = ChartValueType.DateTime;
            _theChart.Series.Add(chartSeriesMin);

            Series chartSeriesAvg = new Series();
            chartSeriesAvg.Name = GetChartAvgSeriesName(metric.ToString());
            chartSeriesAvg.ChartType = SeriesChartType.FastLine;
            chartSeriesAvg.BorderWidth = 1;
            chartSeriesAvg.Color = avgColor;
            chartSeriesAvg.ChartArea = aChartArea.Name;
            chartSeriesAvg.XValueType = ChartValueType.DateTime;
            _theChart.Series.Add(chartSeriesAvg);
            
            switch (metric)
            {
                case SystemMetricModel.SystemMetrics.Core:                    
                case SystemMetricModel.SystemMetrics.Load_Avg:                   
                case SystemMetricModel.SystemMetrics.File_System:
                case SystemMetricModel.SystemMetrics.Memory:
                case SystemMetricModel.SystemMetrics.Swap:
                    {
                        chartSeries.ToolTip = SystemMetricModel.GetOverallSummaryTitle(metric) + ": \n" + " Min: #VALY2{F2}%\n Max: #VALY1{F2}%\n" + " #VALX{yy/MM/dd hh:mm:ss}";
                    }
                    break;
                case SystemMetricModel.SystemMetrics.Disk:
                case SystemMetricModel.SystemMetrics.Tse: 
                case SystemMetricModel.SystemMetrics.Virtual_Memory:
                case SystemMetricModel.SystemMetrics.Network_Rcv:
                case SystemMetricModel.SystemMetrics.Network_Txn: 
                    {
                        chartSeries.ToolTip = SystemMetricModel.GetOverallSummaryTitle(metric) + ": \n" + " Min: #VALY2{F}\n Max :#VALY1{F}\n" + " #VALX{yy/MM/dd hh:mm:ss}";
                    }
                    break; 
                default:
                    break;
            }
        
        
        }

        public static string GetChartAvgSeriesName(string systemMetric)
        {
            return string.Format("Series_{0}_AVG", systemMetric);
        }

        public static string GetChartMinSeriesName(string systemMetric)
        {
            return string.Format("Series_{0}_MIN", systemMetric);
        }

        public static string GetChartMaxSeriesName(string systemMetric)
        {
            return string.Format("Series_{0}_MAX", systemMetric);
        }

        #endregion Private methods
    }
}
