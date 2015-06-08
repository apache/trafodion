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
    /// Datagridview to display MaterializedView attributes
    /// </summary>
    public class MaterializedViewAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructs the datagridview to display MaterializedView attributes
        /// </summary>
        /// <param name="aDatabaseObjectsControl">A reference to the database navigation tree</param>
        /// <param name="aTrafodionMaterializedView">MV whose attributes are to be displayed</param>
        public MaterializedViewAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionMaterializedView aTrafodionMaterializedView)
            : base(aDatabaseObjectsControl, aTrafodionMaterializedView)
        {
        }
        /// <summary>
        /// The MaterializedView whose attributes are displayed in the datagridview
        /// </summary>
        public TrafodionMaterializedView TrafodionMaterializedView
        {
            get { return TrafodionObject as TrafodionMaterializedView; }
        }

        /// <summary>
        /// Loads the attribute information into the datagridview for display
        /// </summary>
        override protected void LoadRows()
        {
            AddRow(Properties.Resources.AuditCompressed, TrafodionMaterializedView.FormattedIsAuditCompress);
            AddRow(Properties.Resources.ClearOnPurge, TrafodionMaterializedView.FormattedIsClearOnPurge);
            AddRow(Properties.Resources.BlockSize, TrafodionMaterializedView.FormattedBlockSize);
            AddRow(Properties.Resources.CreationRefreshType, TrafodionMaterializedView.FormattedCreationRefreshType);
            AddRow(Properties.Resources.InitializatonType, TrafodionMaterializedView.initializationType);
            AddRow(Properties.Resources.CommitRefreshEach, TrafodionMaterializedView.FormattedCommitRefreshEach);
            AddRow(Properties.Resources.RefreshedAtTimestamp, TrafodionMaterializedView.FormattedRefreshedAtTimestamp);
        }
        
        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new MaterializedViewAttributesDataGridView(null, TrafodionMaterializedView);
        }

        #endregion

    }
}
