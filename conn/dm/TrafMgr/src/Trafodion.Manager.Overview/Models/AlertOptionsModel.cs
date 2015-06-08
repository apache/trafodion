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
using Trafodion.Manager.DatabaseArea.Queries;
using Trafodion.Manager.Framework.Queries;
using Trafodion.Manager.DatabaseArea.Queries.Controls;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Model for the Alert widget options. 
    /// This object is persisted across TrafodionManager sessions and reloaded whenever the Alerts widget is recreated
    /// </summary>
    [Serializable]
    public class AlertOptionsModel: ICloneable
    {
        #region private member variables

        private bool _fetchOpenAlertsOnly = true;

        TimeRangesHandler.Range _theTimeRange;
        bool _includeLiveFeed = true;
        DateTime _theCustomEndTime;
        DateTime _theCustomStartTime;

        [NonSerialized]
        private TimeSpan _serverGMTOffset;

        private static List<TimeRangesHandler> _theTimeRanges = new List<TimeRangesHandler>();

        #endregion private member variables

        #region Public Properties        

        /// <summary>
        /// Enum to indicate the alert level
        /// </summary>
        public enum AlertType { EMERGENCY = 0, IMMEDIATE = 1, CRITICAL = 2, ERROR =3, WARNING=4 };
        

        /// <summary>
        /// Indicates if only open alerts should be fetched
        /// </summary>
        public bool FetchOpenAlertsOnly
        {
            get { return _fetchOpenAlertsOnly; }
            set { _fetchOpenAlertsOnly = value; }
        }
                       
        public TimeRangesHandler.Range TimeRange
        {
            get { return _theTimeRange; }
            set { _theTimeRange = value; }
        }
        
        public bool IncludeLiveFeed
        {
            get { return _includeLiveFeed; }
            set { _includeLiveFeed = value; }
        }

        public TimeSpan ServerGMTOffset
        {
            get { return _serverGMTOffset; }
            set { _serverGMTOffset = value; }
        }

        public DateTime TheEndTime
        {
            get { return _theCustomEndTime; }
            set { _theCustomEndTime = value; }
        }
        
        public DateTime TheStartTime
        {
            get { return _theCustomStartTime; }
            set { _theCustomStartTime = value; }
        }

        public List<TimeRangesHandler> TimeRanges
        {
            get
            {
                if (_theTimeRanges != null && _theTimeRanges.Count == 0)
                    _theTimeRanges = InitTimeRanges();
                return _theTimeRanges; 
            }
            set { _theTimeRanges = value; }
        }        

        #endregion Public Properties

        #region Constructor/Initializer


        /// <summary>
        /// Constructor
        /// </summary>
        public AlertOptionsModel()
        {
            _theTimeRange = TimeRangesHandler.Range.Last30Days;
            _includeLiveFeed = true;
            _theCustomEndTime = TimeRangesHandler.MaxCustomEndTime;
            _theCustomStartTime = InitCustomStartTime();
            _theTimeRanges = InitTimeRanges();
        }

        #endregion Constructor/Initializer


        #region Public methods
        
        public string GetFilterSQL()
        {
            string TimeFilter = "";

            switch (_theTimeRange)
            {
                case TimeRangesHandler.Range.Last10Minutes:
                case TimeRangesHandler.Range.Last20Minutes:
                case TimeRangesHandler.Range.Last30Minutes:
                case TimeRangesHandler.Range.Last1Hour:
                case TimeRangesHandler.Range.Last24Hours:
                case TimeRangesHandler.Range.Last7Days:
                case TimeRangesHandler.Range.Last14Days:
                case TimeRangesHandler.Range.Last30Days:
                case TimeRangesHandler.Range.Today:
                    {
                        TimeFilter = "CREATE_TS_LCT";
                        DateTime[] timeRanges = TimeRangesHandler.GetDateTimeRange(GetServerLCTTime(DateTime.Now), _theTimeRange);
                        DateTime _theStartTime = timeRanges[0];
                        TimeFilter = string.Format("{0} >= {1}", TimeFilter, getDateForQuery(_theStartTime));
                        break;
                    }

                case TimeRangesHandler.Range.LiveFeedOnly:
                    break;
                    
                case TimeRangesHandler.Range.AllTimes:
                    break;

                case TimeRangesHandler.Range.CustomRange:
                    TimeFilter = "CREATE_TS_LCT";
                    
                    if (_theCustomStartTime == TimeRangesHandler.MinCustomStartTime) //This means Start Time is not set, so it's set as default Min Value;
                    {
                        TimeFilter = string.Format("{0} <= {1}", TimeFilter, getDateForQuery(_theCustomEndTime));
                    }
                    else if (_theCustomEndTime == TimeRangesHandler.MaxCustomEndTime) //This means End Time is not set, so it's set as default Max Value;
                    {
                        TimeFilter = string.Format("{0} >= {1}", TimeFilter, getDateForQuery(_theCustomStartTime));
                    }
                    else 
                    {
                        TimeFilter = string.Format("({0} >= {1}) AND ({2} <= {3}) ", TimeFilter, getDateForQuery(_theCustomStartTime), TimeFilter, getDateForQuery(_theCustomEndTime));
                    }
                    break;                
            }
            return TimeFilter;            
        }

        //Helper to create the date string that is needed in the SQL statement
        public string getDateForQuery(DateTime aDateTime)
        {
            string timestamp = aDateTime.ToString("yyyy-MM-dd HH:mm:ss");
            return string.Format("TIMESTAMP '{0}'", timestamp);
        }

        public  DateTime InitCustomStartTime()
        {
            //DateTime start = DateTime.Now;
            //start = start.AddDays(-29);
            //start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
            DateTime start = DateTime.Now.AddDays(-30);
            return start;
        }

        public List<TimeRangesHandler> InitTimeRanges()
        {
            List<TimeRangesHandler> timeRangeList = new List<TimeRangesHandler>();
            foreach (var item in Enum.GetValues(typeof(TimeRangesHandler.Range)))
            {
                TimeRangesHandler handler=new TimeRangesHandler((TimeRangesHandler.Range)item);
                timeRangeList.Add(handler);
            }
            return timeRangeList;
        }

        public DateTime GetServerLCTTime(DateTime userLocalTime)
        {
            TimeSpan timeSpan = this.ServerGMTOffset;
            DateTime serverLctTime = userLocalTime.ToUniversalTime() + timeSpan;
            return serverLctTime;
        }

        public override bool Equals(object obj)
        {
            if (obj == null)
                return false;
            
            AlertOptionsModel alertOptions = obj as AlertOptionsModel;
            if ((System.Object)alertOptions == null)
                return false;

            if (FetchOpenAlertsOnly == alertOptions.FetchOpenAlertsOnly
                && TimeRange == alertOptions.TimeRange
                && IncludeLiveFeed == alertOptions.IncludeLiveFeed
                && TheStartTime == alertOptions.TheStartTime
                && TheEndTime == alertOptions.TheEndTime)
                return true;
            else
                return false;
        }


        #endregion

        #region ICloneable Members

        object ICloneable.Clone()
        {
            return this.Clone();
        }

        public AlertOptionsModel Clone() 
        {
            return (AlertOptionsModel)this.MemberwiseClone();
        }

        #endregion
    }
}
