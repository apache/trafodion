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
using System.Drawing;
using System.Timers;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{


    public partial class TrafodionRealTimeLineGraph : UserControl
    {
           /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.Container components = null;


        private int brushCount;

        private ArrayList graphValues = new ArrayList();
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
                this.Refresh();
            }
        }
        private System.Timers.Timer RefreshClock = null;
        private int timelineRange = 50;
        private int nDataSeparation = 0;

        private int numScrolled = 0;
        private int scrollThreshold = 1;
        private int scrollModifier = 1;

        public int GraphMaxRange
        {
            get { return timelineRange; }
            set { timelineRange = value; }
        }

        public void AddGraphValue(float graphValue)
        {
            if (this.graphValues == null)
            {
                this.graphValues = new ArrayList();
            }
            this.graphValues.Add(graphValue);
            if (this.graphValues.Count > GraphMaxRange)
            {
                this.graphValues.RemoveAt(0);
            }
            
            this.numScrolled++;
            if (this.numScrolled > scrollThreshold)
            {
                this.numScrolled = 0;
            }
            //Baaad cross threading!
            //this.Refresh();
        }

        //For adding an array of floats to the graph
        public void AddGraphValues(ArrayList graphValueArray)
        {
            if (this.graphValues == null)
            {
                this.graphValues = new ArrayList();
            }
            /*
            for(int i=0;i<graphValueArray.Count;i++)
            {
                if (this.graphValues[i] == null)
                    this.graphValues.Add(new ArrayList());
            }*/


            this.graphValues.Add(graphValueArray);
            if (this.graphValues.Count > GraphMaxRange)
            {
                this.graphValues.RemoveAt(0);
            }

            this.numScrolled++;
            if (this.numScrolled > scrollThreshold)
            {
                this.numScrolled = 0;
            }
        }



        private Pen graphColor  = new Pen(Color.Black);
        /// <summary>
        /// Exposed to allow parent to define what brushes to use to draw data items.
        /// This array should be as long as the data array to avoid repeating colors.
        /// </summary>
        public Pen GraphColor
        {
            get { return graphColor; }

            set
            {
                graphColor = value;
                this.Refresh();
            }
        }
       
        private ArrayList graphColors = new ArrayList();
        public ArrayList GraphColors
        {
            get { return graphColors; }
            set { graphColors = value;
                  //this.Refresh();
                }
        }
        

        /// <summary>
        /// Default constructor creates a GraphDisplay control with a GraphType of PieChart.
        /// </summary>
        public TrafodionRealTimeLineGraph ()
        {
            // This call is required by the Windows.Forms Form Designer.
            InitializeComponent();

            RefreshClock = new System.Timers.Timer();
            RefreshClock.Elapsed += new ElapsedEventHandler(RefreshClock_Elapsed);
            RefreshClock.Interval = 40;
            RefreshClock.Start();
        }

        void RefreshClock_Elapsed(object sender, ElapsedEventArgs e)
        {
            //RefreshClock.Stop();
            //this.recordingCount += this.RefreshClock.Interval;
            this.Invalidate();
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
            this.SuspendLayout();
            // 
            // TrafodionRealTimeLineGraph 
            // 
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.DoubleBuffered = true;
            this.Margin = new System.Windows.Forms.Padding(0);
            this.Name = "TrafodionRealTimeLineGraph ";
            this.ResumeLayout(false);

        }
        #endregion

        protected override void OnPaint(PaintEventArgs e)
        {
            Graphics g = e.Graphics;
            DrawBarGraph(g);
            DrawScaleLines(g);
        }


        //Function for drawing Scale lines on the Chart
        private void DrawScaleLines(Graphics g)
        {
                int numLines = 10;
                float distanceBetwixt = ((float)this.Height / (float)numLines);
                Pen objVerticalPen = new Pen(Color.Silver);

                // Draw horizontal scale lines 
                /********************************************************************/
                for (int i = 0; i < numLines; i++)
                {
                    g.DrawLine(objVerticalPen, 0.0f, (i * distanceBetwixt),
                   (float)this.Width, (i * distanceBetwixt));
                }
                /********************************************************************/
    
            
                //Draw 'slightly darker' half-way mark
                /********************************************************************/
                  g.DrawLine(new Pen(Color.Gray), 0.0f, ((float)this.Height / 2.0f),
                  (float)this.Width, ((float)this.Height / 2.0f));
                /********************************************************************/


                //Write Scale Indicators
               // g.DrawString("50 %", Font, Brushes.Black, new PointF(0, 0));
                
             float startPosition = (this.Width - (this.nDataSeparation * this.numScrolled));// -this.nDataSeparation;

                float HDistanceBetwixt = this.nDataSeparation*2; //10;//((float)this.Width / 10.0f);
                if (HDistanceBetwixt <= 0)
                {
                    HDistanceBetwixt = 1;

                }
            //If the distance between the horizontal lines is 'small'
                //then double the distance and adjust the scale numbers
                if (HDistanceBetwixt < 8)
                {
                    this.scrollModifier = 2;
                    this.scrollThreshold = 3;//this.scrollModifier * 2;
                }
                else
                {
                    if (this.scrollThreshold > 1)
                    {
                        if (((int)this.scrollModifier / 2) == ((float)this.scrollModifier / 2.0f))
                        {
                            this.scrollModifier = 1;
                        }
                        else
                        {
                            this.scrollModifier = 0;
                        }
                    }
                    this.scrollModifier = 1;
                    this.scrollThreshold = 1;
                }




                for (float j = startPosition; j >= 0; j -= (HDistanceBetwixt*this.scrollModifier))
                {
                    float currentPos = (j);
                    //Draw Segment Seperator
                    g.DrawLine(objVerticalPen, currentPos, 0.0f,
                     currentPos, this.Height);
                }
                //dispose the pen
                objVerticalPen.Dispose();
        }




        /// <summary>
        /// Called by the OnPaint handler, this function uses the control's Graphics
        /// object to draw a bar graph of the data supplied to the GraphValues property.
        /// </summary>
        /// <param name="g">The PaintEventArgs.Graphics object from the OnPaint handler.</param>
        private void DrawBarGraph(Graphics g)
        {
            Pen objVerticalPen = null;
            try
            {
                //The range of the line graph
                //this.timelineRange = 50;

                // Calculate the distance between data points.
                this.nDataSeparation = (int)(((float)this.Width / (float)timelineRange) + .5);//(this.Size.Width / nCount);

                //check if values exist
                if (graphValues == null)
                {
                    return;
                }
                else if (graphValues.Count == 0)
                {
                    return;
                }

                //define bars

                int numBars = graphValues.Count;
                int total = 1;
                float percentage;
                int barSpacing;
                int currentHorizontalPosition = 0;  //start at edge of control
                Rectangle bar;
                int barHeight;
                int barWidth;
                int barY;

                // Get the count into a local variable
                //   so that it's a little easier and the
                //   code is more readable.


                // The panel height is used to calculate the
                //   y coordinate for drawing.
                int nPanelHeight = this.Size.Height - 1;

                // We'll use this as an offset into the panel
                //   since we want some space from the left margin.
                int nXOffset = (this.Width);//nDataSeparation / 2;


                // Create the two pens we'll use. We don't want
                //   to create each one for every loop iteration
                //   since that will be a performance slow down.
                objVerticalPen = new Pen(Color.Silver);


                // Loop through the data.
                int nCount = this.GraphValues.Count;
                for (int i = nCount; i > 0; i--)
                {
                    for (int j = 0; j < ((ArrayList)this.GraphValues[0]).Count; j++)
                    {
                        // If this isn't the last data item,
                        //   draw the connecting line to the next point.
                        if (i > 1)
                        {
                            PointF CurrentPoint = new PointF(
                                (float)nXOffset - ((float)(nCount - i) * nDataSeparation),
                                (float)nPanelHeight - ((float)nPanelHeight * (float)((ArrayList)graphValues[i - 1])[j]));

                            PointF NextPoint = new PointF(
                                (float)nXOffset - ((float)(nCount - i + 1) * nDataSeparation),
                               (float)nPanelHeight - ((float)nPanelHeight * (float)((ArrayList)graphValues[i - 2])[j]));


                            //Get the appropriate color from the color array                   
                            g.DrawLine(GetPen(j), CurrentPoint, NextPoint);
                        }
                    }
                }
            }
            catch (Exception e)
            {

            }
            finally
            {
                if (objVerticalPen != null)
                {
                    objVerticalPen.Dispose();
                }
            }
        }


        /// <summary>
        /// This method provides access to various types (colors) of brushes
        /// for the internal drawing methods.
        /// </summary>
        /// <returns>Brush object</returns>
        private Pen GetPen(int aColorIndex)
        {
            try {
                return ((Pen)this.graphColors[aColorIndex]);
            }
            catch (Exception e) {
                return this.graphColor;
            }
        }

    }
}
