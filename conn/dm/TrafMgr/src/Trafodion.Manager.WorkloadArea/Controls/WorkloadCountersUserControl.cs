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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WorkloadCountersUserControl : UserControl
    {
        #region Member variables

        private DataTable statementCounterDataTable = new DataTable();
        private ConnectionDefinition _connectionDefinition;
        int _lastRefreshRate = 30;
        //private StatementCounterDetailsForm _statementCounterDetailsForm = null;        

        #endregion Member variables


        public DataTable QueriesDataTable
        {
            get { return statementCounterDataTable; }
            set { statementCounterDataTable = value; }
        }

        public WorkloadCountersUserControl()
        {
            InitializeComponent();
            setupComponent();
            Reset();
        }


        private void setupComponent()
        {
            statementCounterDataTable.Columns.Add(StatementCounterDetailsForm.DATE_TIME_COLUMN_NAME, typeof(DateTime));
            statementCounterDataTable.Columns.Add(StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_RUNNING, typeof(Int32));
            statementCounterDataTable.Columns.Add(StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_COMPLETED, typeof(Int32));
            statementCounterDataTable.Columns.Add(StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_WAITING, typeof(Int32));
            statementCounterDataTable.Columns.Add(StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_HOLDORSUSPENDED, typeof(Int32));
            statementCounterDataTable.Columns.Add(StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_REJECTED, typeof(Int32));
        }

        public void LoadCounters(MonitorWorkloadDataProvider workloadDataProvider) 
        { 
            DataTable queryDataTable = workloadDataProvider.GetDataTable();
            _connectionDefinition = workloadDataProvider.ConnectionDefinition;
            _lastRefreshRate = workloadDataProvider.RefreshRate; //used by the statement counter details screen.
            _countersDetailsButton.Enabled = true;

            Int32 numExecuting = 0;
            Int32 numCompleted = 0;
            Int32 numWaiting = 0;
            Int32 numHold = 0;
            Int32 numSuspendedByAdmin = 0;
            Int32 numRejected = 0;

            if (queryDataTable != null && queryDataTable.Rows.Count > 0 && queryDataTable.Columns.Contains("QUERY_STATE"))
            {
                try
                {
                    numExecuting = (Int32)queryDataTable.Compute("COUNT(QUERY_STATE)", "QUERY_STATE = 'EXECUTING'");
                    numCompleted = (Int32)queryDataTable.Compute("COUNT(QUERY_STATE)", "QUERY_STATE = 'COMPLETED'");
                    numRejected = (Int32)queryDataTable.Compute("COUNT(QUERY_STATE)", "QUERY_STATE = 'REJECTED'");
                    numWaiting = (Int32)queryDataTable.Compute("COUNT(QUERY_STATE)", "QUERY_STATE = 'WAITING'");

                    string holdOrSuspendedFilter = "QUERY_STATE = 'HOLD'  OR " +
                                                   "QUERY_STATE = 'HOLDING' OR " +
                                                   "QUERY_STATE = 'SUSPENDED'";
                    numHold = (Int32)queryDataTable.Compute("COUNT(QUERY_STATE)", holdOrSuspendedFilter);
                }
                catch (Exception e)
                {
                    MessageBox.Show("Error setting Workload summary counters. " +
                                                "Details = " + e.Message);
                }
            }

            _executingCounterText.Text = numExecuting.ToString();
            _completedCounterText.Text = numCompleted.ToString();
            _holdCounterText.Text = numHold.ToString();
            _waitingCounterText.Text = numWaiting.ToString();
            _rejectedCounterText.Text = numRejected.ToString();

            addQueryCounterHistoryEntry(numExecuting, numCompleted, numWaiting, numHold, numRejected);
        }



        public void addQueryCounterHistoryEntry(Int32 numRunning, Int32 numCompleted, Int32 numWaiting, Int32 numHold, Int32 numRejected)
        {
            MonitorWorkloadOptions workloadOptions = MonitorWorkloadOptions.GetOptions();
            try
            {
                //Add a new row to statement counter details history datatable
                DataRow aRow = statementCounterDataTable.NewRow();
                aRow[StatementCounterDetailsForm.DATE_TIME_COLUMN_NAME] = DateTime.UtcNow + _connectionDefinition.ServerGMTOffset;
                aRow[StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_RUNNING] = numRunning;
                aRow[StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_COMPLETED] = numCompleted;
                aRow[StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_WAITING] = numWaiting;
                aRow[StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_HOLDORSUSPENDED] = numHold;
                aRow[StatementCounterDetailsForm.QUERY_COUNTERS_COLUMN_NAME_REJECTED] = numRejected;
                
                statementCounterDataTable.Rows.Add(aRow);

                //if statement counter details history datatable exceeds max Graph Data Points, Delete early rows
                if (statementCounterDataTable.Rows.Count > workloadOptions.MaxGraphPoints)
                {
                    DataRow[] rows = statementCounterDataTable.Select("", string.Format("{0} DESC", StatementCounterDetailsForm.DATE_TIME_COLUMN_NAME));
                    if (rows.Length > workloadOptions.MaxGraphPoints)
                    {
                        for (int idx = rows.Length - 1; idx >= workloadOptions.MaxGraphPoints; idx--)
                            rows[idx].Delete();

                        statementCounterDataTable.AcceptChanges();
                    }
                }

                //If the statement counter details window is open for the current system, update the details screen with the new metrics.
                ManagedWindow statementCounterDetails = WindowsManager.GetManagedWindow(TrafodionForm.TitlePrefix + _connectionDefinition.Name + " : " + "Statement Counter History");
                if (statementCounterDetails != null)
                {
                    foreach (Control control in statementCounterDetails.Controls)
                    {
                        if (control is StatementCounterDetailsForm)
                        {
                            ((StatementCounterDetailsForm)control).UpdateMetrics(statementCounterDataTable, _lastRefreshRate);
                            break;
                        }
                    }
                }
            }
            catch (Exception)
            {
            }

        }       
        
        /// <summary>
        /// Reset the metrics
        /// </summary>
        public void Reset()
        {
            _executingCounterText.Text = "";
            _completedCounterText.Text = "";
            _holdCounterText.Text = "";
            _waitingCounterText.Text = "";
            _rejectedCounterText.Text = "";
            statementCounterDataTable.Rows.Clear();
            _countersDetailsButton.Enabled = false;
        }

        private void _countersDetailsButton_Click(object sender, EventArgs e)
        {            
            //Check if a detail window is already open for the current system
            string windowTitle = TrafodionForm.TitlePrefix + _connectionDefinition.Name + " : " + "Statement Counter History";
            ManagedWindow statementCounterDetails = WindowsManager.GetManagedWindow(windowTitle);
            if (statementCounterDetails != null)
            {
                //detail window already exists. update the data and bring it to front.
                foreach (Control control in statementCounterDetails.Controls)
                {
                    if (control is StatementCounterDetailsForm)
                    {
                        ((StatementCounterDetailsForm)control).UpdateMetrics(statementCounterDataTable, _lastRefreshRate);
                        break;
                    }
                }

                if (WindowsManager.Exists(windowTitle))
                {
                    WindowsManager.Restore(windowTitle);
                    WindowsManager.BringToFront(windowTitle);
                }
            }
            else
            {
                //create a new window for the statement counter details and display
            StatementCounterDetailsForm statementCounterDetailsNew = new StatementCounterDetailsForm();
            ManagedWindow history = (ManagedWindow)WindowsManager.PutInWindow(new System.Drawing.Size(1080, 500), statementCounterDetailsNew, "Statement Counter History",
                    _connectionDefinition);
            statementCounterDetailsNew.UpdateMetrics(statementCounterDataTable, _lastRefreshRate);
            }
        }
      
    }
}
