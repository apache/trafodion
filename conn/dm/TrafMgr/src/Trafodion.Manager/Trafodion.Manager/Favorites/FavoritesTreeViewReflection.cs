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

namespace Trafodion.Manager.Framework.Favorites
{

    /// <summary>
    /// This class is used to represent a favorites tree when it is persisted.
    /// </summary>
    [Serializable]
    public class FavoritesTreeViewReflection
    {
        //private string theTester;

        //public string TheTester
        //{
        //    get { return theTester; }
        //    set { theTester = value; }
        //}

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aFavoritesTreeView">The favorites tree view to prepare for persistence</param>
        public FavoritesTreeViewReflection(FavoritesTreeView aFavoritesTreeView)
        {
            foreach (FavoritesTreeNode theFavoritesTreeNode in aFavoritesTreeView.Nodes)
            {
                if (theFavoritesTreeNode is FavoritesFolder)
                {
                    theFavoritesTreeNodeReflections.Add(new FavoritesFolderReflection(theFavoritesTreeNode as FavoritesFolder));
                }
                else if (theFavoritesTreeNode is Favorite)
                {
                    theFavoritesTreeNodeReflections.Add(new FavoriteReflection(theFavoritesTreeNode as Favorite));
                }
            }
        }

        private List<FavoritesTreeNodeReflection> theFavoritesTreeNodeReflections = new List<FavoritesTreeNodeReflection>();

        public List<FavoritesTreeNodeReflection> FavoritesTreeNodeReflections
        {
            get { return theFavoritesTreeNodeReflections; }
            set { theFavoritesTreeNodeReflections = value; }
        }

        public FavoritesTreeView MakeFavoritesTreeView(NavigationTreeView aNavigationTreeView)
        {
            FavoritesTreeView theFavoritesTreeView = new FavoritesTreeView();
            theFavoritesTreeView.NavigationTreeView = aNavigationTreeView;
            theFavoritesTreeView.Nodes.Clear();

            ProcessNodes(aNavigationTreeView, FavoritesTreeNodeReflections, theFavoritesTreeView.Nodes);

            return theFavoritesTreeView;
        }

        private void ProcessNodes(NavigationTreeView aNavigationTreeView, List<FavoritesTreeNodeReflection> aFavoritesTreeNodeReflections, TreeNodeCollection aTreeNodeCollection)
        {

            foreach (FavoritesTreeNodeReflection theFavoritesTreeNodeReflection in aFavoritesTreeNodeReflections)
            {
                if (theFavoritesTreeNodeReflection is FavoritesFolderReflection)
                {
                    FavoritesFolderReflection theFavoritesFolderReflection = theFavoritesTreeNodeReflection as FavoritesFolderReflection;
                    FavoritesFolder theFavoritesFolder = new FavoritesFolder(theFavoritesFolderReflection.Name);
                    aTreeNodeCollection.Add(theFavoritesFolder);
                    ProcessNodes(aNavigationTreeView, theFavoritesFolderReflection.FavoritesTreeNodeReflections, theFavoritesFolder.Nodes);
                }
                else if (theFavoritesTreeNodeReflection is FavoriteReflection)
                {
                    FavoriteReflection theFavoriteReflection = theFavoritesTreeNodeReflection as FavoriteReflection;
                    Favorite theFavorite = new Favorite(aNavigationTreeView, theFavoriteReflection.Name, theFavoriteReflection.TargetFullPath, theFavoriteReflection.ImageKey, theFavoriteReflection.SystemName);
                    aTreeNodeCollection.Add(theFavorite);
                }
            }

        }

    }

    [Serializable]
    abstract public class FavoritesTreeNodeReflection
    {

        public FavoritesTreeNodeReflection(FavoritesTreeNode aFavoritesTreeNode)
        {
            Name = aFavoritesTreeNode.Text;
            foreach (FavoritesTreeNode theFavoritesTreeNode in aFavoritesTreeNode.Nodes)
            {
                if (theFavoritesTreeNode is FavoritesFolder)
                {
                    theFavoritesTreeNodeReflections.Add(new FavoritesFolderReflection(theFavoritesTreeNode as FavoritesFolder));
                }
                else if (theFavoritesTreeNode is Favorite)
                {
                    theFavoritesTreeNodeReflections.Add(new FavoriteReflection(theFavoritesTreeNode as Favorite));
                }  
            }
        }

        private List<FavoritesTreeNodeReflection> theFavoritesTreeNodeReflections = new List<FavoritesTreeNodeReflection>();

        public List<FavoritesTreeNodeReflection> FavoritesTreeNodeReflections
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

    [Serializable]
    public class FavoritesFolderReflection : FavoritesTreeNodeReflection
    {

        public FavoritesFolderReflection(FavoritesTreeNode aFavoritesTreeNode)
            : base(aFavoritesTreeNode)
        {
        }

    }

    [Serializable]
    public class FavoriteReflection : FavoritesTreeNodeReflection
    {

        public FavoriteReflection(Favorite aFavorite) : base(aFavorite)
        {
            TargetFullPath = aFavorite.NavigationTreeNodeFullPath;
            imageKey = aFavorite.ImageKey;
            systemName = aFavorite.SystemName;
        }

        private string theTargetFullPath = "";
        private string imageKey = "";
        private string systemName = "";

        public string ImageKey
        {
            get { return imageKey; }
            set { imageKey = value; }
        }

        public string TargetFullPath
        {
            get { return theTargetFullPath; }
            set { theTargetFullPath = value; }
        }
        public string SystemName
        {
            get { return systemName; }
            set { systemName = value; }
        }
    }

}
