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
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A data grid view showing a system's catalogs
    /// </summary>
    public class CatalogsDataGridView : DatabaseAreaObjectsDataGridView
    {

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDatabaseTreeCatalog"> A database tree view if available for our use else null</param>
        public CatalogsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
        }

        public int Load(TrafodionSystem aTrafodionSystem)
        {

            List<TrafodionCatalog> theTrafodionCatalogs = aTrafodionSystem.TrafodionCatalogs;

            Columns.Clear();
            Rows.Clear();
            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));
            Columns.Add("theLocationColumn", Properties.Resources.Location);
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            foreach (TrafodionCatalog theTrafodionCatalog in theTrafodionCatalogs)
            {
                Rows.Add(new object[] {  
                    CreateLinkToObject(theTrafodionCatalog),
                    theTrafodionCatalog.VolumeName, 
                    theTrafodionCatalog.UID });
            }

            return Rows.Count;
        }

        protected virtual DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionCatalog theTrafodionCatalog)
        {
            return new DatabaseAreaObjectsDataGridViewLink(TheDatabaseTreeView, theTrafodionCatalog);
        }
    }

}
