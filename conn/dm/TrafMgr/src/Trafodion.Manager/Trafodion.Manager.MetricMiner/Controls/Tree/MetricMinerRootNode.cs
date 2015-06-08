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
using System.IO;
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.MetricMiner.Controls.Tree
{
    public class MetricMinerRootNode : NavigationTreeFolder
    {        
        private AdhocReportNode _theAdHocNode = null;
        private static bool _defaultReportFolderLoaded = false;

        public bool IsRefreshingRootNode
        {
            get;
            set;
        }
 
        public AdhocReportNode AdHocNode
        {
            get { return _theAdHocNode; }
            set { _theAdHocNode = value; }
        }

        public MetricMinerRootNode()
        {
            this.Text = "Reports";
            this.ToolTipText = "Metric Miner Reports";
            ImageKey = MetricMinerWidgetsTreeView.MM_REPORT_ROOT_ICON;
            SelectedImageKey = MetricMinerWidgetsTreeView.MM_REPORT_ROOT_ICON;
            AddDefaultReportsFolder();
            this.Expand();
        }
        
        public void RemoveLibrary(MetricMinerReportFolder libraryFolder)
        {
            this.Nodes.Remove(libraryFolder);
            TabbedMetricMinerControl.TheLibraryManager.RemovePath(libraryFolder.FolderPath);
        }

        private void AddLibrary()
        {
            FolderBrowserDialog dlg = new FolderBrowserDialog();
            dlg.Description = "Select the folder where the report library exists";
            dlg.ShowNewFolderButton = true;
            dlg.RootFolder = Environment.SpecialFolder.MyComputer;
            if (LibraryManager.Instance.LastUsedPath != null)
            {
                dlg.SelectedPath = LibraryManager.Instance.LastUsedPath;
            }
            if (dlg.ShowDialog() == DialogResult.OK)
            {
                string path = dlg.SelectedPath;
                AddPath(path);
            }
        }

        private void AddDefaultReportsFolder()
        {
            string defaultReportsFolder = Path.Combine(Environment.CurrentDirectory,"Reports");
            //If default Report folder exists and it has not been implicitly loaded yet, load the reports in the default report folder
            //The boolean _defaultReportFolderLoaded ensures this is done only one during the lifetime of the running application.
            //If the user were to remove the default report folder explicitly and relaunch metric miner, the default report folder will not be loaded again.
            //The user has to manually load the default folder or restart TrafodionManager.
            if (Directory.Exists(defaultReportsFolder) && !TabbedMetricMinerControl.TheLibraryManager.ContainsPath(defaultReportsFolder) && !_defaultReportFolderLoaded)
            {
                AddPath(defaultReportsFolder);
                _defaultReportFolderLoaded = true;
            }
        }

        private void AddPath(string aPath)
        {
            if (TabbedMetricMinerControl.TheLibraryManager.ContainsPath(aPath))
            {
                MessageBox.Show("The library that you are trying to add has already been added.", "Library Already Added", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                try
                {
                    if (this.TreeView != null)
                    {
                        this.TreeView.Cursor = Cursors.WaitCursor;
                    }
                    TabbedMetricMinerControl.TheLibraryManager.AddPath(aPath);
                    MetricMinerReportFolder reportFolder = new MetricMinerReportFolder(aPath, aPath);
                    this.Nodes.Add(reportFolder);
                    reportFolder.Expand();
                }
                finally
                {
                    if (this.TreeView != null)
                    {
                        this.TreeView.Cursor = Cursors.Default;
                    }

                }
            }
        }


        /// <summary>
        /// Refreshes the node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            this.IsRefreshingRootNode = true;
        }


        /// <summary>
        /// This method lets the TreeNodes to add context menu items that are specific to the node
        /// The Navigation tree calls this method and passes a context menu strip to which the menu items need to be added
        /// The base nodes implementation of this method needs to be called first to have the common menu items added
        /// </summary>
        /// <param name="aContextMenuStrip">The context menu strip to which the menu items have to be added</param>
        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //Add menu item to add a new library
            aContextMenuStrip.Items.Add(GetAddLibraryMenuItem(this));
            aContextMenuStrip.Items.Add(new ToolStripSeparator());

            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

        }

        /// <summary>
        /// Static method to create a Procedure code file menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public ToolStripMenuItem GetAddLibraryMenuItem(TreeNode node)
        {
            ToolStripMenuItem addLibraryMenuItem = new ToolStripMenuItem("Add Report Folder...");
            addLibraryMenuItem.Tag = node;
            addLibraryMenuItem.Click += new EventHandler(addLibraryMenuItem_Click);

            return addLibraryMenuItem;
        }

        void addLibraryMenuItem_Click(object sender, EventArgs e)
        {
            AddLibrary();
        }

        /// <summary>
        /// Populate the child nodes
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();

            MetricMinerWidgetsTreeView mmTreeView = this.TreeView as MetricMinerWidgetsTreeView;
            if (mmTreeView != null)
            {
                //we will add a node for the ad-hoc report by refault
                _theAdHocNode = new AdhocReportNode();
                this.Nodes.Add(_theAdHocNode);
                int imgIdx = mmTreeView.ImageList.Images.IndexOfKey("Folder");

                this.ImageIndex = imgIdx;
                this.SelectedImageIndex = imgIdx;
            }

            List<string> pathsToRemove = new List<string>();
            foreach (string path in TabbedMetricMinerControl.TheLibraryManager.LibraryPaths)
            {
                if (Directory.Exists(path))
                {
                    this.Nodes.Add(new MetricMinerReportFolder(path, path));
                }
                else
                {
                    pathsToRemove.Add(path);
                }
            }

            foreach (string path in pathsToRemove)
            {
                TabbedMetricMinerControl.TheLibraryManager.LibraryPaths.Remove(path);
            }

            if (mmTreeView != null && _theAdHocNode != null)
            {
                mmTreeView.TreePopulateImpl(mmTreeView);
            }
        }

        /// <summary>
        /// The connection definition associated with this tree node
        /// </summary>
        public override ConnectionDefinition TheConnectionDefinition
        {
            get { return null; }
        }

        /// <summary>
        /// The short description of the root tree node
        /// </summary>
        public override string ShortDescription
        {
            get { return Text; }
        }

        /// <summary>
        /// The long description of the root tree node
        /// </summary>
        public override string LongerDescription
        {
            get { return Text; }
        }

    }
}
