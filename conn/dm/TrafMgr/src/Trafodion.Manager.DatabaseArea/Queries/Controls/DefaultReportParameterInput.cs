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
    public partial class DefaultReportParameterInput : UserControl, ReportParameterInput
    {
        public event EventHandler InputChanged;

        public DefaultReportParameterInput()
        {
            InitializeComponent();
        }

        object ReportParameterInput.getParamValue(ReportParameter parameter) 
        {
            return paramCombo.Text;
        }

        void ReportParameterInput.setParameter(ReportParameter parameter) 
        {
            populateFromParam(parameter);
            paramGroup.Text = parameter.DisplayName;
            paramToolTip.SetToolTip(paramCombo, parameter.Description);
        }

        void ReportParameterInput.setParameters(List<ReportParameter> parameters) 
        {
            
            if ((parameters != null)  && (parameters.Count == 1))
            {
                ReportParameter tempReportParameter = parameters[0] as ReportParameter;
                if (tempReportParameter != null)
                {
                    ((ReportParameterInput)this).setParameter((ReportParameter)tempReportParameter);
                }
            }
        }

        //Enable or disable this control
        public void enable(bool enable)
        {
            paramCombo.Enabled = enable;
        }


        //populate the timeCombo using the parameter passed.
        private void populateFromParam(ReportParameter parameter)
        {
            if (parameter != null)
            {
                paramCombo.Items.Clear();
                if (parameter.Value != null)
                {
                    paramCombo.Items.Add(parameter.Value);
                }
                if (parameter.PossibleValues != null)
                {
                    foreach (object tempObj in parameter.PossibleValues)
                    {                    
                        if ((tempObj != null) && (parameter.Value != null) && (parameter.Value.Equals(tempObj)))
                        {
                            //do not add it again
                        }
                        else
                        {
                            if (tempObj is string)
                            {
                                paramCombo.Items.Add(((string)tempObj).Trim());
                            }
                            else
                            {
                                paramCombo.Items.Add(tempObj);
                            }
                        }

                    }
                }

                if (parameter.Value != null)
                {
                    paramCombo.SelectedItem = parameter.Value;
                }
            }
        }

        protected virtual void OnInputChanged(InputEventArgs e)
        {
            if (InputChanged != null)
            {
                InputChanged(this, e);
            }
        }
        /// <summary>
        /// Raise change event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void paramCombo_TextChanged(object sender, EventArgs e)
        {
            OnInputChanged(new InputEventArgs());
        }
        /// <summary>
        /// Validate user input.
        /// </summary>
        /// <returns></returns>
        bool ReportParameterInput.IsValidInput()
        {
            //return paramCombo.Text.Trim().Length > 0;
            return true;
        }
    }
}
