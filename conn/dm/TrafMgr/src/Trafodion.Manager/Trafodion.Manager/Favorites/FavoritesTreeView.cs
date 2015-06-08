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
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;

namespace Trafodion.Manager.Framework.Favorites
{
    [ToolboxItem(false)]
    public class FavoritesTreeView : TrafodionTreeView
    {

        public FavoritesTreeView()
        {
            InitializeComponent();

            ShowNodeToolTips = true;

            HideSelection = false;

            theRootFavoritesFolder = new FavoritesFolder();
            Nodes.Add(theRootFavoritesFolder);

            Enter += new EventHandler(FavoritesTreeView_Enter);
            DragDrop += new DragEventHandler(FavoritesTreeView_DragDrop);
            DragEnter += new DragEventHandler(FavoritesTreeView_DragEnter);
            DragOver += new DragEventHandler(FavoritesTreeView_DragOver);
            DragLeave += new EventHandler(FavoritesTreeView_DragLeave);
            ItemDrag += new ItemDragEventHandler(FavoritesTreeView_ItemDrag);

            AfterSelect += new TreeViewEventHandler(FavoritesTreeView_AfterSelect);
            NodeMouseClick += new TreeNodeMouseClickEventHandler(FavoritesTreeView_NodeMouseClick);
            MouseDown += new MouseEventHandler(FavoritesTreeView_MouseDown);

            theNewFolderMenuItem.Click += new EventHandler(theNewFolderMenuItem_Click);
            theRemoveMenuItem.Click += new EventHandler(theRemoveMenuItem_Click);
        }

        public void Dump()
        {

            MemoryStream theMemoryStream = new MemoryStream();
            BinaryFormatter theBinaryFormatter = new BinaryFormatter();
            theBinaryFormatter.Serialize(theMemoryStream, new FavoritesTreeViewReflection(this));

            FavoritesTreeViewReflection theTreeViewReflection;

//            AppDomain.CurrentDomain.AssemblyResolve += NCCAssemblyResolver.Handler;

            try
            {
                theMemoryStream.Position = 0;
                theTreeViewReflection = (FavoritesTreeViewReflection)theBinaryFormatter.Deserialize(theMemoryStream);
                FavoritesTreeView theFavoritesTreeView = theTreeViewReflection.MakeFavoritesTreeView(NavigationTreeView);
                TrafodionForm theForm = new TrafodionForm();
                theFavoritesTreeView.Dock = DockStyle.Fill;
                theForm.Controls.Add(theFavoritesTreeView);
                theForm.Show();

            }
            finally
            {
//                AppDomain.CurrentDomain.AssemblyResolve -= NCCAssemblyResolver.Handler;
            }


        }

        private TrafodionContextMenuStrip theContextMenu;
        private System.ComponentModel.IContainer components;
        private ToolStripMenuItem theNewFolderMenuItem;
        private ToolStripMenuItem theRemoveMenuItem;

        private NavigationTreeView theNavigationTreeView = null;

        public NavigationTreeView NavigationTreeView
        {
            get { return theNavigationTreeView; }
            set
            {
                theNavigationTreeView = value;
                AllowDrop = (value != null);
                ImageList = theNavigationTreeView.ImageList;
                //ImageList = (value != null) ? theTrafodionTreeControl.TheImageList : null;
            }
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
            if ((theTreeNode != null) && (theTreeNode is FavoritesFolder))
            {
                FavoritesFolder theFavoritesFolder = theTreeNode as FavoritesFolder;
                AddToFavorites(theFavoritesFolder, NavigationTreeView.MouseDownTreeNode);
            }
            IsDragging = false;
        }

        private void FavoritesTreeView_DragEnter(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.Move;
        }

        private void FavoritesTreeView_DragLeave(object sender, EventArgs e)
        {
        }

        private void FavoritesTreeView_DragOver(object sender, DragEventArgs e)
        {
            TreeNode theTreeNode = GetNodeAt(PointToClient(new Point(e.X, e.Y)));

            if (theTreeNode is FavoritesFolder)
            {
                e.Effect = DragDropEffects.Move;
            }
            else
            {
                e.Effect = DragDropEffects.None;
            }
        }

        private void FavoritesTreeView_ItemDrag(object sender, ItemDragEventArgs e)
        {

        }

        // Add to current favorites folder
        public void AddToFavorites(TreeNode aTreeNode)
        {
            FavoritesFolder theFavoritesFolder;

            // Check to see if there is a selection
            if (SelectedNode == null)
            {

                // No selection, use the root
                theFavoritesFolder = (FavoritesFolder)TopNode;

            }
            else
            {

                // There is a selection.  Check to see if it is a folder.
                TreeNode theTreeNode = SelectedNode;
                while (!(theTreeNode is FavoritesFolder))
                {

                    // Not a folder, try the parent
                    theTreeNode = theTreeNode.Parent;

                }

                // We now have a node that is a folder
                theFavoritesFolder = theTreeNode as FavoritesFolder;
            }

            // Now add to it
            AddToFavorites(theFavoritesFolder, aTreeNode);

        }

