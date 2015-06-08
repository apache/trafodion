//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using System.Collections;
using System.Collections.Generic;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSWorkloadCanvas : UserControl
    {
        #region Constants
        public const int WORKLOAD_EXEC_TIMEOUT = 600;
        public const int WORKLOAD_EXEC_NO_TIMEOUT = 0;
        public const int WORKLOAD_REFRESH_RATE = 30;

        // Key for searching watched queries
        public const string QUERY_WATCH_LIST_KEY = "{0}@{1}";
        #endregion

        Dictionary<ConnectionDefinition, Hashtable> _watchedQueries = new Dictionary<ConnectionDefinition, Hashtable>();
        Dictionary<ConnectionDefinition, Hashtable> _watchedReorgProgress = new Dictionary<ConnectionDefinition, Hashtable>();

        public WMSWorkloadCanvas()
        {
            InitializeComponent();
        }

        public Connection GetConnection(ConnectionDefinition connectionDefinition)
        {
            Connection connection = null;
            connection = new Connection(connectionDefinition);
            return connection;
        }

        //public WMSQueryInfo GetWatchedQueryWindow(ConnectionDefinition aConnectionDefinition, string aQueryId)
        public WMSQueryDetailUserControl GetWatchedQueryWindow(ConnectionDefinition aConnectionDefinition, string aQueryId, string aStartTime)
        {
            Hashtable queryWindows = null;

            if (_watchedQueries.ContainsKey(aConnectionDefinition))
            {
                if (_watchedQueries.TryGetValue(aConnectionDefinition, out queryWindows))
                {
                    string key = string.Format(QUERY_WATCH_LIST_KEY, aQueryId, aStartTime);
                    if (queryWindows != null && queryWindows.ContainsKey(key))
                    {
                        WMSQueryDetailUserControl queryInfo = (WMSQueryDetailUserControl)queryWindows[key];
                        return queryInfo;
                    }
                }
            }
            return null;
        }

        public ReorgProgressUserControl GetWatchedReorgProgressWindow(ConnectionDefinition aConnectionDefinition, string aQueryId, string aStartTime)
        {
            Hashtable reorgProgressWindows = null;

            if (_watchedReorgProgress.ContainsKey(aConnectionDefinition))
            {
                if (_watchedReorgProgress.TryGetValue(aConnectionDefinition, out reorgProgressWindows))
                {
                    if (reorgProgressWindows != null && reorgProgressWindows.ContainsKey(aQueryId))
                    {
                        ReorgProgressUserControl reorgProgressWindow = (ReorgProgressUserControl)reorgProgressWindows[aQueryId];
                        return reorgProgressWindow;
                    }
                }
            }
            return null;
        }
        
        public string[] GetWatchedQueryList(ConnectionDefinition aConnectionDefinition)
        {
            Hashtable queryWindows = null;
            _watchedQueries.TryGetValue(aConnectionDefinition, out queryWindows);
            if (queryWindows != null)
            {
                string[] queryIds = new string[queryWindows.Keys.Count];
                queryWindows.Keys.CopyTo(queryIds, 0);
                return queryIds;
            }
            return new string[0];
        }

        public string[] GetWatchedReorgQueryList(ConnectionDefinition aConnectionDefinition)
        {
            Hashtable reorgProgressWindows = null;
            _watchedReorgProgress.TryGetValue(aConnectionDefinition, out reorgProgressWindows);
            if (reorgProgressWindows != null)
            {
                string[] queryIds = new string[reorgProgressWindows.Keys.Count];
                reorgProgressWindows.Keys.CopyTo(queryIds, 0);
                return queryIds;
            }
            return new string[0];
        }

        //public void AddQueryToWatch(WMSQueryInfo queryInfo)
        public void AddQueryToWatch(WMSQueryDetailUserControl queryInfo)
        {
            Hashtable queryWindows = null;
            _watchedQueries.TryGetValue(queryInfo.ConnectionDefinition, out queryWindows);
            if (queryWindows == null)
            {
                queryWindows = new Hashtable();
                _watchedQueries.Add(queryInfo.ConnectionDefinition, queryWindows);
            }
            string sKey=string.Format(QUERY_WATCH_LIST_KEY, queryInfo.QueryId, queryInfo.QueryStartTime);
            if (queryWindows.ContainsKey(sKey))
            {
                queryWindows.Remove(sKey);
            }
            queryWindows.Add(sKey, queryInfo);
        }

        public void RemoveQueryFromWatch(WMSQueryDetailUserControl queryInfo)
        {
            Hashtable queryWindows = null;
            _watchedQueries.TryGetValue(queryInfo.ConnectionDefinition, out queryWindows);
            if (queryWindows != null && queryWindows.ContainsKey(string.Format(QUERY_WATCH_LIST_KEY, queryInfo.QueryId, queryInfo.QueryStartTime)))
            {
                queryWindows.Remove(string.Format(QUERY_WATCH_LIST_KEY, queryInfo.QueryId, queryInfo.QueryStartTime));
            }
        }

        public ReorgProgressUserControl GetWatchedReorgWindow(ConnectionDefinition aConnectionDefinition, string aQueryId)
        {
            Hashtable reorgWindows = null;

            if (_watchedReorgProgress.ContainsKey(aConnectionDefinition))
            {
                if (_watchedReorgProgress.TryGetValue(aConnectionDefinition, out reorgWindows))
                {
                    if (reorgWindows != null && reorgWindows.ContainsKey(aQueryId))
                    {
                        ReorgProgressUserControl reorg = (ReorgProgressUserControl)reorgWindows[aQueryId];
                        return reorg;
                    }
                }
            }
            return null;
        }

        public void AddQueryToWatchReorg(ReorgProgressUserControl reorg)
        {
            Hashtable reorgWindows = null;
            _watchedReorgProgress.TryGetValue(reorg.ConnectionDefinition, out reorgWindows);
            if (reorgWindows == null)
            {
                reorgWindows = new Hashtable();
                _watchedReorgProgress.Add(reorg.ConnectionDefinition, reorgWindows);
            }
            string sKey = reorg.QueryId;
            if (reorgWindows.ContainsKey(sKey))
            {
                reorgWindows.Remove(sKey);
            }
            reorgWindows.Add(sKey, reorg);
        }

        public void RemoveQueryFromWatchReorg(ReorgProgressUserControl reorg)
        {
            Hashtable reorgWindows = null;
            _watchedReorgProgress.TryGetValue(reorg.ConnectionDefinition, out reorgWindows);
            if (reorgWindows != null && reorgWindows.ContainsKey(reorg.QueryId))
            {
                reorgWindows.Remove(reorg.QueryId);
            }
        }
    }
}
