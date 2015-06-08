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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.UniversalWidget;
using System.Collections;

namespace Trafodion.Manager.OverviewArea.Models
{
    class RepositoryDataProvider: DatabaseDataProvider
    {
        #region Fields

        private Dictionary<SystemMetricModel.SystemMetrics, string> _metricViewMapping = new Dictionary<SystemMetricModel.SystemMetrics, string>()
        {
            {SystemMetricModel.SystemMetrics.Core, "MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_NODE_1"},
            {SystemMetricModel.SystemMetrics.File_System, "MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_FILESYSTEM_1"}

        };

        public string sqltext1 = "select [first 500] * from MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_NODE_1";

        public string sqltext2 = "select [first 500] * from MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_QUERY_1 where SUBMIT_LCT_TS < TIMESTAMP '{0}' and EXEC_END_LCT_TS >TIMESTAMP '{1}'";
        public string sqltext3 = "select [first 10] * from MANAGEABILITY.INSTANCE_REPOSITORY.METRIC_QUERY_1";

        #endregion Fields

        #region Properties


        #endregion Properties


        #region Constructors

        public RepositoryDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        #endregion Constructors


        #region Public Methods
        public override void DoPrefetchSetup(Hashtable predefinedParametersHash)
        {
            DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;

            //dbConfig.SQLText = sqltext;
            base.DoPrefetchSetup(predefinedParametersHash);
        }  

        #endregion Public Methods

    }
}
