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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Controls;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class is used to display indexes under a schema or indexable object such as a table or MV.
    /// </summary>
    class TrafodionIndexesTabPage : SchemaLevelObjectListTabPage
    {
        // The object whose indexes will be displayed. This must be a TrafodionSchema or IndexedSchemaObject.
        TrafodionObject _sqlMxObject;

        /// <summary>
        /// Instantiates a new tab page for a given schema.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A handle to the control with the tree. Used for hyperlinking.</param>
        /// <param name="aTrafodionSchema">The schema whose indexes will be displayed.</param>
        public TrafodionIndexesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema)
            : base(aDatabaseObjectsControl, aTrafodionSchema, Properties.Resources.Indexes)
        {
            _headerText = Properties.Resources.ThisSchemaHasNObjects + Text;
            _sqlMxObject = aTrafodionSchema;
        }

        /// <summary>
        /// Instantiates a new tab page for a given indexable object.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A handle to the control with the tree. Used for hyperlinking.</param>
        /// <param name="aIndexedSchemaObject">The indexable object whose indexes will be displayed.</param>
        public TrafodionIndexesTabPage(DatabaseObjectsControl aDatabaseObjectsControl, IndexedSchemaObject aIndexedSchemaObject)
            : base(aDatabaseObjectsControl, aIndexedSchemaObject.TheTrafodionSchema, Properties.Resources.Indexes)
        {
            _headerText = Properties.Resources.ThereAreNIndexes;
            _sqlMxObject = aIndexedSchemaObject;
        }

        public override void PrepareForPopulate()
        {
            if (_sqlMxObject is TrafodionSchema)
            {
                object a = (_sqlMxObject as TrafodionSchema).TrafodionIndexes;
            }
            else if (_sqlMxObject is IndexedSchemaObject)
            {
                object a = (_sqlMxObject as IndexedSchemaObject).TrafodionIndexes;
            }
        }

        protected override void Populate()
        {
            IndexesPanel indexesPanel;
            if (_sqlMxObject is TrafodionSchema)
            {
                indexesPanel = new IndexesPanel(
                TheDatabaseObjectsControl,
                _sqlMxObject,
                (_sqlMxObject as TrafodionSchema).TrafodionIndexes,
                _headerText);
            }
            else if (_sqlMxObject is IndexedSchemaObject)
            {
                indexesPanel = new IndexesPanel(
                TheDatabaseObjectsControl,
                _sqlMxObject,
                (_sqlMxObject as IndexedSchemaObject).TrafodionIndexes,
                _headerText);
            }
            else
            {
                // This is unexpected and indicates a coding error.
                throw new ApplicationException();
            }

            indexesPanel.Dock = DockStyle.Fill;

            Controls.Clear();
            Controls.Add(indexesPanel);
        }
    }
}
