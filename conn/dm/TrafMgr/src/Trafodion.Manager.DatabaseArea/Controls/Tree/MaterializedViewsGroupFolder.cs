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
    /// Summary description for MaterializedViewGroupsFolder.
    /// </summary>
    public class MaterializedViewGroupsFolder : SchemaItemsFolder
    {

        /// <summary>
        /// Constructs a MaterializedViewGroupsFolder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public MaterializedViewGroupsFolder(TrafodionSchema aTrafodionSchema)
            : base( Properties.Resources.MVGroups, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Selects the Materialized View Groups folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionMaterializedViewGroup)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is MaterializedViewGroupLeaf)
                    {
                        MaterializedViewGroupLeaf theMaterializedViewGroupLeaf = theNode as MaterializedViewGroupLeaf;
                        if (theMaterializedViewGroupLeaf.TrafodionMaterializedViewGroup.InternalName.Equals(theTargetInternalName))
                        {
                            theMaterializedViewGroupLeaf.TreeView.SelectedNode = theMaterializedViewGroupLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the MV Groups list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        /// 
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionMaterializedViewGroups = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionMaterializedViewGroups;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionMaterializedViewGroup theTrafodionMaterializedViewGroup in TheTrafodionSchema.TrafodionMaterializedViewGroups)
            {
                MaterializedViewGroupLeaf theMaterializedViewGroupItem = new MaterializedViewGroupLeaf(theTrafodionMaterializedViewGroup);
                Nodes.Add(theMaterializedViewGroupItem);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionMaterializedViewGroup theTrafodionMaterializedViewGroup in TheTrafodionSchema.TrafodionMaterializedViewGroups)
            {
                if (nameFilter.Matches(theTrafodionMaterializedViewGroup.ExternalName))
                {
                    MaterializedViewGroupLeaf theMaterializedViewGroupItem = new MaterializedViewGroupLeaf(theTrafodionMaterializedViewGroup);
                    Nodes.Add(theMaterializedViewGroupItem);
                }
            }
        }
    }
}
