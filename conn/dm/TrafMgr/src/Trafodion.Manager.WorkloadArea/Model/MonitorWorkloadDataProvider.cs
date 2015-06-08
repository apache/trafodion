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
using System.Data;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// This custom data provider is to used for the Live view display, where a sequence of operations are done
    /// This is achieved by overriding the DoFetchData method
    /// </summary>
    public class MonitorWorkloadDataProvider : DatabaseDataProvider
    {
        #region member variables

        ConnectionDefinition _connectionDefinition;
        public readonly object SyncRootForFetchData = new object();

        #endregion member variables

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set { _connectionDefinition = value; }
        }

        public MonitorWorkloadDataProvider(ConnectionDefinition aConnectionDefinition, DatabaseDataProviderConfig dbConfig)
            :base(dbConfig)
        {
            _connectionDefinition = aConnectionDefinition;
        }

        /// <summary>
        /// The DoFetchData method is overridden to run a sequence of operations
        /// First the base class DoFetchData method is called to get the list of running queries. 
        /// Then the Wms system model is refreshed and a status wms command is run.
        /// Finally the list of wms services is refreshed
        /// All this data is required for the live view. 
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            base.DoFetchData(worker, e);
        }
    }
}
