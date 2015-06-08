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
using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using Trafodion.Manager.UniversalWidget;
using Newtonsoft.Json.Linq;
using System.Net;
using System.IO;
using System.Text;

namespace Trafodion.Manager.UniversalWidget
{
    public class WebDataProvider : GenericDataProvider
    {

        #region Fields

        private DataTable _theDataTable = null;
        private int _configuredRefreshRate = 0;
        private bool _timerStarted = false;
        private bool _started = false;
        private string _theResponse = null;
        private HttpWebRequest _theWebRequest = null;
        private HttpWebResponse _theWebResponse = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: DataProviderConfig
        /// The configuration object of this data provider
        /// </summary>
        public override DataProviderConfig DataProviderConfig
        {
            get
            {
                return base.DataProviderConfig;
            }
            set
            {
                base.DataProviderConfig = value;
            }
        }


        /// <summary>
        /// Property: ConfiguredRefreshRate - the configured refresh rate to be used for retries.
        /// </summary>
        public int ConfiguredRefreshRate
        {
            get { return _configuredRefreshRate; }
            set { _configuredRefreshRate = value; }
        }

        public string Response
        {
            get { return _theResponse; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor of this data provider
        /// </summary>
        /// <param name="aDataProviderConfig"></param>
        public WebDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {

        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Returns the custom event args for the DatabaseDataProvider
        /// </summary>
        /// <returns></returns>
        public override DataProviderEventArgs GetDataProviderEventArgs()
        {
            DataProviderEventArgs evtArgs = new DataProviderEventArgs();
            return evtArgs;
        }

        /// <summary>
        /// To start the data provider
        /// </summary>
        public override void Start()
        {
            Start(new Hashtable());
        }

        /// <summary>
        /// To start the data provider
        /// </summary>
        /// <param name="defaultParameters"></param>
        public override void Start(Hashtable defaultParameters)
        {
            //  The starting of the Livefeed conneciton is done in the Prefetch. 
            base.Start(defaultParameters);            
        }

        /// <summary>
        /// To stop the data provider
        /// </summary>
        public override void Stop()
        {
            _started = false;
            base.Stop();
        }

        /// <summary>
        // To cancel the data fetch.
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchCancel(Trafodion.Manager.Framework.WorkerThread worker)
        {
            if (_theWebRequest != null)
            {
                _theWebRequest = null;
            }

            _theResponse = null;

        }

        /// <summary>
        /// To handle a data fetch error. 
        /// </summary>
        /// <param name="worker"></param>
        public override void DoFetchError(Trafodion.Manager.Framework.WorkerThread worker)
        {
            _theResponse = null;
        }

        /// <summary>
        /// To perform prefrech setup if there is anything. 
        /// </summary>
        /// <param name="parameters"></param>
        public override void DoPrefetchSetup(System.Collections.Hashtable parameters)
        {
            //do nothing
        }

        /// <summary>
        /// To perform data fetch.  Now, go fire up a web request
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            try
            {
                string url = ((WebDataProviderConfig)DataProviderConfig).TheURL;
                _theWebRequest = (HttpWebRequest)WebRequest.Create(url);
                _theWebRequest.Method = ((WebDataProviderConfig)DataProviderConfig).MethodType;
                _theWebRequest.ContentType = ((WebDataProviderConfig)DataProviderConfig).ContentType;
                _theWebRequest.Accept = ((WebDataProviderConfig)DataProviderConfig).ContentType;
                    

                if (_theWebRequest.Method.Equals("POST"))
                {
                    string postParameters = ((WebDataProviderConfig)DataProviderConfig).PostParameters;
                    if (!string.IsNullOrEmpty(postParameters))
                    {
                        byte[] inputDataStream = Encoding.UTF8.GetBytes(postParameters);
                        _theWebRequest.ContentLength = inputDataStream.Length;
                        Stream newStream = _theWebRequest.GetRequestStream();
                        newStream.Write(inputDataStream, 0, inputDataStream.Length);
                        newStream.Close();
                    }
                }
                _theWebResponse = (HttpWebResponse)_theWebRequest.GetResponse();
                if (_theWebResponse.StatusCode == HttpStatusCode.OK)
                {
                    Stream dataStream = _theWebResponse.GetResponseStream();
                    StreamReader reader = new StreamReader(dataStream);
                    _theResponse = reader.ReadToEnd();
                    dataStream.Close();
                    _theWebResponse.Close();
                }
                else
                {
                    Stream dataStream = _theWebResponse.GetResponseStream();
                    StreamReader reader = new StreamReader(dataStream);
                    _theResponse = reader.ReadToEnd();
                    dataStream.Close();
                    _theWebResponse.Close();
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
                }
                throw new Exception(ex.Message);
            }
        }

        public static string DoWebRequest(WebDataProviderConfig aWebDataProviderConfig)
        {
            string url = aWebDataProviderConfig.TheURL;
            WebRequest aWebRequest = WebRequest.Create(url);
            aWebRequest.Method = aWebDataProviderConfig.MethodType;
            if (aWebRequest.Method.Equals("POST"))
            {
                aWebRequest.ContentType = "application/x-www-form-urlencoded;charset=utf-8";
                string postParameters = aWebDataProviderConfig.PostParameters;
                if (!string.IsNullOrEmpty(postParameters))
                {
                    byte[] inputDataStream = Encoding.UTF8.GetBytes(postParameters);
                    aWebRequest.ContentLength = inputDataStream.Length;
                    Stream newStream = aWebRequest.GetRequestStream();
                    newStream.Write(inputDataStream, 0, inputDataStream.Length);
                    newStream.Close();
                }
            }
            WebResponse aWebResponse = aWebRequest.GetResponse();
            Stream dataStream = aWebResponse.GetResponseStream();
            StreamReader reader = new StreamReader(dataStream);
            string response = reader.ReadToEnd();
            dataStream.Close();
            aWebResponse.Close();
            return response;
        }
        /// <summary>
        /// To report data fetch progress.  
        /// </summary>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        public override void DoFetchProgress(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.ProgressChangedEventArgs e)
        {
            // do nothing, this is a push.
        }

        /// <summary>
        /// Returns the data as a datatable after it has been fetched 
        /// </summary>
        /// <returns></returns>
        public override DataTable GetDataTable()
        {
            return _theDataTable;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// to initiate the live feed conneciton
        /// </summary>
        private void InitiateConnection()
        {

            // If there is a timer, stop it. 
            StopRefreshTimer();
  
            // Fire up the get ready event for widgets to get ready.
            FireInitDataproviderForFetch(GetDataProviderEventArgs());

            _started = true;
        }

        /// <summary>
        /// Stop the refresh timer.  Since LiveFeed connection is a push technology, there is no need
        /// to refresh the "fetching".  So, we want to stop the timer. 
        /// </summary>
        private void StopRefreshTimer()
        {
            if (_timerStarted)
            {
                StopTimer();
                _timerStarted = false;
                //_configuredRefreshRate = RefreshRate; // in case the user has selected a different rate.
                RefreshRate = 0;
            }
        }

        /// <summary>
        /// Start the refresh timer.  This is needed when the LiveFeed connection has lost and the
        /// refresh timer will cause the re-start to occur.  As a result, the LiveFeed session may be
        /// re-established. 
        /// Note: Manupulateing the RefreshRate to disable the "timer paused" status on the widget.
        /// </summary>
        private void StartRefreshTimer()
        {
            if (_timerStarted)
            {
                return;
            }

            RefreshRate = _configuredRefreshRate;
            if (RefreshRate != 0)
            {
                StartTimer();
                _timerStarted = true;
            }
        }

        #endregion Private methods
    }
}
