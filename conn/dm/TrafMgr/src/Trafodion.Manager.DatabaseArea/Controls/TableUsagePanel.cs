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

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A panel that displays information about a table's usage relationships.
    /// </summary>
    public class TableUsagePanel : TablePanel
    {
        private DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// Create a panel that displays information about a table's usage relationships.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The DatabaseTreeView control</param>
        /// <param name="aTrafodionTable">The table whose usage is to be displayed</param>
        public TableUsagePanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTable aTrafodionTable)
            : base(Properties.Resources.Usage, aTrafodionTable)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }

        /// <summary>
        /// Populate the panel
        /// </summary>
        protected override void Load()
        {
            //Create the usage panel and add to parent container.
            TrafodionUsagePanel<TrafodionTable> usagePanel = new TrafodionUsagePanel<TrafodionTable>(_databaseObjectsControl, TrafodionTable);
            usagePanel.Dock = DockStyle.Fill;
            usagePanel.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(usagePanel);

            //Add the different types of usage objects to the usage panel.
            usagePanel.AddUsageObjects(Properties.Resources.UsedBy, TrafodionTable.TheTrafodionMaterializedViews);
            usagePanel.AddUsageObjects(Properties.Resources.UsedBy, TrafodionTable.TheTrafodionViews);
            usagePanel.AddUsageObjects(Properties.Resources.Has, TrafodionTable.TheTrafodionSynonyms);
        }

        /// <summary>
        /// Clone the panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new TableUsagePanel(null, TrafodionTable);
        }
    }

}
