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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Favorites;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{

    /// <summary>
    /// This class is used to represent a favorites tree when it is persisted.
    /// </summary>
    [Serializable]
    public class ConnectivityFavoritesTreeViewReflection
    {

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aFavoritesTreeView">The favorites tree view to prepare for persistence</param>

        public ConnectivityFavoritesTreeViewReflection(FavoritesTreeView aFavoritesTreeView)
        //public ConnectivityFavoritesTreeViewReflection(ConnectivityFavoritesTreeView aFavoritesTreeView)
        {
            foreach (ConnectivityFavoritesTreeNode theFavoritesTreeNode in aFavoritesTreeView.Nodes)
            {
                if (theFavoritesTreeNode is FavoritesTreeFolder)
                {
                    theFavoritesTreeNodeReflections.Add(new ConnectivityFavoritesFolderReflection(theFavoritesTreeNode as ConnectivityFavoritesTreeFolder));
                }
                else if (theFavoritesTreeNode is ConnectivityFavorite)
                {
                    theFavoritesTreeNodeReflections.Add(new ConnectivityFavoriteReflection(theFavoritesTreeNode as ConnectivityFavorite));
                }
            }
        }

        private List<ConnectivityFavoritesTreeNodeReflection> theFavoritesTreeNodeReflections = new List<ConnectivityFavoritesTreeNodeReflection>();

        public List<ConnectivityFavoritesTreeNodeReflection> FavoritesTreeNodeReflections
        {
            get { return theFavoritesTreeNodeReflections; }
            set { theFavoritesTreeNodeReflections = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aNavigationTreeView"></param>
        /// <returns></returns>
        public FavoritesTreeView MakeFavoritesTreeView(ConnectivityTreeView aNavigationTreeView)
//        public ConnectivityFavoritesTreeView MakeFavoritesTreeView(ConnectivityTreeView aNavigationTreeView)
        {
            //ConnectivityFavoritesTreeView theFavoritesTreeView = new ConnectivityFavoritesTreeView();
            FavoritesTreeView theFavoritesTreeView = new FavoritesTreeView();
            theFavoritesTreeView.NavigationTreeView = aNavigationTreeView;
            theFavoritesTreeView.Nodes.Clear();
            theFavoritesTreeView.LabelEdit = true;

            ProcessNodes(aNavigationTreeView, FavoritesTreeNodeReflections, theFavoritesTreeView.Nodes);

            return theFavoritesTreeView;
        }

        private void ProcessNodes(ConnectivityTreeView aNavigationTreeView, List<ConnectivityFavoritesTreeNodeReflection> aFavoritesTreeNodeReflections, TreeNodeCollection aTreeNodeCollection)
        {

            foreach (ConnectivityFavoritesTreeNodeReflection theFavoritesTreeNodeReflection in aFavoritesTreeNodeReflections)
            {
                if (theFavoritesTreeNodeReflection is ConnectivityFavoritesFolderReflection)
                {
                    ConnectivityFavoritesFolderReflection theFavoritesFolderReflection = theFavoritesTreeNodeReflection as ConnectivityFavoritesFolderReflection;
                    ConnectivityFavoritesTreeFolder theFavoritesFolder = new ConnectivityFavoritesTreeFolder(theFavoritesFolderReflection.Name);
                    aTreeNodeCollection.Add(theFavoritesFolder);
                    ProcessNodes(aNavigationTreeView, theFavoritesFolderReflection.FavoritesTreeNodeReflections, theFavoritesFolder.Nodes);
                }
                else if (theFavoritesTreeNodeReflection is ConnectivityFavoriteReflection)
                {
                    ConnectivityFavoriteReflection theFavoriteReflection = theFavoritesTreeNodeReflection as ConnectivityFavoriteReflection;
                    ConnectivityFavorite theFavorite = new ConnectivityFavorite(aNavigationTreeView, theFavoriteReflection.Name, theFavoriteReflection.TargetFullPath);
                    aTreeNodeCollection.Add(theFavorite);
                }
            }

        }

    }

    [Serializable]
    abstract public class ConnectivityFavoritesTreeNodeReflection
    {

        public ConnectivityFavoritesTreeNodeReflection(ConnectivityFavoritesTreeNode aFavoritesTreeNode)
        {
            Name = aFavoritesTreeNode.Text;
            foreach (ConnectivityFavoritesTreeNode theFavoritesTreeNode in aFavoritesTreeNode.Nodes)
            {
                if (theFavoritesTreeNode is ConnectivityFavoritesTreeFolder)
                {
                    theFavoritesTreeNodeReflections.Add(new ConnectivityFavoritesFolderReflection(theFavoritesTreeNode as ConnectivityFavoritesTreeFolder));
                }
                else if (theFavoritesTreeNode is ConnectivityFavorite)
                {
                    theFavoritesTreeNodeReflections.Add(new ConnectivityFavoriteReflection(theFavoritesTreeNode as ConnectivityFavorite));
                }  
            }
        }

        private List<ConnectivityFavoritesTreeNodeReflection> theFavoritesTreeNodeReflections = new List<ConnectivityFavoritesTreeNodeReflection>();

        public List<ConnectivityFavoritesTreeNodeReflection> FavoritesTreeNodeReflections
        {
            get { return theFavoritesTreeNodeReflections; }
            set { theFavoritesTreeNodeReflections = value; }
        }

        private string theName = "";

        public string Name
        {
            get { return theName; }
            set { theName = value; }
        }

    }

    /// <summary>
    /// 
    /// </summary>
    [Serializable]
    public class ConnectivityFavoritesFolderReflection : ConnectivityFavoritesTreeNodeReflection
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aFavoritesTreeNode"></param>
        public ConnectivityFavoritesFolderReflection(ConnectivityFavoritesTreeNode aFavoritesTreeNode)
            : base(aFavoritesTreeNode)
        {
        }

    }

    /// <summary>
    /// 
    /// </summary>
    [Serializable]
    public class ConnectivityFavoriteReflection : ConnectivityFavoritesTreeNodeReflection
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aFavorite"></param>
        public ConnectivityFavoriteReflection(ConnectivityFavorite aFavorite)
            : base(aFavorite)
        {
            TargetFullPath = aFavorite.NavigationTreeNodeFullPath;
        }

        private string theTargetFullPath = "";

        /// <summary>
        /// 
        /// </summary>
        public string TargetFullPath
        {
            get { return theTargetFullPath; }
            set { theTargetFullPath = value; }
        }

    }

}
