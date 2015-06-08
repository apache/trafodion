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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    class ViewAttributesDataGridView : TrafodionObjectAttributesDataGridView
    {
        public ViewAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionView aTrafodionView)
            : base(aDatabaseObjectsControl, aTrafodionView)
        {
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new ViewAttributesDataGridView(null, TrafodionObject as TrafodionView);
        }

        protected override void LoadRows()
        {
            base.LoadRows();
            if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ140&&(TrafodionObject is TrafodionView))
            {
               // AddRow(Properties.Resources.ValidDef, TrafodionView.DisplayValidState(((TrafodionView)TrafodionObject).Valid_Def));
            }
        }



        #endregion
    }
}
