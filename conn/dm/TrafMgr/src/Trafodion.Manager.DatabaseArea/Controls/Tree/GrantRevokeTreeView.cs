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

using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Connections;
using System;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    public class CatalogConnectionFactory : ConnectionFolderFactory
    {
        public TrafodionCatalog _sqlMxCatalog;
        public CatalogConnectionFactory(TrafodionCatalog aTrafodionCatalog)
        {
            _sqlMxCatalog = aTrafodionCatalog;
        }

        override public NavigationTreeConnectionFolder NewNavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            return new Trafodion.Manager.DatabaseArea.Controls.Tree.SchemasFolder(_sqlMxCatalog);
        }
    }

    /// <summary>
    /// A database navigation tree view that supports checkboxes for each tree node
    /// and displays the Active systems which is a subset of the main database navigation tree.
    /// </summary>
    public class GrantRevokeTreeView : DatabaseTreeView
    {
        TrafodionCatalog _sqlMxCatalog;

        /// <summary>
        /// The various checked states of a Node
        /// </summary>
        public enum NodeState
        {
            NotSet, UnChecked, Checked, Partial
        };

        /// <summary>
        /// Constructs the database active system tree view
        /// </summary>
        public GrantRevokeTreeView(TrafodionCatalog aTrafodionCatalog)
        {
            _sqlMxCatalog = aTrafodionCatalog;
            CheckBoxes = true;
            NavigationTreeConnectionFolderFactory = new CatalogConnectionFactory(_sqlMxCatalog);
            BeforeCheck += TreeView_BeforeCheck;
        }


        protected override void WndProc(ref Message m)
        {
            // Suppress WM_LBUTTONDBLCLK
            if (m.Msg == 0x203) { m.Result = IntPtr.Zero; }
            else base.WndProc(ref m);
        }
        
        public override void SetAndPopulateRootFolders()
        {
            Nodes.Clear();
            CatalogsFolder catalogsFolder = new CatalogsFolder(_sqlMxCatalog.TrafodionSystem);
            Nodes.Add(catalogsFolder);
            catalogsFolder.DoPopulate(TheNavigationTreeNameFilter);
            SelectedNode = catalogsFolder;
            SelectedNode.Checked = true;
        }

        public override NavigationTreeConnectionFolder ExpandConnection(ConnectionDefinition aConnectionDefinition)
        {
            if (Nodes.Count > 0)
            {
                if (Nodes[0] is NavigationTreeConnectionFolder)
                {
                    return ((NavigationTreeConnectionFolder)Nodes[0]);
                }
            }
            throw new Exception("Cannot find catalogs folder for " + aConnectionDefinition.Name);

        }

        public override TreeNode FindByFullPath(string aFullPath)
        {
            string[] theNodeNames = aFullPath.Split(PathSeparator.ToCharArray(0, 1));

            TreeNode theTreeNode = FindByFullPath(Nodes, theNodeNames, 1);

            return theTreeNode;
        }
        
        /// <summary>
        /// Handles a check box being clicked on the tree view.
        /// Based on the node, the child nodes and/or parent node's checkbox states are also handled
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TreeView_BeforeCheck(object sender, TreeViewCancelEventArgs e)
        {
            if (e.Node is SchemaFolder ||
                e.Node is TableFolder ||
                e.Node is MaterializedViewFolder ||
                e.Node is ViewLeaf ||
                e.Node is ProcedureLeaf ||
                e.Node is LibraryLeaf )
            {
                //These are valid objects for which privileges can be granted.
                //Allow check to continue.
            }
            else
            {
                //Attemp to click on a object for which privileges is not supported.
                //cancel tree check event.
                e.Cancel = true;
            }

        }

        /// <summary>
        /// Disable the context menu on this tree view
        /// </summary>
        public override bool AllowContextMenu
        {
            get
            {
                return false;
            }
        }

    }
}
