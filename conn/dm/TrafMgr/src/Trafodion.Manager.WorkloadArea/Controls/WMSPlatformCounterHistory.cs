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
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSPlatformCounterHistory : UserControl, IMenuProvider, IMainToolBarConsumer
    {
        #region Member variables

        WidgetCanvas _mainCanvas;
        WidgetCanvas _graphCanvas;

        private const string WMS_PLATFORM_COUNTER_GRID_CONFIG_ID = "GridConfig_WmsPlatformCounter";
        public const string TIME_COLUMN_NAME = "Time";
        public const string MAX_NODE_BUSY_COLUMN_NAME = "Max CPU Busy Threshold";
        public const string MAX_MEM_USAGE_COLUMN_NAME = "Max Memory Usage Threshold";
        public const string PERCENT_NODE_BUSY_COLUMN_NAME = "Node Busy %";
        public const string PERCENT_MEM_USAGE_COLUMN_NAME = "Memory Usage %";
        public const string MAX_EXEC_QUERIES_COLUMN_NAME = "Max Exec Queries Threshold";
        public const string COUNT_EXEC_QUERIES_COLUMN_NAME = "Exec Queries";

        public const double MAX_EXE_QUERIES_Y_AXIS_VALUE = 5000;

        #endregion Member variables

        public WMSPlatformCounterHistory()
        {
            InitializeComponent();

            this._countersGrid.CreateGridConfig(WMS_PLATFORM_COUNTER_GRID_CONFIG_ID);
        }
        /// <summary>
        /// Initialize te WMS Platform counter details screen
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WMSPlatformCounterHistory_Load(object sender, EventArgs e)
        {
            //Remove the default chart instances created by the designer file 
            //and create them in a common way for all 4 charts in this screen.
            Controls.Remove(_avgMemUsageChart);
            Controls.Remove(_avgNodeBusyChart);
            Controls.Remove(_curExecQueriesChart);

            //Initialize Avg Node Busy Chart
            _avgNodeBusyChart = CreateChart(Color.SteelBlue);
            _avgNodeBusyChart.Dock = DockStyle.Fill;
            _avgNodeBusyChart.Series.Add(new Series());
            _avgNodeBusyChart.Series.Add(new Series());
            _avgNodeBusyChart.Series[0].ChartType = SeriesChartType.Area;
            _avgNodeBusyChart.Series[0].Color = Color.ForestGreen;
            _avgNodeBusyChart.Series[1].ChartType = SeriesChartType.Line;
            _avgNodeBusyChart.Series[1].BorderWidth = 2;
            _avgNodeBusyChart.Series[1].Color = Color.Red;

            //Initialize Avg Memory Usage Chart
            _avgMemUsageChart = CreateChart(Color.Black);
            _avgMemUsageChart.Dock = DockStyle.Fill;
            _avgMemUsageChart.Series.Add(new Series());
            _avgMemUsageChart.Series.Add(new Series());
            _avgMemUsageChart.Series[0].ChartType = SeriesChartType.Area;
            _avgMemUsageChart.Series[1].ChartType = SeriesChartType.Line;
            _avgMemUsageChart.Series[0].Color = Color.ForestGreen;
            _avgMemUsageChart.Series[1].Color = Color.Red;
            _avgMemUsageChart.Series[1].BorderWidth = 2;
            
            //Initialize Avg Exec Queries Chart
            _curExecQueriesChart = CreateChart(Color.Black);
            _curExecQueriesChart.Dock = DockStyle.Fill;
            _curExecQueriesChart.Series.Add(new Series());
            _curExecQueriesChart.Series.Add(new Series());
            _curExecQueriesChart.Series[0].ChartType = SeriesChartType.Area;
            _curExecQueriesChart.Series[1].ChartType = SeriesChartType.Line;
            _curExecQueriesChart.Series[0].Color = Color.ForestGreen;
            _curExecQueriesChart.Series[1].Color = Color.Red;
            _curExecQueriesChart.Series[1].BorderWidth = 2;

            //The charts and the grid for the raw counter values are placed in an outer canvas
            _mainCanvas = new WidgetCanvas();
            _mainCanvas.Dock = DockStyle.Fill;
            _mainCanvas.ThePersistenceKey = "PlatformCounterHistory";
            Controls.Add(_mainCanvas);

            //Initialize the outer canvas
            _mainCanvas.LayoutManager = new GridLayoutManager(6, 1);

            //Create the inner canvas to hold the charts
            _graphCanvas = new WidgetCanvas();
            _graphCanvas.ThePersistenceKey = "PlatformCounterHistoryGraphs1";

            //Add the graph canvas to the main canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 4, 1);
            WidgetContainer graphContainer = new WidgetContainer(_mainCanvas, _graphCanvas, "");
            graphContainer.Name = "PlatformCounterHistoryGraphs";
            graphContainer.AllowDelete = false;
            _mainCanvas.AddWidget(graphContainer, gridConstraint, -1);

            //Add the grid to the main canvas
            gridConstraint = new GridConstraint(4, 0, 2, 1);
            WidgetContainer gridContainer = new WidgetContainer(_mainCanvas, _dataGridPanel, "Counter Details");
            gridContainer.Name = "PlatformCounterHistoryGrid";
            gridContainer.AllowDelete = false;            
            _mainCanvas.AddWidget(gridContainer, gridConstraint, -1);


            //Initialize the inner graph canvas
            _graphCanvas.LayoutManager = new GridLayoutManager(3, 10); ;

            GridConstraint gridConstraint1 = new GridConstraint(0, 0, 1, 9);
            WidgetContainer avgNodeBusyChartContainer = new WidgetContainer(_graphCanvas, _avgNodeBusyChart, "Average Node Busy %");
            avgNodeBusyChartContainer.Name = "AvgNodeBusyChart";
            avgNodeBusyChartContainer.AllowDelete = false;
            _graphCanvas.AddWidget(avgNodeBusyChartContainer, gridConstraint1, -1);

            gridConstraint1 = new GridConstraint(0, 9, 1, 1);
            WidgetContainer avgNodeBusyTextContainer = new WidgetContainer(_graphCanvas, _nodeBusySplitContainer, "Node Busy");
            avgNodeBusyTextContainer.Name = "AvgNodeBusyText";
            avgNodeBusyTextContainer.AllowDelete = false;
            avgNodeBusyTextContainer.AllowMaximize = false;
            _graphCanvas.AddWidget(avgNodeBusyTextContainer, gridConstraint1, -1);

            gridConstraint1 = new GridConstraint(1, 0, 1, 9);
            WidgetContainer avgMemUsageChartContainer = new WidgetContainer(_graphCanvas, _avgMemUsageChart, "Average Memory Usage %");
            avgMemUsageChartContainer.Name = "AvgMemUsageChart";
            avgMemUsageChartContainer.AllowDelete = false;
            _graphCanvas.AddWidget(avgMemUsageChartContainer, gridConstraint1, -1);

            gridConstraint1 = new GridConstraint(1, 9, 1, 1);
            WidgetContainer avgMemUsageTextContainer = new WidgetContainer(_graphCanvas, _memUsageSplitContainer, "Memory Usage");
            avgMemUsageTextContainer.Name = "AvgMemUsageText";
            avgMemUsageTextContainer.AllowDelete = false;
            avgMemUsageTextContainer.AllowMaximize = false;
            _graphCanvas.AddWidget(avgMemUsageTextContainer, gridConstraint1, -1);
            
            //ExecQueries
            gridConstraint1 = new GridConstraint(2, 0, 1, 9);
            WidgetContainer curExecQueriseChartContainer = new WidgetContainer(_graphCanvas, _curExecQueriesChart, "Number of Executing Queries");
            curExecQueriseChartContainer.Name = "CurExecQueriseChart";
            curExecQueriseChartContainer.AllowDelete = false;
            _graphCanvas.AddWidget(curExecQueriseChartContainer, gridConstraint1, -1);

            gridConstraint1 = new GridConstraint(2, 9, 1, 1);
            WidgetContainer curExecQueriseTextContainer = new WidgetContainer(_graphCanvas, _execQueriesSplitContainer, "Exec Queries");
            curExecQueriseTextContainer.Name = "CurExecQueriseText";
            curExecQueriseTextContainer.AllowDelete = false;
            curExecQueriseTextContainer.AllowMaximize = false;
            _graphCanvas.AddWidget(curExecQueriseTextContainer, gridConstraint1, -1);

            _graphCanvas.InitializeCanvas();
            _mainCanvas.InitializeCanvas();

            _graphCanvas.Locked = LockManager.Locked;
            _mainCanvas.Locked = LockManager.Locked;

            _countersGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
            _countersGrid.AddButtonControlToParent(DockStyle.Bottom);
            _countersGrid.AddCountControlToParent("Number of Graph points : {0}", DockStyle.Top);
        }

        #region IMenuProvider implementation

        /// <summary>
        /// Implemeting the IMenuProvider interface. Provide the Lock and Reset Layout menu items
        /// </summary>
        /// <returns></returns>
        public Trafodion.Manager.Framework.Controls.TrafodionMenuStrip GetMenuItems(ImmutableMenuStripWrapper aMenuStripWrapper)
        {
            //get the menu items from the canvas
            TrafodionToolStripMenuItem theResetLayoutMenuItem = _mainCanvas.ResetLayoutMenuItem;
            TrafodionToolStripMenuItem theLockStripMenuItem = _mainCanvas.LockMenuItem;

            //Since we have the graph canvas inside the main canvas, we need to listen to the lock and reset layout events on the 
            //main canvas, so that we can do the same on the graph canvas
            _mainCanvas.LockMenuItem.Click += new EventHandler(LockMenuItem_Click);
            _mainCanvas.ResetLayoutMenuItem.Click += new EventHandler(ResetLayoutMenuItem_Click);

            System.Windows.Forms.ToolStripSeparator toolStripSeparator1 = new TrafodionToolStripSeparator();

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

            //Create the same menu structure as we have for main
            ToolStripMenuItem fileMenuItem = new ToolStripMenuItem();
            fileMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator1,
            theResetLayoutMenuItem,
            theLockStripMenuItem});

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

        /// <summary>
        /// When the outer main canvas is reset, the inner graph canvas is also reset
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void ResetLayoutMenuItem_Click(object sender, EventArgs e)
        {
            _graphCanvas.ResetLayoutMenuItem.PerformClick();
        }
        /// <summary>
        /// When the outer main canvas is locked, the inner graph canvas is also locked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void LockMenuItem_Click(object sender, EventArgs e)
        {
            _graphCanvas.LockMenuItem.PerformClick();
        }

        #endregion IMenuProvider implementation

        /// <summary>
        /// Reset the chart values and data grid
        /// </summary>
        private void Reset()
        {
            _avgNodeBusyChart.Series[0].Points.Clear();
            _avgNodeBusyChart.Series[1].Points.Clear();
            _avgMemUsageChart.Series[0].Points.Clear();
            _avgMemUsageChart.Series[1].Points.Clear();
            _curExecQueriesChart.Series[0].Points.Clear();
            _curExecQueriesChart.Series[1].Points.Clear();
            _nodeBusyTextBox.Text = "0%";
            _memUsageTextBox.Text = "0%";
            _execQueriesTextBox.Text = "0";
            _countersGrid.Rows.Clear();
            _nodeBusyVerticalProgressBar.Value = 0;
            _nodeBusyVerticalProgressBar.Update();
            _memUsageVerticalProgressBar.Value = 0;
            _memUsageVerticalProgressBar.Update();
        }

        /// <summary>
        /// Update the graphs and grid with updated list of metrics
        /// </summary>
        /// <param name="wmsPlatformMetrics"></param>
        /// <param name="refreshRate"></param>
        public void UpdateMetrics(DataTable wmsPlatformMetrics, int refreshRate)
        {
            Reset();

            //Since we clear the series and reset, we need to set the series properties every time.
            _avgNodeBusyChart.Series[0].XValueMember = TIME_COLUMN_NAME;
            _avgNodeBusyChart.Series[0].YValueMembers = PERCENT_NODE_BUSY_COLUMN_NAME;
            _avgNodeBusyChart.Series[0].XValueType = ChartValueType.DateTime;
            _avgNodeBusyChart.Series[1].XValueMember = TIME_COLUMN_NAME;
            _avgNodeBusyChart.Series[1].YValueMembers = MAX_NODE_BUSY_COLUMN_NAME;
            _avgNodeBusyChart.Series[1].XValueType = ChartValueType.DateTime;
            _avgNodeBusyChart.Series[0].ToolTip = "Value #VALY at #VALX{dd MMM yyy HH:mm:ss}";
            _avgNodeBusyChart.Series[1].ToolTip = "Max Value #VALY at #VALX{dd MMM yyy HH:mm:ss}";

            _avgMemUsageChart.Series[0].XValueMember = TIME_COLUMN_NAME;
            _avgMemUsageChart.Series[0].YValueMembers = PERCENT_MEM_USAGE_COLUMN_NAME;
            _avgMemUsageChart.Series[0].XValueType = ChartValueType.DateTime;
            _avgMemUsageChart.Series[1].XValueMember = TIME_COLUMN_NAME;
            _avgMemUsageChart.Series[1].YValueMembers = MAX_MEM_USAGE_COLUMN_NAME;
            _avgMemUsageChart.Series[1].XValueType = ChartValueType.DateTime;
            _avgMemUsageChart.Series[0].ToolTip = "Value #VALY at #VALX{dd MMM yyy HH:mm:ss}";
            _avgMemUsageChart.Series[1].ToolTip = "Max Value #VALY at #VALX{dd MMM yyy HH:mm:ss}";
            
            _curExecQueriesChart.Series[0].XValueMember = TIME_COLUMN_NAME;
            _curExecQueriesChart.Series[0].YValueMembers = COUNT_EXEC_QUERIES_COLUMN_NAME;
            _curExecQueriesChart.Series[0].XValueType = ChartValueType.DateTime;
            _curExecQueriesChart.Series[1].XValueMember = TIME_COLUMN_NAME;
            _curExecQueriesChart.Series[1].YValueMembers = MAX_EXEC_QUERIES_COLUMN_NAME;
            _curExecQueriesChart.Series[1].XValueType = ChartValueType.DateTime;
            _curExecQueriesChart.Series[0].ToolTip = "Value #VALY at #VALX{dd MMM yyy HH:mm:ss}";
            _curExecQueriesChart.Series[1].ToolTip = "Max Value #VALY at #VALX{dd MMM yyy HH:mm:ss}";

            _curExecQueriesChart.ChartAreas[0].AxisY.Maximum = GetYAsixMaxValue(wmsPlatformMetrics);


            if (wmsPlatformMetrics.Rows.Count > 0)
            {
                //Update the charts and recalibrate the x axis time in terms of seconds or minutes or hours or days
                UpdateChart(_avgNodeBusyChart, wmsPlatformMetrics, refreshRate);
                UpdateChart(_avgMemUsageChart, wmsPlatformMetrics, refreshRate);
                UpdateChart(_curExecQueriesChart, wmsPlatformMetrics, refreshRate);

                //Update the progress bar and text controls
                DataRow currentRow = wmsPlatformMetrics.Rows[wmsPlatformMetrics.Rows.Count - 1];
                int nodeBusyPercent = (int)((double)currentRow[PERCENT_NODE_BUSY_COLUMN_NAME]);
                if (nodeBusyPercent < 0)
                    nodeBusyPercent = 0;
                if (nodeBusyPercent > 100)
                    nodeBusyPercent = 100;
                _nodeBusyVerticalProgressBar.Value = nodeBusyPercent;
                _nodeBusyVerticalProgressBar.Update();
                _nodeBusyTextBox.Text = string.Format("{0:N0}%", nodeBusyPercent);

                int memUsagePercent = (int)((double)currentRow[PERCENT_MEM_USAGE_COLUMN_NAME]);
                if (memUsagePercent < 0)
                    memUsagePercent = 0;
                if (memUsagePercent > 100)
                    memUsagePercent = 100;
                _memUsageVerticalProgressBar.Value = memUsagePercent;
                _memUsageVerticalProgressBar.Update();
                _memUsageTextBox.Text = string.Format("{0:N0}%", memUsagePercent);
                
                int execQueriesCount = (int)((double)currentRow[COUNT_EXEC_QUERIES_COLUMN_NAME]);
                if (execQueriesCount < 0)
                    execQueriesCount = 0;
                _execQueriesTextBox.Text = string.Format("{0:N0}", execQueriesCount);

                //Update the data grid
                _countersGrid.FillWithDataConfig(wmsPlatformMetrics);
                FormatGridCell();
            }
        }

        private void FormatGridCell()
        {
            TenTec.Windows.iGridLib.iGCellStyle fCellStyleDate  = new TenTec.Windows.iGridLib.iGCellStyle(true);
            fCellStyleDate.FormatString = "{0:yyyy/MM/dd HH:mm:ss}";
            _countersGrid.Cols[WMSPlatformCounterHistory.TIME_COLUMN_NAME].CellStyle = fCellStyleDate;
        }


        private double GetYAsixMaxValue(DataTable wmsPlatformMetrics)
        {
            double maxValue=0, curValue = 0;
            for (int i = 0; i < wmsPlatformMetrics.Rows.Count; i++)
            {
                double value1 = Convert.ToDouble(wmsPlatformMetrics.Rows[i][MAX_EXEC_QUERIES_COLUMN_NAME]);
                maxValue = Math.Max(maxValue, value1);

                double value2 = Convert.ToDouble(wmsPlatformMetrics.Rows[i][COUNT_EXEC_QUERIES_COLUMN_NAME]);
                curValue = Math.Max(curValue, value2);                
            }

            double result= Math.Max(maxValue, curValue);
            result = result == 0 ? MAX_EXE_QUERIES_Y_AXIS_VALUE : result;
            return result = ((int)((result-1) / 10) + 1) * 10;
        }

        /// <summary>
        /// Clone a DataTable
        /// </summary>
        /// <param name="dtSource"></param>
        /// <returns></returns>
        private DataTable CloneDataTable(DataTable dtSource)
        {
            DataTable dtClone = new DataTable();
            foreach (DataColumn column in dtSource.Columns)
            {
                dtClone.Columns.Add(column.ColumnName, column.DataType);
            }
            foreach (DataRow row in dtSource.Rows)
            {
                dtClone.Rows.Add(row.ItemArray);
            }
            return dtClone;
        }

        /// <summary>
        /// Updates the chart with new values
        /// </summary>
        /// <param name="theChart"></param>
        /// <param name="wmsPlatformMetrics"></param>
        /// <param name="refreshRate"></param>
        private void UpdateChart(Chart theChart, DataTable dtChart, int refreshRate)
        {
            theChart.Invalidate();

            DataTable dtClone = CloneDataTable(dtChart);
            theChart.DataSource = dtClone; //set the datasource of the chart to point to the datatable

            if (dtClone.Rows.Count > 0)
            {
                bool suitableTimeUnitsFound = false;//Used to determine if a suitable time unit is found for X axis

                //Since there could be a lot of data points that is being graphed, we cannot always represent the x axis in 
                //seconds or hours or minutes all the time.

                //So based on the current period for which the history is available, we recompute the X axis and represent it 
                //in an appropriate time units

                DataRow currentRow = dtClone.Rows[dtClone.Rows.Count - 1];
                DateTime currentMin = ((DateTime)dtClone.Rows[0][TIME_COLUMN_NAME]);
                DateTime currentMax = ((DateTime)currentRow[TIME_COLUMN_NAME]);

                TimeSpan timeDiff = currentMax.Subtract(currentMin);

                //The refresh rate determines how frequently entries are written into history.
                //If refresh is less than 50 seconds, then try to express the X axis in seconds.
                if (refreshRate < 50)
                {
                    //Atmost displaying 21 labels in x axis seems to work better. 
                    //So check if we are going to exceed that limit (630 = 30 secs x 21) where 30 secs is assumed to be refresh rate
                    if (timeDiff.TotalSeconds < 630)
                    {
                        currentMax = currentMax.AddSeconds(30);// Set the current x axis max to an additional buffer of 30 seconds
                        theChart.ChartAreas[0].AxisX.LabelStyle.IntervalType = DateTimeIntervalType.Seconds;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Interval = 30;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Angle = -80;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.Interval = 30;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.IntervalType = DateTimeIntervalType.Seconds;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                        suitableTimeUnitsFound = true;
                    }
                    else
                    {
                        //Looks like we exceeded the number of labels, so continue to see if we can express x axis in minutes
                        suitableTimeUnitsFound = false;
                    }
                }

                //If refresh rate is more than 60 seconds, try to express x axis in minutes
                if (refreshRate > 60 || !suitableTimeUnitsFound)
                {
                    //Atmost displaying 21 labels in x axis seems to work better.
                    if (timeDiff.TotalMinutes < 21)
                    {
                        //If we have less than 21 labels, express x axis in minutes
                        currentMax = currentMax.AddMinutes(1);
                        theChart.ChartAreas[0].AxisX.LabelStyle.IntervalType = DateTimeIntervalType.Minutes;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Interval = 1;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Angle = -80;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.Interval = 1;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.IntervalType = DateTimeIntervalType.Minutes;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                        suitableTimeUnitsFound = true;
                    }
                    else
                    {
                        //Looks like we exceeded the number of labels, so continue to see if we can express x axis in hours
                        suitableTimeUnitsFound = false;
                    }
                }

                if (!suitableTimeUnitsFound)
                {
                    //Atmost displaying 21 labels in x axis seems to work better.
                    if (timeDiff.TotalHours < 21)
                    {
                        //If we have less than 21 labels, express x axis in hours
                        currentMax = currentMax.AddHours(1);
                        theChart.ChartAreas[0].AxisX.LabelStyle.IntervalType = DateTimeIntervalType.Hours;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Interval = 1;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Angle = -80;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.Interval = 1;
                        theChart.ChartAreas[0].AxisX.MajorTickMark.IntervalType = DateTimeIntervalType.Hours;
                        theChart.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
                        suitableTimeUnitsFound = true;
                    }
                    else
                    {
                        //Looks like we exceeded the number of labels, so continue to see if we can express x axis in days
                        suitableTimeUnitsFound = false;
                    }
                }

                //If no suitable time unit found yet, express the x axis in days
                if (!suitableTimeUnitsFound)
                {
                    currentMax = currentMax.AddDays(1);
                    theChart.ChartAreas[0].AxisX.LabelStyle.IntervalType = DateTimeIntervalType.Days;
                    theChart.ChartAreas[0].AxisX.LabelStyle.Interval = 1;
                    theChart.ChartAreas[0].AxisX.LabelStyle.Angle = -80;
                    theChart.ChartAreas[0].AxisX.MajorTickMark.Interval = 1;
                    theChart.ChartAreas[0].AxisX.MajorTickMark.IntervalType = DateTimeIntervalType.Days;
                    theChart.ChartAreas[0].AxisX.LabelStyle.Format = "dd ddd";
                }

                //Reset the min and max for the x axis, since we have repopulated the chart.
                theChart.ChartAreas[0].AxisX.Minimum = currentMin.ToOADate();
                theChart.ChartAreas[0].AxisX.Maximum = currentMax.ToOADate();

                //since we might have recalibrated x axis with a new time unit, set these properties again
                theChart.ChartAreas[0].AxisX.IsLabelAutoFit = true;
                theChart.ChartAreas[0].AxisX.LabelAutoFitStyle =
                    LabelAutoFitStyles.DecreaseFont | LabelAutoFitStyles.IncreaseFont | LabelAutoFitStyles.WordWrap | LabelAutoFitStyles.StaggeredLabels;
            }

            //Finally update the chart to reflect the changes
            theChart.Update();
        }

        /// <summary>
        /// Helper routine to create a chart with the desirable default properties
        /// </summary>
        /// <param name="gradientColor">Gradient color background for the chart area</param>
        /// <returns></returns>
        private Chart CreateChart(Color gradientColor)
        {
            // Create a Chart
            Chart theChart = new Chart();
            theChart.BackColor = Color.WhiteSmoke;
            theChart.BorderlineColor = Color.Black;
            theChart.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            theChart.BorderlineWidth = 1;
            theChart.BorderSkin.SkinStyle = System.Windows.Forms.DataVisualization.Charting.BorderSkinStyle.Emboss;

            // Create Chart Area
            ChartArea theChartArea = new ChartArea();
            theChartArea.BackColor = gradientColor;
            theChartArea.BackGradientStyle = System.Windows.Forms.DataVisualization.Charting.GradientStyle.DiagonalRight;
            theChartArea.BorderColor = Color.Black;
            theChartArea.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            theChartArea.Name = "Default";
            theChartArea.ShadowColor = System.Drawing.Color.Transparent;
            theChartArea.IsSameFontSizeForAllAxes = true;

            theChartArea.AxisX.MajorGrid.Enabled = false;
            theChartArea.AxisX.MajorTickMark.Enabled = true;
            theChartArea.AxisX.MajorTickMark.TickMarkStyle = TickMarkStyle.AcrossAxis;
            //theChartArea.AxisX.Title = "Time";
            //theChartArea.AxisX.TitleFont = new Font("Tahoma", 7);
            theChartArea.AxisX.LineColor = System.Drawing.Color.Black;
            theChartArea.AxisX.IsMarginVisible = false;

            theChartArea.AxisY.MajorGrid.Enabled = false;
            theChartArea.AxisY.MajorTickMark.Enabled = true;
            theChartArea.AxisY.MajorTickMark.TickMarkStyle = TickMarkStyle.AcrossAxis;
            theChartArea.AxisY.Minimum = 0.0;
            theChartArea.AxisY.Maximum = 100.0;
            //theChartArea.AxisY.Title = "Value";
            //theChartArea.AxisY.TitleFont = new Font("Tahoma", 7);
            theChartArea.AxisY.LineColor = System.Drawing.Color.Black;

            //Allow zoom on x axis
            theChartArea.CursorX.IsUserEnabled = true;
            theChartArea.CursorX.IsUserSelectionEnabled = true;
            theChartArea.CursorX.Interval = 0;
            theChartArea.AxisX.ScaleView.Zoomable = true;
            theChartArea.AxisX.ScrollBar.IsPositionedInside = true;
            theChartArea.AxisX.ScrollBar.BackColor = SystemColors.Control;
            theChartArea.AxisX.ScrollBar.ButtonColor = SystemColors.ScrollBar;
            theChartArea.AxisX.ScaleView.SmallScrollSize = double.NaN;
            theChart.ChartAreas.Add(theChartArea);

            return theChart;
        }

        #region IMainToolBarConsumer implementation

        /// <summary>
        /// Implementating the IMainToolBarConsumer interface, which the consumer could elect buttons to show and modify 
        /// the Help button to invoke context sensitive help topic.
        /// </summary>
        /// <param name="aMainToolBar"></param>
        public void CustomizeMainToolBarItems(Trafodion.Manager.Framework.MainToolBar aMainToolBar)
        {
            // Now, turn on all of the tool strip buttons for PCFTool
            aMainToolBar.TheSystemToolToolStripItem.Visible = true;
            aMainToolBar.TheSystemsToolStripSeparator.Visible = true;
            aMainToolBar.TheSQLWhiteboardToolStripItem.Visible = true;
            aMainToolBar.TheNCIToolStripItem.Visible = true;
            aMainToolBar.TheMetricMinerToolStripItem.Visible = true;
            aMainToolBar.TheOptionsToolStripItem.Visible = true;
            aMainToolBar.TheToolsStripSeparator.Visible = true;
            aMainToolBar.TheWindowManagerToolStripItem.Visible = true;
            aMainToolBar.TheWindowManagerStripSeparator.Visible = true;
            aMainToolBar.TheHelpToolStripItem.Visible = true;

            ///Customize the help topic if it is desired.
            aMainToolBar.UnRegisterDefaultHelpEventHandler();
            aMainToolBar.TheHelpToolStripItem.Alignment = ToolStripItemAlignment.Right;
            aMainToolBar.TheHelpToolStripItem.Click += new EventHandler(TheHelpToolStripItem_Click);
        }

        /// <summary>
        /// The event handler for the context sensitive 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheHelpToolStripItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.PlatformCounters);
        }

        #endregion IMainToolBarConsumer

    }
}
