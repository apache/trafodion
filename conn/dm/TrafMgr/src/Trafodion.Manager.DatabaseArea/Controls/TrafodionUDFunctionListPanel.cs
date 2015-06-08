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

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that displays a list of sql functions
    /// </summary>
    public class SqlMxUDFunctionListPanel : SqlMxSchemaObjectListPanel<SqlMxUDFunction>
    {
        public SqlMxUDFunctionListPanel(DatabaseObjectsControl aDatabaseObjectsControl, string aHeaderText, SqlMxObject parentSqlMxObject,
                    List<SqlMxUDFunction> sqlMxObjects, string aTitle)
            : base(aDatabaseObjectsControl, aHeaderText, parentSqlMxObject, sqlMxObjects, aTitle)
        {
        }

        /// <summary>
        /// Overrides to create a custom datagridview to hold the list of functions
        /// </summary>
        protected override void CreateDataGridView()
        {
            //Construct the SqlMxViewsDataGridView datagridview to display the SqlMxViews list
            _sqlMxSchemaObjectsDataGridView = new SqlMxUDFunctionsDataGridView(TheDatabaseObjectsControl, TheParentSqlMxObject, TheSqlMxObjects);
            _sqlMxSchemaObjectsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(_sqlMxSchemaObjectsDataGridView);

            // These will be added to the grid's parent which is us
            _sqlMxSchemaObjectsDataGridView.AddCountControlToParent(TheHeaderText, DockStyle.Top);
            _sqlMxSchemaObjectsDataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }

        /// <summary>
        /// Clone this window
        /// </summary>
        /// <returns></returns>
        public override Control Clone()
        {
            //Override this method, so the custom panel is cloned instead of the base panel
            SqlMxUDFunctionListPanel sqlMxObjectListPanel = new SqlMxUDFunctionListPanel(null, TheHeaderText,
                                                                        TheParentSqlMxObject, TheSqlMxObjects, TheTitle);
            return sqlMxObjectListPanel;
        }
    }
}
