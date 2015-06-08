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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A TabPage that displays information about a materialized views's columns.
    /// </summary>
    class MaterializedViewColumnsTabPage : DelayedPopulateClonableTabPage
    {
        private TrafodionMaterializedView _sqlMxMaterializedView;
        DatabaseObjectsControl _aDatabaseObjectsControl;

        /// <summary>
        /// Create a TabPage that displays information about a materialized view's columns.
        /// </summary>
        /// <param name="aTrafodionMaterializedView">The materialized view whose columns are to be displayed.</param>
        public MaterializedViewColumnsTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedView aTrafodionMaterializedView)
            : base(Properties.Resources.Columns)
        {
            _sqlMxMaterializedView = aTrafodionMaterializedView;
            _aDatabaseObjectsControl = aDatabaseObjectsControl;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxMaterializedView.Columns;
        }
        /// <summary>
        /// Populate the tab page
        /// </summary>
        protected override void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            MaterializedViewColumnsPanel theMaterializedViewColumnsPanel = new MaterializedViewColumnsPanel(_aDatabaseObjectsControl, _sqlMxMaterializedView);
            theMaterializedViewColumnsPanel.Dock = DockStyle.Fill;
            Controls.Add(theMaterializedViewColumnsPanel);
        }
    }

}
