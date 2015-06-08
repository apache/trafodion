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

using System;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that display the histograms statistics for a sql table
    /// </summary>
    class TableStatisticsPanel : TablePanel
    {
        /// <summary>
        /// Create a panel that displays information about a table's statistics.
        /// </summary>
        /// <param name="aTrafodionTable">The TrafodionSchemaObject whose usage is to be displayed</param>
        public TableStatisticsPanel(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.Statistics, aTrafodionTable)
        {
            Load();
        }

        /// <summary>
        /// Populate the panel with the user control that display the statistics
        /// </summary>
        protected override void Load()
        {
            TableHistogramStatsControl tableStatsControl = new TableHistogramStatsControl(TrafodionTable);
            tableStatsControl.Dock = DockStyle.Fill;
            Controls.Add(tableStatsControl);
        }

        /// <summary>
        /// Clone the panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new TableStatisticsPanel(TrafodionTable);
        }
    }
}
