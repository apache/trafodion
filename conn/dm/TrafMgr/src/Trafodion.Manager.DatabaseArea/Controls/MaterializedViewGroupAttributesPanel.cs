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
    /// MaterializedViewGroup attributes panel
    /// </summary>
    public class MaterializedViewGroupAttributesPanel : MaterializedViewGroupPanel
    {
        /// <summary>
        /// Constructs a MaterializedViewGroup attributes panel
        /// </summary>
        /// <param name="aMaterializedViewGroup">MaterializedViewGroup whose attributes need to be displayed</param>
        public MaterializedViewGroupAttributesPanel(TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
            : base(Properties.Resources.Attributes, aTrafodionMaterializedViewGroup)
        {
            Load();
        }
        /// <summary>
        /// Creates a datagridview to display the MaterializedViewGroup attributes
        /// </summary>
        protected override void Load()
        {
            _dataGridView = new MaterializedViewGroupAttributesDataGridView(null, TrafodionMaterializedViewGroup);
            _dataGridView.TheHeaderText = WindowTitle;
            _dataGridView.Dock = DockStyle.Fill;
            Controls.Add(_dataGridView);
            _dataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }
        /// <summary>
        /// Clones the table attributes panel into a new window
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new MaterializedViewGroupAttributesPanel(TrafodionMaterializedViewGroup);
        }

        private void AddRow(string aName, object aValue)
        {
            _dataGridView.Rows.Add(new object[] { aName, aValue });
        }

        private MaterializedViewGroupAttributesDataGridView _dataGridView;

    }

}
