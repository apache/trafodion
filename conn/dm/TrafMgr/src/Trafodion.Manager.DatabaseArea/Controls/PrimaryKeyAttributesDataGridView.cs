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

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A datagridview that display that attributes of a primary key
    /// </summary>
    public class PrimaryKeyAttributesDataGridView : TrafodionObjectAttributesDataGridView
    {
        /// <summary>
        /// Constructs a datagridview to display the attributes of a primary key
        /// </summary>
        /// <param name="aDatabaseObjectsControl"></param>
        /// <param name="aTrafodionPrimaryKey"></param>
        public PrimaryKeyAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionPrimaryKey aTrafodionPrimaryKey)
            : base(aDatabaseObjectsControl, aTrafodionPrimaryKey)
        {
        }
        /// <summary>
        /// The Primary key object whose attributes are displayed in the datagridview
        /// </summary>
        public TrafodionPrimaryKey TheTrafodionPrimaryKey
        {
            get { return TrafodionObject as TrafodionPrimaryKey; }
            set { TrafodionObject = value; }
        }
        /// <summary>
        /// Loads the primary key attributes into the datagridview
        /// </summary>
        override protected void LoadRows()
        {
            AddRow(Properties.Resources.Droppable, Trafodion.Manager.Framework.Utilities.YesNo(TheTrafodionPrimaryKey.CanDropPrimaryKey));
        }

        /// <summary>
        /// Overrider the default attribute "Name" and call it Primary key constraint name
        /// </summary>
        /// <returns></returns>
        override protected string GetDisplayNameForNameAttribute()
        {
            return Properties.Resources.PrimaryKeyName;
        }


        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        override public Control Clone()
        {
            return new PrimaryKeyAttributesDataGridView(null, TheTrafodionPrimaryKey);
        }

        #endregion

    }
}
