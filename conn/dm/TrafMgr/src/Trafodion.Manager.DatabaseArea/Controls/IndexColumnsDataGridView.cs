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

using System.Collections.Generic;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagrid view for Index Columns
    /// </summary>
    public class IndexColumnsDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor for index columns
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public IndexColumnsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
        }

        /// <summary>
        /// Loads the Index Columns Data Grid view with the information from an index
        /// </summary>
        /// <param name="aTrafodionIndex"></param>
        /// <returns></returns>
        public int Load(TrafodionIndex aTrafodionIndex)
        {

            List<TrafodionIndexColumn> theTrafodionIndexColumnDefs = aTrafodionIndex.TrafodionColumns;

            Columns.Clear();
            Rows.Clear();

            Columns.Add("theColumnNameColumn", Properties.Resources.ColumnName);
            Columns.Add("theOrderByColumn", Properties.Resources.SortOrder);
            Columns.Add("theAddedByColumn", Properties.Resources.AddedBy);

            foreach (TrafodionIndexColumn theTrafodionIndexColumnDef in theTrafodionIndexColumnDefs)
            {
                Rows.Add(new object[] {
                        theTrafodionIndexColumnDef.ExternalName, 
                        theTrafodionIndexColumnDef.IsAscending ? Properties.Resources.Ascending : Properties.Resources.Descending,
                        theTrafodionIndexColumnDef.TheSystemAddedColumn ? Properties.Resources.System : Properties.Resources.User
                        });
            }

            return Rows.Count;
        }
    }

}
