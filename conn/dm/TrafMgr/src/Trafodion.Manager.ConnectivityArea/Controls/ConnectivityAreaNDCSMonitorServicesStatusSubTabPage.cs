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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public class ConnectivityAreaNDCSMonitorServicesStatusSubPage : DelayedPopulateTabPage
    {
        private ConnectivityAreaNDCSMonitorServicesStatusUserControl theConnectivityAreaNDCSMonitorServicesStatusUserControl;

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityAreaNDCSMonitorServicesStatusUserControl TheConnectivityAreaNDCSMonitorServicesStatusUserControl
        {
            get { return theConnectivityAreaNDCSMonitorServicesStatusUserControl; }
            set { theConnectivityAreaNDCSMonitorServicesStatusUserControl = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityAreaNDCSMonitorServicesStatusSubPage(
            ConnectivityAreaNDCSMonitorServicesStatusUserControl theNDCSMonitorServicesStatusUserControl)
            : base(Properties.Resources.TabPageLabel_Services)
        {
            TheConnectivityAreaNDCSMonitorServicesStatusUserControl = theNDCSMonitorServicesStatusUserControl;
        }

        public override void PrepareForPopulate()
        {
            if (TheConnectivityAreaNDCSMonitorServicesStatusUserControl.NdcsService != null)
            {
                object a = TheConnectivityAreaNDCSMonitorServicesStatusUserControl.NdcsService.GetStatus();
            }
            else
            {
                object a  = TheConnectivityAreaNDCSMonitorServicesStatusUserControl.NdcsSystem.GetAllServiceStatus();
            }            
        }
        /// <summary>
        /// 
        /// </summary>
        protected override void  Populate()
        {
            Controls.Clear();
            TheConnectivityAreaNDCSMonitorServicesStatusUserControl.Dock = DockStyle.Fill;
            TheConnectivityAreaNDCSMonitorServicesStatusUserControl.BackColor = Color.WhiteSmoke;
            TheConnectivityAreaNDCSMonitorServicesStatusUserControl.Populate();
            Controls.Add(TheConnectivityAreaNDCSMonitorServicesStatusUserControl);
        }

    }
}
