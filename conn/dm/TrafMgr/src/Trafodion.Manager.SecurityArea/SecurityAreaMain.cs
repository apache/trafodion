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
using Trafodion.Manager.SecurityArea.Controls;
using Trafodion.Manager.SecurityArea.Model;
using System.Collections.Generic;

namespace Trafodion.Manager.SecurityArea
{
    public class SecurityAreaMain : ITrafodionArea, IOptionsProvider
    {
        private TrafodionTabPage _configurationTabPage;
        private SecurityNavigator _securityNavigator = null;
        private TrafodionTabControl _securityRightPane = null;

        /// <summary>
        /// Read only property that the famework uses to get our name
        /// </summary>
        public string AreaName { get { return Properties.Resources.SecurityAreaName; } }

        /// <summary>
        /// Read only property that the famework uses to get our navigator
        /// </summary>
        public Control Navigator { get { return SecurityNavigator; } }

        /// <summary>
        /// Read only property that the famework uses to get our right apne
        /// </summary>
        public Control RightPane { get { return SecurityRightPane; } }

        /// <summary>
        /// Read only property that framework uses to figure out the area's currently used connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return SecurityNavigator.SecurityTreeView.CurrentConnectionDefinition;
            }
        }
        /// <summary>
        /// The right pane for the database area
        /// </summary>
        public TrafodionTabControl SecurityRightPane
        {
            get { return _securityRightPane; }
            set { _securityRightPane = value; }
        }

        /// <summary>
        /// The navigation control for the database area
        /// </summary>
        public SecurityNavigator SecurityNavigator
        {
            get { return _securityNavigator; }
            set { _securityNavigator = value; }
        }
        /// <summary>
        /// Image displayed in the Area button
        /// </summary>
        public Image Image
        {
            get { return global::Trafodion.Manager.Properties.Resources.SecurityIcon; }
        }
        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            _securityNavigator.SecurityTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the DatbaseTreeView always. We dont want the focus on the Favorites tree.
            _securityNavigator.SecurityTreeView.Select();
        }
        
        public SecurityAreaMain()
        {
            SecurityNavigator = new SecurityNavigator();
            SecurityRightPane = new TrafodionTabControl();
            _configurationTabPage = new TrafodionTabPage(Properties.Resources.SecurityAreaName);
            SecurityObjectsControl theSecurityObjectsControl = new SecurityObjectsControl(SecurityNavigator);
            theSecurityObjectsControl.Dock = DockStyle.Fill;
            _configurationTabPage.Controls.Add(theSecurityObjectsControl);
            SecurityRightPane.TabPages.Add(_configurationTabPage);

            SecurityRightPane.Selected += TabSelected;

            //This is needed to deal with the menus just after the control is loaded
            SecurityRightPane.ParentChanged += new System.EventHandler(SecurityRightPane_ParentChanged);

        }

        void SecurityRightPane_ParentChanged(object sender, EventArgs e)
        {
            if ((SecurityRightPane != null) && (SecurityRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = SecurityRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }
        }

        private void TabSelected(object sender, TabControlEventArgs e)
        {
            if ((SecurityRightPane != null) && (SecurityRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = SecurityRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }

            //When a tab is selected, we want to ensure that the correct values are populated
            _securityNavigator.SecurityTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the SecurityTreeView always. We dont want the focus on the Favorites tree.
            _securityNavigator.SecurityTreeView.Select();

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
                optionObjects.Add(SecurityAreaOptions.OptionsKey, SecurityAreaOptions.GetOptions());
                return optionObjects;
            }
        }

        #endregion
    }
}
