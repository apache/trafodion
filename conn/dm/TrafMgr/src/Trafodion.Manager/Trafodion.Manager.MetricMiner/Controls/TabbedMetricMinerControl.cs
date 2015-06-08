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
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Queries;
using Trafodion.Manager.MetricMiner.Controls.Tree;
using Trafodion.Manager.UniversalWidget;
using System.IO;

namespace Trafodion.Manager.MetricMiner.Controls
{
    /// <summary>
    /// The main control for the metric miner
    /// </summary>
    public partial class TabbedMetricMinerControl : UserControl, IMainToolBarConsumer
    {
        Dictionary<string, List<TabPage>> reportsBeingDisplayed = new Dictionary<string, List<TabPage>>();
        public static LibraryManager TheLibraryManager = LibraryManager.Instance;
        string AdhocReportTabName = "Ad-hoc Report";
        private bool expandedState = false;

        public TabbedMetricMinerControl()
        {
            InitializeComponent();
            _theMetricMinerNavigator.TheMetricMinerControl = this;
            _theMMTreeView.TheTree.NodeMouseClick += new TreeNodeMouseClickEventHandler(TheTree_NodeMouseClick);
            _theMMTreeView.TheTree.TreePopulateImpl = TreePopulateImpl;

            //Clear the tab pages
            _theReportsTab.TabPages.Clear();
            _theReportsTab.MouseDoubleClick += tabPage_MouseDoubleClick;
            _theMetricMinerNavigator.ReportSelectedImpl = ReportSelectedImpl;

        }

        public bool IsExpanded
        {
            get { return expandedState; }
            set { expandedState = value; }
        }

