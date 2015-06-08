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
using System;
using System.Data.Linq;
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class SchemaPrivilegeOptionsControl : UserControl, IPrivilegeOptionsProvider
    {
        GrantRevokeControl _grantRevokeTool;
        private readonly ConnectionDefinition.SERVER_VERSION serverVersion;

        public SchemaPrivilegeOptionsControl()
        {
            InitializeComponent();
        }

        public SchemaPrivilegeOptionsControl(GrantRevokeControl grantRevokeTool, ConnectionDefinition.SERVER_VERSION serverVersion)
        {
            InitializeComponent();
            _grantRevokeTool = grantRevokeTool;
            this.serverVersion = serverVersion;
            if (this.serverVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _createLibraryCheckBox.Visible = _alterLibraryCheckBox.Visible = _dropLibraryCheckBox.Visible = false;
            }
            Reset();
        }

        void UpdateControls()
        {
            if (_allCheckBox.Checked)
            {
                _allDDLCheckBox.Enabled = _allDMLCheckBox.Enabled = _allUTILITYCheckBox.Enabled = false;
            }
            else
            {
                _allDDLCheckBox.Enabled = _allDMLCheckBox.Enabled = _allUTILITYCheckBox.Enabled = true;
            }

            if (_allDDLCheckBox.Checked)
            {
                _createCheckBox.Enabled = _alterCheckBox.Enabled = _dropCheckBox.Enabled = false;
            }
            else
            {
                _createCheckBox.Enabled = _alterCheckBox.Enabled = _dropCheckBox.Enabled = true;
            }

            if (_allDMLCheckBox.Checked)
            {
                _selectCheckBox.Enabled = _insertCheckBox.Enabled = _updateCheckBox.Enabled = _deleteCheckBox.Enabled = false;
                _executeCheckBox.Enabled = _referencesCheckBox.Enabled = _usageCheckBox.Enabled = false;
            }
            else
            {
                _selectCheckBox.Enabled = _insertCheckBox.Enabled = _updateCheckBox.Enabled = _deleteCheckBox.Enabled = true;
                _executeCheckBox.Enabled = _referencesCheckBox.Enabled = _usageCheckBox.Enabled = true;
            }

            _replicateCheckBox.Enabled = !_allUTILITYCheckBox.Checked;

            if (_createCheckBox.Checked)
            {
                _createTableCheckBox.Enabled = _createTriggerCheckBox.Enabled = _createMVCheckBox.Enabled = _createViewCheckBox.Enabled = false;
                _createMVGroupCheckBox.Enabled = _createProcedureCheckBox.Enabled = _createSynonymCheckBox.Enabled = _createLibraryCheckBox.Enabled = false;
            }
            else
            {
                _createTableCheckBox.Enabled = _createTriggerCheckBox.Enabled = _createMVCheckBox.Enabled = _createViewCheckBox.Enabled = true;
                _createMVGroupCheckBox.Enabled = _createProcedureCheckBox.Enabled = _createSynonymCheckBox.Enabled = _createLibraryCheckBox.Enabled = true;
            }
            if (_alterCheckBox.Checked)
            {
                _alterTableCheckBox.Enabled = _alterTriggerCheckBox.Enabled = _alterMVCheckBox.Enabled = _alterViewCheckBox.Enabled = false;
                _alterMVGroupCheckBox.Enabled = _alterSynonymCheckBox.Enabled = _alterLibraryCheckBox.Enabled = false;
            }
            else
            {
                _alterTableCheckBox.Enabled = _alterTriggerCheckBox.Enabled = _alterMVCheckBox.Enabled = _alterViewCheckBox.Enabled = true;
                _alterMVGroupCheckBox.Enabled = _alterSynonymCheckBox.Enabled = _alterLibraryCheckBox.Enabled = true;
            }
            if (_dropCheckBox.Checked)
            {
                _dropTableCheckBox.Enabled = _dropTriggerCheckBox.Enabled = _dropMVCheckBox.Enabled = _dropViewCheckBox.Enabled = false;
                _dropMVGroupCheckBox.Enabled = _dropProcedureCheckBox.Enabled = _dropSynonymCheckBox.Enabled = _dropLibraryCheckBox.Enabled = false;
            }
            else
            {
                _dropTableCheckBox.Enabled = _dropTriggerCheckBox.Enabled = _dropMVCheckBox.Enabled = _dropViewCheckBox.Enabled = true;
                _dropMVGroupCheckBox.Enabled = _dropProcedureCheckBox.Enabled = _dropSynonymCheckBox.Enabled = _dropLibraryCheckBox.Enabled = true;
            }


            /*
            * Compatibility for M6 & M7-
            * For M6, those new features should be hidden and disabled
            * For M7, the statement below could be removed
            */
            if (this.serverVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _utilityGroupBox.Visible = _replicateCheckBox.Visible = _allUTILITYCheckBox.Visible = _usageCheckBox.Visible = false;
                _replicateCheckBox.Checked = _allUTILITYCheckBox.Checked = _usageCheckBox.Checked = false;
            }
        }

        private void _allCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            _allDDLCheckBox.Checked = _allDMLCheckBox.Checked = _allUTILITYCheckBox.Checked = _allCheckBox.Checked;
            UpdateControls();
        }

        private void _allDMLCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (_allDMLCheckBox.Checked)
            {
                _selectCheckBox.Checked = _insertCheckBox.Checked = _updateCheckBox.Checked = _deleteCheckBox.Checked = true;
                _executeCheckBox.Checked = _referencesCheckBox.Checked = _usageCheckBox.Checked = true;
            }
            else
            {
                _selectCheckBox.Checked = _insertCheckBox.Checked = _updateCheckBox.Checked = _deleteCheckBox.Checked = false;
                _executeCheckBox.Checked = _referencesCheckBox.Checked = _usageCheckBox.Checked = false;
            }

            UpdateControls();
        }

        private void _allDDLCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (_allDDLCheckBox.Checked)
            {
                _createCheckBox.Checked = _alterCheckBox.Checked = _dropCheckBox.Checked = true;
            }
            else
            {
                _createCheckBox.Checked = _alterCheckBox.Checked = _dropCheckBox.Checked = false;
            }
            UpdateControls();
        }

        private void _createCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (_createCheckBox.Checked)
            {
                _createTableCheckBox.Checked = _createTriggerCheckBox.Checked = _createMVCheckBox.Checked = _createViewCheckBox.Checked = true;
                _createMVGroupCheckBox.Checked = _createProcedureCheckBox.Checked = _createSynonymCheckBox.Checked = _createLibraryCheckBox.Checked = true;
            }
            else
            {
                _createTableCheckBox.Checked = _createTriggerCheckBox.Checked = _createMVCheckBox.Checked = _createViewCheckBox.Checked = false;
                _createMVGroupCheckBox.Checked = _createProcedureCheckBox.Checked = _createSynonymCheckBox.Checked = _createLibraryCheckBox.Checked = false;
            }
            UpdateControls();
        }

        private void _alterCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (_alterCheckBox.Checked)
            {
                _alterTableCheckBox.Checked = _alterTriggerCheckBox.Checked = _alterMVCheckBox.Checked = _alterViewCheckBox.Checked = true;
                _alterMVGroupCheckBox.Checked = _alterSynonymCheckBox.Checked = _alterLibraryCheckBox.Checked = true;
            }
            else
            {
                _alterTableCheckBox.Checked = _alterTriggerCheckBox.Checked = _alterMVCheckBox.Checked = _alterViewCheckBox.Checked = false;
                _alterMVGroupCheckBox.Checked = _alterSynonymCheckBox.Checked = _alterLibraryCheckBox.Checked = false;
            }
            UpdateControls();
        }

        private void _dropCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (_dropCheckBox.Checked)
            {
                _dropTableCheckBox.Checked = _dropTriggerCheckBox.Checked = _dropMVCheckBox.Checked = _dropViewCheckBox.Checked = true;
                _dropMVGroupCheckBox.Checked = _dropProcedureCheckBox.Checked = _dropSynonymCheckBox.Checked = _dropLibraryCheckBox.Checked = true;
            }
            else
            {
                _dropTableCheckBox.Checked = _dropTriggerCheckBox.Checked = _dropMVCheckBox.Checked = _dropViewCheckBox.Checked = false;
                _dropMVGroupCheckBox.Checked = _dropProcedureCheckBox.Checked = _dropSynonymCheckBox.Checked = _dropLibraryCheckBox.Checked = false;

            }
            UpdateControls();
        }


        private void _allUTILITYCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            _replicateCheckBox.Checked = _allUTILITYCheckBox.Checked;
            UpdateControls();
        }


        #region IPrivilegeOptionsProvider Members

        public void Reset()
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                _allCheckBox.Checked = true;
                UpdateSelections();
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        public string GetSQLPrivilegeClause()
        {
            List<string> selectedPrivileges = new List<string>();

            if (_allCheckBox.Checked)
            {
                selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALL));
            }
            else
            {
                if (_allDMLCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALL_DML));
                }
                else
                {
                    if(_selectCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_SELECT));
                    if (_insertCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_INSERT));
                    if (_updateCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_UPDATE));
                    if (_deleteCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DELETE));
                    if (_executeCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_EXECUTE));
                    if (_referencesCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_REFERENCE));
                    if (_usageCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_USAGE));
                }
                if (_allDDLCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALL_DDL));
                }
                else
                {
                    if (_createCheckBox.Checked)
                    {
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE));
                    }
                    else
                    {
                        if (_createTriggerCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_TRIGGER));
                        if (_createSynonymCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_SYNONYM));
                        if (_createProcedureCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_PROCEDURE));
                        if (_createViewCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_VIEW));
                        if (_createMVGroupCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_MV_GROUP));
                        if (_createMVCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_MV));
                        if (_createTableCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_TABLE));
                        if (this.serverVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                        {
                            if (_createLibraryCheckBox.Checked)
                                selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_CREATE_LIBRARY));
                        }

                    }
                    if (_dropCheckBox.Checked)
                    {
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP));
                    }
                    else
                    {
                        if (_dropTriggerCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_TRIGGER));
                        if (_dropSynonymCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_SYNONYM));
                        if (_dropProcedureCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_PROCEDURE));
                        if (_dropViewCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_VIEW));
                        if (_dropMVGroupCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_MV_GROUP));
                        if (_dropMVCheckBox.Checked)
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_MV));
                        if (_dropTableCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_TABLE));
                        if (this.serverVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                        {
                            if (_dropLibraryCheckBox.Checked)
                                selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DROP_LIBRARY));
                        }
                    }
                    if (_alterCheckBox.Checked)
                    {
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER));
                    }
                    else
                    {
                        if (_alterTriggerCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_TRIGGER));
                        if (_alterSynonymCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_SYNONYM));
                        if (_alterViewCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_VIEW));
                        if (_alterMVGroupCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_MV_GROUP));
                        if (_alterMVCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_MV));
                        if (_alterTableCheckBox.Checked)
                            selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_TABLE));
                        if (this.serverVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                        {
                            if (_alterLibraryCheckBox.Checked)
                                selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALTER_LIBRARY));
                        }
                    }
                }


                if (_allUTILITYCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALL_UTILITY));
                }
                else
                {
                    if (_replicateCheckBox.Checked)
                    {
                        selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_REPLICATE));
                    }
                }
            }

            return string.Join(", ", selectedPrivileges.ToArray());
        }

        public void UpdateSelections()
        {
            UpdateControls();
        }

        public string IsValid()
        {
            if (string.IsNullOrEmpty(GetSQLPrivilegeClause()))
                return Properties.Resources.NoSchemaPrivilegeSelectedMessage;

            return "";
        }

        #endregion  IPrivilegeOptionsProvider Members

    }
}
