//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
    /// A panel that shows the DivisionByKey Information for a Table or MV or Index.
    /// </summary>
    public class DivisionByKeyPanel : TrafodionObjectPanel
    {
        private TrafodionSchemaObject _sqlMxSchemaObject;

        /// <summary>
        /// Create a panel that shows the DivisionByKey Information for a Table or MV or Index
        /// </summary>
        /// <param name="aTrafodionSchemaObject">The Table or MV or Index</param>
        public DivisionByKeyPanel(TrafodionSchemaObject aTrafodionSchemaObject)
            : base(Properties.Resources.DivisionByKey, aTrafodionSchemaObject)
        {
            _sqlMxSchemaObject = aTrafodionSchemaObject;
            Load();
        }

        /// <summary>
        /// Populate the datagrid with DivisionByKey Information for a Table or MV or Index
        /// </summary>
        protected void Load()
        {

            // Get rid of any previous controls
            Controls.Clear();

            // Create a new grid
            DivisionByKeyDataGridView theDivisionByKeyDataGridView = new DivisionByKeyDataGridView(null);

            // Tell it to load the DivisionByKey Information for a Table or MV or Index
            theDivisionByKeyDataGridView.Load(_sqlMxSchemaObject);

            theDivisionByKeyDataGridView.TheHeaderText = WindowTitle;

            theDivisionByKeyDataGridView.Dock = DockStyle.Fill;
            Controls.Add(theDivisionByKeyDataGridView);

            theDivisionByKeyDataGridView.AddCountControlToParent(Properties.Resources.DivisionByKeyHeader, DockStyle.Top);

            // And put buttons at the bottom
            theDivisionByKeyDataGridView.AddButtonControlToParent(DockStyle.Bottom);

        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            return new DivisionByKeyPanel(_sqlMxSchemaObject);
        }

        /// <summary>
        /// A string suitable for a window title.
        /// </summary>
        /// <returns>The string</returns>
        public override string WindowTitle
        {
            get { return _sqlMxSchemaObject.VisibleAnsiName + " " + Properties.Resources.DivisionByKey; }
        }

        #endregion
    }

}

