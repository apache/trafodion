//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using System.Collections.Generic;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public class WMSQueryMetricsUserControl : TrafodionPropertyGridUserControl
    {
        #region Fields

        private WMSQueryDetailInfoUtils.Metrics _theMetric; 

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: the available metrics 
        /// </summary>
        public List<string> AvailMetrics
        {
            get { return base.AvailPropertyList; }
            set { base.AvailPropertyList = value; }
        }

        /// <summary>
        /// Property: the currently selected metrics
        /// </summary>
        public  List<string> SelectedMetrics
        {
            get { return base.SelectedPropertyList; }
            set { base.SelectedPropertyList = value; }
        }        

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        public WMSQueryMetricsUserControl(WMSQueryDetailInfoUtils.Metrics aMetric, List<string> availableMetrics, List<string> aSelectedMetrics)
            : base(string.Format("{0} Metrics", aMetric), availableMetrics, aSelectedMetrics)
        {
            _theMetric = aMetric;
            base.AvailListTitle = Properties.Resources.AvailableMetrics;
            base.SelectedListTitle = Properties.Resources.CurrentlyUsedMetrics;
        }

        #endregion Constructor
    }
}
