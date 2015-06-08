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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Navigation
{
    /// <summary>
    /// Abstract class for all nodes in a navigation tree
    /// </summary>
    abstract public class NavigationTreeNode : TreeNode
    {
        /// <summary>
        /// Default constructor
        /// </summary>
        public NavigationTreeNode()
        {
            ImageKey = NavigationTreeView.BLANK_DOCUMENT_ICON;
            SelectedImageKey = NavigationTreeView.BLANK_DOCUMENT_ICON;
        }

        abstract public string ShortDescription { get; }
        abstract public string LongerDescription { get; }
        
        abstract protected void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter);


        virtual public String LongerDescriptionLink 
        {
            get { return ""; } 
        }

        virtual public Object LongerDescriptionLinkTag
        {
            get { return null; }
        }

        virtual public string FavoritesImageKey
        {
            get { return ImageKey; }
        }

        /// <summary>
        /// Called by the Navigation Tree View Dipose method
        /// Allows for any cleanup needed on the navigation tree node
        /// </summary>
        /// <param name="disposing"></param>
        public virtual void Dispose(bool disposing)
        {

        }

        /// <summary>
        /// Refreshes the state of the tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        virtual protected void DoRefresh(NavigationTreeNameFilter aNameFilter)
        {
            try
            {
                if (this.TreeView != null)
                {
                    this.TreeView.Cursor = Cursors.WaitCursor;
                }
                Refresh(aNameFilter);
            }
            catch (Connections.MostRecentConnectionTestFailedException)
            {
                // This is an OK exception ... we handle it by not populating
            }
            catch (Connections.PasswordNotSetException)
            {
                // This is an OK exception ... we handle it by not populating
            }
            catch (Exception e)
            {
                if (this.TreeView != null)
                {
                    this.TreeView.Cursor = Cursors.Default;
                }
                string errorMessage = (e.InnerException != null) ? e.InnerException.Message : e.Message;
                MessageBox.Show(Utilities.GetForegroundControl(), errorMessage, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            finally
            {
                if (this.TreeView != null)
                {
                    this.TreeView.Cursor = Cursors.Default;
                }
            }
            if (TreeView != null)
            {
                TreeView.SelectedNode = Parent;
                TreeView.SelectedNode = this;
            }
        }

        /// <summary>
        /// The connection definition associated with this tree node
        /// </summary>
        virtual public ConnectionDefinition TheConnectionDefinition
        {
            get
            {
                NavigationTreeNode theParent = Parent as NavigationTreeNode;
                if (theParent != null)
                {
                    return theParent.TheConnectionDefinition;
                }
                return null;
            }
            set
            {
                NavigationTreeNode theParent = Parent as NavigationTreeNode;
                if (theParent != null)
                {
                    theParent.TheConnectionDefinition = value;
                }
            }
        }

        /// <summary>
        /// This method lets the TreeNodes to add context menu items that are specific to the node
        /// The Navigation tree calls this method and passes a context menu strip to which the menu items need to be added
        /// </summary>
        /// <param name="aContextMenuStrip">The context menu strip to which the menu items have to be added</param>
        virtual public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            if (this.Parent != null)
            {
                if ((((NavigationTreeView)this.TreeView).FavoritesTreeView != null)/* &&
                (TheConnectionDefinition != null) && 
                (TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)*/
                                                                                                    )
                {
                    ToolStripMenuItem addToFavoritesMenuItem = new ToolStripMenuItem(Properties.Resources.AddToFavoritesMenuText);
                    addToFavoritesMenuItem.Click += new EventHandler(theAddToFavoritesMenuItem_Click);
                    aContextMenuStrip.Items.Add(addToFavoritesMenuItem);
                }
            }

            ToolStripMenuItem refreshMenuItem = new ToolStripMenuItem(Properties.Resources.RefreshMenuText);
            refreshMenuItem.Click += new EventHandler(refreshMenuItem_Click);
            aContextMenuStrip.Items.Add(refreshMenuItem);
        }

        /// <summary>
        /// Event handler for the Add to Favorites event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theAddToFavoritesMenuItem_Click(object sender, EventArgs e)
        {
            ((NavigationTreeView)this.TreeView).FavoritesTreeView.AddToFavorites(this);
        }

        /// <summary>
        /// Event handler for the Refresh menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void refreshMenuItem_Click(object sender, EventArgs e)
        {
            DoRefresh(((NavigationTreeView)this.TreeView).TheNavigationTreeNameFilter);
        }

        public void RefreshNode()
        {
            DoRefresh(((NavigationTreeView)this.TreeView).TheNavigationTreeNameFilter);
        }
    }
}
