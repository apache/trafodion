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
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Model;
using System.Windows.Forms;
using System;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Represents the services folder in the navigation tree
    /// </summary>
    public class ConfigurationFolder : WmsTreeFolder
    {
        private ServicesFolder _servicesFolder;
        private RulesFolder _wmsRulesFolder;

        #region Properties

        /// <summary>
        /// Returns the WMS System associated with this folder
        /// </summary>
        public WmsSystem WmsSystem
        {
            get { return WmsObject as WmsSystem; }
            set
            {
                WmsObject = WmsSystem;
                DoRefresh(null);
            }
        }

        public override Trafodion.Manager.Framework.Connections.ConnectionDefinition TheConnectionDefinition
        {
            get
            {
                return WmsSystem.ConnectionDefinition;
            }
            set
            {
                ;
            }
        }
        #endregion Properties


        /// <summary>
        /// Default constructor for the services folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public ConfigurationFolder(WmsSystem aWmsSystem)
            :base(aWmsSystem, false)
        {
            Text = ShortDescription;
        }
        
        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
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

        /// <summary>
        /// Refreshes the list of services
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            WmsSystem.WmsServices = null;
        }

        /// <summary>
        /// Returns the name displayed in the navigation tree
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return "Configuration";
            }
        }

        /// <summary>
        /// Returns the longer description
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

            aContextMenuStrip.Items.Add(new ToolStripSeparator());

            ToolStripMenuItem startWMSMenuItem = new ToolStripMenuItem(Properties.Resources.StartService);
            startWMSMenuItem.Tag = this;
            startWMSMenuItem.Click += new EventHandler(startWMSMenuItem_Click);
            if (WmsSystem.State.Equals(WmsCommand.ACTIVE_STATE))
            {
                startWMSMenuItem.Enabled = false;
            }
            
            ToolStripMenuItem stopWMSMenuItem = new ToolStripMenuItem(Properties.Resources.StopService);
            stopWMSMenuItem.Tag = this;
            stopWMSMenuItem.Click += new EventHandler(stopWMSMenuItem_Click);
            if (WmsSystem.State.Equals(WmsCommand.STOPPED_STATE))
            {
                stopWMSMenuItem.Enabled = false;
            }
            
            ToolStripMenuItem holdWMSMenuItem = new ToolStripMenuItem(Properties.Resources.HoldService);
            holdWMSMenuItem.Tag = this;
            holdWMSMenuItem.Click += new EventHandler(holdWMSMenuItem_Click);
            if (WmsSystem.State.Equals(WmsCommand.HOLD_STATE))
            {
                holdWMSMenuItem.Enabled = false;
            }

            ToolStripMenuItem releaseWMSMenuItem = new ToolStripMenuItem(Properties.Resources.ReleaseService);
            releaseWMSMenuItem.Tag = this;
            releaseWMSMenuItem.Click += new EventHandler(releaseWMSMenuItem_Click);
            if (WmsSystem.State.Equals(WmsCommand.ACTIVE_STATE))
            {
                releaseWMSMenuItem.Enabled = false;
            }

            // Add to the context menu
            aContextMenuStrip.Items.Add(startWMSMenuItem);
            aContextMenuStrip.Items.Add(stopWMSMenuItem);
            aContextMenuStrip.Items.Add(holdWMSMenuItem);
            aContextMenuStrip.Items.Add(releaseWMSMenuItem);

            ToolStripMenuItem generateScriptMenuItem = new ToolStripMenuItem(Properties.Resources.ExportConfiiguration);
            generateScriptMenuItem.Tag = this.WmsSystem;
            generateScriptMenuItem.Click += new EventHandler(generateScriptMenuItem_Click);
            aContextMenuStrip.Items.Add(generateScriptMenuItem);
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

        void holdWMSMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                DialogResult result = MessageBox.Show("\nAre you sure you want to put service(s) on hold?",
                                                      TrafodionForm.TitlePrefix + "Hold Service(s)", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    WmsSystem.HoldWMS();
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
    }
}
