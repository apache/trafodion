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
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Trafodion.Manager.Framework.Queries;
using System.Collections;
using System.Windows.Forms;

namespace Trafodion.Manager.OverviewArea.Models
{
    public class TimeRangesHandler
    {
        static private string[] _theTimeRanges = { "All Times", "Custom Range", "Last 10 Minutes", "Last 20 Minutes", "Last 30 Minutes", "Last 1 Hour", "Today", "Last 24 Hours", "Last 7 Days", "Last 14 Days", "Last 30 Days", "Live Feed Only" };
        public enum Range { AllTimes = 0, CustomRange, Last10Minutes, Last20Minutes, Last30Minutes, Last1Hour, Today, Last24Hours, Last7Days, Last14Days, Last30Days, LiveFeedOnly };
        private Range _theRange = Range.Last30Days;
        private bool _isAllTimes = false;
        private bool _isCustomRange = false;
        public Hashtable rangeTable = new Hashtable();

        //private DateTime _defaulCustomtStartTime=new DateTime();
        //private DateTime _defaultCustomEndTime=new DateTime();
        private DateTime _defaultCustomStartTime;

        public static DateTime MaxCustomEndTime = DateTimePicker.MaxDateTime.AddYears(-1);
        public static DateTime MinCustomStartTime = DateTimePicker.MinimumDateTime.AddYears(1);
        
        public DateTime DefaultCustomStartTime
        {
            get
            {
                _defaultCustomStartTime = DateTime.Now.AddDays(-30);
                return _defaultCustomStartTime;
            }
            set { _defaultCustomStartTime = value; }
        }

        public Range TheRange
        {
            get { return _theRange; }
            set { _theRange = value; }
        }
        public bool IsAllTimes
        {
            get { return _isAllTimes; }
        }

        public bool IsCustomRange
        {
            get { return _isCustomRange; }
        }

        public TimeRangesHandler()
        {
            foreach (int item in Enum.GetValues(typeof(Range)))
            {
                rangeTable.Add((Range)item, _theTimeRanges[item]);
            }            
        }

        public TimeRangesHandler(Range aRange):this()
        {
            _theRange = aRange;
            _isCustomRange = (aRange.Equals(Range.CustomRange));
            _isAllTimes = (aRange.Equals(Range.AllTimes));
        }

        //public DateTime[] GetDateTimeRange()
        //{
        //    return GetDateTimeRange(DateTime.Now);
        //}
        public String GetTimeRangeString(Range aRange)
        {
            if (rangeTable.ContainsKey(aRange))
            {
                return rangeTable[aRange] as String;
            }
            return rangeTable[Range.Last30Days] as String; 
        }
        public static DateTime[] GetDateTimeRange(DateTime serverTime, Range _theRange)
        {
            DateTime[] returnValue = new DateTime[2];
            returnValue[1] = serverTime;
            switch (_theRange)
            {
                case Range.Last10Minutes:
                    returnValue[0] = serverTime.AddMinutes(-10);
                    break;
                case Range.Last20Minutes:
                    returnValue[0] = serverTime.AddMinutes(-20);
                    break;
                case Range.Last30Minutes:
                    returnValue[0] = serverTime.AddMinutes(-30);
                    break;
                case Range.Last1Hour:
                    returnValue[0] = serverTime.AddMinutes(-60);
                    break;
                case Range.Today:
                    returnValue[0] = serverTime.Date; 
                    break;
                case Range.Last24Hours:
                    returnValue[0] = serverTime.AddHours(-24);
                    break;
                case Range.Last7Days:
                    returnValue[0] = serverTime.AddDays(-7);
                    break;
                case Range.Last14Days:
                    returnValue[0] = serverTime.AddDays(-14);
                    break;
                case Range.Last30Days:
                    returnValue[0] = serverTime.AddDays(-30);                    
                    break;

                case Range.AllTimes:
                case Range.LiveFeedOnly:                    
                case Range.CustomRange:
                    return null;

            }
            return returnValue;
        }

        public override string ToString()
        {
            return _theTimeRanges[(int)_theRange];
        }

        public override bool Equals(object obj)
        {
            TimeRangesHandler trh = obj as TimeRangesHandler;
            if (trh != null)
            {
                return (_theRange == trh.TheRange);
            }
            return false;
        }

        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

    }
}
