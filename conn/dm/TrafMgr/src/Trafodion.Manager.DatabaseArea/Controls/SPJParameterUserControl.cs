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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// User control to allow editing procedure parameters
    /// </summary>
    public partial class SPJParameterUserControl : UserControl
    {
        #region Private member variables

        private TrafodionProcedureColumn _sqlMxProcedureColumn;
        private DataTypePanel _dataTypePanel;
        private Form _parentForm;

        #endregion Private member variables

        /// <summary>
        /// Constructs the paramter control for the given procedure column
        /// </summary>
        /// <param name="aTrafodionProcedureColumn"></param>
        public SPJParameterUserControl(TrafodionProcedureColumn aTrafodionProcedureColumn, Form aParentForm)
        {
            InitializeComponent();
            _sqlMxProcedureColumn = aTrafodionProcedureColumn;
            _nameTextBox.MaxLength = TrafodionName.MaximumNameLength;
            _parentForm = aParentForm;
            
            setupComponents();
            
        }

        /// <summary>
        /// Initialize the UI controls with information from the procedure column model.
        /// </summary>
        private void setupComponents()
        {
            _nameTextBox.Text = _sqlMxProcedureColumn.ExternalName;
            if (_sqlMxProcedureColumn.FormattedDirection().Equals(TrafodionProcedureColumn.DirectionIn, StringComparison.CurrentCultureIgnoreCase))
            {
                _inRadioButton.Checked = true;
                _outRadioButton.Enabled = false;
                _inOutRadioButton.Enabled = false;
            }
            else
            {
                _inRadioButton.Enabled = false;
                _outRadioButton.Enabled = true;
                _inOutRadioButton.Enabled = true;
                _outRadioButton.Checked = true;
            }
            _javaTypeTextBox.Text = _sqlMxProcedureColumn.JavaDataType;

            string javaType = _sqlMxProcedureColumn.JavaDataType;
            //base java type is java type with square brackets removed. for example array types.
            string baseJavaType = DataTypeHelper.GetBaseJavaType(javaType);

            //Based on the java type, populate the SQL datatype combo box with valid entries
            //Also instantiate the right datatype panel based on the java type.
            //Note some datatypes don't need a type panel since there are no extra attributes to edit.
            switch (baseJavaType)
            {
                case DataTypeHelper.JAVA_STRING:
                    {
                        _sqlDataTypeComboBox.Items.AddRange(new string[] {
                           DataTypeHelper.SQL_VARCHAR, DataTypeHelper.SQL_CHARACTER, DataTypeHelper.SQL_NCHAR});
                        setDataTypePanel(new CharDataTypePanel(_sqlMxProcedureColumn, this));
                        break;
                    }
                case DataTypeHelper.JAVA_DATE:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_DATE);
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        _dataTypePanel = null;
                        break;
                    }
                case DataTypeHelper.JAVA_TIME:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_TIME);
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        setDataTypePanel(new TimeDataTypePanel(_sqlMxProcedureColumn, this));
                        break;
                    }
                case DataTypeHelper.JAVA_TIMESTAMP:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_TIMESTAMP);                                                
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        setDataTypePanel(new TimestampDataTypePanel(_sqlMxProcedureColumn, this));
                        break;
                    }
                case DataTypeHelper.JAVA_BIGDECIMAL:
                    {
                        _sqlDataTypeComboBox.Items.AddRange(new string[] {
                            DataTypeHelper.SQL_SIGNED_NUMERIC, 
                            DataTypeHelper.SQL_SIGNED_DECIMAL});
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        setDataTypePanel(new DecimalDataTypePanel(_sqlMxProcedureColumn, this));
                        break;
                    }
                case DataTypeHelper.JAVA_BIGINTEGER:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_SIGNED_NUMERIC);
                        setDataTypePanel(new DecimalDataTypePanel(_sqlMxProcedureColumn, this));
                        break;
                    }                
                case DataTypeHelper.JAVA_PRIMITIVE_DOUBLE:
                case DataTypeHelper.JAVA_DOUBLE:
                    {
                        _sqlDataTypeComboBox.Items.AddRange(new string[] {
                            DataTypeHelper.SQL_DOUBLE, 
                            DataTypeHelper.SQL_FLOAT});
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        _dataTypePanel = null;
                        break;
                    }
                case DataTypeHelper.JAVA_PRIMITIVE_FLOAT:
                case DataTypeHelper.JAVA_FLOAT:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_REAL);
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        _dataTypePanel = null;
                        break;
                    }
                case DataTypeHelper.JAVA_PRIMITIVE_INT:
                case DataTypeHelper.JAVA_INTEGER:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_SIGNED_INTEGER);
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        _dataTypePanel = null;
                        break;
                    }
                case DataTypeHelper.JAVA_PRIMITIVE_LONG:
                case DataTypeHelper.JAVA_LONG:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_SIGNED_LARGEINT);
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        _dataTypePanel = null;
                        break;
                    }
                case DataTypeHelper.JAVA_PRIMITIVE_SHORT:
                    {
                        _sqlDataTypeComboBox.Items.Add(DataTypeHelper.SQL_SIGNED_SMALLINT);
                        _sqlDataTypeComboBox.SelectedIndex = 0;
                        _dataTypePanel = null;
                        break;
                    }
                //We know that only java.math.BigDecimal, Date, Time and Timestamp can be extended. Rest all the classes are final classes and cannot be extended.
                //hence for an unknown type, the user has to only choose from the Numeric and decimal types
                case DataTypeHelper.SQL_UNKNOWN_TYPE:
                    {
                        break;
                    }
            }
            if (_sqlDataTypeComboBox.Items.Count > 0)
            {
                if (String.IsNullOrEmpty(_sqlMxProcedureColumn.TheSQLDataType) || !_sqlDataTypeComboBox.Items.Contains(_sqlMxProcedureColumn.TheSQLDataType))
                {
                    _sqlDataTypeComboBox.SelectedIndex = 0;
                }
                else
                {
                    _sqlDataTypeComboBox.SelectedItem = _sqlMxProcedureColumn.TheSQLDataType;
                }
            }

            if (_nameTextBox.CanFocus)
            {
                _nameTextBox.Focus();
            }
            _nameTextBox.Select();
        }

        /// <summary>
        /// Reset the controls based on the SQL dataype selected. 
        /// For example, the datatype attributes panel has to reset based on the sql type.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _sqlDataTypeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            _sqlMxProcedureColumn.TheSQLDataType = _sqlDataTypeComboBox.SelectedItem as string;

            if (_dataTypePanel != null)
            {
                _dataTypePanel.SetupComponents();
            }

            // For double, need to change the data type panel
            if (_sqlMxProcedureColumn.TheSQLDataType == DataTypeHelper.SQL_FLOAT)
            {
                _dataTypePanel = new FloatDataTypePanel(_sqlMxProcedureColumn);
            }
            if (_sqlMxProcedureColumn.TheSQLDataType == DataTypeHelper.SQL_DOUBLE)
            {
                _dataTypePanel = null;
            }
        }

        /// <summary>
        /// Initialize the datatype panel
        /// </summary>
        /// <param name="aDataTypePanel"></param>
        private void setDataTypePanel(DataTypePanel aDataTypePanel)
        {
            _dataTypePanel = aDataTypePanel;
            _dataTypePanelContainer.Controls.Clear();
            _dataTypePanelContainer.Controls.Add(_dataTypePanel);
        }

        /// <summary>
        /// Update the procedure column model with the information from the user control i.e. updates made by the user
        /// </summary>
        public void UpdateModel()
        {
            _sqlMxProcedureColumn.ExternalName = _nameTextBox.Text;
            if (_dataTypePanel != null)
            {
                _dataTypePanel.UpdateModel();
            }
        }

        /// <summary>
        /// If any invalid values are specified in the controls, 
        /// sets an error message and disables the ok button in the parent dialog
        /// </summary>
        /// <param name="anErrorMsg"></param>
        public void setErrorMsg(string anErrorMsg)
        {
            _errorText.Text = anErrorMsg;
            if (_parentForm != null && _parentForm is EditParameterDialog)
            {
                EditParameterDialog theCreateSPJControl = (EditParameterDialog)_parentForm;
                if (theCreateSPJControl != null)
                {
                    theCreateSPJControl.disableCreate(anErrorMsg.Length > 0);
                }
            }
        }

        private void _inRadioButton_Click(object sender, EventArgs e)
        {
            _sqlMxProcedureColumn.TheDirection = "I";
        }

        private void _inOutRadioButton_Click(object sender, EventArgs e)
        {
            _sqlMxProcedureColumn.TheDirection = "N";
        }

        private void _outRadioButton_Click(object sender, EventArgs e)
        {
            _sqlMxProcedureColumn.TheDirection = "O";
        }

        /// <summary>
        /// Valid the name as the user is typing it. If name is invalid, an error message
        /// is displayed in the error text label.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _nameTextBox_TextChanged(object sender, EventArgs e)
        {
            string name = _nameTextBox.Text.Trim();

            System.Text.StringBuilder errorStringHolder = new System.Text.StringBuilder();
            if (!TrafodionName.Validate(name, errorStringHolder))
            {
                string error = errorStringHolder.ToString();
                if (String.IsNullOrEmpty(error))
                {
                    error = " ";
                }
                setErrorMsg(error);
            }
            else
            {
                //Name is valid but make sure it is not used by another column.
                bool nameAlreadyExists = false;
                foreach (TrafodionColumn column in _sqlMxProcedureColumn.TrafodionProcedure.Columns)
                {
                    if(column != _sqlMxProcedureColumn && name.Equals(column.ExternalName))
                    {
                        nameAlreadyExists = true;
                        break;
                    }
                }
                if (nameAlreadyExists)
                {
                    setErrorMsg(Properties.Resources.ParameterNameAlreadyInUse);
                }
                else
                {
                    setErrorMsg("");
                }
            }
        }
    }
}
