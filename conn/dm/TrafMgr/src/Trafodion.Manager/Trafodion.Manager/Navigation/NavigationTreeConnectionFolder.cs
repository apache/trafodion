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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Navigation
{

    /// <summary>
    /// This is the base class for a folder that reperesents a connection definition,
    /// what the user sees in a system.  In order to specialize it, you derive your 
    /// area's system folder from it and drive a factory from NavigationTreeConnectionFolderFactory
    /// which hands out instances of your derived class.  You the pass your derived factory to the
    /// tree via the tree's NavigationTreeConnectionFolderFactory property.  Then, each time the tree
    /// needs to have a system folder, it will get one for your area.
    /// </summary>
    public class NavigationTreeConnectionFolder : NavigationTreeFolder
    {

        private ConnectionDefinition theConnectionDefinition;

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aConnectionDefinition">ition for what the user sees as the system</param>
        public NavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            Text = aConnectionDefinition.Name;
            ImageKey = NavigationTreeView.SERVER_ICON;
            SelectedImageKey = NavigationTreeView.SERVER_ICON;

            theConnectionDefinition = aConnectionDefinition;
            ConnectionDefinition.Changed += ConnectionDefinitionChanged;
        }

        public void RemoveHandlers()
        {
            ConnectionDefinition.Changed -= ConnectionDefinitionChanged;
        }

        override public ConnectionDefinition TheConnectionDefinition
        {
            get { return theConnectionDefinition; }
            set { theConnectionDefinition = value; }
        }

        void ConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if ((aConnectionDefinition == TheConnectionDefinition) && (aReason == ConnectionDefinition.Reason.Name))
            {
                Text = ShortDescription;
            }
            if ((aConnectionDefinition == TheConnectionDefinition) && (aReason == ConnectionDefinition.Reason.Password))
            {
                TheConnectionDefinition.SuppressEvents();
                TheConnectionDefinition.Password = aConnectionDefinition.Password;
                TheConnectionDefinition.AllowEvents();
            }
        }

        protected override void PrepareForPopulate()
        {
            
        }

        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
        }

        public void AddCommonContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            // Add menu items:  Add To Favorites and Refresh
            base.AddToContextMenu(aContextMenuStrip);

            // Add menu item: Disconnect 
            if (TheConnectionDefinition.PasswordIsSet && TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                ToolStripMenuItem theClearPasswordMenuItem = new ToolStripMenuItem(Properties.Resources.ClearPasswordMenuText);
                theClearPasswordMenuItem.Click += new EventHandler(TheClearPasswordMenuItemClick);
                aContextMenuStrip.Items.Add(theClearPasswordMenuItem);
            }
        }

        public override void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            if (TheConnectionDefinition.PasswordIsSet && TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                ToolStripMenuItem theClearPasswordMenuItem = new ToolStripMenuItem(Properties.Resources.ClearPasswordMenuText);
                theClearPasswordMenuItem.Click += new EventHandler(TheClearPasswordMenuItemClick);
                aContextMenuStrip.Items.Add(theClearPasswordMenuItem);

                //ToolStripMenuItem theChangePasswordMenuItem = new ToolStripMenuItem(Properties.Resources.ChangePasswordMenuText);
                //theChangePasswordMenuItem.Click += new EventHandler(TheChangePasswordMenuItemClick);
                //aContextMenuStrip.Items.Add(theChangePasswordMenuItem);

                ToolStripMenuItem theEditSystemMenuItem = new ToolStripMenuItem(Properties.Resources.EditSystemMenuText);
                theEditSystemMenuItem.Click += new EventHandler(TheConnectSystemMenuItemClick);
                aContextMenuStrip.Items.Add(theEditSystemMenuItem);
            }

            if (TheConnectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)
            {
                //ToolStripMenuItem theConnectSystemMenuItem = new ToolStripMenuItem(Properties.Resources.ConnectSystemMenuText);
                ToolStripMenuItem theConnectSystemMenuItem = new ToolStripMenuItem("Connect ...");
                theConnectSystemMenuItem.Click += new EventHandler(TheConnectSystemMenuItemClick);
                aContextMenuStrip.Items.Add(theConnectSystemMenuItem);

                if (TrafodionContext.Instance.TheTrafodionMain.ActiveAreaName.Contains("Monitoring"))
                {
                    //if (TheConnectionDefinition.TheState != ConnectionDefinition.State.LiveFeedTestSucceeded)
                    //{
                    //    ToolStripMenuItem theLiveFeedConnectSystemMenuItem = new ToolStripMenuItem("Live Feed Only Connect ...");
                    //    theLiveFeedConnectSystemMenuItem.Click += new EventHandler(TheConnectSystemMenuItemClick);
                    //    aContextMenuStrip.Items.Add(theLiveFeedConnectSystemMenuItem);
                    //}
                    //else
                    //{
                    //    ToolStripMenuItem theLiveFeedConnectSystemMenuItem = new ToolStripMenuItem("Live Feed Only Disconnect");
                    //    theLiveFeedConnectSystemMenuItem.Click += new EventHandler(TheClearPasswordMenuItemClick);
                    //    aContextMenuStrip.Items.Add(theLiveFeedConnectSystemMenuItem);
                    //}
                    if (TheConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded) 
                    {
                        ToolStripMenuItem theLiveFeedConnectSystemMenuItem = new ToolStripMenuItem("Live Feed Disconnect");
                        theLiveFeedConnectSystemMenuItem.Click += new EventHandler(TheClearPasswordMenuItemClick);
                        aContextMenuStrip.Items.Add(theLiveFeedConnectSystemMenuItem);
                    }

                }

                ToolStripMenuItem theEditSystemMenuItem = new ToolStripMenuItem(Properties.Resources.EditSystemMenuText);
                theEditSystemMenuItem.Click += new EventHandler(TheConnectSystemMenuItemClick);
                aContextMenuStrip.Items.Add(theEditSystemMenuItem);
            }
            else
            {
                ToolStripMenuItem theTestSystemMenuItem = new ToolStripMenuItem(Properties.Resources.TestSystemMenuText);
                theTestSystemMenuItem.Click += new EventHandler(TheTestSystemMenuItemClick);
                aContextMenuStrip.Items.Add(theTestSystemMenuItem);
            }

            ToolStripMenuItem theAddSystemLikeMenuItem = new ToolStripMenuItem(Properties.Resources.AddSystemLikeMenuText);
            theAddSystemLikeMenuItem.Click += new EventHandler(TheAddSystemLikeMenuItemClick);
            aContextMenuStrip.Items.Add(theAddSystemLikeMenuItem);

            ToolStripMenuItem theRemoveSystemMenuItem = new ToolStripMenuItem(Properties.Resources.RemoveSystemMenuText);
            theRemoveSystemMenuItem.Click += new EventHandler(TheRemoveSystemMenuItemClick);
            aContextMenuStrip.Items.Add(theRemoveSystemMenuItem);

            if (TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            {
                //aContextMenuStrip.Items.Add(new ToolStripSeparator());
                // event viewer ...
                ToolStripMenuItem theTextEventsMenuItem = new ToolStripMenuItem(Properties.Resources.TextEvents);
                ToolStripMenuItem theLiveMenuItem = new ToolStripMenuItem(Properties.Resources.LiveEvents);
                theLiveMenuItem.Click += new EventHandler(theLiveEventsMenuItem_Click);
                theTextEventsMenuItem.DropDownItems.Add(theLiveMenuItem);
                ToolStripMenuItem theReposMenuItem = new ToolStripMenuItem(Properties.Resources.RepositoryEvents);
                theReposMenuItem.Click += new EventHandler(theRepositoryEventsMenuItem_Click);
                theTextEventsMenuItem.DropDownItems.Add(theReposMenuItem);
                //aContextMenuStrip.Items.Add(theTextEventsMenuItem);

                //Security audit logs
                if (TheConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                {
                    ToolStripMenuItem theSecurityAuditLogsMenuItem = new ToolStripMenuItem(global::Trafodion.Manager.Properties.Resources.SecurityAuditLogs);
                    ToolStripMenuItem theSecurityAuditLogsViewerMenuItem = new ToolStripMenuItem(global::Trafodion.Manager.Properties.Resources.SecurityAuditLogsViewer);
                    theSecurityAuditLogsViewerMenuItem.Click += new EventHandler(theRepositoryAuditLogsMenuItem_Click);
                    ToolStripMenuItem theAuditLogConfigurationMenuItem = new ToolStripMenuItem(global::Trafodion.Manager.Properties.Resources.SecurityAuditLogsConfig);
                    theAuditLogConfigurationMenuItem.Click += new EventHandler(theAuditLogConfigurationMenuItem_Click);
                    theSecurityAuditLogsMenuItem.DropDownItems.Add(theSecurityAuditLogsViewerMenuItem);
                    theSecurityAuditLogsMenuItem.DropDownItems.Add(theAuditLogConfigurationMenuItem);
                    //aContextMenuStrip.Items.Add(theSecurityAuditLogsMenuItem);
                    theSecurityAuditLogsViewerMenuItem.Enabled = true;
                    if (!TheConnectionDefinition.ComponentPrivilegeExists("AUDIT_LOGGING", "UPDATE_CONFIGURATION"))
                    {
                        theAuditLogConfigurationMenuItem.Enabled = false;
                    }
                    else
                    {
                        theAuditLogConfigurationMenuItem.Enabled = true;
                    }
                }
            }
            else
            {
                ToolStripMenuItem theLiveEventMenuItem = new ToolStripMenuItem(Properties.Resources.LiveEvents);
                theLiveEventMenuItem.Click += new EventHandler(theLiveEventsMenuItem_Click);
                //aContextMenuStrip.Items.Add(theLiveEventMenuItem);
            }

        }


        void theDialOutEventsMenuItem_Click(object sender, EventArgs e)
        {            
            try
            {
                BrowserUtilities.LaunchDialOutEvents(TheConnectionDefinition);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message);
            }
        }

        void theDatabaseEventsMenuItem_Click(object sender, EventArgs e)
        {            
            try
            {
                BrowserUtilities.LaunchDatabaseEvents(TheConnectionDefinition);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message);
            }
        }

        void theRepositoryAuditLogsMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                TrafodionContext.Instance.TheTrafodionMain.LaunchAuditLogViewer(TheConnectionDefinition, false);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message);
            }
        }

        void theAuditLogConfigurationMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                TrafodionContext.Instance.TheTrafodionMain.LaunchAuditLogConfig(TheConnectionDefinition);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message);
            }
        }

        void theRepositoryEventsMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                TrafodionContext.Instance.TheTrafodionMain.LaunchEventViewer(TheConnectionDefinition, false);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message);
            }
        }

        void theLiveEventsMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                TrafodionContext.Instance.TheTrafodionMain.LaunchEventViewer(TheConnectionDefinition, true);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message);
            }
        }

        void TheClearPasswordMenuItemClick(object sender, EventArgs e)
        {

            if (WindowsManager.ClonedWindowsStillOpenForConnection(TheConnectionDefinition))
            {
                // Some clone-windows opened
                string confirmMessage = String.Format(Properties.Resources.ClearPasswordMessage, new Object[] { TheConnectionDefinition.Name });
                confirmMessage +=  "\n\n" + Properties.Resources.ClonedWindowsStillOpenMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                {
                    return;
                }

                // To Close all ManagedWindows associated with this connection def.
                WindowsManager.CloseAllManagedWindowsPerConnection(TheConnectionDefinition, true);
                TheConnectionDefinition.ClearPassword();
            }
            else
            {
                // No clone-windows opened
                //string confirmMessage = "Do you really want to clear\nthe password for " + TheConnectionDefinition.Name + "?";
                string confirmMessage = String.Format(Properties.Resources.ClearPasswordMessage, new Object[] { TheConnectionDefinition.Name });
                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                {
                    return;
                }
                TheConnectionDefinition.ClearPassword();
            }
        }


        void TheChangePasswordMenuItemClick(object sender, EventArgs e)
        {
            Trafodion.Manager.Connections.Controls.ChangePasswordDialog theChangePasswordDialog = 
                new Trafodion.Manager.Connections.Controls.ChangePasswordDialog(TheConnectionDefinition, true);
            theChangePasswordDialog.ShowDialog();
        }

        void TheAddSystemLikeMenuItemClick(object sender, EventArgs e)
        {
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog();
            theConnectionDefinitionDialog.NewLike(TheConnectionDefinition);
        }

        void TheConnectSystemMenuItemClick(object sender, EventArgs e)
        {
            //// if connection is still active, user cannot edit the connection
            //if (TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
            //{
            //    MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ActiveSystemEditWarning, Properties.Resources.Warning, MessageBoxButtons.OK);
            //    return;
            //}

            //Close any cloned windows that use this connection
            WindowsManager.CloseAllManagedWindowsPerConnection(TheConnectionDefinition, false);

            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog(false);
            theConnectionDefinitionDialog.Edit(TheConnectionDefinition);
        }

        /// <summary>
        /// Gets called when user clicks "Remove System" from the popup menu on a System
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void TheRemoveSystemMenuItemClick(object sender, EventArgs e)
        {

            if (WindowsManager.ClonedWindowsStillOpenForConnection(TheConnectionDefinition))
            {
                // Some clone-windows opened message 
                string confirmMessage = String.Format(Properties.Resources.RemoveSystemMessage, new Object[] { TheConnectionDefinition.Name });
                confirmMessage += "\n\n" + Properties.Resources.ClonedWindowsStillOpenMessage;

                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                {
                    return;
                }

                // To Close all ManagedWindows associated with this connection def.
                WindowsManager.CloseAllManagedWindowsPerConnection(TheConnectionDefinition, true);
                TheConnectionDefinition.Remove(this);
            }
            else
            {
                // No clone-windows opened message 
                string confirmMessage = String.Format(Properties.Resources.RemoveSystemMessage, new Object[] { TheConnectionDefinition.Name });
                if (MessageBox.Show(Utilities.GetForegroundControl(), confirmMessage, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                {
                    return;
                }
                TheConnectionDefinition.Remove(this);
            }

        }

        void TheTestSystemMenuItemClick(object sender, EventArgs e)
        {
            TheConnectionDefinition.DoTestOnly();
        }

        public override string ShortDescription
        {
            get { return theConnectionDefinition.Name; }
        }

        public override string LongerDescription
        {
            get { return theConnectionDefinition.Description; }
        }
        public override string FavoritesImageKey
        {
            get
            {
                return NavigationTreeView.SERVER_ICON;
            }
        }
    }
}
