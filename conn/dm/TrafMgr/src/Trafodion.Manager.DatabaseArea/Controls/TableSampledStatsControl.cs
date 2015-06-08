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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    /// <summary>
    /// A user control that displays the of a table column by sampling the table content
    /// </summary>
    public partial class TableSampledStatsControl : UserControl
    {
        private SqlMxTableColumn _sqlMxTableColumn = null;
        private System.ComponentModel.BackgroundWorker _backgroundWorker;

        /// <summary>
        /// Constructs the user control to display the statistics for the table column
        /// </summary>
        /// <param name="sqlMxTableColumn">The SQL table column model</param>
        public TableSampledStatsControl(SqlMxTableColumn sqlMxTableColumn)
        {
            InitializeComponent();
            _sqlMxTableColumn = sqlMxTableColumn;

            //Display the screen first and then populate. So process the enter event
            this.Enter += new System.EventHandler(TableSampledStatsControl_Enter);
        }

        /// <summary>
        /// When the focus enters this control, populate with data.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TableSampledStatsControl_Enter(object sender, EventArgs e)
        {
            Cursor = Cursors.WaitCursor;
            try
            {
                _sqlMxTableColumn.SampledStatistics = null;//reset the earlier sample
                InitializeBackgoundWorker();
                _sampledStatsSummaryControl.InitializeControl(_sqlMxTableColumn);
                _sampledStatsFreqValuesControl.InitializeControl();
                _sampledStatsIntervalControl.InitializeControl();
                _backgroundWorker.RunWorkerAsync(_sqlMxTableColumn);
            }
            catch (Exception ex)
            {
                string header = String.Format(Properties.Resources.ErrorWhileSampling, _sqlMxTableColumn.SqlMxSchemaObject.VisibleAnsiName);
                MessageBox.Show(Utilities.GetForegroundControl(), ex.Message, header, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            Cursor = Cursors.Default;
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
            RunSample((SqlMxTableColumn)e.Argument, worker, e);
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
                MessageBox.Show(Utilities.GetForegroundControl(), e.Error.Message, Properties.Resources.Error, MessageBoxButtons.OK);
            }
            else if (e.Cancelled)
            {
            }
            else
            {
                SqlMxTableColumn sqlMxTableColumn = e.Result as SqlMxTableColumn;
                _sampledStatsSummaryControl.Populate(sqlMxTableColumn);
                _sampledStatsFreqValuesControl.Populate(sqlMxTableColumn);
                _sampledStatsIntervalControl.Populate(sqlMxTableColumn);
                _sampledStatsGraphControl.DrawChart(_sampledStatsIntervalControl.BoundaryDataGridView);
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

        void RunSample(SqlMxTableColumn sqlMxTableColumn, BackgroundWorker worker, DoWorkEventArgs e)
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
                int count = sqlMxTableColumn.SampledStatistics.FrequentValues.Count;
                count = sqlMxTableColumn.SampledStatistics.SampledIntervals.Count;
                e.Result = sqlMxTableColumn;
            }
        }

        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_backgroundWorker != null)
                {
                    _backgroundWorker.CancelAsync();
                }
                _sampledStatsSummaryControl.Dispose();
                _sampledStatsFreqValuesControl.Dispose();
                _sampledStatsIntervalControl.Dispose();
                _sampledStatsGraphControl.Dispose();
            }
        }

    }
}
