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
using System.Collections.Generic;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Models
{
    public class LiveFeedSystem : LiveFeedObject
    {
        #region Fields

        private static Dictionary<ConnectionDefinition, LiveFeedSystem> _activeSystem = new Dictionary<ConnectionDefinition, LiveFeedSystem>(new MyConnectionDefinitionComparer());

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        private LiveFeedSystem(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            //Add this instance to the static dictionary, so in future if need a reference to this system, 
            //we can look up using the connection definition
            if (!_activeSystem.ContainsKey(aConnectionDefinition))
                _activeSystem.Add(aConnectionDefinition, this);

            //Subscribe to connection definition's events, so that you can maintain the static dictionary
            ConnectionDefinition.Changed += new ConnectionDefinition.ChangedHandler(ConnectionDefinition_Changed);

        }
        ~LiveFeedSystem()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }
        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Find the LiveFeed system model object
        /// </summary>
        /// <param name="connectionDefinition"></param>
        /// <returns></returns>
        public static LiveFeedSystem FindSystemModel(ConnectionDefinition connectionDefinition)
        {
            LiveFeedSystem LiveFeedSystem = null;
            _activeSystem.TryGetValue(connectionDefinition, out LiveFeedSystem);
            if (LiveFeedSystem == null)
            {
                LiveFeedSystem = new LiveFeedSystem(connectionDefinition);
            }
            return LiveFeedSystem;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// If the connection definition has changed/removed, the static dictionary is updated accordingly
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        private void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason != ConnectionDefinition.Reason.Tested)
            {
                _activeSystem.Remove(aConnectionDefinition);
            }
        }

        #endregion Private methods
    }
}


