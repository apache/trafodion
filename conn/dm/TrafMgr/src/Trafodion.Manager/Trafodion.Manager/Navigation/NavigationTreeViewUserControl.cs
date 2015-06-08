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

using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.Framework.Navigation
{
    public partial class NavigationTreeViewUserControl : UserControl
    {
        public NavigationTreeViewUserControl()
        {
            InitializeComponent();
            //NavigationTreeView = new NavigationTreeView();
            if (TrafodionContext.Instance.TheTrafodionMain != null)
            {
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheParentToolStripItem.Click += TheParentToolStripItem_Click;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.ThePreviousToolStripItem.Click += ThePreviousToolStripItem_Click;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheNextToolStripItem.Click += TheNextToolStripItem_Click;
            }
        }

        void MyDispose(bool disposing)
        {
            if (TrafodionContext.Instance.TheTrafodionMain != null)
            {
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheParentToolStripItem.Click -= TheParentToolStripItem_Click;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.ThePreviousToolStripItem.Click -= ThePreviousToolStripItem_Click;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheNextToolStripItem.Click -= TheNextToolStripItem_Click;
            }
            if (theNavigationTreeView != null)
            {
                theNavigationTreeView.Selected -= theNavigationTreeView_Selected;
            }
        }

        void TheNextToolStripItem_Click(object sender, System.EventArgs e)
        {
            theNavigationTreeView.SelectNext();
        }

        void ThePreviousToolStripItem_Click(object sender, System.EventArgs e)
        {
            theNavigationTreeView.SelectPrevious();
        }

        void TheParentToolStripItem_Click(object sender, System.EventArgs e)
        {
            theNavigationTreeView.SelectParent();
        }

        private NavigationTreeView theNavigationTreeView;

        public NavigationTreeView NavigationTreeView
        {
            get { return theNavigationTreeView; }
            set 
            {
                if (theNavigationTreeView != null)
                {
                    Controls.Remove(theNavigationTreeView);
                    theNavigationTreeView.Selected -= theNavigationTreeView_Selected;
                }
                theNavigationTreeView = value;
                if (theNavigationTreeView != null)
                {
                    theNavigationTreeView.Dock = DockStyle.Fill;
                    Controls.Add(theNavigationTreeView);
                    theNavigationTreeView.Selected += theNavigationTreeView_Selected;
                }
            }
        }

        /// <summary>
        /// Enables the connect/disconnect toolbar buttons based on the connection definition state
        /// </summary>
        /// <param name="aNavigationTreeNode"></param>
        void theNavigationTreeView_Selected(NavigationTreeNode aNavigationTreeNode)
        {
            TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheRunScriptToolStripItem.Enabled = false;
            if (aNavigationTreeNode.TheConnectionDefinition != null)
            {
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.Visible = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheRunScriptToolStripItem.Visible = false;

                if (aNavigationTreeNode.TheConnectionDefinition.TheState == Trafodion.Manager.Framework.Connections.ConnectionDefinition.State.TestSucceeded)
                {
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheConnectToolStripItem.Enabled = false;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDisconnectToolStripItem.Enabled = true;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheEventViewerToolStripItem.Enabled = false;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheRunScriptToolStripItem.Enabled = false;

                    if (aNavigationTreeNode.TheConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem.Enabled = false;
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = false;
                            //aNavigationTreeNode.TheConnectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS", "DOWNLOAD_TAR");
                    }
                    else
                    {
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem.Visible = false;
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Visible = false;
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = false;
                    }
                    //if version >=M10
                    if (aNavigationTreeNode.TheConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                    {
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = false;
                           // (aNavigationTreeNode.TheConnectionDefinition.RoleName.Equals("DB__ROOTROLE") 
                           // || aNavigationTreeNode.TheConnectionDefinition.DatabaseUserName.Equals("DB__ROOT"));
                    }
                    else
                    {
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.Visible = false;
                        TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = false;
                    }
                }
                else
                {
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheConnectToolStripItem.Enabled = true;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDisconnectToolStripItem.Enabled = false;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheEventViewerToolStripItem.Enabled = true;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem.Enabled = false;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = false;
                    TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = false;
                }
            }
            else
            {
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheConnectToolStripItem.Enabled = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDisconnectToolStripItem.Enabled = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheEventViewerToolStripItem.Enabled = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheAuditLogViewerToolStripItem.Enabled = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheDownloadOSIMFilesViewerToolStripItem.Enabled = false;
                TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheUpdateConfigurationToolStripItem.Enabled = false;
            }
            TrafodionContext.Instance.TheTrafodionMain.CurrentConnectionDefinition = aNavigationTreeNode.TheConnectionDefinition;
            TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheParentToolStripItem.Enabled = (aNavigationTreeNode.Parent != null);
            TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.ThePreviousToolStripItem.Enabled = (aNavigationTreeNode.PrevNode != null);
            TrafodionContext.Instance.TheTrafodionMain.TheMainToolBar.TheNextToolStripItem.Enabled = (aNavigationTreeNode.NextNode != null);
        }

    }
}
