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
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Favorites;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// </summary>
    public class ConnectivityTreeView : Trafodion.Manager.Framework.Navigation.NavigationTreeView
    {
        public const string SERVICES_ICON = "SERVICES_ICON";
        public const string DATASOURCE_ICON = "DATASOURCE_ICON";

        FavoritesTreeView theConnectivityFavoritesTreeView = null;

        public FavoritesTreeView ConnectivityFavoritesTreeView
        {
            get { return theConnectivityFavoritesTreeView; }
            set { theConnectivityFavoritesTreeView = value; }
        }


        /// <summary>
        /// 
        /// </summary>
        public ConnectivityTreeView()
        {
            this.theImageList.Images.Add(SERVICES_ICON, global::Trafodion.Manager.Properties.Resources.ServicesIcon);
            this.theImageList.Images.Add(DATASOURCE_ICON, global::Trafodion.Manager.Properties.Resources.DataSourceIcon);
            ItemDrag += new ItemDragEventHandler(ConnectivityTreeView_ItemDrag);
        }

        private void ConnectivityTreeView_ItemDrag(object sender, ItemDragEventArgs e)
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
                    NavigationTreeConnectionsFolder theNavigationTreeMyActiveSystemsFolder = theTreeNode as NavigationTreeConnectionsFolder;
                    NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = theNavigationTreeMyActiveSystemsFolder.FindConnectionFolder(aConnectionDefinition);
                    if (theNavigationTreeConnectionFolder != null)
                    {

                        // Make sure that the connections folder has been populated by selecting
                        // the connections folder if it isn't.  
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
