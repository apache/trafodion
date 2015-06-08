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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagrid for an Procedure's attributes
    /// </summary>
    public class RoutineAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructor for RoutineAttributesDataGridView
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionRoutine"></param>
        public RoutineAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionRoutine aTrafodionRoutine)
            : base(aDatabaseObjectsControl, aTrafodionRoutine)
        {
        }

        /// <summary>
        /// Gets the TrafodionRoutine
        /// </summary>
        public TrafodionRoutine TrafodionRoutine
        {
            get
            {
                return TrafodionObject as TrafodionRoutine;
            }
        }
        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new RoutineAttributesDataGridView(null, TrafodionRoutine);
        }

        #endregion
    }
}
