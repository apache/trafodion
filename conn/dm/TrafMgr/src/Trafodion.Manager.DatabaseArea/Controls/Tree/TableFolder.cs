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
    /// Summary description for TableFolder.
    /// </summary>
    public class TableFolder : DatabaseTreeFolder
    {
        /// <summary>
        /// Creates a TableFolder object that represents a specifc Sql table in the tree
        /// </summary>
        /// <param name="aTrafodionTable"></param>
        public TableFolder(TrafodionTable aTrafodionTable)
            :base(aTrafodionTable)
        {
            ImageKey = DatabaseTreeView.DB_TABLE_ICON;
            SelectedImageKey = DatabaseTreeView.DB_TABLE_ICON;
        }

        /// <summary>
        /// Selects the Table in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject is TrafodionTable)
            {
                TrafodionTable theTrafodionTable = aTrafodionObject as TrafodionTable;
                string theTargetInternalName = theTrafodionTable.InternalName;

                DoPopulate(null);

                foreach (TreeNode theTreeNode in Nodes)
                {
                    if (theTreeNode is TableFolder)
                    {
                        TableFolder theTableFolder = theTreeNode as TableFolder;
                        if (theTableFolder.TrafodionTable.InternalName.Equals(theTargetInternalName))
                        {
                            theTableFolder.TreeView.SelectedNode = theTableFolder;
                            return true;
                        }
                    }
                }
            }
            else 
            {
                DoPopulate(null);

                foreach (TreeNode theTreeNode in Nodes)
                {
                    if (theTreeNode is TableItemsFolder)
                    {
                        TableItemsFolder theTableItemsFolder = theTreeNode as TableItemsFolder;
                        if (theTableItemsFolder.SelectTrafodionObject(aTrafodionObject))
                        {
                            return true;
                        }
                    }
                }
            }
            

            return false;
        }

        /// <summary>
        /// Refreshes the view associated with this folder and throws away the current indexes folder.
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TrafodionTable.Refresh();
            Nodes.Clear();
        }

        /// <summary>
        /// Clears the children nodes
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            TableIndexesFolder theTableIndexesFolder = new TableIndexesFolder(TrafodionTable);
            Nodes.Add(theTableIndexesFolder);
            TableTriggersFolder theTableTriggersFolder = new TableTriggersFolder(TrafodionTable);
            Nodes.Add(theTableTriggersFolder);
        }

        /// <summary>
        /// Property that sets/gets the assoicate Table object
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return (TrafodionTable)this.TrafodionObject; }
        }

        /// <summary>
        /// Long description of the table
        /// </summary>
        override public string LongerDescription
        {
            get { return "Table " + TrafodionTable.VisibleAnsiName; }
        }

    }
}
