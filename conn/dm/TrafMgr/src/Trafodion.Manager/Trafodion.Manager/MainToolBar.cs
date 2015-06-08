// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;

namespace Trafodion.Manager.Framework
{
    /// <summary>
    /// This class provides a shell for the toolbars that are displayed by the TrafodionMain class
    /// and the window manager. None of the handlers are here. They have been implemented
    /// in the TrafodionMain and the window manager classes. This has been done to bring in 
    /// all of the toolbars in one place.
    /// </summary>
    public partial class MainToolBar : UserControl
    {
        #region private member variables

        private MainMenu theMainMenu;

        #endregion private member variables

        #region public properties

        /// <summary>
        /// List below are the names of the button in the main tool bar.  The name will be used for 
        /// identifying a specific button in the main tool bar so that we could customize for 
        /// specific widget to use.  
        /// Note: Add the new button's name here whenever a new button is added to the main tool bar.
        /// </summary>
        public const string TheConnectToolStripItemName = "connectToolStripButton";
        public const string TheDisconnectToolStripItemName = "disconnectToolStripButton";
        public const string TheSystemToolToolStripItemName = "systemToolStripButton";
        public const string TheOpenToolStripItemName = "openToolStripButton";
        public const string TheSaveToolStripItemName = "saveToolStripButton";
        public const string TheCutToolStripItemName = "cutToolStripButton";
        public const string TheCopyToolStripItemName = "copyToolStripButton";
        public const string ThePasteToolStripItemName = "pasteToolStripButton";
        public const string ThePreviousToolStripItemName = "previousToolStripButton";
        public const string TheParentToolStripItemName = "parentToolStripButton";
        public const string TheNextToolStripItemName = "nextToolStripButton";
        public const string TheMetricMinerToolStripItemName = "mmToolStripButton";
        public const string TheSQLWhiteboardToolStripItemName = "sqlWhiteBoardToolStripButton";
        public const string TheNCIToolStripItemName = "nciToolStripButton";
        public const string TheOptionsToolStripItemName = "optionsToolStripButton";
        public const string TheWindowManagerToolStripItemName = "windowManagerToolStripButton";
        public const string TheHelpToolStripItemName = "helpToolStripButton";
        public const string TheSystemsToolStripSeparatorName = "sytemToolStripSeparator";
        public const string TheOpenSaveToolStripSeparatorName = "openSaveToolStripSeparator";
        public const string TheCutCopyPasteStripSeparatorName = "cutCopyPasteToolStripSeparator";
        public const string TheToolsStripSeparatorName = "toolsToolStripSeparator";
        public const string TheArrowsStripSeparatorName = "arrowsToolStripSeparator";
        public const string TheWindowManagerStripSeparatorName = "windowManagerToolStripSeparator";
        public const string TheEventViewerToolStripItemName = "eventViewerToolStripButton";
        public const string TheAuditLogsViewerToolStripItemName = "auditLogToolStripButton";

        /// <summary>
        /// The ToolStrip itself
        /// </summary>
        public TrafodionToolStrip TheFrameWorkToolBar
        {
            get { return theMainToolBarStrip; }
        }

        public ToolStripItemCollection Items
        {
            get { return TheFrameWorkToolBar.Items; }
        }

        /// <summary>
        /// The Connect button
        /// </summary>
        public ToolStripItem TheConnectToolStripItem
        {
            get { return connectToolStripButton; }
        }

        /// <summary>
        /// The Disconnect button
        /// </summary>
        public ToolStripItem TheDisconnectToolStripItem
        {
            get { return disconnectToolStripButton; }
        }

        /// <summary>
        /// The start System tools button
        /// </summary>
        public ToolStripItem TheSystemToolToolStripItem
        {
            get { return systemToolStripButton; }
        }

        /// <summary>
        /// The Open persistence file button
        /// </summary>
        public ToolStripItem TheOpenToolStripItem
        {
            get { return openToolStripButton; }
        }

