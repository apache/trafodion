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
    /// A generic datagridview to display a list of Sql schema Objects.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class TrafodionSchemaObjectsDataGridView<T> : DatabaseAreaObjectsDataGridView where T: TrafodionSchemaObject
    {
        private TrafodionObject _parentTrafodionObject;

        /// <summary>
        /// The parent schema object
        /// </summary>
        public TrafodionObject TheParentTrafodionObject
        {
            get { return _parentTrafodionObject; }
            private set
            {
                _parentTrafodionObject = value;
                SetupColumns();
            }
        }

        private List<T> _sqlMxObjects;
        /// <summary>
        /// The list of schema objects displayed in the datagridview
        /// </summary>
        public List<T> TheTrafodionObjects
        {
            get { return _sqlMxObjects; }
        }

        /// <summary>
        /// Default constructor for the UI designer
        /// </summary>
        public TrafodionSchemaObjectsDataGridView()
            : base(null)
        {
            TheParentTrafodionObject = null;
        }

        /// <summary>
        /// Create a generic Datagridview to display the list of Schema Objects
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control has access to the DatabaseTreeView</param>
        /// <param name="aParentTrafodionObject">The parent sql object in whose context, we are displaying this list</param>
        /// <param name="aTrafodionObjectList">The list of TrafodionSchemaObjects that need to be displayed</param>
        public TrafodionSchemaObjectsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionObject aParentTrafodionObject, List<T> aTrafodionObjectList)
            : base(aDatabaseObjectsControl)
        {
            TheParentTrafodionObject = aParentTrafodionObject;
            _sqlMxObjects = aTrafodionObjectList;

            Load();
        }

        /// <summary>
        /// Load the datagridview with the Sql Object's data
        /// </summary>
        /// <returns></returns>
        virtual public int Load()
        {
            Rows.Clear();

            foreach (TrafodionSchemaObject sqlMxSchemaObject in TheTrafodionObjects)
            {
                //TODO:ROLEBasedDisplay
                //if (sqlMxSchemaObject.IsMetadataObject)
                  //  continue;

                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxSchemaObject.VisibleAnsiName))
                {
                    Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxSchemaObject), 
                            sqlMxSchemaObject.Owner,
                            sqlMxSchemaObject.UID, 
                            sqlMxSchemaObject.FormattedCreateTime(),
                            sqlMxSchemaObject.FormattedRedefTime()
                    });
                }
            }

            return Rows.Count;
        }
        /// <summary>
        /// Creates the link to the name of the Sql schema object, in the name column of the datagridview
        /// </summary>
        /// <param name="aTrafodionSchemaObject"></param>
        /// <returns></returns>
        protected virtual DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionSchemaObject aTrafodionSchemaObject)
        {
            return new DatabaseAreaObjectsDataGridViewLink(TheDatabaseTreeView, aTrafodionSchemaObject);
        }

        /// <summary>
        /// Call this function if the columns ever need to change.
        /// </summary>
        protected virtual void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));
            Columns.Add("theOwnerColumn", Properties.Resources.Owner);
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);

            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }
    }
}
