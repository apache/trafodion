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

using System.Collections.Generic;
using System.Data.Odbc;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A generic datagridview to display the details for a list of Indexes.
    /// </summary>
    public class SqlMxIndexesDetailsDataGridView :SqlMxSchemaObjectDetailsDataGridView 
    {

        /// <summary>
        /// Default constructor for the UI designer
        /// </summary>
        public SqlMxIndexesDetailsDataGridView()
            : base()
        {
        }

        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aParentSqlMxObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aSqlMxObjectList">The list of SqlMxSchemaObjects that need to be displayed</param>
        public SqlMxIndexesDetailsDataGridView(DatabaseObjectDetailsControl aDatabaseObjectDetailsControl,
            SqlMxObject aParentSqlMxObject,
            SqlMxSystem aSqlMxSystem,
            SqlMxCatalog aSqlMxCatalog,
            SqlMxSchema aSqlMxSchema)
            : base(aDatabaseObjectDetailsControl, aParentSqlMxObject, aSqlMxSystem, aSqlMxCatalog, aSqlMxSchema)
        {
        }


        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aParentSqlMxObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aSqlMxObjectList">The list of SqlMxSchemaObjects that need to be displayed</param>
        public SqlMxIndexesDetailsDataGridView(DatabaseObjectDetailsControl aDatabaseObjectDetailsControl,
            SqlMxObject aParentSqlMxObject,
            SqlMxSystem aSqlMxSystem,
            SqlMxCatalog aSqlMxCatalog,
            SqlMxSchema aSqlMxSchema,
            string aType)
            : base(aDatabaseObjectDetailsControl, aParentSqlMxObject, aSqlMxSystem, aSqlMxCatalog, aSqlMxSchema, aType)
        {
        }

        /// <summary>
        /// The object type to be displayed
        /// </summary>
        public override string ObjectType
        {
            get { return "Indexes"; }
        }

        /// <summary>
        /// The parent object type to be displayed
        /// </summary>
        public override string ParentObjectType
        {
            get {return  (TheParentSqlMxObject is SqlMxTable) ? "Table" : ((TheParentSqlMxObject is SqlMxMaterializedView) ? "Materialized View" : "Schema"); }
        }

        /// <summary>
        /// Load the datagridview with the Sql Object's data
        /// </summary>
        /// <returns></returns>
        override public int Load()
        {

            TheDataTable = TheSqlMxSchemaObjectsSummary.getIndexesSummary();
            return TheDataTable.Rows.Count;
        }

        /// <summary>
        /// Load the datagridview with the Sql Object's data
        /// </summary>
        /// <returns></returns>
        override public int Load(string aType)
        {
            if (TheParentSqlMxObject is SqlMxTable)
            {
                TheDataTable = TheSqlMxSchemaObjectsSummary.getIndexesForTable(TheParentSqlMxObject.ExternalName);
            } 
            else if (TheParentSqlMxObject is SqlMxMaterializedView)
            {
                TheDataTable = TheSqlMxSchemaObjectsSummary.getIndexesForMV(TheParentSqlMxObject.ExternalName);
            }
            return TheDataTable.Rows.Count;
        }


        /// <summary>
        /// Call this function if the columns ever need to change.
        /// </summary>
        override protected void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();
        }
    }
}
