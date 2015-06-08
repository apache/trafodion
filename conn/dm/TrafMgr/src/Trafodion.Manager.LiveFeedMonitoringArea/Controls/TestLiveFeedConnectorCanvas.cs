//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.LiveFeedFramework.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// Test LiveFeed Canvas 
    /// </summary>
    public partial class TestLiveFeedConnectorCanvas : UserControl, ICloneToWindow
    {

        #region Fields

        public const int LiveFeedMonitorRefreshRate = 30;  // 30 seconds
        private ConnectionDefinition _theConnectionDefinition = null;
        private LiveFeedConnection _LiveFeedConnection = null;
        private UniversalWidgetConfig _config1 = null;
        private LiveFeedUniversalWidget _widget1 = null;
        private UniversalWidgetConfig _config2 = null;
        private LiveFeedUniversalWidget _widget2 = null;

        private TestLiveFeedConnectorUserControl _monitorConnector = null;
        private DisplayTestSubscriptionsUserControl _monitorPublications = null;
        private CachedLiveFeedProtoBufDataProvider _theLiveFeedDataProvider = null;

        private LiveFeedConnection.LiveFeedConnectionStateChanged _LiveFeedConnectionStateChangeHandler = null;
        private LiveFeedConnection.OnDataArrivalHandler _LiveFeedConnectionDataArrivalHandler = null;

        private ICloneToWindow _theICloneToWindow = null;
        private string _theTitle = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private delegate void HandleLiveFeedConnectionEvents(Object obj, LiveFeedConnectionEventArgs e);
        private ToolStripButton _configServerButton = new ToolStripButton();

        private bool _started = false;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Started - the unser control is started
        /// </summary>
        public bool Started
        {
            get { return _started; }
        }

        /// <summary>
        /// Property: The connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: LiveFeedConnection
        /// The widget's LiveFeed Connection
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            get { return _LiveFeedConnection; }
        }

        /// <summary>
        /// Property: The window title
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor for a brand new creation
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aTitle"></param>
        public TestLiveFeedConnectorCanvas(ConnectionDefinition aConnectionDefinition, string aTitle)
        {
            _theConnectionDefinition = aConnectionDefinition;
            _theTitle = aTitle;
            InitializeComponent();
            ShowWidgets();
        }

        /// <summary>
        /// Constructor for cloning
        /// </summary>
        /// <param name="aLiveFeedMonitorCanvas"></param>
        public TestLiveFeedConnectorCanvas(TestLiveFeedConnectorCanvas aLiveFeedMonitorCanvas)
        {
            _theConnectionDefinition = aLiveFeedMonitorCanvas.ConnectionDefn;
            _theTitle = aLiveFeedMonitorCanvas.WindowTitle;
            InitializeComponent();
            ShowWidgets(aLiveFeedMonitorCanvas);
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// The interface method for ICloneWindow.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            TestLiveFeedConnectorCanvas theLiveFeedMonitorCanvas = new TestLiveFeedConnectorCanvas(this);
            return theLiveFeedMonitorCanvas;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                Shutdown();
            }
        }

        /// <summary>
        /// Show all widgets
        /// </summary>
        private void ShowWidgets()
        {
            ShowWidgets(null);
        }

        /// <summary>
        /// Create all of the widgets
        /// </summary>
        private void ShowWidgets(TestLiveFeedConnectorCanvas clone)
        {
            GridLayoutManager gridLayoutManager = new GridLayoutManager(5, 1);
            gridLayoutManager.CellSpacing = 4;
            this._theCanvas.LayoutManager = gridLayoutManager;

            // Create the first widget for monitoring the connector
            _config1 = new UniversalWidgetConfig();
            CachedLiveFeedDataProviderConfig qpidConfig = new CachedLiveFeedDataProviderConfig();
            qpidConfig.TheDataFormat = LiveFeedDataProviderConfig.LiveFeedDataFormat.ProtoBuf;
            qpidConfig.ConnectionDefinition = this.ConnectionDefn;
            //qpidConfig.Configure(new string[] { LiveFeedRoutingKeyMapper.PUBS_common_text_event
            //                                    , LiveFeedRoutingKeyMapper.PUBS_lv_cpu_metrics
            //                                    , LiveFeedRoutingKeyMapper.PUBS_lv_filesystem_metrics
            //                                    , LiveFeedRoutingKeyMapper.PUBS_lv_memory_metrics
            //                                    , LiveFeedRoutingKeyMapper.PUBS_lv_metric_ssd
            //                                    , LiveFeedRoutingKeyMapper.PUBS_lv_state_ssd
            //                                    , LiveFeedRoutingKeyMapper.PUBS_lv_alerts});
            qpidConfig.Configure(LiveFeedRoutingKeyMapper.AllPublicationNames);

            _config1.DataProviderConfig = qpidConfig;
            _config1.Name = "LiveFeed Connector";
            _config1.Title = "LiveFeed Connector";
            _config1.ShowRefreshButton = false;
            _config1.ShowProviderToolBarButton = false;
            _config1.ShowTimerSetupButton = false;
            _config1.ShowExportButtons = false;
            _widget1 = new LiveFeedUniversalWidget();
            _widget1.UniversalWidgetConfiguration = _config1;
            _theLiveFeedDataProvider = (CachedLiveFeedProtoBufDataProvider)_widget1.DataProvider;
            _LiveFeedConnection = _theLiveFeedDataProvider.LiveFeedConnection;

            if (clone != null)
            {
                _monitorConnector = new TestLiveFeedConnectorUserControl(clone._monitorConnector);
            }
            else
            {
                _monitorConnector = new TestLiveFeedConnectorUserControl(_LiveFeedConnection);
            }

            _monitorConnector.DataProvider = _widget1.DataProvider;
            _widget1.DataDisplayControl = _monitorConnector;

            //Add the widget to the canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 3, 1);
            WidgetContainer container1 = _theCanvas.AddWidget(_widget1, _config1.Name, _config1.Title, gridConstraint, -1);

            // Create the 2nd Widget for monitoring the queue length of every publication
            _config2 = new UniversalWidgetConfig();
            _config2.DataProviderConfig = qpidConfig;
            _config2.Name = "Publication Monitor";
            _config2.Title = "Publication Monitor";
            _config2.ShowToolBar = false;
            _config2.ShowProviderToolBarButton = false;
            _widget2 = new LiveFeedUniversalWidget();
            _widget2.UniversalWidgetConfiguration = _config2;

            _monitorPublications = new DisplayTestSubscriptionsUserControl(_LiveFeedConnection);
            _monitorPublications.DataProvider = _widget2.DataProvider;
            _widget2.DataDisplayControl = _monitorPublications;
            gridConstraint = new GridConstraint(3, 0, 2, 1);
            WidgetContainer container2 = _theCanvas.AddWidget(_widget2, _config2.Name, _config2.Title, gridConstraint, -1);

            AddToolStripButtons();

            _LiveFeedConnectionStateChangeHandler = new LiveFeedConnection.LiveFeedConnectionStateChanged(InvokeHandleOnStateChanged);
            _LiveFeedConnection.OnStateChanged += _LiveFeedConnectionStateChangeHandler;

            _theLiveFeedDataProvider.Start();
            _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

            // If the connection is started, fire up all of the display. 
            if (LiveFeedConnection.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Started)
            {
                if (!_monitorConnector.Started)
                {
                    _monitorConnector.Start();
                }

                if (!_monitorPublications.Started)
                {
                    _monitorPublications.Startup();
                }
            }

            _started = true;
        }

        private void AddToolStripButtons()
        {
            _configServerButton.Text = "Configure Live Feed Server";
            _configServerButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            _configServerButton.Image = Trafodion.Manager.Properties.Resources.AlterIcon;
            _configServerButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _configServerButton.Name = "_configureLiveFeedButton";
            _configServerButton.Click += new EventHandler(_configServerButton_Click);
            //_clientRuleButton.Enabled = false;
            _widget1.AddToolStripItem(_configServerButton);

            //Add a separator
            ToolStripSeparator toolStripSeparator1 = new ToolStripSeparator();
            toolStripSeparator1.Name = "_configureLiveFeedSeparator";
            toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
            toolStripSeparator1.Visible = true;
            _widget1.AddToolStripItem(toolStripSeparator1);
        }

        void _configServerButton_Click(object sender, EventArgs e)
        {
            LiveFeedBrokerConfigDialog dialog = new LiveFeedBrokerConfigDialog(_theLiveFeedDataProvider.LiveFeedConnection);
            DialogResult result = dialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                _theLiveFeedDataProvider.Stop();
                _theLiveFeedDataProvider.RefreshRate = _theLiveFeedDataProvider.LiveFeedConnection.BrokerConfiguration.SessionRetryTimer;
                _theLiveFeedDataProvider.Start();
            }
        }

        /// <summary>
        /// New data arrival invoker
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleEvents(DataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedMonitoring,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// The new data arrival handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            _monitorConnector.GoodCounterLabel = string.Format(Properties.Resources.ReceivedCounter, _LiveFeedConnection.TotalReceivedCount);
            if (_LiveFeedConnection.LastReceivedPacket != null)
            {
                _monitorConnector.LastReceivedStats = _LiveFeedConnection.LastReceivedPacket.Publication;
                _monitorConnector.ReceivedTime = _LiveFeedConnection.LastReceivedPacket.Timestamp;
            }

            Dictionary<string, List<object>> dataStats = _theLiveFeedDataProvider.GetStats();
            lock (dataStats)
            {
                // Now, clear the data we have retrieved.
                foreach (string key in dataStats.Keys)
                {
                    _monitorConnector.TallyPublicationCounter(key);
                }
                dataStats.Clear();
            }
        }

        /// <summary>
        /// To start the user control
        /// </summary>
        public void Start()
        {
            if (Started)
                return;

            _theLiveFeedDataProvider.Start();
            _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

            // If the connection is started, fire up all of the display. 
            if (LiveFeedConnection.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Started)
            {
                if (!_monitorConnector.Started)
                {
                    _monitorConnector.Start();
                }

                if (!_monitorPublications.Started)
                {
                    _monitorPublications.Startup();
                }
            }
            else
            {
                if (_monitorConnector.Started)
                {
                    _monitorConnector.Stop();
                }

                if (_monitorPublications.Started)
                {
                    _monitorPublications.Cleanup();
                }
            }
        }

        /// <summary>
        /// To stop the user control
        /// </summary>
        public void Stop()
        {
            Stop(false);
        }

        /// <summary>
        /// To stop the user control
        /// </summary>
        /// <param name="shutdown"></param>
        public void Stop(bool shutdown)
        {
            if (shutdown)
            {
                Shutdown();
            }
            else
            {
                _theLiveFeedDataProvider.Stop();
                _widget1.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                CleanupAllDisplays();
                _started = false;
            }
        }

        /// <summary>
        /// Monitoring the connection state change.  We're interested in the Started and Removed events. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void InvokeHandleOnStateChanged(object sender, LiveFeedConnectionEventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new HandleLiveFeedConnectionEvents(LiveFeedConnection_OnStateChanged), new object[] { sender, (LiveFeedConnectionEventArgs)e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedMonitoring,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        /// <summary>
        /// handler for connection state change. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void LiveFeedConnection_OnStateChanged(object sender, LiveFeedConnectionEventArgs e)
        {
            if (e.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Started)
            {
                _theLiveFeedDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                if (!_monitorConnector.Started)
                {
                    _monitorConnector.Start();
                }

                if (!_monitorPublications.Started)
                {
                    _monitorPublications.Startup();
                }
            }

            else if (e.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Stopped)
            {
                Stop();
            }

            else if (e.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Disconnected)
            {
                Stop();
            }

            else if (e.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Removed)
            {
                // The LiveFeed Connection is going away, let's clean everything up. 
                Shutdown();
            }
        }

        /// <summary>
        /// To clean everything up in this Canvas.  
        /// </summary>
        private void Shutdown()
        {
            _theLiveFeedDataProvider.Stop();
            _LiveFeedConnection.OnStateChanged -= _LiveFeedConnectionStateChangeHandler;
            _widget1.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            CleanupAllDisplays();
            _started = false;
        }

        /// <summary>
        /// To clenup the all display
        /// </summary>
        private void CleanupAllDisplays()
        {
            if (_monitorConnector.Started)
            {
                _monitorConnector.Stop();
            }

            if (_monitorPublications.Started)
            {
                _monitorPublications.Cleanup();
            }
        }

        #endregion Private methods
    }
}

