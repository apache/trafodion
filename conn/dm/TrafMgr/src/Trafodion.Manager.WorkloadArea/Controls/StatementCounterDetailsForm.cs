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
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using System.Collections;
using Trafodion.Manager.Framework;
using System.Windows.Forms.DataVisualization.Charting;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class StatementCounterDetailsForm : UserControl, IMenuProvider, IMainToolBarConsumer 
    {
        #region Constants        

        WidgetCanvas _mainCanvas;

        private const string WMS_STATEMENT_COUNTER_GRID_CONFIG_ID = "GridConfig_WmsStatementCounter";
        public const string DATE_TIME_COLUMN_NAME = "Time";
        public const string QUERY_COUNTERS_COLUMN_NAME_RUNNING = "Running";
        public const string QUERY_COUNTERS_COLUMN_NAME_COMPLETED = "Completed";
        public const string QUERY_COUNTERS_COLUMN_NAME_WAITING = "Waiting";
        public const string QUERY_COUNTERS_COLUMN_NAME_HOLDORSUSPENDED = "Hold/Suspended";
        public const string QUERY_COUNTERS_COLUMN_NAME_REJECTED = "Rejected";        

        #endregion        

        public StatementCounterDetailsForm()
        {
            InitializeComponent();
            statementCounterDetailsIGrid.CreateGridConfig(WMS_STATEMENT_COUNTER_GRID_CONFIG_ID);
            statementCounterDetailsIGrid.AddButtonControlToParent(DockStyle.Bottom);
            statementCounterDetailsIGrid.AddCountControlToParent("Number of Graph points : {0}", DockStyle.Top);
        }

        //Fill Statement Counter Details Chart With Content
        public void StatementCounterDetailsForm_Load(object sender, EventArgs e) 
        {
            //The charts and the grid for the raw counter values are placed in an outer canvas
            _mainCanvas = new WidgetCanvas();
            _mainCanvas.Dock = DockStyle.Fill;
            _mainCanvas.ThePersistenceKey = "StatementCounterHistory";
            Controls.Add(_mainCanvas);

            //Initialize the outer canvas
            _mainCanvas.LayoutManager = new GridLayoutManager(2, 1);
            
            //Add the chart to the main canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer graphContainer = new WidgetContainer(_mainCanvas, statementCounterDetailsChart, "Statement Counters");
            graphContainer.Name = "StatementCounterHistoryChart";
            graphContainer.AllowDelete = false;
            _mainCanvas.AddWidget(graphContainer, gridConstraint, -1);

            //Add Grid to the main canvas
            gridConstraint = new GridConstraint(1, 0, 1, 1);
            WidgetContainer gridContainer = new WidgetContainer(_mainCanvas, _statementGridPanel, "Statement Counter Details");
            gridContainer.Name = "StatementCounterHistoryGrid";
            gridContainer.AllowDelete = false;
            _mainCanvas.AddWidget(gridContainer, gridConstraint, -1);

            _mainCanvas.InitializeCanvas();
            _mainCanvas.Locked = LockManager.Locked;
                    
        }


        public void UpdateMetrics(DataTable statementCounterDataTable, int refreshRate)
        {
            Reset();

            statementCounterDetailsChart.Series[0].YValueMembers = QUERY_COUNTERS_COLUMN_NAME_RUNNING;
            statementCounterDetailsChart.Series[1].YValueMembers = QUERY_COUNTERS_COLUMN_NAME_COMPLETED;
            statementCounterDetailsChart.Series[2].YValueMembers = QUERY_COUNTERS_COLUMN_NAME_WAITING;
            statementCounterDetailsChart.Series[3].YValueMembers = QUERY_COUNTERS_COLUMN_NAME_HOLDORSUSPENDED;
            statementCounterDetailsChart.Series[4].YValueMembers = QUERY_COUNTERS_COLUMN_NAME_REJECTED;

            //Set Series Name
            statementCounterDetailsChart.Series[0].Name = QUERY_COUNTERS_COLUMN_NAME_RUNNING;
            statementCounterDetailsChart.Series[1].Name = QUERY_COUNTERS_COLUMN_NAME_COMPLETED;
            statementCounterDetailsChart.Series[2].Name = QUERY_COUNTERS_COLUMN_NAME_WAITING;
            statementCounterDetailsChart.Series[3].Name = QUERY_COUNTERS_COLUMN_NAME_HOLDORSUSPENDED;
            statementCounterDetailsChart.Series[4].Name = QUERY_COUNTERS_COLUMN_NAME_REJECTED;

            foreach (Series mySeries in statementCounterDetailsChart.Series)
            {
                mySeries.XValueMember = DATE_TIME_COLUMN_NAME;
                //Tooltip
                mySeries.ToolTip = "Total Statement Counters [" + mySeries.Name+ "] = #VALY at #VALX{dd MMM yyy HH:mm:ss}";
            }           
            
            if (statementCounterDataTable.Rows.Count > 0)
            {
                UpdateChart(statementCounterDetailsChart, statementCounterDataTable, refreshRate);

                //Update the data grid
                statementCounterDetailsIGrid.FillWithDataConfig(statementCounterDataTable);
                FormatGridCell();
            }
        }

        private void FormatGridCell()
        {
            TenTec.Windows.iGridLib.iGCellStyle fCellStyleDate = new TenTec.Windows.iGridLib.iGCellStyle(true);
            fCellStyleDate.FormatString = "{0:yyyy/MM/dd HH:mm:ss}";
            statementCounterDetailsIGrid.Cols[WMSPlatformCounterHistory.TIME_COLUMN_NAME].CellStyle = fCellStyleDate;
        }

        private void Reset()
        { 
            //Clear Series Data Points
            foreach (Series mySeries in statementCounterDetailsChart.Series) 
            {
                mySeries.Points.Clear();
            }        
            //Clear IGrid
            statementCounterDetailsIGrid.Rows.Clear();
        }

        ///<summary>
        ///Updates chart with new values
        ///</summary>
        private void UpdateChart(Chart theChart, DataTable statementCounterDataTable, int refreshRate) 
        {
            theChart.Invalidate();
            theChart.DataSource = statementCounterDataTable;

            if (statementCounterDataTable.Rows.Count > 0)
            {
                bool suitableTimeUnitsFound = false;//Used to determine if a suitable time unit is found for X axis

                //Since there could be a lot of data points that is being graphed, we cannot always represent the x axis in 
                //seconds or hours or minutes all the time.

                //So based on the current period for which the history is available, we recompute the X axis and represent it 
                //in an appropriate time units

                DataRow currentRow = statementCounterDataTable.Rows[statementCounterDataTable.Rows.Count - 1];
                DateTime currentMin = ((DateTime)statementCounterDataTable.Rows[0][DATE_TIME_COLUMN_NAME]);
                DateTime currentMax = ((DateTime)currentRow[DATE_TIME_COLUMN_NAME]);

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

        #region IMenuProvider Members

        public TrafodionMenuStrip GetMenuItems(ImmutableMenuStripWrapper aMenuStrip)
        {
            TrafodionToolStripMenuItem theResetLayoutMenuItem = _mainCanvas.ResetLayoutMenuItem;
            TrafodionToolStripMenuItem theLockStripMenuItem = _mainCanvas.LockMenuItem;

            System.Windows.Forms.ToolStripSeparator toolStripSeparator1 = new TrafodionToolStripSeparator();

            //Obtain the index of the exit menu because we want to insert the
            //menus just above the exit menu
            int exitIndex = aMenuStrip.getMenuIndex(global::Trafodion.Manager.Properties.Resources.MenuExit);

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

        #endregion

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
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.StatementCounterHistory);
        }

        #endregion IMainToolBarConsumer


    }
}
