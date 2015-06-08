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

using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls
{

    /// <summary>
    /// A data grid view with the capability of know about a name filter and a database objects control.
    /// </summary>
    public class DatabaseAreaObjectsDataGridView : TrafodionDataGridView
    {

        private NavigationTreeNameFilter theNameFilter = null;
        private DatabaseObjectsControl theDatabaseObjectsControl = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public DatabaseAreaObjectsDataGridView()
        {
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public DatabaseAreaObjectsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl)
        {
            TheDatabaseObjectsControl = aDatabaseObjectsControl;
            if (TheDatabaseObjectsControl != null)
            {
                TheNameFilter = aDatabaseObjectsControl.TheNameFilter;
            }
        }

        public DatabaseTreeView TheDatabaseTreeView
        {
            get
            {
                return (TheDatabaseObjectsControl == null) ? null : TheDatabaseObjectsControl.TheDatabaseTreeView;
            }
        }

        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return theDatabaseObjectsControl; }
            set { theDatabaseObjectsControl = value; }
        }

        public NavigationTreeNameFilter TheNameFilter
        {
            get { return theNameFilter; }
            set { theNameFilter = value; }
        }

    }

}
