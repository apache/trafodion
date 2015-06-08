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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Favorites;

namespace Trafodion.Manager.OverviewArea.Controls.Tree
{
    /// <summary>
    /// </summary>
    public class OverviewTreeView : Trafodion.Manager.Framework.Navigation.NavigationTreeView
    {
        public event EventHandler RefreshRequestedEvent;

        //ConnectivityFavoritesTreeView theConnectivityFavoritesTreeView = null;
        FavoritesTreeView theConnectivityFavoritesTreeView = null;

        //public ConnectivityFavoritesTreeView ConnectivityFavoritesTreeView
        public FavoritesTreeView ConnectivityFavoritesTreeView
        {
            get { return theConnectivityFavoritesTreeView; }
            set { theConnectivityFavoritesTreeView = value; }
        }


        /// <summary>
        /// 
        /// </summary>
        public OverviewTreeView()
        {
            ItemDrag += new ItemDragEventHandler(OverviewTreeView_ItemDrag);
            //MouseDown += new MouseEventHandler(OverviewTreeView_MouseDown);
            //BeforeExpand += new TreeViewCancelEventHandler(OverviewTreeView_BeforeExpand);
            //BeforeCollapse += new TreeViewCancelEventHandler(OverviewTreeView_BeforeCollapse);
            //AfterCollapse += new TreeViewEventHandler(OverviewTreeView_AfterCollapse);

        }

        /// <summary>
        /// Raise the refresh request event
        /// </summary>
        public void OnRefreshRequestedEvent()
        {
            EventHandler handler = RefreshRequestedEvent;
            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        private void OverviewTreeView_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            // if (e.Node != null && e.Node is NavigationTreeFolder)
            //{
            //    this.Select(e.Node as ConnectivityTreeNode);
            //}
        }

        private void OverviewTreeView_BeforeCollapse(object sender, TreeViewCancelEventArgs e)
        {
            // make it selected so the drag-n-drop can get updated Connection info...
            //this.Select(e.Node as ConnectivityTreeNode);
        }

        private void OverviewTreeView_AfterCollapse(object sender, TreeViewEventArgs e)
        {
            // make it selected so the drag-n-drop can get updated Connection info...
            //this.Select(e.Node as ConnectivityTreeNode);
        }

        private void OverviewTreeView_MouseDown(object sender, MouseEventArgs e)
        {
            //MouseDownTreeNode = GetNodeAt(e.X, e.Y) as ConnectivityTreeNode;
            // make it selected so the drag-n-drop can get updated Connection info...
            //this.Select(MouseDownTreeNode);

        }

        private void OverviewTreeView_ItemDrag(object sender, ItemDragEventArgs e)
        {
            if (this.FavoritesTreeView != null)
            {
                FavoritesTreeView.IsDragging = true;
                DoDragDrop(e.Item, DragDropEffects.Move);
            }
        }

        /// <summary>
        /// Selects the tree node that contains the Connectivity object
        /// </summary>
        /// <param name="aConnectivityObject">The Connectivity object to select</param>
        /// <returns>True if selection was successful, else returns false</returns>
        public bool SelectOverviewObject(ConnectionDefinition aConnectivityObject)
        {
            /*
            NavigationTreeConnectionFolder connectionFolder = ExpandConnection(aConnectivityObject.ConnectionDefinition);
            ConnectivitySystemFolder systemFolder = connectionFolder as ConnectivitySystemFolder;
            if (systemFolder != null)
            {
                return systemFolder.SelectConnectivityObject(aConnectivityObject);
            }
             * */
            return false;
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
                if (theTreeNode is NavigationTreeConnectionsFolder)
                {
                    NavigationTreeConnectionsFolder theNavigationTreeMySystemsFolder = theTreeNode as NavigationTreeConnectionsFolder;
                    NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = theNavigationTreeMySystemsFolder.FindConnectionFolder(aConnectionDefinition);
                    if (theNavigationTreeConnectionFolder != null)
                    {

                        // Make sure that the connections folder has been populated by selecting
                        // the connections folder if it isn't.  This reduces flashing in the right
                        // pane when clikcing on a SQL object hyperlink in the right pane.
                        if (!theNavigationTreeConnectionFolder.IsExpanded)
                        {
                            Select(theNavigationTreeConnectionFolder);
                        }

                        return theNavigationTreeConnectionFolder;
                    }
                }
            }
            throw new Exception("Not found in active systems: " + aConnectionDefinition.Name);
        }
    }
}
