// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.Globalization;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;


namespace Trafodion.Manager.BDRArea.Controls
{
    public partial class MyCalendarPopUp : TrafodionForm
    {
        #region Fields
        // used by calendar and time selection
        private char[] _TimeDelimiters = { ':', ' ' };
        private string[] _TimeParts = new string[6];

        // calendar and time selection use these to hold the yyyy-mm-dd hh:mm:ss assembly
        //private string _WorkingFromTS;
        //private string _WorkingToTS;

        // Ultimately, the chosen begin/end DT's are held as DateTime types in UTC
        // if LCT box is checked, make the offset
        private DateTime _MyNow = DateTime.UtcNow; 
        private DateTime _MyThen = DateTime.UtcNow; 

        private string _OnBehalfOf;

        // need this to access items in my caller
        MyWidgetControl _myWidgetControl;
        #endregion Fields


        public MyCalendarPopUp(MyWidgetControl x, string aString)
        {
            InitializeComponent();

            _myWidgetControl = x;
            _OnBehalfOf = aString;

            // load the time as 1/2 hr intervals
            _chooseFromTimeComboBox.Items.AddRange(new object[] {
                "00:00:00",  "00:30:00",
                "01:00:00",  "01:30:00",
                "02:00:00",  "02:30:00",
                "03:00:00",  "03:30:00",
                "04:00:00",  "04:30:00",
                "05:00:00",  "05:30:00",
                "06:00:00",  "06:30:00",
                "07:00:00",  "07:30:00",
                "08:00:00",  "08:30:00",
                "09:00:00",  "09:30:00",
                "10:00:00",  "10:30:00",
                "11:00:00",  "11:30:00",
                "12:00:00",  "12:30:00",
                "13:00:00",  "13:30:00",
                "14:00:00",  "14:30:00",
                "15:00:00",  "15:30:00",
                "16:00:00",  "16:30:00",
                "17:00:00",  "17:30:00",
                "18:00:00",  "18:30:00",
                "19:00:00",  "19:30:00",
                "20:00:00",  "20:30:00",
                "21:00:00",  "21:30:00",
                "22:00:00",  "22:30:00",
                "23:00:00",  "23:30:00"
            });
            _chooseToTimeComboBox.Items.AddRange(new object[] {
                "00:00:00",  "00:30:00",
                "01:00:00",  "01:30:00",
                "02:00:00",  "02:30:00",
                "03:00:00",  "03:30:00",
                "04:00:00",  "04:30:00",
                "05:00:00",  "05:30:00",
                "06:00:00",  "06:30:00",
                "07:00:00",  "07:30:00",
                "08:00:00",  "08:30:00",
                "09:00:00",  "09:30:00",
                "10:00:00",  "10:30:00",
                "11:00:00",  "11:30:00",
                "12:00:00",  "12:30:00",
                "13:00:00",  "13:30:00",
                "14:00:00",  "14:30:00",
                "15:00:00",  "15:30:00",
                "16:00:00",  "16:30:00",
                "17:00:00",  "17:30:00",
                "18:00:00",  "18:30:00",
                "19:00:00",  "19:30:00",
                "20:00:00",  "20:30:00",
                "21:00:00",  "21:30:00",
                "22:00:00",  "22:30:00",
                "23:00:00",  "23:30:00"
            });

            // default TO to current UTC
            toTSTextBox.Text = _MyNow.ToString("yyyy-MM-dd HH:mm:ss");
        }

        private void CheckForUTC_From()
        {
            if (_utcCheckBox.Checked)   // need LCT adjust
            {
                TimeZone local = TimeZone.CurrentTimeZone;
                TimeSpan offset = local.GetUtcOffset(DateTime.Now);

                fromTSTextBox.Text = _MyThen.AddHours(Convert.ToDouble(offset.TotalHours)).ToString("yyyy-MM-dd HH:mm:ss");
            }
            else
            {  fromTSTextBox.Text = _MyThen.ToString("yyyy-MM-dd HH:mm:ss");  }
        }

        private void CheckForUTC_To()
        {
            // Method name is misleading.
            // Originally accepted LCT, so checkbox adjusted to UTC.
            // By user request, entered times are treated as UTC - HPIT systems run on UTC - so filters default to UTC;
            // if user wants LCT adjust, then check the box.

            if (_utcCheckBox.Checked)   // need LCT adjust
            {
                TimeZone local = TimeZone.CurrentTimeZone;
                TimeSpan offset = local.GetUtcOffset(DateTime.Now);

                //MessageBox.Show("UTC Off: " + offset.ToString());
                toTSTextBox.Text = _MyNow.AddHours(Convert.ToDouble(offset.TotalHours)).ToString("yyyy-MM-dd HH:mm:ss"); }
            else
            {  toTSTextBox.Text = _MyNow.ToString("yyyy-MM-dd HH:mm:ss"); }
        }

