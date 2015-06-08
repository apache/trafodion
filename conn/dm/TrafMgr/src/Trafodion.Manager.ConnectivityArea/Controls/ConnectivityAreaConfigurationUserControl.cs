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
using Trafodion.Manager.ConnectivityArea.Model;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User control that's displayed in the right pane
    /// </summary>
    public partial class ConnectivityAreaConfigurationUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private ConnectivityNavigationControl _connectivityNavigationControl;
        private ConnectivityTreeView _connectivityTreeView;
        private NavigationTreeNode _activeTreeNode;

        private NDCSObject _ndcsObject;

        private NavigationTreeView.SelectedHandler theConnectivityTreeControlSelectedHandler = null;
        private int currentSelectedDataSourceFolderSubTabPageIdx = 0;
        private int currentSelectedDataSourcesFolderSubTabPageIdx = 0;

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
        public NavigationTreeNode ActiveTreeNode
        {
            get { return _activeTreeNode; }
            set { _activeTreeNode = value; }
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

                    if (treeAvailable)
                    {
                        if (theConnectivityTreeControlSelectedHandler == null)
                        {
                            theConnectivityTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(ConnectivityTreeControlSelected);
                        }
                        ConnectivityTreeView.Selected += theConnectivityTreeControlSelectedHandler;

                        //if (theTrafodionTreeControlExceptionOccurredHandler == null)
                        //{
                        //    theTrafodionTreeControlExceptionOccurredHandler = new NavigationTreeView.ExceptionOccurredHandler(TrafodionTreeControlExceptionOccurred);
                        //}
                        //TheDatabaseTreeView.ExceptionOccurred += theTrafodionTreeControlExceptionOccurredHandler;

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

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            ConnectivityAreaConfigurationUserControl theClonedConnectivityAreaConfigurationUserControl =
                new ConnectivityAreaConfigurationUserControl(this._connectivityTreeView);

            theClonedConnectivityAreaConfigurationUserControl.ActiveTreeNode = this.ActiveTreeNode;
            theClonedConnectivityAreaConfigurationUserControl.SetLabels(theTopPanelLowerLabel.Text);

            if (theClonedConnectivityAreaConfigurationUserControl.ActiveTreeNode is TreeNode)
            {
                theClonedConnectivityAreaConfigurationUserControl.TabSelected();
            }

            theClonedConnectivityAreaConfigurationUserControl.Size = Size;

            return theClonedConnectivityAreaConfigurationUserControl;
        }

        /// <summary>
        /// Get the window title of the cloned window
        /// </summary>
        public string WindowTitle
        {
            get { return Properties.Resources.TabPageTitle_Monitoring + " | " + theTopPanelLowerLabel.Text; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this.ConnectivityTreeView.CurrentConnectionDefinition; }
        }

        #endregion

        /// <summary>
        /// Default Constructor
        /// </summary>
        public ConnectivityAreaConfigurationUserControl(ConnectivityTreeView theConnectivityTreeView)
        {
            InitializeComponent();

            if (theConnectivityTreeView != null)
            {
                this._connectivityTreeView = theConnectivityTreeView;
            }

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";
        }

        /// <summary>
        /// Tree reference free Constructor
        /// </summary>
        public ConnectivityAreaConfigurationUserControl(NDCSObject theNDCSObject)
        {
            InitializeComponent();

            if (theNDCSObject != null)
            {
                this._ndcsObject = theNDCSObject;
            }

            // Initially non-blank to make them easy to see in the designer so set them empty now
            theTopPanelLowerLabel.Text = "";
        }

        /// <summary>
        /// empty constructor for 'Create New'
        /// </summary>
        public ConnectivityAreaConfigurationUserControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 
        /// </summary>
        public void TabSelected()
        {
            if (Trafodion.Manager.Framework.Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            if(_activeTreeNode != null && _activeTreeNode.TreeView != null)
            {
               TreeNode node = ((NavigationTreeView)_activeTreeNode.TreeView).FindByFullPath(_activeTreeNode.FullPath);
                if (node != null)
                {
                    this.ConnectivityTreeControlSelected(this._activeTreeNode);
                }
            }
        }


        /// <summary>
        /// Handles clicks to the Top TabControl tabs
        /// </summary>
        /// 
        void ConnectivityConfiguration_PropogateTabSelectionChange(object sender, EventArgs e)
        {
            TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)this._connectivityConfigurationSubTabControl.SelectedTab;

            if (this.ConnectivityTreeView.SelectedNode is DataSourceFolder)
            {
                currentSelectedDataSourceFolderSubTabPageIdx = this._connectivityConfigurationSubTabControl.SelectedIndex;
            }
            else if (this.ConnectivityTreeView.SelectedNode is DataSourcesFolder)
            {
                currentSelectedDataSourcesFolderSubTabPageIdx = this._connectivityConfigurationSubTabControl.SelectedIndex;
            }
        }

        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public void ConnectivityTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            if (theConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                string theTopPanelUpperText = (theConnectionDefinition != null) ? theConnectionDefinition.Description : "";
                string theTopPanelLowerText = aNavigationTreeNode.LongerDescription;
                string theTopPanelLowerLabelRightText = aNavigationTreeNode.LongerDescriptionLink;
                object theTopPanelLowerLabelRightTextTag = aNavigationTreeNode.LongerDescriptionLinkTag;

                // Change depending on what type of node is selected                
                // Note: only Datasources Folder and Datasource folder has Configuration Tab
                if (aNavigationTreeNode is DataSourcesFolder)
                {
                    DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode;
                    AddToTabControl(new ConnectivityAreaDatasourcesConfigPermissionsUserControl(dataSourcesFolder.ConnectivitySystem),
                            Properties.Resources.NDCSPrivileges);
                }
                else if (aNavigationTreeNode is DataSourceFolder)
                {
                    DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode.Parent;
                    DataSourceFolder dataSourceFolder = (DataSourceFolder)aNavigationTreeNode;

                    dataSourceFolder.NdcsDataSource.PopulateDetail();
                    AddToTabControl(new ConnectivityAreaDatasourceConfigGeneralUserControl(dataSourceFolder.NdcsDataSource, false),
                            Properties.Resources.TabPageLabel_GeneralProperties);

                    AddNewTabControl(new ConnectivityAreaDatasourceConfigDefineSetUserControl(dataSourceFolder.NdcsDataSource), Properties.Resources.TabPageLabel_Sets);

                    AddNewTabControl(new ConnectivityAreaDatasourceConfigCQDUserControl(dataSourceFolder.NdcsDataSource),
                           Properties.Resources.TabPageLabel_CQD);

                    AddNewTabControl(new ConnectivityAreaDatasourceConfigControlledTablesUserControl(dataSourceFolder.NdcsDataSource),
                           Properties.Resources.TabPageLabel_ControlledTables);


                    if (currentSelectedDataSourcesFolderSubTabPageIdx > -1)
                    {
                        this._connectivityConfigurationSubTabControl.SelectTab(currentSelectedDataSourcesFolderSubTabPageIdx);
                    }

                }

                this._connectivityConfigurationSubTabControl.TrafodionTabControlSelectedEvent += new TabControlEventHandler(ConnectivityConfiguration_PropogateTabSelectionChange);

                SetLabels(theTopPanelLowerText);
            }

        }


        /// <summary>
        /// Helper method to add a control into a new tab into the right pane
        /// </summary>
        /// <param name="aUserControl"></param>
        /// <param name="aTabText"></param>
        private void AddNewTabControl(UserControl aUserControl, string aTabText)
        {

            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            AddNewTabControl(theTabPage);

        }

        private void AddNewTabControl(TrafodionTabPage aTabPage)
        {
            if(TheConnectivityAreaTabControl == null)
                TheConnectivityAreaTabControl = new TrafodionTabControl();

            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            TheConnectivityAreaTabControl.TabPages.Add(aTabPage);
        }

        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabText"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl topTabControl = new TrafodionTabControl();
            // Create the tab page with the user control dock filled
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);

            AddToTabControl(theTabPage);
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
                return _connectivityConfigurationSubTabControl;
            }
            set
            {
                _ConnectivityMainBodyPanel.Controls.Clear();
                _connectivityConfigurationSubTabControl = value;
                _connectivityConfigurationSubTabControl.Dock = DockStyle.Fill;
                _ConnectivityMainBodyPanel.Controls.Add(_connectivityConfigurationSubTabControl);
            }
        }

        private delegate void SetLabelsDelegate(string aTopPanelLowerText);
        private void SetLabels(string aTopPanelLowerText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelLowerText});
            }
            else
            {
                theTopPanelLowerLabel.Text = aTopPanelLowerText;
            }
        }
    }
}
