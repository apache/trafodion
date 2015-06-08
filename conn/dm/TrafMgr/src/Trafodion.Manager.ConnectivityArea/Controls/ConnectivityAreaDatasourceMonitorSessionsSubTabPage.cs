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
    public class ConnectivityAreaDatasourceMonitorSessionsSubTabPage : DelayedPopulateClonableTabPage
    {
        private ConnectivityAreaDatasourceMonitorSessionsUserControl theConnectivityAreaDatasourceMonitorSessionsUserControl;

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityAreaDatasourceMonitorSessionsUserControl TheConnectivityAreaDatasourceMonitorSessionsUserControl
        {
            get { return theConnectivityAreaDatasourceMonitorSessionsUserControl; }
            set { theConnectivityAreaDatasourceMonitorSessionsUserControl = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityAreaDatasourceMonitorSessionsSubTabPage(ConnectivityAreaDatasourceMonitorSessionsUserControl theDatasourceMonitorSessionsUserControl)
            : base(Properties.Resources.TabPageLabel_NDCSServerStatus)
        {
            TheConnectivityAreaDatasourceMonitorSessionsUserControl = theDatasourceMonitorSessionsUserControl;

        }


        /// <summary>
        /// Delay populate the control
        /// </summary>
        public override void Refresh()
        {
            DoPopulate();
        }

        public override void PrepareForPopulate()
        {
            object a = TheConnectivityAreaDatasourceMonitorSessionsUserControl.NDCSDataSource.GetAllSessions();
        }
        /// <summary>
        /// 
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();
            TheConnectivityAreaDatasourceMonitorSessionsUserControl.Dock = DockStyle.Fill;
            TheConnectivityAreaDatasourceMonitorSessionsUserControl.BackColor = Color.WhiteSmoke;
            Cursor.Current = Cursors.WaitCursor;
            try
            {
                TheConnectivityAreaDatasourceMonitorSessionsUserControl.Populate();
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }

            Controls.Add(TheConnectivityAreaDatasourceMonitorSessionsUserControl);
        }

    }
}