        /// <summary>
        /// The Save persistence button
        /// </summary>
        public ToolStripItem TheSaveToolStripItem
        {
            get { return saveToolStripButton; }
        }

        /// <summary>
        /// The Cut button
        /// </summary>
        public ToolStripItem TheCutToolStripItem
        {
            get { return cutToolStripButton; }
        }

        /// <summary>
        /// The Copy button
        /// </summary>
        public ToolStripItem TheCopyToolStripItem
        {
            get { return copyToolStripButton; }
        }

        /// <summary>
        /// The Paste button
        /// </summary>
        public ToolStripItem ThePasteToolStripItem
        {
            get { return pasteToolStripButton; }
        }

        /// <summary>
        /// The Help button
        /// </summary>
        public ToolStripItem TheHelpToolStripItem
        {
            get { return helpToolStripButton; }
        }

        /// <summary>
        /// The Go To Parent button (Navigation)
        /// </summary>
        public ToolStripItem TheParentToolStripItem
        {
            get { return parentToolStripButton; }
        }

        /// <summary>
        /// The Go to previous object button (Navigation)
        /// </summary>
        public ToolStripItem ThePreviousToolStripItem
        {
            get { return previousToolStripButton; }
        }

        /// <summary>
        /// The Go to Next object button (Navigation)
        /// </summary>
        public ToolStripItem TheNextToolStripItem
        {
            get { return nextToolStripButton; }
        }

        /// <summary>
        /// The start Metric Miner button
        /// </summary>
        public ToolStripItem TheMetricMinerToolStripItem
        {
            get { return mmToolStripButton; }
        }

        /// <summary>
        /// The start SQL Whiteboard button
        /// </summary>
        public ToolStripItem TheSQLWhiteboardToolStripItem
        {
            get { return sqlWhiteBoardToolStripButton; }
        }

        /// <summary>
        /// The start NCI button
        /// </summary>
        public ToolStripItem TheNCIToolStripItem
        {
            get { return nciToolStripButton; }
        }

        /// <summary>
        /// The start Event Viewer button
        /// </summary>
        public ToolStripItem TheEventViewerToolStripItem
        {
            get { return eventViewerToolStripButton; }
        }

        /// <summary>
        /// The Options button
        /// </summary>
        public ToolStripItem TheOptionsToolStripItem
        {
            get { return optionsToolStripButton; }
        }

        /// <summary>
        /// To start window manager button
        /// </summary>
        public ToolStripItem TheWindowManagerToolStripItem
        {
            get { return windowManagerToolStripButton; }
        }

        /// <summary>
        /// The tool strip separator for the connect/disconnect/system tools buttons
        /// </summary>
        public ToolStripSeparator TheSystemsToolStripSeparator
        {
            get { return sytemToolStripSeparator; }
        }

        /// <summary>
        /// The tool strip separator for the Open/Save buttons
        /// </summary>
        public ToolStripSeparator TheOpenSaveToolStripSeparator
        {
            get { return openSaveToolStripSeparator; }
        }

        /// <summary>
        /// The tool strip separator for the Cut/Copy/Paste buttons
        /// </summary>
        public ToolStripSeparator TheCutCopyPasteStripSeparator
        {
            get { return cutCopyPasteToolStripSeparator; }
        }

        /// <summary>
        /// The tool strip separator for the TrafodionManager Tools
        /// </summary>
        public ToolStripSeparator TheToolsStripSeparator
        {
            get { return toolsToolStripSeparator; }
        }

        /// <summary>
        /// The tool strip separator for the navigation buttons
        /// </summary>
        public ToolStripSeparator TheArrowsStripSeparator
        {
            get { return arrowsToolStripSeparator; }
        }

        /// <summary>
        /// The tool strip separator for the window manager button
        /// </summary>
        public ToolStripSeparator TheWindowManagerStripSeparator
        {
            get { return windowManagerToolStripSeparator; }
        }

        /// <summary>
        /// The audit log Viewer button
        /// </summary>
        public ToolStripItem TheAuditLogViewerToolStripItem
        {
            get { return auditLogToolStripButton; }
        }

