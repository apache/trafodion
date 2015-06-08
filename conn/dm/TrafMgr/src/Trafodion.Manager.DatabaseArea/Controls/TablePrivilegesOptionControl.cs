//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Collections.Generic;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// Table privileges options for Grant/Revoke tool
    /// </summary>
    public partial class TablePrivilegesOptionControl : UserControl, IPrivilegeOptionsProvider
    {
        #region private member variables

        private GrantRevokeControl _grantRevokeTool;
        private ColumnPrivilegesOptionControl _colSelectPrivControl;
        private ColumnPrivilegesOptionControl _colInsertPrivControl;
        private ColumnPrivilegesOptionControl _colUpdatePrivControl;
        private ColumnPrivilegesOptionControl _colReferencePrivControl;

        private readonly ConnectionDefinition.SERVER_VERSION serverVersion;

        #endregion private member variables

        #region public methods

        /// <summary>
        /// Default constructor
        /// </summary>
        public TablePrivilegesOptionControl()
        {
            InitializeComponent();
            AddColumnOptionPanels();
            TrafodionToolTip1.IsBalloon = false;
        }

        /// <summary>
        /// Constructor with grant/revoke tool
        /// </summary>
        /// <param name="grantRevokeTool"></param>
        public TablePrivilegesOptionControl(GrantRevokeControl grantRevokeTool, ConnectionDefinition.SERVER_VERSION serverVersion)
        : this()
        {
            _grantRevokeTool = grantRevokeTool;
            this.serverVersion = serverVersion;
            Reset();
        }

        #endregion public methods

        #region private methods

        /// <summary>
        /// Add all of the column options
        /// </summary>
        private void AddColumnOptionPanels()
        {
            _colSelectPrivControl = new ColumnPrivilegesOptionControl(ColumnPrivilegesOptionControl.ColumnPriv_Select);
            _colSelectPrivControl.Dock = DockStyle.Fill;
            _colPrivTableLayoutPanel.Controls.Add(_colSelectPrivControl);
            _colInsertPrivControl = new ColumnPrivilegesOptionControl(ColumnPrivilegesOptionControl.ColumnPriv_Insert);
            _colInsertPrivControl.Dock = DockStyle.Fill;
            _colPrivTableLayoutPanel.Controls.Add(_colInsertPrivControl);
            _colUpdatePrivControl = new ColumnPrivilegesOptionControl(ColumnPrivilegesOptionControl.ColumnPriv_Update);
            _colUpdatePrivControl.Dock = DockStyle.Fill;
            _colPrivTableLayoutPanel.Controls.Add(_colUpdatePrivControl);
            _colReferencePrivControl = new ColumnPrivilegesOptionControl(ColumnPrivilegesOptionControl.ColumnPriv_Reference);
            _colReferencePrivControl.Dock = DockStyle.Fill;
            _colPrivTableLayoutPanel.Controls.Add(_colReferencePrivControl);
        }
        
        /// <summary>
        /// Update all controls
        /// </summary>
        private void UpdateControls()
        {
            if (_allCheckBox.Checked)
            {
                _objectLevelPrivilegesGroupBox.Enabled = _columnLevelPrivilegesGroupBox.Enabled = false;
                _colSelectPrivControl.Enabled = _colInsertPrivControl.Enabled = _colUpdatePrivControl.Enabled = _colReferencePrivControl.Enabled = false;
                _colSelectPrivControl.AllColRadioButtonChecked = _colInsertPrivControl.AllColRadioButtonChecked =
                    _colUpdatePrivControl.AllColRadioButtonChecked = _colReferencePrivControl.AllColRadioButtonChecked = true;
            }
            else
            {
                _objectLevelPrivilegesGroupBox.Enabled = _columnLevelPrivilegesGroupBox.Enabled = true;
            }

            if (_grantRevokeTool != null)
            {
                //If more than one object selected, then we cannot allow selection of columns for column privileges
                //default to no columns
                if (_grantRevokeTool.CurrentSelectedObjects.Count > 1 && _grantRevokeTool.Action == TrafodionObject.PrivilegeAction.GRANT)
                {
                    _columnLevelPrivilegesGroupBox.Enabled = false;
                    _colSelectPrivControl.Enabled = _colInsertPrivControl.Enabled = _colUpdatePrivControl.Enabled = _colReferencePrivControl.Enabled = false;
                }

                bool isNonTableSelected = false;    // Indicate if there's any selected object which is not a table
                bool isMVSelected = false;
                foreach(TrafodionObject selectedObject in _grantRevokeTool.CurrentSelectedObjects)
                {
                    if (!(selectedObject is TrafodionTable))                // If selected object is not a Table
                    {
                        isNonTableSelected = true;

                        if (selectedObject is TrafodionMaterializedView)    // If selected object is a Materialized View
                        {
                            isMVSelected = true;
                            break;
                        }
                    }
                }


                /*
                 * Only when selected object is a Table, enable the Replicate check box.
                */
                if (isNonTableSelected) // If selected object is not a Table
                {
                    _replicateCheckBox.Checked = false;
                    _replicateCheckBox.Enabled = false;
                }
                else                    // If selelcted object is a Table
                {
                    _replicateCheckBox.Enabled = true;
                }

                if (isMVSelected)
                {
                    _columnLevelPrivilegesGroupBox.Visible = false;
                    _colSelectPrivControl.NoColRadioButtonChecked = _colInsertPrivControl.NoColRadioButtonChecked =
                        _colUpdatePrivControl.NoColRadioButtonChecked = _colReferencePrivControl.NoColRadioButtonChecked = true;
                    _insertCheckBox.Enabled = _selectCheckBox.Enabled = _deleteCheckBox.Enabled = true;
                    _updateCheckBox.Enabled = _referenceCheckBox.Enabled = false;
                    _updateCheckBox.Checked = _referenceCheckBox.Checked = false;
                }
                else // so this must be the table or view
                {
                    _columnLevelPrivilegesGroupBox.Visible = true;
                    _colSelectPrivControl.Enabled = _colInsertPrivControl.Enabled = _colUpdatePrivControl.Enabled = _colReferencePrivControl.Enabled = true;
                    _insertCheckBox.Enabled = _selectCheckBox.Enabled = _updateCheckBox.Enabled = _referenceCheckBox.Enabled = true;
                    _deleteCheckBox.Enabled = true;
                }

                if (_selectCheckBox.Checked)
                {
                    _colSelectPrivControl.Enabled = false;
                }

                if (_insertCheckBox.Checked)
                {
                    _colInsertPrivControl.Enabled = false;
                }

                if (_updateCheckBox.Checked)
                {
                    _colUpdatePrivControl.Enabled = false;
                }

                if (_referenceCheckBox.Checked)
                {
                    _colReferencePrivControl.Enabled = false;
                }



                /*
                * Compatibility for M6 & M7-
                * For M6, those new features should be hidden and disabled
                * For M7, the statement below could be removed
                */
                if (this.serverVersion < ConnectionDefinition.SERVER_VERSION.SQ120)
                {
                    _replicateCheckBox.Visible = false;
                    _replicateCheckBox.Checked = false;
                }
            }
        }

        /// <summary>
        /// The event handler for All CheckBox check changed events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _allCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            _selectCheckBox.Checked = _insertCheckBox.Checked = _updateCheckBox.Checked = _referenceCheckBox.Checked = _deleteCheckBox.Checked = _replicateCheckBox.Checked = _allCheckBox.Checked;
            UpdateControls();
        }

        /// <summary>
        /// Select CheckBox check change events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _selectCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            _colSelectPrivControl.Enabled = !_selectCheckBox.Checked;
            if (_selectCheckBox.Checked)
            {
                _colSelectPrivControl.AllColRadioButtonChecked = true;
            }
            else
            {
                _colSelectPrivControl.NoColRadioButtonChecked = true;
            }
        }

        /// <summary>
        /// Insert CheckBox check change events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _insertCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            _colInsertPrivControl.Enabled = !_insertCheckBox.Checked;
            if (_insertCheckBox.Checked)
            {
                _colInsertPrivControl.AllColRadioButtonChecked = true;
            }
            else
            {
                _colInsertPrivControl.NoColRadioButtonChecked = true;
            }
        }

        /// <summary>
        /// Update CheckBox check change events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _updateCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            _colUpdatePrivControl.Enabled = !_updateCheckBox.Checked;
            if (_updateCheckBox.Checked)
            {
                _colUpdatePrivControl.AllColRadioButtonChecked = true;
            }
            else
            {
                _colUpdatePrivControl.NoColRadioButtonChecked = true;
            }
        }

        /// <summary>
        /// Reference CheckBox check change events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _referenceCheckBox_CheckedChanged(object sender, System.EventArgs e)
        {
            _colReferencePrivControl.Enabled = !_referenceCheckBox.Checked;
            if (_referenceCheckBox.Checked)
            {
                _colReferencePrivControl.AllColRadioButtonChecked = true;
            }
            else
            {
                _colReferencePrivControl.NoColRadioButtonChecked = true;
            }
        }

        #endregion private methods

        #region IPrivilegeOptionsProvider Members

        /// <summary>
        /// Reset the control
        /// </summary>
        public void Reset()
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                UpdateSelections();
                _allCheckBox.Checked = true;
            }
            finally
            {
                Cursor = Cursors.Default;
            }
        }

        /// <summary>
        /// Returns the SQL privilege clauses for the Grant/Revoke commands
        /// </summary>
        /// <returns></returns>
        public string GetSQLPrivilegeClause()
        {
            List<string> selectedPrivileges = new List<string>();
            if (_allCheckBox.Checked)
            {
                selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_ALL));
            }
            else
            {
                // The delete priv option
                if (_deleteCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_DELETE));
                }

                // The REPLICATE privilege option
                if (_replicateCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_REPLICATE));
                }

                // The insert priv option
                if (_selectCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_SELECT));
                }
                else
                {
                    string privCommand = _colSelectPrivControl.GetSQLPrivilegeClause();
                    if (!string.IsNullOrEmpty(privCommand))
                    {
                        selectedPrivileges.Add(privCommand);
                    }
                }

                // The insert priv option
                if (_insertCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_INSERT));
                }
                else
                {
                    string privCommand = _colInsertPrivControl.GetSQLPrivilegeClause();
                    if (!string.IsNullOrEmpty(privCommand))
                    {
                        selectedPrivileges.Add(privCommand);
                    }
                }

                // The update priv option
                if (_updateCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_UPDATE));
                }
                else
                {
                    string privCommand = _colUpdatePrivControl.GetSQLPrivilegeClause();
                    if (!string.IsNullOrEmpty(privCommand))
                    {
                        selectedPrivileges.Add(privCommand);
                    }
                }

                // The reference priv option
                if (_referenceCheckBox.Checked)
                {
                    selectedPrivileges.Add(TrafodionPrivilegeTypeHelper.GetSQLPrivilegeClause(TrafodionPrivilegeTypeHelper.TYPE_REFERENCE));
                }
                else
                {
                    string privCommand = _colReferencePrivControl.GetSQLPrivilegeClause();
                    if (!string.IsNullOrEmpty(privCommand))
                    {
                        selectedPrivileges.Add(privCommand);
                    }
                }
            }

            return string.Join(", ", selectedPrivileges.ToArray());
        }

        /// <summary>
        /// Update the column lists
        /// </summary>
        public void UpdateSelections()
        {
            _colSelectPrivControl.ColumnList = null;
            _colInsertPrivControl.ColumnList = null;
            _colUpdatePrivControl.ColumnList = null;
            _colReferencePrivControl.ColumnList = null;

            if (_grantRevokeTool.CurrentSelectedObjects.Count == 1)
            {
                TrafodionColumn[] columnsList = null;
                if(_grantRevokeTool.CurrentSelectedObjects[0] is TrafodionTable)
                {
                    columnsList = ((TrafodionTable)_grantRevokeTool.CurrentSelectedObjects[0]).Columns.ToArray();
                } else if (_grantRevokeTool.CurrentSelectedObjects[0] is TrafodionView)
                {
                    columnsList = ((TrafodionView)_grantRevokeTool.CurrentSelectedObjects[0]).Columns.ToArray();
                }
                _colSelectPrivControl.ColumnList = columnsList;
                _colInsertPrivControl.ColumnList = columnsList;
                _colUpdatePrivControl.ColumnList = columnsList;
                _colReferencePrivControl.ColumnList = columnsList;
            }
            UpdateControls();
        }

        /// <summary>
        /// Is the option selection valid?
        /// </summary>
        /// <returns></returns>
        public string IsValid()
        {
            string message = "";
            if (!_selectCheckBox.Checked)
            {
                message = _colSelectPrivControl.IsValid();
                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }
            }

            if (!_insertCheckBox.Checked)
            {
                message = _colInsertPrivControl.IsValid();
                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }
            }

            if (!_updateCheckBox.Checked)
            {
                message = _colUpdatePrivControl.IsValid();
                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }
            }

            if (!_referenceCheckBox.Checked)
            {
                message = _colReferencePrivControl.IsValid();
                if (!string.IsNullOrEmpty(message))
                {
                    return message;
                }
            }

            if(string.IsNullOrEmpty(GetSQLPrivilegeClause()))
                return Properties.Resources.NoObjectPrivilegesSelectedMessage;

            return "";
        }

        #endregion IPrivilegeOptionsProvider Members
    }
}
