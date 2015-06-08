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
using Trafodion.Manager.Framework.Connections;
using System;

namespace Trafodion.Manager.OverviewArea.Models
{
    [Serializable]
    public class DcsConnectionOptions
    {
        private string _hostName = "";

        public string HostName
        {
            get { return _hostName; }
            set { _hostName = value; }
        }
        private String _port = "41000";

        public String Port
        {
            get { return _port; }
            set { _port = value; }
        }

        public string WebServerURL
        {
            get { return _hostName.Trim() +":"+ _port.Trim(); }
        }

        public DcsConnectionOptions()
        {
        }

        public DcsConnectionOptions(ConnectionDefinition connectionDefinition)
        {
            _hostName = connectionDefinition.Host;
            _port = "41000";
        }

        public void SaveIntoPersistence()
        {

        }

        public static DcsConnectionOptions GetOptions(ConnectionDefinition connectionDefinition)
        {
            DcsConnectionOptions dcsConnectionOptions = null;

            try
            {
                dcsConnectionOptions = (DcsConnectionOptions)connectionDefinition.GetPropertyObject("DcsConnectionOptions");

                if (dcsConnectionOptions == null)
                {
                    dcsConnectionOptions = new DcsConnectionOptions(connectionDefinition);
                }
            }
            catch (Exception ex)
            {
                dcsConnectionOptions = new DcsConnectionOptions(connectionDefinition);
            }

            return dcsConnectionOptions;
        }
    }
}
