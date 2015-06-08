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
using System.Windows.Forms.DataVisualization.Charting;
using System.Windows.Forms;
using ZedGraph;
namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class TriageChartControl : UserControl
    {

		#region Enums
		public enum  GraphDataAggregateType { SUM = 0, AVERAGE = 1, MINIMUM = 2, MAXIMUM = 3, STANDARDDEVIATION = 4};

		#endregion  /*  End of Region  Enums.  */


		#region Instance Members

		//  Data for this widget
        //  private String _data = null;
        private Icon _icon = null;
		private String _graphMetricUnit = "";
		private String _displayedTimeRange = "";

		private bool _allowDrillDown = false;
        private ZedGraphControl _graph = new ZedGraphControl();
		private GraphEventHandler _pointValueHandler = null;
		private bool _allowSetScaleToDefault = false;
		private bool _allowAggregateFunctions = false;
		private ZedGraphControl.ContextMenuBuilderEventHandler  _contextMenuHandler = null;

		private GraphDataAggregateType _dataAggregateType = GraphDataAggregateType.SUM;
		private GraphDataAggregateType[] _graphDataAggregationFunctions = null;
        private string _data;
        private string _type;
        private bool _timeOffsetGMTAdd = false;
        private TimeSpan _displayTimeOffsetFromGMT = TimeSpan.FromTicks(0);
        private TriageHelper _theTriageHelper = null;

        public TriageHelper TriageHelper
        {
            get { return _theTriageHelper; }
            set { _theTriageHelper = value; }
        }
        public String WidgetType
        {
            get { return this._type; }
            set { this._type = value; }
        }

        #endregion

        public TriageChartControl()
        {
            InitializeComponent();
            initializeWidget();
            initializeGraphControl();
            this._graph.Show();
        }

        delegate void addPointCallback(double x, double y);


        //public NCCGraphWidget() : base() {
        //    base.AutoSize = false;
        //    initializeWidget(new Point(0, 0) );

        //}  /*  End of  NCCGraphWidget Default Constructor.  */


        //public NCCGraphWidget(NCCWorkspaceView view, String title,
        //                      Point position, Size bestFit)
        //    : base(view, title, position, bestFit)
        //{
        //    base.Size = bestFit;
        //    base.AutoSize = false;

        //    initializeWidget(position);

        //    initializeGraphControl();
        //    this._graph.Show();

        //}


		private void initializeAggregateFunctionList() {
			this._graphDataAggregationFunctions = new GraphDataAggregateType[5];
			this._graphDataAggregationFunctions[0] = GraphDataAggregateType.SUM;
			this._graphDataAggregationFunctions[1] = GraphDataAggregateType.AVERAGE;
			this._graphDataAggregationFunctions[2] = GraphDataAggregateType.MINIMUM;
			this._graphDataAggregationFunctions[3] = GraphDataAggregateType.MAXIMUM;
			this._graphDataAggregationFunctions[4] = GraphDataAggregateType.STANDARDDEVIATION;
		}


		private void initializeWidget() {
			initializeAggregateFunctionList();

			base.BackColor = Color.FromKnownColor(KnownColor.Snow);

			this._graph.Dock = DockStyle.Fill;

			this._graph.Click += new EventHandler(graphClickEventHandler);

			this.Controls.Add(this._graph);
			this._graph.Show();
            //this.activate();
            //this.deactivate();
		}


		private void graphClickEventHandler(Object sender, EventArgs eArgs) {
			//base.activate();
		}


        #region Properties

        public Icon Icon
        {
            get { return this._icon; }
            set { this._icon = value; }
        }

		public bool AllowDrillDown
		{
			get { return this._allowDrillDown; }
			set { this._allowDrillDown = value; }
		}

		public GraphPane GraphPane
		{
			get { return this._graph.GraphPane; }
		}


		public GraphEventHandler PointValueEventHandler {
			get { return this._pointValueHandler; }
			set { this._pointValueHandler = value; }
		}

		public EventHandler ClickEventHandler {
			set { this._graph.Click += new EventHandler(value); }
		}

		public ZedGraphControl.ContextMenuBuilderEventHandler ContextMenuHandler {
			set {
				if (null != this._contextMenuHandler)
					this._graph.ContextMenuBuilder -= this._contextMenuHandler;

				this._contextMenuHandler = value;
				this._graph.ContextMenuBuilder += value;
			}

		}

		public static int  NumGraphDataAggregateTypes {
			get { return  1 + ((int) GraphDataAggregateType.STANDARDDEVIATION); }
		}

		public virtual GraphDataAggregateType DataAggregateType {
			get { return this._dataAggregateType; }
			set {
				this._dataAggregateType = value;
				WidgetData = ((int) value).ToString();
				updateGraphPaneTitle();
			}
		}

		public String WidgetData {
            get { return _data; }
			set {
				try {
					int idx = Int16.Parse(value);
					if ((0 > idx) || (this._graphDataAggregationFunctions.Length <= idx))
						return;

                    _data = value;
					this._dataAggregateType = this._graphDataAggregationFunctions[idx];
					updateGraphPaneTitle();

				} catch (Exception) {
				}

			}
		}


		public int DataAggregateIndex {
			get { return (int)this.DataAggregateType; }
		}


		public String GraphMetricUnit {
			get { return this._graphMetricUnit; }
			set { this._graphMetricUnit = value; }
		}

		public ZedGraphControl ZedGraphControl {
			get { return this._graph; }
		}

		public bool ShowSetScaleToDefaultOption {
			get { return this._allowSetScaleToDefault; }
			set { this._allowSetScaleToDefault = value; }
		}

		public bool ShowAggregateFunctions {
			get { return this._allowAggregateFunctions; }
			set { this._allowAggregateFunctions = value; }
		}

        #endregion


		public virtual void configure() {
		}


		protected String aggregateTypeForDisplay() {
			return getAggregateFunctionType(this.DataAggregateType);
		}


		protected String getAggregateFunctionType(GraphDataAggregateType dataAggrType) {
			String type = "Average";

			switch (dataAggrType) {
				case GraphDataAggregateType.SUM: 
						type = "Total";
						break;

				case GraphDataAggregateType.AVERAGE:
						type = "Average";
						break;

				case GraphDataAggregateType.MAXIMUM :
						type = "Maximum";
						break;

				case GraphDataAggregateType.MINIMUM:
						type = "Minimum";
						break;

				case GraphDataAggregateType.STANDARDDEVIATION: 
						type = "Standard Deviation";
						break;

					default: type = "Average";
							 break;
			}

			return type;
		}


		public String getViewTimeRange() {
			return this._displayedTimeRange;
		}

		delegate void setViewTimeRangeDelegate(DateTime startTime, DateTime endTime);
		public void setViewTimeRange(DateTime startTime, DateTime endTime) {
			if (this.InvokeRequired) {
				this.Invoke(new setViewTimeRangeDelegate(setViewTimeRange),
							new Object[] { startTime, endTime });
				return;
			}

			try {
				this._displayedTimeRange = startTime.ToString("g") + "  to  " + endTime.ToString("g");
				updateGraphPaneTitle();
			} catch (Exception) {
			}

		}


		protected virtual void updateGraphPaneTitle() {
			String name = WidgetType;
			this.GraphPane.Title.Text = base.Text = "" + aggregateTypeForDisplay() + " " + name;

			if ((null != this._displayedTimeRange)  &&  
				(0 < this._displayedTimeRange.Trim().Length) )
				this.GraphPane.Title.Text = base.Text + "\n" + this._displayedTimeRange;


			this._graph.Invalidate();
			base.Refresh();
		}


        public void initializeGraphControl()
        {
            // Grab a reference to the ZedGraphControl's GraphPane.
            GraphPane myPane = this._graph.GraphPane;

			// Set the size of the legend, title and both the X and Y axis and their labels.
			myPane.Legend.FontSpec.Size = 16;
			myPane.Title.FontSpec.Size = 15;
			updateGraphPaneTitle();

            myPane.XAxis.Title.FontSpec.Size = 12;
            myPane.XAxis.Title.Text = "Time [Days/Hours/Mins]";

			myPane.YAxis.Title.FontSpec.Size = 12;
			myPane.YAxis.Title.Text = "Metric Value";

            double[] y = { 0 };
            double[] x = { 0 };
            x[0] = (double)new XDate(DateTime.Now.ToLocalTime());


            // Fill the axis background with a color gradient
			myPane.Fill = new Fill(Color.White, Color.LightGray, 45.0F);
			myPane.Chart.Fill = new Fill(Color.AntiqueWhite, Color.CornflowerBlue, 45.0f);

            // Draw a box item to highlight a value range
            // not working -- Ram!! :^(
            long now = DateTime.Now.ToBinary() + 60*60;
            double aaa = (double)new XDate(DateTime.FromBinary(now) );
            BoxObj box = new BoxObj(0, 100, aaa, 20, Color.Empty,
									Color.FromArgb(150, Color.LightGreen));
            box.Fill = new Fill(Color.Moccasin,
								Color.FromArgb(200, Color.LightGreen), 45.0F);

            // Use the BehindAxis zorder to draw the highlight beneath the
			// grid lines
            box.ZOrder = ZOrder.E_BehindAxis;
            myPane.GraphObjList.Add(box);


            // Generate a Red "MetricCurve".
            LineItem myCurve = myPane.AddCurve("MetricCurve", x, y, Color.Red);

            // Make the symbols opaque by filling them with white
            myCurve.Symbol.Fill = new Fill(Color.Moccasin);
			myCurve.Label.IsVisible = false;
   
			myPane.YAxis.Scale.FontSpec.Size = 11;
            // Display the Y axis grid lines
            myPane.YAxis.MajorGrid.IsVisible = true;
            myPane.YAxis.MinorGrid.IsVisible = true;

            // Set the XAxis to date type
            myPane.XAxis.Type = ZedGraph.AxisType.Date;

            // X axis step size is 1 day
            myPane.XAxis.Scale.MajorStep = 1;
			myPane.XAxis.Scale.MinorUnit = DateUnit.Second;
			myPane.XAxis.Scale.MajorUnit = DateUnit.Minute;

			// tilt the x axis labels to an angle of 65 degrees
            myPane.XAxis.Scale.FontSpec.Angle = 65;
            myPane.XAxis.Scale.FontSpec.IsBold = true;
            myPane.XAxis.Scale.FontSpec.Size = 11;
            myPane.XAxis.Scale.Format= "T";
            myPane.XAxis.Scale.FormatAuto = true;

            
            // Add a text item to label the highlighted range
            TextObj text = new TextObj("Drill down via rubberbanding.\nClick left Mouse button and drag.",
                0.05f, 0.95f, CoordType.ChartFraction, AlignH.Left, AlignV.Bottom);
            text.FontSpec.Fill.IsVisible = false;
            text.FontSpec.Border.IsVisible = false;
            text.FontSpec.IsBold = true;
            text.FontSpec.Size = 10;
            text.FontSpec.FontColor = Color.Black;
            text.FontSpec.IsItalic = true;
            text.FontSpec.StringAlignment = StringAlignment.Near;
            myPane.GraphObjList.Add(text);

			// Add the ContextMenyu builder callback function. 
			this._graph.ContextMenuBuilder += new ZedGraphControl.ContextMenuBuilderEventHandler(MyContextMenuBuilder);

            // Show tooltips when the mouse hovers over a point
            this._graph.IsShowPointValues = true;
            this._graph.PointValueEvent += new ZedGraphControl.PointValueHandler(MyPointValueHandler);

            // Handle the Zoom Event
            this._graph.ZoomEvent += new ZedGraphControl.ZoomEventHandler(zedgraphControl_ZoomEventHandler);

            // Size the control to fit the window
            resizeGraph();

            // Tell ZedGraph to calculate the axis ranges
            // Note that you MUST call this after enabling IsAutoScrollRange, since AxisChange() sets
            // up the proper scrolling parameters
            this._graph.AxisChange();

            this._graph.BorderStyle = BorderStyle.None;
            // Make sure the Graph gets redrawn
            this._graph.Invalidate();
        }


        protected override void OnResize(EventArgs eventargs)
        {
            base.OnResize(eventargs);
            resizeGraph();
        }


		public void disableGraphAggregateFunction(GraphDataAggregateType disableFunc) {
			List<GraphDataAggregateType> aggrList = new List<GraphDataAggregateType>();

			foreach (GraphDataAggregateType item in this._graphDataAggregationFunctions) {
				if (item != disableFunc)
					aggrList.Add(item);
			}

			this._graphDataAggregationFunctions = aggrList.ToArray();
		}


		public void EnableScrollbars(bool horizontalScrolling,
									 bool verticalScrolling,
									 bool zLevelScrolling) {

			if (!horizontalScrolling  &&  !verticalScrolling  &&
				!zLevelScrolling)
				return;


			// Enable scrollbars if needed
			this._graph.IsShowHScrollBar = horizontalScrolling;
			this._graph.IsShowVScrollBar = verticalScrolling;
			this._graph.IsScrollY2 = zLevelScrolling;
			
			this._graph.IsAutoScrollRange = true;

		}

        public void addPointToGraph(double x, double y)
        {
            if (this._graph.InvokeRequired)
            {
                addPointCallback d = new addPointCallback(addPointToGraph);
                this.Invoke(d, new object[] { x, y });
            }
            else
            {
                CurveItem c = this._graph.GraphPane.CurveList["MetricCurve"];
				try {
					IPointListEdit list = c.Points as IPointListEdit;
					list.Add(x, y);

				} catch (Exception) {
					c.AddPoint(x, y);
				}

				this._graph.AxisChange();
                this._graph.Invalidate();
                // this._graph.Refresh();
                // this._graph.GraphPane.AxisChange(_graph.CreateGraphics() );
            }


        }

		delegate void refreshGraphCallback(bool flag);
		public void refreshGraph(bool flag) {
			if (this._graph.InvokeRequired) {
				this.Invoke(new refreshGraphCallback(refreshGraph), new object[] { flag } );
			} else {
				if (flag) {
					this._graph.RestoreScale(this._graph.GraphPane);
					this._graph.Refresh();
				}

				resizeGraph();
				this._graph.AxisChange();

				if (flag)
					this._graph.Invalidate();

			}
		}

		public void refreshGraph() {
			refreshGraph(true);
			// refreshGraph(false);
		}


		public void addPointList(IPointList pts) {
			double minXAxisValue = 0;
			if (0 < pts.Count)
				minXAxisValue = pts[0].X;

			addPointList(pts, minXAxisValue, false);
		}


		delegate void addPointListCallback(IPointList pts, double  minXAxisValue, bool fillCurve);
		public void addPointList(IPointList pts, double minXAxisValue, bool fillCurve)
		{
			if (this._graph.InvokeRequired) {
				addPointListCallback d = new addPointListCallback(addPointList);
				this.Invoke(d, new object[] { pts, minXAxisValue, fillCurve });
			}
			else {
				this._graph.Visible = false;
				CurveItem c = this._graph.GraphPane.CurveList["MetricCurve"];
				this._graph.GraphPane.CurveList.Remove(c);


				Color curveColor = Color.Red;
				if (fillCurve)
					curveColor = Color.Green;

				LineItem myCurve = this._graph.GraphPane.AddCurve("MetricCurve", pts, curveColor);
				if (fillCurve)
					myCurve.Line.Fill = new Fill(Color.White, curveColor, 45F);

				myCurve.Label.IsVisible = false;

				// make the x axis scale minimum is at the minimum data value
				this._graph.GraphPane.XAxis.Scale.Min = minXAxisValue;

				// Make the symbols opaque by filling them with Mocassin
				myCurve.Symbol.Fill = new Fill(Color.Moccasin);

				this._graph.Refresh();

				resizeGraph();
				this._graph.AxisChange();
				this._graph.Invalidate();
				this._graph.Visible = true;
			}
		}


		public void addPoints(double[] x, double[] y) {
			double minXAxisValue = 0;
			if (0 < x.Length)
				minXAxisValue = x[0];

			addPoints(x, y, minXAxisValue, false);
		}


		public void addPoints(double[] x, double[] y, double minXAxisValue) {
			addPoints(x, y, minXAxisValue, false);
		}

		delegate void addPointsCallback(double[] x, double[] y, double minXAxisValue, bool fillCurve);
		public void addPoints(double[] x, double[] y, double minXAxisValue, bool fillCurve)
		{
			int limit = x.Length;
			if (x.Length != y.Length)
			{
				limit = Math.Min(x.Length, y.Length);
			}

			if (this._graph.InvokeRequired)
			{
				addPointsCallback d = new addPointsCallback(addPoints);
				this.Invoke(d, new object[] { x, y, minXAxisValue, fillCurve});
			}
			else
			{
				RollingPointPairList rollingPtList = new RollingPointPairList(new PointPairList(x, y));
				addPointList(rollingPtList, minXAxisValue, fillCurve);
			}

		}


        public void resizeWidget(double wFactor, double hFactor)
        {
			//base.resize(wFactor, hFactor);

			/*
			int xPos = base.Location.X - this.Parent.Location.X ; 
			int yPos = base.Location.Y - this.Parent.Location.Y;

			base.Location = new Point((int)(xPos * wFactor), (int)(yPos * hFactor));
			 */

            int width = base.Size.Width;
            int height = base.Size.Height;
            //base.Size = new System.Drawing.Size((int)(width * wFactor),
            //                                           (int)(height * hFactor));

            resizeGraph();
        }

        public void resizeGraph()
        {
            int xPadding = base.Padding.Left;
            int yPadding = base.Padding.Top;

            this._graph.Location = new Point(xPadding, yPadding);
            // Leave a small margin around the outside of the control
            this._graph.Size = new Size(
                base.Width - this._graph.Location.X - xPadding,
                base.Height - this._graph.Location.Y - yPadding);


			// this._graph.GraphPane.XAxis.Title.FontSpec.Size = 20;
			// this._graph.GraphPane.YAxis.Title.FontSpec.Size = 20;

        }

        /// <summary>
        /// Display customized tooltips when the mouse hovers over a point
        /// </summary>
        private string MyPointValueHandler(ZedGraphControl control, GraphPane pane,
                        CurveItem curve, int iPt)
        {
            // Get the PointPair that is under the mouse
			PointPair pt = curve[iPt];

			if (null != this._pointValueHandler) {
				return this._pointValueHandler.PointValueEvent(pt);
			}

			XDate xd = new XDate(pt.X);
			String formatted_Y_Value = "";
			try {
				formatted_Y_Value = String.Format(TriageHelper.getNumberFormatForCurrentLocale(2), pt.Y);
			} catch(Exception) {
				formatted_Y_Value = pt.Y.ToString("f2");
			}

			String counterName = base.Text;
            if (0 >= base.Text.Length)
                counterName = curve.Label.Text;
            else if (1 < pane.CurveList.Count)
                counterName = base.Text + " [" + curve.Label.Text + "]";

			return  counterName + " = " + formatted_Y_Value + this._graphMetricUnit + 
					"  at  " + xd.ToString();

        }

		private bool alreadyZooming = false;

        // Respond to a Zoom Event
        private void zedgraphControl_ZoomEventHandler(ZedGraphControl control, ZoomState oldState,
													  ZoomState newState)
        {
            
            if (this._graph.GraphPane.ZoomStack.IsEmpty) 
            {
                //When User Click the Zoom-out button, while the old state is Emplty, let the graph refresh instead of going to an Empty State :)
                refreshGraph();
            }                
            
            if (!this._allowDrillDown)
				return;
             
            //if (alreadyZooming)
            //{
            //    alreadyZooming = false;
            //    return;
            //}
            //alreadyZooming = true;

            // Here we get notification everytime the user zooms
            double x0, y0, z0;
            PointF pf0 = new PointF(0, 0);
            this._graph.GraphPane.ReverseTransform(pf0, out x0, out y0, out z0);

            double x1, y1, z1;
            PointF pf1 = this._graph.GraphPane.Rect.Size.ToPointF();
            this._graph.GraphPane.ReverseTransform(pf1, out x1, out y1, out z1);

            control.ZoomOut(this._graph.GraphPane);

			String  startTime = ((XDate) x0).ToString();
			String  endTime   = ((XDate) x1).ToString();

            //if (null == this.View)
            //    return;

#if DEBUG
            //this.View.Workspace.setStatus("DEBUG : You are zooming into time: " +
            //                         startTime + " TO " + endTime +
            //                         "  [metric between " + y0 + " - " + y1 +
            //                         "] ");
#else
			//this.View.Workspace.setStatus(" Lassoed time range is FROM " +
										  //startTime + " TO " + endTime + " ");
#endif

            //this.View.Workspace.populateGridData();
            //triage
            //this.View.ActiveWidget = this;
            //DialogResult result = MessageBox.Show("\nThe time range you selected and are drilling down on is : \n" + 
            //                                      "\tFrom:  " + startTime + "\n" + 
            //                                      "\t    To:  " + endTime + "\n\n" + 
            //                                      "Do you wish to retrieve the associated queries that were running \n" +
            //                                      "on the Trafodion Platform during the selected time range?\n\n",
            //                                      "Time Range Drill-Down", MessageBoxButtons.YesNo,
            //                                      MessageBoxIcon.Question);
            //if (DialogResult.Yes == result)
            //    this.View.Workspace.invokeTriage(startTime, endTime, this);

        }


        //private void InitializeComponent()
        //{
        //    this.SuspendLayout();
        //    this.ResumeLayout(false);

        //}


		private void addGraphDataAggregateFunctionMenuItems(ContextMenuStrip menuStrip) {
			if (false == this._allowAggregateFunctions)
				return; 


			// Create a new menu item.
			ToolStripMenuItem menuItem = new ToolStripMenuItem("Aggregate Function");

			// Give it a tag.
			menuItem.Tag = "GraphDataAggregateFunction";

			ToolStripMenuItem currentItem = null;

			// Build the list of aggregate Functions and add 'em to this menu item.
			foreach (GraphDataAggregateType dataAggrType in _graphDataAggregationFunctions) {
				ToolStripMenuItem functionItem = new ToolStripMenuItem();
				String displayName = getAggregateFunctionType(dataAggrType);

				functionItem.Tag = dataAggrType;
				functionItem.Text = displayName;

				if (this._dataAggregateType == dataAggrType)
					currentItem = functionItem;

				functionItem.Click += new EventHandler(dataAggregateFunctionMenuItem_Click);

				menuItem.DropDownItems.Add(functionItem);
			}

			//  Check/select the current in use data aggregation function.
			if (null != currentItem)
				currentItem.Checked = true;

			//  Finally add the data aggregate function option to the right click menu.
			menuStrip.Items.Add(menuItem);
		}


		public void MyContextMenuBuilder(ZedGraphControl control, ContextMenuStrip rightClickMenu,
										  Point pt) {

			addGraphDataAggregateFunctionMenuItems(rightClickMenu);

			foreach (ToolStripItem menuItem in rightClickMenu.Items) {
				String tag = (String)menuItem.Tag;
				if (null != tag) {
					if ("set_default".Equals(tag.Trim() )) {
						menuItem.Enabled = this._allowSetScaleToDefault;
						menuItem.Visible = this._allowSetScaleToDefault;
					} else if ("undo_all".Equals(tag.Trim())) {
						menuItem.Enabled = false;
						menuItem.Visible = false;
					} else if("unzoom".Equals(tag.Trim()))
                    {
                        //menuItem.Enabled = false;
                        //menuItem.Visible = true;
                    }
				}

			}

		}

		void dataAggregateFunctionMenuItem_Click(object sender, EventArgs e) {
			ToolStripMenuItem menuItem = (ToolStripMenuItem) sender;

			try {
				GraphDataAggregateType selectedAggregateType = (GraphDataAggregateType)menuItem.Tag;
				if (selectedAggregateType == this._dataAggregateType)
					return;

				this.DataAggregateType = selectedAggregateType;

			} catch (Exception exc) {
                Trafodion.Manager.Framework.Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "Warning: Ignoring missing tag for Graph Data Aggregation Type. " +
                                                "Details = " + exc.Message);

			}

            //NCCWorkspaceView  view = base.View;
            //if (null != view)
            //    view.refreshGraphWidget(this);

		}


        private enum QueryState
        {
            ALL = 999, QUERY_RUNNING = 1, QUERY_COMPLETED = 0,
            STATE_UNKNOWN = 9, QUERY_HAD_ERRORS = -1
        };

        private PointPairList getCandleStickPoints(DataTable dt, QueryState filterState)
        {
            String queryFilter = "STATE = 'Completed'";

            switch (filterState)
            {
                case QueryState.QUERY_RUNNING:
                    queryFilter = "STATE = 'Running'";
                    break;

                case QueryState.QUERY_HAD_ERRORS:
                    queryFilter = "STATE = 'Error'  OR  " +
                                  "(STATE = 'Completed' " +
                                  " AND  (TRIM(ERROR_CODE) <> '' AND ERROR_CODE <> 0)) \n";
                    break;

                case QueryState.QUERY_COMPLETED:
                    queryFilter = "STATE = 'Completed'  AND  (TRIM(ERROR_CODE) = ''  OR  ERROR_CODE = 0) \n";
                    break;

                case QueryState.STATE_UNKNOWN:
                    queryFilter = "STATE = 'Abnormally Terminated'";
                    break;

                case QueryState.ALL:
                default:
                    queryFilter = " (1 = 1) ";
                    break;
            }

            PointPairList ipts = new PointPairList();
            int idx = 1;
            long tSec = TimeSpan.TicksPerSecond;

            //bool showControl = nccTriageShowControlCheckBox.Checked;
            bool showControl = true;

            if (null == dt)
                return ipts;

            DataRow[] graphTableRows = dt.Select(queryFilter, "START_TIME");
            foreach (DataRow row in graphTableRows)
            {
                try
                {
                    String qid = (String)row["QUERY_ID"];
                    String qryState = (String)row["STATE"];
                    String entryID = "";
                    String qryType = "";

                    if (true)
                    {
                        entryID = row["ENTRY_ID"].ToString().Trim();
                        qryType = (String)row["SQL_TYPE"];

                        String htKey = qid.Trim() + "^_^" + entryID;
                        if ((_theTriageHelper != null) && (_theTriageHelper.HiddenQIDs_ht.ContainsKey(htKey)))
                            continue;

                        if (!showControl &&
                            ("CONTROL".Equals(qryType) || "SHOWCONTROL".Equals(qryType) ||
                             "SHOWSHAPE".Equals(qryType)))
                            continue;

                        /* ||
                         "DDL [CREATE VOLATILE]".Equals(qryType) ))
                        continue;
                         */
                    }


                    DateTime startTime = (DateTime)row["START_TIME"];
                    TimeSpan executionTime = TimeSpan.MinValue;
                    try
                    {
                        String elapsedTime = (String)row["ELAPSED_TIME"];
                        if ((null != elapsedTime) && (0 < elapsedTime.Trim().Length))
                            executionTime = TimeSpan.Parse(elapsedTime);
                        else
                            executionTime = DateTime.Now.ToUniversalTime().Subtract(startTime);
                    }
                    catch (Exception)
                    {
                        executionTime = DateTime.Now.ToUniversalTime().Subtract(startTime);
                    }

                    if ((null != qryState) &&
                        "Abnormally Terminated".Equals(qryState.ToUpper()))
                        executionTime = TriageHelper.TWELVE_HOURS;

                    //This method is to get the actual endTime value, especially millisecond value. Because defalut XDate construct will omit 
                    //milliseconds value, and it will cause later calculation elapsed time not precise.
                    DateTime tempGMTStartTime = getDisplayTimeFromGMT(startTime);
                    XDate qryStartTime;
                    qryStartTime = new XDate(tempGMTStartTime.Year, tempGMTStartTime.Month, tempGMTStartTime.Day, tempGMTStartTime.Hour, tempGMTStartTime.Minute, tempGMTStartTime.Second, tempGMTStartTime.Millisecond);
                    
                    TimeSpan ticks = new TimeSpan(executionTime.Ticks);                    
                    
                    DateTime endTime = new DateTime();
                    
                    if (row["END_TIME"] is DateTime)
                    {
                        endTime = (DateTime)row["END_TIME"];
                    }
                    else 
                    {
                        //Here EndTime is used for late to calculate Elspased Time - (EndTime-StartTime), and the actual EndTime will diaplay blank 
                        //as state is running
                        endTime = startTime.Add(ticks); 
                    }
                    //This method is to get the actual endTime value, especially millisecond value. Because defalut XDate construct will omit 
                    //milliseconds value, and it will cause later calculation elapsed time not precise.
                    DateTime tempGMTEndTime = getDisplayTimeFromGMT(endTime);
                    XDate qryEndTime;
                    qryEndTime = new XDate(tempGMTEndTime.Year, tempGMTEndTime.Month, tempGMTEndTime.Day, tempGMTEndTime.Hour, tempGMTEndTime.Minute, tempGMTEndTime.Second, tempGMTEndTime.Millisecond);


                    ipts.Add((double)idx, (double)qryEndTime, (double)qryStartTime, qid);
                    idx++;
                }
                catch (Exception)
                {
                }

            }

            return ipts;
        }

        public DateTime getDisplayTimeFromGMT(DateTime t)
        {
            if (this._timeOffsetGMTAdd)
                return t.Add(this._displayTimeOffsetFromGMT);

            return t.Subtract(this._displayTimeOffsetFromGMT);
        }


        public void UpdateGraphWidget(DataTable filteredDataTable)
        {
            try
            {
                Hide();
                updateMeisterGraph(filteredDataTable);

            }
            finally
            {
                Show();
            }

        }


        private void updateMeisterGraph(DataTable graphDataTable)
        {
            CurveItem c = GraphPane.CurveList["MetricCurve"];
            GraphPane myPane = GraphPane;

            if (null != c)
                myPane.CurveList.Remove(c);

            // Set up the Title and X and Y Axis.
            myPane.Title.Text = "Queries Candlestick Graph";
            myPane.XAxis.Title.Text = "Queries";
            myPane.YAxis.Title.Text = "Execution Time";

            // Set the XAxis to linear type.
            myPane.XAxis.Type = ZedGraph.AxisType.Linear;
            myPane.XAxis.Scale.MajorStep = 10;
            myPane.XAxis.Scale.MinorStep = 1;
            // myPane.XAxis.MinSpace = 5;

            myPane.XAxis.IsVisible = false;
            myPane.XAxis.Scale.FormatAuto = true;
            myPane.XAxis.Scale.Format = "F";

            // Set the YAxis to date type
            myPane.YAxis.Type = ZedGraph.AxisType.Date;

            // Y axis step size is 1 Hour
            myPane.YAxis.Scale.MajorStep = 1;
            myPane.YAxis.Scale.MinorUnit = DateUnit.Second;
            myPane.YAxis.Scale.MajorUnit = DateUnit.Hour;

            // tilt the x axis labels to an angle of 65 degrees
            myPane.YAxis.Scale.FontSpec.IsBold = true;
            myPane.YAxis.Scale.FontSpec.Size = 11;
            myPane.YAxis.Scale.Format = "t";

            EnableScrollbars(true, false, false);

            // Allow horizontol pan and zoom
            ZedGraphControl.IsEnableHPan = true;
            ZedGraphControl.IsEnableHZoom = true;

            // Vertical pan and zoom not allowed
            ZedGraphControl.IsEnableVPan = false;
            ZedGraphControl.IsEnableVZoom = false;
            //ZedGraphControl.IsShowVScrollBar = true;

            ZedGraphControl.ZoomOutAll(myPane);

            PointPairList allQueryPoints = getCandleStickPoints(graphDataTable, QueryState.ALL);

            c = GraphPane.CurveList["QueriesBarGraph"];
            if (null != c)
                myPane.CurveList.Remove(c);

            // ShowSetScaleToDefaultOption = true;

            HiLowBarItem barCurve = myPane.AddHiLowBar("QueriesBarGraph", allQueryPoints, Color.Green);
            // Fill the bar with a red-white-red gradient for a 3d look
            barCurve.Bar.Fill = new Fill(Color.Green, Color.White, Color.Green, 0);

            // Make the bar width based size in points
            barCurve.Bar.IsAutoSize = false;
            barCurve.Bar.Size = 15;

#if CLUSTERBARGRAPH
			GraphPane.BarSettings.ClusterScaleWidthAuto = true;
			myPane.BarSettings.MinClusterGap = .03f;  //  Space between each cluster of bars. 
			myPane.BarSettings.MinBarGap = .1f;       //  Space between each bars.
#endif

            barCurve.Label.IsVisible = false;

            GraphPane.BarSettings.Type = BarType.ClusterHiLow;

            if (null == PointValueEventHandler)
            {
                TriageGraphEventHandler th = new TriageGraphEventHandler(_theTriageHelper);
                PointValueEventHandler = th;
            }


            try
            {
                if (null != graphDataTable)
                {
                    DataRow[] rows = graphDataTable.Select("1 = 1", "START_TIME");
                    if (rows.Length > 0)
                    {
                        DateTime startTime = (DateTime)rows[0]["START_TIME"];
                        XDate qryStartTime = new XDate(getDisplayTimeFromGMT(startTime).ToUniversalTime());
                        myPane.YAxis.Scale.Min = (double)qryStartTime;
                    }


                    DateTime endTime = DateTime.Now;
                    rows = graphDataTable.Select("END_TIME > #01/01/0001 00:00:00#", "END_TIME");
                    if (rows.Length > 0)
                        endTime = (DateTime)rows[rows.Length - 1]["END_TIME"];


                    XDate qryEndTime = new XDate(getDisplayTimeFromGMT(endTime).ToUniversalTime());
                    myPane.YAxis.Scale.Max = (double)qryEndTime;
                }

            }
            catch (Exception)
            {
            }

            // Fill the axis background with a color gradient
            // myPane.Chart.Fill = new Fill( Color.White, Color.FromArgb( 255, 255, 166), 45.0F );

            // myPane.XAxis.ResetAutoScale(myPane, CreateGraphics());

            int pixels = (int)(Width * 0.95);  // Use 95% for now.
            int maxQueries = (int)pixels / 20;

            int minIndex = 0;
            int maxIndex = Math.Min(maxQueries, Math.Max(0, allQueryPoints.Count - 1));
            // minIndex = 0; // allQueryPoints.Count - maxQueries;
            //int maxIndex = maxQueries; // allQueryPoints.Count - 1;
            if (minIndex < allQueryPoints.Count)
                myPane.XAxis.Scale.Min = 0;

            if (maxIndex < allQueryPoints.Count)
                myPane.XAxis.Scale.Max = maxIndex;


            Text = "Triage Space Candlestick Graph";
            Refresh();
            refreshGraph();
            

        }

        #region MS Chart related code
        
        #endregion

    }

    public abstract class GraphEventHandler
    {
        public abstract String PointValueEvent(PointPair pt);
    }
}
