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
using System.Windows.Forms;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSSummaryUserControl : UserControl
    {
        public WMSSummaryUserControl()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Reset the metrics in the summary counter widgets
        /// </summary>
        public void Reset()
        {
            _platformCountersUserControl.Reset();
            _servicesCountersUserControl.Reset();
            _workloadCountersUserControl.Reset();
        }

        /// <summary>
        /// Load the new WMS metrics into the summary counter widgets
        /// </summary>
        /// <param name="workloadDataProvider"></param>
        public void LoadCounters(MonitorWorkloadDataProvider workloadDataProvider)
        {
            _workloadCountersUserControl.LoadCounters(workloadDataProvider);
        }

        /// <summary>
        /// Load the Service summary and WMS platform metric counters
        /// </summary>
        /// <param name="summaryDataProvider"></param>
        public void LoadServiceAndPlatformCounters(SummaryCountsDataProvider summaryDataProvider)
        {
            if (summaryDataProvider.ConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()))
            {
                _servicesCountersUserControl.LoadCounters(summaryDataProvider);
                _servicesCountersUserControl.Visible = true;

                _platformCountersUserControl.LoadCounters(summaryDataProvider);
                _platformCountersUserControl.Visible = true;
            }
            else
            {
                _servicesCountersUserControl.Visible = false;
                _platformCountersUserControl.Visible = false;
            }
        }
        
    }
}
