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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A datagridview that displays a list of SqlMxViews
    /// </summary>
    class SqlMxViewsDataGridView : SqlMxSchemaObjectsDataGridView<SqlMxView>
    {
        public SqlMxViewsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, SqlMxObject aParentSqlMxObject, List<SqlMxView> aSqlMxObjectList)
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

            foreach (SqlMxView sqlMxView in TheSqlMxObjects)
            {
                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxView.VisibleAnsiName))
                {
                    if (sqlMxView.ConnectionDefinition.ServerVersion>=ConnectionDefinition.SERVER_VERSION.SQ131)
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxView), 
                            sqlMxView.Owner,
                            SqlMxView.DisplayValidState(sqlMxView.Valid_Def),
                            sqlMxView.UID, 
                            sqlMxView.FormattedCreateTime(),
                            sqlMxView.FormattedRedefTime()
                        });
                    }
                    else 
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxView), 
                            sqlMxView.Owner,
                            sqlMxView.UID, 
                            sqlMxView.FormattedCreateTime(),
                            sqlMxView.FormattedRedefTime()
                        });
                    }                    
                }
            }
            return Rows.Count;
        }

        protected override void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));
            Columns.Add("theOwnerColumn", Properties.Resources.Owner);

            //Starting from M9, add Valid_Def column, using TheParentSqlMxObject because SetupColumns method is called in TheParentSqlMxObject set method.
            if (TheParentSqlMxObject != null && TheParentSqlMxObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ131)
            {
                Columns.Add("theValidDefColumn", Properties.Resources.ValidDef);
            }
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);            
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }        
    }
}
