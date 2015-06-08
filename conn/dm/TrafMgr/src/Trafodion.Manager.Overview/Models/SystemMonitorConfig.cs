//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2012-2015 Hewlett-Packard Development Company, L.P.
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
using System.Runtime.Serialization;
using System.Xml;
using System.Xml.Serialization;
using System.Data;

namespace Trafodion.Manager.OverviewArea.Models
{
    [Serializable]
    [XmlRoot("SystemMonitorConfig")]
    //[XmlInclude(typeof(DatabaseDataProviderConfig))]
    public class SystemMonitorConfig
    {
        #region Fields

        private string _theSystemName = null;
        private string _thePortNumber = null;
        private string _theSessionRetryTimer = null;
        private DataTable _theMetricChartDataTable = null;

        private DataTable _theHealthStatesDataTable = null;

        #endregion Fields

        #region Properties

        [XmlElement("SystemName")]
        public string SystemName
        {
            get { return _theSystemName; }
            set { _theSystemName = value; }
        }

        [XmlElement("PortNumber")]
        public string PortNumber
        {
            get { return _thePortNumber; }
            set { _thePortNumber = value; }
        }

        [XmlElement("SessionRetryTimer")]
        public string SessionRetryTimer
        {
            get { return _theSessionRetryTimer; }
            set { _theSessionRetryTimer = value; }
        }

        [XmlElement("MetricChartDataTable")]
        public DataTable MetricChartDataTable
        {
            get { return _theMetricChartDataTable; }
            set { _theMetricChartDataTable = value; }
        }

        [XmlElement("HealthStatesDataTable")]
        public DataTable HealthStatesDataTable
        {
            get { return _theHealthStatesDataTable; }
            set { _theHealthStatesDataTable = value; }
        }

        #endregion Properties
    }
}
