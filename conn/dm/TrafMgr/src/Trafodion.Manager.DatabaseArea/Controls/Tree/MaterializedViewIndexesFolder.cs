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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    /// <summary>
    /// Summary description for MV Indexes Folder.
    /// </summary>
    public class MaterializedViewIndexesFolder : MaterializedViewItemsFolder
    {

        /// <summary>
        /// Constructor for the MV indexes folder
        /// </summary>
        /// <param name="aTrafodionMaterializedView">the Materialized View</param>
        public MaterializedViewIndexesFolder(TrafodionMaterializedView aTrafodionMaterializedView)
            : base(Properties.Resources.Indexes, aTrafodionMaterializedView)
        {

        }

        /// <summary>
        /// Tries to find the TrafodionObject in its list of table indexes, returns false if no match is found
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionIndex)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is MaterializedViewIndexLeaf)
                    {
                        MaterializedViewIndexLeaf theMVIndexLeaf = theNode as MaterializedViewIndexLeaf;
                        if (theMVIndexLeaf.TrafodionIndex.InternalName.Equals(theTargetInternalName))
                        {
                            theMVIndexLeaf.TreeView.SelectedNode = theMVIndexLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Refresh the MV indexes folder, clears the MV indexes
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TrafodionMaterializedView.RefreshIndexes();
        }

        protected override void PrepareForPopulate()
        {
            object c = TrafodionMaterializedView.TrafodionIndexes;
        }        
        
        /// <summary>
        /// Populates the MVIndexesFolder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();


            foreach (TrafodionIndex theTrafodionIndex in TrafodionMaterializedView.TrafodionIndexes)
            {
                MaterializedViewIndexLeaf theMVIndexLeaf = new MaterializedViewIndexLeaf(theTrafodionIndex);
                Nodes.Add(theMVIndexLeaf);
            }
        }

        /// <summary>
        /// Short description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Text + " on " + TrafodionMaterializedView.ExternalName;
            }
        }

        /// <summary>
        /// Long description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return Text + " on Materialized View " + TrafodionMaterializedView.VisibleAnsiName;
            }
        }

    }
}
