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
    /// Datagrid for an Index's attributes
    /// </summary>
    public class IndexAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructor for IndexAttributesDataGridView
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionIndex"></param>
        public IndexAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionIndex aTrafodionIndex)
            : base(aDatabaseObjectsControl, aTrafodionIndex)
        {
        }

        /// <summary>
        /// Gets the TrafodionIndex
        /// </summary>
        public TrafodionIndex TrafodionIndex
        {
            get 
            { 
                return TrafodionObject as TrafodionIndex; 
            }
        }

        /// <summary>
        /// Load the rows for the index attributes
        /// </summary>
        override protected void LoadRows()
        {            
           // AddRow(Properties.Resources.Unique, Utilities.YesNo(TrafodionIndex.IsUniques));
           // AddRow(Properties.Resources.Populated, TrafodionIndex.IsValidData);

            //AddRow(Properties.Resources.SystemCreated, TrafodionIndex.IsSystemCreated);
            //AddRow(Properties.Resources.HashPartitioned, TrafodionIndex.IsHashPartitioned);
            //AddRow(Properties.Resources.AuditCompressed, TrafodionIndex.IsAuditCompress);
            //AddRow(Properties.Resources.BlockSize, TrafodionIndex.FormattedBlockSize);
           // AddRow(Properties.Resources.ClearOnPurge, TrafodionIndex.IsClearOnPurge);
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new IndexAttributesDataGridView(null, TrafodionIndex);
        }

        #endregion
    }
}
