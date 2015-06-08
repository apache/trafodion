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
using System.Threading;
using System.ComponentModel;

namespace Trafodion.Manager.Framework
{
    //This class encapsulates a thread that can be used to do a time consuming task and not
    //block the UI.

    //The method signature of this class is very similar to BackgroundWorker. This is intentional
    //because I wanted to minimize the changes needed to existing code that use the background worker
    //class
    public class WorkerThread
    {
        #region Member Variables
        //The following delegates will be provided by the class that intends to use the WorkerThread

        //This delegate will actually be the time comsuming task performed by the thread
        public delegate void DoWorkHandler(object sender, DoWorkEventArgs e);

        //This delegate will be invoked by the thread once it has completed the DoWorkHandler OR
        //has encountered an error doing so OR the user has cancelled the operation. 
        public delegate void RunWorkerCompletedHandler(object sender, RunWorkerCompletedEventArgs e);

        //This delegate will be used to notify the caller of any progress changes.
        public delegate void ProgressChangedHandler(object sender, ProgressChangedEventArgs e);

        public DoWorkHandler DoWork;
        public RunWorkerCompletedHandler RunWorkerCompleted;
        public ProgressChangedHandler ProgressChanged;

        private Thread _theThread;
        private bool _CancellationPending = false;
        #endregion

        #region Properties
        public bool CancellationPending
        {
          get { return _CancellationPending; }
        }

        public bool IsBusy
        {
            get
            {
                if (_theThread != null)
                {
                    return _theThread.IsAlive;
                }
                return false;
            }
        }

        #endregion

        #region Public Methods
        //Cancel the long running method
        public void CancelAsync()
        {
            if (_theThread != null)
            {
                _CancellationPending = true;
                RunWorkerCompletedEventArgs e = new RunWorkerCompletedEventArgs(null, null, true);
                try
                {
                    //Spawn a new thread to call the cancel. That way the UI thread will never be blocked
                    //even if the query execution thread is blocked inside un-managed code.
                    Thread abortThread = new Thread(new ThreadStart(_theThread.Abort));
                    abortThread.IsBackground = true;
                    abortThread.Start();
                    //_theThread.Abort();
                }
                catch (Exception ex)
                {
                    //do nothing
                }
                if (RunWorkerCompleted != null)
                {
                    RunWorkerCompleted(this, e);
                }
            }
        }

        //Start the long running method
        public void RunWorkerAsync()
        {
            _theThread = new Thread(RunThread);
            _theThread.IsBackground = true;
            _CancellationPending = false;
            _theThread.Start();
        }

        //This method can be used to report any progress updates on the time consuming task
        //Note: In the DoWorkHandler delegate, the reference to the WorkerThread is being 
        // passed. The caller can use that handle to call the ReportProgress to notify 
        // any progress changes.
        public void ReportProgress(ProgressChangedEventArgs e)
        {
            if (ProgressChanged != null)
            {
                ProgressChanged(this, e);
            }
        }
        #endregion

        #region Private Methods
        //Helper method to run the thread
        private void RunThread()
        {
            if (DoWork != null)
            {
                RunWorkerCompletedEventArgs e = new RunWorkerCompletedEventArgs(null, null, false);
                try
                {
                    DoWork(this, new DoWorkEventArgs(null));
                }
                catch (Exception ex)
                {
                    e = new RunWorkerCompletedEventArgs(null, ex, false);
                }
                if (RunWorkerCompleted != null)
                {
                    RunWorkerCompleted(this, e);
                }
            }
        }
        #endregion

    }
}
