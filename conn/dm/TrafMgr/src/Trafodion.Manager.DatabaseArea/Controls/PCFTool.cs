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
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// The procedure code file user control
    /// </summary>
    public partial class PCFTool : UserControl 
    {
        #region Private Member Variables

        enum PromptResult { OVERWRITE, DO_NOT_OVERWRITE, PROMPT };
        enum PCFOperation { Refresh, GetClasses, GetMethods, Upload, Download, Rename, Delete };
        const string TrafodionManager_MESSAGE_SEPARATOR = "TrafodionManager_MESSAGE_SEPARATOR";
        TrafodionIGrid _theMethodsGrid;
        ConnectionDefinition _theConnectionDefinition;
        Hashtable _operationParamaters = new Hashtable();
        System.ComponentModel.BackgroundWorker _backgroundWorker;
        bool isOperationCancelled = false;

        // default is no column has underline link.
        int HyperlinkColIndex = -1;

        TreeViewEventHandler _afterSelectHandler = null;
        TreeViewCancelEventHandler _beforeExpandHandler = null;
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;
        EventHandler _methodsGridSelectionChangeHandler = null;
        InvalidateEventHandler _methodsGridInvalidatedHandler = null;

        System.Data.Odbc.OdbcCommand _pcfCommandObject = null;
        PCFModel _pcfModel = null;

        //Max allowed file size in bytes
        long FILE_UPLOAD_SIZE = -1;
        bool _parentClosing = false;

        #endregion Private Member Variables

        #region Public Properties

        /// <summary>
        /// The class tree node that is currently selected in the code file tree
        /// </summary>
        public ClassTreeNode SelectedClassTreeNode
        {
            get
            {
                TreeNode node = _codeFilesTree.SelectedNode;
                if (node != null && node is ClassTreeNode)
                {
                    return ((ClassTreeNode)node);
                }
                return null;
            }
        }

        /// <summary>
        /// The method that is currently selected in the methods grid
        /// </summary>
        public JavaMethod SelectedMethod
        {
            get
            {
                JavaMethod javaMethod = null;
                if (_theMethodsGrid.SelectedRows.Count > 0)
                {
                    javaMethod = new JavaMethod(_theMethodsGrid.SelectedRows[0].Cells[0].Value as string,
                                                                _theMethodsGrid.SelectedRows[0].Cells[1].Value as string
                                                           );
                }
                return javaMethod;
            }
        }

        #endregion Public Properties

        #region Constructors, Initializers, Destructors

        /// <summary>
        /// Constructs the procedure code file control
        /// </summary>
        /// <param name="aConnectionDefinition">The connection used to populate the code file tool</param>
        public PCFTool(ConnectionDefinition aConnectionDefinition)
        {
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            setupComponents();
        }

        /// <summary>
        /// Constructor called by the CreateSPJ tool.
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="isCreateMode">Set to true when invoked from CreateSPJ tool</param>
        public PCFTool(ConnectionDefinition aConnectionDefinition, bool isCreateMode)
        {
            InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
            setupComponents();

            //OK button is visible only when PCFTool is invoked from CreateSPJ tool
            if (!_parentClosing)
            {
                _okButton.Visible = isCreateMode;
            }
            _okButton.Enabled = false;
        }

        /// <summary>
        /// Initialize the UI controls and set up the handlers
        /// </summary>
        private void setupComponents()
        {
            //Initialize the PCF model
            _pcfModel = new PCFModel(_theConnectionDefinition);
            _pcfCommandObject = _pcfModel.PCFCommandObject;

            if (_theConnectionDefinition.MaxUserTableSize > 0)
            {
                FILE_UPLOAD_SIZE = _theConnectionDefinition.MaxUserTableSize * 1024 * 1024;//user quota in MB converted to bytes.
            }

            InitializeBackgoundWorker();

            //Since iGrid's constructor takes a license string, construct and initialize
            //outside of the .net designer
            InitializeMethodsGrid();

            //Listen to code file tree selection event
            _codeFilesTree.ConnectionDefinition = _theConnectionDefinition;
            _afterSelectHandler = new TreeViewEventHandler(CodeFilesTree_AfterSelect);
            _beforeExpandHandler = new TreeViewCancelEventHandler(CodeFilesTree_BeforeExpand);
            _codeFilesTree.BeforeExpand += _beforeExpandHandler;
            _codeFilesTree.AfterSelect += _afterSelectHandler;
        }

        void ParentForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            _parentClosing = true;
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            CancelAsync();

            _codeFilesTree.BeforeExpand -= _beforeExpandHandler;
            _codeFilesTree.AfterSelect -= _afterSelectHandler;
            _theMethodsGrid.CellClick -= _cellClickHandler;
            _theMethodsGrid.SelectionChanged -= _methodsGridSelectionChangeHandler;
            _theMethodsGrid.Invalidated -= _methodsGridInvalidatedHandler;

            _hyperLinkCellManager.Detach();
        }

        /// <summary>
        /// Initialize the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            //Before do that, first prepare for the case which user has canceled the request.
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
        /// Since iGrid's constructor takes a license string, construct and initialize
        /// outside of the .net designer
        /// </summary>
        void InitializeMethodsGrid()
        {
            _theMethodsGrid = new Trafodion.Manager.Framework.Controls.TrafodionIGrid("aQB2AFkAZAB3AGkAVwBlAG4ALgBIAHMANQBsAGUAdAB0AEkATwBaAHQAdAB1AEcAUgB2AG4AZAB1AG0ATQB5AGgAcgBlAHUATgBlAGsASgAtAHMAMAA2AHgAYwBwAHQAOABhAHYAbgAwAHkAZABhADkAOQBtADAAIABiAHIAUAB5ADAAdQBrAEMANAAxAG8AcQBhAHcAMgByAHoAMABhAA==");
            this._theMethodsGrid.BackColorEvenRows = System.Drawing.Color.WhiteSmoke;
            this._theMethodsGrid.BorderStyle = TenTec.Windows.iGridLib.iGBorderStyle.Standard;
            this._theMethodsGrid.DefaultCol.CellStyle = new TenTec.Windows.iGridLib.iGCellStyle(true);
            this._theMethodsGrid.DefaultCol.ColHdrStyle = new TenTec.Windows.iGridLib.iGColHdrStyle(true);
            this._theMethodsGrid.Dock = System.Windows.Forms.DockStyle.Fill;
            this._theMethodsGrid.ForeColor = System.Drawing.SystemColors.WindowText;
            this._theMethodsGrid.Header.Height = 19;
            this._theMethodsGrid.Location = new System.Drawing.Point(0, 0);
            this._theMethodsGrid.Name = "_theMethodsGrid";
            this._theMethodsGrid.ReadOnly = true;
            this._theMethodsGrid.RowMode = true;
            this._theMethodsGrid.Size = new System.Drawing.Size(498, 362);
            this._theMethodsGrid.TabIndex = 0;
            this._theMethodsGrid.TreeCol = null;
            this._theMethodsGrid.TreeLines.Color = System.Drawing.SystemColors.WindowText;
            this._theMethodsGrid.DoubleClickHandler = MethodsGrid_DoubleClickHandler;
            this._splitContainer.Panel2.Controls.Add(this._theMethodsGrid);

            //Listen to grid events
            _methodsGridSelectionChangeHandler = new EventHandler(MethodsGrid_SelectionChanged);
            _theMethodsGrid.SelectionChanged += _methodsGridSelectionChangeHandler;

            _cellClickHandler = new iGCellClickEventHandler(MethodsGrid_CellClick);
            _theMethodsGrid.CellClick += _cellClickHandler;

            _methodsGridInvalidatedHandler = new InvalidateEventHandler(MethodsGrid_Invalidated);
            _theMethodsGrid.Invalidated += _methodsGridInvalidatedHandler;
        }

        /// <summary>
        /// Refreshes the Code file list when the user control is loaded
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void PCFTool_Load(object sender, EventArgs e)
        {
            if (ParentForm != null)
            {
                ParentForm.FormClosing += new FormClosingEventHandler(ParentForm_FormClosing);
            }
            RefreshUI();
        }

        /// <summary>
        /// Cancels any currently running background work
        /// </summary>
        private void CancelAsync()
        {
            if (_backgroundWorker != null && _backgroundWorker.IsBusy)
            {
                isOperationCancelled = true;
                _backgroundWorker.CancelAsync();
                if (_pcfCommandObject != null)
                {
                    try
                    {
                        _pcfCommandObject.Cancel();
                    }
                    catch (Exception)
                    {
                    };
                }
            }
        }

        #endregion Constructors, Initializers, Destructors

        #region Display methods for code file tree
        /// <summary>
        /// Handle the before expand event and load the models for the child tree nodes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void CodeFilesTree_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            //If background worker is busy, ignore the event
            if (_backgroundWorker.IsBusy)
            {
                e.Cancel = true;
                return;
            }
            if (e.Node != null)
            {
                //If jar folder is expanded, load the class names in the background thread
                if (e.Node is JarTreeFolder)
                {
                    //Get the list of class names
                    _operationParamaters.Clear();
                    _operationParamaters.Add("Operation", PCFOperation.GetClasses);
                    _operationParamaters.Add("TreeNode", e.Node);
                    _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);
                    InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingClassNames, e.Node.Text));
                    _theMethodsGrid.Clear();
                    _backgroundWorker.RunWorkerAsync(_operationParamaters);
                }
            }
        }

        /// <summary>
        /// Handles the after select event of the code fie tree
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CodeFilesTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            //If the background worker is busy, ignore the event
            if (_backgroundWorker.IsBusy)
            {
                return;
            }
            if (e.Node != null)
            {
                e.Node.Expand();
                //If the selected node is the root folder, display the code file details in the grid on the right pane
                if (e.Node is PCFTreeFolder)
                {
                    ShowCodeFileInfo(this.PopulateDataFileTable(_codeFilesTree.AccessLevel));
                    ResetStatusStrip();
                }
                else if (e.Node is JarTreeFolder)
                {
                    if (((JarTreeFolder)e.Node).JarFile.ClassNames == null)
                    {
                        if (!_backgroundWorker.IsBusy)
                        {
                            //Get the list of class names
                            _operationParamaters.Clear();
                            _operationParamaters.Add("Operation", PCFOperation.GetClasses);
                            _operationParamaters.Add("TreeNode", e.Node);
                            _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);
                            InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingClassNames, e.Node.Text));
                            _theMethodsGrid.Clear();
                            _backgroundWorker.RunWorkerAsync(_operationParamaters);
                        }
                    }
                    else
                    {
                        //If the selected node is the jar tree node, display the class details in the grid on the right pane
                        ShowJarInfo(this.PopulateJarClassTable((JarTreeFolder)e.Node));
                        ResetStatusStrip();
                    }
                }
                else
                    if (e.Node is ClassTreeNode)
                    {
                        //If the selected node is a class node, request the method details to be displayed
                        _theMethodsGrid.Clear();
                        _operationParamaters.Clear();
                        _operationParamaters.Add("Operation", PCFOperation.GetMethods);
                        _operationParamaters.Add("TreeNode", e.Node);
                        _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

                        //Intiate the method details fetch as a background operation
                        //The methods are displayed on the grid in the background worker completed event handler method
                        InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingMethods, e.Node.Text));
                        _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    }
            }
        }

        /// <summary>
        /// Enabled the buttons based on the selection in the methods grid
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void MethodsGrid_SelectionChanged(object sender, EventArgs e)
        {
            //OK button is enabled only when a method is selected.
            if (_codeFilesTree.SelectedNode is ClassTreeNode)
            {
                _okButton.Enabled = (_theMethodsGrid.SelectedRows.Count > 0);
            }
            else
            {
                _okButton.Enabled = false;
            }

            // only need to manage buttons when the root of the tree node is selected
            if (_codeFilesTree.SelectedNode != null && _codeFilesTree.SelectedNode is PCFTreeFolder)
            {
                // Can handle multiple delete/download...
                _theDeleteButton.Enabled = _theDownloadButton.Enabled = (this._theMethodsGrid.SelectedRows.Count > 0);
                //Rename one file at a time
                _theRenameButton.Enabled = (this._theMethodsGrid.SelectedRows.Count == 1);
            }
        }

        /// <summary>
        /// Handle the hyperlink selection on the grid
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MethodsGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            //If background worker is busy, no-op the hyperlink selection
            if (_backgroundWorker.IsBusy)
            {
                return;
            }

            string aNodeFullPath;
            TreeNode aPCFTreeNode;

            iGCell CurrentCell = _theMethodsGrid.Cells[e.RowIndex, e.ColIndex];

            // Handle hyperline here
            if (e.ColIndex == HyperlinkColIndex)
            {
                aNodeFullPath = _theMethodsGrid.Rows[e.RowIndex].Tag.ToString();
                aPCFTreeNode = _codeFilesTree.FindByFullPath(aNodeFullPath);
                if (aPCFTreeNode != null)
                {
                    aPCFTreeNode.EnsureVisible();
                    aPCFTreeNode.Expand();
                    _codeFilesTree.SelectedNode = aPCFTreeNode;
                    _codeFilesTree.Focus();
                }
                else
                {
                    // ToDo: Tree and the right pane is out of sync, add warning message here.
                    //file exists
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                        Properties.Resources.TreeRefreshNeeded,
                        Properties.Resources.Confirm, MessageBoxButtons.OK);
                }
            }
        }

        private void MethodsGrid_DoubleClickHandler(int rowNumber)
        {
            //Do nothing..
        }
        /// <summary>
        /// Helper method to populate the class name details into a datatable
        /// </summary>
        /// <param name="aJarFolder"></param>
        /// <returns></returns>
        private DataTable PopulateJarClassTable(JarTreeFolder aJarFolder)
        {

            List<string> classes = aJarFolder.JarFile.ClassNames;
            DataRow row = null;

            // Now, define column definitions for datatable.
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(Properties.Resources.JarClassName, typeof(System.String));

            dataTable.TableName = aJarFolder.FullPath;

            if (classes != null)
            {
                foreach (string className in classes)
                {
                    row = dataTable.NewRow();
                    row[Properties.Resources.JarClassName] = className;
                    dataTable.Rows.Add(row);
                }
            }

            return dataTable;
        }

        /// <summary>
        /// Helper method to populate the code file details into a datatable
        /// </summary>
        /// <returns></returns>
        private DataTable PopulateDataFileTable(PCFModel.AccessLevel accessLevel)
        {
            DataRow row = null;
            // Now, define column definitions for datatable.
            DataTable dataTable = new DataTable();

            dataTable.Columns.Add(Properties.Resources.CodeFileName, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.CodeFileSize, typeof(SizeObject));
            dataTable.Columns.Add(Properties.Resources.CodeFileDate, typeof(UnixFileTimestamp));

            if (_codeFilesTree.Nodes.Count > 0)
            {
                dataTable.TableName = _codeFilesTree.Nodes[0].Text;
            }
            else
            {
                dataTable.TableName = _codeFilesTree.ConnectionDefinition.RoleName;
            }

            try
            {
                foreach (CodeFile aCodeFile in _codeFilesTree.CodeFiles)
                {
                    row = dataTable.NewRow();
                    row[Properties.Resources.CodeFileName] = aCodeFile.Name;
                    row[Properties.Resources.CodeFileSize] = new SizeObject(aCodeFile.Size);
                    row[Properties.Resources.CodeFileDate] = new UnixFileTimestamp(aCodeFile.LastModifiedTime);
                    dataTable.Rows.Add(row);
                }
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message,
                    Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                // return back whatever got.
                return dataTable;
            }
            return dataTable;
        }

        /// <summary>
        /// Display the code file details in the grid
        /// </summary>
        /// <param name="aCodeFileInfoTable"></param>
        public void ShowCodeFileInfo(DataTable aCodeFileInfoTable)
        {
            HyperlinkColIndex = 0; // first column

            _theMethodsGrid.BeginUpdate();

            _theMethodsGrid.FillWithData(aCodeFileInfoTable);
            _theMethodsGrid.Dock = DockStyle.Fill;
            _theMethodsGrid.AutoResizeCols = true;
            _theMethodsGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;
            // For Tag the full tree path
            PCFTreeFolder userFolder = (PCFTreeFolder)_codeFilesTree.Nodes[0];

            for (int i = 0; i < _theMethodsGrid.Rows.Count; i++)
            {
                //"super.services\\MddStoredProcs.jar"
                _theMethodsGrid.Rows[i].Tag = aCodeFileInfoTable.TableName + "\\" + _theMethodsGrid.Rows[i].Cells[0].Value;
            }

            // For File Size
            _theMethodsGrid.Cols[1].CellStyle.TextAlign = iGContentAlignment.TopRight;

            _theMethodsGrid.EndUpdate();

            _hyperLinkCellManager.Attach(_theMethodsGrid, HyperlinkColIndex);

            // It's better to call this method after you attached iGHyperlinkCellManager to the grid
            // because the manager can change the formatting of the links dynamically
            _theMethodsGrid.Cols.AutoWidth();
            // allow user to select multiple code files for deletion
            _theMethodsGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;

        }

        /// <summary>
        /// Display the class details of the jar file, in the grid
        /// </summary>
        /// <param name="aJarClassNameListTable"></param>
        private void ShowJarInfo(DataTable aJarClassNameListTable)
        {
            HyperlinkColIndex = 0;    // first column

            _theMethodsGrid.BeginUpdate();

            _theMethodsGrid.FillWithData(aJarClassNameListTable);
            _theMethodsGrid.Dock = DockStyle.Fill;
            _theMethodsGrid.AutoResizeCols = true;
            _theMethodsGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;

            // For Tag the full tree path
            PCFTreeFolder userFolder = (PCFTreeFolder)_codeFilesTree.Nodes[0];

            for (int i = 0; i < _theMethodsGrid.Rows.Count; i++)
            {
                _theMethodsGrid.Rows[i].Tag = aJarClassNameListTable.TableName + "\\" + _theMethodsGrid.Rows[i].Cells[0].Value;
            }

            _theMethodsGrid.EndUpdate();

            // DO NOT USE hyperlink
            _hyperLinkCellManager.Attach(_theMethodsGrid, HyperlinkColIndex);
            // It's better to call this method after you attached iGHyperlinkCellManager to the grid
            // because the manager can change the formatting of the links dynamically
            _theMethodsGrid.Cols.AutoWidth();
            _theMethodsGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.One;

        }

        /// <summary>
        /// Display the method details for the selected class 
        /// </summary>
        /// <param name="aTable">A DataTable to be used with iGrid</param>
        public void ShowClassMethods(DataTable aTable)
        {
            HyperlinkColIndex = -1;   // no hyperlink

            _theMethodsGrid.BeginUpdate();

            _theMethodsGrid.FillWithData(aTable);
            _theMethodsGrid.Dock = DockStyle.Fill;
            _theMethodsGrid.AutoResizeCols = true;
            _theMethodsGrid.AutoWidthColMode = iGAutoWidthColMode.HeaderAndCells;

            _theMethodsGrid.EndUpdate();

            // DO NOT USE hyperlink
            _hyperLinkCellManager.Attach(_theMethodsGrid, HyperlinkColIndex);
            // It's better to call this method after you attached iGHyperlinkCellManager to the grid
            // because the manager can change the formatting of the links dynamically
            _theMethodsGrid.Cols.AutoWidth();
            _theMethodsGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.One;
        }

        /// <summary>
        /// Enables the OK button based on the grid selection
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void MethodsGrid_Invalidated(object sender, InvalidateEventArgs e)
        {
            //Disable the OK button whenever the grid is refreshed.
            if (this._codeFilesTree.SelectedNode is ClassTreeNode)
            {
                _okButton.Enabled = (this._theMethodsGrid.SelectedRows.Count > 0);
            }
            else
            {
                _okButton.Enabled = false;
            }
        }

        /// <summary>
        /// Invoked before starting a background operation
        /// Sets the appropriate status label and enables progress bar
        /// </summary>
        /// <param name="statusMessage"></param>
        void InitializeStatusStrip(String statusMessage)
        {
            _cancelButton.Text = Properties.Resources.Cancel;
            _statusLabel.Text = statusMessage;
            if (!_parentClosing)
            {
                _progressBar.Visible = true;
            }
            _theUploadButton.Enabled = false;
            _theDeleteButton.Enabled = false;
            _theDownloadButton.Enabled = false;
            _theRenameButton.Enabled = false;
            _refreshGuiButton.Enabled = false;
            isOperationCancelled = false;
        }

        /// <summary>
        /// Resets the status label and hides progress bar
        /// Invoked after a background operation completes
        /// </summary>
        void ResetStatusStrip()
        {
            _cancelButton.Text = Properties.Resources.Close;
            if (!_parentClosing)
            {
                _progressBar.Visible = false;
            }
            _statusLabel.Text = "";

            // reset all buttons state
            _refreshGuiButton.Enabled = true;
            _theUploadButton.Enabled = true;
            _theDeleteButton.Enabled = false;
            _theRenameButton.Enabled = false;
            _theDownloadButton.Enabled = false;

            if (_codeFilesTree.SelectedNode != null && _codeFilesTree.SelectedNode is JarTreeFolder)
            {
                _theDownloadButton.Enabled = true;
                _theRenameButton.Enabled = true;
                _theDeleteButton.Enabled = true;
            }
            else if (_codeFilesTree.SelectedNode != null && _codeFilesTree.SelectedNode is PCFTreeFolder)
            {
                // Can handle multiple delete/download...
                _theDeleteButton.Enabled = _theDownloadButton.Enabled = (this._theMethodsGrid.SelectedRows.Count > 0);
                //Rename one file at a time
                _theRenameButton.Enabled = (this._theMethodsGrid.SelectedRows.Count == 1);
            }
        }

        /// <summary>
        /// Refreshes the user control and refetches the details
        /// </summary>
        private void RefreshUI()
        {
            ResetStatusStrip();
            if (_codeFilesTree.Nodes.Count > 0)
            {
                _codeFilesTree.Nodes[0].Nodes.Clear();
            }
            _theMethodsGrid.Clear();
            _operationParamaters.Clear();
            _operationParamaters.Add("Operation", PCFOperation.Refresh);
            _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

            //Request the code file list to be refreshed as a background operation
            InitializeStatusStrip(Properties.Resources.StatusRefreshCodeFilesList);
            _backgroundWorker.RunWorkerAsync(_operationParamaters);
        }

        #endregion Display methodsfor code file tree

        #region Button Handlers

        /// <summary>
        /// Handles the Refresh button click and refreshes the code files list
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _refreshGuiButton_Click(object sender, EventArgs e)
        {
            RefreshUI();
        }
        
        /// <summary>
        /// Handles the OK button click and lets a method to be selected
        /// Applicable only when the PCF tool is invoked from the Create SPJ wizard
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _okButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                Form parent = (Form)Parent;
                parent.DialogResult = DialogResult.OK;
                parent.Close();
            }
        }

        /// <summary>
        /// Handles the close/cancel button click
        /// If the background worker is busy, the cancel event cancels the background work
        /// If the background worker is not busy, the cancel event closes the PCF tool
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _cancelButton_Click(object sender, EventArgs e)
        {
            //If the background worker is not busy, the button text is "Close"
            //Close the PCF tool
            if (!_backgroundWorker.IsBusy)
            {
                if (Parent != null && Parent is Form)
                {
                    ((Form)Parent).Close();
                }
            }
            else
            {
                //If background worker is busy, the button text is Cancel
                //Cancel the background work
                CancelAsync();
            }
            _cancelButton.Enabled = false;
        }
        
        /// <summary>
        /// Handle the upload button click and let users upload one or more jar files
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theUploadButton_Click(object sender, EventArgs e)
        {
            //Display the File Open dialog
            OpenFileDialog dlg = new OpenFileDialog();
            dlg.Filter = "SPJ Code Jar Files(*.jar)|*.jar";
            dlg.FilterIndex = 0;
            dlg.Title = "Upload Code Files";
            dlg.Multiselect = true;
            dlg.FileName = "";
            dlg.AddExtension = true;

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                string[] filenames = dlg.FileNames;
                List<string> existingFiles = new List<string>();
                List<string> newFiles = new List<string>();

                //Identify which of the user selected files already exist in the Code file tree
                foreach (string fileName in filenames)
                {
                    string[] names = fileName.Split('\\');
                    string name = names[names.Length - 1];
                    CodeFile codeFile = _codeFilesTree.FindCodeFileByName(name);
                    if (codeFile != null)
                    {
                        existingFiles.Add(fileName);
                    }
                    else
                    {
                        newFiles.Add(fileName);
                    }
                }

                _operationParamaters.Clear();
                _operationParamaters.Add("Operation", PCFOperation.Upload);
                _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

                DataTable ignoredFilesTable = new DataTable();
                ignoredFilesTable.Columns.Add("File Name");
                ignoredFilesTable.Columns.Add("Details");

                if (existingFiles.Count > 0)
                {
                    FindAndRemoveInvalidFiles(existingFiles, ref ignoredFilesTable);
                }

                if (newFiles.Count > 0)
                {
                    FindAndRemoveInvalidFiles(newFiles, ref ignoredFilesTable);
                }

                if (ignoredFilesTable.Rows.Count > 0)
                {
                    string summaryMessage = Properties.Resources.PCF_IGNORE_FILES_MESSAGE;
                    TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(summaryMessage, ignoredFilesTable, System.Drawing.SystemIcons.Warning);
                    md.ShowDialog();
                }

                //If one or more files exist, prompt user for overwrite confirmation
                if (existingFiles.Count > 0)
                {
                    string message = Properties.Resources.PCF_FILE_OVERWRITE_MESSAGE1 + "\n";
                    int nCount = 0;
                    foreach (string existingName in existingFiles)
                    {
                        string[] names = existingName.Split('\\');
                        string name = names[names.Length - 1];
                        message += "\n" + name;
                        nCount++;
                        if (nCount > 20)
                            break;
                    }
                    if (nCount > 20)
                    {
                        message += String.Format(Properties.Resources.AndMore, (nCount - 20)) ;
                    }
                    message += "\n\n" + Properties.Resources.PCF_FILE_OVERWRITE_MESSAGE2 + "\n\n";

                    DialogResult result = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                        message, Properties.Resources.Warning, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);

                    //If user confirmed overwrite, send the complete file name list
                    if (result == DialogResult.Yes)
                    {
                        existingFiles.AddRange(newFiles);
                        _operationParamaters.Add("FileNames", existingFiles.ToArray());
                    }
                    else
                        if (result == DialogResult.No)
                        {
                            //If user declined overwrite, skip the files that exist and only send the files that do not exist already
                            _operationParamaters.Add("FileNames", newFiles.ToArray());
                        }
                        else
                        {
                            //If user decided to abort, cancel the upload and return
                            return;
                        }
                }
                else
                {
                    if (newFiles.Count > 0)
                    {
                        _operationParamaters.Add("FileNames", newFiles.ToArray());
                    }
                    else
                    {
                        return;
                    }
                }

                //Initiate the upload as a background operation
                InitializeStatusStrip(String.Format(Properties.Resources.StatusUploading,""));
                _backgroundWorker.RunWorkerAsync(_operationParamaters);
                _cancelButton.Enabled = true;
            }
        }

        public void FindAndRemoveInvalidFiles(List<string> fileNameList, ref DataTable ignoredFilesTable)
        {
            foreach (string fileName in fileNameList)
            {
                if (!fileName.EndsWith(".jar", StringComparison.OrdinalIgnoreCase))
                {
                    ignoredFilesTable.Rows.Add(new string[] { fileName, Properties.Resources.PCF_NOT_JAR_MESSAGE });
                }
                FileInfo f = new FileInfo(fileName);
                if (f.Length == 0)
                {
                    ignoredFilesTable.Rows.Add(new string[] { fileName, Properties.Resources.PCF_ZERO_SIZE_MESSAGE });
                }
                if(FILE_UPLOAD_SIZE > 0 && f.Length > FILE_UPLOAD_SIZE)
                {
                    ignoredFilesTable.Rows.Add(new string[] { fileName, string.Format(Properties.Resources.PCF_SIZE_EXCEEDS_MESSAGE, Utilities.FormatSize(FILE_UPLOAD_SIZE)) });
                }
            }

            foreach (DataRow row in ignoredFilesTable.Rows)
            {
                fileNameList.Remove(row[0] as string);
            }
        }

        /// <summary>
        /// Handle the delete button click and let users delete one or more code files
        /// User has the option to delete a single file by selecting a single file in 
        /// the code file tree and the pressing the delete button or to select one or more
        /// files in the code files list on the right pane and then press the delete button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theDeleteButton_Click(object sender, EventArgs e)
        {
            List<string> selectedCodeFileNames = new List<string>();

            //If a jar tree node is selected in the code file tree, mark that file for delete
            if (_codeFilesTree.SelectedNode != null && _codeFilesTree.SelectedNode is JarTreeFolder)
            {
                selectedCodeFileNames.Add(((JarTreeFolder)_codeFilesTree.SelectedNode).JarFile.Name);
            }
            else
            {
                // user has one or more Code File Name selected in the grid 
                foreach (TenTec.Windows.iGridLib.iGRow theRow in _theMethodsGrid.SelectedRows)
                {
                    selectedCodeFileNames.Add(theRow.Cells[Properties.Resources.ProcedureCodeFileName].Text);
                }
            }

            if (selectedCodeFileNames.Count > 0)
            {
                //Display a message for user confirmation to proceed with the delete
                string message = Properties.Resources.PCF_FILE_DELETE_MESSAGE + "\n";
                int nCount = 0;
                foreach (string fileName in selectedCodeFileNames)
                {
                    message += "\n" + fileName;
                    nCount++;
                    if (nCount > 20)
                        break;
                }
                if (nCount > 20)
                {
                    message += String.Format(Properties.Resources.AndMore, (nCount - 20));
                }
                message += "\n\n" + Properties.Resources.PCF_FILE_DELETE_MESSAGE_1 + "\n\n";

                DialogResult result = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), 
                    message, Properties.Resources.Warning, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    _operationParamaters.Clear();
                    _operationParamaters.Add("Operation", PCFOperation.Delete);
                    _operationParamaters.Add("FileNames", selectedCodeFileNames.ToArray());
                    _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

                    //Initiate the delete as a background operation
                    InitializeStatusStrip(String.Format(Properties.Resources.StatusDeleting, ""));
                    _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    _cancelButton.Enabled = false;
                }
            }
        }

        /// <summary>
        /// Handle the download button click and let users download one or more files
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theDownloadButton_Click(object sender, EventArgs e)
        {
            //Identify the files to download
            List<string> selectedCodeFileNames = new List<string>();

            //If a jar tree node is selected, mark the file for download
            if (_codeFilesTree.SelectedNode != null && _codeFilesTree.SelectedNode is JarTreeFolder)
            {
                selectedCodeFileNames.Add(((JarTreeFolder)_codeFilesTree.SelectedNode).JarFile.Name);
            }
            else
            {
                // user has one or more Code File Name selected in the grid
                foreach (TenTec.Windows.iGridLib.iGRow theRow in _theMethodsGrid.SelectedRows)
                {
                    selectedCodeFileNames.Add(theRow.Cells[Properties.Resources.ProcedureCodeFileName].Text);
                }
            }

            if (selectedCodeFileNames.Count > 0)
            {
                //Display a folder browser dialog to let the user select the target folder.
                FolderBrowserDialog folderDialog = new FolderBrowserDialog();
                folderDialog.Description = Properties.Resources.SelectTargetFolderForJars;
                folderDialog.SelectedPath = Trafodion.Framework.Utilities.FileDialogLocation();

                if (folderDialog.ShowDialog() == DialogResult.OK)
                {
                    string targetDirectory = folderDialog.SelectedPath;
                    List<string> existingFiles = new List<string>();
                    List<string> newFiles = new List<string>();
                    foreach (string fileName in selectedCodeFileNames)
                    {
                        if (File.Exists(Path.Combine(targetDirectory, fileName)))
                        {
                            existingFiles.Add(fileName);
                        }
                        else
                        {
                            newFiles.Add(fileName);
                        }

                    }
                    //Download the files
                    _operationParamaters.Clear();
                    _operationParamaters.Add("Operation", PCFOperation.Download);
                    _operationParamaters.Add("FolderName", targetDirectory);
                    _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

                    //If one or more files exist, prompt user for overwrite confirmation
                    if (existingFiles.Count > 0)
                    {
                        string message = String.Format(Properties.Resources.PCF_FILE_DOWNLOAD_OVERWRITE_MESSAGE, targetDirectory) + "\n";
                        int nCount = 0;
                        foreach (string existingName in existingFiles)
                        {
                            message += "\n" + existingName;
                            nCount++;
                            if (nCount > 20)
                                break;
                        }
                        if (nCount > 20)
                        {
                            message += String.Format(Properties.Resources.AndMore, (nCount - 20));
                        }
                        message += "\n\n" + Properties.Resources.PCF_FILE_OVERWRITE_MESSAGE2 + "\n\n";

                        DialogResult result = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                            message, Properties.Resources.Warning, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);

                        //If user confirmed overwrite, send the complete file name list
                        if (result == DialogResult.Yes)
                        {
                            _operationParamaters.Add("FileNames", selectedCodeFileNames.ToArray());
                        }
                        else
                            if (result == DialogResult.No)
                            {
                                //If user declined overwrite, skip the files that exist and only send the files that do not exist already
                                _operationParamaters.Add("FileNames", newFiles.ToArray());
                            }
                            else
                            {
                                //If user decided to abort, cancel the upload and return
                                return;
                            }
                    }
                    else
                    {
                        _operationParamaters.Add("FileNames", selectedCodeFileNames.ToArray());
                    }

                    //Inititate the download as a background operation
                    InitializeStatusStrip(String.Format(Properties.Resources.StatusDownloading, ""));
                    _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    _cancelButton.Enabled = true;
                }
            }
        }

        /// <summary>
        /// Handle the rename button click and let user rename a single code file
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theRenameButton_Click(object sender, EventArgs e)
        {
            String currentFileName = "";
            if (_codeFilesTree.SelectedNode != null && _codeFilesTree.SelectedNode is JarTreeFolder)
            {
                currentFileName = _codeFilesTree.SelectedNode.Text;
            }
            else
            if (_theMethodsGrid.SelectedRows.Count == 1)
            {
                currentFileName = _theMethodsGrid.SelectedRows[0].Cells[Properties.Resources.CodeFileName].Value as string;
            }

            if(String.IsNullOrEmpty(currentFileName))
                return;

            int extStart = currentFileName.LastIndexOf('.');
            string CFExt = "";
            //Identify the current extension of the code file
            if (extStart > -1)
            {
                //Valid file extension
                CFExt = currentFileName.Substring(extStart, (currentFileName.Length - extStart));
            }

            //Confirm that the user wants to rename this file
            DialogResult dr = MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                            string.Format(Properties.Resources.PCF_FILE_RENAME_MESSAGE, currentFileName),
                            Properties.Resources.Confirm, MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            if (dr == DialogResult.Yes)
            {
                RenameCodeFile rcf = new RenameCodeFile();
                DialogResult renameResult = rcf.ShowDialog();
                if (renameResult == DialogResult.OK)
                {
                    String newCodeFileName = rcf.CodeFileName;

                    //Check if they specified an extension. If not, append the old extension before the rename
                    if(newCodeFileName.LastIndexOf('.') < 0)
                    {
                        newCodeFileName += CFExt;
                    }

                    _operationParamaters.Clear();
                    _operationParamaters.Add("Operation", PCFOperation.Rename);
                    _operationParamaters.Add("CurrentFileName", currentFileName);
                    _operationParamaters.Add("NewFileName", newCodeFileName);
                    _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

                    //Initiate the rename as a background operation
                    InitializeStatusStrip(String.Format(Properties.Resources.StatusRenaming, currentFileName, newCodeFileName));
                    _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    _cancelButton.Enabled = false;
                }
            }
        }

        #endregion Button Handlers

        #region Fetch methods that run in background


        /// <summary>
        /// The method invoked by the background thread to refresh the list of code files
        /// This method invokes the PCF model to get the list
        /// </summary>
        private void RefreshCodeFileList()
        {
            //Clear the cache of code files stored in the PCF tree
            this._codeFilesTree.CodeFiles.Clear();

            SqlMxSystem sqlMxSystem = SqlMxSystem.FindSqlMxSystem(_theConnectionDefinition);
            DataTable table = _pcfModel.GetFiles(PCFModel.AccessLevel.Role);
            DataView sortedView = table.DefaultView;
            sortedView = table.DefaultView;
            sortedView.Sort = table.Columns[0].ColumnName + " asc";
            table = sortedView.ToTable();

            //Add the user access level code files to the PCF tree
            if (table != null)
            {
                for (int i = 0; i < table.Rows.Count; i++)
                {
                    CodeFile file = new CodeFile((string)table.Rows[i][0], (long)table.Rows[i][2], (long)table.Rows[i][3], PCFModel.AccessLevel.Role);
                    file.FullyQualifiedPath = (string)table.Rows[i][1];
                    this._codeFilesTree.CodeFiles.Add(file);
                }
            }
        }

        /// <summary>
        /// Refreshes the list of class name for a given code file
        /// </summary>
        /// <param name="aCodeFile"></param>
        private void RefreshClassNames(CodeFile aCodeFile)
        {
            if (aCodeFile.ClassNames == null)
            {
                aCodeFile.ClassNames = _pcfModel.getClasses(aCodeFile.AccessLevel, aCodeFile.Name);
            }
        }

        /// <summary>
        /// Refreshes the method details for a given class
        /// </summary>
        /// <param name="aClassTreeNode"></param>
        private void RefreshMethods(ClassTreeNode aClassTreeNode)
        {
            if (aClassTreeNode.Methods == null)
            {
                String className = (string)aClassTreeNode.Tag;
                String jarFile = null;
                CodeFile _theCodeFile = aClassTreeNode.Parent.Tag as CodeFile;

                if (_theCodeFile != null)
                {
                    jarFile = _theCodeFile.Name;
                }
                aClassTreeNode.Methods = _pcfModel.getMethods(_theCodeFile.AccessLevel, jarFile, className);
            }
        }

        #endregion Fetch methods that run in background

        #region SPJ Management methods that run in background

        /// <summary>
        /// Uploads a file
        /// </summary>
        /// <param name="fileName"></param>
        private void UploadFile(PCFModel.AccessLevel accessLevel, string fileName, BackgroundWorker worker, DoWorkEventArgs e)
        {
            int BLOCK_SIZE = 10240;
            int BUFFER_SIZE = BLOCK_SIZE * 3 / 4;
            string[] names = fileName.Split('\\');
            string name = names[names.Length - 1];

            string content = "";
            FileInfo f = new FileInfo(fileName);

            BinaryReader br = new BinaryReader(f.OpenRead());

            Boolean first = true;
            int count = 0;
            byte[] buffer = new byte[BUFFER_SIZE];
            char[] charString;
            Array.Clear(buffer, 0, BUFFER_SIZE);
            try
            {
                //Read the file contents in binary and convert to base64
                //and send 10K chunks of bytes at a time
                //for considering uploading file is big, we check cancellation flag
                while ((count = br.Read(buffer, 0, BUFFER_SIZE)) > 0 && !worker.CancellationPending)
                {
                    content = System.Convert.ToBase64String(buffer, 0, count);
                    charString = content.ToCharArray();
                    int length = charString.Length;
                    if (first)
                    {
                        //The first chunk, identified by the last parameter of 1
                        _pcfModel.write(accessLevel, name, charString, length, 1);
                        first = false;
                    }
                    else
                    {
                        _pcfModel.write(accessLevel, name, charString, length, 0);
                    }

                    Array.Clear(buffer, 0, BUFFER_SIZE);
                }
            }
            finally
            {
                br.Close();

                //considering big file
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }

            }
        }

        /// <summary>
        /// Deletes a code file from Trafodion
        /// </summary>
        /// <param name="aFile"></param>
        private void RemoveFile(PCFModel.AccessLevel accessLevel, String aFile)
        {
            _pcfModel.delete(accessLevel, aFile);
        }

        /// <summary>
        /// Downloads a given file to the specified target directory
        /// </summary>
        /// <param name="fileName"></param>
        /// <param name="targetDirectory"></param>
        private void DownloadFile(PCFModel.AccessLevel accessLevel, string fileName, String targetDirectory, BackgroundWorker worker, DoWorkEventArgs e)
        {
            if (fileName != null)
            {
                string content = null;
                int count = 0, totalCount = 0;
                byte[] data = new byte[10240];

                FileInfo f = new FileInfo(Path.Combine(targetDirectory, fileName));
                BinaryWriter br = new BinaryWriter(f.OpenWrite());
                try
                {
                    //Read 10K chunks of base64 bytes and convert them to binary
                    //and write to target file
                    do
                    {
                        //The count is the actual count rather than the length of the content
                        _pcfModel.get(accessLevel, fileName, totalCount, out content, out count);

                        //fileContent.Append(content);
                        int len = content.Length;
                        if (count > 0)
                        {
                            data = System.Convert.FromBase64String(content);
                            br.Write(data, 0, count);
                        }
                        totalCount += count;
                    } while (count > 0 && !worker.CancellationPending);
                }
                catch (Exception ex)
                {
                    if (!worker.CancellationPending)
                    {
                        throw ex;
                    }
                }
                finally
                {
                    br.Close();

                    //considering big file
                    if (worker.CancellationPending)
                    {
                        e.Cancel = true;
                    }
                }
            }
        }

        /// <summary>
        /// Renames a code file
        /// </summary>
        /// <param name="currentFileName"></param>
        /// <param name="newFileName"></param>
        private void RenameFile(PCFModel.AccessLevel accessLevel, String currentFileName, string newFileName)
        {
            _pcfModel.rename(accessLevel, currentFileName, newFileName);
        }

        #endregion SPJ Management methods

        #region Background worker methods

        /// <summary>
        /// Background Method that is asynchronously invoked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender, System.ComponentModel.DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            try
            {
                HandlePCFOperations((Hashtable)e.Argument, worker, e);
            }
            catch (Exception ex)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }
                throw ex;
            }
        }

        /// <summary>
        /// The  worker method that does the PCF operations
        /// </summary>
        /// <param name="operationParameters"></param>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        private void HandlePCFOperations(Hashtable operationParameters, BackgroundWorker worker, DoWorkEventArgs e)
        {
            PCFOperation operation = (PCFOperation)operationParameters["Operation"];
            PCFModel.AccessLevel accessLevel = (PCFModel.AccessLevel)operationParameters["AccessLevel"];

            switch (operation)
            {
                case PCFOperation.Refresh:
                    {
                        //try
                        {
                            //On a Refresh operation, refresh the code file list
                            RefreshCodeFileList();
                            break;
                        }
                        //catch (Exception ex)
                        //{
                        //    throw ex;
                        //}

                    }
                case PCFOperation.GetClasses:
                    {
                        JarTreeFolder aJarFolder = (JarTreeFolder)operationParameters["TreeNode"];
                        //Get list of class names for the code file
                        RefreshClassNames(aJarFolder.JarFile);
                        break;
                    }
                case PCFOperation.GetMethods:
                    {
                        ClassTreeNode classTreeNode = (ClassTreeNode)operationParameters["TreeNode"];
                        //If the methods are not already loaded, load them for the given class
                        if (classTreeNode.Methods == null)
                        {
                            RefreshMethods(classTreeNode);
                        }
                        break;
                    }
                case PCFOperation.Upload:
                    {
                        string[] fileNames = (string[])operationParameters["FileNames"];
                        System.Text.StringBuilder errorMessageBuilder = new System.Text.StringBuilder();

                        //Upload each of the file in the parameter list
                        foreach (string fileName in fileNames)
                        {
                            //If user has request cancel, abort further work
                            if (worker.CancellationPending)
                            {
                                e.Cancel = true;
                                return;
                            }
                            //Displays the status message for the current file
                            worker.ReportProgress(0, String.Format(Properties.Resources.StatusUploading, fileName));

                            try
                            {
                                UploadFile(accessLevel, fileName, worker, e);
                            }
                            catch (Exception ex)
                            {
                                //Since multi-file upload is allowed, capture individual file upload failures and report one exception
                                //at the end
                                errorMessageBuilder.AppendFormat("{0}{1}{2}\n", fileName, TrafodionManager_MESSAGE_SEPARATOR, ex.Message);
                            }
                        }
                        if (!worker.CancellationPending && errorMessageBuilder.ToString().Length > 0)
                        {
                            throw new Exception(errorMessageBuilder.ToString());
                        }
                        break;
                    }
                case PCFOperation.Download:
                    {
                        string[] fileNames = (string[])operationParameters["FileNames"];
                        string targetDirectory = (string)operationParameters["FolderName"];
                        System.Text.StringBuilder errorMessageBuilder = new System.Text.StringBuilder();

                        //Download each file in the parameter list
                        foreach (string fileName in fileNames)
                        {
                            //If user has request cancel, abort further work
                            if (worker.CancellationPending)
                            {
                                e.Cancel = true;
                                return;
                            }
                            //Displays the status message for the current file
                            worker.ReportProgress(0, String.Format(Properties.Resources.StatusDownloading, fileName));

                            try
                            {
                                DownloadFile(accessLevel, fileName, targetDirectory, worker, e);
                            }
                            catch (Exception ex)
                            {
                                //Since multi-file download is allowed, capture individual file download failures and report one exception
                                //at the end
                                errorMessageBuilder.AppendFormat("{0}{1}{2}\n", fileName, TrafodionManager_MESSAGE_SEPARATOR, ex.Message);
                            }
                        }
                        if (!worker.CancellationPending && errorMessageBuilder.ToString().Length > 0)
                        {
                            throw new Exception(errorMessageBuilder.ToString());
                        }
                        break;
                    }
                case PCFOperation.Delete:
                    {
                        string[] fileNames = (string[])operationParameters["FileNames"];
                        System.Text.StringBuilder errorMessageBuilder = new System.Text.StringBuilder();
                        foreach (string fileName in fileNames)
                        {
                            //If user has request cancel, abort further work
                            if (worker.CancellationPending)
                            {
                                e.Cancel = true;
                                return;
                            }
                            //Displays the status message for the current file
                            worker.ReportProgress(0, String.Format(Properties.Resources.StatusDeleting, fileName));

                            try
                            {
                                RemoveFile(accessLevel, fileName);
                            }
                            catch (Exception ex)
                            {
                                //Since multi-file delete is allowed, capture individual file delete failures and report one exception
                                //at the end
                                errorMessageBuilder.AppendFormat("{0}{1}{2}\n", fileName, TrafodionManager_MESSAGE_SEPARATOR, ex.Message);
                            }
                        }
                        if (errorMessageBuilder.ToString().Length > 0)
                        {
                            throw new Exception(errorMessageBuilder.ToString());
                        }
                        break;
                    }
                case PCFOperation.Rename:
                    {
                        string currentFileName = (string)operationParameters["CurrentFileName"];
                        string newFileName = (string)operationParameters["NewFileName"];
                        //Rename the code file
                        RenameFile(accessLevel, currentFileName, newFileName);
                        break;
                    }
                default:
                    {
                        break;
                    }
            }
        }

        /// <summary>
        /// Handles any progress events from the background worker
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_ProgressChanged(object sender, System.ComponentModel.ProgressChangedEventArgs e)
        {
            //set the status label for multi-file operations
            _statusLabel.Text = e.UserState as string;
        }

        /// <summary>
        /// Handle completion events from the background worker
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(object sender, System.ComponentModel.RunWorkerCompletedEventArgs e)
        {
            ResetStatusStrip();
            PCFOperation operation = (PCFOperation)this._operationParamaters["Operation"];

            //If there was error in the background operation, display the error
            if (e.Error != null)
            {
                //set control status when error occurred.
                _cancelButton.Text = Properties.Resources.Close;
                //_statusLabel.Text = e.Error.Message;
                //MessageBox.Show(e.Error.Message, "error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                if (!_parentClosing)
                {
                    _progressBar.Visible = false;
                }
                //_theUploadButton.Enabled = false;
                //_theDeleteButton.Enabled = false;
                //_theDownloadButton.Enabled = false;
                //_theRenameButton.Enabled = false;
                //_refreshGuiButton.Enabled = true;

                string operationName = System.Enum.GetName(typeof(PCFOperation), operation);

                if (!isOperationCancelled)
                {
                    switch (operation)
                    {
                         case PCFOperation.Download:
                            {
                                string[] errorMessages = e.Error.Message.Split(new string[] { "\n" }, StringSplitOptions.RemoveEmptyEntries);
                                DataTable errorTable = new DataTable();
                                errorTable.Columns.Add("Code File Name");
                                errorTable.Columns.Add("Error Text");
                                foreach (string message in errorMessages)
                                {
                                    string[] tokens = message.Split(new string[] { TrafodionManager_MESSAGE_SEPARATOR }, StringSplitOptions.RemoveEmptyEntries);
                                    if (tokens.Length > 1)
                                    {
                                        errorTable.Rows.Add(new object[] { tokens[0], tokens[1] });
                                    }
                                }
                                string summaryMessage = String.Format("{0} failed for one or more file(s).", operationName);
                                TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(summaryMessage, errorTable, System.Drawing.SystemIcons.Error);
                                md.ShowDialog();
                                break;
                            }

                        case PCFOperation.GetClasses:
                            {
                                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                                String.Format(Properties.Resources.PCFOperationFailed, operationName) + "\n\n" + e.Error.Message,
                                                Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);

                                //In case exception is encountered; the class names should contain null 
                                //This way, all the tree node and right pane will display properly
                                JarTreeFolder jarFolder = (JarTreeFolder)_operationParamaters["TreeNode"];
                                _codeFilesTree.PopulateFolder(jarFolder);
                                ShowJarInfo(PopulateJarClassTable(jarFolder));
                                _codeFilesTree.SelectedNode = jarFolder;
                                break;
                            }

                        default:
                            {
                                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    String.Format(Properties.Resources.PCFOperationFailed, operationName) + "\n\n" + e.Error.Message,
                                    Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
                                break;
                            }
                    }
                }
                else
                {
                    switch (operation)
                    {
                        case PCFOperation.Delete:
                            {
                                string message = "Delete partial files operation is failed./n";
                                message += e.Error.Message;
                                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), message, Properties.Resources.Info,
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                                break;
                            }
                        case PCFOperation.Refresh:
                            {
                                //Notify the user that the after operation refresh has been cancelled.
                                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                        Properties.Resources.PCFRefreshCancelledMessage, 
                                        Properties.Resources.Info,
                                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                                break;
                            }
                        default:
                            {
                                break;
                            }
                    }
                }
            }
            else
            if (e.Cancelled)
            {
                switch (operation)
                {
                    case PCFOperation.Upload:
                        {
                            DeleteUploadedFiles();
                            break;
                        }
                    case PCFOperation.Download:
                        {
                            DeleteDownloadedFiles();
                            break;
                        }
                    case PCFOperation.Refresh:
                        {
                            //Notify the user that the after operation refresh has been cancelled.
                            MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    Properties.Resources.PCFRefreshCancelledMessage, 
                                    Properties.Resources.Info,
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
            }
            else
            {
                switch (operation)
                {
                    case PCFOperation.Refresh:
                        {
                            //Since the refresh operation fetched a fresh list of code files
                            //display this list in the tree and the grid
                           _codeFilesTree.SetAndPopulateRootFolders();
                            ShowCodeFileInfo(this.PopulateDataFileTable(_codeFilesTree.AccessLevel));

                            //Force selection on the root node, so the contents are displayed on the right pane.
                            if (_codeFilesTree.Nodes.Count > 0)
                            {
                                _codeFilesTree.SelectedNode = _codeFilesTree.Nodes[0];
                            }

                            if (isOperationCancelled)
                            {
                                //Notify the user that the after operation refresh has been cancelled.
                                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    Properties.Resources.PCFRefreshCancelledMessage, 
                                    Properties.Resources.Info,
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                            }
                            break;
                        }
                case PCFOperation.GetClasses:
                    {
                        //Since we have a fresh list of classes for the jar file, refresh the tree and the right pane
                        JarTreeFolder jarFolder = (JarTreeFolder)_operationParamaters["TreeNode"];
                        _codeFilesTree.PopulateFolder(jarFolder);
                        ShowJarInfo(PopulateJarClassTable(jarFolder));
                        _codeFilesTree.SelectedNode = jarFolder;
                        break;
                    }
                case PCFOperation.GetMethods:
                    {
                        //Since we have a fresh list of meethods for the class, refresh the tree and the right pane
                        ClassTreeNode classTreeNode = (ClassTreeNode)_operationParamaters["TreeNode"];
                        DataTable table = classTreeNode.Methods;
                        if (table != null)
                        {
                            ShowClassMethods(table);
                        }
                        _codeFilesTree.SelectedNode = classTreeNode;
                        break;
                    }
                    case PCFOperation.Upload:
                        {
                            //if CancellationPending flag is not set to true then we delete partial uploaded files from here.
                            if (isOperationCancelled == true)
                            {
                                DeleteUploadedFiles();
                            }
                            else
                            {
                                //Since new files have been uploaded, refresh the tree to display the new files
                                RefreshUI();
                            }
                            break;
                        }
                    case PCFOperation.Download:
                        {
                            if (isOperationCancelled == true)
                            {
                                DeleteDownloadedFiles();
                            }
                            else
                            {
                                //Notify the user that the download completed successfully
                                MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                    Properties.Resources.PCFDownloadComplete, Properties.Resources.Info,
                                    MessageBoxButtons.OK, MessageBoxIcon.Information);
                            }
                            break;
                        }
                    case PCFOperation.Delete:
                        {
                            //delete partial files operation will invoke this method, so we need to differenciate partial deleting from user deleting operation.
                            if (!isOperationCancelled)
                            {
                                //Since some code files have been deleted, refresh the tree to display the current state
                                RefreshUI();
                            }
                            break;
                        }
                    case PCFOperation.Rename:
                        {
                            //Since a code has been renamed , refresh the tree to display the current state
                            RefreshUI();
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
            }

        }

        //used to delete partial file delete operation.
        private void DeleteUploadedFiles()
        {
            string[] fileNames = (string[])_operationParamaters["FileNames"];
            List<string> filenameList = new List<string>();

            foreach (string fileName in fileNames)
            {
                string[] names = fileName.Split('\\');
                string name = names[names.Length - 1];
                filenameList.Add(name);
            }

            _operationParamaters.Clear();
            _operationParamaters.Add("Operation", PCFOperation.Delete);
            _operationParamaters.Add("FileNames", filenameList.ToArray());
            _operationParamaters.Add("AccessLevel", _codeFilesTree.AccessLevel);

            //Initiate the delete as a background operation
            InitializeStatusStrip(String.Format(Properties.Resources.StatusDeleting, ""));
            isOperationCancelled = true;
            _backgroundWorker.RunWorkerAsync(_operationParamaters);
            _cancelButton.Enabled = false;
        }

        private void DeleteDownloadedFiles()
        {
            string[] fileNames = (string[])_operationParamaters["FileNames"];
            string targetDirectory = (string)_operationParamaters["FolderName"];
            System.Text.StringBuilder errorMessage = new System.Text.StringBuilder();
            foreach (string fileName in fileNames)
            {
                try
                {
                    File.Delete(Path.Combine(targetDirectory, fileName));
                }
                catch (Exception ex)
                {
                    errorMessage.Append("delete file " + fileName + " is failed./n");
                }
            }
            string message = "";
            if (errorMessage.Length > 0)
            {
                message = "Download operation is cancelled, but failed to delete one or more file(s) which already downloaded.";
            }
            else
            {
                message = "Download operation is cancelled.";
            }

            MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), message, Properties.Resources.Info,
                MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        #endregion Background worker methods

        private void _helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UsePCFT);
        }

    }
}
