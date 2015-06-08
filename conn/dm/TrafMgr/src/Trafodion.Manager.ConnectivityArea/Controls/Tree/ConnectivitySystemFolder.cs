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

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// Navigation tree folder representing a system
    /// </summary>
    public class ConnectivitySystemFolder : NavigationTreeConnectionFolder
    {
        #region Fields

        private NDCSSystem _connectivitySystem;
        private NDCSServicesFolder _ndcsservicesFolder;
        private DataSourcesFolder _ndcsdatasourceFolder;

        #endregion Fields

        #region Properties

        /// <summary>
        /// 
        /// </summary>
        public NDCSSystem ConnectivitySystem
        {
            get { return _connectivitySystem; }
            set { _connectivitySystem = value; }
        }

        #endregion Properties

        /// <summary>
        /// Constructor for System Folder
        /// </summary>
        /// <param name="aConnectivitySystem"></param>
        public ConnectivitySystemFolder(NDCSSystem aConnectivitySystem)
            : base(aConnectivitySystem.ConnectionDefinition)
        {
            ConnectivitySystem = aConnectivitySystem;
            Text = ShortDescription;
            ConnectivitySystem.Refresh(); //If Connectivity system is reloaded from cache, this makes sure we refresh the state.
        }

        /// <summary>
        /// Selects this tree node
        /// </summary>
        /// <param name="aConnectivityObject"></param>
        /// <returns></returns>
        public bool SelectConnectivityObject(NDCSObject aConnectivityObject)
        {
            if (aConnectivityObject is NDCSSystem)
            {
                if (ConnectivitySystem.Name.Equals(aConnectivityObject.Name))
                {
                    this.TreeView.SelectedNode = this;
                    return true;
                }
                return false;
            }
            else
            {
                return false;
            }
        }

        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            if ((TheConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded) && (_connectivitySystem.ConnectivitySupported))
            {
                _ndcsservicesFolder = new NDCSServicesFolder(_connectivitySystem);
                Nodes.Add(_ndcsservicesFolder);

                if (_connectivitySystem.UserHasInfoPrivilege)
                {
                    _ndcsdatasourceFolder = new DataSourcesFolder(_connectivitySystem);
                    Nodes.Add(_ndcsdatasourceFolder);
                }
            }
        }

        /// <summary>
        /// Refresh this node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            ConnectivitySystem.Refresh();
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
            TreeView.SelectedNode = Parent;
            TreeView.SelectedNode = this;
        }

    }
}
