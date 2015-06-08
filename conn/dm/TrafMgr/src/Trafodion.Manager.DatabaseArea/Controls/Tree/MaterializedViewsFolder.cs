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
    /// Summary description for MaterializedViewsFolder.
    /// </summary>
    public class MaterializedViewsFolder : SchemaItemsFolder
    {

        /// <summary>
        /// Constructs a MaterializedView Folder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public MaterializedViewsFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.MaterializedViews, aTrafodionSchema)
        {
        }

        /// <summary>
        /// Selects the Materialized Views  folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if ( (aTrafodionObject is TrafodionMaterializedView) || (aTrafodionObject is TrafodionIndex) )
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    MaterializedViewFolder theMaterializedViewFolder = theNode as MaterializedViewFolder;
                    if (aTrafodionObject is TrafodionMaterializedView)
                    {
                        if (theMaterializedViewFolder.TrafodionMaterializedView.InternalName.Equals(theTargetInternalName))
                        {
                            theMaterializedViewFolder.TreeView.SelectedNode = theMaterializedViewFolder;
                            return true;
                        }
                    }
                    else
                    {
                        if (theMaterializedViewFolder.SelectTrafodionObject(aTrafodionObject))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the MV list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        /// 
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionMaterializedViews = null;
        }
        
        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionMaterializedViews;
        }
        
        protected override void AddNodes()
        {
            foreach (TrafodionMaterializedView theTrafodionMaterializedView in TheTrafodionSchema.TrafodionMaterializedViews)
            {
                MaterializedViewFolder theMaterializedViewItem = new MaterializedViewFolder(theTrafodionMaterializedView);
                Nodes.Add(theMaterializedViewItem);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionMaterializedView theTrafodionMaterializedView in TheTrafodionSchema.TrafodionMaterializedViews)
            {
                if (nameFilter.Matches(theTrafodionMaterializedView.ExternalName))
                {
                    MaterializedViewFolder theMaterializedViewItem = new MaterializedViewFolder(theTrafodionMaterializedView);
                    Nodes.Add(theMaterializedViewItem);
                }
            }
        }
    }
}
