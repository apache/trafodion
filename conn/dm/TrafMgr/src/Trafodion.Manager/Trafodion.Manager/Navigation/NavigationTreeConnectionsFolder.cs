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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Connections.Controls;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework.Navigation
{
    public class NavigationTreeConnectionsFolder : NavigationTreeFolder
    {

        public NavigationTreeConnectionsFolder()
        {
            Text = Properties.Resources.MySystemsText;
            ImageKey = NavigationTreeView.FOLDER_CLOSED_ICON;
            SelectedImageKey = NavigationTreeView.FOLDER_CLOSED_ICON;
            theChangedHandler = new ConnectionDefinition.ChangedHandler(ConnectionDefinitionChanged);
            ConnectionDefinition.Changed += theChangedHandler;
        }

        ~NavigationTreeConnectionsFolder()
        {
            if (theChangedHandler != null)
            {
                ConnectionDefinition.Changed -= theChangedHandler;
            }
        }

        override public string ShortDescription
        {
            get
            {
                return Properties.Resources.MySystemsText;
            }
        }

        override public string LongerDescription
        {
            get
            {
                return Properties.Resources.MySystemsText;
            }
        }
        protected override void PrepareForPopulate()
        {
            
        }

        protected override void RemoveListeners()
        {
            foreach (TreeNode node in Nodes)
            {
                if (node is NavigationTreeConnectionFolder)
                {
                    ((NavigationTreeConnectionFolder)node).RemoveHandlers();
                }
            }
        }
        override protected void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            RemoveListeners();
            Nodes.Clear();

            foreach (ConnectionDefinition theConnectionDefinition in ConnectionDefinition.ConnectionDefinitions)
            {
                AddConnectionFolder(theConnectionDefinition);
            }

        }

        public void AddCommonContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            base.AddToContextMenu(aContextMenuStrip);
        }

        public override void AddToContextMenu(TrafodionContextMenuStrip aContextMenuStrip)
        {
            AddCommonContextMenu(aContextMenuStrip);

            ToolStripMenuItem theAddSystemMenuItem = new ToolStripMenuItem(Properties.Resources.AddSystemMenuText);

            theAddSystemMenuItem.Click += new EventHandler(TheAddSystemMenuItemClick);
            aContextMenuStrip.Items.Add(theAddSystemMenuItem);
        }

        void TheAddSystemMenuItemClick(object sender, EventArgs e)
        {
            ConnectionDefinitionDialog theConnectionDefinitionDialog = new ConnectionDefinitionDialog();
            theConnectionDefinitionDialog.New();
        }

        // Find a connection with the same name
        private NavigationTreeConnectionFolder FindConnectionFolder(string aConnectionName)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is NavigationTreeConnectionFolder)
                {
                    NavigationTreeConnectionFolder theConnectionFolder = theTreeNode as NavigationTreeConnectionFolder;
                    if (theConnectionFolder.TheConnectionDefinition.Name.Equals(aConnectionName))
                    {
                        return theConnectionFolder;
                    }
                }
            }
            return null;
        }

        // Find an exact connection instance
        public NavigationTreeConnectionFolder FindConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is NavigationTreeConnectionFolder)
                {
                    NavigationTreeConnectionFolder theConnectionFolder = theTreeNode as NavigationTreeConnectionFolder;
                    if (theConnectionFolder.TheConnectionDefinition == aConnectionDefinition)
                    {
                        return theConnectionFolder;
                    }
                }
            }
            return null;
        }

        private delegate void ConnectionDefinitionDelegate(ConnectionDefinition aConnectionDefinition);

        virtual public bool BelongsInThisFolder(ConnectionDefinition aConnectionDefinition)
        {
            return true;
        }


        private void SelectConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            NavigationTreeConnectionFolder connFolder = FindConnectionFolder(aConnectionDefinition.Name);
            if (connFolder != null && TrafodionContext.Instance.TheTrafodionMain.IsControlInActiveTrafodionArea(TreeView))
            {
                ((NavigationTreeView)TreeView).SelectedNode = connFolder;
               // ((NavigationTreeView)TreeView).FireSelected(connFolder);
            }
        }

        private void UpdateConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            if (TreeView != null)
            {
                if (TreeView.InvokeRequired)
                {
                    TreeView.Invoke(new ConnectionDefinitionDelegate(UpdateConnectionFolder), new object[] { aConnectionDefinition });
                }
                else
                {
                    NavigationTreeConnectionFolder connFolder = FindConnectionFolder(aConnectionDefinition.Name);
                    if (connFolder != null)
                    {
                        NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = ((NavigationTreeView)TreeView).NavigationTreeConnectionFolderFactory.UpdateNavigationTreeConnectionFolder(aConnectionDefinition, connFolder);
                        if (theNavigationTreeConnectionFolder != null && connFolder != theNavigationTreeConnectionFolder)
                        {
                            Nodes.Remove(connFolder);
                            Nodes.Add(theNavigationTreeConnectionFolder);
                        }
                        if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                        {
                            theNavigationTreeConnectionFolder.ImageKey = NavigationTreeView.CONNECTED_SERVER_ICON;
                            theNavigationTreeConnectionFolder.SelectedImageKey = NavigationTreeView.CONNECTED_SERVER_ICON;
                            string nodeText = theNavigationTreeConnectionFolder.Text;
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Bold);
                            theNavigationTreeConnectionFolder.Text = string.Empty;
                            theNavigationTreeConnectionFolder.Text = nodeText;
                            theNavigationTreeConnectionFolder.IsPopulated = false;
                            theNavigationTreeConnectionFolder.DoPopulate(null);
                         }
                        else if (aConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
                        {
                            theNavigationTreeConnectionFolder.ImageKey = NavigationTreeView.LF_CONNECTED_SERVER_ICON;
                            theNavigationTreeConnectionFolder.SelectedImageKey = NavigationTreeView.LF_CONNECTED_SERVER_ICON;
                            string nodeText = theNavigationTreeConnectionFolder.Text;
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Bold);
                            theNavigationTreeConnectionFolder.Text = string.Empty;
                            theNavigationTreeConnectionFolder.Text = nodeText;
                            theNavigationTreeConnectionFolder.IsPopulated = false;
                            theNavigationTreeConnectionFolder.DoPopulate(null);
                        }
                        else
                        {
                            theNavigationTreeConnectionFolder.ImageKey = NavigationTreeView.SERVER_ICON;
                            theNavigationTreeConnectionFolder.SelectedImageKey = NavigationTreeView.SERVER_ICON;
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Regular | FontStyle.Italic);
                            theNavigationTreeConnectionFolder.Nodes.Clear();
                        }
                        // Select the folder we point to but only if we are in the active area.  Otherwise it's wasted effort
                        // because it's done too soon.  This can happen during a persistence restore.
                        if (TrafodionContext.Instance.TheTrafodionMain.IsControlInActiveTrafodionArea(TreeView))
                        {
                            ((NavigationTreeView)TreeView).SelectedNode = theNavigationTreeConnectionFolder;
                            ((NavigationTreeView)TreeView).FireSelected(theNavigationTreeConnectionFolder);
                        }
                    }
                }
            }
        }

        private void AddConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            AddConnectionFolder(aConnectionDefinition, -1);
        }

        private void AddConnectionFolder(ConnectionDefinition aConnectionDefinition, int index)
        {

            if (TreeView != null)
            {
                if (TreeView.InvokeRequired)
                {
                    TreeView.Invoke(new ConnectionDefinitionDelegate(AddConnectionFolder), new object[] { aConnectionDefinition, index });
                }
                else
                {
                    NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = FindConnectionFolder(aConnectionDefinition.Name);
                    if (theNavigationTreeConnectionFolder == null)
                    {
                        theNavigationTreeConnectionFolder = ((NavigationTreeView)TreeView).NavigationTreeConnectionFolderFactory.NewNavigationTreeConnectionFolder(aConnectionDefinition);
                        if (index < 0)
                        {
                            Nodes.Add(theNavigationTreeConnectionFolder);
                        }
                        else
                        {
                            Nodes.Insert(index, theNavigationTreeConnectionFolder);
                        }

                        if (aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                        {
                            theNavigationTreeConnectionFolder.ImageKey = NavigationTreeView.CONNECTED_SERVER_ICON;
                            theNavigationTreeConnectionFolder.SelectedImageKey = NavigationTreeView.CONNECTED_SERVER_ICON;
                            string nodeText = theNavigationTreeConnectionFolder.Text;
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Bold);
                            theNavigationTreeConnectionFolder.Text = string.Empty;
                            theNavigationTreeConnectionFolder.Text = nodeText;
                            // Select the folder we point to but only if we are in the active area.  Otherwise it's wasted effort
                            // because it's done too soon.  This can happen during a persistence restore.
                            if (TrafodionContext.Instance.TheTrafodionMain.IsControlInActiveTrafodionArea(TreeView))
                            {
                                TreeView.SelectedNode = theNavigationTreeConnectionFolder;
                            }
                        }
                        else if (aConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
                        {
                            theNavigationTreeConnectionFolder.ImageKey = NavigationTreeView.LF_CONNECTED_SERVER_ICON;
                            theNavigationTreeConnectionFolder.SelectedImageKey = NavigationTreeView.LF_CONNECTED_SERVER_ICON;
                            string nodeText = theNavigationTreeConnectionFolder.Text;
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Bold);
                            theNavigationTreeConnectionFolder.Text = string.Empty;
                            theNavigationTreeConnectionFolder.Text = nodeText;
                            // Select the folder we point to but only if we are in the active area.  Otherwise it's wasted effort
                            // because it's done too soon.  This can happen during a persistence restore.
                            if (TrafodionContext.Instance.TheTrafodionMain.IsControlInActiveTrafodionArea(TreeView))
                            {
                                TreeView.SelectedNode = theNavigationTreeConnectionFolder;
                            }
                        }
                        else
                        {
                            theNavigationTreeConnectionFolder.ImageKey = NavigationTreeView.SERVER_ICON;
                            theNavigationTreeConnectionFolder.SelectedImageKey = NavigationTreeView.SERVER_ICON;
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Regular | FontStyle.Italic);
                        }
                    }
                    else
                    {
                        if (theNavigationTreeConnectionFolder.TheConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded)
                        {
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Bold);
                        }
                        else
                        {
                            theNavigationTreeConnectionFolder.NodeFont = new Font(theNavigationTreeConnectionFolder.TreeView.Font, FontStyle.Regular | FontStyle.Italic);
                        }
                    }
                }
            }
        }

        private void RemoveConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            if ((TreeView != null) && TreeView.InvokeRequired)
            {
                TreeView.Invoke(new ConnectionDefinitionDelegate(RemoveConnectionFolder), new object[] { aConnectionDefinition });
            }
            else
            {
                NavigationTreeConnectionFolder theConnectionFolder = FindConnectionFolder(aConnectionDefinition.Name);
                if (theConnectionFolder != null)
                {
                    Nodes.Remove(theConnectionFolder);
                }
            }
        }

        override protected void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {

        }

        void ConnectionDefinitionChanged(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (TreeView != null)
            {
                if (aReason == ConnectionDefinition.Reason.Added)
                {
                    if (BelongsInThisFolder(aConnectionDefinition))
                    {
                        AddConnectionFolder(aConnectionDefinition);
                    }
                }
                else if (aReason == ConnectionDefinition.Reason.Removed)
                {
                    RemoveConnectionFolder(aConnectionDefinition);
                }
                else if ((aReason == ConnectionDefinition.Reason.Tested && aConnectionDefinition.TheState == ConnectionDefinition.State.TestSucceeded) ||
                         (aReason == ConnectionDefinition.Reason.Tested && aConnectionDefinition.TheState == ConnectionDefinition.State.LiveFeedTestSucceeded)
                            || aReason == ConnectionDefinition.Reason.Disconnected)
                {
                    //You remove and add the node to force recreation of the connection folder
                    //with the new connection state
                    try
                    {
                        this.TreeView.BeginUpdate();
                        ((NavigationTreeView)this.TreeView).EditInProgress = true;
                        NavigationTreeConnectionFolder folder = FindConnectionFolder(aConnectionDefinition);
                        int nodeIndex = folder.Index;
                        RemoveConnectionFolder(aConnectionDefinition);
                        AddConnectionFolder(aConnectionDefinition, nodeIndex);
                        //UpdateConnectionFolder(aConnectionDefinition);
                        ((NavigationTreeView)this.TreeView).EditInProgress = false;
                        this.TreeView.EndUpdate();
                        SelectConnectionFolder(aConnectionDefinition);
                    }
                    catch (Exception e)
                    {
                        ((NavigationTreeView)this.TreeView).EditInProgress = false;
                        this.TreeView.EndUpdate();
                    }
                }
            }
        }

        private ConnectionDefinition.ChangedHandler theChangedHandler;
    }
}
