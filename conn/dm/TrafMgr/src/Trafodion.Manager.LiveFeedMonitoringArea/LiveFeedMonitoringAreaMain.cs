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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.LiveFeedMonitoringArea.Controls.Tree;
using Trafodion.Manager.LiveFeedMonitoringArea.Controls;
using Trafodion.Manager.LiveFeedMonitoringArea.Models;
using System.Collections.Generic;

namespace Trafodion.Manager.LiveFeedMonitoringArea
{
    public class LiveFeedMonitoringAreaMain : ITrafodionArea, IOptionsProvider
    {
        private TrafodionTabPage _configurationTabPage;
        private LiveFeedMonitorNavigator _LiveFeedMonitorNavigator = null;
        private TrafodionTabControl _LiveFeedMonitorRightPane = null;

        /// <summary>
        /// Read only property that the famework uses to get our name
        /// </summary>
        public string AreaName { get { return Trafodion.Manager.LiveFeedMonitoringArea.Properties.Resources.AreaName; } }

        /// <summary>
        /// Read only property that the famework uses to get our navigator
        /// </summary>
        public Control Navigator { get { return _LiveFeedMonitorNavigator; } }

        /// <summary>
        /// Read only property that the famework uses to get our right apne
        /// </summary>
        public Control RightPane { get { return _LiveFeedMonitorRightPane; } }

        /// <summary>
        /// Read only property that framework uses to figure out the area's currently used connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return _LiveFeedMonitorNavigator.LiveFeedMonitorTreeView.CurrentConnectionDefinition;
            }
        }
        /// <summary>
        /// The right pane for the database area
        /// </summary>
        public TrafodionTabControl LiveFeedMonitorRightPane
        {
            get { return _LiveFeedMonitorRightPane; }
            set { _LiveFeedMonitorRightPane = value; }
        }

        /// <summary>
        /// The navigation control for the database area
        /// </summary>
        public LiveFeedMonitorNavigator LiveFeedMonitorNavigator
        {
            get { return _LiveFeedMonitorNavigator; }
            set { _LiveFeedMonitorNavigator = value; }
        }
        /// <summary>
        /// Image displayed in the Area button
        /// </summary>
        public Image Image
        {
            get { return Properties.Resources.MessageQueuing; }
        }
        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            LiveFeedMonitorNavigator.LiveFeedMonitorTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the DatbaseTreeView always. We dont want the focus on the Favorites tree.
            LiveFeedMonitorNavigator.LiveFeedMonitorTreeView.Select();
        }

        public LiveFeedMonitoringAreaMain()
        {
            LiveFeedMonitorNavigator = new LiveFeedMonitorNavigator();
            LiveFeedMonitorRightPane = new TrafodionTabControl();
            _configurationTabPage = new TrafodionTabPage(Properties.Resources.AreaName);
            LiveFeedMonitorObjectsControl theLiveFeedMonitorObjectsControl = new LiveFeedMonitorObjectsControl(LiveFeedMonitorNavigator);
            theLiveFeedMonitorObjectsControl.Dock = DockStyle.Fill;
            _configurationTabPage.Controls.Add(theLiveFeedMonitorObjectsControl);
            LiveFeedMonitorRightPane.TabPages.Add(_configurationTabPage);

            LiveFeedMonitorRightPane.Selected += TabSelected;

            //This is needed to deal with the menus just after the control is loaded
            LiveFeedMonitorRightPane.ParentChanged += new System.EventHandler(RightPane_ParentChanged);

        }

        void RightPane_ParentChanged(object sender, EventArgs e)
        {
            if ((LiveFeedMonitorRightPane != null) && (LiveFeedMonitorRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = LiveFeedMonitorRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }
        }

        private void TabSelected(object sender, TabControlEventArgs e)
        {
            if ((LiveFeedMonitorRightPane != null) && (LiveFeedMonitorRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = LiveFeedMonitorRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }

            //When a tab is selected, we want to ensure that the correct values are populated
            LiveFeedMonitorNavigator.LiveFeedMonitorTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the SecurityTreeView always. We dont want the focus on the Favorites tree.
            LiveFeedMonitorNavigator.LiveFeedMonitorTreeView.Select();

        }

        #region IOptionsProvider implementation

        /// <summary>
        /// Property that the framework reads to get the options control
        /// </summary>
        public List<IOptionControl> OptionControls
        {
            get
            {
                return null;
            }
        }

        /// <summary>
        /// The Database options object
        /// </summary>
        public Dictionary<String, IOptionObject> OptionObjects
        {
            get
            {
                Dictionary<string, IOptionObject> optionObjects = new Dictionary<string, IOptionObject>();
                return optionObjects;
            }
        }

        #endregion
    }
}


