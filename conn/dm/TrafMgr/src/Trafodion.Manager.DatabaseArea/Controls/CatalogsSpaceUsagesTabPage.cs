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

namespace Trafodion.Manager.DatabaseArea.Controls
{

    public class CatalogsSpaceUsagesTabPage : Trafodion.Manager.Framework.Controls.DelayedPopulateTabPage
    {

        private TrafodionSystem _sqlMxSystem;
        private DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDatabaseObjectsControl A Database object control or null"></param>
        /// <param name="aTrafodionCatalog A Trafodion catalog object"></param>
        public CatalogsSpaceUsagesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSystem aTrafodionSystem)
            : base(Properties.Resources.SpaceUsage)
        {
            TheDatabaseObjectsControl = aDatabaseObjectsControl;
            _sqlMxSystem = aTrafodionSystem;
        }
        
        /// <summary>
        /// Populate the grid
        /// </summary>
        override protected void Populate()
        {
            Controls.Clear();

            SystemSpaceUsageSummaryControl usages = new SystemSpaceUsageSummaryControl(TheDatabaseObjectsControl, _sqlMxSystem);
            usages.Dock = DockStyle.Fill;
            Controls.Add(usages);
        }
        /// <summary>
        /// Get or Set the SQLMXCatalog model
        /// </summary>
        public TrafodionSystem TheTrafodionSystem
        {
            get { return _sqlMxSystem; }
        }

        /// <summary>
        /// get or set the TheDatabaseObjectsControl
        /// </summary>
        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
            set { _databaseObjectsControl = value; }
        }

    }
}
