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
using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Favorites
{
    [ToolboxItem(false)]
    public partial class FavoritesTreeViewUserControl : UserControl
    {
        public FavoritesTreeViewUserControl()
        {
            InitializeComponent();
            FavoritesTreeView = new FavoritesTreeView();
        }

        private FavoritesTreeView theFavoritesTreeView;

        public FavoritesTreeView FavoritesTreeView
        {
            get { return theFavoritesTreeView; }
            set 
            { 
                if (theFavoritesTreeView != null)
                {
                    Controls.Remove(theFavoritesTreeView);
                }
                theFavoritesTreeView = value;
                if (theFavoritesTreeView != null)
                {
                    theFavoritesTreeView.Dock = DockStyle.Fill;
                    Controls.Add(theFavoritesTreeView);
                }
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            theFavoritesTreeView.Dump();
        }

    }

}
