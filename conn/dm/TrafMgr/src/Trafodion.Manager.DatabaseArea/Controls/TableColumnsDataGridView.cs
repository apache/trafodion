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
    /// Data grid view for listing the table columns
    /// </summary>
    public class TableColumnsDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public TableColumnsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
        }

        /// <summary>
        /// Loads the column information into the datagridview
        /// </summary>
        /// <param name="aTrafodionTable">Table</param>
        /// <returns>result of the load, true or false</returns>
        public int Load(TrafodionTable aTrafodionTable)
        {

            List<TrafodionColumn> sqlMxTableColumns = aTrafodionTable.Columns;

            Columns.Clear();
            Rows.Clear();
            System.Windows.Forms.DataGridViewImageColumn primaryKeyColumn = new System.Windows.Forms.DataGridViewImageColumn();
            primaryKeyColumn.Name = Properties.Resources.PrimaryKey;
            primaryKeyColumn.HeaderText = Properties.Resources.PrimaryKey;
            primaryKeyColumn.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.Automatic;
            Columns.Add("theColumnNumberColumn", "Column Number");
            Columns.Add(primaryKeyColumn);
            Columns.Add("theColumnNameColumn", Properties.Resources.ColumnName);
            Columns.Add("theDataTypeColumn", Properties.Resources.DataType);
            Columns.Add("theNullableColumn", Properties.Resources.Nullable);
            Columns.Add("theDefaultColumn", Properties.Resources.Default);
            //Columns["thePrimaryKeyColumn"].ValueType = typeof(System.Drawing.Image);
            Columns.Add("theHeadingColumn", Properties.Resources.Heading);
            foreach (TrafodionTableColumn sqlMxTableColumn in sqlMxTableColumns)
            {
                int index = Rows.Add(new object[] {
                        sqlMxTableColumn.TheColumnNumber,
                        sqlMxTableColumn.InPrimaryKey ? global::Trafodion.Manager.Properties.Resources.PrimaryKeyIcon : global::Trafodion.Manager.Properties.Resources.BlankIcon,
                        sqlMxTableColumn.FormattedColumnName(), 
                        sqlMxTableColumn.FormattedDataType(),
                        sqlMxTableColumn.FormattedNullable()+" "+sqlMxTableColumn.FormattedNullDropable(),
                        sqlMxTableColumn.FormattedDefault(),
                        sqlMxTableColumn.FormattedHeading()
                    });

                if (Rows[index].Cells[Properties.Resources.PrimaryKey] is System.Windows.Forms.DataGridViewImageCell)
                {
                    ((System.Windows.Forms.DataGridViewImageCell)Rows[index].Cells[Properties.Resources.PrimaryKey]).Description = sqlMxTableColumn.InPrimaryKey ? "X" : "";
                }
            }

            return Rows.Count;
        }
    }

}
