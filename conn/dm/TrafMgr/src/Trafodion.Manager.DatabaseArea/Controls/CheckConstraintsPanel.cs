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
    /// A panel that shows the Check Constraints for a TrafodionTable.
    /// </summary>
    public class CheckConstraintsPanel : TablePanel
    {

        /// <summary>
        /// Create a panel that shows the Check Constraint Information for a TrafodionTable.
        /// </summary>
        /// <param name="aTrafodionTable">The table the checked contraints are on</param>
        public CheckConstraintsPanel(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.CheckConstraints, aTrafodionTable)
        {
            Load();
        }        

        /// <summary>
        /// Populate the check constraints panel.
        /// </summary>
        protected override void Load()
        {

            // Get rid of any previous controls
            Controls.Clear();

            // Create a new grid
            CheckConstraintsDataGridView theCheckConstraintsDataGridView = new CheckConstraintsDataGridView(null);

            // Tell the grid to load the check constraints data
            theCheckConstraintsDataGridView.Load(TrafodionTable);

            // Tell the grid about the window title so that it will show up on the pages of a PDF
            theCheckConstraintsDataGridView.TheHeaderText = WindowTitle;

            // Add the datagrid for the columns
            theCheckConstraintsDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theCheckConstraintsDataGridView);

            // Put a summary at the top
            theCheckConstraintsDataGridView.AddCountControlToParent(Properties.Resources.CheckConstraintsCountHeader, DockStyle.Top);

            // And put buttons at the bottom
            theCheckConstraintsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            return new CheckConstraintsPanel(TrafodionTable);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>The string</returns>
        public override string WindowTitle
        {
            get { return TrafodionTable.VisibleAnsiName + " " + Properties.Resources.CheckConstraints; }
        }

        #endregion
    }

}

