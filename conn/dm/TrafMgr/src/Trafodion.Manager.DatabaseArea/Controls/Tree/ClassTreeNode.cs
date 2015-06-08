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

using System.Data;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Represents a class node under a jar file node
    /// </summary>
    public class ClassTreeNode : NavigationTreeNode
    {
        #region Private member variables

        private DataTable _methods = null;

        #endregion Private member variables

        #region Public properties

        /// <summary>
        /// The datatable containing details on the methods in the class
        /// </summary>
        public DataTable Methods
        {
            get { return _methods; }
            set { _methods = value; }
        }

        /// <summary>
        /// The short description of the class tree node
        /// </summary>
        public override string ShortDescription
        {
            get { return this.Text; }
        }
        /// <summary>
        /// The long description of the class node
        /// </summary>
        public override string LongerDescription
        {
            get { return this.Text; }
        }

        /// <summary>
        /// The code file to which the class belongs to
        /// </summary>
        public CodeFile CodeFile
        {
            get { return this.Parent.Tag as CodeFile; }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs a class tree node, given the class name
        /// </summary>
        /// <param name="className"></param>
        public ClassTreeNode(string className)
        {
            this.Tag = className;
            this.Text = className;
            ImageKey = PCFTreeView.CLASS_ICON;
            SelectedImageKey = PCFTreeView.CLASS_ICON;
        }

        /// <summary>
        /// Refreshes the node
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Refresh(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {

        }
    }
}
