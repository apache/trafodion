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

using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A Tab control for Table objects
    /// </summary>
    public class TableTabControl : TrafodionTabControl
    {
        /// <summary>
        /// Constructs a Tab control for the table. 
        /// The tab control constructs all the tab pages that are to be displayed
        /// for a Table object
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionTable"></param>
        public TableTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTable aTrafodionTable)
        {
            TabPages.Add(new TableColumnsTabPage(aTrafodionTable));
            TabPages.Add(new PrimaryKeyTabPage(aTrafodionTable));
            //TabPages.Add(new HashKeyTabPage(aTrafodionTable));
            //TabPages.Add(new StoreOrderTabPage(aTrafodionTable));

            //TabPages.Add(new DivisionByKeyTabPage(aTrafodionTable));
           // TabPages.Add(new CheckConstraintsTabPage(aTrafodionTable));
           // TabPages.Add(new UniqueConstraintsTabPage(aDatabaseObjectsControl, aTrafodionTable));
            //TabPages.Add(new ForeignKeyTabPage(aDatabaseObjectsControl, aTrafodionTable));
            TabPages.Add(new TableAttributesTabPage(aTrafodionTable));
            //TabPages.Add(new PartitionsTabPage(aTrafodionTable));
            //TabPages.Add(new TableUsageTabPage(aDatabaseObjectsControl, aTrafodionTable));
            TabPages.Add(new TrafodionObjectDDLTabPage(aTrafodionTable));

            //if (!aTrafodionTable.IsMetadataObject)
           // {
           //     TabPages.Add(new TableStatisticsTabPage(aTrafodionTable));
           // }
           // TabPages.Add(new SchemaObjectPrivilegesTabPage(aTrafodionTable));
        }

    }
}
