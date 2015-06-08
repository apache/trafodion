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
    /// A panel that display's a view's usage relationships.
    /// </summary>
    public class ViewUsagePanel : TrafodionObjectPanel
    {
        private DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// Creates a panel to display a view's usage relationships.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The DatabaseTreeView control</param>
        /// <param name="aTrafodionView">The view whose usage is to be displayed</param>
        public ViewUsagePanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionView aTrafodionView)
            : base(Properties.Resources.Usage, aTrafodionView)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }

        /// <summary>
        /// Populate the panel
        /// </summary>
        protected void Load()
        {
            TrafodionView View = TheTrafodionObject as TrafodionView;
            //Create the usage panel and add to parent container.
            TrafodionUsagePanel<TrafodionView> usagePanel = new TrafodionUsagePanel<TrafodionView>(_databaseObjectsControl, View);
            usagePanel.Dock = DockStyle.Fill;
            usagePanel.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(usagePanel);

            //Add the different types of usage objects to the usage panel.
            usagePanel.AddUsageObjects(Properties.Resources.Uses, View.TheTrafodionTablesUsedBy);
            usagePanel.AddUsageObjects(Properties.Resources.Uses, View.TheTrafodionMVsUsedBy);
            usagePanel.AddUsageObjects(Properties.Resources.Uses, View.TheTrafodionViewsUsedBy);
            usagePanel.AddUsageObjects(Properties.Resources.Uses, View.TheTrafodionRoutinesUsedBy);
            usagePanel.AddUsageObjects(Properties.Resources.UsedBy, View.TheTrafodionViewsUsing);
            usagePanel.AddUsageObjects(Properties.Resources.Has, View.TrafodionSynonyms);
        }

        /// <summary>
        /// Clone the panel
        /// </summary>
        /// <returns>A new ViewUsagePanel.</returns>
        override public Control Clone()
        {
            return new ViewUsagePanel(null, TheTrafodionObject as TrafodionView);
        }
    }
}
