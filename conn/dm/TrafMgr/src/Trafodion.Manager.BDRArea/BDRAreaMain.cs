// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;
using Trafodion.Manager;
using Trafodion.Manager.BDRArea.Controls;

namespace Trafodion.Manager.BDRArea
{
    public class BDRAreaMain : ITrafodionArea
    {
        #region Fields
        private BDRAreaNavigator _navigatorControl = new BDRAreaNavigator();
        private MyRightPaneControl _rightPaneControl = new MyRightPaneControl();
        #endregion Fields

        #region Properties
        /// <summary>
        /// Property that the framework reads to find the area's
        /// name for its button and the active area label
        /// </summary>
        public string AreaName
        {
            get { return Properties.Resources.AreaName; }
        }

        /// <summary>
        /// Property that the framework reads to find the area's
        /// Image for its button and the active area label
        /// </summary>
        public Image Image
        {
            get { return Properties.Resources.MyIcon; }
        }
        #endregion Properties

        /// <summary>
        /// Property that the framework reads to get the area's
        /// navigator control
        /// </summary>
        public Control Navigator
        {
            get { return _navigatorControl; }
        }

        /// <summary>
        /// Property that the framework reads to get the area's right
        /// pane control
        /// </summary>
        public Control RightPane
        {
            get { return _rightPaneControl; }
        }

        /// <summary>
        /// Creates a new instance.
        /// </summary>
        public BDRAreaMain()
        {
            _rightPaneControl.TheNavigationControl = _navigatorControl;
        }

        /// <summary>
        /// TrafodionManager Framework invokes this method when this area
        /// is activated. Any intilizations that need be done
        /// are handled here
        /// </summary>
        public void OnActivate()
        {
            _navigatorControl.TheNavigationTreeView.CurrentConnectionDefinition =
                TrafodionContext.Instance.CurrentConnectionDefinition;
            //Force the selection on the ConnectivityTreeView always.
            //We don't want the focus on the Favorites tree.
            _navigatorControl.TheNavigationTreeView.Select();
        }

    }
}
