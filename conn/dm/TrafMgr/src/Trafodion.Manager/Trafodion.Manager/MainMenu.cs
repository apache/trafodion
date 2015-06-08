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
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Reflection;
using System.Runtime.InteropServices;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This class provides a shell for the menus that are displayed by the TrafodionMain class
    /// and the window manager. None of the handlers are here. They have been implemented
    /// in the TrafodionMain and the window manager classes. This has been done to bring in 
    /// all of the menus in one place.
    /// </summary>
    public partial class MainMenu : UserControl
    {
        public MainMenu()
        {
            InitializeComponent();
            this.fileToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuFile;
            this.newToolStripMenuItem.Name                                  = global::Trafodion.Manager.Properties.Resources.MenuNew;
            this.openToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuOpen;
            this.saveToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuSave;
            this.saveAsToolStripMenuItem.Name                               = global::Trafodion.Manager.Properties.Resources.MenuSaveAs;
            this.printToolStripMenuItem.Name                                = global::Trafodion.Manager.Properties.Resources.MenuPrint;
            this.printPreviewToolStripMenuItem.Name                         = global::Trafodion.Manager.Properties.Resources.MenuPrintPreview;
            this.exitToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuExit;
            this.editToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuEdit;
            this._theUndoToolStripMenuItem.Name                             = global::Trafodion.Manager.Properties.Resources.MenuUndo;
            this._theRedoToolStripMenuItem.Name                             = global::Trafodion.Manager.Properties.Resources.MenuRedo;
            this._theCutToolStripMenuItem.Name                              = global::Trafodion.Manager.Properties.Resources.MenuCut;
            this._theCopyToolStripMenuItem.Name                             = global::Trafodion.Manager.Properties.Resources.MenuCopy;
            this._thePasteToolStripMenuItem.Name                            = global::Trafodion.Manager.Properties.Resources.MenuPaste;
            this._theSelectAllToolStripMenuItem.Name                        = global::Trafodion.Manager.Properties.Resources.MenuSelectAll;
            this.viewToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuView;
            this.customizeAreasToolStripMenuItem.Name                       = global::Trafodion.Manager.Properties.Resources.MenuShowAreas;
            this.jumpToToolStripMenuItem.Name                               = global::Trafodion.Manager.Properties.Resources.MenuJumpTo;
            this.toolsToolStripMenuItem.Name                                = global::Trafodion.Manager.Properties.Resources.MenuTools;
            this.sqlWhiteboardToolStripMenuItem.Name                        = global::Trafodion.Manager.Properties.Resources.SQLWhiteboard;
            this.metricMinerToolStripMenuItem.Name                          = global::Trafodion.Manager.Properties.Resources.MetricMiner;
            this.nCIToolStripMenuItem.Name                                  = global::Trafodion.Manager.Properties.Resources.NCI;
            this.theSystemsToolStripMenuItem.Name                           = global::Trafodion.Manager.Properties.Resources.MenuSystemTool;
            this.optionsToolStripMenuItem.Name                              = global::Trafodion.Manager.Properties.Resources.MenuOptions;
            this.commandInterpreterToolStripMenuItem.Name                   = global::Trafodion.Manager.Properties.Resources.MenuCommandInterpreter;
            this.performanceMonitorToolStripMenuItem.Name                   = global::Trafodion.Manager.Properties.Resources.MenuPerformanceMonitor;
            this.performanceAnalysisToolStripMenuItem.Name                  = global::Trafodion.Manager.Properties.Resources.MenuPerformanceAnalysis;
            this.workspaceDesignerToolStripMenuItem1.Name                   = global::Trafodion.Manager.Properties.Resources.MenuWorkspaceDesigner;
            this.theWindowsToolStripMenuItem.Name                           = global::Trafodion.Manager.Properties.Resources.MenuWindows;
            this.theWindowsManagerToolStripMenuItem.Name                    = global::Trafodion.Manager.Properties.Resources.MenuWindowsManager;
            this.theCloseAllWindowsExceptMainWindowToolStripMenuItem.Name   = global::Trafodion.Manager.Properties.Resources.MenuCloseAllWindowsExceptMainWindow;
            this.mainWindowToolStripMenuItem.Name                           = global::Trafodion.Manager.Properties.Resources.MenuMainWindow;
            this.cloneThisWindowToolStripMenuItem.Name                      = global::Trafodion.Manager.Properties.Resources.MenuCloneThisWindow;
            this.helpToolStripMenuItem.Name                                 = global::Trafodion.Manager.Properties.Resources.MenuHelp;
            this.contentsToolStripMenuItem.Name                             = global::Trafodion.Manager.Properties.Resources.MenuContents;
            this.indexToolStripMenuItem.Name                                = global::Trafodion.Manager.Properties.Resources.MenuIndex;
            this.searchToolStripMenuItem.Name                               = global::Trafodion.Manager.Properties.Resources.MenuSearch;
            this.aboutToolStripMenuItem.Name                                = global::Trafodion.Manager.Properties.Resources.MenuAbout;
            this.eventViewerToolStripMenuItem.Name                          = global::Trafodion.Manager.Properties.Resources.MenuEventViewer;
            this.liveEventsToolStripMenuItem.Name                           = global::Trafodion.Manager.Properties.Resources.LiveEvents;
            this.repositoryEventsToolStripMenuItem.Name                     = global::Trafodion.Manager.Properties.Resources.RepositoryEvents;
            this.securityAuditLogsToolStripMenuItem.Name                    = global::Trafodion.Manager.Properties.Resources.SecurityAuditLogs;
            this.securityAuditLogsViewerToolStripMenuItem.Name              = global::Trafodion.Manager.Properties.Resources.SecurityAuditLogsViewer;
            this.auditLoggingConfigurationToolStripMenuItem.Name            = global::Trafodion.Manager.Properties.Resources.SecurityAuditLogsConfig;

            this.FileToolStripMenuItem.MouseDown += new System.Windows.Forms.MouseEventHandler(this.fileToolStripMenuItemMouseDown);
            this.OpenToolStripMenuItem.Click            += new System.EventHandler(this.openToolStripMenuItemClick);
            this.SaveToolStripMenuItem.Click            += new System.EventHandler(this.saveToolStripMenuItemClick);
            this.SaveAsToolStripMenuItem.Click          += new System.EventHandler(this.saveAsToolStripMenuItemClick);
            this.EditToolStripMenuItem.DropDownOpening  += new System.EventHandler(this.EditToolStripMenuItemDropDownOpening);
            this.TheUndoToolStripMenuItem.Click         += new System.EventHandler(this.TheUndoToolStripMenuItemClick);
            this.TheRedoToolStripMenuItem.Click         += new System.EventHandler(this.TheRedoToolStripMenuItemClick);
            this.TheCutToolStripMenuItem.Click          += new System.EventHandler(this.TheCutToolStripMenuItemClick);
            this.TheCopyToolStripMenuItem.Click         += new System.EventHandler(this.TheCopyToolStripMenuItemClick);
            this.ThePasteToolStripMenuItem.Click        += new System.EventHandler(this.ThePasteToolStripMenuItemClick);
            this.TheSelectAllToolStripMenuItem.Click    += new System.EventHandler(this.TheSelectAllToolStripMenuItemClick);
            this.ContentsToolStripMenuItem.Click        += new System.EventHandler(this.contentsToolStripMenuItem_Click);
            this.IndexToolStripMenuItem.Click           += new System.EventHandler(this.indexToolStripMenuItem_Click);
            this.SearchToolStripMenuItem.Click          += new System.EventHandler(this.searchToolStripMenuItem_Click);
            this.AboutToolStripMenuItem.Click           += new System.EventHandler(this.aboutToolStripMenuItem_Click);
            this.ToolsToolStripMenuItem.DropDownOpening += new EventHandler(ToolsToolStripMenuItem_DropDownOpening);
            this.viewToolStripMenuItem.Visible = false;
            this.customizeAreasToolStripMenuItem.Visible = false;

        }

        void ToolsToolStripMenuItem_DropDownOpening(object sender, EventArgs e)
        {
            ConnectionDefinition currentConnection = TrafodionContext.Instance.CurrentConnectionDefinition;
            repositoryEventsToolStripMenuItem.Enabled = (currentConnection != null && currentConnection.TheState == ConnectionDefinition.State.TestSucceeded);
            liveEventsToolStripMenuItem.Enabled = (currentConnection != null);
            securityAuditLogsToolStripMenuItem.Visible = false;
            securityAuditLogsViewerToolStripMenuItem.Enabled = false;
            auditLoggingConfigurationToolStripMenuItem.Enabled = false;
            runScriptToolStripMenuItem.Enabled = false;

            this.downloadOSIMDataToolStripMenuItem.Visible = false;
            this.downloadOSIMDataToolStripMenuItem.Enabled = false;

            this.LDAPConnectionConfigurationToolStripMenuItem.Visible = false;
            this.LDAPConnectionConfigurationToolStripMenuItem.Enabled = false;

           /* if (currentConnection != null && 
                currentConnection.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                runScriptToolStripMenuItem.Enabled = true;
                if (currentConnection.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    securityAuditLogsViewerToolStripMenuItem.Enabled = true;

                    if (currentConnection.ComponentPrivilegeExists("AUDIT_LOGGING", "UPDATE_CONFIGURATION"))
                    {
                        auditLoggingConfigurationToolStripMenuItem.Enabled = true;
                    }
                    //only user that has the DOWNLOAD_TAR privilege can click this menuitem
                    if (currentConnection.ComponentPrivilegeExists("SQL_OPERATIONS", "DOWNLOAD_TAR"))
                    {
                        downloadOSIMDataToolStripMenuItem.Enabled = true;
                    }
                }
                else
                {
                    runScriptToolStripMenuItem.Enabled = false;
                    securityAuditLogsToolStripMenuItem.Visible = false;
                    this.downloadOSIMDataToolStripMenuItem.Visible = false;
                }

                // >=M10
                if (currentConnection.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    if (currentConnection.RoleName.Equals("DB__ROOTROLE") || currentConnection.DatabaseUserName.Equals("DB__ROOT"))
                    {
                        LDAPConnectionConfigurationToolStripMenuItem.Enabled = true;
                    }
                }
                else
                {
                    this.LDAPConnectionConfigurationToolStripMenuItem.Visible = false;
                }
            }*/
        }

        #region Properties
        /// <summary>
        /// 
        /// </summary>
        public System.Windows.Forms.ToolStripMenuItem TheUndoToolStripMenuItem
        {
            get { return _theUndoToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheRedoToolStripMenuItem
        {
            get { return _theRedoToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheCutToolStripMenuItem
        {
            get { return _theCutToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheCopyToolStripMenuItem
        {
            get { return _theCopyToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem ThePasteToolStripMenuItem
        {
            get { return _thePasteToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheSelectAllToolStripMenuItem
        {
            get { return _theSelectAllToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem SaveToolStripMenuItem
        {
            get { return saveToolStripMenuItem; }
        }
        public System.Windows.Forms.ToolStripMenuItem FileToolStripMenuItem
        {
            get { return fileToolStripMenuItem; }
        }

        public Trafodion.Manager.Framework.Controls.TrafodionMenuStrip TheMainMenuBar
        {
            get { return theMainMenuBar; }
        }

        public System.Windows.Forms.ToolStripMenuItem NewToolStripMenuItem
        {
            get { return newToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem OpenToolStripMenuItem
        {
            get { return openToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem SaveToolStripMenuItem1
        {
            get { return saveToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem SaveAsToolStripMenuItem
        {
            get { return saveAsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem PrintToolStripMenuItem
        {
            get { return printToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem PrintPreviewToolStripMenuItem
        {
            get { return printPreviewToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem ExitToolStripMenuItem
        {
            get { return exitToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem EditToolStripMenuItem
        {
            get { return editToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem ViewToolStripMenuItem
        {
            get { return viewToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem CustomizeAreasToolStripMenuItem
        {
            get { return customizeAreasToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem JumpToToolStripMenuItem
        {
            get { return jumpToToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem ToolsToolStripMenuItem
        {
            get { return toolsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem SqlWhiteboardToolStripMenuItem
        {
            get { return sqlWhiteboardToolStripMenuItem; }
        }
        public System.Windows.Forms.ToolStripMenuItem MetricMinerToolStripMenuItem
        {
            get { return metricMinerToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem NCIToolStripMenuItem
        {
            get { return nCIToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheSystemsToolStripMenuItem
        {
            get { return theSystemsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem OptionsToolStripMenuItem
        {
            get { return optionsToolStripMenuItem; }
        }
        public System.Windows.Forms.ToolStripMenuItem CommandInterpreterToolStripMenuItem
        {
            get { return commandInterpreterToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem PerformanceMonitorToolStripMenuItem
        {
            get { return performanceMonitorToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem PerformanceAnalysisToolStripMenuItem
        {
            get { return performanceAnalysisToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem WorkspaceDesignerToolStripMenuItem1
        {
            get { return workspaceDesignerToolStripMenuItem1; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheWindowsToolStripMenuItem
        {
            get { return theWindowsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheWindowsManagerToolStripMenuItem
        {
            get { return theWindowsManagerToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem TheCloseAllWindowsExceptMainWindowToolStripMenuItem
        {
            get { return theCloseAllWindowsExceptMainWindowToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem HelpToolStripMenuItem
        {
            get { return helpToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem ContentsToolStripMenuItem
        {
            get { return contentsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem IndexToolStripMenuItem
        {
            get { return indexToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem SearchToolStripMenuItem
        {
            get { return searchToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem AboutToolStripMenuItem
        {
            get { return aboutToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem MainWindowToolStripMenuItem
        {
            get { return mainWindowToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem CloneThisWindowToolStripMenuItem
        {
            get { return cloneThisWindowToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem LiveEventsToolStripMenuItem
        {
            get { return liveEventsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem RepositoryEventsToolStripMenuItem
        {
            get { return repositoryEventsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem LiveAuditLogsToolStripMenuItem
        {
            get { return liveAuditLogsToolStripMenuItem; }
        }

        public System.Windows.Forms.ToolStripMenuItem SecurityAuditLogsViewerToolStripMenuItem
        {
            get { return securityAuditLogsViewerToolStripMenuItem; }
        }
        public System.Windows.Forms.ToolStripMenuItem SecurityAuditLogConfigurationToolStripMenuItem
        {
            get { return auditLoggingConfigurationToolStripMenuItem; }
        }
        public System.Windows.Forms.ToolStripMenuItem DownloadOSIMDataToolStripMenuItem
        {
            get { return this.downloadOSIMDataToolStripMenuItem; }
        }
        public System.Windows.Forms.ToolStripMenuItem LDAPConnectionConfigurationToolStripMenuItem
        {
            get { return this.configurationToolStripMenuItem; }
        }
        public ToolStripMenuItem RunScriptToolStripMenuItem
        {
            get { return this.runScriptToolStripMenuItem; }
        }
        #endregion

        #region EditMenuSupport

        // Import GetFocus() from user32.dll
        [DllImport("user32.dll", CharSet = CharSet.Auto,
           CallingConvention = CallingConvention.Winapi)]
        internal static extern IntPtr GetFocus();

        /// Windows API GetParent function
        [DllImport("user32", SetLastError = true)]
        public extern static IntPtr GetParent(IntPtr hwnd);

        private Control GetFocusControl()
        {
            Control focusControl = null;
            IntPtr focusHandle = GetFocus();
            if (focusHandle != IntPtr.Zero)
            {
                // returns null if handle is not to a .NET control
                focusControl = Control.FromHandle(focusHandle);
                // Try the parent, since with a ComboBox, we get a 
                // handle to an inner control.
                if (focusControl == null)
                    focusControl = Control.FromHandle(GetParent(focusHandle));
            }
            return focusControl;
        }

        private TextBoxBase GetFocusTextboxBase()
        {
            return GetFocusControl() as TextBoxBase;
        }

        private enum EditFunc
        {
            Undo, Redo, Cut, Copy, Paste, SelectAll
        }

        private void DoEdit(EditFunc anEditFunc)
        {
            Control theFocusControl = GetFocusControl();
            if ((theFocusControl != null) && (theFocusControl is TextBoxBase))
            {
                TextBoxBase theTextBoxBase = theFocusControl as TextBoxBase;
                switch (anEditFunc)
                {
                    case EditFunc.Copy:
                        {
                            theTextBoxBase.Copy();
                            break;
                        }
                    case EditFunc.Cut:
                        {
                            theTextBoxBase.Cut();
                            break;
                        }
                    case EditFunc.Paste:
                        {
                            theTextBoxBase.Paste();
                            break;
                        }
                    case EditFunc.Redo:
                        {
                            if (theTextBoxBase is RichTextBox)
                            {
                                ((RichTextBox)theTextBoxBase).Redo();
                            }
                            break;
                        }
                    case EditFunc.SelectAll:
                        {
                            theTextBoxBase.SelectAll();
                            break;
                        }
                    case EditFunc.Undo:
                        {
                            theTextBoxBase.Undo();
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }
            }
            else if (theFocusControl != null && (theFocusControl is ComboBox))
            {
                ComboBox theComboBox = (ComboBox)theFocusControl;
                switch (anEditFunc)
                {
                    case EditFunc.Copy:
                        {
                            Clipboard.SetDataObject(theComboBox.Text);                            
                            break;
                        }
                    case EditFunc.Cut:
                        {
                            Clipboard.SetDataObject(theComboBox.Text);
                            theComboBox.Text = "";
                            break;
                        }
                    case EditFunc.Paste:
                        {
                            if(Clipboard.ContainsText())
                            {
                                theComboBox.Text = Clipboard.GetText();
                            }                            
                            break;
                        }                    
                    case EditFunc.SelectAll:
                        {
                            theComboBox.SelectAll();
                            break;
                        }                    
                    default:
                        {
                            break;
                        }
                }
            }

        }

        private void TheUndoToolStripMenuItemClick(object sender, EventArgs e)
        {
            DoEdit(EditFunc.Undo);
        }

        private void TheRedoToolStripMenuItemClick(object sender, EventArgs e)
        {
            DoEdit(EditFunc.Redo);
        }

        private void TheCutToolStripMenuItemClick(object sender, EventArgs e)
        {
            DoEdit(EditFunc.Cut);
        }

        private void TheCopyToolStripMenuItemClick(object sender, EventArgs e)
        {
            DoEdit(EditFunc.Copy);
        }

        private void ThePasteToolStripMenuItemClick(object sender, EventArgs e)
        {
            DoEdit(EditFunc.Paste);
        }

        private void TheSelectAllToolStripMenuItemClick(object sender, EventArgs e)
        {
            DoEdit(EditFunc.SelectAll);
        }

        private void EditToolStripMenuItemDropDownOpening(object sender, EventArgs e)
        {
            DoEditToolStripMenuItemDropDownOpening();
        }

        private void DoEditToolStripMenuItemDropDownOpening()
        {
            TextBoxBase theTextBoxBase = (GetFocusControl() as TextBoxBase);
            bool enableEdit = (theTextBoxBase != null);

            this.TheCopyToolStripMenuItem.Enabled = enableEdit && (theTextBoxBase.SelectedText.Length > 0);
            this.TheCutToolStripMenuItem.Enabled = enableEdit && (theTextBoxBase.SelectedText.Length > 0);
            this.ThePasteToolStripMenuItem.Enabled = enableEdit && Clipboard.ContainsText();
            if (theTextBoxBase is RichTextBox)
            {
                this.TheRedoToolStripMenuItem.Enabled = enableEdit && ((RichTextBox)theTextBoxBase).CanRedo;
            }
            this.TheSelectAllToolStripMenuItem.Enabled = enableEdit && (theTextBoxBase.Text.Length > 0);
            this.TheUndoToolStripMenuItem.Enabled = enableEdit && (theTextBoxBase.CanUndo);
        }
        
        #endregion

 

        /// <summary>
        /// Called when the user clicks File | Open
        /// </summary>
        /// <param name="sender">The event generator</param>
        /// <param name="e">The event</param>
        private void openToolStripMenuItemClick(object sender, EventArgs e)
        {
            Persistence.DoOpenFile();
        }

        /// <summary>
        /// Called when the user clicks File | Save
        /// </summary>
        /// <param name="sender">The event generator</param>
        /// <param name="e">The event</param>
        private void saveToolStripMenuItemClick(object sender, EventArgs e)
        {
            Persistence.SaveAllToDefault();
        }

        /// <summary>
        /// Called when the user clicks File | Save As
        /// </summary>
        /// <param name="sender">The event generator</param>
        /// <param name="e">The event</param>
        private void saveAsToolStripMenuItemClick(object sender, EventArgs e)
        {
            Persistence.DoSaveFileAs();
        }

        /// <summary>
        /// Called when the user does a mouse down on the file menu head.  Decides whether or not to enable File | Save
        /// </summary>
        /// <param name="sender">The event generator</param>
        /// <param name="e">The event</param>
        private void fileToolStripMenuItemMouseDown(object sender, MouseEventArgs e)
        {
            this.SaveToolStripMenuItem.Enabled = (Persistence.CurrentFileName != null);
        }

        private void contentsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelp(HelpNavigator.TableOfContents, "");
        }

        private void indexToolStripMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelp(HelpNavigator.Index, "");
        }

        private void searchToolStripMenuItem_Click(object sender, EventArgs e)
        {
            //The empty string is needed so that Help starts up in the serach tab
            TrafodionHelpProvider.Instance.ShowHelp(HelpNavigator.Find, "");

        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Trafodion.Manager.TrafodionAboutBox theAboutBox = new Trafodion.Manager.TrafodionAboutBox();
            theAboutBox.ShowDialog();
        }

    }
}
