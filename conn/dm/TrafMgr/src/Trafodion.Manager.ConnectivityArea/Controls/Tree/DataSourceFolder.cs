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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{

    /// <summary>
    /// Tree node wrapping a NDCSService
    /// </summary>
    public class DataSourceFolder : ConnectivityTreeNode
    {
        private ToolStripMenuItem editDataSourceMenuItem;
        private ToolStripMenuItem deleteDataSourceMenuItem;
        private ToolStripMenuItem addDataSourceLikeMenuItem;
        private bool _editThisDS = false;

        /// <summary>
        /// 
        /// </summary>
        public bool EditThisDS
        {
            get { return _editThisDS; }
            set { _editThisDS = value; }
        }


        /// <summary>
        /// Get the underlying service
        /// </summary>
        public NDCSDataSource NdcsDataSource
        {
            get { return NDCSObject as NDCSDataSource; }
        }

        /// <summary>
        /// Constructor for a Service Folder
        /// </summary>
        /// <param name="aNDCSDataSource"></param>
        public DataSourceFolder(NDCSDataSource aNDCSDataSource)
            : base(aNDCSDataSource)
        {
            InitializeComponent();
            ImageKey = ConnectivityTreeView.DATASOURCE_ICON;
            SelectedImageKey = ConnectivityTreeView.DATASOURCE_ICON;
        }

        /// <summary>
        /// Refresh this node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            NdcsDataSource.Refresh();
        }

        /// <summary>
        /// Adds context menu items for the service folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            TreeView.SelectedNode = this;

            if (TheConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ALTER"))
            {
                aContextMenuStrip.Items.Add(editDataSourceMenuItem);
            }

            if(TheConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_DELETE"))
            {
                aContextMenuStrip.Items.Add(deleteDataSourceMenuItem);
                
                // Delete menu is enabled only for user created datasources
                deleteDataSourceMenuItem.Enabled = !NdcsDataSource.IsSystemDataSource ;
            }
            if (TheConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ADD"))
            {
                aContextMenuStrip.Items.Add(addDataSourceLikeMenuItem);
            }

        }

        /// <summary>
        /// Refreshes the state of the tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        void DoEdit(NavigationTreeNameFilter aNameFilter)
        {
            try
            {
                Refresh(aNameFilter);
            }
            catch (Trafodion.Manager.Framework.Connections.MostRecentConnectionTestFailedException)
            {
                // This is an OK exception ... we handle it by not populating
            }
            catch (Trafodion.Manager.Framework.Connections.PasswordNotSetException)
            {
                // This is an OK exception ... we handle it by not populating
            }
            catch (Exception e)
            {
                string errorMessage = (e.InnerException != null) ? e.InnerException.Message : e.Message;
                MessageBox.Show(Utilities.GetForegroundControl(), errorMessage, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            if (TreeView != null)
            {
                TreeView.SelectedNode = Parent;
                TreeView.SelectedNode = this;
            }
        }

        /// <summary>
        /// Edit Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void editDataSourceMenuItem_Click(object sender, EventArgs e)
        {
            this._editThisDS = true;
            if (TreeView.SelectedNode == this)
                TreeView.SelectedNode = null;

            TreeView.SelectedNode = this;
        }

        /// <summary>
        /// Handles when the user clicks on the add new Data Source Like context menu item
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void addDataSourceLikeMenuItem_Click(object sender, EventArgs e)
        {
            ConnectivityAreaDatasourceConfigurationUserControl theControl = null;
            DataSourcesFolder ds = null;

            try
            {
                TreeView.SelectedNode = this;
                //Create an DataSourcesFolder object for the parent node of a selected child node. 
                ds = (DataSourcesFolder)TreeView.SelectedNode.Parent;
                
                theControl = new ConnectivityAreaDatasourceConfigurationUserControl((ConnectivityTreeView)this.TreeView, DSConfigFormType.CreateLike);//TabPages[SelectedIndex].Controls[0];
                theControl.ConnectivityTreeControlSelected(this);
                theControl.CreateForm = WindowsManager.PutInWindow(theControl.Size, theControl, "Create New Datasource", true, TrafodionContext.Instance.CurrentConnectionDefinition);                

                //ds.AddNewNode();
                //this.Name = TreeView.SelectedNode.Name;
                //ds.AddNewNodeLike(this);
            }

            catch (Exception ex)
            {
                //    Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                if (ds != null)
                {                    
                    this.NdcsDataSource.NDCSSystem.RefreshDataSources();
                    ds.refreshTree();
                    ds.refreshRightPane();
                }                
            }

        }


        /// <summary>
        /// StartService Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void startServiceMenuItem_Click(object sender, EventArgs e)
        {
        }


        /// <summary>
        /// Delete Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void deleteDataSourceMenuItem_Click(object sender, EventArgs e)
        {
            string theText = "";
            theText = String.Format(Properties.Resources.Message_ConfirmDSDeletion, this.NdcsDataSource.Name);

            TreeView.SelectedNode = this;

            // Create an DataSourcesFolder object for the parent node of a selected child node. 
            DataSourcesFolder dataSourcesFolder = (DataSourcesFolder)TreeView.SelectedNode.Parent;

            if (MessageBox.Show(Utilities.GetForegroundControl(), theText, Properties.Resources.DialogTitle_ConfirmDSDeletion, MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
                try
                {
                    this.NdcsDataSource.NDCSSystem.DeleteDataSource(this.NdcsDataSource);
                    dataSourcesFolder.refreshTree();
                    dataSourcesFolder.refreshRightPane();
                }
                catch (System.Data.Odbc.OdbcException oe)
                {
                    // Got an ODBC error. Show it.
                    MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
                }
                catch (Exception ex)
                {
                    // Got some other exception.  Show it.
                    MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK);
                }
            }
            else
            {
                // If the user is tricky and right-clicks on the DataSource node while the DataSources node is selected,
                // then update the right pane with the current object
                if (this.TreeView.SelectedNode.Equals(Parent))
                {
                    ((DataSourcesFolder)Parent).refreshRightPane();
                }
            }
        }


        /// <summary>
        /// Refresh the right pane when add, hold, release, alter service
        /// </summary>
        public void refreshRightPane()
        {
            // Recreate the right pane
            TreeView.SelectedNode = Parent;
            TreeView.SelectedNode = this;
        }


        private void InitializeComponent()
        {
            this.editDataSourceMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_Edit);
            this.deleteDataSourceMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_Delete);
            this.addDataSourceLikeMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_AddDataSourceLike);
            // 
            // editDataSourceMenuItem
            // 
            this.editDataSourceMenuItem.Name = "editDataSourceMenuItem";
            this.editDataSourceMenuItem.Tag = this;
            this.editDataSourceMenuItem.Enabled = true;
            this.editDataSourceMenuItem.Text = Properties.Resources.ContextMenu_Edit;
            this.editDataSourceMenuItem.Click += new EventHandler(editDataSourceMenuItem_Click);
            // 
            // deleteDataSourceMenuItem
            // 
            this.deleteDataSourceMenuItem.Name = "deleteDataSourceMenuItem";
            this.deleteDataSourceMenuItem.Tag = this;
            this.deleteDataSourceMenuItem.Enabled = true;
            this.deleteDataSourceMenuItem.Text = Properties.Resources.ContextMenu_Delete;
            this.deleteDataSourceMenuItem.Click += new EventHandler(deleteDataSourceMenuItem_Click);
            // 
            // addDataSourceLikeMenuItem
            // 
            this.addDataSourceLikeMenuItem.Tag = this;
            this.addDataSourceLikeMenuItem.Enabled = true;
            this.addDataSourceLikeMenuItem.Text = Properties.Resources.ContextMenu_AddDataSourceLike;
            this.addDataSourceLikeMenuItem.Click += new EventHandler(addDataSourceLikeMenuItem_Click);

        }

    }
}
