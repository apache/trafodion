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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea
{
    /// <summary>
    /// This control is used to navigate through the list of Systems. There is no favorites area.
    /// </summary>
    public partial class SystemsNavigator : UserControl
    {
        /// <summary>
        /// The tree used to navigate amongsts the systems and their respective folders.
        /// </summary>
        public NavigationTreeView NavigationTree
        {
            get { return _navigationTreeViewUserControl.NavigationTreeView; }
        }

        /// <summary>
        /// Creates a new instance.
        /// </summary>
        public SystemsNavigator()
        {
            InitializeComponent();

            // Create a factory that makes our kind of connections folder and tell the navigation
            // tree to use it.
            _navigationTreeViewUserControl.NavigationTreeView.NavigationTreeConnectionFolderFactory = new SystemConnectionFolderFactory();

            // Tell the navigation tree to refresh itself to start things off.
            _navigationTreeViewUserControl.NavigationTreeView.SetAndPopulateRootFolders();
        }
    }

    /// <summary>
    /// This is the factory that knows how to create our specialized connection folders.
    /// </summary>
    public class SystemConnectionFolderFactory : NavigationTreeConnectionFolderFactory
    {

        /// <summary>
        /// This method is called when the tree needs a new connection folder
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition that that the folder will represent</param>
        /// <returns>our specialized connection folder which must be derived from NavigationTreeConnectionFolder</returns>
        public override NavigationTreeConnectionFolder NewNavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {

            // Create a new instance our our specialized connection folder and return it
            return new SystemConnectionFolder(aConnectionDefinition);

        }
    }

    /// <summary>
    /// Our specialized connection folder derived from NavigationTreeConnectionFolder and
    /// adding our special behaviors.
    /// </summary>
    public class SystemConnectionFolder : NavigationTreeConnectionFolder
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aConnectionDefinition">the connection definition that that the folder will represent</param>
        public SystemConnectionFolder(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
            // The NavigationTreeConnectionFolder puts a node in by default so that the folder's contents
            // can be expanded by clicking the "plus" sign. Since we don't have any children under a system node,
            // we'll clear out the node list.
            Nodes.Clear();
        }

        /// <summary>
        /// Called to always repopulate our children
        /// </summary>
        /// <param name="aNavigationTreeNameFilter">the name filter to be used</param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            // And populate
            Populate(aNavigationTreeNameFilter);
        }
    }
}
