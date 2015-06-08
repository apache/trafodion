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
    /// A a panel that displays information about an index's columns.
    /// </summary>
    public class IndexColumnsPanel : IndexPanel
    {

        /// <summary>
        /// Create a panel that displays information about an index's columns.
        /// </summary>
        /// <param name="aTrafodionIndex">The index whose columns are to be displayed.</param>
        public IndexColumnsPanel(TrafodionIndex aTrafodionIndex)
            : base("Columns", aTrafodionIndex)
        {

            // Populate us
            Load();

        }

        /// <summary>
        /// Populate the index columns.
        /// </summary>
        override protected void Load()
        {

            // Get rid of any previous controls
            Controls.Clear();

            // Create a new grid
            IndexColumnsDataGridView theIndexColumnsDataGridView = new IndexColumnsDataGridView(null);

            // Tell it to load the column info for our table index
            theIndexColumnsDataGridView.Load(TrafodionIndex);

            // Tell the grid about the window title so that it will show up on the pages of a PDF
            theIndexColumnsDataGridView.TheHeaderText = WindowTitle;

            // Fill the panel with the grid
            theIndexColumnsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theIndexColumnsDataGridView);

            // Put a summary at the top
            theIndexColumnsDataGridView.AddCountControlToParent(Properties.Resources.ColumnCountHeader, DockStyle.Top);

            // And put buttons at the bottom
            theIndexColumnsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary> 
        /// <returns>A new index columns instance</returns>
        override public Control Clone()
        {
            return new IndexColumnsPanel(TrafodionIndex);
        }

    }

}
