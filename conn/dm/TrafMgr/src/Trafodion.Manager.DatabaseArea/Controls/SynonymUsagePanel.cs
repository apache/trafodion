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
    /// A a panel that displays information about a synonym's usage.
    /// </summary>
    public class SynonymUsagePanel : TrafodionObjectPanel
    {
        private DatabaseObjectsControl _databaseObjectsControl;

        /// <summary>
        /// The Synonym
        /// </summary>
        public TrafodionSynonym TrafodionSynonym
        {
            get { return (TrafodionSynonym)TheTrafodionObject; }
        }
        
        /// <summary>
        /// Create a panel that displays information about a synonym's usage.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The DatabaseTreeView control</param>
        /// <param name="aTrafodionSynonym">The Synonym whose usage is to be displayed</param>
        public SynonymUsagePanel(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSynonym aTrafodionSynonym)
            : base(Properties.Resources.Usage, aTrafodionSynonym)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            Load();
        }

        /// <summary>
        /// Populate the panel
        /// </summary>
        protected void Load()
        {
            List<TrafodionSchemaObject> referencedObjects = new List<TrafodionSchemaObject>();
            referencedObjects.Add(TrafodionSynonym.ReferencedObject);

            //Create the usage panel and add to parent container.
            TrafodionUsagePanel<TrafodionSynonym> usagePanel = new TrafodionUsagePanel<TrafodionSynonym>(_databaseObjectsControl, TrafodionSynonym);
            usagePanel.Dock = DockStyle.Fill;
            usagePanel.BorderStyle = BorderStyle.FixedSingle;
            Controls.Add(usagePanel);

            //Add the usage objects to the usage panel.
            usagePanel.AddUsageObjects(Properties.Resources.Reference, referencedObjects);

        }

        /// <summary>
        /// Clone the panel
        /// </summary>
        /// <returns></returns>
        override public Control Clone()
        {
            return new SynonymUsagePanel(null, TrafodionSynonym);
        }
    }

}
