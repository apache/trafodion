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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for TrafodionSchemaFolder.
    /// </summary>
    public class SchemaFolder : DatabaseTreeFolder, IHasTrafodionCatalog, IHasTrafodionSchema
    {
        private TablesFolder                  _theTablesFolder;
        private MaterializedViewsFolder       _theMaterializedViewsFolder;
        private MaterializedViewGroupsFolder  _theMaterializedViewGroupsFolder;
        private ViewsFolder                   _theViewsFolder;
        private SchemaIndexesFolder _theSchemaIndexesFolder;
        private ProceduresFolder _theProceduresFolder;
        private LibrariesFolder _theLibrariesFolder;
        private SynonymsFolder _theSynonymsFolder; 
        private SequencesFolder _theSequencesFolder;
        private FunctionsFolder _theFunctionsFolder;
        private FunctionActionsFolder _theFunctionActionsFolder;
        private TableMappingFunctionsFolder _theTableMappingFunctionFolder;

        /// <summary>
        /// The constructor 
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// TODO: Images/Icons for the folders to indicate selection state
        public SchemaFolder(TrafodionSchema aTrafodionSchema)
            :base(aTrafodionSchema)
        {
            ImageKey = DatabaseTreeView.DB_SCHEMA_ICON;
            SelectedImageKey = DatabaseTreeView.DB_SCHEMA_ICON;
        }

        /// <summary>
        /// Selects the specified node in the tree, as if user had clicked on the node
        /// </summary>
        /// <param name="aTrafodionObject"> the object that need be selected</param>
        /// <returns> true if the object was found, false otherwise</returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            DoPopulate(null);

            bool isFound = false;
            bool isSearchingTrafodionIndex = (aTrafodionObject as TrafodionIndex) != null;
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is SchemaItemsFolder)
                {
                    SchemaItemsFolder theSchemaItemsFolder = theTreeNode as SchemaItemsFolder;

                    /* CR 5672: Index SHOW DDL error when right click "Show DDL" menu item on Index
                     * If the searching target is TrafodionIndex, we should let the loop continue;
                     * If not, we should stop the loop since target has been found.
                     * 
                     * 1. TrafodionIndex may be shared by 2 index tree nodes(TableIndexLeaf & IndexLeaf) of different level in same tree
                     * 2. Each calling of theSchemaItemsFolder.SelectTrafodionObject(aTrafodionObject) method will load index node belong to itself
                     * 3. TableIndexLeaf node is ahead of IndexLeaf node, so TableIndexLeaf will be found first
                     * 4. It's very possible we are searching for IndexLeaf node, rather than TableIndexLeaf node
                     * 
                     * So to make sure IndexLeaf node will be loaded also, even if TableIndexLeaf has been found, 
                     * we should continue to search IndexLeaf node.
                     * 
                     */
                    if (!isSearchingTrafodionIndex)
                    {
                        if (theSchemaItemsFolder.SelectTrafodionObject(aTrafodionObject))
                        {
                            isFound = true;
                            break;
                        }
                    }
                    else
                    {
                        bool isTablesFolder = (theSchemaItemsFolder as TablesFolder) != null;
                        bool isIndexesFolder = (theSchemaItemsFolder as SchemaIndexesFolder) != null;
                        // Only search Tables folder or Indexes folder for better performance
                        if (isTablesFolder || isIndexesFolder)
                        {
                            if (theSchemaItemsFolder.SelectTrafodionObject(aTrafodionObject))
                            {
                                isFound = true;
                            }
                        }
                    }
                }
            }
            return isFound;
        }


        /// <summary>
        /// Refresh the contents of the folder
        /// </summary>
        /// <param name="aNameFilter"> specifications of filtering criteria to what subset of objects needs be shown</param> 
        /// TODO: Filtering feature
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.Refresh();
        }

        /// <summary>
        /// Add childern-folders to the specific "Schema" folder
        /// </summary>
        /// <param name="aNameFilter"> specifications of filtering criteria to what subset of objects needs be shown</param> 
        /// TODO: Filtering feature
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            if (this.TreeView is BrowseLibraryTreeView)
            {
                _theLibrariesFolder = new LibrariesFolder(TheTrafodionSchema);
                Nodes.Add(_theLibrariesFolder);
            }
            else
            {
                _theTablesFolder = new TablesFolder(TheTrafodionSchema);
                //_theMaterializedViewsFolder = new MaterializedViewsFolder(TheTrafodionSchema);
                //_theMaterializedViewGroupsFolder = new MaterializedViewGroupsFolder(TheTrafodionSchema);
                _theViewsFolder = new ViewsFolder(TheTrafodionSchema);
                _theSchemaIndexesFolder = new SchemaIndexesFolder(TheTrafodionSchema);
                _theSequencesFolder = new SequencesFolder(TheTrafodionSchema);
                _theLibrariesFolder = new LibrariesFolder(TheTrafodionSchema);
                _theProceduresFolder = new ProceduresFolder(TheTrafodionSchema);
                //_theSynonymsFolder = new SynonymsFolder(TheTrafodionSchema);

                Nodes.Add(_theTablesFolder);
                //Nodes.Add(_theMaterializedViewsFolder);
                //Nodes.Add(_theMaterializedViewGroupsFolder);
                Nodes.Add(_theViewsFolder);
                Nodes.Add(_theSchemaIndexesFolder);
                Nodes.Add(_theLibrariesFolder);
                Nodes.Add(_theProceduresFolder);
                Nodes.Add(_theSequencesFolder);
                
                //if (TheTrafodionSchema.Version >= 2500 && TheTrafodionSchema.TrafodionUDFunctions.Count > 0)
                //{
                //    _theFunctionsFolder = new FunctionsFolder(TheTrafodionSchema);
                //    Nodes.Add(_theFunctionsFolder);
                //}

                //if (TheTrafodionSchema.Version >= 2500 && TheTrafodionSchema.TrafodionFunctionActions.Count > 0)
                //{
                //    _theFunctionActionsFolder = new FunctionActionsFolder(TheTrafodionSchema);
                //    Nodes.Add(_theFunctionActionsFolder);
                //}

                //if (TheTrafodionSchema.Version >= 2500 && TheTrafodionSchema.TrafodionTableMappingFunctions.Count > 0)
                //{
                //    _theTableMappingFunctionFolder = new TableMappingFunctionsFolder(TheTrafodionSchema);
                //    Nodes.Add(_theTableMappingFunctionFolder);
                //}

                //Nodes.Add(_theSynonymsFolder);
            }
        }

        /// <summary>
        /// returns the Schema object
        /// </summary>
           public TrafodionSchema TheTrafodionSchema
        {
            get { return (TrafodionSchema)this.TrafodionObject; }
        }

        /// <summary>
        /// returns in the format e.g. "Schema CatalogName.ABC"
        /// </summary>
        override public string LongerDescription
        {
            // to make it with space "Schema " 
            get { return Properties.Resources.Schema + "  " + TheTrafodionSchema.VisibleAnsiName; } 
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

            //if (TheTrafodionSchema.ConnectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS","CREATE_PROCEDURE"))
            //{
            //    aContextMenuStrip.Items.Add(GetShowPCFToolMenuItem(this));
            //}
            //aContextMenuStrip.Items.Add(GetCreateProcedureMenuItem(this));
        }

        /// <summary>
        /// Static method to create a Create procedure context menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public ToolStripMenuItem GetCreateProcedureMenuItem(TreeNode node)
        {
            ToolStripMenuItem createProcMenuItem = new ToolStripMenuItem(Properties.Resources.CreateProcedure + "...");
            createProcMenuItem.Tag = node;
            createProcMenuItem.Click += new EventHandler(createProcFromLibMenuItem_Click);

            return createProcMenuItem;
        }

        /// <summary>
        /// Event handler for the create procedure for M7 and later
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void createProcFromLibMenuItem_Click(object sender, EventArgs e)
        {  
            CreateSPJFromLibrary createProcedureDialog = new CreateSPJFromLibrary(this.TheTrafodionSchema);
            createProcedureDialog.ShowDialog();
        }

        /// <summary>
        /// Static method to create a Library code file menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public ToolStripMenuItem GetShowBrowseLibraryToolMenuItem(TreeNode node)
        {
            ToolStripMenuItem showBrowseLibMenuItem = new ToolStripMenuItem(Properties.Resources.BrowseLibraryTool);
            showBrowseLibMenuItem.Tag = node;
            showBrowseLibMenuItem.Click += new EventHandler(showBrowseLibMenuItem_Click);            

            return showBrowseLibMenuItem;
        }
        /// <summary>
        /// Static method to create a Create Library context menu item
        /// </summary>
        /// <returns>The context menu item</returns>
        public ToolStripMenuItem GetCreateLibraryMenuItem(TreeNode node)
        {
            ToolStripMenuItem createLibraryMenuItem = new ToolStripMenuItem(Properties.Resources.CreateLibrary + "...");
            createLibraryMenuItem.Tag = node;
            createLibraryMenuItem.Click += new EventHandler(createLibraryMenuItem_Click);

            return createLibraryMenuItem;
        }

        /// <summary>
        /// Event handler for the Library code file menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void showBrowseLibMenuItem_Click(object sender, EventArgs e)
        {

            // Get the sqlObject from the node that was selected.
            Object sourceNode = ((ToolStripMenuItem)sender).Tag;
            TrafodionObject theTrafodionObject = null;

            if (sourceNode is DatabaseTreeFolder)
            {
                theTrafodionObject = ((DatabaseTreeFolder)sourceNode).TrafodionObject;
            }
            else if (sourceNode is DatabaseTreeNode)
            {
                theTrafodionObject = ((DatabaseTreeNode)sourceNode).TrafodionObject;
            }
            BrowseLibraryDialog browseLibraryDialog = new BrowseLibraryDialog(theTrafodionObject, false);
            browseLibraryDialog.ShowDialog();

        }
        /// <summary>
        /// Event handler for the create Library menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void createLibraryMenuItem_Click(object sender, EventArgs e)
        {
            CreateLibraryUserControl cld = new CreateLibraryUserControl(this.TheTrafodionSchema);
            cld.ShowDialog();
        }

        public override void TrafodionObject_ModelChangedEvent(object sender, TrafodionObject.TrafodionModelChangeEventArgs e)
        {
            if (e.EventId == TrafodionObject.ChangeEvent.SchemaViewValidated && e.ChangeInitiator is TrafodionSchema)
            {
                DoRefresh(null);
                SelectTrafodionObject(e.ChangeInitiator);
            }
        }
            

        #region IHasTrafodionCatalog Members

        /// <summary>
        /// The model of the catalog in which the schema exists
        /// </summary>
        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return TheTrafodionSchema.TheTrafodionCatalog; }
        }

        #endregion
    }

}
