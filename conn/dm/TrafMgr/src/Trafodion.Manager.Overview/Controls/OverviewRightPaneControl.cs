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
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.OverviewArea.Controls.Tree;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// User control for the right pane,  no need to clone the top Right Panel
    /// </summary>
    public partial class OverviewRightPaneControl : UserControl
    {
        #region Fields

        private OverviewNavigationControl _overviewNavigationControl;
        private OverviewTreeView _overviewTreeView;

        private FavoritesTreeView _overviewFavoritesTreeView;

        private NavigationTreeView.SelectedHandler theOverviewTreeControlSelectedHandler = null;
        private TreeViewCancelEventHandler theOverviewTreeControlBeforeSelectedHandler = null; //To capture 'Before Select'
        private NavigationTreeNameFilter.ChangedHandler theFilterChangedHandler = null;

        private MySystemsUserControl _theMySystemsUserControl = new MySystemsUserControl();
        private TrafodionTabPage _theSystemsTabPage = new TrafodionTabPage("My Systems");

        private FixSystemUserControl _theFixSystemUserControl = null;


        //private DynamicSystemSummaryUserControl _theDynamicSystemSummaryUserControl = null;

        //private TrafodionTabPage _theDynamicTabPage = new TrafodionTabPage("Dynamic");

        //private OverallSystemSummaryUserControl _theOverallSystemSummaryUserControl = null;
        //private SystemMonitorUserControl _theSystemMonitorUserControl = null;
        private TrafodionTabPage _theSystemMonitorTabPage = new TrafodionTabPage(Properties.Resources.SystemMonitor);

        //TrafodionTabControl _theMonitoringTabCtl = new TrafodionTabControl();
        //TrafodionTabPage _theRealTimeMonitoringTabPage = new TrafodionTabPage("Real Time");
        //TrafodionTabPage _theTimelineMonitoringTabPage = new TrafodionTabPage("Timeline");

        //private AlertsUserControl _alertsUserControl;
        //private SystemAlertsUserControl _systemAlertsUserControl1;
        private TrafodionTabPage _theSystemAlertsTabPage = new TrafodionTabPage(Properties.Resources.Alerts);        

        private OffenderWorkloadCanvas _offenderWorkloadCanvas;
        private TrafodionTabPage _theSystemOffenderTabPage = new TrafodionTabPage(Properties.Resources.SystemOffender);

        private SQLOffenderCanvas _sqlOffenderCanvas;
        private TrafodionTabPage _theSQLOffenderTabPage = new TrafodionTabPage(Properties.Resources.SQLOffender);

        private WorkloadsUserControl _workloadsMonitorUserControl;
        private TrafodionTabPage _theWorkloadsMonitorTabPage = new TrafodionTabPage(Properties.Resources.MonitorWorkload);

        private TriageSpaceUserControl _theTriageSpaceUserControl;
        private TrafodionTabPage _theTriageSpaceTabPage = new TrafodionTabPage(Properties.Resources.TriageSpace);

        private DcsSessionsMonitor _theDcsSessionMonitor;
        private TrafodionTabPage _theDcsSessionMonitorTabPage = new TrafodionTabPage("DCS Sessions");
        
        private TrafodionTabControl _theMySystemTabControl = new TrafodionTabControl();

        private static int currentTopLevelTabIndex = 0;

        //Bad idea?
        private NavigationTreeNode _currentNode = null;


        private bool selectedNodeChanged = false;

        private object theMostRecentWorkObject = null;
        private NavigationTreeNameFilter theNameFilter = new NavigationTreeNameFilter();

        private bool _needToInitAlertWidgetData = true;
        private bool _needToInitSystemOffenderWidgetData = true;
        private bool _needToInitSQLOffenderWidgetData = true;
        private bool _needToInitWorkloadMonitorWidgetData = true;
        private bool _needToInitTriageSpaceWidgetData = true;

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
        public OverviewTreeView OverviewTreeView
        {
            get { return _overviewTreeView; }
            set { _overviewTreeView = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public FavoritesTreeView OverviewFavoritesTreeView
        //public ConnectivityFavoritesTreeView ConnectivityFavoritesTreeView
        {
            get { return _overviewFavoritesTreeView; }
            set { _overviewFavoritesTreeView = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        public OverviewNavigationControl OverviewNavigationControl
        {
            get { return _overviewNavigationControl; }
            set
            {
                _overviewNavigationControl = value;

                _overviewTreeView = (_overviewNavigationControl == null) ? null : _overviewNavigationControl.OverviewTreeView;
                _overviewFavoritesTreeView = (_overviewNavigationControl == null) ? null : (FavoritesTreeView)_overviewNavigationControl.OverviewFavoritesTreeView;

                {
                    bool treeAvailable = (_overviewTreeView != null);
                    bool favoritestreeAvailable = (_overviewFavoritesTreeView != null);
                    if (treeAvailable)
                    {
                        if (theOverviewTreeControlSelectedHandler == null)
                        {
                            theOverviewTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(OverviewTreeControlSelected);
                        }

                        OverviewTreeView.Selected += theOverviewTreeControlSelectedHandler;

                        OverviewTreeView.RefreshRequestedEvent += OverviewTreeView_RefreshRequestedEvent;
                        if (theFilterChangedHandler == null)
                        {
                            theFilterChangedHandler = new NavigationTreeNameFilter.ChangedHandler(FilterChanged);
                        }

                        NavigationTreeNameFilter.Changed += theFilterChangedHandler;                        
                    }

                }

            }
        }

        void OverviewTreeView_RefreshRequestedEvent(object sender, EventArgs e)
        {
            CleanupControls();
            OverviewTreeControlSelected(OverviewTreeView.SelectedNode as NavigationTreeNode);
        }

        #endregion Properties

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectivityNavigationControl"></param>
        public OverviewRightPaneControl(OverviewNavigationControl aOverviewNavigationControl)
        {
            InitializeComponent();
            _theMySystemTabControl.Dock = DockStyle.Fill;
            _theMySystemTabControl.Location = new Point(0, 0);
            _theMySystemTabControl.Name = "MySystemTabcontrol";
            _theMySystemTabControl.Padding = new Point(10, 5);
            _theMySystemTabControl.SelectedIndex = 0;
            _theMySystemTabControl.TabIndex = 0;
            
            this.OverviewNavigationControl = aOverviewNavigationControl;
            _overviewTopTabControl.SelectedIndexChanged += new EventHandler(_overviewTopTabControl_SelectedIndexChanged);
            ConnectionDefinition.Changed += ConnectionDefinition_Changed;
        }

        ~OverviewRightPaneControl()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }

        void _overviewTopTabControl_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_overviewTopTabControl.SelectedIndex != -1)
            {
                switch (_overviewTopTabControl.SelectedTab.Text)
                {
/*                    case "System Monitor":
                        if (Utilities.IsInstalledLiveFeedDriver())
                        {
                            _theSystemMonitorTabPage.Controls.Clear();

                            if (_theSystemMonitorUserControl == null)
                            {
                                _theSystemMonitorUserControl = new SystemMonitorUserControl(_currentNode.TheConnectionDefinition);
                                _theSystemMonitorUserControl.CreateControl();
                                _theSystemMonitorUserControl.Dock = DockStyle.Fill;
                                _theSystemMonitorUserControl.BackColor = Color.WhiteSmoke;
                                _theSystemMonitorUserControl.OnSystemMonitorShowOffenderClickImpl += _theSystemMonitorUserControl_OnSystemMonitorShowOffenderClickImpl;
                                _theSystemMonitorUserControl.OnSystemMonitorShowSQLOffenderClickImpl += _theSystemMonitorUserControl_OnSystemMonitorShowSQLOffenderClickImpl;
                                _theSystemMonitorTabPage.Controls.Clear();
                                _theSystemMonitorTabPage.Controls.Add(_theSystemMonitorUserControl);
                            }
                            else if (_theSystemMonitorUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition && _currentNode.TheConnectionDefinition != null)
                            {
                                _theSystemMonitorUserControl.OnSystemMonitorShowOffenderClickImpl -= _theSystemMonitorUserControl_OnSystemMonitorShowOffenderClickImpl;
                                _theSystemMonitorUserControl.OnSystemMonitorShowSQLOffenderClickImpl -= _theSystemMonitorUserControl_OnSystemMonitorShowSQLOffenderClickImpl;
                                _theSystemMonitorUserControl.Dispose();
                                _theSystemMonitorUserControl = new SystemMonitorUserControl(_currentNode.TheConnectionDefinition);
                                _theSystemMonitorUserControl.CreateControl();
                                _theSystemMonitorUserControl.Dock = DockStyle.Fill;
                                _theSystemMonitorUserControl.BackColor = Color.WhiteSmoke;
                                _theSystemMonitorUserControl.OnSystemMonitorShowOffenderClickImpl += _theSystemMonitorUserControl_OnSystemMonitorShowOffenderClickImpl;
                                _theSystemMonitorUserControl.OnSystemMonitorShowSQLOffenderClickImpl += _theSystemMonitorUserControl_OnSystemMonitorShowSQLOffenderClickImpl;
                                _theSystemMonitorTabPage.Controls.Clear();
                                _theSystemMonitorTabPage.Controls.Add(_theSystemMonitorUserControl);
                            }
                            else if (!this._theSystemMonitorTabPage.Controls.Contains(_theSystemMonitorUserControl))
                            {
                                _theSystemMonitorTabPage.Controls.Add(_theSystemMonitorUserControl);
                            }

                            ////Add realtime system monitoring control
                            //if (_theOverallSystemSummaryUserControl == null)
                            //{
                            //    _theOverallSystemSummaryUserControl = new OverallSystemSummaryUserControl(_currentNode.TheConnectionDefinition);
                            //    _theOverallSystemSummaryUserControl.CreateControl();
                            //    _theOverallSystemSummaryUserControl.Dock = DockStyle.Fill;
                            //    _theOverallSystemSummaryUserControl.BackColor = Color.WhiteSmoke;
                            //    _theOverallSystemSummaryUserControl.OnShowOffenderClickImpl += theOverallSystemSummaryUserControl_OnShowOffenderClickImpl;
                            //    _theRealTimeMonitoringTabPage.Controls.Clear();
                            //    _theRealTimeMonitoringTabPage.Controls.Add(_theOverallSystemSummaryUserControl);

                            //}
                            //else if (_theOverallSystemSummaryUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition && _currentNode.TheConnectionDefinition!=null)
                            //{
                            //    _theOverallSystemSummaryUserControl.OnShowOffenderClickImpl -= theOverallSystemSummaryUserControl_OnShowOffenderClickImpl;
                            //    _theOverallSystemSummaryUserControl.Dispose();
                            //    _theOverallSystemSummaryUserControl = new OverallSystemSummaryUserControl(_currentNode.TheConnectionDefinition);
                            //    _theOverallSystemSummaryUserControl.CreateControl();
                            //    _theOverallSystemSummaryUserControl.Dock = DockStyle.Fill;
                            //    _theOverallSystemSummaryUserControl.BackColor = Color.WhiteSmoke;
                            //    _theOverallSystemSummaryUserControl.OnShowOffenderClickImpl += theOverallSystemSummaryUserControl_OnShowOffenderClickImpl;
                            //    _theRealTimeMonitoringTabPage.Controls.Clear();
                            //    _theRealTimeMonitoringTabPage.Controls.Add(_theOverallSystemSummaryUserControl);
                            //}
                            //else if (!this._theRealTimeMonitoringTabPage.Controls.Contains(_theOverallSystemSummaryUserControl))
                            //{
                            //    _theRealTimeMonitoringTabPage.Controls.Add(_theOverallSystemSummaryUserControl);
                            //}

                            //if (!this._theMonitoringTabCtl.TabPages.Contains(_theRealTimeMonitoringTabPage))
                            //{
                            //    _theMonitoringTabCtl.TabPages.Add(_theRealTimeMonitoringTabPage);
                            //}

                            ////Add history system monitoring control
                            //if (_theDynamicSystemSummaryUserControl == null)
                            //{
                            //    if (_theOverallSystemSummaryUserControl != null)
                            //    {
                            //        _theDynamicSystemSummaryUserControl = new DynamicSystemSummaryUserControl(_currentNode.TheConnectionDefinition);
                            //        //_theOverallSystemSummaryUserControl.DynamicSummaryControl = _theDynamicSystemSummaryUserControl;
                            //    }
                            //    else
                            //        return;
                            //    _theDynamicSystemSummaryUserControl.CreateControl();
                            //    _theDynamicSystemSummaryUserControl.Dock = DockStyle.Fill;
                            //    _theDynamicSystemSummaryUserControl.BackColor = Color.WhiteSmoke;
                            //    _theTimelineMonitoringTabPage.Controls.Clear();
                            //    _theTimelineMonitoringTabPage.Controls.Add(_theDynamicSystemSummaryUserControl);
                            //}
                            //else if (_theDynamicSystemSummaryUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition)
                            //{
                            //    _theDynamicSystemSummaryUserControl.Dispose();

                            //    if (_theOverallSystemSummaryUserControl != null)
                            //    {
                            //        _theDynamicSystemSummaryUserControl = new DynamicSystemSummaryUserControl(_currentNode.TheConnectionDefinition);
                            //        //_theOverallSystemSummaryUserControl.DynamicSummaryControl = _theDynamicSystemSummaryUserControl;
                            //    }
                            //    else
                            //        return;

                            //    _theDynamicSystemSummaryUserControl.CreateControl();
                            //    _theDynamicSystemSummaryUserControl.Dock = DockStyle.Fill;
                            //    _theDynamicSystemSummaryUserControl.BackColor = Color.WhiteSmoke;
                            //    _theTimelineMonitoringTabPage.Controls.Clear();
                            //    _theTimelineMonitoringTabPage.Controls.Add(_theDynamicSystemSummaryUserControl);

                            //}
                            //else if (!this._theTimelineMonitoringTabPage.Controls.Contains(_theDynamicSystemSummaryUserControl))
                            //{
                            //    _theTimelineMonitoringTabPage.Controls.Add(_theDynamicSystemSummaryUserControl);
                            //}

                            //if (!this._theMonitoringTabCtl.TabPages.Contains(_theTimelineMonitoringTabPage))
                            //{
                            //    _theMonitoringTabCtl.TabPages.Add(_theTimelineMonitoringTabPage);
                            //}
                            //_theMonitoringTabCtl.Dock = DockStyle.Fill;
                            //_theSystemMonitorTabPage.Controls.Add(_theMonitoringTabCtl);
                        }
                        else
                        {
                            _theSystemMonitorTabPage.Controls.Clear();
                            Trafodion.Manager.Framework.Controls.TrafodionLabel labelError=new TrafodionLabel();
                            labelError.Location = new System.Drawing.Point(0, 0);
                            labelError.Dock = System.Windows.Forms.DockStyle.Top;
                            labelError.ForeColor = Color.Red;
                            labelError.Text = string.Format(Trafodion.Manager.Properties.Resources.ErrorMustInstalledLiveFeedDriver, Utilities.Is64bitOS() ? "64 bit" : "32 bit");
                            _theSystemMonitorTabPage.Controls.Add(labelError);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theSystemMonitorTabPage))
                        {
                            AddNewTabControl(_theSystemMonitorTabPage);
                        }
                        break;
                    case "Alerts":
                       
                        if (_currentNode.TheConnectionDefinition == null)
                            return;

                        if (Utilities.IsInstalledLiveFeedDriver())
                        {
                            if (_currentNode.TheConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
                            {
                                if (null == this._alertsUserControl)
                                {
                                    InitAlertsUserControl();
                                }
                                else if (_alertsUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition || 
                                    (_alertsUserControl.InitializationState == 1 && _alertsUserControl.InitializationInProgress != true) 
                                    ||_needToInitAlertWidgetData)
                                {
                                    _alertsUserControl.Dispose();
                                    InitAlertsUserControl();
                                }

                                if (!this._theSystemAlertsTabPage.Controls.Contains(_alertsUserControl))
                                {
                                    _theSystemAlertsTabPage.Controls.Add(_alertsUserControl);
                                } 
                            }
                            else
                            {

                                if (this._alertsUserControl != null)
                                {
                                    if (this._alertsUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition 
                                        || this._alertsUserControl.InitializationState != 1
                                        || _needToInitAlertWidgetData)
                                    {
                                        _alertsUserControl.Dispose();
                                        _alertsUserControl = null;
                                        InitAlertsUserControl();
                                    }
                                    if (!this._theSystemAlertsTabPage.Controls.Contains(_alertsUserControl))
                                    {
                                        _theSystemAlertsTabPage.Controls.Add(_alertsUserControl);
                                    }
                                }
                                else
                                {
                                    InitAlertsUserControl();
                                }
                                if (!this._theSystemAlertsTabPage.Controls.Contains(_alertsUserControl))
                                {
                                    _theSystemAlertsTabPage.Controls.Add(_alertsUserControl);
                                }

                            }
                        }
                        else
                        {
                            _theSystemAlertsTabPage.Controls.Clear();
                            Trafodion.Manager.Framework.Controls.TrafodionLabel labelError = new TrafodionLabel();
                            labelError.Location = new System.Drawing.Point(0, 0);
                            labelError.Dock = System.Windows.Forms.DockStyle.Top;
                            labelError.ForeColor = Color.Red;
                            labelError.Text = string.Format(Trafodion.Manager.Properties.Resources.ErrorMustInstalledLiveFeedDriver, Utilities.Is64bitOS() ? "64 bit" : "32 bit");
                            _theSystemAlertsTabPage.Controls.Add(labelError);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theSystemAlertsTabPage))
                        {
                            AddNewTabControl(_theSystemAlertsTabPage);
                        }
                        break;

                    case "System Offender":
                        if (_currentNode.TheConnectionDefinition == null)
                            return;

                        if (_offenderWorkloadCanvas == null)
                        {
                            _offenderWorkloadCanvas = new OffenderWorkloadCanvas(_currentNode.TheConnectionDefinition);
                            _offenderWorkloadCanvas.CreateControl();
                            _offenderWorkloadCanvas.Dock = DockStyle.Fill;
                            _offenderWorkloadCanvas.BackColor = Color.WhiteSmoke;
                            _theSystemOffenderTabPage.Controls.Clear();
                            _theSystemOffenderTabPage.Controls.Add(_offenderWorkloadCanvas);

                        }
                        else if (_offenderWorkloadCanvas.ConnectionDefinition != _currentNode.TheConnectionDefinition 
                            || _needToInitSystemOffenderWidgetData)
                        {
                            _needToInitSystemOffenderWidgetData = false;
                            _offenderWorkloadCanvas.Dispose();
                            _offenderWorkloadCanvas = new OffenderWorkloadCanvas(_currentNode.TheConnectionDefinition);
                            _offenderWorkloadCanvas.CreateControl();
                            _offenderWorkloadCanvas.Dock = DockStyle.Fill;
                            _offenderWorkloadCanvas.BackColor = Color.WhiteSmoke;
                            _theSystemOffenderTabPage.Controls.Clear();
                            _theSystemOffenderTabPage.Controls.Add(_offenderWorkloadCanvas);
                        }

                        else if (!this._theSystemOffenderTabPage.Controls.Contains(_offenderWorkloadCanvas))
                        {
                            _theSystemOffenderTabPage.Controls.Add(_offenderWorkloadCanvas);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theSystemOffenderTabPage))
                        {
                            AddNewTabControl(_theSystemOffenderTabPage);
                        }
                        break;

                    case "SQL Offender":
                        if (_currentNode.TheConnectionDefinition == null)
                            return;

                        if (_sqlOffenderCanvas == null)
                        {
                            _sqlOffenderCanvas = new SQLOffenderCanvas(_currentNode.TheConnectionDefinition);
                            _sqlOffenderCanvas.CreateControl();
                            _sqlOffenderCanvas.Dock = DockStyle.Fill;
                            _sqlOffenderCanvas.BackColor = Color.WhiteSmoke;
                            _theSQLOffenderTabPage.Controls.Clear();
                            _theSQLOffenderTabPage.Controls.Add(_sqlOffenderCanvas);

                        }
                        else if (_sqlOffenderCanvas.ConnectionDefinition != _currentNode.TheConnectionDefinition 
                            || _needToInitSQLOffenderWidgetData)
                        {
                            _needToInitSQLOffenderWidgetData = false;
                            _sqlOffenderCanvas.Dispose();
                            _sqlOffenderCanvas = new SQLOffenderCanvas(_currentNode.TheConnectionDefinition);
                            _sqlOffenderCanvas.CreateControl();
                            _sqlOffenderCanvas.Dock = DockStyle.Fill;
                            _sqlOffenderCanvas.BackColor = Color.WhiteSmoke;
                            _theSQLOffenderTabPage.Controls.Clear();
                            _theSQLOffenderTabPage.Controls.Add(_sqlOffenderCanvas);
                        }

                        else if (!this._theSQLOffenderTabPage.Controls.Contains(_sqlOffenderCanvas))
                        {
                            _theSQLOffenderTabPage.Controls.Add(_sqlOffenderCanvas);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theSQLOffenderTabPage))
                        {
                            AddNewTabControl(_theSQLOffenderTabPage);
                        }
                        break;


                    case "Workload Monitor":
                        if (_currentNode.TheConnectionDefinition == null)
                            return;
                        if (_workloadsMonitorUserControl == null)
                        {
                            _workloadsMonitorUserControl = new WorkloadsUserControl(_currentNode.TheConnectionDefinition);
                            _workloadsMonitorUserControl.CreateControl();
                            _workloadsMonitorUserControl.Dock = DockStyle.Fill;
                            _workloadsMonitorUserControl.BackColor = Color.WhiteSmoke;

                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.LoadQueriesToTriageSpaceEvent += loadQueriesToTriageSpace;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.GetSessionEvent += GetSession;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.TheMonitorWorkloadWidget.StartDataProvider();
                            _theWorkloadsMonitorTabPage.Controls.Clear();
                            _theWorkloadsMonitorTabPage.Controls.Add(_workloadsMonitorUserControl);
                        }
                        else if (_workloadsMonitorUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition 
                            || _needToInitWorkloadMonitorWidgetData)
                        {
                            _needToInitWorkloadMonitorWidgetData = false;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.LoadQueriesToTriageSpaceEvent -= loadQueriesToTriageSpace;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.GetSessionEvent -= GetSession;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.TheMonitorWorkloadWidget.DataProvider.Stop();

                            _workloadsMonitorUserControl.Dispose();
                            _workloadsMonitorUserControl = new WorkloadsUserControl(_currentNode.TheConnectionDefinition);
                            _workloadsMonitorUserControl.CreateControl();
                            _workloadsMonitorUserControl.Dock = DockStyle.Fill;
                            _workloadsMonitorUserControl.BackColor = Color.WhiteSmoke;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.LoadQueriesToTriageSpaceEvent += loadQueriesToTriageSpace;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.GetSessionEvent += GetSession;
                            _workloadsMonitorUserControl.MonitorWorkloadCanvas.TheMonitorWorkloadWidget.StartDataProvider();
                            _theWorkloadsMonitorTabPage.Controls.Clear();
                            _theWorkloadsMonitorTabPage.Controls.Add(_workloadsMonitorUserControl);
                        }
                        else if (!this._theWorkloadsMonitorTabPage.Controls.Contains(_workloadsMonitorUserControl))
                        {
                            _theWorkloadsMonitorTabPage.Controls.Add(_workloadsMonitorUserControl);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theWorkloadsMonitorTabPage))
                        {
                            AddNewTabControl(this._theWorkloadsMonitorTabPage);
                        }
                        break;*/

                    case "DCS Sessions":
                        if (_currentNode.TheConnectionDefinition == null)
                            return;
                        if (_theDcsSessionMonitor== null)
                        {
                            _theDcsSessionMonitor = new DcsSessionsMonitor(_currentNode.TheConnectionDefinition);
                            _theDcsSessionMonitor.CreateControl();
                            _theDcsSessionMonitor.Dock = DockStyle.Fill;
                            _theDcsSessionMonitor.BackColor = Color.WhiteSmoke;
                            _theDcsSessionMonitorTabPage.Controls.Clear();
                            _theDcsSessionMonitorTabPage.Controls.Add(_theDcsSessionMonitor);
                        }
                        else if (_theDcsSessionMonitor.ConnectionDefn != _currentNode.TheConnectionDefinition)
                        {
                            _theDcsSessionMonitor.Dispose();
                            _theDcsSessionMonitor = new DcsSessionsMonitor(_currentNode.TheConnectionDefinition);
                            _theDcsSessionMonitor.CreateControl();
                            _theDcsSessionMonitor.Dock = DockStyle.Fill;
                            _theDcsSessionMonitor.BackColor = Color.WhiteSmoke;
                            _theDcsSessionMonitorTabPage.Controls.Clear();
                            _theDcsSessionMonitorTabPage.Controls.Add(_theDcsSessionMonitor);
                        }
                        else if (!this._theDcsSessionMonitorTabPage.Controls.Contains(_theDcsSessionMonitor))
                        {
                            _theDcsSessionMonitorTabPage.Controls.Add(_theDcsSessionMonitor);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theDcsSessionMonitorTabPage))
                        {
                            AddNewTabControl(this._theDcsSessionMonitorTabPage);
                        }
                        break;
                    case "Triage Space":
                        if (_currentNode.TheConnectionDefinition == null)
                            return;
                        if (_theTriageSpaceUserControl == null)
                        {
                            _theTriageSpaceUserControl = new TriageSpaceUserControl(_currentNode.TheConnectionDefinition);
                            _theTriageSpaceUserControl.CreateControl();
                            _theTriageSpaceUserControl.Dock = DockStyle.Fill;
                            _theTriageSpaceUserControl.BackColor = Color.WhiteSmoke;
                            _theTriageSpaceTabPage.Controls.Clear();
                            _theTriageSpaceTabPage.Controls.Add(_theTriageSpaceUserControl);
                        }
                        else if (_theTriageSpaceUserControl.ConnectionDefn != _currentNode.TheConnectionDefinition 
                            || _needToInitTriageSpaceWidgetData)
                        {
                            _needToInitTriageSpaceWidgetData = false;
                            _theTriageSpaceUserControl.Dispose();
                            _theTriageSpaceUserControl = new TriageSpaceUserControl(_currentNode.TheConnectionDefinition);
                            _theTriageSpaceUserControl.CreateControl();
                            _theTriageSpaceUserControl.Dock = DockStyle.Fill;
                            _theTriageSpaceUserControl.BackColor = Color.WhiteSmoke;
                            _theTriageSpaceTabPage.Controls.Clear();
                            _theTriageSpaceTabPage.Controls.Add(_theTriageSpaceUserControl);
                        }
                        else if (!this._theTriageSpaceTabPage.Controls.Contains(_theTriageSpaceUserControl))
                        {
                            _theTriageSpaceTabPage.Controls.Add(_theTriageSpaceUserControl);
                        }

                        if (!this._overviewTopTabControl.TabPages.Contains(_theTriageSpaceTabPage))
                        {
                            AddNewTabControl(this._theTriageSpaceTabPage);
                        }
                        break;

                    default:
                        break;
                }
            }
        }

        /// <summary>
        /// Handles clicks to the Overview Tree
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        void OverviewTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            //Set the previous path so we know when the user selects a new node
            _currentNode = aNavigationTreeNode;
            if (_overviewTopTabControl.TabCount > 0 && _overviewTopTabControl.SelectedIndex >= 0)
            {
                currentTopLevelTabIndex = _overviewTopTabControl.SelectedIndex;
            }
            //this._overviewTopTabControl.SelectedIndexChanged -= _overviewTopTabControl_SelectedIndexChanged;
            //this._overviewTopTabControl.TabPages.Clear();
            //this._overviewTopTabControl.SelectedIndexChanged += _overviewTopTabControl_SelectedIndexChanged;

            if (aNavigationTreeNode is OverviewSystemFolder)
            {
                if (aNavigationTreeNode.TheConnectionDefinition != null &&
                    aNavigationTreeNode.TheConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded &&
                    aNavigationTreeNode.TheConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
                {

                    if (_theFixSystemUserControl == null)
                    {
                        _theFixSystemUserControl = new FixSystemUserControl(aNavigationTreeNode.TheConnectionDefinition);
                    }
                    else
                    {
                        _theFixSystemUserControl.TheConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
                    }

                    //Fix for CR 7677. Exception in System Monitor when you switch systems
                    this._overviewTopTabControl.SelectedIndexChanged -= _overviewTopTabControl_SelectedIndexChanged;
                    this._overviewTopTabControl.TabPages.Clear();
                    this._overviewTopTabControl.SelectedIndexChanged += _overviewTopTabControl_SelectedIndexChanged;

                    this._overviewRightPanePanel.Controls.Clear();
                    this._overviewRightPanePanel.Controls.Add(this._overviewTopTabControl);
                    AddNewTabControl(this._theFixSystemUserControl, "Connect");
                }
                else if (aNavigationTreeNode.TheConnectionDefinition != null && 
                         aNavigationTreeNode.TheConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
                {
                    // LiveFeed Test Succeeded.
                    this._overviewRightPanePanel.Controls.Clear();
                    this._overviewTopTabControl.TabPages.Clear();
                    this._overviewRightPanePanel.Controls.Add(this._overviewTopTabControl);

/*                    #region MonitoringTab

                    if (!this._overviewTopTabControl.TabPages.Contains(_theSystemMonitorTabPage))
                    {
                        this._theSystemMonitorTabPage.Controls.Clear();
                        AddNewTabControl(this._theSystemMonitorTabPage);
                    }
                    else
                    {
                        this._theSystemMonitorTabPage.Controls.Clear();
                    }

                    #endregion

                    //#region DynamicTab
                    //if (!this._overviewTopTabControl.TabPages.Contains(_theDynamicTabPage))
                    //{
                    //    this._theDynamicTabPage.Controls.Clear();
                    //    AddNewTabControl(this._theDynamicTabPage);
                    //}
                    //else
                    //{
                    //    this._theDynamicTabPage.Controls.Clear();
                    //}
                    //#endregion DynamcTab

                    #region Add Alerts Tab
                    if (!this._overviewTopTabControl.TabPages.Contains(_theSystemAlertsTabPage))
                    {
                        _theSystemAlertsTabPage.Controls.Clear();
                        AddNewTabControl(this._theSystemAlertsTabPage);
                    }
                    else
                    {
                        _theSystemAlertsTabPage.Controls.Clear();
                    }

                    #endregion  Add Alerts Tab*/
                }
                else if (aNavigationTreeNode.TheConnectionDefinition != null)

                //if system connected.
                //if (aNavigationTreeNode.TheConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
                {
                    // LiveFeed Test Succeeded.
                    this._overviewRightPanePanel.Controls.Clear();
                    this._overviewTopTabControl.TabPages.Clear();
                    this._overviewRightPanePanel.Controls.Add(this._overviewTopTabControl);

/*                    #region MonitoringTab

                    if (!this._overviewTopTabControl.TabPages.Contains(_theSystemMonitorTabPage))
                    {
                        this._theSystemMonitorTabPage.Controls.Clear();
                        AddNewTabControl(this._theSystemMonitorTabPage);
                    }
                    else
                    {
                        this._theSystemMonitorTabPage.Controls.Clear();
                    }

                    #endregion

                    //#region DynamicTab
                    //if (!this._overviewTopTabControl.TabPages.Contains(_theDynamicTabPage))
                    //{
                    //    this._theDynamicTabPage.Controls.Clear();
                    //    AddNewTabControl(this._theDynamicTabPage);
                    //}
                    //else
                    //{
                    //    this._theDynamicTabPage.Controls.Clear();
                    //}
                    //#endregion DynamcTab

                    #region Add Alerts Tab
                    if (!this._overviewTopTabControl.TabPages.Contains(_theSystemAlertsTabPage))
                    {
                        _theSystemAlertsTabPage.Controls.Clear();
                        AddNewTabControl(this._theSystemAlertsTabPage);
                    }
                    else
                    {
                        _theSystemAlertsTabPage.Controls.Clear();
                    }

                    #endregion  Add Alerts Tab

                    #region SystemOffender Tab
                    
                    if (!this._overviewTopTabControl.TabPages.Contains(_theSystemOffenderTabPage))
                    {
                        _theSystemOffenderTabPage.Controls.Clear();
                        AddNewTabControl(this._theSystemOffenderTabPage);
                    }
                    else
                    {
                        _theSystemOffenderTabPage.Controls.Clear();
                    }

                    #endregion

                    #region SQLOffender Tab
                    if (aNavigationTreeNode.TheConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
                    {
                        if (!this._overviewTopTabControl.TabPages.Contains(_theSQLOffenderTabPage))
                        {
                            _theSQLOffenderTabPage.Controls.Clear();
                            AddNewTabControl(this._theSQLOffenderTabPage);
                        }
                        else
                        {
                            _theSQLOffenderTabPage.Controls.Clear();
                        }

                    }
                    #endregion

                    #region Workload Monitor                    

                    if (!this._overviewTopTabControl.TabPages.Contains(_theWorkloadsMonitorTabPage))
                    {
                        _theWorkloadsMonitorTabPage.Controls.Clear();
                        AddNewTabControl(this._theWorkloadsMonitorTabPage);
                    }
                    else
                    {
                        _theWorkloadsMonitorTabPage.Controls.Clear();
                    }

                    #endregion
                    */

                    #region Triage Space Tab
                    //if (_theTriageSpaceUserControl == null)
                    //{
                    //    _theTriageSpaceUserControl = new TriageSpaceUserControl(aNavigationTreeNode.TheConnectionDefinition);
                    //}
                    //else if (_theTriageSpaceUserControl.ConnectionDefn != aNavigationTreeNode.TheConnectionDefinition)
                    //{
                    //    _theTriageSpaceUserControl.Dispose();
                    //    _theTriageSpaceUserControl = new TriageSpaceUserControl(aNavigationTreeNode.TheConnectionDefinition);
                    //}
                    //_theTriageSpaceUserControl.CreateControl();
                    //_theTriageSpaceUserControl.Dock = DockStyle.Fill;
                    //if (!this._overviewTopTabControl.TabPages.Contains(_theTriageSpaceTabPage))
                    //{
                    //    _theTriageSpaceTabPage.Controls.Clear();
                    //    AddNewTabControl(this._theTriageSpaceTabPage, this._theTriageSpaceUserControl);
                    //}

                    if (!this._overviewTopTabControl.TabPages.Contains(_theDcsSessionMonitorTabPage))
                    {
                        _theDcsSessionMonitorTabPage.Controls.Clear();
                        AddNewTabControl(this._theDcsSessionMonitorTabPage);
                    }
                    else
                    {
                        _theDcsSessionMonitorTabPage.Controls.Clear();
                    }

                    if (!this._overviewTopTabControl.TabPages.Contains(_theTriageSpaceTabPage))
                    {
                        _theTriageSpaceTabPage.Controls.Clear();
                        AddNewTabControl(this._theTriageSpaceTabPage);
                    }
                    else
                    {
                        _theTriageSpaceTabPage.Controls.Clear();
                    }
                    #endregion
                    
                }
                else //The system has not been tested (Connected)
                {
                    CleanupControls();
                }
            }
            else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
            {
                // The active systems folder is selected.
                // In the future we can do a multi-system aggregation
                _theMySystemsUserControl.Populate();

                //Make sure the active systems tab is visible.
                //if (!this._overviewTopTabControl.TabPages.Contains(this._theSystemsTabPage))
                //    AddNewTabControl(this._theSystemsTabPage, _theMySystemsUserControl);
                if (!this._theMySystemTabControl.TabPages.Contains(this._theSystemsTabPage))
                {
                    this._theMySystemsUserControl.Dock = DockStyle.Fill;
                    this._theSystemsTabPage.Controls.Add(this._theMySystemsUserControl);
                    this._theMySystemTabControl.TabPages.Add(this._theSystemsTabPage);
                }

                this._overviewRightPanePanel.Controls.Clear();
                this._overviewRightPanePanel.Controls.Add(this._theMySystemTabControl);
            }

            if (currentTopLevelTabIndex >= 0 && currentTopLevelTabIndex < _overviewTopTabControl.TabCount)
            {
                //Somehow, you need to reset it and set it again in order to make the selectedIndexChanged event works...
                _overviewTopTabControl.SelectedIndex = -1;
                _overviewTopTabControl.SelectedIndex = currentTopLevelTabIndex;
            }
        }

        void CleanupControls()
        {
            // Clear system offender widget
            if (_offenderWorkloadCanvas != null)
            {
                _offenderWorkloadCanvas.Dispose();
                _offenderWorkloadCanvas = null;
            }
            if (_sqlOffenderCanvas != null)
            {
                _sqlOffenderCanvas.Dispose();
                _sqlOffenderCanvas = null;

            }
            // Clear workload monitor widget
            if (_workloadsMonitorUserControl != null)
            {
                if (_workloadsMonitorUserControl.MonitorWorkloadCanvas != null)
                {
                    _workloadsMonitorUserControl.MonitorWorkloadCanvas.LoadQueriesToTriageSpaceEvent -= loadQueriesToTriageSpace;
                    _workloadsMonitorUserControl.MonitorWorkloadCanvas.GetSessionEvent -= GetSession;
                }

                _workloadsMonitorUserControl.Dispose();
                _workloadsMonitorUserControl = null;
            }

            // Clear triage space widget
            if (_theTriageSpaceUserControl != null)
            {
                _theTriageSpaceUserControl.Dispose();
                _theTriageSpaceUserControl = null;
            }

            if (_theDcsSessionMonitor != null)
            {
                _theDcsSessionMonitor.Dispose();
                _theDcsSessionMonitor = null;
            }
            
            // Remove system offender tab page from tab control
            if (this._overviewTopTabControl.TabPages.Contains(_theSystemOffenderTabPage))
            {
                this._overviewTopTabControl.TabPages.Remove(_theSystemOffenderTabPage);
            }

            // Remove workload monitor tab page from tab control
            if (this._overviewTopTabControl.TabPages.Contains(_theWorkloadsMonitorTabPage))
            {
                this._overviewTopTabControl.TabPages.Remove(_theWorkloadsMonitorTabPage);
            }

            // Remove triage space tab page from tab control
            if (this._overviewTopTabControl.TabPages.Contains(_theTriageSpaceTabPage))
            {
                this._overviewTopTabControl.TabPages.Remove(_theTriageSpaceTabPage);
            }

            // Remove triage space tab page from tab control
            if (this._overviewTopTabControl.TabPages.Contains(_theDcsSessionMonitorTabPage))
            {
                this._overviewTopTabControl.TabPages.Remove(_theDcsSessionMonitorTabPage);
            }

        }

        void _theSystemMonitorUserControl_OnSystemMonitorShowOffenderClickImpl(ShowOffenderEventArgs args)
        {
            if (_theSystemOffenderTabPage != null && _overviewTopTabControl.TabPages.Contains(this._theSystemOffenderTabPage))
            {
                _overviewTopTabControl.SelectedTab = _theSystemOffenderTabPage;
                if (_offenderWorkloadCanvas != null)
                {
                    _offenderWorkloadCanvas.HandleMouseClickCPU(args);
                }
            }
        }

        void _theSystemMonitorUserControl_OnSystemMonitorShowSQLOffenderClickImpl()
        {
            if (_theSystemOffenderTabPage != null && _overviewTopTabControl.TabPages.Contains(this._theSQLOffenderTabPage))
            {
                _overviewTopTabControl.SelectedTab = _theSQLOffenderTabPage;
            }
        }

        private void FilterChanged(NavigationTreeNameFilter aNameFilter)
        {
            theNameFilter = aNameFilter;
            if ((theMostRecentWorkObject != null) && (theMostRecentWorkObject is TreeNode))
            {
                OverviewTreeControlSelected(theMostRecentWorkObject as NavigationTreeNode);
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
            this._overviewTopTabControl.TabPages.Add(aTabPage);
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

            this._overviewTopTabControl = topTabControl;
        }

        /// <summary>
        /// Helper method to add a tab page into the right pane, replacing what was there previously
        /// </summary>
        /// <param name="aTabPage"></param>
        private void AddToTabControl(TrafodionTabPage aTabPage)
        {
            TrafodionTabControl aConnectivityTabControl = new TrafodionTabControl();
            aConnectivityTabControl.TabPages.Add(aTabPage);
            _overviewTopTabControl = aConnectivityTabControl;
        }

        private void loadQueriesToTriageSpace()
        {
            TrafodionIGrid theGrid = this._workloadsMonitorUserControl.MonitorWorkloadCanvas.MonitorWorkloadIGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                //Focus on Triage Tab
                _overviewTopTabControl.SelectedTab = _theTriageSpaceTabPage;
                _theTriageSpaceUserControl.loadSelectedLiveViewQueries(theGrid);

            }
        }

        public void loadSnapshotQueriesToTriageSpace(object sender, EventArgs e)
        {
            //TrafodionIGrid theGrid = this._theDynamicSystemSummaryUserControl.SnapshotControl.SnapshotIGrid;
            //if (theGrid.SelectedRowIndexes.Count > 0)
            //{
            //    //Focus on Triage Tab
            //    _overviewTopTabControl.SelectedTab = _theTriageSpaceTabPage;
            //    _theTriageSpaceUserControl.loadSelectedLiveViewQueries(theGrid);

            //}

        }

        private void GetSession()
        {
            TrafodionIGrid theGrid = this._workloadsMonitorUserControl.MonitorWorkloadCanvas.MonitorWorkloadIGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                _overviewTopTabControl.SelectedTab = _theTriageSpaceTabPage;
                _theTriageSpaceUserControl.GetSession(theGrid);
            }
        }
        
        private void InitAlertsUserControl() 
        {
/*            this._alertsUserControl = new AlertsUserControl(_currentNode.TheConnectionDefinition);
            _alertsUserControl.CreateControl();
            _alertsUserControl.Dock = DockStyle.Fill;
            _alertsUserControl.BackColor = Color.WhiteSmoke;
            _theSystemAlertsTabPage.Controls.Clear();
            _theSystemAlertsTabPage.Controls.Add(_alertsUserControl);
            _needToInitAlertWidgetData = false;*/
        }

        private void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if ((aReason == ConnectionDefinition.Reason.Disconnected) || (aReason == ConnectionDefinition.Reason.Removed))
            {
                _needToInitAlertWidgetData = true;
                _needToInitSystemOffenderWidgetData = true;
                _needToInitSQLOffenderWidgetData = true;
                _needToInitWorkloadMonitorWidgetData = true;
                _needToInitTriageSpaceWidgetData = true;

                if (_currentNode != null) 
                {
/*                    if (_alertsUserControl != null)
                    {
                        _alertsUserControl.Dispose();
                        _alertsUserControl = null;
                    }
                    if (_theSystemMonitorUserControl != null) 
                    {
                        _theSystemMonitorUserControl.Dispose();
                        _theSystemMonitorUserControl = null;
                    }*/
                    CleanupControls();
                }
            }
        }
    }
}
