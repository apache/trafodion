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
    /// Data grid view for listing the Hash Key information on a table
    /// </summary>
    public class HashKeyDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor for a datagrid view that holds information about the Hash Key columns of a table
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public HashKeyDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {            
        }

        /// <summary>
        /// Loads the Hash Key information into the datagridview
        /// </summary>
        /// <param name="aTrafodionTable">Table</param>
        /// <returns>result of the load, # of rows</returns>
        public int Load(TrafodionTable aTrafodionTable)
        {

            List<TrafodionHashKeyColumnDef> theTrafodionHashKeyColumnDefs = 
                                                aTrafodionTable.TheTrafodionHashKeyColumnDefs;

            Columns.Clear();
            Rows.Clear();

            Columns.Add("thePartKeySeqNumber", "Key Sequence");
            //Columns.Add("thePositionInRowColumn", "Ordinal Position Withing Access Path");
            Columns.Add("theHashKeyColumnName", Properties.Resources.ColumnName);
            //Columns.Add("theHashKeyColumnNumber", "Position in Base Table");
            Columns.Add("theSortOrderColumn", Properties.Resources.SortOrder);

            foreach (TrafodionHashKeyColumnDef theTrafodionHashKeyColumnDef in theTrafodionHashKeyColumnDefs)
            {
                Rows.Add(new object[] 
                {
                    theTrafodionHashKeyColumnDef.ThePartKeySeqNum,
                    //theTrafodionHashKeyColumnDef.FormattedPositionInRow,
                    theTrafodionHashKeyColumnDef.ExternalName,
                    //theTrafodionHashKeyColumnDef.FormattedColumnNumber,
                    theTrafodionHashKeyColumnDef.IsAscending ? Properties.Resources.Ascending : Properties.Resources.Descending
                });
            }
            return Rows.Count;
        }
    }

}
