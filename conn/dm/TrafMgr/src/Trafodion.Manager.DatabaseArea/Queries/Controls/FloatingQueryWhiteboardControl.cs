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
using System.Data;
using System.IO;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// the class for SQL Whiteboard main control
    /// </summary>
    public partial class FloatingQueryWhiteboardControl : UserControl, IMenuProvider, IMainToolBarConsumer //, ICloneToWindow
    {
        private static readonly string SQLWhiteboardPersistenceKey = "SQLWhiteboard";

        private QueryListUserControl _theQueryListUserControl;
        private QueryUserControl _theQueryUserControl;
        private QueryDetailsUserControl _theQueryDetailsUserControl;
        private DatabaseTreeView _theDatabaseTreeView;
        private NavigationTreeView.SelectedHandler _theDatabaseTreeViewSelectedHandler = null;
        private System.Drawing.Size _theCurrentSize;
        private Trafodion.Manager.Framework.MainToolBar _theMainToolBar = null;

        /// <summary>
        /// The default constructor.
        /// </summary>
        public FloatingQueryWhiteboardControl()
            : this(null)
        {
        }

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="aDatabaseTreeView"></param>
        public FloatingQueryWhiteboardControl(DatabaseTreeView aDatabaseTreeView)
        {
            _theDatabaseTreeView = aDatabaseTreeView;
            InitializeComponent();
            widgetCanvas.ThePersistenceKey = SQLWhiteboardPersistenceKey;

            this._theCurrentSize = this.Size;
            int myWidth = this.Size.Width;
            int myHeight = this.Size.Height;

            // Start to create canvas and widgets.
            GridLayoutManager gridLayoutManager = new GridLayoutManager(5, 2);
            gridLayoutManager.CellSpacing = 4;
            widgetCanvas.LayoutManager = gridLayoutManager;

            // Create Statement list widget.
            GridConstraint gridConstraint = new GridConstraint(0, 0, 3, 1);
            _theQueryListUserControl = new QueryListUserControl();
            WidgetContainer widgetContainer = new WidgetContainer(widgetCanvas, _theQueryListUserControl, "Statement List");
            widgetContainer.Name = "Statement List";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // Create Statement widget.
            gridConstraint = new GridConstraint(0, 1, 3, 1);
            _theQueryUserControl = new QueryUserControl();
            widgetContainer = new WidgetContainer(widgetCanvas, _theQueryUserControl, "Statement");
            widgetContainer.Name = "Statement";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // Create Statement details widget.
            gridConstraint = new GridConstraint(3, 0, 2, 2);
            _theQueryDetailsUserControl = new QueryDetailsUserControl();
            widgetContainer = new WidgetContainer(widgetCanvas, _theQueryDetailsUserControl, "Statement Details");
            widgetContainer.Name = "Statement Details";
            widgetContainer.AllowDelete = false;
            this.widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            _theQueryUserControl.TheQueryListUserControl = _theQueryListUserControl;
            _theQueryUserControl.TheQueryDetailsUserControl = _theQueryDetailsUserControl;
            _theQueryUserControl.OnMySystemChanged += TheQueryUserControl_OnMySystemChanged;

            if (_theDatabaseTreeView != null)
            {
                _theQueryUserControl.TheDatabaseTreeView = _theDatabaseTreeView;
                _theDatabaseTreeViewSelectedHandler = new NavigationTreeView.SelectedHandler(TheDatabaseTreeViewSelected);
                _theDatabaseTreeView.Selected += _theDatabaseTreeViewSelectedHandler;
                NodeActivated(_theDatabaseTreeView.SelectedNode);
            }

            Persistence.PersistenceHandlers += PersistencePersistenceHandlers;

            this.widgetCanvas.InitializeCanvas();
        }

        /// <summary>
        /// To reset the SQL whiteboard layout
        /// </summary>
        public void ResetLayout()
        {
            this.widgetCanvas.ResetWidgetLayout();
        }

        /// <summary>
        /// To load a list of SQL statements into the SQL Whiteboard's Statement Grid.
        /// </summary>
        /// <param name="aListOfStatements">the list of statements</param>
        /// <param name="from">From where the statements come</param>
        public void LoadStatements(ArrayList aListOfStatements, string from)
        {
            _theQueryListUserControl.LoadStatements(aListOfStatements, from);
        }

        #region Properties 

        /// <summary>
        /// The selected connection
        /// </summary>
        public ConnectionDefinition TheSelectedConnectionDefinition
        {
            get { return _theQueryUserControl.TheSelectedConnectionDefinition; }
            set { _theQueryUserControl.TheSelectedConnectionDefinition = value; }
        }

        /// <summary>
        /// The selected catalog
        /// </summary>
        public TrafodionCatalog TheSelectedTrafodionCatalog
        {
            get { return _theQueryUserControl.TheSelectedTrafodionCatalog; }
            set { _theQueryUserControl.TheSelectedTrafodionCatalog = value; }
        }

        /// <summary>
        /// The selected schema 
        /// </summary>
        public TrafodionSchema TheSelectedTrafodionSchema
        {
            get { return _theQueryUserControl.TheSelectedTrafodionSchema; }
            set { _theQueryUserControl.TheSelectedTrafodionSchema = value; }
        }


        #endregion /* end of Properties region */


        #region ICloneToWindow Members

        public Control Clone()
        {
            FloatingQueryWhiteboardControl theCloneQueryWhiteboardUserControl = new FloatingQueryWhiteboardControl();
            theCloneQueryWhiteboardUserControl.TheSelectedConnectionDefinition = TheSelectedConnectionDefinition;
            theCloneQueryWhiteboardUserControl.TheSelectedTrafodionCatalog = TheSelectedTrafodionCatalog;
            theCloneQueryWhiteboardUserControl.TheSelectedTrafodionSchema = TheSelectedTrafodionSchema;
            return theCloneQueryWhiteboardUserControl;
        }

        public string WindowTitle
        {
            get { return "SQL Whiteboard"; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheSelectedConnectionDefinition; }

        }
        
        #endregion

        #region IMainToolBarConsumer implementation 

        /// <summary>
        /// Implementating the IMainToolBarConsumer interface, which the consumer could elect buttons to show and modify 
        /// the Help button to invoke context sensitive help topic.
        /// </summary>
        /// <param name="aMainToolBar"></param>
        public void CustomizeMainToolBarItems(Trafodion.Manager.Framework.MainToolBar aMainToolBar)
        {
            // Now, turn on all of the tool strip buttons for Whiteboard.
            _theMainToolBar = aMainToolBar;
            aMainToolBar.TheSystemToolToolStripItem.Visible = true;
            aMainToolBar.TheSystemsToolStripSeparator.Visible = true;
            aMainToolBar.TheNCIToolStripItem.Visible = true;
            aMainToolBar.TheMetricMinerToolStripItem.Visible = true;
            aMainToolBar.TheOptionsToolStripItem.Visible = true;
            aMainToolBar.TheEventViewerToolStripItem.Visible = false;
            aMainToolBar.TheToolsStripSeparator.Visible = true;
            aMainToolBar.TheWindowManagerToolStripItem.Visible = true;
            aMainToolBar.TheWindowManagerStripSeparator.Visible = true;
            aMainToolBar.TheHelpToolStripItem.Visible = true;
            aMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
            aMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
            aMainToolBar.TheUpdateConfigurationToolStripItem.Visible = false;
            aMainToolBar.TheRunScriptToolStripItem.Visible = false;

            ///Customize the help topic if it is desired.
            aMainToolBar.UnRegisterDefaultHelpEventHandler();
            aMainToolBar.TheHelpToolStripItem.Alignment = ToolStripItemAlignment.Right;
            aMainToolBar.TheHelpToolStripItem.Click += new EventHandler(TheHelpToolStripItem_Click);

            //Enable/Disable envent view/audit log viewer/OSIM download first
            if (_theQueryUserControl != null &&
                _theQueryUserControl.TheSelectedConnectionDefinition != null)
            {
                aMainToolBar.TheEventViewerToolStripItem.Enabled = true;
                this.SetToolStripItemStatus(_theQueryUserControl.TheSelectedConnectionDefinition);
            }
            else
            {
                aMainToolBar.TheEventViewerToolStripItem.Enabled = false;
                aMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
                aMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
                aMainToolBar.TheUpdateConfigurationToolStripItem.Visible = false;
                aMainToolBar.TheRunScriptToolStripItem.Visible = false;				
            }

            if (TrafodionContext.Instance.isCommunityEdition)
            {
                aMainToolBar.TheEventViewerToolStripItem.Visible = false;
                aMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
                aMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;			
                aMainToolBar.TheUpdateConfigurationToolStripItem.Visible = false;
                aMainToolBar.TheRunScriptToolStripItem.Visible = false;
            }            
        }

        /// <summary>
        /// The event handler for the context sensitive 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheHelpToolStripItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SQLWhiteboard);
        }

        #endregion 

        #region IMenuProvider implementation
        /// <summary>
        /// Implementing the IMenuProvider interface
        /// </summary>
        /// <returns></returns>
        public Trafodion.Manager.Framework.Controls.TrafodionMenuStrip GetMenuItems(ImmutableMenuStripWrapper aMenuStripWrapper)
        {
            //get the menu items from the canvas
            TrafodionToolStripMenuItem theResetLayoutMenuItem = widgetCanvas.ResetLayoutMenuItem;
            TrafodionToolStripMenuItem theLockStripMenuItem = widgetCanvas.LockMenuItem;
            System.Windows.Forms.ToolStripSeparator toolStripSeparator1 = new TrafodionToolStripSeparator();
            System.Windows.Forms.ToolStripSeparator toolStripSeparator2 = new TrafodionToolStripSeparator();

            //Obtain the index of the exit menu because we want to insert the
            //menus just above the exit menu
            int exitIndex = aMenuStripWrapper.getMenuIndex(global::Trafodion.Manager.Properties.Resources.MenuExit);

            //Set the properties of the menu items correctly
            theResetLayoutMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theResetLayoutMenuItem.MergeIndex = exitIndex;
            theLockStripMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theLockStripMenuItem.MergeIndex = exitIndex;
            toolStripSeparator1.MergeAction = System.Windows.Forms.MergeAction.Insert;
            toolStripSeparator1.MergeIndex = exitIndex;
            toolStripSeparator2.MergeAction = System.Windows.Forms.MergeAction.Insert;
            toolStripSeparator2.MergeIndex = exitIndex;

            // Add menu items to load and save sql files
            TrafodionToolStripMenuItem theSaveSqlMenuItem = new TrafodionToolStripMenuItem();
            theSaveSqlMenuItem.Text = Properties.Resources.SaveSqlFile;
            theSaveSqlMenuItem.Click += new EventHandler(theSaveSqlMenuItem_Click);
            theSaveSqlMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theSaveSqlMenuItem.MergeIndex = exitIndex;

            TrafodionToolStripMenuItem theLoadSqlMenuItem = new TrafodionToolStripMenuItem();
            theLoadSqlMenuItem.Text = Properties.Resources.LoadSqlFile;
            theLoadSqlMenuItem.Click += new EventHandler(theLoadSqlMenuItem_Click);
            theLoadSqlMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            theLoadSqlMenuItem.MergeIndex = exitIndex;

            //Create the same menu structure as we have for main
            ToolStripMenuItem fileMenuItem = new ToolStripMenuItem();
            fileMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            toolStripSeparator1,
            theResetLayoutMenuItem,
            theLockStripMenuItem,
            toolStripSeparator2,
            theSaveSqlMenuItem,
            theLoadSqlMenuItem
            });

            //Appropriately set the menu name and text for the file menu
            fileMenuItem.MergeAction = System.Windows.Forms.MergeAction.MatchOnly;
            fileMenuItem.Name = global::Trafodion.Manager.Properties.Resources.MenuFile;
            fileMenuItem.Text = global::Trafodion.Manager.Properties.Resources.MenuFile;

            //Create menu for formatting statement textbox
            TrafodionToolStripMenuItem theHighlightSyntaxMenuItem = new TrafodionToolStripMenuItem();
            theHighlightSyntaxMenuItem.Text = Properties.Resources.HighlightSyntax;
            theHighlightSyntaxMenuItem.Click += new EventHandler(theFormatMenuItem_Click);
            theHighlightSyntaxMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;

            ToolStripMenuItem formatMenuItem = new ToolStripMenuItem();
            formatMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
                theHighlightSyntaxMenuItem
            });

            // Insert it right before the tools menu
            int toolsIndex = aMenuStripWrapper.getMenuIndex(global::Trafodion.Manager.Properties.Resources.MenuTools);
            formatMenuItem.MergeIndex = toolsIndex;
            formatMenuItem.MergeAction = System.Windows.Forms.MergeAction.Insert;
            formatMenuItem.Name = global::Trafodion.Manager.Properties.Resources.MenuFormat;
            formatMenuItem.Text = global::Trafodion.Manager.Properties.Resources.MenuFormat;

            //Create the menu strip
            Trafodion.Manager.Framework.Controls.TrafodionMenuStrip menus = new Trafodion.Manager.Framework.Controls.TrafodionMenuStrip();
            menus.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
                formatMenuItem, 
                fileMenuItem});

            return menus;
        }

        void theFormatMenuItem_Click(object sender, EventArgs e)
        {
            _theQueryUserControl.TheQueryInputControl.TheQueryTextBox.SyntaxHighlightAll();
        }

        void theLoadSqlMenuItem_Click(object sender, EventArgs e)
        {
            OpenFileDialog openDialog = new OpenFileDialog();
            openDialog.InitialDirectory = Trafodion.Manager.Framework.Utilities.FileDialogLocation();
            openDialog.Title = Properties.Resources.LoadSqlTitle;
            openDialog.Filter = "Text File (*.sql;*.txt)|*.sql;*.txt|All Files (*.*)|*.*";
            openDialog.FilterIndex = 1;
            openDialog.RestoreDirectory = true;
            openDialog.Multiselect = true;
            List<ReportDefinition> reportDefinitions = new List<ReportDefinition>();
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("File Name");
            errorTable.Columns.Add("Error Text");

            if (openDialog.ShowDialog(this) == DialogResult.OK)
            {
                foreach (string FileName in openDialog.FileNames)
                {
                    if (!File.Exists(FileName))
                    {
                        continue;
                    }

                    StreamReader sr = null;
                    try
                    {
                        sr = File.OpenText(FileName);
                        string inputText = sr.ReadToEnd();
                        inputText = inputText.Replace("\r\n", "\n");
                        string name = ParseReportDefinitionName(FileName);
                        SimpleReportDefinition reportDefinition = new SimpleReportDefinition(name);
                        reportDefinition.SetProperty(ReportDefinition.DEFINITION, inputText);
                        reportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, inputText);

                        if (QueryStringsPersistence.ReportDefinitionExists(name))
                        {
                            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), 
                                    string.Format("Query - {0} already exists in the statement list. Do you want to overwrite it?", FileName), 
                                    "SQL Whiteboard Load Statement", 
                                    MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                            if (result == DialogResult.No)
                            {
                                //errorTable.Rows.Add(new object[] { FileName, "Query already exists in statement list" });
                                continue;
                            }
                        }

                        QueryStringsPersistence.Add(reportDefinition);
                        reportDefinitions.Add(reportDefinition);
                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { FileName, ex.Message});
                    }
                    finally
                    {
                        if (sr != null)
                        {
                            sr.Close();
                        }
                    }
                }
                //Save the file location for future reference
                Trafodion.Manager.Framework.Utilities.FileDialogLocation(openDialog.FileNames[0]);
                if (reportDefinitions.Count > 0)
                {
                    int rowIndex = _theQueryListUserControl.TheQueryListDataGridView.IndexOfReportDefinition(reportDefinitions[reportDefinitions.Count - 1]);
                    if (rowIndex >= 0)
                    {
                        _theQueryListUserControl.TheQueryListDataGridView.Rows[rowIndex].Selected = true;
                    }
                }

                if (errorTable.Rows.Count > 0)
                {
                    string summaryMessage = String.Format("Failed to load one or more file(s).");
                    TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(summaryMessage, errorTable, System.Drawing.SystemIcons.Error);
                    md.ShowDialog();
                }
            }
        }

        void theSaveSqlMenuItem_Click(object sender, EventArgs e)
        {
            string theSqlText = _theQueryUserControl.TheQueryInputControl.TheQueryTextBox.Text;

            SaveFileDialog saveDialog = new SaveFileDialog();
            saveDialog.InitialDirectory = Trafodion.Manager.Framework.Utilities.FileDialogLocation();
            saveDialog.Title = Properties.Resources.SaveSqlTitle;
            saveDialog.Filter = "Text File (*.sql;*.txt)|*.sql;*.txt|All Files (*.*)|*.*";
            saveDialog.RestoreDirectory = true;
            if (saveDialog.ShowDialog(this) == DialogResult.OK)
            {
                StreamWriter sw = null;
                FileStream fs = null;

                try
                {
                    fs = new FileStream(saveDialog.FileName, FileMode.Create);
                    sw = new StreamWriter(fs);
                    string[] separators = { Environment.NewLine, "\n"};
                    string[] textArray = theSqlText.Split(separators, StringSplitOptions.None);
                    foreach (string lineOfText in textArray)
                    {
                        sw.WriteLine(lineOfText);
                    }

                    // Persist the last file-location: 
                    Trafodion.Manager.Framework.Utilities.FileDialogLocation(saveDialog.FileName);
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


        #endregion


        #region Private Methods

        /// <summary>
        /// The Resize event handler.
        /// </summary>
        private void FloatingQueryWhiteboardControl_Resize(object sender, EventArgs e)
        {
            //System.Drawing.Size newSize = ((Control)sender).Size;

            //if (this.Parent != null)
            //{
            //    // We don't want to shrunk too small or grow bigger than the container.
            //    if (newSize.Width >= this.DefaultSize.Width &&
            //        newSize.Height >= this.DefaultSize.Height &&
            //        newSize.Width <= this.Parent.Size.Width &&
            //        newSize.Height <= this.Parent.Size.Height)
            //    {
            //        //System.Console.WriteLine("DB.OnResize[Prnt]:w=" + this.Parent.Size.Width + ";h=" + this.Parent.Size.Height);
            //        //System.Console.WriteLine("DB.OnResize[New ]:w=" + newSize.Width + ";h=" + newSize.Height);
            //        //System.Console.WriteLine("DB.OnResize[Old ]:w=" + this._theCurrentSize.Width + ";h=" + this._theCurrentSize.Height);
            //        double wf = (newSize.Width / 1.0F) / (this._theCurrentSize.Width / 1.0F);
            //        double hf = (newSize.Height / 1.0F) / (this._theCurrentSize.Height / 1.0F);
            //        //System.Console.WriteLine("DB.OnResize[fctr]:w=" + wf + ";h=" + hf);
            //        this.widgetCanvas.resize(wf, hf);

            //        // Remember the current size.
            //        this._theCurrentSize = newSize;
            //    }
            //}
        }

        void TheDatabaseTreeViewSelected(NavigationTreeNode aTreeNode)
        {
            NodeActivated(aTreeNode);
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {   
                if (_theDatabaseTreeViewSelectedHandler != null)
                {
                    _theDatabaseTreeView.Selected -= _theDatabaseTreeViewSelectedHandler;
                    _theDatabaseTreeViewSelectedHandler = null;
                }

                Persistence.PersistenceHandlers -= PersistencePersistenceHandlers;
                _theQueryUserControl.OnMySystemChanged -= TheQueryUserControl_OnMySystemChanged;

                _theQueryDetailsUserControl.Dispose();
                _theQueryUserControl.Dispose();
                _theQueryListUserControl.Dispose();
                widgetCanvas.Dispose();
            }
            base.Dispose(disposing);
        }

        private void TheDatabaseTreeViewNodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            //NodeActivated(e.Node);
        }

        private void NodeActivated(TreeNode aTreeNode)
        {

            if (Utilities.InUnselectedTabPage(this))
            {
                return;
            }

            if (aTreeNode is NavigationTreeNode)
            {

                // We can here while the list is being modified 
                for (int theAttempts = 1; theAttempts < 3; theAttempts++)
                {
                    try
                    {
                        TheSelectedConnectionDefinition = ((NavigationTreeNode)aTreeNode).TheConnectionDefinition;
                        break;
                    }
                    catch (System.InvalidOperationException ioe)
                    {

                        if (theAttempts > 1)
                        {
                            return;
                        }

                    }
                }

            }

            TrafodionObject theTrafodionObject = null;

            if (aTreeNode is DatabaseTreeFolder)
            {
                DatabaseTreeFolder theDatabaseTreeFolder = aTreeNode as DatabaseTreeFolder;
                theTrafodionObject = theDatabaseTreeFolder.TrafodionObject;
            }
            else if (aTreeNode is DatabaseTreeNode)
            {
                DatabaseTreeNode theDatabaseTreeNode = aTreeNode as DatabaseTreeNode;
                theTrafodionObject = theDatabaseTreeNode.TrafodionObject;
            }

            if (theTrafodionObject != null)
            {
                if (theTrafodionObject is IHasTrafodionCatalog)
                {
                    TheSelectedTrafodionCatalog = ((IHasTrafodionCatalog)theTrafodionObject).TheTrafodionCatalog;
                }

                if (theTrafodionObject is IHasTrafodionSchema)
                {
                    TheSelectedTrafodionSchema = ((IHasTrafodionSchema)theTrafodionObject).TheTrafodionSchema;
                }

            }
        }

        private string ParseReportDefinitionName(string reportFileName)
        {
            string[] tokens = reportFileName.Split(new string[] { "@" }, StringSplitOptions.RemoveEmptyEntries);
            string fileName = tokens[tokens.Length - 1];
            int extensionIndex = fileName.LastIndexOf(".");
            if (extensionIndex > 0)
            {
                fileName = fileName.Substring(0, extensionIndex);
            }
            return fileName;
        }

        private void PersistencePersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aReportDefinitionsOperation)
        {
            switch (aReportDefinitionsOperation)
            {
                case Persistence.PersistenceOperation.Load:
                    {
                        QueryStringsPersistence.LoadPersistence();
                        break;
                    }
                case Persistence.PersistenceOperation.Save:
                    {
                        QueryStringsPersistence.TheReportDefinitions = _theQueryUserControl.TheQueryListDataGridView.GetGroupReports(Properties.Resources.PersistenceFile);
                        QueryStringsPersistence.SavePersistence(aDictionary);
                        break;
                    }
                default:
                    {
                        break;
                    }
            }
        }

        void TheQueryUserControl_OnMySystemChanged(object sender, EventArgs e)
        {
            //if (_theQueryUserControl.TheSelectedConnectionDefinition != null &&
            //    //_theQueryUserControl.TheSelectedConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            if (_theQueryUserControl.TheSelectedConnectionDefinition != null)
            {
                _theMainToolBar.TheEventViewerToolStripItem.Enabled = true;
            }
            else
            {
                _theMainToolBar.TheEventViewerToolStripItem.Enabled = false;
            }

            if (TrafodionContext.Instance.isCommunityEdition)
            {
                _theMainToolBar.TheEventViewerToolStripItem.Visible = false;
                _theMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
                _theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
            }
            this.SetToolStripItemStatus(_theQueryUserControl.TheSelectedConnectionDefinition);

        }

        private void SetToolStripItemStatus(ConnectionDefinition conn)
        {
            _theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
            _theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = false;
            _theMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
            _theMainToolBar.TheAuditLogViewerToolStripItem.Enabled = false;
            _theMainToolBar.TheUpdateConfigurationToolStripItem.Visible = false;
            _theMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = false;
            _theMainToolBar.TheRunScriptToolStripItem.Visible = false;
            _theMainToolBar.TheRunScriptToolStripItem.Enabled = false;
            
            if (conn != null &&
                conn.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                if (conn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    //only user that has the DOWNLOAD_TAR privilege can click this menuitem
                    if (conn.ComponentPrivilegeExists("SQL_OPERATIONS", "DOWNLOAD_TAR"))
                    {
                        //_theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = true;
                    }

                    if (conn.ComponentPrivilegeExists("AUDIT_LOGGING", "UPDATE_CONFIGURATION"))
                    {
                       // _theMainToolBar.TheAuditLogViewerToolStripItem.Enabled = true;
                    }
                }
                else
                {
                   // _theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = _theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = false;
                  //  _theMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
                }
                if (conn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                   // _theMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = (conn.RoleName.Equals("DB__ROOTROLE")
                      //      || conn.DatabaseUserName.Equals("DB__ROOT"));
                   // _theMainToolBar.TheRunScriptToolStripItem.Enabled = true;
                }
                else
                {
                    _theMainToolBar.TheUpdateConfigurationToolStripItem.Visible = _theMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = false;
                    //_theMainToolBar.TheRunScriptToolStripItem.Enabled = true;
                }
	            if (TrafodionContext.Instance.isCommunityEdition)
	            {
	                _theMainToolBar.TheEventViewerToolStripItem.Visible = false;
	                _theMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
	                _theMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
	            }				
            }
        }

        #endregion /* end of Private Methods region */

    }
}
