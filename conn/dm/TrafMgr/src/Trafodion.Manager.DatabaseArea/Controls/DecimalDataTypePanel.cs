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
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class DecimalDataTypePanel : DataTypePanel
    {
        // Precision
        private const int THE_DEFAULT_PRECISION = 9;
        private const int THE_MINIMUM_PRECISION = 1;
        private const int THE_MAXIMUM_PRECISION_DECIMAL = 18;
        private const int THE_MAXIMUM_PRECISION_NUMERIC = 128;
        
        // Scale
        private const int THE_DEFAULT_SCALE = 0;
        private const int THE_MINIMUM_SCALE = 0;
        private const int THE_MAXIMUM_SCALE = 18;

        private bool isBigInteger = false;
        private const int THE_DEFAULT_PRECISION_BIGINT = 18;
        private const int THE_MINIMUM_PRECISION_BIGINT = 18;
        private const int THE_MAXIMUM_PRECISION_BIGINT = 128;

        SPJParameterUserControl _theSPJParameterUserControl;

        public DecimalDataTypePanel(TrafodionColumn aTrafodionColumn, SPJParameterUserControl aSPJParameterUserControl)
            :base(aTrafodionColumn)
        {
            _theSPJParameterUserControl = aSPJParameterUserControl;
            InitializeComponent();

            SetupComponents();
        }

        public override void SetupComponents()
        {
            if (TrafodionColumn is TrafodionProcedureColumn)
            {
                isBigInteger = DataTypeHelper.GetBaseJavaType(
                    ((TrafodionProcedureColumn)TrafodionColumn).JavaDataType).Equals(DataTypeHelper.JAVA_BIGINTEGER);
            }

            // Set the min/max values depending on the type
            if (isBigInteger)
            {
                _precisionUpDown.Minimum = THE_MINIMUM_PRECISION_BIGINT;
                _precisionUpDown.Maximum = THE_MAXIMUM_PRECISION_BIGINT;
            }
            else
            {
                _precisionUpDown.Minimum = THE_MINIMUM_PRECISION;
                _scaleNumericUpDown.Minimum = THE_MINIMUM_SCALE;

                if (TrafodionColumn.TheSQLDataType == DataTypeHelper.SQL_SIGNED_DECIMAL)
                {
                    _precisionUpDown.Maximum = THE_MAXIMUM_PRECISION_DECIMAL;
                }
                else if (TrafodionColumn.TheSQLDataType == DataTypeHelper.SQL_SIGNED_NUMERIC)
                {
                    _precisionUpDown.Maximum = THE_MAXIMUM_PRECISION_NUMERIC;
                }
            }

            // Set the value of the controls
            if (!TrafodionColumn.IsDefaultValues)
            {
                _precisionUpDown.Value = TrafodionColumn.TheColPrecision;
                _scaleNumericUpDown.Value = TrafodionColumn.TheColScale;
            }
            else
            {
                if (isBigInteger)
                {
                    _precisionUpDown.Value = THE_DEFAULT_PRECISION_BIGINT;
                    _scaleNumericUpDown.Value = THE_DEFAULT_SCALE;
                }
                else
                {
                    _precisionUpDown.Value = THE_DEFAULT_PRECISION;
                    _scaleNumericUpDown.Value = THE_DEFAULT_SCALE;
                }
            }            
        }

        public override void UpdateModel()
        {
            TrafodionColumn.TheColPrecision = (int)_precisionUpDown.Value;
            TrafodionColumn.TheColScale = (int)_scaleNumericUpDown.Value;
            TrafodionColumn.IsDefaultValues = false;
        }

        private void ValidateControls()
        {
            if (_scaleNumericUpDown.Value > _precisionUpDown.Value)
            {
                _theSPJParameterUserControl.setErrorMsg(Properties.Resources.ScaleCannotExceedPrecision);
            }
            else
            {
                _theSPJParameterUserControl.setErrorMsg("");
            }
        }

        private void _scaleNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            ValidateControls();
        }

        private void _precisionUpDown_ValueChanged(object sender, EventArgs e)
        {
            ValidateControls();
        }

        private void _scaleNumericUpDown_KeyUp(object sender, KeyEventArgs e)
        {
            ValidateControls();
        }

        private void _precisionUpDown_KeyUp(object sender, KeyEventArgs e)
        {
            ValidateControls();
        }


    }
}
