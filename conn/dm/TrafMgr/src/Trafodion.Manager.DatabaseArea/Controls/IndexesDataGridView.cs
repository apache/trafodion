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

using System;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A data grid that shows information about indexes.
    /// </summary>
    public class IndexesDataGridView : DatabaseAreaObjectsDataGridView
    {
        private bool _showDependencyColumn = false;
        private TrafodionObject _parentTrafodionObject;
        private List<TrafodionIndex> _indexList;

        /// <summary>
        /// The object that represents the parent of the list of indexes displayed in this
        /// data grid.
        /// </summary>
        public TrafodionObject ParentObject
        {
            get { return _parentTrafodionObject; }
            private set
            {
                _parentTrafodionObject = value;

                _showDependencyColumn = _parentTrafodionObject != null && _parentTrafodionObject is TrafodionSchema;
                SetupColumns();
                Load();
            }
        }

        /// <summary>
        /// Create a Datagridview to display a list of indexes.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control has access to the DatabaseTreeView</param>
        /// <param name="aParentObject">The parent schema object in whose context, we are displaying this list</param>
        /// <param name="aIndexList">The list of indexes that need to be displayed</param>
        public IndexesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, IndexedSchemaObject aParentObject, List<TrafodionIndex> aIndexList)
            : base(aDatabaseObjectsControl)
        {
            _indexList = aIndexList;

            // Setting the parent object will trigger a load.
            ParentObject = aParentObject;
        }

        /// <summary>
        /// Create a Datagridview to display a list of indexes.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control has access to the DatabaseTreeView</param>
        /// <param name="aParentObject">The parent schema object in whose context, we are displaying this list</param>
        /// <param name="aIndexList">The list of indexes that need to be displayed</param>
        public IndexesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aParentObject, List<TrafodionIndex> aIndexList)
            : base(aDatabaseObjectsControl)
        {
            _indexList = aIndexList;

            // Setting the parent object will trigger a load.
            ParentObject = aParentObject;
        }

        /// <summary>
        /// Load the datagridview with the index data.   The index datagrid is different on four situations
        /// Support vs non support or weather the grid needs the dependency column or not.  The dependency 
        /// is displayed for indexes listed in the indexes folder under schema.
        /// </summary>
        /// <returns></returns>
        public int Load()
        {
            Rows.Clear();

            foreach (TrafodionIndex sqlMxIndex in _indexList)
            {
                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxIndex.VisibleAnsiName))
                {
                    if (_showDependencyColumn)
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxIndex),
                            CreateLinkToObject(sqlMxIndex.IndexedSchemaObject),
                            sqlMxIndex.Owner,
                            sqlMxIndex.UID, 
                            sqlMxIndex.FormattedCreateTime(),
                            sqlMxIndex.FormattedRedefTime()
                        });
                    }
                    else
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxIndex),
                            sqlMxIndex.Owner,
                            sqlMxIndex.UID, 
                            sqlMxIndex.FormattedCreateTime(),
                            sqlMxIndex.FormattedRedefTime()
                        });
                    }
                }
            }

            return Rows.Count;
        }

        private void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.Name));

            if (_showDependencyColumn)
            {
                Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theOnColumn", Properties.Resources.On));
            }

            Columns.Add("theOwnerColumn", Properties.Resources.Owner);
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }

        /// <summary>
        /// Creates the link to the name of the Sql schema object, in the name column of the datagridview
        /// </summary>
        /// <param name="aTrafodionSchemaObject"></param>
        /// <returns></returns>
        private DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionSchemaObject aTrafodionSchemaObject)
        {
            return new DatabaseAreaObjectsDataGridViewLink(TheDatabaseTreeView, aTrafodionSchemaObject);
        }
    }
}
