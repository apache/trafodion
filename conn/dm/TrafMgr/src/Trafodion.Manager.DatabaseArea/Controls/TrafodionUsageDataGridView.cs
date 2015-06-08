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
    /// A generic datagridview to display a list of usage on a schema object.
    /// The usage list displays the depedent object names as 3 part or 2 part ansi names
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class TrafodionUsageDataGridView<T> : DatabaseAreaObjectsDataGridView where T : TrafodionSchemaObject
    {
        protected T _sqlMxSchemaObject;

        /// <summary>
        /// Create a generic Datagridview to display a list of usage on a schema object.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">The control has access to the DatabaseTreeView</param>
        /// <param name="sqlMxSchemaObject">The parent schema object in whose context, we are displaying this list</param>
        public TrafodionUsageDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, T sqlMxSchemaObject)
            : base(aDatabaseObjectsControl)
        {
            _sqlMxSchemaObject = sqlMxSchemaObject;
            SetupColumns();
        }

        /// <summary>
        /// Call this function if the columns ever need to change.
        /// </summary>
        virtual public void SetupColumns()
        {
            Columns.Clear();
            Rows.Clear();

            Columns.Add("theRelationColumn", Properties.Resources.UsageType);
            Columns.Add("theObjectTypeColumn", Properties.Resources.UsageObjectType);
            Columns.Add(new DatabaseAreaObjectsDataGridViewLinkColumn("theNameColumn", Properties.Resources.UsageObjectName));
            Columns.Add("theUIDColumn", Properties.Resources.MetadataUID);
            //Create time and redefinition time apply to all schema objects
            Columns.Add("theCreateTimeColumn", Properties.Resources.CreationTime);
            Columns.Add("theRedefTimeColumn", Properties.Resources.RedefinitionTime);
        }

        /// <summary>
        /// Add the usage objects to the grid
        /// </summary>
        /// <typeparam name="UT">The usage object type</typeparam>
        /// <param name="usageType">The type of the usage object</param>
        /// <param name="sqlMxUsageObjects">List of the usage objects</param>
        /// <returns></returns>
        public virtual int AddUsage<UT>(string usageType, List<UT> sqlMxUsageObjects) where UT : TrafodionSchemaObject
        {
            if (sqlMxUsageObjects != null)
            {
                foreach (UT usageObject in sqlMxUsageObjects)
                {
                    //TODO:ROLEBasedDisplay
                    //if (usageObject.IsMetadataObject)
                    //    continue;

                    if ((TheNameFilter == null) || TheNameFilter.Matches(usageObject.VisibleAnsiName))
                    {
                            Rows.Add(new object[] {
                                usageType,
                                usageObject.DisplayObjectType,
                                new DatabaseAreaObjectsVisibleAnsiLink(TheDatabaseTreeView, usageObject),
                                usageObject.UID, 
                                usageObject.FormattedCreateTime(),
                                usageObject.FormattedRedefTime()
                        });
                    }
                }
            }

            return Rows.Count;
        }
    }
}
