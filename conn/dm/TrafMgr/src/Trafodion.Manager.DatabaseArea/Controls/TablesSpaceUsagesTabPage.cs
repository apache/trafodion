//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
    /// This is a tab page that displays a list of Table objects' space usage
    /// </summary>
    public class TablesSpaceUsagesTabPage : SchemaLevelObjectListTabPage
    {
        private DatabaseObjectsControl _theDatabaseObjectsCcontrol; 
        private SqlMxSchema _theSqlMxSchema;

        /// <summary>
        /// Constructs a tables tab page and passes the parameters to the base class for storage
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aSqlMxSchema"></param>
        /// <param name="aTabName"></param>
        public TablesSpaceUsagesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, SqlMxSchema aSqlMxSchema, string aTabName)
            : base(aDatabaseObjectsControl, aSqlMxSchema, aTabName)
        {
            _theDatabaseObjectsCcontrol = aDatabaseObjectsControl;
            _theSqlMxSchema = aSqlMxSchema;
            CreatePanel();
        }

        private void CreatePanel()
        {
            this.Controls.Clear();
            DatabaseObjectDetailsControl detailsControl = new DatabaseObjectDetailsControl();
            detailsControl.Dock = DockStyle.Fill;
            this.Controls.Add(detailsControl);

            SqlMxSchemaObjectDetailsDataGridView detailsGrid = null;

            detailsGrid = new SqlMxTableDetailsDataGridView(detailsControl, _theSqlMxSchema, _theSqlMxSchema.TheSqlMxCatalog.SqlMxSystem, _theSqlMxSchema.TheSqlMxCatalog, _theSqlMxSchema);
            detailsGrid.DoLoad();
        }
    }

}
