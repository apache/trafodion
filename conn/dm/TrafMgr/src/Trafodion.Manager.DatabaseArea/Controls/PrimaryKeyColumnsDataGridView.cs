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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;


namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagridview to display the list of primary key columns
    /// </summary>
    class PrimaryKeyColumnsDataGridView : DatabaseAreaObjectsDataGridView, ICloneToWindow
    {
        private TrafodionPrimaryKey _sqlMxPrimaryKey;

        /// <summary>
        /// Constructs a primary key datagridview
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A reference to the navigation tree</param>
        /// <param name="aTrafodionPrimaryKey">Primary key</param>
        public PrimaryKeyColumnsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionPrimaryKey aTrafodionPrimaryKey)
            : base(aDatabaseObjectsControl)
        {
            _sqlMxPrimaryKey = aTrafodionPrimaryKey;

            Columns.Add("theColumnPosition", Properties.Resources.ColumnPosition);
            Columns.Add("theNameColumn", Properties.Resources.ColumnName);
            Columns.Add("theDataTypeColumn", Properties.Resources.DataType);
            Columns.Add("theSortOrderColumn", Properties.Resources.SortOrder);
            Columns.Add("theAddedByColumn", Properties.Resources.AddedBy);

            int columnNumber = 0;
            foreach (TrafodionPrimaryKeyColumnDef sqlMxPrimaryKeyColumnDef in aTrafodionPrimaryKey.TheTrafodionPrimaryKeyColumnDefs)
            {
                columnNumber++;

                Rows.Add(new object[] {
                    columnNumber,
                    sqlMxPrimaryKeyColumnDef.TheTrafodionTableColumn.ExternalName,
                    sqlMxPrimaryKeyColumnDef.TheTrafodionTableColumn.FormattedDataType(),
                    sqlMxPrimaryKeyColumnDef.IsAscending ? Properties.Resources.Ascending : Properties.Resources.Descending,
                    sqlMxPrimaryKeyColumnDef.IsSystemAddedColumn ? Properties.Resources.System : Properties.Resources.User
                    });
            }
        }

        public TrafodionPrimaryKey TheTrafodionPrimaryKey
        {
            get { return _sqlMxPrimaryKey; }
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            return new PrimaryKeyColumnsDataGridView(null, TheTrafodionPrimaryKey);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>A string</returns>
        virtual public string WindowTitle
        {
            get { return TheTrafodionPrimaryKey.VisibleAnsiName + " Columns"; }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TheTrafodionPrimaryKey.ConnectionDefinition; }
        }

        #endregion

    }
}
