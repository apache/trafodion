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

using System;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using System.Drawing;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public class ConnectivityAreaMonitoringTabPage : DelayedPopulateClonableTabPage
    {
        private ConnectivityAreaMonitoringUserControl theConnectivityAreaMonitoringUserControl;

        public ConnectivityAreaMonitoringUserControl TheConnectivityAreaMonitoringUserControl
        {
            get { return theConnectivityAreaMonitoringUserControl; }
            set { theConnectivityAreaMonitoringUserControl = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityAreaMonitoringTabPage(ConnectivityAreaMonitoringUserControl theMonitoringUserControl)
            : base(Properties.Resources.MonitoringTabName)
        {
            TheConnectivityAreaMonitoringUserControl = theMonitoringUserControl;
        }

        /// <summary>
        /// 
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();
            TheConnectivityAreaMonitoringUserControl.Dock = DockStyle.Fill;
            TheConnectivityAreaMonitoringUserControl.BackColor = Color.WhiteSmoke;

            Controls.Add(TheConnectivityAreaMonitoringUserControl);

        }

    }
}
