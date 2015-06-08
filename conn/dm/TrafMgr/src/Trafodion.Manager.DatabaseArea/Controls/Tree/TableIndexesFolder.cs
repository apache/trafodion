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
    /// Summary description for TablesFolder.
    /// </summary>
    public class TableIndexesFolder : TableItemsFolder
    {
        /// <summary>
        /// Constructor for the table indexes folder
        /// </summary>
        /// <param name="aTrafodionTable"></param>
        public TableIndexesFolder(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.Indexes, aTrafodionTable)
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
                    if (theNode is TableIndexLeaf)
                    {
                        TableIndexLeaf theTableIndexLeaf = theNode as TableIndexLeaf;
                        if (theTableIndexLeaf.TrafodionIndex.InternalName.Equals(theTargetInternalName))
                        {
                            theTableIndexLeaf.TreeView.SelectedNode = theTableIndexLeaf;
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
            TrafodionTable.RefreshIndexes();
        }

        protected override void PrepareForPopulate()
        {
            object c = TrafodionTable.TrafodionIndexes;
        }

        /// <summary>
        /// Populates the TableIndexesFolder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();

            foreach (TrafodionIndex theTrafodionTableIndex in TrafodionTable.TrafodionIndexes)
            {
                TableIndexLeaf theTableIndexLeaf = new TableIndexLeaf(theTrafodionTableIndex);
                Nodes.Add(theTableIndexLeaf);                 
            }
        }

        /// <summary>
        /// Returns the short description
        /// </summary>
        override public string ShortDescription
        {
            get
            {
                return Text + " on " + TrafodionTable.ExternalName;
            }
        }

        /// <summary>
        /// Long description
        /// </summary>
        override public string LongerDescription
        {
            get
            {
                return Text + " on Table " + TrafodionTable.VisibleAnsiName;
            }
        }
    }
}
