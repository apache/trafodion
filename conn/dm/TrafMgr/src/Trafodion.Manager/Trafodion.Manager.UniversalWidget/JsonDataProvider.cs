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
using System.Collections.Generic;
using System.Data;
using Trafodion.Manager.UniversalWidget;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Trafodion.Manager.UniversalWidget
{
    public class JsonDataProvider : WebDataProvider
    {

        #region Fields

        private DataTable _theDataTable = null;

        #endregion Fields

        #region Properties
        public JObject JSONObject
        {
            get
            {
                try
                {
                    return Newtonsoft.Json.Linq.JObject.Parse(Response);
                }
                catch (Exception ex)
                {
                    return null;
                }
            }
        }
        #endregion Properties

        #region Constructor

        public JsonDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {

        }

        #endregion Constructor

        /// <summary>
        /// Returns the data as a datatable after it has been fetched 
        /// </summary>
        /// <returns></returns>
        public override DataTable GetDataTable()
        {
            if (!string.IsNullOrEmpty(Response))
            {
                List<Dictionary<string, string>> list = JsonConvert.DeserializeObject<List<Dictionary<string, string>>>(Response);
                if (list == null)
                    return null;

                bool first = true;
                _theDataTable = new DataTable();
                foreach (Dictionary<string, string> dict in list)
                {
                    if (first)
                    {
                        foreach (string key in dict.Keys)
                        {
                            _theDataTable.Columns.Add(key);
                        }
                        first = false;
                    }

                    DataRow row = _theDataTable.NewRow();
                    foreach (string key in dict.Keys)
                    {
                        row[key] = dict[key];
                    }
                    _theDataTable.Rows.Add(row);
                }
            }

            return _theDataTable;
        }
    }
}
