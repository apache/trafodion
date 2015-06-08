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

using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Navigation;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Base class for a Tab page that is used to display a list of schema level objects 
    /// </summary>
    public class SchemaLevelObjectListTabPage : DelayedPopulateClonableTabPage, IHasTrafodionSchema
    {
        private TrafodionSchema                       _sqlMxSchema = null;
        private DatabaseObjectsControl _databaseObjectsControl = null;
        protected string _headerText = null;
        private string _tabName    = null;

        /// <summary>
        /// Default constructor for the UI designer.
        /// </summary>
        public SchemaLevelObjectListTabPage()
        {

        }
        /// <summary>
        /// Constructs the tab page
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A reference to the database navigation tree</param>
        /// <param name="aTrafodionSchema">The schema object</param>
        /// <param name="aTabName">Title for the tab</param>
        public SchemaLevelObjectListTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema, string aTabName)
            : base(aTabName)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _sqlMxSchema = aTrafodionSchema;
            _headerText = Properties.Resources.ThisSchemaHasNObjects + aTabName;; // "This schema has {0} " 
            _tabName = aTabName;
        }

        /// <summary>
        /// This method is overridden by the subclasses which provide the implementation
        /// </summary>
        protected override void Populate()
        {

        }

        /// <summary>
        /// The Database navigation tree
        /// </summary>
        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
        }


        /// <summary>
        /// The schema to which the list of objects belong to
        /// </summary>
        public TrafodionSchema TheTrafodionSchema
        {
            get { return _sqlMxSchema; }
        }

        /// <summary>
        /// The header text for the datagridview
        /// </summary>
        public string HeaderText
        {
            get { return _headerText; }
        }


        /// <summary>
        /// Title for the tab
        /// </summary>
        public string TabName
        {
            get { return _tabName; }
        }
    }

}
