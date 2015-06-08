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
using System.Threading;
using System.Windows.Forms;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.UniversalWidget;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls
{
    /// <summary>
    /// User control to monitor subscriptions used by TestLiveFeedConnector
    /// </summary>
    public partial class DisplayTestSubscriptionsUserControl : UserControl, IDataDisplayControl
    {

        #region Fields

        private const int DefaultRefreshRate = 3000; // refreseh every 3 seconds
        private const string ThePublicationNameColumn = "thePublicationNameColumn";
        private const string TheSubscriptionCountColumn = "theSubscriptionCountColumn";
        private const string TheQueueLengthColumn = "theQueueLengthColumn";

        private Dictionary<string, Label> _theSubscriptionLabels = new Dictionary<string, Label>();
        private LiveFeedConnection _theLiveFeedConnection = null;
        private bool _started = false;
        private BackgroundWorker _theBackgroundWorker = null;
        private DataProvider _theDataProvider = null;
        private UniversalWidgetConfig _theWidgetConfig = null;
        private IDataDisplayHandler _theDataDisplayHandler = null;
        private int _theRefreshRate = DefaultRefreshRate;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Started - the user control is started
        /// </summary>
        public bool Started
        {
            get { return (_started); }
        }

        /// <summary>
        /// Property: LiveFeedConnection - the live feed connection used by this user control
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            set { _theLiveFeedConnection = value; }
            get { return _theLiveFeedConnection; }
        }

        /// <summary>
        /// Property: DataProvider - the data provider used by this widget
        /// </summary>
        public DataProvider DataProvider
        {
            get { return _theDataProvider; }
            set { _theDataProvider = value; }
        }

        /// <summary>
        /// Proerpty: UniversalWidgetConfiguration - the configuration of the widget
        /// </summary>
        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get { return _theWidgetConfig; }
            set { _theWidgetConfig = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - the data display handler
        /// </summary>
        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _theDataDisplayHandler; }
            set { _theDataDisplayHandler = value; }
        }

        /// <summary>
        /// Property: DataDisplayHandler - not used at this time
        /// </summary>
        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        /// <summary>
        /// Property: RefreshRate - the refresh rate for monitoring the connector
        /// </summary>
        public int RefreshRate
        {
            get { return _theRefreshRate; }
            set { _theRefreshRate = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor 
        /// </summary>
        /// <param name="aLiveFeedConnection"></param>
        public DisplayTestSubscriptionsUserControl(LiveFeedConnection aLiveFeedConnection)
        {
            _theLiveFeedConnection = aLiveFeedConnection;
            InitializeComponent();
        }

        #endregion Constructors

        #region Public Methods

        /// <summary>
        /// To start up the widget 
        /// </summary>
        public void Startup()
        {
            foreach (string publication in _theLiveFeedConnection.AllPublications)
            {
                iGRow row = _pubsGrid.Rows.Add();
                row.Cells[ThePublicationNameColumn].Value = publication;
                row.Cells[TheSubscriptionCountColumn].Value = 0;
                row.Cells[TheQueueLengthColumn].Value = 0;
            }

            FireUpExectionThread();
            _started = true;
        }

        /// <summary>
        /// Clean up every thing here. 
        /// </summary>
        public void Cleanup()
        {
            this._pubsGrid.Rows.Clear();
            if (this._theBackgroundWorker != null)
            {
                this._theBackgroundWorker.CancelAsync();
                this._theBackgroundWorker = null;
            }
            _started = false;
        }

        /// <summary>
        /// Persistent configruation - this is not used. But, for need to be here to fulfill the requirement of the interface
        /// </summary>
        public void PersistConfiguration()
        {
        }

        /// <summary>
        /// Refresh the display of the publication stats
        /// </summary>
        new public void Refresh()
        {
            Dictionary<string, LiveFeedSubscription> subscriptions = ((LiveFeedDataProvider)DataProvider).Subscriptions;
            if (subscriptions != null)
            {
                if (_theLiveFeedConnection.AllPublications != null)
                {
                    foreach (string pub in _theLiveFeedConnection.AllPublications)
                    {
                        int count = 0;
                        int length = 0;
                        LiveFeedSubscription sub = null;
                        subscriptions.TryGetValue(pub, out sub);
                        if (sub != null)
                        {
                            UpdateAPub(pub, 1, sub.QueueLength);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Start the execution thread for this control, which takes 
        /// the arriving stats out of the input queue and update the 
        /// data table in the control. 
        /// </summary>
        public void FireUpExectionThread()
        {
            // Initialize background worker
            _theBackgroundWorker = new BackgroundWorker();
            _theBackgroundWorker.WorkerReportsProgress = true;
            _theBackgroundWorker.WorkerSupportsCancellation = true;
            _theBackgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _theBackgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _theBackgroundWorker.ProgressChanged += new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
            _theBackgroundWorker.RunWorkerAsync();
        }

        #endregion Public Methods

        #region Prviate Methods

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                Cleanup();
            }
        }

        /// <summary>
        /// Update a publication stats
        /// </summary>
        /// <param name="aPubName"></param>
        /// <param name="subscriptions"></param>
        /// <param name="length"></param>
        private void UpdateAPub(string aPubName, int subscriptions, int length)
        {
            foreach (iGRow row in _pubsGrid.Rows)
            {
                string name = row.Cells[ThePublicationNameColumn].Value as string;
                if (name.Equals(aPubName))
                {
                    row.Cells[TheSubscriptionCountColumn].Value = subscriptions;
                    row.Cells[TheQueueLengthColumn].Value = length;
                    return;
                }
            }
        }

        /// <summary>
        /// Cancel the background worker
        /// </summary>
        private void CancelAsync()
        {
            if ((this._theBackgroundWorker != null) && (this._theBackgroundWorker.IsBusy))
            {
                this._theBackgroundWorker.CancelAsync();
            }
        }

        /// <summary>
        /// The main loop of the background worker. 
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
                        Thread.Sleep(_theRefreshRate);
                    }
                }
                catch (System.Threading.ThreadAbortException)
                {
                    //Thread aborted; do nothing.
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
        /// Processing when the background worker is done. 
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
        /// Processing the update of the test connector
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void BackgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            this.Refresh();
        }

        #endregion Private Methods
    }

}

