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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Controls;
using System.IO;

namespace Trafodion.Manager.MetricMiner.Controls.Tree
{
    public class MetricMinerWidgetNode : NavigationTreeNode
    {
        public delegate void NodeDeleted(object sender, EventArgs e);
        public event NodeDeleted OnNodeDeleted;

        public MetricMinerWidgetNode(UniversalWidgetConfig aUniversalWidgetConfig)
        {
            Config = aUniversalWidgetConfig;
        }

        public UniversalWidgetConfig Config
        {
            get
            {
                return (UniversalWidgetConfig)Tag;
            }

            set
            {
                this.Tag = value;
                this.Text = WidgetRegistry.GetWidgetDisplayName(value);
                this.ToolTipText = value.Title;

                if (value.SupportCharts && value.ChartConfig != null && value.ChartConfig.ChartAreaConfigs != null && value.ChartConfig.ChartAreaConfigs.Count > 0)
                {
                    ImageKey = MetricMinerWidgetsTreeView.MM_CHART_REPORT_ICON;
                    SelectedImageKey = MetricMinerWidgetsTreeView.MM_CHART_REPORT_ICON;
                }
                else
                {
                    if ((value.AssociatedWidgets != null) && (value.AssociatedWidgets.Count > 0))
                    {
                        ImageKey = MetricMinerWidgetsTreeView.MM_Linked_REPORT_ICON;
                        SelectedImageKey = MetricMinerWidgetsTreeView.MM_Linked_REPORT_ICON;
                    }
                    else
                    {
                        ImageKey = MetricMinerWidgetsTreeView.MM_REPORT_ICON;
                        SelectedImageKey = MetricMinerWidgetsTreeView.MM_REPORT_ICON;
                    }
                }
            }
        }

        /// <summary>
        /// The short description of the root tree node
        /// </summary>
        public override string ShortDescription
        {
            get 
            {
                UniversalWidgetConfig aConfig = this.Tag as UniversalWidgetConfig;
                if (aConfig != null)
                {
                    return WidgetRegistry.GetWidgetDisplayName(aConfig);
                }
                return Text;
            }
        }

        /// <summary>
        /// The long description of the root tree node
        /// </summary>
        public override string LongerDescription
        {
            get 
            {
                UniversalWidgetConfig aConfig = this.Tag as UniversalWidgetConfig;
                if (aConfig != null)
                {
                    return aConfig.Title;
                }
                return Text;
            }
        }
        /// <summary>
        /// Refreshes the node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            //since the SQL text is already changed, so we need to get original SQL text from local hard disk and restore it.
            UniversalWidgetConfig aConfig = this.Tag as UniversalWidgetConfig;
            if (aConfig != null)
            {
                string fullWidgetName = Path.Combine(aConfig.ReportPath,aConfig.ReportFileName);
                if (File.Exists(fullWidgetName))
                {
                    UniversalWidgetConfig newWidgetConfig = WidgetRegistry.GetInstance().LoadWidgetFromFileWithoutRegistry(fullWidgetName);
                    newWidgetConfig.ReportID = aConfig.ReportID;
                    newWidgetConfig.SupportCharts = aConfig.SupportCharts;
                    newWidgetConfig.ReportFileName = aConfig.ReportFileName;
                    foreach (AssociatedWidgetConfig asw in newWidgetConfig.AssociatedWidgets)
                    {
                        //initiate report ID
                        asw.CallingWidgetID = newWidgetConfig.ReportID;
                        asw.CalledWidgetID = WidgetRegistry.GetInstance().GetWidgetByName(asw.CalledWidgetName).ReportID;
                    }
                    newWidgetConfig.ReportPath = aConfig.ReportPath;
                    ((DatabaseDataProviderConfig)aConfig.DataProviderConfig).SQLText = ((DatabaseDataProviderConfig)newWidgetConfig.DataProviderConfig).SQLText;
                    //this.Tag = newWidgetConfig;
                    this.Config = newWidgetConfig;
                }
            }
        }

        /// <summary>
        /// This method lets the TreeNodes to add context menu items that are specific to the node
        /// The Navigation tree calls this method and passes a context menu strip to which the menu items need to be added
        /// The base nodes implementation of this method needs to be called first to have the common menu items added
        /// </summary>
        /// <param name="aContextMenuStrip">The context menu strip to which the menu items have to be added</param>
        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            //Add the menu item to delete it
            if (!(this is AdhocReportNode))
            {
                aContextMenuStrip.Items.Add(GetDeleteMenuItem(this));
            }
        }

        /// <summary>
        /// Static method to create a Delete context menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public ToolStripMenuItem GetDeleteMenuItem(TreeNode node)
        {
            ToolStripMenuItem deleteMenuItem = new ToolStripMenuItem("Delete Report");
            deleteMenuItem.Tag = node;
            deleteMenuItem.Click += new EventHandler(deleteMenuItem_Click);

            return deleteMenuItem;
        }

        void deleteMenuItem_Click(object sender, EventArgs e)
        {
            UniversalWidgetConfig aConfig = this.Tag as UniversalWidgetConfig;
            if (aConfig != null)
            {
                DialogResult delete = MessageBox.Show("The file associated with the report " + WidgetRegistry.GetWidgetDisplayName(aConfig) + " will be deleted and cannot be recovered. \n Do you want to continue?", "Delete Report", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (delete == DialogResult.Yes)
                {
                    WidgetRegistry.GetInstance().DeleteWidget(aConfig);
                    //for considering logical path in navigation tree, need to clear all nodes.
                    clearNodes(this);
                    //Remove();
                    FireNodeDeleted(null);
                }
            }
        }

        private void FireNodeDeleted(EventArgs e)
        {
            if (OnNodeDeleted != null)
            {
                OnNodeDeleted(this, e);
            }
        }

        //recursively invoke this method to delete node from starting node.
        private void clearNodes(TreeNode node)
        {
            TreeNode current = node.Parent;
            node.Remove();
            if (current.Text.Contains("\\"))
            {
                return;
            }
            if (current.Nodes.Count == 0)
            {
                clearNodes(current);
            }
        }
     }
}
