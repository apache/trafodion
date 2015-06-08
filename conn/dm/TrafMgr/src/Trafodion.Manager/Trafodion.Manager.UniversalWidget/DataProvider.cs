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
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Data;
using System.Windows.Forms;
//using ZedGraph;
namespace Trafodion.Manager.UniversalWidget
{
    /// <summary>
    /// Base class for different data providers
    /// </summary>
    public abstract class DataProvider
    {
        public delegate void InitDataproviderForFetch(object sender, DataProviderEventArgs e);
        public delegate void FetchingData(object sender, DataProviderEventArgs e);
        public delegate void NewDataArrived(object sender, DataProviderEventArgs e);
        public delegate void ErrorEncountered(object sender, DataProviderEventArgs e);
        public delegate void FetchCancelled(object sender, DataProviderEventArgs e);
        public delegate void TimerStateChanged(object sender, TimerEventArgs e);
        public delegate void BeforeFetchingData(object sender, DataProviderEventArgs e);

        public event FetchingData OnInitDataproviderForFetch;
        public event FetchingData OnFetchingData;
        public event NewDataArrived OnNewDataArrived;
        public event ErrorEncountered OnErrorEncountered;
        public event ErrorEncountered OnFetchCancelled;
        public event TimerStateChanged OnTimerStateChanged;
        public event BeforeFetchingData OnBeforeFetchingData;
        DataProviderConfig _theDataProviderConfig;

        Timer _theRefreshTimer;
        protected long _timeTakenToFetchData = 0;

        /// <summary>
        /// Provide interface to start and stop the timer
        /// </summary>
        public abstract void StartTimer();
        public abstract void StopTimer();

        /// <summary>
        /// Returns the data as a datatable
        /// </summary>
        /// <returns></returns>
        public abstract DataTable GetDataTable();

        /// <summary>
        /// This method will be to intitate the dataprovider to strt the fetching of data.
        /// For a DatabaseDataProvider it could mean starting the fetch and starting the timer
        /// for a repetitive fetch. For a MDD Data provider it could mean subscribing to an event             
        /// </summary>
        public abstract void Start(Hashtable defaultParameters);
        public abstract void Start();
        public abstract void RefreshData();

        /// <summary>
        /// Stop the data provider
        /// </summary>
        public abstract void Stop();


        public virtual int RefreshRate
        {
            get { return DataProviderConfig.RefreshRate; }
            set { DataProviderConfig.RefreshRate = value; }
        }

        public virtual DataProviderConfig DataProviderConfig
        {
            get { return _theDataProviderConfig; }
            set { _theDataProviderConfig = value; }
        }


        public virtual long TimeTakenToFetchData
        {
            get { return _timeTakenToFetchData; }
        }

        public Timer RefreshTimer
        {
            get { return _theRefreshTimer; }
            set { _theRefreshTimer = value; }
        }

        /// <summary>
        /// Fire timer related events
        /// </summary>
        /// <param name="e"></param>
        public void FireTimerStateChanged(TimerEventArgs e)
        {
            if (OnTimerStateChanged != null)
            {
                OnTimerStateChanged(this, e);
            }
        }

        /// <summary>
        /// Fire the OnInitDataproviderForFetch data event
        /// </summary>
        /// <param name="e"></param>
        public void FireInitDataproviderForFetch(DataProviderEventArgs e)
        {
            if (OnInitDataproviderForFetch != null)
            {
                OnInitDataproviderForFetch(this, e);
            }
        }

        /// <summary>
        /// Fire the fetching data event
        /// </summary>
        /// <param name="e"></param>
        public void FireFetchingData(DataProviderEventArgs e)
        {
            if (OnFetchingData != null)
            {
                OnFetchingData(this, e);
            }
        }

        /// <summary>
        /// Fire the before fetching data event
        /// this event is used to handle some additional function before fetching data
        /// </summary>
        /// <param name="e"></param>
        public void FireBeforeFetchingData(DataProviderEventArgs e)
        {
            if (OnBeforeFetchingData != null)
            {
                OnBeforeFetchingData(this, e);
            }
        }

        /// <summary>
        /// Fires an event indicating that new data has arrived
        /// </summary>
        /// <param name="e"></param>
        public void FireNewDataArrived(DataProviderEventArgs e)
        {
            if (OnNewDataArrived != null)
            {
                OnNewDataArrived(this, e);
            }
        }

        /// <summary>
        /// Fires an event indicating that error has occurred
        /// </summary>
        /// <param name="e"></param>
        public void FireErrorEncountered(DataProviderEventArgs e)
        {
            if (OnErrorEncountered != null)
            {
                OnErrorEncountered(this, e);
            }
        }

