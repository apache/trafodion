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
    class RoutineUsageTabPage : DelayedPopulateClonableTabPage
    {
        private DatabaseObjectsControl _databaseObjectsControl;
        private TrafodionRoutine _sqlMxRoutine;

        /// <summary>
        /// Creates a tab page to display a view's usage.
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionView"></param>
        public RoutineUsageTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionRoutine aTrafodionRoutine)
            : base(Properties.Resources.Usage)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _sqlMxRoutine = aTrafodionRoutine;
        }

        /// <summary>
        /// Populates the tab page with new data.
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            // Create the panel and fill this tab page with it.
            RoutineUsagePanel routineUsagePanel = new RoutineUsagePanel(_databaseObjectsControl, _sqlMxRoutine);
            routineUsagePanel.Dock = DockStyle.Fill;
            Controls.Add(routineUsagePanel);
        }
    }
}
