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
    public class TriggersTabPage : SchemaLevelObjectListTabPage
    {
        private TrafodionTable _sqlMxTable;

        /// <summary>
        /// Constructs a triggers tab page and passes the parameters to the base class for storage
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionTable"></param>
        /// <param name="aTabName"></param>
        public TriggersTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTable aTrafodionTable, string aTabName)
            : base(aDatabaseObjectsControl, aTrafodionTable.TheTrafodionSchema, aTabName)
        {
            _headerText = Properties.Resources.ThisTableHasNObjects + aTabName; // "This table has {0} "
            _sqlMxTable = aTrafodionTable;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxTable.TrafodionTriggers;
        }
        /// <summary>
        /// Fills the page with the list of Triggers 
        /// A panel with the datagridview of the Triggers is added to the tab page using e generic helper class
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();
            TrafodionSchemaObjectListPanel<TrafodionTrigger> theTriggersListPanel = new TrafodionSchemaObjectListPanel<TrafodionTrigger>(TheDatabaseObjectsControl, HeaderText, TheTrafodionSchema, _sqlMxTable.TrafodionTriggers, TabName);
            theTriggersListPanel.Dock = DockStyle.Fill;
            Controls.Add(theTriggersListPanel);
        }


    }


}
