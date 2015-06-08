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

using System;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that shows the Store Order Information for a TrafodionTable.
    /// </summary>
    public class StoreOrderPanel : TablePanel
    {

        /// <summary>
        /// Create a panel that shows the Store Order Information for a TrafodionTable.
        /// </summary>
        /// <param name="aTrafodionTable">The Table</param>
        public StoreOrderPanel(TrafodionTable aTrafodionTable)
            : base(Properties.Resources.StoreOrder, aTrafodionTable)
        {
            Load();
        }

        /// <summary>
        /// Populate the datagrid with store order information.
        /// </summary>
        protected override void Load()
        {

            // Get rid of any previous controls
            Controls.Clear();

            // Create a new grid
            StoreOrderDataGridView theStoreOrderDataGridView = new StoreOrderDataGridView(null);

            // Tell it to load the column info for our table index
            theStoreOrderDataGridView.Load(TrafodionTable);

            // Tell the grid about the window title so that it will show up on the pages of a PDF
            theStoreOrderDataGridView.TheHeaderText = WindowTitle;

            // Insert the Two Datagrids in reverse order.  
            // Add the datagrid for the columns
            theStoreOrderDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theStoreOrderDataGridView);

            theStoreOrderDataGridView.AddCountControlToParent(Properties.Resources.StoreBy + ": " + TrafodionTable.FormattedStoreOrder, DockStyle.Top);

            // And put buttons at the bottom
            theStoreOrderDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            return new StoreOrderPanel(TrafodionTable);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>The string</returns>
        public override string WindowTitle
        {
            get { return TrafodionTable.VisibleAnsiName + " " + Properties.Resources.StoreOrder; }
        }

        #endregion
    }

}

