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
//using System.Linq;
using System.Collections;
using System.Drawing;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// This class exists to suppress change events on connection definitions as they are being loaded and saved.
    /// See "if (aConnectionDefinition is ScratchConnectionDefinition)" in the persistence code above.
    /// </summary>
    [Serializable]
    public class MetricGUIConfigurationDefinition
    {
        private bool _visible = true;

        public bool IsEnabled
        {
            get { return _visible; }
            set { _visible = value; }
        }
        private bool _timeline = false;

        public bool ShowOnTimeline
        {
            get { return _timeline; }
            set { _timeline = value; }
        }
        private int _barColor = Color.Green.ToArgb();
        private int _thresholdValue;
        private SystemMetric _sysMetric;

        public Brush BarBrushColor
        {
            get { return new SolidBrush(Color.FromArgb(_barColor)); }
            set {
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

        public int ThresholdValue
        {
            get { return _thresholdValue; }
            set { _thresholdValue = value; }
        }

        public SystemMetric SystemMetric
        {
            get { return _sysMetric; }
            set { _sysMetric = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public MetricGUIConfigurationDefinition(SystemMetric aSystemMetric)
        {
            //LoadDefault Settings
            this._sysMetric = aSystemMetric;
            this._thresholdValue = aSystemMetric.MaxValThreshold;

            BarBrushColor = aSystemMetric.BarColor;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public MetricGUIConfigurationDefinition(SystemMetric aSystemMetric, bool aEnabled)
        {
            this._visible = aEnabled;
            //LoadDefault Settings
            this._sysMetric = aSystemMetric;
            this._thresholdValue = aSystemMetric.MaxValThreshold;

            BarBrushColor = aSystemMetric.BarColor;
        }

    }


    [Serializable]
    public class SystemMonitorConfigurationDefinition
    {
        #region MemberVariables
        //RealTimeBarGraph options

        private ArrayList _metricGUISettings = new ArrayList();
        private NSMServerConfigurationDefinition _nsmServerConfigDef = new NSMServerConfigurationDefinition();

        //Global settings applied to all metrics
        private bool _aggregate = false;

        //General UI options
        private bool _alwaysOnTop = false;        

        private bool _showStatDisks = false;
        private bool _showStatConnectivity = false;
        private bool _showStatTransactions = false;
        private bool _showStatAlerts = false;

        private bool _showStatusOnTop = false;
        private bool _showSystemStatusDetails = false;
        private bool _showTimeline = false;
        private bool _showSegSeparators = false;
        private bool _swapBGonDisconnect = false;


        //Global color settings
        private int _colorBarBackground = Color.WhiteSmoke.ToArgb();
        private int _colorThresholdExceeded = Color.Gray.ToArgb();
        private int _colorMouseOver = Color.Blue.ToArgb();
        private int _colorCPUDown = Color.Black.ToArgb();
        private int _colorSegSeparator = Color.SteelBlue.ToArgb();
        private int _colorDisconnected = Color.Pink.ToArgb();

        //Timeline settings
        private SystemMetric _timelineMetricToTrack = MetricTypes.CPUBusy;
        private int _timelineMaxRange = 50;
        private int _colorTimelineBack = Color.White.ToArgb();
        private int _colorTimelineFore = Color.White.ToArgb();

        private int selectedTab = 0;
        private int splitterDistance = -1;

        #endregion


        #region Properties

        public SystemMetric TimelineMetricToTrack
        {
            get { return _timelineMetricToTrack; }
            set { _timelineMetricToTrack = value; }
        }

        public ArrayList MetricGUISettings
        {
            get { return _metricGUISettings; }
        }

        public NSMServerConfigurationDefinition NSMServerConfigDef
        {
            get { return _nsmServerConfigDef; }
            set { _nsmServerConfigDef = value; }
        }

        public bool Aggregate
        {
            get { return _aggregate; }
            set { _aggregate = value; }
        }

        //General UI options
        public bool AlwaysOnTop
        {
            get { return _alwaysOnTop; }
            set { _alwaysOnTop = value; }
        }

        public bool ShowStatDisks
        {
            get { return _showStatDisks; }
            set { 
                    _showStatDisks = value;
                    _nsmServerConfigDef.FetchDisk = value;
                }
        }
        public bool ShowStatConnectivity
        {
            get { return _showStatConnectivity; }
            set { 
                    _showStatConnectivity = value;
                    _nsmServerConfigDef.FetchConnectivity = value;
                }
        }
        public bool ShowStatTransactions
        {
            get { return _showStatTransactions; }
            set { 
                    _showStatTransactions = value;
                    _nsmServerConfigDef.FetchTransactions = value;
                }
        }
        public bool ShowStatAlerts
        {
            get { return _showStatAlerts; }
            set { _showStatAlerts = value; }
        }


        public bool ShowStatusOnTop
        {
            get { return _showStatusOnTop; }
            set { _showStatusOnTop = value; }
        }

        public bool ShowSystemStatusDetails
        {
            get { return _showSystemStatusDetails; }
            set { _showSystemStatusDetails = value; }
        }

        public int SplitterDistance
        {
            get { return splitterDistance; }
            set { splitterDistance = value; }
        }

        public int SelectedTab
        {
            get { return selectedTab; }
            set { selectedTab = value; }
        }

        public bool ShowTimeline
        {
            get { return _showTimeline; }
            set { _showTimeline = value; }
        }

        //Global color settings
        public Color ColorBarBackground
        {
            get { return Color.FromArgb(_colorBarBackground); }
            set { _colorBarBackground = value.ToArgb(); }
        }

        public Color ColorThresholdExceeded
        {
            get { return Color.FromArgb(_colorThresholdExceeded); }
            set { _colorThresholdExceeded = value.ToArgb(); }
        }

        public Color ColorMouseOver
        {
            get { return Color.FromArgb(_colorMouseOver); }
            set { _colorMouseOver = value.ToArgb(); }
        }

        public Color ColorCPUDown
        {
            get { return Color.FromArgb(_colorCPUDown); }
            set { _colorCPUDown = value.ToArgb(); }
        }

        public Color ColorSegSeparator
        {
            get { return Color.FromArgb(_colorSegSeparator); }
            set { _colorSegSeparator = value.ToArgb(); }
        }

        public Color ColorDisconnected
        {
            get { return Color.FromArgb(_colorDisconnected); }
            set { _colorDisconnected = value.ToArgb(); }
        }

        //Timeline settings
        public int TimelineMaxRange
        {
            get { return _timelineMaxRange; }
            set { _timelineMaxRange = value; }
        }

        public Color ColorTimelineBack
        {
            get { return Color.FromArgb(_colorTimelineBack); }
            set { _colorTimelineBack = value.ToArgb(); }
        }

        public Color ColorTimelineFore
        {
            get { return Color.FromArgb(_colorTimelineFore); }
            set { _colorTimelineFore = value.ToArgb(); }
        }

        public bool ShowSegSeparators
        {
            get { return _showSegSeparators; }
            set { _showSegSeparators = value; }
        }

        public bool SwapBGonDisconnect
        {
            get { return _swapBGonDisconnect; }
            set { _swapBGonDisconnect = value; }
        }

        #endregion
        
        /// <summary>
        /// Creates a new uninitialized connection definition
        /// </summary>
        public SystemMonitorConfigurationDefinition()
        {
            InitializeMetricSettings();
            //this.AddMetric(MetricTypes.Timeline);
            // Always start with the default port number.
            //this._metricGUISettings.
        }

        public void InitializeMetricSettings()
        {
            this._metricGUISettings.Clear();
            this.AddMetric(MetricTypes.CPUBusy, true);            
            this.AddMetric(MetricTypes.FreeMemory, true);            
        }

        public void AddMetric(SystemMetric aSysMetric, bool aEnabled)
        {
            MetricGUIConfigurationDefinition mgcd = this.GetGUISettingsForMetric(aSysMetric);
            if (null == mgcd)
            { this._metricGUISettings.Add(new MetricGUIConfigurationDefinition(aSysMetric, aEnabled)); }
            else
            {
                mgcd.IsEnabled = aEnabled;
            }
        }

        public void AddMetric(SystemMetric aSysMetric, int aMaxThresholdValue)
        {
            _metricGUISettings.Add(new MetricGUIConfigurationDefinition(aSysMetric));
        }

        public MetricGUIConfigurationDefinition GetGUISettingsForMetric(SystemMetric aSysMetric)
        {
            foreach (MetricGUIConfigurationDefinition metricGUIConDef in this._metricGUISettings)
            {
                if (metricGUIConDef.SystemMetric.Equals(aSysMetric))
                    return metricGUIConDef;
            }

            return null;
        }




    }
}
