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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// The tab page that shows privileges on a schema.
    /// </summary>
    public class SchemaSpaceUsagesTabPage : DelayedPopulateTabPage, IHasTrafodionSchema
    {
        private TrafodionSchema _schema;
        DatabaseObjectsControl _databaseObjectsControl;

        #region Properties

        /// <summary>
        /// The schema that this tab is displaying.
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get { return _schema; }
            set { _schema = value; }
        }

        #endregion

        /// <summary>
        /// Creates a new tab page to show schema privileges.
        /// </summary>
        /// <param name="schema">The schema whose privileges will be displayed.</param>
        public SchemaSpaceUsagesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema schema)
            : base(Properties.Resources.SpaceUsage)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _schema = schema;
        }

        /// <summary>
        /// Adds the tab's datagrid to the tab.
        /// </summary>
        override protected void Populate()
        {
            this.Controls.Clear();
            SchemaSpaceUsageSummaryControl schemaSpaceControl = new SchemaSpaceUsageSummaryControl(_databaseObjectsControl, _schema);
            schemaSpaceControl.Dock = DockStyle.Fill;
            this.Controls.Add(schemaSpaceControl);
        }
    }
}
