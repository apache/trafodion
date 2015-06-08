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

using System;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class FunctionActionsDataGridView : DatabaseAreaObjectsDataGridView
    {
        private bool _showDependencyColumn = false;
        private TrafodionObject _parentTrafodionObject;
        private List<TrafodionFunctionAction> _functionActionList;

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

        public FunctionActionsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl,
            TrafodionUDFunction aParentObject, List<TrafodionFunctionAction> aFunctionActionList)
            : base(aDatabaseObjectsControl)
        {
            _functionActionList = aFunctionActionList;
            ParentObject = aParentObject;
        }

        public FunctionActionsDataGridView(DatabaseObjectsControl aDatabaseObjectsControl,
            TrafodionSchema aParentObject, List<TrafodionFunctionAction> aFunctionActionList)
            : base(aDatabaseObjectsControl)
        {
            _functionActionList = aFunctionActionList;
            ParentObject = aParentObject;
        }

        public int Load()
        {
            Rows.Clear();

            foreach (TrafodionFunctionAction sqlMxFunctionAction in _functionActionList)
            {
                if ((TheNameFilter == null) || TheNameFilter.Matches(sqlMxFunctionAction.VisibleAnsiName))
                {
                    if (_showDependencyColumn)
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxFunctionAction),
                            CreateLinkToObject(sqlMxFunctionAction.TrafodionUDFunction),
                            sqlMxFunctionAction.UID, 
                            sqlMxFunctionAction.FormattedCreateTime(),
                            sqlMxFunctionAction.FormattedRedefTime()
                        });
                    }
                    else
                    {
                        Rows.Add(new object[] {
                            CreateLinkToObject(sqlMxFunctionAction),
                            sqlMxFunctionAction.UID, 
                            sqlMxFunctionAction.FormattedCreateTime(),
                            sqlMxFunctionAction.FormattedRedefTime()
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

            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theActionNameColumn", Properties.Resources.ActionName));

            if (_showDependencyColumn)
            {
                Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theUniversalFunctionColumn", Properties.Resources.UniversalFunction));
            }

            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }

        private DatabaseAreaObjectsDataGridViewLink CreateLinkToObject(TrafodionSchemaObject aTrafodionSchemaObject)
        {
            return new DatabaseAreaObjectsDataGridViewLink(TheDatabaseTreeView, aTrafodionSchemaObject);
        }
    }
}
