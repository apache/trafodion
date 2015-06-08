//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
    /// <summary>
    /// A a panel that displays information about a Library's usage info.
    /// </summary>
    public class LibraryUsagePanel : LibraryPanel
    {
        private DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// Create a panel that displays information about a Library's usage.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The DatabaseTreeView control</param>
        /// <param name="aTrafodionLibrary">The TrafodionLibrary whose usage is to be displayed</param>
        public LibraryUsagePanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionLibrary aTrafodionLibrary)
            : base(Properties.Resources.Usage, aTrafodionLibrary)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }

        /// <summary>
        /// Populate the panel
        /// </summary>
        protected override void Load()
        {
            //Create the usage panel and add to parent container.
            TrafodionUsagePanel<TrafodionLibrary> usagePanel = new TrafodionUsagePanel<TrafodionLibrary>(_databaseObjectsControl, TrafodionLibrary);
            usagePanel.Dock = DockStyle.Fill;
            usagePanel.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(usagePanel);

            //Add usage objects to the usage panel.
            usagePanel.AddUsageObjects(Properties.Resources.UsedBy, TrafodionLibrary.TheUsedByTrafodionRoutines);
          
            
        }

        /// <summary>
        /// Clone the panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new LibraryUsagePanel(null, TrafodionLibrary);
        }
    }
}
