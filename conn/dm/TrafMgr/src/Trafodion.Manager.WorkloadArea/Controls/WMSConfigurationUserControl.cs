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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.WorkloadArea.Controls.Tree;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User control that's displayed in the right pane
    /// </summary>
    public partial class WMSConfigurationUserControl : UserControl
    {
        #region Fields

        private WMSNavigationControl _wmsNavigationControl;
        private WMSTreeView _wmsTreeView;
        private NavigationTreeView.SelectedHandler theWMSTreeControlSelectedHandler = null;

        #endregion Fields

        # region Properties

        public WMSTreeView WmsTreeView
        {
            get { return _wmsTreeView; }
            set { _wmsTreeView = value; }
        }


        public WMSNavigationControl WmsNavigationControl
        {
            get { return _wmsNavigationControl; }
            set
            {
                _wmsNavigationControl = value;

                _wmsTreeView = (_wmsNavigationControl == null) ? null : _wmsNavigationControl.WMSTreeView;

                {
                    bool treeAvailable = (_wmsTreeView != null);

                    if (treeAvailable)
                    {
                        if (theWMSTreeControlSelectedHandler == null)
                        {
                            theWMSTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(WMSTreeControlSelected);
                        }
                        WmsTreeView.Selected += theWMSTreeControlSelectedHandler;

                        //if (theSqlMxTreeControlExceptionOccurredHandler == null)
                        //{
                        //    theSqlMxTreeControlExceptionOccurredHandler = new NavigationTreeView.ExceptionOccurredHandler(SqlMxTreeControlExceptionOccurred);
                        //}
                        //TheDatabaseTreeView.ExceptionOccurred += theSqlMxTreeControlExceptionOccurredHandler;

                        //if (theFilterChangedHandler == null)
                        //{
                        //    theFilterChangedHandler = new NavigationTreeNameFilter.ChangedHandler(FilterChanged);
                        //}

                        //NavigationTreeNameFilter.Changed += theFilterChangedHandler;
                    }

                }

            }
        }

        #endregion Properties

        /// <summary>
        /// Default Constructor
        /// </summary>
        public WMSConfigurationUserControl()
        {
            InitializeComponent();

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";
        }

        /// <summary>
        /// Handles clicks to the WMSTree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        void WMSTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            string theTopPanelUpperText = (theConnectionDefinition != null) ? theConnectionDefinition.Description : "";
            string theTopPanelLowerText = aNavigationTreeNode.LongerDescription;
            string theTopPanelLowerLabelRightText = aNavigationTreeNode.LongerDescriptionLink;
            object theTopPanelLowerLabelRightTextTag = aNavigationTreeNode.LongerDescriptionLinkTag;

            

            // Change depending on what type of node is selected                
            if (aNavigationTreeNode is ServicesFolder)
            {
                ServicesFolder theServicesFolder = (ServicesFolder) aNavigationTreeNode;
                AddToTabControl(new ServicesUserControl(this, theServicesFolder.WmsSystem), Properties.Resources.Services);
            }
            else if (aNavigationTreeNode is ServiceFolder)
            {
                ServicesFolder theServicesFolder = (ServicesFolder)aNavigationTreeNode.Parent;
                ServiceFolder theServiceFolder = (ServiceFolder)aNavigationTreeNode;
                AddToTabControl(new AlterServicePane(theServicesFolder.WmsSystem, theServiceFolder.WmsService), Properties.Resources.AlterService);

            }
            else
            {
                
                if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                {
                    AddToTabControl(new FixSystemTabPage(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                {
                    AddToTabControl(new WMSActiveSystemsUserControl(this), Properties.Resources.MyActiveSystems);
                }
                else if (aNavigationTreeNode is SystemFolder)
                {
                    // If logged in, show the WMS Configuration tab
                    SystemFolder theSystemFolder = (SystemFolder) aNavigationTreeNode;
                    AddToTabControl(new EditSystemConfigurationUserControl(theSystemFolder.WmsSystem), 
                        Properties.Resources.EditSystemConfig);
                }

            }
            
            SetLabels(theTopPanelUpperText, theTopPanelLowerText);
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl wmsTabControl = new TrafodionTabControl();

            // Create the tab page with the user control dock filled
            TabPage theTabPage = new TabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            theTabPage.Controls.Add(aUserControl);
            wmsTabControl.TabPages.Add(theTabPage);

            TheTabControl = wmsTabControl;
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TabPage aTabPage)
        {
            TrafodionTabControl wmsTabControl = new TrafodionTabControl();
            wmsTabControl.TabPages.Add(aTabPage);

            TheTabControl = wmsTabControl;
        }

        /// <summary>
        /// Get and set the tab control
        /// </summary>
        public TrafodionTabControl TheTabControl
        {
            get
            {
                return _configurationTabControl;
            }
            set
            {
                _configurationPanel.Controls.Clear();
                _configurationTabControl = value;
                _configurationTabControl.Dock = DockStyle.Fill;
                _configurationPanel.Controls.Add(_configurationTabControl);
            }
        }

        private delegate void SetLabelsDelegate(string aTopPanelLowerText, string aTopPanelLowerLabelRightText);
        private void SetLabels(string aTopPanelLowerText, string aTopPanelLowerLabelRightText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelLowerText, aTopPanelLowerLabelRightText });
            }
            else
            {
                theTopPanelLowerLabel.Text = aTopPanelLowerText;

                theTopPanelLowerLabelRight.Text = aTopPanelLowerLabelRightText;

            }
        }
    }
}