        /// <summary>
        /// Fires an event indicating that the user has cancelled the fetch
        /// </summary>
        /// <param name="e"></param>
        public void FireFetchCancelled(DataProviderEventArgs e)
        {
            if (OnFetchCancelled != null)
            {
                OnFetchCancelled(this, e);
            }
        }

        /// <summary>
        /// For plotting data on ZedGraph, we need doubles.
        /// All DataProviders will have this utility method 
        /// </summary>
        /// <param name="anObject"></param>
        /// <returns></returns>
        public static double GetDouble(Object anObject)
        {
            if (anObject.GetType().Equals(typeof(Int64)))
            {
                return GetDouble((Int64)anObject);
            }
            else if (anObject.GetType().Equals(typeof(Int32)))
            {
                return GetDouble((Int32)anObject);
            }
            else if (anObject.GetType().Equals(typeof(Int16)))
            {
                return GetDouble((Int16)anObject);
            }
            else if (anObject.GetType().Equals(typeof(Decimal)))
            {
                return GetDouble((Decimal)anObject);
            }
            else if (anObject.GetType().Equals(typeof(DateTime)))
            {
                return GetDouble((DateTime)anObject);
            }
            else if (anObject.GetType().Equals(typeof(string)))
            {
                return GetDouble((string)anObject);
            }
            return 0;
        }

        public static double GetDouble(Int64 aValue)
        {
            return (double)aValue;
        }
        public static double GetDouble(Int16 aValue)
        {
            return (double)aValue;
        }

        public static double GetDouble(Decimal aValue)
        {
            return (double)aValue;
        }

        public static double GetDouble(Int32 aValue)
        {
            return (double)aValue;
        }

        public static double GetDouble(string aValue)
        {
            double ret = 0;
            try
            {
                ret = double.Parse(aValue);
            }
            catch (Exception ex)
            {

            }
            return ret;
        }


        public static double GetDouble(DateTime aValue)
        {
            //double ret = new XDate(aValue);
            double ret = aValue.ToOADate();
            return ret;
        }

    }

    public class TimerEventArgs : EventArgs
    {
        public enum TimerEventTypes { Started, Paused, Ticked, Fired };
        public static string[] EventTypes = {"Refresh Started", "Refresh Paused", "Ticked", "Fired"};
        TimerEventTypes _theEventType;
        int _theTickValue;
        int _theRefreshRate;


        public TimerEventArgs()
        {
        }
        public TimerEventArgs(TimerEventTypes eventType)
        {
            this._theEventType = eventType;
        }
        public TimerEventArgs(TimerEventTypes eventType, int aTickValue)
        {
            this._theEventType = eventType;
            this._theTickValue = aTickValue;
        }
        public int RefreshRate
        {
            get { return _theRefreshRate; }
            set { _theRefreshRate = value; }
        }

        public int TickValue
        {
            get { return _theTickValue; }
            set { _theTickValue = value; }
        }

        public TimerEventTypes EventType
        {
            get { return _theEventType; }
            set { _theEventType = value; }
        }

    }
    /// <summary>
    /// This class has the fields needed to support the dataprovider events
    /// </summary>
    public class DataProviderEventArgs : EventArgs
    {
        Dictionary<string, object> _theEventParameters;
        Exception _theException;

        public Exception Exception
        {
            get { return _theException; }
            set { _theException = value; }
        }

        public Dictionary<string, object> EventParameters
        {
            get { return _theEventParameters; }
        }

        /// <summary>
        /// Add a parameter to the event arg
        /// </summary>
        /// <param name="aKey"></param>
        /// <param name="aValue"></param>
        public void AddEventProperty(string aKey, object aValue)
        {
            if (_theEventParameters == null)
            {
                _theEventParameters = new Dictionary<string, object>();
            }
            if (_theEventParameters.ContainsKey(aKey))
            {
                _theEventParameters.Remove(aKey);
            }
            _theEventParameters.Add(aKey, aValue);
        }

        /// <summary>
        /// Returns the value of  a viven event property
        /// </summary>
        /// <param name="aKey"></param>
        /// <returns></returns>
        public object GetEventProperty(string aKey)
        {
            if (_theEventParameters != null && _theEventParameters.ContainsKey(aKey))
            {
                return _theEventParameters[aKey];
            }
            return null;
        }
    }

    public class FetchVetoException : Exception
    {
        public FetchVetoException()
            : base()
        {
        }

        public FetchVetoException(string message)
            : base(message)
        {
        }
    }

}
