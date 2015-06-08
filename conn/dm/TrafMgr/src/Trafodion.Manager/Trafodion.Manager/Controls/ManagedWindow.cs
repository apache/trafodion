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
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections.Controls;
using System.Runtime.InteropServices;
using Trafodion.Manager.Framework.Connections;
using System.Configuration;
using System.IO;


namespace Trafodion.Manager.Framework.Controls
{

    /// <summary>
    /// Rather than creating a window directly, create an instance of this class and place your control(s) in it or
    /// derive your window from this class.  By so doing, your window will be automatically managed by the windows 
    /// manager and windows menu.
    /// </summary>
    public partial class ManagedWindow : TrafodionForm
    {

        ConnectionDefinition _connectionDefinition = null;
        EventHandler theSystemsToolHandler = null;
        EventHandler theExitToolHandler = null;
        EventHandler theWindowsDropDownHandler = null;
        EventHandler theWindowsHandler = null;
        EventHandler theMetricMinerHandler = null;
        EventHandler theSQLWhiteboardHandler = null;
        EventHandler theNCIHandler = null;
        EventHandler theOptionsHandler = null;
        EventHandler theLiveEventsHandler = null;
        EventHandler theRepositoryEventsHandler = null;
        EventHandler theLiveAuditLogsHandler = null;
        EventHandler theRepositoryAuditLogsHandler = null;
        EventHandler theAuditLogsConfigHandler = null;
        EventHandler theDownloadOSIMDataHandler = null;
        EventHandler theUpdateLDAPConfigurationHandler = null;
        EventHandler theRunScriptHandler = null;

        private bool _needsPersistence = false;

        private bool _isIndependent = false;

        private Trafodion.Manager.Framework.MainMenu _theMainMenu = new Trafodion.Manager.Framework.MainMenu();

        private Trafodion.Manager.Framework.MainToolBar _theMainToolBar = null;

        private FormWindowState _lastWindowState = FormWindowState.Normal;

        #region Properties

        /// <summary>
        /// Returns the main menu for the window
        /// </summary>
        public Trafodion.Manager.Framework.MainMenu TheMainMenu
        {
            get { return _theMainMenu; }
        }

        /// <summary>
        /// Returns a reference to the main menu bar
        /// </summary>
        public TrafodionMenuStrip MainMenuBar
        {
            get { return theMainMenuBar; }
        }

        /// <summary>
        /// Returns a reference to the main tool bar
        /// </summary>
        public MainToolBar TheMainToolBar
        {
            get { return _theMainToolBar; }
            set { _theMainToolBar = value; }
        }

        /// <summary>
        /// Returns a reference to the main tool strip (for tool buttons)
        /// </summary>
        public TrafodionToolStrip TheManagedWindowToolButtons
        {
            get { return theManagedWindowToolButtons; }
        }

