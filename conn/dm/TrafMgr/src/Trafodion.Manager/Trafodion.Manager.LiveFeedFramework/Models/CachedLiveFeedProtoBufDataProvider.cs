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
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// The class of the specialized LiveFeed data provider, which would allow more than one subscriptions at the same time
    /// and keeps all of the data in cache in ProtoBuf format. 
    /// </summary>
    public class CachedLiveFeedProtoBufDataProvider : LiveFeedDataProvider
    {
        #region Fields

        private Dictionary<string, List<object>> _theStats = new Dictionary<string, List<object>>();

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: DataStats - all of the arrived Stats to this data provider
        /// Note: it is the consumer's responsibility to remove the used element from the list.
        /// </summary>
        public Dictionary<string, List<object>> DataStats
        {
            get { return _theStats; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor of this data provider
        /// </summary>
        /// <param name="aDataProviderConfig"></param>
        public CachedLiveFeedProtoBufDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        #endregion Constructors

        /// <summary>
        /// Event handler for the LiveFeedSubscription data arrival event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public override void LiveFeedSubscription_OnNewDataArrived(object sender, EventArgs e)
        {
            LiveFeedSubscription sub = (LiveFeedSubscription)sender;
            LastReceivedPublication = sub.PublicationName;
            List<object> stats = null;

            if (_theStats.ContainsKey(LastReceivedPublication))
            {
                stats = _theStats[LastReceivedPublication] as List<object>;
            }

            if (stats == null)
            {
                stats = new List<object>();
                _theStats.Add(LastReceivedPublication, stats);
            }

            stats.AddRange(sub.GetAllNewStats());
            if (stats.Count > ((CachedLiveFeedDataProviderConfig)this.DataProviderConfig).CacheSize)
            {
                int minus = stats.Count - ((CachedLiveFeedDataProviderConfig)this.DataProviderConfig).CacheSize;
                stats.RemoveRange(0, minus);
            }

            DataProviderEventArgs evtArgs = GetDataProviderEventArgs(LastReceivedPublication);
            FireNewDataArrived(evtArgs);
        }

        /// <summary>
        /// Get all stats data from a single publication
        /// </summary>
        /// <param name="aPublication"></param>
        /// <returns></returns>
        public List<object> GetStats(string aPublication)
        {
            List<object> stats = null;
            _theStats.TryGetValue(aPublication, out stats);
            return stats;
        }

        /// <summary>
        /// Get all the received data from all publications
        /// </summary>
        /// <returns></returns>
        public Dictionary<string, List<object>> GetStats()
        {
            return _theStats;
        }

        /// <summary>
        /// Override the Stop method to also clear out the cache.
        /// </summary>
        public override void Stop()
        {
            if (!((CachedLiveFeedDataProviderConfig)this.DataProviderConfig).KeepCacheOnStop)
            {
                _theStats.Clear();
            }
            base.Stop();
        }

        /// <summary>
        /// To clear all cache
        /// </summary>
        public void ClearCache()
        {
            _theStats.Clear();
        }
    }
}
