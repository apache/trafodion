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
using System.Windows.Forms;
using Trafodion.Manager;
using Trafodion.Manager.Framework.Navigation;
using System.Runtime.Serialization;

namespace Trafodion.Manager.Framework.Favorites
{

    public class FavoritesFolder : FavoritesTreeNode
    {

        public FavoritesFolder()
            : this("My Favorites")
        {
        }

        public FavoritesFolder(string aText)
        {
            Text = aText;
            ImageKey = NavigationTreeView.FOLDER_CLOSED_ICON;
            SelectedImageKey = NavigationTreeView.FOLDER_CLOSED_ICON;
        }

        public Favorite GetFavoriteFor(TreeNode aTreeNode)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is Favorite)
                {
                    Favorite theFavorite = theTreeNode as Favorite;

                    // favorite may be out of date and have no tree node
                    try
                    {
                        if (theFavorite.NavigationTreeNode == aTreeNode)
                        {
                            return theFavorite;
                        }
                    }
                    catch (Exception) { }
                }

            }
            return null;
        }

        public bool ContainsFavoriteFor(TreeNode aTreeNode)
        {
            return GetFavoriteFor(aTreeNode) != null;
        }

        public bool Contains(string aName)
        {
            foreach (TreeNode theNode in Nodes)
            {
                FavoritesTreeNode theFavoritesTreeNode = theNode as FavoritesTreeNode;
                if (theFavoritesTreeNode != null)
                {
                    if (theFavoritesTreeNode.Text.Equals(aName))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

    }
}
