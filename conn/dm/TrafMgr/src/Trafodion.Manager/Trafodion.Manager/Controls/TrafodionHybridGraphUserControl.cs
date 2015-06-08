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
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    public partial class TrafodionHybridGraphUserControl : UserControl
    {
        private SystemMetric _activeSystemMetric;
        public delegate void ChangingHandler(object sender, MetricBarClickArgs args);
        public event ChangingHandler MouseClickBar;

        private string toolTipText = "";
        private int toolTipFontSize = 10;

        //Line Graph fields
        private ArrayList _rawDataArray = new ArrayList();
        private int _maxRecords = 50;
        private Dictionary<SystemMetric, bool> _settingDictionary = new Dictionary<SystemMetric, bool>();

        private ArrayList _activeSystemMetrics = new ArrayList();
        public Dictionary<SystemMetric, Pen> ColorDictionary = new Dictionary<SystemMetric, Pen>();
        // End Line Graph fields

        public SystemMetric ActiveSystemMetric
        {
            get { return _activeSystemMetric; }
            set { _activeSystemMetric = value; }
        }


        public string ToolTipText
        {
            get { return toolTipText; }
            set {
                toolTipText = value;
                this.toolTip1.SetToolTip(this.statusStrip7, toolTipText);
            }
        }

        public TrafodionHybridGraphUserControl()
        {
            InitializeComponent();
            this.toolTip1.Popup += new System.Windows.Forms.PopupEventHandler(this.toolTip1_Popup);
            this.toolTip1.Draw += new System.Windows.Forms.DrawToolTipEventHandler(this.toolTip1_Draw);
        }

        public TrafodionHybridGraphUserControl(SystemMetric aMetricType) : this()
        {
            this._activeSystemMetric = aMetricType;
            this.graphLabel_toolStripStatusLabel.Text = aMetricType.Label;


            if (aMetricType.LabelTooltip != null && aMetricType.LabelTooltip.Length > 0)
            {
                toolTipText = aMetricType.LabelTooltip;
                this.toolTip1.SetToolTip(this.statusStrip7, aMetricType.LabelTooltip);
            }
            else
            {
                toolTipText = aMetricType.Label;
                this.toolTip1.SetToolTip(this.statusStrip7, aMetricType.Label);
            }

            this.RealTimeGraph.MouseClickBar += new TrafodionRealTimeBarGraph.ChangingHandler(RealTimeGraph_MouseClickBar);
        }

        void RealTimeGraph_MouseClickBar(object sender, BarClickArgs args)
        {
            if (null != MouseClickBar)
                MouseClickBar(this, new MetricBarClickArgs(this._activeSystemMetric, args.SegmentClicked, args.CpuClicked));
        }

        public Trafodion.Manager.Framework.Controls.TrafodionRealTimeBarGraph RealTimeGraph
        {
            get { return barGraph_TrafodionRealTimeBarGraph; }
            set { barGraph_TrafodionRealTimeBarGraph = value; }
        }


        public Trafodion.Manager.Framework.Controls.TrafodionRealTimeLineGraph RealTimeLineGraph
        {
            get { return lineGraph_TrafodionRealTimeLineGraph; }
            set { lineGraph_TrafodionRealTimeLineGraph = value; }
        }

        public void AddGraphValues(Dictionary<string, float> valuesToAdd)
        {
            ArrayList temp = new ArrayList();
            temp.Add(valuesToAdd[ActiveSystemMetric.Type]);
            RealTimeLineGraph.AddGraphValues(temp);
        }

        public void UpdateGraphics()
        {
            //dispose of the old pens
            ArrayList newColorList = new ArrayList();
            foreach (SystemMetric activeSysMetric in this._activeSystemMetrics)
            {
                newColorList.Add(ColorDictionary[activeSysMetric]);
            }
            this.RealTimeLineGraph.GraphColors = newColorList;
        }


        public void SetAggregationType(SystemMetric aSystemMetric, bool enabled)
        {
            //Toggle the given System Metric
        }

        public void SetMetricVisibility(SystemMetric aSystemMetric, bool aEnabled)
        {
            //Toggle the given System Metric

            //If this metric IS active and we DON'T want it to be:
            if (this._activeSystemMetrics.Contains(aSystemMetric) && !aEnabled)
                this._activeSystemMetrics.Remove(aSystemMetric);

            //If this metric ISN'T active and we WANT it to be:
            if (!this._activeSystemMetrics.Contains(aSystemMetric) && aEnabled)
                this._activeSystemMetrics.Add(aSystemMetric);

            this.RealTimeGraph.GraphValues = new ArrayList();
            this.UpdateGraphics();
            this.UpdateGraphDataFromRaw();
        }

        public void UpdateRawData(Dictionary<string, float> aRawDataObject)
        {
            //Update the array that holds the raw data
            this._rawDataArray.Add(aRawDataObject);
            if (this._rawDataArray.Count > this._maxRecords)
            {
                this._rawDataArray.RemoveAt(0);
            }

            //Calculate the new graph entry based on the current settings
            UpdateGraphData(aRawDataObject);
        }

        private void UpdateGraphDataFromRaw()
        {
            foreach (Dictionary<string, float> rawData in this._rawDataArray)
            {
                UpdateGraphData(rawData);
            }
        }

        private void UpdateGraphData(Dictionary<string, float> aRawDataObject)
        {
            ArrayList timelineValues = new ArrayList();

            //For each ACTIVE metric type...
            foreach (SystemMetric activeSysMetric in this._activeSystemMetrics)
            {
                if (aRawDataObject.ContainsKey(activeSysMetric.Type))
                    timelineValues.Add(aRawDataObject[activeSysMetric.Type]);
            }
            RealTimeLineGraph.AddGraphValues(timelineValues);
        }

        private void graphLabel_toolStripStatusLabel_Click(object sender, EventArgs e)
        {
            if (null != MouseClickBar)
                MouseClickBar(this, new MetricBarClickArgs(this._activeSystemMetric, -1, -1));
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

            /*
            if (fancyTooltips)
            {
                SystemBrush = SystemBrushes.ActiveCaptionText;
                // Draw the custom background. 
                e.Graphics.FillRectangle(SystemBrushes.ActiveCaption, e.Bounds);//newRec);//e.Bounds);
            }*/
            //else
            //{
                e.DrawBackground();
            //}

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
