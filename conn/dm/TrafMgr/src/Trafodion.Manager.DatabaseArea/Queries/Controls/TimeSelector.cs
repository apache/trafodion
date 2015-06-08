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
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class TimeSelector : UserControl, ReportParameterInput
    {
        public const string TimePattern = "yyyy-MM-dd HH:mm:ss";
//        public static readonly string TimePattern = Trafodion.Manager.Framework.Utilities.DateTimeFormatString;

        public event EventHandler ManualChangeHandler;

        public event EventHandler InputChanged;

        ReportParameter theParam = null;

        public TimeSelector()
        {
            InitializeComponent();
        }

        private void timeCombo_TextChanged(object sender, EventArgs e)
        {
            validateControls();
            OnInputChanged(new InputEventArgs());
        }

        private void btnTimeSelector_Click(object sender, EventArgs e)
        {

        }

        private void addDay_Click(object sender, EventArgs e)
        {
            try
            {
                DateTime dt = getDateTime();
                dt = dt.AddDays(1);
                timeCombo.Text = dt.ToString(TimePattern);
            }
            catch (FormatException ex)
            {
            }
            ReportManualChangeEvent();
        }

        private void minusDay_Click(object sender, EventArgs e)
        {
            try
            {
                DateTime dt = getDateTime();
                dt = dt.AddDays(-1);
                timeCombo.Text = dt.ToString(TimePattern);
            }
            catch (FormatException ex)
            {
            }
            ReportManualChangeEvent();
        }

        private void addHour_Click(object sender, EventArgs e)
        {
            try
            {
                DateTime dt = getDateTime();
                dt = dt.AddHours(1);
                timeCombo.Text =dt.ToString(TimePattern) ;
            }
            catch (FormatException ex)
            {
            }
            ReportManualChangeEvent();
        }

        private void minusHour_Click(object sender, EventArgs e)
        {
            try
            {
                DateTime dt = getDateTime();
                dt = dt.AddHours(-1);
                timeCombo.Text = dt.ToString(TimePattern) ;
            }
            catch (FormatException ex)
            {
            }
            ReportManualChangeEvent();
        }

        //Checks to see if the time input is valid
        private bool isTimeValid()
        {
            bool ret = false;
            object selectedObj = timeCombo.Text;
            if (selectedObj != null)
            {
                try
                {
                    //DateTime.ParseExact(selectedObj.ToString(), TimePattern, null);
                    DateTime.Parse(selectedObj.ToString());
                    ret = true;
                }
                catch (FormatException ex)
                {
                    ret = false;
                }
            }
            return ret;
        }

        private DateTime getDateTime()
        {
            //return DateTime.ParseExact(timeCombo.Text, TimePattern, null);
            return DateTime.Parse(timeCombo.Text);
        }
        //Enables disables the controls
        private void validateControls()
        {
            if (isTimeValid())
            {
                addDay.Enabled = true;
                minusDay.Enabled = true;
                addHour.Enabled = true;
                minusHour.Enabled = true;
            }
            else
            {
                addDay.Enabled = false;
                minusDay.Enabled = false;
                addHour.Enabled = false;
                minusHour.Enabled = false;

            }
        }


        //If the input is valid will return the time as entered by the user
        private string getUserInput()
        {
            string ret = null;
            object selectedObj = timeCombo.Text;
            if (selectedObj != null)
            {
                //currently do nothing
                ret = selectedObj.ToString();
                //try
                //{
                //    DateTime dt = DateTime.ParseExact(selectedObj.ToString(), TimePattern, null);
                //    ret = dt.ToString(TimePattern);
                //}
                //catch (FormatException ex)
                //{
                //    ret = null;
                //}
            }
            return ret;
        }

        //Method to set the time
        public void setTime(DateTime time)
        {
            string formattedTime = time.ToString(TimePattern);
            theParam = (theParam == null) ? new ReportParameter() : theParam;
            theParam.Value = formattedTime;
            populateFromParam(theParam);
        }


        //returns the value of the param as provided by the user
        Object ReportParameterInput.getParamValue(ReportParameter param) 
        {
            return getUserInput();
        }

        //this control can handle only one parameter
        void ReportParameterInput.setParameters(List<ReportParameter> parameters) 
        {
            if ((parameters != null)  && (parameters.Count == 1))
            {
                ReportParameter tempParam = parameters[0] as ReportParameter;
                if (tempParam != null)
                {
                    theParam = tempParam;
                    ((ReportParameterInput)this).setParameter(tempParam);
                }
            }
        }

        void ReportParameterInput.setParameter(ReportParameter param) 
        {
            ReportParameter tempParam = new ReportParameter();
            tempParam.PossibleValues  = new ArrayList();
            if (param != null)
            {
                tempParam.ParamName = param.ParamName;
                tempParam.DisplayName = param.DisplayName;
                tempParam.Description = param.Description;
                tempParam.Value = param.Value;
                if (param.PossibleValues != null)
                {
                    foreach (object paramValue in param.PossibleValues)
                    {
                        tempParam.PossibleValues.Add(paramValue);
                    }
                }
            }
            timeComboToolTip.SetToolTip(timeCombo, tempParam.Description);
            populateFromParam(tempParam);
            validateControls();
        }

        //populate the timeCombo using the param passed.
        private void populateFromParam(ReportParameter param)
        {
            if (param != null)
            {
                timeCombo.Items.Clear();
                if (param.Value != null)
                {
                    if (param.Value is DateTime)
                    {
                        param.Value = ((DateTime)param.Value).ToString(TimePattern);
                    }
                    timeCombo.Items.Add(param.Value);
                }
                if (param.PossibleValues != null)
                {
                    foreach (object paramValue in param.PossibleValues)
                    {
                        if ((paramValue != null) && (param.Value != null) && (param.Value.Equals(paramValue)))
                        {
                            //do not add it again
                        }
                        else
                        {
                            timeCombo.Items.Add(paramValue);
                        }
                    }
                }

                if (param.Value != null)
                {
                    timeCombo.SelectedItem = param.Value;
                }

            }
        }

        private void timeCombo_KeyPress(object sender, KeyPressEventArgs e)
        {
            ReportManualChangeEvent();
        }

        private void ReportManualChangeEvent()
        {
            EventHandler handler = ManualChangeHandler;
            if (handler != null)
            {
                handler(this, new EventArgs());
            }
        }

        protected virtual void OnInputChanged(InputEventArgs e)
        {
            if (InputChanged != null)
            {
                InputChanged(this, e);
            }
        }

        bool ReportParameterInput.IsValidInput()
        {
            return isTimeValid();
        }
    }
}
