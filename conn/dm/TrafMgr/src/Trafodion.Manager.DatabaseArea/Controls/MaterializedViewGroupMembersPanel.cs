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
    /// MaterializedViewGroup members panel
    /// </summary>
    public class MaterializedViewGroupMembersPanel : MaterializedViewGroupPanel
    {

        private DatabaseObjectsControl                   _databaseObjectsControl;
        private MaterializedViewGroupMembersDataGridView _dataGridView;

        /// <summary>
        /// Constructs a MaterializedViewGroup members panel
        /// </summary>
        /// <param name="aMaterializedViewGroup">MaterializedViewGroup whose members need to be displayed</param>
        public MaterializedViewGroupMembersPanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
            : base(Properties.Resources.Members, aTrafodionMaterializedViewGroup)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }
        /// <summary>
        /// Creates a datagridview to display the MaterializedViewGroup members
        /// </summary>
        protected override void Load()
        {
            // may be:  the generic TrafodionSchemaObjectsDataGridView<T> could be used here.

            // need to pass _databaseObjectsControl to help navigate to the Naviagtion-Tree when a MV Names is clicked from Right-Pane
            _dataGridView = new MaterializedViewGroupMembersDataGridView(_databaseObjectsControl);
            _dataGridView.Load(TrafodionMaterializedViewGroup);

            _dataGridView.TheHeaderText = WindowTitle;
            _dataGridView.Dock = DockStyle.Fill;
            Controls.Add(_dataGridView);

            // These will be added to the grid's parent which is us
            _dataGridView.AddCountControlToParent(Properties.Resources.MVGroupMembersCount, DockStyle.Top);
            _dataGridView.AddButtonControlToParent(DockStyle.Bottom);
        }
        /// <summary>
        /// Clones the MVGroup  members panel into a new window
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new MaterializedViewGroupMembersPanel(null, TrafodionMaterializedViewGroup);
        }

        private void AddRow(string aName, object aValue)
        {
            _dataGridView.Rows.Add(new object[] { aName, aValue });
        }


    }

}
