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
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using LiveFeedManaged;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    public class LiveFeedConnection
    {
        #region Fields

        public enum LiveFeedConnectionState { Stopped = 0, Starting, Started, Removed, Disconnected };
        public delegate void LiveFeedConnectionStateChanged(object sender, LiveFeedConnectionEventArgs eArgs);
        public LiveFeedConnectionStateChanged OnStateChanged;

        // Define trace sub area. 
        private const string TRACE_SUB_AREA_NAME = "LiveFeedConnection";

        // Define the filtering types
        public const string LV_FILTER_TYPE_ANY = "ANY";
        public const string LV_FILTER_TYPE_ALL = "ALL";

        // The broker configuration.
        private LiveFeedBrokerConfiguration _theBrokerConfiguration = null;

        // The connector receive queue. 
        private ReceiveQueue _connectorReceiveQueue = new ReceiveQueue("Connector.ReceiveQueue");

        private ConnectionDefinition _theConnectionDefinition;

        private LiveFeedConnectionState _currentState = LiveFeedConnectionState.Stopped;

        private LiveFeedConnectionState _lastState = LiveFeedConnectionState.Stopped;

        // The list of possible publications for this connection.
        private string[] _allPublications = null;

        // The reason for the last failure
        private string _lastFailedReason = "";

        // Event handler list
        private EventHandlerList theEventHandlers = new EventHandlerList();

        // Timer for monitoring session
        private System.Threading.Timer _sessionMonitorTimer = null;
        private TimerCallback _sessionMonitorTimerCB = null;

        private const string SHUTDOWN_COMMAND = "Shutdown";

        // Keep track of all of the received packages in the background worker.
        private List<ReceivedPackage> _backgroundReceivedPackages = new List<ReceivedPackage>();

        // The background worker
        private BackgroundWorker _backgroundWorker = null;

        // The total number of received packages
        private ulong _totalReceivedCount = 0;

        // The total number of bad packages received
        private ulong _badReceivedCount = 0;

        // The unique registration ID to identify this TrafodionManager to the broker.
        private string _theLiveFeedCustomRegistrationID = null;

        // Keep track all of the subscriptions on this connection.
        private Dictionary<string, int> _subscriptions = new Dictionary<string, int>();
        //private Hashtable _subHT = new Hashtable();

        // There should be only one at this time. 
        private List<LiveFeedMessageQueue> _theLiveFeedQueues = new List<LiveFeedMessageQueue>();

        // Worker thread
        private Trafodion.Manager.Framework.WorkerThread _WorkerThread = null;
        private Trafodion.Manager.Framework.WorkerThread _ReceiveQueueWorkerThread = null;

        // Livefeed Listener
        private LiveFeedManaged.ManagedExternalListener _theLiveFeedListener = null;

        // Last received package
        private ReceivedPackage _theLastReceivedPackage = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ConnectionDefn
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: LiveFeedCustomRegistrationID
        /// </summary>
        public string LiveFeedCustomRegistrationID
        {
            get { return _theLiveFeedCustomRegistrationID; }
            set { _theLiveFeedCustomRegistrationID = value; }
        }

        /// <summary>
        /// Property: Started?
        /// </summary>
        public bool Started
        {
            get { return (CurrentState == LiveFeedConnectionState.Started); }
        }

        /// <summary>
        /// Property: CurrentState - Current state of the connection
        /// </summary>
        public LiveFeedConnectionState CurrentState
        {
            get { return _currentState; }
            set { _currentState = value; }
        }

        /// <summary>
        /// Property: LastState - Last state of the connection
        /// </summary>
        public LiveFeedConnectionState LastState
        {
            get { return _lastState; }
            set { _lastState = value; }
        }

        /// <summary>
        /// Property: LastFailedReason - Reason of the last failed conneciton
        /// </summary>
        public String LastFailedReason
        {
            get { return _lastFailedReason; }
            set { _lastFailedReason = value; }
        }

        /// <summary>
        /// Property: ConnectorReceiveQueue - This receive queue keeps all of the received LiveFeed data in the raw format.
        /// </summary>
        public ReceiveQueue ConnectorReceiveQueue
        {
            get { return _connectorReceiveQueue; }
        }

        /// <summary>
        /// Property: TotalReceivedCount - total packages received by this connection
        /// </summary>
        public ulong TotalReceivedCount
        {
            get { return _totalReceivedCount; }
            set { _totalReceivedCount = value; }
        }

        /// <summary>
        /// Property: BadReceivedCount - the number of bad packages received
        /// </summary>
        public ulong BadReceivedCount
        {
            get { return _badReceivedCount; }
            set { _badReceivedCount = value; }
        }

        /// <summary>
        /// Property: Subscriptions - Lists of all current subscriptions.
        /// Note: For now, there should be only one Subscription for one connection.
        /// </summary>
        public Dictionary<string, int> Subscriptions
        {
            get { return _subscriptions; }
            set { _subscriptions = value; }
        }

        /// <summary>
        /// Property: BrokerConfiguration - the configuration of the live feed broker
        /// </summary>
        public LiveFeedBrokerConfiguration BrokerConfiguration
        {
            get { return _theBrokerConfiguration; }
        }

        /// <summary>
        /// Property: Queues - the queues for this connection; but for now there should be only one queue in a conneciton
        /// </summary>
        public List<LiveFeedMessageQueue> LiveFeedQueues
        {
            get { return _theLiveFeedQueues; }
        }

        /// <summary>
        /// Property: AllPublications
        /// All publications published in the connection
        /// [TBD] in the future, the backend should come up a method to give us this list dynamically.
        /// </summary>
        public string[] AllPublications
        {
            set { _allPublications = value; }
            get { return _allPublications; }
        }

        /// <summary>
        /// Property: LastReceivedPacket - the package just received
        /// </summary>
        public ReceivedPackage LastReceivedPacket
        {
            get { return _theLastReceivedPackage; }
            set { _theLastReceivedPackage = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public LiveFeedConnection(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
            _theLiveFeedCustomRegistrationID = string.Format("{0}:{1}", SystemInformation.ComputerName, Process.GetCurrentProcess().Id.ToString());
            _theBrokerConfiguration = new LiveFeedBrokerConfiguration(aConnectionDefinition);

            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
            ConnectionDefinition.OnLiveFeedPropertyChanged += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_OnLiveFeedPropertyChanged);
            // We'll be expacting all of the defined publications
            this._allPublications = LiveFeedRoutingKeyMapper.AllPublicationNames;

            // Now, it's time to fire up the background thread handling the call setups.
            InitializeBackgoundWorkers();
        }

        ~LiveFeedConnection()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }
        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To open the LiveFeed connection 
        /// [TBD] this is not really doing anything at this time. We may want to simplify it by removing this. 
        /// </summary>
        public void Open()
        {
            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                               String.Format("Open to Live Feed Host {0}:{1} - {2}.",
                                             ConnectionDefn.LiveFeedHostName,
                                             ConnectionDefn.LiveFeedPort,
                                             ConnectionDefn.UserName));

            // Time to fire a state changed event.
            FireOnStateChanged(new LiveFeedConnectionEventArgs(_lastState, CurrentState, Properties.Resources.Opened));
        }

        /// <summary>
        /// To start the LiveFeed connection 
        /// </summary>
        public void Start()
        {
            if (LiveFeedQueues.Count <= 0)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                   String.Format("Starting with no subscriptions to Live Feed Host {0}:{1} - {2}.",
                                 ConnectionDefn.LiveFeedHostName,
                                 ConnectionDefn.LiveFeedPort,
                                 ConnectionDefn.UserName));
                return;
            }
            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                               String.Format("Connecting to Live Feed Host {0}:{1} - {2}.", 
                                             ConnectionDefn.LiveFeedHostName, 
                                             ConnectionDefn.LiveFeedPort, 
                                             ConnectionDefn.UserName));

            // Go ahead to start the connection etc. 
            _WorkerThread.RunWorkerAsync();
            _ReceiveQueueWorkerThread.RunWorkerAsync();

            LiveFeedConnectionRegistry.Instance.AddToActiveRegistry(this);
            
            // Also, let's reset the stats counter
            TotalReceivedCount = 0;
            BadReceivedCount = 0;

            CurrentState = LiveFeedConnectionState.Started;

            // Time to fire a state changed event.
            FireOnStateChanged(new LiveFeedConnectionEventArgs(_lastState, CurrentState, "Started"));
        }

        /// <summary>
        /// To close this LiveFeed connection 
        /// </summary>
        /// <param name="aReason"></param>
        public void Close(string aReason, bool toCancelBackgroundThreads)
        {
            if (CurrentState == LiveFeedConnectionState.Stopped)
            {
                return;
            }

            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                    String.Format("Closing connection to Live Feed Host {0}:{1} - {2} due to {3}.",
                                             ConnectionDefn.LiveFeedHostName, 
                                             ConnectionDefn.LiveFeedPort, 
                                             ConnectionDefn.UserName,
                                             aReason));