        /// <summary>
        /// ConnectionDefinition property that needs be in all ClonedWindow e.g. to enable its closing
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _connectionDefinition; }
            set 
            { 
                _connectionDefinition = value;
                TrafodionBannerControl1.ConnectionDefinition = _connectionDefinition;
            }
        }

        /// <summary>
        /// This property indicates if the content in this managed window needs to be 
        /// persisted.
        /// </summary>
        public bool NeedsPersistence
        {
            get { return _needsPersistence; }
            set { _needsPersistence = value; }
        }

        /// <summary>
        /// This property is used to show or hide the banner
        /// </summary>
        public bool BannerVisible
        {
            get { return TrafodionBannerControl1.Visible; }
            set { TrafodionBannerControl1.Visible = value; }
        }
        /// <summary>
        /// To Enable/disable description display 
        /// </summary>
        public bool ShowDescription
        {
            get { return TrafodionBannerControl1.ShowDescription; }
            set { TrafodionBannerControl1.ShowDescription = value; }
        }

        /// <summary>
        /// The last window state to restore to
        /// </summary>
        public FormWindowState LastWindowState
        {
            get { return _lastWindowState; }
        }

        /// <summary>
        /// This window will be running independent of the connection open/close. So, the user needs to close this window 
        /// explicitly. 
        /// </summary>
        public bool IsIndependent
        {
            get { return _isIndependent; }
            set { _isIndependent = value; }
        }

        #endregion

        /// <summary>
        /// Constructor
        /// </summary>
        public ManagedWindow()
        {

            // Cal the code generated by the forms designer
            InitializeComponent();

            //The name is needed because this is what will be used by ToolStripManager to find the tool strip
            this.theMainMenuBar.Name = global::Trafodion.Manager.Properties.Resources.ManagedWindowMenuStrip;

            //Populate the main menu bar and disable the ones that we don't intend to display
            this.associateMenuHandlers();
            this.theMainMenuBar.AllowMerge = true;
            this._theMainMenu.ViewToolStripMenuItem.Visible = false;
            this._theMainMenu.OpenToolStripMenuItem.Visible = false;
            //this._theMainMenu.OptionsToolStripMenuItem.Visible = false;
            //this._theMainMenu.NCIToolStripMenuItem.Visible = false;
            
            //Merge the menus from main menu to the menu strip
            System.Windows.Forms.ToolStripManager.Merge(this._theMainMenu.TheMainMenuBar, this.theMainMenuBar);
         }

        /// <summary>
        /// Title
        /// </summary>
        public override string Text
        {
            get
            {
                return base.Text;
            }
            set
            {
                base.Text = value;
                if (base.Text.Equals(TrafodionForm.TitlePrefix + Properties.Resources.MetricMiner))
                {
                    this._theMainMenu.MetricMinerToolStripMenuItem.Visible = false;
                }
                if (base.Text.Equals(TrafodionForm.TitlePrefix + Properties.Resources.SQLWhiteboard))
                {
                    this._theMainMenu.SqlWhiteboardToolStripMenuItem.Visible = false;
                }
                if (base.Text.Equals(TrafodionForm.TitlePrefix + Properties.Resources.Options))
                {
                    this._theMainMenu.OptionsToolStripMenuItem.Visible = false;
                }
                //hide mainmenubar for OSIM data download window
                string systemIdentifier = (_connectionDefinition != null) ? _connectionDefinition.Name + " : " : "";
                if (base.Text.Equals(TrafodionForm.TitlePrefix + systemIdentifier + Properties.Resources.DownloadOSIMDataFileTitle))
                {
                    this.theMainMenuBar.Visible = false;
                }
            }
        }
        /// <summary>
        /// Override this method to provide a custom title for the windows menu and windows manager
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Text;
        }

        /// <summary>
        /// Show/Hide the Tool Button Strip for the managed window; by default, it is hidden.
        /// </summary>
        public bool ShowToolStripButtons
        {
            get { return theManagedWindowToolButtons.Visible; }
            set 
            { 
                theManagedWindowToolButtons.Visible = value;
                if (value)
                {
                    EnableMainToolButtons();
                }
                else
                {
                    DisableMainToolButtons();
                }
            }
        }

        /// <summary>
        /// Restore the window to its last window state
        /// </summary>
        public void Restore()
        {
            this.WindowState = _lastWindowState;
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            removeMenuHandlers();
            System.Windows.Forms.ToolStripManager.RevertMerge(this.theMainMenuBar);
            System.Windows.Forms.ToolStripManager.Merge(this._theMainMenu.TheMainMenuBar, this.theMainMenuBar);
            
            //Notify persistence to persist since this window is marked for persistence
            if (NeedsPersistence)
            {
                Persistence.SaveAllToDefault();
            }

            foreach (Control control in Controls)
            {
                control.Dispose();
            }
            
            Controls.Clear();

            base.OnFormClosing(e);
        }
 

        /// <summary>
        /// Called when the user clicks Tools | Systems
        /// </summary>
        /// <param name="sender">The event generator</param>
        /// <param name="e">The event</param>
        void TheSystemsToolStripMenuItemClick(object sender, EventArgs e)
        {

            // Run the systems tool
            (new ConnectionsTool()).ShowDialog();

        }

        protected override void OnActivated(EventArgs e)
        {
            base.OnActivated(e);
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            if (this.WindowState != FormWindowState.Minimized)
            {
                _lastWindowState = this.WindowState;
            }
        }

        /// <summary>
        /// To create a maintoolbar and merge it into the managed window. 
        /// However, turned all of the buttons in the maintoolbar.  The Control, which implements
        /// IMainToolBarConsumer should turn those buttons it wants to support/display. 
        /// </summary>
        private void EnableMainToolButtons()
        {
            //Handles ToolBars
            if (this._theMainToolBar == null)
            {
                this._theMainToolBar = new MainToolBar(_theMainMenu);
            }

            this.TheMainToolBar.Name = "ManagedWindowToolStrip";
            this.TheMainToolBar.HideAllButtons();
            System.Windows.Forms.ToolStripManager.Merge(this._theMainToolBar.TheFrameWorkToolBar, this.theManagedWindowToolButtons);
        }

        /// <summary>
        /// To reveret the merged main tool buttons
        /// </summary>
        private void DisableMainToolButtons()
        {
            if (this._theMainToolBar != null)
            {
                System.Windows.Forms.ToolStripManager.RevertMerge(this.theManagedWindowToolButtons);
            }
        }

        /// <summary>
        /// Associate menu item handlers.
        /// </summary>
        private void associateMenuHandlers()
        {
            theSystemsToolHandler = new EventHandler(TheSystemsToolStripMenuItemClick);
            this._theMainMenu.TheSystemsToolStripMenuItem.Click += theSystemsToolHandler;

            theExitToolHandler = new System.EventHandler(this.exitToolStripMenuItem_Click);
            this._theMainMenu.ExitToolStripMenuItem.Click += theExitToolHandler;

            theWindowsDropDownHandler = new System.EventHandler(this.TheWindowsToolStripMenuItemDropDownOpening);
            this._theMainMenu.TheWindowsToolStripMenuItem.DropDownOpening += theWindowsDropDownHandler;

            theWindowsHandler = new System.EventHandler(this.theMainWindowToolStripMenuItem_Click);
            this._theMainMenu.MainWindowToolStripMenuItem.Click += theWindowsHandler;

            theMetricMinerHandler = new EventHandler(MetricMinerToolStripMenuItem_Click);
            _theMainMenu.MetricMinerToolStripMenuItem.Click += theMetricMinerHandler;

            theSQLWhiteboardHandler = new EventHandler(SqlWhiteboardToolStripMenuItem_Click);
            _theMainMenu.SqlWhiteboardToolStripMenuItem.Click += theSQLWhiteboardHandler;

            theNCIHandler = new EventHandler(NCIToolStripMenuItem_Click);
            _theMainMenu.NCIToolStripMenuItem.Click += theNCIHandler;

            theOptionsHandler = new EventHandler(OptionsToolStripmenuItem_Click);
            _theMainMenu.OptionsToolStripMenuItem.Click += theOptionsHandler;

           theLiveEventsHandler = new EventHandler(LiveEventsToolStripMenuItem_Click);
            _theMainMenu.LiveEventsToolStripMenuItem.Click += theLiveEventsHandler;

            theRepositoryEventsHandler = new EventHandler(RepositoryEventsToolStripMenuItem_Click);
            _theMainMenu.RepositoryEventsToolStripMenuItem.Click += theRepositoryEventsHandler;

            theLiveAuditLogsHandler = new EventHandler(LiveAuditLogsToolStripMenuItem_Click);
            _theMainMenu.LiveAuditLogsToolStripMenuItem.Click += theLiveAuditLogsHandler;

            theRepositoryAuditLogsHandler = new EventHandler(RepositoryAuditLogsToolStripMenuItem_Click);
            _theMainMenu.SecurityAuditLogsViewerToolStripMenuItem.Click += theRepositoryAuditLogsHandler;

            theAuditLogsConfigHandler = new EventHandler(AuditLogsConfigurationToolStripMenuItem_Click);
            _theMainMenu.SecurityAuditLogConfigurationToolStripMenuItem.Click += theAuditLogsConfigHandler;

            theDownloadOSIMDataHandler = new EventHandler(downloadOSIMDataToolStripMenuItem_Click);
            _theMainMenu.DownloadOSIMDataToolStripMenuItem.Click += theDownloadOSIMDataHandler;

            theUpdateLDAPConfigurationHandler = new EventHandler(updateLDAPConfigurationToolStripMenuItem_Click);
            _theMainMenu.LDAPConnectionConfigurationToolStripMenuItem.Click += theUpdateLDAPConfigurationHandler;

            theRunScriptHandler = new EventHandler(runScriptToolStripMenuItem_Click);
            _theMainMenu.RunScriptToolStripMenuItem.Click += theRunScriptHandler;
        }

        void SqlWhiteboardToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheSQLWhiteboardToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheSQLWhiteboardToolStripItem.PerformClick();
            }
        }

        void MetricMinerToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheMetricMinerToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheMetricMinerToolStripItem.PerformClick();
            }
        }

        private void NCIToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheNCIToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheNCIToolStripItem.PerformClick();
            }
        }

        private void OptionsToolStripmenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheOptionsToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheOptionsToolStripItem.PerformClick();
            }
        }

        void LiveAuditLogsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem != null)
            {
                TrafodionMain.LaunchAuditLogViewer(TrafodionContext.Instance.CurrentConnectionDefinition, true);

            }
        }

        void RepositoryAuditLogsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem != null)
            {
                TrafodionMain.LaunchAuditLogViewer(TrafodionContext.Instance.CurrentConnectionDefinition, false);
            }
        }

        void AuditLogsConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null)
            {
                TrafodionMain.LaunchAuditLogConfig(TrafodionContext.Instance.CurrentConnectionDefinition);
            }
        }

        void LiveEventsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheEventViewerToolStripItem != null)
            {
                TrafodionMain.LaunchEventViewer(TrafodionContext.Instance.CurrentConnectionDefinition, true);
            }
        }

        void RepositoryEventsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheEventViewerToolStripItem != null)
            {
                TrafodionMain.LaunchEventViewer(TrafodionContext.Instance.CurrentConnectionDefinition, false);
            }
        }

        private void removeMenuHandlers()
        {
            if (theSystemsToolHandler != null)
            {
                this._theMainMenu.TheSystemsToolStripMenuItem.Click -= theSystemsToolHandler;
            }

            if (theExitToolHandler != null)
            {
                this._theMainMenu.ExitToolStripMenuItem.Click -= theExitToolHandler;
            }

            if (theWindowsDropDownHandler != null)
            {
                this._theMainMenu.TheWindowsToolStripMenuItem.DropDownOpening -= theWindowsDropDownHandler;
            }

            if (theWindowsHandler != null)
            {
                this._theMainMenu.MainWindowToolStripMenuItem.Click -= theWindowsHandler;
            }

            if (theMetricMinerHandler != null)
            {
                _theMainMenu.MetricMinerToolStripMenuItem.Click -= theMetricMinerHandler;
            }

            if (theSQLWhiteboardHandler != null)
            {
                _theMainMenu.SqlWhiteboardToolStripMenuItem.Click -= theSQLWhiteboardHandler;
            }

            if (theLiveEventsHandler != null)
            {
                _theMainMenu.LiveEventsToolStripMenuItem.Click -= theLiveEventsHandler;
            }

            if (theRepositoryEventsHandler != null)
            {
                _theMainMenu.RepositoryEventsToolStripMenuItem.Click -= theRepositoryEventsHandler;
            }
            if (theUpdateLDAPConfigurationHandler != null)
            {
                _theMainMenu.LDAPConnectionConfigurationToolStripMenuItem.Click -= theUpdateLDAPConfigurationHandler;
            }
            if (theDownloadOSIMDataHandler != null)
            {
                _theMainMenu.DownloadOSIMDataToolStripMenuItem.Click -= theDownloadOSIMDataHandler;
            }

            if (theRunScriptHandler != null)
            {
                _theMainMenu.RunScriptToolStripMenuItem.Click -= theRunScriptHandler;
            }
        }

        /// <summary>
        /// Called when the user selects the main window entry on the windows menu
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theMainWindowToolStripMenuItem_Click(object sender, EventArgs e)
        {

            // The windows manager knows how to do it
            WindowsManager.ActivateMainWindow();

        }

        /// <summary>
        /// Called when the windows menu is about to be dropped down
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheWindowsToolStripMenuItemDropDownOpening(object sender, EventArgs e)
        {

            // let the windows manager calculate the correct menu contents
            WindowsManager.FixupWindowsMenu(this, this._theMainMenu.TheWindowsToolStripMenuItem);

        }

        /// <summary>
        /// called when OSIM data menu is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void downloadOSIMDataToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.PerformClick();
            }
        }

        /// <summary>
        /// called when LDAP configuration menu is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void updateLDAPConfigurationToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.PerformClick();
            }
        }

        /// <summary>
        /// called when run script menu is clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void runScriptToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
            if (TrafodionMain != null && TrafodionMain.TheMainToolBar.TheRunScriptToolStripItem != null)
            {
                TrafodionMain.TheMainToolBar.TheRunScriptToolStripItem.PerformClick();
            }
        }

        #region EditMenuSupport

        // Import GetFocus() from user32.dll
        [DllImport("user32.dll", CharSet = CharSet.Auto,
           CallingConvention = CallingConvention.Winapi)]
        internal static extern IntPtr GetFocus();

        private Control GetFocusControl()
        {
            Control focusControl = null;
            IntPtr focusHandle = GetFocus();
            if (focusHandle != IntPtr.Zero)
                // returns null if handle is not to a .NET control
                focusControl = Control.FromHandle(focusHandle);
            return focusControl;
        }

        private TextBoxBase GetFocusTextboxBase()
        {
            return GetFocusControl() as TextBoxBase;
        }

         #endregion


        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (Control control in Controls)
            {
                control.Dispose();
            }
            Close();
        }

        private void ManagedWindow_FormClosing(object sender, FormClosingEventArgs e)
        {
            //The following code has been moved to OnFormClosing because the persistence event was
            //being fired after the event handler was being removed.
            //Persistence.SaveAllToDefault();
        }


    }
}
