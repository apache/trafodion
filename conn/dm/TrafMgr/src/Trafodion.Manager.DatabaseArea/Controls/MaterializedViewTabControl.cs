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
    /// A tab control that displays information about a materialized view.
    /// </summary>
    public class MaterializedViewTabControl : TrafodionTabControl
    {
        /// <summary>
        /// Construct the materialized view tab control and set the tab pages
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionMaterializedView"></param>
        public MaterializedViewTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedView aTrafodionMaterializedView)
        {
            TabPages.Add(new MaterializedViewColumnsTabPage(aDatabaseObjectsControl, aTrafodionMaterializedView));
            TabPages.Add(new DivisionByKeyTabPage(aTrafodionMaterializedView));
            TabPages.Add(new MaterializedViewAttributesTabPage(aTrafodionMaterializedView));
            TabPages.Add(new PartitionsTabPage(aTrafodionMaterializedView));
            TabPages.Add(new MaterializedViewUsageTabPage(aDatabaseObjectsControl, aTrafodionMaterializedView));
            TabPages.Add(new TrafodionObjectDDLTabPage(aTrafodionMaterializedView));
            TabPages.Add(new MVStatisticsTabPage(aTrafodionMaterializedView));
            // Add the privileges tab.
            TabPages.Add(new SchemaObjectPrivilegesTabPage(aTrafodionMaterializedView));
        }
    }
}
