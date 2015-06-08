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
using System.ComponentModel;
using System.Reflection;

namespace Trafodion.Manager.Framework
{
    class TrafodionProgressHelper
    {
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        private Object returnValue;
        private Exception _error;
        public event EventHandler<TrafodionProgressCompletedArgs> ProgressCompletedEvent;

        /// <summary>
        /// Return value of the method that is invoked in the background
        /// </summary>
        public Object ReturnValue
        {
            get { return returnValue; }
        }

        /// <summary>
        /// Exception if any from the background method invocation
        /// </summary>
        public Exception Error
        {
            get { return _error; }
        }
        
        public TrafodionProgressHelper(TrafodionProgressArgs aProgressArgs)
        {
            InitializeBackgoundWorker();
            _backgroundWorker.RunWorkerAsync(aProgressArgs);
        }

        public void Cancel()
        {
            if (_backgroundWorker != null)
            {
                _backgroundWorker.CancelAsync();
            }
        }

        /// <summary>
        /// Set up the BackgroundWorker object by attaching event handlers. 
        /// </summary>
        private void InitializeBackgoundWorker()
        {
            _backgroundWorker = new System.ComponentModel.BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(BackgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted +=
                new RunWorkerCompletedEventHandler(BackgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged +=
                new ProgressChangedEventHandler(BackgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// This event handler is where the actual,
        /// potentially time-consuming DDL work is done.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_DoWork(object sender,
            DoWorkEventArgs e)
        {
            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            DoWork((TrafodionProgressArgs)e.Argument, worker, e);
        }


        /// <summary>
        /// This event handler deals with the results of the
        /// background operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void BackgroundWorker_RunWorkerCompleted(
            object sender, RunWorkerCompletedEventArgs e)
        {
            // First, handle the case where an exception was thrown.
            if (e.Error != null)
            {
                if (e.Error is TargetInvocationException || e.Error.InnerException != null)
                {
                    _error = e.Error.InnerException;
                }
                else
                {
                    _error = e.Error;
                }
                TrafodionProgressCompletedArgs completeEvent = new TrafodionProgressCompletedArgs();
                completeEvent.Error = _error;
                OnProgressCompletedEvent(completeEvent);
            }
            else if (e.Cancelled)
            {
            }
            else
            {
                returnValue = e.Result;
                TrafodionProgressCompletedArgs completeEvent = new TrafodionProgressCompletedArgs();
                completeEvent.ReturnValue = e.Result;
                OnProgressCompletedEvent(completeEvent);
            }
        }

        /// <summary>
        /// This event handler updates the progress bar and appends the DDL text to the output textbox
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>

        private void BackgroundWorker_ProgressChanged(object sender,
            ProgressChangedEventArgs e)
        {
        }

        void DoWork(TrafodionProgressArgs progressArgs, BackgroundWorker worker, DoWorkEventArgs e)
        {
            // Abort the operation if the user has canceled.
            // Note that a call to CancelAsync may have set 
            // CancellationPending to true just after the
            // last invocation of this method exits, so this 
            // code will not have the opportunity to set the 
            // DoWorkEventArgs.Cancel flag to true. This means
            // that RunWorkerCompletedEventArgs.Cancelled will
            // not be set to true in your RunWorkerCompleted
            // event handler. This is a race condition.

            if (worker.CancellationPending)
            {
                e.Cancel = true;
            }
            else
            {
                MethodInfo methodInfo = progressArgs.ObjectInstance.GetType().GetMethod(progressArgs.MethodName);
                if (methodInfo != null)
                {
                    e.Result = methodInfo.Invoke(progressArgs.ObjectInstance, progressArgs.Args);
                }
            }
        }

        /// <summary>
        /// Raise the model changed event
        /// </summary>
        protected virtual void OnProgressCompletedEvent(TrafodionProgressCompletedArgs completedEvent)
        {
            EventHandler<TrafodionProgressCompletedArgs> handler = ProgressCompletedEvent;
            if (handler != null)
            {
                handler(this, completedEvent);
            }
        }
    }

    /// <summary>
    /// The argument for a progress dialog
    /// </summary>
    public class TrafodionProgressArgs
    {
        private string _text;
        private Object _objectInstance;
        private string _methodName;
        private Object[] _args;

        /// <summary>
        /// Instance of the object whose method is invoked in background
        /// </summary>
        public Object ObjectInstance
        {
            get { return _objectInstance; }
            set { _objectInstance = value; }
        }

        /// <summary>
        /// Name of the method to be invoked in background
        /// </summary>
        public string MethodName
        {
            get { return _methodName; }
            set { _methodName = value; }
        }
        /// <summary>
        /// arguments for the method
        /// </summary>
        public Object[] Args
        {
            get { return _args; }
            set { _args = value; }
        }

        /// <summary>
        /// Title that describes the background task
        /// </summary>
        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }

        public TrafodionProgressArgs(string title, object aObjectInstance, string aMethodName, Object[] args)
        {
            _text = title;
            _objectInstance = aObjectInstance;
            _methodName = aMethodName;
            _args = args;
        }
    }

    /// <summary>
    /// Event argument for a sql model change event
    /// </summary>
    public class TrafodionProgressCompletedArgs : EventArgs
    {
        private object _returnValue;
        private Exception _error;

        /// <summary>
        /// Constructs a model change event
        /// </summary>
        /// <param name="eventId"></param>
        /// <param name="sqlMxObject"></param>
        public TrafodionProgressCompletedArgs()
        {
        }

        public object ReturnValue
        {
            get { return _returnValue; }
            set { _returnValue = value; }
        }

        /// <summary>
        /// ChangeEvent that identifies the type of model change
        /// </summary>
        public Exception Error
        {
            get { return _error; }
            set { _error = value; }
        }
    }

}
