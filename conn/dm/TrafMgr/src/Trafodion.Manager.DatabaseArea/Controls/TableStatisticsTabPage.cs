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
    /// A TabPage that displays table statistics.
    /// </summary>
    public class TableStatisticsTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionTable _sqlMxTable;

        /// <summary>
        /// Create a TabPage that displays a table statistics
        /// </summary>
        /// <param name="aTrafodionTable">The table whose statistics is to be displayed.</param>
        public TableStatisticsTabPage(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.Statistics)
        {
            _sqlMxTable = aTrafodionTable;
        }

        public override void PrepareForPopulate()
        {
            _sqlMxTable.LoadTableStatistics();
        }

        /// <summary>
        /// Populates the tab page with a panel that displays the table statistics
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            TableStatisticsPanel tableStatisticsPanel = new TableStatisticsPanel(TrafodionTable);
            tableStatisticsPanel.Dock = DockStyle.Fill;
            Controls.Add(tableStatisticsPanel);
        }


        #region Properties

        /// <summary>
        /// The table property.  Changing it to a new value will not have an effect
        /// until the Refresh() method is called.  We don't call it directly because we want this page to be delayed.
        /// </summary>
        public TrafodionTable TrafodionTable
        {
            get { return _sqlMxTable; }
        }

        #endregion Properties

    }
}
