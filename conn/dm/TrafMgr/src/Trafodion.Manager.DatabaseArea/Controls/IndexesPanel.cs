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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// This panel will display 
    /// </summary>
    public class IndexesPanel : TrafodionSchemaObjectListPanel<TrafodionIndex>, ICloneToWindow
    {        
        /// <summary>
        /// Creates a new panel to display index information.
        /// </summary>
        /// <param name="databaseObjectsControl"></param>
        /// <param name="parentObject">The schema who is viratually parenting the list of indexes.</param>
        /// <param name="indexList">The list of indexes who is under the given parent.</param>
        /// <param name="header">The text to display just above the data grid.</param>
        public IndexesPanel(DatabaseObjectsControl databaseObjectsControl, TrafodionObject parentTrafodionObject, List<TrafodionIndex> sqlMxIndexes, string headerText)
            : base(databaseObjectsControl, headerText, parentTrafodionObject, sqlMxIndexes, Properties.Resources.Indexes)
        {
        }

        /// <summary>
        /// Creates a clone of this panel.
        /// </summary>
        /// <returns>The cloned panel.</returns>
        public Control Clone()
        {
            return new IndexesPanel(null, TheParentTrafodionObject, TheTrafodionObjects, TheHeaderText);
        }
        
        protected override void AddGridColumn()
        {
            this.grid.Cols.Add("Name", Properties.Resources.Name);

            bool showDependencyColumn = TheParentTrafodionObject != null && TheParentTrafodionObject is TrafodionSchema;
            if (showDependencyColumn)
            {
                this.grid.Cols.Add("theOnColumn", Properties.Resources.On);
            }

            this.grid.Cols.Add("Owner", Properties.Resources.Owner);
            this.grid.Cols.Add("MetadataUID", Properties.Resources.MetadataUID);
            this.grid.Cols.Add("CreationTime", Properties.Resources.CreationTime);
            this.grid.Cols.Add("RedefinitionTime", Properties.Resources.RedefinitionTime);
        }

        protected override object[] ExtractValues(TrafodionSchemaObject sqlMxSchemaObject)
        {
            TrafodionIndex sqlMxIndex = (TrafodionIndex)sqlMxSchemaObject;
            bool showDependencyColumn = TheParentTrafodionObject != null && TheParentTrafodionObject is TrafodionSchema;
            if (showDependencyColumn)
            {
                return new object[] {
                            sqlMxIndex.ExternalName,
                            sqlMxIndex.IndexedSchemaObject.ExternalName,
                            sqlMxIndex.Owner,
                            sqlMxIndex.UID, 
                            sqlMxIndex.FormattedCreateTime(),
                            sqlMxIndex.FormattedRedefTime()
                        };
            }
            else
            {
                return new object[] {
                            sqlMxIndex.ExternalName,
                            sqlMxIndex.Owner,
                            sqlMxIndex.UID, 
                            sqlMxIndex.FormattedCreateTime(),
                            sqlMxIndex.FormattedRedefTime()
                        };
            }
        }
    }
}
