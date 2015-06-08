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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.ConnectivityArea.Model;


namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User control for the right pane,  no need to clone the top Right Panel
    /// </summary>
    public partial class ConnectivityRightPaneControl : UserControl
    {
        #region Fields

        /// <summary>
        /// The clonable implementer or not
        /// </summary>
        private ICloneToWindow theICloneToWindow = null;

        private ConnectivityNavigationControl _connectivityNavigationControl;
        private ConnectivityTreeView _connectivityTreeView;

//        private ConnectivityFavoritesTreeView _connectivityFavoritesTreeView;
        private FavoritesTreeView _connectivityFavoritesTreeView;

        private NavigationTreeView.SelectedHandler theConnectivityTreeControlSelectedHandler = null;
        private TreeViewCancelEventHandler theConnectivityTreeControlBeforeSelectedHandler = null; //To capture 'Before Select'
        private NavigationTreeNameFilter.ChangedHandler theFilterChangedHandler = null;

        //private ConnectivityFavoritesTreeView.SelectedHandler theConnectivityFavoritesTreeControlSelectedHandler = null;

        // For System tab
        private ConnectivityAreaSystemsUserControl _connectivityAreaSystemsUserControl;
        private TrafodionTabPage _theSystemTabPage = new TrafodionTabPage(Properties.Resources.SystemTabName);

        // For Monitoring tab
        private ConnectivityAreaMonitoringUserControl _connectivityAreaMonitoringUserControl;
        private TrafodionTabPage _theMonitoringTabPage = new TrafodionTabPage(Properties.Resources.MonitoringTabName);

        // For Configuration tab
        private ConnectivityAreaDatasourceConfigurationUserControl _connectivityAreaConfigurationUserControl;
        private TrafodionTabPage _theConfigurationTabPage = new TrafodionTabPage(Properties.Resources.ConfigurationTabName);

        private ConnectivityAreaErrorUserControl _connectivityAreaErrorUserControl;
        private TrafodionTabPage _theErrorTabPage = new TrafodionTabPage("System");

        // For My Favorites tab
        //private ConnectivityAreaMyFavoritesUserControl _connectivityAreaMyFavoritesUserControl;

        // only DataSource has 2 top level tabs that we want to keep tracking
        private int currentSelectedDataSourceTabPageIdx = 0;
        private string previousFullPath = "";
        private bool selectedNodeChanged = false;

        private object theMostRecentWorkObject = null;
        private NavigationTreeNameFilter theNameFilter = new NavigationTreeNameFilter();

        public NavigationTreeNameFilter TheNameFilter
        {
            get { return theNameFilter; }
            set { theNameFilter = value; }
        }

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
        public FavoritesTreeView ConnectivityFavoritesTreeView
        //public ConnectivityFavoritesTreeView ConnectivityFavoritesTreeView
        {
            get { return _connectivityFavoritesTreeView; }
            set { _connectivityFavoritesTreeView = value; }
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
                _connectivityFavoritesTreeView = (_connectivityNavigationControl == null) ? null : (FavoritesTreeView)_connectivityNavigationControl.ConnectivityFavoritesTreeView;

                {
                    bool treeAvailable = (_connectivityTreeView != null);
                    bool favoritestreeAvailable = (_connectivityFavoritesTreeView != null);
                    if (treeAvailable)
                    {
                        if (theConnectivityTreeControlSelectedHandler == null)
                        {
                            theConnectivityTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(ConnectivityTreeControlSelected);
                        }

                        //Event handler added to capture the 'Before Select' event. 
                        //This allows us to cancel if changes haven't been saved
                        if (theConnectivityTreeControlBeforeSelectedHandler == null)
                        {
                            theConnectivityTreeControlBeforeSelectedHandler = new TreeViewCancelEventHandler(ConnectivityTreeViewControlBeforeSelect);
                        }
                        
                        ConnectivityTreeView.Selected += theConnectivityTreeControlSelectedHandler;
                        ConnectivityTreeView.BeforeSelect += theConnectivityTreeControlBeforeSelectedHandler;
                       
                        if (theFilterChangedHandler == null)
                        {
                            theFilterChangedHandler = new NavigationTreeNameFilter.ChangedHandler(FilterChanged);
                        }

                        NavigationTreeNameFilter.Changed += theFilterChangedHandler;
                    }

                    //if (favoritestreeAvailable)
                    //{
                    //    if (theConnectivityFavoritesTreeControlSelectedHandler == null)
                    //    {
                    //        theConnectivityFavoritesTreeControlSelectedHandler = new ConnectivityFavoritesTreeView.SelectedHandler(ConnectivityFavoritesTreeControlSelected);
                    //    }
                    //    ConnectivityFavoritesTreeView += theConnectivityFavoritesTreeControlSelectedHandler;
                    //}

                }

            }
        }

        void ConnectivityRightPaneControl_Click(object sender, EventArgs e)
        {
            throw new NotImplementedException();
        }


        #endregion Properties

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectivityNavigationControl"></param>
        public ConnectivityRightPaneControl(ConnectivityNavigationControl aConnectivityNavigationControl)
        {
            InitializeComponent();
            this.ConnectivityNavigationControl = aConnectivityNavigationControl;

            _connectivityTopTabControl.TrafodionTabControlSelectedEvent += new TabControlEventHandler(ConnectivityRightPaneControl_PropogateTabSelectionChange);
            //_connectivityTopTabControl.MouseClick += new System.Windows.Forms.MouseEventHandler(ConnectivityRightPaneControl_MouseClick);

//            this.connectivityAreaSystemsUserControl.ConnectivityNavigationControl = aConnectivityNavigationControl;
//            this.connectivityAreaMonitoringUserControl.ConnectivityNavigationControl = aConnectivityNavigationControl;
//            this.connectivityAreaConfigurationUserControl.ConnectivityNavigationControl = aConnectivityNavigationControl;
        }

        /// <summary>
        /// Handles clicks to the Top TabControl tabs
        /// </summary>
        /// 
        public void ConnectivityRightPaneControl_PropogateTabSelectionChange(object sender, TabControlEventArgs e)
        {
            TrafodionTabPage theSelectedTabPage = (TrafodionTabPage)_connectivityTopTabControl.SelectedTab;

            if (theSelectedTabPage != null && theSelectedTabPage.Equals(_theConfigurationTabPage))
            {
                this._connectivityAreaConfigurationUserControl.TabSelected();


                
                //if (_connectivityAreaConfigurationUserControl.TheConnectivityAreaTabControl.TabCount == 0)
                //{
                //    // make all tabs by sending in the Tab is selected message
                //    // handle mouse click on Tab...
                //    this._connectivityAreaConfigurationUserControl.TabSelected();            
                //}
                //else
                //{
                //    // tab is available, make it visible
                //    _connectivityAreaConfigurationUserControl.Show();
                //}

            }
            else if (theSelectedTabPage != null && theSelectedTabPage.Equals(_theMonitoringTabPage))
            {
                // SK -- handle mouse click on tab...
                //_connectivityAreaMonitoringUserControl.Show();
                this._connectivityAreaMonitoringUserControl.TabSelected();
            }

            if (this.ConnectivityTreeView.SelectedNode is DataSourceFolder)
            {
                if (theSelectedTabPage != null)
                    currentSelectedDataSourceTabPageIdx = this._connectivityTopTabControl.SelectedIndex;
            }
        }


        /// <summary>
        /// Handles the event triggered before a 'Select' is performed on the Connectivity Tree
        /// </summary>
        void ConnectivityTreeViewControlBeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            try
            {
                // check if user is about to change slected node
                if (previousFullPath != e.Node.FullPath)
                {
                    // keep tracking if the RightPane control has to refresh/reload
                    selectedNodeChanged = true;

                    //Check to see if the user was configuring a DataSource
                    if (((TrafodionTabPage)_connectivityTopTabControl.SelectedTab).Equals(_theConfigurationTabPage) && (this._connectivityAreaConfigurationUserControl != null && this._connectivityAreaConfigurationUserControl.datasourceValuesChanged()))
                    {
                        
                        DialogResult dr = MessageBox.Show(Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessage,
                                Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessageCaption, 
                                MessageBoxButtons.YesNoCancel);
                        if (dr == DialogResult.Yes)
                        {
                            //Apply Changes then continue
                            _connectivityAreaConfigurationUserControl.ApplyChanges();
                            e.Cancel = false;
                        }
                        else if (dr == DialogResult.No)
                        {
                            //Discard changes and continue
                            e.Cancel = false;
                        }
                        else
                        {
                            //Cancel navigation
                            e.Cancel = true;
                        }
                    }
                    else if (((TrafodionTabPage)_connectivityTopTabControl.SelectedTab).Equals(_theMonitoringTabPage))
                    {
                        if (this._connectivityAreaMonitoringUserControl != null && this._connectivityAreaMonitoringUserControl.datasourceValuesChanged())
                        {
                            DialogResult dr = MessageBox.Show(Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessage,
                                Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessageCaption, 
                                MessageBoxButtons.YesNoCancel);
                            if (dr == DialogResult.Yes)
                            {
                                //Apply Changes then continue
                                this._connectivityAreaMonitoringUserControl.ApplyTracingChanges();
                                e.Cancel = false;
                            }
                            else if (dr == DialogResult.No)
                            {
                                //Discard changes and continue
                                this._connectivityAreaMonitoringUserControl.CancelTracingChanges();
                                e.Cancel = false;
                            }
                            else
                            {
                                //Cancel navigation
                                e.Cancel = true;
                            }
                        }

                        if (this._connectivityAreaMonitoringUserControl != null
                            && this._connectivityAreaMonitoringUserControl.PermissionsTabPage != null
                            && this._connectivityAreaMonitoringUserControl.PermissionsTabPage.PermissionsUserControl != null
                            && this._connectivityAreaMonitoringUserControl.PermissionsTabPage.PermissionsUserControl.HasPrivilegesChanged)
                        {
                            if (!this._connectivityAreaMonitoringUserControl.PermissionsTabPage.PermissionsUserControl.HasNewGranteeAdded)
                            {
                                DialogResult dr = MessageBox.Show(Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessage,
                                Trafodion.Manager.Properties.Resources.UnappliedChangesConfirmMessageCaption, 
                                MessageBoxButtons.YesNoCancel);
                                if (dr == DialogResult.Yes)
                                {
                                    //Apply Changes then continue
                                    this._connectivityAreaMonitoringUserControl.PermissionsTabPage.PermissionsUserControl.ApplyChanges();
                                    e.Cancel = false;
                                }
                                else if (dr == DialogResult.No)
                                {
                                    //Discard changes and continue
                                    this._connectivityAreaMonitoringUserControl.PermissionsTabPage.PermissionsUserControl.ResetChanges();
                                    e.Cancel = false;
                                }
                                else
                                {
                                    //Cancel navigation
                                    e.Cancel = true;
                                }
                            }
                            else
                            {
                                DialogResult dr = MessageBox.Show(Trafodion.Manager.Properties.Resources.UnappliedChangesOfAddNewGranteeConfirmMessage,
                                Trafodion.Manager.Properties.Resources.UnappliedChangesOfAddNewGranteeConfirmCaption, 
                                MessageBoxButtons.YesNo);
                                if (dr == DialogResult.Yes)
                                {
                                    //Discard changes and continue
                                    this._connectivityAreaMonitoringUserControl.PermissionsTabPage.PermissionsUserControl.ResetChanges();
                                    e.Cancel = false;
                                }
                                else
                                {
                                    //Cancel navigation
                                    e.Cancel = true;
                                }
                            }
                        }

                    }
                }
                else
                {
                    // keep tracking if the RightPane control has to refresh/reload
                    selectedNodeChanged = false;
                }
            }
            catch (Exception ex)
            {
            }
        }

        /// <summary>
        /// Handles clicks to the  Connectivity Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        void ConnectivityTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            if (Trafodion.Manager.Framework.Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            previousFullPath = aNavigationTreeNode.FullPath;
            theMostRecentWorkObject = aNavigationTreeNode;

            ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
                        
            if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded) ||
                (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder))
            {
                // only showing the system tab
                if (this._theMonitoringTabPage != null)
                {
                    if (this._connectivityTopTabControl.TabPages.Contains(this._theMonitoringTabPage) )
                    {
                        this._connectivityTopTabControl.TabPages.Remove(this._theMonitoringTabPage);
                    }
                }

                if (this._theConfigurationTabPage != null)
                {
                    if (this._connectivityTopTabControl.TabPages.Contains(this._theConfigurationTabPage))
                    {
                        this._connectivityTopTabControl.TabPages.Remove(this._theConfigurationTabPage);
                    }
                }

                if (this._theErrorTabPage != null)
                {
                    if (this._connectivityTopTabControl.TabPages.Contains(this._theErrorTabPage))
                    {
                        this._connectivityTopTabControl.TabPages.Remove(this._theErrorTabPage);
                    }
                }
                
                //this._connectivityTopTabControl.TabPages.Clear();

                // Systems Tab control
                if (this._connectivityAreaSystemsUserControl == null)
                {
                    _connectivityAreaSystemsUserControl = new ConnectivityAreaSystemsUserControl(ConnectivityNavigationControl.ConnectivityTreeView);
                }

                // check if the system tab is visiable
                if (!this._connectivityTopTabControl.TabPages.Contains(this._theSystemTabPage))
                {
                    AddNewTabControl(this._theSystemTabPage, this._connectivityAreaSystemsUserControl);
                }

                _connectivityAreaSystemsUserControl.ConnectivityTreeControlSelected(aNavigationTreeNode);

            }
            else if ((aNavigationTreeNode is ConnectivitySystemFolder) ||
                (aNavigationTreeNode is NDCSServicesFolder) ||
                (aNavigationTreeNode is NDCSServiceFolder) ||
                (aNavigationTreeNode is DataSourcesFolder))
            {
                // Show Monitoring Tab control only
                // remove all the tab pages from the Top Tab Control collection
                // only showing the Monitoring tab
                // remove the systems tab and configuration tab
                if (this._connectivityTopTabControl.TabPages.Contains(this._theSystemTabPage))
                    this._connectivityTopTabControl.TabPages.Remove(this._theSystemTabPage);

                if (this._connectivityTopTabControl.TabPages.Contains(this._theConfigurationTabPage))
                    this._connectivityTopTabControl.TabPages.Remove(this._theConfigurationTabPage);


                if (aNavigationTreeNode is ConnectivitySystemFolder 
                    && (((ConnectivitySystemFolder)aNavigationTreeNode).TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded) 
                    && (!((ConnectivitySystemFolder)aNavigationTreeNode).ConnectivitySystem.ConnectivitySupported))
                {
                    // This system is NOT supported, show Error page here

                    // check if the monitoring tab is visiable
                    if (_theMonitoringTabPage != null)
                    {
                        if (this._connectivityTopTabControl.TabPages.Contains(this._theMonitoringTabPage))
                            this._connectivityTopTabControl.TabPages.Remove(this._theMonitoringTabPage);
                    }

                    if (null == this._connectivityAreaErrorUserControl)
                        this._connectivityAreaErrorUserControl = new ConnectivityAreaErrorUserControl(ConnectivityNavigationControl.ConnectivityTreeView);

                    if (!this._connectivityTopTabControl.TabPages.Contains(this._theErrorTabPage))
                    {
                        AddNewTabControl(this._theErrorTabPage, this._connectivityAreaErrorUserControl);
                    }

                    this._connectivityAreaErrorUserControl.ConnectivityTreeControlSelected(aNavigationTreeNode);

                }
                else
                {
                    // Is user clicking the same Active System Folder node or not?

                    if (this._connectivityTopTabControl.TabPages.Contains(this._theErrorTabPage))
                        this._connectivityTopTabControl.TabPages.Remove(this._theErrorTabPage);

                    if (this._connectivityAreaMonitoringUserControl == null)
                    {
                        this._connectivityAreaMonitoringUserControl = new ConnectivityAreaMonitoringUserControl(ConnectivityNavigationControl.ConnectivityTreeView);

                        if (this._theMonitoringTabPage != null && this._connectivityTopTabControl.TabPages.Contains(this._theMonitoringTabPage))
                        {
                            this._connectivityTopTabControl.TabPages.Remove(this._theMonitoringTabPage);
                        }
                        _theMonitoringTabPage = new TrafodionTabPage(Properties.Resources.MonitoringTabName);
                    }

                    // check if the monitoring tab is visiable
                    if (!this._connectivityTopTabControl.TabPages.Contains(this._theMonitoringTabPage))
                    {
                        // make it visible
                        AddNewTabControl(this._theMonitoringTabPage, this._connectivityAreaMonitoringUserControl);
                    }

                    // Monitoring Tab control
                    this._connectivityAreaMonitoringUserControl.ActiveTreeNode = aNavigationTreeNode;
                    this._connectivityAreaMonitoringUserControl.ConnectivityTreeControlSelected(aNavigationTreeNode);
                }
            }
            else if (aNavigationTreeNode is DataSourceFolder)
            {
                DataSourceFolder dataSourceFolder = aNavigationTreeNode as DataSourceFolder;

                // Both Monitoring and Configuration Tab controls
                // remove all the tab pages from the Top Tab Control collection
                // only showing the Monitoring tab
                // remove the systems tab and configuration tab
                if (this._connectivityTopTabControl.TabPages.Contains(this._theErrorTabPage))
                {
                    this._connectivityTopTabControl.TabPages.Remove(this._theErrorTabPage);
                }

                if (this._connectivityTopTabControl.TabPages.Contains(this._theSystemTabPage))
                {
                    this._connectivityTopTabControl.TabPages.Remove(this._theSystemTabPage);
                }

                if (this._connectivityAreaMonitoringUserControl == null)
                {
                    this._connectivityAreaMonitoringUserControl = new ConnectivityAreaMonitoringUserControl(ConnectivityNavigationControl.ConnectivityTreeView);
                }

                // check if the monitoring tab is visiable
                if (!this._connectivityTopTabControl.TabPages.Contains(this._theMonitoringTabPage))
                {
                    // make it visible
                    AddNewTabControl(this._theMonitoringTabPage, this._connectivityAreaMonitoringUserControl);
                }

                this._connectivityAreaMonitoringUserControl.ActiveTreeNode = aNavigationTreeNode;

                // check if the user is log on as services user account
                // Do not let all users see config tab of datasources
                if (this._connectivityAreaConfigurationUserControl == null)
                {
                    this._connectivityAreaConfigurationUserControl = new ConnectivityAreaDatasourceConfigurationUserControl(ConnectivityNavigationControl.ConnectivityTreeView);//new ConnectivityAreaConfigurationUserControl(ConnectivityNavigationControl.ConnectivityTreeView);
                }

                // check if the configuration tab is visiable
                if (!this._connectivityTopTabControl.TabPages.Contains(this._theConfigurationTabPage))
                {
                    // make it visible
                    AddNewTabControl(this._theConfigurationTabPage, this._connectivityAreaConfigurationUserControl);
                }

                if (dataSourceFolder.EditThisDS)
                {
                    dataSourceFolder.EditThisDS = false;
                    currentSelectedDataSourceTabPageIdx = this._connectivityTopTabControl.TabPages.IndexOf(this._theConfigurationTabPage);
                }
                this._connectivityAreaConfigurationUserControl.ActiveTreeNode = aNavigationTreeNode;

                int oldIndex = _connectivityTopTabControl.SelectedIndex;

                if (currentSelectedDataSourceTabPageIdx > -1)
                {
                    this._connectivityTopTabControl.SelectTab(currentSelectedDataSourceTabPageIdx);
                }
                if (_connectivityTopTabControl.SelectedIndex == oldIndex)
                {
                    //Perform updates to both Configuration and Monitoring tabs (improved GUI response)
                    // Change depending on what type of node is selected                
                    if (this._connectivityTopTabControl.SelectedTab.Equals(this._theConfigurationTabPage))
                    {
                        this._connectivityAreaConfigurationUserControl.TabSelected();
                    }
                    else if (this._connectivityTopTabControl.SelectedTab.Equals(this._theMonitoringTabPage))
                    {
                        this._connectivityAreaMonitoringUserControl.TabSelected();
                    }
                }
            }
        }

        private void FilterChanged(NavigationTreeNameFilter aNameFilter)
        {
            theNameFilter = aNameFilter;
            if ((theMostRecentWorkObject != null) && (theMostRecentWorkObject is TreeNode))
            {
                ConnectivityTreeControlSelected(theMostRecentWorkObject as NavigationTreeNode);
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
            //TheConnectivityAreaTabControl.TabPages.Add(theTabPage);
            AddNewTabControl(theTabPage);

        }

        /// <summary>
        /// Helper method to add a control into a specified tab into the right pane
        /// </summary>
        /// <param name="theTabPage"></param>
        /// <param name="aUserControl"></param>
        private void AddNewTabControl(TrafodionTabPage theTabPage, UserControl aUserControl)
        {
            aUserControl.Dock = DockStyle.Fill;
            aUserControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aUserControl);
            //TheConnectivityAreaTabControl.TabPages.Add(theTabPage);
            AddNewTabControl(theTabPage);
        }

        private void AddNewTabControl(TrafodionTabPage aTabPage)
        {
            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            this._connectivityTopTabControl.TabPages.Add(aTabPage);
        }


        /// <summary>
        /// Helper method to add controls into a tab into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aUserControl"></param>
        private void AddToTabControl(UserControl aUserControl, string aTabText)
        {
            TrafodionTabControl topTabControl = new TrafodionTabControl();

            // Create the tab page with the user control dock filled
            TrafodionTabPage theTabPage = new TrafodionTabPage(aTabText);
            aUserControl.Dock = DockStyle.Fill;
            theTabPage.Controls.Add(aUserControl);
            topTabControl.TabPages.Add(theTabPage);

             this._connectivityTopTabControl = topTabControl;
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TrafodionTabPage aTabPage)
        {
            TrafodionTabControl aConnectivityTabControl = new TrafodionTabControl();
            aConnectivityTabControl.TabPages.Add(aTabPage);
             _connectivityTopTabControl = aConnectivityTabControl;
        }


    }
}
