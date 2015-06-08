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
using Trafodion.Manager.Framework.Queries;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    public partial class TimeRangeInput : UserControl, ReportParameterInput
    {
        //public const string CUSTOM_RANGE = "Custom Range";
        //public const string LAST_10_MINS = "Last 10 Minutes";
        //public const string LAST_20_MINS = "Last 20 Minutes";
        //public const string LAST_30_MINS = "Last 30 Minutes";
        //public const string LAST_HOUR = "Last Hour";
        //public const string TODAY = "Today";
        //public const string LAST_24_HOURS = "Last 24 Hours";
        //public const string YESTERDAY = "Yesterday";
        //public const string THIS_WEEK = "This Week";
        //public const string LAST_7_DAYS = "Last 7 days";
        //public const string LAST_WEEK = "Last Week";
        //public const string LAST_14_DAYS = "Last 14 Days";
        //public const string LAST_30_DAYS = "Last 30 Days";
        //public const string THIS_MONTH = "This Month";

        //public string[] TIME_RANGES = new string[] { CUSTOM_RANGE, LAST_HOUR, TODAY, LAST_24_HOURS, 
        //                                            YESTERDAY, THIS_WEEK, LAST_7_DAYS, LAST_WEEK, 
        //                                            LAST_14_DAYS, LAST_30_DAYS, THIS_MONTH };

        public event EventHandler InputChanged;

        public string TimeRangeString
        {
            get 
            {
                if (timeRangeCombo.SelectedItem != null)
                {
                    return timeRangeCombo.SelectedItem.ToString();
                }
                return "";
            }
            set
            {
                if(value != null && timeRangeCombo.Items.Contains(value))
                {
                    timeRangeCombo.SelectedItem = value;
                }
            }
        }

        public string RangeGroupBoxText
        {
            get { return groupBox1.Text; }
            set { groupBox1.Text = value; }
        }

        public TimeRangeInput()
        {
            InitializeComponent();
            loadSelectorCombo();
            this.startTime.ManualChangeHandler += new EventHandler(startTime_ManualChangeHandler);
            this.endTime.ManualChangeHandler += new EventHandler(endTime_ManualChangeHandler);
        }

        void endTime_ManualChangeHandler(object sender, EventArgs e)
        {
            timeRangeCombo.SelectedItem = TimeRangeInputBase.CUSTOM_RANGE;
        }

        void startTime_ManualChangeHandler(object sender, EventArgs e)
        {
            timeRangeCombo.SelectedItem = TimeRangeInputBase.CUSTOM_RANGE;
        }

        Object ReportParameterInput.getParamValue(ReportParameter param) 
        {
              if (param.ParamName.Equals(ReportParameterProcessor.FROM_TIME))
              {
                  return ((ReportParameterInput)startTime).getParamValue(param);
              }
              else if (param.ParamName.Equals(ReportParameterProcessor.TO_TIME))
              {
                   return ((ReportParameterInput)endTime).getParamValue(param);
              }
              else if (param.ParamName.Equals(ReportParameterProcessor.TIME_RANGE_KEY))
              {
                  param.Value = TimeRangeString;
                  ReportParameterProcessor.Instance.persistReportParam(param);
                  return TimeRangeString;
              }
              return null;
        }

        void ReportParameterInput.setParameter(ReportParameter param) 
        {
              if (param.ParamName.Equals(ReportParameterProcessor.FROM_TIME))
              {
                  ((ReportParameterInput)startTime).setParameter(param);
              }
              else if (param.ParamName.Equals(ReportParameterProcessor.TO_TIME))
              {
                  ((ReportParameterInput)endTime).setParameter(param);
              }
              else if (param.ParamName.Equals(ReportParameterProcessor.TIME_RANGE_KEY))
              {
                  TimeRangeString = param.Value as string;
              }
        }

        void ReportParameterInput.setParameters(List<ReportParameter> parameters) 
        {
            foreach (ReportParameter temp in parameters)
            {
                ((ReportParameterInput)this).setParameter(temp);
            }
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            setDate((string)timeRangeCombo.SelectedItem);
        }

        //load combo with the default ranges
        private void loadSelectorCombo()
        {
            for (int i = 0; i < TimeRangeInputBase.TIME_RANGES.Length; i++)
            {
                timeRangeCombo.Items.Add(TimeRangeInputBase.TIME_RANGES[i]);
            }
        }//loadCombo

        //Get the day of the week
        static int getDayOfWeek(DayOfWeek dayOfWeek)
        {
            int ret = 0;
            if (dayOfWeek == DayOfWeek.Sunday)
            {
                ret = 0;
            }
            else if (dayOfWeek == DayOfWeek.Monday)
            {
                ret = 1;
            }
            else if (dayOfWeek == DayOfWeek.Tuesday)
            {
                ret = 2;
            }
            else if (dayOfWeek == DayOfWeek.Wednesday)
            {
                ret = 3;
            }
            else if (dayOfWeek == DayOfWeek.Thursday)
            {
                ret = 4;
            }
            else if (dayOfWeek == DayOfWeek.Friday)
            {
                ret = 5;
            }
            else if (dayOfWeek == DayOfWeek.Saturday)
            {
                ret = 6;
            }
            return ret;
        }

        private void setDate(string type)
        {
            DateTime start = DateTime.Now;
            DateTime end = DateTime.Now;
            DateTime currentTime = DateTime.Now;

            ComputeTimeRange(type, out start, out end);

            //if (!type.Equals(TimeRangeInputBase.CUSTOM_RANGE))
            {
                startTime.setTime(start);
                endTime.setTime(end);
            }
        }

        public static void ComputeTimeRange(string type, out DateTime start, out DateTime end)
        {
            start = DateTime.Now;
            end = DateTime.Now;
            DateTime currentTime = DateTime.Now;

            if (type.Equals(TimeRangeInputBase.LAST_7_DAYS))
            {
                start = currentTime.AddDays(-6);
                start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                end = currentTime;
            }
            else if (type.Equals(TimeRangeInputBase.THIS_WEEK))
            {
                start = currentTime;
                start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                start = start.AddDays(-1 * getDayOfWeek(start.DayOfWeek));
                end = currentTime;
            }
            else if (type.Equals(TimeRangeInputBase.LAST_WEEK))
            {
                end = currentTime;
                end = new DateTime(end.Year, end.Month, end.Day, 0, 0, 0);
                end = end.AddDays(-1 * getDayOfWeek(start.DayOfWeek));
                end = end.AddSeconds(-1);
                start = end.AddDays(-7);
                start = start.AddSeconds(1);
            }
            else if (type.Equals(TimeRangeInputBase.LAST_14_DAYS))
            {
                start = currentTime.AddDays(-13);
                start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                end = currentTime;
            }
            else if (type.Equals(TimeRangeInputBase.LAST_30_DAYS) || type.Equals(TimeRangeInputBase.CUSTOM_RANGE))
            {
                start = currentTime.AddDays(-29);
                start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                end = currentTime;
            }
            else if (type.Equals(TimeRangeInputBase.THIS_MONTH))
            {
                start = currentTime;
                start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                start = start.AddDays(-1 * (start.Day - 1));
                end = currentTime;
            }

            else if (type.Equals(TimeRangeInputBase.YESTERDAY))
            {
                end = currentTime;
                end = new DateTime(end.Year, end.Month, end.Day, 0, 0, 0);
                start = end.AddDays(-1);
                end = end.AddSeconds(-1);
            }
            else if (type.Equals(TimeRangeInputBase.LAST_24_HOURS))
            {
                end = currentTime;
                start = end.AddHours(-23);
                start = start.AddMinutes(-60);
                start = start.AddSeconds(1);
            }
            else if (type.Equals(TimeRangeInputBase.TODAY))
            {
                start = currentTime;
                start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                end = currentTime;
            }
            else if (type.Equals(TimeRangeInputBase.LAST_HOUR))
            {
                start = currentTime;
                start = start.AddMinutes(-60);
                start = start.AddSeconds(1);
                end = currentTime;
            }
        }

        public static string ComputeSQLTimeRange(string type)
        {
            switch (type)
            {
                case TimeRangeInputBase.LAST_10_MINS:
                    {
                        return "BETWEEN DATE_SUB(CURRENT_TIMESTAMP(0), INTERVAL '9:59' MINUTE TO SECOND) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_20_MINS:
                    {
                        return "BETWEEN DATE_SUB(CURRENT_TIMESTAMP(0), INTERVAL '19:59' MINUTE TO SECOND) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_30_MINS:
                    {
                        return "BETWEEN DATE_SUB(CURRENT_TIMESTAMP(0), INTERVAL '29:59' MINUTE TO SECOND) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_HOUR:
                    {
                        return "BETWEEN DATE_SUB(CURRENT_TIMESTAMP(0), INTERVAL '59:59' MINUTE TO SECOND) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.TODAY:
                    {
                        return "BETWEEN DATE_SUB(CURRENT_TIMESTAMP(0), CAST(DATEFORMAT(CURRENT_TIME,DEFAULT) AS INTERVAL HOUR TO SECOND)) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_24_HOURS:
                    {
                        return "BETWEEN DATE_SUB(CURRENT_TIMESTAMP(0), INTERVAL '23:59:59' HOUR TO SECOND) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.YESTERDAY:
                    {
                        return "BETWEEN CAST(DATE_SUB(CURRENT_DATE, INTERVAL '1' DAY) AS TIMESTAMP(6)) AND CAST(DATE_SUB(CURRENT_DATE, INTERVAL '1' SECOND) AS TIMESTAMP(6))";
                    }
                case TimeRangeInputBase.THIS_WEEK:
                    {
                        return "BETWEEN CAST(CURRENT_DATE + 1 - DAYOFWEEK(CURRENT_DATE) AS TIMESTAMP(6)) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_7_DAYS:
                    {
                        return "BETWEEN CAST(DATE_SUB(CURRENT_DATE, INTERVAL '6' DAY) AS TIMESTAMP(6)) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_WEEK:
                    {
                        return "BETWEEN CAST(CURRENT_DATE + 1 - 7 - DAYOFWEEK(CURRENT_DATE) AS TIMESTAMP(6)) AND CAST(DATE_SUB(CURRENT_DATE + 1 -  DAYOFWEEK(CURRENT_DATE), INTERVAL '1' SECOND) AS TIMESTAMP(6))";
                    }
                case TimeRangeInputBase.LAST_14_DAYS:
                    {
                        return "BETWEEN CAST(DATE_SUB(CURRENT_DATE, INTERVAL '13' DAY) AS TIMESTAMP(6)) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.THIS_MONTH:
                    {
                        return "BETWEEN CAST(CURRENT_DATE + 1 - DAYOFMONTH(CURRENT_DATE) AS TIMESTAMP(6)) AND CURRENT_TIMESTAMP";
                    }
                case TimeRangeInputBase.LAST_30_DAYS:
                default:
                    {
                        return "BETWEEN CAST(DATE_SUB(CURRENT_DATE, INTERVAL '29' DAY) AS TIMESTAMP(6)) AND CURRENT_TIMESTAMP";
                    }
            }
        }

        private void endTime_Load(object sender, EventArgs e)
        {

        }

        protected virtual void OnInputChanged(InputEventArgs e)
        {
            if (InputChanged != null)
            {
                InputChanged(this, e);
            }
        }

        /// <summary>
        /// Fire the user input changing event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void startTime_InputChanged(object sender, EventArgs e)
        {
            OnInputChanged(new InputEventArgs());
        }

        /// <summary>
        /// Fire the user input changing event.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void endTime_InputChanged(object sender, EventArgs e)
        {
            OnInputChanged(new InputEventArgs());
        }

        /// <summary>
        /// Validate user input
        /// </summary>
        /// <returns></returns>
        bool ReportParameterInput.IsValidInput()
        {
            return ((ReportParameterInput)startTime).IsValidInput() &&
                ((ReportParameterInput)endTime).IsValidInput() && isValidTimeRange();
        }

        /// <summary>
        /// Comparing time range
        /// </summary>
        /// <returns>True:From time is smaller than To time. otherwise return false</returns>
        bool isValidTimeRange()
        {            
            ReportParameter para=new ReportParameter();
            object objStart= ((ReportParameterInput)startTime).getParamValue(para);
            object objEnd = ((ReportParameterInput)endTime).getParamValue(para);
            if (DateTime.Compare(Convert.ToDateTime(objStart), Convert.ToDateTime(objEnd)) <= 0)
            {

                _theErrorMsg.Text = string.Empty;
                _theErrorMsg.Visible = false;
                return true;
            }
            else
            {
                _theErrorMsg.Text = Properties.Resources.CompareTimeRangeErrorMsg;
                _theErrorMsg.Visible = true;
                return false;
            }
            
        }
    }
}
