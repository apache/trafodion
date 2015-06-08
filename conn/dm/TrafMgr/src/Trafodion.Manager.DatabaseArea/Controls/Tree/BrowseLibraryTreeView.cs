//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
    /// <summary>
    /// A database navigation tree view that displays libraries objects 
    /// </summary>
    public class BrowseLibraryTreeView : DatabaseTreeView
    {
        TrafodionCatalog _sqlMxCatalog;

   
        /// <summary>
        /// Constructs the libraries tree view
        /// </summary>
        public BrowseLibraryTreeView(TrafodionCatalog aTrafodionCatalog)
        {
            _sqlMxCatalog = aTrafodionCatalog;
            NavigationTreeConnectionFolderFactory = new CatalogConnectionFactory(_sqlMxCatalog);
            Selected += new SelectedHandler(BrowseLibraryTreeView_Selected);
        }

        private void BrowseLibraryTreeView_Selected(NavigationTreeNode aTreeNode)
        {
            TrafodionObject theTrafodionObject = null;

            if (aTreeNode is LibrariesFolder)
            {
                LibrariesFolder librariesFolder = aTreeNode as LibrariesFolder;
                theTrafodionObject = librariesFolder.TrafodionObject;
            }
            else if (aTreeNode is LibraryLeaf)
            {
                LibraryLeaf libraryLeaf = aTreeNode as LibraryLeaf;
                theTrafodionObject = libraryLeaf.TrafodionObject;
            }

            if (theTrafodionObject != null)
            {
                if (theTrafodionObject is IHasTrafodionCatalog)
                {
                    //TheSelectedTrafodionCatalog = ((IHasTrafodionCatalog)theTrafodionObject).TheTrafodionCatalog;
                }

                if (theTrafodionObject is IHasTrafodionSchema)
                {
                    //TheSelectedTrafodionSchema = ((IHasTrafodionSchema)theTrafodionObject).TheTrafodionSchema;
                }

            }        
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
            NavigationTreeNameFilter treeNameFilter = new NavigationTreeNameFilter();
            catalogsFolder.DoPopulate(treeNameFilter);
                       
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
