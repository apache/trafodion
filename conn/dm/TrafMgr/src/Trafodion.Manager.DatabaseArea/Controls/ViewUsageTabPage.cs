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
    /// A tab page that shows a view's usage.
    /// </summary>
    public class ViewUsageTabPage : DelayedPopulateClonableTabPage
    {
        private DatabaseObjectsControl _databaseObjectsControl;
        private TrafodionView _sqlMxView;

        /// <summary>
        /// Creates a tab page to display a view's usage.
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionView"></param>
        public ViewUsageTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionView aTrafodionView)
            : base(Properties.Resources.Usage)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _sqlMxView = aTrafodionView;
        }

        public override void PrepareForPopulate()
        {
            object a = _sqlMxView.TheTrafodionMVsUsedBy;
            a = _sqlMxView.TheTrafodionRoutinesUsedBy;
            a = _sqlMxView.TheTrafodionTablesUsedBy;
            a = _sqlMxView.TheTrafodionViewsUsedBy;
            a = _sqlMxView.TheTrafodionViewsUsing;
        }
        /// <summary>
        /// Populates the tab page with new data.
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            ViewUsagePanel viewUsagePanel = new ViewUsagePanel(_databaseObjectsControl, _sqlMxView);
            viewUsagePanel.Dock = DockStyle.Fill;
            Controls.Add(viewUsagePanel);
        }
    }
}
