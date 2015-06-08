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

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A TabPage that displays a table's attributes.
    /// </summary>
    public class TableAttributesTabPage : DelayedPopulateClonableTabPage, IHasTrafodionTable
    {
        private TrafodionTable _sqlMxTable;
        /// <summary>
        /// The table object whose attributes are displayed in this tab page
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return _sqlMxTable; }
        }

        /// <summary>
        /// Create a TabPage that displays a table's attributes.
        /// </summary>
        /// <param name="aTrafodionTable">The table whose columns are to be displayed.</param>
        public TableAttributesTabPage(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.Attributes)
        {
             _sqlMxTable = aTrafodionTable;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxTable.TheBlockSize;
            a = _sqlMxTable.TheLastUpdateStatsTimestamp;
            a = _sqlMxTable.IsUpdateStatsEnabled;
            //a = _sqlMxTable.IsInsertLog;
        }
        

        /// <summary>
        /// Constructs a table attributes panel and adds it to the table attributes tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            TableAttributesPanel tableAttributesPanel = new TableAttributesPanel(TrafodionTable);
            tableAttributesPanel.Dock = DockStyle.Fill;
            Controls.Add(tableAttributesPanel);
        }
    }

}
