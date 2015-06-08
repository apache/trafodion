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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Controls.Tree
{
    /// <summary>
    /// Navigation tree folder representing a system
    /// </summary>
    public class OverviewSystemFolder : NavigationTreeConnectionFolder
    {
        #region Fields

        private ConnectionDefinition _connectionDefinition;
        //private NDCSServicesFolder _ndcsservicesFolder;
        //private DataSourcesFolder _ndcsdatasourceFolder;

        #endregion Fields

        #region Properties

        /// <summary>
        /// 
        /// </summary>
        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set { _connectionDefinition = value; }
        }

        #endregion Properties

        /// <summary>
        /// Constructor for System Folder
        /// </summary>
        /// <param name="aConnectivitySystem"></param>
        public OverviewSystemFolder(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            ConnectionDefinition = aConnectionDefinition;
        }

        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
        }

        /// <summary>
        /// Refreshes the folder by clearing and populating the child nodes
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void DoRefresh(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            IsPopulated = false;

            try
            {
                if (this.TreeView != null)
                {
                    this.TreeView.Cursor = Cursors.WaitCursor;
                }
                Refresh(aNameFilter);
                if (TreeView == null)
                {
                    //This may happen if the Node has been removed during the refresh
                    return;
                }
                Populate(aNameFilter);
                IsPopulated = true;
            }
            catch (Trafodion.Manager.Framework.Connections.MostRecentConnectionTestFailedException mrctfe)
            {
                // This is an OK exception ... we handle it by not populating
            }
            catch (Trafodion.Manager.Framework.Connections.PasswordNotSetException pnse)
            {
                // This is an OK exception ... we handle it by not populating
            }
            catch (Exception e)
            {
                // Any other exceptions are unexpected
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
        }

        /// <summary>
        /// Refresh this node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            //.Refresh();
            refreshRightPane();
        }

        /// <summary>
        /// Short Description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return TheConnectionDefinition.Name;
            }
        }
        
        /// <summary>
        /// Longer Description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return ShortDescription;
            }
        }

        /// <summary>
        /// Adds items to the context menu for the services folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);
        }


        /// <summary>
        /// Refreshes the right pane 
        /// </summary>
        public void refreshRightPane()
        {
            // Recreate the right pane
            //TreeView.SelectedNode = Parent;
            //TreeView.SelectedNode = this;
            ((OverviewTreeView)TreeView).OnRefreshRequestedEvent();
        }

    }
}
