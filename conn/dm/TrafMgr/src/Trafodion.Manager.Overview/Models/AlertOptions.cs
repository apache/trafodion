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

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Model for the Alert widget options. 
    /// This object is persisted across TrafodionManager sessions and reloaded whenever the Alerts widget is recreated
    /// </summary>
    [Serializable]
    public class AlertOptions
    {
        #region private member variables

        private bool _fetchOpenAlertsOnly = true;
        //private Dictionary<string, AlertType> _severityMappings = null;

        ReportParameter _startTimeParameter = null;
        ReportParameter _endTimeParameter = null;
        string _timeRangeString = null;

        #endregion private member variables

        #region Public Properties

        public string TimeRangeString
        {
            get 
            {
                if (_timeRangeString == null)
                {
                    _timeRangeString = TimeRangeInputBase.LAST_30_DAYS;
                }
                return _timeRangeString; 
            }
            set { _timeRangeString = value; }
        }
        
        public ReportParameter StartTimeParameter
        {
            get 
            {
                if (_startTimeParameter == null)
                {
                    _startTimeParameter = new ReportParameter();
                    _startTimeParameter.ParamName = ReportParameterProcessorBase.FROM_TIME;
                    DateTime start = DateTime.Now;
                    start = start.AddDays(-29);
                    start = new DateTime(start.Year, start.Month, start.Day, 0, 0, 0);
                    _startTimeParameter.Value = start;
                }

                return _startTimeParameter; 
            }
            set { _startTimeParameter = value; }
        }

        public ReportParameter EndTimeParameter
        {
            get 
            {
                if (_endTimeParameter == null)
                {
                    _endTimeParameter = new ReportParameter();
                    _endTimeParameter.ParamName = ReportParameterProcessorBase.TO_TIME;
                    _endTimeParameter.Value = DateTime.Now;
                }
                return _endTimeParameter; 
            }
            set { _endTimeParameter = value; }
        }

        /// <summary>
        /// Enum to indicate the alert level
        /// </summary>
        public enum AlertType { EMERGENCY = 0, IMMEDIATE = 1, CRITICAL = 2, ERROR =3 };

        ///// <summary>
        ///// Dictionary of severity to alert level mappings
        ///// </summary>
        //public Dictionary<string, AlertType> SeverityMappings
        //{
        //    get { return _severityMappings; }
        //}

        /// <summary>
        /// Indicates if only open alerts should be fetched
        /// </summary>
        public bool FetchOpenAlertsOnly
        {
            get { return _fetchOpenAlertsOnly; }
            set { _fetchOpenAlertsOnly = value; }
        }

        /// <summary>
        /// The image list for alert levels
        /// </summary>
        [NonSerialized]
        public static System.Windows.Forms.ImageList AlertsImageList = new System.Windows.Forms.ImageList();

        #endregion Public Properties

        #region Constructor/Initializer

        ///// <summary>
        ///// Static initializer for the AlertOptions
        ///// </summary>
        //static AlertOptions()
        //{
        //    //Image list is not presisted and so recreated once for the running TrafodionManager instance
        //    AlertsImageList = new System.Windows.Forms.ImageList();
        //    //AlertsImageList.Images.Add(System.Drawing.SystemIcons.Information);
        //    //AlertsImageList.Images.Add(System.Drawing.SystemIcons.Warning);
        //    //AlertsImageList.Images.Add(System.Drawing.SystemIcons.Error);
        //    AlertsImageList.Images.Add(Properties.Resources.InfoAlertIcon);
        //    AlertsImageList.Images.Add(Properties.Resources.WarningAlertIcon);
        //    AlertsImageList.Images.Add(Properties.Resources.ErrorAlertIcon);
        //    //AlertsImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
        //    AlertsImageList.ImageSize = new System.Drawing.Size(16, 16);
        //    AlertsImageList.TransparentColor = System.Drawing.Color.Transparent;
        //}

        /// <summary>
        /// Constructor
        /// </summary>
        public AlertOptions()
        {
            //_severityMappings = new Dictionary<string, AlertType>();
            //_severityMappings.Add("EMERGENCY", AlertType.Error);
            //_severityMappings.Add("IMMEDIATE", AlertType.Error);
            //_severityMappings.Add("CRITICAL", AlertType.Error);
            //_severityMappings.Add("ERROR", AlertType.Error);
            //_severityMappings.Add("WARNING", AlertType.Warning);
            //_severityMappings.Add("NOTICE", AlertType.Warning);
            //_severityMappings.Add("INFORMATIONAL", AlertType.Information);
            //_severityMappings.Add("DEBUG", AlertType.Information);
        }

        #endregion Constructor/Initializer
    }
}
