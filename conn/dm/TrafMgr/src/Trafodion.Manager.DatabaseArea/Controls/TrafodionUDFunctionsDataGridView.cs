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
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A datagridview that displays a list of SqlMxViews
    /// </summary>
    class SqlMxUDFunctionsDataGridView : SqlMxSchemaObjectsDataGridView<SqlMxUDFunction>
    {
        public SqlMxUDFunctionsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, SqlMxObject aParentSqlMxObject, List<SqlMxUDFunction> aSqlMxObjectList)
            : base(aDatabaseObjectsControl, aParentSqlMxObject, aSqlMxObjectList)
        {
        }

        /// <summary>
        /// Override the base Load and implement a custom load to load the view's data into the datagridview
        /// </summary>
        /// <returns></returns>
        override public int Load()
        {
            Rows.Clear();

            foreach (SqlMxUDFunction sqlMxUDFunction in TheSqlMxObjects)
            {
                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxUDFunction.VisibleAnsiName))
                {
                    Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxUDFunction), 
                            sqlMxUDFunction.FormattedType,
                            sqlMxUDFunction.UID, 
                            sqlMxUDFunction.FormattedCreateTime(),
                            sqlMxUDFunction.FormattedRedefTime()
                    });
                }
            }

            return Rows.Count;
        }

        /// <summary>
        /// Call this function if the columns ever need to change.
        /// </summary>
        protected override void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));
            Columns.Add("theTypeColumn", Properties.Resources.FunctionType);
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }
    }
}
