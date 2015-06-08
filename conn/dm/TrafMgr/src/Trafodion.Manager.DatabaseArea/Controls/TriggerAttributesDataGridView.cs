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
    /// Datagrid for an Trigger's attributes
    /// </summary>
    public class TriggerAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructor for TriggerAttributesDataGridView
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionTrigger"></param>
        public TriggerAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionTrigger aTrafodionTrigger)
            : base(aDatabaseObjectsControl, aTrafodionTrigger)
        {
        }

        /// <summary>
        /// Gets the TrafodionTrigger
        /// </summary>
        public TrafodionTrigger TrafodionTrigger
        {
            get 
            {
                return TrafodionObject as TrafodionTrigger; 
            }
        }

        override protected void LoadRows()
        {

            AddRow(Properties.Resources.IsEnabled, TrafodionTrigger.FormattedIsEnabled);
            AddRow(Properties.Resources.ActivationTime, TrafodionTrigger.FormattedActivationTime);
            AddRow(Properties.Resources.Operation, TrafodionTrigger.FormattedOperation);
            AddRow(Properties.Resources.Granularity, TrafodionTrigger.FormattedGranularity);
               
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new TriggerAttributesDataGridView(null, TrafodionTrigger);
        }

        #endregion
    }
}
