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
    public class MaterializedViewGroupTabControl : TrafodionTabControl
    {

        /// <summary>
        ///  Trafodion  Object representing MV group  
        /// </summary>
        private TrafodionMaterializedViewGroup theTrafodionMaterializedViewGroup;

        /// <summary>
        /// The Model object for MV Group
        /// </summary>
        public TrafodionMaterializedViewGroup TheTrafodionMaterializedViewGroup
        {
            get { return theTrafodionMaterializedViewGroup; }
            set { theTrafodionMaterializedViewGroup = value; }
        }

        /// <summary>
        /// The control  to add Tab-pages for MV Group
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionMaterializedViewGroup"></param>
        public MaterializedViewGroupTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
        {
            TheTrafodionMaterializedViewGroup = aTrafodionMaterializedViewGroup;
            TabPages.Add(new MaterializedViewGroupAttributesTabPage(aTrafodionMaterializedViewGroup));
            TabPages.Add(new MaterializedViewGroupMembersTabPage(aDatabaseObjectsControl,aTrafodionMaterializedViewGroup));
            TabPages.Add(new TrafodionObjectDDLTabPage(aTrafodionMaterializedViewGroup));
        }

    }
}