#if DEBUG
            Console.WriteLine(String.Format("Closing connection to Live Feed Host {0}:{1} - {2} due to {3}.",
                                             ConnectionDefn.LiveFeedHostName,
                                             ConnectionDefn.LiveFeedPort,
                                             ConnectionDefn.UserName,
                                             aReason));
#endif

            // To signal the backgroundworker thread to shut down. 
            if (toCancelBackgroundThreads)
            {
                this.CancelAsync();
            }

            this.CancelAsyncReceiveQueueWorkerThread();
            _connectorReceiveQueue.Clear();

            _lastState = CurrentState;
            _lastFailedReason = aReason;
            CurrentState = LiveFeedConnectionState.Stopped;

            // Time to fire a state changed event.
            FireOnStateChanged(new LiveFeedConnectionEventArgs(_lastState, CurrentState, _lastFailedReason));
            LiveFeedConnectionRegistry.Instance.RemoveFromActiveRegistry(this);
        }

        /// <summary>
        /// Reconfigure the server.
        /// [TBD] is there a way, the re-connect can be done automatically? Currently, the subscription is cleaned up at  close, and only the upper layer
        /// could redo the re-subscriptions. 
        /// </summary>
        public void ReConfigure()
        {
            this._theBrokerConfiguration.Host = ConnectionDefn.LiveFeedHostName;
            this._theBrokerConfiguration.PortNumber = ConnectionDefn.LiveFeedPort;
            this._theBrokerConfiguration.SessionRetryTimerNumber = ConnectionDefn.LiveFeedRetryTimer;
        }

        /// <summary>
        /// To subscribe to a publication
        /// </summary>
        /// <param name="aPublication"></param>
        public LiveFeedSubscription Subscribe(string aPublication)
        {
            LiveFeedMessageQueue queue = null;
            LiveFeedSubscription sub = null;

            if (LiveFeedQueues.Count == 0)
            {
                queue = new LiveFeedMessageQueue(this);
                LiveFeedQueues.Add(queue);
                sub = queue.Subscribe(aPublication);
            }
            else
            {
                queue = LiveFeedQueues[0];
                
                // try to eliminate duplication here
                queue.UnSubscribe(aPublication);
                sub = queue.Subscribe(aPublication);
            }

            if (!Subscriptions.ContainsKey(aPublication))
            {
                // since there will be only one at all time.
                Subscriptions.Add(aPublication, 1);
            }

            return sub;
        }

        /// <summary>
        /// To subscribe a publcation with a filter
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aFilter"></param>
        /// <returns></returns>
        public LiveFeedSubscription Subscribe(string aPublication, string[] aFilters)
        {
            LiveFeedSubscription sub = Subscribe(aPublication);
            if (sub != null)
                sub.Filters = aFilters;

            return sub;
        }

        /// <summary>
        /// To unsubscribe a publication 
        /// </summary>
        /// <param name="aPublication"></param>
        public void UnSubscribe(string aPublication)
        {
            if (LiveFeedQueues.Count > 0)
            {
                LiveFeedMessageQueue que = LiveFeedQueues[0];
                foreach (LiveFeedSubscription sub in que.Subscriptions)
                {
                    if (sub.PublicationName.Equals(aPublication))
                    {
                        que.UnSubscribe(sub);
                    }
                }

                if (que.Subscriptions.Count == 0)
                {
                    LiveFeedQueues.Remove(que);
                }
            }

            if (Subscriptions.ContainsKey(aPublication))
            {
                Subscriptions[aPublication]--;
                Subscriptions.Remove(aPublication);
            }
            
        }

        /// <summary>
        /// To drop a single subscription and close the connection if no subscription is left.
        /// </summary>
        /// <param name="anLiveFeedSubscription"></param>
        public void UnSubscribe(LiveFeedSubscription aLiveFeedSubscription)
        {
            UnSubscribe(aLiveFeedSubscription, false);
        }

        /// <summary>
        /// To drop a single subscription from a publication. 
        /// Note: This is to be called by the LiveFeedSubscrption.  The LiveFeedSubscription is 
        ///       responsible for cleaning up ist's own allocated resources (e.g. ReceiveQueue).
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aLiveFeedSubscription"></param>
        public void UnSubscribe(LiveFeedSubscription aLiveFeedSubscription, bool aKeepConnection)
        {
            string publication = aLiveFeedSubscription.PublicationName;
            if (LiveFeedQueues.Count > 0)
            {
                LiveFeedMessageQueue que = LiveFeedQueues[0];
                que.UnSubscribe(aLiveFeedSubscription);

                if (que.Subscriptions.Count == 0)
                {
                    LiveFeedQueues.Remove(que);
                }
            }

            if (Subscriptions.ContainsKey(publication))
            {
                Subscriptions[publication]--;
                Subscriptions.Remove(publication);
            }
            
        }

        /// <summary>
        /// UnSubscribe the entire list of subscriptions
        /// </summary>
        /// <param name="anLiveFeedSubs"></param>
        public void UnSubscribe()
        {
            foreach (LiveFeedMessageQueue que in LiveFeedQueues)
            {
                que.Subscriptions.Clear();
            }

            LiveFeedQueues.Clear();

            Close("All unsubscribed", true);
        }

        /// <summary>
        /// Fire the state change event
        /// </summary>
        /// <param name="e"></param>
        public void FireOnStateChanged(LiveFeedConnectionEventArgs e)
        {
            if (OnStateChanged != null)
            {
                OnStateChanged(this, e);
            }
        }

        /// <summary>
        /// The delegate event handler for Data arrival.
        /// </summary>
        /// <param name="aSender"></param>
        public delegate void OnDataArrivalHandler(object aSender);

        [NonSerialized()]
        private static readonly string theOnDataArrivalKey = "OnDataArrival";

        /// <summary>
        /// Add an event handler to, or remove one from, the list
        /// </summary>
        public event OnDataArrivalHandler OnDataArrival
        {
            add { theEventHandlers.AddHandler(theOnDataArrivalKey, value); }
            remove { theEventHandlers.RemoveHandler(theOnDataArrivalKey, value); }
        }

        /// <summary>
        /// Fire an event with this connection definition as the source
        /// </summary>
        public void FireOnDataArrival()
        {
            // Get the list of the right kind of handlers
            OnDataArrivalHandler theHandlers = (OnDataArrivalHandler)theEventHandlers[theOnDataArrivalKey];

            // Check to see if there any
            if (theHandlers != null)
            {
                // Multicast to them all
                theHandlers(this);
            }
        }

        /// <summary>
        /// This is the callback Receive() method from the C++ API. It receives a single message from the backend.
        /// [TBD] the other option is to create another listning thread, which does all of these time consuming 
        /// deserialization works. That way, the receive method can quickly go back to read another message.  But, 
        /// again, when we are not finished with one message, can we afford to read the next message? 
        /// </summary>
        /// <param name="data"></param>
        /// <param name="size"></param>
        /// <param name="routingKey"></param>
        public void ReceivePackages(byte[] data, Int32 size, string routingKey)
        {
            Logger.OutputToLog(TraceOptions.TraceOption.LIVEFEEDDATA,
                   TraceOptions.TraceArea.LiveFeedFramework,
                   TRACE_SUB_AREA_NAME,
                   String.Format("Received a package with routing key = {0}; size = {1}.", routingKey, size));

            // processing data only when there are subscribers
            string publication = LiveFeedRoutingKeyMapper.GetPublication(routingKey);
            if (Subscriptions.ContainsKey(publication))
            {
                MemoryStream ms = new MemoryStream(data);
                BinaryReader reader = new BinaryReader(ms, Encoding.UTF8);
                DeserializeAndQueue(publication, reader);
                reader.Close();
            }
        }

        public string DoTest()
        {
            return DoTest(this.ConnectionDefn.UserName, this.ConnectionDefn.Password);
        }

        public string DoTest(string user, string password)
        {
            string errorMsg = string.Empty;
            try
            {
                string queueName = LiveFeedMessageQueue.GetUniqueueName(this);
                errorMsg = DoConnect(queueName, user, password);
                _theLiveFeedListener.closeAMQPConnection();
            }
            catch (Exception ex)
            {
                errorMsg = ex.Message;
            }
            return errorMsg;            
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// There is a change occurred in the ConnectionDefinitions. But, we're only interested in the LiveFeed changes
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            DoReConfigure(aConnectionDefinition, aReason);
        }
        /// <summary>
        /// This we deal with LiveFeed Properties Changes
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        void ConnectionDefinition_OnLiveFeedPropertyChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            DoReConfigure(aConnectionDefinition, aReason);
        }

        private void DoReConfigure(ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason) 
        {
            if ((aReason == ConnectionDefinition.Reason.LiveFeedPort ||
                 aReason == ConnectionDefinition.Reason.LiveFeedRetryTimer) &&
                aConnectionDefinition == _theConnectionDefinition)
            {
                this.ReConfigure();
            }
        }

        private string DoConnect(string queueName, string user, string password)
        {
            // First to create a new ExternalListener. Since the current Live Feed API does not do well in reusing the ExternalListener
            // we jsut not trying to re-use it at this time. 
            if (_theBrokerConfiguration.Port == -1)
            {
                _theLiveFeedListener = new ManagedExternalListener(_theBrokerConfiguration.Host, user, password);
            }
            else
            {
                _theLiveFeedListener = new ManagedExternalListener(_theBrokerConfiguration.Host, _theBrokerConfiguration.Port, user, password);
            }

            int error = _theLiveFeedListener.getLastError();
            if (error != 0)
            {
                string errMsg = _theLiveFeedListener.getCodeText(error);
                string message = string.Format("Live Feed Connection Test {0} ({1}:{2}) : declare queue {3} failed with error = {4}:{5}",
                    this.ConnectionDefn.Name,
                    this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                    queueName, error,errMsg.Equals(string.Empty)?"NULL.":errMsg);
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, message);
                return errMsg;
            }

            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                               TraceOptions.TraceArea.LiveFeedFramework,
                               TRACE_SUB_AREA_NAME,
                               String.Format("Live Feed Connection Test {0} ({1}:{2}) : declare queue {3} successful.",
                                             this.ConnectionDefn.Name,
                                             this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                             queueName));
            return string.Empty;

        }

        /// <summary>
        /// To deserialize the incoming package and file an new data arrival event to the subscriber.
        /// </summary>
        /// <param name="aPublication"></param>
        /// <param name="aReader"></param>
        /// <returns></returns>
        private ReceivedPackage DeserializeAndQueue(string aPublication, BinaryReader aReader)
        {
            Object stats = LiveFeedStatsTransformer.Deserialize(aPublication, aReader, this);
            ReceivedPackage package = new ReceivedPackage(aPublication, stats, DateTime.Now);
            LastReceivedPacket = package;
            TotalReceivedCount++;
            ConnectorReceiveQueue.Enqueue(package);
            return package;
        }

        /// <summary>
        /// Set up the WorkerThread object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorkers()
        {
            //Before do that, first prepare for the case which user has canceled the request.
            _WorkerThread = new Trafodion.Manager.Framework.WorkerThread();
            //_WorkerThread.
            _WorkerThread.DoWork = WorkerThread_DoWork;
            _WorkerThread.RunWorkerCompleted = WorkerThread_RunWorkerCompleted;

            //Now, also initialize the background worker for processing received package
            _ReceiveQueueWorkerThread = new Trafodion.Manager.Framework.WorkerThread();
            _ReceiveQueueWorkerThread.DoWork = BackgroundWorker_DoWork;
            _ReceiveQueueWorkerThread.RunWorkerCompleted = BackgroundWorker_RunWorkerCompleted;
            _ReceiveQueueWorkerThread.ProgressChanged = BackgroundWorker_ProgressChanged;
            _backgroundReceivedPackages.Clear();
        }

        /// <summary>
        /// Cancel the running background thread 
        /// </summary>
        private void CancelAsync()
        {
            if ((this._WorkerThread != null) && (this._WorkerThread.IsBusy))
            {
                if (_theLiveFeedListener != null)
                {
                    int error = 0;
                    Thread closeThread = new Thread(new ThreadStart(() => { error=_theLiveFeedListener.closeAMQPConnection(); }));
                    closeThread.IsBackground = true;
                    closeThread.Start();
                    //int error = _theLiveFeedListener.closeAMQPConnection();                    

                    if (error != 0)
                    {
                        string errMsg = _theLiveFeedListener.getCodeText(error);
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("closeAMQPConnection() failed with error - {0}:{1}", error,errMsg.Equals(string.Empty)?"NULL.":errMsg));
                    }
                    else
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("closeAMQPConnection() succeeded.", error));
                    }
                }

                if (this.LiveFeedQueues.Count > 0)
                {
                    LiveFeedMessageQueue que = this.LiveFeedQueues[0];
                    que.Subscriptions.Clear();
                    this.LiveFeedQueues.Clear();
                }

                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                   String.Format("Cancel listener worker thread."));
                this._WorkerThread.CancelAsync();
            }
        }

        private void CancelAsyncReceiveQueueWorkerThread()
        {
            if ((this._ReceiveQueueWorkerThread != null) && (this._ReceiveQueueWorkerThread.IsBusy))
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                   String.Format("Cancel ReceiveQueue worker thread."));
                this._ReceiveQueueWorkerThread.CancelAsync();
            }
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void  WorkerThread_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the WorkerThread that raised this event.
            Trafodion.Manager.Framework.WorkerThread worker = sender as Trafodion.Manager.Framework.WorkerThread;
            int error = 0;

            try
            {
                if (this.LiveFeedQueues.Count > 0)
                {
                    // First to create a new ExternalListener. Since the current Live Feed API does not do well in reusing the ExternalListener
                    // we jsut not trying to re-use it at this time. 
                    if (_theBrokerConfiguration.Port == -1)
                    {
                        if (string.IsNullOrEmpty(this.ConnectionDefn.PlatformReleaseVersion) ||
                            ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                        {
                            _theLiveFeedListener = new ManagedExternalListener(_theBrokerConfiguration.Host, ConnectionDefn.UserName, ConnectionDefn.Password);
                        }
                        else
                        {
                            _theLiveFeedListener = new ManagedExternalListener(_theBrokerConfiguration.Host, "", "");
                        }
                    }
                    else
                    {
                        if (string.IsNullOrEmpty(this.ConnectionDefn.PlatformReleaseVersion) ||
                            ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                        {
                            _theLiveFeedListener = new ManagedExternalListener(_theBrokerConfiguration.Host, _theBrokerConfiguration.Port, ConnectionDefn.UserName, ConnectionDefn.Password);
                        }
                        else
                        {
                            _theLiveFeedListener = new ManagedExternalListener(_theBrokerConfiguration.Host, _theBrokerConfiguration.Port, "", "");
                        }
                    }

                    LiveFeedMessageQueue que = this.LiveFeedQueues[0];

                    error = _theLiveFeedListener.getLastError();
                    if (error != 0)
                    {
                        string errMsg = _theLiveFeedListener.getCodeText(error);
                        string message = string.Format("Live Feed Connection {0} ({1}:{2}) : Connect {3} failed with error = {4}:{5}",
                                        this.ConnectionDefn.Name,
                                        this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                        que.Name, error, errMsg.Equals(string.Empty) ? "NULL." : errMsg);
                        throw new Exception(errMsg);
                    }

                    // Create the External listener and register the callback receive method
                    LiveFeedManaged.ManagedExternalListener.CSharpCallbackDelegate cb = new LiveFeedManaged.ManagedExternalListener.CSharpCallbackDelegate(this.ReceivePackages);
                    _theLiveFeedListener.registerCallback(cb);

                    error = _theLiveFeedListener.declareQueue(que.Name);
                    if (error != 0)
                    {
                        string errMsg = _theLiveFeedListener.getCodeText(error);
                        string message = string.Format("Live Feed Connection {0} ({1}:{2}) : declare queue {3} failed with error = {4}:{5}",
                            this.ConnectionDefn.Name,
                            this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                            que.Name, error, errMsg.Equals(string.Empty) ? "NULL." : errMsg);
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, message);
                        // give it a 2nd try if we have not yet find out which version we are connecting
                        if (string.IsNullOrEmpty(this.ConnectionDefn.PlatformReleaseVersion))
                        {
                            errMsg = DoConnect(que.Name, "", "");
                        }

                        if (string.IsNullOrEmpty(errMsg))
                        {
                            //errMsg = _theLiveFeedListener.getCodeText(error);
                            //message = string.Format("Live Feed Connection {0} ({1}:{2}) : declare queue {3} failed with error = {4}:{5}",
                            //                this.ConnectionDefn.Name,
                            //                this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                            //                que.Name, error, errMsg.Equals(string.Empty) ? "NULL." : errMsg);
                            throw new Exception(errMsg);
                        }
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.LiveFeedFramework,
                                       TRACE_SUB_AREA_NAME,
                                       String.Format("Live Feed Connection {0} ({1}:{2}) : declare queue {3} successful.",
                                                     this.ConnectionDefn.Name,
                                                     this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                     que.Name));

                    foreach (LiveFeedSubscription sub in que.Subscriptions)
                    {
                        string subscriptionRoutingKey = _theLiveFeedListener.getSubsKey(sub.RoutingKey);
                        if (!string.IsNullOrEmpty(subscriptionRoutingKey))
                        {
                            sub.RealLifeRoutingKey = subscriptionRoutingKey;
                            error = _theLiveFeedListener.bindQueue(que.Name, "amq.topic", subscriptionRoutingKey, "NATIVE", 0, 0);
                            if (error != 0)
                            {
                                string errMsg = _theLiveFeedListener.getCodeText(error);
                                string message = string.Format("Live Feed Connection {0} ({1}:{2}) : bind queue {3} to routing key {4} failed with error = {5}:{6}",
                                                                this.ConnectionDefn.Name,
                                                                this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                                que.Name, 
                                                                subscriptionRoutingKey, error,errMsg.Equals(string.Empty)?"NULL.":errMsg);
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, message);
                                throw new Exception(errMsg);
                            }

                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                String.Format("Live Feed Connection {0} ({1}:{2}) : bind queue {3} to routing key {4} successfully.",
                                                                this.ConnectionDefn.Name,
                                                                this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                                que.Name, 
                                                                subscriptionRoutingKey));
                        }

                        if (sub.Filters != null)
                        {
                            foreach (string filter in sub.Filters)
                            {
                                error = _theLiveFeedListener.filter(filter);   
                                
                                if (error != 0)
                                {
                                    string errMsg = _theLiveFeedListener.getCodeText(error);
                                    string message = string.Format("Live Feed Connection {0} ({1}:{2}) : applying filter [{3}] to queue {4} failed with error = {5}:{6}",
                                                                    this.ConnectionDefn.Name,
                                                                    this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                                    filter, que.Name, error, errMsg.Equals(string.Empty) ? "NULL." : errMsg);
                                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, message);
                                    throw new Exception(errMsg);
                                }

                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                                    String.Format("Live Feed Connection {0} ({1}:{2}) : applied filter [{3}] to queue {4} successfully.",
                                                                    this.ConnectionDefn.Name,
                                                                    this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort, filter, que.Name));
                            }
                        }
                    }

                    error = _theLiveFeedListener.subscribeQueue(que.Name);
                    if (error != 0)
                    {
                        string errMsg = _theLiveFeedListener.getCodeText(error);
                        string message = string.Format("Live Feed Connection {0} ({1}:{2}) : subscribe queue {3} failed with error = {4}:{5}", 
                                                        this.ConnectionDefn.Name,
                                                        this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                        que.Name, error, errMsg.Equals(string.Empty) ? "NULL." : errMsg);
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME, message);
                        throw new Exception(errMsg);
                    }

                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                       String.Format("Live Feed Connection {0} ({1}:{2}) : subscribe queue {3} successful.", 
                                                        this.ConnectionDefn.Name,
                                                        this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                        que.Name));

                    if (error == 0)
                    {
                        // Now, listen and this is a blocking call
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                           String.Format("Live Feed Connection {0} ({1}:{2}) : listening.",
                                                        this.ConnectionDefn.Name,
                                                        this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort));
                        _theLiveFeedListener.listen();
                    }

                    // If there is an error, the thread will complete. 
                }
            }

            catch (Exception ex)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                   String.Format("Live Feed Connection {0} ({1}:{2}) encountered exception : {3}.", 
                                                this.ConnectionDefn.Name,
                                                this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort,
                                                ex.Message));
                Thread.Sleep(500); // somehow, we need a little bit of time delay for error reporting to happen.
                throw ex;
            }
            finally
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.LiveFeedFramework, TRACE_SUB_AREA_NAME,
                                   String.Format("Live Feed Connection {0} ({1}:{2}) Thread Completed.", 
                                                this.ConnectionDefn.Name,
                                                this.ConnectionDefn.LiveFeedHostName, this.ConnectionDefn.LiveFeedPort));

                // Do have to close the connection right here, since the completion will do that anyway. 
            }
        }

        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WorkerThread_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.LiveFeedFramework,
                                   TRACE_SUB_AREA_NAME,
                                   String.Format("Background work completed with error: {0}", e.Error.Message));
                _lastFailedReason = string.Format("Exception Occurred: {0}", e.Error.Message);
            }
            else if (e.Cancelled)
            {
                // do nothing
                _lastFailedReason = "Session cancelled";
            }
            else
            {
                // do nothing
                _lastFailedReason = "Session concluded";
            }

            Close(_lastFailedReason, false);
        }




        /// <summary>
        /// The main loop for the background worker, which does the dispatching of the received packages.
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            WorkerThread worker = sender as WorkerThread;

            bool go_on = true;

            // loop until the thread is shutdown to load the received
            while (go_on)
            {
                try
                {
                    ReceivedPackage package = null;
                    object data = null;
                    int goodCounter = 0;

                    if ((package = (ReceivedPackage)_connectorReceiveQueue.WaitForDequeue()) != null)
                    {
                        // Time to shut down?
                        if (e.Cancel ||
                            package.Publication.Equals(SHUTDOWN_COMMAND))
                        {
                            go_on = false;
                        }
                        else
                        {
                            lock (_backgroundReceivedPackages)
                            {
                                if (_subscriptions.Count > 0)
                                {
                                    _backgroundReceivedPackages.Add(package);
                                    goodCounter++;
                                }
                                go_on = GetAllArrivals(worker, ref goodCounter);
                            }
                        }
                    }

                    // Now, try to export the loaded datatables 
                    // to output queues.
                    if (goodCounter > 0)
                    {
                        worker.ReportProgress(null);
                    }
                }
                catch (System.Threading.ThreadAbortException)
                {
                    // Thread aborted, we'll just exit quietly.
                }
            }
            
            // It's all done
            _connectorReceiveQueue.Clear();
        }

        private bool GetAllArrivals(WorkerThread worker, ref int aGoodCounter)
        {
            ReceivedPackage package = null;
            while ((package = (ReceivedPackage)_connectorReceiveQueue.Dequeue()) != null)
            {
                if (package.Publication.Equals(SHUTDOWN_COMMAND))
                {
                    return false;
                }

                if (_subscriptions.Count > 0)
                {
                    _backgroundReceivedPackages.Add(package);
                    aGoodCounter++;
                }
            }

            return true;
        }

        private void ProcessReceivedPackage(ReceivedPackage package)
        {
            if (package != null)
            {
                if (this.LiveFeedQueues.Count > 0)
                {
                    LiveFeedMessageQueue que = (LiveFeedMessageQueue)this.LiveFeedQueues[0];
                    foreach (LiveFeedSubscription sub in que.Subscriptions)
                    {
                        if (sub.PublicationName.Equals(package.Publication))
                        {
                            sub.ReceiveNewData(package.Data);
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// This event handler deals with the results of the background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.LiveFeedFramework,
                                   TRACE_SUB_AREA_NAME,
                                   String.Format("Background work completed with error: {0}", e.Error.Message));
            }
            else if (e.Cancelled)
            {
                // do nothing
            }
            else
            {
                // do nothing
            }
        }

        /// <summary>
        /// This event handler gets called when the progress of the dataprovider is updated
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            lock (_backgroundReceivedPackages)
            {
                foreach (ReceivedPackage package in _backgroundReceivedPackages)
                {
                    try
                    {
                        ProcessReceivedPackage(package);
                    }
                    catch (Exception ex)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.LiveFeedFramework,
                                           TRACE_SUB_AREA_NAME,
                                           String.Format("Background work progress report with error: {0}", ex.Message));
                    }
                }
                _backgroundReceivedPackages.Clear();
            }
        }
        #endregion Private methods

    }
}
