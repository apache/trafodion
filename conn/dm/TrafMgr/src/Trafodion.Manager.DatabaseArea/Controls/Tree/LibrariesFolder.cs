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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for LibrariesFolder.
    /// </summary>
    public class LibrariesFolder : SchemaItemsFolder
    {

        /// <summary>
        /// Constructs a LibrariesFolder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public LibrariesFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Libraries, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Handles the Library created and dropped events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        override public void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {
            if (e.EventId == TrafodionObject.ChangeEvent.LibraryCreated || e.EventId == TrafodionObject.ChangeEvent.LibraryAltered)
            {
                DoRefresh(null);

                SelectTrafodionObject(e.ChangeInitiator);
            }
            else if (e.EventId == TrafodionObject.ChangeEvent.LibraryDropped)
            {
                DoRefresh(null);
            }
        }

        /// <summary>
        /// Selects the Libraries folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionLibrary || aTrafodionObject is JarClass)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (this.TreeView is BrowseLibraryTreeView)
                    {
                        if (theNode is LibraryFolder)
                        {
                            LibraryFolder theLibraryFolder = theNode as LibraryFolder;
                            if (aTrafodionObject is JarClass)
                            {
                                if (theLibraryFolder.SelectTrafodionObject(aTrafodionObject))
                                {
                                    return true;
                                }
                            }
                            else
                            {
                                if (theLibraryFolder.TheTrafodionLibrary.InternalName.Equals(theTargetInternalName))
                                {
                                    theLibraryFolder.TreeView.SelectedNode = theLibraryFolder;
                                    return true;
                                }
                            }
                        }
                    }
                    else
                    {
                        if (theNode is LibraryLeaf)
                        {
                            LibraryLeaf theLibraryLeaf = theNode as LibraryLeaf;
                            if (theLibraryLeaf.TrafodionLibrary.InternalName.Equals(theTargetInternalName))
                            {
                                theLibraryLeaf.TreeView.SelectedNode = theLibraryLeaf;
                                return true;
                            }
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the libraries list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        /// 
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionLibraries = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionLibraries;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionLibrary sqlMxLibrary in TheTrafodionSchema.TrafodionLibraries)
            {
                if (this.TreeView is BrowseLibraryTreeView)
                {
                    LibraryFolder libraryFolder = new LibraryFolder(sqlMxLibrary);
                    Nodes.Add(libraryFolder);
                }
                else
                {
                    LibraryLeaf libraryLeaf = new LibraryLeaf(sqlMxLibrary);
                    Nodes.Add(libraryLeaf);
                }
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionLibrary sqlMxLibrary in TheTrafodionSchema.TrafodionLibraries)
            {
                if (nameFilter.Matches(sqlMxLibrary.ExternalName))
                {
                    if (this.TreeView is BrowseLibraryTreeView)
                    {
                        LibraryFolder libraryFolder = new LibraryFolder(sqlMxLibrary);
                        Nodes.Add(libraryFolder);
                    }
                    else
                    {
                        LibraryLeaf libraryLeaf = new LibraryLeaf(sqlMxLibrary);
                        Nodes.Add(libraryLeaf);
                    }
                }
            }
        }

        /// <summary>
        /// This method lets the TreeNodes to add context menu items that are specific to the node
        /// The Navigation tree calls this method and passes a context menu strip to which the menu items need to be added
        /// The base nodes implementation of this method needs to be called first to have the common menu items added
        /// </summary>
        /// <param name="aContextMenuStrip">The context menu strip to which the menu items have to be added</param>
        override public void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            //If the database tree view allows context menu, show the context menus
            base.AddToContextMenu(aContextMenuStrip);

            SchemaFolder theSchemaFolder = (this.Parent) as SchemaFolder;
            if (TheTrafodionSchema.ConnectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS", "CREATE_LIBRARY"))
            {
                aContextMenuStrip.Items.Add(theSchemaFolder.GetShowBrowseLibraryToolMenuItem(this));
                aContextMenuStrip.Items.Add(theSchemaFolder.GetCreateLibraryMenuItem(this));
            }
        }

    }
}
