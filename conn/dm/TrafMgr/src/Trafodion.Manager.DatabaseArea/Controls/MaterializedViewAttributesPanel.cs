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
    /// MaterializedView attributes panel
    /// </summary>
    public class MaterializedViewAttributesPanel : MaterializedViewPanel
    {
        /// <summary>
        /// Constructs a MaterializedView attributes panel
        /// </summary>
        /// <param name="aTrafodionMaterializedView">MaterializedView whose attributes need to be displayed</param>
        public MaterializedViewAttributesPanel(TrafodionMaterializedView aTrafodionMaterializedView)
            : base(Properties.Resources.Attributes, aTrafodionMaterializedView)
        {
            Load();
        }
        /// <summary>
        /// Creates a datagridview to display the table attributes
        /// </summary>
        protected override void Load()
        {
            _dataGridView = new MaterializedViewAttributesDataGridView(null, TrafodionMaterializedView);
            _dataGridView.TheHeaderText = WindowTitle;
            _dataGridView.Dock = DockStyle.Fill;
            Controls.Add(_dataGridView);
            _dataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }
        /// <summary>
        /// Clones the MaterializedView attriutes panel into a new window
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new MaterializedViewAttributesPanel(TrafodionMaterializedView);
        }

        private void AddRow(string aName, object aValue)
        {
            _dataGridView.Rows.Add(new object[] { aName, aValue });
        }

        private MaterializedViewAttributesDataGridView _dataGridView;

    }

}
