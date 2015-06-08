//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Datagrid for an Library's attributes
    /// </summary>
    public class LibraryAttributesDataGridView : TrafodionSchemaObjectAttributesDataGridView
    {
             /// <summary>
        /// Constructor for LibraryAttributesDataGridView
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionLibrary"></param>
        public LibraryAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionLibrary aTrafodionLibrary)
            : base(aDatabaseObjectsControl, aTrafodionLibrary)
        {
        }

        /// <summary>
        /// Gets the TrafodionLibrary
        /// </summary>
        public TrafodionLibrary TrafodionLibrary
        {
            get 
            {
                return TrafodionObject as TrafodionLibrary; 
            }
        }

        override protected void LoadRows()
        {
#warning : The code below should be removed after enable browsing of System libraries
            if (TrafodionLibrary.IsMetadataObject)
            {
                MessageBox.Show("Browsing is currently not supported for System libraries.");
                return;
            }

            //if (TrafodionLibrary.ConnectionDefinition.ComponentPrivilegeExists("SQL_OPERATIONS", "CREATE_LIBRARY"))
            {
                AddRow(Properties.Resources.FileName, TrafodionLibrary.FileName);
            }
            //AddRow(Properties.Resources.ClientName, TrafodionLibrary.ClientName);
            //AddRow(Properties.Resources.ClientFileName, TrafodionLibrary.ClientFileName);
            if (ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140) 
            {
                AddRow("Type", TrafodionLibrary.IsMetadataObject ? "System" : "User"); 
            }
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new LibraryAttributesDataGridView(null, TrafodionLibrary);
        }

        #endregion
    }
}
