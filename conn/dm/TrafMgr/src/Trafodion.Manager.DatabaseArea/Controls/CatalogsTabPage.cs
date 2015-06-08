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
    public class CatalogsTabPage : DelayedPopulateClonableTabPage
    {
        private DatabaseObjectsControl theDatabaseObjectsControl;
        private TrafodionSystem theTrafodionSystem;

        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return theDatabaseObjectsControl; }
            set { theDatabaseObjectsControl = value; }
        }

        public TrafodionSystem TheTrafodionSystem
        {
            get { return theTrafodionSystem; }
            set { theTrafodionSystem = value; }
        }

        public CatalogsTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSystem aTrafodionSystem)
            : base(Properties.Resources.Catalogs)
        {
            TheDatabaseObjectsControl = aDatabaseObjectsControl;
            TheTrafodionSystem = aTrafodionSystem;
        }

        public override void PrepareForPopulate()
        {
            object catalogs = theTrafodionSystem.TrafodionCatalogs;
        }

        protected override void Populate()
        {
            Controls.Clear();

            CatalogsPanel theCatalogsPanel = new CatalogsPanel(TheDatabaseObjectsControl, theTrafodionSystem);

            theCatalogsPanel.Dock = DockStyle.Fill;
            Controls.Add(theCatalogsPanel);

        }

    }
}
