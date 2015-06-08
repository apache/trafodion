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
using System.Linq;
using System.Windows.Forms;
using System.ComponentModel;
using System.Collections.Generic;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Displays a schema object's privileges in a datagrid.
    /// </summary>
    public class SchemaObjectPrivilegeDataGridView : DatabaseAreaObjectsDataGridView, ICloneToWindow
    {
        private TrafodionSchemaObject _schemaObject;
        private bool _showColumnPrivileges = true;

        #region Properties

        /// <summary>
        /// Read only property that supplies a suitable title for the managed window.
        /// </summary>
        public string WindowTitle
        {
            get { return _schemaObject.VisibleAnsiName + " " + Properties.Resources.Privileges; }
        }


        /// <summary>
        /// Stores Connection Definition Property for this object
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return SchemaObject.ConnectionDefinition; }
        }

        /// <summary>
        /// Determines if column privileges should be shown or not.
        /// </summary>
        public bool ShowColumnPrivileges
        {
            get { return _showColumnPrivileges; }
            set { _showColumnPrivileges = value; }
        }

        /// <summary>
        /// The schema that is displayed in the table.
        /// </summary>
        public TrafodionSchemaObject SchemaObject
        {
            get { return _schemaObject; }
            set
            {
                _schemaObject = value;

                // Clear's the old schema object's info.
                Rows.Clear();

                if (_schemaObject == null)
                {
                    return;
                }
                
                // Add the schema object's schema privilege info into the table.
                foreach (SchemaPrivilege priv in _schemaObject.TheTrafodionSchema.Privileges)
                {
                    string privilegeList = priv.DisplayPrivileges(_schemaObject.SchemaObjectType);

                    if (privilegeList.Length > 0)
                    {
                        Rows.Add(new object[] {
                            priv.GranteeName,
                            priv.GrantorName,
                            (priv.Grantable ? "X" : ""),
                            Properties.Resources.Schema,
                            privilegeList
                        });
                    }
                }

                // Add the schema object's privilege information into the table.
                foreach (SchemaObjectPrivilege priv in _schemaObject.Privileges)
                {
                    string privilegeList = priv.DisplayPrivileges();

                    if (privilegeList.Length > 0)
                    {
                        Rows.Add(new object[] {
                            priv.GranteeName,
                            priv.GrantorName,
                            (priv.Grantable ? "X" : ""),
                            Properties.Resources.Object,
                            privilegeList
                        });
                    }
                }

                // Show the column privileges when desired.
                if (ShowColumnPrivileges)
                {
                    // Add the column privilege information into the table.
                    IHasTrafodionColumns hasColumns = _schemaObject as IHasTrafodionColumns;
                    if (hasColumns != null)
                    {
                        string[] granteeNames = (from priv in hasColumns.ColumnPrivileges select priv.GranteeName).Distinct().ToArray();
                        foreach (string granteeName in granteeNames)
                        {
                            string[] grantorNames = (from priv in hasColumns.ColumnPrivileges where priv.GranteeName.Equals(granteeName) select priv.GrantorName).Distinct().ToArray();
                            foreach (string grantorName in grantorNames)
                            {
                                ColumnPrivilege[] grantableColumnPrivileges = (from priv in hasColumns.ColumnPrivileges where priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) && priv.Grantable == true select priv).Distinct().ToArray();
                                ColumnPrivilege[] nonGrantableColumnPrivileges = (from priv in hasColumns.ColumnPrivileges where priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) && priv.Grantable == false select priv).Distinct().ToArray();

                                if(grantableColumnPrivileges.Length > 0)
                                {
                                    string selectString = "";
                                    string insertString = "";
                                    string updateString = "";
                                    string refString = "";

                                    string[] selectArray = (from priv in grantableColumnPrivileges where priv.Select == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (selectArray.Length == hasColumns.Columns.Count)
                                    {
                                        selectString = string.Format("Select : ({0})", "All Columns");
                                    }
                                    else if (selectArray.Length > 0)
                                    {
                                        selectString = string.Format("Select : ({0})",
                                                                string.Join(", ", selectArray));
                                    }

                                    string[] insertArray = (from priv in grantableColumnPrivileges where priv.Insert == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (insertArray.Length == hasColumns.Columns.Count)
                                    {
                                        insertString = string.Format("Insert : ({0})", "All Columns");
                                    }
                                    else if (insertArray.Length > 0)
                                    {
                                        insertString = string.Format("Insert : ({0})",
                                                                string.Join(", ", insertArray));
                                    }

                                    string[] updateArray = (from priv in grantableColumnPrivileges where priv.Update == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (updateArray.Length == hasColumns.Columns.Count)
                                    {
                                        updateString = string.Format("Update : ({0})", "All Columns");
                                    }
                                    else if (updateArray.Length > 0)
                                    {
                                        updateString = string.Format("Update : ({0})",
                                                                string.Join(", ", updateArray));
                                    }

                                    string[] referenceArray = (from priv in grantableColumnPrivileges where priv.Reference == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (referenceArray.Length == hasColumns.Columns.Count)
                                    {
                                        refString = string.Format("References : ({0})", "All Columns");
                                    }    
                                    else if (referenceArray.Length > 0)
                                    {
                                        refString = string.Format("References : ({0})",
                                                                string.Join(", ", referenceArray));
                                    }

                                    List<string> colPriv = new List<string>();
                                    if (!string.IsNullOrEmpty(selectString))
                                    {
                                        colPriv.Add(selectString);
                                    }

                                    if (!string.IsNullOrEmpty(insertString))
                                    {
                                        colPriv.Add(insertString);
                                    }

                                    if (!string.IsNullOrEmpty(updateString))
                                    {
                                        colPriv.Add(updateString);
                                    }

                                    if (!string.IsNullOrEmpty(refString))
                                    {
                                        colPriv.Add(refString);
                                    }

                                    if (colPriv.Count > 0)
                                    {
                                        Rows.Add(new object[] {
                                        granteeName,
                                        grantorName,
                                        "X",
                                        Properties.Resources.Column,
                                        string.Join("\n", colPriv.ToArray())
                                        });
                                    }
                                }

                                if(nonGrantableColumnPrivileges.Length > 0)
                                {
                                    string selectString = "";
                                    string insertString = "";
                                    string updateString = "";
                                    string refString = "";

                                    string[] selectArray = (from priv in nonGrantableColumnPrivileges where priv.Select == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (selectArray.Length == hasColumns.Columns.Count)
                                    {
                                        selectString = string.Format("Select : ({0})", "All Columns");
                                    }
                                    else if (selectArray.Length > 0)
                                    {
                                        selectString = string.Format("Select : ({0})",
                                                                string.Join(", ", selectArray));
                                    }

                                    string[] insertArray = (from priv in nonGrantableColumnPrivileges where priv.Insert == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (insertArray.Length == hasColumns.Columns.Count)
                                    {
                                        insertString = string.Format("Insert : ({0})", "All Columns");
                                    }
                                    else if (insertArray.Length > 0)
                                    {
                                        insertString = string.Format("Insert : ({0})",
                                                                string.Join(", ", insertArray));
                                    }

                                    string[] updateArray = (from priv in nonGrantableColumnPrivileges where priv.Update == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (updateArray.Length == hasColumns.Columns.Count)
                                    {
                                        updateString = string.Format("Update : ({0})", "All Columns");
                                    }
                                    else if (updateArray.Length > 0)
                                    {
                                        updateString = string.Format("Update : ({0})",
                                                                string.Join(", ", updateArray));
                                    }

                                    string[] referenceArray = (from priv in nonGrantableColumnPrivileges where priv.Reference == true && priv.GranteeName.Equals(granteeName) && priv.GrantorName.Equals(grantorName) select TrafodionName.ExternalForm(priv.ColumnName)).ToArray();
                                    if (referenceArray.Length == hasColumns.Columns.Count)
                                    {
                                        refString = string.Format("References : ({0})", "All Columns");
                                    }
                                    else if (referenceArray.Length > 0)
                                    {
                                        refString = string.Format("References : ({0})",
                                                                string.Join(", ", referenceArray));
                                    }

                                    List<string> colPriv = new List<string>();
                                    if (!string.IsNullOrEmpty(selectString))
                                    {
                                        colPriv.Add(selectString);
                                    }

                                    if (!string.IsNullOrEmpty(insertString))
                                    {
                                        colPriv.Add(insertString);
                                    }

                                    if (!string.IsNullOrEmpty(updateString))
                                    {
                                        colPriv.Add(updateString);
                                    }

                                    if (!string.IsNullOrEmpty(refString))
                                    {
                                        colPriv.Add(refString);
                                    }

                                    if (colPriv.Count > 0)
                                    {
                                        Rows.Add(new object[] {
                                        granteeName,
                                        grantorName,
                                        "",
                                        Properties.Resources.Column,
                                        string.Join("\n", colPriv.ToArray())
                                        });
                                    }
                                }
                            }
                        }

                    }
                }

                // Don't forget to sort the data!
                Sort();
            }
        }

        #endregion

        /// <summary>
        /// Creates a new datagrid view to display schema object privileges.
        /// </summary>
        public SchemaObjectPrivilegeDataGridView()
        {
            // Setup the datagrid's columns.
            ColumnCount = 5;
            Dock = DockStyle.Fill;

            Columns[0].HeaderText = Properties.Resources.Grantee;
            Columns[0].FillWeight = 1;

            Columns[1].HeaderText = Properties.Resources.Grantor;
            Columns[1].FillWeight = 1;

            Columns[2].HeaderText = Properties.Resources.WithGrant;
            Columns[2].FillWeight = 1;

            Columns[3].HeaderText = Properties.Resources.GrantLevel;
            Columns[3].FillWeight = 1;

            Columns[4].HeaderText = Properties.Resources.Privileges;
            Columns[4].FillWeight = 4;
            Columns[4].DefaultCellStyle.WrapMode = DataGridViewTriState.True;

            // Set the default sort column.
            Sort(Columns[0], ListSortDirection.Ascending);
        }

        /// <summary>
        /// Creates a new datagrid view to display schema object privileges.
        /// </summary>
        /// <param name="schemaObject">The schema object whose privileges will be displayed.</param>
        public SchemaObjectPrivilegeDataGridView(TrafodionSchemaObject schemaObject)
            : this()
        {
            // Assign the schema object. This will automatically populate the grid.
            SchemaObject = schemaObject;
        }

        /// <summary>
        /// Get a clone suitable for embedding in a managed window.
        /// </summary>
        /// <returns>The control</returns>
        public Control Clone()
        {
            return new SchemaObjectPrivilegeDataGridView(SchemaObject);
        }

        /// <summary>
        /// Writes all of the privileges in a single string as a comma separated list.
        /// </summary>
        /// <param name="colPriv">The privileges to string-ify.</param>
        /// <returns>A comma seperated list of privileges.</returns>
        private string writeColumnPrivileges(ColumnPrivilege colPriv)
        {
            string privMsg = "";

            if (colPriv.Select)
            {
                privMsg += (privMsg.Length > 0 ? ", " : "");
                privMsg += Properties.Resources.Select;
            }

            if (colPriv.Insert)
            {
                privMsg += (privMsg.Length > 0 ? ", " : "");
                privMsg += Properties.Resources.Insert;
            }

            if (colPriv.Update)
            {
                privMsg += (privMsg.Length > 0 ? ", " : "");
                privMsg += Properties.Resources.Update;
            }

            if (colPriv.Reference)
            {
                privMsg = Properties.Resources.Reference;
            }

            return privMsg + " : (" + TrafodionName.ExternalForm(colPriv.ColumnName) + ")";
        }

        /// <summary>
        /// This function is supposed to filter out column privileges that are set at the object level. It
        /// currently does not work correctly.
        /// TODO: Fix function.
        /// </summary>
        /// <param name="schemaObjectList"></param>
        /// <param name="columnCount"></param>
        /// <param name="colPrivList"></param>
        /// <returns></returns>
        private List<ColumnPrivilege> filterColumnPrivileges(List<SchemaObjectPrivilege> schemaObjectList, int columnCount, List<ColumnPrivilege> colPrivList)
        {
            List<ColumnPrivilege> filteredColumnPrivileges = new List<ColumnPrivilege>();

            // Sort the lists so that we can go through them faster.
            schemaObjectList.Sort();
            colPrivList.Sort();

            // Next, walk through the list of privileges and compare them to the column privileges.
            foreach (SchemaObjectPrivilege schemaPriv in schemaObjectList)
            {
                // The ColumnPrivileges will be sorted based on column number.
                // We start at column 0 and just say that we match.
                int currentColNumber = 0;
                bool contiguousMatch = true;

                foreach (ColumnPrivilege colPriv in colPrivList)
                {
                    // Are we at the next expected column and have we matched so far?
                    if (contiguousMatch && currentColNumber == colPriv.ColumnNumber)
                    {
                        // We're at the next column number. Let's verify that we still match.
                        contiguousMatch = colPriv.CompareTo(schemaPriv) == 0;

                        // Add this column, we'll worry about removing it later if all of the columns match.
                        filteredColumnPrivileges.Add(colPriv);

                        // shifting the index for easier reading
                        int columnsAdded = currentColNumber + 1;
                        if (columnsAdded == columnCount)
                        {
                            // We've reached the last column, were all columns a match?
                            if (contiguousMatch)
                            {
                                // Yes, remove them from the list.
                                filteredColumnPrivileges.RemoveRange(filteredColumnPrivileges.Count - columnsAdded, columnsAdded);
                            }

                            // Nope, start over.
                            currentColNumber = 0;
                            contiguousMatch = true;
                        }
                        else
                        {
                            // We're not at the end yet, prepare for the next column.
                            currentColNumber++;
                        }
                    }
                    else
                    {
                        // We do not have a contiguous match, add this column and start over.
                        currentColNumber = 0;
                        contiguousMatch = true;

                        filteredColumnPrivileges.Add(colPriv);
                    }
                }
            }

            return filteredColumnPrivileges;
        }
    }
}
