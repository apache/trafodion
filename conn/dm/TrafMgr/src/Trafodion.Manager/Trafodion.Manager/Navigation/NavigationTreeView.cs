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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;

namespace Trafodion.Manager.Framework.Navigation
{
    [ToolboxItem(false)]
    public class NavigationTreeView : TrafodionTreeView, IConnectionDefinitionSelector
    {

        #region member variables

        private bool disposed = false;
        private bool _editInProgress = false;
        private delegate void ExpandTreeNodeDelegate(TreeNode aTreeNode);
        FavoritesTreeView theFavoritesTreeView = null;

        private NavigationTreeConnectionsFolder theConnectionsFolder = new NavigationTreeConnectionsFolder();
        private IContainer components;
        public delegate void SelectedHandler(NavigationTreeNode aTreeNode);
        public delegate void ExceptionOccurredHandler(Exception anException);

        private EventHandlerList theEventHandlers = new EventHandlerList();

        private static readonly string theSelectedKey = "Selected";
        private static readonly string theExceptionOccurredKey = "ExceptionOccurred";
        private TreeNode theMouseDownTreeNode = null;
        private NavigationTreeNameFilter theNavigationTreeNameFilter = new NavigationTreeNameFilter();
        private NavigationTreeNameFilter.ChangedHandler theFilterChangedHandler = null;

        private NavigationTreeConnectionFolderFactory theNavigationTreeConnectionFolderFactory = new NavigationTreeConnectionFolderFactory();

        #endregion member variables

        #region public properties

        public NavigationTreeConnectionFolderFactory NavigationTreeConnectionFolderFactory
        {
            get { return theNavigationTreeConnectionFolderFactory; }
            set { theNavigationTreeConnectionFolderFactory = value; }
        }

        public bool EditInProgress
        {
            get { return _editInProgress; }
            set { _editInProgress = value; }
        }

        public FavoritesTreeView FavoritesTreeView
        {
            get { return theFavoritesTreeView; }
            set { theFavoritesTreeView = value; }
        }

        public event SelectedHandler Selected
        {
            add { theEventHandlers.AddHandler(theSelectedKey, value); }
            remove { theEventHandlers.RemoveHandler(theSelectedKey, value); }
        }

        public event ExceptionOccurredHandler ExceptionOccurred
        {
            add { theEventHandlers.AddHandler(theExceptionOccurredKey, value); }
            remove { theEventHandlers.RemoveHandler(theExceptionOccurredKey, value); }
        }

        /// <summary>
        /// Enables or disables the display of ContextMenu on this tree view
        /// </summary>
        virtual public bool AllowContextMenu
        {
            get { return true; }
        }

        public TreeNode MouseDownTreeNode
        {
            get { return theMouseDownTreeNode; }
            set { theMouseDownTreeNode = value; }
        }

        public NavigationTreeNameFilter TheNavigationTreeNameFilter
        {
            get { return theNavigationTreeNameFilter; }
            set
            {
                theNavigationTreeNameFilter = value;
                ApplyFilter();
            }
        }

        /// <summary>
        /// The current connection definition
        /// </summary>
        virtual public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                //If a node is selected in the navigation tree, returns the node's connection definition
                if (SelectedNode != null)
                {
                    if (SelectedNode is NavigationTreeNode)
                        return ((NavigationTreeNode)SelectedNode).TheConnectionDefinition;
                    if (SelectedNode is NavigationTreeFolder)
                        return ((NavigationTreeFolder)SelectedNode).TheConnectionDefinition;
                }
                else
                {
                    //If no node is selected, return the connection definition associated with the first node in the active systems folder
                    NavigationTreeConnectionsFolder connectionsFolder = (NavigationTreeConnectionsFolder)theConnectionsFolder.Nodes[0];
                    if (connectionsFolder != null)
                    {
                        return connectionsFolder.TheConnectionDefinition;
                    }
                }
                return null;
            }