        void TheTree_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                NavigationTreeNode reportNode = e.Node as NavigationTreeNode;
                if (reportNode != null)
                {
                    selectNode(reportNode);
                }
            }
            else if (e.Button == MouseButtons.Right)
            {
                _theMMTreeView.TheTree.SelectedNode = e.Node;
            }
        }


        void selectNode(Trafodion.Manager.Framework.Navigation.NavigationTreeNode aTreeNode)
        {
            if (aTreeNode != null)
            {
                if (aTreeNode is AdhocReportNode)
                {
                    AdhocReportNode adhocReportNode = aTreeNode as AdhocReportNode;
                    bool addNew = shouldAddNew(AdhocReportTabName);
                    if (addNew)
                    {
                        AddAdhocTab(adhocReportNode.Config);
                    }
                    else
                    {
                        SelectTab(AdhocReportTabName);
                    }
                }
                else if (aTreeNode is MetricMinerWidgetNode)
                {
                    MetricMinerWidgetNode widgetNode = aTreeNode as MetricMinerWidgetNode;
                    //string reportKey = widgetNode.Config.Name;
                    string reportKey = widgetNode.Config.ReportID;
                    bool addNew = shouldAddNew(widgetNode.Config);
                    if (addNew)
                    {
                        //in case of user has edit the report in current opened tab, than re-read report from disk.
                        widgetNode.RefreshNode();
                        //Not sure if we should show the query by default
                        //AddTab(widgetNode, aTreeNode is AdhocReportNode);
                        AddTab(widgetNode, true);
                    }
                    else
                    {
                        SelectTab(reportKey);
                    }
                    //we only register delete event only once
                    widgetNode.OnNodeDeleted -= widgetNode_OnNodeDeleted;
                    widgetNode.OnNodeDeleted += widgetNode_OnNodeDeleted;
                }
            }
        }

        /// <summary>
        /// we close tab window if user delete node from left side navigation tree.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void widgetNode_OnNodeDeleted(object sender, EventArgs e)
        {
            MetricMinerWidgetNode mmNode = sender as MetricMinerWidgetNode;
            if (mmNode != null)
            {
                TabPage deletedTabPage = null;
                string reportID = mmNode.Config.ReportID;
                //Remove the tab page from the hash
                if (reportsBeingDisplayed.ContainsKey(reportID))
                {
                    reportsBeingDisplayed.Remove(reportID);

                    ArrayList deleteTabList = new ArrayList();
                    foreach (TabPage tabPage in _theReportsTab.TabPages)
                    {
                        if (tabPage.Text == AdhocReportTabName)
                        {
                            continue;
                        }
                        if (tabPage.Controls.Count > 0)
                        {
                            MetricMinerReportTabContent mmReportTabContent = tabPage.Controls[0] as MetricMinerReportTabContent;
                            if (mmReportTabContent.Config.ReportID == reportID)
                            {
                                _theMetricMinerNavigator.RemoveReportFromCombo(mmNode.Config, tabPage);
                                deletedTabPage = tabPage;
                                deleteTabList.Add(tabPage);
                            }
                        }
                    }
                    if (deletedTabPage != null)
                    {
                        foreach (TabPage tabPage in deleteTabList)
                        {
                            _theReportsTab.TabPages.Remove(tabPage);
                        }
                    }
                }
            }
        }

        private void TreePopulateImpl(MetricMinerWidgetsTreeView aTreeView)
        {
            selectNode(aTreeView.RootNode.AdHocNode);
        }

        public void SetReportFromHistory(HistorylogElement hle)
        {
            UniversalWidgetConfig config = WidgetRegistry.GetDefaultDBConfig();
            config.Name = "Ad-Hoc Report";
            config.Title = "Ad-Hoc Report";
            ((DatabaseDataProviderConfig)config.DataProviderConfig).SQLText = hle.SqlText;
            AddAdhocTab(config, false);
            //string reportKey = AdhocReportTabName;
            //if ((reportsBeingDisplayed.ContainsKey(reportKey)) && (reportsBeingDisplayed[reportKey].Count > 0))
            //{
            //    SelectTab(reportKey);
            //}
        }

        private bool shouldAddNew(string reportKey)
        {
            bool addNew = false;
            if ((reportsBeingDisplayed.ContainsKey(reportKey)) && (reportsBeingDisplayed[reportKey].Count > 0))
            {
                /*
                 * When users are refreshing the root node, the AdhocReportNode will be seleted by default after population
                 * But we should suppress the dialog to prompt users to create a new tab for AdhocReport, 
                 * because it's not a necessary action and it is not user-friendly
                 */
                if (!this._theMMTreeView.TheTree.RootNode.IsRefreshingRootNode)
                {
                    //ask the user if they want to see the same report or if they want to open another report
                    DialogResult result = MessageBox.Show(" An instance of " + reportKey + " report is already loaded. Do you want a new instance of that report?",
                        "Do you want a new report?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                    if (result == DialogResult.Yes)
                    {
                        addNew = true;
                    }
                }
                else
                {
                    this._theMMTreeView.TheTree.RootNode.IsRefreshingRootNode = false;
                }
            }
            else
            {
                addNew = true;
            }
            return addNew;
        }

        private bool shouldAddNew(UniversalWidgetConfig widgetConfig)
        {
            bool addNew = false;
            if ((reportsBeingDisplayed.ContainsKey(widgetConfig.ReportID)) && (reportsBeingDisplayed[widgetConfig.ReportID].Count > 0))
            {
                //ask the user if they want to see the same report or if they want to open another report
                DialogResult result = MessageBox.Show(" An instance of " + widgetConfig.Name + " report is already loaded. Do you want a new instance of that report?",
                    "Do you want a new report?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    addNew = true;
                }
            }
            else
            {
                addNew = true;
            }
            return addNew;
        }

        private void AddTab(MetricMinerWidgetNode widgetNode, bool showQuery)
        {
            AddTab(widgetNode.Config, showQuery,_theReportsTab.TabPages.Count);
            //TabPage tabPage = new TabPage(widgetNode.Text);
            //tabPage.Tag = widgetNode.Config;
            //MetricMinerReportTabContent tabContent = new MetricMinerReportTabContent(widgetNode.Config);
            //tabContent.ShowQuery(showQuery);
            //tabContent.Dock = DockStyle.Fill;

            ////Set the Drill down manager for the widget
            //tabContent.Widget.DataDisplayControl.DrillDownManager = _theDrillDownManager;
            //_theDrillDownManager.OnDrillDownRequested += new DrillDownManager.DrillDownRequested(_theDrillDownManager_OnDrillDownRequested);


            //tabPage.Controls.Add(tabContent);
            //_theReportsTab.TabPages.Add(tabPage);

            //List<TabPage> tabs = null;
            //if (reportsBeingDisplayed.ContainsKey(widgetNode.LongerDescription))
            //{
            //    tabs = reportsBeingDisplayed[widgetNode.LongerDescription];
            //}
            //else
            //{
            //    tabs = new List<TabPage>();
            //    reportsBeingDisplayed.Add(widgetNode.LongerDescription, tabs);
            //}
            //tabs.Add(tabPage);
        }

        private TabPage AddTab(
            UniversalWidgetConfig aConfig,
            bool showQuery,int insertIndex)
        {
            TabPage tabPage = new TabPage(WidgetRegistry.GetWidgetDisplayName(aConfig));
            tabPage.ToolTipText = ((aConfig.Title != null) && (aConfig.Title.Trim().Length > 0)) ? aConfig.Title : WidgetRegistry.GetWidgetDisplayName(aConfig);
            tabPage.Tag = aConfig;
            MetricMinerReportTabContent tabContent = new MetricMinerReportTabContent(aConfig);

            tabContent.OnUpdateReportTab += new MetricMinerReportTabContent.UpdateReportTab(tabContent_OnUpdateReportTab);
            tabContent.OnAfterSaveAs += new MetricMinerReportTabContent.AfterSaveAs(tabContent_OnAfterSaveAs);

            tabContent.ShowQuery(showQuery);
            tabContent.RestoreCatalogAndSchema();
            tabContent.Dock = DockStyle.Fill;
            tabContent.DoCloseImpl = this.DoCloseImpl;
            tabContent.ReportSuccessImpl = this.ReportSuccessImpl;
            tabContent.OnExpandRequested += tabContent_OnExpandRequested;


            //Tell the drill down manager how to display the next report
            tabContent.Widget.DataDisplayControl.DrillDownManager.ShowNewWidgetImpl = this.ShowNewWidgetImpl;
            
            tabPage.Controls.Add(tabContent);       
            _theReportsTab.TabPages.Insert(insertIndex,tabPage);           

            List<TabPage> tabs = null;
            //if (reportsBeingDisplayed.ContainsKey(aConfig.Name))
            if (reportsBeingDisplayed.ContainsKey(aConfig.ReportID))
            {
                //tabs = reportsBeingDisplayed[aConfig.Name];
                tabs = reportsBeingDisplayed[aConfig.ReportID];
            }
            else
            {
                tabs = new List<TabPage>();
                //reportsBeingDisplayed.Add(aConfig.Name, tabs);
                reportsBeingDisplayed.Add(aConfig.ReportID, tabs);
            }
            tabs.Add(tabPage);
            //SelectTab(aConfig.Name);
            SelectTab(aConfig.ReportID);
            return tabPage;
        }

        private void tabContent_OnAfterSaveAs(MetricMinerReportTabContent aMetricMinerReportTabContent, UniversalWidgetConfig aUniversalWidgetConfig)
        {
            int index=_theReportsTab.SelectedIndex;
            DoCloseImpl(aMetricMinerReportTabContent);
            AddTab(aUniversalWidgetConfig, true,index);            

            //NavigationTreeNode reportNode = _theMMTreeView.TheTree.getNodeForConfig(aUniversalWidgetConfig) as NavigationTreeNode;
            //if (reportNode != null)
            //{
            //    selectNode(reportNode);
            //}
            
        }

        //update tab name if user changes report name.
        void tabContent_OnUpdateReportTab(object sender, EventArgs e)
        {
            MetricMinerReportTabContent reportTab = sender as MetricMinerReportTabContent;
            UniversalWidgetConfig config = reportTab.Config;
            string reportName = config.Name;
            TreeNode reportNode = _theMMTreeView.TheTree.getNodeForConfig(config);
            if (reportNode != null)
            {
                ((MetricMinerWidgetNode)reportNode).Config = config;
            }

            if (reportsBeingDisplayed.ContainsKey(config.ReportID))
            {
                List<TabPage> tabPageList = reportsBeingDisplayed[config.ReportID];

                foreach (TabPage tabPage in tabPageList)
                {
                    if (tabPage.Text != WidgetRegistry.GetWidgetDisplayName(reportName))
                    {
                        tabPage.Text = WidgetRegistry.GetWidgetDisplayName(reportName);
                    }
                }
            }
        }

        void tabContent_OnExpandRequested(MetricMinerReportTabContent tabContent)
        {
            ExpandTabs();
        }

        void tabPage_MouseDoubleClick(object sender, MouseEventArgs e)
        {
            //ExpandTabs();
            popoutWinWindow(_theReportsTab.SelectedTab);
        }

        public void ExpandTabs()
        {
            _theNavAndReportsSplitPanel.Panel1Collapsed = !expandedState;

            foreach (TabPage tab in _theReportsTab.TabPages)
            {
                foreach (Control ctl in tab.Controls)
                {
                    if (ctl is MetricMinerReportTabContent)
                    {
                        ((MetricMinerReportTabContent)ctl).ExpandArea(!expandedState);
                        break;
                    }
                }
            }
            expandedState = !expandedState;
        }

        public void popoutWinWindow(TabPage aTabPage)
        {
            if (aTabPage != null)
            {
                MetricMinerReportTabContent tabcontent = aTabPage.Controls[0] as MetricMinerReportTabContent;
                if (tabcontent != null)
                {
                    RemoveTab(tabcontent);
                    WindowsManager.PutInWindow(new Size(800, 600), tabcontent, tabcontent.Config.Name, tabcontent.Config.DataProviderConfig.ConnectionDefinition);
                }
            }
        }

        public void CloseAllReports()
        {
            if (_theReportsTab.TabPages.Count > 0)
            {
                DialogResult result = MessageBox.Show("Are you sure you want to close all loaded reports?", "Close all reports?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {

                    foreach (TabPage tab in _theReportsTab.TabPages)
                    {
                        MetricMinerReportTabContent reportTabContent = GetTabContent(tab);
                        RemoveTab(reportTabContent);
                    }
                }
            }

            //We have to make sure that if we are in an expanded state when all reports are closed
            //we come back to the collapsed state so that the navigation window is displayed again
            if (expandedState)
            {
                ExpandTabs();
            }
        }

        private TabPage AddAdhocTab(UniversalWidgetConfig aConfig)
        {
            return AddAdhocTab(aConfig, true);
        }
        private TabPage AddAdhocTab(UniversalWidgetConfig aConfig, bool setDefaults)
        {
            DatabaseDataProviderConfig dbDataProviderConfig = aConfig.DataProviderConfig as DatabaseDataProviderConfig;

            //clear sql statement if setDefaults flag is set. This is the default behavior.            
            if (setDefaults)
            {
                dbDataProviderConfig.SQLText = "";
            }

            aConfig.LastExecutionTime = "";
            //clear column list
            dbDataProviderConfig.ColumnMappings = null;

            aConfig.Name = AdhocReportTabName;
            aConfig.Title = AdhocReportTabName;

            //turned on the chart by default
            aConfig.ShowChart = true;

            TabPage tabPage = new TabPage(AdhocReportTabName);
            tabPage.ToolTipText = ("Run adhoc queries by typing in the SQL text");
            tabPage.Tag = aConfig;
            AdhocReportTabContent tabContent = new AdhocReportTabContent(aConfig);
            tabContent.OnExpandRequested += tabContent_OnExpandRequested;
            tabContent.ShowQuery(true);
            tabContent.Dock = DockStyle.Fill;
            tabContent.DoCloseImpl = this.DoCloseImpl;
            tabContent.ReportSuccessImpl = this.ReportSuccessImpl;
            //the SQL query input area is always enabled for ad hoc report.
            tabContent.getQueryInputControl.ReadOnly = false;
            tabContent.QueryPlanDataDisplayControl.IsActiveTabPage = true;

            //Tell the drill down manager how to display the next report
            tabContent.Widget.DataDisplayControl.DrillDownManager.ShowNewWidgetImpl = this.ShowNewWidgetImpl;

            tabPage.Controls.Add(tabContent);
            _theReportsTab.TabPages.Add(tabPage);

            List<TabPage> tabs = null;
            if (reportsBeingDisplayed.ContainsKey(AdhocReportTabName))
            {
                tabs = reportsBeingDisplayed[AdhocReportTabName];
            }
            else
            {
                tabs = new List<TabPage>();
                reportsBeingDisplayed.Add(AdhocReportTabName, tabs);
            }
            tabs.Add(tabPage);
            _theReportsTab.SelectedTab = tabPage;
            return tabPage;
        }

        private void ReportSuccessImpl(MetricMinerReportTabContent aMetricMinerReportTabContent, TabPage aTabPage)
        {
            _theMetricMinerNavigator.AddReportToCombo(aMetricMinerReportTabContent.Widget.UniversalWidgetConfiguration, aTabPage);
        }

        //Implementation to close a report tab content
        private void DoCloseImpl(MetricMinerReportTabContent reportTabContent)
        {
            RemoveTab(reportTabContent);
        }

        private void RemoveTab(MetricMinerReportTabContent reportTabContent)
        {
            reportTabContent.OnExpandRequested -= tabContent_OnExpandRequested;
            reportTabContent.OnAfterSaveAs -= tabContent_OnAfterSaveAs;

            //Remove the reference of the ShowNewWidgetImpl from the drill down manager
            if (reportTabContent.Widget.DataDisplayControl.DrillDownManager != null)
            {
                reportTabContent.Widget.DataDisplayControl.DrillDownManager.ShowNewWidgetImpl = null;
            }

            TabPage aTabPage = reportTabContent.Parent as TabPage;
            if (aTabPage != null)
            {
                //string reportName = reportTabContent.Config.Name;
                //if (reportTabContent is AdhocReportTabContent)
                //{
                //    reportName = AdhocReportTabName;
                //}
                ////Remove the tab page from the hash
                //if (reportsBeingDisplayed.ContainsKey(reportName))
                //{
                //    List<TabPage> tabs = reportsBeingDisplayed[reportName];
                //    if (tabs.Contains(aTabPage))
                //    {
                //        tabs.Remove(aTabPage);
                //        _theMetricMinerNavigator.RemoveReportFromCombo(reportTabContent.Widget.UniversalWidgetConfiguration, aTabPage);
                //    }

                //    //TODO: We have to handle the following
                //    //1. If the report is running when the user tries to close the tab, we have to 
                //    //   stop the query.
                //    //2. Notify users for saving results if it has been changed

                //}

                ////Remove the tab page from the UI
                //_theReportsTab.TabPages.Remove(aTabPage);

                ////Remove the double click handler on the tab page
                ////save the configuration
                //if (!(reportTabContent is AdhocReportTabContent))
                //{
                //    WidgetRegistry.GetInstance().SaveWidget(reportTabContent.Widget.UniversalWidgetConfiguration);
                //}TEST

                string reportID = reportTabContent.Config.ReportID;
                if (reportTabContent is AdhocReportTabContent)
                {
                    reportID = AdhocReportTabName;
                }
                //Remove the tab page from the hash
                if (reportsBeingDisplayed.ContainsKey(reportID))
                {
                    List<TabPage> tabs = reportsBeingDisplayed[reportID];
                    if (tabs.Contains(aTabPage))
                    {
                        tabs.Remove(aTabPage);
                        _theMetricMinerNavigator.RemoveReportFromCombo(reportTabContent.Widget.UniversalWidgetConfiguration, aTabPage);
                    }

                    //TODO: We have to handle the following
                    //1. If the report is running when the user tries to close the tab, we have to 
                    //   stop the query.
                    //2. Notify users for saving results if it has been changed

                }
                //only user changed query and run that query then we ask user to decide whether save or not.
                if (reportTabContent.EditMode && reportTabContent.RunFlag)
                {
                    DialogResult result = MessageBox.Show("The current report might have been changed. Do you want to save the report?", "Close report?", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                    if (result == DialogResult.Yes)
                    {
                        //Remove the double click handler on the tab page
                        //save the configuration
                        if (!(reportTabContent is AdhocReportTabContent))
                        {
                            WidgetRegistry.GetInstance().SaveWidget(reportTabContent.Widget.UniversalWidgetConfiguration);
                        }
                    }
                    else
                    {
                        //since the SQL text is already changed, so we need to get original SQL text from local hard disk and restore it.
                        UniversalWidgetConfig oldWidgetConfig = reportTabContent.Widget.UniversalWidgetConfiguration;
                        string fullWidgetName = Path.Combine(oldWidgetConfig.ReportPath,oldWidgetConfig.ReportFileName);
                        UniversalWidgetConfig newWidgetConfig = WidgetRegistry.GetInstance().LoadWidgetFromFileWithoutRegistry(fullWidgetName);
                        ((DatabaseDataProviderConfig)oldWidgetConfig.DataProviderConfig).SQLText = ((DatabaseDataProviderConfig)newWidgetConfig.DataProviderConfig).SQLText;
                    }
                }

                //Remove the tab page from the UI
                _theReportsTab.TabPages.Remove(aTabPage);

                ////Remove the double click handler on the tab page
                ////save the configuration
                //if (!(reportTabContent is AdhocReportTabContent))
                //{
                //    WidgetRegistry.GetInstance().SaveWidget(reportTabContent.Widget.UniversalWidgetConfiguration);
                //}
            }

        }

        /// <summary>
        /// Implements the report selection for the navigator
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aTabPage"></param>
        void ReportSelectedImpl(UniversalWidgetConfig aConfig, TabPage aTabPage)
        {
            _theReportsTab.SelectedTab = aTabPage;
        }

        /// <summary>
        /// Displays the new report as a new tab
        /// </summary>
        /// <param name="config"></param>
        private void ShowNewWidgetImpl(UniversalWidgetConfig config, WidgetLinkerObject widgetLinker)
        {
            bool addNew = shouldAddNew(config);
            TabPage tab = null;
            if (addNew)
            {
                tab = AddTab(config, true, _theReportsTab.TabPages.Count);
                _theReportsTab.SelectedTab = tab;
            }
            else
            {
                tab = SelectTab(config.ReportID);
            }

            //We have to execute the existing report with the new parameters in widgetLinker
            MetricMinerReportTabContent tabContent = GetTabContent(tab);
            tabContent.SelectCatalogAndSchema(widgetLinker);

            //Create a copy of the Hashtable so that we can pass the schema and catalog info
            //For some reason, the passed widgetLinker.MappedParameters is immutable
            Hashtable paramsToPass = new Hashtable();
            foreach (object key in widgetLinker.MappedParameters.Keys)
            {
                if (widgetLinker.RowHashTable.Contains(key))
                {
                    paramsToPass.Add(key, widgetLinker.RowHashTable[key]);
                }
                else
                {
                    paramsToPass.Add(key, widgetLinker.MappedParameters[key]);
                }
            }
            //Pass the ctalog and schema to the data provider. 
            //TODO: If a user names a parameter "CATALOG_NAME" or "SCHEMA_NAME", this code will fail.
            //We will have to address this moving forward.

            string passedCatalog = ((widgetLinker.AdditionalParameters != null) && (widgetLinker.AdditionalParameters.ContainsKey("CATALOG"))) ? widgetLinker.AdditionalParameters["CATALOG"] as string : null;
            string passedSchema = ((widgetLinker.AdditionalParameters != null) && (widgetLinker.AdditionalParameters.ContainsKey("SCHEMA"))) ? widgetLinker.AdditionalParameters["SCHEMA"] as string : null;
            if ((passedCatalog != null) && (passedSchema != null))
            {
                if (paramsToPass.Contains(ReportParameterProcessorBase.CATALOG_NAME))
                {
                    paramsToPass.Remove(ReportParameterProcessorBase.CATALOG_NAME);
                }
                paramsToPass.Add(ReportParameterProcessorBase.CATALOG_NAME, passedCatalog);

                if (paramsToPass.Contains(ReportParameterProcessorBase.SCHEMA_NAME))
                {
                    paramsToPass.Remove(ReportParameterProcessorBase.SCHEMA_NAME);
                }
                paramsToPass.Add(ReportParameterProcessorBase.SCHEMA_NAME, passedSchema);
            }

            if (tabContent != null)
            {
                tabContent.Widget.DataProvider.Start(paramsToPass);
            }
        }

        /// <summary>
        /// Given the report name, selects the first tab from the list
        /// </summary>
        /// <param name="reportKey"></param>
        /// <returns></returns>
        private TabPage SelectTab(string reportKey)
        {
            TabPage tab = null;
            if (reportsBeingDisplayed.ContainsKey(reportKey))
            {
                tab = reportsBeingDisplayed[reportKey][0];
                _theReportsTab.SelectedTab = tab;
            }
            return tab;
        }

        //Gets the MetricMinerReportTabContent in the tab
        private MetricMinerReportTabContent GetTabContent(TabPage aTabPage)
        {
            if (aTabPage != null)
            {
                foreach (Control control in aTabPage.Controls)
                {
                    if (control is MetricMinerReportTabContent)
                    {
                        return control as MetricMinerReportTabContent;
                    }
                }
            }
            return null;
        }

        private void _theReportsTab_SelectedIndexChanged(object sender, EventArgs e)
        {
            TabPage tabPage = _theReportsTab.SelectedTab;
            if (tabPage != null && tabPage.Text == AdhocReportTabName)
            {
                MetricMinerReportTabContent reportTab = tabPage.Controls[0] as MetricMinerReportTabContent;
                reportTab.getQueryInputControl.ReadOnly = false;
            }

            //set querycontrol flag
            foreach (TabPage page in _theReportsTab.TabPages)
            {
                MetricMinerReportTabContent mmContent = page.Controls[0] as MetricMinerReportTabContent;
                if (mmContent != null)
                {
                    if (page == _theReportsTab.SelectedTab)
                    {
                        mmContent.QueryPlanDataDisplayControl.IsActiveTabPage = true;
                    }
                    else
                    {
                        mmContent.QueryPlanDataDisplayControl.IsActiveTabPage = false;
                    }

                    mmContent.QueryPlanDataDisplayControl.ReloadQueryPlan();
                }
            }

        }

        #region IMainToolBarConsumer implementation

        /// <summary>
        /// Implementating the IMainToolBarConsumer interface, which the consumer could elect buttons to show and modify 
        /// the Help button to invoke context sensitive help topic.
        /// </summary>
        /// <param name="aMainToolBar"></param>
        public void CustomizeMainToolBarItems(Trafodion.Manager.Framework.MainToolBar aMainToolBar)
        {
            // Now, turn on all of the tool strip buttons for Whiteboard.
            aMainToolBar.TheSystemToolToolStripItem.Visible = true;
            aMainToolBar.TheSystemsToolStripSeparator.Visible = true;
            aMainToolBar.TheNCIToolStripItem.Visible = true;
            aMainToolBar.TheSQLWhiteboardToolStripItem.Visible = true;
            aMainToolBar.TheOptionsToolStripItem.Visible = true;
            aMainToolBar.TheToolsStripSeparator.Visible = true;
            aMainToolBar.TheWindowManagerToolStripItem.Visible = true;
            aMainToolBar.TheWindowManagerStripSeparator.Visible = true;
            aMainToolBar.TheHelpToolStripItem.Visible = true;

            ///Customize the help topic if it is desired.
            aMainToolBar.UnRegisterDefaultHelpEventHandler();
            aMainToolBar.TheHelpToolStripItem.Alignment = ToolStripItemAlignment.Right;
            aMainToolBar.TheHelpToolStripItem.Click += new EventHandler(TheHelpToolStripItem_Click);
        }

        /// <summary>
        /// The event handler for the context sensitive 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheHelpToolStripItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.MetricMiner);
        }

        #endregion 
    }
}
