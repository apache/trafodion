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
using System.ComponentModel;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Connections.Controls
{
    public partial class BackgroundConnectDialog : TrafodionForm
    {
        private System.ComponentModel.BackgroundWorker _backgroundWorker;
        private ConnectionArgs _connectionArgs;
        private int _elapsedTime = 0;

        public BackgroundConnectDialog()
        {
            InitializeComponent();
            StartPosition = FormStartPosition.CenterParent;
        }
        public BackgroundConnectDialog(ConnectionArgs aConnectionArgs)
        {
            InitializeComponent();
            _connectionTimerLabel.Text = string.Format(Properties.Resources.ConnectionElapseTimeMessage, _elapsedTime);
            _connectionArgs = aConnectionArgs;
            StartPosition = FormStartPosition.CenterParent;
            _connectionTimer.Tick += _connectionTimer_Tick;
            _connectionTimer.Interval = 1000;
            _connectionTimer.Enabled = true;
            _connectionTimer.Start();
            InitializeBackgoundWorker();
            _backgroundWorker.RunWorkerAsync(aConnectionArgs);

        }

        void _connectionTimer_Tick(object sender, EventArgs e)
        {
            _elapsedTime++;
            _connectionTimerLabel.Text = string.Format(Properties.Resources.ConnectionElapseTimeMessage, _elapsedTime);
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
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "Begin Connection", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            }

            // Get the BackgroundWorker that raised this event.
            BackgroundWorker worker = sender as BackgroundWorker;

            // Assign the result of the computation
            // to the Result property of the DoWorkEventArgs
            // object. This is will be available to the 
            // RunWorkerCompleted eventhandler.
            DoConnect((ConnectionArgs)e.Argument, worker, e);
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
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "End Connection", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            }
            if (e.Error != null)
            {
                DialogResult = System.Windows.Forms.DialogResult.Cancel;
                Close();
                MessageBox.Show(Utilities.GetForegroundControl(), e.Error.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            else if (e.Cancelled)
            {
                // Next, handle the case where the user canceled 
                // the operation.
                // Note that due to a race condition in 
                // the DoWork event handler, the Cancelled
                // flag may not have been set, even though
                // CancelAsync was called.
                DialogResult = System.Windows.Forms.DialogResult.Cancel;
                Close();
            }
            else
            {
                // Finally, handle the case where the operation 
                // succeeded.
                DialogResult = System.Windows.Forms.DialogResult.OK;
                Close();
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
            //if progress is 1 then connection is complete and listeners are being notified.
            //Disable cancel button so as not to interrupt the event chain.
            if (e.ProgressPercentage == 100)
            {
                _cancelButton.Enabled = false;
                _statusTextBox.Text = "";
            }
            else
            {
                _statusTextBox.Text = ((string)e.UserState);
            }
        }

        /// <summary>
        /// This method is invoked by the worker thread to fetch DDL for the selected objects
        /// The fetched DDL is reported back in a progress event
        /// </summary>
        /// <param name="sqlMxObjectList"></param>
        /// <param name="worker"></param>
        /// <param name="e"></param>
        void DoConnect(ConnectionArgs aConnectionArgs, BackgroundWorker worker, DoWorkEventArgs e)
        {
            aConnectionArgs.ConnectionDefinition.DoBackgroundConnects(aConnectionArgs.IsTestOnly, worker, e);
        }

        void DoCancelBackgroundConnect()
        {

        }

        private void _cancelButton_Click(object sender, EventArgs e)
        {
            HandleClose();
        }

        void HandleClose()
        {
            _connectionTimer.Stop();
            if (_backgroundWorker.IsBusy)
            {
                _backgroundWorker.CancelAsync();
            }
        }
        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            HandleClose();
            base.OnFormClosing(e);
        }
    }

    public class ConnectionArgs
    {
        private bool _isTestOnly;
        private bool _suppressSuccessMessage;
        private ConnectionDefinition _connectionDefinition;

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
        }

        public bool SuppressSuccessMessage
        {
            get { return _suppressSuccessMessage; }
        }

        public bool IsTestOnly
        {
            get { return _isTestOnly; }
        }

        public ConnectionArgs(ConnectionDefinition aConnectionDefinition, bool suppressSuccessMessage, bool isTestOnly)
        {
            _connectionDefinition = aConnectionDefinition;
            _suppressSuccessMessage = suppressSuccessMessage;
            _isTestOnly = isTestOnly;
        }
    }

}
