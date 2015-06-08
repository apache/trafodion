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

using System.Collections.Generic;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;


namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Node that encapsulates the information about the jar
    /// </summary>
    public class ClassFileTreeNode : NavigationTreeNode
    {
        CodeFile _theClassFile = null;
        List<JavaMethod> _theMethods = null;
        public ClassFileTreeNode(CodeFile aFile)
        {
            this._theClassFile = aFile;
            Text = aFile.Name;
            Tag = aFile;
            ImageKey = PCFTreeView.CLASS_ICON;
            SelectedImageKey = PCFTreeView.CLASS_ICON;
        }

        public CodeFile ClassFile
        {
            get { return _theClassFile; }
            set { _theClassFile = value; Tag = value; }
        }

        /// <summary>
        /// The short description of the Wms object contained in the tree node
        /// </summary>
        public override string ShortDescription
        {
            get { return _theClassFile.Name; }
        }
        /// <summary>
        /// The long description of the Wms object contained in the tree node
        /// </summary>
        public override string LongerDescription
        {
            get { return _theClassFile.Name; }
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