        // Add to specific favorites folder
        private void AddToFavorites(FavoritesFolder aFavoritesFolder, TreeNode aTreeNode)
        {
            Favorite theFavorite = null;

            //// Check to see if it is already in this favorites folder
            //if (aFavoritesFolder.ContainsFavoriteFor(aTreeNode))
            //{

            //    // It is ... we will not create another
            //    theFavorite = aFavoritesFolder.GetFavoriteFor(aTreeNode);

            //}
            //else
            {
                FavoritesFolder theFavoritesFolder = (aFavoritesFolder != null) ? aFavoritesFolder : theRootFavoritesFolder;
                if ((aTreeNode != null) && (aTreeNode is NavigationTreeNode) &&
                    (this.NavigationTreeView.CurrentConnectionDefinition != null))
                {
                    theFavorite = new Favorite(NavigationTreeView, (NavigationTreeNode)aTreeNode);
                    FavoriteNameDialog theFavoriteNameDialog = new FavoriteNameDialog("New Favorite", "Please enter a name for the favorite ...");
                    theFavoriteNameDialog.TheString = this.NavigationTreeView.CurrentConnectionDefinition.Name + ": " + theFavorite.Text;
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
                NavigationTreeView.Select(theFavorite.NavigationTreeNode);
                NavigationTreeView.FireSelected(theFavorite.NavigationTreeNode);
                SelectedNode = theFavorite;
                Focus();
            }

        }


        private void RemoveFromFavorites(FavoritesTreeNode aFavoriteTreeNode)
        {

            if (aFavoriteTreeNode is FavoritesFolder)
            {
                FavoritesFolder theFavoritesFolder = aFavoriteTreeNode as FavoritesFolder;
                foreach (TreeNode theTreeNode in theFavoritesFolder.Nodes)
                {
                    RemoveFromFavorites(theTreeNode as FavoritesTreeNode);
                }
                theFavoritesFolder.Parent.Nodes.Remove(theFavoritesFolder);
                ShowFavorite(theFavoritesFolder.Parent as FavoritesFolder);
            }
            else if (aFavoriteTreeNode is Favorite)
            {
                Favorite theFavorite = aFavoriteTreeNode as Favorite;
                theFavorite.Parent.Nodes.Remove(theFavorite);
                ShowFavorite(theFavorite.Parent as FavoritesFolder);
            }

        }

        private FavoritesFolder AddNewFavoritesFolder(FavoritesFolder aFavoritesFolder)
        {
            FavoriteNameDialog theFavoriteNameDialog = new FavoriteNameDialog("New Favorites Folder", "Please enter a name for the folder ...");
            if (theFavoriteNameDialog.ShowDialog() != DialogResult.OK)
            {
                return null;
            }

            FavoritesFolder theNewFavoritesFolder = new FavoritesFolder(theFavoriteNameDialog.TheString);
            aFavoritesFolder.Nodes.Add(theNewFavoritesFolder);
            SelectedNode = theNewFavoritesFolder;
            return theNewFavoritesFolder;
        }

        private bool isDragging = false;

        public bool IsDragging
        {
            get { return isDragging; }
            set { isDragging = value; }
        }

        private FavoritesFolder theRootFavoritesFolder = null;

        private void FavoritesTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            ShowFavorite(e.Node);
        }

        private void ShowFavorite(TreeNode aTreeNode)
        {
            if (aTreeNode is Favorite)
            {
                Favorite theFavorite = aTreeNode as Favorite;
                try
                {
                    NavigationTreeView.Select(theFavorite.NavigationTreeNode);
                }
                catch (CannotResolveFavorite ex)
                {
                    bool connectionDefFound = false;
                    string connectionName = theFavorite.SystemName;

                    if (string.IsNullOrEmpty(theFavorite.SystemName))
                    {
                        connectionName = GetSystemName(theFavorite.NavigationTreeNodeFullPath);
                    }

                    foreach(ConnectionDefinition aConnDefn in ConnectionDefinition.ConnectionDefinitions)
                    {
                        if (!aConnDefn.Name.Equals(connectionName))
                            continue;

                        connectionDefFound = true;

                        if (aConnDefn.TheState != ConnectionDefinition.State.TestSucceeded)
                        {
                            ConnectionDefinitionDialog connDialog = new ConnectionDefinitionDialog();
                            connDialog.Edit(aConnDefn);
                        }
                        if (aConnDefn.TheState == ConnectionDefinition.State.TestSucceeded)
                        {
                            try
                            {
                                NavigationTreeView.Select(theFavorite.NavigationTreeNode);
                            }
                            catch (CannotResolveFavorite ex1)
                            {
                                string msg = String.Format(Properties.Resources.FavoritesNotFoundError, new object[] {
                                            theFavorite.Text, theFavorite.NavigationTreeNodeFullPath,"\n\n" });
                                MessageBox.Show(Utilities.GetForegroundControl(), msg, Properties.Resources.FavoritesErrorHeader, MessageBoxButtons.OK);
                            }
                        }
                        else
                        {
                            ex.ShowMessageBox(); //Error selecting the favorites.
                        }
                    }

                    if(!connectionDefFound)
                    {
                        string msg = String.Format(Properties.Resources.FavoritesNotFoundError, new object[] {
                                            theFavorite.Text, theFavorite.NavigationTreeNodeFullPath,"\n\n" });
                        MessageBox.Show(Utilities.GetForegroundControl(), msg, Properties.Resources.FavoritesErrorHeader, MessageBoxButtons.OK);
                    }
                }
            }
            else if (aTreeNode is FavoritesFolder)
            {
                FavoritesFolder theFavoritesFolder = aTreeNode as FavoritesFolder;

                if (TrafodionContext.Instance.TheTrafodionMain != null)
                {
                    TrafodionContext.Instance.TheTrafodionMain.ShowFavoritesFolder(theFavoritesFolder);
                }

            }
        }

