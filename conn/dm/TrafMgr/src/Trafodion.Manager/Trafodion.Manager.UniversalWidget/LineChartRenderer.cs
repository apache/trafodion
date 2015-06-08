//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
using System.Text;
using System.Drawing;
using System.Windows.Forms;
//using ZedGraph;
using System.Windows.Forms.DataVisualization.Charting;

namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// This class will be responsible for rendering line charts
    /// </summary>
    public class LineChartRenderer : ChartRenderer
    {
        public override ChartTypes ChartRendererType
        {
            get { return ChartTypes.Line; }
        }

        //Method to render a Zedgraph
        //public override void RenderChart(GraphPane aGraphPane, UniversalWidgetConfig aConfig, DataProvider aDataProvider)
        //{
        //    //_theGraphPane = aGraphPane;
        //    //_theLineChartConfig = aConfig.ChartConfig as LineChartConfig;
        //    //if (_theLineChartConfig != null)
        //    //{
        //    //    //use the theme to set the UI aspects of the chart
        //    //    ChartTheme theme = ThemeManager.Instance.GetTheme(DefaultLineChartTheme.DefaultLineChartThemeName);
        //    //    _theLineChartConfig.ApplyTheme(theme);


        //    //    aGraphPane.Title.Text = _theLineChartConfig.Title;
        //    //    aGraphPane.XAxis.Title.Text = _theLineChartConfig.XaxisTitle;
        //    //    aGraphPane.YAxis.Title.Text = _theLineChartConfig.YAxisTitle;
        //    //    aGraphPane.XAxis.Type = _theLineChartConfig.XaxisType;

        //    //    //DataProvider provider = aConfig.DataProvider;
        //    //    DataProvider provider = aDataProvider;
        //    //    DataTable data = provider.GetDataTable();
        //    //    List<TrafodionPointPairList> ppList = GetPointPairList(data);

        //    //    foreach (TrafodionPointPairList pointPairList in ppList)
        //    //    {
        //    //        ChartLine chartLine = _theLineChartConfig.GetChartLineForPointConfig(pointPairList.PointConfig);
        //    //        if (chartLine != null)
        //    //        {
        //    //            LineItem curve = _theGraphPane.AddCurve(chartLine.ChartLineLabel, pointPairList, chartLine.ChartLineColor);
        //    //            curve.Symbol = new Symbol();
        //    //            if (chartLine.TheChartLineSymbol != null)
        //    //            {
        //    //                curve.Symbol.Type = chartLine.TheChartLineSymbol;
        //    //            }
        //    //            else
        //    //            {
        //    //                curve.Symbol.Type = SymbolType.None;
        //    //            }

        //    //            if (chartLine.ChartLineFill != null)
        //    //            {
        //    //                curve.Line.Fill = chartLine.ChartLineFill;
        //    //            }

        //    //            if (chartLine.SymbolFill != null)
        //    //            {
        //    //                curve.Symbol.Fill = chartLine.SymbolFill;
        //    //            }
        //    //            else if (curve.Symbol.Type != SymbolType.None)
        //    //            {
        //    //                curve.Symbol.Fill = new Fill(Color.White);
        //    //            }
        //    //        }
        //    //    }

        //    //    if (_theLineChartConfig.ChartFill != null)
        //    //    {
        //    //        aGraphPane.Chart.Fill = _theLineChartConfig.ChartFill;
        //    //    }
        //    //}
        //}

        //This method can be used to populate a chart from the data table and a configuration
        public override void RenderChart(Control aControl, ChartConfig chartConfig, DataTable aDataTable)
        {
            if (aDataTable == null)
                return;

            System.Windows.Forms.DataVisualization.Charting.Chart _theChart = aControl as System.Windows.Forms.DataVisualization.Charting.Chart;

            //do cleanup
            doCleanupOfChart(_theChart);

            //Populate the chart
            populateChartFromConfig(_theChart, chartConfig, aDataTable);

        }

        //Method to render a MS graph
        public override void RenderChart(Control aControl, UniversalWidgetConfig aConfig, DataProvider aDataProvider)
        {
            RenderChart(aControl, 
                aConfig.ChartConfig,
                aDataProvider.GetDataTable());
            //System.Windows.Forms.DataVisualization.Charting.Chart _theChart = aControl as System.Windows.Forms.DataVisualization.Charting.Chart;
            //DataProvider provider = aDataProvider;
            //DataTable data = provider.GetDataTable();
            //ChartConfig chartConfig = aConfig.ChartConfig;

            ////do cleanup
            //doCleanupOfChart(_theChart);

            ////Populate the chart
            //populateChartFromConfig(_theChart, chartConfig, data);
        }

        //Cleans up the chart control
        private void doCleanupOfChart(System.Windows.Forms.DataVisualization.Charting.Chart aChart)
        {
            aChart.ChartAreas.Clear();
            aChart.Series.Clear();
            aChart.Legends.Clear();
            aChart.Titles.Clear();
            aChart.DataBindings.Clear();
        }


        //This method shall be used to re-populate the chart from the configuration and the data
        private void populateChartFromConfig(System.Windows.Forms.DataVisualization.Charting.Chart _theChart, 
            ChartConfig chartConfig, 
            DataTable data)
        {
            if ((_theChart != null) && (chartConfig != null) && (data != null))
            {
                //Setup chart's attributes
                _theChart.BackColor = chartConfig.BackColor;
                _theChart.BorderlineColor = chartConfig.BorderLineColor;
                _theChart.BorderlineDashStyle = chartConfig.BorderLineDashStyle;
                _theChart.BorderlineWidth = chartConfig.BorderLineWidth;
                _theChart.Palette = chartConfig.ChartPalette;

                //Get the list of Titles and create them
                foreach (TitleConfig aTitleConfig in chartConfig.TitleConfigs)
                {
                    Title title = getChartTitle(aTitleConfig.Name, aTitleConfig.Text);
                    title.DockedToChartArea = aTitleConfig.DockedChartAreaName;
                    _theChart.Titles.Add(title);
                }

                //Get the list of Legends and create them
                foreach (LegendConfig aLegendConfig in chartConfig.LegendConfigs)
                {
                    System.Windows.Forms.DataVisualization.Charting.Legend legend = GetLegend(aLegendConfig.Name, aLegendConfig.Text);
                    legend.DockedToChartArea = aLegendConfig.DockedChartAreaName;
                    _theChart.Legends.Add(legend);
                }

                //Get a list of chart area names from the config, create them and add them to the chart
                //Also add each series
                foreach (ChartAreaConfig aAreaConfig in chartConfig.ChartAreaConfigs)
                {
                    ChartArea chartArea = GetChartArea(aAreaConfig.Name);
                    chartArea.CursorX.IsUserEnabled = true;
                    chartArea.CursorX.IsUserSelectionEnabled = true;
                    chartArea.AxisX.ScaleView.Zoomable = true;
                    chartArea.AxisX.Title = aAreaConfig.XAxisTitle;
                    if (aAreaConfig.XAxisLabelStyleFont != null)
                    {
                        chartArea.AxisX.IsLabelAutoFit = false;
                        chartArea.AxisX.LabelStyle.Font = aAreaConfig.XAxisLabelStyleFont;
                    }
                    else
                    {
                        chartArea.AxisX.IsLabelAutoFit = true;
                        chartArea.AxisX.LabelStyle = new LabelStyle();
                    }
                    chartArea.AxisX.LabelStyle.IntervalType = DateTimeIntervalType.Auto;
                    chartArea.AxisX.LabelStyle.IntervalOffsetType = DateTimeIntervalType.Auto;
                    chartArea.AxisX.LabelStyle.Format = aAreaConfig.XAxisLabelStyleFormat;
                    chartArea.AxisX.Interval = 1;
                    chartArea.AxisX.IntervalOffset = 1;
                    chartArea.AxisX.MajorTickMark.Interval = 1;
                    chartArea.AxisX.MajorTickMark.IntervalOffset = 1;

                    chartArea.CursorY.IsUserEnabled = true;
                    chartArea.CursorY.IsUserSelectionEnabled = true;
                    chartArea.AxisY.ScaleView.Zoomable = true;
                    chartArea.AxisY.Title = aAreaConfig.YAxisTitle;
                    if (aAreaConfig.YAxisLabelStyleFont != null)
                    {
                        chartArea.AxisY.IsLabelAutoFit = false;
                        chartArea.AxisY.LabelStyle.Font = aAreaConfig.YAxisLabelStyleFont;
                    }
                    else
                    {
                        chartArea.AxisY.IsLabelAutoFit = true;
                        chartArea.AxisY.LabelStyle = new LabelStyle();
                    }
                    chartArea.AxisY.LabelStyle.IntervalType = DateTimeIntervalType.Auto;
                    chartArea.AxisY.LabelStyle.IntervalOffsetType = DateTimeIntervalType.Auto;
                    chartArea.AxisY.LabelStyle.Format = aAreaConfig.YAxisLabelStyleFormat;


                    // 3D attributes
                    chartArea.Area3DStyle.Enable3D = aAreaConfig.Enable3D;
                    chartArea.Area3DStyle.Inclination = aAreaConfig.Inclination;
                    chartArea.Area3DStyle.IsClustered = aAreaConfig.IsClustered;
                    chartArea.Area3DStyle.IsRightAngleAxes = aAreaConfig.IsRightAngleAxes;
                    chartArea.Area3DStyle.LightStyle = aAreaConfig.LightStyleProperty;
                    chartArea.Area3DStyle.Perspective = aAreaConfig.Perspective;
                    chartArea.Area3DStyle.PointDepth = aAreaConfig.PointDepth;
                    chartArea.Area3DStyle.PointGapDepth = aAreaConfig.PointGapDepth;
                    chartArea.Area3DStyle.Rotation = aAreaConfig.Rotation;
                    chartArea.Area3DStyle.WallWidth = aAreaConfig.WallWidth;

                    // Appearance 
                    chartArea.BackColor = aAreaConfig.BackColor;
                    chartArea.BorderColor = aAreaConfig.BorderColor;
                    chartArea.BorderDashStyle = aAreaConfig.BorderDashStyle;
                    chartArea.BorderWidth = aAreaConfig.BorderWidth;

                    if (aAreaConfig.EnableTitle)
                    {
                        _theChart.Titles[aAreaConfig.TitleName].Font = aAreaConfig.ChartFont;
                        _theChart.Titles[aAreaConfig.TitleName].Docking = aAreaConfig.TitleDocking;
                        _theChart.Titles[aAreaConfig.TitleName].IsDockedInsideChartArea = aAreaConfig.IsTitleInsideChart;
                    }

                    if (aAreaConfig.EnableLegend)
                    {
                        _theChart.Legends[aAreaConfig.LegendName].Font = aAreaConfig.ChartFont;
                        _theChart.Legends[aAreaConfig.LegendName].Docking = aAreaConfig.LegendDocking;
                        _theChart.Legends[aAreaConfig.LegendName].IsDockedInsideChartArea = aAreaConfig.IsLegendInsideChart;
                    }

                    //// Alignment 
                    //chartArea.AlignmentOrientation = aAreaConfig.AlignmentOrientation;
                    //chartArea.AlignmentStyle = aAreaConfig.AlignmentStyle;
                    //chartArea.AlignWithChartArea = aAreaConfig.AlignWithChartArea;

                    _theChart.ChartAreas.Add(chartArea);
                    foreach (ChartSeriesConfig seriesConfig in aAreaConfig.ChartSeriesConfigs)
                    {
                        Series series = GetSeries(seriesConfig.Name, aAreaConfig.Name, seriesConfig.LegendName);
                        _theChart.Series.Add(series);
                        if (!string.IsNullOrEmpty(seriesConfig.YValueColumnName))
                        {
                            series.YValueMembers = seriesConfig.YValueColumnName;
                            series.YValueType = seriesConfig.YValueColumnDataType;
                        }
                        if (!string.IsNullOrEmpty(seriesConfig.XValueColumnName))
                        {
                            series.XValueMember = seriesConfig.XValueColumnName;
                            series.XValueType = seriesConfig.XValueColumnDataType;          
                        }
                        series.ChartType = seriesConfig.ChartType;
                        series.Enabled = seriesConfig.Enabled;
                        series.Color = seriesConfig.Color;
                        series.Palette = seriesConfig.Palette;
                        series.IsValueShownAsLabel = seriesConfig.IsValueShownAsLabel;
                        series.Label = seriesConfig.Label;
                        series.LabelFormat = seriesConfig.LabelFormat;
                        series.ToolTip = seriesConfig.ToolTip;

                        series.Font = aAreaConfig.ChartFont;
                    }
                }

                //Bind Data
                _theChart.DataSource = data;
                _theChart.DataBind();
            }
        }

        //Method to render a MS graph
        public void RenderChartForColumn(Control aControl, UniversalWidgetConfig aConfig, DataProvider aDataProvider, string aColName, bool append)
        {
            //Get the config
            ChartConfig config = null;
            if (append)
            {
                config = (aConfig.ChartConfig == null) ? new ChartConfig() : aConfig.ChartConfig;
            }
            else
            {
                config = new ChartConfig();
            }
            aConfig.ChartConfig = config;
            
            //Get the legend config
            LegendConfig legendConfig = null;
            if (config.LegendConfigs.Count > 0)
            {
                legendConfig =  config.LegendConfigs[0] ;
            }
            else
            {
                legendConfig =  new LegendConfig("Legend", "Legend");
                config.LegendConfigs.Add(legendConfig);
            }

            //Get the area config
            ChartAreaConfig areaConfig = new ChartAreaConfig();
            areaConfig.Name = "Area" + System.DateTime.Now.Ticks;


            //Add the series to the area
            ChartSeriesConfig  seriesConfig = new ChartSeriesConfig();
            seriesConfig.ChartAreaName = areaConfig.Name;
            seriesConfig.Name = areaConfig.Name + "_" + aColName;
            seriesConfig.Text = aColName;
            seriesConfig.LegendName = legendConfig.Name;
            seriesConfig.YValueColumnName = aColName;
            areaConfig.ChartSeriesConfigs.Add(seriesConfig);
            config.ChartAreaConfigs.Add(areaConfig);

            RenderChart(aControl, aConfig, aDataProvider);


            //System.Windows.Forms.DataVisualization.Charting.Chart _theChart = (System.Windows.Forms.DataVisualization.Charting.Chart)aControl;
            //DataProvider provider = aDataProvider;
            //DataTable data = provider.GetDataTable();

            //if (!append)
            //{
            //    //do cleanup
            //    _theChart.ChartAreas.Clear();
            //    _theChart.Series.Clear();
            //    _theChart.Legends.Clear();
            //    _theChart.Titles.Clear();

            //    //Get a list of chart area names from the config, create them and add them to the chart
            //    ChartArea chartArea = GetChartArea(aColName);

            //    //Get the list of legends from the config, and create them
            //    System.Windows.Forms.DataVisualization.Charting.Legend legend = GetLegend(aColName, aColName);

            //    //Get the list of series from the config, and create them
            //    Series series = GetSeries(aColName, aColName, aColName);

            //    //Get the title for the chart
            //    Title title = getChartTitle(aColName, aColName);

            //    //Add the components of the chart
            //    _theChart.Series.Add(series);
            //    _theChart.ChartAreas.Add(chartArea);
            //    //_theChart.Legends.Add(legend);
            //    _theChart.Titles.Add(title);

            //    //Bind Data
            //    _theChart.DataSource = data;
            //    _theChart.Series[aColName].YValueMembers = aColName;
            //    _theChart.DataBind();
            //}
            //else
            //{
            //    ChartArea chartArea = _theChart.ChartAreas.FindByName(aColName);
            //    if (chartArea == null)
            //    {
            //        chartArea = GetChartArea(aColName);
            //        System.Windows.Forms.DataVisualization.Charting.Legend legend = GetLegend("Legend1", aColName);
            //        Series series = GetSeries(aColName, aColName, "Legend1");
            //        _theChart.Series.Add(series);
            //        _theChart.ChartAreas.Add(chartArea);
            //       // _theChart.Legends.Add(legend);
            //        _theChart.Series[aColName].YValueMembers = aColName;
            //    }

            //    //Series series = _theChart.Series.FindByName(aColName);
            //    //if (series == null)
            //    //{
            //    //    series = GetSeries(aColName, "ChartArea1", "Legend1");
            //    //    if (_theChart.Series.Count > 0)
            //    //    {
            //    //        series.ChartType = SeriesChartType.Line;
            //    //    }
            //    //}
            //    //_theChart.Series.Add(series);
            //    //_theChart.Series[aColName].YValueMembers = aColName;
            //}
        }


        private Series GetSeries(string aSeriesName, string aChartAreaName, string aLegendName)
        {
            Series series = new Series();
            if (aChartAreaName != null)
                series.ChartArea = aChartAreaName;
            if (!string.IsNullOrEmpty(aLegendName))
            {
                series.Legend = aLegendName;
                series.IsVisibleInLegend = true;
            }
            else
            {
                series.IsVisibleInLegend = false;
            }

            if (aSeriesName != null)
                series.Name = aSeriesName;
            return series;
        }

        private ChartArea GetChartArea(string aChartAreaName)
        {
            ChartArea chartArea = new ChartArea();
            chartArea.Name = aChartAreaName;
            return chartArea;
        }

        private System.Windows.Forms.DataVisualization.Charting.Legend GetLegend(string aLegendName, string aTitle)
        {
            System.Windows.Forms.DataVisualization.Charting.Legend legend = new System.Windows.Forms.DataVisualization.Charting.Legend();
            legend.Name = aLegendName;
            legend.Title = aTitle;
            return legend;
        }

        private Title getChartTitle(string aTitledName, string aTitle)
        {
            Title title = new Title();
            title.Name = aTitledName;
            title.Text = aTitle;
            return title;
        }

        //public List<TrafodionPointPairList> GetPointPairList(DataTable aDataTable)
        //{
        //    List<TrafodionPointPairList> pointPairs = new List<TrafodionPointPairList>();
        //    foreach (ChartLine chartLine in this._theLineChartConfig.ChartLines)
        //    {
        //        PointConfig pointConfig = chartLine.PointConfig;
        //        TrafodionPointPairList pp = new TrafodionPointPairList(pointConfig);
        //        for (int i = 0; i < aDataTable.Rows.Count; i++)
        //        {
        //            pp.Add((pointConfig.XColName != null) ? DatabaseDataProvider.GetDouble(aDataTable.Rows[i][pointConfig.XColName]) : i,
        //                (pointConfig.YColName != null) ? DatabaseDataProvider.GetDouble(aDataTable.Rows[i][pointConfig.YColName]) : 0);
        //        }
        //        pointPairs.Add(pp);
        //    }
        //    return pointPairs;
        //}
    }
}
