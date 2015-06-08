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
using System.IO;
using System.Threading;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.MetricMiner.Controls.Tree
{
    public class MetricMinerReportFolder : NavigationTreeFolder
    {
        public delegate void WidgetDeletedCallback(UniversalWidgetConfig config);
        public delegate void WidgetAddCallback(UniversalWidgetConfig config);
        public delegate void WidgetUpdateCallback(string path);
        public delegate void WidgetRenamedCallback(string path, UniversalWidgetConfig config);
        #region Private member variables

        private bool isLibraryFolder = false;
        private string _theFolderPath = null;
        FileSystemWatcher _theFileSystemWatcher;
        private System.Threading.Timer mTimer = null;
        private int mDelay = 800;

        #endregion Private member variables

        #region Public properties

        /// <summary>
        /// The folder path
        /// </summary>
        public string FolderPath
        {
            get { return _theFolderPath; }
        }

        /// <summary>
        /// Indicates if the folder is the root node of a library. In that case
        /// the tag will have the path to the library
        /// </summary>
        public bool IsLibraryFolder
        {
            get { return isLibraryFolder; }
            set { isLibraryFolder = value; }
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
        #endregion Public properties

        /// <summary>
        /// Constructs a MetricMinerReportFolder folder 
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        public MetricMinerReportFolder(String aText)
        {
            Name = aText;
            Text = aText;
        }


        public MetricMinerReportFolder(String aText, string aFolderPath)
            : this(aText)
        {
            ToolTipText = aFolderPath;
            _theFolderPath = aFolderPath;
            isLibraryFolder = true;
            _theFileSystemWatcher = new FileSystemWatcher(aFolderPath, "*.widget");
            //  Watch for changes in LastAccess and LastWrite times, and
            //  the renaming of files or directories. 
            _theFileSystemWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName | NotifyFilters.DirectoryName;
            //  Register a handler that gets called when a 
            //  file is created, changed, or deleted.
            _theFileSystemWatcher.Changed += new FileSystemEventHandler(OnChanged);

            _theFileSystemWatcher.Created += new FileSystemEventHandler(OnCreated);

            _theFileSystemWatcher.Deleted += new FileSystemEventHandler(OnDeleted);

            //  Register a handler that gets called when a file is renamed.
            _theFileSystemWatcher.Renamed += new RenamedEventHandler(OnRenamed);

            //  Register a handler that gets called if the 
            //  FileSystemWatcher needs to report an error.
            _theFileSystemWatcher.Error += new ErrorEventHandler(OnError);

            if (mTimer == null)
            {
                mTimer = new System.Threading.Timer(new TimerCallback(OnTimer), null, Timeout.Infinite, Timeout.Infinite);
            }

            //  Begin watching.
            _theFileSystemWatcher.EnableRaisingEvents = true;

            ImageKey = MetricMinerWidgetsTreeView.MM_LIBRARY_FOLDER_ICON;
            SelectedImageKey = MetricMinerWidgetsTreeView.MM_LIBRARY_FOLDER_ICON;
        }

        public override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (_theFileSystemWatcher != null)
                {
                    _theFileSystemWatcher.Dispose();
                    _theFileSystemWatcher = null;
                }
            }
        }

        public delegate void TreeUpdateCallback(NavigationTreeNameFilter aNavigationTreeNameFilter);
        private void OnTimer(object state)
        {
            this.TreeView.Invoke(new TreeUpdateCallback(this.Populate), new NavigationTreeNameFilter());
        }

        //  This method is called when a file is  changed, or deleted.
        private void OnChanged(object source, FileSystemEventArgs e)
        {
            if (this.TreeView != null)
            {
                if (this.TreeView.InvokeRequired)
                {
                    try
                    {
                        this.TreeView.Invoke(new WidgetUpdateCallback(this.doUpdateNode), e.FullPath);
                    }
                    catch
                    {
                        mTimer.Change(mDelay, Timeout.Infinite);
                    }
                }
                else
                {
                    doUpdateNode(e.FullPath);
                }
            }
        }
        private void doUpdateNode(string fullPath)
        {
            UniversalWidgetConfig config = WidgetRegistry.GetInstance().GetWidgetForFileName(fullPath);
            //get report path from fullpath and it will be stored in universal widget config.
            string reportPath = fullPath.Substring(0, fullPath.LastIndexOf("\\"));
            //universal widget config is null if report is just created.
            if (config == null)
            {
                //load report config from local hard disk and store it to registry at the same time.
                UniversalWidgetConfig newConfig = WidgetRegistry.GetInstance().LoadWidgetFromFile(fullPath);

                newConfig.ReportFileName = fullPath;
                newConfig.ReportPath = reportPath;
                addWidgetToNode(newConfig, this);
            }
                //if user updates existing report configuration then the config got from registy is not null.
            else
            {
                //get node from tree based on report ID and delete that node and finally add new node with updated configuration.
                //TreeNode node = getNodeForConfig(config, this);
                //if (node != null)
                //{
                //    node.Remove();
                //    //clearNodes(node);
                //}
                //addWidgetToNode(config, this);
            }
            updateReportCount(this);
        }

        private void updateReportCount(MetricMinerReportFolder folder)
        {
            int reportCount = 0;
            GetWidgetsCount(this, ref reportCount);
            folder.Text = folder.Name + string.Format(" ({0} Reports)", reportCount);
        }

        void GetWidgetsCount(MetricMinerReportFolder folder, ref int reportCount)
        {
            foreach(TreeNode node in folder.Nodes)
            {
                if (node is MetricMinerWidgetNode)
                    ++reportCount;

                if (node is MetricMinerReportFolder)
                {
                    GetWidgetsCount((MetricMinerReportFolder)node, ref reportCount);
                }
            }
        }

        //this method is used to given node from node tree.
        //private void clearNodes(TreeNode node)
        //{
        //    TreeNode current = node.Parent;
        //    node.Remove();
        //    //stop recursive invoking if current node is a directory level.
        //    if (current.Text.Contains("\\"))
        //    {
        //        return;
        //    }
        //    //stop recursive invoking if current node has no child.
        //    if (current.Nodes.Count == 0)
        //    {
        //        clearNodes(current);
        //    }
        //}


        //  This method is called when a file is created
        private void OnCreated(object source, FileSystemEventArgs e)
        {
            //UniversalWidgetConfig config = WidgetRegistry.GetInstance().LoadWidgetFromFile(e.FullPath);
            // The created event gets fired as noon as the file is created, even before all of the content
            // is written to the file. Hence there is no point in trying to read it and load it. 
            // We will load it once the content has been saved and the update event gets fired.
            //this.TreeView.Invoke(new WidgetAddCallback(this.doAddNode), config);
        }
        //private void doAddNode(UniversalWidgetConfig config)
        //{
        //    if (config != null)
        //    {
        //        addWidgetToNode(config, this);
        //    }
        //}

        //  This method is called when a file is deleted
        private void OnDeleted(object source, FileSystemEventArgs e)
        {
            //actually delete operation is done in MetricMinerWidgetNode's function deleteMenuItem_Click, so we do not need this method.

            UniversalWidgetConfig config = WidgetRegistry.GetInstance().GetWidgetForFileName(e.FullPath);
            if (this.TreeView != null)
            {
                if (this.TreeView.InvokeRequired)
                {
                    this.TreeView.Invoke(new WidgetDeletedCallback(this.doDeleteNode), config);
                }
                else
                {
                    doDeleteNode(config);
                }
            }
        }

        private void doDeleteNode(UniversalWidgetConfig config)
        {
            //the reason is same as above.
            if (config != null)
            {
                removeWidgetFromNode(config, this);
                WidgetRegistry.GetInstance().RemoveWidgetFromRegistry(config);
            }

            updateReportCount(this);
        }

        //  This method is called when a file is renamed.
        private void OnRenamed(object source, RenamedEventArgs e)
        {
            UniversalWidgetConfig config = WidgetRegistry.GetInstance().GetWidgetForFileName(e.OldFullPath);
            if (this.TreeView != null)
            {
                if (this.TreeView.InvokeRequired)
                {
                    this.TreeView.Invoke(new WidgetRenamedCallback(this.doRenameNode), new object[] { e.FullPath, config });
                }
                else
                {
                    doRenameNode(e.FullPath, config);
                }
            }
        }

        private void doRenameNode(string fullPath, UniversalWidgetConfig config)
        {
            if (config != null)
            {
                //remove the old file
                removeWidgetFromNode(config, this);

                //add the config with the new file
                config = WidgetRegistry.GetInstance().LoadWidgetFromFile(fullPath);
                addWidgetToNode(config, this);
            }
        }

        //  This method is called when the FileSystemWatcher detects an error.
        private void OnError(object source, ErrorEventArgs e)
        {

            if (e.GetException().GetType() == typeof(InternalBufferOverflowException))
            {
                mTimer.Change(mDelay, Timeout.Infinite);
                //  This can happen if Windows is reporting many file system events quickly 
                //  and internal buffer of the  FileSystemWatcher is not large enough to handle this
                //  rate of events. The InternalBufferOverflowException error informs the application
                //  that some of the file system events are being lost.

                //TODO: we might have to refresh the whole folder in this situation
            }
        }

        /// <summary>
        /// Finds the parent node that points to a report library and refreshes that parent
        /// After the refresh is complete, the focus is set back on the current folder
        /// </summary>
        /// <param name="aNameFilter"></param>
        protected override void DoRefresh(NavigationTreeNameFilter aNameFilter)
        {
            TreeView theTreeView = this.TreeView;
            string theCurrentPath = this.FullPath;
            string relativePath = "";

            if (_theFolderPath != null) //If folder path is null then it is an intermediatery folder. No action for refresh
            {
                base.DoRefresh(aNameFilter);
            }
            else
            {
                TreeNode refreshableParent = this.Parent;
                while (refreshableParent != null)
                {
                    if (refreshableParent is MetricMinerReportFolder && ((MetricMinerReportFolder)refreshableParent).FolderPath != null)
                    {
                        relativePath = theCurrentPath.Replace(refreshableParent.FullPath + "\\", "");
                        ((MetricMinerReportFolder)refreshableParent).DoRefresh(aNameFilter);
                        TreeNode newNode = ((NavigationTreeView)theTreeView).FindChildNodeByRelativePath(refreshableParent, relativePath);
                        if (theTreeView != null && newNode != null)
                        {
                            theTreeView.SelectedNode = newNode;
                            newNode.Expand();
                        }
                        break;
                    }
                    else
                    {
                        refreshableParent = refreshableParent.Parent;
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
            //Nodes.Clear();
            Populate(aNavigationTreeNameFilter);
        }

        /// <summary>
        /// Populate the child nodes
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            if ((this.Parent != null) && (this.Parent is MetricMinerRootNode))
            {
                Nodes.Clear();
            }

            MetricMinerWidgetsTreeView mmTreeView = this.TreeView as MetricMinerWidgetsTreeView;
            if (mmTreeView != null)
            {
                List<UniversalWidgetConfig> configs = WidgetRegistry.GetInstance().LoadWidgets(_theFolderPath);
                foreach (UniversalWidgetConfig config in configs)
                {
                    // Metric minder supports charts; override the saved config since this property is new.
                    config.SupportCharts = true;

                    addWidgetToNode(config, this);
                    ////load for old version of reports
                    if (string.IsNullOrEmpty(config.ReportFileName))
                    {
                        config.ReportFileName = config.Name + ".widget";
                        foreach (AssociatedWidgetConfig asw in config.AssociatedWidgets)
                        {
                            //initiate report ID
                            asw.CallingWidgetID = config.ReportID;
                            asw.CalledWidgetID = WidgetRegistry.GetInstance().GetWidgetByName(asw.CalledWidgetName).ReportID;
                        }
                    }
                    config.ReportPath = _theFolderPath;
                }
                if ((this.Parent != null) && (this.Parent is MetricMinerRootNode))
                {
                    this.Text = this.Name + string.Format(" ({0} Reports)", configs.Count);
                }
            }
            //this.TreeView.Sort();
        }

        /// <summary>
        /// Given a node, adds a config to a tree node
        /// </summary>
        /// <param name="config"></param>
        /// <param name="aFolder"></param>
        private void addWidgetToNode(UniversalWidgetConfig config, MetricMinerReportFolder aFolder)
        {
            string name = config.Name;
            string[] parts = name.Split(new char[] { '@' });
            MetricMinerReportFolder currentFolder = aFolder;
            for (int i = 0; i < parts.Length; i++)
            {
                if (i < (parts.Length - 1))
                {
                    if (parts[i].Trim().Length > 0)
                    {
                        if (currentFolder.Nodes.ContainsKey(parts[i]))
                        {
                            currentFolder = currentFolder.Nodes[parts[i]] as MetricMinerReportFolder;
                        }
                        else
                        {
                            MetricMinerReportFolder folder = new MetricMinerReportFolder(parts[i]);
                            folder.Nodes.Clear();
                            currentFolder.Nodes.Add(folder);
                            currentFolder = folder;
                        }
                    }
                }
                else
                {
                    currentFolder.Nodes.Add(new MetricMinerWidgetNode(config));
                }
            }
            //this.TreeView.Sort();
        }

        /// <summary>
        /// Given a universal widget config, finds the node in the children nodes
        /// </summary>
        /// <param name="config"></param>
        /// <returns></returns>
        public TreeNode getNodeForConfig(UniversalWidgetConfig config)
        {
            return getNodeForConfig(config, this);
        }

        /// <summary>
        /// Given a universal widget config, finds the node in the tree starting from a given folder
        /// </summary>
        /// <param name="config"></param>
        /// <param name="aFolder"></param>
        /// <returns></returns>
        private TreeNode getNodeForConfig(UniversalWidgetConfig config, MetricMinerReportFolder aFolder)
        {
            return searchTreeNode(config,aFolder);
            //TreeNode ret = null;
            //string name = config.Name;
            //string[] parts = name.Split(new char[] { '@' });
            //MetricMinerReportFolder currentFolder = aFolder;
            //for (int i = 0; i < parts.Length; i++)
            //{
            //    if (currentFolder != null)
            //    {
            //        if (i < (parts.Length - 1))
            //        {
            //            if (currentFolder.Nodes.ContainsKey(parts[i]))
            //            {
            //                currentFolder = currentFolder.Nodes[parts[i]] as MetricMinerReportFolder;
            //                Console.WriteLine(currentFolder.FullPath);
            //            }
            //            else
            //            {
            //                return searchTreeNode(config,currentFolder);//MM1
            //            }
            //        }
            //        else
            //        {
            //            return searchTreeNode(config,currentFolder);
            //        }
            //    }
            //}
            //return null;
        }

        //recursively invoke this method to get universal config, since it's stored in leaf in navigation tree.
        private TreeNode searchTreeNode(UniversalWidgetConfig config, MetricMinerReportFolder aFolder)
        {
            TreeNode retValue = null;

            foreach (TreeNode tn in aFolder.Nodes)
            {
                UniversalWidgetConfig widgetConfig = tn.Tag as UniversalWidgetConfig;
                //if widgetconfig is not null then it's a leaf. get configuration and do comparision with report ID.
                if (widgetConfig != null)
                {
                    if (config.ReportID == widgetConfig.ReportID)
                    {
                        retValue = tn;
                        break;
                    }
                }
                //if widgetconfig is null then it's folder, need to drill down to leaf.
                else
                {
                    MetricMinerReportFolder currentFolder = tn as MetricMinerReportFolder;
                    if (currentFolder != null)
                    {
                        retValue = searchTreeNode(config, currentFolder);
                        if (retValue != null)
                        {
                            break;
                        }
                    }
                }
            }
            return retValue;
        }

        /// <summary>
        /// Given a config, removes it from the tree if it's under the given folder
        /// </summary>
        /// <param name="config"></param>
        /// <param name="aFolder"></param>
        private void removeWidgetFromNode(UniversalWidgetConfig config, MetricMinerReportFolder aFolder)
        {
            TreeNode nodeToDelete = getNodeForConfig(config, aFolder);
            if (nodeToDelete != null)
            {
                nodeToDelete.Remove();
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
            if (isLibraryFolder)
            {
                aContextMenuStrip.Items.Add(GetImportNVReportMenu(this));
                aContextMenuStrip.Items.Add(new ToolStripSeparator());
                aContextMenuStrip.Items.Add(GetRemoveLibraryFolderMenu(this));
                aContextMenuStrip.Items.Add(new ToolStripSeparator());
            }
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

        }

        private ToolStripMenuItem GetRemoveLibraryFolderMenu(TreeNode node)
        {
            ToolStripMenuItem removeMenuItem = new ToolStripMenuItem("Remove Report Folder");
            removeMenuItem.Tag = node;
            removeMenuItem.Click += new EventHandler(removeMenuItem_Click);
            return removeMenuItem;
        }

        void removeMenuItem_Click(object sender, EventArgs e)
        {
            //call the parent to remove the node
            if (this.Parent is MetricMinerRootNode)
            {
                MetricMinerRootNode parent = this.Parent as MetricMinerRootNode;
                DialogResult remove = MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to remove the Report Folder?", "Remove Report Folder", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (remove == DialogResult.Yes)
                {
                    parent.RemoveLibrary(this);

                    WidgetRegistry.GetInstance().RemoveWidgetFromRegistry(this.FolderPath);
                }
            }
        }

        private ToolStripMenuItem GetImportNVReportMenu(TreeNode node)
        {
            ToolStripMenuItem importNVReportMenuItem = new ToolStripMenuItem("Import NV Report...");
            importNVReportMenuItem.Tag = node;
            importNVReportMenuItem.Click += new EventHandler(importNVReportMenuItem_Click);
            return importNVReportMenuItem;
        }

        void importNVReportMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog ofn = new OpenFileDialog();
            ofn.Filter = "NV Reports file (*.txt)|*.txt";
            ofn.Title = "Select Trafodion Report file";
            ofn.Multiselect = true;
            if (ofn.ShowDialog() == DialogResult.OK)
            {
                if (ofn.FileNames.Length == 1)
                {
                    UniversalWidgetConfig config = ImportNVReport(ofn.FileNames[0]);
                    SaveConfigurationDialog saveDialog = new SaveConfigurationDialog(config);
                    saveDialog.LibraryPath = _theFolderPath;
                    saveDialog.ShowDialog();
                    if (saveDialog.SelectedOption == DialogResult.OK)
                    {
                        string libraryPath = saveDialog.LibraryPath;
                        string fileName = config.Name.EndsWith(".widget", StringComparison.InvariantCultureIgnoreCase) ? config.Name : string.Format("{0}.widget", config.Name);
                        try
                        {
                            WidgetRegistry.GetInstance().SaveWidget(config, libraryPath, fileName);
                        }
                        catch (Exception ex)
                        {
                            MessageBox.Show(Utilities.GetForegroundControl(), "Error importing report : " + ex.Message,
                                "Error in import", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        }
                    }
                }
                else if (ofn.FileNames.Length > 1)
                {
                    foreach (string fn in ofn.FileNames)
                    {
                        try
                        {
                            UniversalWidgetConfig config = ImportNVReport(fn);
                            string fileName = config.Name.EndsWith(".widget", StringComparison.InvariantCultureIgnoreCase) ? config.Name : string.Format("{0}.widget", config.Name);
                            WidgetRegistry.GetInstance().SaveWidget(config, _theFolderPath, fileName);
                        }
                        catch (Exception ex)
                        {
                            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), string.Format("Error encountered in importing {0} \r\n Error: {1} \r\n Do you want to continue with the rest of the import?", fn, ex.Message)
                                , "Error in import"
                                , MessageBoxButtons.YesNo
                                , MessageBoxIcon.Question);
                            if (result != DialogResult.Yes)
                            {
                                break;
                            }
                        }

                    }
                }
            }
        }

        /// <summary>
        /// Given a file name that contains a SQL trafodion report, creates a config that has the same attributes
        /// as the Trafodion report. 
        /// </summary>
        /// <param name="widgetName"></param>
        /// <returns></returns>
        public UniversalWidgetConfig ImportNVReport(string FileName)
        {
            UniversalWidgetConfig ret = null;
            if (!File.Exists(FileName))
            {
                return ret;
            }

            StreamReader sr = null;
            try
            {

                sr = File.OpenText(FileName);

                string inputText = sr.ReadToEnd();
                inputText = inputText.Replace("\r\n", "\n");
                //string name = ParseReportDefinitionName(FileName);
                string name = Path.GetFileNameWithoutExtension(FileName);
                SimpleReportDefinition reportDefinition = new SimpleReportDefinition(name);
                reportDefinition.SetProperty(ReportDefinition.DEFINITION, inputText);
                reportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, inputText);

                ParameterDocumentationReader docReader = new ParameterDocumentationReader();
                DocumentationObject docObj = docReader.getDocumentationObject(reportDefinition);
                ret = WidgetRegistry.GetDefaultDBConfig();

                ret.Name = name;
                ret.Title = ((docObj == null) || string.IsNullOrEmpty(docObj.QueryTitle)) ? name : docObj.QueryTitle;
                ret.Description = reportDefinition.Description;
                ret.WidgetVersion = ((docObj == null) || string.IsNullOrEmpty(docObj.QueryRevNum)) ? "" : docObj.QueryRevNum;
                ret.ServerVersion = ((docObj == null) || string.IsNullOrEmpty(docObj.ReposRevNum)) ? "" : docObj.ReposRevNum;

                DatabaseDataProviderConfig dbDataProviderConfig = ret.DataProviderConfig as DatabaseDataProviderConfig;
                dbDataProviderConfig.SQLText = Trafodion.Manager.DatabaseArea.Queries.ReportParameterProcessor.Instance.getQueryForExecution(reportDefinition.QueryText, new List<ReportParameter>());
            }
            catch (Exception ex)
            {
            }
            finally
            {
                if (sr != null)
                {
                    sr.Close();
                }
            }
            return ret;
        }

    }
}
