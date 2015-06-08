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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// The data provider for LiveFeed
    /// </summary>
    public class LiveFeedDataProvider : GenericDataProvider
    {
        #region Fields

        public static string NewArrivalDataTableKey = "new_livefeed_datatable";

        protected DataTable _theDataTable = null;

        // this list keeps all of the LiveFeedSubscriptions, which have been subscribed to the connection.
        // The configuration (what publications to subscribe) is kept in LiveFeedDataProviderConfig. 
        private Dictionary<string, LiveFeedSubscription> _subscriptions = new Dictionary<string, LiveFeedSubscription>();

        protected LiveFeedSubscription.NewDataArrived _theDataHandler;
        private string _theLastReceivedPublication = null;

        // The live feed connection
        private LiveFeedConnection _LiveFeedConnection = null;

        // Live feed connection state events
        private LiveFeedConnection.LiveFeedConnectionStateChanged _LiveFeedConnectionChangeHandler = null;
        private bool _registeredLiveFeedConnectionEvent = false;
        private int _configuredRefreshRate = 0;
        private bool _timerStarted = false;
        private bool _started = false;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: LiveFeedConnection
        /// The LiveFeed Connection of this data provider
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            get { return _LiveFeedConnection; }
            set { _LiveFeedConnection = value; }
        }

        /// <summary>
        /// Property: DataProviderConfig
        /// The configuration object of this data provider
        /// </summary>
        public override DataProviderConfig DataProviderConfig
        {
            get
            {
                return base.DataProviderConfig;
            }
            set
            {
                base.DataProviderConfig = value;
            }
        }

        /// <summary>
        /// Property: Subscriptions - the dictionary of the LiveFeedSubscriptions
        /// </summary>
        public Dictionary<string, LiveFeedSubscription> Subscriptions
        {
            get { return _subscriptions; }
        }

        /// <summary>
        /// Property: LastReceivedPublication - the publication name of the last received package
        /// </summary>
        public string LastReceivedPublication
        {
            get { return _theLastReceivedPublication; }
            set { _theLastReceivedPublication = value; }
        }

        /// <summary>
        /// Property: ConfiguredRefreshRate - the configured refresh rate to be used for retries.
        /// </summary>
        public int ConfiguredRefreshRate
        {
            get { return _configuredRefreshRate; }
            set { _configuredRefreshRate = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor of this data provider
        /// </summary>
        /// <param name="aDataProviderConfig"></param>
        public LiveFeedDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
            _LiveFeedConnection = LiveFeedConnectionRegistry.Instance.GetOneLiveFeedConnection(((LiveFeedDataProviderConfig)aDataProviderConfig).ConnectionDefinition);
            _LiveFeedConnection.Open();
            _theDataHandler = new LiveFeedSubscription.NewDataArrived(LiveFeedSubscription_OnNewDataArrived);
            _LiveFeedConnectionChangeHandler = new LiveFeedConnection.LiveFeedConnectionStateChanged(LiveFeedConnection_OnStateChanged);
            _configuredRefreshRate = _LiveFeedConnection.BrokerConfiguration.SessionRetryTimer;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Returns the custom event args for the DatabaseDataProvider
        /// </summary>
        /// <returns></returns>
        public override DataProviderEventArgs GetDataProviderEventArgs()
        {
            DataProviderEventArgs evtArgs = new DataProviderEventArgs();
            return evtArgs;
        }

        /// <summary>
        /// Get a new data provider event arg with the publication as the property
        /// </summary>
        /// <param name="aPublication"></param>
        /// <returns></returns>
        public DataProviderEventArgs GetDataProviderEventArgs(string aPublication)
        {
            DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
            evtArgs.AddEventProperty("Publication", aPublication);
            return evtArgs;
        }

        /// <summary>
        /// To start the data provider
        /// </summary>
        public override void Start()
        {
            Start(new Hashtable());
        }

        /// <summary>
        /// Refreshes method
        /// </summary>
        public override void RefreshData()
        {
            // this is push technology, the data arrives by itself.
            // but, make sure it is started
            ReStart();
        }

        /// <summary>
        /// To restart the the data provider
        /// </summary>
        public void ReStart()
        {
            //if (!_started)
            //{
                InitiateConnection();
            //}
        }

        /// <summary>
        /// To start the data provider
        /// </summary>
        /// <param name="defaultParameters"></param>
        public override void Start(Hashtable defaultParameters)
        {
            //  The starting of the Livefeed conneciton is done in the Prefetch. 
            base.Start(defaultParameters);            
        }

        /// <summary>
        /// To stop the data provider
        /// </summary>
        public override void Stop()
        {
            if (_registeredLiveFeedConnectionEvent)
            {
                _LiveFeedConnection.OnStateChanged -= _LiveFeedConnectionChangeHandler;
                _registeredLiveFeedConnectionEvent = false;
            }

            CleanupAllSubscriptions();
            DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
            this.FireFetchCancelled(evtArgs);
            _LiveFeedConnection.Close("Data Provider Stop", true);
            _started = false;
            base.Stop();
        }

        /// <summary>
        // To cancel the data fetch. The effect is to shut down the data provider. 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchCancel(Trafodion.Manager.Framework.WorkerThread worker)
        {
            // We're treating this as the stop of the connection. But, since we're sharing the 
            // connection, we only dis-associate ourselves with the conneciton. 
            if (_registeredLiveFeedConnectionEvent)
            {
                _LiveFeedConnection.OnStateChanged -= _LiveFeedConnectionChangeHandler;
                _registeredLiveFeedConnectionEvent = false;
            }
            CleanupAllSubscriptions();
        }

        /// <summary>
        /// To handle a data fetch error.  The effect will shut down the data provider. 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchError(Trafodion.Manager.Framework.WorkerThread worker)
        {
            if (_registeredLiveFeedConnectionEvent)
            {
                _LiveFeedConnection.OnStateChanged -= _LiveFeedConnectionChangeHandler;
                _registeredLiveFeedConnectionEvent = false;
            }
            CleanupAllSubscriptions();
        }

        /// <summary>
        /// To perform prefrech setup if there is anything. 
        /// </summary>
        /// <param name="parameters"></param>
        public override void DoPrefetchSetup(System.Collections.Hashtable parameters)
        {
            InitiateConnection();
        }

        /// <summary>
        /// To perform data fetch.  Since this is a push connection, the data arrive on its own.  
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            // do nothing, this is a push. 
        }

        /// <summary>
        /// To report data fetch progress.  
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchProgress(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.ProgressChangedEventArgs e)
        {
            // do nothing, this is a push.
        }

        /// <summary>
        /// Returns the data as a datatable after it has been fetched 
        /// </summary>
        /// <returns></returns>
        public override DataTable GetDataTable()
        {
            return _theDataTable;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// to initiate the live feed conneciton
        /// </summary>
        private void InitiateConnection()
        {
            // Make sure all configured publications have been subscribed; this has to be done before start the live feed conneciton. 
            ScanSbuscriptionList();

            // Make sure the connection is started.
            if (!_LiveFeedConnection.Started)
            {
                try
                {
                    if (!_registeredLiveFeedConnectionEvent)
                    {
                        _LiveFeedConnection.OnStateChanged += _LiveFeedConnectionChangeHandler;
                        _registeredLiveFeedConnectionEvent = true;
                    }
                    _LiveFeedConnection.Start();
                }
                catch (Exception ex)
                {
                    DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                    evtArgs.Exception = ex;
                    FireErrorEncountered(evtArgs);
                    return;
                }
            }

            // If there is a timer, stop it. 
            StopRefreshTimer();
  
            // Fire up the get ready event for widgets to get ready.
            FireInitDataproviderForFetch(GetDataProviderEventArgs());

            _started = true;
        }


        /// <summary>
        /// Walk through our configured subscription list and make sure all of them 
        /// are properly subscribed to LiveFeed connection. 
        /// Note: this has to be done before the live feed connection is started.
        /// </summary>
        private void ScanSbuscriptionList()
        {
            // Walk through the configured list.  
            foreach (string key in ((LiveFeedDataProviderConfig)this.DataProviderConfig).SubscriptionConfiguration.Keys)
            {
                string[] filters = ((LiveFeedDataProviderConfig)this.DataProviderConfig).SubscriptionConfiguration[key] as string[];
                LiveFeedSubscription sub = null;
                _subscriptions.TryGetValue(key, out sub);
                if (null == sub)
                {
                    // Go ahead to subscribe to the connection. 
                    SubScribes(key, filters);
                }
            }
        }

        /// <summary>
        /// Walk through our subscription list and cleanup 
        /// all of them.  
        /// Note: Usually, this is done when we just detect a connection lost or 
        /// the widget just issue a stop command. 
        /// </summary>
        private void CleanupAllSubscriptions()
        {
            foreach (string key in ((LiveFeedDataProviderConfig)this.DataProviderConfig).SubscriptionConfiguration.Keys)

            {
                LiveFeedSubscription sub = null;
                _subscriptions.TryGetValue(key, out sub);
                if (null != sub)
                {
                    UnSubscribe(key);
                }
            }
        }

        /// <summary>
        /// To subscription a publication to an LiveFeed conneciton. 
        /// </summary>
        /// <param name="aPublication"></param>
        private void SubScribes(string aPublication, string[] aFilters)
        {
            if (!_subscriptions.ContainsKey(aPublication))
            {
                LiveFeedSubscription sub = _LiveFeedConnection.Subscribe(aPublication, aFilters);
                _subscriptions.Add(aPublication, sub);
                this.AttachDataHandler(sub);
            }
        }

        /// <summary>
        /// To unsubscribe a publication. 
        /// </summary>
        /// <param name="aPublication"></param>
        private void UnSubscribe(string aPublication)
        {
            LiveFeedSubscription sub = null;
            _subscriptions.TryGetValue(aPublication, out sub);
            if (null != sub)
            {
                this.ReleaseDataHandler(sub);
                sub.Cleanup();
                _subscriptions.Remove(aPublication);
            }
        }

        /// <summary>
        /// Release data handler from a subscription.
        /// </summary>
        /// <param name="sub"></param>
        private void ReleaseDataHandler(LiveFeedSubscription sub)
        {
            if (this._theDataHandler != null)
            {
                sub.OnNewDataArrived -= this._theDataHandler;
            }
        }

        /// <summary>
        /// To attach a data handler to a subscription.
        /// </summary>
        /// <param name="sub"></param>
        private void AttachDataHandler(LiveFeedSubscription sub)
        {
            if (this._theDataHandler != null)
            {
                sub.OnNewDataArrived += this._theDataHandler;
            }
        }

        /// <summary>
        /// Event handler for the LiveFeedConnection change event. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void LiveFeedConnection_OnStateChanged(object sender, LiveFeedConnectionEventArgs e)
        {
            if (e.CurrentState == Trafodion.Manager.LiveFeedFramework.Models.LiveFeedConnection.LiveFeedConnectionState.Started)
            {
                ScanSbuscriptionList();
                StopRefreshTimer();

                // Fire up the get ready event for widgets to get ready.
                DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                FireInitDataproviderForFetch(evtArgs);
                _started = true;
            }
            else if (e.CurrentState == Trafodion.Manager.LiveFeedFramework.Models.LiveFeedConnection.LiveFeedConnectionState.Stopped)
            {
                Stop();
                DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                if ((e.Exception == null))
                {
                    evtArgs.Exception = new Exception((e.Reason == null ? "LiveFeed connection lost" : e.Reason));
                }
                else
                {
                    evtArgs.Exception = e.Exception;
                }
                FireErrorEncountered(evtArgs);

                // In the meantime, enable the timer if it is desired.  This will cause the restart to happen
                StartRefreshTimer();
                _started = false;
            }
            else if (e.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Disconnected)
            {
                Stop();
                _started = false;
            }
            else if (e.CurrentState == LiveFeedConnection.LiveFeedConnectionState.Removed)
            {
                Stop();
            }
        }

        /// <summary>
        /// Event handler for the LiveFeedSubscription data arrival event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public virtual void LiveFeedSubscription_OnNewDataArrived(object sender, EventArgs e)
        {
            LiveFeedSubscription sub = (LiveFeedSubscription)sender;
            _theLastReceivedPublication = sub.PublicationName;
            _theDataTable = sub.GetDataTableStats();
            DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
            FireNewDataArrived(evtArgs);
        }

        /// <summary>
        /// Stop the refresh timer.  Since LiveFeed connection is a push technology, there is no need
        /// to refresh the "fetching".  So, we want to stop the timer. 
        /// </summary>
        private void StopRefreshTimer()
        {
            if (_timerStarted)
            {
                StopTimer();
                _timerStarted = false;
                //_configuredRefreshRate = RefreshRate; // in case the user has selected a different rate.
                RefreshRate = 0;
            }
        }

        /// <summary>
        /// Start the refresh timer.  This is needed when the LiveFeed connection has lost and the
        /// refresh timer will cause the re-start to occur.  As a result, the LiveFeed session may be
        /// re-established. 
        /// Note: Manupulateing the RefreshRate to disable the "timer paused" status on the widget.
        /// </summary>
        private void StartRefreshTimer()
        {
            if (_timerStarted)
            {
                return;
            }

            RefreshRate = _configuredRefreshRate;
            if (RefreshRate != 0)
            {
                StartTimer();
                _timerStarted = true;
            }
        }

        #endregion Private methods
    }
}
