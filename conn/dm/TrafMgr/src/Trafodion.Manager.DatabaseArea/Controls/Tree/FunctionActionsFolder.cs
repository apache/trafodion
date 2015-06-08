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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Navigation;


namespace Trafodion.Manager.DatabaseArea.Controls.Tree
{
    public class FunctionActionsFolder : SchemaItemsFolder
    {
        private TrafodionUDFunction _sqlMxUDFunction;

        public TrafodionUDFunction TrafodionUDFunction
        {
            get { return _sqlMxUDFunction; }

        }

        public FunctionActionsFolder(TrafodionSchema aTrafodionSchema)
            : base(Properties.Resources.FunctionActions, aTrafodionSchema)
        {

        }

        public FunctionActionsFolder(TrafodionUDFunction aTrafodionUDFunction)
            : base(Properties.Resources.FunctionActions, aTrafodionUDFunction.TheTrafodionSchema)
        {
            _sqlMxUDFunction = aTrafodionUDFunction;
        }

        public TrafodionFunctionAction TrafodionFunctionAction
        {
            get { return (TrafodionFunctionAction)this.TrafodionObject; }

        }

        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionFunctionAction)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is FunctionActionLeaf)
                    {
                        FunctionActionLeaf theFunctionActionLeaf = theNode as FunctionActionLeaf;
                        if (theFunctionActionLeaf.TrafodionFunctionAction.InternalName.Equals(theTargetInternalName))
                        {
                            theFunctionActionLeaf.TreeView.SelectedNode = theFunctionActionLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TheTrafodionSchema.TrafodionFunctionActions = null;
        }
        protected override void PrepareForPopulate()
        {
            object c = TheTrafodionSchema.TrafodionFunctionActions;
        }



        protected override void AddNodes()
        {
            if (TrafodionUDFunction == null)
            {
                foreach (TrafodionFunctionAction theTrafodionFunctionAction in TheTrafodionSchema.TrafodionFunctionActions)
                {
                    FunctionActionLeaf theFunctionActionLeaf = new FunctionActionLeaf(theTrafodionFunctionAction);
                    Nodes.Add(theFunctionActionLeaf);
                }
            }
            else
            {
                foreach (TrafodionFunctionAction theTrafodionFunctionAction in TrafodionUDFunction.TrafodionFunctionActions)
                {
                    FunctionActionLeaf theFunctionActionLeaf = new FunctionActionLeaf(theTrafodionFunctionAction);
                    Nodes.Add(theFunctionActionLeaf);
                }
            }
        }

        protected override void AddNodes(NavigationTreeNameFilter nameFilter)
        {
            foreach (TrafodionFunctionAction theTrafodionFunctionAction in TheTrafodionSchema.TrafodionFunctionActions)
            {
                if (TrafodionUDFunction.ExternalName.Equals(theTrafodionFunctionAction.TrafodionUDFunction.ExternalName))
                {
                    if (nameFilter.Matches(theTrafodionFunctionAction.ExternalName))
                    {
                        FunctionActionLeaf theFunctionActionLeaf = new FunctionActionLeaf(theTrafodionFunctionAction);
                        Nodes.Add(theFunctionActionLeaf);
                    }
                }
            }
        }
    }
}
