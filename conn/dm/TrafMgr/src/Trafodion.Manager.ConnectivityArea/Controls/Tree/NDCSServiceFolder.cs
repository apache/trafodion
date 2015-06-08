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
using System.Text;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.ConnectivityArea.Model;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.ConnectivityArea.Controls.Tree
{
    /// <summary>
    /// Tree node wrapping a NDCSService object
    /// </summary>
    public class NDCSServiceFolder : ConnectivityTreeNode
    {
        #region Private member variables

        ToolStripMenuItem startNDCSServiceMenuItem;
        ToolStripMenuItem stopNDCSServiceMenuItem;

        #endregion Private member variables


        /// <summary>
        /// Get the underlying service
        /// </summary>
        public NDCSService NdcsService
        {
            get { return NDCSObject as NDCSService; }
        }

        /// <summary>
        /// Constructor for a Service Folder
        /// </summary>
        /// <param name="aNdcsService"></param>
        public NDCSServiceFolder(NDCSService aNdcsService)
            :base(aNdcsService)
        {
            InitializeComponent();
            ImageKey = ConnectivityTreeView.SERVICES_ICON;
            SelectedImageKey = ConnectivityTreeView.SERVICES_ICON;
        }

        /// <summary>
        /// Refresh this node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            NdcsService.Refresh();
        }


        /// <summary>
        /// Adds context menu items for the service folder
        /// </summary>
        /// <param name="aContextMenuStrip"></param>
        public override void AddToContextMenu(Trafodion.Manager.Framework.Controls.TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);

            TreeView.SelectedNode = this;            

            // only Admin user can start/stop NDCS Services
            if (NdcsService.AssociationServer != null)
            {
                if ((NdcsService.AssociationServer.ServerState == NDCSServer.ServerStateEnum.SRVR_STOPPING) ||
                    (NdcsService.AssociationServer.ServerState == NDCSServer.ServerStateEnum.SRVR_STOPPED) ||
                    (NdcsService.AssociationServer.ServerState == NDCSServer.ServerStateEnum.SRVR_STOP_WHEN_DISCONNECTED))
                {
                    startNDCSServiceMenuItem.Enabled = true;
                    stopNDCSServiceMenuItem.Enabled = false;
                }
                else
                {
                    startNDCSServiceMenuItem.Enabled = false;
                    stopNDCSServiceMenuItem.Enabled = true;
                }

                if (TheConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_START"))
                {
                    aContextMenuStrip.Items.Add(startNDCSServiceMenuItem);
                }
                if(TheConnectionDefinition.ComponentPrivilegeExists("HPDCS", "ADMIN_STOP"))
                {
                    aContextMenuStrip.Items.Add(stopNDCSServiceMenuItem);
                }
            }
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
                NdcsService.Start();
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
                TreeView.SelectedNode = this;
            }

        }

        /// <summary>
        /// Start Service Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void startNDCSServiceMenuItem_Click(object sender, EventArgs e)
        {
            try
            {
                Cursor.Current = Cursors.WaitCursor;
                try
                {
                    NdcsService.Start();
                }
                finally
                {
                    Cursor.Current = Cursors.Default;
                    TreeView.SelectedNode = this;
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
            finally
            {
                refreshRightPane();
            }

        }

        /// <summary>
        /// Stop Menu Item handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void stopNDCSServiceMenuItem_Click(object sender, EventArgs e)
        {
            StopNDCSServiceDialog theStopNDCSServiceDialog;
            NDCSObject.StopMode theStopMode;

            int theConnectedCount = 0;
            string theStopReason;
            try
            {
                theStopNDCSServiceDialog = new StopNDCSServiceDialog(this.Text, theConnectedCount);
                if (theStopNDCSServiceDialog.ShowDialog() == DialogResult.OK)
                {
                    theStopReason = theStopNDCSServiceDialog.TheReason;
                    if (theStopNDCSServiceDialog.StopMode == 0)
                    {
                        theStopMode = NDCSObject.StopMode.STOP_IMMEDIATE;
                    }
                    else
                    {
                        theStopMode = NDCSObject.StopMode.STOP_DISCONNECT;
                    }

                    Cursor.Current = Cursors.WaitCursor;
                    try
                    {
                        NdcsService.Stop(theStopMode, theStopReason);
                    }
                    finally
                    {
                        Cursor.Current = Cursors.Default;
                    }
                }

                theStopNDCSServiceDialog.Dispose();

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
                refreshRightPane();

            }
        }


        /// <summary>
        /// Refresh the right pane when add, hold, release, alter service
        /// </summary>
        public void refreshRightPane()
        {
            // Recreate the right pane
            TreeView.SelectedNode = Parent;
            TreeView.SelectedNode = this;
        }

        private void InitializeComponent()
        {
            //
            // startNDCSServiceMenuItem
            //
            this.startNDCSServiceMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_Start);
            this.startNDCSServiceMenuItem.Name = "startNDCSServiceMenuItem";
            this.startNDCSServiceMenuItem.Tag = this;
            this.startNDCSServiceMenuItem.Text = Properties.Resources.ContextMenu_Start;
            this.startNDCSServiceMenuItem.Click += new EventHandler(startNDCSServiceMenuItem_Click);

            //
            // stopNDCSServiceMenuItem
            //
            this.stopNDCSServiceMenuItem = new ToolStripMenuItem(Properties.Resources.ContextMenu_Stop);
            this.stopNDCSServiceMenuItem.Name = "stopNDCSServiceMenuItem";
            this.stopNDCSServiceMenuItem.Tag = this;
            this.stopNDCSServiceMenuItem.Click += new EventHandler(stopNDCSServiceMenuItem_Click);


        }

    }
}
