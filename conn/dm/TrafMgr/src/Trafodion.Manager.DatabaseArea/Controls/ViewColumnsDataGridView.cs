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
    /// Data grid view for listing the columns in a View.
    /// </summary>
    public class ViewColumnsDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database tree view if available for our use else null</param>
        public ViewColumnsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
        }

        /// <summary>
        /// Loads the column information into the datagridview
        /// </summary>
        /// <param name="aTrafodionView">View</param>
        /// <returns>result of the load, true or false</returns>
        public int Load(TrafodionView aTrafodionView)
        {

            List<TrafodionColumn> sqlMxViewColumns = aTrafodionView.Columns;

            Columns.Clear();
            Rows.Clear();

            Columns.Add("theColumnNumberColumn", "Column Number");
            Columns.Add("theColumnNameColumn", Properties.Resources.ColumnName);
            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("SourceObject", Properties.Resources.SourceObject));
            Columns.Add("theDataTypeColumn", Properties.Resources.DataType);
            Columns.Add("theNullableColumn", Properties.Resources.Nullable);
            Columns.Add("theDefaultColumn", Properties.Resources.Default);
            
            Columns.Add("theHeadingColumn", Properties.Resources.Heading);
            foreach (TrafodionViewColumn sqlMxViewColumn in sqlMxViewColumns)
            {
                Rows.Add(new object[] {
                        sqlMxViewColumn.TheColumnNumber,
                        sqlMxViewColumn.FormattedColumnName(), 
                        CreateLinkToObject(sqlMxViewColumn.GetUsedTable(sqlMxViewColumn.TheColumnNumber)),
                        sqlMxViewColumn.FormattedDataType(),
                        sqlMxViewColumn.FormattedNullable()+" "+sqlMxViewColumn.FormattedNullDropable(),
                        sqlMxViewColumn.FormattedDefault(),
                        sqlMxViewColumn.FormattedHeading()
                    });
            }

            return Rows.Count;
        }

        /// <summary>
        /// Creates the link to the name of the Sql schema object, in the name column of the datagridview
        /// </summary>
        /// <param name="aTrafodionSchemaObject"></param>
        /// <returns></returns>
        protected virtual DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionObject aTrafodionObject)
        {
            if (aTrafodionObject != null)
            {
                return new DatabaseAreaObjectsVisibleAnsiLink(TheDatabaseTreeView, aTrafodionObject);
            }
            else
            {
                return null;
            }
        }
                
    }
    

}
