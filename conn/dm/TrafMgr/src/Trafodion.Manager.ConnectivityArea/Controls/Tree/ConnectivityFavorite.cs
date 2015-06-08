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
using System.Runtime.Serialization;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
	/// <summary>
	/// base class for nodes in a favorites tree, both leaves and folders.
	/// </summary>
    /// 
    [Serializable()]
    public class ConnectivityFavorite : ConnectivityFavoritesTreeNode
	{

        /// <summary>
        /// Constructor called when a favorite is being reloaded from persistence
        /// </summary>
        /// <param name="aNavigationTreeView">The navigation tree to which the favorite refers</param>
        /// <param name="aName">The favorite's name in the favorites tree</param>
        /// <param name="aTargetFullPath">The path to the favorite in the navigation tree</param>
        public ConnectivityFavorite(ConnectivityTreeView aNavigationTreeView, string aName, string aTargetFullPath)
        {
            theNavigationTreeView = aNavigationTreeView;
            theNavigationTreeNodeFullPath = aTargetFullPath;
            Text = aName;
            //ImageIndex = NavigationTreeNode.ImageIndex; 
            //SelectedImageIndex = NavigationTreeNode.SelectedImageIndex;

            ToolTipText = NavigationTreeNodeFullPath;
        }

        /// <summary>
        /// Constructor called when the user adds a favorite
        /// </summary>
        /// <param name="aNavigationTreeView">The navigation tree to which the favorite refers</param>
        /// <param name="aNavigationTreeNode">The node in the navigation tree</param>
        public ConnectivityFavorite(ConnectivityTreeView aNavigationTreeView, ConnectivityTreeNode aNavigationTreeNode)
        {
            theNavigationTreeView = aNavigationTreeView;
            theNavigationTreeNodeFullPath = aNavigationTreeNode.FullPath;
            Text = NavigationTreeNode.Text;
            ImageIndex = NavigationTreeNode.ImageIndex;
            SelectedImageIndex = NavigationTreeNode.SelectedImageIndex;

            ToolTipText = NavigationTreeNodeFullPath;
        }

        /// <summary>
        /// Accessor for the navigation tree node to which this favorite refers
        /// </summary>
        public ConnectivityTreeNode NavigationTreeNode
		{
			get
			{

                try
                {
                    TreeNode theTreeNode = NavigationTreeView.FindByFullPath(NavigationTreeNodeFullPath);

                    if (theTreeNode is ConnectivityTreeNode)
                    {
                        return (theTreeNode as ConnectivityTreeNode);
                    }
                    throw new CannotResolveFavorite(this);
                }
                catch (FindByFullPathFailed)
                {
                    throw new CannotResolveFavorite(this);
                }

            }

        }

        /// <summary>
        /// Short human readable description of this favorite.
        /// </summary>
		public string ShortDescription
		{
			get
			{
				return (NavigationTreeNode != null) ? NavigationTreeNode.ShortDescription : "...";
			}
		}

        /// <summary>
        /// A longer human readable description of this favorite.
        /// </summary>
		public string LongerDescription
		{
			get
			{
				return (NavigationTreeNode != null) ? NavigationTreeNode.LongerDescription : "...";
			}
		}

        private ConnectivityTreeView theNavigationTreeView = null;

        /// <summary>
        /// Accessor for the navigation tree that goes with this favorite
        /// </summary>
        public ConnectivityTreeView NavigationTreeView
        {
            get
            {
                return theNavigationTreeView;
            }
        }

        /// <summary>
        /// Accessor for the full path string
        /// </summary>
        public string NavigationTreeNodeFullPath
		{
			get
			{
				return theNavigationTreeNodeFullPath;
			}
		}

		private string theNavigationTreeNodeFullPath;

	}

    /// <summary>
    /// This exception is raised when the user attempts to use a favorite whose reference in the
    /// navgation tree cannot be found.
    /// </summary>
    public class CannotResolveFavorite : Exception
    {

        private ConnectivityFavorite _theFavorite;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aFavorite">The favorite to which this exception refers</param>
        public CannotResolveFavorite(ConnectivityFavorite aFavorite)
        {
            _theFavorite = aFavorite;
        }

        /// <summary>
        /// Accessor for the favorite to which this exception refers
        /// </summary>
        public ConnectivityFavorite Favorite
        {
            get { return _theFavorite; }
        }

        /// <summary>
        /// A human readable descripton of the exception
        /// </summary>
        /// <returns>A human readable descripton of the exception</returns>
        public override string ToString()
        {
            return Message;
        }

        /// <summary>
        /// Shows a message box that describes the exception
        /// </summary>
        public void ShowMessageBox()
        {
            MessageBox.Show(Utilities.GetForegroundControl(), Message, Properties.Resources.FavoritesErrorHeader, MessageBoxButtons.OK);
        }

        /// <summary>
        /// Accessor for a human readable descripton of the exception
        /// </summary>
        /// <returns>A human readable descripton of the exception</returns>
        public override string Message
        {
            get
            {
                return String.Format(Properties.Resources.FavoritesErrorMessage, new object[] {
                    Favorite.Text, Favorite.NavigationTreeNodeFullPath,"\n\n" });
            }
        }

    }

}
