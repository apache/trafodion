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
    /// Data grid view for listing the store order information on a table
    /// </summary>
    public class StoreOrderDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor for a datagrid view that holds information about the store order columns of a table
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public StoreOrderDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {            
        }

        /// <summary>
        /// Loads the store order information into the datagridview
        /// </summary>
        /// <param name="aTrafodionTable">Table</param>
        /// <returns>result of the load, true or false</returns>
        public int Load(TrafodionTable aTrafodionTable)
        {

            List<TrafodionStoreOrderColumnDef> sqlMxStoreOrderColumnDefs = aTrafodionTable.TheTrafodionStoreOrderColumnDefs;

            Columns.Clear();
            Rows.Clear();

            Columns.Add("thePositionInRowColumn", Properties.Resources.ColumnPosition);
            Columns.Add("theStoreOrderNameColumn", Properties.Resources.ColumnName);
            Columns.Add("theColumnNameColumn", Properties.Resources.SortOrder);
            Columns.Add("theDataTypeColumn", Properties.Resources.AddedBy);

            foreach (TrafodionStoreOrderColumnDef sqlMxStoreOrderColumnDef in sqlMxStoreOrderColumnDefs)
            {
                Rows.Add(new object[] {
                        sqlMxStoreOrderColumnDef.FormattedPositionInRow,
                        sqlMxStoreOrderColumnDef.ExternalName,
                        sqlMxStoreOrderColumnDef.IsAscending ? Properties.Resources.Ascending : Properties.Resources.Descending,
                        sqlMxStoreOrderColumnDef.TheSystemAddedColumn ? Properties.Resources.System : Properties.Resources.User
                    });
            }

            return Rows.Count;
        }
    }

}
