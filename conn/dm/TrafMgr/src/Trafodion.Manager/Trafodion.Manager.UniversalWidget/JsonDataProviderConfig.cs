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
using System.Xml;
using Trafodion.Manager.UniversalWidget;
using System.Collections.Generic;
using Trafodion.Manager.Framework.Connections;
using System.Xml.Serialization;

namespace Trafodion.Manager.UniversalWidget
{
    [Serializable]
    public class JsonDataProviderConfig : WebDataProviderConfig
    {
        public JsonDataProviderConfig()
        {
            ProviderName = "Json";
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (_theDataProvider == null)
            {
                _theDataProvider = new JsonDataProvider(this);
            }
            return _theDataProvider;
        }
    }
}
