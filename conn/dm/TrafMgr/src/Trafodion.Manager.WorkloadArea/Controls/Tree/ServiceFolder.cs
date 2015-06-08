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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls.Tree
{
    /// <summary>
    /// Tree node wrapping a WmsService
    /// </summary>
    public class ServiceFolder : WmsTreeNode
    {
        /// <summary>
        /// Get the underlying service
        /// </summary>
        public WmsService WmsService
        {
            get { return WmsObject as WmsService; }
        }

        /// <summary>
        /// Constructor for a Service Folder
        /// </summary>
        /// <param name="aWmsService"></param>
        public ServiceFolder(WmsService aWmsService)
            :base(aWmsService)
        {    
        }

        /// <summary>
        /// Refresh this node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            base.Refresh(aNavigationTreeNameFilter);

            WmsService.Refresh();
        }

        /// <summary>
        /// Adds context menu items for the service folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            ToolStripMenuItem deleteServiceMenuItem = new ToolStripMenuItem(Properties.Resources.DeleteService);
            deleteServiceMenuItem.Tag = this;
            deleteServiceMenuItem.Click += new EventHandler(deleteServiceMenuItem_Click);
            aContextMenuStrip.Items.Add(deleteServiceMenuItem);

            ToolStripMenuItem startServiceMenuItem = new ToolStripMenuItem(Properties.Resources.StartService);
            startServiceMenuItem.Tag = this;
            startServiceMenuItem.Click += new EventHandler(startServiceMenuItem_Click);
            aContextMenuStrip.Items.Add(startServiceMenuItem);

            ToolStripMenuItem stopServiceMenuItem = new ToolStripMenuItem(Properties.Resources.StopService);
            stopServiceMenuItem.Tag = this;
            stopServiceMenuItem.Click += new EventHandler(stopServiceMenuItem_Click);
            aContextMenuStrip.Items.Add(stopServiceMenuItem);

            //startServiceMenuItem.Enabled = stopServiceMenuItem.Enabled = deleteServiceMenuItem.Enabled = false;
            //if (WmsService.State.Equals(WmsService.ACTIVE))
            //{
            //    stopServiceMenuItem.Enabled = true;
            //}
            //else if (WmsService.State.Equals(WmsService.STOPPED))
            //{
            //    startServiceMenuItem.Enabled = true;
            //    deleteServiceMenuItem.Enabled = true;
            //}

            stopServiceMenuItem.Enabled = WmsService.State.Equals(WmsService.ACTIVE) 
                && WmsService.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STOP.ToString());
            startServiceMenuItem.Enabled = WmsService.State.Equals(WmsService.STOPPED)
                && WmsService.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_START.ToString());
            deleteServiceMenuItem.Enabled = WmsService.State.Equals(WmsService.STOPPED)
                && WmsService.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_DELETE.ToString());

            // Only add hold and release menu items for services users
            //if (WmsService.WmsSystem.ConnectionDefinition.IsServicesUser)
            //{
                ToolStripMenuItem holdServiceMenuItem = new ToolStripMenuItem(Properties.Resources.HoldService);
                holdServiceMenuItem.Tag = this;
                holdServiceMenuItem.Click += new EventHandler(holdServiceMenuItem_Click);

                ToolStripMenuItem releaseServiceMenuItem = new ToolStripMenuItem(Properties.Resources.ReleaseService);
                releaseServiceMenuItem.Tag = this;
                releaseServiceMenuItem.Click += new EventHandler(releaseServiceMenuItem_Click);

                holdServiceMenuItem.Enabled = WmsService.State.Equals(WmsService.ACTIVE)
                && WmsService.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_HOLD.ToString());

                releaseServiceMenuItem.Enabled = WmsService.State.Equals(WmsService.HOLD)
                && WmsService.WmsSystem.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_RELEASE.ToString());

                aContextMenuStrip.Items.Add(holdServiceMenuItem);
                aContextMenuStrip.Items.Add(releaseServiceMenuItem);
                //if (WmsService.State.Equals(WmsService.ACTIVE))
                //{
                //    holdServiceMenuItem.Enabled = true;
                //    releaseServiceMenuItem.Enabled = false;
                //}
                //else if (WmsService.State.Equals(WmsService.HOLD))
                //{
                //    holdServiceMenuItem.Enabled = false;
                //    releaseServiceMenuItem.Enabled = true;
                //}
                //else if (WmsService.State.Equals(WmsService.STOPPED))
                //{
                //    holdServiceMenuItem.Enabled = false;
                //    releaseServiceMenuItem.Enabled = false;
                //}

                
            //}
        }

        /// <summary>
        /// StartService Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void startServiceMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WmsService.Start();
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Refresh the services list
                WmsService.WmsSystem.WmsServices = null;

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
        /// StopService Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void stopServiceMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WmsService.Stop(false);
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Refresh the services list
                WmsService.WmsSystem.WmsServices = null;

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
        /// Delete Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void deleteServiceMenuItem_Click(object sender, EventArgs e)
        {
            if (MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.ConfirmDeleteWMS, Properties.Resources.ConfirmTitle, MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
                try
                {
                    WmsService.Delete(false);
                    if (this.TreeView.SelectedNode.Equals(this))
                    {
                        TreeView.SelectedNode = Parent;
                    }
                }
                catch (System.Data.Odbc.OdbcException oe)
                {
                    // Refresh the services list
                    WmsService.WmsSystem.WmsServices = null;
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

        /// <summary>
        /// Hold Menu Item Handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void holdServiceMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WmsService.Hold();
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Refresh the services list
                WmsService.WmsSystem.WmsServices = null;

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
        /// Release Serivce Menu Item Handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void releaseServiceMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                WmsService.Release();
            }
            catch (System.Data.Odbc.OdbcException oe)
            {
                // Refresh the services list
                WmsService.WmsSystem.WmsServices = null;

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
