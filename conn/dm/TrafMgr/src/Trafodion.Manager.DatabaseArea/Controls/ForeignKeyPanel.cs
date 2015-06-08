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
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that shows the Foreign Key Information for a TrafodionTable.
    /// </summary>
    public class ForeignKeyPanel : TablePanel
    {
        DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// Create a panel that shows the Foreign Key Information for a TrafodionTable.
        /// </summary>
        /// <param name="aTrafodionTable">The TrafodionTable</param>
        public ForeignKeyPanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTable aTrafodionTable)
            : base(Properties.Resources.ForeignKeys, aTrafodionTable)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }

        /// <summary>
        /// Populate the foreign key datagrid
        /// </summary>
        protected override void Load()
        {

            // Get rid of any previous controls
            Controls.Clear();

            // Create a new grid
            ForeignKeyDataGridView theForeignKeyDataGridView = new ForeignKeyDataGridView(_databaseObjectsControl);

            // Tell it to load the foreign key info for our table
            theForeignKeyDataGridView.Load(TrafodionTable);

            // Tell the grid about the window title so that it will show up on the pages of a PDF
            theForeignKeyDataGridView.TheHeaderText = WindowTitle;

            // Fill the panel with the grid
            theForeignKeyDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theForeignKeyDataGridView);

            // Put a summary at the top
            theForeignKeyDataGridView.AddCountControlToParent(Properties.Resources.TableForeignKeyCountHeader, DockStyle.Top);

            // And put buttons at the bottom
            theForeignKeyDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            return new ForeignKeyPanel(null, TrafodionTable);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>The string</returns>
        public override string WindowTitle
        {
            get { return TrafodionTable.VisibleAnsiName + " " + Properties.Resources.ForeignKeys; }
        }

        #endregion
    }

}
