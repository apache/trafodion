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
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.Odbc;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Microsoft.Win32;

namespace Trafodion.Manager.Framework
{

    public struct IGridSortObject
    {
        public int ColIndex;
        public TenTec.Windows.iGridLib.iGSortOrder SortOrder;
        public TenTec.Windows.iGridLib.iGSortType SortType;

        public IGridSortObject(int col, TenTec.Windows.iGridLib.iGSortOrder sortOrder, TenTec.Windows.iGridLib.iGSortType sortType)
        {
            ColIndex = col;
            SortOrder = sortOrder;
            SortType = sortType;
        }
    }

    /// <summary>
    /// Scroll direction parameter for SendMessag message
    /// </summary>
    enum ScrollDirection
    {
        Horizontal = 0x0114,      
        Vertical = 0x0115
    }

    /// <summary>
    /// Horizontal scroll bar command parameter for SendMessage
    /// </summary>
    enum HScrollBarCommands
    {
        SB_LINELEFT = 0,
        SB_LINERIGHT = 1,
        SB_PAGELEFT = 2,
        SB_PAGERIGHT = 3,
        SB_THUMBPOSITION = 4,
        SB_THUMBTRACK = 5,
        SB_LEFT = 6,
        SB_RIGHT = 7,
        SB_ENDSCROLL = 8
    }; 
        
    /// <summary>
    /// Vertical scroll bar command parameter for SendMessage
    /// </summary>
    enum VScrollBarCommands
    {
        SB_LINEUP = 0,
        SB_LINEDOWN = 1,
        SB_PAGEUP = 2,
        SB_PAGEDOWN = 3,
        SB_THUMBPOSITION = 4,
        SB_THUMBTRACK = 5,
        SB_TOP = 6,
        SB_BOTTOM = 7,
        SB_ENDSCROLL = 8
    };


    /// <summary>
    /// Utility class with useful conversion methods
    /// </summary>
    static public class Utilities
    {
        static public bool HAS_ERROR_ON_LOADING_CONFIG_FILE = false;
        private const string REGEX_SERVER_FILE_PATH = @"^/((\.*[a-zA-Z0-9_\-]+\.*)*/+|\.{3,}/+)*((\.*[a-zA-Z0-9_\-]+\.*)+|\.{3,})+$";
        private const string REGEX_SERVER_FILE_NAME = @"^((\.*[a-zA-Z0-9_\-]+\.*)+|\.{3,})+$";

        [DllImport("user32.dll")]
        static extern IntPtr GetForegroundWindow();

        [DllImport("kernel32.dll", SetLastError=true)]
        public static extern void GlobalMemoryStatusEx(ref MEMORYSTATUSEX lpBuffer);

        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        internal static extern IntPtr SendMessage(IntPtr hwnd, Int32 wMsg, IntPtr wParam, IntPtr lParam);

        public struct MEMORYSTATUSEX
        {
            /// DWORD->unsigned int   
            public uint dwLength;
            /// DWORD->unsigned int   
            public uint dwMemoryLoad;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullTotalPhys;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullAvailPhys;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullTotalPageFile;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullAvailPageFile;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullTotalVirtual;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullAvailVirtual;
            /// DWORDLONG->ULONGLONG->unsigned __int64   
            public ulong ullAvailExtendedVirtual;
        }

        [DllImport("kernel32.dll")]
        private static extern void GetNativeSystemInfo(ref SystemInfoNative lpSystemInfo);

        private struct SystemInfoNative
        {
            internal ushort processorArchitecture;
            private ushort reserved;
            internal uint pageSize;
            internal IntPtr minimumApplicationAddress;
            internal IntPtr maximumApplicationAddress;
            internal IntPtr activeProcessorMask;
            internal uint numberOfProcessors;
            internal uint processorType;
            internal uint allocationGranularity;
            internal ushort processorLevel;
            internal ushort processorRevision;
        }


        
        /// <summary>
        /// Long format used by Trafodion Database Manager for date time : yyyy-MM-dd HH':'mm':'ss.FFFFFF
        /// </summary>
        static public string DateTimeLongFormat12HourString = "yyyy-MM-dd hh':'mm':'ss.FFFFFF tt";

        /// <summary>
        /// 24 hour format long time
        /// </summary>
        static public string DateTimeLongFormat24HourString = "yyyy-MM-dd HH':'mm':'ss.FFFFFF";

        /// <summary>
        /// Short format used by Trafodion Database Manager for date time : yyyy-MM-dd HH':'mm':'ss
        /// </summary>
        static public string DateTimeFormatString = "yyyy-MM-dd hh':'mm':'ss tt";
        /// <summary>
        /// Format used by Trafodion Database Manager for date : yyyy-MM-dd
        /// </summary>
        static public string DateFormatString = "yyyy-MM-dd";

        /// <summary>
        /// Standard datetime format
        /// </summary>
        public static string STANDARD_DATETIME_FORMAT = "yyyy-MM-dd HH':'mm':'ss";
        
        static public string LAST_FILE_LOCATION_KEY = "LastFileLocationKey";
        public const String MICROSOFT_ODBC_INI_REGISTRY_KEY = "SOFTWARE\\ODBC\\ODBC.INI";
        public const String MICROSOFT_ODBC_DSN_REGISTRY_KEY = "SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources";
        public const long NANO_SECOND_FROM_1970 = 621355968000000000L;
        
        /**
         *  Windows Registry ODBC Server name key.
         */
        public const string REGISTRY_SERVER_KEY = "Server";

        /**
         *  Windows Registry ODBC Schema name key.
         */
        public const string REGISTRY_SCHEMA_KEY = "Schema";

        /// <summary>
        /// Standardize datetime to a string
        /// </summary>
        /// <param name="dateTime"></param>
        /// <returns></returns>
        public static string StandardizeDateTime(DateTime dateTime)
        {
            return dateTime.ToString(STANDARD_DATETIME_FORMAT);
        }


        /// <summary>
        /// Converts a Trafodion Julian timestamp to DateTime string
        /// </summary>
        /// <param name="aJulianTimestamp">a long value representing the julian timestamp</param>
        /// <returns>a DateTime string</returns>
        static public string FormattedJulianTimestamp(long aJulianTimestamp)
        {
            return FormattedJulianTimestamp(aJulianTimestamp, "Never");
        }

        /// <summary>
        /// Converts a Trafodion Julian timestamp to DateTime string
        /// </summary>
        /// <param name="aJulianTimestamp">a long value representing the julian timestamp</param>
        /// <param name="defaultValue">a string value to display if the julian value is not a positive integer</param>
        /// <returns>a DateTime string</returns>
        static public string FormattedJulianTimestamp(long aJulianTimestamp, string defaultValue)
        {
            return FormattedJulianTimestamp(aJulianTimestamp, defaultValue, TimeSpan.FromSeconds(0), false, false, STANDARD_DATETIME_FORMAT);
        }

