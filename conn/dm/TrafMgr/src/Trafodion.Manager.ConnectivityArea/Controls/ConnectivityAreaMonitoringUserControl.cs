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
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.DatabaseArea.Controls;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User control that's displayed in the right pane
    /// </summary>
    public partial class ConnectivityAreaMonitoringUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        private ConnectivityTreeView _connectivityTreeView;
        private NavigationTreeNode _activeTreeNode;

        // only tabs that we want to keep tracking
        private static int currentSelectedNDCSServicesFolderSubTabPageIdx = 0;
        private static int currentSelectedDataSourcesFolderSubTabPageIdx = 0;
        private static int currentSelectedSystemFolderSubTabPageIdx = 0;

        // At System node, in Monitoring tab - 
        private ConnectivityAreaNDCSMonitorServicesStatusUserControl _theNDCSServicesStatusUserControl;
        private ConnectivityAreaNDCSMonitorServicesStatusSubPage _theNDCSServicesSummarySbuTabPage;
        private ConnectivityAreaDatasourceSummaryUserControl _theConnectivityAreaDatasourcesSummaryUserControl;
        private ConnectivityAreaDatasourcesSummarySubTabPage _theDatasourcesSummarySubTabPage;
        private NDCSPermissionsTabPage _thePermissionsTabPage;
        private AlterComponentPrivilegesUserControl _thePrivilegesUserControl;

        // At HPDCS Services node, in Monitoring tab -
        private ConnectivityAreaNDCSMonitorDSStatusUserControl _theNDCSMonitorDSStatusUserControl;
        private ConnectivityAreaNDCSMonitorDSStatusSubTabPage _theNDCSMonitorDSStatusSutTabPage;
        private ConnectivityAreaNDCSMonitorSessionsUserControl _theNDCSMonitorSessionsUserControl;
        private ConnectivityAreaNDCSMonitorSessionsSubTabPage _theNDCSMonitorSessionsSubTabPage;
        // At Data Source node in Monitoring tab - 
        private ConnectivityAreaDatasourceMonitorStatusUserControl _theDatasourceMonitorStatusUserControl;

        private ConnectivityAreaDatasourceMonitorSessionsUserControl _theDatasourceMonitorSessionsUserControl;
        private ConnectivityAreaDatasourceMonitorSessionsSubTabPage _theDatasourceMonitorSessionsSubTabPage;


        #endregion Fields

        # region Properties

        /// <summary>
        /// Expose Permission tab page
        /// </summary>
        public NDCSPermissionsTabPage PermissionsTabPage
        {
            get { return _thePermissionsTabPage; }
        }

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

        #endregion Properties

        #region ICloneToWindow Members

        /// <summary>
        /// Creates a new instance
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            ConnectivityAreaMonitoringUserControl theClonedConnectivityAreaMonitoringUserControl =
                new ConnectivityAreaMonitoringUserControl(this._connectivityTreeView);

            theClonedConnectivityAreaMonitoringUserControl.ActiveTreeNode = this.ActiveTreeNode;
            theClonedConnectivityAreaMonitoringUserControl.SetLabels(theTopPanelLowerLabel.Text);

            if (theClonedConnectivityAreaMonitoringUserControl.ActiveTreeNode is TreeNode)
            {
                theClonedConnectivityAreaMonitoringUserControl.TabSelected();
            }
            //else if (theClonedConnectivityAreaMonitoringUserControl is Exception)
            //{
            //    // raise the exception here
            //}

            theClonedConnectivityAreaMonitoringUserControl.Size = Size;

            return theClonedConnectivityAreaMonitoringUserControl;

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
        public ConnectivityAreaMonitoringUserControl(ConnectivityTreeView theConnectivityTreeView)
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
        /// Handles clicks to the Sub TabControl tabs
        /// </summary>
        /// 
        void ConnectivityMonitoring_PropogateTabSelectionChange(object sender, EventArgs e)
        {
            TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)this._connectivityTopTabControl.SelectedTab;

            if (this.ConnectivityTreeView.SelectedNode is NDCSServiceFolder || this.ConnectivityTreeView.SelectedNode is NDCSServicesFolder)
            {
                currentSelectedNDCSServicesFolderSubTabPageIdx = this._connectivityTopTabControl.SelectedIndex;
            }
            else if (this.ConnectivityTreeView.SelectedNode is DataSourceFolder || this.ConnectivityTreeView.SelectedNode is DataSourcesFolder)
            {
                currentSelectedDataSourcesFolderSubTabPageIdx = this._connectivityTopTabControl.SelectedIndex;
            }
            else if (this.ConnectivityTreeView.SelectedNode is ConnectivitySystemFolder)
            {
                currentSelectedSystemFolderSubTabPageIdx = this._connectivityTopTabControl.SelectedIndex;
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
                string theTopPanelLowerText = aNavigationTreeNode.LongerDescription;

                // Change depending on what type of node is selected                
                if (aNavigationTreeNode is NDCSServicesFolder)
                {
                    _connectivityTopTabControl.TabPages.Clear();

                    NDCSServicesFolder theServicesFolder = (NDCSServicesFolder)aNavigationTreeNode;

                    if (_theNDCSServicesSummarySbuTabPage != null)
                    {
                        _theNDCSServicesSummarySbuTabPage.Dispose();
                    }
                    if (_theNDCSServicesStatusUserControl != null)
                    {
                        _theNDCSServicesStatusUserControl.Dispose();
                    }

                    _theNDCSServicesStatusUserControl = new ConnectivityAreaNDCSMonitorServicesStatusUserControl(theServicesFolder.ConnectivitySystem, ConnectivityTreeView);
                    _theNDCSServicesSummarySbuTabPage = new ConnectivityAreaNDCSMonitorServicesStatusSubPage(this._theNDCSServicesStatusUserControl);
                    AddToTabControl(this._theNDCSServicesSummarySbuTabPage);
                    _theNDCSServicesStatusUserControl.HideRefreshButton(false);

                    if (_theNDCSMonitorDSStatusSutTabPage != null)
                    {
                        _theNDCSMonitorDSStatusSutTabPage.Dispose();
                    }
                    if (_theNDCSMonitorDSStatusUserControl != null)
                    {
                        _theNDCSMonitorDSStatusUserControl.Dispose();
                    }
                    _theNDCSMonitorDSStatusUserControl = new ConnectivityAreaNDCSMonitorDSStatusUserControl(theServicesFolder.ConnectivitySystem, ConnectivityTreeView);
                    _theNDCSMonitorDSStatusSutTabPage = new ConnectivityAreaNDCSMonitorDSStatusSubTabPage(this._theNDCSMonitorDSStatusUserControl);
                    AddNewTabControl(this._theNDCSMonitorDSStatusSutTabPage);

                    if (_theNDCSMonitorSessionsSubTabPage != null)
                    {
                        _theNDCSMonitorSessionsSubTabPage.Dispose();
                    }
                    if (_theNDCSMonitorSessionsUserControl != null)
                    {
                        _theNDCSMonitorSessionsUserControl.Dispose();
                    }
                    _theNDCSMonitorSessionsUserControl = new ConnectivityAreaNDCSMonitorSessionsUserControl(theServicesFolder.ConnectivitySystem, ConnectivityTreeView);
                    _theNDCSMonitorSessionsSubTabPage = new ConnectivityAreaNDCSMonitorSessionsSubTabPage(this._theNDCSMonitorSessionsUserControl);
                    AddNewTabControl(this._theNDCSMonitorSessionsSubTabPage);

                    if (currentSelectedNDCSServicesFolderSubTabPageIdx > -1)
                    {
                        this._connectivityTopTabControl.SelectTab(currentSelectedNDCSServicesFolderSubTabPageIdx);
                    }

                }
                else if (aNavigationTreeNode is NDCSServiceFolder)
                {
                    _connectivityTopTabControl.TabPages.Clear();
                    theTopPanelLowerText = Properties.Resources.NDCSService + " " + aNavigationTreeNode.LongerDescription;

                    NDCSServicesFolder theServicesFolder = (NDCSServicesFolder)aNavigationTreeNode.Parent;
                    NDCSServiceFolder theServiceFolder = (NDCSServiceFolder)aNavigationTreeNode;

                    if (_theNDCSServicesSummarySbuTabPage != null)
                    {
                        _theNDCSServicesSummarySbuTabPage.Dispose();
                    }
                    if (_theNDCSServicesStatusUserControl != null)
                    {
                        _theNDCSServicesStatusUserControl.Dispose();
                    }
                    _theNDCSServicesStatusUserControl = new ConnectivityAreaNDCSMonitorServicesStatusUserControl(theServicesFolder.ConnectivitySystem, ConnectivityTreeView, aNavigationTreeNode.Tag as NDCSService);
                    _theNDCSServicesStatusUserControl.HideRefreshButton(false);
                    _theNDCSServicesSummarySbuTabPage = new ConnectivityAreaNDCSMonitorServicesStatusSubPage(this._theNDCSServicesStatusUserControl);
                    AddToTabControl(this._theNDCSServicesSummarySbuTabPage);

                    if (_theNDCSMonitorDSStatusSutTabPage != null)
                    {
                        _theNDCSMonitorDSStatusSutTabPage.Dispose();
                    }
                    if (_theNDCSMonitorDSStatusUserControl != null)
                    {
                        _theNDCSMonitorDSStatusUserControl.Dispose();
                    }
                    _theNDCSMonitorDSStatusUserControl = new ConnectivityAreaNDCSMonitorDSStatusUserControl(theServicesFolder.ConnectivitySystem, ConnectivityTreeView, aNavigationTreeNode.Tag as NDCSService);
                    _theNDCSMonitorDSStatusSutTabPage = new ConnectivityAreaNDCSMonitorDSStatusSubTabPage(this._theNDCSMonitorDSStatusUserControl);
                    AddNewTabControl(this._theNDCSMonitorDSStatusSutTabPage);

                    if (_theNDCSMonitorSessionsSubTabPage != null)
                    {
                        _theNDCSMonitorSessionsSubTabPage.Dispose();
                    }
                    if (_theNDCSMonitorSessionsUserControl != null)
                    {
                        _theNDCSMonitorSessionsUserControl.Dispose();
                    }
                    _theNDCSMonitorSessionsUserControl = new ConnectivityAreaNDCSMonitorSessionsUserControl(theServicesFolder.ConnectivitySystem, ConnectivityTreeView, aNavigationTreeNode.Tag as NDCSService);
                    _theNDCSMonitorSessionsSubTabPage = new ConnectivityAreaNDCSMonitorSessionsSubTabPage(this._theNDCSMonitorSessionsUserControl);
                    AddNewTabControl(this._theNDCSMonitorSessionsSubTabPage);

                    if (currentSelectedNDCSServicesFolderSubTabPageIdx > -1 && currentSelectedSystemFolderSubTabPageIdx < _connectivityTopTabControl.TabPages.Count)
                    {
                        this._connectivityTopTabControl.SelectTab(currentSelectedNDCSServicesFolderSubTabPageIdx);
                    }
                }
                else if (aNavigationTreeNode is DataSourcesFolder)
                {
                    // display Datasources Tab only
                    DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode;

                    if (_theConnectivityAreaDatasourcesSummaryUserControl != null)
                    {
                        _theConnectivityAreaDatasourcesSummaryUserControl.Dispose();
                    }
                    this._theConnectivityAreaDatasourcesSummaryUserControl = new ConnectivityAreaDatasourceSummaryUserControl(dataSourcesFolder.ConnectivitySystem, ConnectivityTreeView);
                    _theConnectivityAreaDatasourcesSummaryUserControl.Populate();
                    AddToTabControl(_theConnectivityAreaDatasourcesSummaryUserControl, Properties.Resources.TabPageLabel_Datasources);
                    // there will be only on tab at this level... so, select the first Tab
                    this._connectivityTopTabControl.SelectTab(0);
                }
                else if (aNavigationTreeNode is DataSourceFolder)
                {

                    theTopPanelLowerText = Properties.Resources.Label_Datasource + " " + aNavigationTreeNode.LongerDescription;

                    DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)aNavigationTreeNode.Parent;
                    DataSourceFolder dataSourceFolder = (DataSourceFolder)aNavigationTreeNode;

                    if (_theDatasourceMonitorStatusUserControl != null)
                    {
                        _theDatasourceMonitorStatusUserControl.Dispose();
                    }
                    _theDatasourceMonitorStatusUserControl = new ConnectivityAreaDatasourceMonitorStatusUserControl(dataSourcesFolder.ConnectivitySystem, this._connectivityTreeView, aNavigationTreeNode.Tag as NDCSDataSource);
                    AddToTabControl(_theDatasourceMonitorStatusUserControl, Properties.Resources.NDCSDSStatus);

                    if (_theDatasourceMonitorSessionsSubTabPage != null)
                    {
                        if (_connectivityTopTabControl.TabPages.Contains(_theDatasourceMonitorSessionsSubTabPage))
                        {
                            _connectivityTopTabControl.TabPages.Remove(_theDatasourceMonitorSessionsSubTabPage);
                        }
                        _theDatasourceMonitorSessionsSubTabPage.Dispose();
                    }
                    if (_theDatasourceMonitorSessionsUserControl != null)
                    {
                        _theDatasourceMonitorSessionsUserControl.Dispose();
                    }
                    _theDatasourceMonitorSessionsUserControl = new ConnectivityAreaDatasourceMonitorSessionsUserControl(this._connectivityTreeView, dataSourceFolder.Tag as NDCSDataSource);
                    _theDatasourceMonitorSessionsSubTabPage = new ConnectivityAreaDatasourceMonitorSessionsSubTabPage(this._theDatasourceMonitorSessionsUserControl);
                    AddNewTabControl(this._theDatasourceMonitorSessionsSubTabPage);

                    if (currentSelectedDataSourcesFolderSubTabPageIdx > -1)
                    {
                        this._connectivityTopTabControl.SelectTab(currentSelectedDataSourcesFolderSubTabPageIdx);
                        if (this._connectivityTopTabControl.SelectedTab is ConnectivityAreaDatasourceMonitorSessionsSubTabPage)
                        {
                            this._theDatasourceMonitorSessionsSubTabPage.Refresh();
                        }
                    }
                }
                else if (aNavigationTreeNode is ConnectivitySystemFolder)
                {
                    // System Folder has been selected, if logged in, show the Connectivity Monitoring tab 
                    //with both "HPDCS Services" and "Datasources" Summary tabs only.
                    theTopPanelLowerText = aNavigationTreeNode.LongerDescription.Trim() + " - " + Properties.Resources.AreaName;

                    // Remove all the tabs pages from the TabControl
                    this._connectivityTopTabControl.TabPages.Clear();

                    ConnectivitySystemFolder theSystemFolder = (ConnectivitySystemFolder)aNavigationTreeNode;

                    if (_theNDCSServicesSummarySbuTabPage != null)
                    {
                        _theNDCSServicesSummarySbuTabPage.Dispose();
                    }
                    if (_theNDCSServicesStatusUserControl != null)
                    {
                        _theNDCSServicesStatusUserControl.Dispose();
                    }
                    // either first time access the node or when the selection changed from NDCSService node to system folder node
                    this._theNDCSServicesStatusUserControl = new ConnectivityAreaNDCSMonitorServicesStatusUserControl(theSystemFolder.ConnectivitySystem, ConnectivityTreeView);
                    this._theNDCSServicesSummarySbuTabPage = new ConnectivityAreaNDCSMonitorServicesStatusSubPage(this._theNDCSServicesStatusUserControl);
                    this._theNDCSServicesStatusUserControl.HideRefreshButton(true);
                    AddToTabControl(this._theNDCSServicesSummarySbuTabPage);

                    if (_theDatasourcesSummarySubTabPage != null)
                    {
                        _theDatasourcesSummarySubTabPage.Dispose();
                    }
                    if (_theConnectivityAreaDatasourcesSummaryUserControl != null)
                    {
                        _theConnectivityAreaDatasourcesSummaryUserControl.Dispose();
                    }
                    this._theConnectivityAreaDatasourcesSummaryUserControl = new ConnectivityAreaDatasourceSummaryUserControl(theSystemFolder.ConnectivitySystem, ConnectivityTreeView);
                    this._theDatasourcesSummarySubTabPage = new ConnectivityAreaDatasourcesSummarySubTabPage(this._theConnectivityAreaDatasourcesSummaryUserControl);
                    // make it visible
                    AddNewTabControl(this._theDatasourcesSummarySubTabPage);

                    if (this._thePrivilegesUserControl != null)
                    {
                        this._thePrivilegesUserControl.Dispose();
                    }
                    if (this._thePermissionsTabPage != null)
                    {
                        _thePermissionsTabPage.Dispose();
                    }
                    this._thePrivilegesUserControl = new AlterComponentPrivilegesUserControl(theSystemFolder.ConnectivitySystem.ConnectionDefinition, "HPDCS");
                    this._thePermissionsTabPage = new NDCSPermissionsTabPage(_thePrivilegesUserControl);

                    AddNewTabControl(this._thePermissionsTabPage);

                    if (currentSelectedSystemFolderSubTabPageIdx > -1 && currentSelectedSystemFolderSubTabPageIdx < _connectivityTopTabControl.TabPages.Count)
                    {
                        this._connectivityTopTabControl.SelectTab(currentSelectedSystemFolderSubTabPageIdx);
                        this._connectivityTopTabControl.SelectedTab.Refresh();
                    }

                }

                //this._connectivityTopTabControl.TrafodionTabControlSelectedEvent += new TabControlEventHandler(ConnectivityMonitoring_PropogateTabSelectionChange);

                SetLabels(theTopPanelLowerText);
            }
        }

        /// <summary>
        /// Helper method to add a control into a new tab into the right pane
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddNewTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            theTabPage.AutoScroll = true;
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            //TheConnectivityAreaTabControl.TabPages.Add(theTabPage);
            AddNewTabControl(theTabPage);
        }

        private void AddNewTabControl(TrafodionTabPage aTabPage)
        {
            aTabPage.AutoScroll = true;
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
            //MonitoringSubTabControl.TabPages.Add(theTabPage);
            //TheConnectivityAreaTabControl = MonitoringSubTabControl;
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
        /// 
        /// </summary>
        public bool datasourceValuesChanged()
        {
            try
            {
                if (!ConnectionDefn.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
                    return false;

                if ((_theNDCSMonitorDSStatusUserControl != null) && (_theNDCSMonitorDSStatusUserControl.ValuesChanged))
                {
                    return true;
                }
                else if ((_theDatasourceMonitorStatusUserControl != null) && (_theDatasourceMonitorStatusUserControl.ValuesChanged))
                {
                    return true;
                }
                
            }
            catch (Exception e)
            {
            }

            return false;
        }

        /// <summary>
        /// 
        /// </summary>
        public void ApplyTracingChanges()
        {
            try
            {
                TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)_connectivityTopTabControl.SelectedTab;

                if (theSelectedTabPage.Equals(_theNDCSMonitorDSStatusSutTabPage))
                {
                    _theNDCSMonitorDSStatusUserControl.ApplyTracingChanges();
                }
                else
                {
                    _theDatasourceMonitorStatusUserControl.ApplyTracingChanges();
                }
            }
            catch (Exception e)
            {
            }
        }


        /// <summary>
        /// 
        /// </summary>
        public void CancelTracingChanges()
        {
            try
            {
                TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)_connectivityTopTabControl.SelectedTab;

                if (theSelectedTabPage.Equals(_theNDCSMonitorDSStatusSutTabPage))
                {
                    _theNDCSMonitorDSStatusUserControl.CancelTracingChanges();
                }
                else
                {
                    _theDatasourceMonitorStatusUserControl.CancelTracingChanges();
                }

            }
            catch (Exception e)
            {
            }
        }

        /// <summary>
        /// Get and set the tab control
        /// </summary>
        public TrafodionTabControl TheConnectivityAreaTabControl
        {
            get
            {
                return _connectivityTopTabControl;
            }
            set
            {
                foreach (Control control in _ConnectivityMainBodyPanel.Controls)
                {
                    control.Dispose();
                }
                _ConnectivityMainBodyPanel.Controls.Clear();

                if (_connectivityTopTabControl != null)
                {
                    _connectivityTopTabControl.TrafodionTabControlSelectedEvent -= ConnectivityMonitoring_PropogateTabSelectionChange;
                }
                _connectivityTopTabControl = value;
                if (_connectivityTopTabControl != null)
                {
                    _connectivityTopTabControl.TrafodionTabControlSelectedEvent += ConnectivityMonitoring_PropogateTabSelectionChange;
                }
                _connectivityTopTabControl.Dock = DockStyle.Fill;
                _ConnectivityMainBodyPanel.Controls.Add(_connectivityTopTabControl);
            }
        }

        void _connectivityTopTabControl_TrafodionTabControlSelectedEvent(object sender, TabControlEventArgs e)
        {
            throw new NotImplementedException();
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
                theTopPanelLowerLabel.Text = aTopPanelLowerText.Trim();
                this.Monitor_TrafodionToolTip.SetToolTip(theTopPanelLowerLabel, aTopPanelLowerText);
            }
        }

        private void _refreshButton_Click(object sender, EventArgs e)
        {

        }
    }
}
