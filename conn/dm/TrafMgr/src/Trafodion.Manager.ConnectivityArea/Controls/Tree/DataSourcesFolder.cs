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
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{


    /// <summary>
    /// Represents the services folder in the navigation tree
    /// </summary>
    public class DataSourcesFolder : ConnectivityTreeFolder
    {

        #region Private member variables
        
        private ToolStripMenuItem addDataSourceMenuItem;
        private int _indexOfLastNewNode;

        #endregion Private member variables

        #region Properties

        /// <summary>
        /// Returns the Connectivity System associated with this folder
        /// </summary>
        public NDCSSystem ConnectivitySystem
        {
            get { return ConnectivityObject as NDCSSystem; }
        }
               

        #endregion Properties


        /// <summary>
        /// Default constructor for the services folder
        /// </summary>
        /// <param name="aConnectivitySystem"></param>
        public DataSourcesFolder(NDCSSystem aConnectivitySystem)
            :base(aConnectivitySystem, true)
        {
            InitializeComponent();
            //ShowNodeToolTips = true;
            //HideSelection = false;
            _indexOfLastNewNode = 1;
        }

        /// <summary>
        /// Selects this tree node
        /// </summary>
        /// <param name="aNDCSServiceObject"></param>
        /// <returns></returns>
        public bool SelectNDCSServiceObject(NDCSObject aNDCSServiceObject)
        {
            DoPopulate(null);

            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is NDCSServiceFolder)
                {
                    NDCSServiceFolder ndcsserviceFolder = theTreeNode as NDCSServiceFolder;
                }
            }
            return false;
        }

        protected override void PrepareForPopulate()
        {
            object a = ConnectivitySystem.NDCSDataSources;
        }
        
        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();

            foreach (NDCSDataSource datasource in ConnectivitySystem.NDCSDataSources)
            {
                DataSourceFolder dataSourceFolder = new DataSourceFolder(datasource);
                Nodes.Add(dataSourceFolder);
            }
        }

        /// <summary>
        /// Refreshes the list of services
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            // Repopulate the tree
            this.ConnectivitySystem.RefreshDataSources();
            TreeView.SelectedNode = this;
        }

        /// <summary>
        /// Returns the name displayed in the navigation tree
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Properties.Resources.DataSources;
            }
        }

        /// <summary>
        /// Returns the longer description
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
            base.AddToContextMenu(aContextMenuStrip);

            // make this node be selected
            TreeView.SelectedNode = this;

            // only allow Admin user to add datasource
            if (TheConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_ADD"))
            {
                // Add to the context menu
                aContextMenuStrip.Items.Add(addDataSourceMenuItem);
            }
        }


        /// <summary>
        /// Handles when the user clicks on the add new Data Source context menu item
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void addDataSourceMenuItem_Click(object sender, EventArgs e)
        {
            ConnectivityAreaDatasourceConfigurationUserControl theControl = new ConnectivityAreaDatasourceConfigurationUserControl((ConnectivityTreeView)this.TreeView, DSConfigFormType.Create);//TabPages[SelectedIndex].Controls[0];
            theControl.CreateForm = WindowsManager.PutInWindow(theControl.Size,theControl,"Create New Datasource",true,TrafodionContext.Instance.CurrentConnectionDefinition);
            
            theControl.ConnectivityTreeControlSelected(this);
        }

        /// <summary>
        /// Method to add a new Data Source node on the selection of 'Create Datasource' context menu item
        /// </summary>
        public void AddNewNode()
        {
            string _newNodeName;          

            _newNodeName = "";            
            
            try
            {
                for (int i = 1; i <= Nodes.Count; i++)
                {
                    _newNodeName = Properties.Resources.NewNodeNamePrefix + i.ToString();
                    
                    if (!CheckForNode(_newNodeName))
                    {
                        DataSourceFolder dataSourceFolder1 = new DataSourceFolder(new NDCSDataSource(ConnectivitySystem, _newNodeName));

                        if (i == 1)
                        {
                            Nodes.Add(dataSourceFolder1);                           
                            
                        }
                        else
                        {
                            Nodes.Insert(_indexOfLastNewNode+1, dataSourceFolder1);
                        } 
                        break;
                        
                    }                    

                }
                
            }
                catch (Exception ex)
            {
                //    Got some other exception.  Show it.                      
                      MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            
            
        }

        /// <summary>
        /// Method to add a new Data Source node on the selection of 'Create Datasource Like...' context menu item
        /// </summary>
        public void AddNewNodeLike(DataSourceFolder dataSourcefolderObj)
        {
            string _newNodeName;

            _newNodeName = "";
            
            string _datasourceFoldername;

            try
            {
                _datasourceFoldername = dataSourcefolderObj.Name;

                for (int i = 1; i <= Nodes.Count; i++)
                {
                    // what we want to do is use the existing one's name and add an extension
                    // this whould make it work
                    
                    _newNodeName = _datasourceFoldername + "_" + i.ToString();
                    
                    if (!CheckForNode(_newNodeName))
                    {
                        // Option 1:
                        NDCSDataSource ndcsDataSourceObj = new NDCSDataSource(ConnectivitySystem,_newNodeName);
                        // we need to copy the properties from the current datasource into the new one
                        // something like :   ndcsDataSourceObj = dataSourcefolderObj.NdcsDataSource

                        DataSourceFolder dataSourceFolder1 = new DataSourceFolder(ndcsDataSourceObj);
                        // Option 2:
                        //dataSourceFolder1 = dataSourcefolderObj;
                        //In the above statement instead of creating a new DataSourceFolder object, the DataSourceFolder object passed
                        // in the parameter of this function need to be used.
                        //dataSourceFolder1.Name = dataSourcefolderObj.Name + "_" + i.ToString();
  
                        if (i == 1)
                        {
                            Nodes.Add(dataSourceFolder1);

                        }
                        else
                        {
                            Nodes.Insert(_indexOfLastNewNode + 1, dataSourceFolder1);
                            
                        }
                        
                        break;

                    }

                }

            }
            catch (Exception ex)
            {
                //    Got some other exception.  Show it.                      
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }


        }

        /// <summary>
        /// Method to check the existence of a node in the Treenodes collection 
        /// </summary>
        /// <param name="_newNodeName"></param>
        /// <returns></returns>
        private bool CheckForNode(string _newNodeName)
        {
            foreach (TreeNode _datasourceNode in Nodes)
            {
                if (_datasourceNode.Text == _newNodeName)
                {
                    _indexOfLastNewNode = _datasourceNode.Index;
                    return true;
                }
            }
            return false;
        }
       
        /// <summary>
        /// Refresh the tree when add service, hold, release all services
        /// </summary>
        public void refreshTree()
        {
            // Repopulate the tree
            Populate(null);
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
            this.addDataSourceMenuItem = new System.Windows.Forms.ToolStripMenuItem(Properties.Resources.ContextMenu_AddDataSource);
            //
            // addDataSourceMenuItem
            //
            this.addDataSourceMenuItem.Name = "addDataSourceMenuItem";
            this.addDataSourceMenuItem.Tag = this;
            this.addDataSourceMenuItem.Enabled = true;
            this.addDataSourceMenuItem.Text = Properties.Resources.ContextMenu_AddDataSource;
            this.addDataSourceMenuItem.Click += new EventHandler(addDataSourceMenuItem_Click);

        }

    }
}
