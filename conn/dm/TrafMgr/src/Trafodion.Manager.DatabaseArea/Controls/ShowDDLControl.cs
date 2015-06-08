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
using System.ComponentModel;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls 
{
    /// <summary>
    /// This user control allows a user to select objects for which DDL information needs to be generated
    /// The user control displays a the active systems from the database navigation tree and the user
    /// can select objects from any one schema
    /// The user control is added to a ManagedWindow at the time this control is instantiated.
    /// </summary>
    public partial class ShowDDLControl : UserControl
    {
        private ShowDDLTreeView _databaseActiveSystemsTreeView = null;
        private TrafodionObject _initalTrafodionObject = null;
        //private string _currentSchemaName = null;
        private TrafodionCatalog _currentTrafodionCatalog = null;
        private List<TrafodionObject> _selectedTrafodionObjects = new List<TrafodionObject>();
        private SaveFileDialog _saveFileDialog = new SaveFileDialog();
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        private TreeNode _mainDatabaseTreeNode = null;
        private const string _cancelString = "&Cancel";
        private const string _doneString = "&Done";

        /// <summary>
        /// Constructs this user control
        /// </summary>
        /// <param name="sourceNode">The source node in the database tree, from which this tool was launched</param>
        public ShowDDLControl(Object sourceNode)
        {
            InitializeComponent();
            InitializeBackgoundWorker();

            _mainDatabaseTreeNode = (TreeNode)sourceNode;
            if (sourceNode is DatabaseTreeFolder)
            {
                _initalTrafodionObject = ((DatabaseTreeFolder)sourceNode).TrafodionObject;
            }
            else if (sourceNode is DatabaseTreeNode)
            {
                _initalTrafodionObject = ((DatabaseTreeNode)sourceNode).TrafodionObject;
            }

            _databaseActiveSystemsTreeView = new ShowDDLTreeView(_initalTrafodionObject.ConnectionDefinition);
            _databaseActiveSystemsTreeView.Dock = DockStyle.Fill;
            _databaseActiveSystemsTreeView.CheckAndAddListener();

            treePanel.Controls.Add(_databaseActiveSystemsTreeView);
            _databaseActiveSystemsTreeView.BeforeCheck += new TreeViewCancelEventHandler(ActiveSystemsTreeView_BeforeCheck);
            _databaseActiveSystemsTreeView.AfterCheck += new TreeViewEventHandler(ActiveSystemsTreeView_AfterCheck);

            _databaseActiveSystemsTreeView.SetAndPopulateRootFolders();
            _saveFileDialog.InitialDirectory = Trafodion.Manager.Framework.Utilities.FileDialogLocation();

            _saveFileDialog.AddExtension = true;
            _saveFileDialog.DefaultExt = "sql";
            _saveFileDialog.Filter =  "Text File (*.sql;*.txt)|*.sql;*.txt|All Files (*.*)|*.*";
        }

        /// <summary>
        /// Peform load time initializations
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ShowDDLControl_Load(object sender, EventArgs e)
        {
            if (_databaseActiveSystemsTreeView.SelectTrafodionObject(_initalTrafodionObject))
            {
                _databaseActiveSystemsTreeView.SelectedNode = _databaseActiveSystemsTreeView.FindByFullPath(_mainDatabaseTreeNode.FullPath);
                    _databaseActiveSystemsTreeView.SelectedNode.Checked = true;
                if (_initalTrafodionObject is TrafodionCatalog)
                {
                    _currentTrafodionCatalog = (TrafodionCatalog)_initalTrafodionObject;
                }
                if (_initalTrafodionObject is TrafodionSchema)
                {
                    _currentTrafodionCatalog = ((TrafodionSchema)_initalTrafodionObject).TheTrafodionCatalog;
                }
                if (_initalTrafodionObject is TrafodionSchemaObject)
                {
                    _currentTrafodionCatalog = ((TrafodionSchemaObject)_initalTrafodionObject).TheTrafodionCatalog;
                }
            }
            doneButton.Text = _doneString;
            HideStatusBar();
        }

        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted += 
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// Clear all current selections in the tree
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void clearButton_Click(object sender, EventArgs e)
        {
            _databaseActiveSystemsTreeView.ResetCheckBoxes();
            _selectedTrafodionObjects.Clear();
            loadButton.Enabled = false;
            appendButton.Enabled = false;
        }

        /// <summary>
        /// If the button is in "Cancel" mode ask the background worker to stop working
        /// If the button is in "Done" mode, ask the parent form to close
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void doneButton_Click(object sender, EventArgs e)
        {
            if (doneButton.Text.Equals(_doneString))
            {
                ((Form)Parent).Close();
            }
            else
            {
                this._backgroundWorker.CancelAsync();
            }
        }

        /// <summary>
        /// Online help for this dialog
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void helpButton_Click(object sender, EventArgs e)
        {

        }
        
        /// <summary>
        /// Saves the DDL text to a file
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void saveButton_Click(object sender, EventArgs e)
        {
            if (_saveFileDialog.ShowDialog() == DialogResult.OK)
            {
                StreamWriter sw = null;
                FileStream fs = null;
                try
                {
                    fs = new FileStream(_saveFileDialog.FileName, FileMode.Create);
                    sw = new StreamWriter(fs);

                    string[] separators = { Environment.NewLine, "\n" };
                    string[] textArray = ddlOutputTextBox.Text.Split(separators, StringSplitOptions.None);
                    foreach (string lineOfText in textArray)
                    {
                        sw.WriteLine(lineOfText);
                    }

                    // Persist the last file-location: 
                    Trafodion.Manager.Framework.Utilities.FileDialogLocation(_saveFileDialog.FileName);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.FileSaveFailure + " : " + ex.Message, 
                        Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
                finally
                {
                    if (sw != null)
                    {
                        sw.Close();
                    }
                    if (fs != null)
                    {
                        fs.Close();
                    }
                }
            }
        }

        /// <summary>
        /// Loads and displays the DDL for the selected objects and replaces the contents in the output text box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void loadButton_Click(object sender, EventArgs e)
        {
            if (ddlOutputTextBox.Text.Length > 0)
            {
                if (MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ShowDDLOverwriteMessage, Properties.Resources.Warning,
                        MessageBoxButtons.YesNo, MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    ddlOutputTextBox.Clear();
                }
            }
            try
            {
                FillSelectedObjectsList();
                GetReadyToProcessDDL();
                Application.DoEvents();
                GetDDLForSelectedObjects();
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// Loads and displays the DDL of selected objects and appends the text to the output box
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void appendButton_Click(object sender, EventArgs e)
        {
            _selectedTrafodionObjects.Clear();
            try
            {
                FillSelectedObjectsList();
                GetReadyToProcessDDL();
                Application.DoEvents();
                GetDDLForSelectedObjects();
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.Error,
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        
        /// <summary>
        /// Reset state before beginning to process the DDL for selected objects
        /// </summary>
        private void GetReadyToProcessDDL()
        {
            loadButton.Enabled = false;
            appendButton.Enabled = false;
            clearButton.Enabled = false;
            ResetProgressBar();
        }

        /// <summary>
        /// Reset the status bar for reporting progress
        /// </summary>
        public void ResetProgressBar()
        {
            toolStripProgressBar.Maximum = _selectedTrafodionObjects.Count;
            toolStripProgressBar.Minimum = 1;
            toolStripProgressBar.Step = 1;
            toolStripProgressBar.Value = 1;
        }

        /// <summary>
        /// Hide the status and progress bar when they are not displaying information
        /// </summary>
        private void HideStatusBar()
        {
            toolStripStatusLabel.Text = "";
            toolStripStatusLabel.Visible = false;

            ResetProgressBar();
            toolStripProgressBar.Visible = false;
        }

        
        /// <summary>
        /// Do validations to see if the node's check box is allowed to be checked.
        /// If it not allowed, cancel the event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ActiveSystemsTreeView_BeforeCheck(object sender, TreeViewCancelEventArgs e)
        {
            if (e.Action != TreeViewAction.Unknown)
            {
                if (!e.Node.Checked)
                {
                    if (e.Node is SchemasFolder || e.Node is CatalogsFolder || e.Node is CatalogFolder)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.SchemaLevelShowDDLMessage, Properties.Resources.Exception, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        e.Cancel = true;
                        return;
                    }
                    if (e.Node is DatabaseTreeNode || e.Node is DatabaseTreeFolder)
                    {
                        TrafodionCatalog newTrafodionCatalog = null;
                        TrafodionObject selectedTrafodionObject = null;
                        if (e.Node is DatabaseTreeNode)
                        {
                            selectedTrafodionObject = ((DatabaseTreeNode)e.Node).TrafodionObject;
                        }
                        if (e.Node is DatabaseTreeFolder)
                        {
                            selectedTrafodionObject = ((DatabaseTreeFolder)e.Node).TrafodionObject;
                        }
                        if (selectedTrafodionObject is TrafodionCatalog)
                        {
                            newTrafodionCatalog = (TrafodionCatalog)selectedTrafodionObject;
                        }
                        if (selectedTrafodionObject is TrafodionSchema)
                        {
                            newTrafodionCatalog = ((TrafodionSchema)selectedTrafodionObject).TheTrafodionCatalog;
                        }
                        if (selectedTrafodionObject is TrafodionSchemaObject)
                        {
                            newTrafodionCatalog = ((TrafodionSchemaObject)selectedTrafodionObject).TheTrafodionCatalog;
                        }

                        //If selected node is not a schema object or one of its children, then it is an error
                        //User has to select a schema or a child object only
                        if (newTrafodionCatalog == null)
                        {
                            MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.SchemaLevelShowDDLMessage, Properties.Resources.Exception, MessageBoxButtons.OK, MessageBoxIcon.Error);
                            e.Cancel = true;
                            return;
                        }

                        //You can only select objects from the same schema. If user attempted to select objects
                        //from different schemas, throw an error
                        if(_currentTrafodionCatalog != null)
                        {
                            if (!_currentTrafodionCatalog.InternalName.Equals(newTrafodionCatalog.InternalName))
                            {
                                MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.MultiCatalogSelectedInShowDDLMessage, Properties.Resources.Exception, MessageBoxButtons.OK, MessageBoxIcon.Error);
                                e.Cancel = true;
                                return;
                            }
                            else
                            {
                                if (!_currentTrafodionCatalog.TrafodionSystem.InternalName.Equals(newTrafodionCatalog.TrafodionSystem.InternalName))
                                {
                                    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.MultiSystemSelectedInShowDDLMessage, Properties.Resources.Exception, MessageBoxButtons.OK, MessageBoxIcon.Error);
                                    e.Cancel = true;
                                    return;
                                }
                            }
                        }
                    }
                    else
                    {
                        //If selected node is not a schema object or one of its children, then it is an error
                        //User has to select a schema or a child object only
                        MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.SchemaLevelShowDDLMessage, Properties.Resources.Exception, MessageBoxButtons.OK, MessageBoxIcon.Error);
                        e.Cancel = true;
                        return;
                    }
                }
            }
        }

        /// <summary>
        /// Handle the clicks on the checkboxes in the tree
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ActiveSystemsTreeView_AfterCheck(object sender, TreeViewEventArgs e)
        {
            // The code only executes if the user caused the checked state to change.
            if (e.Action != TreeViewAction.Unknown)
            {
                //Store the schema name of the selected node to enforce the rule that objects from
                //only one schema can be selected at a time
                if (e.Node.Checked)
                {
                    if (_currentTrafodionCatalog == null)
                    {
                        if (e.Node is DatabaseTreeNode || e.Node is DatabaseTreeFolder)
                        {
                            TrafodionObject selectedTrafodionObject = null;
                            if (e.Node is DatabaseTreeNode)
                            {
                                selectedTrafodionObject = ((DatabaseTreeNode)e.Node).TrafodionObject;
                            }
                            if (e.Node is DatabaseTreeFolder)
                            {
                                selectedTrafodionObject = ((DatabaseTreeFolder)e.Node).TrafodionObject;
                            }
                            if (selectedTrafodionObject is TrafodionCatalog)
                            {
                                _currentTrafodionCatalog = (TrafodionCatalog)selectedTrafodionObject;
                            }
                            if (selectedTrafodionObject is TrafodionSchema)
                            {
                                _currentTrafodionCatalog = ((TrafodionSchema)selectedTrafodionObject).TheTrafodionCatalog;
                            }
                            if (selectedTrafodionObject is TrafodionSchemaObject)
                            {
                                _currentTrafodionCatalog = ((TrafodionSchemaObject)selectedTrafodionObject).TheTrafodionCatalog;
                            }
                        }
                    }
                }
                else
                {
                    if (e.Node.IsSelected)
                    {
                        if (e.Node.TreeView != null && e.Node == e.Node.TreeView.SelectedNode)
                        {
                            e.Node.TreeView.SelectedNode = null;
                            _currentTrafodionCatalog = null;
                        }
                    }
                }
            }
            //If tree has any nodes selected, hide the status bar and enable the load/append buttons
            if (_databaseActiveSystemsTreeView.HasNodesChecked)
            {
                toolStripStatusLabel.Visible = false;
                loadButton.Enabled = true;
                appendButton.Enabled = true;
            }
            else
            {
                //If no object is selected in the tree, prompt the user to select one
                toolStripStatusLabel.Visible = true;
                toolStripStatusLabel.Text = Properties.Resources.ShowDDLToolPleaseSelectMessage;

                //Disable the load/append buttons if no items selected
                _currentTrafodionCatalog = null;
                loadButton.Enabled = false;
                appendButton.Enabled = false;
            }
        }

        /// <summary>
        /// Starting with root nodes, recursively traverses the tree, finds all the checked nodes and adds the
        /// sql object to a list
        /// </summary>
        private void FillSelectedObjectsList()
        {
            foreach (TreeNode childNode in _databaseActiveSystemsTreeView.Nodes)
            {
                FillSelectedObjectsList(childNode);
            }
        }

        /// <summary>
        /// Recursively traverses the tree from the given node, finds all the checked nodes and adds the
        /// sql object to a list
        /// </summary>
        private void FillSelectedObjectsList(TreeNode node)
        {
            //If a node is found with its checkbox selected add the sql objects 
            //associated with node based on the type of node, to the selected list
            if (node.Checked)
            {
                if (node is SchemaFolder)
                {
                    SchemaFolder schemaFolder = (SchemaFolder)node;
                    _selectedTrafodionObjects.Add(schemaFolder.TheTrafodionSchema); //schema has ddl, so add it to selected list

                    //If schema folder state is fully checked and not partial, add all schema objects to the selected list
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionTables);
                        AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionMaterializedViews);
                        AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionMaterializedViewGroups);
                        AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionViews);
                        AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionProcedures);
                        AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionLibraries);
                        if (schemaFolder.TheTrafodionSchema.Version >= 2500)
                        {
                            AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionUDFunctions);
                            AddSqlObjectsToSelectedList(schemaFolder.TheTrafodionSchema.TrafodionFunctionActions);
                        }

                        //If all objects included in schema, no need to check the rest of the sub tree, exit this routine.
                        return;
                    }
                }
                if (node is SchemaIndexesFolder)
                {
                    if (!node.Parent.Checked)
                    {
                        //If SchemaIndexesFolder has a checked state, then include all indexes contained in this folder
                        if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                        {
                            AddSqlObjectsToSelectedList(((SchemaIndexesFolder)node).TheTrafodionSchema.TrafodionIndexes);

                            //If all objects included in indexes folder, no need to check the rest of the sub tree, exit this routine.
                            return;
                        }
                    }
                }
                if (node is TablesFolder)
                {
                    //If TablesFolder has a checked state, then include all tables contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((TablesFolder)node).TheTrafodionSchema.TrafodionTables);

                        //If all objects included in tables folder, no need to check the rest of the sub tree, exit this routine.
                        return;
                    }
                }
                if (node is TableFolder)
                {
                    
                    //If parent folder already included this table, do add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                    {
                        _selectedTrafodionObjects.Add(((TableFolder)node).TrafodionTable);

                        //Table DDL will return all its child DDL. So no need to traverse the child nodes
                        return;
                    }
                }
                if (node is TableIndexesFolder)
                {
                    if (!node.Parent.Checked)
                    {
                        //If TableIndexesFolder has a checked state, then include all indexes contained in this folder
                        if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                        {
                            AddSqlObjectsToSelectedList(((TableIndexesFolder)node).TrafodionTable.TrafodionIndexes);

                            //If all objects included in indexes folder, no need to check the rest of the sub tree, exit this routine.
                            return;
                        }
                    }
                }
                if ((node is IndexLeaf) && !(node is MaterializedViewIndexLeaf)) //MaterializedViewIndexLeaf will be added at the below branch statement.
                {
                    //If table already added to selected list, then the index is already included
                    IndexedSchemaObject table = ((IndexLeaf)node).TrafodionIndex.IndexedSchemaObject;
                    if(!_selectedTrafodionObjects.Contains(table))
                        _selectedTrafodionObjects.Add(((IndexLeaf)node).TrafodionIndex);
                }
                if (node is TableTriggersFolder)
                {
                    if (!node.Parent.Checked)
                    {
                        //If TableTriggersFolder has a checked state, then include all triggers contained in this folder
                        if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                        {
                            AddSqlObjectsToSelectedList(((TableTriggersFolder)node).TrafodionTable.TrafodionTriggers);

                            //If all objects included in triggers folder, no need to check the rest of the sub tree, exit this routine.
                            return;
                        }
                    }
                }
                if (node is TableTriggerLeaf)
                {
                    //If table already added to selected list, then the index is already included
                    TrafodionTable table = ((TableTriggerLeaf)node).TrafodionTrigger.TrafodionTable;
                    if (!_selectedTrafodionObjects.Contains(table))
                        _selectedTrafodionObjects.Add(((TableTriggerLeaf)node).TrafodionTrigger);
                }

                if (node is MaterializedViewsFolder)
                {
                    //If MaterializedViewsFolder has a checked state, then include all MVs contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((MaterializedViewsFolder)node).TheTrafodionSchema.TrafodionMaterializedViews);

                        //If all objects included in mv folder, no need to check the rest of the sub tree, exit this routine.
                        return;
                    }
                }
                if (node is MaterializedViewFolder)
                {
                    //If parent folder already included this table, do add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                    {
                        _selectedTrafodionObjects.Add(((MaterializedViewFolder)node).TrafodionMaterializedView);

                        //If Mv is selected all its child ddl is already fetched. So no need to check the rest of the sub tree, exit this routine.
                        return;
                    }
                }
                if (node is MaterializedViewIndexesFolder)
                {
                    if (!node.Parent.Checked)
                    {
                        //If MaterializedViewIndexesFolder has a checked state, then include all indexes contained in this folder
                        if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                        {
                            AddSqlObjectsToSelectedList(((MaterializedViewIndexesFolder)node).TrafodionMaterializedView.TrafodionIndexes);

                            //If all objects included in MV indexes folder, no need to check the rest of the sub tree, exit this routine.
                            return;
                        }
                    }
                }
                if (node is MaterializedViewIndexLeaf)
                {
                    //If MV already added to selected list, then the index is already included
                    IndexedSchemaObject mv = ((MaterializedViewIndexLeaf)node).TrafodionIndex.IndexedSchemaObject;
                    if (!_selectedTrafodionObjects.Contains(mv))
                        _selectedTrafodionObjects.Add(((MaterializedViewIndexLeaf)node).TrafodionIndex); 
                } 
                
                if (node is MaterializedViewGroupsFolder)
                {
                    //If MaterializedViewGroupsFolder has a checked state, then include all MVGroups contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((MaterializedViewGroupsFolder)node).TheTrafodionSchema.TrafodionMaterializedViewGroups);

                        //If all objects included in MV groups folder, no need to check the rest of the sub tree, exit this routine.
                        return;

                    }
                }
                if (node is MaterializedViewGroupLeaf)
                {
                    //If parent folder already included this mvgroup, dont add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                        _selectedTrafodionObjects.Add(((MaterializedViewGroupLeaf)node).TrafodionMaterializedViewGroup);
                } 
                if (node is ViewsFolder)
                {
                    //If ViewsFolder has a checked state, then include all views contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((ViewsFolder)node).TheTrafodionSchema.TrafodionViews);

                        //If all objects included in views folder, no need to check the rest of the sub tree, exit this routine.
                        return;
                    }
                }
                if (node is ViewLeaf)
                {
                    //If parent folder already included this table, dont add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                        _selectedTrafodionObjects.Add(((ViewLeaf)node).TrafodionView);
                }
                if (node is SynonymsFolder)
                {
                    //If ViewsFolder has a checked state, then include all views contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        //Synonyms needs special handling. 
                        //You need to check to see if the referenced object already selected. If so that object's ddl 
                        //will include the synonym ddl as well
                        List<TrafodionSynonym> synonyms = ((SynonymsFolder)node).TheTrafodionSchema.TrafodionSynonyms;
                        foreach (TrafodionSynonym synonym in synonyms)
                        {
                            if (_selectedTrafodionObjects.Contains(synonym.ReferencedObject))
                                continue;

                            _selectedTrafodionObjects.Add(synonym);
                        }
                        return;
                    }
                }
                if (node is SynonymLeaf)
                {
                    //Synonyms needs special handling. 
                    //You need to check to see if the referenced object already selected. If so that object's ddl 
                    //will include the synonym ddl as well
                    SynonymLeaf synonym = (SynonymLeaf)node;
                    TreeNode parentObjectNode = _databaseActiveSystemsTreeView.FindNodeForTrafodionObject(synonym.TrafodionSynonym.ReferencedObject);
                    if (parentObjectNode == null || !parentObjectNode.Checked)
                    {
                        _selectedTrafodionObjects.Add(((SynonymLeaf)node).TrafodionSynonym);
                    }
                }
                if (node is ProceduresFolder)
                {
                    //If Procedures folder has a checked state, then include all procedures contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((ProceduresFolder)node).TheTrafodionSchema.TrafodionProcedures);

                        //If all objects included in views folder, no need to check the rest of the sub tree, exit this routine.
                        return;
                    }
                }
                if (node is ProcedureLeaf)
                {
                    //If parent folder already included this table, dont add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                        _selectedTrafodionObjects.Add(((ProcedureLeaf)node).TrafodionProcedure);
                }
                if (node is FunctionsFolder)
                {
                    //If Procedures folder has a checked state, then include all procedures contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((FunctionsFolder)node).TheTrafodionSchema.TrafodionUDFunctions);

                        return;
                    }
                }
                if (node is FunctionLeaf)
                {
                    //If parent folder already included this table, dont add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                        _selectedTrafodionObjects.Add(((FunctionLeaf)node).TrafodionUDF);
                }
                if (node is FunctionActionsFolder)
                {
                    //If Procedures folder has a checked state, then include all procedures contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((FunctionActionsFolder)node).TheTrafodionSchema.TrafodionFunctionActions);

                        return;
                    }
                }
                if (node is FunctionActionLeaf)
                {
                    //If parent folder already included this table, dont add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                        _selectedTrafodionObjects.Add(((FunctionActionLeaf)node).TrafodionFunctionAction);
                }
                if (node is LibrariesFolder)
                {
                    //If Libraries folder has a checked state, then include all libraries contained in this folder
                    if ((ShowDDLTreeView.NodeState)node.Tag != ShowDDLTreeView.NodeState.Partial)
                    {
                        AddSqlObjectsToSelectedList(((LibrariesFolder)node).TheTrafodionSchema.TrafodionLibraries);

                        return;
                    }
                }
                if (node is LibraryLeaf)
                {
                    //If parent folder already included this Library, dont add it again to the list
                    if (!((ShowDDLTreeView.NodeState)node.Parent.Tag == ShowDDLTreeView.NodeState.Checked))
                        _selectedTrafodionObjects.Add(((LibraryLeaf)node).TrafodionLibrary);
                }
            }

            foreach (TreeNode childNode in node.Nodes)
            {
                if (childNode.Text == null || childNode.Text.Equals(""))
                    continue;

                FillSelectedObjectsList(childNode);
            }
        }

        /// <summary>
        /// Add all objects from list to selected objects list
        /// </summary>
        /// <param name="sqlMxObjects">list of objects that need to be added to the selected list</param>
        private void AddSqlObjectsToSelectedList<T>(List<T> sqlMxObjects) where T : TrafodionObject
        {
            foreach (TrafodionObject sqlMxObject in sqlMxObjects)
            {
                if(sqlMxObject is TrafodionSchemaObject && ((TrafodionSchemaObject)sqlMxObject).IsMetadataObject)
                        continue;

                _selectedTrafodionObjects.Add(sqlMxObject);
            }
        }

        /// <summary>
        /// Calls the ShowDDLWorker to work on loading the DDL for the selected objects
        /// </summary>
        private void GetDDLForSelectedObjects()
        {
            doneButton.Text = _cancelString; //Temporarily change the text on the done button to cancel
            saveButton.Enabled = false; //Save button is disabled until work completes
            clearTextButton.Enabled = false; //Clear button is disabled until work completes
            ddlOutputTextBox.AppendText(Environment.NewLine);
            ddlOutputTextBox.AppendText("----Trafodion Database Manager ShowDDL started at " + Trafodion.Manager.Framework.Utilities.CurrentFormattedDateTime);
            ddlOutputTextBox.AppendText(Environment.NewLine);

            Application.DoEvents(); //consume other application events before invoking the background worker

            // Start the asynchronous operation.
            _backgroundWorker.RunWorkerAsync(_selectedTrafodionObjects);
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            FetchDDL((List<TrafodionObject>)e.Argument, worker, e);
        }


        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), e.Error.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                ddlOutputTextBox.AppendText(Environment.NewLine);
                ddlOutputTextBox.AppendText("----Operation Aborted");
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                ddlOutputTextBox.AppendText(Environment.NewLine);
                ddlOutputTextBox.AppendText("----Operation Completed");
            }
            ddlOutputTextBox.AppendText(" at " + Trafodion.Manager.Framework.Utilities.CurrentFormattedDateTime);
            ddlOutputTextBox.AppendText(Environment.NewLine);
            HideStatusBar();

            _selectedTrafodionObjects.Clear();

            loadButton.Enabled = true;
            appendButton.Enabled = true;
            clearButton.Enabled = true;

            if (ddlOutputTextBox.Text.Length > 0)
                saveButton.Enabled = true; //Enable the save button

            clearTextButton.Enabled = true;
            doneButton.Text = _doneString;
        }

        /// <summary>
        /// This event handler updates the progress bar and appends the DDL text to the output textbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
 
        private void BackgroundWorker_ProgressChanged(object sender,
            ProgressChangedEventArgs e)
        {
            toolStripProgressBar.Visible = true;
            toolStripStatusLabel.Visible = true;
            toolStripProgressBar.PerformStep();

            string ddlText = e.UserState as string;
            ddlOutputTextBox.AppendText(ddlText);

            string text = string.Format(Properties.Resources.ShowDDLToolRequestSummaryMessage1,
                                            toolStripProgressBar.Value, toolStripProgressBar.Maximum);
            toolStripStatusLabel.Text = text;
            statusStrip.Update();
        }

        /// <summary>
        /// This method is invoked by the worker thread to fetch DDL for the selected objects
        /// The fetched DDL is reported back in a progress event
        /// </summary>
        /// <param name="sqlMxObjectList"></param>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        void FetchDDL(List<TrafodionObject> sqlMxObjectList, BackgroundWorker worker, DoWorkEventArgs e)
        {
            for(int i=0; i< sqlMxObjectList.Count;i++)
            {
                // Abort the operation if the user has canceled.
                // Note that a call to CancelAsync may have set 
                // CancellationPending to true just after the
                // last invocation of this method exits, so this 
                // code will not have the opportunity to set the 
                // DoWorkEventArgs.Cancel flag to true. This means
                // that RunWorkerCompletedEventArgs.Cancelled will
                // not be set to true in your RunWorkerCompleted
                // event handler. This is a race condition.

                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }
                else
                {
                    string text = Environment.NewLine + "--DDL for " + sqlMxObjectList[i].VisibleAnsiName + Environment.NewLine; 

                    try
                    {
                        text += sqlMxObjectList[i].DDLText;
                    }
                    catch (Exception ex)
                    {
                        text += ex.Message;
                    }
                    worker.ReportProgress(0, text);
                }
            }
        }

        private void clearTextButton_Click(object sender, EventArgs e)
        {
            ddlOutputTextBox.Clear();
            saveButton.Enabled = false;
        }
    }
}
