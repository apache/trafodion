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
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.DatabaseArea.Model
{
    public class ComponentPrivilegesDataProvider : DatabaseDataProvider
    {
        public ComponentPrivilegesDataProvider()
            : base(new DatabaseDataProviderConfig())
        {

        }

        /// <summary>
        /// Fetch the component privileges by invoking the system model
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(dbConfig.ConnectionDefinition);
            sqlMxSystem.RefreshComponentPrivileges();
        }
    }
}
