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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagridview to display MV Group members 
    /// </summary>
    public class MaterializedViewGroupMembersDataGridView : DatabaseAreaObjectsDataGridView
    {
        /// <summary>
        /// Constructs the datagridview to display MaterializedViewGroup members
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A reference to the database navigation tree</param>
        /// <param name="aTrafodionMaterializedViewGroup">MaterializedViewGroup whose members are to be displayed</param>
        public MaterializedViewGroupMembersDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {

        }

 
        /// <summary>
        /// Loads the MaterializedViewGroup members Data Grid view with the information from an MaterializedViewGroup
        /// </summary>
        /// <param name="aTrafodionMaterializedViewGroup"></param>
        /// <returns>RowCount</returns>
        public int Load(TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
        {
            // Method usually called by MaterializedViewGroupMembersPanel::Load()

            List<TrafodionMaterializedView> theTrafodionMaterializedViews = aTrafodionMaterializedViewGroup.TheTrafodionMaterializedViews;

            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);

            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);


            foreach (TrafodionMaterializedView theMV in theTrafodionMaterializedViews)
            {
                Rows.Add(new object[] {
                            CreateLinkToObject(theMV), 
                            theMV.UID, 
                            theMV.FormattedCreateTime(),
                            theMV.FormattedRedefTime()
                    });
            }

            return Rows.Count;
        }



        /// <summary>
        /// Creates the link to the name of the Sql MV object, in the name column of the datagridview
        /// </summary>
        /// <param name="aTrafodionMaterializedView"></param>
        /// <returns></returns>
        protected virtual DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionMaterializedView aTrafodionMaterializedView)
        {
            // Note: Leverage DatabaseAreaObjectsVisibleAnsiLink to get 2 or 3 part ANSI  names  to display here 
            // instead of DatabaseAreaObjectsDataGridViewLink 
            return new DatabaseAreaObjectsVisibleAnsiLink(TheDatabaseTreeView, aTrafodionMaterializedView);
        }
    }
}
