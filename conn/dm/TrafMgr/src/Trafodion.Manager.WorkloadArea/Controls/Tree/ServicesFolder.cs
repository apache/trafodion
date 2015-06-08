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
    /// Represents the services folder in the navigation tree
    /// </summary>
    public class ServicesFolder : WmsTreeFolder
    {

        #region Properties

        /// <summary>
        /// Returns the WMS System associated with this folder
        /// </summary>
        public WmsSystem WmsSystem
        {
            get { return WmsObject as WmsSystem; }
        }

        #endregion Properties


        /// <summary>
        /// Default constructor for the services folder
        /// </summary>
        /// <param name="aWmsSystem"></param>
        public ServicesFolder(WmsSystem aWmsSystem)
            :base(aWmsSystem, true)
        {
        }
        
        /// <summary>
        /// Populates this tree node
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
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
                return Properties.Resources.Services;                
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

            ToolStripMenuItem addServiceMenuItem = new ToolStripMenuItem(Properties.Resources.AddService);
            addServiceMenuItem.Tag = this;
            addServiceMenuItem.Click += new EventHandler(addServiceMenuItem_Click);
            aContextMenuStrip.Items.Add(addServiceMenuItem);

            ToolStripMenuItem holdAllServicesMenuItem = new ToolStripMenuItem(Properties.Resources.HoldAllServices);
            holdAllServicesMenuItem.Tag = this;
            holdAllServicesMenuItem.Click += new EventHandler(holdAllServicesMenuItem_Click);

            ToolStripMenuItem releaseAllServicesMenuItem = new ToolStripMenuItem(Properties.Resources.ReleaseAllServices);
            releaseAllServicesMenuItem.Tag = this;
            releaseAllServicesMenuItem.Click += new EventHandler(releaseAllServicesMenuItem_Click);

            //addServiceMenuItem.Enabled = holdAllServicesMenuItem.Enabled = releaseAllServicesMenuItem.Enabled = WmsSystem.IsServicesLoaded;
            addServiceMenuItem.Enabled = WmsSystem.IsServicesLoaded && WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ADD.ToString());
            holdAllServicesMenuItem.Enabled=WmsSystem.IsServicesLoaded && WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());
            releaseAllServicesMenuItem.Enabled = WmsSystem.IsServicesLoaded && WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());

            // Add to the context menu
            aContextMenuStrip.Items.Add(holdAllServicesMenuItem);
            aContextMenuStrip.Items.Add(releaseAllServicesMenuItem);
        }

        /// <summary>
        /// Handles the release all context menu click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void releaseAllServicesMenuItem_Click(object sender, EventArgs e)
        {
            if (null == WmsSystem) return;

            DialogResult result = MessageBox.Show(  Utilities.GetForegroundControl(),
                                                    "\nAre you sure that you wish to release all services?",
                                                    TrafodionForm.TitlePrefix + "Release All Services",
                                                    MessageBoxButtons.YesNo, MessageBoxIcon.Question);

            if (result == DialogResult.Yes)
            {
                Cursor.Current = Cursors.WaitCursor;
                try
                {
                    WmsSystem.ReleaseAllServices();
                    DoRefresh(null);
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
                finally
                {
                    Cursor.Current = Cursors.Default;
                }
            }
        }

        /// <summary>
        /// Handles the hold all context menu item click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void holdAllServicesMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                DialogResult result = MessageBox.Show("\nAre you sure that you wish to place all services on hold?",
                                                TrafodionForm.TitlePrefix + "Hold All Services", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
                if (result == DialogResult.Yes)
                {
                    WmsSystem.HoldAllServices();
                    DoRefresh(null);
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

        /// <summary>
        /// Handles when the user clicks on the add service context menu item
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public void addServiceMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WMSEditServiceDialog theAddServiceDialog = new WMSEditServiceDialog(WmsCommand.WMS_ACTION.ADD_SERVICE, WmsSystem, null);
                if (theAddServiceDialog.ShowDialog() == DialogResult.OK)
                {
                    DoRefresh(null);
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
