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
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagridview to display table attributes
    /// </summary>
    public class TableAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructs the datagridview to display table attributes
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A reference to the database navigation tree</param>
        /// <param name="aTrafodionTable">Table whose attributes are to be displayed</param>
        public TableAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTable aTrafodionTable)
            : base(aDatabaseObjectsControl, aTrafodionTable)
        {
        }
        /// <summary>
        /// The table whose attributes are displayed in the datagridview
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return TrafodionObject as TrafodionTable; }
        }

        /// <summary>
        /// Loads the attribute inforation into the datagridview for display
        /// </summary>
        override protected void LoadRows()
        {

            //AddRow(Properties.Resources.LogInsertsOnly, TrafodionTable.FormattedIsInsertLog);
            //AddRow(Properties.Resources.ReorgEnabled, TrafodionTable.IsReorgEnabled);
            //AddRow(Properties.Resources.UpdateStatsEnabled, TrafodionTable.IsUpdateStatsEnabled);
            //AddRow(Properties.Resources.LastUpdateStatistics, TrafodionTable.FormattedLastUpdateStatsTimestamp);
            //AddRow(Properties.Resources.AuditCompressed, TrafodionTable.FormattedIsAuditCompress);
            //AddRow(Properties.Resources.ClearOnPurge, TrafodionTable.FormattedIsClearOnPurge);
            //AddRow(Properties.Resources.BlockSize, TrafodionTable.FormattedBlockSize);
            //AddRow(Properties.Resources.MaximumSize, TrafodionTable.FormattedMaxTableSize);
            //AddRow(Properties.Resources.RecordSize, TrafodionTable.FormattedRecordSize);
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new TableAttributesDataGridView(null, TrafodionTable);
        }

        #endregion

    }
}
