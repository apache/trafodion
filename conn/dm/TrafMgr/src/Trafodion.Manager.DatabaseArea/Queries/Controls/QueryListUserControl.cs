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

#define BATCH_EXPLAIN
#define BATCH_EXECUTE

using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// Class for QueryList control
    /// </summary>
    public partial class QueryListUserControl : UserControl
    {
        #region Fields

        private QueryListDataGridView _theQueryListDataGridView;
        private const string TRACE_SUB_AREA_NAME = "Query List";
        private List<ReportDefinition> _theCurrentBatchReportDefinitions = null;

        /// <summary>
        /// Defines batch mode state.
        /// </summary>
        public enum BatchMode { None = 0, BatchExecute, BatchExplain, BatchCancelled }

        private BatchMode _currentBatchMode = BatchMode.None;

        //to enable the support of batch mode. 
        private bool _batchModeEnabled = true;
        private bool _batchButtonsEnabled = false;

        /// <summary>
        /// Delegate method for aborting currently running query in QueryUserControl. 
        /// </summary>
        private QueryUserControl.AbortCurrentQueryMethod _abortCurrentQueryMethod = null;

        /// <summary>
        /// Delegate method for validating currently running query in QueryUserControl.
        /// </summary>
        private QueryUserControl.AllowToStartQueryMethod _allowToStartQueryMethod = null;

        /// <summary>
        /// Event delegate declaration for begin and end batch operation.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="args"></param>
        public delegate void BatchOperationChanged(object sender, EventArgs args);
        public event BatchOperationChanged BeginBatchOperation;
        public event BatchOperationChanged EndBatchOperation;

        /// <summary>
        /// Class for reporting multiple operation results
        /// </summary>
        public class MultipleOpResults
        {
            public String Summary;
            public DataTable ErrorTable;
        }

        private const string ReportDefinitionNameTag = "-- ReportDefinition.Name";
        private const string TrafodionProductVersioningTag = "-- Trafodion Database Manager 1.0 SQL Statement Text File --";

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: TheQueryListDataGridView - the data grid itself
        /// </summary>
        public QueryListDataGridView TheQueryListDataGridView
        {
            get { return _theQueryListDataGridView; }
        }

        /// <summary>
        /// Property: TheCurrentBatchMode - the current batch mode for the control.
        /// </summary>
        public BatchMode TheCurrentBatchMode
        {
            get { return _currentBatchMode; }
            set { _currentBatchMode = value; }
        }

        /// <summary>
        /// Property: EnableBatchButtons - to disable/enable batch buttons.
        /// </summary>
        public bool EnableBatchButtons
        {
            get { return _batchButtonsEnabled; }
            set 
            {
                if (_batchButtonsEnabled != value)
                {
                    _batchButtonsEnabled = value;
                    UpdateControls();
                }
            }
        }

        /// <summary>
        /// Property: AbortCurrentQueryMethod - to set/get the method for aborting current running query
        /// </summary>
        public QueryUserControl.AbortCurrentQueryMethod AbortCurrentQueryMethod
        {
            get { return _abortCurrentQueryMethod; }
            set { _abortCurrentQueryMethod = value; }
        }

        /// <summary>
        /// Property: ValidateCurrentQueryMethod - to set/get the method for validating current running query
        /// </summary>
        public QueryUserControl.AllowToStartQueryMethod AllowToStartQueryMethod
        {
            get { return _allowToStartQueryMethod; }
            set { _allowToStartQueryMethod = value; }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// The Statement List user control
        /// </summary>
        public QueryListUserControl()
        {
            InitializeComponent();

            _theQueryListDataGridView = new QueryListDataGridView();
            _theQueryListDataGridView.Dock = DockStyle.Fill;
            _theQueryListDataGridView.BorderStyle = BorderStyle.FixedSingle;

            Controls.Add(_theQueryListDataGridView);
            _theQueryListDataGridView.BringToFront();

            _theQueryListDataGridView.SelectionChanged += TheGrid_SelectionChanged;
            _theQueryListDataGridView.CheckBoxChanged += TheQueryListDataGridViewCheckBoxChanged;

            UpdateControls();

            // Also, listen to the report changes (but, we're interested in it only when in batch mode).
            ReportDefinition.Changed += ReportDefinitionChanged;

            // Initialize open file dialog
            _theOpenFileDialog.Title = Properties.Resources.WhiteboardOpenFileDialogTitle;
            _theOpenFileDialog.FilterIndex = 1;
            _theOpenFileDialog.RestoreDirectory = true;
            _theOpenFileDialog.Multiselect = true;
            _theOpenFileDialog.ShowHelp = true;
            _theOpenFileDialog.HelpRequest += OpenSaveFileDialog_HelpRequest;

            _theSaveFileDialog.Title = Properties.Resources.WhiteboardSaveFileDialogTitle;
            _theSaveFileDialog.ShowHelp = true;
            _theSaveFileDialog.HelpRequest += OpenSaveFileDialog_HelpRequest;
        }

        #endregion Constructor 

        #region Public methods

        /// <summary>
        /// Save the checked statements to a SQL whitebard data file. 
        /// Note: Keep this method as PUBLIC since this is invoked by the TrafodionProgressDialog.
        /// </summary>
        /// <param name="fs"></param>
        /// <returns></returns>
        public MultipleOpResults SavedToWhiteboardQueryDataFile(Stream fs)
        {
            MultipleOpResults results = new MultipleOpResults();
            DataTable errorTable = new DataTable();
            Hashtable saveItemsHt = new Hashtable();
            int count = 0;

            //Gets all the checked items
            List<iGRow> checkedList = TheQueryListDataGridView.CheckedRows;

            foreach (iGRow theRow in checkedList)
            {
                ReportDefinition report = theRow.Cells[QueryListDataGridView.COL_KEY_REPORTDEFINITION].Value as ReportDefinition;
                try
                {
                    if (null != report)
                    {
                        if (null != report.ResultContainer)
                        {
                            QueryResultContainer container = (QueryResultContainer)report.ResultContainer;
                            Control control = container.QueryResultControl;
                            if (control != null)
                            {
                                if (control is QueryResultControl)
                                {
                                    string execTimeLabel = container.GetExecutionTimeLabelText();
                                    string elapsedTimeLabel = container.GetElapsedTimeLabelText();
                                    string execStatusLabel = container.GetExecutionStatusLabel();
                                    report.SetProperty(ReportDefinition.SAVED_EXEC_TIME_LABEL, execTimeLabel);
                                    report.SetProperty(ReportDefinition.SAVED_EXEC_ELAPSED_TIME_LABEL, elapsedTimeLabel);
                                    report.SetProperty(ReportDefinition.SAVED_EXEC_STATUS_LABEL, execStatusLabel);
                                    TrafodionIGrid grid = ((QueryResultControl)control).TheGrid;
                                    NCCWorkbenchQueryData wbqd = (NCCWorkbenchQueryData)report.GetProperty(ReportDefinition.EXPLAIN_PLAN_DATA);
                                    if (null == wbqd)
                                    {
                                        wbqd = new NCCWorkbenchQueryData(NCCQueryPlan.TRAFODION_QLIST_UNFILED_NAMESPACE, 100000);//this._workspace.Options.FetchLimit);
                                        report.SetProperty(ReportDefinition.EXPLAIN_PLAN_DATA, wbqd);
                                    }
                                    wbqd.queryRunDataTable = grid.ExportToDataTable(report.DataTable.Clone());
                                    wbqd.executeQueryCached = true;
                                }
                                else if (control is SqlStatementTextBox)
                                {
                                    //string exception = ((SqlStatementTextBox)control).Text;
                                    //report.SetProperty(ReportDefinition.SAVED_EXEC_EXCEPTION, exception);
                                }
                            }
                        }

                        if (null != report.PlanContainer)
                        {
                            QueryPlanContainer container = (QueryPlanContainer)report.PlanContainer;
                            Control control = container.QueryPlanControl;

                            if (null != control)
                            {
                                if (control is QueryPlanControl)
                                {
                                    report.SetProperty(ReportDefinition.SAVED_PLAN_TIME_LABEL, container.GetEvaluationTimeLabelText());
                                    report.SetProperty(ReportDefinition.SAVED_PLAN_ELAPSED_TIME_LABEL, container.GetElapsedTimeLabelText());
                                    report.SetProperty(ReportDefinition.SAVED_PLAN_STATUS_LABEL, container.GetEvaluationStatusLabel());
                                }
                                else if (control is SqlStatementTextBox)
                                {
                                    //string exception = ((SqlStatementTextBox)control).Text;
                                    //report.SetProperty(ReportDefinition.SAVED_PLAN_EXCEPTION, exception);
                                }
                            }
                        }

                        if (saveItemsHt.Contains(report.Name))
                        {
                            throw new Exception(String.Format("Duplidate statement name - {0} found; please change the name before save to a data file.", report.Name));
                        }

                        saveItemsHt.Add(theRow.Index.ToString(), report);
                        count++;
                    }
                }
                catch (Exception ex)
                {
                    if (errorTable.Columns.Count <= 0)
                    {
                        errorTable.Columns.Add("From");
                        errorTable.Columns.Add("Statement Name");
                        errorTable.Columns.Add("Exception");
                    }
                    errorTable.Rows.Add(new object[] { theRow.Cells[QueryListDataGridView.COL_KEY_GROUPNAME].Value as string, 
                                                       report.Name, 
                                                       ex.Message});
                }
            }

            try
            {
                BinaryFormatter bf = new BinaryFormatter();
                bf.Serialize(fs, saveItemsHt);
            }
            catch (SerializationException se)
            {
                // logging a log
                count = 0;
                string errorMessage = "\nFailed to save queries to SQL Whiteboard data file\n" +
                                                       "  \t\"" + _theSaveFileDialog.FileName + "\"\n\n" +
                                                       "Problem: \t Unable to save queries to the specified SQL Whiteboard data file. \n\n" +
                                                       "Solution: \t Please see error details for recovery information. \n\n" +
                                                       "Details: \t " + se.Message + "\n\n";
                if (errorTable.Columns.Count <= 0)
                {
                    errorTable.Columns.Add("Exception");
                    errorTable.Rows.Add(new object[] { errorMessage });
                }
                else
                {
                    errorTable.Rows.Add(new object[] { "", 
                                                       "", 
                                                       errorMessage});
                }
            }

            finally
            {
                fs.Close();
            }

            if (errorTable.Rows.Count == 0)
            {
                if (count > 1)
                {
                    results.Summary = String.Format(Properties.Resources.WhiteboardSaveResultSummaryMessagePural, count);
                }
                else
                {
                    results.Summary = String.Format(Properties.Resources.WhiteboardSaveResultSummaryMessageSingular, count);
                }
                results.ErrorTable = null;
            }
            else
            {
                results.Summary =
                    String.Format("Total {0} out of {1} selected statements {2} been saved; However, the following exception{3} been reported:",
                                  count,
                                  checkedList.Count,
                                  (count > 1) ? "have" : "has",
                                  (errorTable.Rows.Count > 1) ? "s have" : " has");
                results.ErrorTable = errorTable;
            }

            return results;
        }

        /// <summary>
        /// Save the checked statements to a text file. 
        /// Note: Keep this method as PUBLIC since this is invoked by the TrafodionProgressDialog.
        /// </summary>
        /// <param name="fs">A text file stream</param>
        /// <returns></returns>
        public MultipleOpResults SavedToTextFile(Stream fs)
        {
            MultipleOpResults results = new MultipleOpResults();
            DataTable errorTable = new DataTable();
            StringBuilder textBuilder = new StringBuilder();
            textBuilder.AppendLine(TrafodionProductVersioningTag);

            int count = 0;

            //Gets all the checked items
            List<iGRow> checkedList = TheQueryListDataGridView.CheckedRows;

            foreach (iGRow theRow in checkedList)
            {
                ReportDefinition report = theRow.Cells[QueryListDataGridView.COL_KEY_REPORTDEFINITION].Value as ReportDefinition;
                try
                {
                    if (null != report)
                    {

                        string statement = report.GetProperty(ReportDefinition.FULL_RAW_TEXT) as string;
                        if (statement == null)
                        {
                            statement = report.GetProperty(ReportDefinition.DEFINITION) as string;
                            if (statement == null)
                            {
                                continue;
                            }
                        }

                        // Insert a report definition name tag for output: this will be used for statement name at input.
                        textBuilder.AppendLine(String.Format("{0}={1} --^", ReportDefinitionNameTag, report.Name));

                        if (statement.Trim().EndsWith(";"))
                        {
                            textBuilder.AppendFormat(statement);
                        }
                        else
                        {
                            textBuilder.AppendFormat("{0};",statement);
                        }
                            
                        textBuilder.AppendLine();
                        count++;
                    }
                }
                catch (Exception ex)
                {
                    if (errorTable.Columns.Count <= 0)
                    {
                        errorTable.Columns.Add("From");
                        errorTable.Columns.Add("Statement Name");
                        errorTable.Columns.Add("Exception");
                    }
                    errorTable.Rows.Add(new object[] { theRow.Cells[QueryListDataGridView.COL_KEY_GROUPNAME].Value as string, 
                                                       report.Name, 
                                                       ex.Message});
                }
            }

            StreamWriter sw = null;
            try
            {
                sw = new StreamWriter(fs);
                sw.Write(textBuilder.ToString());
            }
            catch (SerializationException se)
            {
                // logging a log
                count = 0;
                string errorMessage = "\nFailed to save queries to SQL Whiteboard data file\n" +
                                                       "  \t\"" + _theSaveFileDialog.FileName + "\"\n\n" +
                                                       "Problem: \t Unable to save queries to the specified SQL Whiteboard data file. \n\n" +
                                                       "Solution: \t Please see error details for recovery information. \n\n" +
                                                       "Details: \t " + se.Message + "\n\n";
                if (errorTable.Columns.Count <= 0)
                {
                    errorTable.Columns.Add("Exception");
                    errorTable.Rows.Add(new object[] { errorMessage });
                }
                else
                {
                    errorTable.Rows.Add(new object[] { "", 
                                                       "", 
                                                       errorMessage});
                }
            }

            finally
            {
                if (sw != null)
                    sw.Close();
                fs.Close();
            }

            if (errorTable.Rows.Count == 0)
            {
                if (count > 1)
                {
                    results.Summary = String.Format(Properties.Resources.WhiteboardSaveResultSummaryMessagePural, count);
                }
                else
                {
                    results.Summary = String.Format(Properties.Resources.WhiteboardSaveResultSummaryMessageSingular, count);
                }
                results.ErrorTable = null;
            }
            else
            {
                results.Summary =
                    String.Format("Total {0} out of {1} selected statements {2} been saved; However, the following exception{3} been reported:",
                                  count,
                                  checkedList.Count,
                                  (count > 1) ? "have" : "has",
                                  (errorTable.Rows.Count > 1) ? "s have" : " has");
                results.ErrorTable = errorTable;
            }

            return results;
        }

        /// <summary>
        /// To Load a list of statements into the Statement DataGrid
        /// Note: do not have to worry about duplicate query id (which will be used as statement name), 
        ///       the triage space makes sure the query id is unique.
        /// </summary>
        /// <param name="aListOfStatements"></param>
        /// <param name="from"></param>
        public void LoadStatements(ArrayList aListOfStatements, string from)
        {
            List<ReportDefinition> reportDefinitions = new List<ReportDefinition>();
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Query ID");
            errorTable.Columns.Add("Query Text");

            foreach (StatementDefinition statement in aListOfStatements)
            {
                if (String.IsNullOrEmpty(statement.QueryText) || String.IsNullOrEmpty(statement.QueryID))
                {
                    errorTable.Rows.Add(new object[] { statement.QueryID, statement.QueryText });
                }
                else
                {
                    SimpleReportDefinition report = CreateReport(statement.QueryText, statement.QueryID);
                    ((SimpleReportDefinition)report).Group = from;
                    report.SetProperty(ReportDefinition.START_TIME, statement.StartTime);
                    TheQueryListDataGridView.LoadReportDefinition(report, from);
                    reportDefinitions.Add(report);
                }
            }

            //Now, select the last report being imported
            if (reportDefinitions.Count > 0)
            {
                int rowIndex = this.TheQueryListDataGridView.IndexOfReportDefinition(reportDefinitions[reportDefinitions.Count - 1]);
                if (rowIndex >= 0)
                {
                    this.TheQueryListDataGridView.Rows[rowIndex].Selected = true;
                }
            }

            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd =
                    new TrafodionMultipleMessageDialog(Properties.Resources.ErrorBlankQueryIdOrText,
                                                    errorTable,
                                                    System.Drawing.SystemIcons.Warning);
                mmd.ShowDialog();
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Statement list grid selection changed event
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheGrid_SelectionChanged(object sender, EventArgs e)
        {
            if (_currentBatchMode == BatchMode.None)
            {
                UpdateControls();
            }
        }

        /// <summary>
        /// Event handler for statement list checkbox changed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheQueryListDataGridViewCheckBoxChanged(object sender, iGCellClickEventArgs e)
        {
            if (_currentBatchMode == BatchMode.None)
            {
                UpdateControls();
            }
        }

        /// <summary>
        /// private dispose method, which manually cleanup all of the report defintions.
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            //Whiteboard is closing. So Clear the results of all report definitions and free up the resources
            if (disposing)
            {
                for (int row = 0; row < _theQueryListDataGridView.Rows.Count; row++)
                {
                    ReportDefinition theReportDefinition = TheQueryListDataGridView.GetReportDefinition(row);
                    if (theReportDefinition != null)
                    {
                        if (theReportDefinition.ResultContainer != null)
                        {
                            theReportDefinition.ResultContainer.Dispose();
                            theReportDefinition.ResultContainer = null;
                        }
                        if (theReportDefinition.PlanContainer != null)
                        {
                            theReportDefinition.PlanContainer.Dispose();
                            theReportDefinition.PlanContainer = null;
                        }
                    }
                }
            }
            _theQueryListDataGridView.CheckBoxChanged -= TheQueryListDataGridViewCheckBoxChanged;
            _theQueryListDataGridView.SelectionChanged -= TheGrid_SelectionChanged;
            _theOpenFileDialog.HelpRequest -= OpenSaveFileDialog_HelpRequest;
            _theSaveFileDialog.HelpRequest -= OpenSaveFileDialog_HelpRequest;
        }

        /// <summary>
        /// Event handler when the Batch Execute button is clicked. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheBatchExecuteButtonClick(object sender, EventArgs e)
        {
            DoRunBatch(ReportDefinition.Operation.Execute);
        }

        /// <summary>
        /// Event handler when the Batch Explain button is clicked.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheBatchExplainButtonClick(object sender, EventArgs e)
        {
            DoRunBatch(ReportDefinition.Operation.Explain);
        }

        /// <summary>
        /// Start a batch job. 
        /// </summary>
        /// <param name="op"></param>
        private void DoRunBatch(ReportDefinition.Operation op)
        {
            switch (this._currentBatchMode)
            {
                case BatchMode.None:
                    if (this._theQueryListDataGridView.CheckedReportDefinitions.Count > 0)
                    {
                        //Make sure the previous query completed or stopped completely.
                        if (this.AllowToStartQueryMethod != null)
                        {
                            if (!this.AllowToStartQueryMethod())
                            {
                                MessageBox.Show(Utilities.GetForegroundControl(), 
                                    Properties.Resources.QueryAlreadyExecutingMessage,
                                    string.Format(Properties.Resources.WarningTitle, 
                                                  global::Trafodion.Manager.Properties.Resources.ProductName, 
                                                  Properties.Resources.SQLWhiteboard), 
                                    MessageBoxButtons.OK, 
                                    MessageBoxIcon.Warning);
                                return;
                            }
                        }

                        if (op == ReportDefinition.Operation.Execute)
                        {
                            this._currentBatchMode = BatchMode.BatchExecute;
                            this._theBatchExecuteButton.Text = Properties.Resources.CancelBatch;
                            this._theBatchExplainButton.Enabled = false;
                        }
                        else
                        {
                            this._currentBatchMode = BatchMode.BatchExplain;
                            this._theBatchExplainButton.Text = Properties.Resources.CancelBatch;
                            this._theBatchExecuteButton.Enabled = false;
                        }
                        
                        // Now, fire up the begin batch operation event
                        FireBeginBatchOperationEvent();
                        //this.TheQueryListDataGridView.ReadOnly = true;
                        this.TheQueryListDataGridView.BatchMode = true;

                        // Remember the list.
                        this._theCurrentBatchReportDefinitions = this.TheQueryListDataGridView.CheckedReportDefinitions;

                        // Fire up the first one in the list. 
                        ReportDefinition report = this._theCurrentBatchReportDefinitions[0];
                        int rowIndex = this.TheQueryListDataGridView.IndexOfReportDefinition(report);
                        this.TheQueryListDataGridView.Rows[rowIndex].Selected = true;
                        this.TheQueryListDataGridView.InvokeDoubleClicked(rowIndex);
                    }
                    break;

                case BatchMode.BatchExplain:
                case BatchMode.BatchExecute:
                    // This must be the cancel batch request
                    _currentBatchMode = BatchMode.BatchCancelled;

                    // Now, go ahead to cancel the currently running query. 
                    // This is done through the delegate function set by the QueryUserControl. 
                    if (this.AbortCurrentQueryMethod != null)
                    {
                        this.AbortCurrentQueryMethod(false);
                    }
                    ResetBatchMode();
                    break;

                default:
                    ResetBatchMode();
                    break;
            }
            UpdateControls();
        }

        /// <summary>
        /// If the selected row can be moved DOWN?
        /// </summary>
        /// <param name="rowIndex"></param>
        /// <returns></returns>
        private bool ThisRowCanMoveUp(int rowIndex)
        {
            if (rowIndex > 0 &&
                TheQueryListDataGridView.Rows[rowIndex - 1].Type == iGRowType.Normal)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// If the selected row can be moved UP?
        /// </summary>
        /// <param name="rowIndex"></param>
        /// <returns></returns>
        private bool ThisRowCanMoveDown(int rowIndex)
        {
            if ((rowIndex < TheQueryListDataGridView.Rows.Count - 1) &&
                TheQueryListDataGridView.Rows[rowIndex + 1].Type == iGRowType.Normal)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Enable/disable move buttons
        /// </summary>
        /// <param name="rowIndex"></param>
        private void EnableDisableMoveButtons(int rowIndex)
        {
            if (ThisRowCanMoveUp(rowIndex))
            {
                _moveOneUpButton.Enabled = true;
            }
            else
            {
                _moveOneUpButton.Enabled = false;
            }

            if (ThisRowCanMoveDown(rowIndex))
            {
                _moveOneDownButton.Enabled = true;
            }
            else
            {
                _moveOneDownButton.Enabled = false;
            }
        }

        /// <summary>
        /// Update all control buttons
        /// </summary>
        private void UpdateControls()
        {
            if (TheCurrentBatchMode == BatchMode.None)
            {
                bool rowsChecked = (_theQueryListDataGridView.CheckedRows.Count > 0);
                bool rowsSelected = this.TheQueryListDataGridView.SelectedRow != null && this.TheQueryListDataGridView.SelectedRow.Type == iGRowType.Normal;

                if (_batchModeEnabled)
                {
                    _theBatchExecuteButton.Enabled = (rowsChecked && EnableBatchButtons);
                    _theBatchExplainButton.Enabled = (rowsChecked && EnableBatchButtons);
                }

                // This button will be enabled only if there is checked statement not belong to the persistence file: 
                _moveToPersistenceFileButton.Enabled = rowsChecked && this.TheQueryListDataGridView.CheckedRowsContainNonPersistentGroup();

                // These buttons will be enabled only if rows are checked:
                _discardStatementButton.Enabled = rowsChecked;
                _theSaveStatementResultsButton.Enabled = rowsChecked;

                _moveOneDownButton.Enabled = rowsSelected && ThisRowCanMoveDown(this.TheQueryListDataGridView.SelectedRow.Index);
                _moveOneUpButton.Enabled = rowsSelected && ThisRowCanMoveUp(this.TheQueryListDataGridView.SelectedRow.Index);

                // Always enable these buttons:
                _theLoadStatementResultsButton.Enabled = true;
                if (_checkAllCheckBox.Checked && !rowsChecked)
                {
                    _checkAllCheckBox.Checked = false;
                }
                else if (!_checkAllCheckBox.Checked && TheQueryListDataGridView.Rows.Count > 0 && TheQueryListDataGridView.AllRowsAreChecked)
                {
                    _checkAllCheckBox.Checked = true;
                }

                _checkAllCheckBox.Enabled = true;
            }
            else
            {
                // All buttons are temporarily disabled for batch run.
                _moveToPersistenceFileButton.Enabled = false;
                _discardStatementButton.Enabled = false;
                _theSaveStatementResultsButton.Enabled = false;
                _theLoadStatementResultsButton.Enabled = false;
                _moveOneUpButton.Enabled = false;
                _moveOneDownButton.Enabled = false;
                _checkAllCheckBox.Enabled = false;
            }
        }

        /// <summary>
        /// Event handler for discard statement button click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheDiscardButtonClick(object sender, EventArgs e)
        {
            QueryDiscardOptionDialog qd = new QueryDiscardOptionDialog();
            if (qd.ShowDialog() == DialogResult.OK)
            {
                if (qd.DisardStatementAndResults)
                {
                    _theQueryListDataGridView.DeleteCheckedRows();
                }
                else
                {
                    _theQueryListDataGridView.DiscardResultsOfCheckedRows();
                }

                // Update the button accordingly. 
                UpdateControls();
            }
        }

        /// <summary>
        /// Event handler for Load statement tool strip button clicked
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheLoadStatementsButtonClick(object sender, EventArgs e)
        {
            _theOpenFileDialog.InitialDirectory = Trafodion.Manager.Framework.Utilities.FileDialogLocation();
            List<ReportDefinition> reportDefinitions = new List<ReportDefinition>();
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("File Name");
            errorTable.Columns.Add("Error Text");

            DialogResult result = _theOpenFileDialog.ShowDialog();
            if (result == DialogResult.OK)
            {
                foreach (string FileName in _theOpenFileDialog.FileNames)
                {
                    if (!File.Exists(FileName))
                    {
                        errorTable.Rows.Add(new object[] { FileName, "File does not exist" });
                        continue;
                    }

                    if (TheQueryListDataGridView.ContainsGroup(FileName))
                    {
                        // Found the file has been loaded. 
                        if (MessageBox.Show(Utilities.GetForegroundControl(),
                                    String.Format(Properties.Resources.SQLWhiteboardMessageFileHasBeenLoaded, FileName),
                                    string.Format(Properties.Resources.WarningTitle,
                                                  global::Trafodion.Manager.Properties.Resources.ProductName,
                                                  Properties.Resources.SQLWhiteboard), 
                                    MessageBoxButtons.YesNo,
                                    MessageBoxIcon.Exclamation,
                                    MessageBoxDefaultButton.Button2) != DialogResult.Yes)
                        {
                            // User does not want the file to be reloaded, go on to next file.
                            continue;
                        }
                        else
                        {
                            // User agrees to remove the group and reload the file again.
                            TheQueryListDataGridView.RemoveWholeGroup(FileName);
                        }
                    }

                    // check if this is a Whiteboard query data file format
                    string fileExt = Path.GetExtension(FileName);
                    if (!string.IsNullOrEmpty(fileExt) && fileExt.Equals(".wqd", StringComparison.OrdinalIgnoreCase))
                    {
                        try
                        {
                            LoadQueryDataFromFile(FileName, reportDefinitions);
                        }
                        catch (Exception ex)
                        {
                            errorTable.Rows.Add(new object[] { FileName, ex.Message });
                        }
                    }
                    else
                    {
                        try
                        {
                            LoadStatementsFromFile(FileName, reportDefinitions);
                        }
                        catch (Exception ex)
                        {
                            errorTable.Rows.Add(new object[] { FileName, ex.Message });
                        }
                    }
                }

                //Save the file location for future reference
                Trafodion.Manager.Framework.Utilities.FileDialogLocation(_theOpenFileDialog.FileNames[0]);

                //Now, select the last report being imported
                if (reportDefinitions.Count > 0)
                {
                    int rowIndex = this.TheQueryListDataGridView.IndexOfReportDefinition(reportDefinitions[reportDefinitions.Count - 1]);
                    if (rowIndex >= 0)
                    {
                        this.TheQueryListDataGridView.Rows[rowIndex].Selected = true;
                    }
                }

                // Report error if there is any
                if (errorTable.Rows.Count > 0)
                {
                    string summaryMessage = String.Format("Failed to load one or more file(s).");
                    TrafodionMultipleMessageDialog md = new TrafodionMultipleMessageDialog(summaryMessage, errorTable, System.Drawing.SystemIcons.Error);
                    md.ShowDialog();
                }
            }
        }

        /// <summary>
        /// Event handler for open file dialog help requested
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OpenSaveFileDialog_HelpRequest(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.LoadAndSaveStatements);
        }

        /// <summary>
        /// Event handler for report defintion change event
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aReportDefinition"></param>
        /// <param name="aReason"></param>
        private void ReportDefinitionChanged(object aSender, ReportDefinition aReportDefinition, ReportDefinition.Reason aReason)
        {
            if (!(this._currentBatchMode == BatchMode.None) && 
                this._theCurrentBatchReportDefinitions != null &&
                (aReportDefinition == this._theCurrentBatchReportDefinitions[0]))
            {
                switch (aReason)
                {
                    case ReportDefinition.Reason.ExecutionCompleted:
                        {
                            switch (this._currentBatchMode)
                            {
                                case BatchMode.BatchExecute:
                                case BatchMode.BatchExplain:
                                    // This is a Batch exection, first remove the one just completed, then process the result.
                                    this._theCurrentBatchReportDefinitions.RemoveAt(0);

                                    // Usually, we want to stop the batch execution if an exception occurs.
                                    // But, there is an option for user to instruct whiteboard to continue.
                                    string executionStatus = (string)aReportDefinition.GetProperty(ReportDefinition.LAST_EXECUTION_STATUS);
                                    if (!SQLWhiteboardOptions.GetOptions().StopBatchOperationOnExceptions ||
                                        (!string.IsNullOrEmpty(executionStatus) &&
                                         executionStatus.Equals(Properties.Resources.QueryExecutionSuccess)))
                                    {
                                        if (this._theCurrentBatchReportDefinitions.Count > 0)
                                        {
                                            // Fire up next one on the list. 
                                            ReportDefinition report = this._theCurrentBatchReportDefinitions[0];
                                            int rowIndex = this.TheQueryListDataGridView.IndexOfReportDefinition(report);
                                            this.TheQueryListDataGridView.Rows[rowIndex].Selected = true;
                                            // but, first post a delay
                                            System.Threading.Thread.Sleep(1000);
                                            this.TheQueryListDataGridView.InvokeDoubleClicked(rowIndex);
                                        }
                                        else
                                        {
                                            // It's all done.  
                                            ResetBatchMode();
                                            UpdateControls();
                                        }
                                    }
                                    else
                                    {
                                        // Error occurs; stop the batch execution.  
                                        this._theCurrentBatchReportDefinitions.Clear();
                                        ResetBatchMode();
                                        UpdateControls();
                                    }
                                    break;

                                case BatchMode.BatchCancelled:
                                    // Error occurs; stop the batch execution.  
                                    this._theCurrentBatchReportDefinitions.Clear();
                                    ResetBatchMode();
                                    UpdateControls();
                                    break;

                                default:
                                    break;
                            }
                            break;
                        }
                    default:
                        break;
                }
            }
        }

        /// <summary>
        /// To reset batch mode and reset all batch buttons.
        /// </summary>
        private void ResetBatchMode()
        {
            this._currentBatchMode = BatchMode.None;
            this._theCurrentBatchReportDefinitions = null;
            this._theBatchExecuteButton.Text = Properties.Resources.BatchExecute;
            this._theBatchExplainButton.Text = Properties.Resources.BatchExplain;
            this.TheQueryListDataGridView.BatchMode = false;
            FireEndBatchOperationEvent();
        }

        /// <summary>
        /// Create a simple report defintion with the given input text.
        /// </summary>
        /// <param name="aStatement"></param>
        /// <param name="name"></param>
        /// <returns></returns>
        private SimpleReportDefinition CreateReport(string aStatement, string name)
        {
            string inputText = aStatement.Replace("\r\n", "\n");
            if (QueryStringsPersistence.ReportDefinitionExists(name))
            {
                ReportDefinition existingReport = QueryStringsPersistence.GetReportDefinition(name);
                if (existingReport is SimpleReportDefinition)
                {
                    name = ((SimpleReportDefinition)existingReport).GetNewName();
                }
            }
            SimpleReportDefinition reportDefinition = new SimpleReportDefinition(name.Trim());
            reportDefinition.SetProperty(ReportDefinition.DEFINITION, inputText);
            reportDefinition.SetProperty(ReportDefinition.FULL_RAW_TEXT, inputText);
            return reportDefinition;
        }

        /// <summary>
        /// Event handler for Checkall checkbox click
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void CheckAllCheckBox_Click(object sender, EventArgs e)
        {
            if (_checkAllCheckBox.Checked)
            {
                _theQueryListDataGridView.CheckAll();
            }
            else
            {
                _theQueryListDataGridView.UncheckAll();
            }
        }

        /// <summary>
        /// To load all of the statements from a file. 
        /// Note: the file is assumed to be in text format.
        /// </summary>
        /// <param name="aFileName"></param>
        /// <param name="aListOfReports"></param>
        private void LoadStatementsFromFile(string aFileName, List<ReportDefinition> aListOfReports)
        {
            // all the other file extensions are considered in text format ...
            StreamReader sr = null;
            try
            {
                sr = File.OpenText(aFileName);
                string inputText = sr.ReadToEnd();
                string[] fname = ParseReportDefinitionName(aFileName).Split('\\');
                string name = fname[fname.Length - 1];

                // We assumed the text file is using semicolumn(;) as the statement separator.
                string[] statements = inputText.Split(';');

                // If it is our own text format, try to sanitize the text a little bit to 
                // make sure that every statement start with the report definition name tag. 
                if (inputText.StartsWith(TrafodionProductVersioningTag))
                {
                    int statementIndex = 0;
                    
                    // Clear the product version tag from the first statement
                    statements[0] = statements[0].Replace(TrafodionProductVersioningTag, "").Trim();

                    // Now, start sanitize all statements: every statement should start with the 
                    // report definition name tag. 
                    for (int i = 0; i < statements.Length; i++)
                    {
                        if (statements[i].Trim().StartsWith(ReportDefinitionNameTag))
                        {
                            statementIndex = i;
                        }
                        else
                        {
                            statements[statementIndex] = string.Format("{0}{1}", statements[statementIndex], statements[i]);
                            statements[i] = "\n";
                        }
                    }
                }

                int index = 1;
                for (int i = 0; i < statements.Length; i++)
                {
                    string statement = statements[i].Trim();
                    string statementName = null;
                    if (!string.IsNullOrEmpty(statement))
                    {
                        if (statement.StartsWith(ReportDefinitionNameTag))
                        {
                            string[] texts = statement.Split('^');
                            if (texts.Length > 1)
                            {
                                statementName = texts[0].Trim().Replace(ReportDefinitionNameTag, "").Replace("=", "").Replace("--", "");
                                statement = texts[1].Trim();
                            }
                            else
                            {
                                statementName = String.Format("{0}-{1}", name, index.ToString("D3"));
                            }
                        }
                        else
                        {
                            statementName = String.Format("{0}-{1}", name, index.ToString("D3"));
                        }

                        ReportDefinition report = CreateReport(statement + ";", statementName);

                        // Set group to the specified file name
                        ((SimpleReportDefinition)report).Group = aFileName;
                        TheQueryListDataGridView.LoadReportDefinition(report, aFileName);
                        aListOfReports.Add(report);
                        index++;
                    }
                }
            }
            catch (Exception ex)
            {
                TheQueryListDataGridView.RemoveWholeGroup(aFileName);
                throw ex;
            }
            finally
            {
                if (sr != null)
                {
                    sr.Close();
                }
            }
        }

        /// <summary>
        /// To load from a SQL Whiteboard query data file (wqd format). 
        /// Note: the caller needs to catch the exception.
        /// </summary>
        /// <param name="aFileName"></param>
        /// <param name="aListOfReports"></param>
        private void LoadQueryDataFromFile(string aFileName, List<ReportDefinition> aListOfReports)
        {
            Stream fs = null;

            try
            {
                fs = File.Open(aFileName, FileMode.Open, FileAccess.Read);
                Hashtable tempht = new Hashtable();

                BinaryFormatter formatter = new BinaryFormatter();
                tempht = (Hashtable)formatter.Deserialize(fs);

                ArrayList sorter = new ArrayList();
                sorter.AddRange(tempht.Keys);
                sorter.Sort();

                foreach (string name in sorter)
                {
                    //ReportDefinition report = de.Value as ReportDefinition;
                    SimpleReportDefinition report = (SimpleReportDefinition)tempht[name];
                    if (null != report)
                    {
                        string execException = (string)report.GetProperty(ReportDefinition.SAVED_EXEC_EXCEPTION);
                        if (!string.IsNullOrEmpty(execException))
                        {
                            QueryResultContainer container = new QueryResultContainer((Control)new QueryResultControl());
                            report.ResultContainer = container;
                            SqlStatementTextBox theTextBox = new SqlStatementTextBox();
                            theTextBox.ReadOnly = true;
                            theTextBox.Text = execException;
                            theTextBox.Dock = DockStyle.Fill;
                            theTextBox.WordWrap = true;
                            ((QueryResultContainer)report.ResultContainer).QueryResultControl = theTextBox;
                        }

                        string planException = (string)report.GetProperty(ReportDefinition.SAVED_PLAN_EXCEPTION);
                        if (!string.IsNullOrEmpty(planException))
                        {
                            QueryPlanContainer container = new QueryPlanContainer(new QueryPlanControl());
                            report.PlanContainer = container;
                            SqlStatementTextBox theTextBox = new SqlStatementTextBox();
                            theTextBox.ReadOnly = true;
                            theTextBox.Text = planException;
                            theTextBox.Dock = DockStyle.Fill;
                            theTextBox.WordWrap = true;
                            ((QueryPlanContainer)report.PlanContainer).QueryPlanControl = theTextBox;
                        }

                        NCCWorkbenchQueryData wbqd = (NCCWorkbenchQueryData)report.GetProperty(ReportDefinition.EXPLAIN_PLAN_DATA);
                        if (wbqd != null)
                        {
                            // Reports may have wbqd created to save execution result, so we need to check if there is really a plan.
                            string planTimeLabel = report.GetProperty(ReportDefinition.SAVED_PLAN_TIME_LABEL) as string;
                            if (!string.IsNullOrEmpty(planTimeLabel))
                            {
                                QueryPlanContainer container = new QueryPlanContainer(new QueryPlanControl());
                                report.PlanContainer = container;
                                ((QueryPlanControl)container.QueryPlanControl).LoadQueryData(wbqd);
                                string planElapsedTimeLabel = report.GetProperty(ReportDefinition.SAVED_PLAN_ELAPSED_TIME_LABEL) as string;
                                string planStatusLabel = report.GetProperty(ReportDefinition.SAVED_PLAN_STATUS_LABEL) as string;
                                container.SetAllEvaluationLabel(planTimeLabel, planElapsedTimeLabel, planStatusLabel);
                            }
                            
                            if (wbqd.executeQueryCached && wbqd.queryRunDataTable != null)
                            {
                                QueryResultContainer result_container = new QueryResultContainer((Control)new QueryResultControl());
                                report.ResultContainer = result_container;
                                string execTimeLabel = report.GetProperty(ReportDefinition.SAVED_EXEC_TIME_LABEL) as string;
                                string elapsedTimeLabel = report.GetProperty(ReportDefinition.SAVED_EXEC_ELAPSED_TIME_LABEL) as string;
                                string execStatusLabel = report.GetProperty(ReportDefinition.SAVED_EXEC_STATUS_LABEL) as string;
                                result_container.SetAllExecutionLabel(execTimeLabel, elapsedTimeLabel, execStatusLabel);
                                ((QueryResultControl)result_container.QueryResultControl).LoadTable(wbqd.queryRunDataTable);
                                report.ResetDataTable();
                            }
                        }

                        // Set group to the specified file name
                        ((SimpleReportDefinition)report).Group = aFileName;
                        TheQueryListDataGridView.LoadReportDefinition(report, aFileName);
                        aListOfReports.Add(report);
                    }
                }
            }
            catch (Exception e)
            {
                throw e;
            }
            finally
            {
                if (null != fs)
                fs.Close();
            }
        }

        /// <summary>
        /// Event handler for Save statement button
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void TheSaveStatementResultsButtonClick(object sender, EventArgs e)
        {
            DialogResult result = _theSaveFileDialog.ShowDialog();

            if (result == DialogResult.OK)
            {
                Stream fs = _theSaveFileDialog.OpenFile();

                // Single file selection only, so we should have gotten one file name.
                TrafodionProgressArgs args = null;

                // check if this is a Whiteboard query data file or text file
                string fileExt = Path.GetExtension(_theSaveFileDialog.FileName);
                if (!string.IsNullOrEmpty(fileExt) && fileExt.Equals(".wqd", StringComparison.OrdinalIgnoreCase))
                {
                    args = new TrafodionProgressArgs(String.Format("Saving to file {0}", _theSaveFileDialog.FileName), this, "SavedToWhiteboardQueryDataFile", new Object[1] { fs });
                }
                else
                {
                    args = new TrafodionProgressArgs(String.Format("Saving to file {0}", _theSaveFileDialog.FileName), this, "SavedToTextFile", new Object[1] { fs });
                }

                TrafodionProgressDialog progressDialog = new TrafodionProgressDialog(args);
                progressDialog.ShowDialog();

                if (progressDialog.Error != null)
                {
                    throw progressDialog.Error;
                }
                else if (progressDialog.ReturnValue is MultipleOpResults)
                {
                    MultipleOpResults results = (MultipleOpResults)progressDialog.ReturnValue;
                    if (results.ErrorTable != null)
                    {
                        TrafodionMultipleMessageDialog mmd =
                            new TrafodionMultipleMessageDialog(results.Summary,
                                                            results.ErrorTable,
                                                            System.Drawing.SystemIcons.Warning);
                        mmd.ShowDialog();
                    }
                    else
                    {
                        MessageBox.Show(results.Summary, "Save Statement Completed", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                }
            }
        }

        /// <summary>
        /// The event handler for Help button clicked. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _theHelpStripButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SQLWhiteboardStatementList);
        }

        /// <summary>
        /// Move the checked statements to the persistence file.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _moveToPersistenceFileButton_Click(object sender, EventArgs e)
        {
            TheQueryListDataGridView.MovedCheckedRowsToPersistenceFile();
            UpdateControls();
        }

        /// <summary>
        /// Move the currently selected statement (note: not the checked statements) on position up.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _moveOneUpButton_Click(object sender, EventArgs e)
        {
            if (TheQueryListDataGridView.SelectedRow != null &&
                TheQueryListDataGridView.Rows[TheQueryListDataGridView.SelectedRow.Index].Type == iGRowType.Normal)
            {
                // This is a regular row; not a group name
                // Also, moving boundary is within the grid and within the same group name.  
                if (ThisRowCanMoveUp(TheQueryListDataGridView.SelectedRow.Index))
                {
                    TheQueryListDataGridView.SelectedRow.Move(TheQueryListDataGridView.SelectedRow.Index - 1);
                    EnableDisableMoveButtons(TheQueryListDataGridView.SelectedRow.Index);
                }
            }
        }

        /// <summary>
        /// Move the currently selected statement on position down. 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void _moveOneDownButton_Click(object sender, EventArgs e)
        {
            if (TheQueryListDataGridView.SelectedRow != null &&
                TheQueryListDataGridView.Rows[TheQueryListDataGridView.SelectedRow.Index].Type == iGRowType.Normal)
            {
                if (TheQueryListDataGridView.SelectedRow.Index < TheQueryListDataGridView.Rows.Count - 1)
                {
                    // move only if not at the end.
                    if (TheQueryListDataGridView.SelectedRow.Index == TheQueryListDataGridView.Rows.Count - 2)
                    {
                        // now, it is at the 2nd to the last. just move it. 
                        TheQueryListDataGridView.SelectedRow.Move(TheQueryListDataGridView.SelectedRow.Index + 2);
                        EnableDisableMoveButtons(TheQueryListDataGridView.SelectedRow.Index);
                    }
                    else if (TheQueryListDataGridView.Rows[TheQueryListDataGridView.SelectedRow.Index + 1].Type == iGRowType.Normal)
                    {
                        TheQueryListDataGridView.SelectedRow.Move(TheQueryListDataGridView.SelectedRow.Index + 2);
                        EnableDisableMoveButtons(TheQueryListDataGridView.SelectedRow.Index);
                    }
                }
            }
        }

        /// <summary>
        /// To parse report definition name
        /// </summary>
        /// <param name="reportFileName"></param>
        /// <returns></returns>
        private string ParseReportDefinitionName(string reportFileName)
        {
            string[] tokens = reportFileName.Split(new string[] { "@" }, StringSplitOptions.RemoveEmptyEntries);
            string fileName = tokens[tokens.Length - 1];
            int extensionIndex = fileName.LastIndexOf(".");
            if (extensionIndex > 0)
            {
                fileName = fileName.Substring(0, extensionIndex);
            }
            return fileName;
        }

        /// <summary>
        /// Fire up the Begin Batch Operation event.
        /// There are many buttons and controls to temproarily disabled. 
        /// </summary>
        private void FireBeginBatchOperationEvent()
        {
            if (BeginBatchOperation != null)
            {
                BeginBatchOperation(this, new EventArgs());
            }
        }

        /// <summary>
        /// Fire up the End Batch Operation event. 
        /// To listener will need to restore all buttons and controls.  
        /// </summary>
        private void FireEndBatchOperationEvent()
        {
            if (EndBatchOperation != null)
            {
                EndBatchOperation(this, new EventArgs());
            }
        }

        #endregion Private methods
    }
}
