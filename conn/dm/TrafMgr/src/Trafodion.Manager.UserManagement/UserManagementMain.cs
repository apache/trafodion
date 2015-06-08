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

using System;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UserManagement.Controls;

namespace Trafodion.Manager.UserManagement
{
    public class UserManagementMain : ITrafodionArea
    {
        private TrafodionTabPage _configurationTabPage;
        private UserMgmtNavigator _userMgmtNavigator = null;
        private TrafodionTabControl _userMgmtRightPane = null;

        /// <summary>
        /// Read only property that the famework uses to get our name
        /// </summary>
        public string AreaName { get { return Properties.Resources.UserMgmtAreaName; } }

        /// <summary>
        /// Read only property that the famework uses to get our navigator
        /// </summary>
        public Control Navigator { get { return UserMgmtNavigator; } }

        /// <summary>
        /// Read only property that the famework uses to get our right apne
        /// </summary>
        public Control RightPane { get { return UserMgmtRightPane; } }

        /// <summary>
        /// Read only property that framework uses to figure out the area's currently used connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return UserMgmtNavigator.TheNavigationTreeView.CurrentConnectionDefinition;
            }
        }
        /// <summary>
        /// The right pane for the database area
        /// </summary>
        public TrafodionTabControl UserMgmtRightPane
        {
            get { return _userMgmtRightPane; }
            set { _userMgmtRightPane = value; }
        }

        /// <summary>
        /// The navigation control for the database area
        /// </summary>
        public UserMgmtNavigator UserMgmtNavigator
        {
            get { return _userMgmtNavigator; }
            set { _userMgmtNavigator = value; }
        }

        /// <summary>
        /// Image displayed in the Area button
        /// </summary>
        public Image Image
        {
            get { return global::Trafodion.Manager.Properties.Resources.MultiUserIcon; }
        }

        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            _userMgmtNavigator.TheNavigationTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the DatbaseTreeView always. We dont want the focus on the Favorites tree.
            _userMgmtNavigator.TheNavigationTreeView.Select();
        }
        
        public UserManagementMain()
        {
            UserMgmtNavigator = new UserMgmtNavigator();
            UserMgmtRightPane = new TrafodionTabControl();
            _configurationTabPage = new TrafodionTabPage(Properties.Resources.UserMgmtAreaName);
            UserMgmtRightPaneControl theUserMgmtRightPaneControl = new UserMgmtRightPaneControl(UserMgmtNavigator);
            theUserMgmtRightPaneControl.Dock = DockStyle.Fill;
            _configurationTabPage.Controls.Add(theUserMgmtRightPaneControl);
            UserMgmtRightPane.TabPages.Add(_configurationTabPage);

            UserMgmtRightPane.Selected += TabSelected;

            //This is needed to deal with the menus just after the control is loaded
            UserMgmtRightPane.ParentChanged += new System.EventHandler(UserMgmtRightPane_ParentChanged);

        }

        void UserMgmtRightPane_ParentChanged(object sender, EventArgs e)
        {
            if ((UserMgmtRightPane != null) && (UserMgmtRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = UserMgmtRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }
        }

        private void TabSelected(object sender, TabControlEventArgs e)
        {
            if ((UserMgmtRightPane != null) && (UserMgmtRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = UserMgmtRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }

            //When a tab is selected, we want to ensure that the correct values are populated
            _userMgmtNavigator.TheNavigationTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the SecurityTreeView always. We dont want the focus on the Favorites tree.
            _userMgmtNavigator.TheNavigationTreeView.Select();

        }
    }
}
