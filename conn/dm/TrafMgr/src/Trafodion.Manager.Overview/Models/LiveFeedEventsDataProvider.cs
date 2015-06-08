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
using System.Data;
using Trafodion.Manager.Framework;
using Trafodion.Manager.LiveFeedFramework.Models;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Specialized Cached LiveFeed Data provider for Live Events
    /// </summary>
    public class LiveFeedEventsDataProvider : CachedLiveFeedDataProvider
    {
        #region Fields

        private EventFilterModel _theEventFilterModel;
        private int _theNewlyArrivedEventCount = 0;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: EventFilterModel - event filter
        /// </summary>
        public EventFilterModel EventFilterModel
        {
            get { return _theEventFilterModel; }
            set
            {
                _theEventFilterModel = value;
                Persistence.Put(EventFilterModel.LiveEventFilterPersistenceKey, _theEventFilterModel);
            }
        }

        /// <summary>
        /// Property: NewlyArrivedEventCount - the new events in the data provider; since this is a cached data provider, the new 
        /// events are the first n number of rows.
        /// </summary>
        public int NewlyArrivedEventCount
        {
            get { return _theNewlyArrivedEventCount; }
            set { _theNewlyArrivedEventCount = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDataProviderConfig"></param>
        public LiveFeedEventsDataProvider(CachedLiveFeedDataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
            aDataProviderConfig.KeepCacheOnStop = true;
        }

        #endregion Constructors

        /// <summary>
        /// Overrides the default DoPrefetchSetup and sets up the query based on the number of rows requested and the 
        /// filter criteria.
        /// </summary>
        /// <param name="predefinedParametersHash"></param>
        public override void DoPrefetchSetup(Hashtable predefinedParametersHash)
        {
            LiveFeedEventDataProviderConfig dbConfig = DataProviderConfig as LiveFeedEventDataProviderConfig;
            string[] filters = GetLiveFeedFilters(_theEventFilterModel);

            if (dbConfig.SubscriptionConfiguration.ContainsKey(LiveFeedRoutingKeyMapper.PUBS_common_text_event))
            {
                dbConfig.SubscriptionConfiguration[LiveFeedRoutingKeyMapper.PUBS_common_text_event] = filters;
            }

            base.DoPrefetchSetup(predefinedParametersHash);
        }

        /// <summary>
        /// Override event handler for new data arrived event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public override void LiveFeedSubscription_OnNewDataArrived(object sender, EventArgs e)
        {
            LiveFeedSubscription sub = (LiveFeedSubscription)sender;
            DataTable dataTable = sub.GetDataTableStats();
            if (dataTable != null)
            {
                LastReceivedPublication = sub.PublicationName;
                bool rowAdded = false;

                lock (this)
                {
                    if (_theDataTable == null)
                    {
                        _theDataTable = GetDataTableCopy(dataTable);
                    }

                    string filter = _theEventFilterModel.GetFilterLiveFeedSQL();

                    if (string.IsNullOrEmpty(filter))
                    {
                        for (int i = dataTable.Rows.Count - 1; i >= 0; i--)
                        {
                            DataRow dr = dataTable.Rows[i];
                            if ((null == _FilterManager) || _FilterManager.MatchesFilterCriteria(dr))
                            {
                                // Add a row if there is no filter manager configured or if it matches filter criteria.
                                DataRow row1 = GetRowCopyForTable(dr, _theDataTable);

                                //Add the row to the store
                                addRowToStore(row1, _theDataTable);
                                rowAdded = true;
                            }
                        }

                        NewlyArrivedEventCount += dataTable.Rows.Count;
                    }
                    else
                    {
                        dataTable.CaseSensitive = _theEventFilterModel.CaseSensitive;
                        DataRow[] rows = dataTable.Select(filter);
                        foreach (DataRow row in rows)
                        {
                            // Add a row if there is no filter manager configured or if it matches filter criteria.
                            DataRow row1 = GetRowCopyForTable(row, _theDataTable);

                            //Add the row to the store
                            addRowToStore(row1, _theDataTable);
                            rowAdded = true;
                        }

                        NewlyArrivedEventCount += rows.Length;
                    }

                    if (rowAdded)
                    {
                        FireNewDataArrived(GetDataProviderEventArgs());
                    }
                }
            }
        }

        /// <summary>
        /// Get the live feed filters
        /// </summary>
        /// <param name="aEventFilterModel"></param>
        /// <returns></returns>
        private string[] GetLiveFeedFilters(EventFilterModel aEventFilterModel)
        {
            string[] ret = null;
            ret = aEventFilterModel.GetFilterLiveFeed();
            return ret;

        }
    }

    [Serializable]
    public class LiveFeedEventDataProviderConfig : CachedLiveFeedDataProviderConfig
    {
        // Special data provider configuration for live event
        //[XmlIgnore]
        [NonSerialized]
        new protected LiveFeedEventsDataProvider _theDataProvider = null;
        //[XmlIgnore]
        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.MDD; }
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (_theDataProvider == null)
            {
                _theDataProvider = new LiveFeedEventsDataProvider(this);
            }
            return _theDataProvider;
        }
    }
}
