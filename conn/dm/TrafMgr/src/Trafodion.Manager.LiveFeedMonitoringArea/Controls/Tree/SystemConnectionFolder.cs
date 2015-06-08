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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.LiveFeedMonitoringArea.Models;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls.Tree
{
    class SystemConnectionFolder : NavigationTreeConnectionFolder
    {
        private LiveFeedSystem _system;

        public LiveFeedSystem LiveFeedSystem
        {
            get { return _system; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition that that the folder will represent</param>
        public SystemConnectionFolder(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                _system = LiveFeedSystem.FindSystemModel(aConnectionDefinition);
            }
        }

        void _system_LookupCompletedEvent(object sender, System.EventArgs e)
        {
            Populate(null);
        }

        /// <summary>
        /// Called to always repopulate our children
        /// </summary>
        /// <param name="aNavigationTreeNameFilter">the name filter to be used</param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            // And populate
            Populate(aNavigationTreeNameFilter);
        }

        public override string LongerDescription
        {
            get
            {
                return TheConnectionDefinition.Name;
            }
        }

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();

            //if (TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                Nodes.Add(new MonitorConnectorsFolder());
                Nodes.Add(new TestingFolder());
            }
        }
    }
}
