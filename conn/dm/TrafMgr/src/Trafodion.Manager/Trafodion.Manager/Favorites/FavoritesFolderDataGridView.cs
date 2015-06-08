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
using Trafodion.Manager;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Favorites
{
    public class FavoritesFolderDataGridView : TrafodionDataGridView
    {
        private FavoritesFolder theFavoritesFolder;

        public FavoritesFolder TheFavoritesFolder
        {
            get { return theFavoritesFolder; }
            set
            {
                theFavoritesFolder = value;

                Rows.Clear();
                Columns.Clear();

                Columns.Add("theWindowTitleColumn", "Name");
                Columns.Add("theDescriptionColumn", "Description");

                foreach (TreeNode theTreeNode in TheFavoritesFolder.Nodes)
                {
                    if (theTreeNode is Favorite)
                    {
                        Favorite theFavorite = theTreeNode as Favorite;

                        Rows.Add(new object[] {
                        theFavorite,
                        theFavorite.NavigationTreeNodeFullPath
                    });

                    }
                    else if (theTreeNode is FavoritesFolder)
                    {
                        FavoritesFolder theChildFavoritesFolder = theTreeNode as FavoritesFolder;

                        Rows.Add(new object[] {
                        theChildFavoritesFolder.Text,
                        "Favorites Folder"
                    });

                    }

                }

            }
        }

        public FavoritesFolderDataGridView(FavoritesFolder aFavoritesFolder)
        {
            TheFavoritesFolder = aFavoritesFolder;
            this.DoubleClick += FavoritesFolderDataGridView_DoubleClick;
        }

        void FavoritesFolderDataGridView_DoubleClick(object sender, EventArgs e)
        {
            if(SelectedRows.Count > 0)
            {
                Favorite theSelectedFavorite = Rows[0].Cells[0].Value as Favorite;
                if (theSelectedFavorite != null && theSelectedFavorite.TreeView != null)
                {
                    theSelectedFavorite.TreeView.SelectedNode = theSelectedFavorite;
                }
            }
        }

    }
}