        /// <summary>
        /// The download OSIM files Viewer button
        /// </summary>
        public ToolStripItem TheDownloadOSIMFilesViewerToolStripItem
        {
            get { return this.downloadOSIMToolStripButton; }
        }

        /// <summary>
        /// The update LDAP Configuration button
        /// </summary>
        public ToolStripItem TheUpdateConfigurationToolStripItem
        {
            get { return this.updateConfigurationToolStripButton; }
        }

        public ToolStripItem TheRunScriptToolStripItem
        {
            get { return this._runScriptToolStripButton; }
        }

        #endregion public properties

        #region Constructor

        /// <summary>
        /// Constructor 
        /// </summary>
        /// <param name="mainMenu"></param>
        public MainToolBar(MainMenu mainMenu)
        {
            InitializeComponent();
            theMainMenu = mainMenu;

            theMainToolBarStrip.Renderer = new TrafodionToolStripRenderer(false);

            RegisterAllEventHandlers();

        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// To hide all of the buttons
        /// </summary>
        public void HideAllButtons()
        {
            SetAllButtonVisible(false);
        }

        /// <summary>
        /// To show all of the buttons
        /// </summary>
        public void ShowAllButtons()
        {
            SetAllButtonVisible(true);
        }

        /// <summary>
        /// To register all default event handlers
        /// </summary>
        public void RegisterAllEventHandlers()
        {
            this.connectToolStripButton.Click += connectToolStripButton_Click;
            this.disconnectToolStripButton.Click += disconnectToolStripButton_Click;
            this.openToolStripButton.Click += openToolStripButton_Click;
            this.saveToolStripButton.Click += saveToolStripButton_Click;
            this.cutToolStripButton.Click += cutToolStripButton_Click;
            this.copyToolStripButton.Click += copyToolStripButton_Click;
            this.pasteToolStripButton.Click += pasteToolStripButton_Click;
            this.helpToolStripButton.Click += helpToolStripButton_Click;
            this.optionsToolStripButton.Click += optionsToolStripButton_Click;
            this.sqlWhiteBoardToolStripButton.Click += sqlWhiteBoardToolStripButton_Click;
            this.nciToolStripButton.Click += nciToolStripButton_Click;
            this.systemToolStripButton.Click += systemToolStripButton_Click;
            this.windowManagerToolStripButton.Click += windowManagerToolStripButton_Click;
            this.mmToolStripButton.Click += mmToolStripButton_Click;
            this.eventViewerToolStripButton.Click += eventViewerToolStripButton_Click;
            this.auditLogToolStripButton.Click += auditLogToolStripButton_Click;
            this.downloadOSIMToolStripButton.Click += downloadOSIMToolStripButton_Click;
            this.downloadOSIMToolStripButton.EnabledChanged += downloadOSIMToolStripButton_EnabledChanged;
            this.updateConfigurationToolStripButton.Click += updateConfigurationToolStripButton_Click;
            this._runScriptToolStripButton.Click += _runScriptToolStripButton_Click;
        }

        /// <summary>
        /// To unregister all of the default event handlers
        /// </summary>
        public void UnRegisterAllEventHandlers()
        {
            this.connectToolStripButton.Click -= connectToolStripButton_Click;
            this.disconnectToolStripButton.Click -= disconnectToolStripButton_Click;
            this.openToolStripButton.Click -= openToolStripButton_Click;
            this.saveToolStripButton.Click -= saveToolStripButton_Click;
            this.cutToolStripButton.Click -= cutToolStripButton_Click;
            this.copyToolStripButton.Click -= copyToolStripButton_Click;
            this.pasteToolStripButton.Click -= pasteToolStripButton_Click;
            this.helpToolStripButton.Click -= helpToolStripButton_Click;
            this.optionsToolStripButton.Click -= optionsToolStripButton_Click;
            this.sqlWhiteBoardToolStripButton.Click -= sqlWhiteBoardToolStripButton_Click;
            this.nciToolStripButton.Click -= nciToolStripButton_Click;
            this.systemToolStripButton.Click -= systemToolStripButton_Click;
            this.windowManagerToolStripButton.Click -= windowManagerToolStripButton_Click;
            this.mmToolStripButton.Click -= mmToolStripButton_Click;
            this.eventViewerToolStripButton.Click -= eventViewerToolStripButton_Click;
            this.auditLogToolStripButton.Click -= auditLogToolStripButton_Click;
            this.downloadOSIMToolStripButton.Click -= downloadOSIMToolStripButton_Click;
            this.updateConfigurationToolStripButton.Click -= updateConfigurationToolStripButton_Click;
            this._runScriptToolStripButton.Click -= _runScriptToolStripButton_Click;
        }

        /// <summary>
        /// To show or hide a specific item in the main tool bar
        /// </summary>
        /// <param name="anItemName"></param>
        /// <param name="aVisible"></param>
        public void SetToolStripItemVisibility(string anItemName, bool aVisible)
        {
            if (this.TheFrameWorkToolBar.Items[anItemName] != null)
            {
                this.TheFrameWorkToolBar.Items[anItemName].Visible = aVisible;
            }
        }

        /// <summary>
        /// To enable or disable a specific item in the main tool bar
        /// </summary>
        /// <param name="anItemName"></param>
        /// <param name="toEnable"></param>
        public void EnableStripItem(string anItemName, bool toEnable)
        {
            if (this.TheFrameWorkToolBar.Items[anItemName] != null)
            {
                this.TheFrameWorkToolBar.Items[anItemName].Enabled = toEnable;
            }
        }

        /// <summary>
        /// The unregister default help handler; this will make room for context sensitive help topic
        /// </summary>
        public void UnRegisterDefaultHelpEventHandler()
        {
            this.TheHelpToolStripItem.Click -= helpToolStripButton_Click;
        }

        #endregion Public methods

        #region Private mthods

        /// <summary>
        /// To turn on/off all of the defined buttons in the tool strip
        /// </summary>
        /// <param name="visible"></param>
        private void SetAllButtonVisible(bool visible)
        {
            this.connectToolStripButton.Visible = visible;
            this.disconnectToolStripButton.Visible = visible;
            this.sytemToolStripSeparator.Visible = visible;
            this.openToolStripButton.Visible = visible;
            this.saveToolStripButton.Visible = visible;
            this.openSaveToolStripSeparator.Visible = visible;
            this.cutToolStripButton.Visible = visible;
            this.copyToolStripButton.Visible = visible;
            this.pasteToolStripButton.Visible = visible;
            this.cutCopyPasteToolStripSeparator.Visible = visible;
            this.optionsToolStripButton.Visible = visible;
            this.sqlWhiteBoardToolStripButton.Visible = visible;
            this.nciToolStripButton.Visible = visible;
            this.systemToolStripButton.Visible = visible;
            this.mmToolStripButton.Visible = visible;
            this.toolsToolStripSeparator.Visible = visible;
            this.windowManagerToolStripButton.Visible = visible;
            this.windowManagerToolStripSeparator.Visible = visible;
            this.parentToolStripButton.Visible = visible;
            this.previousToolStripButton.Visible = visible;
            this.nextToolStripButton.Visible = visible;
            this.arrowsToolStripSeparator.Visible = visible;
            this.helpToolStripButton.Visible = visible;
            this.eventViewerToolStripButton.Visible = false;
            this.auditLogToolStripButton.Visible = false;
            this.downloadOSIMToolStripButton.Visible = false;
            this.updateConfigurationToolStripButton.Visible = false;
            this._runScriptToolStripButton.Visible = false;
        }

        /// <summary>
        /// The default event handler for MM button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void mmToolStripButton_Click(object sender, EventArgs e)
        {
            if (WindowsManager.Exists(TrafodionForm.TitlePrefix + Properties.Resources.MetricMiner))
            {
                WindowsManager.BringToFront(TrafodionForm.TitlePrefix + Properties.Resources.MetricMiner);
            }
            else
            {
                theMainMenu.MetricMinerToolStripMenuItem.PerformClick();
            }
        }

        /// <summary>
        /// The default event handler for Connect button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void connectToolStripButton_Click(object sender, EventArgs e)
        {
            ConnectionDefinition theConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
            if (theConnectionDefinition != null && theConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
            {
                ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);                
                theConnectionDefinitionDialog.Edit(theConnectionDefinition);
            }
        }

        /// <summary>
        /// The default event handler for Disconnect button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void disconnectToolStripButton_Click(object sender, EventArgs e)
        {
            ConnectionDefinition theConnectionDefinition = TrafodionContext.Instance.CurrentConnectionDefinition;
            if (WindowsManager.ClonedWindowsStillOpenForConnection(theConnectionDefinition))
            {
                // Some clone-windows opened
                string confirmMessage = String.Format(Properties.Resources.ClearPasswordMessage, new Object[] { theConnectionDefinition.Name });
                confirmMessage += "\n\n" + Properties.Resources.ClonedWindowsStillOpenMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
                {
                    return;
                }

                // To Close all ManagedWindows associated with this connection def.
                WindowsManager.CloseAllManagedWindowsPerConnection(theConnectionDefinition, true);
                theConnectionDefinition.ClearPassword();
            }
            else
            {
                // No clone-windows opened
                //string confirmMessage = "Do you really want to clear\nthe password for " + TheConnectionDefinition.Name + "?";
                string confirmMessage = String.Format(Properties.Resources.ClearPasswordMessage, new Object[] { theConnectionDefinition.Name });
                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes)
                {
                    return;
                }
                theConnectionDefinition.ClearPassword();
            }
        }