            // Note: Set doesn't set the value. Instead it selects the Connection definition node that matches the name
            // of the ConnectionDefinition passed
            set
            {
                if ((value != null) && (value.Name != null))
                {
                    //Set the active system first
                    foreach (NavigationTreeConnectionFolder folder in theConnectionsFolder.Nodes)
                    {
                        if (folder.TheConnectionDefinition != null)
                        {
                            if (folder.TheConnectionDefinition.Name.Equals(value.Name))
                            {
                                Select(folder);
                                break;
                            }
                        }
                    }
                }
            }
        }

        #endregion public properties

        #region public methods

        public NavigationTreeView()
        {
            InitializeComponent();

            ShowNodeToolTips = true;

            HideSelection = false;
            if (theFilterChangedHandler == null)
            {
                theFilterChangedHandler = new NavigationTreeNameFilter.ChangedHandler(FilterChanged);
            }
            NavigationTreeNameFilter.Changed += theFilterChangedHandler;

            BeforeSelect += NavigationTreeView_BeforeSelect;
            AfterSelect += NavigationTree_AfterSelect;
            BeforeExpand += NavigationTree_BeforeExpand;
            ItemDrag += NavigationTreeView_ItemDrag;
            NodeMouseClick += NavigationTreeView_NodeMouseClick;
            MouseDown += NavigationTreeView_MouseDown;
        }

        public void SelectParent()
        {
            if ((SelectedNode != null) && (SelectedNode.Parent != null))
            {
                SelectedNode = SelectedNode.Parent;
            }
        }

        public void SelectPrevious()
        {
            if ((SelectedNode != null) && (SelectedNode.PrevNode != null))
            {
                SelectedNode = SelectedNode.PrevNode;
            }
        }

        public void SelectNext()
        {
            if ((SelectedNode != null) && (SelectedNode.NextNode != null))
            {
                SelectedNode = SelectedNode.NextNode;
            }
        }

        /// <summary>
        /// Sets the root folders and populates them.
        /// Overridden by the subclasses as needed to present a different view of the root folders
        /// </summary>
        virtual public void SetAndPopulateRootFolders()
        {
            Nodes.Clear();

            Nodes.Add(theConnectionsFolder);
            PopulateFolder(theConnectionsFolder);

            SelectedNode = theConnectionsFolder;
            ExpandTreeNode(SelectedNode);
        }

        public virtual void TreeNodePopulated(TreeNode node)
        {
            //Allows subclasses to do any post processing after folder is populate asynchronously.
        }

        public virtual TreeNode FindByFullPath(string aFullPath)
        {
            string[] theNodeNames = aFullPath.Split(PathSeparator.ToCharArray(0, 1));

            TreeNode theTreeNode = FindByFullPath(Nodes, theNodeNames, 0);

            return theTreeNode;

        }

        public void Select(TreeNode aTreeNode)
        {
            if (SelectedNode == aTreeNode)
            {
                SelectedNode = null;// aTreeNode.Parent;
            }
            SelectedNode = aTreeNode;
            Focus();
        }

        public void FireSelected(NavigationTreeNode aTreeNode)
        {
            try
            {
                SelectedHandler theSelectedHandlers = (SelectedHandler)theEventHandlers[theSelectedKey];

                if (theSelectedHandlers != null)
                {
                    theSelectedHandlers(aTreeNode);
                }

                //Notify the context that the user has selected a new connection
                TrafodionContext.Instance.OnConnectionDefinitionSelection(this);
            }
            catch (Exception e)
            {
                //Control foregroundControl = Utilities.GetForegroundControl();
                //if (foregroundControl != null)
                //{
                MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                //}
                //else
                //{
                //    MessageBox.Show(e.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                //}
            }

        }


        #endregion public methods

        #region private methods

        void NavigationTreeView_BeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            if (e.Node is DummyTreeNode || _editInProgress)
            {
                e.Cancel = true;
            }
        }

        ~NavigationTreeView()
        {
            Dispose(false);
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            switch (keyData)
            {
                case Keys.Control | Keys.C:
                case Keys.Control | Keys.Shift | Keys.C:
                    {
                        if (SelectedNode != null)
                        {
                            Clipboard.SetDataObject(this.SelectedNode.Text);
                        }
                        break;
                    }
            }
            return base.ProcessCmdKey(ref msg, keyData);
        }

        protected TreeNode FindByFullPath(TreeNodeCollection aNodes, string[] aNodeNames, int anOffset)
        {
            string theNodeName = aNodeNames[anOffset];
            foreach (TreeNode theTreeNode in aNodes)
            {
                if (theTreeNode.Text.Equals(theNodeName))
                {
                    if (anOffset == aNodeNames.Length - 1)
                    {
                        return theTreeNode;
                    }

                    // This needs to be marshalled onto the tree view's thread and we must wait for it
                    EndInvoke(BeginInvoke(new ExpandTreeNodeDelegate(ExpandTreeNode), new object[] { theTreeNode }));
                    
                    return FindByFullPath(theTreeNode.Nodes, aNodeNames, anOffset + 1);
                }
            }
            throw new FindByFullPathFailed(this, aNodeNames);
        }

        /// <summary>
        /// Given a parent node, finds the child node matching the relative path
        /// </summary>
        /// <param name="parentNode">Parent tree node</param>
        /// <param name="relativePath">relative path from the parent</param>
        /// <returns></returns>
        public TreeNode FindChildNodeByRelativePath(TreeNode parentNode, string relativePath)
        {
            string[] theNodeNames = relativePath.Split(PathSeparator.ToCharArray(0, 1));

            TreeNode theTreeNode = FindByFullPath(parentNode.Nodes, theNodeNames, 0);

            return theTreeNode;
        }
        
        protected void ExpandTreeNode(TreeNode aTreeNode)
        {
            if (aTreeNode is NavigationTreeFolder)
            {
                ((NavigationTreeFolder)aTreeNode).DoPopulate(null);
            }
            aTreeNode.Expand();
        }

        private void FilterChanged(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            TheNavigationTreeNameFilter = aNavigationTreeNameFilter;
        }


        private void NavigationTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            ShowActiveArea();
            FireSelected(e.Node as NavigationTreeNode);
        }

        private void NavigationTree_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            ShowActiveArea();
            if (e.Node !=null && e.Node is NavigationTreeFolder)
            {
                PopulateFolder(e.Node as NavigationTreeFolder);
            }
        }

        private void FireExceptionOccurred(Exception anException)
        {
            ExceptionOccurredHandler theExceptionOccurredHandlers = (ExceptionOccurredHandler)theEventHandlers[theExceptionOccurredKey];

            if (theExceptionOccurredHandlers != null)
            {
                theExceptionOccurredHandlers(anException);
            }
            else
            {
                MessageBox.Show(Utilities.GetForegroundControl(), anException.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
        }

        private void NavigationTreeView_ItemDrag(object sender, ItemDragEventArgs e)
        {
            if (FavoritesTreeView != null)
            {
                FavoritesTreeView.IsDragging = true;
                DoDragDrop(e.Item, DragDropEffects.Move);
            }
        }
        /// <summary>
        /// Instantiate a new ContextMenuStrip and have the Node fill the context menu items
        /// Display the custom context menu
        /// </summary>
        /// <param name="e"></param>
        private void ShowContextMenu(TreeNodeMouseClickEventArgs e)
        {
            TrafodionContextMenuStrip contextMenuStrip = new TrafodionContextMenuStrip();

            //Have the Tree Node fill its context menu items
            ((NavigationTreeNode)e.Node).AddToContextMenu(contextMenuStrip);

            contextMenuStrip.Show(this, new Point(e.X, e.Y));
        }

        private void NavigationTreeView_MouseDown(object sender, MouseEventArgs e)
        {
            MouseDownTreeNode = GetNodeAt(e.X, e.Y) as NavigationTreeNode;
        }

        private void NavigationTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            ShowActiveArea();
            if (e.Button == MouseButtons.Right)
            {
                if (SelectedNode != e.Node)
                {
                    SelectedNode = e.Node;
                }

                //Show context menu's only if this option is enabled
                if (AllowContextMenu)
                {
                    ShowContextMenu(e);
                }
            }
        }

        private void ShowActiveArea()
        {
            if (TrafodionContext.Instance.TheTrafodionMain != null)
            {
                TrafodionContext.Instance.TheTrafodionMain.HideFavoritesFolder();
            }
        }

        protected void PopulateFolder(NavigationTreeFolder aNavigationTreeFolder)
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                aNavigationTreeFolder.DoPopulateAsync(TheNavigationTreeNameFilter);
            }
            catch (Exception anException)
            {
                Cursor = Cursors.Default; //set default cursor before event handling
                FireExceptionOccurred(anException);
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        private bool ApplyFilter(TreeNode aTreeNode)
        {
            List<TreeNode> theNonMatches = new List<TreeNode>();

            foreach (TreeNode theChildTreeNode in aTreeNode.Nodes)
            {
                if (!ApplyFilter(theChildTreeNode))
                {
                    theNonMatches.Add(theChildTreeNode);
                }
            }
            foreach (TreeNode theNonMatchTreeNode in theNonMatches)
            {
                aTreeNode.Nodes.Remove(theNonMatchTreeNode);
            }

            return true;
        }

        private void ApplyFilter()
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                ApplyFilter(theTreeNode);
            }
        }

        private void InitializeComponent()
        {
            this.SuspendLayout();
            // 
            // NavigationTreeView
            // 
            this.LineColor = System.Drawing.Color.Black;
            this.ResumeLayout(false);

        }

        /// <summary>
        /// Method called when resources need be freed up--!! be careful about the reuse of freed up resources.
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            AfterSelect -= NavigationTree_AfterSelect;
            BeforeExpand -= NavigationTree_BeforeExpand;
            ItemDrag -= NavigationTreeView_ItemDrag;
            NodeMouseClick -= NavigationTreeView_NodeMouseClick;
            MouseDown -= NavigationTreeView_MouseDown;
            BeforeSelect -= NavigationTreeView_BeforeSelect;
            
            //Dispose the tree nodes
            DisposeTreeNodes(null, disposing);

            ////Not sure if need this code below anymore since the nodes have been disposed.
            //if (!this.disposed)
            //{
            //    //TODO: For some reason, when we exit the application. Nodes.Clear() throws
            //    //"Error creating window handle." Exception. Till we determine the reason 
            //    //for that, the folowing kludge will help hide the problem.
            //    try
            //    {
            //        if (Nodes != null && Nodes.Count > 0)
            //        {
            //            Nodes.Clear();
            //        }
            //        base.Dispose(disposing);
            //    }
            //    catch
            //    {
            //        //do nothing
            //    }
            //}
            this.disposed = true;
        }

        /// <summary>
        /// Dipose the tree nodes so they can do clean up.
        /// </summary>
        /// <param name="parentNode"></param>
        /// <param name="disposing"></param>
        void DisposeTreeNodes(TreeNode parentNode, bool disposing)
        {
            if (disposing)
            {
                TreeNodeCollection childNodes = parentNode != null ? parentNode.Nodes : Nodes;

                //Recurse through the Node collection and dispose each node
                //When the nodes are disposed, their dispose method will perform the required cleanup
                if (childNodes != null && childNodes.Count > 0)
                {
                    foreach (TreeNode childNode in childNodes)
                    {
                        DisposeTreeNodes(childNode, disposing); //dispose all grand children nodes first
                        if (childNode is NavigationTreeNode)
                        {
                            //Dipose method is an extension to NavigationTreeNode.
                            ((NavigationTreeNode)childNode).Dispose(disposing); //then dispose child
                        }
                    }
                }
            }
        }
        #endregion private methods
    }

    public class NavigationTreeNameFilter
    {
        public enum Where { IsExactly = 0, StartsWith = 1, Contains = 2, EndsWith = 3,All = 4 };
        
        public NavigationTreeNameFilter()
        {
        }

        public NavigationTreeNameFilter(Where aWhere, string aNamePart)
        {
            TheWhere = aWhere;
            TheNamePart = aNamePart;
        }

        public bool IsNoOp
        {
            get { return (TheWhere == Where.All); }
        }

        public delegate void ChangedHandler(NavigationTreeNameFilter aNavigationTreeNameFilter);

        static private EventHandlerList theEventHandlers = new EventHandlerList();

        private static readonly string theChangedKey = "Changed";

        static public event ChangedHandler Changed
        {
            add { theEventHandlers.AddHandler(theChangedKey, value); }
            remove { theEventHandlers.RemoveHandler(theChangedKey, value); }
        }

        protected void FireChanged()
        {
            FireChanged(this);
        }

        static private void FireChanged(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            if (aNavigationTreeNameFilter.PostponeChangeEvents)
            {
                aNavigationTreeNameFilter.PostponedChangeEvents = true;
                return;
            }

            ChangedHandler theChangedHandlers = (ChangedHandler)theEventHandlers[theChangedKey];

            if (theChangedHandlers != null)
            {
                theChangedHandlers(aNavigationTreeNameFilter);
            }
        }

        private bool postponedChangeEvents = false;

        public bool PostponedChangeEvents
        {
            get { return postponedChangeEvents; }
            set { postponedChangeEvents = value; }
        }

        private bool postponeChangeEvents = false;

        public bool PostponeChangeEvents
        {
            get { return postponeChangeEvents; }
            set
            {
                postponeChangeEvents = value;
                if (!PostponeChangeEvents && PostponedChangeEvents)
                {
                    PostponedChangeEvents = false;
                    FireChanged();
                }
            }
        }

        public bool Matches(NavigationTreeNode aNavigationTreeNode)
        {
            return Matches(aNavigationTreeNode.Text);
        }

        public bool Matches(string aString)
        {
            switch (TheWhere)
            {
                case Where.All:
                    {
                        return true;
                    }
                case Where.IsExactly:
                    {
                        return aString.Equals(theNamePart);
                    }
                case Where.StartsWith:
                    {
                        return aString.StartsWith(theNamePart);
                    }
                case Where.EndsWith:
                    {
                        return aString.EndsWith(theNamePart);
                    }
                case Where.Contains:
                    {
                        return aString.Contains(theNamePart);
                    }
                default:
                    {
                        return true;
                    }
            }
        }

        private Where theWhere = Where.All;

        public Where TheWhere
        {
            get { return theWhere; }
            set
            {
                theWhere = value;
                FireChanged();
            }
        }

        private string theNamePart = "";

        public string TheNamePart
        {
            get
            {
                return theNamePart;
            }
            set
            {
                theNamePart = value;
                FireChanged();
            }
        }

        private bool _IsHideTablesFolder = false;

        public bool IsHideTablesFolder
        {
            get { return _IsHideTablesFolder; }
            set { _IsHideTablesFolder = value; }
        }
        private bool _IsHideMaterializedViewsFolder = false;

        public bool IsHideMaterializedViewsFolder
        {
            get { return _IsHideMaterializedViewsFolder; }
            set { _IsHideMaterializedViewsFolder = value; }
        }
        private bool _IsHideMaterializedViewGroupsFolder = false;

        public bool IsHideMaterializedViewGroupsFolder
        {
            get { return _IsHideMaterializedViewGroupsFolder; }
            set { _IsHideMaterializedViewGroupsFolder = value; }
        }
        private bool _IsHideViewsFolder = false;

        public bool IsHideViewsFolder
        {
            get { return _IsHideViewsFolder; }
            set { _IsHideViewsFolder = value; }
        }
        private bool _IsHideSchemaIndexesFolder = false;

        public bool IsHideSchemaIndexesFolder
        {
            get { return _IsHideSchemaIndexesFolder; }
            set { _IsHideSchemaIndexesFolder = value; }
        }
        private bool _IsHideProceduresFolder = false;

        public bool IsHideProceduresFolder
        {
            get { return _IsHideProceduresFolder; }
            set { _IsHideProceduresFolder = value; }
        }
        private bool _IsHideLibrariesFolder = false;

        public bool IsHideLibrariesFolder
        {
            get { return _IsHideLibrariesFolder; }
            set { _IsHideLibrariesFolder = value; }
        }
        private bool _IsHideSynonymsFolder = false;

        public bool IsHideSynonymsFolder
        {
            get { return _IsHideSynonymsFolder; }
            set { _IsHideSynonymsFolder = value; }
        }
        private bool _IsHideFunctionsFolder = false;

        public bool IsHideFunctionsFolder
        {
            get { return _IsHideFunctionsFolder; }
            set { _IsHideFunctionsFolder = value; }
        }
        private bool _IsHideFunctionActionsFolder = false;

        public bool IsHideFunctionActionsFolder
        {
            get { return _IsHideFunctionActionsFolder; }
            set { _IsHideFunctionActionsFolder = value; }
        }
        private bool _IsHideTableMappingFunctionFolder = false;

        public bool IsHideTableMappingFunctionFolder
        {
            get { return _IsHideTableMappingFunctionFolder; }
            set { _IsHideTableMappingFunctionFolder = value; }
        }


    }

    public class NavigationTreeConnectionFolderFactory
    {
        virtual public NavigationTreeConnectionFolder NewNavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            return new NavigationTreeConnectionFolder(aConnectionDefinition);
        }

        virtual public NavigationTreeConnectionFolder UpdateNavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition,
            NavigationTreeConnectionFolder connectionTreeFolder)
        {
            return connectionTreeFolder;
        }
    }

    public class FindByFullPathFailed : Exception
    {
        public FindByFullPathFailed(TreeView aTreeView, string[] aNames)
        {
            StringBuilder theStringBuilder = new StringBuilder();
            foreach (string theName in aNames)
            {
                if (theStringBuilder.Length > 0)
                {
                    theStringBuilder.Append(aTreeView.PathSeparator);
                }
                theStringBuilder.Append(theName);
            }
            theFullPath = theStringBuilder.ToString();
        }

        private string theFullPath;

        public string FullPath
        {
            get { return theFullPath; }
            set { theFullPath = value; }
        }

        public override string ToString()
        {
            return Message;
        }

        public override string Message
        {
            get
            {
                return "Cannot find " + FullPath;
            }
        }

    }

}
