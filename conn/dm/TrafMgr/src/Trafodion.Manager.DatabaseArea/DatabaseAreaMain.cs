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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea
{
    /// <summary>
    /// ITrafodionArea implementation for Database area
    /// </summary>
    public class DatabaseAreaMain : ITrafodionArea, IOptionsProvider
    {
        private TrafodionTabPage _theObjectsTabPage;
        //private TrafodionTabPage _theQueriesTabPage;

        /// <summary>
        /// The constructor for the database area main
        /// </summary>
        public DatabaseAreaMain()
        {

            // Create our navigator
            TheDatabaseAreaNavigator = new DatabaseNavigator();

            // Create our right pane
            {
                TheDatabaseRightPane = new TrafodionTabControl();

                // Add the Objects tab
                {
                    _theObjectsTabPage = new TrafodionTabPage("Objects");
                    _theObjectsTabPage.Name = "_theObjectsTabPage";
                    DatabaseObjectsControl theDatabaseObjectsControl = new DatabaseObjectsControl(TheDatabaseAreaNavigator);
                    theDatabaseObjectsControl.Dock = DockStyle.Fill;
                    _theObjectsTabPage.Controls.Add(theDatabaseObjectsControl);
                    TheDatabaseRightPane.TabPages.Add(_theObjectsTabPage);
                }                

                //Add the event handler for tab selection
                TheDatabaseRightPane.Selected += TabSelected;

                //This is needed to deal with the menus just after the control is loaded
                TheDatabaseRightPane.ParentChanged += new System.EventHandler(TheDatabaseRightPane_ParentChanged);

                //// Make sure that the DatabaseObjectsControl is visible if the user clicks the tree
                //{
                //    DatabaseTreeView theDatabaseTreeView = TheDatabaseAreaNavigator.DatabaseTreeView;
                //    theDatabaseTreeView.AfterSelect += new TreeViewEventHandler(DatabaseTreeView_AfterSelect);
                //    theDatabaseTreeView.BeforeExpand += new TreeViewCancelEventHandler(DatabaseTreeView_BeforeExpand);
                //}

#if FOR_DEMOS

                TrafodionTabPage theTabPage;
                theTabPage = new TrafodionTabPage("Monitoring");
                DatabaseMonitoringControl theDatabaseMonitoringControl = new DatabaseMonitoringControl();
                theDatabaseMonitoringControl.Dock = DockStyle.Fill;
                theTabPage.Controls.Add(theDatabaseMonitoringControl);

                TheDatabaseRightPane.TabPages.Add(theTabPage);

                theTabPage = new TrafodionTabPage("Configuration");

                TheDatabaseRightPane.TabPages.Add(theTabPage);

#endif

            }

        }

        void DatabaseTreeView_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            SelectObjectsTabPage();
        }

        void DatabaseTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            SelectObjectsTabPage();
        }

        void TheDatabaseRightPane_ParentChanged(object sender, EventArgs e)
        {
            if ((TheDatabaseRightPane != null) && (TheDatabaseRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = TheDatabaseRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }
        }

        private void SelectObjectsTabPage()
        {
            if ((TheDatabaseRightPane != null) && (!TheDatabaseRightPane.SelectedTab.Name.Equals(_theObjectsTabPage.Name)))
            {
                TheDatabaseRightPane.SelectedTab = _theObjectsTabPage;
            }
            
        }

        private void TabSelected(object sender, TabControlEventArgs e)
        {
            if ((TheDatabaseRightPane != null) && (TheDatabaseRightPane.SelectedTab != null))
            {
                IMenuProvider menuProvider = TheDatabaseRightPane.SelectedTab as IMenuProvider;
                TrafodionContext.Instance.TheTrafodionMain.PopulateMenubar(menuProvider);
            }

            //When a tab is selected, we want to ensure that the correct values are populated
            theDatabaseAreaNavigator.DatabaseTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the DatbaseTreeView always. We dont want the focus on the Favorites tree.
            theDatabaseAreaNavigator.DatabaseTreeView.Select();

        }

        /// <summary>
        /// Read only property that the famework uses to get our name
        /// </summary>
        public string AreaName { get { return Properties.Resources.Database; } }

        /// <summary>
        /// Read only property that the famework uses to get our navigator
        /// </summary>
        public Control Navigator { get { return TheDatabaseAreaNavigator; } }

        /// <summary>
        /// Read only property that the famework uses to get our right apne
        /// </summary>
        public Control RightPane { get { return TheDatabaseRightPane; } }

        /// <summary>
        /// Image displayed in the Area button
        /// </summary>
        public Image Image
        {
            get { return global::Trafodion.Manager.Properties.Resources.DatabaseIcon; }
        }

        /// <summary>
        /// Read only property that framework uses to figure out the area's currently used connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return TheDatabaseAreaNavigator.DatabaseTreeView.CurrentConnectionDefinition;
            }
        }

        /// <summary>
        /// The right pane for the database area
        /// </summary>
        public TrafodionTabControl TheDatabaseRightPane
        {
            get { return theDatabaseRightPane; }
            set { theDatabaseRightPane = value; }
        }

        /// <summary>
        /// The navigation control for the database area
        /// </summary>
        public DatabaseNavigator TheDatabaseAreaNavigator
        {
            get { return theDatabaseAreaNavigator; }
            set { theDatabaseAreaNavigator = value; }
        }

        /// <summary>
        /// This area is now the active area in the right pane of the UI
        /// </summary>
        public void OnActivate()
        {
            theDatabaseAreaNavigator.DatabaseTreeView.CurrentConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;

            //Force the selection on the DatbaseTreeView always. We dont want the focus on the Favorites tree.
            theDatabaseAreaNavigator.DatabaseTreeView.Select();
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
                optionObjects.Add(DatabaseAreaOptions.OptionsKey, DatabaseAreaOptions.GetOptions());
                return optionObjects;
            }
        }
        
        #endregion


        private DatabaseNavigator theDatabaseAreaNavigator = null;
        private TrafodionTabControl theDatabaseRightPane = null;
        //private DatabaseAreaOptions theOptionsPanel = null;
    }

}
