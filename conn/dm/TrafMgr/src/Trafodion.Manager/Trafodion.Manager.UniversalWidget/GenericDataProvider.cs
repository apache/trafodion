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
using System.Threading;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Data;
using System.Windows.Forms;
using System.Data.Odbc;
using System.ComponentModel;
using Trafodion.Manager.Framework.Connections;


namespace Trafodion.Manager.UniversalWidget
{

    /// <summary>
    /// Generic implementation of a dataprovider with extension points for specific providers
    /// </summary>
    public abstract class GenericDataProvider : DataProvider
    {
        private Trafodion.Manager.Framework.WorkerThread    _WorkerThread   = null;
        private int                 _theRefreshRate     = 10;
        private int                 _theTickCount       = 0;
        public  bool                _theTimerPaused     = false;
        protected Hashtable         _theDefaultParameters = null;
        private bool                _cancelledFetch     = false;
        private bool _fetchInProgress = false;

        public GenericDataProvider()
        {
            RefreshTimer = new System.Windows.Forms.Timer();
            RefreshTimer.Tick += new EventHandler(RefreshTimer_Tick);
            InitializeBackgoundWorker();
        }

        public GenericDataProvider(DataProviderConfig aDataProviderConfig)
            : this()
        {
            DataProviderConfig = aDataProviderConfig;
        }

        public bool FetchInProgress
        {
            get { return _fetchInProgress; }
        }

        /// <summary>
        /// This is where the provider might ask for additional user inputs
        /// before making a call to the backend
        /// </summary>
        /// <param name="parameters"></param>
        public abstract void DoPrefetchSetup(Hashtable parameters);

        /// <summary>
        /// This is where the actual fetch will happen
        /// </summary>
        public abstract void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, DoWorkEventArgs e);

        /// <summary>
        /// This will be called if the background thread notifies a progress change
        /// </summary>
        public abstract void DoFetchProgress(Trafodion.Manager.Framework.WorkerThread worker, ProgressChangedEventArgs e);

        /// <summary>
        /// This is where any cleanup shall be done if the fetch is cancelled by the user
        /// </summary>
        public abstract void DoFetchCancel(Trafodion.Manager.Framework.WorkerThread worker);

        /// <summary>
        /// This is where any cleanup can be done if case of error
        /// </summary>
        public abstract void DoFetchError(Trafodion.Manager.Framework.WorkerThread worker);

        
        /// <summary>
        /// This method will be called by the GenericDataProvider to get the
        /// event arg that will be published with each event
        /// </summary>
        /// <returns></returns>
        public virtual DataProviderEventArgs GetDataProviderEventArgs()
        {
            return new DataProviderEventArgs();
        }

        public override DataProviderConfig DataProviderConfig
        {
            get { return base.DataProviderConfig; }
            set 
            {
                base.DataProviderConfig = value;
                _theRefreshRate = value.RefreshRate;
                _theTimerPaused = value.TimerPaused;
            }
        }

        public override int RefreshRate
        {
            get { return DataProviderConfig.RefreshRate; }
            set 
            {
                DataProviderConfig.RefreshRate = value;
                _theRefreshRate = value;
                doStartTimer();
            }
        }

        //Event to handle timer tick
        private void RefreshTimer_Tick(object sender, EventArgs e)
        {
            if ((!_theTimerPaused) && (!_fetchInProgress))
            {
                if (_theTickCount < _theRefreshRate)
                {
                    _theTickCount++;
                    TimerEventArgs evtArg = new TimerEventArgs(TimerEventArgs.TimerEventTypes.Ticked, _theTickCount);
                    evtArg.RefreshRate = _theRefreshRate;
                    FireTimerStateChanged(evtArg);
                }
                else
                {
                    TimerEventArgs evtArg = new TimerEventArgs(TimerEventArgs.TimerEventTypes.Fired, _theTickCount);
                    evtArg.RefreshRate = _theRefreshRate;
                    FireTimerStateChanged(evtArg);
                    _theTickCount = 0;

                    //Refresh the data of the table
                    RefreshData();
                }
            }
        }

        //Start the timer if the refresh rate is > 0
        public override void StartTimer()
        {
            _theTimerPaused = false;
            DataProviderConfig.TimerPaused = _theTimerPaused;
            doStartTimer();
        }

        private void doStartTimer()
        {
            _theTickCount = 0;
            RefreshTimer.Interval = 1000;
            if ((_theRefreshRate > 0) && (!_fetchInProgress) && (!_theTimerPaused))
            {
                if (!RefreshTimer.Enabled)
                {
                    //Since the timer needs to survive longer that the thread that is starting it
                    //we will start it from the GenericUniversalWidget in the handler of the 
                    //TimerStateChanged event

                    //RefreshTimer.Start();
                    //Fire the event
                    TimerEventArgs evtArg = new TimerEventArgs(TimerEventArgs.TimerEventTypes.Started, _theTickCount);
                    evtArg.RefreshRate = _theRefreshRate;
                    FireTimerStateChanged(evtArg);
                }
            }
        }

        //Stop the timer and fire an event
        public override void StopTimer()
        {
            _theTimerPaused = true;
            DataProviderConfig.TimerPaused = _theTimerPaused;
            doStopTimer();
        }

        private void doStopTimer()
        {
            if (RefreshTimer.Enabled)
            {
                RefreshTimer.Stop();
                //Fire the event
                TimerEventArgs evtArg = new TimerEventArgs(TimerEventArgs.TimerEventTypes.Paused, _theTickCount);
                evtArg.RefreshRate = _theRefreshRate;
                FireTimerStateChanged(evtArg);
            }
        }