        /// <summary>
        /// Converts a Trafodion Julian timestamp to local DateTime string with timezone and without microsecond
        /// </summary>
        /// <param name="aJulianTimestamp">a long value representing the julian timestamp</param>
        /// <param name="defaultValue">a string value to display if the julian value is not a positive integer</param>
        /// <param name="anOffset">Offset from UTC time</param>
        /// <param name="timeZoneName">the time zone name for the date time kind</param>
        /// <returns></returns>
        static public string FormattedJulianTimestamp(long aJulianTimestamp, string defaultValue, TimeSpan anOffset, string timeZoneName)
        {
            if (aJulianTimestamp <= 0L)
            {
                return defaultValue;
            }

            long millis_till_1970 = 210866760000009L;
            long milliTime = (aJulianTimestamp / 1000) - millis_till_1970;
            if (milliTime < 0)
            {
                return defaultValue;
            }
            long secondsSinceEpoch = milliTime / 1000;
            DateTime theDateTime = new DateTime();
            if (anOffset == TimeSpan.FromSeconds(0))
            {
                theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc).ToLocalTime();
                return theDateTime.ToString(DateTimeLongFormat12HourString) + " " + GetTimeZone(theDateTime);
            }
            else
            {
                theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Local);
                theDateTime = theDateTime - anOffset; //get back to utc.
                theDateTime = new DateTime(theDateTime.Ticks, DateTimeKind.Utc);
                theDateTime = theDateTime.ToLocalTime();
                return theDateTime.ToString(DateTimeLongFormat12HourString) + " " + GetTimeZone(theDateTime);
            }
        }

        /// <summary>
        /// Converts a Trafodion Julian timestamp to DateTime string
        /// </summary>
        /// <param name="aJulianTimestamp">a long value representing the julian timestamp,unit is microsec</param>
        /// <param name="defaultValue">a string value to display if the julian value is not a positive integer</param>
        /// <param name="anOffset">Offset from UTC time</param>
        /// <param name="toLocalTime">to localtime or servertime</param>
        /// <param name="withTimezone">is formatted to with timezone</param>
        /// <param name="datetimeFormateString">formate style</param>
        /// <returns>formatted string</returns>
        static public string FormattedJulianTimestamp(long aJulianTimestamp, string defaultValue, TimeSpan anOffset, bool toLocalTime, bool withTimezone, string datetimeFormateString)
        {
            if (aJulianTimestamp <= 0L)
            {
                return defaultValue;
            }

            long microsec_till_1970 = 210866760000009000L;
            long microTime = aJulianTimestamp - microsec_till_1970;
            if (microTime < 0)
            {
                return defaultValue;
            }
            DateTime theDateTime = new DateTime();
            if (anOffset == TimeSpan.FromSeconds(0))
            {
                theDateTime = new DateTime((microTime * 10) + NANO_SECOND_FROM_1970, DateTimeKind.Utc);
                if (toLocalTime)
                    theDateTime = theDateTime.ToLocalTime();
            }
            else
            {
                theDateTime = new DateTime((microTime * 10) + NANO_SECOND_FROM_1970, DateTimeKind.Local);
                if (toLocalTime)
                {
                    //get back to utc.
                    theDateTime = theDateTime - anOffset; 
                    theDateTime = new DateTime(theDateTime.Ticks, DateTimeKind.Utc);
                    theDateTime = theDateTime.ToLocalTime();
                }
            }
            return withTimezone ? (theDateTime.ToString(datetimeFormateString) + " " + GetTimeZone(theDateTime)) : theDateTime.ToString(datetimeFormateString);
        }

        /// <summary>
        /// Formats the datetime from seconds Since Epoch to a human readable form
        /// </summary>
        /// <param name="aTimeSecondsSinceEpoch"></param>
        /// <returns></returns>
        public static string GetFormattedTimeFromSecondsSinceEpoch(long aTimeSecondsSinceEpoch)
        {
            DateTime theDateTime = new DateTime((aTimeSecondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc).ToLocalTime();
            return theDateTime.ToString(DateTimeFormatString) + " " + GetTimeZone(theDateTime);
        }

        public static string GetFormattedTimeFromSecondsSinceEpoch(long aTimeSecondsSinceEpoch,bool isToLocal,bool withTimezone)
        {
            DateTime theDateTime  = new DateTime((aTimeSecondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc); ;
            if (isToLocal)
            {
                theDateTime = theDateTime.ToLocalTime();
            }

            return withTimezone ? theDateTime.ToString(DateTimeLongFormat24HourString) + " " + GetTimeZone(theDateTime) : theDateTime.ToString(DateTimeLongFormat24HourString);        

        }

        /// <summary>
        /// Formats the datetime to a human readable form with a input from Unix ls
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns>DateTime Short format used by Trafodion Database Manager</returns>
        static public string GetFormattedTimeFromUnixTimestamp(long aTime)
        {
            long secondsSinceEpoch = aTime / 1000;
            DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc).ToLocalTime();
            return theDateTime.ToString(DateTimeFormatString) + " " + GetTimeZone(theDateTime);
        }

        /// <summary>
        /// Formats the datetime to a human readable form with a input from Unix ls
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns>DateTime Short format used by Trafodion Database Manager</returns>
        static public string GetStandardFormattedTimeFromUnixTimestamp(long aTime)
        {
            long secondsSinceEpoch = aTime / 1000;
            DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc).ToLocalTime();
            return theDateTime.ToString(STANDARD_DATETIME_FORMAT);
        }

        /// <summary>
        /// Formats the datetime to a human readable form with a input from Unix ls
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns>DateTime Short format used by Trafodion Database Manager 3.0</returns>
        static public DateTime GetFormattedTimeFromUnixTimestampUTC(long aTime)
        {
            //long secondsSinceEpoch = aTime / 1000;
            //DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc).ToLocalTime();
            DateTime theDateTime = new DateTime((aTime * 10) + NANO_SECOND_FROM_1970, DateTimeKind.Utc);
            return theDateTime;
        }

        static public DateTime GetFormattedTimeFromUnixTimestampLCT(long aTime)
        {
            //DateTime date_time = Utilities.GetFormattedTimeFromUnixTimestampLCT((long)timestamp / 1000);
            //long secondsSinceEpoch = aTime / 1000;
            DateTime theDateTime = new DateTime((aTime * 10) + NANO_SECOND_FROM_1970, DateTimeKind.Local);
            return theDateTime;
        }

        /// <summary>
        /// Formats the datetime to a human readable form with a input from Unix ls
        /// </summary>
        /// <param name="aTime"></param>
        /// <returns>DateTime Short format used by Trafodion Database Manager</returns>
        static public DateTime GetDateTimeFromUnixTime(long aTime)
        {
            long secondsSinceEpoch = aTime/1000000;
            DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + NANO_SECOND_FROM_1970, DateTimeKind.Utc).ToLocalTime();
            return theDateTime;
        }

        static public string GetFormattedDateTime(DateTime dateTime)
        {
            return GetFormattedDateTime(dateTime, true);
        }

        static public string GetFormattedDateTime(DateTime dateTime, bool includeTimeZone)
        {
            return dateTime.ToString(DateTimeFormatString) + 
                (includeTimeZone ? " " + GetTimeZone(dateTime) : "");
        }

        static public string GetTrafodionSQLDateTime(DateTime dateTime)
        {
            string sqlDateTimeFormatString = "yyyy-MM-dd HH':'mm':'ss";

            return dateTime.ToString(sqlDateTimeFormatString);
        }

        static public string GetTrafodionSQLLongDateTimeWithTimezone(DateTime dateTime, bool is12HourFormat, bool isClientLCT, string aTimeZoneName)
        {
            //string sqlLongDateTimeFormatString = is12HourFormat ? "yyyy-MM-dd hh':'mm':'ss.FFFFFF tt" : "yyyy-MM-dd HH':'mm':'ss.FFFFFF tt";
            string timeZoneName = isClientLCT ? GetTimeZone(dateTime) : aTimeZoneName;

            //return dateTime.ToString(sqlLongDateTimeFormatString) + " " + timeZoneName;
            return string.Format("{0} {1}", GetTrafodionSQLLongDateTime(dateTime, is12HourFormat), timeZoneName);
        }
        static public string GetTrafodionSQLLongDateTime(DateTime dateTime, bool is12HourFormat)
        {
            string sqlLongDateTimeFormatString = is12HourFormat ? DateTimeLongFormat12HourString : DateTimeLongFormat24HourString;

            return dateTime.ToString(sqlLongDateTimeFormatString);
        }                

        /// <summary>
        /// Current date formatted to DateTimeFormatString
        /// </summary>
        static public string CurrentFormattedDateTime
        {
            get
            {
                DateTime current = DateTime.Now;
                return (current.ToString(DateTimeFormatString) + " " + GetTimeZone(current));
            }
        }

        /// <summary>
        /// Converts boolean to On/Off string
        /// </summary>
        /// <param name="aValue">boolean value true/false</param>
        /// <returns>On or Off string</returns>
        static public string OnOff(bool aValue)
        {
            return aValue ? "On" : "Off";
        }

        /// <summary>
        /// Converts Y/N to On/Off string
        /// </summary>
        /// <param name="aValue">A string Y or N</param>
        /// <returns>On or Off string</returns>
        static public string OnOff(string aValue)
        {
            return aValue.Equals("Y") ? "On" : "Off";
        }
        
        /// <summary>
        /// Converts a bool to True/False string
        /// </summary>
        /// <param name="aValue">a boolean value</param>
        /// <returns>True or False string</returns>
        static public string TrueFalse(bool aValue)
        {
            return aValue ? "True" : "False";
        }

        /// <summary>
        /// Converts a Y/N string to True/False string
        /// </summary>
        /// <param name="aValue">A string Y or N</param>
        /// <returns>True or False string</returns>
        static public string TrueFalse(string aValue)
        {
            return aValue.Equals("Y") ? "True" : "False";
        }

        /// <summary>
        /// Converts a bool to Yes/No string
        /// </summary>
        /// <param name="aValue">a boolean value</param>
        /// <returns>Yes or No string </returns>
        static public string YesNo(bool aValue)
        {
            return aValue ? "Yes" : "No";
        }

        /// <summary>
        /// Converts a Y/N string to Yes/No
        /// </summary>
        /// <param name="aValue">A string Y or N</param>
        /// <returns>Yes or No string</returns>
        static public string YesNo(string aValue)
        {
            return aValue.Equals("Y") ? "Yes" : "No";
        }

        /// <summary>
        /// Converts a bool to Enabled/Disabled string
        /// </summary>
        /// <param name="aValue">a boolean value</param>
        /// <returns>Enabled or Disable string</returns>
        static public string EnabledDisabled(bool aValue)
        {
            return aValue ? "Enabled" : "Disabled";
        }

        /// <summary>
        /// An enumeration to list the Size types
        /// </summary>
        public enum SizeType
        {
            /// <summary>
            /// Terabytes
            /// </summary>
            TB, 
            /// <summary>
            /// Gigabytes
            /// </summary>
            GB,
            /// <summary>
            /// Megabytes
            /// </summary>
            MB, 
            /// <summary>
            /// Kilobytes
            /// </summary>
            KB, 
            /// <summary>
            /// Bytes
            /// </summary>
            Bytes 
        }

        /// <summary>
        /// Formats a size value for the size type
        /// </summary>
        /// <param name="aSize">a long value</param>
        /// <param name="type">SizeType enum identifying the untis of input value</param>
        /// <returns>A formatted string that displays the size in the requested units</returns>
        public static string FormatSize(decimal aSize, SizeType type)
        {
            long kilobyte = 1024;
            decimal bytes = 0;

            switch (type)
            {
                case SizeType.TB:
                    {
                        bytes = aSize * kilobyte * kilobyte * kilobyte * kilobyte;
                        break;
                    }
                case SizeType.GB:
                    {
                        bytes = aSize * kilobyte * kilobyte * kilobyte;
                        break;
                    }
                case SizeType.MB:
                    {
                        bytes = aSize * kilobyte * kilobyte;
                        break;
                    }
                case SizeType.KB:
                    {
                        bytes = aSize * kilobyte;
                        break;
                    }
                case SizeType.Bytes:
                    {
                        string postfix = "Bytes";
                        if (aSize == 1)
                        {
                            postfix = "Byte";
                        }
                        return aSize.ToString("N0") + " " + postfix;
                    }
                default:
                    {
                        throw new ApplicationException("Invalid Format type.");
                    }
            }
            return FormatSize(bytes);
        }

        /// <summary>
        /// Formats a bytes value to the highest SizeType possible
        /// </summary>
        /// <param name="aSize">a long value specifying the bytes</param>
        /// <returns>A formatted string specifying the size in bytes/KB/MB/GB/TB</returns>
        public static string FormatSize(decimal aSize)
        {

            // Just make zero be 0 without units
            if (aSize == 0)
            {
                return "0";
            }

            // Make one byte be singluar
            if (aSize == 1)
            {
                return "1 Byte";
            }

            decimal theSize = aSize;
            long kilobyte = 1024;

            decimal[] theSizes =
            { 
                kilobyte * kilobyte * kilobyte * kilobyte,
                kilobyte * kilobyte * kilobyte,
                kilobyte * kilobyte,
                kilobyte,
                0 
            };

            String[] theSizeUnits =
            { 
                "TB", "GB", "MB", "KB", "Bytes" 
            };

            for (int theIndex = 0; ; theIndex++)
            {
                decimal theLimit = theSizes[theIndex];
                if (theSize >= theLimit)
                {
                    if (theLimit > 0)
                    {
                        theSize /= theLimit;
                    }
                    return theSize.ToString("#,##0.##") + " " + theSizeUnits[theIndex];
                }
            }
        }

        
        public static String FormatPercent(double aPercent)
        {
            return aPercent.ToString("F2", CultureInfo.InvariantCulture) + " %";
        }

        public static string GetProductName()
        {
            return Properties.Resources.ProductName;
        }

        /// <summary>
        /// Finds out  what is the language on the current system
        /// </summary>
        /// <returns>true - if current language is Englis </returns>
        public static bool CurrentLanguageIsEnglish()
        {
            // Just for reference--other variants of current Language are:
            // CultureInfo.CurrentCulture.Name
            // Thread.CurrentThread.CurrentCulture.Name

            String lang = Thread.CurrentThread.CurrentUICulture.Name;
            if (lang.Contains("en"))
            {
                // Some version of English--there are at least 14 of those! e.g. en-US 
                return true;
            }
            else
            {
                // current language is NOT English
                return false;
            }
        }

        /// <summary>
        /// Return the names of the HP Trafodion Odbc Windows Drivers installed on this PC
        /// </summary>
        /// <returns>the list of driver names</returns>
        static public List<string> GetHpTrafodionOdbcDriverNames()
        {
            return GetOdbcDriverNames("TRAF ODBC ");
        }

          /// <summary>
        /// Reports whether or not a Control is in a tab page and if so is NOT the currently 
        /// selected tab page in that tab control.
        /// <para/>
        /// THis is usefull when a control wants to take a rest if it can't be seen.
        /// </summary>
        /// <param name="aControl">the control</param>
        /// <returns>true if the control is in a tab page that is in a tab control and
        /// the tab page is not selected</returns>
        static public bool InUnselectedTabPage(Control aControl)
        {
            Control theControl = aControl;
            while (theControl != null)
            {
                if (theControl is TabPage)
                {
                    TabPage theTabPage = theControl as TabPage;
                    TabControl theTabControl = theControl.Parent as TabControl;
                    if ((theTabControl != null) && (theTabControl.SelectedTab != theTabPage))
                    {

                        // Parent is a tab control and we are not its selected tab
                        return true;

                    }
                }
                theControl = theControl.Parent;
            }
            return false;
        }

        /// <summary>
        /// Return the names of the Odbc Windows Drivers installed on this PC whose names
        /// begin with a given prefix.
        /// </summary>
        /// <param name="aPrefix">the prefix</param>
        /// <returns>the list of driver names</returns>
        static public List<string> GetOdbcDriverNames(string aPrefix)
        {
            List<string> theNames = new List<string>();

            RegistryKey theRegistryKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\ODBC\ODBCINST.INI\ODBC Drivers", false);
            if(theRegistryKey != null)
            {
                string[] theValueNames = theRegistryKey.GetValueNames();
                foreach (string theValueName in theValueNames)
                {
                    if ((aPrefix == null) || (aPrefix.Length == 0) || (theValueName.StartsWith(aPrefix)))
                    {
                        theNames.Add(theValueName);
                    }
                }
                theRegistryKey.Close();
            }

            return theNames;
        }
