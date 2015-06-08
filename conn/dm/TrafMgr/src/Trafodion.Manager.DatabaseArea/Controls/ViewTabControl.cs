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
    /// A tab control that displays information about a TrafodionView object.
    /// </summary>
    public class ViewTabControl : TrafodionTabControl
    {
        private TrafodionView _sqlMxView;

        /// <summary>
        /// The TrafodionView object that is displayed in this tab control.
        /// </summary>
        public TrafodionView TrafodionView
        {
            get { return _sqlMxView; }
        }

        /// <summary>
        /// Creates a tab control to display information about a TrafodionView object.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">
        /// This will be used to display links to other related sql object.
        /// </param>
        /// <param name="aTrafodionView">
        /// A TrafodionView object that will be displayed in this control.
        /// </param>
        public ViewTabControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionView aTrafodionView)
        {
            _sqlMxView = aTrafodionView;
            TabPages.Add(new ViewColumnsTabPage(aDatabaseObjectsControl, aTrafodionView));
            TabPages.Add(new ViewAttributesTabPage(TrafodionView));

            //TabPages.Add(new ViewUsageTabPage(aDatabaseObjectsControl, TrafodionView));
            TabPages.Add(new TrafodionObjectDDLTabPage(TrafodionView));
            //TabPages.Add(new SchemaObjectPrivilegesTabPage(TrafodionView));
        }

    }
}
