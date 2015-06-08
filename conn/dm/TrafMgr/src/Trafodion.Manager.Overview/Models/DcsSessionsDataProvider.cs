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
using Newtonsoft.Json.Linq;
using System.Net;
using System.IO;
using System.Text;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// This custom data provider is to used for the Live view display, where a sequence of operations are done
    /// This is achieved by overriding the DoFetchData method
    /// </summary>
    public class DcsSessionsDataProvider : JsonDataProvider
    {
        #region member variables

        ConnectionDefinition _connectionDefinition;
        private JObject _clusterSummaryObject;

        public JObject ClusterSummaryObject
        {
            get { return _clusterSummaryObject; }
        }

        #endregion member variables

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set { _connectionDefinition = value; }
        }

        public DcsSessionsDataProvider(ConnectionDefinition aConnectionDefinition, JsonDataProviderConfig dbConfig)
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
            try
            {
                FetchClusterSummary();
            }
            catch (Exception ex)
            {
            }
        }

        /// <summary>
        /// Fetch the WMS service status and the WMS platform metrics
        /// </summary>
        private void FetchClusterSummary()
        {
            DcsConnectionOptions hadoopOptions = DcsConnectionOptions.GetOptions(_connectionDefinition);
            try
            {
                string url = string.Format("http://{0}/openedbm/restful-services/hadoopservice/clustersummary", ((WebDataProviderConfig)this.DataProviderConfig).TheRootURL);
                WebRequest _theWebRequest = (HttpWebRequest)WebRequest.Create(url);
                _theWebRequest.Method = "POST";
                if (_theWebRequest.Method.Equals("POST"))
                {
                    _theWebRequest.ContentType = "application/x-www-form-urlencoded;charset=utf-8";
                    string postParameters = "";
                    if (!string.IsNullOrEmpty(postParameters))
                    {
                        byte[] inputDataStream = Encoding.UTF8.GetBytes(postParameters);
                        _theWebRequest.ContentLength = inputDataStream.Length;
                        Stream newStream = _theWebRequest.GetRequestStream();
                        newStream.Write(inputDataStream, 0, inputDataStream.Length);
                        newStream.Close();
                    }
                }
                HttpWebResponse _theWebResponse = (HttpWebResponse)_theWebRequest.GetResponse();
                if (_theWebResponse.StatusCode == HttpStatusCode.OK)
                {
                    Stream dataStream = _theWebResponse.GetResponseStream();
                    StreamReader reader = new StreamReader(dataStream);
                    string _theResponse = reader.ReadToEnd();
                    dataStream.Close();
                    _theWebResponse.Close();
                    _clusterSummaryObject = Newtonsoft.Json.Linq.JObject.Parse(_theResponse);
                }
                else
                {
                    Stream dataStream = _theWebResponse.GetResponseStream();
                    StreamReader reader = new StreamReader(dataStream);
                    string _theResponse = reader.ReadToEnd();
                    dataStream.Close();
                    _theWebResponse.Close();
                    _clusterSummaryObject = Newtonsoft.Json.Linq.JObject.Parse(_theResponse);
                }
            }
            catch (WebException ex)
            {
                if (ex.Response != null)
                {
                    Stream dataStream = ex.Response.GetResponseStream();
                    StreamReader reader = new StreamReader(dataStream);
                    string message = reader.ReadToEnd();
                    reader.Close();
                    dataStream.Close();
                    throw new Exception(message);
                }
            }
        }

    }
}
