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
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Timers;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public struct BarClickArgs
    {        
        private int _segmentClicked;
        private int _cpuClicked;

        public int SegmentClicked
        {
            get { return _segmentClicked; }
        }

        public int CpuClicked
        {
            get { return _cpuClicked; }
        }

        public BarClickArgs(int aSegment, int aCPU)
        {
            this._segmentClicked = aSegment;
            this._cpuClicked = aCPU;
        }
    }

    public partial class TrafodionRealTimeBarGraph : UserControl
    {
              private IContainer components;

              private System.Timers.Timer RefreshClock = null;
        private Hashtable[] stanThreshArray = new Hashtable[3];
        public Hashtable[] StanThreshArray
        {
            get { return stanThreshArray; }
            set { 
                stanThreshArray = value;            
            }
        }


        private bool firstRun = true;
        private int numSegments = 0;
        private float segmentWidth = 0;
        private int changeSpeed = 20;
        public int chartMaxValue = 100;
        private bool isMouseOver = false;
        
        public bool smoothMove = true;

        public float segmentSeperatorOffset = 2;
        private int BAR_BUFFER = 1;
        private bool showSegmentSeperator = true;
        private bool thresholdExceededIndicator = true;
        float barWidth = 0;
        float BarWidthModifier = 0;
        int numBars = 0;
        int selectedBar = -1;
        int selectedCPU = -1;
        int selectedSegment = -1;

        private int chartRawMaxValue = 1;

        //Tooltip fields
        string toolTipText = "";
        private bool fancyTooltips = false;
        private int toolTipFontSize = 10;

        //Metric related fields
        private string toolTipLabel = "";
        private string metricType = "";
        private string metricLabel = "";
        private string _metricUnitSuffix = "";
        private bool _metricIntervalBased = false;


        // event declaration 
        public delegate void ChangingHandler(object sender, BarClickArgs args);
        public event ChangingHandler MouseClickBar;


        public string MetricUnitSuffix
        {
            get { return _metricUnitSuffix; }
            set { _metricUnitSuffix = value; }
        }

        public bool IsIntervalBased
        {
            get { return _metricIntervalBased; }
            set { _metricIntervalBased = value; }
        }

        public bool FancyTooltips
        {
            get { return fancyTooltips; }
            set { fancyTooltips = value; }
        }

        public int ToolTipFontSize
        {
            get { return toolTipFontSize; }
            set { toolTipFontSize = value; }
        }

        public int ChartRawMaxValue
        {
            get { return chartRawMaxValue; }
            set { chartRawMaxValue = value; }
        }

        public string TooltipLabel
        {
            get { return toolTipLabel; }
            set { toolTipLabel = value; }
        }
        public string MetricType
        {
            get { return metricType; }
            set { metricType = value; }

        }
        public string MetricLabel
        {
            get { return metricLabel; }
            set { metricLabel = value; }
        }

        public bool ThresholdExceededIndicator
        {
            get { return thresholdExceededIndicator; }
            set { thresholdExceededIndicator = value; }
        }

        public bool ShowSegmentSeperator
        {
            get { return showSegmentSeperator; }
            set { showSegmentSeperator = value; }
        }

        public void AggregateData()
        {
            ArrayList brushArray = new ArrayList();               
            ArrayList temp = GraphValues;
            
            violatorCounts = new ArrayList();
            downCPUCounts = new ArrayList();
            highwaterCPUCounts = new ArrayList();

            ArrayList temp2 = new ArrayList();
            for (int c = 0; c < temp.Count; c++)
            {
                ArrayList segTemp = ((ArrayList)temp[c]);
                int segmentAverage = 0;
                int segmentTotal = 0;
                int segnumValues = 0;
                int violatorCount = 0;
                int downCPUCount = 0;
                int segmentHighWater = 0;

                //Loop through each CPU for this segment
                for (int j = 0; j < segTemp.Count; j++)
                {
                    //check if this CPU is considered 'DOWN'
                    if (!this.downCPUs.Contains(c + "," + j))
                    {
                        int nodeVal = (int)segTemp[j];
                        if (nodeVal >= 100)
                            violatorCount++;

                        if (segmentHighWater < (int)segTemp[j])
                            segmentHighWater = (int)segTemp[j];

                        segmentTotal += (int)segTemp[j];
                        segnumValues++;
                    }
                    else{
                        downCPUCount++;
                    }
                }

                //Calculate the average value for the segment
                segmentAverage = (segmentTotal / segnumValues);
                
                ArrayList newTemp = new ArrayList();
                newTemp.Add(segmentAverage);
                violatorCounts.Add(violatorCount);

                this.highwaterCPUCounts.Add(segmentHighWater);
                this.downCPUCounts.Add(downCPUCount);

                temp2.Add(newTemp);//newTemp);
            }          
            this.aggregatedValues = temp2;
        }


        private bool isDrilled = false;
        public bool IsDrilled
        {
            get { return isDrilled; }
            set
            {isDrilled = value;}
        }

        private bool aggregated = false;
        public bool Aggregate
        {
            get { return aggregated; }
            set {
                
                aggregated = value;
                this.ShowcaseSegment = -1;
                this.IsDrilled = false;
                }
        }

        private bool standardDeviation = false;
       public bool StanDeviate
       {
           get { return standardDeviation; }
           set { standardDeviation = value; }
       }
 
       private ArrayList aggregatedValues = null;
       private ArrayList violatorCounts = null;
       private ArrayList downCPUCounts = null;
       private ArrayList highwaterCPUCounts = null;


       private ArrayList oldGraphValues = null;

       private int showcaseSegment = -1;
       public int ShowcaseSegment
       {
           get { return showcaseSegment; }
           set { showcaseSegment = value; }
       }
       private ArrayList downCPUs = new ArrayList();
       public ArrayList DownCPUs
       {
           get { return downCPUs; }
           set { downCPUs = value; }
       }


       private ArrayList graphValues = null;
        /// <summary>
        /// Exposed to allow parent to define the values to graph.  When this property
        /// is assigned to, the control is invalidated and thus redrawn.
        /// </summary>
        public ArrayList GraphValues
        {
            get { return graphValues; }

            set
            {
                graphValues = value;
                if (this.Aggregate)
                    AggregateData();
            }
        }

        //The Brush used to color the drawn Bar
        private ArrayList graphBrushes = new ArrayList();
        private ToolTip toolTip1;//Brushes.BlueViolet;
        /// <summary>
        /// Exposed to allow parent to define what brushes to use to draw data items.
        /// This array should be as long as the data array to avoid repeating colors.
        /// </summary>
        public ArrayList GraphBrushes
        {
            get { return graphBrushes; }

            set
            {
                graphBrushes = value;
                //this.Refresh();
            }
        }


        private Brush selectedBrush = Brushes.RoyalBlue;

        public Brush SelectedBrush
        {
            get { return selectedBrush; }
            set { selectedBrush = value; }
        }


        private Brush threshExceededBrush = Brushes.DimGray;
        public Brush ThreshExceededBrush
        {
            get { return threshExceededBrush; }

            set
            {
                threshExceededBrush = value;
                this.Refresh();
            }
        }


        private Color segmentSepCol = Color.Black;
        public Color SegmentSepCol
        {
            get { return segmentSepCol; }

            set
            {
                segmentSepCol = value;
            }
        }
        private Brush cPUDownBrush = Brushes.Red;
        public Brush CPUDownBrush
        {
            get { return cPUDownBrush; }
            set { cPUDownBrush = value; }
        }


        private Brush defaultBrush = Brushes.HotPink;
        public Brush DefaultBrush
        {
            get { return defaultBrush; }

            set
            {
                defaultBrush = value;             
                this.Refresh();
            }
        }

        private Brush getBrush(int i)
        {

            if (GraphBrushes == null || GraphBrushes.Count-1 < i)
            {
                if (this.StanDeviate)
                {
                    return new SolidBrush(((Color)this.stanThreshArray[1]["Color"]));
                }

                //this is what you get for not definig a graph brush!
                return defaultBrush;
            }
            else
            {
                return ((Brush)GraphBrushes[i]);
            }
        }



        /// <summary>
        /// Default constructor creates a GraphDisplay control with a GraphType of PieChart.
        /// </summary>
        public TrafodionRealTimeBarGraph()
        {
            // This call is required by the Windows.Forms Form Designer.
            InitializeComponent();

            RefreshClock = new System.Timers.Timer();
            RefreshClock.Elapsed += new ElapsedEventHandler(RefreshClock_Elapsed);
            RefreshClock.Interval = 40;
            RefreshClock.Start();

            this.MouseMove  += new MouseEventHandler(GraphDisplay_MouseMove);
            this.MouseLeave += new EventHandler(GraphDisplay_MouseLeave);
            this.MouseClick += new MouseEventHandler(GraphDisplay_MouseClick);

            this.oldGraphValues = new ArrayList();
        }


        public void TheSystemMonitorConnection_DataRefresh(object sender, EventArgs ea)
        {
            string test  = ea.ToString();
            //
            //throw new NotImplementedException();
        }

        void RefreshClock_Elapsed(object sender, ElapsedEventArgs e)
        {
            //RefreshClock.Stop();
            //this.recordingCount += this.RefreshClock.Interval;
                this.Invalidate();   
        }

        void GraphDisplay_MouseClick(object sender, MouseEventArgs e)
        {
            if (MouseClickBar != null)
            {
                MouseClickBar(this, new BarClickArgs(this.selectedSegment, this.selectedCPU));
            }
        }

        void GraphDisplay_MouseLeave(object sender, EventArgs e)
        {
            this.selectedBar = -1;
            //throw new Exception("The method or operation is not implemented.");
        }

        void GraphDisplay_MouseMove(object sender, MouseEventArgs e)
        {
            barMouseOver(e.X, e.Y);

        }

        public void clearAggregate()
        {
            //this.aggregatedValues.Clear();
            this.oldGraphValues.Clear();
        }

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (components != null)
                {
                    components.Dispose();
                }
                RefreshClock.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code
        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.toolTip1 = new Trafodion.Manager.Framework.Controls.TrafodionToolTip(this.components);
            this.SuspendLayout();
            // 
            // toolTip1
            // 
            this.toolTip1.AutoPopDelay = 5000;
            this.toolTip1.InitialDelay = 0;
            this.toolTip1.OwnerDraw = true;
            this.toolTip1.ReshowDelay = 100;
            this.toolTip1.ShowAlways = true;
            this.toolTip1.UseAnimation = false;
            this.toolTip1.UseFading = false;
            this.toolTip1.Popup += new System.Windows.Forms.PopupEventHandler(this.toolTip1_Popup);
            this.toolTip1.Draw += new System.Windows.Forms.DrawToolTipEventHandler(this.toolTip1_Draw);
            // 
            // TrafodionRealTimeBarGraph
            // 
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.DoubleBuffered = true;
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "TrafodionRealTimeBarGraph";
            this.Size = new System.Drawing.Size(150, 167);
            this.ResumeLayout(false);

        }
        #endregion

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            //create a transparent background for the graph area
            g.FillRectangle(Brushes.Transparent, 0, 0, this.Width, this.Height);

            //give the graph area a border           
            DrawBarGraph(g);
            DrawScaleLines(g);
            
            //if (!isMouseOver)
            //{
            //    this.toolTip1.RemoveAll();//SetToolTip(this, this.toolTipText);           
            //}
            ////display the tooltip
            ////FIX THIS
            //if(isMouseOver)
            //this.toolTip1.SetToolTip(this, this.toolTipText);
            
            //isMouseOver = false;
        }


        private void barMouseOver(int mouseX, int mouseY)
        {

            float individualModifier = (this.BarWidthModifier / this.numBars);
            float intBarWidth = (this.barWidth + individualModifier);
            if (intBarWidth > 0)
            {
                try
                {
                    selectedBar = (int)(mouseX / intBarWidth);
                    //this.toolTip1.Active = true;
                    //this.toolTip1.ShowAlways = true;
                    int barsPerSeg = ((ArrayList)this.graphValues[0]).Count;

                    int gValueD1 = 0;
                    int gValueD2 = 0;

                    if (this.ShowcaseSegment >= 0)
                    {
                        gValueD1 = this.ShowcaseSegment - 1 ;
                        this.selectedSegment = this.showcaseSegment;
                        //barsPerSeg = 1;

                        gValueD2 = this.selectedBar;
                        this.selectedCPU = gValueD2;
                    }
                    else
                    {
                        gValueD1 = (Convert.ToInt16(Math.Floor(Convert.ToDecimal(selectedBar) / Convert.ToDecimal(barsPerSeg))));
                        gValueD2 = selectedBar - (gValueD1 * barsPerSeg);
                        this.selectedSegment = gValueD1 + 1;
                        this.selectedCPU = (gValueD2);

                        if (this.Aggregate)
                        {
                            this.selectedCPU = -1;
                            this.selectedSegment = selectedBar + 1;//gValueD2 + 1;

                        }
                    }

                    ArrayList segmentlist = ((ArrayList)this.graphValues[gValueD1]);
                    int value = (((int)segmentlist[gValueD2]) * chartRawMaxValue)/ 100;
                    string label = string.IsNullOrEmpty(toolTipLabel) ? metricLabel : toolTipLabel;
                    if (!string.IsNullOrEmpty(metricType) && metricType.Equals("M"))
                    {
                        this.toolTipText = "Node " + (gValueD1 + 1) + ": \n" + label + " = ~" + value + this._metricUnitSuffix;
                    }
                    else
                    {
                        this.toolTipText = "Node " + (gValueD1 + 1) + ", CPU " + (gValueD2) + ": \n" + label + " = ~" + value + this._metricUnitSuffix;
                    }
                    if (this.downCPUs.Contains((gValueD1) + "," + (gValueD2)))
                        {
                            this.toolTipText = "Node " + (gValueD1 + 1) + ", CPU " + (gValueD2) + ": \n" + "*CPU DOWN*";
                        }
                    
                    if (this.Aggregate)
                    {
                        if (!this.isDrilled)
                        {
                            value = (int)((ArrayList)(this.aggregatedValues[gValueD2]))[0] * chartRawMaxValue;
                            this.toolTipText = "Node " + (selectedBar + 1) + ": \n" + String.Format(this.metricLabel, "second") + " = ~" + value / 100;//segmentlist[gValueD2];
                            this.toolTipText += "\n  (Click for Node view)";
                        }
                    }

                    isMouseOver = true;
                    if (!toolTip1.GetToolTip(this).Equals(this.toolTipText))
                    {
                        this.toolTip1.SetToolTip(this, this.toolTipText);//.Tag = "huh";
                    }
                }
                catch { }
            }
            else {
                selectedBar = -1;           
            }
        }


        private void DrawScaleLines(Graphics g)
        {
            int numLines = 10;
            float distanceBetwixt = ((float)this.Height / (float)numLines);
            Pen objVerticalPen = new Pen(Color.Silver);

            // Draw horizontal scale lines 
            for (int i = 0; i < numLines; i++)
            {
                g.DrawLine(objVerticalPen, 0.0f, (i * distanceBetwixt),
        (float)this.Width, (i * distanceBetwixt));
            }

            //Draw 'slightly darker' half-way mark
            g.DrawLine(new Pen(Color.Gray), 0.0f, ((float)this.Height / 2.0f),
    (float)this.Width, ((float)this.Height / 2.0f));

            if (ShowSegmentSeperator)
            {
                for (int j = 1; j < numSegments; j++)
                {
                    float currentPos = (this.segmentWidth * j);
                    //Draw Segment Seperator
                    g.DrawLine(new Pen(segmentSepCol, 1.5f), currentPos, 0.0f,
                     currentPos, this.Height);
                }
            }
            //Dispose the pen once done
            objVerticalPen.Dispose();
        }


        public void ResetGraph()
        {
            this.firstRun = true;
            this.smoothMove = false;
        }

        public void QuickSetGraph()
        {
            this.firstRun = false;
            this.smoothMove = false;
        }


        /// <summary>
        /// Called by the OnPaint handler, this function uses the control's Graphics
        /// object to draw a bar graph of the data supplied to the GraphValues property.
        /// </summary>
        /// <param name="g">The PaintEventArgs.Graphics object from the OnPaint handler.</param>
        private void DrawBarGraph(Graphics g)
        {
            //check if values exist
            if (GraphValues == null || GraphValues.Count == 0)
            {
                return;
            }

            ArrayList ValuesToGraph = new ArrayList();
            int numBarsPerSegment = 0;
            if (this.Aggregate)
            {
                if (this.isDrilled && this.ShowcaseSegment > 0)
                {
                    try
                    {
                        ValuesToGraph.Clear();
                        ValuesToGraph.Add(GraphValues[ShowcaseSegment-1]);
                    }
                    catch
                    {
                        if (this.aggregatedValues == null)
                        {
                            this.AggregateData();
                        }
                        ValuesToGraph = this.aggregatedValues;
                    }
                }
                else
                {
                    if (this.aggregatedValues == null)
                    {
                        this.AggregateData();
                    }
                    ValuesToGraph = this.aggregatedValues;
                }
            }
            else {
                ValuesToGraph = GraphValues;

                this.numSegments = ValuesToGraph.Count;
            }

            //the "100% value for this metric
            int total = this.chartMaxValue;

            //Total Number of bars, segments, and bars per segments           
            numBarsPerSegment = ((ArrayList)ValuesToGraph[0]).Count;
            numSegments = ValuesToGraph.Count;
            numBars = numSegments * numBarsPerSegment;

            //Calculate the dimensions of each bar in relation to the dimensions of the window
            RectangleF bar;

            this.BAR_BUFFER = 1;
            BarWidthModifier = (numBars * this.BAR_BUFFER) + ((this.numSegments-1) * segmentSeperatorOffset);
            barWidth = ((float)((this.Width - BarWidthModifier) / (float)numBars));
            if (barWidth < 1)
            {
                segmentSeperatorOffset = 1;
                this.BAR_BUFFER = 0;
                BarWidthModifier = (numSegments - 1);
                barWidth = ((float)((this.Width - BarWidthModifier) / (float)numBars));
            }

            float barHeight = 0;
            float barX = 0;  //start at edge of control
            float barY = 0;

            //Track the width of each segment in order to draw dividing line
            this.segmentWidth = ((barWidth + BAR_BUFFER) * numBarsPerSegment)+segmentSeperatorOffset;

            try
            {
                //define bars
                for (int j = 0; j < numSegments; j++)
                {
                    int[] SegmentData = (int[])((ArrayList)ValuesToGraph[j]).ToArray(typeof(int));
                    
                    //determine bar spacing (edges of graph and bars all evenly spaced)
                    if ((oldGraphValues.Count) < (j + 1))
                    {
                        float[] newOldValues = new float[SegmentData.Length];
                        for (int p = 0; p < newOldValues.Length; p++)
                        {
                            newOldValues[p] = (float)(Convert.ToDouble(SegmentData[p])/100);
                        }
                        oldGraphValues.Add(newOldValues);
                    }

                    if (((float[])oldGraphValues[j]).Length < SegmentData.Length)
                    {
                        oldGraphValues[j] = new float[SegmentData.Length];
                        firstRun = true;
                    }


                    float[] oldSegmentData = ((float[])oldGraphValues[j]);           
                    for (int i = 0; i < SegmentData.Length; i++)
                    {
                        int dataPoint = SegmentData[i];
                        float percentage = (float)dataPoint / total;
                        float oldDistance = oldSegmentData[i] * this.Height;

                        barX += BAR_BUFFER;

                        Brush barBrush = this.defaultBrush;
                        if(this.StanDeviate && !this.isDrilled)
                        {
                          barBrush = this.getBrush(i);
                        }
                        if (this.aggregated && !this.StanDeviate && !this.isDrilled)
                        {
                            if(((int)this.violatorCounts[j]) > 0)
                            {
                                barBrush = this.threshExceededBrush;
                            }
                            if (((int)this.downCPUCounts[j]) > 0)
                            {
                                barBrush = this.cPUDownBrush;
                            }
                        }

                        if (firstRun)
                        {
                            barHeight = 0;
                            oldSegmentData[i] = 0.0f;
                        }
                        else //Not first run...don't reset
                        {
                                int toHeight = (int)(percentage * this.Height);
                                float distance = toHeight - oldDistance;
                                float moveSpeed = (float)(distance / this.changeSpeed);

                                if (!this.smoothMove || Math.Abs(moveSpeed) < .01)
                                { barHeight = toHeight; }
                                else
                                { barHeight = (oldDistance + moveSpeed); }

                                float oldPercentage = (barHeight / this.Height);


                                oldSegmentData[i] = oldPercentage;
                            if (this.downCPUs.Contains(j + "," + i))
                            {
                                barBrush = this.cPUDownBrush;
                            }
                            else if (oldPercentage >= 1 && (ThresholdExceededIndicator || this.isDrilled))
                            {
                                //If the threshold has been exceeded
                                barBrush = this.threshExceededBrush;
                            }

                        }

                        //Calculate the total number of 'bars' for the graph
                        int barCount = (i) + (j * SegmentData.Length);
                        
                        //If the user is currently moused over this bar
                        if (barCount == this.selectedBar)
                            barBrush = this.SelectedBrush;

                        barY = (float)this.Height - barHeight - 1.0f;  //ends just before the drawing area
                        bar = new RectangleF((float)barX, (float)barY, barWidth, barHeight);

                        if (this.aggregated && !this.StanDeviate && !this.isDrilled)
                        {
                            //Create a bar to represent the high-water mark of for this segment  
                            //int highWaterBarHeight = ((int)this.highwaterCPUCounts[j]) * this.Height;
                            //RectangleF highWaterBar = new RectangleF((float)barX, (float)barY, barWidth, highWaterBarHeight);
                            //g.FillRectangle(Brushes.Chocolate, highWaterBar);
                        }

                        g.FillRectangle(barBrush, bar);

                        barX += barWidth;
                    }
                    
                    if (numSegments > j + 1)
                    {

                        barX += this.segmentSeperatorOffset;
                    }

                    //Store old graph values
                    oldGraphValues[j] = oldSegmentData;
                }
            } 
            catch 
            { }

            firstRun = false;
            smoothMove = true;
        }


        // Determines the correct size for the ToolTip.
        private void toolTip1_Popup(object sender, PopupEventArgs e)
        {
            using (Font f = new Font("Tahoma", toolTipFontSize))
            {
                e.ToolTipSize = TextRenderer.MeasureText(toolTip1.GetToolTip(e.AssociatedControl), f);
            }
        }


        // Handles drawing the ToolTip.
        private void toolTip1_Draw(System.Object sender, System.Windows.Forms.DrawToolTipEventArgs e)
        {

            Brush SystemBrush = SystemBrushes.WindowText;

            if (fancyTooltips)
            {
                SystemBrush = SystemBrushes.ActiveCaptionText;
                // Draw the custom background. 
                e.Graphics.FillRectangle(SystemBrushes.ActiveCaption, e.Bounds);//newRec);//e.Bounds);
            }
            else
            {
               e.DrawBackground();
            }

                e.DrawBorder();
                // The using block will dispose the StringFormat automatically.
                using (StringFormat sf = new StringFormat())
                {
                    sf.Alignment = StringAlignment.Near;
                    sf.LineAlignment = StringAlignment.Near;
                    sf.HotkeyPrefix = System.Drawing.Text.HotkeyPrefix.None;
                    sf.FormatFlags = StringFormatFlags.NoWrap;
                    using (Font f = new Font("Tahoma", toolTipFontSize))
                    {
                        e.Graphics.DrawString(e.ToolTipText, f,
                            SystemBrush, e.Bounds, sf);
                    }
                }
            }
        // End Draw the ToolTip.
    }
}
