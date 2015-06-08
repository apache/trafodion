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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Navigation tree folder representing a system
    /// </summary>
    public class SystemFolder : NavigationTreeConnectionFolder
    {
        #region Fields

        private WmsSystem _wmsSystem;
        private ServicesFolder _servicesFolder;
        private RulesFolder _wmsRulesFolder;

        #endregion Fields

        #region Properties

        public WmsSystem WmsSystem
        {
            get { return _wmsSystem; }
            set { _wmsSystem = value; }
        }

        #endregion Properties

        /// <summary>
        /// Constructor for System Folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public SystemFolder(WmsSystem aWmsSystem)
            : base(aWmsSystem.ConnectionDefinition)
        {
            _wmsSystem = aWmsSystem;
            Text = ShortDescription;
            _wmsSystem.Reset(); //If wms system is reloaded from cache, this makes sure we refresh the state.
        }

        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            if (TheConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
            {
                //Tree Nodes are added only users have "ADMIN_STATUS" privilege
                if (TheConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
                {
                    Nodes.Clear();
                    if (_servicesFolder == null)
                    {
                        _servicesFolder = new ServicesFolder(WmsSystem);
                    }
                    if (_wmsRulesFolder == null)
                    {
                        _wmsRulesFolder = new RulesFolder(WmsSystem);
                    }
                    Nodes.Add(_servicesFolder);
                    Nodes.Add(_wmsRulesFolder);
                } 
            }
        }

        /// <summary>
        /// Refresh this node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            try
            {
                _wmsSystem.Refresh();
            }
            catch (Exception ex)
            {
                //Exception is already handled in the right pane.
            }
        }

        /// <summary>
        /// Short Description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return TheConnectionDefinition.Name;
            }
        }
        
        /// <summary>
        /// Longer Description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return ShortDescription;
            }
        }

        /// <summary>
        /// Adds items to the context menu for the services folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            try
            {
                if (this.TheConnectionDefinition != null && TheConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
                {
                    aContextMenuStrip.Items.Add(new ToolStripSeparator());

                    ToolStripMenuItem startWMSMenuItem = new ToolStripMenuItem(Properties.Resources.StartWMS);
                    startWMSMenuItem.Tag = this;
                    startWMSMenuItem.Click += new EventHandler(startWMSMenuItem_Click);
                    ToolStripMenuItem stopWMSMenuItem = new ToolStripMenuItem(Properties.Resources.StopWMS);
                    stopWMSMenuItem.Tag = this;
                    stopWMSMenuItem.Click += new EventHandler(stopWMSMenuItem_Click);

                    ToolStripMenuItem holdWMSMenuItem = new ToolStripMenuItem(Properties.Resources.HoldWMS);
                    holdWMSMenuItem.Tag = this;
                    holdWMSMenuItem.Click += new EventHandler(holdWMSMenuItem_Click);

                    ToolStripMenuItem releaseWMSMenuItem = new ToolStripMenuItem(Properties.Resources.ReleaseWMS);
                    releaseWMSMenuItem.Tag = this;
                    releaseWMSMenuItem.Click += new EventHandler(releaseWMSMenuItem_Click);

                    holdWMSMenuItem.Enabled = WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString()) && WmsSystem.State.Equals(WmsCommand.ACTIVE_STATE);
                    releaseWMSMenuItem.Enabled = WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString()) && WmsSystem.State.Equals(WmsCommand.HOLD_STATE);
                    stopWMSMenuItem.Enabled = WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STOP.ToString()) && !WmsSystem.State.Equals(WmsCommand.STOPPED_STATE);
                    startWMSMenuItem.Enabled = WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_START.ToString()) && WmsSystem.State.Equals(WmsCommand.STOPPED_STATE);

                    // Add to the context menu
                    aContextMenuStrip.Items.Add(startWMSMenuItem);
                    aContextMenuStrip.Items.Add(stopWMSMenuItem);
                    aContextMenuStrip.Items.Add(holdWMSMenuItem);
                    aContextMenuStrip.Items.Add(releaseWMSMenuItem);

                    ToolStripMenuItem generateScriptMenuItem = new ToolStripMenuItem(Properties.Resources.ExportConfiiguration);
                    generateScriptMenuItem.Tag = this.WmsSystem;
                    generateScriptMenuItem.Click += new EventHandler(generateScriptMenuItem_Click);

                    generateScriptMenuItem.Enabled = WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString());

                    aContextMenuStrip.Items.Add(generateScriptMenuItem);

                }
            }
            catch 
            {
                // CR ALMCR58A6738 - User cannot disconnect/remove the active system by right-click menu item when the system is not available.
                // Catch and ignore the error happening when WMS tries to get data but server has stopped connection service. 
                // If not, this error will cause that the Delete/Disconnect context menu cannot be shown,
                // so that users cannot Delete/Disconnect the system when service is not available
            } 

        }
        /// <summary>
        /// Handle the Generate Script menu and generate the configuration script
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void generateScriptMenuItem_Click(object sender, EventArgs e)
        {
            // Get the WmsSystem model from the node that was selected.
            WmsSystem wmsSystem = (WmsSystem)((ToolStripMenuItem)sender).Tag;
            if (wmsSystem != null)
            {
                //Pass the source wmssystem from which the menu was clicked, to the ExportConfigControl constructor
                ExportConfigControl exportConfigControl = new ExportConfigControl(wmsSystem);

                //Place the ExportConfigControl user control into a managed window
                Trafodion.Manager.Framework.Controls.WindowsManager.PutInWindow(exportConfigControl.Size, exportConfigControl, Properties.Resources.ExportConfiiguration + " - " + wmsSystem.ConnectionDefinition.Name, wmsSystem.ConnectionDefinition);
            }
        }

        void startWMSMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WmsSystem.StartWMS();
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }
        }

        void stopWMSMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WMSCommandStopConfirmationDialog wmsStopDialog = new WMSCommandStopConfirmationDialog(true);

                if (DialogResult.Yes == wmsStopDialog.ShowDialog())
                {
                    WmsSystem.StopWMS(wmsStopDialog.StopImmediately);
                }
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Got an ODBC erorr. Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), oe.Message, Properties.Resources.ODBCError, MessageBoxButtons.OK);
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SystemError, MessageBoxButtons.OK);
            }
        }

        void releaseWMSMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WmsSystem.ReleaseWMS();
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, "Error", MessageBoxButtons.OK);
            }
        }

        void holdWMSMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                DialogResult result = MessageBox.Show(Properties.Resources.ConfirmHoldWMS,
                                                      TrafodionForm.TitlePrefix + Properties.Resources.HoldWMS, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    WmsSystem.HoldWMS();
                }
            }
            catch (Exception ex)
            {
                // Got some other exception.  Show it.
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, "Error", MessageBoxButtons.OK);
            }
        }
    }
}
