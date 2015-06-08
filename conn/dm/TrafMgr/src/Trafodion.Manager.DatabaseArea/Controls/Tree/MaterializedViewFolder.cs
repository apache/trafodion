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
	/// Summary description for MaterializedViewFolder.
	/// </summary>
    public class MaterializedViewFolder : DatabaseTreeFolder
	{
        /// <summary>
        /// Creates a folder to hold a materialized view
        /// </summary>
        /// <param name="aTrafodionMaterializedView"></param>
		public MaterializedViewFolder(TrafodionMaterializedView aTrafodionMaterializedView)
            :base(aTrafodionMaterializedView)
		{
            ImageKey = DatabaseTreeView.DB_VIEW_ICON;
            SelectedImageKey = DatabaseTreeView.DB_VIEW_ICON;
		}

        /// <summary>
        /// Selects the Materizlied View in the Database navigation tree
        /// </summary>
        /// <param name="aTrafodionObject"></param>
        /// <returns></returns>
        public bool SelectTrafodionObject(TrafodionObject aTrafodionObject)
        {

            DoPopulate(null);

            foreach (TreeNode theTreeNode in Nodes)
            {
                if (theTreeNode is MaterializedViewItemsFolder)
                {
                    MaterializedViewItemsFolder theMVItemsFolder = theTreeNode as MaterializedViewItemsFolder;
                    if (theMVItemsFolder.SelectTrafodionObject(aTrafodionObject))
                    {
                        return true;
                    }
                }
            }


            return false;
        }

        /// <summary>
        /// Refreshes the folder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Refresh(NavigationTreeNameFilter aNameFilter)
        {
            TrafodionMaterializedView.Refresh();
        }

        /// <summary>
        /// Populates the tree with a Materialized View Indexes folder
        /// </summary>
        /// <param name="aNameFilter"></param>
        override protected void Populate(NavigationTreeNameFilter aNameFilter)
        {
            Nodes.Clear();
            MaterializedViewIndexesFolder theMVIndexesFolder = new MaterializedViewIndexesFolder(TrafodionMaterializedView);
            Nodes.Add(theMVIndexesFolder);
        }

        /// <summary>
        /// The model of the materialized view contained in this node
        /// </summary>
        public TrafodionMaterializedView TrafodionMaterializedView
		{
			get { return (TrafodionMaterializedView)this.TrafodionObject; }
		}

        /// <summary>
        /// Longer description
        /// </summary>
		override public string LongerDescription
		{
			get
			{
                return "Materialized View " + TrafodionMaterializedView.VisibleAnsiName;
			}
		}
	}
}
