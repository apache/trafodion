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
using System.ComponentModel;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Displays a schema's privileges in a datagrid.
    /// </summary>
    public class SchemaPrivilegeDataGridView : DatabaseAreaObjectsDataGridView, ICloneToWindow
    {
        private TrafodionSchema _schema;

        #region Properties

        /// <summary>
        /// Read only property that supplies a suitable title for the managed window.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                string title = "";
                if (_schema != null)
                {
                    title += _schema.VisibleAnsiName + " ";
                }
                title += Properties.Resources.Privileges;
                return title;
            }
        }

        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get
            {
                if (Schema == null)
                {
                    return null;
                }

                return Schema.ConnectionDefinition;
            }
        }


        /// <summary>
        /// The schema that is displayed in the table.
        /// </summary>
        public TrafodionSchema Schema
        {
            get { return _schema; }
            set
            {
                _schema = value;

                // Clear the old schema's info.
                Rows.Clear();

                if (_schema != null)
                {
                    try
                    {
                        // Add the information into the table.
                        foreach (SchemaPrivilege priv in _schema.Privileges)
                        {
                            string privilegeList = priv.DisplayPrivileges();
                            if (privilegeList.Length > 0)
                            {
                                Rows.Add(new object[] {
                                    priv.GranteeName,
                                    priv.GrantorName,
                                    (priv.Grantable ? "X" : ""),
                                    privilegeList
                                    });
                            }
                        }
                    }
                    catch (System.Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, Properties.Resources.SchemaPrivilegeLoadFailureMessage, MessageBoxButtons.OK);
                        return;
                    }
                }

                // Don't forget to sort the data!
                Sort();
            }
        }

        #endregion

        /// <summary>
        /// Creates a new datagrid view to display schema privileges.
        /// </summary>
        public SchemaPrivilegeDataGridView()
        {
            // Setup the datagrid's columns.
            ColumnCount = 4;
            Dock = DockStyle.Fill;

            Columns[0].HeaderText = Properties.Resources.Grantee;
            Columns[0].FillWeight = 1;

            Columns[1].HeaderText = Properties.Resources.Grantor;
            Columns[1].FillWeight = 1;

            Columns[2].HeaderText = Properties.Resources.WithGrant;
            Columns[2].FillWeight = 1;

            Columns[3].HeaderText = Properties.Resources.Privileges;
            Columns[3].FillWeight = 4;
            Columns[3].DefaultCellStyle.WrapMode = DataGridViewTriState.True;

            // Set the default sort column.
            Sort(Columns[0], ListSortDirection.Ascending);
        }

        /// <summary>
        /// Creates a new datagrid view to display schema privileges.
        /// </summary>
        /// <param name="schema">The schema whose privileges will be displayed.</param>
        public SchemaPrivilegeDataGridView(TrafodionSchema schema)
            : this()
        {
            // Assign the schema. This will automatically populate the grid.
            Schema = schema;
        }

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            return new SchemaPrivilegeDataGridView(Schema);
        }

    }
}
