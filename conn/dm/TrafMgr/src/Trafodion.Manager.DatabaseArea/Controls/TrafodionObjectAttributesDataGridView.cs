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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public abstract class TrafodionObjectAttributesDataGridView : DatabaseAreaObjectsDataGridView, ICloneToWindow
    {
        #region Fields

        private TrafodionObject _sqlMxObject;

        #endregion

        #region Properties

        public TrafodionObject TrafodionObject
        {
            get { return _sqlMxObject; }
            set
            {
                _sqlMxObject = value;
                Columns.Clear();
                Rows.Clear();

                Columns.Add("theAttributeColumn", "Attribute");
                Columns.Add("theValueColumn", "Value");

                foreach (DataGridViewColumn col in Columns)
                {
                    col.SortMode = DataGridViewColumnSortMode.NotSortable;
                }

                AddRow(GetDisplayNameForNameAttribute(), TrafodionObject.ExternalName);
                AddRow(Properties.Resources.MetadataUID, TrafodionObject.UID.ToString());
                if (_sqlMxObject is TrafodionSchemaObject)
                {
                    //AddRow(Properties.Resources.Owner, ((TrafodionSchemaObject)_sqlMxObject).Owner);
                }

                //Todo:  Some reshuffling of the LoadRows() ordering needs to be addressed.
                LoadRows();

            }
        }

        #endregion

        /// <summary>
        /// A data grid to display TrafodionObject attributes information.
        /// </summary>
        /// <param name="aDatabaseObjectsControl">
        /// The DatabaseObjectsControl that contains the tree.
        /// </param>
        /// <param name="aTrafodionObject">
        /// The TrafodionObject that will have it's attributes displayed.
        /// </param>
        public TrafodionObjectAttributesDataGridView(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionObject aTrafodionObject)
            : base(aDatabaseObjectsControl)
        {
            TrafodionObject = aTrafodionObject;
        }

        /// <summary>
        /// Derived classes can add addition attributes to the table here.
        /// </summary>
        protected virtual void LoadRows()
        {
        }

        /// <summary>
        /// This method returns the display name for an attribute called "Name"
        /// Some objects may want to call this differently instead of just "Name"
        /// This method is overridden by those object specific datagrid
        /// For example, the PrimaryKeyAttributesDataGridView will override this method to call
        /// the name as "Primary key constraint name"
        /// The default is "Name"
        /// </summary>
        /// <returns>The display name for the "Name" attribute</returns>
        protected virtual string GetDisplayNameForNameAttribute()
        {
            return Properties.Resources.Name;
        }

        /// <summary>
        /// A helper function to add a row to the table.
        /// </summary>
        /// <param name="aName">An attribute name.</param>
        /// <param name="aValue">An attribute value.</param>
        protected void AddRow(string aName, object aValue)
        {
            Rows.Add(new object[] { aName, aValue });
        }

        #region ICloneToWindow

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public abstract Control Clone();

        /// <summary>
        /// Read only property that supplies a suitable base title for the managed window.
        /// </summary>
        public string WindowTitle { get { return TrafodionObject.VisibleAnsiName + " Attributes"; } }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return TrafodionObject.ConnectionDefinition; }
        }

        #endregion
    }
}
