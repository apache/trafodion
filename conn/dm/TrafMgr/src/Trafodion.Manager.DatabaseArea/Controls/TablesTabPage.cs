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
    /// This is a tab page that displays a list of Table objets
    /// </summary>
    public class TablesTabPage : SchemaLevelObjectListTabPage
    {
        /// <summary>
        /// Constructs a tables tab page and passes the parameters to the base class for storage
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionSchema"></param>
        /// <param name="aTabName"></param>
        public TablesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema, string aTabName)
            : base(aDatabaseObjectsControl, aTrafodionSchema, aTabName)
        { }

        public override void PrepareForPopulate()
        {
            object a = TheTrafodionSchema.TrafodionTables;
        }
        /// <summary>
        /// Fills the table tab page with the list of tables. 
        /// A panel with the datagridview of the tables is added to the tab page
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();
            TrafodionSchemaObjectListPanel<TrafodionTable> tablesListPanel = new TrafodionSchemaObjectListPanel<TrafodionTable>(TheDatabaseObjectsControl, HeaderText, TheTrafodionSchema, TheTrafodionSchema.TrafodionTables, TabName);
            tablesListPanel.Dock = DockStyle.Fill;
            Controls.Add(tablesListPanel);
        }
    }

}
