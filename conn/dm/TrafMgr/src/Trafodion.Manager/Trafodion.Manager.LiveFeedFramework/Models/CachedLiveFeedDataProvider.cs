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
using System.Data;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// A specialized LiveFeed data provider which cached a prescribed size
    /// </summary>
    public class CachedLiveFeedDataProvider : LiveFeedDataProvider
    {
        #region Fields

        protected DataTable _DataStore = null;
        protected FilterManager _FilterManager = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: FilterManager - the filter manager
        /// </summary>
        public FilterManager FilterManager
        {
            get { return _FilterManager; }
            set { _FilterManager = value; }
        }

        /// <summary>
        /// Property: CacheSize - the max data size in terms of row count
        /// </summary>
        public int CacheSize
        {
            get { return ((CachedLiveFeedDataProviderConfig)this.DataProviderConfig).CacheSize; }
        }

        /// <summary>
        /// Porperty: KeepCacheOnStop - whether to preserve the cache on stop
        /// </summary>
        public bool KeepCacheOnStop
        {
            get { return ((CachedLiveFeedDataProviderConfig)this.DataProviderConfig).KeepCacheOnStop; }
        }

        #endregion Properties

        #region Constructor 

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aDataProviderConfig"></param>
        public CachedLiveFeedDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Get the filtered data from the data store
        /// </summary>
        /// <returns></returns>
        public DataTable GetFilteredDataFromStore()
        {
            DataTable aDataTable = null;
            lock (this)
            {
                if (_theDataTable != null)
                {
                    aDataTable = GetDataTableCopy(_theDataTable);
                    foreach (DataRow dr in _theDataTable.Rows)
                    {
                        if ((null == _FilterManager) || _FilterManager.MatchesFilterCriteria(dr))
                        {
                            // Add a row if there is no filter manager configured or if it matches filter criteria.
                            aDataTable.Rows.InsertAt(GetRowCopyForTable(dr, aDataTable), 0);
                        }
                    }
                }
                return aDataTable;
            }
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

                lock (this)
                {
                    if (_theDataTable == null)
                    {
                        _theDataTable = GetDataTableCopy(dataTable);
                    }

                    for (int i = dataTable.Rows.Count - 1; i >=0; i--)
                    {
                        DataRow dr = dataTable.Rows[i];
                        if ((null == _FilterManager) || _FilterManager.MatchesFilterCriteria(dr))
                        {
                            // Add a row if there is no filter manager configured or if it matches filter criteria.
                            DataRow row1 = GetRowCopyForTable(dr, _theDataTable);

                            //Add the row to the store
                            addRowToStore(row1, _theDataTable);
                        }
                    }

                    FireNewDataArrived(GetDataProviderEventArgs());
                }
            }
        }

        /// <summary>
        /// The stop method
        /// </summary>
        public override void Stop()
        {
            // Remember to dispose what's in the cache. 
            if (_theDataTable != null && !KeepCacheOnStop)
            {
                _theDataTable.Dispose();
                _theDataTable = null;
            }

            base.Stop();
        }

        /// <summary>
        /// To resize the current cache and simulate a new data arrival so that the data can be re-populated 
        /// with the new limit.  
        /// Note: this works only with Cached dataprovider, which data table has all of the data
        /// cached.
        /// </summary>
        public void ReSizeCache()
        {

            if (_theDataTable != null && _theDataTable.Rows.Count > CacheSize)
            {
                DeleteRowsIfNeeded(_theDataTable);
                FireNewDataArrived(GetDataProviderEventArgs());
            }
        }

        /// <summary>
        /// To cleanup the cache
        /// </summary>
        public void ClearCache()
        {
            if (_theDataTable != null)
            {
                _theDataTable.Dispose();
                _theDataTable = null;
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Get a copy of the data table schema
        /// </summary>
        /// <param name="dataTable"></param>
        /// <returns></returns>
        protected DataTable GetDataTableCopy(DataTable dataTable)
        {
            DataTable ret = new DataTable();
            foreach (DataColumn dc in dataTable.Columns)
            {
                ret.Columns.Add(dc.ColumnName, dc.DataType);
            }
            return ret;
        }

        /// <summary>
        /// Copy a data row from the data table
        /// </summary>
        /// <param name="aRowToCopy"></param>
        /// <param name="dataTable"></param>
        /// <returns></returns>
        protected DataRow GetRowCopyForTable(DataRow aRowToCopy, DataTable dataTable)
        {
            DataRow row = dataTable.NewRow();
            foreach (DataColumn dc in dataTable.Columns)
            {
                row[dc.ColumnName] = aRowToCopy[dc.ColumnName];
            }
            return row;
        }

        /// <summary>
        /// Add row to the data store
        /// </summary>
        /// <param name="aRow"></param>
        /// <param name="aStore"></param>
        protected void addRowToStore(DataRow aRow, DataTable aStore)
        {
                DeleteRowsIfNeeded(aStore);
                aStore.Rows.InsertAt(aRow, 0);
        }

        /// <summary>
        /// Delete a row from the data store 
        /// </summary>
        /// <param name="aDataStore"></param>
        protected void DeleteRowsIfNeeded(System.Data.DataTable aDataStore)
        {
            int currentRowCount = aDataStore.Rows.Count;
            int rowsToBeAdded = 1;
            if ((currentRowCount + rowsToBeAdded) > CacheSize)
            {
                int deleteCount = (currentRowCount + rowsToBeAdded) - CacheSize;
                for ( int i = 1; i <= deleteCount; i++)
                {
                    aDataStore.Rows.RemoveAt(currentRowCount-i);
                }
            }
        }

        #endregion Private methods
    }
}
