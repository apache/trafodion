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
using Trafodion.Manager.DatabaseArea.NCC;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// Class for Query Plan container
    /// </summary>
    public partial class QueryPlanContainer : UserControl
    {
        #region Fields

        private Control _queryPlanControl = null;
        private ReportDefinition _reportDefinition = null;
        private Timer _theTimer = new Timer();
        private int _elapsedTime = 0;
        private bool _planIsDirty = false;

        #endregion Fields

        #region Properties

        /// <summary>
        /// The report definition associate with this container.
        /// </summary>
        public ReportDefinition ReportDefinition
        {
            get { return _reportDefinition; }
            set { _reportDefinition = value; }
        }

        /// <summary>
        /// The real GUI control
        /// </summary>
        public Control QueryPlanControl
        {
            get { return _queryPlanControl; }
            set { _queryPlanControl = value; }
        }

        /// <summary>
        /// Plan is dirty. Therefore, when this is loaded to detail tab control, the plan control needs to be reloaded.
        /// </summary>
        public bool PlanIsDirty
        {
            get { return _planIsDirty; }
            set { _planIsDirty = value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// The constructor
        /// </summary>
        /// <param name="aQueryPlanControl"></param>
        public QueryPlanContainer(QueryPlanControl aQueryPlanControl)
        {
            InitializeComponent();
            _queryPlanControl = aQueryPlanControl;
            _queryPlanControl.Dock = System.Windows.Forms.DockStyle.Fill;
            _queryPlanControlPanel.Controls.Add(_queryPlanControl);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                //Dispose the result set control to free up resources.
                if (_queryPlanControl != null)
                {
                    _queryPlanControl.Dispose();
                    _queryPlanControl = null;
                }
            }
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Get the container ready when exeute explain.
        /// </summary>
        public void GetReadyExecute()
        {
            InitializeTimer();
            _lastEvaluationTimeLabel.Text = Properties.Resources.Evaluating;
            _timeToEvaluateLabel.Text = String.Format(Properties.Resources.TimeElapsed, "0");
            _evaluationStatusLabel.Text = "";

            // Start the timer that keeps track of execution time
            _theTimer.Start();
        }

        /// <summary>
        /// Wrap up the explain execution.
        /// </summary>
        /// <param name="aReportDefinition"></param>
        public void ExecuteComplete(ReportDefinition aReportDefinition)
        {
            _evaluationProgressBar.Visible = false;

            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_TIME, _elapsedTime.ToString());
            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTED_AT_TIME, DateTime.Now.ToLocalTime().ToString());
            aReportDefinition.SetProperty(ReportDefinition.CURRENT_EXECUTION_STATUS, ReportDefinition.STATUS_COMPLETED);

            string lastEvaluatedTime = aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTED_AT_TIME) as string;
            _lastEvaluationTimeLabel.Text = String.Format(Properties.Resources.LastEvaluated, lastEvaluatedTime);

            string executionStatus = aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_STATUS) as string;
            string system = aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_SYSTEM) as string;
            if (!string.IsNullOrEmpty(executionStatus))
            {
                _evaluationStatusLabel.Text = String.Format(Properties.Resources.FinalEvaluationStatusMessage, executionStatus, system);
            }
            else
            {
                _evaluationStatusLabel.Text = "";
            }

            // Reset the timer.
            _elapsedTime = 0;
            if (_theTimer != null)
            {
                _theTimer.Stop();
                _theTimer.Dispose();
            }
        }

        /// <summary>
        /// This method is called when the execute is completed successfully
        /// </summary>
        public void ExecutionSuccess()
        {
            ExecuteComplete(_reportDefinition);
            _queryStatusPanel.Update();
            _queryStatusPanel.Show();
        }

        /// <summary>
        /// Handles execution failure
        /// </summary>
        /// <param name="reportDefinition"></param>
        public void ExecutionFailed(ReportDefinition reportDefinition)
        {
            reportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_STATUS, Properties.Resources.QueryExecutionFailed);
            ExecuteComplete(reportDefinition);
            _queryStatusPanel.Update();
            _queryStatusPanel.Show();
        }

        /// <summary>
        ///Clean up when execute is cancelled        
        /// </summary>
        public void ExecuteCancelled(ReportDefinition aReportDefinition)
        {
            aReportDefinition.SetProperty(ReportDefinition.LAST_EXECUTION_STATUS, Properties.Resources.ExecutionCancelled);
            ExecuteComplete(aReportDefinition);
        }

        /// <summary>
        /// Set all labels.
        /// </summary>
        /// <param name="evalTimeLabel"></param>
        /// <param name="elapsedTimeLabel"></param>
        /// <param name="evalStatusLabel"></param>
        public void SetAllEvaluationLabel(string evalTimeLabel, string elapsedTimeLabel, string evalStatusLabel)
        {
            this._evaluationProgressBar.Visible = false;
            this._lastEvaluationTimeLabel.Text = evalTimeLabel;
            this._timeToEvaluateLabel.Text = elapsedTimeLabel;
            this._evaluationStatusLabel.Text = evalStatusLabel;
            this._queryStatusPanel.Update();
            this._queryStatusPanel.Show();
        }

        /// <summary>
        /// Get the last evaluation time label.
        /// </summary>
        /// <returns></returns>
        public string GetEvaluationTimeLabelText()
        {
            return this._lastEvaluationTimeLabel.Text;
        }

        /// <summary>
        /// Get the last elapsed time label.
        /// </summary>
        /// <returns></returns>
        public string GetElapsedTimeLabelText()
        {
            return this._timeToEvaluateLabel.Text;
        }

        /// <summary>
        /// Get the last evaluation status.
        /// </summary>
        /// <returns></returns>
        public string GetEvaluationStatusLabel()
        {
            return this._evaluationStatusLabel.Text;
        }

        public void ReLoadExplainPlan(ReportDefinition aReportDefinition)
        {
            NCCWorkbenchQueryData wbqd = (NCCWorkbenchQueryData)aReportDefinition.GetProperty(ReportDefinition.EXPLAIN_PLAN_DATA);
            if (wbqd != null)
            {
                //Dispose the result set control to free up resources.
                if (_queryPlanControl != null)
                {
                    _queryPlanControl.Dispose();
                }

                _queryPlanControl = new QueryPlanControl();
                ((QueryPlanControl)_queryPlanControl).LoadQueryData(wbqd);
                _queryPlanControl.Dock = System.Windows.Forms.DockStyle.Fill;
                _queryPlanControlPanel.Controls.Add(_queryPlanControl);
                _queryPlanControlPanel.Update();
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Initialize the timer.
        /// </summary>
        private void InitializeTimer()
        {
            _theTimer.Interval = 1000;
            _theTimer.Tick += new System.EventHandler(this.theTimer_Tick);
        }

        /// <summary>
        /// The event handler when timer pops.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void theTimer_Tick(object sender, EventArgs e)
        {
            //Update the elapsed time
            this._elapsedTime++;
            _timeToEvaluateLabel.Text = String.Format(Properties.Resources.TimeElapsed, this._elapsedTime.ToString());
        }

        #endregion Private methods
    }
}
