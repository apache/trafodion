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

using System.Windows.Forms;
using Trafodion.Manager.ConnectivityArea.Model;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// Represents the services folder in the navigation tree
    /// </summary>
    public class NDCSServicesFolder : ConnectivityTreeFolder
    {

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
        public NDCSServicesFolder(NDCSSystem aConnectivitySystem)
            :base(aConnectivitySystem, true)
        {
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
                    if (ndcsserviceFolder.NDCSObject.Name.Equals(aNDCSServiceObject.Name))
                    {
                        ndcsserviceFolder.TreeView.SelectedNode = ndcsserviceFolder;
                        return true;
                    }
                }
            }
            return false;
        }

        protected override void PrepareForPopulate()
        {
            object c = ConnectivitySystem.NDCSServices;
        }
        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            foreach (NDCSService service in ConnectivitySystem.NDCSServices)
            {
                NDCSServiceFolder ndcsserviceFolder = new NDCSServiceFolder(service);
                Nodes.Add(ndcsserviceFolder);
            }

        }

        /// <summary>
        /// Refreshes the list of services
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            ConnectivitySystem.RefreshServices();
            TreeView.SelectedNode = this;
        }

        /// <summary>
        /// Returns the name displayed in the navigation tree
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Properties.Resources.TabPageLabel_Services;
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
            TreeView.SelectedNode = this;
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

    }
}
