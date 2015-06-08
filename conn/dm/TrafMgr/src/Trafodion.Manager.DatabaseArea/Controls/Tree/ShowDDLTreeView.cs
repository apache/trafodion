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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{

    public class SystemConnectionFactory : ConnectionFolderFactory
    {
        public TrafodionSystem _sqlMxSystem;
        public SystemConnectionFactory(ConnectionDefinition aConnectionDefinition)
        {
            _sqlMxSystem = TrafodionSystem.FindTrafodionSystem(aConnectionDefinition);
        }

        override public NavigationTreeConnectionFolder NewNavigationTreeConnectionFolder(ConnectionDefinition aConnectionDefinition)
        {
            return new Trafodion.Manager.DatabaseArea.Controls.Tree.CatalogsFolder(_sqlMxSystem);
        }
    }
    /// <summary>
    /// A database navigation tree view that supports checkboxes for each tree node
    /// and displays the Active systems which is a subset of the main database navigation tree.
    /// </summary>
    public class ShowDDLTreeView : DatabaseTreeView
    {
        private ConnectionDefinition _connectionDefinition;

        /// <summary>
        /// The various checked states of a Node
        /// </summary>
        public enum NodeState
        {
            NotSet, UnChecked, Checked, Partial
        };

        /// <summary>
        /// Constructs the database active system tree view
        /// </summary>
        public ShowDDLTreeView(ConnectionDefinition aConnectionDefinition)
        {
            _connectionDefinition = aConnectionDefinition;

            CheckBoxes = true; //Enable checkboxes to displayed in the tree

            NavigationTreeConnectionFolderFactory = new SystemConnectionFactory(aConnectionDefinition);

            //Handlers to handle checkbox states
            DrawMode = TreeViewDrawMode.OwnerDrawText;
            DrawNode += new DrawTreeNodeEventHandler(TreeView_DrawNode);
            BeforeCheck += new TreeViewCancelEventHandler(TreeView_BeforeCheck);
            AfterCheck += new TreeViewEventHandler(TreeView_AfterCheck);
            AfterExpand += new TreeViewEventHandler(TreeView_AfterExpand);
        }

        protected override void WndProc(ref Message m)
        {
            // Suppress WM_LBUTTONDBLCLK
            if (m.Msg == 0x203) { m.Result = IntPtr.Zero; }
            else base.WndProc(ref m);
        }

        /// <summary>
        /// Draws a Tree Node
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TreeView_DrawNode(object sender, DrawTreeNodeEventArgs e)
        {
            if (e.Node.StateImageIndex < 0)
                e.Node.StateImageIndex = 0;

            // If a node tag is present, draw its string representation 
            // to the right of the label text.
            if (e.Node.Tag != null && ((NodeState)e.Node.Tag) == NodeState.Partial)
            {
                Font nodeFont = e.Node.NodeFont;
                if (nodeFont == null) nodeFont = ((TreeView)sender).Font;
                    nodeFont = new Font(nodeFont, FontStyle.Italic);

                //e.Graphics.FillRectangle(Brushes.Gray, new Rectangle(e.Node.Bounds.X-10, e.Node.Bounds.Y-10, 20, 30));

                e.Graphics.DrawString(e.Node.Text, nodeFont,
                                    Brushes.DarkSlateGray, e.Bounds.X, e.Bounds.Y);
            }
            // Use the default background and node text.
            else
            {
                e.DrawDefault = true;
            }
        }

        /// <summary>
        /// Handles a check box being clicked on the tree view.
        /// Based on the node, the child nodes and/or parent node's checkbox states are also handled
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TreeView_BeforeCheck(object sender, TreeViewCancelEventArgs e)
        {
            // The code only executes if the user caused the checked state to change.
            if (e.Action != TreeViewAction.Unknown)
            {
                if (e.Node.Checked)
                {
                    if (e.Node.Tag != null)
                    {
                        NodeState state = (NodeState)e.Node.Tag;
                        if (state == NodeState.Partial)
                        {
                            e.Cancel = true;
                            BeginUpdate();
                            e.Node.Tag = NodeState.Checked;
                            CheckAllChildNodes(e.Node, (NodeState)e.Node.Tag);
                            CheckParent(e.Node);
                            EndUpdate();
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Handles a check box being clicked on the tree view.
        /// Based on the node, the child nodes and/or parent node's checkbox states are also handled
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TreeView_AfterCheck(object sender, TreeViewEventArgs e)
        {
            // The code only executes if the user caused the checked state to change.
            if (e.Action != TreeViewAction.Unknown)
            {
                if (e.Node.Checked)
                {
                    e.Node.Tag = NodeState.Checked;
                }
                else
                    e.Node.Tag = NodeState.UnChecked;

                BeginUpdate();
                CheckAllChildNodes(e.Node, (NodeState)e.Node.Tag);
                CheckParent(e.Node);
                EndUpdate();
            }
        }

        /// <summary>
        /// When you expand a tree node, apply the parent node's checked state to the child nodes
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TreeView_AfterExpand(object sender, TreeViewEventArgs e)
        {
            if (e.Node.Tag == null)
                e.Node.Tag = NodeState.NotSet;

            CheckAllChildNodes(e.Node, (NodeState)e.Node.Tag);
        }

        public override void TreeNodePopulated(TreeNode node)
        {
            CheckAllChildNodes(node, (NodeState)node.Tag);
        }
        /// ------------------------------------------------------------------------------------
        /// <summary>
        /// Called after a node changed its state. Has to go through all direct children and
        /// set state based on children's state.
        /// </summary>
        /// <param name="node">Parent node</param>
        /// ------------------------------------------------------------------------------------
        private void CheckParent(TreeNode node)
        {
            if (node is SchemaFolder)
                return;

            TreeNode parent = node.Parent;
            if (parent == null)
                return;

            if (parent is SchemaFolder && !parent.Checked)
            {
                return;
            }

            bool checkedStateOfChildren = false;
            if (parent.Nodes.Count > 0)
            {
                checkedStateOfChildren = parent.FirstNode.Checked;
            }
            bool isAnyChildPartiallyChecked = HasPartiallyCheckedChildNodes(parent);

            if (node.Checked)
            {

                foreach (TreeNode child in parent.Nodes)
                    checkedStateOfChildren &= child.Checked;

                parent.Checked = true;

                if (checkedStateOfChildren)
                {
                    if(isAnyChildPartiallyChecked)
                        parent.Tag = NodeState.Partial;
                    else
                        parent.Tag = NodeState.Checked;
                }
                else
                {
                    parent.Tag = NodeState.Partial;
                }
            }
            else
            {
                foreach (TreeNode child in parent.Nodes)
                    checkedStateOfChildren |= child.Checked;

                if (checkedStateOfChildren)
                {
                    if ((NodeState)parent.Tag != NodeState.Partial)
                    {
                        parent.Tag = NodeState.Partial;
                    }
                }
                else
                {
                    if (parent is SchemaItemsFolder || parent is TableItemsFolder || parent is MaterializedViewItemsFolder)
                    {
                        parent.Tag = NodeState.UnChecked;
                        parent.Checked = false;
                    }
                    else
                    {
                        parent.Tag = NodeState.Partial;
                        parent.Checked = true;
                    }
                }
            }

            CheckParent(parent);
        }

        /// <summary>
        /// Update all child tree nodes recursively with the parent node's checked state.
        /// </summary>
        /// <param name="parentNode"></param>
        /// <param name="state"></param>
        private void CheckAllChildNodes(TreeNode parentNode, NodeState state)
        {
            foreach (TreeNode childNode in parentNode.Nodes)
            {
                if (state == NodeState.Partial)
                {
                    NodeState childState = (NodeState)childNode.Tag;
                    if (childState == NodeState.NotSet)
                    {
                        childNode.Checked = parentNode.Checked;
                        childNode.Tag = NodeState.Checked;
                    }
                }
                else
                {
                    childNode.Tag = state;
                    childNode.Checked = parentNode.Checked;
                }

                if (childNode.Nodes.Count > 0)
                {
                    // If the current node has child nodes, call the CheckAllChildsNodes method recursively.
                    CheckAllChildNodes(childNode, state);
                }
            }
        }

        /// <summary>
        /// Reset all checkboxes in the tree view
        /// </summary>
        public void ResetCheckBoxes()
        {
            foreach (TreeNode node in Nodes)
            {
                ResetCheckBoxes(node);
            }
        }

        /// <summary>
        /// Reset checkboxes for the specified node and all its child node
        /// </summary>
        /// <param name="node"></param>
        public void ResetCheckBoxes(TreeNode node)
        {
            node.Checked = false;

            foreach (TreeNode childNode in node.Nodes)
            {
                if (childNode.Text == null || childNode.Text.Equals(""))
                    continue;

                ResetCheckBoxes(childNode);
            }
        }

        /// <summary>
        /// Indicates if any of the Tree nodes are in a checked state
        /// </summary>
        public bool HasNodesChecked
        {
            get
            {
                foreach (TreeNode node in Nodes)
                {
                    if (HasCheckedChildNodes(node))
                        return true;
                    continue;
                }
                return false;
            }
        }

        /// <summary>
        /// Indicates if any of the child node or its children have a checked state
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        public bool HasCheckedChildNodes(TreeNode node)
        {
            return HasCheckedChildNodes(node, true);
        }

        /// <summary>
        /// Recursively checks to see if any of the child node or its children have a checked state
        /// </summary>
        /// <param name="node"></param>
        /// <param name="recurse"></param>
        /// <returns></returns>
        public bool HasCheckedChildNodes(TreeNode node, bool recurse)
        {
            foreach (TreeNode childNode in node.Nodes)
            {
                if (childNode.Text == null || childNode.Text.Equals(""))
                    continue; 
                
                if (childNode.Checked)
                    return true;

                if (recurse)
                {
                    if (HasCheckedChildNodes(childNode))
                        return true;
                    else
                        continue;
                }
            }
            return false;
        }

        /// <summary>
        /// Recursively checks to see if any of the child node or its children have a checked state
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        public bool HasPartiallyCheckedChildNodes(TreeNode node)
        {
            foreach (TreeNode childNode in node.Nodes)
            {
                if (childNode.Text == null || childNode.Text.Equals(""))
                    continue;
                if (childNode.Tag != null && ((NodeState)childNode.Tag == NodeState.Partial))
                        return true;
            }
            return false;
        }

        /// <summary>
        /// Disable the context menu on this tree view
        /// </summary>
        public override bool AllowContextMenu
        {
            get
            {
                return false;
            }
        }

        /// <summary>
        /// Set and populate the root folders for this tree view
        /// </summary>
        public override void SetAndPopulateRootFolders()
        {
            Nodes.Clear();

            CatalogsFolder catalogsFolder = new CatalogsFolder(TrafodionSystem.FindTrafodionSystem(_connectionDefinition));
            Nodes.Add(catalogsFolder);
            catalogsFolder.DoPopulate(TheNavigationTreeNameFilter);
            SelectedNode = catalogsFolder;
        }

        public override NavigationTreeConnectionFolder ExpandConnection(ConnectionDefinition aConnectionDefinition)
        {
            if (Nodes.Count > 0)
            {
                if (Nodes[0] is NavigationTreeConnectionFolder)
                {
                    return ((NavigationTreeConnectionFolder)Nodes[0]);
                }
            }
            throw new Exception("Cannot find catalogs folder for " + aConnectionDefinition.Name);

        }

        public override TreeNode FindByFullPath(string aFullPath)
        {
            string[] theNodeNames = aFullPath.Split(PathSeparator.ToCharArray(0, 1));

            TreeNode theTreeNode = FindByFullPath(Nodes, theNodeNames, 1);

            return theTreeNode;
        }
        /// <summary>
        /// Finds the tree node that contains this sql object
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns>TreeNode or null, if treenode couldnt be found</returns>
        public TreeNode FindNodeForTrafodionObject(TrafodionObject aTrafodionObject)
        {
            foreach (TreeNode node in Nodes)
            {
                if (node.Text == null || node.Text.Equals(""))
                    continue;
                
                TreeNode targetNode = FindNodeForTrafodionObject(node, aTrafodionObject);
                if (targetNode != null)
                    return targetNode;
            }
            return null;
        }

        /// <summary>
        /// Recurses the tree and finds the tree node that contains this sql object, starting with the given node
        /// </summary>
        /// <param name="node">The node from which to start the search</param>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public TreeNode FindNodeForTrafodionObject(TreeNode node, TrafodionObject aTrafodionObject)
        {
            //If the node is of type databasetreefolder or databasetreenode, then they contain a sql object
            if (node is DatabaseTreeFolder)
            {
                if (((DatabaseTreeFolder)node).TrafodionObject.InternalName.Equals(aTrafodionObject.InternalName))
                    return node;
            }
            else
            if (node is DatabaseTreeNode)
            {
                if (((DatabaseTreeNode)node).TrafodionObject.InternalName.Equals(aTrafodionObject.InternalName))
                    return node;
            }

            //If node has not been found yet, recurse through child nodes
            foreach (TreeNode childNode in node.Nodes)
            {
                if (childNode.Text == null || childNode.Text.Equals(""))
                    continue;

                TreeNode targetNode = FindNodeForTrafodionObject(childNode, aTrafodionObject);
                if (targetNode != null)
                    return targetNode;
            }
            return null;
        }

    }
}
