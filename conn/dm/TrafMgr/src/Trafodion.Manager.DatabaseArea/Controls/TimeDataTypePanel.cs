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
    public partial class TimeDataTypePanel : DataTypePanel
    {

        private const int THE_DEFAULT_PRECISION = 0; 
        private const int THE_MINIMUM_PRECISION = 0;
        private const int THE_MAXIMUM_PRECISION = 6;
        private SPJParameterUserControl _parentControl;

        public TimeDataTypePanel(TrafodionColumn aTrafodionColumn, SPJParameterUserControl aParentControl)
            : base(aTrafodionColumn)
        {
            InitializeComponent();
            _parentControl = aParentControl;
            SetupComponents();
        }

        public override void SetupComponents()
        {
            _precisionNumericUpDown.Minimum = THE_MINIMUM_PRECISION;
            _precisionNumericUpDown.Maximum = THE_MAXIMUM_PRECISION;

            if (!TrafodionColumn.IsDefaultValues)
            {
                _precisionNumericUpDown.Value = TrafodionColumn.TheColPrecision;
            }
            else
            {
                _precisionNumericUpDown.Value = THE_DEFAULT_PRECISION;
            }
        }

        public override void UpdateModel()
        {
            TrafodionColumn.TheDatetimeTrailingPrecision = (int)_precisionNumericUpDown.Value;
            TrafodionColumn.TheColPrecision = (int)_precisionNumericUpDown.Value;
            TrafodionColumn.IsDefaultValues = false;
        }

        private void _precisionNumericUpDown_KeyUp(object sender, System.Windows.Forms.KeyEventArgs e)
        {
            if ((_precisionNumericUpDown.Value > THE_MAXIMUM_PRECISION) || (_precisionNumericUpDown.Value < THE_MINIMUM_PRECISION))
            {
                _parentControl.setErrorMsg(string.Format(Properties.Resources.PrecisionError, new string[] { THE_MINIMUM_PRECISION.ToString(), THE_MAXIMUM_PRECISION.ToString() }));
            }
            else
            {
                _parentControl.setErrorMsg("");
            }
        }
    }
}
