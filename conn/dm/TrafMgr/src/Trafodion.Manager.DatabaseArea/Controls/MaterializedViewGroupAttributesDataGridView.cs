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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagridview to display table attributes 
    /// </summary>
    public class MaterializedViewGroupAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructs the datagridview to display MaterializedViewGroup attributes
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A reference to the database navigation tree</param>
        /// <param name="aTrafodionMaterializedViewGroup">MaterializedViewGroup whose attributes are to be displayed</param>
        public MaterializedViewGroupAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, 
                                                           TrafodionMaterializedViewGroup aTrafodionMaterializedViewGroup)
            : base(aDatabaseObjectsControl, aTrafodionMaterializedViewGroup)
        {
        }
        /// <summary>
        /// The MaterializedViewGroup whose attributes are displayed in the datagridview
        /// </summary>
        public TrafodionMaterializedViewGroup TrafodionMaterializedViewGroup
        {
            get { return TrafodionObject as TrafodionMaterializedViewGroup; }
        }

        /// <summary>
        /// Loads the attribute information into the datagridview for display
        /// </summary>
        override protected void LoadRows()
        {
            // there is nothing to override- really! because all  attributes are laoded by the base classes.
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new MaterializedViewGroupAttributesDataGridView(null, TrafodionMaterializedViewGroup);
        }

        #endregion

    }
}
