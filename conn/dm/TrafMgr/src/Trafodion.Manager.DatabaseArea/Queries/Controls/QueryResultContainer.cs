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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /**************************************************************************
     * Given a DataSet, displays the results in a DataGridView 
     **************************************************************************/
    public partial class QueryResultContainer : UserControl

    {
        private String theSqlStatement;
        private Timer _theTimer = new Timer();
        private int _elapsedTime = 0;
        private Control _queryResultControl;
        private ReportDefinition _reportDefinition;

        public ReportDefinition ReportDefinition
        {
            get { return _reportDefinition; }
            set { _reportDefinition = value; }
        }

        /// <summary>
        /// The elapsed time in seconds
        /// </summary>
        public int ElapsedTime
        {
            get { return _elapsedTime; }
            set { _elapsedTime = value; }
        }

        public Control QueryResultControl
        {
            get { return _queryResultControl; }
            set
            {
                _queryResultControl = value;
            }
        }

        public QueryResultContainer(Control aQueryResultControl)
        {
            InitializeComponent();
            InitializeTimer();
            _nextButton.Enabled = _nextButton.Visible = false;
            _queryResultControl = aQueryResultControl;
            _queryResultControl.Dock = System.Windows.Forms.DockStyle.Fill;
            _resultControlPanel.Controls.Add(_queryResultControl);
            _executionStatusLabel.Text = "";
            headerPanel.Hide();
        }


        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Dispose the result set control to free up resources.
                if (_queryResultControl != null)
                {
                    _queryResultControl.Dispose();
                    _queryResultControl = null;
                }
            }
        }

        private void InitializeTimer()
        {
            _theTimer.Interval = 1000;
            _theTimer.Tick += new System.EventHandler(this.theTimer_Tick);
        }

        private void theTimer_Tick(object sender, EventArgs e)
        {
            //Update the elapsed time
            this._elapsedTime++;
            _timeToEvaluateLabel.Text = String.Format(Properties.Resources.TimeElapsed, this._elapsedTime.ToString());
        }

        public void DisplayHeaderMessage(string message)
        {
            TrafodionRichTextBox messageText = new TrafodionRichTextBox();
            messageText.Text = message;
            messageText.Dock = DockStyle.Fill;
            messageText.BackColor = System.Drawing.Color.WhiteSmoke;
            messageText.ForeColor = System.Drawing.Color.Red;
            messageText.Font = new System.Drawing.Font("Tahoma", 8F);
            messageText.ReadOnly = true;
            messageText.WordWrap = true;
            messageText.Multiline = true;
            headerPanel.Controls.Add(messageText);
            headerPanel.Show();
        }

        public void ResetHeader()
        {
            headerPanel.Controls.Clear();
            headerPanel.Hide();
        }

        public void UpdateProgressBar(int rows)
        {            
            _executionProgressBar.Step = rows - _executionProgressBar.Value;
            _executionProgressBar.PerformStep();
        }

        public void UpdatePage()
        {
            _nextButton.Enabled = _nextButton.Visible = true;
            if (_queryResultControl is QueryResultControl)
            {
                QueryResultControl qc = _queryResultControl as QueryResultControl;
                qc.IsPageMode = true;
                long startRow = qc.LastRowNumber - qc.TheGrid.Rows.Count + 1;
                long endRow = qc.LastRowNumber;
                string countControlText = string.Format("Displaying {0} rows in page {1} (rows {2} to {3})", 
                                                qc.TheGrid.Rows.Count, qc.PageNumber, startRow, endRow);
                qc.TheGrid.UpdateCountControlText(countControlText);
                SetRowsRetrieved(endRow);
            }
        }

        public void SetExecutionTimeLabel(string text)
        {
            _lastExecutionTimeLabel.Text = text;
            this._queryStatusPanel.Update();
        }

        /// <summary>
        /// To set the elapsed time label.
        /// </summary>
        /// <param name="text"></param>
        public void SetElapsedTimeLabel(string text)
        {
            _timeToEvaluateLabel.Text = text;
            this._queryStatusPanel.Update();
        }

        /// <summary>
        /// To set all labels in the execution result.
        /// </summary>
        /// <param name="execTimeLabel"></param>
        /// <param name="elapsedTimeLabel"></param>
        /// <param name="execStatusLabel"></param>
        public void SetAllExecutionLabel(string execTimeLabel, string elapsedTimeLabel, string execStatusLabel)
        {
            this._executionProgressBar.Visible = false;
            this._lastExecutionTimeLabel.Text = execTimeLabel;
            this._timeToEvaluateLabel.Text = elapsedTimeLabel;
            this._executionStatusLabel.Text = execStatusLabel;
            this._queryStatusPanel.Update();
        }

        /// <summary>
        /// To get the last execution time label.
        /// </summary>
        /// <returns></returns>
        public string GetExecutionTimeLabelText()
        {
            return this._lastExecutionTimeLabel.Text;
        }

        /// <summary>
        /// To get the elapsed time label.
        /// </summary>
        /// <returns></returns>
        public string GetElapsedTimeLabelText()
        {
            return this._timeToEvaluateLabel.Text;
        }

        /// <summary>
        /// To get the exectution status label.
        /// </summary>
        /// <returns></returns>
        public string GetExecutionStatusLabel()
        {
            return this._executionStatusLabel.Text;
        }

        public void SetRowsRetrieved(long rows)
        {
            _executionStatusLabel.Text = String.Format(Properties.Resources.RowsFetched, rows);
        }

        public void HideRowsRetrieved()
        {
            _executionStatusLabel.Text = "";
        }

        public void ExecuteComplete(ReportDefinition aReportDefinition)
        {
            _executionProgressBar.Visible = false;

            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_TIME, _elapsedTime.ToString());
            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTED_AT_TIME, DateTime.Now.ToLocalTime().ToString());
            aReportDefinition.SetProperty(ReportDefinition.CURRENT_EXECUTION_STATUS, ReportDefinition.STATUS_COMPLETED);

            string lastEvaluatedTime = aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTED_AT_TIME) as string;
            _lastExecutionTimeLabel.Text = String.Format(Properties.Resources.LastEvaluated, lastEvaluatedTime);

            string executionStatus = aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_STATUS) as string;
            string system = aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_SYSTEM) as string;
            if (!string.IsNullOrEmpty(executionStatus))
            {
                _executionStatusLabel.Text = String.Format(Properties.Resources.FinalEvaluationStatusMessage, executionStatus, system);
            }
            else
            {
                _executionStatusLabel.Text = "";
            }

            if (executionStatus.Equals(Properties.Resources.QueryExecutionSuccess))
            {
                if (_queryResultControl is QueryResultControl)
                {
                    QueryResultControl qc = _queryResultControl as QueryResultControl;
                    if (qc.IsPageMode)
                    {
                        long startRow = qc.LastRowNumber - qc.TheGrid.Rows.Count + 1;
                        long endRow = qc.LastRowNumber;
                        string countControlText = string.Format("Displaying {0} rows in page {1} (rows {2} to {3})",
                                                        qc.TheGrid.Rows.Count, qc.PageNumber + 1, startRow, endRow);
                        qc.TheGrid.UpdateCountControlText(countControlText);
                    }
                }
            }
            _nextButton.Enabled = _nextButton.Visible = false;
            
            _elapsedTime = 0;
            if (_theTimer != null)
            {
                _theTimer.Stop();
                _theTimer.Dispose();
            }
        }

        // This method is called when the execute is completed successfully
        public void ExecutionSuccess(ReportDefinition aReportDefinition)
        {
            ExecuteComplete(aReportDefinition);
        }

        // This method is called when the execute has errors
        public void ExecutionFailed(ReportDefinition aReportDefinition)
        {
            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_STATUS, Properties.Resources.QueryExecutionFailed);
            ExecuteComplete(aReportDefinition);
        }

        //Clean up when execute is cancelled
        public void ExecuteCancelled(ReportDefinition aReportDefinition)
        {
            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_STATUS, Properties.Resources.ExecutionCancelled);
            ExecuteComplete(aReportDefinition);
        }

        public void GetReadyExecute(int aMaxRows)
        {
            // Reset the Progress Bar
            InitProgressBar(aMaxRows);

            // Update status bar
            _lastExecutionTimeLabel.Text = Properties.Resources.Evaluating; 
            _timeToEvaluateLabel.Text = String.Format(Properties.Resources.TimeElapsed, "0");
            _executionStatusLabel.Text = "";

            // Start the timer that keeps track of execution time
            _theTimer.Start();
        }

        /// <summary>
        /// Replace the existing result control with a new one
        /// </summary>
        /// <param name="aResultControl"></param>
        public void ReplaceResultControl(Control aResultControl)
        {
            if (aResultControl != null)
            {
                _resultControlPanel.Controls.Remove(_queryResultControl);
                _queryResultControl.Dispose();
                _queryResultControl = aResultControl;
                _queryResultControl.Dock = System.Windows.Forms.DockStyle.Fill;
                _resultControlPanel.Controls.Add(_queryResultControl);
            }
        }

        /// <summary>
        /// Prepare the progress bar for begin execution
        /// </summary>
        private void InitProgressBar(int aMaxRows)
        {           
            _executionProgressBar.Maximum = aMaxRows;
            _executionProgressBar.Minimum = 0;
            _executionProgressBar.Step = 1;
            _executionProgressBar.Value = 0;
            _executionProgressBar.Visible = true;
        }

        private void nextButton_Click(object sender, EventArgs e)
        {
            if (_reportDefinition != null)
            {
                _reportDefinition.SetProperty(ReportDefinition.FETCH_NEXT_PAGE, true);
                _nextButton.Enabled = false;
            }
        }

    }
}
