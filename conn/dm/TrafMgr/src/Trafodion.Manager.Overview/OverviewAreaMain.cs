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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.OverviewArea.Controls;

namespace Trafodion.Manager.OverviewArea
{
    /// <summary>
    /// The area associated with the OverviewArea, which is used to display general information about the system.
    /// </summary>
    public class OverviewAreaMain : ITrafodionArea
    {
        #region Fields

        //private SystemsNavigator _navigator = new SystemsNavigator();
        //private OverviewPanel _rightPanel = new OverviewPanel();

        private OverviewNavigationControl _overviewNavigatorControl = null;
        private OverviewRightPaneControl _overviewRightPaneControl = null;

        #endregion

        #region Properties

        /// <summary>
        /// Property that the framework reads to find the area's name for its button and the active area label
        /// </summary>
        public string AreaName
        {
            get
            {
                return Properties.Resources.AreaName;
            }
        }


        /// <summary>
        /// Property that the framework reads to get the area's navigator control
        /// </summary>
        public Control Navigator
        {
            get
            {
                return _overviewNavigatorControl;
            }
        }

        /// <summary>
        /// Property that the framework reads to get the area's right pane control
        /// </summary>
        public Control RightPane
        {
            get
            {
                return _overviewRightPaneControl;
            }
        }

        /// <summary>
        /// Image displayed in the Area button
        /// </summary>
        public System.Drawing.Image Image
        {
            get { return global::Trafodion.Manager.Properties.Resources.MonitoringIcon; }
        }

        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            //_navigator.NavigationTree.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
            _overviewNavigatorControl.OverviewTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
            //Force the selection on the ConnectivityTreeView always. We don't want the focus on the Favorites tree.
            _overviewNavigatorControl.OverviewTreeView.Select();
        }

        #endregion

        /// <summary>
        /// Creates a new instance.
        /// </summary>
        public OverviewAreaMain()
        {
            _overviewNavigatorControl = new OverviewNavigationControl();
            _overviewRightPaneControl = new OverviewRightPaneControl(_overviewNavigatorControl);
        }

    }
}
