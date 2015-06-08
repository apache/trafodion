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
using System.ComponentModel;
using System.Data;
using System.Threading;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// LiveFeed Monitor canvas 
    /// </summary>
    public partial class MonitorLiveFeedConnectorsCanvas : UserControl, ICloneToWindow
    {
        #region Fields

        public const int DefaultRefreshRate = 3000; // 3 seconds
        private const string TRACE_SUB_AREA_NAME = "MonitoringLiveFeedConnectors";

        private ConnectionDefinition _theConnectionDefinition = null;
        private DisplayConnectorsUserControl _theDisplayConnectorsUserControl = null;
        private DisplayPublicationsUserControl _theDisplayPublicationsUserControl = null;
        private string _theTitle = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private BackgroundWorker _backgroundWorker = null;
        private bool _started = false;
        private int _refreshRate = DefaultRefreshRate;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: The connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
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
        public MonitorLiveFeedConnectorsCanvas(ConnectionDefinition aConnectionDefinition, string aTitle)
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
        public MonitorLiveFeedConnectorsCanvas(MonitorLiveFeedConnectorsCanvas aMonitorLiveFeedConnectorsCanvas)
        {
            _theConnectionDefinition = aMonitorLiveFeedConnectorsCanvas.ConnectionDefn;
            _theTitle = aMonitorLiveFeedConnectorsCanvas.WindowTitle;
            InitializeComponent();
            ShowWidgets(aMonitorLiveFeedConnectorsCanvas);
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// The interface method for ICloneWindow.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            MonitorLiveFeedConnectorsCanvas theMonitorLiveFeedConnectorsCanvas = new MonitorLiveFeedConnectorsCanvas(this);
            return theMonitorLiveFeedConnectorsCanvas;
        }

        /// <summary>
        /// To start the monitoring 
        /// </summary>
        public void Start()
        {
            if (!_started)
            {
                _started = true;
                FireUpExectionThread();
            }
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
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
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
        private void ShowWidgets(MonitorLiveFeedConnectorsCanvas clone)
        {
            GridLayoutManager gridLayoutManager = new GridLayoutManager(6, 1);
            gridLayoutManager.CellSpacing = 4;
            this._theCanvas.LayoutManager = gridLayoutManager;

            // Add the connections monitor
            GridConstraint gridConstraint = new GridConstraint(0, 0, 3, 1);
            _theDisplayConnectorsUserControl = new DisplayConnectorsUserControl(_theConnectionDefinition);
            WidgetContainer widgetContainer = new WidgetContainer(this._theCanvas, _theDisplayConnectorsUserControl, "LiveFeed Connections");
            widgetContainer.Name = "LiveFeed Connections";
            widgetContainer.AllowDelete = false;
            this._theCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // Create the 2nd widget for monitoring the subscriptions
            gridConstraint = new GridConstraint(3, 0, 3, 1);
            _theDisplayPublicationsUserControl = new DisplayPublicationsUserControl(_theConnectionDefinition);
            widgetContainer = new WidgetContainer(this._theCanvas, _theDisplayPublicationsUserControl, "LiveFeed Publications");
            widgetContainer.Name = "LiveFeed Publications";
            widgetContainer.AllowDelete = false;
            this._theCanvas.AddWidget(widgetContainer, gridConstraint, -1);            

            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
        }

        private void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aConnectionDefinition == _theConnectionDefinition && 
                aReason == ConnectionDefinition.Reason.Removed)
            {
                // The system definition is being removed
                Shutdown();
            }
        }

        /// <summary>
        /// To clean everything up in this Canvas.  
        /// </summary>
        private void Shutdown()
        {
            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                   TraceOptions.TraceArea.LiveFeedFramework,
                   TRACE_SUB_AREA_NAME,
                   "Shutdown Monitoring LiveFeed Connectors.");

            _started = false;
            CleanupAllDisplays();
            CancelAsync();
        }

        /// <summary>
        /// Clean up all displays
        /// </summary>
        private void CleanupAllDisplays()
        {
            // nothing at this time
        }

        /// <summary>
        /// To refresh the monitoring displays
        /// </summary>
        private void DoRefresh()
        {
            DataTable table = _theDisplayConnectorsUserControl.DataTable;
            table.Rows.Clear();

            List<LiveFeedConnection> connections = LiveFeedConnectionRegistry.Instance.GetActiveLiveFeedConnections(_theConnectionDefinition);
            if (connections != null)
            {
                foreach (LiveFeedConnection conn in connections)
                {
                    if (conn.LiveFeedQueues.Count > 0)
                        table.Rows.Add(new object[] { conn.ConnectionDefn.Name, conn.LiveFeedQueues[0].Name, conn.TotalReceivedCount });
                }
            }

            _theDisplayConnectorsUserControl.ReloadData();

            DataTable table1 = _theDisplayPublicationsUserControl.DataTable;
            table1.Rows.Clear();

            foreach (string pubs in LiveFeedRoutingKeyMapper.AllPublicationNames)
            {
                int totalSubscribers = 0;
                if (connections != null)
                {
                    foreach (LiveFeedConnection conn in connections)
                    {
                        if (conn.Subscriptions.ContainsKey(pubs))
                        {
                            totalSubscribers += conn.Subscriptions[pubs];
                        }
                    }
                }
                table1.Rows.Add(new object[] { pubs, totalSubscribers });
            }

            _theDisplayPublicationsUserControl.ReloadData();
        }

                /// <summary>
        /// Start the execution thread for this control, which takes 
        /// the arriving stats out of the input queue and update the 
        /// data table in the control. 
        /// </summary>
        private void FireUpExectionThread()
        {
            // Initialize background worker
            _backgroundWorker = new BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
            _backgroundWorker.RunWorkerAsync();
        }

        /// <summary>
        /// To stop the background thread
        /// </summary>
        private void CancelAsync()
        {
            if (this._backgroundWorker != null)
            {
                this._backgroundWorker.CancelAsync();
                this._backgroundWorker.Dispose();
            }
        }

        /// <summary>
        /// The main loop of the background thread
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            BackgroundWorker worker = sender as BackgroundWorker;
            while (_started)
            {
                try
                {
                    if (e.Cancel || worker.CancellationPending)
                    {
                        _started = false;
                    }
                    else
                    {
                        worker.ReportProgress(0);
                        Thread.Sleep(_refreshRate);
                    }
                }
                catch (System.Threading.ThreadAbortException)
                {
                    //Thread aborted.
                }
                catch (Exception ex)
                {
                    if (_started)
                    {
                        MessageBox.Show("Display Queue Widget Exception: " + ex.Message);
                    }
                }
            }
        }

        /// <summary>
        /// Completion of background thread
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (e.Error != null && _started)
            {
                Exception ex = e.Error;
                MessageBox.Show("Display Queue Widget Exception: " + ex.Message);
            }
        }

        /// <summary>
        /// Change processing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void BackgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            this.DoRefresh();
        }

        #endregion Private Methods
    }
}

