//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Windows.Forms;
using System;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
	/// <summary>
	/// A class leaf node.
	/// </summary>
    public class ClassLeaf : NavigationTreeNode
	{
       #region Private member variables

        private DataTable _methods = null;
        private JarClass _classModel = null;
        private TrafodionLibrary _sqlMxLibrary = null;
        #endregion Private member variables

        #region Public properties

        /// <summary>
        /// The datatable containing details on the methods in the class
        /// </summary>
        public DataTable Methods
        {
            get { return _classModel.MethodList; }
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


        public TrafodionLibrary TheTrafodionLibrary
        {
            get { return _sqlMxLibrary; }
        }

        public JarClass TheJarClass
        {
            get { return _classModel; }
        }
        #endregion Public properties

        /// <summary>
        /// Constructs a class tree node, given the class name
        /// </summary>
        /// <param name="className"></param>
        public ClassLeaf(TrafodionLibrary aTrafodionLibrary,JarClass aClassInJar)
        {
            _sqlMxLibrary = aTrafodionLibrary;
            this.Tag = aClassInJar;
            this.Text = aClassInJar.ClassName;
            this._classModel = aClassInJar;
            ImageKey = BrowseLibraryTreeView.DB_CLASS_ICON;
            SelectedImageKey = BrowseLibraryTreeView.DB_CLASS_ICON;
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
