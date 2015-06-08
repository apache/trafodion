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

using System.Collections.Generic;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.LiveFeedFramework.Models
{
    /// <summary>
    /// Singleton class for live feed connection registry, where all of the live feed connections are managed.
    /// </summary>
    public class LiveFeedConnectionRegistry
    {
        #region Fields

        // Create a singleton 
        private static LiveFeedConnectionRegistry _theInstance = new LiveFeedConnectionRegistry();

        // keep all of the active connections here
        private static Dictionary<string, List<LiveFeedConnection>> _theActiveLiveFeedConnections = new Dictionary<string, List<LiveFeedConnection>>();

        #endregion Fields

        #region Property

        /// <summary>
        /// Get the singleton instance
        /// </summary>
        public static LiveFeedConnectionRegistry Instance
        {
            get { return _theInstance; }
        }

        #endregion Property

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        private LiveFeedConnectionRegistry()
        {
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
            ConnectionDefinition.OnLiveFeedTest += ConnectionDefinition_OnLiveFeedTest;
        }

        ~LiveFeedConnectionRegistry()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
            ConnectionDefinition.OnLiveFeedTest -= ConnectionDefinition_OnLiveFeedTest;
        }
        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Static method to return a list of active LiveFeed connections with connectition's name
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        public List<LiveFeedConnection> GetActiveLiveFeedConnections(ConnectionDefinition aConnectionDefinition)
        {
            List<LiveFeedConnection> connections = null;
            _theActiveLiveFeedConnections.TryGetValue(aConnectionDefinition.Name, out connections);
            return connections;
        }

        /// <summary>
        /// To get one live feed connection 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <returns></returns>
        public LiveFeedConnection GetOneLiveFeedConnection(ConnectionDefinition aConnectionDefinition)
        {
            LiveFeedConnection lvConnection = new LiveFeedConnection(aConnectionDefinition);
            return lvConnection;
        }

        /// <summary>
        /// To add a LiveFeed connection to the active list.
        /// </summary>
        /// <param name="aLiveFeedConneciton"></param>
        public void AddToActiveRegistry(LiveFeedConnection aLiveFeedConneciton)
        {
            List<LiveFeedConnection> connections = null;
            lock (_theActiveLiveFeedConnections)
            {
                _theActiveLiveFeedConnections.TryGetValue(aLiveFeedConneciton.ConnectionDefn.Name, out connections);
                if (connections == null)
                {
                    connections = new List<LiveFeedConnection>();
                    _theActiveLiveFeedConnections.Add(aLiveFeedConneciton.ConnectionDefn.Name, connections);
                }

                connections.Add(aLiveFeedConneciton);
            }
        }

        /// <summary>
        /// To remove a LiveFeed connection from the active list. 
        /// </summary>
        /// <param name="aLiveFeedConnection"></param>
        public void RemoveFromActiveRegistry(LiveFeedConnection aLiveFeedConnection)
        {
            List<LiveFeedConnection> connections = null;
            lock (_theActiveLiveFeedConnections)
            {
                _theActiveLiveFeedConnections.TryGetValue(aLiveFeedConnection.ConnectionDefn.Name, out connections);
                if (connections != null)
                {
                    connections.Remove(aLiveFeedConnection);
                    if (connections.Count == 0)
                    {
                        _theActiveLiveFeedConnections.Remove(aLiveFeedConnection.ConnectionDefn.Name);
                    }
                }
            }
        }

        /// <summary>
        /// Do nothing, just activate the singleton
        /// </summary>
        public void Activate()
        {
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        private void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason == ConnectionDefinition.Reason.Removed ||
                aReason == ConnectionDefinition.Reason.Disconnected)
            {
                List<LiveFeedConnection> connectionList = this.GetActiveLiveFeedConnections(aConnectionDefinition);
                if (connectionList != null && connectionList.Count > 0)
                {
                    LiveFeedConnection[] connections = connectionList.ToArray();
                    foreach (LiveFeedConnection conn in connections)
                    {
                        conn.Close("Connection is being removed", true);
                    }
                }
            }
            else if (aReason == ConnectionDefinition.Reason.Host)
            {
                List<LiveFeedConnection> connectionList = this.GetActiveLiveFeedConnections(aConnectionDefinition);
                if (connectionList != null && connectionList.Count > 0)
                {
                    LiveFeedConnection[] connections = connectionList.ToArray();
                    foreach (LiveFeedConnection conn in connections)
                    {
                        conn.Close("Connection host is changed", true);
                    }
                }
            }
            else if (aReason == ConnectionDefinition.Reason.LiveFeedPort)
            {
                List<LiveFeedConnection> connectionList = this.GetActiveLiveFeedConnections(aConnectionDefinition);
                if (connectionList != null && connectionList.Count > 0)
                {
                    LiveFeedConnection[] connections = connectionList.ToArray();
                    foreach (LiveFeedConnection conn in connections)
                    {
                        conn.Close("Connection port is changed", true);
                    }
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        private void ConnectionDefinition_OnLiveFeedTest(object aSender, ConnectionDefinition aConnectionDefinition)
        {
            LiveFeedConnection livefeedConnection = new LiveFeedConnection(aConnectionDefinition);
            string errorMsg = livefeedConnection.DoTest();
            aConnectionDefinition.CompleteLiveFeedTest(errorMsg);
        }

        #endregion  Private methods
    }
}
