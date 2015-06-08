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
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class TimeReportParameterInput: UserControl, ReportParameterInput
    {
        public event EventHandler InputChanged;

        public TimeReportParameterInput() 
        {
            InitializeComponent();
        }

        void ReportParameterInput.setParameter(ReportParameter param)
        {
            ((ReportParameterInput)timeSelector).setParameter(param);
            groupBox.Text =  param.DisplayName;
        }

        void ReportParameterInput.setParameters(List<ReportParameter> parameters)
        {
            ((ReportParameterInput)timeSelector).setParameters(parameters);
        }
        object ReportParameterInput.getParamValue(ReportParameter param)
        {
            return ((ReportParameterInput)timeSelector).getParamValue(param);
        }

        protected virtual void OnInputChanged(InputEventArgs e)
        {
            if (InputChanged != null)
            {
                InputChanged(this, e);
            }
        }
        /// <summary>
        /// Raise event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void timeSelector_InputChanged(object sender, EventArgs e)
        {
            OnInputChanged(new InputEventArgs());
        }
        /// <summary>
        /// Validate user input
        /// </summary>
        /// <returns></returns>
        bool ReportParameterInput.IsValidInput()
        {
            return ((ReportParameterInput)timeSelector).IsValidInput();
        }
    }
}
