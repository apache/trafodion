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
    /// A TabPage that displays a MaterializedViewGroup's members.
    /// </summary>
    public class MaterializedViewGroupMembersTabPage : DelayedPopulateClonableTabPage, IHasTrafodionMaterializedViewGroup
    {
        private TrafodionMaterializedViewGroup _sqlMxMaterializedViewGroup;
        private DatabaseObjectsControl     _databaseObjectsControl;


        /// <summary>
        /// The database objects control property.  Changing it to a new value will not have an effect
        /// until the Refresh() method is called.  We don't call it directly because we want this page to be delayed.
        /// </summary>
        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
        }


        /// <summary>
        /// The MaterializedViewGroup object whose members are displayed in this tab page
        /// </summary>
        public TrafodionMaterializedViewGroup TrafodionMaterializedViewGroup
        {
            get { return _sqlMxMaterializedViewGroup; }
        }

        /// <summary>
        /// Create a TabPage that displays a MaterializedViewGroup's members.
        /// </summary>
        /// <param name="aTrafodionMaterializedViewGroup">The MaterializedViewGroup whose members are to be displayed.</param>
        public MaterializedViewGroupMembersTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
            : base(Properties.Resources.Members)
        {
            _databaseObjectsControl     = aDatabaseObjectsControl;
            _sqlMxMaterializedViewGroup = aTrafodionMaterializedViewGroup;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxMaterializedViewGroup.TheTrafodionMaterializedViews;
        }
        /// <summary>
        /// Constructs a MaterializedViewGroup members panel and adds it to the MaterializedViewGroup members tab page
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            MaterializedViewGroupMembersPanel materializedViewGroupMembersPanel = new MaterializedViewGroupMembersPanel(TheDatabaseObjectsControl, TrafodionMaterializedViewGroup);
            materializedViewGroupMembersPanel.Dock = DockStyle.Fill;
            Controls.Add(materializedViewGroupMembersPanel);
        }
    }

}
