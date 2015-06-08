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
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.ConnectivityArea.Controls.Tree;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// 
    /// </summary>
    public class FavoritesFolderPanel : Panel
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aFavoritesFolder"></param>
        public FavoritesFolderPanel(FavoritesFolder aFavoritesFolder)
        {
            FavoritesFolderDataGridView theFavoritesFolderDataGridView = new FavoritesFolderDataGridView(aFavoritesFolder);
            theFavoritesFolderDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theFavoritesFolderDataGridView);

            Controls.Add(theFavoritesFolderDataGridView);

            theFavoritesFolderDataGridView.AddCountControlToParent(aFavoritesFolder.Text + " contains {0} items", DockStyle.Top);
            theFavoritesFolderDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }
    }
}
