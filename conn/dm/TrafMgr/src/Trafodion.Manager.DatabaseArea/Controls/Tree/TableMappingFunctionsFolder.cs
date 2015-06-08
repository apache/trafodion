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
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    class TableMappingFunctionsFolder : SchemaItemsFolder
    {
               /// <summary>
        /// Constructs a User Defined Functions Folder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public TableMappingFunctionsFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.TableMappingFunctions, aTrafodionSchema)
        {
            
        }

        /// <summary>
        /// Selects the UDFs folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionTableMappingFunction)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is TableMappingFunctionLeaf)
                    {
                        TableMappingFunctionLeaf theTableMappingFunctionLeaf = theNode as TableMappingFunctionLeaf;
                        if (theTableMappingFunctionLeaf.TrafodionTableMappingFunction.InternalName.Equals(theTargetInternalName))
                        {
                            theTableMappingFunctionLeaf.TreeView.SelectedNode = theTableMappingFunctionLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Resets the UDFs list in the parent schema. The list will be populated when it is accessed next
        /// </summary>
        /// <param name="aNameFilter">A navigation tree filter</param>
        /// 
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionTableMappingFunctions = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionTableMappingFunctions;
        }

        protected override void AddNodes()
        {
            foreach (TrafodionTableMappingFunction theTrafodionTableMappingFunction in TheTrafodionSchema.TrafodionTableMappingFunctions)
            {
                TableMappingFunctionLeaf theTableMappingFunctionLeaf = new TableMappingFunctionLeaf(theTrafodionTableMappingFunction);
                Nodes.Add(theTableMappingFunctionLeaf);
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionTableMappingFunction theTrafodionTableMappingFunction in TheTrafodionSchema.TrafodionTableMappingFunctions)
            {
                if (nameFilter.Matches(theTrafodionTableMappingFunction.ExternalName))
                {
                    TableMappingFunctionLeaf theTableMappingFunctionLeaf = new TableMappingFunctionLeaf(theTrafodionTableMappingFunction);
                    Nodes.Add(theTableMappingFunctionLeaf);
                }
            }
        }
    }
}
