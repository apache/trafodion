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
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Data grid view for listing the table's foreign key information
    /// </summary>
    public class ForeignKeyDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor for foreign key datagrid
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public ForeignKeyDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {            
        }

        /// <summary>
        /// Loads the foreign key information into the datagridview
        /// </summary>
        /// <param name="aTrafodionTable">Table</param>
        /// <returns>number of rows added to the datagrid</returns>
        public int Load(TrafodionTable aTrafodionTable)
        {
            // Tell the table to load the foreign key information            
            aTrafodionTable.LoadForeignKeys();
            List<TrafodionForeignKey> foreignKeyConstraints = aTrafodionTable.TheForeignKeyConstraints;

            Columns.Clear();
            Rows.Clear();

            Columns.Add("theForeignKeyNameColumn", Properties.Resources.Name);
            Columns.Add("theForeignKeyMetadataUID", Properties.Resources.MetadataUID);

            
            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.ForeignTable));
            Columns.Add("theLocalColumnName", Properties.Resources.LocalColumn);
            Columns.Add("theForeignColumnName", Properties.Resources.ForeignColumn);
            Columns.Add("theEnforcedAttribute", Properties.Resources.Enforced);

            foreach (TrafodionForeignKey foreignKeyConstraint in foreignKeyConstraints)
            {
                // Sort the local and foreign column lists if there is more than 1
                if (foreignKeyConstraint.TheForeignKeyLocalColumns.Count > 1)
                {
                    foreignKeyConstraint.TheForeignKeyLocalColumns.Sort(
                        delegate(TrafodionForeignKey.ForeignKeyLocalColumn p1, TrafodionForeignKey.ForeignKeyLocalColumn p2) { return p1.ColumnNumber.CompareTo(p2.ColumnNumber); });
                    foreignKeyConstraint.TheForeignKeyForeignColumns.Sort(
                        delegate(TrafodionForeignKey.ForeignKeyForeignColumn p1, TrafodionForeignKey.ForeignKeyForeignColumn p2) { return p1.ColumnNumber.CompareTo(p2.ColumnNumber); });
                }

                // Add the information to the datagrid
                for (int i = 0; i < foreignKeyConstraint.TheForeignKeyLocalColumns.Count; i++)
                {
                    Rows.Add(new object[] {
                        foreignKeyConstraint.ExternalName,
                        foreignKeyConstraint.UID,
                        new DatabaseAreaObjectsVisibleAnsiLink(TheDatabaseTreeView, foreignKeyConstraint.ForeignTable),
                        foreignKeyConstraint.TheForeignKeyLocalColumns[i].ColumnName,
                        foreignKeyConstraint.TheForeignKeyForeignColumns[i].ColumnName,   
                        Utilities.TrueFalse(foreignKeyConstraint.Enforced)
                    });
                }
            }
            
            return Rows.Count;
        }
    }

}
