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
    public class JarTreeFolder : NavigationTreeFolder
    {
        #region private member variables

        CodeFile _theJarFile = null;
        List<string> _theClasses = null;

        #endregion private member variables

        #region Public properties

        /// <summary>
        /// The code file encapsulated by this tree node
        /// </summary>
        public CodeFile JarFile
        {
            get { return _theJarFile; }
            set { _theJarFile = value; Tag = value; }
        }

        /// <summary>
        /// The short description of the Wms object contained in the tree node
        /// </summary>
        public override string ShortDescription
        {
            get { return _theJarFile.Name; }
        }
        /// <summary>
        /// The long description of the Wms object contained in the tree node
        /// </summary>
        public override string LongerDescription
        {
            get { return _theJarFile.Name; }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs the tree node
        /// </summary>
        /// <param name="aFile"></param>
        public JarTreeFolder(CodeFile aFile)
        {
            this._theJarFile = aFile;
            Text = aFile.Name;
            Tag = aFile;
            ImageKey = PCFTreeView.JAR_ICON;
            SelectedImageKey = PCFTreeView.JAR_ICON;
        }

        protected override void PrepareForPopulate()
        {
            object c = JarFile.ClassNames; //force a fetch of the class names
        }
        /// <summary>
        /// Populates the class for the jar file
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();

            List<string> classNames = JarFile.ClassNames;

            if (classNames != null)
            {
                foreach (string className in classNames)
                {
                    Nodes.Add(new ClassTreeNode(className));
                }
            }
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
