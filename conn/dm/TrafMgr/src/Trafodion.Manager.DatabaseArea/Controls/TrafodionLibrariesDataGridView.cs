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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A generic datagridview to display a list of Libraries.
    /// </summary>
    public class SqlMxLibrariesDataGridView : SqlMxSchemaObjectsDataGridView<SqlMxLibrary>
    {
        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control has access to the DatabaseTreeView</param>
        /// <param name="aParentSqlMxObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aSqlMxObjectList">The list of SqlMxSchemaObjects that need to be displayed</param>
        public SqlMxLibrariesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, SqlMxObject aParentSqlMxObject, List<SqlMxLibrary> aSqlMxObjectList)
            : base(aDatabaseObjectsControl, aParentSqlMxObject, aSqlMxObjectList)
        {
        }

        /// <summary>
        /// Load the datagridview with the Sql Object's data
        /// </summary>
        /// <returns></returns>
        override public int Load()
        {
            Rows.Clear();

            foreach (SqlMxLibrary sqlMxLibrary in TheSqlMxObjects)
            {
                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxLibrary.VisibleAnsiName))
                {
                    if (sqlMxLibrary.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxLibrary),
                            sqlMxLibrary.IsMetadataObject?"System":"User",
                            sqlMxLibrary.UID,                            
                            sqlMxLibrary.FormattedCreateTime(),
                            sqlMxLibrary.FormattedRedefTime()
                        });
                    }
                    else 
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxLibrary), 
                            //sqlMxLibrary.FileName, 
                            sqlMxLibrary.UID,
                            sqlMxLibrary.FormattedCreateTime(),
                            sqlMxLibrary.FormattedRedefTime()
                        });
                    }
                    
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
            if (TheParentSqlMxObject != null && TheParentSqlMxObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140)
            {
                Columns.Add("theTypeColumn", "Type");
            }
            //Columns.Add("theFileNameColumn", Properties.Resources.FileName);
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);           
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }
    }
}
