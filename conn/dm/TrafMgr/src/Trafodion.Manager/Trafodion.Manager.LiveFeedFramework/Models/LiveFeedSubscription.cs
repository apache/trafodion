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
using System.Data;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// This represents a LiveFeed Subscription. The incoming messages are queued in the LiveFeed subscription.  
    /// </summary>
    public class LiveFeedSubscription
    {
        #region Fields

        // Public members
        public delegate void NewDataArrived(object sender, EventArgs e);
        public event NewDataArrived OnNewDataArrived;

        //Private members
        private LiveFeedConnection _theLiveFeedConnection = null;
        private string _thePublicationName = null;
        private ReceiveQueue _receiveQueue = null;
        private DataTable _dataTable = null;
        private string[] _theFilters = null;
        private string _theRealLiveRoutingKey = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// The LiveFeedConnection where this subscription belongs to.  A LiveFeedSubscription belongs to 
        /// only one LiveFeedConnection.
        /// </summary>
        public LiveFeedConnection LiveFeedConnection
        {
            get { return _theLiveFeedConnection; }
            set { _theLiveFeedConnection = value; }
        }

        /// <summary>
        /// The Publication key of this subscription. Publication Key is known internally to the TrafodionManager.
        /// </summary>
        public string PublicationName
        {
            get { return _thePublicationName; }
            set { _thePublicationName = value; }
        }

        /// <summary>
        /// The number of elements currently in the queue of this subscription.
        /// </summary>
        public int QueueLength
        {
            get { return _receiveQueue.Length; }
        }

        /// <summary>
        /// The Routing Key of this subscription.  Routing Key is known to the LiveFeed Broker. 
        /// </summary>
        public string RoutingKey
        {
            get { return LiveFeedRoutingKeyMapper.GetRoutingKey(PublicationName); }
        }

        /// <summary>
        /// Property : RealLifeRoutingKey - the routing key got back from the call to _theLiveFeedListener.getSubsKey()
        /// [NOTE] In M6 implementation, this is the same as RoutingKey.
        /// </summary>
        public string RealLifeRoutingKey
        {
            get { return (_theRealLiveRoutingKey == null) ? RoutingKey : _theRealLiveRoutingKey; }
            set { _theRealLiveRoutingKey = value; }
        }

        /// <summary>
        /// The Receive Queue of this subscription.
        /// </summary>
        public ReceiveQueue ReceiveQueue
        {
            get { return _receiveQueue; }
        }

        /// <summary>
        /// Property: Filter of applied to the subscription
        /// </summary>
        public string[] Filters
        {
            get { return _theFilters; }
            set { _theFilters = value; }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aLiveFeedConnection">The LiveFeed connection to be used</param>
        public LiveFeedSubscription(LiveFeedConnection aLiveFeedConnection, string aPublicationName)
        {
            _theLiveFeedConnection = aLiveFeedConnection;
            _thePublicationName = aPublicationName;
            _receiveQueue = new ReceiveQueue(_thePublicationName);
        }

        /// <summary>
        /// The destructor
        /// </summary>
        ~LiveFeedSubscription()
        {
            _receiveQueue.Clear();
            _receiveQueue = null;
        }

        #endregion Constructor

        #region Public Methods

        /// <summary>
        /// To cleanup this subscription.
        /// </summary>
        public void Cleanup()
        {
            LiveFeedConnection.UnSubscribe(this, true);
            ReceiveQueue.Clear();
            _dataTable = null;
        }
      
        /// <summary>
        /// To receive a new packet
        /// </summary>
        /// <param name="aStats"></param>
        public void ReceiveNewData(Object aStats)
        {
            _receiveQueue.Enqueue(aStats);
            FireNewDataArrived(new EventArgs());
        }

        /// <summary>
        /// The get data table out of the subscription. The data table will contains all of the received messages and transform them into
        /// rows in the data table. 
        /// </summary>
        /// <returns></returns>
        public DataTable GetDataTableStats()
        {
            Object stats = null;
            while ((stats = this.GetOneNewStats()) != null)
            {
                if (_dataTable == null)
                {
                    _dataTable = LiveFeedStatsTransformer.TransformStatsToDataTable(this.PublicationName, stats);
                }
                else
                {
                    this._dataTable.Rows.Clear();
                    LiveFeedStatsTransformer.TransformStatsToDataTable(this.PublicationName, stats, this._dataTable);
                }
            }
            return this._dataTable;
        }

        /// <summary>
        /// Get all of the newly arrived stats. 
        /// </summary>
        /// <returns></returns>
        public List<object> GetAllNewStats()
        {
            List<object> listStats = new List<object>();
            Object stats = null;
            while ((stats = _receiveQueue.Dequeue()) != null)
            {
                listStats.Add(stats);
            }
            return listStats;
        }

        /// <summary>
        /// Get one newly arrived stats.
        /// </summary>
        /// <returns></returns>
        public Object GetOneNewStats()
        {
            return _receiveQueue.Dequeue();
        }

        #endregion Public Methods

        #region Private methods

        /// <summary>
        /// To fire up data arrival event.
        /// </summary>
        /// <param name="e"></param>
        private void FireNewDataArrived(EventArgs e)
        {
            if (OnNewDataArrived != null)
            {
                OnNewDataArrived(this, e);
            }
        }

        #endregion Private methods
    }
}
