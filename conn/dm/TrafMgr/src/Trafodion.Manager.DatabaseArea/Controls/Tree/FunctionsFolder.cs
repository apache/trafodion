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
    public class FunctionsFolder : SchemaItemsFolder
    {
        /// <summary>
        /// Constructs a User Defined Functions Folder
        /// </summary>
        /// <param name="aTrafodionSchema">The parent schema object</param>
        public FunctionsFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.UserDefinedFunctions, aTrafodionSchema)
        {

        }

        /// <summary>
        /// Selects the UDFs folder in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionUDFunction)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is FunctionLeaf)
                    {
                        FunctionLeaf theFunctionLeaf = theNode as FunctionLeaf;
                        if (theFunctionLeaf.TrafodionUDF.InternalName.Equals(theTargetInternalName))
                        {
                            theFunctionLeaf.TreeView.SelectedNode = theFunctionLeaf;
                            return true;
                        }
                    }
                    else if (theNode is UniversalFunctionFolder)
                    {
                        UniversalFunctionFolder theUniversalFunctionFolder = theNode as UniversalFunctionFolder;
                        if (theUniversalFunctionFolder.TrafodionUDFunction.InternalName.Equals(theTargetInternalName))
                        {
                            theUniversalFunctionFolder.TreeView.SelectedNode = theUniversalFunctionFolder;
                            return true;
                        }
                    }

                }
            }
            else if (aTrafodionObject is TrafodionFunctionAction)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is UniversalFunctionFolder)
                    {
                        UniversalFunctionFolder theUniversalFunctionFolder = theNode as UniversalFunctionFolder;
                        if (theUniversalFunctionFolder.SelectTrafodionObject(aTrafodionObject))
                        {
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
            TheTrafodionSchema.TrafodionUDFunctions = null;
        }

        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionUDFunctions;
        }



        protected override void AddNodes()
        {
            foreach (TrafodionUDFunction theTrafodionUDF in TheTrafodionSchema.TrafodionUDFunctions)
            {
                if (theTrafodionUDF.IsUniversal)
                {
                    UniversalFunctionFolder theUniversalFunctionFolder = new UniversalFunctionFolder(theTrafodionUDF);
                    Nodes.Add(theUniversalFunctionFolder);
                }
                else
                {
                    FunctionLeaf theFunctionLeaf = new FunctionLeaf(theTrafodionUDF);
                    Nodes.Add(theFunctionLeaf);
                }
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionUDFunction theTrafodionUDF in TheTrafodionSchema.TrafodionUDFunctions)
            {
                if (nameFilter.Matches(theTrafodionUDF.ExternalName))
                {
                    if (theTrafodionUDF.IsUniversal)
                    {
                        UniversalFunctionFolder theUniversalFunctionFolder = new UniversalFunctionFolder(theTrafodionUDF);
                        Nodes.Add(theUniversalFunctionFolder);
                    }
                    else
                    {
                        FunctionLeaf theFunctionLeaf = new FunctionLeaf(theTrafodionUDF);
                        Nodes.Add(theFunctionLeaf);
                    }
                }
            }
        }
    }
}
