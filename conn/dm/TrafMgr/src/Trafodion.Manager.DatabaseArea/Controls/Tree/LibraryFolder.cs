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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Node class for library folder that contains jar classes.
    /// </summary>
    public class LibraryFolder : NavigationTreeFolder
    {
        #region private member variables
        TrafodionLibrary _sqlMxLibrary = null;

  
        List<string> _theClasses = null;

        #endregion private member variables

        #region Public properties

        public TrafodionLibrary TheTrafodionLibrary
        {
            get { return _sqlMxLibrary; }

        }

        /// <summary>
        /// The short description of the library 
        /// </summary>
        public override string ShortDescription
        {
            get { return _sqlMxLibrary.ExternalName; }
        }
        /// <summary>
        /// The long description of the library 
        /// </summary>
        public override string LongerDescription
        {
            get { return _sqlMxLibrary.ExternalName; }
        }

        #endregion Public properties

        /// <summary>
        /// Constructs the tree node
        /// </summary>
        /// <param name="aFile"></param>
        public LibraryFolder(TrafodionLibrary aTrafodionLibrary)
        {
            this._sqlMxLibrary = aTrafodionLibrary;
            Text = aTrafodionLibrary.ExternalName;
            Tag = aTrafodionLibrary;
            ImageKey = BrowseLibraryTreeView.DB_LIBRARY_ICON;
            SelectedImageKey = BrowseLibraryTreeView.DB_LIBRARY_ICON;
        }

        protected override void PrepareForPopulate()
        {
            object c = _sqlMxLibrary.TheJarClasses; //force a fetch of the jar clases
            
        }
        /// <summary>
        /// Populates the class for the jar file
        /// </summary>
        /// <param name="aNavigationTreeNameFilter"></param>
        protected override void Populate(NavigationTreeNameFilter aNavigationTreeNameFilter)
        {
            Nodes.Clear();
            List<JarClass> classes = _sqlMxLibrary.TheJarClasses;

            if (classes != null)
            {
                foreach (JarClass className in classes)
                {
                    Nodes.Add(new ClassLeaf(_sqlMxLibrary, className));
                }
            }
        }

     

        /// <summary>
        /// Selects the Class Leaf in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is JarClass)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    ClassLeaf theClassLeaf = theNode as ClassLeaf;
                    if (theClassLeaf.TheJarClass.InternalName.Equals(theTargetInternalName))
                    {
                        theClassLeaf.TreeView.SelectedNode = theClassLeaf;
                        return true;
                    }
                }
            }
            return false;
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
