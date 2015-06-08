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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// Shows the system-wide message for a given system.
    /// </summary>
    public partial class SystemMonitorControl : UserControl, ICloneToWindow
    {
        #region Fields
        private SystemMonitorConfigurationDefinition _configurationDefinition;

        private ConnectionDefinition _connectionDefinition;
        private NSMConnection _nsmConnection = null;
        private bool _isActive = false;
        private Dictionary<SystemMetric, UserControl> _graphDictionary = new Dictionary<SystemMetric, UserControl>();

        private SystemMonitorAggregationBreadcrumbs _aggragationBreadcrumbs = new SystemMonitorAggregationBreadcrumbs();

        private bool _settingSplitters = true;
        private bool _mouseClickSplitters = false;
        public int ActiveTab = 0;

        // event declaration 
        public delegate void ChangingHandler(object sender, MetricBarClickArgs args);
        public event ChangingHandler MouseClickCPU;
        #endregion

        ToolStripDropDownClosingEventHandler toolstripConfigListner = null;
        private bool _contextMenuStayOpen = false;

        #region Properties
        public Dictionary<SystemMetric, UserControl> GraphDictionary
        {
            get { return _graphDictionary; }
        }

        /// <summary>
        /// The system status user control that displays the status lights
        /// </summary>
        public TrafodionSystemStatusUserControl SystemStatusUserControl
        {
            get
            {
                if (GraphDictionary.ContainsKey(MetricTypes.SystemStatus))
                {
                    return GraphDictionary[MetricTypes.SystemStatus] as TrafodionSystemStatusUserControl;
                }

                return null;
            }
        }

        /// <summary>
        /// The system status user control that displays the status lights
        /// </summary>
        public TrafodionLineGraphUserControl TimelineUserControl
        {
            get
            {
                if (GraphDictionary.ContainsKey(MetricTypes.Timeline))
                {
                    return GraphDictionary[MetricTypes.Timeline] as TrafodionLineGraphUserControl;
                }

                return null;
            }
        }

        public SystemMonitorConfigurationControl ConfigurationControl
        {
            get { return config_oneGuiPanel; }
        }

        /// <summary>
        /// Read only property that supplies a suitable base title for the managed window.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return Properties.Resources.SystemMonitor;
            }
        }


        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set
            {
                if (value == null)
                    return;

                //Cleanup the previous connection's event listeners
                Deactivate();

                _connectionDefinition = value;

                //find the NSMconnetion or create it
                _nsmConnection = NSMConnection.FindNSMConnection(_connectionDefinition, true);

                ApplyServerSettings();

                //Listen for data updates and disconnects
                _nsmConnection.DataRefresh += new NSMConnection.UpdateHandler(RefreshDataOnChart);
                _nsmConnection.ConnectionEvent += new NSMConnection.ChangingHandler(NSM_ConnectioNLost);

                SystemMonitorConfigurationDefinition persistedConfig = null;
                ////Load the persistant object
                if (null != (persistedConfig = this._connectionDefinition.GetPropertyObject("SysMonConfigurationDefinition") as SystemMonitorConfigurationDefinition))
                {
                    _configurationDefinition = persistedConfig as SystemMonitorConfigurationDefinition;//new SystemMonitorConfigurationDefinition();
                    CleanupConfigDef();
                }
                else
                {
                    _configurationDefinition = new SystemMonitorConfigurationDefinition();
                    this._connectionDefinition.SetPropertyObject("SysMonConfigurationDefinition", _configurationDefinition);
                }

                //Apply settings
                ApplySettings();
                Activate();
            }
        }
        #endregion

        public void CleanupConfigDef()
        {
            if (_configurationDefinition.MetricGUISettings.Count > 2)
            {
                _configurationDefinition.InitializeMetricSettings();

            }
        }

        /// <summary>
        /// Creates a new SystemMonitorControl.
        /// </summary>
        public SystemMonitorControl()
        {
            InitializeComponent();
            Resize += new EventHandler(SystemMonitorControl_Resize);
            this._aggragationBreadcrumbs.FullLinkLabel.Click += new EventHandler(FullLinkLabel_Click);
            this._aggragationBreadcrumbs.Dock = DockStyle.Fill;
            //this._aggragationBreadcrumbs.Margin = new marg;


            this.oneGuiTabControl1.SelectedIndexChanged += new EventHandler(oneGuiTabControl1_SelectedIndexChanged);

            //no connection definition specified so use default settings            
            _configurationDefinition = new SystemMonitorConfigurationDefinition();

            //Initialize the ConfigurationControl
            this.config_oneGuiPanel.ActiveConfigDefinition = _configurationDefinition;
            this.config_oneGuiPanel.ConfigurationChanged += new SystemMonitorConfigurationControl.ChangingHandler(config_oneGuiPanel_ConfigurationChanged);

            //this.PopulateAll();
            this.ApplySettings();
        }

        void FullLinkLabel_Click(object sender, EventArgs e)
        {
            foreach (KeyValuePair<SystemMetric, UserControl> kvp in this._graphDictionary)
            {
                if (kvp.Value is TrafodionHybridGraphUserControl)
                {
                    ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.ShowcaseSegment = -1;
                    ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.IsDrilled = false;
                }
            }
            this._aggragationBreadcrumbs.SetDrillLinkLabel("");
        }


        private void ActivateTab(int aTabIndex)
        {
            ActiveTab = aTabIndex;
            this._configurationDefinition.SelectedTab = aTabIndex;
            this.oneGuiTabControl1.SelectedIndex = aTabIndex;

            this.oneGuiTabControl1.TabPages[aTabIndex].Controls.Clear();
            this.oneGuiTabControl1.TabPages[aTabIndex].Controls.Add(this.graphContainer_tableLayoutPanel);

            this.graphContainer_tableLayoutPanel.Dock = DockStyle.Fill;


            int splitterDistance = this.graphContainer_tableLayoutPanel.Width / 2;
            if (this._configurationDefinition.SplitterDistance > 0)
                splitterDistance = this._configurationDefinition.SplitterDistance;

            foreach (KeyValuePair<SystemMetric, UserControl> kvp in this._graphDictionary)
            {
                if (kvp.Value is TrafodionHybridGraphUserControl)
                {
                    switch (this.oneGuiTabControl1.SelectedIndex)
                    {
                        case 0:
                            //The 'Live' tab is selected
                            ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.Panel1Collapsed = false;
                            ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.Panel2Collapsed = true;
                            break;
                        case 1:
                            //Timeline
                            ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.Panel1Collapsed = true;
                            ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.Panel2Collapsed = false;
                            break;
                        default:
                            //Hybrid
                            ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.Panel1Collapsed = false;
                            ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.Panel2Collapsed = false;
                            break;
                    }
                }
            }
        }

        void oneGuiTabControl1_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (this.oneGuiTabControl1.SelectedIndex != this.ActiveTab)
                ActivateTab(this.oneGuiTabControl1.SelectedIndex);
        }

        /// <summary>
        /// Creates a new SystemMonitorControl.
        /// </summary>
        public SystemMonitorControl(ConnectionDefinition aConnectionDef)
        {
            InitializeComponent();
            Resize += new EventHandler(SystemMonitorControl_Resize);
            this._connectionDefinition = aConnectionDef;

            //Try to load the persistance
            object persistedConfig = null;
            if (null != (persistedConfig = this._connectionDefinition.GetProperty("SysMonConfigurationDefinition")))
            {
                //load from persistance
                _configurationDefinition = new SystemMonitorConfigurationDefinition();
            }
            else
            {
                //create a new configuration definition based on the default
                _configurationDefinition = new SystemMonitorConfigurationDefinition();
                this._connectionDefinition.SetPropertyObject("SysMonConfigurationDefinition", _configurationDefinition);
            }



            //Initialize the ConfigurationControl
            this.config_oneGuiPanel.ActiveConfigDefinition = _configurationDefinition;
            this.config_oneGuiPanel.ConfigurationChanged += new SystemMonitorConfigurationControl.ChangingHandler(config_oneGuiPanel_ConfigurationChanged);

            this.PopulateAll();
            this.ApplySettings();
        }

        void SystemMonitorControl_Resize(object sender, EventArgs e)
        {
            ActivateTab(this.ActiveTab);
        }

        /// <summary>
        /// Creates a new SystemMonitorControl.
        /// </summary>
        public SystemMonitorControl(params SystemMetric[] metrics)
        {
            InitializeComponent();

            //load from persistance
            //OR
            _configurationDefinition = new SystemMonitorConfigurationDefinition();
            //Initialize the ConfigurationControl
            this.config_oneGuiPanel.ActiveConfigDefinition = _configurationDefinition;

            foreach (SystemMetric metric in metrics)
            {
                AddMetric(metric);
            }
            this.config_oneGuiPanel.ConfigurationChanged += new SystemMonitorConfigurationControl.ChangingHandler(config_oneGuiPanel_ConfigurationChanged);
        }

        /// <summary>
        /// Creates a new SystemMonitorControl using another one. It will copy all of the
        /// information from the given control and add itself as a listener to changes made
        /// to the given control.
        /// </summary>
        /// <param name="SystemMonitorControl">The control that will be copied and listened to.</param>
        public SystemMonitorControl(SystemMonitorControl aSystemMonitorControl)
            : this()
        {
            //this.config_oneGuiPanel = aSystemMonitorControl.ConfigurationControl;
            //this.config_oneGuiPanel.ConfigurationChanged += new SystemMonitorConfigurationControl.ChangingHandler(config_oneGuiPanel_ConfigurationChanged);

            this.ConnectionDefn = aSystemMonitorControl.ConnectionDefn;
            this.ActivateTab(aSystemMonitorControl.ActiveTab);

            this.SuspendLayout();
            this.ResumeLayout();

        }

        void config_oneGuiPanel_ConfigurationChanged(object sender, SystemMonitorConfigurationDefinition aConfigDef)
        {
            //hide the configuration panel
            this.config_oneGuiPanel.Hide();

            //Apply the settings
            this.ApplySettings(aConfigDef);

            //TODO: store tab selection
            //this.ActivateTab(0);

            this.ContextMenuStrip = contextMenu_oneGuiContextMenuStrip;
        }


        public void ApplyServerSettings() { ApplyServerSettings(this._configurationDefinition); }
        public void ApplyServerSettings(SystemMonitorConfigurationDefinition aConfigObject)
        {
            //Update the server with the connection settings
            if (this._nsmConnection != null && aConfigObject != null)
                this._nsmConnection.UpdateConfigDef(aConfigObject.NSMServerConfigDef);
        }

        public void ApplySettings() { this.ApplySettings(this._configurationDefinition); }
        public void ApplySettings(SystemMonitorConfigurationDefinition aConfigObject)
        {

            //Hide this user control while we update the display to prevent flickering
            this.Visible = false;

            //Remove the listener to prevent modifying the SortedDictionary during a foreach (Thread safety)
            if (null != _nsmConnection)
            {
                _nsmConnection.DataRefresh -= new NSMConnection.UpdateHandler(RefreshDataOnChart);
                _nsmConnection.ConnectionEvent -= new NSMConnection.ChangingHandler(NSM_ConnectioNLost);
            }


            //Always going to want the SystemStatusUserControl to be created
            //AddSystemStatus(aConfigObject.ShowStatConnectivity, aConfigObject.ShowStatTransactions, aConfigObject.ShowStatDisks, aConfigObject.ShowStatAlerts, aConfigObject.ShowStatusOnTop);

            foreach (MetricGUIConfigurationDefinition metricConfigOptions in aConfigObject.MetricGUISettings)
            {
                //we have already handled the System Status UserControl and the timeline user control
                if (metricConfigOptions.SystemMetric.Label == MetricTypes.SystemStatus.Label
                   || (metricConfigOptions.SystemMetric.Label == MetricTypes.Timeline.Label))
                    continue;

                if (this.GraphDictionary.ContainsKey(metricConfigOptions.SystemMetric) && !metricConfigOptions.IsEnabled)
                {
                    RemoveMetric(metricConfigOptions.SystemMetric);
                }

                if (!this._graphDictionary.ContainsKey(metricConfigOptions.SystemMetric) && metricConfigOptions.IsEnabled)
                {
                    AddMetric(metricConfigOptions.SystemMetric);
                }
            }


            if (null != _nsmConnection)
            {
                //Re-attatch the listener for data updates (Thread safety)
                _nsmConnection.DataRefresh += new NSMConnection.UpdateHandler(RefreshDataOnChart);
                _nsmConnection.ConnectionEvent += new NSMConnection.ChangingHandler(NSM_ConnectioNLost);
            }

            //Call the method to update the display based on the GraphDictionary
            UpdateGraphics();

            //Apply the settings to the NSM server
            ApplyServerSettings();

            //Always on top option
            bool TopMost = aConfigObject.AlwaysOnTop;

            //Done painting, display the control
            ActivateTab(aConfigObject.SelectedTab);
            this.Visible = true;

        }

        public void ClearAllGraphics()
        {
            //We are updating the display. 
            //First clear all graphs -- start from scratch           
            //this._graphDictionary.Clear();

            try
            {
                //Empty the graph container to get ready to add graphs to it
                this.graphContainer_tableLayoutPanel.Controls.Clear();

                //Required since it will not accept '0'
                this.graphContainer_tableLayoutPanel.RowCount = 1;

                //Clear the RowStyles array then add the initial entry
                this.graphContainer_tableLayoutPanel.RowStyles.Clear();
                this.graphContainer_tableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));

                //Clear and collapse the top panel
                SysMon_oneGuiSplitContainer.Panel1.Controls.Clear();
                SysMon_oneGuiSplitContainer.Panel1Collapsed = true;

                //Clear and collapse the bottom panel
                SysMon_oneGuiSplitContainer.Panel2.Controls.Clear();
                SysMon_oneGuiSplitContainer.Panel2Collapsed = true;
            }
            catch (Exception ex)
            {
            }

        }

        //Add all the availiable metrics
        public void PopulateAll()
        {
            //this.AddMetric(MetricTypes.Timeline);
            this.AddMetric(MetricTypes.CPUBusy);
            //this.AddMetric(MetricTypes.DiskIO);
            //this.AddMetric(MetricTypes.CacheHits);
            //this.AddMetric(MetricTypes.Dispatch);
            //this.AddMetric(MetricTypes.Swap);
            this.AddMetric(MetricTypes.FreeMemory);
            //this.AddMetric(MetricTypes.CPUQueueLength);
            //this.AddMetric(MetricTypes.SystemStatus);
        }

        //Start the timer that triggers server requests
        public bool Activate()
        {
            this._isActive = true;



            if (null == _nsmConnection)
                return false;

            if (!_nsmConnection.IsServerActive)
                _nsmConnection.BindAndInitialize();

            return true;
        }

        //Stop broadcasting requests and stop listening for responses
        public bool Deactivate()
        {
            try
            {
                if (null != _nsmConnection)
                {
                    _nsmConnection.DataRefresh -= new NSMConnection.UpdateHandler(RefreshDataOnChart);
                    _nsmConnection.ConnectionEvent -= new NSMConnection.ChangingHandler(NSM_ConnectioNLost);
                    if (_nsmConnection.IsServerActive)
                    {
                        _nsmConnection.Shutdown();
                    }
                }
                this._isActive = false;
                return true;
            }
            catch (Exception e)
            {
                return false;
            }
        }


        private void AddSystemStatus() { AddSystemStatus(true, true, true, true, false); }
        private void AddSystemStatus(bool aConnectionLight, bool aTransactionLight, bool aDiskLight, bool aAlertLight, bool showOnTop)
        {

            if (_graphDictionary.ContainsKey(MetricTypes.SystemStatus))
            {
                TrafodionSystemStatusUserControl existingSysStatControl = _graphDictionary[MetricTypes.SystemStatus] as TrafodionSystemStatusUserControl;
                existingSysStatControl.SetupLights(aConnectionLight, aTransactionLight, aDiskLight, aAlertLight);
            }
            else
            {
                //Create a new System Status user control
                TrafodionSystemStatusUserControl NewSystemStatus = new TrafodionSystemStatusUserControl(aConnectionLight, aTransactionLight, aDiskLight, aAlertLight);
                //Handle the display of the System Status User Control

                NewSystemStatus.Dock = DockStyle.Fill;
                _graphDictionary.Add(MetricTypes.SystemStatus, NewSystemStatus);

            }

        }


        //Pre: this._configurationDefinition has been set
        public void UpdateGraphics()
        {
            //Start with a fresh canvas
            ClearAllGraphics();
            int rowIndex = 0;

            if (this._configurationDefinition.Aggregate)
            {
                //Add the aggregation control
                this._aggragationBreadcrumbs.SetDrillLinkLabel("");
                this.graphContainer_tableLayoutPanel.Controls.Add(this._aggragationBreadcrumbs, 0, rowIndex);

                RowStyle newAggragateBreadcrumRowStyle = new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F);
                this.graphContainer_tableLayoutPanel.RowStyles.Clear();
                this.graphContainer_tableLayoutPanel.RowStyles.Add(newAggragateBreadcrumRowStyle);
            }

            if (null == this._connectionDefinition)
                return;
            //Sort SystemMetric by Defined Display Order
            SortedDictionary<SystemMetric, UserControl> _sortedGraphDictionary = new SortedDictionary<SystemMetric, UserControl>(GraphDictionary, new DisplayOrderComparer());

            //Update connection with the new Maximum threshold values
            foreach (KeyValuePair<SystemMetric, UserControl> kvp in _sortedGraphDictionary)
            {
                if (kvp.Value is TrafodionHybridGraphUserControl)
                {
                    //Track how many graphs are currently displayed
                    rowIndex = this.graphContainer_tableLayoutPanel.RowCount;

                    TrafodionHybridGraphUserControl barGraph = kvp.Value as TrafodionHybridGraphUserControl;
                    RowStyle newRowStyle = new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F);
                    MetricGUIConfigurationDefinition configDef = this._configurationDefinition.GetGUISettingsForMetric(kvp.Key);

                    //Apply Options
                    barGraph.RealTimeGraph.Aggregate = _configurationDefinition.Aggregate;
                    barGraph.RealTimeGraph.IsDrilled = false;
                    barGraph.RealTimeGraph.SelectedBrush = new SolidBrush(_configurationDefinition.ColorMouseOver);
                    barGraph.RealTimeGraph.ThreshExceededBrush = new SolidBrush(_configurationDefinition.ColorThresholdExceeded);
                    barGraph.RealTimeGraph.CPUDownBrush = new SolidBrush(_configurationDefinition.ColorCPUDown);
                    barGraph.RealTimeGraph.BackColor = _configurationDefinition.ColorBarBackground;
                    barGraph.RealTimeGraph.DefaultBrush = configDef.BarBrushColor;
                    barGraph.RealTimeGraph.CPUDownBrush = new SolidBrush(_configurationDefinition.ColorCPUDown);
                    barGraph.RealTimeGraph.ThreshExceededBrush = new SolidBrush(_configurationDefinition.ColorThresholdExceeded);
                    barGraph.RealTimeGraph.SelectedBrush = new SolidBrush(_configurationDefinition.ColorMouseOver);
                    barGraph.RealTimeGraph.ShowSegmentSeperator = _configurationDefinition.ShowSegSeparators;
                    barGraph.RealTimeGraph.SegmentSepCol = _configurationDefinition.ColorSegSeparator;
                    //Set 100 percent threshold
                    barGraph.RealTimeGraph.ChartRawMaxValue = configDef.ThresholdValue;

                    //Timeline configuration
                    barGraph.RealTimeLineGraph.BackColor = _configurationDefinition.ColorTimelineBack;
                    barGraph.ColorDictionary[kvp.Key] = new Pen(configDef.BarBrushColor, 2);
                    barGraph.SetMetricVisibility(kvp.Key, true);
                    barGraph.UpdateGraphics();
                    //Timeline Configuration end

                    barGraph.ToolTipText = (this._connectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded) ? kvp.Key.LabelTooltip + "\n[Click for Offending Queries]" : kvp.Key.LabelTooltip;

                    //Hack because rowIndex cannot be 0
                    if (this.graphContainer_tableLayoutPanel.Controls.Count <= 0)
                    {
                        rowIndex = 0;
                    }
                    else
                    {
                        this.graphContainer_tableLayoutPanel.RowCount++;
                        this.graphContainer_tableLayoutPanel.RowStyles.Add(newRowStyle);
                    }

                    //update the local graph Dictionary
                    this.graphContainer_tableLayoutPanel.Controls.Add(barGraph, 0, rowIndex);
                }
            }


            bool showMetricPanel = (this._graphDictionary.Count > 0);
            //bool showSystemStatusPanel = (this._configurationDefinition.ShowStatConnectivity
            //                                || this._configurationDefinition.ShowStatTransactions
            //                                || this._configurationDefinition.ShowStatDisks
            //                                || this._configurationDefinition.ShowStatAlerts);

            bool showSystemStatusPanel = false;

            //Set which panels will get which controls
            SplitterPanel MetricPanel = (this._configurationDefinition.ShowStatusOnTop) ? SysMon_oneGuiSplitContainer.Panel2 : SysMon_oneGuiSplitContainer.Panel1;
            SplitterPanel SysStatPanel = (this._configurationDefinition.ShowStatusOnTop) ? SysMon_oneGuiSplitContainer.Panel1 : SysMon_oneGuiSplitContainer.Panel2;


            //Set which panels are collapsed and which aren't
            SysMon_oneGuiSplitContainer.Panel1Collapsed = (!showSystemStatusPanel && this._configurationDefinition.ShowStatusOnTop) || (!showMetricPanel && !this._configurationDefinition.ShowStatusOnTop);
            SysMon_oneGuiSplitContainer.Panel2Collapsed = (!showSystemStatusPanel && !this._configurationDefinition.ShowStatusOnTop) || (!showMetricPanel && this._configurationDefinition.ShowStatusOnTop);



            //Set the splitter distance between the MetricGraph and SystemStatus user controls
            int defaultSplitterDistance = 100;
            int splitterDistance = (this._configurationDefinition.ShowStatusOnTop) ? defaultSplitterDistance : this.Height - defaultSplitterDistance;

            //Add the controls to respective panels is they are enabled
            if (showMetricPanel)
                MetricPanel.Controls.Add(this.oneGuiTabControl1);

            //if (showSystemStatusPanel)
            //{

            //    SysMon_oneGuiSplitContainer.FixedPanel = (this._configurationDefinition.ShowStatusOnTop) ? FixedPanel.Panel1 : FixedPanel.Panel2;
            //    SysStatPanel.Controls.Add(this.SystemStatusUserControl);
            //    SysMon_oneGuiSplitContainer.SplitterDistance = splitterDistance;
            //}
        }

        public void RemoveMetric(SystemMetric aMetricType)
        {
            //Keep track of a reference so it can be disposed

            UserControl CustodialReference = this._graphDictionary[aMetricType] as UserControl;
            if (CustodialReference is TrafodionHybridGraphUserControl)
            {
                ((TrafodionHybridGraphUserControl)CustodialReference).splitContainerMain_splitContainer.SplitterMoved -= new SplitterEventHandler(Hybrid_SplitterMoved);
                ((TrafodionHybridGraphUserControl)CustodialReference).MouseClickBar -= new TrafodionHybridGraphUserControl.ChangingHandler(RealTimeGraph_MouseClickBar);
                ((TrafodionHybridGraphUserControl)CustodialReference).splitContainerMain_splitContainer.MouseClick -= new MouseEventHandler(splitContainerMain_splitContainer_MouseClick);
                ((TrafodionHybridGraphUserControl)CustodialReference).splitContainerMain_splitContainer.MouseDown -= new MouseEventHandler(splitContainerMain_splitContainer_MouseDown);
            }

            //Remove the reference to the graph we are no longer displaying
            this._graphDictionary.Remove(aMetricType);

            //Cleanup the old graph user control that we no longer need
            CustodialReference.Dispose();
        }


        public void AddMetric(SystemMetric aMetricType)
        {
            TrafodionHybridGraphUserControl NewBarGraph = new TrafodionHybridGraphUserControl(aMetricType);

            //Initial Setup 
            NewBarGraph.RealTimeGraph.TooltipLabel = aMetricType.LabelTooltip;
            NewBarGraph.RealTimeGraph.MetricType = aMetricType.Type;
            NewBarGraph.RealTimeGraph.MetricLabel = aMetricType.Label;
            NewBarGraph.RealTimeGraph.MetricUnitSuffix = aMetricType.UnitSuffix;
            NewBarGraph.RealTimeGraph.IsIntervalBased = aMetricType.IntervalBased;
            NewBarGraph.Dock = DockStyle.Fill;

            //Attach 'Click' Handler for easier access to mouse events
            NewBarGraph.splitContainerMain_splitContainer.SplitterMoved += new SplitterEventHandler(Hybrid_SplitterMoved);
            NewBarGraph.splitContainerMain_splitContainer.MouseClick += new MouseEventHandler(splitContainerMain_splitContainer_MouseClick);
            NewBarGraph.splitContainerMain_splitContainer.MouseDown += new MouseEventHandler(splitContainerMain_splitContainer_MouseDown);

            NewBarGraph.MouseClickBar += new TrafodionHybridGraphUserControl.ChangingHandler(RealTimeGraph_MouseClickBar);

            this._graphDictionary.Add(aMetricType, NewBarGraph);
        }

        void splitContainerMain_splitContainer_MouseClick(object sender, MouseEventArgs e)
        {
            this._settingSplitters = true;
        }

        void splitContainerMain_splitContainer_MouseDown(object sender, MouseEventArgs e)
        {
            this._settingSplitters = false;
        }


        void Hybrid_SplitterMoved(object sender, SplitterEventArgs e)
        {
            if (this._settingSplitters)
                return;

            this._settingSplitters = true;
            foreach (KeyValuePair<SystemMetric, UserControl> kvp in this._graphDictionary)
            {
                if (kvp.Value is TrafodionHybridGraphUserControl)
                {
                    ((TrafodionHybridGraphUserControl)kvp.Value).splitContainerMain_splitContainer.SplitterDistance = e.SplitX;
                }
            }

            this._configurationDefinition.SplitterDistance = e.SplitX;
        }

        void RealTimeGraph_MouseClickBar(object sender, MetricBarClickArgs args)
        {
            //Handle aggragation
            if (this._configurationDefinition.Aggregate)
            {
                //Segment has been clicked
                if (args.CpuClicked < 0 && args.SegmentClicked > 0)
                {
                    foreach (KeyValuePair<SystemMetric, UserControl> kvp in this._graphDictionary)
                    {
                        if (kvp.Value is TrafodionHybridGraphUserControl)
                        {
                            ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.ShowcaseSegment = args.SegmentClicked;
                            ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.IsDrilled = true;
                        }
                    }
                    this._aggragationBreadcrumbs.SetDrillLinkLabel(args.SegmentClicked.ToString());
                }
            }


            if (null != MouseClickCPU)
                MouseClickCPU(sender, args);
        }




        private Dictionary<string, float> parseTimelineData(SystemSnapshot e)
        {
            int totalCount;
            int numValues;
            Dictionary<string, float> timelineValues = new Dictionary<string, float>();

            //Aggregate the data for temporary 'archiving' in the timeline (Remove 'down' CPUs)
            foreach (string MetricType in e.SampleTypes.Keys)
            {
                //Build Timeline summary info array
                totalCount = 0;
                numValues = 0;

                ArrayList temp = ((ArrayList)((ArrayList)e.SampleTypes[MetricType]));

                //For each segment..
                for (int i = 0; i < temp.Count; i++)
                {
                    ArrayList segTemp = ((ArrayList)temp[i]);

                    //For each CPU in segment i 
                    for (int j = 0; j < segTemp.Count; j++)
                    {
                        if (!e.CPUDownList.Contains(i + "," + j))
                        //if (!this._nsmConnection.CPUDownList.Contains(i + "," + j))
                        {
                            totalCount += (int)segTemp[j];
                            numValues++;
                        }
                    }
                }

                float updateNum = ((float)totalCount / (float)numValues) / 100;
                timelineValues.Add(MetricType, updateNum);
            }

            return timelineValues;
        }

        void NSM_ConnectioNLost(object sender, int ReconnectCount)
        {
            if (this._configurationDefinition.SwapBGonDisconnect)
            {
                foreach (KeyValuePair<SystemMetric, UserControl> kvp in this._graphDictionary)
                {
                    if (kvp.Value is TrafodionHybridGraphUserControl)
                    {
                        ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.BackColor = this._configurationDefinition.ColorDisconnected;
                        ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeLineGraph.BackColor = this._configurationDefinition.ColorDisconnected;
                    }
                }
            }
        }

        public void RefreshDataOnChart(object sender, SystemSnapshot e)
        {

            Dictionary<string, float> TimelineValues = parseTimelineData(e);


            foreach (KeyValuePair<SystemMetric, UserControl> kvp in this._graphDictionary)
            {
                if (kvp.Value is TrafodionHybridGraphUserControl)
                {
                    //TIMELINE -- changed from bar to hybrid
                    ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.GraphValues = (ArrayList)e.SampleTypes[((TrafodionHybridGraphUserControl)kvp.Value).ActiveSystemMetric.Type];
                    ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.DownCPUs = (ArrayList)e.CPUDownList;
                    ((TrafodionHybridGraphUserControl)kvp.Value).UpdateRawData(TimelineValues);

                    ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeGraph.BackColor = this._configurationDefinition.ColorBarBackground;
                    ((TrafodionHybridGraphUserControl)kvp.Value).RealTimeLineGraph.BackColor = this._configurationDefinition.ColorTimelineBack;

                    //((TrafodionHybridGraphUserControl)kvp.Value).DownCPUs = (ArrayList)e.CPUDownList;
                    //END TIMELINE
                }
                else if (kvp.Value is TrafodionSystemStatusUserControl)
                {
                    if (e.SystemSummary != null)
                    {
                        ArrayList errorArray = e.SystemSummary["ErrorArray"] as ArrayList;
                        ((TrafodionSystemStatusUserControl)kvp.Value).UpdateLights(e.SystemSummary);
                        ((TrafodionSystemStatusUserControl)kvp.Value).UpdateDetails(errorArray);
                    }
                }
            }
        }



        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            SystemMonitorControl SystemMonitorControl = new SystemMonitorControl(this);
            return SystemMonitorControl;
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
//            Deactivate();

            ClearAllGraphics();

            //this.oneGuiRealTimeBarGraph1.MouseClickBar -= new TrafodionRealTimeBarGraph.ChangingHandler(oneGuiRealTimeBarGraph1_MouseClickBar);           
            if (null != this._nsmConnection)
                this._nsmConnection.DataRefresh -= this.RefreshDataOnChart;

            foreach (KeyValuePair<SystemMetric, UserControl> kvp in this.GraphDictionary)
            {
                //TIMELINE
                if (kvp.Value is TrafodionBarGraphUserControl)
                {
                    TrafodionHybridGraphUserControl graph = kvp.Value as TrafodionHybridGraphUserControl;
                    graph.MouseClickBar -= new TrafodionHybridGraphUserControl.ChangingHandler(RealTimeGraph_MouseClickBar);
                    graph.splitContainerMain_splitContainer.SplitterMoved -= new SplitterEventHandler(Hybrid_SplitterMoved);
                    graph.MouseClickBar -= new TrafodionHybridGraphUserControl.ChangingHandler(RealTimeGraph_MouseClickBar);

                }
                //END TIMELINE
                ((UserControl)kvp.Value).Dispose();
            }
        }



        private void configuration_ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //User has clicked 'Configure'

            //// Stop listen
            Deactivate();

            //// update graphics
            UpdateGraphics();

            //Display Configuration Control overlay
            //this._configurationDefinition.TimelineMetricToTrack = this.TimelineUserControl.ActiveSystemMetric;
            this.config_oneGuiPanel.ActiveConfigDefinition = this._configurationDefinition;
            this.config_oneGuiPanel.RebuildControlObjects();

            this.config_oneGuiPanel.Show();
            this.config_oneGuiPanel.BringToFront();
            //this.graphContainer_tableLayoutPanel.SendToBack();

            // disable the context menu when SystemMonitorControl is 'NOT' activated
            this.ContextMenuStrip = null;
        }

        private void SystemMonitorControl_Enter(object sender, EventArgs e)
        {
            // reset the context menu when SystemMonitorControl is activated
            if (this.ContextMenuStrip == null)
                this.ContextMenuStrip = contextMenu_oneGuiContextMenuStrip;
        }

        private void cloneToWindowToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Control clonedSystemMonitorControl = Clone();
            WindowsManager.PutInWindow(clonedSystemMonitorControl.Size, clonedSystemMonitorControl, Properties.Resources.SystemMonitor, _connectionDefinition);
        }

    }

    internal class DisplayOrderComparer : IComparer<SystemMetric>
    {
        public int Compare(SystemMetric x, SystemMetric y)
        {
            if (GetDisplayOrder(x.Type) > GetDisplayOrder(y.Type))
            {
                return 1;
            }
            else
            {
                return -1;
            }

        }

        /// <summary>
        /// Get display order by Type,This defined order is the same as control design order:
        ///     #1.CPU Busy
        ///     #2.Disk I/O
        ///     #3.Cache Hits
        ///     #4.Dispatch
        ///     #5.Swap
        ///     #6.Free Memory
        ///     #7.Queue Length
        /// you can change this order by modifying variable of iOrder.
        /// </summary>
        /// <param name="sLabel"></param>
        /// <returns></returns>
        private int GetDisplayOrder(string sType)
        {
            int iOrder = 0;
            switch (sType)
            {
                case "B":
                    iOrder = 1;
                    break;
                case "D":
                    iOrder = 2;
                    break;
                case "C":
                    iOrder = 3;
                    break;
                case "P":
                    iOrder = 4;
                    break;
                case "S":
                    iOrder = 5;
                    break;
                case "M":
                    iOrder = 6;
                    break;
                case "Q":
                    iOrder = 7;
                    break;

            }
            return iOrder;
        }


    }
}
