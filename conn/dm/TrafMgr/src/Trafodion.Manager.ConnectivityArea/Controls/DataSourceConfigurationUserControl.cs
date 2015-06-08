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
    public partial class DataSourceConfigurationUserControl : UserControl
    {
        #region Fields

        private ConnectivityNavigationControl _connectivityNavigationControl;
        private ConnectivityTreeView _connectivityTreeView;
        private NavigationTreeView.SelectedHandler theConnectivityTreeControlSelectedHandler = null;

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

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityNavigationControl ConnectivityNavigationControl
        {
            get { return _connectivityNavigationControl; }
            set
            {
                _connectivityNavigationControl = value;

                _connectivityTreeView = (_connectivityNavigationControl == null) ? null : _connectivityNavigationControl.ConnectivityTreeView;

                {
                    bool treeAvailable = (_connectivityTreeView != null);

                    // The tree navigation buttons should only be visible if there is a tree
                    //theParentButton.Visible = treeAvailable;
                    //theNextButton.Visible = treeAvailable;
                    //thePreviousButton.Visible = treeAvailable;

                    if (treeAvailable)
                    {
                        if (theConnectivityTreeControlSelectedHandler == null)
                        {
                            theConnectivityTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(ConnectivityTreeControlSelected);
                        }
                        ConnectivityTreeView.Selected += theConnectivityTreeControlSelectedHandler;

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
        public DataSourceConfigurationUserControl()
        {
            InitializeComponent();

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelUpperLabel.Text = "";
            theTopPanelLowerLabel.Text = "";

            // Set the button texts from resources
            theParentButton.Text = Properties.Resources.Parent;
            thePreviousButton.Text = Properties.Resources.Previous;
            theNextButton.Text = Properties.Resources.Next;

        }

        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        void ConnectivityTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
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
//                AddToTabControl(new ServicesUserControl(this, theServicesFolder.WmsSystem), Properties.Resources.Services);
            }
            else if (aNavigationTreeNode is NDCSServiceFolder)
            {
                NDCSServicesFolder theServicesFolder = (NDCSServicesFolder)aNavigationTreeNode.Parent;
                NDCSServiceFolder theServiceFolder = (NDCSServiceFolder)aNavigationTreeNode;
//                AddToTabControl(new AlterServicePane(theServicesFolder.WmsSystem, theServiceFolder.WmsService), Properties.Resources.AlterService);

            }
            else
            {
                
                if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                {
                    AddToTabControl(new FixSystemTabPage(theConnectionDefinition));
                }
                else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeMyActiveSystemsFolder)
                {
//                    AddToTabControl(new ConnectivityActiveSystemsUserControl(this), Properties.Resources.MyActiveSystems);
                }
                else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeMyOtherSystemsFolder)
                {
                    AddToTabControl(new MyOtherSystemsTabPage());
                }
                else if (aNavigationTreeNode is ConnectivitySystemFolder)
                {
                    // If logged in, show the WMS Configuration tab
                    ConnectivitySystemFolder theSystemFolder = (ConnectivitySystemFolder) aNavigationTreeNode;
                    //AddToTabControl(new EditSystemConfigurationUserControl(theSystemFolder.ConnectivitySystem), 
                    //    Properties.Resources.EditSystemConfig);
                }

            }
            
            SetLabels(theTopPanelUpperText, theTopPanelLowerText, theTopPanelLowerLabelRightText);
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl dsTabControl = new TrafodionTabControl();

            // Create the tab page with the user control dock filled
            TabPage theTabPage = new TabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            theTabPage.Controls.Add(aUserControl);
            dsTabControl.TabPages.Add(theTabPage);

            TheTabControl = dsTabControl;
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TabPage aTabPage)
        {
            TrafodionTabControl dsTabControl = new TrafodionTabControl();
            dsTabControl.TabPages.Add(aTabPage);

            TheTabControl = dsTabControl;
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
