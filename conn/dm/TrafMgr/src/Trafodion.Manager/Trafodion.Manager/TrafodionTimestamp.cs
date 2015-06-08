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


namespace Trafodion.Manager.Framework
{
   /// <summary>
    /// This object shall be used to display the sizes/timestamps of the objects in a DataGridView
    /// This will allow the sort to happen correctly
    /// </summary>
    public class TrafodionTimestamp : IComparable
    {
        long _theTimestamp;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTime"></param>
        public TrafodionTimestamp(long aTimestamp)
        {
            _theTimestamp = aTimestamp;
        }

        /// <summary>
        /// 
        /// </summary>
        public long Value
        {
            get { return _theTimestamp; }
            set { _theTimestamp = value; }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        /// <returns></returns>
        public int CompareTo(Object obj)
        {
            // should be a long
            if (obj is TrafodionTimestamp)
            {
                return _theTimestamp.CompareTo(((TrafodionTimestamp)obj).Value);
            }
            return -1;
        }

        /// <summary>
        /// The derived class overrides this and returns the proper formatted string.
        /// </summary>
        public override string ToString()
        {
            return Value.ToString();
        }

    }

   
    /// <summary>
    /// This object shall be used to display the Timestamps of the objects in a DataGridView
    /// This will allow the sort to happen correctly
    /// </summary>
    public class JulianTimestamp : TrafodionTimestamp
    {
        private string _defaultValue = "Never";
        private TimeSpan _offset = TimeSpan.FromSeconds(0);
        private string _timeZoneName = "";

        /// <summary>
        /// Constructs a JulianTimestamp object
        /// </summary>
        /// <param name="aTimestamp">the long value representing the timestamp</param>
        public JulianTimestamp(long aTimestamp)
            : base(aTimestamp)
        {
        }

        /// <summary>
        /// Constructs a JulianTimestamp object
        /// </summary>
        /// <param name="aTimestamp">the long value representing the timestamp</param>
        /// <param name="anOffset">Offset from UTC time</param>
        /// <param name="aTimeZoneName">a string value indicating the timezone name, used when date time kind is local</param>
        public JulianTimestamp(long aTimestamp, TimeSpan anOffset, string aTimeZoneName)
            : base(aTimestamp)
        {
            _offset = anOffset;
            _timeZoneName = aTimeZoneName;
        }        
        /// <summary>
        /// Constructs a JulianTimestamp object
        /// </summary>
        /// <param name="aTimestamp">the long value representing the timestamp</param>
        /// <param name="aDefaultValue"> a string value to display if the long value is not a positive integer </param>
        public JulianTimestamp(long aTimestamp, string aDefaultValue)
            : base(aTimestamp)
        {
            _defaultValue = aDefaultValue;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTimestamp">the long value representing the timestamp</param>
        /// <param name="aDefaultValue"> a string value to display if the long value is not a positive integer </param>
        /// <param name="anOffset">Offset from UTC time</param>
        /// <param name="aTimeZoneName">a string value indicating the timezone name, used when date time kind is local</param>
        public JulianTimestamp(long aTimestamp, string aDefaultValue, TimeSpan anOffset, string aTimeZoneName)
            : base(aTimestamp)
        {
            _defaultValue = aDefaultValue;
            _offset = anOffset;
            _timeZoneName = aTimeZoneName;
        }

        /// <summary>
        /// Formats the juliantimestamp in the right time zone format for display
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Utilities.FormattedJulianTimestamp(Value, _defaultValue, _offset, false, false, Utilities.STANDARD_DATETIME_FORMAT);
        }
    }

    /// <summary>
    /// This object shall be used to display the Timestamps of the objects in a DataGridView
    /// This will allow the sort to happen correctly
    /// </summary>
    public class UnixFileTimestamp : TrafodionTimestamp
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTimestamp"></param>
        public UnixFileTimestamp(long aTimestamp)
            : base(aTimestamp)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Utilities.GetFormattedTimeFromUnixTimestamp(Value);
        }
    }

    /// <summary>
    /// This object shall be used to display the Timestamps of the objects in a DataGridView
    /// This will allow the sort to happen correctly
    /// </summary>
    public class EpochTimestamp : TrafodionTimestamp
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="aTimestamp"></param>
        public EpochTimestamp(long aTimeSecondsSinceEpoch)
            : base(aTimeSecondsSinceEpoch)
        {
        }

        /// <summary>
        /// Formats the datetime from seconds Since Epoch to a human readable form
        /// </summary>
        /// <returns></returns>
        public DateTime GetFormattedTimeFromSecondsSinceEpoch()
        {
            return new DateTime((Value * 10000000) + 621355968000000000, DateTimeKind.Utc).ToLocalTime();
        }

        /// <summary>
        /// use formatted string to display this value
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return Utilities.GetFormattedTimeFromSecondsSinceEpoch(Value,false,false);
        }
    }

}
