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
    /// A a panel that displays information about a MaterializedView's columns.
    /// </summary>
    public class MaterializedViewColumnsPanel : MaterializedViewPanel
    {
        DatabaseObjectsControl _aDatabaseObjectsControl;
        /// <summary>
        /// Create a panel that displays information about a View's columns.
        /// </summary>
        /// <param name="aTrafodionMaterializedView">The View whose columns are to be displayed.</param>
        public MaterializedViewColumnsPanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedView aTrafodionMaterializedView)
            : base(Properties.Resources.Columns, aTrafodionMaterializedView)
        {
            _aDatabaseObjectsControl = aDatabaseObjectsControl;
            // Populate us
            Load();

        }

        /// <summary>
        /// Populate us.
        /// </summary>
        override protected void Load()
        {

            // Get rid of any previous controls
            Controls.Clear();

            // Create a new grid
            MaterializedViewColumnsDataGridView theMaterializedViewColumnsDataGridView = new MaterializedViewColumnsDataGridView(_aDatabaseObjectsControl);

            // Tell it to load the column info for our View
            theMaterializedViewColumnsDataGridView.Load(TrafodionMaterializedView);

            // Tell the grid about the window title so that it will show up on the pages of a PDF
            theMaterializedViewColumnsDataGridView.TheHeaderText = WindowTitle;

            // Fill the panel with the grid
            theMaterializedViewColumnsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theMaterializedViewColumnsDataGridView);

            // Put a summary at the top
            theMaterializedViewColumnsDataGridView.AddCountControlToParent(Properties.Resources.ColumnCountHeader, DockStyle.Top);

            // And put buttons at the bottom
            theMaterializedViewColumnsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        /// <summary>
        /// Makes a clone of this panel Materialized View for inclusion in some container.
        /// </summary> 
        /// <returns>A new View columns View instance</returns>
        override public Control Clone()
        {
            return new MaterializedViewColumnsPanel(_aDatabaseObjectsControl, TrafodionMaterializedView);
        }

    }

}
