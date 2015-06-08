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
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User control that's displayed in the right pane
    /// </summary>
    public partial class ConnectivityAreaMyFavoritesUserControl : UserControl
    {
        #region Fields

        private ConnectivityTreeView _connectivityTreeView;
        // only tabs that we want to keep tracking
        private int currentSelectedNDCSServiceFolderSubTabPageIdx = 0;
        private int currentSelectedNDCSServicesFolderSubTabPageIdx = 0;

        #endregion Fields

        # region Properties

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityTreeView ConnectivityTreeView
        {
            get { return _connectivityTreeView; }
            set { _connectivityTreeView = value; }
        }

        #endregion Properties

        /// <summary>
        /// Default Constructor
        /// </summary>
        public ConnectivityAreaMyFavoritesUserControl(ConnectivityTreeView theConnectivityTreeView)
        {
            InitializeComponent();

            if (theConnectivityTreeView != null)
            {
                this._connectivityTreeView = theConnectivityTreeView;
            }

//            this.TrafodionTabControlSelectedEvent += new TabControlEventHandler(ConnectivityMonitoring_PropogateTabSelectionChange);
            this.TabSelectedEvent += new TabControlEventHandler(ConnectivityMonitoring_PropogateTabSelectionChange);

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelUpperLabel.Text = "";
            theTopPanelLowerLabel.Text = "";

            // Set the button texts from resources
            theParentButton.Text = Properties.Resources.Parent;
            thePreviousButton.Text = Properties.Resources.Previous;
            theNextButton.Text = Properties.Resources.Next;

        }

        /// <summary>
        /// Handles clicks to the Top TabControl tabs
        /// </summary>
        /// 
        void ConnectivityMonitoring_PropogateTabSelectionChange(object sender, EventArgs e)
        {
            TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)_connectivityMonitoringSubTabControl.SelectedTab;

            if (this.ConnectivityTreeView.SelectedNode is NDCSServiceFolder)
            {
                currentSelectedNDCSServiceFolderSubTabPageIdx = this._connectivityMonitoringSubTabControl.SelectedIndex;
            }
            else if (this.ConnectivityTreeView.SelectedNode is NDCSServicesFolder)
            {
                currentSelectedNDCSServicesFolderSubTabPageIdx = this._connectivityMonitoringSubTabControl.SelectedIndex;
            }
        }

        
        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public void ConnectivityTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            theParentButton.Enabled = (aNavigationTreeNode.Parent != null);
            thePreviousButton.Enabled = (aNavigationTreeNode.PrevNode != null);
            theNextButton.Enabled = (aNavigationTreeNode.NextNode != null);

            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            string theTopPanelUpperText = (theConnectionDefinition != null) ? theConnectionDefinition.Description : "";
            string theTopPanelLowerText = aNavigationTreeNode.LongerDescription;
            string theTopPanelLowerLabelRightText = aNavigationTreeNode.LongerDescriptionLink;
            object theTopPanelLowerLabelRightTextTag = aNavigationTreeNode.LongerDescriptionLinkTag;

            if (theTopPanelUpperLabel.Text.Equals(theTopPanelLowerLabel.Text))
            {
                theTopPanelUpperText = "";
            }

            // Change depending on what type of node is selected                
            if (aNavigationTreeNode is NDCSServicesFolder)
            {

                NDCSServicesFolder theServicesFolder = (NDCSServicesFolder) aNavigationTreeNode;
                AddToTabControl(new ConnectivityAreaNDCSMonitorServicesStatusUserControl(theServicesFolder.ConnectivitySystem),
                    Properties.Resources.NDCSServicesStatus);

                AddNewTabControl(new ConnectivityAreaNDCSMonitorDSStatusUserControl(theServicesFolder.ConnectivitySystem),
                    Properties.Resources.NDCSDSStatus);

                AddNewTabControl(new ConnectivityAreaNDCSMonitorSessionsUserControl(theServicesFolder.ConnectivitySystem),
                    Properties.Resources.NDCSServerStatus);

                if (currentSelectedNDCSServicesFolderSubTabPageIdx > -1)
                {
                    this._connectivityMonitoringSubTabControl.SelectTab(currentSelectedNDCSServicesFolderSubTabPageIdx);
                }

            }
            else if (aNavigationTreeNode is NDCSServiceFolder)
            {
                NDCSServicesFolder theServicesFolder = (NDCSServicesFolder)aNavigationTreeNode.Parent;
                NDCSServiceFolder theServiceFolder = (NDCSServiceFolder)aNavigationTreeNode;
                AddToTabControl(new ConnectivityAreaNDCSMonitorServicesStatusUserControl(theServicesFolder.ConnectivitySystem, aNavigationTreeNode.Text),
                    Properties.Resources.NDCSServicesStatus);

                AddNewTabControl(new ConnectivityAreaNDCSMonitorDSStatusUserControl(theServicesFolder.ConnectivitySystem, aNavigationTreeNode.Text),
                    Properties.Resources.NDCSDSStatus);

                AddNewTabControl(new ConnectivityAreaNDCSMonitorSessionsUserControl(theServicesFolder.ConnectivitySystem, aNavigationTreeNode.Text),
                    Properties.Resources.NDCSServerStatus);

                if (currentSelectedNDCSServiceFolderSubTabPageIdx > -1)
                {
                    this._connectivityMonitoringSubTabControl.SelectTab(currentSelectedNDCSServiceFolderSubTabPageIdx);
                }
            } else if (aNavigationTreeNode is DataSourcesFolder)
            {
                DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode;
                AddToTabControl(new ConnectivityAreaDatasourceSummaryUserControl(dataSourcesFolder.ConnectivitySystem),
                   "Summary");
            }
            else if (aNavigationTreeNode is DataSourceFolder)
            {
                DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode.Parent;
                DataSourceFolder dataSourceFolder = (DataSourceFolder)aNavigationTreeNode;

                AddToTabControl(new ConnectivityAreaDatasourceMonitorStatusUserControl(dataSourcesFolder.ConnectivitySystem),
                   "Status");

                AddNewTabControl(new ConnectivityAreaDatasourceMonitorSessionsUserControl(dataSourcesFolder.ConnectivitySystem),
                    "Sessions");

            }
            else if (aNavigationTreeNode is ConnectivitySystemFolder)
            {
                // If logged in, show the Connectivity Monitoring tab
                ConnectivitySystemFolder theSystemFolder = (ConnectivitySystemFolder) aNavigationTreeNode;
                AddToTabControl(new ConnectivitySystemSummaryUserControl(theSystemFolder.ConnectivitySystem),
                    Properties.Resources.ActiveSystemSummary);
            }
          
            SetLabels(theTopPanelUpperText, theTopPanelLowerText, theTopPanelLowerLabelRightText);
        }

        /// <summary>
        /// Helper method to add a control into a new tab into the right pane
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddNewTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            //TheConnectivityAreaTabControl.TabPages.Add(theTabPage);
            AddNewTabControl(theTabPage);
        }

        private void AddNewTabControl(TrafodionTabPage aTabPage)
        {
            if (TheConnectivityAreaTabControl == null)
                TheConnectivityAreaTabControl = new TrafodionTabControl();

            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            TheConnectivityAreaTabControl.TabPages.Add(aTabPage);
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        /// <param name="aTabText"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl MonitoringSubTabControl = new TrafodionTabControl();

            // Create the tab page with the user control dock filled
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            MonitoringSubTabControl.TabPages.Add(theTabPage);
            TheConnectivityAreaTabControl = MonitoringSubTabControl;
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TrafodionTabPage aTabPage)
        {
            TrafodionTabControl aConnectivityTabControl = new TrafodionTabControl();
            aConnectivityTabControl.TabPages.Add(aTabPage);
            TheConnectivityAreaTabControl = aConnectivityTabControl;
        }

        /// <summary>
        /// Get and set the tab control
        /// </summary>
        public TrafodionTabControl TheConnectivityAreaTabControl
        {
            get
            {
                return _connectivityMonitoringSubTabControl;
            }
            set
            {
                _ConnectivityMainBodyPanel.Controls.Clear();
                _connectivityMonitoringSubTabControl = value;
                _connectivityMonitoringSubTabControl.Dock = DockStyle.Fill;
                _ConnectivityMainBodyPanel.Controls.Add(_connectivityMonitoringSubTabControl);
            }
        }

        private delegate void SetLabelsDelegate(string aTopPanelUpperText, string aTopPanelLowerText, string aTopPanelLowerLabelRightText);
        private void SetLabels(string aTopPanelUpperText, string aTopPanelLowerText, string aTopPanelLowerLabelRightText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelUpperText, aTopPanelLowerText, aTopPanelLowerLabelRightText });
            }
            else
            {
                theTopPanelUpperLabel.Text = aTopPanelUpperText;
                theTopPanelLowerLabel.Text = aTopPanelLowerText;

                theTopPanelLowerLabelRight.Text = aTopPanelLowerLabelRightText;

            }
        }

        private void theParentButton_Click(object sender, EventArgs e)
        {
            ConnectivityTreeView.SelectParent();
        }

        private void thePreviousButton_Click(object sender, EventArgs e)
        {
            ConnectivityTreeView.SelectPrevious();
        }

        private void theNextButton_Click(object sender, EventArgs e)
        {
            ConnectivityTreeView.SelectNext();
        }

    }
}
