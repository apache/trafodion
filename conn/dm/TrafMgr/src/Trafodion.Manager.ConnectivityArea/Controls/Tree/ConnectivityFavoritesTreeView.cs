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
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// </summary>
    [ToolboxItem(false)]
    public class ConnectivityFavoritesTreeView : Trafodion.Manager.Framework.Favorites.FavoritesTreeView
//    public class ConnectivityFavoritesTreeView : TrafodionTreeView //Trafodion.Manager.Framework.Favorites.FavoritesTreeView
    {
        private ConnectivityFavoritesTreeFolder theRootFavoritesFolder = null;
        private ConnectivityFavoritesTreeNode theMouseDownTreeNode = null;

        private ToolStripMenuItem theNewFolderMenuItem;
        private ToolStripMenuItem theRemoveMenuItem;
        private ToolStripMenuItem theRenameMenuItem;

        private TrafodionContextMenuStrip theContextMenu;
        private System.ComponentModel.IContainer components;

        private ConnectivityTreeView theNavigationTreeView = null;

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityTreeView ConnectivityTreeView
        {
            get { return theNavigationTreeView; }
            set
            {
                theNavigationTreeView = value;
                AllowDrop = (value != null);
            }
        }


        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTreeNode"></param>
        public delegate void SelectedHandler(ConnectivityFavoritesTreeNode aTreeNode);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="anException"></param>
        public delegate void ExceptionOccurredHandler(Exception anException);

        private EventHandlerList theEventHandlers = new EventHandlerList();

        private static readonly string theSelectedKey = "FavoritesSelected";
        private static readonly string theExceptionOccurredKey = "ExceptionOccurred";

        /// <summary>
        /// 
        /// </summary>
        public event SelectedHandler FavoritesSelected
        {
            add { theEventHandlers.AddHandler(theSelectedKey, value); }
            remove { theEventHandlers.RemoveHandler(theSelectedKey, value); }
        }

        /// <summary>
        /// 
        /// </summary>
        public event ExceptionOccurredHandler ExceptionOccurred
        {
            add { theEventHandlers.AddHandler(theExceptionOccurredKey, value); }
            remove { theEventHandlers.RemoveHandler(theExceptionOccurredKey, value); }
        }

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityFavoritesTreeView()
        {

            InitializeComponent();

            ShowNodeToolTips = true;

            HideSelection = false;

            // only create the Root Favorites Folder with this default constructor
            theRootFavoritesFolder = new ConnectivityFavoritesTreeFolder();
            Nodes.Add(theRootFavoritesFolder);

            Enter += new EventHandler(FavoritesTreeView_Enter);
            DragDrop += new DragEventHandler(FavoritesTreeView_DragDrop);
            DragEnter += new DragEventHandler(FavoritesTreeView_DragEnter);
            DragOver += new DragEventHandler(FavoritesTreeView_DragOver);
            DragLeave += new EventHandler(FavoritesTreeView_DragLeave);
            ItemDrag += new ItemDragEventHandler(FavoritesTreeView_ItemDrag);

            AfterSelect += new TreeViewEventHandler(FavoritesTreeView_AfterSelect);
            NodeMouseClick += new TreeNodeMouseClickEventHandler(FavoritesTreeView_NodeMouseClick);
            NodeMouseDoubleClick += new TreeNodeMouseClickEventHandler(FavoritesTreeView_NodeMouseDoubleClick);
            MouseDown += new MouseEventHandler(FavoritesTreeView_MouseDown);

            theNewFolderMenuItem.Click += new EventHandler(theNewFolderMenuItem_Click);
            theRemoveMenuItem.Click += new EventHandler(theRemoveMenuItem_Click);
            theRenameMenuItem.Click += new EventHandler(theRenameMenuItem_Click);


        }

        private ConnectivityFavoritesTreeFolder AddNewFavoritesFolder(ConnectivityFavoritesTreeFolder aFavoritesFolder)
        {
            FavoriteNameDialog theFavoriteNameDialog = new FavoriteNameDialog("New Favorites Folder", "Please enter a name for the folder ...");
            if (theFavoriteNameDialog.ShowDialog() != DialogResult.OK)
            {
                return null;
            }

            ConnectivityFavoritesTreeFolder theNewFavoritesFolder = new ConnectivityFavoritesTreeFolder(theFavoriteNameDialog.TheString);
            aFavoritesFolder.Nodes.Add(theNewFavoritesFolder);
            SelectedNode = theNewFavoritesFolder;
            return theNewFavoritesFolder;
        }

        /// <summary>
        /// Remove selected node from My Favorites tree
        /// </summary>
        /// <param name="aFavoriteTreeNode"></param>
        private void RemoveFromFavorites(ConnectivityFavoritesTreeNode aFavoriteTreeNode)
        {

            if (aFavoriteTreeNode is ConnectivityFavoritesTreeFolder)
            {
                ConnectivityFavoritesTreeFolder theFavoritesFolder = aFavoriteTreeNode as ConnectivityFavoritesTreeFolder;
                foreach (TreeNode theTreeNode in theFavoritesFolder.Nodes)
                {
                    RemoveFromFavorites(theTreeNode as ConnectivityFavoritesTreeNode);
                }
                theFavoritesFolder.Parent.Nodes.Remove(theFavoritesFolder);
                ShowFavorite(theFavoritesFolder.Parent as ConnectivityFavoritesTreeFolder);
            }
            else if (aFavoriteTreeNode is ConnectivityFavorite)
            {
                ConnectivityFavorite theFavorite = aFavoriteTreeNode as ConnectivityFavorite;
                theFavorite.Parent.Nodes.Remove(theFavorite);
                ShowFavorite(theFavorite.Parent as ConnectivityFavoritesTreeFolder);
            }

        }




        /// <summary>
        /// Rename selected node from My Favorites tree
        /// </summary>
        /// <param name="aFavoriteTreeNode"></param>
        private void RenameFavoritesTreeNode(ConnectivityFavoritesTreeNode aFavoriteTreeNode)
        {
            aFavoriteTreeNode.EnsureVisible();
            aFavoriteTreeNode.BeginEdit();
        }



        /// <summary>
        /// Selects the tree node that contains the Connectivity object
        /// </summary>
        /// <param name="aConnectivityObject">The Connectivity object to select</param>
        /// <returns>True if selection was successful, else returns false</returns>
        public bool SelectConnectivityObject(NDCSObject aConnectivityObject)
        {
            NavigationTreeConnectionFolder connectionFolder = ExpandConnection(aConnectivityObject.ConnectionDefinition);
            ConnectivitySystemFolder systemFolder = connectionFolder as ConnectivitySystemFolder;
            if (systemFolder != null)
            {
                return systemFolder.SelectConnectivityObject(aConnectivityObject);
            }
            return false;
        }

        private bool isDragging = false;

        public bool IsDragging
        {
            get { return isDragging; }
            set { isDragging = value; }
        }


        private void FavoritesTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            ShowFavorite(e.Node);
        }

        private void FavoritesTreeView_Enter(object sender, EventArgs e)
        {
            // I do not remember why this is here.  If it is uncommented nodes in the nav tree
            // get selected twice when a fave is clicked

            //if (NavigationTreeView == null)
            //{
            //    return;
            //}

            //if (!IsDragging && (SelectedNode != null) && (SelectedNode is Favorite))
            //{
            //    Favorite theFavorite = (Favorite)SelectedNode;
            //    try
            //    {
            //        NavigationTreeView.FireSelected(theFavorite.NavigationTreeNode);
            //    }
            //    catch (CannotResolveFavorite ex)
            //    {
            //        ex.ShowMessageBox();
            //    }
            //}
        }

        private void FavoritesTreeView_DragDrop(object sender, DragEventArgs e)
        {
            TreeNode theTreeNode = GetNodeAt(PointToClient(new Point(e.X, e.Y)));
            if ((theTreeNode != null) && (theTreeNode is ConnectivityFavoritesTreeFolder))
            {
                ConnectivityFavoritesTreeFolder theFavoritesFolder = theTreeNode as ConnectivityFavoritesTreeFolder;
                AddToFavorites(theFavoritesFolder, ConnectivityTreeView.MouseDownTreeNode);
            }
            IsDragging = false;
        }


        private void FavoritesTreeView_DragEnter(object sender, DragEventArgs e)
        {
            // SK - need to know what kind of object is entering into Favorites Tree view:
            if (ConnectivityTreeView.MouseDownTreeNode is ConnectivityTreeNode)
            {
                e.Effect = DragDropEffects.Move;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void FavoritesTreeView_DragOver(object sender, DragEventArgs e)
        {
            TreeNode theTreeNode = GetNodeAt(PointToClient(new Point(e.X, e.Y)));

            if (theTreeNode is ConnectivityFavoritesTreeFolder && 
                ConnectivityTreeView.MouseDownTreeNode is ConnectivityTreeNode)
            {
                e.Effect = DragDropEffects.Move;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void FavoritesTreeView_DragLeave(object sender, EventArgs e)
        {
            // SK - to do: move the node to some other Favorites folder or remove it from the Favorites tree
        }

        private void FavoritesTreeView_ItemDrag(object sender, ItemDragEventArgs e)
        {
            // SK - to do: handle drag tree nodes into different order...

        }

        private void FavoritesTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(e);
            }
            else if (e.Button == MouseButtons.Left)
            {
                //ShowFavorite(e.Node);
                SelectedNode = e.Node;
            }
        }

        private void FavoritesTreeView_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            try
            {
                if (e.Node is ConnectivityFavoritesTreeFolder)
                {
                    MessageBox.Show("double click this: " + e.Node.Text);
                }
            }
            // handle the execption and inform the user.
            catch (System.ComponentModel.Win32Exception)
            {
                MessageBox.Show("sing in required.....");
            }

        }

        
        private void FavoritesTreeView_MouseDown(object sender, MouseEventArgs e)
        {
            theMouseDownTreeNode = GetNodeAt(e.X, e.Y) as ConnectivityFavoritesTreeNode;
        }

        private void theNewFolderMenuItem_Click(object sender, EventArgs e)
        {
            AddNewFavoritesFolder(theMouseDownTreeNode as ConnectivityFavoritesTreeFolder);
        }

        private void theRefreshMenuItem_Click(object sender, EventArgs e)
        {
        }

        private void theRemoveMenuItem_Click(object sender, EventArgs e)
        {
            RemoveFromFavorites(theMouseDownTreeNode as ConnectivityFavoritesTreeNode);
        }

        private void theRenameMenuItem_Click(object sender, EventArgs e)
        {
            RenameFavoritesTreeNode(theMouseDownTreeNode as ConnectivityFavoritesTreeNode);
        
        }

        private void ShowFavorite(TreeNode aTreeNode)
        {
            if (aTreeNode is ConnectivityFavorite)
            {
                ConnectivityFavorite theFavorite = aTreeNode as ConnectivityFavorite;
                try
                {
                    ConnectivityTreeView.Select(theFavorite.NavigationTreeNode);
                }
                catch (CannotResolveFavorite ex)
                {
                    ex.ShowMessageBox();
                }
            }
            else if (aTreeNode is ConnectivityFavoritesTreeFolder)
            {
                ConnectivityFavoritesTreeFolder theFavoritesFolder = aTreeNode as ConnectivityFavoritesTreeFolder;

                SelectedHandler theSelectedFavoritesHandlers = (SelectedHandler)theEventHandlers[theSelectedKey];
                if (theSelectedFavoritesHandlers != null)
                {
                    theSelectedFavoritesHandlers(theFavoritesFolder);
                }

                
                //if (TrafodionContext.Instance.TheTrafodionMain != null)
                //{
                //    TrafodionContext.Instance.TheTrafodionMain.ShowFavoritesFolder((FavoritesFolder)theFavoritesFolder);
                //}

            }
        }


        // Add to specific favorites folder
        private void AddToFavorites(ConnectivityFavoritesTreeFolder aFavoritesFolder, TreeNode aTreeNode)
        {
            ConnectivityFavorite theFavorite = null;

            //// Check to see if it is already in this favorites folder
            //if (aFavoritesFolder.ContainsFavoriteFor(aTreeNode))
            //{

            //    // It is ... we will not create another
            //    theFavorite = aFavoritesFolder.GetFavoriteFor(aTreeNode);

            //}
            //else
            {
                ConnectivityFavoritesTreeFolder theFavoritesFolder = (aFavoritesFolder != null) ? aFavoritesFolder : theRootFavoritesFolder;
                if ((aTreeNode != null) && (aTreeNode is ConnectivityTreeNode))
                {
                    theFavorite = new ConnectivityFavorite(ConnectivityTreeView, (ConnectivityTreeNode)aTreeNode);
                    FavoriteNameDialog theFavoriteNameDialog = new FavoriteNameDialog("New Favorite", "Please enter a name for the favorite ...");
                    theFavoriteNameDialog.TheString = ConnectivityTreeView.CurrentConnectionDefinition.Name + ": " + theFavorite.Text;
                    while (true)
                    {
                        if (theFavoriteNameDialog.ShowDialog() != DialogResult.OK)
                        {
                            return;
                        }
                        if (theFavoritesFolder.Contains(theFavoriteNameDialog.TheString))
                        {
                            if (MessageBox.Show(Utilities.GetForegroundControl(), "The name is already in use in this favorites folder", "Name already in use", MessageBoxButtons.RetryCancel, MessageBoxIcon.Stop) == DialogResult.Cancel)
                            {
                                return;
                            }
                            continue;
                        }
                        break;
                    }
                    theFavorite.Text = theFavoriteNameDialog.TheString;
                    theFavoritesFolder.Nodes.Add(theFavorite);
                }
            }

            if (theFavorite != null)
            {
                theFavorite.Parent.Expand();
                ConnectivityTreeView.Select(theFavorite.NavigationTreeNode);
                ConnectivityTreeView.FireSelected(theFavorite.NavigationTreeNode);
                SelectedNode = theFavorite;
                Focus();
            }

        }

        /// <summary>
        /// Adds context menu items for the service folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            //base.AddToContextMenu(aContextMenuStrip);

            //ToolStripMenuItem startNDCSServiceMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_Start);
            //startNDCSServiceMenuItem.Tag = this;
            //startNDCSServiceMenuItem.Click += new EventHandler(startNDCSServiceMenuItem_Click);

            //ToolStripMenuItem stopNDCSServiceMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_Stop);
            //stopNDCSServiceMenuItem.Tag = this;
            //stopNDCSServiceMenuItem.Click += new EventHandler(stopNDCSServiceMenuItem_Click);

            //startNDCSServiceMenuItem.Enabled = false;
            //stopNDCSServiceMenuItem.Enabled = true;
            //aContextMenuStrip.Items.Add(startNDCSServiceMenuItem);
            //aContextMenuStrip.Items.Add(stopNDCSServiceMenuItem);
        }


        private void ShowContextMenu(TreeNodeMouseClickEventArgs e)
        {
            // Only one level of folder
            theNewFolderMenuItem.Visible = (e.Node.Level == 0);
            // Only child level can be removed
            theRemoveMenuItem.Visible = (e.Node.Level != 0);
            theRenameMenuItem.Visible = (e.Node.Level != 0);

            theContextMenu.Show(this, new Point(e.X, e.Y));
        }

        
        /// <summary>
        /// Expands the connection folder and returns the connection folder
        /// </summary>
        /// <param name="aConnectionDefinition">Connection Definition</param>
        /// <returns>Connection folder</returns>
        public NavigationTreeConnectionFolder ExpandConnection(ConnectionDefinition aConnectionDefinition)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is NavigationTreeMyActiveSystemsFolder)
                {
                    NavigationTreeMyActiveSystemsFolder theNavigationTreeMyActiveSystemsFolder = theTreeNode as NavigationTreeMyActiveSystemsFolder;
                    NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = theNavigationTreeMyActiveSystemsFolder.FindConnectionFolder(aConnectionDefinition);
                    if (theNavigationTreeConnectionFolder != null)
                    {
                        return theNavigationTreeConnectionFolder;
                    }
                }
            }
            throw new Exception("Not found in active systems: " + aConnectionDefinition.Name);
        }

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.theContextMenu = new TrafodionContextMenuStrip(this.components);
            this.theNewFolderMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.theRemoveMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.theRenameMenuItem = new System.Windows.Forms.ToolStripMenuItem();

            this.theContextMenu.SuspendLayout();
            this.SuspendLayout();
            // 
            // theContextMenu
            // 
            this.theContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.theNewFolderMenuItem,
            this.theRemoveMenuItem,
            this.theRenameMenuItem});
            this.theContextMenu.Name = "theContextMenu";
            this.theContextMenu.Size = new System.Drawing.Size(155, 48);
            // 
            // theNewFolderMenuItem
            // 
            this.theNewFolderMenuItem.Name = "theNewFolderMenuItem";
            this.theNewFolderMenuItem.Size = new System.Drawing.Size(154, 22);
            this.theNewFolderMenuItem.Enabled = true;
            this.theNewFolderMenuItem.Text = "New Favorites Folder ...";
            // 
            // theRemoveMenuItem
            // 
            this.theRemoveMenuItem.Name = "theRemoveMenuItem";
            this.theRemoveMenuItem.Size = new System.Drawing.Size(154, 22);
            this.theRemoveMenuItem.Enabled = true;
            this.theRemoveMenuItem.Text = "Remove";
            // 
            // theRemoveMenuItem
            // 
            this.theRenameMenuItem.Name = "theRenameMenuItem";
            this.theRenameMenuItem.Size = new System.Drawing.Size(154, 22);
            this.theRenameMenuItem.Enabled = true;
            this.theRenameMenuItem.Text = "Rename";
            // 
            // FavoritesTreeView
            // 
            this.LineColor = System.Drawing.Color.Black;
            this.theContextMenu.ResumeLayout(false);
            this.ResumeLayout(false);

        }

    }

}
