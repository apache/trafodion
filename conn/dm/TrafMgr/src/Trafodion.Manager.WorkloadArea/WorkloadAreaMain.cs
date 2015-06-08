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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.WorkloadArea
{
    /// <summary>
    /// Workload Area
    /// </summary>
    public class WorkloadAreaMain : ITrafodionArea
    {
        #region Fields

        private WMSNavigationControl _wmsNavigatorControl = null;
        private WMSRightPaneControl _wmsRightPaneControl = null;
        private TrafodionTabControl _workloadsRightPane = null;
        private TrafodionTabPage _workloadsTabPage = null;
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
        /// Read only property that framework uses to figure out the area's currently used connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return _wmsNavigatorControl.WMSTreeView.CurrentConnectionDefinition;
            }
        }
        /// <summary>
        /// Property that the framework reads to get the area's navigator control
        /// </summary>
        public Control Navigator
        {
            get
            {
                return _wmsNavigatorControl;
            }
        }

        /// <summary>
        ///  Property that the framework reads to get the area's right pane control
        /// </summary>
        public Control RightPane
        {
            get
            {
                return _workloadsRightPane;
            }
        }

        /// <summary>
        /// Image for the workload area
        /// </summary>
        public System.Drawing.Image Image
        {
            get { return global::Trafodion.Manager.Properties.Resources.WorkloadIcon; }
        }

        #endregion Properties

        /// <summary>
        /// Creates a new instance of workload area
        /// </summary>
        public WorkloadAreaMain()
        {
            _wmsNavigatorControl = new WMSNavigationControl();
            _workloadsRightPane = new TrafodionTabControl();
            _workloadsTabPage = new TrafodionTabPage("Configuration");
            _wmsRightPaneControl = new WMSRightPaneControl(_wmsNavigatorControl);
            _wmsRightPaneControl.Dock = DockStyle.Fill;

            _workloadsTabPage.Controls.Add(_wmsRightPaneControl);
            _workloadsRightPane.TabPages.Add(_workloadsTabPage);

            _workloadsRightPane.Selected += TabSelected;

            //This is needed to deal with the menus just after the control is loaded
            _workloadsRightPane.ParentChanged += new System.EventHandler(WorkloadRightPane_ParentChanged);
        }

        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            _wmsNavigatorControl.WMSTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the WMSTreeView always. We don't want the focus on the Favorites tree.
            _wmsNavigatorControl.WMSTreeView.Select();
        }

        void WorkloadRightPane_ParentChanged(object sender, EventArgs e)
        {
            if ((_workloadsRightPane != null) && (_workloadsRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = _workloadsRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }
        }

        private void TabSelected(object sender, TabControlEventArgs e)
        {
            if ((_workloadsRightPane != null) && (_workloadsRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = _workloadsRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }

            //When a tab is selected, we want to ensure that the correct values are populated
            _wmsRightPaneControl.TheNavigationUserControl.TheNavigationTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the Tree view always. We dont want the focus on the Favorites tree.
            _wmsRightPaneControl.TheNavigationUserControl.TheNavigationTreeView.Select();

        }
    }
}
