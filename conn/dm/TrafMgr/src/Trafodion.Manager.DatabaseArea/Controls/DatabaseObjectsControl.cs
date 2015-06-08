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
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class DatabaseObjectsControl : UserControl, Trafodion.Manager.Framework.Controls.ICloneToWindow
    {

        // The parent node of the previously selected node
        private TreeNode _thePreviousParent = null;

        // Mapping of parent node to most recently selected tab in one of its children
        private Dictionary<TreeNode, string> _theParentToTabName = new Dictionary<TreeNode, string>();

        // Since we cannot tell when nodes are removed from the tree we cannot know when to remove them from 
        // the dictionary.  Instead, we keep a parallel LRU with a limit.  When nodes fall off the LRU, we
        // remove them from the dictionary.  The LRU member limit is set high enough that the user probably 
        // won't notice that we've forgotten the select tab for some folder later on.  ;^)
        private int _theRecentParentsLimit = 50;
        private List<TreeNode> _theRecentParents = new List<TreeNode>();


        public DatabaseObjectsControl(DatabaseNavigator aDatabaseAreaLeftControl)
            : this()
        {
            TheDatabaseAreaLeftControl = aDatabaseAreaLeftControl;
        }

        public DatabaseObjectsControl()
        {

            InitializeComponent();

            TheDatabaseAreaLeftControl = null;

            // Initially non-blank to make them easy to see in the designer so set them empty now
            this.topPanel_richTextBox.Text = "";
        }

        public Control Clone()
        {
            DatabaseObjectsControl theClonedDatabaseAreaObjectsControl = new DatabaseObjectsControl();
            string linkText = "";
            if (topPanel_richTextBox.Controls.ContainsKey("LinkLabel"))
                linkText = ((LinkLabel)topPanel_richTextBox.Controls["LinkLabel"]).Text;

            theClonedDatabaseAreaObjectsControl.SetLabels(this.topPanel_richTextBox.Text, linkText);

            if (theMostRecentWorkObject is TreeNode)
            {
                theClonedDatabaseAreaObjectsControl.TrafodionTreeControlSelected(theMostRecentWorkObject as NavigationTreeNode);
            }
            else if (theMostRecentWorkObject is Exception)
            {
                theClonedDatabaseAreaObjectsControl.TrafodionTreeControlExceptionOccurred(theMostRecentWorkObject as Exception);
            }

            theClonedDatabaseAreaObjectsControl.Size = Size;

            return theClonedDatabaseAreaObjectsControl;
        }

        public string WindowTitle
        {
            get { return "" + " | " + this.topPanel_richTextBox.Text; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            // TBD_YOGI- July 17th
            get
            {
                ConnectionDefinition connDef = null;
                if (theMostRecentWorkObject is TreeNode)
                {
                    // Get the sqlObject from the node that was selected.
                    Object sourceNode = theMostRecentWorkObject as NavigationTreeNode;
                    TrafodionObject theTrafodionObject = null;
                    if (sourceNode is DatabaseTreeFolder)
                    {
                        theTrafodionObject = ((DatabaseTreeFolder)sourceNode).TrafodionObject;
                    }
                    else if (sourceNode is DatabaseTreeNode)
                    {
                        theTrafodionObject = ((DatabaseTreeNode)sourceNode).TrafodionObject;
                    }
                    if (theTrafodionObject != null)
                    {
                        connDef = theTrafodionObject.ConnectionDefinition;
                    }
                }

                /***
                else  if (theMostRecentWorkObject is Exception)
                {
                    return null;
                }**/

                return connDef;
            } 
        }




        void TrafodionTreeControlSelected(NavigationTreeNode aNavigationTreeNode)
        {
            if (Trafodion.Manager.Framework.Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            //Console.WriteLine("Entered database tree selection : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));

            //Have the DatabaseTreeView listen to database area option change event, if it is not already listening.
            if (TheDatabaseAreaLeftControl != null && TheDatabaseAreaLeftControl.DatabaseTreeView != null)
            {
                TheDatabaseAreaLeftControl.DatabaseTreeView.CheckAndAddListener();
            }

            theMostRecentWorkObject = aNavigationTreeNode;
            string theTopPanelUpperText = "";
            string theTopPanelLowerText = "";
            string theTopPanelLowerLabelRightText = "";
            object theTopPanelLowerLabelRightTextTag = null;
            string initialTabPageText = "";

            if (TheTabControl != null && TheTabControl.SelectedTab != null)
                initialTabPageText = TheTabControl.SelectedTab.Text;

            try
            {

                // If there was a previous parent (none if root selected) and there is a tab control
                // and a tab is selected, we want to reselect that same tab if any child of that parent is
                // selected in the future.
                if ((_thePreviousParent != null) && (TheTabControl != null) && (TheTabControl.SelectedTab != null))
                {

                    // Get the text of the selected tab
                    string theSelectedTabText = TheTabControl.SelectedTab.Text;

                    // Check to see if we already have an entry for this parent
                    if (_theParentToTabName.ContainsKey(_thePreviousParent))
                    {

                        // We do.  Overstore the previous entry.
                        _theParentToTabName[_thePreviousParent] = theSelectedTabText;

                        // Remove this parent from wherever it is in the LRU
                        _theRecentParents.Remove(_thePreviousParent);

                    }
                    else
                    {

                        // There is no entry for this parent, so add it to the dictionary.
                        _theParentToTabName.Add(_thePreviousParent, theSelectedTabText);
                    }

                    // At this point, the parent is not in the LRU so add it to the front
                    _theRecentParents.Insert(0, _thePreviousParent);

                    // Clear off the tail of the LRU if it is above the entry count limit
                    while (_theRecentParents.Count > _theRecentParentsLimit)
                    {

                        // Remove the corresponding dictionary entry which is the real goal
                        _theParentToTabName.Remove(_theRecentParents[_theRecentParentsLimit]);

                        // Remove the LRU entry
                        _theRecentParents.RemoveAt(_theRecentParentsLimit);

                    }

                }

                ConnectionDefinition theConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
                theTopPanelLowerText = aNavigationTreeNode.LongerDescription;
                theTopPanelLowerLabelRightText = aNavigationTreeNode.LongerDescriptionLink;
                theTopPanelLowerLabelRightTextTag = aNavigationTreeNode.LongerDescriptionLinkTag;


                if (aNavigationTreeNode is TableFolder)
                {
                    TableFolder theTableFolder = aNavigationTreeNode as TableFolder;
                    TheTabControl = new TableTabControl(this, theTableFolder.TrafodionTable);
                }
                else if (aNavigationTreeNode is MaterializedViewFolder)
                {
                    MaterializedViewFolder theMaterializedViewFolder = aNavigationTreeNode as MaterializedViewFolder;
                    TheTabControl = new MaterializedViewTabControl(this, theMaterializedViewFolder.TrafodionMaterializedView);
                }
                else if (aNavigationTreeNode is MaterializedViewGroupLeaf)
                {
                    MaterializedViewGroupLeaf theMaterializedViewGroupLeaf = aNavigationTreeNode as MaterializedViewGroupLeaf;
                    TheTabControl = new MaterializedViewGroupTabControl(this, theMaterializedViewGroupLeaf.TrafodionMaterializedViewGroup);
                }
                else if (aNavigationTreeNode is ViewLeaf)
                {
                    ViewLeaf theViewLeaf = aNavigationTreeNode as ViewLeaf;
                    TheTabControl = new ViewTabControl(this, theViewLeaf.TrafodionView);
                }
                else if (aNavigationTreeNode is LibraryLeaf)
                {
                    LibraryLeaf theLibraryleaf = aNavigationTreeNode as LibraryLeaf;
                    TheTabControl = new LibraryTabControl(this, theLibraryleaf.TrafodionLibrary);
                }
                else if (aNavigationTreeNode is ProcedureLeaf)
                {
                    ProcedureLeaf theProcedureLeaf = aNavigationTreeNode as ProcedureLeaf;
                    TheTabControl = new ProcedureTabControl(this, theProcedureLeaf.TrafodionProcedure);
                }
                else if (aNavigationTreeNode is FunctionLeaf)
                {
                    FunctionLeaf theFunctionLeaf = aNavigationTreeNode as FunctionLeaf;
                    TheTabControl = new FunctionTabControl(this, theFunctionLeaf.TrafodionUDF);
                }
                else if (aNavigationTreeNode is FunctionActionLeaf)
                {
                    FunctionActionLeaf theFunctionActionLeaf = aNavigationTreeNode as FunctionActionLeaf;
                    TheTabControl = new FunctionActionTabControl(this, theFunctionActionLeaf.TrafodionFunctionAction);
                }
                else if (aNavigationTreeNode is UniversalFunctionFolder)
                {
                    UniversalFunctionFolder theUniversalFunctionFolder = aNavigationTreeNode as UniversalFunctionFolder;
                    TheTabControl = new FunctionTabControl(this, theUniversalFunctionFolder.TrafodionUDFunction);
                }
                else if (aNavigationTreeNode is TableMappingFunctionLeaf)
                {
                    TableMappingFunctionLeaf theTableMappingFunctionLeaf = aNavigationTreeNode as TableMappingFunctionLeaf;
                    TheTabControl = new TableMappingFunctionTabControl(this, theTableMappingFunctionLeaf.TrafodionTableMappingFunction);
                }
                else if (aNavigationTreeNode is SynonymLeaf)
                {
                    SynonymLeaf theSynonymLeaf = aNavigationTreeNode as SynonymLeaf;
                    TheTabControl = new SynonymTabControl(this, theSynonymLeaf.TrafodionSynonym);
                }
                else if (aNavigationTreeNode is SequenceLeaf)
                {
                    SequenceLeaf theSequenceLeaf = aNavigationTreeNode as SequenceLeaf;
                    TheTabControl = new SequenceTabControl(this, theSequenceLeaf.TrafodionSequence);
                }
                else if (aNavigationTreeNode is CatalogFolder)
                {
                    CatalogFolder theCatalogFolder = aNavigationTreeNode as CatalogFolder;
                    TheTabControl = new CatalogTabControl(this, theCatalogFolder.TheTrafodionCatalog);
                }
                else if (aNavigationTreeNode is SchemaFolder)
                {
                    SchemaFolder theSchemaFolder = aNavigationTreeNode as SchemaFolder;
                    TheTabControl = new SchemaTabControl(this, theSchemaFolder.TheTrafodionSchema);
                }
                else if (aNavigationTreeNode is IndexLeaf)
                {
                    IndexLeaf theIndexLeaf = aNavigationTreeNode as IndexLeaf;
                    TheTabControl = new IndexTabControl(this, theIndexLeaf.TrafodionIndex);
                }
                else if (aNavigationTreeNode is TableTriggerLeaf)
                {
                    TableTriggerLeaf theTableTriggerLeaf = aNavigationTreeNode as TableTriggerLeaf;
                    TheTabControl = new TriggerTabControl(this, (TrafodionTrigger)theTableTriggerLeaf.TrafodionTrigger);
                }
                else // The following all simply use a TrafodionTabControl instead of a derived one
                {

                    // Use a TrafodionTabControl
                    TheTabControl = new TrafodionTabControl();

                    if ((theConnectionDefinition != null) && (theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded))
                    {
                        AddToTabControl(new FixSystemTabPage(theConnectionDefinition));
                    }
                    else if (aNavigationTreeNode is Trafodion.Manager.Framework.Navigation.NavigationTreeConnectionsFolder)
                    {
                        AddToTabControl(new MySystemsTabPage());
                    }
                    else if (aNavigationTreeNode is CatalogsFolder)
                    {
                        CatalogsFolder theCatalogsFolder = aNavigationTreeNode as CatalogsFolder;
                        AddToTabControl(new CatalogsTabPage(this, theCatalogsFolder.TheTrafodionSystem));
                        //AddToTabControl(new CatalogsSpaceUsagesTabPage(this, theCatalogsFolder.TheTrafodionSystem));
                    }
                    else if (aNavigationTreeNode is SchemasFolder)
                    {
                        SchemasFolder theSchemasFolder = aNavigationTreeNode as SchemasFolder;
                        AddToTabControl(new SchemasTabPage(this, theSchemasFolder.TheTrafodionCatalog));
                        //AddToTabControl(new CatalogSpaceUsagesTabPage(this, theSchemasFolder.TheTrafodionCatalog));
                    }
                    else if (aNavigationTreeNode is TablesFolder)
                    {
                        TablesFolder theTablesFolder = aNavigationTreeNode as TablesFolder;
                        AddTablesPage(theTablesFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is MaterializedViewsFolder)
                    {
                        MaterializedViewsFolder theMaterializedViewsFolder = aNavigationTreeNode as MaterializedViewsFolder;
                        AddMaterializedViewsPage(theMaterializedViewsFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is MaterializedViewGroupsFolder)
                    {
                        MaterializedViewGroupsFolder theMaterializedViewGroupsFolder = aNavigationTreeNode as MaterializedViewGroupsFolder;
                        AddMaterializedViewGroupsPage(theMaterializedViewGroupsFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is SchemaIndexesFolder)
                    {
                        SchemaIndexesFolder theSchemaIndexesFolder = aNavigationTreeNode as SchemaIndexesFolder;
                        AddToTabControl(new TrafodionIndexesTabPage(this, theSchemaIndexesFolder.TheTrafodionSchema));
                        //AddToTabControl(new IndexesSpaceUsageSummaryTabPage(this, theSchemaIndexesFolder.TheTrafodionSchema));
                    }
                    else if (aNavigationTreeNode is ProceduresFolder)
                    {
                        ProceduresFolder theProceduresFolder = aNavigationTreeNode as ProceduresFolder;
                        AddProceduresPage(theProceduresFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is LibrariesFolder)
                    {
                        LibrariesFolder theLibrariesFolder = aNavigationTreeNode as LibrariesFolder;
                        AddLibrariesPage(theLibrariesFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is FunctionActionsFolder)
                    {
                        FunctionActionsFolder theFunctionActionsFolder = aNavigationTreeNode as FunctionActionsFolder;
                        if (theFunctionActionsFolder.TrafodionUDFunction == null)
                        {
                            AddToTabControl(new TrafodionFunctionActionsTabPage(this, theFunctionActionsFolder.TheTrafodionSchema));
                        }
                        else
                        {
                            AddToTabControl(new TrafodionFunctionActionsTabPage(this, theFunctionActionsFolder.TrafodionUDFunction));
                        }
                    }
                    else if (aNavigationTreeNode is TableMappingFunctionsFolder)
                    {
                        TableMappingFunctionsFolder theTableMappingFolder = aNavigationTreeNode as TableMappingFunctionsFolder;
                        AddTableMappingFunctionsPage(theTableMappingFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is SynonymsFolder)
                    {
                        SynonymsFolder theSynonymsFolder = aNavigationTreeNode as SynonymsFolder;
                        AddSynonymsPage(theSynonymsFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is SequencesFolder)
                    {
                        SequencesFolder theSequenceFolder = aNavigationTreeNode as SequencesFolder;
                        AddSynonymsPage(theSequenceFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is ViewsFolder)
                    {
                        ViewsFolder theViewsFolder = aNavigationTreeNode as ViewsFolder;
                        AddViewsPage(theViewsFolder.TheTrafodionSchema);
                    }
                    else if (aNavigationTreeNode is TableIndexesFolder)
                    {
                        TableIndexesFolder theTableIndexesFolder = aNavigationTreeNode as TableIndexesFolder;
                        AddTableIndexesPage(theTableIndexesFolder.TrafodionTable);
                        //AddToTabControl(new IndexesSpaceUsageSummaryTabPage(this, theTableIndexesFolder.TrafodionTable));
                    }
                    else if (aNavigationTreeNode is MaterializedViewIndexesFolder)
                    {
                        MaterializedViewIndexesFolder theMVIndexesFolder = aNavigationTreeNode as MaterializedViewIndexesFolder;
                        AddMVIndexesPage(theMVIndexesFolder.TrafodionMaterializedView);
                        //AddToTabControl(new IndexesSpaceUsageSummaryTabPage(this, theMVIndexesFolder.TrafodionMaterializedView));
                    }
                    else if (aNavigationTreeNode is TableTriggersFolder)
                    {
                        TableTriggersFolder theTableTriggersFolder = aNavigationTreeNode as TableTriggersFolder;
                        AddTableTriggersPage(theTableTriggersFolder.TrafodionTable);

                    }
                    else
                    {
                        AddToTabControl(new ToDoTabPage());
                    }
                }
                //Console.WriteLine("created right pane control : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));

                // Try to select a tab with the same text (not name) as before
                {
                    bool selected = false;

                    // Save this node's parent for next time
                    _thePreviousParent = aNavigationTreeNode.Parent;

                    if (initialTabPageText.Equals(Properties.Resources.SpaceUsage) || initialTabPageText.Equals("Partitions"))
                    {
                        TabPage spaceTab = null;

                        foreach (TabPage tab in TheTabControl.TabPages)
                        {
                            if (tab.Text.Equals(initialTabPageText) || tab.Text.Equals("Partitions"))
                            {
                                spaceTab = tab;
                                break;
                            }
                        }

                        if (spaceTab != null)
                        {
                            selected = true;
                            TheTabControl.SelectTab(spaceTab);
                        }
                    }

                    if(!selected)
                    {
                        // Check to see if this node has a parent and there is an entry for this node's parent
                        if ((_thePreviousParent != null) && (_theParentToTabName.ContainsKey(_thePreviousParent)))
                        {

                            // There is, retrieve the saved tab text
                            string theSelectedTabText = _theParentToTabName[_thePreviousParent];

                            // Look through all of the tabs
                            foreach (TabPage theTabPage in TheTabControl.TabPages)
                            {

                                // Check to see if this tab has text and it matches
                                if ((theTabPage.Text != null) && (theTabPage.Text.Equals(theSelectedTabText)))
                                {

                                    // It does, select that tab
                                    TheTabControl.SelectTab(theTabPage);

                                    // Note that we did select a tab
                                    selected = true;

                                    // And exit the loop
                                    break;
                                }
                            }
                        }

                        // Check to see if a tab was selected
                        if (!selected && TheTabControl.TabCount > 0)
                        {
                            TheTabControl.SelectTab(0);
                        }
                    }
                }

            }
            catch (Exception anException)
            {
                // TODO: Deal with exception if object loaded into the tabcontrol is null
                AddToTabControl(anException);
            }

            SetLabels(theTopPanelLowerText, theTopPanelLowerLabelRightText);
            SetLinkLabelTag(theTopPanelLowerLabelRightTextTag);
            //Console.WriteLine("after set labels : " + DateTime.Now.ToString("yyyy-MM-dd hh':'mm':'ss.FFFFFF"));
        }

        private delegate void SetLabelsDelegate(string aTopPanelLowerText, string aTopPanelLowerLabelRightText);
        private void SetLabels(string aTopPanelLowerText, string aTopPanelLowerLabelRightText)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLabelsDelegate(SetLabels), new object[] { aTopPanelLowerText, aTopPanelLowerLabelRightText });
            }
            else
            {

                _dataBaseObjectsToolTip.SetToolTip(this.topPanel_richTextBox, aTopPanelLowerText);

                this.topPanel_richTextBox.Controls.Clear();      
                this.topPanel_richTextBox.Text = aTopPanelLowerText;

                if (aTopPanelLowerLabelRightText.Length > 0)
                {
                    Point linkLabelPosition = this.topPanel_richTextBox.GetPositionFromCharIndex(aTopPanelLowerText.Length);                   
                    LinkLabel ll = new LinkLabel();
                    ll.AutoEllipsis = true;
                    ll.AutoSize = true;
                    ll.Location = linkLabelPosition;
                    ll.Text = aTopPanelLowerLabelRightText;
                    ll.Links.Add(0, aTopPanelLowerLabelRightText.Length);
                    ll.Name = "LinkLabel";
                    ll.LinkClicked += new LinkLabelLinkClickedEventHandler(ll_LinkClicked);
                    this.topPanel_richTextBox.Controls.Add(ll);
                }

                //Rectangle r = theTopPanelLowerLabel.Bounds;
                //theTopPanelLowerLabelRight.Location = new Point(r.X + r.Width + 10, r.Y);

                //theTopPanelLowerLabelRight.Text = "";//aTopPanelLowerLabelRightText;
            
            }
        }



        private delegate void SetLinkLabelTagDelegate(Object linkLabelTag);
        private void SetLinkLabelTag(Object linkLabelTag)
        {
            if (InvokeRequired)
            {
                Invoke(new SetLinkLabelTagDelegate(SetLinkLabelTag), new object[] { linkLabelTag });
            }
            else
            {
                if(this.topPanel_richTextBox.Controls.ContainsKey("LinkLabel"))
               ((LinkLabel)this.topPanel_richTextBox.Controls["LinkLabel"]).Links[0].Tag = linkLabelTag;
            }
        }
        

        private void TrafodionTreeControlExceptionOccurred(Exception anException)
        {
            theMostRecentWorkObject = anException;
            TrafodionTreeControlExceptionOccurredThreadProc(anException);
        }

        private void TrafodionTreeControlExceptionOccurredThreadProc(object anObject)
        {
            AddToTabControl(anObject as Exception);
        }

        private void FilterChanged(NavigationTreeNameFilter aNameFilter)
        {
            theNameFilter = aNameFilter;
            if ((theMostRecentWorkObject != null) && (theMostRecentWorkObject is TreeNode))
            {
                TrafodionTreeControlSelected(theMostRecentWorkObject as NavigationTreeNode);
            }
        }

        private void AddToTabControl(Exception anException)
        {
            string theMessage = null;
            Control theControl = null;
            string theTabText = Properties.Resources.Exception; // "Exception";

            // Fresh tab control containing only this exception
            TheTabControl = new TrafodionTabControl();

            if (anException is OdbcException)
            {
                theTabText = Properties.Resources.ODBCException; //  "ODBC Exception";
                theMessage = ConnectionDefinition.FixOdbcExceptionMessage((OdbcException)anException);
            }
            else if (anException is PasswordNotSetException)
            {
                theControl = new FixSystemTabPage(((PasswordNotSetException)anException).TheConnectionDefinition);
            }
            else if (anException is MostRecentConnectionTestFailedException)
            {
                theControl = new FixSystemTabPage(((MostRecentConnectionTestFailedException)anException).TheConnectionDefinition);
            }
            else
            {
                theMessage = anException.Message;
            }

            if (theMessage != null)
            {
#if DEBUG

                // Include a stack trace and do not center
                theMessage += "\n" + anException.StackTrace;
                TrafodionTextBox theMessageTextBox = new TrafodionTextBox();
                theMessageTextBox.ReadOnly = true;
                theMessageTextBox.WordWrap = true;
                theMessageTextBox.Multiline = true;
                theMessageTextBox.Text = theMessage;

                AddToTabControl(theTabText, theMessageTextBox);

#else
                AddToTabControl(theTabText, theMessage);
#endif
            }
            else if (theControl != null)
            {
                if (theControl is TabPage)
                {
                    AddToTabControl((TabPage)theControl);
                }
                else
                {
                    AddToTabControl(theTabText, theControl);
                }
            }
            else
            {
                AddToTabControl(theTabText, anException.ToString());
            }
        }

        private void AddToTabControl(string aName, string aMessage)
        {
            TrafodionRichTextBox theMessageTextBox = new TrafodionRichTextBox();
            theMessageTextBox.WordWrap = true;
            theMessageTextBox.Multiline = true;
            theMessageTextBox.ReadOnly = true;
            theMessageTextBox.Text = aMessage;
        }

        private void AddToTabControl(string aName, Control aControl)
        {
            if (aControl is DataGridView)
            {
                DataGridView theDataGridView = aControl as DataGridView;
                if (theDataGridView.Rows.Count == 0)
                {
                    AddToTabControl(aName, Properties.Resources.ThereIsNothingIn + aName); //"There is nothing in " + aName);
                    return;
                }
            }
            TabPage theTabPage = new TrafodionTabPage(aName);
            aControl.Dock = DockStyle.Fill;
            aControl.BackColor = Color.WhiteSmoke;
            theTabPage.Controls.Add(aControl);
            AddToTabControl(theTabPage);
        }

        private void AddToTabControl(TabPage aTabPage)
        {
            aTabPage.Size = new System.Drawing.Size(1, 1); //To prevent a small square from being shown until the tab page resizes
            TheTabControl.TabPages.Add(aTabPage);
        }

        //private void AddSchemaAttributesPage(TrafodionSchema aTrafodionSchema)
        //{
        //    AddToTabControl(new SchemaAttributesTabPage(this, aTrafodionSchema));
        //}

        private void AddTablesPage(TrafodionSchema aTrafodionSchema)
        {
            // last arg is the name/string that appears as the tab-name e.g. "Tables"  etc.
            AddToTabControl(new TablesTabPage(this, aTrafodionSchema, Properties.Resources.Tables));
            //AddToTabControl(new TablesSpaceUsageSummaryTabPage(this, aTrafodionSchema));
        }

        private void AddToTabControl(MaterializedViewsFolder aMaterializedViewsFolder)
        {
            TrafodionSchema theTrafodionSchema = aMaterializedViewsFolder.TheTrafodionSchema;
            AddMaterializedViewsPage(theTrafodionSchema);
        }

        private void AddMaterializedViewsPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new MaterializedViewsTabPage(this, aTrafodionSchema, Properties.Resources.MaterializedViews));
            //AddToTabControl(new MVsSpaceUsageSummaryTabPage(this, aTrafodionSchema));
        }

        private void AddToTabControl(ProceduresFolder aProceduresFolder)
        {
            TrafodionSchema theTrafodionSchema = aProceduresFolder.TheTrafodionSchema;
            AddProceduresPage(theTrafodionSchema);
        }
        
        private void AddToTabControl(MaterializedViewGroupsFolder aMaterializedViewGroupsFolder)
        {
            TrafodionSchema theTrafodionSchema = aMaterializedViewGroupsFolder.TheTrafodionSchema;
            AddMaterializedViewGroupsPage(theTrafodionSchema);
        }

        private void AddMaterializedViewGroupsPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new MaterializedViewGroupsTabPage(this, aTrafodionSchema, Properties.Resources.MVGroups));
        }

        private void AddProceduresPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new ProceduresTabPage(this, aTrafodionSchema, Properties.Resources.Procedures));
        }

        private void AddLibrariesPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new LibrariesTabPage(this, aTrafodionSchema, Properties.Resources.Libraries));
        }

        private void AddTableMappingFunctionsPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new TableMappingFunctionsTabPage(this, aTrafodionSchema, Properties.Resources.TableMappingFunctions));
        }    

        private void AddToTabControl(SynonymsFolder aSynonymsFolder)
        {
            TrafodionSchema theTrafodionSchema = aSynonymsFolder.TheTrafodionSchema;
            AddSynonymsPage(theTrafodionSchema);
        }

        private void AddSynonymsPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new SynonymsTabPage(this, aTrafodionSchema, Properties.Resources.Synonyms));
        }
        private void AddSequencesPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new SequencesTabPage(this, aTrafodionSchema, Properties.Resources.Sequences));
        }

        private void AddViewsPage(TrafodionSchema aTrafodionSchema)
        {
            AddToTabControl(new ViewsTabPage(this, aTrafodionSchema, Properties.Resources.Views));
        }

        private void AddTableIndexesPage(TrafodionTable aTrafodionTable)
        {
            AddToTabControl(new TrafodionIndexesTabPage(this, aTrafodionTable));
            //AddToTabControl(new TrafodionIndexesSpaceUsagesTabPage(this, aTrafodionTable));
        }

        private void AddMVIndexesPage(TrafodionMaterializedView aTrafodionMaterializedView)
        {
            AddToTabControl(new TrafodionIndexesTabPage(this, aTrafodionMaterializedView));
            //AddToTabControl(new TrafodionIndexesSpaceUsagesTabPage(this, aTrafodionMaterializedView));
        }

        private void AddTableTriggersPage(TrafodionTable aTrafodionTable)
        {
            AddToTabControl(new TriggersTabPage(this, aTrafodionTable, Properties.Resources.Triggers));
        }


        private object theMostRecentWorkObject = null;
        private NavigationTreeNameFilter theNameFilter = new NavigationTreeNameFilter();

        public NavigationTreeNameFilter TheNameFilter
        {
            get { return theNameFilter; }
            set { theNameFilter = value; }
        }

        private DatabaseTreeView theDatabaseTreeView = null;

        public DatabaseTreeView TheDatabaseTreeView
        {
            get { return theDatabaseTreeView; }
        }

        private DatabaseNavigator theDatabaseAreaLeftControl;

        public DatabaseNavigator TheDatabaseAreaLeftControl
        {
            get { return theDatabaseAreaLeftControl; }
            set
            {
                theDatabaseAreaLeftControl = value;

                theDatabaseTreeView = (theDatabaseAreaLeftControl == null) ? null : theDatabaseAreaLeftControl.DatabaseTreeView;

                {
                    bool treeAvailable = (theDatabaseTreeView != null);

                    if (treeAvailable)
                    {
                        if (theTrafodionTreeControlSelectedHandler == null)
                        {
                            theTrafodionTreeControlSelectedHandler = new NavigationTreeView.SelectedHandler(TrafodionTreeControlSelected);
                        }
                        TheDatabaseTreeView.Selected += theTrafodionTreeControlSelectedHandler;

                        if (theTrafodionTreeControlExceptionOccurredHandler == null)
                        {
                            theTrafodionTreeControlExceptionOccurredHandler = new NavigationTreeView.ExceptionOccurredHandler(TrafodionTreeControlExceptionOccurred);
                        }
                        TheDatabaseTreeView.ExceptionOccurred += theTrafodionTreeControlExceptionOccurredHandler;

                        if (theFilterChangedHandler == null)
                        {
                            theFilterChangedHandler = new NavigationTreeNameFilter.ChangedHandler(FilterChanged);
                        }

                        NavigationTreeNameFilter.Changed += theFilterChangedHandler;
                    }

                }

            }
        }

        private TabControl theTabControl = null;

        public TabControl TheTabControl
        {
            get
            {
                return theTabControl;
            }
            set
            {
                theTabControlPanel.Controls.Clear();

                theTabControl = value;

                theTabControl.Dock = DockStyle.Fill;
                theTabControlPanel.Controls.Add(theTabControl);

            }
        }


        private NavigationTreeView.SelectedHandler theTrafodionTreeControlSelectedHandler = null;
        private NavigationTreeView.ExceptionOccurredHandler theTrafodionTreeControlExceptionOccurredHandler = null;
        private NavigationTreeNameFilter.ChangedHandler theFilterChangedHandler = null;

        private void button1_Click(object sender, EventArgs e)
        {
            TrafodionForm theForm = new TrafodionForm();
            DatabaseObjectsControl theClonedDatabaseAreaObjectsControl = (DatabaseObjectsControl)Clone();
            theClonedDatabaseAreaObjectsControl.Dock = DockStyle.Fill;
            theForm.Controls.Add(theClonedDatabaseAreaObjectsControl);
            theForm.ClientSize = Size;
            theForm.Text = theClonedDatabaseAreaObjectsControl.WindowTitle;
            theForm.Show();
        }

        void ll_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            if (e.Link.Tag != null)
            {
                TheDatabaseTreeView.SelectTrafodionObject((TrafodionObject)e.Link.Tag);
            }
        }
    }

}