#region Client DSN

        /// <summary>
        /// Returns a string from the registry given the key
        /// </summary>
        /// <param name="dsKeyName"></param>
        /// <param name="name"></param>
        /// <returns></returns>
        public static string getODBCStringValue(String dsKeyName, String name)
        {
            return getStringValue(MICROSOFT_ODBC_INI_REGISTRY_KEY + "\\" + dsKeyName, name);
        }

        /// <summary>
        /// Gets the ODBC server version from the registry
        /// </summary>
        /// <param name="dsName"></param>
        /// <returns></returns>
        public static String getODBCDriverVersion(String dsName)
        {
            return getStringValue(MICROSOFT_ODBC_DSN_REGISTRY_KEY, dsName);
        }
        private static String getStringValue(String keyName, String name)
        {
            try
            {
                String value = getRegistryStringValue(Registry.CurrentUser, keyName, name);
                if (null == value)
                    value = getRegistryStringValue(Registry.LocalMachine, keyName, name);

                if (null != value)
                    return value;

            }
            catch (Exception e)
            {
            }

            return "";
        }
        private static RegistryKey getRegistryKey(RegistryKey regKey, String subKeyName)
        {
            if (null == regKey)
                return null;

            try
            {
                RegistryKey rk = regKey.OpenSubKey(subKeyName);
                return rk;
            }
            catch (Exception e)
            {
            }

            return null;
        }

        private static String getRegistryStringValue(RegistryKey regKey, String subKeyName, String name)
        {
            RegistryKey rk = getRegistryKey(regKey, subKeyName);
            if (null != rk)
            {
                try
                {
                    String value = (String)rk.GetValue(name);
                    rk.Close();
                    return value;
                }
                catch (Exception e)
                {
                }
            }

            return null;
        }

