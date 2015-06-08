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

using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Model;


namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Represents the services folder in the navigation tree
    /// </summary>
    public class AdminRolesFolder : WmsTreeFolder
    {
        #region Properties

        /// <summary>
        /// Returns the WMS System associated with this folder
        /// </summary>
        public WmsSystem WmsSystem
        {
            get { return WmsObject as WmsSystem; }
        }

        #endregion Properties

        /// <summary>
        /// Default constructor for the services folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public AdminRolesFolder(WmsSystem aWmsSystem)
            :base(aWmsSystem, false)
        {
            Text = "Admin Roles";

        }
        
        /// <summary>
        /// Populates this tree node
        /// </summary>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();
        }

        /// <summary>
        /// Refreshes the list of services
        /// </summary>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            WmsSystem.WmsAdminRoles = null;
        } 

        /// <summary>
        /// Adds items to the context menu for the services folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);
        }
    }
}
