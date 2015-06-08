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
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.LiveFeedMonitoringArea.Controls.Tree
{

    /// <summary>
    /// The LiveFeed navigation tree.
    /// </summary>
    public class LiveFeedMonitorTreeView : Trafodion.Manager.Framework.Navigation.NavigationTreeView
    {
        /// <summary>
        /// Expands the connection folder and returns the connection folder
        /// </summary>
        /// <param name="aConnectionDefinition">Connection Definition</param>
        /// <returns>Connection folder</returns>
        public NavigationTreeConnectionFolder ExpandConnection(ConnectionDefinition aConnectionDefinition)
        {
            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is NavigationTreeConnectionsFolder)
                {
                    NavigationTreeConnectionsFolder theNavigationTreeMyActiveSystemsFolder = theTreeNode as NavigationTreeConnectionsFolder;
                    NavigationTreeConnectionFolder theNavigationTreeConnectionFolder = theNavigationTreeMyActiveSystemsFolder.FindConnectionFolder(aConnectionDefinition);
                    if (theNavigationTreeConnectionFolder != null)
                    {

                        // Make sure that the connections folder has been populated by selecting
                        // the connections folder if it isn't.  This reduces flashing in the right
                        // pane when clikcing on a SQL object hyperlink in the right pane.
                        if (!theNavigationTreeConnectionFolder.IsExpanded)
                        {
                            Select(theNavigationTreeConnectionFolder);
                        }

                        return theNavigationTreeConnectionFolder;
                    }
                }
            }
            throw new Exception("Not found in my systems: " + aConnectionDefinition.Name);
        }

        protected override void OnNodeMouseClick(TreeNodeMouseClickEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                if (SelectedNode != e.Node)
                {
                    SelectedNode = e.Node;
                }
            }
            base.OnNodeMouseClick(e);
        }
    }
}