#endregion

        /// <summary>
        ///  This function will launch a runnable type file.  If it is not found then it will 
        ///  prompt the user for the filename.  If the user types in a new filename that runs, then that
        ///  filename can be used by the calling function to save to settings for later reference. 
        /// </summary>
        /// <param name="fileName">Fully qualified name of file to run.</param>
        /// <param name="args">arguments for the process</param>
        /// <param name="programName">The name of the program.  This will get displayed in the error box
        /// if it is not found.  Can be left blank of Null</param>
        static public bool LaunchProcess(ref String fileName, String args, String programName, out Process startedProcess)
        {
            startedProcess = null;

            if (String.IsNullOrEmpty(fileName))
            {
                return false;
            }

            if (String.IsNullOrEmpty(programName))
            {
                programName = Properties.Resources.Program;
            }

            OpenFileDialog openFileDialog1 = new OpenFileDialog();


            // These are the Win32 error code for file not found or access denied.
            const int ERROR_FILE_NOT_FOUND = 2;
            const int ERROR_ACCESS_DENIED = 5;

            ProcessStartInfo startInfo = new ProcessStartInfo();
            bool isStarted = false;

            int attempt = 0;
            while (!isStarted && attempt <2)
            {
                try
                {
                    attempt++;
                    startInfo = new ProcessStartInfo(fileName);

                    if (!String.IsNullOrEmpty(args))
                    {
                        startInfo.Arguments = @args;
                    }

                    isStarted = true;
                    startInfo.CreateNoWindow = true;
                    Process process = Process.Start(startInfo);


                    Thread.Sleep(500);

                    startedProcess = process;

                }
                catch (System.ComponentModel.Win32Exception ex)
                {
                    isStarted = false;
                    if (ex.NativeErrorCode == ERROR_FILE_NOT_FOUND)
                    {
                        DialogResult results = MessageBox.Show(Utilities.GetForegroundControl(), String.Format(Properties.Resources.BrowseForFile, programName),
                            String.Format(Properties.Resources.FileNotFound, programName), MessageBoxButtons.YesNo, MessageBoxIcon.Question);


                        if (results == DialogResult.Yes)
                        {
                            try
                            {
                                openFileDialog1.FileName = Path.GetFileName(fileName);
                                //Setup an initial directory to display in openfiledialog if possible.
                                if (openFileDialog1.InitialDirectory == "")
                                {
                                    openFileDialog1.InitialDirectory = Path.GetDirectoryName(fileName);
                                }
                            }
                            catch (Exception ex2)
                            {
                                //Not a big deal if this gets hit. Used to keep users from seeing an
                                //exception.  
#if DEBUG
                                System.Console.WriteLine(ex2.Message);
#endif
                            }

                            //Could not find the given directory so just default it to C:
                            if (!Directory.Exists(openFileDialog1.InitialDirectory))
                            {
                                openFileDialog1.InitialDirectory = ("C:");
                            }


                            //Set up some file filters for better usability.
                            openFileDialog1.Filter = Properties.Resources.FileFilter;

                            if (openFileDialog1.ShowDialog() == DialogResult.OK)
                            {
                                startInfo.FileName = openFileDialog1.FileName;
                                fileName = openFileDialog1.FileName;

                            }
                            else
                            {
                                return isStarted;
                            }
                        }
                        else
                        {
                            return isStarted;
                        }
                    }
                    else
                    {
                        if (ex.NativeErrorCode == ERROR_ACCESS_DENIED)
                        {

                            MessageBox.Show(Utilities.GetForegroundControl(), Properties.Resources.NoPermission, Properties.Resources.AccessDenied, MessageBoxButtons.OK, MessageBoxIcon.Warning);
                            return isStarted;
                        }
                        else
                        {
                            return false;
                        }

                    }

                }
                catch (Exception ex1)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), ex1.Message, "error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            openFileDialog1.Dispose();
            return isStarted;
        }

        /// <summary>
        /// Removes leading and trailing parenthesis in the string
        /// </summary>
        /// <param name="value"></param>
        /// <returns></returns>
        static public string RemoveLeadingAndTailingParenthesis(string value)
        {
            value.Trim();
            if (value[0] == '(')
                value = value.Substring(1);
            if (value[value.Length - 1] == ')')
                value = value.Substring(0, value.Length - 1);
            return value;
        }


        /// <summary>
        /// Last saved location of a FileDialog
        /// </summary>
        /// <returns>directory name where the FileDialog saved last file in his session or previous session</returns>
        static public string FileDialogLocation()
        {
            //string lastSavedLocation = Persistence.HomeDirectory;
            LastSavedLocation location;
            try
            {
                location = Persistence.Get(LAST_FILE_LOCATION_KEY) as LastSavedLocation;
                if (location == null || location.Path.Trim().Equals(""))
                {
                    return Persistence.HomeDirectory;
                }
            }
            catch(Exception)
            {
                    return Persistence.HomeDirectory;
            }
            return location.Path;
        }


        /// <summary>
        /// Last saved location of a FileDialog
        /// </summary>
        /// <returns>directory name where the FileDialog saved last file in his session or previous session</returns>
        static public void FileDialogLocation(string location)
        {
            LastSavedLocation savedLocation = new LastSavedLocation();
            savedLocation.Path = location;
            try
            {
                Persistence.Put(LAST_FILE_LOCATION_KEY, savedLocation);
            }
            catch (Exception)
            {
            }
        }

        /// <summary>
        /// Returns the current foreground control
        /// </summary>
        /// <returns></returns>
        static public Control GetForegroundControl()
        {
            Control foregroundControl = null;
            IntPtr handle = GetForegroundWindow();
            if (handle != null)
            {
                foregroundControl = Control.FromHandle(handle);
            }
            return foregroundControl;
        }

        static public bool CheckOdbcDriverExists()
        {
            List<string> theValueNames = Utilities.GetHpTrafodionOdbcDriverNames();
            bool installed = theValueNames.Count > 0;
            if (!installed)
            {
                Connections.Controls.NoHpOdbcDriversDialog theNoHpOdbcDriversDialog = new Connections.Controls.NoHpOdbcDriversDialog();
                theNoHpOdbcDriversDialog.ShowDialog();
            }
            return installed;
        }

        static public ArrayList GetSortObject(TenTec.Windows.iGridLib.iGrid gridSorted)
        {
            if (null == gridSorted)
                return null;

            ArrayList sortObject = new ArrayList();
            for (int i = 0; i < gridSorted.SortObject.Count; i++)
            {
                TenTec.Windows.iGridLib.iGSortItem sortItemToClone = ((TenTec.Windows.iGridLib.iGSortItem)gridSorted.SortObject[i]);

                IGridSortObject sortItem = new IGridSortObject(sortItemToClone.ColIndex, sortItemToClone.SortOrder, sortItemToClone.SortType);
                sortObject.Add(sortItem);
            }
            return sortObject;
        }

        static public void ApplySort(TenTec.Windows.iGridLib.iGrid gridToSort, ArrayList savedSort)
        {
            if (null == gridToSort)
                return;

            gridToSort.SortObject.Clear();
            for (int i = 0; i < savedSort.Count; i++)
            {
                IGridSortObject sortItem = ((IGridSortObject)savedSort[i]);
                gridToSort.SortObject.Add(sortItem.ColIndex, sortItem.SortOrder, sortItem.SortType);
            }

            gridToSort.Sort();
        }


        static public string GetTimeZone(DateTime dateTime)
        {
            System.TimeZone tz = System.TimeZone.CurrentTimeZone;
            string timeZoneName = tz.IsDaylightSavingTime(dateTime) ? tz.DaylightName : tz.StandardName;
            return GetTimeZoneAbbreviation(timeZoneName);
        }

        static public string GetTimeZoneAbbreviation(string timeZoneName)
        {
            string abbreviation = "";
            foreach (char currentChar in timeZoneName.ToCharArray())
            {
                if (char.IsUpper(currentChar))
                {
                    abbreviation = abbreviation + Char.ToString(currentChar);
                }
            }
            return abbreviation;

        }

        public static void FindSize(Object anObject)
        {
            long lSize = 0;

            try
            {
                System.IO.MemoryStream stream = new System.IO.MemoryStream();
                BinaryFormatter objFormatter = new BinaryFormatter();
                objFormatter.Serialize(stream, anObject);
                lSize = stream.Length;
            }
            catch (Exception excp)
            {
                Console.WriteLine("Error: " + excp.Message);
            }

        }

        /// <summary>
        /// Break an ansi name into name parts
        /// </summary>
        /// <param name="anAnsiName"></param>
        /// <returns></returns>
        public static string[] CrackSQLAnsiName(string anAnsiName)
        {
            int theAnsiLength = anAnsiName.Length;
            bool inQuotes = false;
            int theBeginOffset = 0;

            int theResultPart = 0;

            string[] theResult = new string[3];

            for (int theCurrentOffset = 0; theCurrentOffset < theAnsiLength; theCurrentOffset++)
            {
                char theCharacter = anAnsiName[theCurrentOffset];

                switch (theCharacter)
                {
                    case '"':
                        {
                            inQuotes = !inQuotes;
                            break;
                        }
                    case '.':
                        {
                            if (!inQuotes)
                            {
                                theResult[theResultPart] = anAnsiName.Substring(theBeginOffset, theCurrentOffset - theBeginOffset);
                                theResultPart++;
                                theBeginOffset = theCurrentOffset + 1;
                            }
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }

            }

            theResult[theResultPart] = anAnsiName.Substring(theBeginOffset);
            theResultPart++;

            return theResult;
        }

        public static string ExternalForm(string anInternalName)
        {
            return ExternalForm(anInternalName, false);
        }

        /// <summary>
        /// Returns a SQL name's external form.  
        /// </summary>
        /// <param name="anInternalName"></param>
        /// <returns></returns>
        public static string ExternalForm(string anInternalName, bool includeLowerCase)
        {
            string theName = anInternalName.TrimEnd();

            // May be empty?
            if (theName.Length == 0)
            {
                return "";
            }

            string pattern = includeLowerCase ? "^[A-Za-z0-9_]+$" : "^[A-Z0-9_]+$";
            // If it contains specials, it needs to be delimited.
            if (Regex.IsMatch(theName, pattern))
            {

                // No specials, it's itself
                return theName;

            }

            if ((anInternalName.StartsWith("\"")) && (anInternalName.EndsWith("\"")))
            {
                //it has been delimited.
                return theName;
            }
            else
            {
                theName = theName.Replace("\"", "\"\"");
            }            

            // It has specials; delimit it.
            return "\"" + theName + "\"";

        }

        /// <summary>
        /// Returns a SQL name's internal form.  
        /// </summary>
        /// <param name="anExternalName"></param>
        /// <returns></returns>
        public static string InternalForm(string anExternalName)
        {
            int theLength = anExternalName.Length;

            if (theLength > 1) 
            {
                if ((anExternalName.StartsWith("\"")) && (anExternalName.EndsWith("\"")))
                {
                    return anExternalName.Substring(1, theLength - 2);
                }
                else
                {
                    if(anExternalName.StartsWith("\""))
                    {
                        return anExternalName.Substring(1);
                    }
                }
            }
            return anExternalName.ToUpper();
        }

        /// <summary>
        /// Gives the external format of a user name or role name
        /// </summary>
        /// <param name="anInternalName"></param>
        /// <returns></returns>
        public static string ExternalUserName(string anInternalName)
        {
            return ExternalForm(anInternalName, true);
        }

        /// <summary>
        /// Gets the percent of available virtual memory
        /// </summary>
        /// <returns></returns>
        public static double GetPercentAvailableVirtualMemory()
        {
            MEMORYSTATUSEX ms = new MEMORYSTATUSEX();
            ms.dwLength = (uint)Marshal.SizeOf(ms);
            GlobalMemoryStatusEx(ref ms);

            ulong totalVirs = ms.ullTotalVirtual;
            ulong availVir = ms.ullAvailVirtual;
            return availVir * 100.0 / totalVirs;
        }

        public static ulong GetCurrentFreeVirtualMemory()
        {
            MEMORYSTATUSEX ms = new MEMORYSTATUSEX();
            ms.dwLength = (uint)Marshal.SizeOf(ms);
            GlobalMemoryStatusEx(ref ms);

            return ms.ullAvailVirtual;
        }

        public static ulong GetCurrentUsedVirtualMemory()
        {
            MEMORYSTATUSEX ms = new MEMORYSTATUSEX();
            ms.dwLength = (uint)Marshal.SizeOf(ms);
            GlobalMemoryStatusEx(ref ms);

            return ms.ullTotalVirtual - ms.ullAvailVirtual;
        }

        public static MEMORYSTATUSEX GetCurrentMemoryStatus()
        {
            MEMORYSTATUSEX ms = new MEMORYSTATUSEX();
            ms.dwLength = (uint)Marshal.SizeOf(ms);
            GlobalMemoryStatusEx(ref ms);
            return ms;
        }

        /// <summary>
        /// A text wrapper, which breaks a string into lines of the specified length.  This useful for 
        /// creating tool tips. 
        /// </summary>
        /// <param name="text"></param>
        /// <param name="maxLength"></param>
        /// <returns></returns>
        public static string TrafodionTextWrapper(string text, int maxLength)
        {
            string[] Words = text.Split(' ');
            string currentLine = "";
            int currentLineLength = 0;
            string textLinesString = "";

            foreach (string currentWord in Words)
            {
                if (currentWord.Length > 0)
                {
                    if (currentLineLength + currentWord.Length + 1 < maxLength)
                    {
                        currentLine += " " + currentWord;
                        currentLineLength += (currentWord.Length + 1);
                    }
                    else
                    {
                        // Append to the output text
                        textLinesString += currentLine + System.Environment.NewLine;

                        // Create a new line. 
                        currentLine = currentWord;
                        currentLineLength = currentWord.Length;
                    }
                }
            }

            if (!string.IsNullOrEmpty(currentLine))
            {
                textLinesString += currentLine;
            }

            return textLinesString.Trim();
        }

        public static String formatTimeSpan(TimeSpan myTimeSpan)
        {
            return string.Format("{0:00}:{1:00}:{2:00}.{3:000}", (int)myTimeSpan.TotalHours, myTimeSpan.Minutes, myTimeSpan.Seconds, myTimeSpan.Milliseconds);
        }

        /// <summary>
        /// To execute a non query on an odbc command.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aTraceOption"></param>
        /// <param name="anArea"></param>
        /// <param name="aSubArea"></param>
        /// <param name="aTimeout"></param>
        /// <returns></returns>
        public static int ExecuteNonQuery(OdbcCommand anOpenCommand, TraceOptions.TraceOption aTraceOption, TraceOptions.TraceArea anArea, string aSubArea, bool aTimeout)
        {
            if (aTimeout)
            {
                anOpenCommand.CommandTimeout = GeneralOptions.GetOptions().CommandTimeOut;
            }
            else
            {
                anOpenCommand.CommandTimeout = 0;
            }

            Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("ExecuteNonQuery: {0}", anOpenCommand.CommandText));
            if (anOpenCommand.Parameters.Count > 0)
            {
                foreach (OdbcParameter param in anOpenCommand.Parameters)
                {
                    Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("\tParam {0}, value = {1}", param.ParameterName, param.Value));
                }
            }

            try
            {
                DateTime startTime = DateTime.Now;
                int result = anOpenCommand.ExecuteNonQuery();
                DateTime endtime = DateTime.Now;
                TimeSpan duration = endtime - startTime;
                Logger.OutputToLog(TraceOptions.TraceOption.PERF, anArea, aSubArea, String.Format("\t{0}\t{1}", anOpenCommand.CommandText, formatTimeSpan(duration)));
                return result;
            }
            catch (Exception ex)
            {
                String shortCommandText = anOpenCommand.CommandText.Length > 10 ? anOpenCommand.CommandText.Substring(0, 10) : anOpenCommand.CommandText;
                Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("ExecuteNonQuery: {0} failed. Exception: {1}", shortCommandText, ex.Message));
                throw ex;
            }
        }

        public static int ExecuteNonQuery(OdbcCommand anOpenCommand, TraceOptions.TraceOption aTraceOption, TraceOptions.TraceArea anArea, string aSubArea, bool aTimeout, out DataTable warningsTable)
        {
            if (aTimeout)
            {
                anOpenCommand.CommandTimeout = GeneralOptions.GetOptions().CommandTimeOut;
            }
            else
            {
                anOpenCommand.CommandTimeout = 0;
            }

            Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("ExecuteNonQuery: {0}", anOpenCommand.CommandText));
            if (anOpenCommand.Parameters.Count > 0)
            {
                foreach (OdbcParameter param in anOpenCommand.Parameters)
                {
                    Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("\tParam {0}, value = {1}", param.ParameterName, param.Value));
                }
            }

            SQLWarningHandler sqlWarningHandler = new SQLWarningHandler(anOpenCommand.Connection);

            try
            {
                DateTime startTime = DateTime.Now;
                int result = anOpenCommand.ExecuteNonQuery();
                DateTime endtime = DateTime.Now;
                TimeSpan duration = endtime - startTime;
                Logger.OutputToLog(TraceOptions.TraceOption.PERF, anArea, aSubArea, String.Format("\t{0}\t{1}", anOpenCommand.CommandText, formatTimeSpan(duration)));
                return result;
            }
            catch (Exception ex)
            {
                String shortCommandText = anOpenCommand.CommandText.Length > 10 ? anOpenCommand.CommandText.Substring(0, 10) : anOpenCommand.CommandText;
                Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("ExecuteNonQuery: {0} failed. Exception: {1}", shortCommandText, ex.Message));
                throw ex;
            }
            finally
            {
                warningsTable = sqlWarningHandler.WarningsDataTable;
                sqlWarningHandler.UnRegister(anOpenCommand.Connection);
            }
        }
        /// <summary>
        /// To execute a query on an odbc command.
        /// </summary>
        /// <param name="anOpenCommand"></param>
        /// <param name="aTraceOption"></param>
        /// <param name="anArea"></param>
        /// <param name="aSubArea"></param>
        /// <param name="aTimeout"></param>
        /// <returns></returns>
        public static OdbcDataReader ExecuteReader(OdbcCommand anOpenCommand, TraceOptions.TraceOption aTraceOption, TraceOptions.TraceArea anArea, string aSubArea, bool aTimeout)
        {
            if (aTimeout)
            {
                anOpenCommand.CommandTimeout = GeneralOptions.GetOptions().CommandTimeOut;
            }
            else
            {
                anOpenCommand.CommandTimeout = 0;
            }

            Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("ExecuteReader: {0}", anOpenCommand.CommandText));
            if (anOpenCommand.Parameters.Count > 0)
            {
                foreach (OdbcParameter param in anOpenCommand.Parameters)
                {
                    Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("\tParam {0}, value = {1}", param.ParameterName, param.Value));
                }
            }

            try
            {
                DateTime startTime = DateTime.Now;
                OdbcDataReader reader = anOpenCommand.ExecuteReader();
                DateTime endtime = DateTime.Now;
                TimeSpan duration = endtime - startTime;
                Logger.OutputToLog(TraceOptions.TraceOption.PERF, anArea, aSubArea, String.Format("\t{0}\t{1}", anOpenCommand.CommandText, formatTimeSpan(duration)));
                return reader;
            }
            catch (Exception ex)
            {
                String shortCommandText = anOpenCommand.CommandText.Length > 10 ? anOpenCommand.CommandText.Substring(0, 10) : anOpenCommand.CommandText;
                Logger.OutputToLog(aTraceOption, anArea, aSubArea, String.Format("ExecuteReader: {0} failed. Exception: {1}", shortCommandText, ex.Message));
                throw ex;
            }
        }

        /// <summary>
        /// Copy properties from one object to another object of the same type
        /// </summary>
        /// <typeparam name="T">Object type</typeparam>
        /// <param name="source">source object</param>
        /// <param name="target">target object</param>
        public static void CopyProperties<T>(T source, ref T target)
        {
            PropertyInfo[] properties = typeof(T).GetProperties();
            foreach (PropertyInfo property in properties)
            {
                if (property.CanWrite)
                {
                    property.SetValue(target, property.GetValue(source, null), null);
                }
            }
        }

        /// <summary>
        /// Disable a control and all its child controls.
        /// </summary>
        /// <param name="control"></param>
        public static void DisableControl(Control control)
        {
            if (control != null)
            {
                control.Enabled = false;
                foreach (Control childControl in control.Controls)
                {
                    DisableControl(childControl);
                }
            }
        }

        /**
		 *  <summary>
		 *  Clones the columns from the specified DataTable ensuring that the column names
		 *  and datatypes match and returns an empty cloned DataTable.
		 *  </summary>
		 *
		 *  <param name="theTable">Table to be cloned</param>
		 *  <remarks>Since: NPA2.0</remarks>
		 */
        public static DataTable cloneDataTableColumns(DataTable theTable)
        {
            DataTable lore = new DataTable();
            try
            {
                foreach (DataColumn col in theTable.Columns)
                {
                    DataColumn colCopy = new DataColumn(col.ColumnName, col.DataType,
                                                        col.Expression, col.ColumnMapping);
                    lore.Columns.Add(colCopy);
                }

            }
            catch (Exception e)
            {
                if (Logger.IsTracingEnabled)
                    Logger.OutputToLog("Utilities cloneDataTableColumns() error = " + e.Message +
                                                "\n\t Doing this via Clone() and Clear().");

                lore = theTable.Clone();
                lore.Rows.Clear();
            }

            return lore;  //  A clone of Data!! :^)
        }

        /// <summary>
        /// Determines if the native OS is 32 bit or 64 bit
        /// </summary>
        /// <returns></returns>
        public static bool Is64bitOS()
        {
            SystemInfoNative native = new SystemInfoNative();

            GetNativeSystemInfo(ref native);

            return (native.processorArchitecture == 6 || native.processorArchitecture == 9);
        }

        public static bool IsValidServerFilePath(string path)
        {
            return Regex.IsMatch(path.Trim(), REGEX_SERVER_FILE_PATH);
        }

        public static bool IsValidServerFileName(string fileName)
        {
            return Regex.IsMatch(fileName.Trim(), REGEX_SERVER_FILE_NAME);
        }

        /// <summary>
        /// Launch a managed window without persistences
        /// </summary>
        /// <param name="aTitle"></param>
        /// <param name="aControl"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aSize"></param>
        /// <param name="aCheckExisting"></param>
        public static bool LaunchManagedWindow(string aTitle, Control aControl, ConnectionDefinition aConnectionDefinition, Size aSize, bool aCheckExisting)
        {
            return LaunchManagedWindow(aTitle, aControl, false, aConnectionDefinition, aSize, aCheckExisting);
        }

	    /// <summary>
        /// Launch a managed window with the persistence option, return true/false if a new window is created.
        /// </summary>
        /// <param name="aTitle"></param>
        /// <param name="aControl"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aSize"></param>
        public static bool LaunchManagedWindow(string aTitle, Control aControl, bool aNeedsPersistence, ConnectionDefinition aConnectionDefinition, Size aSize, bool aCheckExisting)
        {
            try
            {
                string systemIdentifier = (aConnectionDefinition != null) ? aConnectionDefinition.Name + " : " : "";
                string windowTitle = TrafodionForm.TitlePrefix + systemIdentifier + aTitle;
                if (aCheckExisting && WindowsManager.Exists(windowTitle))
                {
                    WindowsManager.Restore(windowTitle);
                    WindowsManager.BringToFront(windowTitle);
                    return false;
                }
                else
                {
                    ManagedWindow form = (ManagedWindow)WindowsManager.PutInWindow(aSize, aControl, aTitle, aNeedsPersistence, aConnectionDefinition);
                    return true;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Exception: " + ex.Message, aTitle, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            return false;
        }

        public static bool BringExistingWindowToFront(string title, ConnectionDefinition connectionDefinition)
        {            
            string systemIdentifier = (connectionDefinition != null) ? connectionDefinition.Name + " : " : "";
            string windowTitle = TrafodionForm.TitlePrefix + systemIdentifier + title;
            bool isExisting = WindowsManager.Exists(windowTitle);
            if (isExisting)
            {
                WindowsManager.Restore(windowTitle);
                WindowsManager.BringToFront(windowTitle);
            }
            return isExisting;
        }

        /// <summary>
        /// A simple way of validating a file/path name
        /// </summary>
        /// <param name="aFileName"></param>
        /// <returns></returns>
        public static bool IsValidFileName(string aFileName)
        {
            bool valid = false; 
            try 
            {   
                new System.IO.FileInfo(aFileName);   
                valid = true; 
            } 
            catch (Exception)
            {}

            return valid;
        }

        public static bool ContainOnlyAlphaNumericCharacters(string str)
        {
            foreach (char c in str) {
                if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
                {
                    // is alpha numeric
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// check whether there are duplicate values in a list 
        /// </summary>
        /// <param name="strArray"></param>
        /// <returns></returns>
        public static bool containsDuplicateString(List<string> strArray)
        {
            List<string> listTemp = new List<string>();
            foreach (string str1 in strArray)
            {
                if (listTemp.Contains(str1))
                {
                    return true;
                }
                else
                {
                    listTemp.Add(str1);
                }
            }
            return false;
        }

        /// <summary>
        /// Check Live Feed Driver installed
        /// </summary>
        /// <returns>true: installed; false: not installed</returns>
        public static bool IsInstalledLiveFeedDriver()
        {
            bool isthere = false;
            RegistryKey theRegistryKey = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Hewlett-Packard\HP Live Feed", false);
            if (theRegistryKey != null)
            {
                isthere = true;
                theRegistryKey.Close();
            }

            return isthere;
        }
  

        public static T CloneDataGridView<T>(T sourceDataGridView) where T : TrafodionDataGridView, new()
        {
            T clonedDataGridView = new T();
            foreach (DataGridViewColumn column in sourceDataGridView.Columns)
            {
                clonedDataGridView.Columns.Add((DataGridViewColumn)column.Clone());
            }
            foreach (DataGridViewRow row in sourceDataGridView.Rows)
            {
                DataGridViewRow clonedRow = (DataGridViewRow)row.Clone();
                for (Int32 index = 0; index < row.Cells.Count; index++)
                {
                    clonedRow.Cells[index].Value = row.Cells[index].Value;
                }
                clonedDataGridView.Rows.Add(clonedRow);
            }
            return clonedDataGridView;
        }

        /// <summary>
        /// Call an action in global lock
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="action"></param>
        /// <param name="param"></param>
        public static void RunInGlobalLock<T>(Action<T> action, T param)
        {
            // Get application GUID as defined in AssemblyInfo.cs
            string appGuid = ((GuidAttribute)Assembly.GetEntryAssembly().GetCustomAttributes(typeof(GuidAttribute), false).GetValue(0)).Value.ToString();

            // Unique id for global mutex - Global prefix means it is global to the machine, and mutex is visible in all terminal server sessions
            //                            - Local prefix means it is visible only in the terminal server session where it was created. 
            //                              In this case, a separate mutex with the same name can exist in each of the other terminal server sessions on the server
            string mutexId = string.Format("Global\\{{{0}}}", appGuid);

            using (Mutex mutex = new Mutex(false, mutexId))
            {
                // Set up security for multi-user usage
                MutexAccessRule allowEveryoneRule = new MutexAccessRule(new SecurityIdentifier(WellKnownSidType.WorldSid, null), MutexRights.FullControl, AccessControlType.Allow);
                MutexSecurity securitySettings = new MutexSecurity();
                securitySettings.AddAccessRule(allowEveryoneRule);
                mutex.SetAccessControl(securitySettings);

                bool hasHandle = false;
                try
                {
                    try
                    {
                        // Wait 3 minutes
                        hasHandle = mutex.WaitOne(3*60*1000);
                        if (hasHandle == false) throw new TimeoutException("Timeout waiting for exclusive access");
                    }
                    catch (AbandonedMutexException)
                    {
                        // Log the fact the mutex was abandoned in another process, it will still get aquired
                        hasHandle = true;
                    }
                                        
                    action(param);
                }
                finally
                {
                    // edited by acidzombie24, added if statemnet
                    if (hasHandle)
                        mutex.ReleaseMutex();
                }
            }
        }

        /// <summary>
        /// Convert a DataReader to DataTable
        /// </summary>
        /// <param name="reader"></param>
        /// <returns>DataTable</returns>
        public static DataTable GetDataTableForReader(OdbcDataReader reader)
        {
            DataTable dataTable = new DataTable();
            // Add columns to the result data table
            int theFieldCount = reader.FieldCount;
            for (int colNum = 0; colNum < theFieldCount; colNum++)
            {
                try
                {
                    string colName = reader.GetName(colNum);
                    dataTable.Columns.Add(colName, reader.GetFieldType(colNum));
                }
                catch (Exception ex)
                {
                    dataTable.Columns.Add(new DataColumn());
                }
            }


            while (reader.Read())
            {
                //worker.ReportProgress();
                object[] theCurrRow = new object[reader.FieldCount];
                for (int currField = 0; currField < reader.FieldCount; currField++)
                {
                    try
                    {
                        theCurrRow[currField] = reader.GetValue(currField);
                    }
                    catch (Exception ex)
                    {
                        try
                        {
                            theCurrRow[currField] = reader.GetString(currField);
                        }
                        catch (Exception e1)
                        { }

                    }
                }

                //Add rows to the result data table
                dataTable.Rows.Add(theCurrRow);
            }
            reader.Close();
            return dataTable;
        }

        /// <summary>
        /// Set the location pop-up window to center of its parent
        /// </summary>
        /// <param name="popupWindow"></param>
        public static void SetCenterParent(Form popupWindow)
        {
            Form parent = Form.ActiveForm;
            popupWindow.Load += new EventHandler
                (
                    (sender, e) =>
                    {
                        if (parent != null)
                        {
                            int centerX = parent.Left + parent.Width / 2;
                            int centerY = parent.Top + parent.Height / 2;
                            popupWindow.Location = new Point(centerX - popupWindow.Width / 2, centerY - popupWindow.Height / 2);
                        }
                    }
                );
        }

        #region Play Sound

        [DllImport("winmm.dll", SetLastError = true)]
        static extern bool PlaySound(string pszSound, UIntPtr hmod, uint fdwSound);

        [Flags]
        public enum SoundFlags
        {
            /// <summary>play synchronously (default)</summary>
            SND_SYNC = 0x0000,
            /// <summary>play asynchronously</summary>
            SND_ASYNC = 0x0001,
            /// <summary>silence (!default) if sound not found</summary>
            SND_NODEFAULT = 0x0002,
            /// <summary>pszSound points to a memory file</summary>
            SND_MEMORY = 0x0004,
            /// <summary>loop the sound until next sndPlaySound</summary>
            SND_LOOP = 0x0008,
            /// <summary>don¡¯t stop any currently playing sound</summary>
            SND_NOSTOP = 0x0010,
            /// <summary>Stop Playing Wave</summary>
            SND_PURGE = 0x40,
            /// <summary>don¡¯t wait if the driver is busy</summary>
            SND_NOWAIT = 0x00002000,
            /// <summary>name is a registry alias</summary>
            SND_ALIAS = 0x00010000,
            /// <summary>alias is a predefined id</summary>
            SND_ALIAS_ID = 0x00110000,
            /// <summary>name is file name</summary>
            SND_FILENAME = 0x00020000,
            /// <summary>name is resource name or atom</summary>
            SND_RESOURCE = 0x00040004
        }

        public static void PlaySound(string soundFile)
        {
            PlaySound(soundFile, UIntPtr.Zero, (uint)(SoundFlags.SND_FILENAME | SoundFlags.SND_ASYNC));
        }

        public static void PlaySoundSync(string soundFile)
        {
            PlaySound(soundFile, UIntPtr.Zero, (uint)(SoundFlags.SND_FILENAME | SoundFlags.SND_SYNC));
        }

        #endregion Play Sound

        #region Flash on Task Bar

        // To support flashing.
        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool FlashWindowEx(ref FLASHWINFO pwfi);

        //Flash both the window caption and taskbar button.
        //This is equivalent to setting the FLASHW_CAPTION | FLASHW_TRAY flags. 
        public const UInt32 FLASHW_ALL = 3;

        // Flash continuously until the window comes to the foreground. 
        public const UInt32 FLASHW_TIMERNOFG = 12;

        [StructLayout(LayoutKind.Sequential)]
        public struct FLASHWINFO
        {
            public UInt32 cbSize;
            public IntPtr hwnd;
            public UInt32 dwFlags;
            public UInt32 uCount;
            public UInt32 dwTimeout;
        }

        // Do the flashing - this does not involve a raincoat.
        public static bool FlashWindowEx(Form form)
        {
            IntPtr hWnd = form.Handle;
            FLASHWINFO fInfo = new FLASHWINFO();

            fInfo.cbSize = Convert.ToUInt32(Marshal.SizeOf(fInfo));
            fInfo.hwnd = hWnd;
            fInfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            fInfo.uCount = UInt32.MaxValue;
            fInfo.dwTimeout = 0;

            return FlashWindowEx(ref fInfo);
        }

        #endregion Flash on Task Bar
    }
}
