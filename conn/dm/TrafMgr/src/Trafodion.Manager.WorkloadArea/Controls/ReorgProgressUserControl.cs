//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.ComponentModel;
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// Display Reorg Progress class
    /// </summary>
    public partial class ReorgProgressUserControl : UserControl
    {
        #region Fields

       
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_title = null;
        BackgroundWorker _backgroundWorker = null;
        private ConnectionDefinition _connectionDefinition;
        private WMSWorkloadCanvas m_parent = null;
 
        private const string COL_TOTAL_TABLES = "Total Tables";

        private const string COL_TABLE_NAME = "Table Name";
        private const string COL_TOTAL_PARTITIONS = "Total Partitions";
        private const string COL_REORG_NOT_NEEDED = "Reorg Not Needed";
        private const string COL_REORG_NOT_STARTED = "Reorg Not Started";
        private const string COL_REORG_RUNNING = "Reorg Running";
        private const string COL_REORG_COMPLETED = "Reorg Completed";
        private const string COL_REORG_ERROR = "Reorg Error";
        private const string COL_REORG_PROGRESS = "Reorg Progress";

        private const string COL_DT_TABLE_NAME = "tableName";
        private const string COL_DT_TOTAL_PARTITIONS = "totalPartitions";
        private const string COL_DT_REORG_NOT_NEEDED = "partitionsReorgNotNeeded";
        private const string COL_DT_REORG_NOT_STARTED = "partitionsReorgNotStarted";
        private const string COL_DT_REORG_RUNNING = "partitionsReorgRunning";
        private const string COL_DT_REORG_COMPLETED = "partitionsReorgCompleted";
        private const string COL_DT_REORG_ERROR = "partitionsReorgError";
        private const string COL_DT_REORG_PROGRESS = "partitionsReorgProgress";

        private DataTable _dataTableTableSummaryInfo = null;
        private DataTable _dataTablePartitionSummaryInfo = null;
        private DataTable _dataTableDetailsInfo = null;
        private Hashtable _hashtableResult = null;


        #endregion Fields

        #region Properties
        /// <summary>
        /// Property: QueryId - the query ID
        /// </summary>
        public string QueryId
        {
            get { return m_qid; }
            set { m_qid = _theQueryIdTextBox.Text = value; }
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _connectionDefinition; }
            set
            {
                _connectionDefinition = value;
            }
        }
        #endregion Properties

        #region Contructors

        public ReorgProgressUserControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid)
        {
            InitializeComponent();
            m_parent = parent;
            _connectionDefinition = aConnectionDefinition;
            m_qid = qid;
            _theQueryIdTextBox.Text = m_qid;
            _theQueryIdTextBox.ReadOnly = true;
            _theRefreshButton.Enabled=false;
            _theStopQuery.Enabled = false;
            ResizeGroupbox();
            initDetailGrid();
            initBackgroundWorker();
            DoWorkBackground();
        }
        #endregion Constructors

        #region Private Methods
        private void initDetailGrid()
        {
            infoIGrid.Clear();
            iGCol tableNameCol = infoIGrid.Cols.Add(COL_TABLE_NAME, COL_TABLE_NAME);            
            iGCol totalPartitions = infoIGrid.Cols.Add(COL_TOTAL_PARTITIONS, COL_TOTAL_PARTITIONS);
            iGCol reorgNotNeeded = infoIGrid.Cols.Add(COL_REORG_NOT_NEEDED, COL_REORG_NOT_NEEDED);
            iGCol reorgNotStarted = infoIGrid.Cols.Add(COL_REORG_NOT_STARTED, COL_REORG_NOT_STARTED);
            iGCol reorgRunning = infoIGrid.Cols.Add(COL_REORG_RUNNING, COL_REORG_RUNNING);
            iGCol reorgCompleted = infoIGrid.Cols.Add(COL_REORG_COMPLETED, COL_REORG_COMPLETED);
            iGCol reorgError = infoIGrid.Cols.Add(COL_REORG_ERROR, COL_REORG_ERROR);
            iGCol reorgProgress = infoIGrid.Cols.Add(COL_REORG_PROGRESS, COL_REORG_PROGRESS);
            infoIGrid.AutoResizeCols = true;

        }

        private void BindingSummary()
        {

            if (_dataTableTableSummaryInfo != null)
            {

                foreach (DataRow dr in _dataTableTableSummaryInfo.Rows)
                {
                    _totalTablesText.Text = dr["totalTables"].ToString();
                    _reorgNotNeededText.Text = dr["tablesReorgNotNeeded"].ToString();
                    _reorgNotStartedText.Text = dr["tablesReorgNotStarted"].ToString();
                    _reorgRunningText.Text = dr["tablesReorgRunning"].ToString();
                    _reorgCompletedText.Text = dr["tablesReorgCompleted"].ToString();
                    _reorgErrorText.Text = dr["tablesReorgError"].ToString();
                    _reorgProgressText.Text = dr["tablesReorgProgress"].ToString()+"%";

                    _totalPartitionsText.Text = dr["totalPartitions"].ToString();
                    _reorgNotNeededPtText.Text = dr["partitionsReorgNotNeeded"].ToString();
                    _reorgNotStartedPtText.Text = dr["partitionsReorgNotStarted"].ToString();
                    _reorgRunningPtText.Text = dr["partitionsReorgRunning"].ToString();
                    _reorgCompletedPtText.Text = dr["partitionsReorgCompleted"].ToString();
                    _reorgErrorPtText.Text = dr["partitionsReorgError"].ToString();
                    _reorgProgressPtText.Text = dr["partitionsReorgProgress"].ToString() + "%";
                }
            }
        }

        private void BindingDetails()
        {
            if (_dataTableDetailsInfo != null)
            {
                infoIGrid.Rows.Clear();
                foreach (DataRow dr in _dataTableDetailsInfo.Rows)
                {
                    iGRow row = infoIGrid.Rows.Add();
                    row.Cells[COL_TABLE_NAME].Value = dr[COL_DT_TABLE_NAME].ToString();
                    row.Cells[COL_TOTAL_PARTITIONS].Value = dr[COL_DT_TOTAL_PARTITIONS].ToString();
                    row.Cells[COL_REORG_NOT_NEEDED].Value = dr[COL_DT_REORG_NOT_NEEDED].ToString();
                    row.Cells[COL_REORG_NOT_STARTED].Value = dr[COL_DT_REORG_NOT_STARTED].ToString();
                    row.Cells[COL_REORG_RUNNING].Value = dr[COL_DT_REORG_RUNNING].ToString();
                    row.Cells[COL_REORG_COMPLETED].Value = dr[COL_DT_REORG_COMPLETED].ToString();
                    row.Cells[COL_REORG_ERROR].Value = dr[COL_DT_REORG_ERROR].ToString();
                    row.Cells[COL_REORG_PROGRESS].Value = dr[COL_DT_REORG_PROGRESS].ToString()+"%";
                }
            }
        }

        private void ResetData()
        {
            _totalTablesText.Text = string.Empty;
            _reorgNotNeededText.Text = string.Empty;
            _reorgNotStartedText.Text = string.Empty;
            _reorgRunningText.Text = string.Empty;
            _reorgCompletedText.Text = string.Empty;
            _reorgErrorText.Text = string.Empty;
            _reorgProgressText.Text = string.Empty;

            _totalPartitionsText.Text = string.Empty;
            _reorgNotNeededPtText.Text = string.Empty;
            _reorgNotStartedPtText.Text = string.Empty;
            _reorgRunningPtText.Text = string.Empty;
            _reorgCompletedPtText.Text = string.Empty;
            _reorgErrorPtText.Text = string.Empty;
            _reorgProgressPtText.Text = string.Empty;

            infoIGrid.Rows.Clear();
        }

        /// <summary>
        /// Cancels any currently running background work
        /// </summary>
        private void CancelAsync()
        {
            if (_backgroundWorker != null && _backgroundWorker.IsBusy)
            {
                _backgroundWorker.CancelAsync();
            }
        }

        private void DoWorkBackground()
        {
            ResetData();
            _backgroundWorker.RunWorkerAsync(m_qid);
        }

        /// <summary>
        /// status message setting.
        /// </summary>
        /// <param name="statusMessage"></param>
        private void ReportStatusStrip(String statusMessage)
        {
            _statusLabel.Text = statusMessage;
            _progressBar.Visible = !string.IsNullOrEmpty(statusMessage);
        }

         /// <summary>
        /// initialize a background worker.
        /// </summary>
        private void initBackgroundWorker()
        {
            _backgroundWorker = new BackgroundWorker();
            _backgroundWorker.WorkerReportsProgress = true;
            _backgroundWorker.WorkerSupportsCancellation = true;
            _backgroundWorker.DoWork += new DoWorkEventHandler(_backgroundWorker_DoWork);
            _backgroundWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(_backgroundWorker_RunWorkerCompleted);
            _backgroundWorker.ProgressChanged += new ProgressChangedEventHandler(_backgroundWorker_ProgressChanged);
        }

        /// <summary>
        /// Do work of updating Audit Logging Configuration
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            BackgroundWorker worker = sender as BackgroundWorker;

            //try
            {
                LoadReorgProgress(worker, e);
            }
            /*catch (Exception ex)
            {
                if (worker.CancellationPending)
                {
                    e.Cancel = true;
                }
                throw ex;
            }*/
        }

  
        private void LoadReorgProgress(BackgroundWorker worker, DoWorkEventArgs e)
        {      
            worker.ReportProgress(0);
            _hashtableResult=new Hashtable();
            Connection aConnection = new Connection(ConnectionDefinition);
            if (worker.CancellationPending)
            {
                e.Cancel = true;
                return;
            }
            DataTable dtSummary= Utilities.GetDataTableForReader(Queries.ExecuteReorgProgressSummaryInfo(aConnection, e.Argument.ToString()));
            _hashtableResult.Add("SUM",dtSummary);
            worker.ReportProgress(50);
            if (worker.CancellationPending)
            {
                e.Cancel = true;
                return;
            }
            DataTable dtDetails = Utilities.GetDataTableForReader(Queries.ExecuteReorgProgressDetailInfo(aConnection, e.Argument.ToString()));
            _hashtableResult.Add("DT", dtDetails);
            e.Result = _hashtableResult;
            worker.ReportProgress(100);
        }
      

        /// <summary>
        /// update status to UI progress bar.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            string msg = string.Empty;
            switch (e.ProgressPercentage)
            {
                case 0:
                    {
                        ReportStatusStrip("Fetching Reorg summary information...");
                        break;
                    }
                case 50:
                    {
                        ReportStatusStrip("Fetching Reorg details information...");
                        break;
                    }
                case 100:
                    {
                        ReportStatusStrip(string.Empty);
                        break;
                    }
                default:
                    break;

            }
        }

        /// <summary>
        /// worker completes process.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _backgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            ReportStatusStrip(string.Empty);

            if (e.Cancelled == true)
            {
                _theRefreshButton.Enabled = true;
                _theStopQuery.Enabled = false;
                return;
            }

            if (e.Error != null)
            {
                if (e.Error.Message.Contains("is not found"))
                {
                    MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Reorg status for this query could not be retrieved. It is possible the query completed already.",
                        "Reorg Progress",
                        MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else
                {
                    if (e.Error.Message.Contains("Reorg status for this qid could not be retrieved"))
                    {
                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Failed to fetch Reorg status for this query : " + e.Error.Message,
                            "Reorg Progress",
                            MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                    else
                    {

                        MessageBox.Show(Trafodion.Manager.Framework.Utilities.GetForegroundControl(), "Failed to fetch Reorg status for this query : " + e.Error.Message,
                            "Reorg Progress",
                            MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
                this.ParentForm.Close();
            }
            else
            {
                Hashtable ht =(Hashtable) e.Result;
                _dataTableTableSummaryInfo =(DataTable) ht["SUM"];
                _dataTableDetailsInfo = (DataTable)ht["DT"];
                BindingSummary();
                BindingDetails();
            }

            _theRefreshButton.Enabled = true;
            _theStopQuery.Enabled = false;
        }

        /// <summary>
        /// My disposing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                CancelAsync();
                _backgroundWorker.DoWork -= _backgroundWorker_DoWork;
                _backgroundWorker.RunWorkerCompleted -= _backgroundWorker_RunWorkerCompleted;
                _backgroundWorker.ProgressChanged -= _backgroundWorker_ProgressChanged;

                if (this.m_parent != null)
                {
                    this.m_parent.RemoveQueryFromWatchReorg(this);
                }
            }


        }
        
        private void ResizeGroupbox()
        {
            _tableStatusGroupBox.Width = this.Width / 2 - _tableStatusGroupBox.Location.X * 2;
            _partitionStatusGroupBox.Location = new System.Drawing.Point(this.Width / 2, _partitionStatusGroupBox.Location.Y);
            _partitionStatusGroupBox.Width = this.Width / 2 - _tableStatusGroupBox.Location.X * 2;

        }
        #endregion Private Methods

        #region Public Methods

        public void DoRefresh()
        {
            _theRefreshButton.PerformClick();
        }

        #endregion Public Methods

        #region Events
        private void _theRefreshButton_Click(object sender, EventArgs e)
        {
            _theRefreshButton.Enabled = false;
            _theStopQuery.Enabled = true;
            DoWorkBackground();
        }

        private void ReorgProgressUserControl_Resize(object sender, EventArgs e)
        {
            ResizeGroupbox();
        }

        private void _theStopQuery_Click(object sender, EventArgs e)
        {
            CancelAsync();
        }
        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ReorgProgressDlg);
        }
        #endregion Events


       
    }

 
}
