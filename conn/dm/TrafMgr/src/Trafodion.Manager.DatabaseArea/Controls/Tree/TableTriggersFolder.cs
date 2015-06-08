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
    /// Summary description for TablesTriggersFolder.
    /// </summary>
    public class TableTriggersFolder : TableItemsFolder
    {
        /// <summary>
        /// Constructor for the table triggers folder
        /// </summary>
        /// <param name="aTrafodionTable"></param>
        public TableTriggersFolder(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.Triggers, aTrafodionTable)
        {
        }

        /// <summary>
        /// Tries to find the TrafodionObject in its list of table triggers, returns false if no match is found
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        override public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionTrigger)
            {
                DoPopulate(null);

                string theTargetInternalName = aTrafodionObject.InternalName;
                foreach (TreeNode theNode in Nodes)
                {
                    if (theNode is TableTriggerLeaf)
                    {
                        TableTriggerLeaf theTableTriggerLeaf = theNode as TableTriggerLeaf;
                        if (theTableTriggerLeaf.TrafodionTrigger.InternalName.Equals(theTargetInternalName))
                        {
                            theTableTriggerLeaf.TreeView.SelectedNode = theTableTriggerLeaf;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        /// <summary>
        /// Refresh the table triggers folder, clears the table triggers
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TrafodionTable.ClearTriggers();
        }

        protected override void PrepareForPopulate()
        {
            object c = TrafodionTable.TrafodionTriggers;
        }
        /// <summary>
        /// Populates the TableTriggersFolder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();


            foreach (TrafodionTrigger theTrafodionTableTrigger in TrafodionTable.TrafodionTriggers)
                {
                    TableTriggerLeaf theTableTriggerLeaf = new TableTriggerLeaf(theTrafodionTableTrigger);
                    Nodes.Add(theTableTriggerLeaf);                 
                }
            

        }

    }
}