        private bool CheckChronology()
        {
            // FROM date must be earlier than TO date
            // check THEN earlier than NOW, if NOW has been set
            if (_MyThen > _MyNow)
            {
                MessageBox.Show("FROM time must occur before TO time!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return true;
            }
            return false;
        }

        /* The calendar + time controls work on the date differently than the pre-defined time ranges. 
         * Have both work with DateTime _MyNow & _MyThen.  Doing so should allow choosing in any order/combination and 
         * still arrive at a valid datetime format - altho chronology may fail.
         * 
         * Allow any selection for Then and Now...chronology check made at OK-button
         */
        private void monthCalendar1_DateChanged(object sender, DateRangeEventArgs e)
        {
            _MyThen = e.Start.Date;
            CheckForUTC_From();
        }

        private void monthCalendar2_DateChanged(object sender, DateRangeEventArgs e)
        {
            _MyNow = e.Start.Date;
            CheckForUTC_To();
        }

        private void _chooseFromTimeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            // The selected time replaces HH:MM:SS in _MyThen..._MyThen could have been set from either
            // the calendar control or Predefined drop list, so make this flexible to replace the time with
            // the selection
            
            _TimeParts = _chooseFromTimeComboBox.Text.ToString().Split(_TimeDelimiters);    // produces 3 items

            _MyThen = _MyThen.Date;         // truncate whatever time portion
            
            double ticks = Convert.ToInt16(_TimeParts[0]) * 3600 + 
                           Convert.ToInt16(_TimeParts[1]) * 60 + 
                           Convert.ToInt16(_TimeParts[2]);             // ticks = hrs + min + sec
            _MyThen = _MyThen.AddSeconds(ticks);

            CheckForUTC_From();
        }

        private void _chooseToTimeComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            _TimeParts = _chooseToTimeComboBox.Text.ToString().Split(_TimeDelimiters);
            _MyNow = _MyNow.Date;         // truncate whatever time portion
            double ticks = Convert.ToInt16(_TimeParts[0]) * 3600 +
                           Convert.ToInt16(_TimeParts[1]) * 60 +
                           Convert.ToInt16(_TimeParts[2]);             // ticks = hrs + min + sec
            _MyNow = _MyNow.AddSeconds(ticks);

            CheckForUTC_To();
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            // check for proper order
            if (CheckChronology()) return;
            
            switch (_OnBehalfOf)
            {
                case "START":
                _myWidgetControl.SetStartFromDate = fromTSTextBox.Text;
                _myWidgetControl.SetStartToDate = toTSTextBox.Text;
                break;

                case "END":
                _myWidgetControl.SetEndFromDate = fromTSTextBox.Text;
                _myWidgetControl.SetEndToDate = toTSTextBox.Text;
                break;
            }

            this.Close();
        }

        private void cancelButton_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void dateFiltersListComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {

            //TimeZone local = TimeZone.CurrentTimeZone;
            //TimeSpan offset = local.GetUtcOffset(DateTime.Now);
            string TheItem = (string) dateFiltersListComboBox.SelectedItem;
            Calendar myCal = CultureInfo.InvariantCulture.Calendar;

            // Depending on which predefined item was picked, I'll need a TimeSpan object to represent the
            // desired number of days or hours
            // ...and adjust for UTC, since PREVIOUS x units are relative to client PC

            //MessageBox.Show("offset: " + offset + "\nNow: " + _MyNow);
            _MyNow = System.DateTime.UtcNow;
            switch (TheItem)
            {
                case "Previous 24 Hr":  
                    _MyThen = _MyNow - (new TimeSpan(24,0,0));
                    break;
                case "Previous 1 Hr":
                    _MyThen = _MyNow.Subtract(new TimeSpan(1, 0, 0));
                    break;
                case "Previous 2 Hr":
                    _MyThen = _MyNow.Subtract(new TimeSpan(2, 0, 0));
                    break;
                case "Previous 3 Hr":
                    _MyThen = _MyNow.Subtract(new TimeSpan(3, 0, 0));
                    break;
                case "Previous 4 Hr":
                    _MyThen = _MyNow.Subtract(new TimeSpan(4, 0, 0));
                    break;
                case "Previous 1 Day":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(1, 0, 0, 0));
                    break;
                case "Previous 2 Days":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(2, 0, 0, 0));
                    break;
                case "Previous 3 Days":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(3, 0, 0, 0));
                    break;
                case "Previous 4 Days":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(4, 0, 0, 0));
                    break;
                case "Previous 5 Days":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(5, 0, 0, 0));
                    break;
                case "Previous 6 Days":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(6, 0, 0, 0));
                    break;
                case "Previous 1 Week":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(7, 0, 0, 0));
                    break;
                case "Previous 2 Weeks":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(14, 0, 0, 0));
                    break;
                case "Previous 3 Weeks":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = System.DateTime.UtcNow.Date.Subtract(new TimeSpan(21, 0, 0, 0));
                    break;
                case "Previous 1 Month":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = myCal.AddMonths(_MyNow, -1);
                    break;
                case "Previous 2 Months":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = myCal.AddMonths(_MyNow, -2);
                    break;
                case "Previous 3 Months":
                    _MyNow = System.DateTime.UtcNow.Date;
                    _MyThen = myCal.AddMonths(_MyNow, -3);
                    break;
            }

            //MessageBox.Show("Now = " + _MyNow.ToString("yyyy-MM-dd HH:mm:ss") +
            //                "\nThen = " + _MyThen.ToString("yyyy-MM-dd HH:mm:ss"));

            CheckForUTC_From();
            CheckForUTC_To();
        }

        private void _utcCheckBox_Click(object sender, EventArgs e)
        {
            CheckForUTC_From();
            CheckForUTC_To();
        }
    }
}
