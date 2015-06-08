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
using System.Drawing;
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Controls
{
    [Serializable]
    public struct SystemMetric : IComparable
    {
        public string Type, Label;
        public int MaxValThreshold;
        private int _barColor;
        private string _unitSuffix;
        private bool _intervalBased;
        private string _labelTooltip;

        public string LabelTooltip
        {
            get { return _labelTooltip; }
            set { _labelTooltip = value; }
        }

        public bool IntervalBased
        {
            get { return _intervalBased; }
            set { _intervalBased = value; }
        }

        public string UnitSuffix
        {
            get { return _unitSuffix; }
            set { _unitSuffix = value; }
        }

        public Brush BarColor
        {
            get { return new SolidBrush(Color.FromArgb(_barColor)); }
            set
            {
                Pen pen = null;
                try
                {
                    pen = new Pen(value);
                    _barColor = pen.Color.ToArgb();
                }
                finally
                {
                    pen.Dispose();
                }
            }
        }

        public SystemMetric(string aType, string aLabel)
        {
            Pen pen = null;
            try
            {
                Type = aType;
                Label = aLabel;
                this._labelTooltip = "";
                MaxValThreshold = -1;
                this._unitSuffix = "";
                this._intervalBased = false;
                pen = new Pen(System.Drawing.Brushes.Blue);
                this._barColor = pen.Color.ToArgb();
            }
            finally
            {
                pen.Dispose();
            }
        }

        public SystemMetric(string aType, string aLabel, int aMaxValThresh)
        {
            Pen pen = null;
            try
            {
                Type = aType;
                Label = aLabel;
                this._labelTooltip = "";
                MaxValThreshold = aMaxValThresh;
                this._unitSuffix = "";
                this._intervalBased = false;
                pen = new Pen(System.Drawing.Brushes.Blue);
                this._barColor = pen.Color.ToArgb();
            }
            finally
            {
                pen.Dispose();
            }
        }
         
        public SystemMetric(string aType, string aLabel, int aMaxValThresh, Brush aBrush)
        {
            Pen pen = null;
            try
            {
                Type = aType;
                Label = aLabel;
                this._labelTooltip = "";
                MaxValThreshold = aMaxValThresh;
                this._intervalBased = false;
                this._unitSuffix = "";
                pen = new Pen(aBrush);
                this._barColor = pen.Color.ToArgb();
            }
            finally
            {
                pen.Dispose();
            }
        }

        public SystemMetric(string aType, string aLabel, int aMaxValThresh, string aUnitSuffix, bool aIntervalBased, Brush aBrush)
        {
            Pen pen = null;
            try
            {
                this.Type = aType;
                Label = aLabel;
                this._labelTooltip = "";
                MaxValThreshold = aMaxValThresh;
                this._unitSuffix = aUnitSuffix;
                this._intervalBased = aIntervalBased;
                pen = new Pen(aBrush);
                this._barColor = pen.Color.ToArgb();
            }
            finally
            {
                pen.Dispose();
            }
        }

        public SystemMetric(string aType, string aLabel, string aLabelTooltip, int aMaxValThresh, string aUnitSuffix, bool aIntervalBased, Brush aBrush)
        {
            Pen pen = null;
            try
            {
                this.Type = aType;
                Label = aLabel;
                this._labelTooltip = aLabelTooltip;
                MaxValThreshold = aMaxValThresh;
                this._unitSuffix = aUnitSuffix;
                this._intervalBased = aIntervalBased;
                pen = new Pen(aBrush);
                this._barColor = pen.Color.ToArgb();
            }
            finally
            {
                pen.Dispose();
            }
        }
        public override int GetHashCode()
        {
            int hashCode = Type.GetHashCode();
            return hashCode;
        }

        public int CompareTo(object obj)
        {
            SystemMetric sm = (SystemMetric)obj;
            if (this.Type.Equals(sm.Type))
            { 
                return 0;            
            } else {
                return 1;
            }
        }

        public override bool Equals(object obj)
        {
            try
            {
                SystemMetric sm = (SystemMetric)obj;

                return (sm.Type == Type);
            }
            catch
            {
                return false;
            }
        }

        public override string ToString()
        {
            return this.Label;
        }
    }


    public struct MetricBarClickArgs
    {
        private SystemMetric _sysMetric;
        private int _segmentClicked;
        private int _cpuClicked;

        public SystemMetric SysMetric
        {
            get { return _sysMetric; }
        }

        public int SegmentClicked
        {
            get { return _segmentClicked; }
        }

        public int CpuClicked
        {
            get { return _cpuClicked; }
        }

        public MetricBarClickArgs(SystemMetric aSysMetric, int aSegment, int aCPU )
        {
            this._sysMetric = aSysMetric;
            this._segmentClicked = aSegment;
            this._cpuClicked = aCPU;
        }
    }


    public partial class TrafodionBarGraphUserControl : UserControl
    {
        private SystemMetric _activeSystemMetric;
        public delegate void ChangingHandler(object sender, MetricBarClickArgs args);
        public event ChangingHandler MouseClickBar;


        public SystemMetric ActiveSystemMetric
        {
            get { return _activeSystemMetric; }
            set { _activeSystemMetric = value; }
        }

        public TrafodionBarGraphUserControl()
        {
            InitializeComponent();
        }

        public TrafodionBarGraphUserControl(SystemMetric aMetricType)
        {
            InitializeComponent();

            this._activeSystemMetric = aMetricType;
            this.graphLabel_toolStripStatusLabel.Text = aMetricType.Label;
            this.RealTimeGraph.MouseClickBar += new TrafodionRealTimeBarGraph.ChangingHandler(RealTimeGraph_MouseClickBar);
        }

        void RealTimeGraph_MouseClickBar(object sender, BarClickArgs args)
        {
            if(null != MouseClickBar)
                MouseClickBar(this,new MetricBarClickArgs(this._activeSystemMetric,args.SegmentClicked,args.CpuClicked));
        }

        public Trafodion.Manager.Framework.Controls.TrafodionRealTimeBarGraph RealTimeGraph
        {
            get { return graph_TrafodionRealTimeBarGraph; }
            set { graph_TrafodionRealTimeBarGraph = value; }
        }
    }

    public static class MetricTypes
    {
        public static SystemMetric CPUBusy = new SystemMetric("B", "CPU Busy (Prototype)", "CPU Busy",100, "% Busy", false,System.Drawing.Brushes.DarkBlue);
        public static SystemMetric DiskIO = new SystemMetric("D", "Disk I/O", "Disk I/O",300, " Operations per second", true, System.Drawing.Brushes.Purple);
        public static SystemMetric CacheHits = new SystemMetric("C", "Cache Hits", "Cache Hits", 50000, " Cache Accesses per second", true, System.Drawing.Brushes.RosyBrown);
        public static SystemMetric Dispatch = new SystemMetric("P", "Dispatch", "Dispatch", 10000, " Context Switches per second", true, System.Drawing.Brushes.SeaGreen);
        public static SystemMetric Swap = new SystemMetric("S", "Swap", "Swap", 50, " Page Faults per second", true, System.Drawing.Brushes.Red);
        public static SystemMetric FreeMemory = new SystemMetric("M", "Free Memory (Prototype)", "Free Memory", 8192, " MB", false, System.Drawing.Brushes.DarkMagenta);
        public static SystemMetric CPUQueueLength = new SystemMetric("Q", "Queue Length", "Queue Length", 20, " Queued Processes", false,System.Drawing.Brushes.Beige);
        public static SystemMetric SystemStatus = new SystemMetric("Status", "System Status");
        public static SystemMetric Timeline = new SystemMetric("Timeline", "Timeline");

    }
    


}
