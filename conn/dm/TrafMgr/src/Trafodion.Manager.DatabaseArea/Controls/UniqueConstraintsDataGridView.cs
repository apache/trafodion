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
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Data grid view for listing the Unique Constraints on a table
    /// </summary>
    public class UniqueConstraintsDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor for a datagrid view that holds information about the Unique constraints of a table
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public UniqueConstraintsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
        }

        /// <summary>
        /// Loads the Unique constraints data into the datagridview
        /// </summary>
        /// <param name="aTrafodionTable">Table</param>
        /// <returns>result of the load, true or false</returns>
        public int Load(TrafodionTable aTrafodionTable)
        {
            Columns.Clear();
            Columns.Add("theUniqueConstraintName", Properties.Resources.ConstraintName);
            Columns.Add("theUniqueConstraintUID", Properties.Resources.MetadataUID);
            Columns.Add("theUniqueConstraintColumns", Properties.Resources.Columns);

            //Reset the datagrid contents
            Rows.Clear();

            //Ask the table model to load the unique constraints
            aTrafodionTable.LoadUniqueConstraints();

            foreach (TrafodionUniqueConstraint theTrafodionUniqueConstraint in aTrafodionTable.TheUniqueConstraints)
            {
                Rows.Add(new object[] {
                    theTrafodionUniqueConstraint.VisibleAnsiName,
                    theTrafodionUniqueConstraint.UID,
                    theTrafodionUniqueConstraint.FormattedColumnNames,
                    ""
                });
            }

            return Rows.Count;
        }
    }

}
