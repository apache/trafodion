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

using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.ConnectivityArea.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.ConnectivityArea.Model;
using System;

namespace Trafodion.Manager.ConnectivityArea
{
    /// <summary>
    /// Workload Area
    /// </summary>
    public class ConnectivityAreaMain : ITrafodionArea
    {
        #region Fields

        private ConnectivityNavigationControl _connectivityNavigatorControl = null;
        private ConnectivityRightPaneControl _connectivityRightPaneControl = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Returns the area name for the framework button and active area label
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
                return _connectivityNavigatorControl;
            }
        }

        /// <summary>
        /// Property that the framework reads to get the area's right pane control
        /// </summary>
        public Control RightPane
        {
            get
            {
                return _connectivityRightPaneControl;
            }
        }
        /// <summary>
        /// Image displayed in the Area button
        /// </summary>
        public Image Image
        {
            get { return global::Trafodion.Manager.Properties.Resources.ConnectivityIcon; }
        }

        #endregion Properties

        /// <summary>
        /// Creates a new instance of workload area
        /// </summary>
        public ConnectivityAreaMain()
        {
            _connectivityNavigatorControl = new ConnectivityNavigationControl();
            _connectivityRightPaneControl = new ConnectivityRightPaneControl(_connectivityNavigatorControl);

        }

        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            _connectivityNavigatorControl.ConnectivityTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the ConnectivityTreeView always. We don't want the focus on the Favorites tree.
            _connectivityNavigatorControl.ConnectivityTreeView.Select();

        }

    }
}
