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
    /// This class is used to represent an Indexes folder under a schema.
    /// </summary>
    public class SchemaIndexesFolder : SchemaItemsFolder
    {
        /// <summary>
        /// Instantiates a new folder.
        /// </summary>
        /// <param name="aTrafodionSchema">The schema object whose indexes are under this folder.</param>
        public SchemaIndexesFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.Indexes, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Tries to find the TrafodionObject in its list of table indexes, returns false if no match is found
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public override bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionIndex)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is IndexLeaf)
                    {
                        IndexLeaf indexLeaf = theNode as IndexLeaf;
                        if (indexLeaf.TrafodionIndex.InternalName.Equals(theTargetInternalName))
                        {
                            indexLeaf.TreeView.SelectedNode = indexLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Refresh the table indexes folder, clears the table indexes
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.RefreshIndexes();
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionIndexes;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionIndex index in TheTrafodionSchema.TrafodionIndexes)
            {
                IndexLeaf indexLeaf = new IndexLeaf(index);
                Nodes.Add(indexLeaf);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionIndex index in TheTrafodionSchema.TrafodionIndexes)
            {
                if (nameFilter.Matches(index.ExternalName))
                {
                    IndexLeaf indexLeaf = new IndexLeaf(index);
                    Nodes.Add(indexLeaf);
                }
            }
        }
    }
}
