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
    /// Data grid view for listing the Check Constraints on a table
    /// </summary>
    public class CheckConstraintsDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor for a datagrid view that holds information about the check constraints of a table
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public CheckConstraintsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {            
        }

        /// <summary>
        /// Loads the check constraints data into the datagridview
        /// </summary>
        /// <param name="aTrafodionTable">Table</param>
        /// <returns>result of the load, true or false</returns>
        public int Load(TrafodionTable aTrafodionTable)
        {
            aTrafodionTable.LoadCheckConstraints();
            List<TrafodionCheckConstraint> sqlMxCheckConstraints = aTrafodionTable.TheCheckConstraints;

            Columns.Clear();
            Rows.Clear();

            Columns.Add("theCheckConstraintName", Properties.Resources.ConstraintName);
            Columns.Add("theCheckConstraintUID", Properties.Resources.MetadataUID);
            Columns.Add("theCheckConstraintText", Properties.Resources.Text);

            // Turn on cell wrapping for the text column and give more weight to the text column
            DataGridViewColumn theCheckConstraintTextColumn = this.Columns["theCheckConstraintText"];
            theCheckConstraintTextColumn.DefaultCellStyle.WrapMode = DataGridViewTriState.True;
            theCheckConstraintTextColumn.FillWeight = 2;

            DataGridViewColumn theCheckConstraintNameColumn = this.Columns["theCheckConstraintName"];
            theCheckConstraintNameColumn.FillWeight = 1;

            DataGridViewColumn theCheckConstraintUIDColumn = this.Columns["theCheckConstraintUID"];
            theCheckConstraintUIDColumn.FillWeight = 1;

            sqlMxCheckConstraints.Sort();

            foreach (TrafodionCheckConstraint theTrafodionCheckConstraint in sqlMxCheckConstraints)
            {
                Rows.Add(new object[] {
                        theTrafodionCheckConstraint.VisibleAnsiName,
                        theTrafodionCheckConstraint.UID,
                        theTrafodionCheckConstraint.Text
                    });
            }

            return Rows.Count;
        }
    }

}
