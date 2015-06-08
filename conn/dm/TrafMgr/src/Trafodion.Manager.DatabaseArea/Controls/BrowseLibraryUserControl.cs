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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Library deployment user control.
    /// </summary>
    public partial class BrowseLibraryUserControl : UserControl 
    {
        #region Private Member Variables
        //XML schema definition dataset
        private DataSet _dataSet = new DataSet();
        private BrowseLibraryTreeView _browseLibraryTreeView = null;
        private TrafodionObject _initalTrafodionObject = null;
        ConnectionDefinition _connectionDefinition = null;        

        TreeViewEventHandler _afterSelectHandler = null;
        TreeViewCancelEventHandler _beforeExpandHandler = null;
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        iGCellClickEventHandler _cellClickHandler = null;
        EventHandler _theObjectInfoGridSelectionChangeHandler = null;

        bool isOperationCancelled = false;
        // default is no column has underline link.
        int HyperlinkColIndex = -1;
        System.ComponentModel.BackgroundWorker _backgroundWorker;
        enum BrowseLibraryOperation { Refresh, GetClasses,GetMethods,Download };
        Hashtable _operationParamaters = new Hashtable();
        const string TRAFMGR_MESSAGE_SEPARATOR = "TRAFMGR_MESSAGE_SEPARATOR";
        bool _parentClosing = false;
        //for cancelling operation
        System.Data.Odbc.OdbcCommand _libraryWorkingCommandObject = null;
        #endregion Private Member Variables

        #region public Properties
        /// <summary>
        /// The class tree node that is currently selected in the code file tree
        /// </summary>
        public ClassLeaf SelectedClassTreeNode
        {
            get
            {
                TreeNode node = _browseLibraryTreeView.SelectedNode;
                if (node != null && node is ClassLeaf)
                {
                    return ((ClassLeaf)node);
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

                if (_theObjectInfoGrid.SelectedRows.Count > 0)
                {
                    javaMethod = new JavaMethod(_theObjectInfoGrid.SelectedRows[0].Cells[0].Value as string,
                                                _theObjectInfoGrid.SelectedRows[0].Cells[1].Value as string
                                                           );
                }
                return javaMethod;
            }
        }
        #endregion

        #region Constructors, Initializers, Destructors
        /// <summary>
        /// Constructor 
        /// </summary>
        /// <param name="aTrafodionObject">a selected object</param>
        /// <param name="isCreateMode">non-create mode for select a method to create procedure.</param>
        public BrowseLibraryUserControl(TrafodionObject aTrafodionObject,bool isCreateMode)
        {
            InitializeComponent();
            _initalTrafodionObject = aTrafodionObject;
            //OK button is visible only when Browse Library Tool is invoked from CreateSPJ tool
            if (!_parentClosing)
            {
                _theOKButton.Visible = isCreateMode;
            }
            //Load XSD dataset.
            _dataSet.ReadXmlSchema(Path.Combine(Application.StartupPath, "TrafodionSPJ.XSD"));

            TrafodionCatalog _currentTrafodionCatalog = null;
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
            _connectionDefinition = _initalTrafodionObject.ConnectionDefinition;

            _browseLibraryTreeView = new BrowseLibraryTreeView(_currentTrafodionCatalog);
            _browseLibraryTreeView.Dock = DockStyle.Fill;
            _browseLibraryTreeView.CheckAndAddListener();

            _cellClickHandler = new iGCellClickEventHandler(ObjectInfoGrid_CellClick);
            _theObjectInfoGrid.CellClick += _cellClickHandler;
            _theObjectInfoGridSelectionChangeHandler = new EventHandler(_theObjectInfoGrid_SelectionChanged);
            _theObjectInfoGrid.SelectionChanged += _theObjectInfoGridSelectionChangeHandler;
            _theObjectInfoGrid.DoubleClickHandler = ObjectInfoGrid_DoubleClickHandler;
            //Set selected rows color while losed focus.
            _theObjectInfoGrid.SelCellsBackColorNoFocus = System.Drawing.SystemColors.Highlight;
            _theObjectInfoGrid.SelCellsForeColorNoFocus = System.Drawing.SystemColors.HighlightText;
            _theObjectInfoGrid.AllowColumnFilter = false;

            _splitContainer.Panel1.Controls.Add(_browseLibraryTreeView);
            _browseLibraryTreeView.SetAndPopulateRootFolders();
            _browseLibraryTreeView.SelectTrafodionObject(_initalTrafodionObject);
            _afterSelectHandler= new TreeViewEventHandler(_browseLibraryTreeView_AfterSelect);
            _beforeExpandHandler = new TreeViewCancelEventHandler(_browseLibraryTreeView_BeforeExpand);
             _browseLibraryTreeView.AfterSelect += _afterSelectHandler;
             _browseLibraryTreeView.BeforeExpand += _beforeExpandHandler;

             SetToolTipsForButtons();

             InitializeBackgoundWorker();
             ReportStatusStrip(string.Empty);
             UpdateControls();
        }

        /// <summary>
        /// Cleanup before dispose
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            CancelAsync();

            _browseLibraryTreeView.BeforeExpand -= _beforeExpandHandler;
            _browseLibraryTreeView.AfterSelect -= _afterSelectHandler;
            _browseLibraryTreeView.Dispose();
            _theObjectInfoGrid.CellClick -= _cellClickHandler;
            _theObjectInfoGrid.SelectionChanged -= _theObjectInfoGridSelectionChangeHandler;
            _hyperLinkCellManager.Detach();
        }
        #endregion

        #region Control Events

        private void BrowseLibraryUserControl_Load(object sender, EventArgs e)
        {
            if (ParentForm != null)
            {
                ParentForm.FormClosing += new FormClosingEventHandler(ParentForm_FormClosing);
            }
        }

        private void ParentForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            _parentClosing = true;
        }

        private void ObjectInfoGrid_DoubleClickHandler(int rowNumber)
        {
            //Do nothing..
        }

        /// <summary>
        /// Before expand event process for tree view.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _browseLibraryTreeView_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            //If background worker is busy, ignore the event
            if (_backgroundWorker.IsBusy)
            {
                e.Cancel = true;
                return;
            }
            if (e.Node != null)
            {
                if (e.Node is LibraryFolder)
                {
                    //If Library folder is expanding, load the class names in the background thread
                    if (((LibraryFolder)e.Node).TheTrafodionLibrary.TheJarClasses == null)
                    {
                        if (!_backgroundWorker.IsBusy)
                        {
                            //Get the list of class names
                            _operationParamaters.Clear();
                            _operationParamaters.Add("Operation", BrowseLibraryOperation.GetClasses);
                            _operationParamaters.Add("TreeNode", e.Node);
                            InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingClassNames, e.Node.Text));
                            _theObjectInfoGrid.Clear();
                            _backgroundWorker.RunWorkerAsync(_operationParamaters);
                        }
                    }
                }
              
            }
        }

        /// <summary>
        /// After select event process for tree view.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _browseLibraryTreeView_AfterSelect(object sender, TreeViewEventArgs e)
        {
            ArrayList arrTrafodionObjects = new ArrayList();
            if (_browseLibraryTreeView.SelectedNode is CatalogsFolder)
            {
                HyperlinkColIndex = 0; // first column
                CatalogsFolder theCatalogsFolder = _browseLibraryTreeView.SelectedNode as CatalogsFolder;
                ShowObjectDataInfo(PopulateCatalogsDataTable(theCatalogsFolder.TheTrafodionSystem, ref arrTrafodionObjects), arrTrafodionObjects);
            }
            else if (_browseLibraryTreeView.SelectedNode is CatalogFolder)
            {
                HyperlinkColIndex = 0; // first column
                CatalogFolder theCatalogFolder = _browseLibraryTreeView.SelectedNode as CatalogFolder;
                ShowObjectDataInfo(PopulateSchemasDataTable(theCatalogFolder.TheTrafodionCatalog, ref arrTrafodionObjects), arrTrafodionObjects);
            }
            else if (_browseLibraryTreeView.SelectedNode is SchemasFolder)
            {
                HyperlinkColIndex = 0; // first column
                SchemasFolder theSchemasFolder = _browseLibraryTreeView.SelectedNode as SchemasFolder;
                ShowObjectDataInfo(PopulateSchemasDataTable(theSchemasFolder.TheTrafodionCatalog, ref arrTrafodionObjects), arrTrafodionObjects);
            }
            else if (_browseLibraryTreeView.SelectedNode is SchemaFolder)
            {
                HyperlinkColIndex = 0; // first column
                SchemaFolder theSchemaFolder = _browseLibraryTreeView.SelectedNode as SchemaFolder;
                ShowObjectDataInfo(PopulateLibrariesDataTable(theSchemaFolder.TheTrafodionSchema, ref arrTrafodionObjects), arrTrafodionObjects);
            }
            else if (_browseLibraryTreeView.SelectedNode is LibrariesFolder)
            {
                HyperlinkColIndex = 0; // first column
                LibrariesFolder theLibrariesFolder = _browseLibraryTreeView.SelectedNode as LibrariesFolder;
                ShowObjectDataInfo(PopulateLibrariesDataTable(theLibrariesFolder.TheTrafodionSchema, ref arrTrafodionObjects), arrTrafodionObjects);
            }
            else if (_browseLibraryTreeView.SelectedNode is LibraryFolder)
            {
                this._browseLibraryTreeView.Cursor = Cursors.WaitCursor;
                HyperlinkColIndex = 0; // first column

                if (((LibraryFolder)e.Node).TheTrafodionLibrary.TheJarClasses == null)
                {
                    if (!_backgroundWorker.IsBusy)
                    {
                        //Get the list of class names
                        _operationParamaters.Clear();
                        _operationParamaters.Add("Operation", BrowseLibraryOperation.GetClasses);
                        _operationParamaters.Add("TreeNode", e.Node);
                        InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingClassNames, e.Node.Text));
                        _theObjectInfoGrid.Clear();
                        _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    }
                }
                else
                {
                    //If the selected node is the jar tree node, display the class details in the grid on the right pane
                    ((LibraryFolder)e.Node).IsPopulated = false;
                    ((LibraryFolder)e.Node).DoPopulate(null);
                    ShowObjectDataInfo(PopulateJarClassDataTable(((LibraryFolder)e.Node).TheTrafodionLibrary.TheJarClasses,
                        ref arrTrafodionObjects), arrTrafodionObjects);
                    this._browseLibraryTreeView.Cursor = Cursors.Default;
                }
            }
            else if (_browseLibraryTreeView.SelectedNode is ClassLeaf)
            {
                this._browseLibraryTreeView.Cursor = Cursors.WaitCursor;
                HyperlinkColIndex = -1; // no hyperlink column

                if (((ClassLeaf)e.Node).Methods == null)
                {
                    if (!_backgroundWorker.IsBusy)
                    {
                        //Get the list of class names
                        _operationParamaters.Clear();
                        _operationParamaters.Add("Operation", BrowseLibraryOperation.GetMethods);
                        _operationParamaters.Add("TreeNode", e.Node);
                        InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingMethods, e.Node.Text));
                        _theObjectInfoGrid.Clear();
                        _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    }
                }
                else
                {
                    ShowObjectDataInfo(((ClassLeaf)e.Node).Methods, null);
                    this._browseLibraryTreeView.Cursor = Cursors.Default;
                }
            }
            UpdateControls();
        }

        /// <summary>
        /// Handle the hyperlink selection on the grid
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ObjectInfoGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            //If background worker is busy, no-op the hyperlink selection
            if (_backgroundWorker.IsBusy)
            {
                return;
            }

            // Handle hyperline here
            if (e.ColIndex == HyperlinkColIndex)
            {
                TrafodionObject theTrafodionObject = null;
                if (_theObjectInfoGrid.Rows[e.RowIndex].Tag is TrafodionObject)
                {
                    theTrafodionObject = (TrafodionObject)_theObjectInfoGrid.Rows[e.RowIndex].Tag;
                }

                if (theTrafodionObject != null)
                {
                    _browseLibraryTreeView.SelectTrafodionObject(theTrafodionObject);
                }
            }
        }
        /// <summary>
        /// update button status while selection changes in the igrid.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theObjectInfoGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (_browseLibraryTreeView.SelectedNode is SchemaFolder ||
                _browseLibraryTreeView.SelectedNode is LibrariesFolder)
            {
                if (_theObjectInfoGrid.SelectedRowIndexes.Count > 0)
                {
                    _theDownloadButton.Enabled = true;
                }
                else
                {
                    _theDownloadButton.Enabled = false;
                }
            }
            else if (_browseLibraryTreeView.SelectedNode is ClassLeaf)
            {
                if (_theObjectInfoGrid.SelectedRowIndexes.Count > 0)
                {
                    _theOKButton.Enabled = true;
                }
                else
                {
                    _theOKButton.Enabled = false;
                }
            }
        }

        /// <summary>
        /// Popup create library dialog to create library.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCreateButton_Click(object sender, EventArgs e)
        {
            TrafodionSchema selectedSchemaObject = null;
            if (_browseLibraryTreeView.SelectedNode is SchemaFolder)
            {
                selectedSchemaObject = ((SchemaFolder)_browseLibraryTreeView.SelectedNode).TheTrafodionSchema;
            }
            else if (_browseLibraryTreeView.SelectedNode is LibrariesFolder)
            {
                selectedSchemaObject = ((LibrariesFolder)_browseLibraryTreeView.SelectedNode).TheTrafodionSchema;
            }
            else if (_browseLibraryTreeView.SelectedNode is LibraryFolder)
            {
                selectedSchemaObject = ((LibraryFolder)_browseLibraryTreeView.SelectedNode).TheTrafodionLibrary.TheTrafodionSchema;
            }
            if (selectedSchemaObject != null)
            {
                CreateLibraryUserControl cld = new CreateLibraryUserControl(selectedSchemaObject);
                cld.ShowDialog();
            }
        }

        /// <summary>
        /// Drop library 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theDropButton_Click(object sender, EventArgs e)
        {
            TrafodionLibrary library = null;
            if (_browseLibraryTreeView.SelectedNode is LibraryFolder)
            {
                library = ((LibraryFolder)_browseLibraryTreeView.SelectedNode).TheTrafodionLibrary;
            }
            if (library != null)
            {
                try
                {
                    DropConfirmDialog dlg = new DropConfirmDialog(Properties.Resources.DropLibrary,
                        string.Format(Properties.Resources.DropLibraryConfirm, library.ExternalName),
                        Trafodion.Manager.Properties.Resources.Question,
                        Properties.Resources.DropLibraryOptionDescription, false);
                    if (dlg.ShowDialog() == DialogResult.Yes)
                    {
                        library.Drop(dlg.OptionValue);
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), ex.Message,
                        Properties.Resources.DropLibraryError, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

        }

        /// <summary>
        /// refresh treeview
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theRefreshGuiButton_Click(object sender, EventArgs e)
        {
            //_browseLibraryTreeView.Refresh();
            _theObjectInfoGrid.Clear();
            if (_browseLibraryTreeView.SelectedNode is LibraryFolder)
            {
                //Get the list of class names
                _operationParamaters.Clear();
                _operationParamaters.Add("Operation", BrowseLibraryOperation.GetClasses);
                _operationParamaters.Add("TreeNode", _browseLibraryTreeView.SelectedNode);
                InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingClassNames, 
                    _browseLibraryTreeView.SelectedNode.Text));
                _theObjectInfoGrid.Clear();
                _backgroundWorker.RunWorkerAsync(_operationParamaters);
            }
            else if (_browseLibraryTreeView.SelectedNode is ClassLeaf)
            {
                //Get the list of class names
                _operationParamaters.Clear();
                _operationParamaters.Add("Operation", BrowseLibraryOperation.GetMethods);
                _operationParamaters.Add("TreeNode", _browseLibraryTreeView.SelectedNode);
                InitializeStatusStrip(String.Format(Properties.Resources.StatusFetchingMethods, 
                    _browseLibraryTreeView.SelectedNode.Text));
                _theObjectInfoGrid.Clear();
                _backgroundWorker.RunWorkerAsync(_operationParamaters);
            }
            else
            {
                ((NavigationTreeNode)_browseLibraryTreeView.SelectedNode).RefreshNode();
            }
            
        }

        /// <summary>
        /// Select a method of class to create a procedure.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theOKButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                Form parent = (Form)Parent;
                parent.DialogResult = DialogResult.OK;
                parent.Close();
            }
        }

        /// <summary>
        /// Close dialog.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theCloseButton_Click(object sender, EventArgs e)
        {
            _theCloseButton.Enabled = false;
            //If the background worker is not busy, the button text is "Close"
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
            
        }

        /// <summary>
        /// Download jar files of selected library.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theDownloadButton_Click(object sender, EventArgs e)
        {
            //If background worker is busy, no-op the hyperlink selection
            if (_backgroundWorker.IsBusy)
            {
                return;
            }

            List<TrafodionLibrary> listLibraries = new List<TrafodionLibrary>();
            if (_browseLibraryTreeView.SelectedNode is LibrariesFolder ||
                _browseLibraryTreeView.SelectedNode is SchemaFolder)
            {
                if (_theObjectInfoGrid.SelectedRowIndexes.Count > 0)
                {
                    foreach (iGRow row in _theObjectInfoGrid.SelectedRows)
                    {
                        listLibraries.Add((TrafodionLibrary)row.Tag);
                    }
                }
            }            
            else if (_browseLibraryTreeView.SelectedNode is LibraryFolder)
            {
                listLibraries.Add(((LibraryFolder)_browseLibraryTreeView.SelectedNode).TheTrafodionLibrary);
            }
            else if (_browseLibraryTreeView.SelectedNode is ClassLeaf)
            {
                listLibraries.Add(((ClassLeaf)_browseLibraryTreeView.SelectedNode).TheTrafodionLibrary);
            }
           
            
            if (listLibraries.Count > 0)
            {
                //Display a folder browser dialog to let the user select the target folder.
                FolderBrowserDialog folderDialog = new FolderBrowserDialog();
                folderDialog.Description = Properties.Resources.SelectTargetFolderForJars;
                folderDialog.SelectedPath = Trafodion.Manager.Framework.Utilities.FileDialogLocation();

                if (folderDialog.ShowDialog() == DialogResult.OK)
                {
                    string targetDirectory = folderDialog.SelectedPath;

                    //Save the file location for future reference
                    Trafodion.Manager.Framework.Utilities.FileDialogLocation(targetDirectory);

                    List<TrafodionLibrary> existingFilesLib = new List<TrafodionLibrary>();
                    List<TrafodionLibrary> newFilesLib = new List<TrafodionLibrary>();
                    foreach (TrafodionLibrary lib in listLibraries)
                    {
                        if (File.Exists(Path.Combine(targetDirectory, lib.ClientCodeFileName)))
                        {
                            existingFilesLib.Add(lib);
                        }
                        else
                        {
                            newFilesLib.Add(lib);
                        }

                    }

                   //Download the files
                    _operationParamaters.Clear();
                    _operationParamaters.Add("Operation", BrowseLibraryOperation.Download);
                    _operationParamaters.Add("FolderName", targetDirectory);

                    //If one or more files exist, prompt user for overwrite confirmation
                    if (existingFilesLib.Count > 0)
                    {
                        string message = String.Format(Properties.Resources.PCF_FILE_DOWNLOAD_OVERWRITE_MESSAGE, targetDirectory) + "\n";
                        int nCount = 0;
                        foreach (TrafodionLibrary lib in existingFilesLib)
                        {
                            message += "\n" + lib.ClientCodeFileName;
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
                            _operationParamaters.Add("Libraries", listLibraries);
                        }
                        else if (result == DialogResult.No)
                        {
                            //If user declined overwrite, skip the files that exist and only send the files that do not exist already
                            _operationParamaters.Add("Libraries", newFilesLib);
                        }
                        else
                        {
                            //If user decided to abort, cancel the upload and return
                            return;
                        }
                    }
                    else
                    {
                        _operationParamaters.Add("Libraries", listLibraries);
                    }

                    //Inititate the download as a background operation
                    InitializeStatusStrip(String.Format(Properties.Resources.StatusDownloading, ""));
                    _backgroundWorker.RunWorkerAsync(_operationParamaters);
                    _theDownloadButton.Enabled = false;
                    _theCloseButton.Enabled = true;
                }
            }            
        }

        /// <summary>
        /// Popup context help topics.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.UseLibBrowserDialog);
        }

   
        #endregion Control Events

        #region Private methods
        /// <summary>
        /// set tool tips for buttons(Create,)
        /// </summary>
        private void SetToolTipsForButtons()
        {
            ToolTip tooltip = new ToolTip();

            // Force the ToolTip text to be displayed whether or not the form is active.
            tooltip.ShowAlways = true;
            
            // Set up the ToolTip text for the Button and Checkbox.
            tooltip.SetToolTip(this._theCreateButton, Properties.Resources.ToolTipsCreateButton);
            tooltip.SetToolTip(this._theDropButton, Properties.Resources.ToolTipsDropButton);
            tooltip.SetToolTip(this._theDownloadButton, Properties.Resources.ToolTipsDownloadButton);
            tooltip.SetToolTip(this._theRefreshGuiButton, Properties.Resources.ToolTipsRefreshButton);
            tooltip.SetToolTip(this._theOKButton, Properties.Resources.ToolTipsOKButton);
            tooltip.SetToolTip(this._theCloseButton, Properties.Resources.ToolTipsCloseButton);

        }

        /// <summary>
        /// Downloads a given file to the specified target directory
        /// </summary>
        /// <param name="fileName"></param>
        /// <param name="targetDirectory"></param>
        private void DownloadFile(TrafodionLibrary library,String targetDirectory, BackgroundWorker worker, DoWorkEventArgs e)
        {
            if (!string.IsNullOrEmpty(library.CodeFileName))
            {
                string content = null;
                int count = 0, totalCount = 0;
                byte[] data = new byte[10240];

                FileInfo f = new FileInfo(Path.Combine(targetDirectory, library.ClientCodeFileName));
                BinaryWriter br = new BinaryWriter(f.OpenWrite());
                try
                {
                    //Read 10K chunks of base64 bytes and convert them to binary
                    //and write to target file
                    do
                    {
                        //The count is the actual count rather than the length of the content
                        library.getJarFile(library.CodeFileName, totalCount, out content, out count, ref _libraryWorkingCommandObject);

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
        /// update status of background worker.
        /// </summary>
        /// <param name="statusMessage"></param>
        private void ReportStatusStrip(String statusMessage)
        {
            _statusLabel.Text = statusMessage;
            _progressBar.Visible = !string.IsNullOrEmpty(statusMessage);
        }

        /// <summary>
        /// If the user control is hosted within a Form, closes the form
        /// </summary>
        private void Close()
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        /// <summary>
        /// Populate all catalogs to a datatable used for display objects in the right panel.
        /// </summary>
        /// <param name="aTrafodionSystem"></param>
        /// <param name="arrTrafodionObjects"></param>
        /// <returns></returns>
        private DataTable PopulateCatalogsDataTable(TrafodionSystem aTrafodionSystem,ref ArrayList arrTrafodionObjects)
        {
            DataRow row = null;
            DataTable dataTable = new DataTable();            
            dataTable.Columns.Add(Properties.Resources.Name, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.Location, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MetadataUID, typeof(System.Int64));
            foreach (TrafodionCatalog theTrafodionCatalog in aTrafodionSystem.TrafodionCatalogs)
            {              
                row = dataTable.NewRow();
                row[Properties.Resources.Name] = theTrafodionCatalog.ExternalName;
                row[Properties.Resources.Location] = theTrafodionCatalog.VolumeName;
                row[Properties.Resources.MetadataUID] = theTrafodionCatalog.UID;                
                dataTable.Rows.Add(row);
                arrTrafodionObjects.Add(theTrafodionCatalog);
            }

            return dataTable;
        }


        /// <summary>
        /// Populate all Schemas of a catalog to a datatable used for display objects in the right panel.
        /// </summary>
        /// <param name="aTrafodionCatalog"></param>
        /// <param name="arrTrafodionObjects"></param>
        /// <returns></returns>
        private DataTable PopulateSchemasDataTable(TrafodionCatalog aTrafodionCatalog, ref ArrayList arrTrafodionObjects)
        {
            DataRow row = null;
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(Properties.Resources.Name, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.Owner, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.Version, typeof(System.Int32));
            dataTable.Columns.Add(Properties.Resources.Location, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MetadataUID, typeof(System.Int64));
            foreach (TrafodionSchema theTrafodionSchema in aTrafodionCatalog.TrafodionSchemas)
            {
                row = dataTable.NewRow();
                row[Properties.Resources.Name] = theTrafodionSchema.ExternalName;
                row[Properties.Resources.Owner] = theTrafodionSchema.OwnerName;
                row[Properties.Resources.Version] = theTrafodionSchema.Version;
                row[Properties.Resources.Location] = theTrafodionSchema.Location;
                row[Properties.Resources.MetadataUID] = theTrafodionSchema.UID;
                dataTable.Rows.Add(row);
                arrTrafodionObjects.Add(theTrafodionSchema);
            }

            return dataTable;
        }

        /// <summary>
        /// Populate all Libraries of a schema to a datatable used for display objects in the right panel.
        /// </summary>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="arrTrafodionObjects"></param>
        /// <returns></returns>
        private DataTable PopulateLibrariesDataTable(TrafodionSchema aTrafodionSchema, ref ArrayList arrTrafodionObjects)
        {
            DataRow row = null;
            DataTable dataTable = new DataTable();
            dataTable.Columns.Add(Properties.Resources.Library, typeof(System.String));
            dataTable.Columns.Add(Properties.Resources.MetadataUID, typeof(System.Int64));
            dataTable.Columns.Add(Properties.Resources.CreationTime, typeof(Framework.JulianTimestamp));
            dataTable.Columns.Add(Properties.Resources.RedefinitionTime, typeof(Framework.JulianTimestamp));
           
            foreach (TrafodionLibrary theTrafodionLibrary in aTrafodionSchema.TrafodionLibraries)
            {
                row = dataTable.NewRow();
                row[Properties.Resources.Library] = theTrafodionLibrary.ExternalName;
                row[Properties.Resources.MetadataUID] = theTrafodionLibrary.UID;
                row[Properties.Resources.CreationTime] = theTrafodionLibrary.FormattedCreateTime();
                row[Properties.Resources.RedefinitionTime] = theTrafodionLibrary.FormattedRedefTime();
                dataTable.Rows.Add(row);
                arrTrafodionObjects.Add(theTrafodionLibrary);
            }

            return dataTable;
        }
        /// <summary>
        /// Populate all classes of a Jar file to a datatable used for display objects in the right panel.
        /// </summary>
        /// <returns></returns>
        private DataTable PopulateJarClassDataTable(List<JarClass> listObjects, ref ArrayList arrTrafodionObjects)
        {
            DataRow row = null;
            // Now, define column definitions for datatable.
            DataTable dataTable = new DataTable();

            dataTable.Columns.Add(Properties.Resources.JarClassName, typeof(System.String));

            try
            {
                foreach (JarClass obj in listObjects)
                {
                    row = dataTable.NewRow();
                    row[Properties.Resources.JarClassName] = obj.ClassName;
                    dataTable.Rows.Add(row);
                    arrTrafodionObjects.Add(obj);
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
        /// Display objects in the right panel and set igrid style.
        /// </summary>
        /// <param name="aInfoTable"></param>
        /// <param name="arrTrafodionObject"></param>
        private void ShowObjectDataInfo(DataTable aInfoTable,ArrayList arrTrafodionObject)
        {           
            _theObjectInfoGrid.BeginUpdate();

            _theObjectInfoGrid.Clear();
            _theObjectInfoGrid.FillWithData(aInfoTable);
            _theObjectInfoGrid.AutoResizeCols = true;
            _theObjectInfoGrid.RowMode = true;
            _theObjectInfoGrid.ReadOnly = true;

            for (int i = 0; i < _theObjectInfoGrid.Rows.Count; i++)
            {
                if (arrTrafodionObject!=null && arrTrafodionObject[i]!=null)
                {
                    _theObjectInfoGrid.Rows[i].Tag = arrTrafodionObject[i];
                }
            }

            foreach (iGCol col in _theObjectInfoGrid.Cols)
            {
                col.CellStyle.TextAlign = iGContentAlignment.MiddleLeft;
            }

            _theObjectInfoGrid.EndUpdate();
            _hyperLinkCellManager.Attach(_theObjectInfoGrid, HyperlinkColIndex);
     
             // It's better to call this method after you attached iGHyperlinkCellManager to the grid
             // because the manager can change the formatting of the links dynamically
             _theObjectInfoGrid.Cols.AutoWidth();
             // allow user to select multiple code files for deletion
             _theObjectInfoGrid.SelectionMode = TenTec.Windows.iGridLib.iGSelectionMode.MultiExtended;
        }
   

     
        /// <summary>
        /// update buttons status.
        /// </summary>
        private void UpdateControls()
        {
            _theCloseButton.Text = "&Close";
            _theCloseButton.Enabled = true;
            _theOKButton.Enabled = false;
            _theCreateButton.Enabled = false;
            _theDropButton.Enabled = false;
            _theDownloadButton.Enabled = false;
            _theRefreshGuiButton.Enabled = true;

            if (_browseLibraryTreeView.SelectedNode is SchemaFolder)
            {
                _theCreateButton.Enabled = true;
            }
            else if (_browseLibraryTreeView.SelectedNode is LibrariesFolder)
            {               
                _theCreateButton.Enabled = true;
            }
            else if (_browseLibraryTreeView.SelectedNode is LibraryFolder)
            {
                _theDropButton.Enabled = true;
                _theDownloadButton.Enabled = true;
                _theCreateButton.Enabled = true;
            }

            if (_browseLibraryTreeView.SelectedNode is ClassLeaf)
            {
                if (_theOKButton.Visible &&
                    _theObjectInfoGrid.Rows.Count > 0 &&
                    _theObjectInfoGrid.SelectedRowIndexes.Count > 0)
                {
                    _theOKButton.Enabled = true;
                }
            }
            //Maybe user has Create_Procedure but doesn't have Create_library privilege,disable drop/Create buttons.
            if (!_connectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS", "CREATE_LIBRARY"))
            {
                _theDropButton.Enabled = false;
                _theCreateButton.Enabled = false;
            }
        }

        /// <summary>
        /// Invoked before starting a background operation
        /// Sets the appropriate status label and enables progress bar
        /// </summary>
        /// <param name="statusMessage"></param>
        private void InitializeStatusStrip(String statusMessage)
        {
            _theCloseButton.Text = "&Cancel";
            _statusLabel.Text = statusMessage;
            if (!_parentClosing)
            {
                _progressBar.Visible = true;
            }
            _theCreateButton.Enabled = false;
            _theDropButton.Enabled = false;
            isOperationCancelled = false;
            _theRefreshGuiButton.Enabled = false;
        }

        /// <summary>
        /// Refreshes the method details for a given class
        /// </summary>
        /// <param name="aClassLeaf"></param>
        private void RefreshMethods(JarClass aJarClass)
        {
            //if (aJarClass.MethodList == null)
            //{
                aJarClass.getMethods(aJarClass.ClassName, _dataSet);
            //}
        }

        /// <summary>
        /// Delete file that hasn't been downloaded successfully.
        /// </summary>
        private void DeleteDownloadedFiles()
        {
            List<TrafodionLibrary> librariesList = (List<TrafodionLibrary>)_operationParamaters["Libraries"];
            string targetDirectory = (string)_operationParamaters["FolderName"];
            System.Text.StringBuilder errorMessage = new System.Text.StringBuilder();
            foreach (TrafodionLibrary lib in librariesList)
            {
                try
                {
                    if (File.Exists(Path.Combine(targetDirectory, lib.ClientCodeFileName)))
                    {
                        File.Delete(Path.Combine(targetDirectory, lib.ClientCodeFileName));
                    }
                }
                catch (Exception ex)
                {
                    errorMessage.Append("Delete file " + lib.ClientCodeFileName + " failed./n");
                }
            }
            string message = "";
            if (errorMessage.Length > 0)
            {
                message = Properties.Resources.DeleteDownloadedFileFailMsg;
            }
            else
            {
                message = Properties.Resources.DeleteDownloadedFileOKMsg;
            }

            MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), message, Properties.Resources.Info,
                MessageBoxButtons.OK, MessageBoxIcon.Information);
        }

        /// <summary>
        /// Refreshes the list of class name for a given code file
        /// </summary>
        /// <param name="aCodeFile"></param>
        private void RefreshClassNames(TrafodionLibrary aTrafodionLibrary)
        {
            aTrafodionLibrary.GetClasses(_dataSet);
        }

        #endregion

        #region Background worker methods

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
                HandleBrowseLibOperations((Hashtable)e.Argument, worker, e);
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
        private void HandleBrowseLibOperations(Hashtable operationParameters, BackgroundWorker worker, DoWorkEventArgs e)
        {
            BrowseLibraryOperation operation = (BrowseLibraryOperation)operationParameters["Operation"];

            switch (operation)
            {
                case BrowseLibraryOperation.GetClasses:
                    {
                        LibraryFolder libraryFolder = (LibraryFolder)operationParameters["TreeNode"];
                        //Get list of class names for the code file
                        RefreshClassNames(libraryFolder.TheTrafodionLibrary);
                        break;
                    }
                case BrowseLibraryOperation.GetMethods:
                    {
                        ClassLeaf classLeaf = (ClassLeaf)operationParameters["TreeNode"];
                        RefreshMethods(classLeaf.TheJarClass);
                        break;
                    }

                case BrowseLibraryOperation.Download:
                    {
                        List<TrafodionLibrary> listLibraries = (List<TrafodionLibrary>)operationParameters["Libraries"];
                        string targetDirectory = (string)operationParameters["FolderName"];
                        System.Text.StringBuilder errorMessageBuilder = new System.Text.StringBuilder();

                        //Download each file in the parameter list
                        foreach (TrafodionLibrary library in listLibraries)
                        {
                            //If user has request cancel, abort further work
                            if (worker.CancellationPending)
                            {
                                e.Cancel = true;
                                return;
                            }
                            //Displays the status message for the current file
                            worker.ReportProgress(0, String.Format(Properties.Resources.StatusDownloading, library.ClientCodeFileName));

                            try
                            {
                                DownloadFile(library, targetDirectory, worker, e);
                            }
                            catch (Exception ex)
                            {
                                //Since multi-file download is allowed, capture individual file download failures and report one exception
                                //at the end
                                errorMessageBuilder.AppendFormat("{0}{1}{2}\n", library.ClientCodeFileName, TRAFMGR_MESSAGE_SEPARATOR, ex.Message);
                            }
                        }
                        if (!worker.CancellationPending && errorMessageBuilder.ToString().Length > 0)
                        {
                            throw new Exception(errorMessageBuilder.ToString());
                        }
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
            if (this.IsDisposed) return;
            BrowseLibraryOperation operation = (BrowseLibraryOperation)this._operationParamaters["Operation"];
            _progressBar.Visible = false;
            _statusLabel.Text = string.Empty;
            this._browseLibraryTreeView.Cursor = Cursors.Default;
            this._theCloseButton.Cursor = Cursors.Default;
            UpdateControls();

            //If there was error in the background operation, display the error
            if (e.Error != null)
            {
                #region Display the Error  
                string operationName = System.Enum.GetName(typeof(BrowseLibraryOperation), operation);

                switch (operation)
                {
                    case BrowseLibraryOperation.Download:
                        {
                            string[] errorMessages = e.Error.Message.Split(new string[] { "\n" }, StringSplitOptions.RemoveEmptyEntries);
                            DataTable errorTable = new DataTable();
                            errorTable.Columns.Add("File Name");
                            errorTable.Columns.Add("Error Text");
                            foreach (string message in errorMessages)
                            {
                                string[] tokens = message.Split(new string[] { TRAFMGR_MESSAGE_SEPARATOR }, StringSplitOptions.RemoveEmptyEntries);
                                if (tokens.Length > 1)
                                {
                                    errorTable.Rows.Add(new object[] { tokens[0], tokens[1] });
                                }
                            }
                            string summaryMessage = String.Format("{0} failed for one or more file(s).", operationName);
                            TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(summaryMessage, errorTable, System.Drawing.SystemIcons.Error);
                            md.ShowDialog();
                            DeleteDownloadedFiles();
                            break;
                        }

                    case BrowseLibraryOperation.GetClasses:
                        {
                            MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(),
                                            String.Format(Properties.Resources.PCFOperationFailed, operationName) + "\n\n" + e.Error.Message,
                                            Properties.Resources.Error, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

                #endregion
            }      
            else
            {
                #region All Operations finished successfully
                switch (operation)
                {
                    
                    case BrowseLibraryOperation.GetClasses:
                        {
                            ArrayList arrTrafodionObjects = new ArrayList();
                            LibraryFolder libraryFolder = (LibraryFolder)_operationParamaters["TreeNode"];
                            libraryFolder.DoPopulate(null);
                            _browseLibraryTreeView.SelectedNode = libraryFolder;
                            ShowObjectDataInfo(PopulateJarClassDataTable(libraryFolder.TheTrafodionLibrary.TheJarClasses,
                                ref arrTrafodionObjects), arrTrafodionObjects);
                            libraryFolder.Expand();
                            break;
                        }
                    case BrowseLibraryOperation.GetMethods:
                        {
                            ClassLeaf classLeaf = (ClassLeaf)_operationParamaters["TreeNode"];
                            DataTable table = classLeaf.Methods;
                            if (table != null)
                            {
                                ShowObjectDataInfo(classLeaf.Methods, null); 
                            }
                            _browseLibraryTreeView.SelectedNode = classLeaf;

                            break;
                        }
                   
                    case BrowseLibraryOperation.Download:
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
 
                    default:
                        {
                            break;
                        }
                }
                #endregion
            }
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
                if (_libraryWorkingCommandObject != null)
                {
                    try
                    {
                        _libraryWorkingCommandObject.Cancel();
                    }
                    catch (Exception)
                    {
                    };
                }
            }
        }
    
        #endregion Background worker methods
    }
}
