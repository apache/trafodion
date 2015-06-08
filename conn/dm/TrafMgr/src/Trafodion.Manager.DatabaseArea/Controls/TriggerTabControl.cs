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
    /// Control that contains tabs for an Trigger
    /// </summary>
    public class TriggerTabControl : TrafodionTabControl
    {
        /// <summary>
        /// Constructor for TriggerTabControl
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionTrigger"></param>
        public TriggerTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTrigger aTrafodionTrigger)
        {
            TabPages.Add(new TriggerAttributesTabPage(aTrafodionTrigger));
            TabPages.Add(new TriggerUsageTabPage(aDatabaseObjectsControl, aTrafodionTrigger));
            TabPages.Add(new TrafodionObjectDDLTabPage(aTrafodionTrigger));
        }
    }
}
