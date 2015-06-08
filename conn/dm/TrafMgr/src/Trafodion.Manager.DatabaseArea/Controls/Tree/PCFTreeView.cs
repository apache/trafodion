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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;
using System.ComponentModel;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Treeview to display the files on Trafodion platform
    /// </summary>
    public class PCFTreeView : TrafodionTreeView
    {
        #region Private member variables

        //PCFUserFolder userFolder = null;
        PCFRolesFolder codeFilesFolder = null;
        List<CodeFile> _codeFileList = new List<CodeFile>();
        //List<CodeFile> _privateCodeFileList = new List<CodeFile>();
        ConnectionDefinition _theConnectionDefinition;

        #endregion Private member variables

        #region Public properties

        public const string FOLDER_ICON = "FOLDER_ICON";
        public const string ROLE_ICON = "ROLE_ICON";
        public const string JAR_ICON = "JAR_ICON";
        public const string CLASS_ICON = "CLASS_ICON";
        public const string USER_ICON = "USER_ICON";

        /// <summary>
        /// The list of code files being displayed in the tree
        /// </summary>
         [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public List<CodeFile> CodeFiles
        {
            get { return _codeFileList; }
            set { _codeFileList = value; }
        }

        ///// <summary>
        ///// The list of code files being displayed in the tree
        ///// </summary>
        //public List<CodeFile> PrivateCodeFiles
        //{
        //    get { return _privateCodeFileList; }
        //    set { _privateCodeFileList = value; }
        //}
        
        /// <summary>
        /// The connection definition associated with this instance
        /// </summary>
        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        #endregion Public properties

        #region Public methods

        public PCFTreeView()
        {
            this.theImageList.Images.Add(FOLDER_ICON, global::Trafodion.Manager.Properties.Resources.FolderClosedIcon);
            this.theImageList.Images.Add(ROLE_ICON, global::Trafodion.Manager.Properties.Resources.DBRoleIcon);
            this.theImageList.Images.Add(JAR_ICON, global::Trafodion.Manager.Properties.Resources.JarIcon);
            this.theImageList.Images.Add(CLASS_ICON, global::Trafodion.Manager.Properties.Resources.ClassIcon);
            this.theImageList.Images.Add(USER_ICON, global::Trafodion.Manager.Properties.Resources.SingleUserIcon);
        }
        /// <summary>
        /// Sets the root folder and populates the child nodes
        /// </summary>
        public void SetAndPopulateRootFolders()
        {
            TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(ConnectionDefinition);
            this.Nodes.Clear();

            codeFilesFolder = new PCFRolesFolder(ConnectionDefinition);
            Nodes.Add(codeFilesFolder);
            PopulateFolder(codeFilesFolder);
            codeFilesFolder.Expand();

        }

        public PCFModel.AccessLevel AccessLevel
        {
            get
            {
                //if (SelectedNode != null)
                //{
                //    string[] path = SelectedNode.FullPath.Split(PathSeparator.ToCharArray(0, 1));
                //    foreach (TreeNode node in Nodes)
                //    {
                //        if (path[0] != null && path[0].Equals(node.Text))
                //        {
                //            if (node is PCFRolesFolder)
                //                return PCFModel.AccessLevel.Role;
                //        }
                //    }
                //}
                return PCFModel.AccessLevel.Role;
            }
        }

        /// <summary>
        /// Helper method to find a code file node given the code file name
        /// </summary>
        /// <param name="codeFileName"></param>
        /// <returns></returns>
        public CodeFile FindCodeFileByName(string codeFileName)
        {
            CodeFile theCodeFile = CodeFiles.Find(delegate(CodeFile aCodeFile)
            {
                return aCodeFile.Name.Equals(codeFileName);
            });
            return theCodeFile;
        }

        /// <summary>
        /// Populates the child nodes
        /// </summary>
        /// <param name="aNavigationTreeFolder"></param>
        public void PopulateFolder(NavigationTreeFolder aNavigationTreeFolder)
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                aNavigationTreeFolder.DoPopulate(null);
            }
            catch (Exception anException)
            {
                //FireExceptionOccurred(anException);
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        /// <summary>
        /// Finds a tree node given its full path
        /// </summary>
        /// <param name="aFullPath"></param>
        /// <returns></returns>
        public TreeNode FindByFullPath(string aFullPath)
        {
            string[] theNodeNames = aFullPath.Split(PathSeparator.ToCharArray(0, 1));

            TreeNode theTreeNode = FindByFullPath(Nodes, theNodeNames, 0);

            return theTreeNode;

        }

        /// <summary>
        /// Finds a tree node given the offset and the list of node parts
        /// </summary>
        /// <param name="aNodes"></param>
        /// <param name="aNodeNames"></param>
        /// <param name="anOffset"></param>
        /// <returns></returns>
        private TreeNode FindByFullPath(TreeNodeCollection aNodes, string[] aNodeNames, int anOffset)
        {
            string theNodeName = aNodeNames[anOffset];
            foreach (TreeNode theTreeNode in aNodes)
            {
                if (theTreeNode.Text.Equals(theNodeName))
                {
                    if (anOffset == aNodeNames.Length - 1)
                    {
                        return theTreeNode;
                    }

                    // This needs to be marshalled onto the tree view's thread and we must wait for it
                    //EndInvoke(BeginInvoke(new ExpandTreeNodeDelegate(ExpandTreeNode), new object[] { theTreeNode }));

                    return FindByFullPath(theTreeNode.Nodes, aNodeNames, anOffset + 1);
                }
            }
            throw new FindByFullPathFailed(this, aNodeNames);
        }

        #endregion Public methods
    }
}