        /// <summary>
        /// The default event handler for Window Manager button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void windowManagerToolStripButton_Click(object sender, EventArgs e)
        {
            WindowsManager.DoActivateWindowsManager();
        }

        /// <summary>
        /// The default event handler for system tools button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void systemToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.TheSystemsToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for NCI button 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void nciToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.NCIToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for SQL Whiteboard button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void sqlWhiteBoardToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.SqlWhiteboardToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Help button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void helpToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.ContentsToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Options button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void optionsToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.OptionsToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Paste button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void pasteToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.ThePasteToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Copy button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void copyToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.TheCopyToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Cut button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void cutToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.TheCutToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Save button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void saveToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.SaveToolStripMenuItem.PerformClick();
        }

        /// <summary>
        /// The default event handler for Open button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void openToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.OpenToolStripMenuItem.PerformClick();
        }

        void eventViewerToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.LiveEventsToolStripMenuItem.PerformClick();
        }

        void auditLogToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.SecurityAuditLogsViewerToolStripMenuItem.PerformClick();
        }

        void downloadOSIMToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.DownloadOSIMDataToolStripMenuItem.PerformClick();
        }

        void downloadOSIMToolStripButton_EnabledChanged(object sender, EventArgs e)
        {
            theMainMenu.DownloadOSIMDataToolStripMenuItem.Enabled = downloadOSIMToolStripButton.Enabled;
        }

        void updateConfigurationToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.LDAPConnectionConfigurationToolStripMenuItem.PerformClick();
        }

        void _runScriptToolStripButton_Click(object sender, EventArgs e)
        {
            theMainMenu.RunScriptToolStripMenuItem.PerformClick();
        }

        private void updateConfigurationToolStripButton_EnabledChanged(object sender, EventArgs e)
        {
            theMainMenu.LDAPConnectionConfigurationToolStripMenuItem.Enabled = this.updateConfigurationToolStripButton.Enabled;
            if (this.updateConfigurationToolStripButton.Enabled)
                theMainMenu.LDAPConnectionConfigurationToolStripMenuItem.Visible = true;
        }

        private void MainToolBar_Load(object sender, EventArgs e)
        {

        }

        #endregion Private methods
    }
}
