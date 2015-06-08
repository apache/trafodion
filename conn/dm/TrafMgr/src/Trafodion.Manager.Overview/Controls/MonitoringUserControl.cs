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
#define INC_MONITOR_WORKLOAD

using System;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.OverviewArea.Controls.Tree;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// User control for the right pane,  no need to clone the top Right Panel
    /// </summary>
    public partial class MonitoringUserControl : UserControl, ICloneToWindow
    {
        #region Fields

        /// <summary>
        /// The clonable implementer or not
        /// </summary>
        private ICloneToWindow theICloneToWindow = null;

        private OverviewNavigationControl _overviewNavigationControl;
        private OverviewTreeView _overviewTreeView;

        private SystemAlertsUserControl _systemAlertsUserControl;
        private OffenderWorkloadCanvas _offenderWorkloadCanvas;
        private TabbedWorkloadUserControlWrapper _workloadsUserControl;

        private ConnectionDefinition _connectionDefinition;

        private NavigationTreeView.SelectedHandler theOverviewTreeControlSelectedHandler = null;
        private TreeViewCancelEventHandler theOverviewTreeControlBeforeSelectedHandler = null; //To capture 'Before Select'
        private NavigationTreeNameFilter.ChangedHandler theFilterChangedHandler = null;

        private TrafodionTabPage _theSystemAlertsTabPage = new TrafodionTabPage(Properties.Resources.Alerts);
        private TrafodionTabPage _theSystemOffenderTabPage = new TrafodionTabPage(Properties.Resources.SystemOffender);
        private TrafodionTabPage _theMonitorWorkloadTabPage = new TrafodionTabPage(Properties.Resources.MonitorWorkload);
        private TrafodionTabPage _theAdvancedSystemMonitorTabPage = new TrafodionTabPage(Properties.Resources.SystemMonitor);

        private TrafodionStatusLightUserControl _alarmLight;
        private TrafodionStatusLightUserControl.ChangingHandler alarmLightClickHandler = null;

        private static int currentTopLevelTabIndex = 0;

        private NavigationTreeNode _currentNode = null;

        private bool selectedNodeChanged = false;

        private object theMostRecentWorkObject = null;
        private NavigationTreeNameFilter theNameFilter = new NavigationTreeNameFilter();        


        #endregion Fields


        # region Properties

        public int CurrentTopLevelTabIndex
        {
            get { return _monitoringTopTabControl.SelectedIndex; }
            set
            {
                MonitoringUserControl.currentTopLevelTabIndex = value;
                _monitoringTopTabControl.SelectedIndex = value;
            }
        }

        public OverviewTreeView OverviewTreeView
        {
            get { return _overviewTreeView; }
            set { _overviewTreeView = value; }
        }

        public SystemMonitorControl SystemMonitorControl
        {
            get { return _systemMonitorControl; }
            set { _systemMonitorControl = value; }
        }

        public NavigationTreeNameFilter TheNameFilter
        {
            get { return theNameFilter; }
            set { theNameFilter = value; }
        }

        public NavigationTreeNode CurrentNode
        {
            get { return _currentNode; }
            set { _currentNode = value; }
        }

        /// <summary>
        /// Stores Connection Definition for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return this._connectionDefinition; }
            set { this._connectionDefinition = value; }
        }

        #endregion Properties


        public MonitoringUserControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectivityNavigationControl"></param>
        public MonitoringUserControl(OverviewTreeView aOverviewTreeview)
            : this()
        {
            this._overviewTreeView = aOverviewTreeview;
        }

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectivityNavigationControl"></param>
        public MonitoringUserControl(MonitoringUserControl aMonitorControl)
            : this()
        {
            this.OverviewTreeControlSelected(aMonitorControl.CurrentNode);
        }

        /// <summary>
        /// Handles clicks to the Overview Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        public void OverviewTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            this._currentNode = aNavigationTreeNode;
            this._connectionDefinition = aNavigationTreeNode.TheConnectionDefinition;            

            SetupNSM();            

            SetupTabPages();            

            if (currentTopLevelTabIndex >= 0 && currentTopLevelTabIndex < _monitoringTopTabControl.TabCount)
            {
                _monitoringTopTabControl.SelectedIndex = currentTopLevelTabIndex;
            }
        }

        private void SetupNSM()
        {
            _systemMonitorControl.ConnectionDefn = this._connectionDefinition;
            _systemMonitorControl.Activate();
        }

        private void SetupTabPages()
        {
            if (this._connectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
            {
                this.splitContainer_oneGuiSplitContainer.Panel2Collapsed = false;

                //Check if we've already added the light from another instance of the Widget
                if (this._workloadsUserControl == null)
                {
                    this._workloadsUserControl = new TabbedWorkloadUserControlWrapper(this._connectionDefinition);
                }
                else
                {
                    this._workloadsUserControl.TheConnectionDefinition = _connectionDefinition;
                }
                this.splitContainer_oneGuiSplitContainer.Panel2.Controls.Clear();
                _monitoringTopTabControl = _workloadsUserControl.TheTabControl;
                this.splitContainer_oneGuiSplitContainer.Panel2.Controls.Add(_monitoringTopTabControl);
                if (null != _systemMonitorControl)
                {
                    _offenderWorkloadCanvas = _workloadsUserControl.OffenderWorkloadCanvas;
                    this._systemMonitorControl.MouseClickCPU += new SystemMonitorControl.ChangingHandler(_systemMonitorControl_MouseClickCPU);
                }
                if ((null == this._systemAlertsUserControl) || (!this._connectionDefinition.Equals(this._systemAlertsUserControl.ConnectionDefn)))
                {
                    this._systemAlertsUserControl = new SystemAlertsUserControl(this._connectionDefinition);
                    // Force Handle creation by reading it. 
                    IntPtr hnd;
                    if (!_systemAlertsUserControl.IsHandleCreated || _systemAlertsUserControl.Handle == IntPtr.Zero)
                        hnd = _systemAlertsUserControl.Handle; 
                }
                if (_monitoringTopTabControl.TabPages.Contains(_theSystemAlertsTabPage))
                {
                    _theSystemAlertsTabPage.Controls.Clear();
                    _monitoringTopTabControl.TabPages.Remove(_theSystemAlertsTabPage);
                }
                AddNewTabControl(_theSystemAlertsTabPage, _systemAlertsUserControl);

                this.splitContainer_oneGuiSplitContainer.Panel2.Controls.Add(_monitoringTopTabControl);                    
            }
            else //The system has not been tested (Connected)
            {
                if (_systemAlertsUserControl != null)
                {
                    _systemAlertsUserControl.Dispose();
                    _systemAlertsUserControl = null;
                }

                if (_offenderWorkloadCanvas != null)
                {
                    _offenderWorkloadCanvas.Dispose();
                    _offenderWorkloadCanvas = null;
                }

                if (_workloadsUserControl != null)
                {
                    _workloadsUserControl.Dispose();
                    _workloadsUserControl = null;
                }
                this.splitContainer_oneGuiSplitContainer.Panel2Collapsed = true;
            }
        }

        void SystemStatusUserControl_Click(object sender, EventArgs e)
        {

            if (this._monitoringTopTabControl.TabPages.Contains(this._theSystemOffenderTabPage))
            {
                this._monitoringTopTabControl.SelectedTab = this._theSystemOffenderTabPage;
            }
            else
            {
                //Cleanup click reference -- tab got lost
                this._systemMonitorControl.MouseClickCPU -= new SystemMonitorControl.ChangingHandler(_systemMonitorControl_MouseClickCPU);
            }
        }

        void _systemMonitorControl_MouseClickCPU(object sender, MetricBarClickArgs args)
        {
            if (this._monitoringTopTabControl.TabPages.Contains(this._theSystemOffenderTabPage))
            {
                this._monitoringTopTabControl.SelectedTab = this._theSystemOffenderTabPage;
            }
            //if (_offenderWorkloadCanvas != null)
            //{
            //    _offenderWorkloadCanvas.HandleMouseClickCPU(args);
            //}
            else
            {
                //Cleanup click reference -- tab got lost
                this._systemMonitorControl.MouseClickCPU -= new SystemMonitorControl.ChangingHandler(_systemMonitorControl_MouseClickCPU);
            }
        }

        void _alarmLight_MouseClickLight(object sender, EventArgs args)
        {
            if (this._monitoringTopTabControl.TabPages.Contains(this._theSystemAlertsTabPage))
            {
                this._monitoringTopTabControl.SelectedTab = this._theSystemAlertsTabPage;
            }

            else if (_alarmLight != null)
            {
                if (this._connectionDefinition != null && this._connectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                {
                    if (MessageBox.Show(Properties.Resources.LogonConfirmationForAlerts, Properties.Resources.Confirm,
                        MessageBoxButtons.OKCancel, MessageBoxIcon.Question) == DialogResult.OK)
                    {
                        ConnectionDefinitionDialog cdd = new ConnectionDefinitionDialog(false);
                        cdd.Edit(this._connectionDefinition);
                    }
                }
                //
                //Cleanup click reference -- tab got lost
                //_alarmLight.MouseClickLight -= new TrafodionStatusLightUserControl.ChangingHandler(_alarmLight_MouseClickLight);
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
            AddNewTabControl(theTabPage);
        }

        private void AddNewTabControl(TrafodionTabPage aTabPage)
        {
            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            this._monitoringTopTabControl.TabPages.Add(aTabPage);
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

            this._monitoringTopTabControl = topTabControl;
        }

        private TrafodionPanel theTopPanel;
        private TrafodionTextBox theTopPanelUpperLabel;

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TrafodionTabPage aTabPage)
        {
            TrafodionTabControl aConnectivityTabControl = new TrafodionTabControl();
            aConnectivityTabControl.TabPages.Add(aTabPage);
            _monitoringTopTabControl = aConnectivityTabControl;
        }



        /// <summary>
        /// Read only property that supplies a suitable base title for the managed window.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return "System Monitoring";
            }
        }

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            MonitoringUserControl MonitorControl = new MonitoringUserControl(this);
            return MonitorControl;
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {

            if (null != _alarmLight)
                _alarmLight.MouseClickLight -= new TrafodionStatusLightUserControl.ChangingHandler(_alarmLight_MouseClickLight);

            //this.oneGuiRealTimeBarGraph1.MouseClickBar -= new TrafodionRealTimeBarGraph.ChangingHandler(oneGuiRealTimeBarGraph1_MouseClickBar);           
            if (null != this._offenderWorkloadCanvas)
            {
                this._offenderWorkloadCanvas.Dispose();
            }
            if (null != this._systemAlertsUserControl)
            {
                this._systemAlertsUserControl.Dispose();
            }

            if (null != this._workloadsUserControl)
            {
                this._workloadsUserControl.Dispose();
            }
            if (this._systemMonitorControl != null)
            {
                _systemMonitorControl.MouseClickCPU -= _systemMonitorControl_MouseClickCPU;
                _systemMonitorControl.Deactivate();
            }
        }
    }
}
