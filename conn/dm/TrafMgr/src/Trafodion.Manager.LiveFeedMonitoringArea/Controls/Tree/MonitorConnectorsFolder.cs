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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls.Tree
{
    class MonitorConnectorsFolder : NavigationTreeFolder
    {
        #region Fields

        #endregion Fields

        #region Public Properties

        public override string LongerDescription
        {
            get { return "Monitor Connectors"; }
        }

        public override string ShortDescription
        {
            get { return "Monitor Connectors"; }
        }

        #endregion Public Properties

        public MonitorConnectorsFolder()
        {
            Text = LongerDescription;
            Nodes.Clear();
        }

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }
    }
}