        private string GetSystemName(string fullNodePath)
        {
            string[] nameParts = fullNodePath.Split(PathSeparator.ToCharArray(0, 1));
            if (nameParts.Length >= 2)
                return nameParts[1]; //system name is 2nd part. 1st part is the MySystems folder name.

            return "";
        }

        private void FavoritesTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                SelectedNode = e.Node;
                ShowContextMenu(e);
            }

            else if (e.Button == MouseButtons.Left)
            {
                if (SelectedNode == e.Node)
                {
                    ShowFavorite(e.Node);
                }
                else
                {
                    SelectedNode = e.Node;
                }
            }
        }

        private void ShowContextMenu(TreeNodeMouseClickEventArgs e)
        {
            theNewFolderMenuItem.Visible = (e.Node is FavoritesFolder);
            theRemoveMenuItem.Visible = (e.Node != theRootFavoritesFolder);
            theContextMenu.Show(this, new Point(e.X, e.Y));
        }

        private void theNewFolderMenuItem_Click(object sender, EventArgs e)
        {
            AddNewFavoritesFolder(theMouseDownTreeNode as FavoritesFolder);
        }

        private void theRefreshMenuItem_Click(object sender, EventArgs e)
        {
        }

        private void theRemoveMenuItem_Click(object sender, EventArgs e)
        {
            RemoveFromFavorites(theMouseDownTreeNode as FavoritesTreeNode);
        }

        private void FavoritesTreeView_MouseDown(object sender, MouseEventArgs e)
        {
            theMouseDownTreeNode = GetNodeAt(e.X, e.Y) as FavoritesTreeNode;
        }

        private FavoritesTreeNode theMouseDownTreeNode = null;

        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.theContextMenu = new TrafodionContextMenuStrip(this.components);
            this.theNewFolderMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.theRemoveMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.theContextMenu.SuspendLayout();
            this.SuspendLayout();
            // 
            // theContextMenu
            // 
            this.theContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.theNewFolderMenuItem,
            this.theRemoveMenuItem});
            this.theContextMenu.Name = "theContextMenu";
            this.theContextMenu.Size = new System.Drawing.Size(155, 48);
            // 
            // theNewFolderMenuItem
            // 
            this.theNewFolderMenuItem.Name = "theNewFolderMenuItem";
            this.theNewFolderMenuItem.Size = new System.Drawing.Size(154, 22);
            this.theNewFolderMenuItem.Text = "New Folder ...";
            // 
            // theRemoveMenuItem
            // 
            this.theRemoveMenuItem.Name = "theRemoveMenuItem";
            this.theRemoveMenuItem.Size = new System.Drawing.Size(154, 22);
            this.theRemoveMenuItem.Text = "Remove";
            // 
            // FavoritesTreeView
            // 
            this.LineColor = System.Drawing.Color.Black;
            this.theContextMenu.ResumeLayout(false);
            this.ResumeLayout(false);

        }

    }

    //[Serializable]
    //public class FavoriteTreeViewReflection : FavoritesTreeViewReflection
    //{
    //    public FavoriteTreeViewReflection(FavoritesTreeView aFavoritesTreeView) : base(aFavoritesTreeView)
    //    {
    //    }
    //}

    //[Serializable]
    //public class FavoriteTreeNodeReflection : FavoritesTreeNodeReflection
    //{
    //    private string theTargetPath;

    //    public string TargetPath
    //    {
    //        get { return theTargetPath; }
    //        set { theTargetPath = value; }
    //    }

    //}

    //[Serializable]
    //public class FavoritesFolderTreeNodeReflection : FavoritesTreeNodeReflection
    //{
    //}

}
