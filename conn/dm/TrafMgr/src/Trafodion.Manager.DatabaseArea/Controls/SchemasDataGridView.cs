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
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Schemas Data grid View class used to list schemas and schema items
    /// </summary>
    public class SchemasDataGridView : DatabaseAreaObjectsDataGridView
    {
        
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A database objects control if available for our use else null</param>
        public SchemasDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
            : base(aDatabaseObjectsControl)
        {
        }

        /// <summary> 
        /// Loads the rows into the SchemasDataGridView
        /// </summary>
        /// <param name="aTrafodionCatalog"> A Catalog object</param>
        /// <returns></returns>
        public int Load(TrafodionCatalog aTrafodionCatalog)
        {

            List<TrafodionSchema> theTrafodionSchemas = aTrafodionCatalog.TrafodionSchemas;

            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));
            //Columns.Add("theOwnerColumn", Properties.Resources.Owner);
            //Columns.Add("theVersionColumn", Properties.Resources.Version);
            //Columns.Add("theLocationColumn", Properties.Resources.Location);
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            foreach (TrafodionSchema theTrafodionSchema in theTrafodionSchemas)
            {
                if (!TrafodionName.IsAnInternalSchemaName(theTrafodionSchema.ExternalName))
                {
                    Rows.Add(new object[] { 
                        CreateLinkToObject(theTrafodionSchema),
                            
                       // theTrafodionSchema.OwnerName,
                        //theTrafodionSchema.Version, 
                        //theTrafodionSchema.Location, 
                        theTrafodionSchema.UID
                    });
                }
            }
            
            return Rows.Count;
        }
        
        /// <summary>
        /// Gets a link type string to click on in a datagridview
        /// </summary>
        /// <param name="aTrafodionSchema"> A Schema object</param>
        /// <returns></returns>
        protected virtual DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionSchema aTrafodionSchema)
        {
            return new DatabaseAreaObjectsDataGridViewLink(TheDatabaseTreeView, aTrafodionSchema);
        }
               
    }
}
