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
using System.Text;
using Trafodion.Manager.MetricMiner.Controls;

namespace Trafodion.Manager.MetricMiner
{
    /// <summary>
    /// A Singleton class for managing various aspects of MM
    /// It will have a reference to the MetricMiner control
    /// </summary>
    public class MetricMinerController
    {
        private static MetricMinerController _theMetricMinerController =  new MetricMinerController();
        private TabbedMetricMinerControl _theTabbedMetricMinerControl;
        private Dictionary<string, CatalogAndSchema> connectionSettings = new Dictionary<string, CatalogAndSchema>();
        private object myLock = new object();
        private MetricMinerController()
        {
        }

        public static MetricMinerController Instance
        {
            get
            {
                return _theMetricMinerController;
            }
        }

        public TabbedMetricMinerControl TheMetricMinerControl
        {
            get { return _theTabbedMetricMinerControl; }
            set { _theTabbedMetricMinerControl = value; }
        }


        public void SaveCatalogAndSchema(string aConnectionName, string aCatalogName, string aSchemaName)
        {
            lock (myLock)
            {
                if (connectionSettings.ContainsKey(aConnectionName))
                {
                    connectionSettings.Remove(aConnectionName);
                }
            }
            connectionSettings.Add(aConnectionName, new CatalogAndSchema(aCatalogName, aSchemaName));
        }

        public CatalogAndSchema GetCatalogAndSchema(string aConnectionName)
        {
            lock (myLock)
            {
                if (connectionSettings.ContainsKey(aConnectionName))
                {
                    CatalogAndSchema temp = connectionSettings[aConnectionName];
                    return new CatalogAndSchema(temp.CatalogName, temp.SchemaName);
                }
                return null;
            }
        }
    }

    public class CatalogAndSchema
    {
        private string _theCatalogName;
        private string _theSchemaName;

        public CatalogAndSchema(string aCatalogName, string aSchemaName)
        {
            _theCatalogName = aCatalogName;
            _theSchemaName = aSchemaName;
        }

        public string SchemaName
        {
            get { return _theSchemaName; }
            set { _theSchemaName = value; }
        }

        public string CatalogName
        {
            get { return _theCatalogName; }
            set { _theCatalogName = value; }
        }
    }

}
