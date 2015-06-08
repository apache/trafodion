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
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class is used to display indexes under a schema or indexable object such as a table or MV.
    /// </summary>
    class SqlMxIndexesSpaceUsagesTabPage : SchemaLevelObjectListTabPage
    {
        // The object whose indexes will be displayed. This must be a SqlMxSchema or IndexedSchemaObject.
        SqlMxObject _sqlMxObject = null;
        SqlMxSchema _sqlMxSchema = null;

        /// <summary>
        /// Instantiates a new tab page for a given schema.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A handle to the control with the tree. Used for hyperlinking.</param>
        /// <param name="aSqlMxSchema">The schema whose indexes will be displayed.</param>
        public SqlMxIndexesSpaceUsagesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, SqlMxSchema aSqlMxSchema)
            : base(aDatabaseObjectsControl, aSqlMxSchema, Properties.Resources.SpaceUsage)
        {
            _headerText = Properties.Resources.ThisSchemaHasNObjects + Text;
            _sqlMxSchema = aSqlMxSchema;
            CreatePanel();
        }

        /// <summary>
        /// Instantiates a new tab page for a given indexable object.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A handle to the control with the tree. Used for hyperlinking.</param>
        /// <param name="aIndexedSchemaObject">The indexable object whose indexes will be displayed.</param>
        public SqlMxIndexesSpaceUsagesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, IndexedSchemaObject aIndexedSchemaObject)
            : base(aDatabaseObjectsControl, aIndexedSchemaObject.TheSqlMxSchema, Properties.Resources.SpaceUsage)
        {
            _headerText = Properties.Resources.ThereAreNIndexes;
            _sqlMxObject = aIndexedSchemaObject;
        }

        /// <summary>
        /// Creates the panel. It expects _headerText and _sqlMxObject to be
        /// initialized prior to calling this function. _sqlMxObject
        /// must be a SqlMxSchema or IndexedSchemaObject object.
        /// </summary>
        private void CreatePanel()
        {
            this.Controls.Clear();
            DatabaseObjectDetailsControl detailsControl = new DatabaseObjectDetailsControl();
            detailsControl.Dock = DockStyle.Fill;
            this.Controls.Add(detailsControl);

            SqlMxSchemaObjectDetailsDataGridView detailsGrid = null;

            if (_sqlMxObject == null)
            {
                detailsGrid = new SqlMxIndexesDetailsDataGridView(detailsControl, _sqlMxSchema, _sqlMxSchema.TheSqlMxCatalog.SqlMxSystem, _sqlMxSchema.TheSqlMxCatalog, _sqlMxSchema);
                detailsGrid.DoLoad();
            }
            else
            {
                detailsGrid = new SqlMxIndexesDetailsDataGridView(detailsControl, _sqlMxObject, _sqlMxSchema.TheSqlMxCatalog.SqlMxSystem, _sqlMxSchema.TheSqlMxCatalog, _sqlMxSchema, "IX");
                detailsGrid.DoLoad("IX");
            }
        }
    }
}
