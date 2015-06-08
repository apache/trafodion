//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
    /// Data grid view for listing the Division By Keys on a table/MV/Index
    /// </summary>
    public class DivisionByKeyDataGridView : DatabaseAreaObjectsDataGridView
    {
        /// <summary>
        /// Constructor for a datagrid view that holds information about the Division By Keys on a table/MV/Index
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public DivisionByKeyDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
            this.AutoSizeColumnsMode = System.Windows.Forms.DataGridViewAutoSizeColumnsMode.Fill;
        }

        /// <summary>
        /// Loads the division by information into the datagridview
        /// </summary>
        /// <param name="aTrafodionSchemaObject">Table or MV or Index</param>
        /// <returns>result of the load, true or false</returns>
        public int Load(TrafodionSchemaObject aTrafodionSchemaObject)
        {
            Columns.Clear();
            Rows.Clear(); 
            
            List<TrafodionDivisionByColumnDef> divisionByColumnDefs = aTrafodionSchemaObject.TheTrafodionDivisionByColumnDefs;

            Columns.Add("thePositionInRowColumn", Properties.Resources.ColumnPosition); 
            Columns.Add("Column Name", "Column Name");
            Columns.Add("Column Number", "Column Number");
            Columns.Add("Expression", "Expression");
            Columns.Add("theColumnNameColumn", Properties.Resources.SortOrder);
            Columns.Add("theDataTypeColumn", Properties.Resources.AddedBy);
            Columns["Expression"].FillWeight = 400;

            foreach (TrafodionDivisionByColumnDef sqlMxDivisionByColumnDef in divisionByColumnDefs)
            {
                Rows.Add(new object[] {
                    sqlMxDivisionByColumnDef.SequenceNumber,
                    sqlMxDivisionByColumnDef.ExternalName,
                    sqlMxDivisionByColumnDef.ColumnNumber,
                    sqlMxDivisionByColumnDef.Expression,
                    sqlMxDivisionByColumnDef.IsAscending ? Properties.Resources.Ascending : Properties.Resources.Descending,
                    sqlMxDivisionByColumnDef.TheSystemAddedColumn ? Properties.Resources.System : Properties.Resources.User
                });
            }

            return Rows.Count;
        }
    }
}
