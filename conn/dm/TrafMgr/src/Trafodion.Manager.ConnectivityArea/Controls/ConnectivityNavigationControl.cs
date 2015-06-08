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
using Trafodion.Manager.ConnectivityArea.Controls.Tree;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Favorites;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls
{
    /// <summary>
    /// User Control for the navigation pane
    /// </summary>
    public partial class ConnectivityNavigationControl : NavigationUserControl, IConnectionDefinitionSelector
    {

        #region Properties

        /// <summary>
        /// Get the associated Connectivity Tree View
        /// </summary>
        public ConnectivityTreeView ConnectivityTreeView
        {
            get { return TheNavigationTreeView as ConnectivityTreeView; }        
        }

        /// <summary>
        /// Get the associated Favorites Tree View
        /// </summary>
        public FavoritesTreeView ConnectivityFavoritesTreeView
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
                return ConnectivityTreeView.CurrentConnectionDefinition;
            }
        }

        #endregion Properties

        /// <summary>
        /// Constructor for The Navigation Control for the Connectivity area
        /// </summary>
        public ConnectivityNavigationControl()
        {
            InitializeComponent();

            TheNavigationTreeView = new ConnectivityTreeView();
        }

        public override NavigationTreeConnectionFolderFactory TheConnectionFolderFactory
        {
            get { return new ConnectionFolderFactory(); }
        }

        public override string ThePersistenceKey
        {
            get { return "ConnectivityAreaPersistence"; }
        }
    }
}
