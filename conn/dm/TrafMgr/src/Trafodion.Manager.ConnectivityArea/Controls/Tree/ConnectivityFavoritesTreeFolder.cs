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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Windows.Forms;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// Base class for Tree folder in Connectivity Area
    /// that holds a ConnectivityObject
    /// </summary>
    public class ConnectivityFavoritesTreeFolder : ConnectivityFavoritesTreeNode
    {
        #region Fields

        private bool isRootNode = false;

        #endregion Fields


        # region Properties

        /// <summary>
        /// 
        /// </summary>
        public bool IsRootNode
        {
            get { return isRootNode; }
            set { isRootNode = value; }
        }

        #endregion Properties

        /// <summary>
        /// 
        /// </summary>
        public ConnectivityFavoritesTreeFolder()
            : this("My Favorites")
        {
            IsRootNode = true;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aText"></param>
        public ConnectivityFavoritesTreeFolder(string aText)
        {
            Text = aText;
            ImageIndex = 0;
            SelectedImageIndex = 1;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTreeNode"></param>
        /// <returns></returns>
        public ConnectivityFavorite GetFavoriteFor(ConnectivityTreeNode aTreeNode)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is ConnectivityFavorite)
                {
                    ConnectivityFavorite theFavorite = theTreeNode as ConnectivityFavorite;

                    // favorite may be out of date and have no tree node
                    try
                    {
                        if (theFavorite.NavigationTreeNode == aTreeNode)
                        {
                            return theFavorite;
                        }
                    }
                    catch (System.Data.Odbc.OdbcException oe)
                    {
                        // Got an ODBC erorr. Show it.
                        MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
                    }
                    catch (Exception ex) 
                    {
                        // Got some other exception.  Show it.
                        MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
                    }
                }

            }
            return null;
        }

        public bool ContainsFavoriteFor(ConnectivityTreeNode aTreeNode)
        {
            return GetFavoriteFor(aTreeNode) != null;
        }

        public bool Contains(string aName)
        {
            foreach (TreeNode theNode in Nodes)
            {
                ConnectivityFavoritesTreeNode theFavoritesTreeNode = theNode as ConnectivityFavoritesTreeNode;
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