        //Starts the data provider and any timers needed to refresh it automatically
        public override void Start()
        {
            this.Start(new Hashtable());
        }

        /// <summary>
        /// Refreshes the data with the existing parameters
        /// </summary>
        public override void RefreshData()
        {
            Start(_theDefaultParameters);
        }

        /// <summary>
        /// Fetch the data and start the timer for re-fetch if needed
        /// </summary>
        public override void Start(Hashtable defaultParameters)
        {

            try
            {
                FireBeforeFetchingData(GetDataProviderEventArgs());
                //Notify all listeners that we are about to start a new fetch cycle
                //Can throw a veto exception if anyone has a problem with that
                FireInitDataproviderForFetch(GetDataProviderEventArgs());
                _fetchInProgress = true;
                _theDefaultParameters = defaultParameters;

                //Stop the timer. it will start again after the background thread has completed
                doStopTimer();

                Hashtable predefinedParametersHash = new Hashtable();
                //Populate the predefinedParametersHash with the system properties and the user provided properties

                //Add the passed params to the predefinedParametersHash
                if (defaultParameters != null)
                {
                    foreach (DictionaryEntry de in defaultParameters)
                    {
                        if (predefinedParametersHash.ContainsKey(de.Key))
                        {
                            predefinedParametersHash.Remove(de.Key);
                        }
                        predefinedParametersHash.Add(de.Key, de.Value);
                    }
                }

                //This is where the provider might ask for additional user inputs
                //before making a call to the backend. If the necessary information
                //is not obtained from the user, a Veto exception can be thrown
                DoPrefetchSetup(predefinedParametersHash);


                //Notify listeners that we are about to make the backend calls.
                //This is where some one might choose to veto the call
                FireFetchingData(GetDataProviderEventArgs());

                //fetch the data
                _WorkerThread.RunWorkerAsync();

            }
            catch (FetchVetoException ex)
            {
                DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                this.FireFetchCancelled(evtArgs);
            }
            catch (Exception ex)
            {
                DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                evtArgs.Exception = ex;
                this.FireErrorEncountered(evtArgs);
            }
        }

        /// <summary>
        /// Stop the data fetch
        /// </summary>
        public override void Stop()
        {
            this.CancelAsync();
        }

        /// <summary>
        /// Set up the WorkerThread object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            //Before do that, first prepare for the case which user has canceled the request.
            _WorkerThread = new Trafodion.Manager.Framework.WorkerThread(); // new System.ComponentModel.WorkerThread();
            //_WorkerThread.WorkerReportsProgress = true;
            //_WorkerThread.WorkerSupportsCancellation = true;
            _WorkerThread.DoWork = WorkerThread_DoWork;
            _WorkerThread.RunWorkerCompleted = WorkerThread_RunWorkerCompleted;
            _WorkerThread.ProgressChanged = WorkerThread_ProgressChanged;
        }

        private void _theDatabaseObjectDetailsControl_Disposed(object sender, System.EventArgs e)
        {
            this.CancelAsync();
        }

        //Cancel the running background thread
        private void CancelAsync()
        {
            _fetchInProgress = false;
            if ((this._WorkerThread != null) && (this._WorkerThread.IsBusy))
            {
                _cancelledFetch = true;
                this._WorkerThread.CancelAsync();
                try
                {
                    DoFetchCancel(_WorkerThread);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Data Provider Error");
                }
                DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                this.FireFetchCancelled(evtArgs);
            }
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WorkerThread_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the WorkerThread that raised this event.
            Trafodion.Manager.Framework.WorkerThread worker = sender as Trafodion.Manager.Framework.WorkerThread;
            this._timeTakenToFetchData = 0;
            try
            {
                //Any exception will be reported as an error in the background thread
                DateTime startTime = DateTime.Now;
                DoFetchData(worker, e);
                _timeTakenToFetchData = DateTime.Now.Ticks - startTime.Ticks;
            }
            finally
            {
                this._fetchInProgress = false;
            }
        }

        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WorkerThread_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                DataProviderEventArgs evtArgs = GetDataProviderEventArgs();
                if (!_cancelledFetch)
                {
                    evtArgs.Exception = e.Error;

                    //do on error cleanup
                    try
                    {
                        DoFetchError(_WorkerThread);
                    }
                    catch (Exception ex)
                    {
                        //do nothing
                    }

                    this.FireErrorEncountered(evtArgs);

                    // If wants to continue on Error.
                    if (DataProviderConfig.TimerContinuesOnError &&
                        (this._theRefreshRate > 0) && (!this._theTimerPaused))
                    {
                        doStartTimer();
                }
                }
                else
                {
                    this.FireFetchCancelled(evtArgs);
                }
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                //this.TheTrafodionSchemaObjectsSummary.TheQuery.Cancel();
            }
            else
            {
                //Once the data has been obtained fire the event
                this.FireNewDataArrived(GetDataProviderEventArgs());
                //Note: If timer will start only if we get a successful response
                //That way we will not get the same error for every refresh interval
                if ((this._theRefreshRate > 0) && (! this._theTimerPaused))
                {
                    doStartTimer();
                }
            }
        }
        /// <summary>
        /// This event handler gets called when the progress of the dataprovider is updated
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>

        private void WorkerThread_ProgressChanged(object sender,
            ProgressChangedEventArgs e)
        {
            // Get the WorkerThread that raised this event.
            Trafodion.Manager.Framework.WorkerThread worker = sender as Trafodion.Manager.Framework.WorkerThread;

            try
            {
                DoFetchProgress(worker, e);
            }
            catch (Exception ex)
            {
                //do nothing if the DoFetchProgress throws exception. just ignore it.
            }
        }
    }


}
