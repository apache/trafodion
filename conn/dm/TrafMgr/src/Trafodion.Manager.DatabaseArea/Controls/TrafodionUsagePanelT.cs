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

using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A panel that displays information about a triggers's usage relationships.
    /// THIS VERSION IS USED ONLY BY TRIGGERS, CLUDGE TO ALLOW TRIGGERS TO ADD IT'S COLUMNS TO SCHEMA PANEL DATA GRID
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class TrafodionUsagePanelT<T> : TrafodionUsagePanel<T> where T : TrafodionSchemaObject
    {
        /// <summary>
        /// Constructs a panel that displays information about a triggers's usage relationships.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control that has reference to the DatabaseTreeView</param>
        /// <param name="sqlMxSchemaObject">The triggers whose usage info is displayed</param>
        public TrafodionUsagePanelT(DatabaseObjectsControl aDatabaseObjectsControl, T sqlMxSchemaObject)
            :base(aDatabaseObjectsControl, sqlMxSchemaObject)
        {
        }

        /// <summary>
        /// Overrides and creates a grid to handle additional columns for trigger usage
        /// </summary>
        public override void CreateDataGridView()
        {
            //Construct the generic datagridview to display the TrafodionSchemaObject list
            _sqlMxUsageDataGridView = new TrafodionUsageDataGridViewT<T>(_databaseObjectsControl, TrafodionSchemaObject);
        }

        /// <summary>
        /// Add the usage objects to the grid
        /// </summary>
        /// <typeparam name="UT">The usage object type</typeparam>
        /// <param name="usageType">The type of the usage object</param>
        /// <param name="usageObjects">List of the usage objects</param>
        public override void AddUsageObjects<UT>(string usageType, List<UT> usageObjects)
        {
            _sqlMxUsageDataGridView.AddUsage(usageType, usageObjects);
        }

        #region ICloneToWindow

        /// <summary>
        /// Makes a clone of this panel suitable for inclusion in some container.
        /// </summary>
        /// <returns>The clone control</returns>
        override public Control Clone()
        {
            TrafodionUsagePanelT<T> theTrafodionUsagePanel = new TrafodionUsagePanelT<T>(null, TrafodionSchemaObject);
            return theTrafodionUsagePanel;
        }

        #endregion ICloneToWindow
    }

}
