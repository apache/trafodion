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
using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    class RoutineUsagePanel : TrafodionObjectPanel
    {
        private DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// Creates a panel to display a view's usage relationships.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The DatabaseTreeView control</param>
        /// <param name="aTrafodionView">The view whose usage is to be displayed</param>
        public RoutineUsagePanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionRoutine aTrafodionRoutine)
            : base(Properties.Resources.Usage, aTrafodionRoutine)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }

        /// <summary>
        /// Populate the panel
        /// </summary>
        protected void Load()
        {
            TrafodionRoutine routine = TheTrafodionObject as TrafodionRoutine;

            TrafodionUsagePanel<TrafodionRoutine> usagePanel = new TrafodionUsagePanel<TrafodionRoutine>(_databaseObjectsControl, routine);
            usagePanel.Dock = DockStyle.Fill;
            usagePanel.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(usagePanel);

            List<TrafodionSchemaObject> usageObjects = routine.LoadTrafodionSchemaObjects(routine);
            List<TrafodionSchemaObject> usesObjectList = new List<TrafodionSchemaObject>();
            List<TrafodionSchemaObject> usedByObjectList = new List<TrafodionSchemaObject>();

            foreach(TrafodionSchemaObject usageObject in usageObjects)
            {
                if(usageObject is TrafodionTrigger)
                {
                    usedByObjectList.Add(usageObject);
                }
                else
                {
                    usesObjectList.Add(usageObject);
                }
            }
            if (usesObjectList.Count > 0)
            {
                usagePanel.AddUsageObjects(Properties.Resources.Uses, usesObjectList);
            }
            if (usedByObjectList.Count > 0)
            {
                usagePanel.AddUsageObjects(Properties.Resources.UsedBy, usedByObjectList);
            }
        }

        /// <summary>
        /// Clone the panel
        /// </summary>
        /// <returns>A new ViewUsagePanel.</returns>
        override public Control Clone()
        {
            return new RoutineUsagePanel(null, TheTrafodionObject as TrafodionRoutine);
        }
    }
}
