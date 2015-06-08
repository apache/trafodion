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
    /// A a panel that displays information about a View's columns.
    /// </summary>
    public class ViewColumnsPanel : ViewPanel
    {
        DatabaseObjectsControl _aDatabaseObjectsControl;
        /// <summary>
        /// Create a panel that displays information about a View's columns.
        /// </summary>
        /// <param name="aTrafodionView">The View whose columns are to be displayed.</param>
        public ViewColumnsPanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionView aTrafodionView)
            : base(Properties.Resources.Columns, aTrafodionView)
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
            ViewColumnsDataGridView theViewColumnsDataGridView = new ViewColumnsDataGridView(_aDatabaseObjectsControl);

            // Tell it to load the column info for our View
            theViewColumnsDataGridView.Load(TrafodionView);

            // Tell the grid about the window title so that it will show up on the pages of a PDF
            theViewColumnsDataGridView.TheHeaderText = WindowTitle;

            // Fill the panel with the grid
            theViewColumnsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theViewColumnsDataGridView);

            // Put a summary at the top
            theViewColumnsDataGridView.AddCountControlToParent(Properties.Resources.ColumnCountHeader, DockStyle.Top);

            // And put buttons at the bottom
            theViewColumnsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        /// <summary>
        /// Makes a clone of this panel suiView for inclusion in some container.
        /// </summary> 
        /// <returns>A new View columns View instance</returns>
        override public Control Clone()
        {
            return new ViewColumnsPanel(_aDatabaseObjectsControl, TrafodionView);
        }

    }

}
