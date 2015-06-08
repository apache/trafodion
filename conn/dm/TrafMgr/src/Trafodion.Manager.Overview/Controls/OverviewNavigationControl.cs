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
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.OverviewArea.Controls.Tree;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// User Control for the navigation pane
    /// </summary>
    public partial class OverviewNavigationControl : NavigationUserControl, IConnectionDefinitionSelector
    {
        #region Properties

        /// <summary>
        /// Get the associated Connectivity Tree View
        /// </summary>
        public OverviewTreeView OverviewTreeView
        {
            get { return TheNavigationTreeView as OverviewTreeView; }        
        }

        /// <summary>
        /// Get the associated Favorites Tree View
        /// </summary>
        public FavoritesTreeView OverviewFavoritesTreeView
        //public ConnectivityFavoritesTreeView  ConnectivityFavoritesTreeView
        {
            get { return TheFavoritesTreeView; }
        }

        /// <summary>
        /// Get the associated connection definition
        /// </summary>
        public ConnectionDefinition CurrentConnectionDefinition
        {
            get
            {
                return OverviewTreeView.CurrentConnectionDefinition;
            }
        }

        #endregion Properties

        /// <summary>
        /// Constructor for The Navigation Control for the Connectivity area
        /// </summary>
        public OverviewNavigationControl()
        {
            InitializeComponent();

            TheNavigationTreeView = new OverviewTreeView();
        }

        public override NavigationTreeConnectionFolderFactory TheConnectionFolderFactory
        {
            get { return new ConnectionFolderFactory(); }
        }

        public override string ThePersistenceKey
        {
            get { return "OverviewAreaPersistence"; }
        }
    }
}
