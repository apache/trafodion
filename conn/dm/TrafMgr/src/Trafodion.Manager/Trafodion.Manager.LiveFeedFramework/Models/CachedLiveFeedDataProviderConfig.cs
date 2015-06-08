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
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// Specialized cached LiveFeed data provider config.
    /// </summary>
    [Serializable]
    public class CachedLiveFeedDataProviderConfig : LiveFeedDataProviderConfig
    {
        #region Fields

        private int _CacheSize = 1000;
        private bool _KeepCacheOnStop = false;

        #endregion Fields

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (TheDataProvider == null)
            {
                if (TheDataFormat == LiveFeedDataFormat.DataTable)
                {
                    TheDataProvider = new CachedLiveFeedDataProvider(this);
                }
                else
                {
                    TheDataProvider = new CachedLiveFeedProtoBufDataProvider(this);
                }
            }
            return TheDataProvider;
        }

        /// <summary>
        /// Property: CacheSize - the cache limit for this data provider
        /// </summary>
        public int CacheSize
        {
            get { return _CacheSize; }
            set { _CacheSize = value; }
        }

        /// <summary>
        /// Property: KeepCacheOnStop - the cache will be preserved at stop
        /// </summary>
        public bool KeepCacheOnStop
        {
            get { return _KeepCacheOnStop; }
            set { _KeepCacheOnStop = value; }
        }
    }
}
