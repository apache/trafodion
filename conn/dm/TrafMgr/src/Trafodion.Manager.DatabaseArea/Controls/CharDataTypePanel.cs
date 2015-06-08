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
using Trafodion.Manager.DatabaseArea.Model;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class CharDataTypePanel : DataTypePanel
    {
        private bool _isNCharMode = false;
        private SPJParameterUserControl _spjParameterUserControl;

        public CharDataTypePanel(TrafodionColumn aTrafodionColumn, SPJParameterUserControl aParentControl)
            :base(aTrafodionColumn)
        {
            InitializeComponent();

            lengthNumericUpDown.Minimum = DataTypeHelper.MINIMUM_CHARACTER_LENGTH;
            lengthNumericUpDown.Maximum = DataTypeHelper.MAXIMUM_CHARACTER_LENGTH;
            _spjParameterUserControl = aParentControl;

            SetupComponents();
        }

        public override void SetupComponents()
        {
            _isNCharMode = TrafodionColumn.TheSQLDataType.Equals(DataTypeHelper.SQL_NCHAR, StringComparison.CurrentCultureIgnoreCase) ? true : false;
            if (_isNCharMode)
            {
                TrafodionColumn.TheCharacterSet = DataTypeHelper.CHARSET_UCS2;
            }

            charsetComboBox.Items.Clear();
            foreach (string charsetName in DataTypeHelper.SupportedCharSets)
            {
                charsetComboBox.Items.Add(charsetName);
            }
            charsetComboBox.Visible = !_isNCharMode;

            //Populate controls with values from Model
            lengthNumericUpDown.Value = TrafodionColumn.TheColumnSize;
            upshiftCheckBox.Checked = TrafodionColumn.IsUpshifted;

            varyingCheckBox.Checked = TrafodionColumn.TheSQLDataType.Equals(DataTypeHelper.SQL_VARCHAR, StringComparison.CurrentCultureIgnoreCase);
            //Don't display the varying checkbox for VARCHAR
            varyingCheckBox.Visible = !TrafodionColumn.TheSQLDataType.Equals(DataTypeHelper.SQL_VARCHAR, StringComparison.CurrentCultureIgnoreCase);

            if (!String.IsNullOrEmpty(TrafodionColumn.TheCharacterSet) && charsetComboBox.Items.Contains(TrafodionColumn.TheCharacterSet))
            {
                charsetComboBox.SelectedItem = TrafodionColumn.TheCharacterSet;
            }
            else
            {
                charsetComboBox.SelectedIndex = 0;
            }
        }

        public override void UpdateModel()
        {
            TrafodionColumn.TheSQLDataType = (varyingCheckBox.Checked ? DataTypeHelper.SQL_VARCHAR : DataTypeHelper.SQL_CHARACTER);
            TrafodionColumn.IsUpshifted = upshiftCheckBox.Checked;
            TrafodionColumn.TheCharacterSet = charsetComboBox.SelectedItem as string;
            TrafodionColumn.TheColumnSize = (int)lengthNumericUpDown.Value;
        }

        private void lengthNumericUpDown_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            if ( (lengthNumericUpDown.Value > 32708) || (lengthNumericUpDown.Value < 1))
            {
                _spjParameterUserControl.setErrorMsg(Properties.Resources.LengthError);
            }
            else
            {
                _spjParameterUserControl.setErrorMsg("");
            }
        }
    }
}
