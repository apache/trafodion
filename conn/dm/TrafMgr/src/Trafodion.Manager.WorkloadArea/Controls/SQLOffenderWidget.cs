//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.
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
using System.Data;
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public abstract partial class SQLOffenderWidget : WMSWorkloadCanvas
	{
        #region Members

        private static Size ChildrenWindowSize = new Size(800, 600);
        public delegate void UpdateStatus(Object obj, EventArgs e);
        public delegate void UpdateError(object sender, DataProviderEventArgs e);

        ConnectionDefinition _theConnectionDefinition;
        UniversalWidgetConfig _sqlOffenderConfig = null;
        GenericUniversalWidget _sqlOffenderWidget;
        private TrafodionIGridToolStripMenuItem _workloadDetailMenuItem = null;
        //private TrafodionIGridToolStripMenuItem _displayParentChildQueriesMenuItem = null;
        private TrafodionIGridToolStripMenuItem _displaySQLTextMenuItem = null;
        private TrafodionIGridToolStripMenuItem _displaySQLPlanMenuItem = null;
        private TrafodionIGridToolStripMenuItem _displayHistoryMenuItem = null;

        public GenericUniversalWidget SqlOffenderWidget
        {
            get { return _sqlOffenderWidget; }
            set { _sqlOffenderWidget = value; }
        }
        TrafodionIGrid _sqlOffenderIGrid = null;

		Connection _conn = null;

		#endregion

        public abstract String HelpTopic
        {
            get;
        }

        public abstract String ConfigName
        {
            get;
        }

        public abstract String TitleName
        {
            get;
        }

		public ConnectionDefinition ConnectionDefinition
		{
			get { return _theConnectionDefinition; }
			set
			{
				if (_theConnectionDefinition != null)
				{
                    if (_sqlOffenderWidget != null)
                        _sqlOffenderWidget.DataProvider.Stop();
                }

				if (_theConnectionDefinition != null && value != null && !_theConnectionDefinition.Equals(value))
				{
                    _sqlOffenderIGrid.Rows.Clear();
                }
				_theConnectionDefinition = value;

				if (_theConnectionDefinition != null)
				{
					if (_sqlOffenderWidget != null)
					{
                        if ((_sqlOffenderConfig != null) && (_sqlOffenderConfig.DataProviderConfig != null))
                        {
                            _sqlOffenderWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;
                            _sqlOffenderWidget.DataProvider.Start();
                        }
                    }
					else
					{
						ShowWidgets();
					}
				}
			}
		}


		public SQLOffenderWidget()
		{
			InitializeComponent();
		}

        public SQLOffenderWidget(ConnectionDefinition aConnectionDefinition)
		{
			InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
			ShowWidgets();
		}

		public void ShowWidgets()
		{
            //Create the configuration. If one is persisted, we use that otherwise we create one
            UniversalWidgetConfig tempSQLOffenderConfig = WidgetRegistry.GetConfigFromPersistence(ConfigName);

            List<ColumnMapping> persistedColumnMappings = null;
            if (tempSQLOffenderConfig == null)
            {
                _sqlOffenderConfig = WidgetRegistry.GetDefaultDBConfig();
                _sqlOffenderConfig.Name = ConfigName;
                DatabaseDataProviderConfig dbConfig = _sqlOffenderConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.CommandTimeout = 180;
                dbConfig.OpenCommand = "WMSOPEN";
                dbConfig.SQLText = SQLText;
                dbConfig.CloseCommand = "WMSCLOSE";
                dbConfig.RefreshRate = 30;
            }
            else
            {
                DatabaseDataProviderConfig dbConfig = tempSQLOffenderConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.SQLText = SQLText;
                _sqlOffenderConfig = tempSQLOffenderConfig;
                persistedColumnMappings = dbConfig.ColumnMappings;
            }
            _sqlOffenderConfig.Title = "";
            ((DatabaseDataProviderConfig)_sqlOffenderConfig.DataProviderConfig).TimerPaused = false;

            _sqlOffenderConfig.DataProviderConfig.DefaultVisibleColumnNames = DefaultVisibleColumns;
            _sqlOffenderConfig.DataProviderConfig.ColumnMappings = ColumnMappings;
            //ColumnMapping.Synchronize(_sqlOffenderConfig.DataProviderConfig.ColumnMappings, persistedColumnMappings);

            //Set the connection definition if available
            _sqlOffenderConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;

            //Set to continue the timer even when execption encountered. 
            _sqlOffenderConfig.DataProviderConfig.TimerContinuesOnError = true;

            _sqlOffenderConfig.ShowHelpButton = true;
            _sqlOffenderConfig.HelpTopic = HelpTopic;


            //Create a UW using the configuration             
            _sqlOffenderWidget = new GenericUniversalWidget();
            _sqlOffenderWidget.DataProvider = new DatabaseDataProvider(_sqlOffenderConfig.DataProviderConfig);
            ((TabularDataDisplayControl)_sqlOffenderWidget.DataDisplayControl).LineCountFormat = TitleName;

            _sqlOffenderWidget.UniversalWidgetConfiguration = _sqlOffenderConfig;
            _sqlOffenderWidget.Dock = DockStyle.Fill;
            _theWidgetGroupBox.Controls.Add(_sqlOffenderWidget);

            //Add custom toolstrip buttons
            AddToolStripButtons();

            //Add popup menu items to the table
            TabularDataDisplayControl dataDisplayControl = _sqlOffenderWidget.DataDisplayControl as TabularDataDisplayControl;
            if (dataDisplayControl != null)
            {
                _workloadDetailMenuItem = new TrafodionIGridToolStripMenuItem();
                _workloadDetailMenuItem.Text = "Workload Detail...";
                _workloadDetailMenuItem.Enabled = false;
                _workloadDetailMenuItem.Click += new EventHandler(workloadDetailMenuItem_Click);
                dataDisplayControl.AddMenuItem(_workloadDetailMenuItem);
                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                _displaySQLTextMenuItem = new TrafodionIGridToolStripMenuItem();
                _displaySQLTextMenuItem.Text = "Display Full SQL Text...";
                _displaySQLTextMenuItem.Enabled = false;
                _displaySQLTextMenuItem.Click += new EventHandler(displaySQLTextMenuItem_Click);
                dataDisplayControl.AddMenuItem(_displaySQLTextMenuItem);

                _displaySQLPlanMenuItem = new TrafodionIGridToolStripMenuItem();
                _displaySQLPlanMenuItem.Text = "Display SQL Plan...";
                _displaySQLPlanMenuItem.Enabled = false;
                _displaySQLPlanMenuItem.Click += new EventHandler(displaySQLPlanMenuItem_Click);
                dataDisplayControl.AddMenuItem(_displaySQLPlanMenuItem);
            }

            _sqlOffenderWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            _sqlOffenderWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;
            _sqlOffenderWidget.DataProvider.OnErrorEncountered += InvokeHandleError;

            _sqlOffenderIGrid = ((TabularDataDisplayControl)_sqlOffenderWidget.DataDisplayControl).DataGrid;
            _sqlOffenderIGrid.CellMouseDown += new iGCellMouseDownEventHandler(_sqlOffenderIGrid_CellMouseDown);
            _sqlOffenderIGrid.DoubleClickHandler = this._sqlOffenderIGrid_DoubleClick;
            _sqlOffenderIGrid.RowMode = true;

            _sqlOffenderWidget.StartDataProvider();
        }

        void DataProvider_OnErrorEncountered(DataProviderEventArgs e)
        {
            throw new NotImplementedException();
        }

        public abstract String SQLText
        {
            get;

        }

        public abstract List<string> DefaultVisibleColumns
        {
            get;
        }

        public abstract List<ColumnMapping> ColumnMappings
        {
            get;
        }

        void _sqlOffenderIGrid_CellMouseDown(object sender, iGCellMouseDownEventArgs e)
        {
            if (e.RowIndex >= 0 && e.Button == MouseButtons.Right)
            {
                iGRow row = _sqlOffenderIGrid.Rows[e.RowIndex];
                iGRowCellCollection coll = row.Cells;

                if (e.ColIndex >= 0)
                {
                    row.Cells[e.ColIndex].Selected = true;
                }
                string query_id = (string)coll["SQL_QUERY_ID"].Value;
                _workloadDetailMenuItem.Enabled = _displaySQLTextMenuItem.Enabled = _displaySQLPlanMenuItem.Enabled = !string.IsNullOrEmpty(query_id);
            }
        }

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
        }

        private void InvokeHandleError(object sender, DataProviderEventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateError(HandleError), new object[] { sender, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HandleError(object sender, DataProviderEventArgs e)
        {

        }
        
        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "Begin System Offender Fetch", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            }
        }
        
        void _sqlOffenderIGrid_DoubleClick(int row)
		{
			invokeWorkloadDetail(row);
		}

        void displaySQLTextMenuItem_Click(object sender, EventArgs events)
        {
            //DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, "SQL_QUERY_ID"].Value as string;
                    string query_text = "";
                    if (iGrid.Cols.KeyExists("SQL_QUERY_TEXT"))
                        query_text = iGrid.Cells[row, "SQL_QUERY_TEXT"].Value as string;

                    int length = Int32.MaxValue;
                    string mxosrvrStartTime = "";
                    string title = string.Format(Properties.Resources.TitleQueryText, query_id);
                    WorkloadPlanControl wpc = new WorkloadPlanControl(_theConnectionDefinition, query_id, mxosrvrStartTime, query_text, length, false, "", false);
                    Utilities.LaunchManagedWindow(title, wpc, _theConnectionDefinition, ChildrenWindowSize, true);
                }
            }
        }

        void displaySQLPlanMenuItem_Click(object sender, EventArgs events)
        {
            //DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, "SQL_QUERY_ID"].Value as string;
                    string query_text = "";
                    if (iGrid.Cols.KeyExists("SQL_QUERY_TEXT"))
                        query_text = iGrid.Cells[row, "SQL_QUERY_TEXT"].Value as string;
                    int length = Int32.MaxValue;
                    string mxosrvrStartTime = "";
                    string dataSource = "";
                    string title = string.Format(Properties.Resources.TitleQueryPlan, query_id);
                    WorkloadPlanControl wpc = new WorkloadPlanControl(_theConnectionDefinition, query_id, mxosrvrStartTime, query_text, length, true, dataSource, true);
                    Utilities.LaunchManagedWindow(title, wpc, _theConnectionDefinition, ChildrenWindowSize, true);
                }
            }
        }

		/// <summary>
		/// Cleanup
		/// </summary>
		/// <param name="disposing"></param>
		void MyDispose(bool disposing)
		{
			if (disposing)
			{
                if (_sqlOffenderWidget != null)
                {
                    _sqlOffenderWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                    _sqlOffenderWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData; 
                    _sqlOffenderWidget.DataProvider.Stop();
                }
			}
		}

		/// <summary>
		/// Add custom tool strip buttons pertaining to WMS Offender
		/// </summary>
		private void AddToolStripButtons()
		{

		}


		void DisplayErrorPanel(string errorMessage)
		{
			Controls.Clear();

			TrafodionLabel errorLabel = new TrafodionLabel();
			errorLabel.Text = errorMessage;
			errorLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			errorLabel.Dock = DockStyle.Fill;

			Controls.Add(errorLabel);
		}

		private void invokeWorkloadDetail(int row)
		{
            string query_id = _sqlOffenderIGrid.Cells[row, "SQL_QUERY_ID"].Value as string;
            if (string.IsNullOrEmpty(query_id)) //If query id null or blank, no details to fetch.
                return;

            DatabaseDataProviderConfig dbConfig = _sqlOffenderConfig.DataProviderConfig as DatabaseDataProviderConfig;
			Connection _conn = GetConnection(dbConfig.ConnectionDefinition);
			if (_conn != null)
			{
				OdbcConnection odbcCon = _conn.OpenOdbcConnection;
				OdbcCommand command = new OdbcCommand();
				bool wmsOpened = false;
				try
				{
					Cursor.Current = Cursors.WaitCursor;
					command.Connection = odbcCon;
					command.CommandTimeout = 180;
					command.CommandText = "WMSOPEN";
					command.ExecuteNonQuery();
					wmsOpened = true;
                    string sql = "STATUS QUERY " + query_id + " MERGED";
					DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);

					if (dataTable.Rows.Count > 0)
					{
						DataRow dr = dataTable.Rows[0];

                        string mxosrvrStartTime = "";
                        if (dataTable.Columns.Contains(WmsCommand.COL_QUERY_START_TIME))
                        {
                            mxosrvrStartTime = dataTable.Rows[0][ WmsCommand.COL_QUERY_START_TIME] as string;
                        }
                        string title = string.Format(Properties.Resources.TitleQueryDetails, string.Format("{0}@{1}", query_id, mxosrvrStartTime));
                        WMSQueryDetailUserControl queryInfo = GetWatchedQueryWindow(_theConnectionDefinition, query_id, mxosrvrStartTime);
                        if (queryInfo != null)
                        {
                            queryInfo.LoadData(dataTable);
                            string systemIdentifier = (_theConnectionDefinition != null) ? _theConnectionDefinition.Name + " : " : "";
                            string windowTitle = TrafodionForm.TitlePrefix + systemIdentifier + title;
                            if (WindowsManager.Exists(windowTitle))
                            {
                                WindowsManager.Restore(windowTitle);
                                WindowsManager.BringToFront(windowTitle);
                            }
                        }
                        else
                        {
                            WMSQueryDetailUserControl queryDetails = new WMSQueryDetailUserControl(this, _theConnectionDefinition, query_id, mxosrvrStartTime, dataTable);
                            AddQueryToWatch(queryDetails);

                            // Launch a managed window with persistence.
                            Utilities.LaunchManagedWindow(string.Format(Properties.Resources.TitleQueryDetails, string.Format("{0}@{1}", query_id, mxosrvrStartTime)),
                                queryDetails, true, _theConnectionDefinition, WMSQueryDetailUserControl.IdealWindowSize, false);
                        }

                    }
					else
					{
                        MessageBox.Show("Query statistics not available for the selected query in WMS. It may be that the query has already completed.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Warning);
					}
				}
				catch (OdbcException ex)
				{
                    MessageBox.Show("Error: Query statistics not available for the selected query in WMS. It may be that the query has already completed.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Warning);
				}
				finally
				{
					if (wmsOpened)
					{
						command.CommandText = "WMSCLOSE";
						command.ExecuteNonQuery();
					}
					if (_conn != null)
					{
						_conn.Close();
					}
					Cursor.Current = Cursors.Default;
				}
			}
		}

		private void invokeWorkloadDetail(object sender, EventArgs events)
		{
			TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
			if (mi != null)
			{
				TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
				if (eventObj != null)
				{
					invokeWorkloadDetail(eventObj.Row);
				}
			}
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void workloadDetailMenuItem_Click(object sender, EventArgs events)
		{
			invokeWorkloadDetail(sender, events);
		}

        private void getTransButton_Click(object sender, EventArgs events)
        {
            TransactionUserControl transactionUserControl = new TransactionUserControl(_theConnectionDefinition);
            Utilities.LaunchManagedWindow(Properties.Resources.TitleTransactionsOfAllNodes, transactionUserControl, _theConnectionDefinition, TransactionUserControl.IdealWindowSize, true);
        }

        protected string getDisplayColumnName(string colName)
        {
            return TrafodionIGridUtils.ConvertUnderlineToBreak(colName);
        }
	}

    public class CPUOffenderWidget : SQLOffenderWidget
    {
        public CPUOffenderWidget(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }

        public override string HelpTopic
        {
            get { return HelpTopics.UsingSQLCPUOffender; }
        }

        public override string ConfigName
        {
            get { return "CPUOffenderConfig"; }
        }

        public override string TitleName
        {
            get { return "Workloads using the most SQL CPU time"; }
        }

        public override string SQLText
        {
            get { return "STATUS QUERIES OFFENDER CPU"; }
        }

        public override List<string> DefaultVisibleColumns
        {
            get 
            {
                List<string> defaultVisibleColumns = new List<string>();
                defaultVisibleColumns.Add("SQL_CURRENT_TIME");
                defaultVisibleColumns.Add("SQL_QUERY_ID");
                defaultVisibleColumns.Add("SQL_NUM_PROCESSES");
                defaultVisibleColumns.Add("SQL_DIFF_CPU_TIME_SECS");
                return defaultVisibleColumns;
            }
        }

        public override List<ColumnMapping> ColumnMappings
        {
            get
            {
                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("SQL_CURRENT_TIME", getDisplayColumnName("Time"), 150));
                columnMappings.Add(new ColumnMapping("SQL_QUERY_ID", getDisplayColumnName("Query ID"), 500));
                columnMappings.Add(new ColumnMapping("SQL_NUM_PROCESSES", getDisplayColumnName("Number of_SQL Processes"), 100));
                columnMappings.Add(new ColumnMapping("SQL_DIFF_CPU_TIME_SECS", getDisplayColumnName("Delta SQL_CPU Time (secs)"), 150));
                return columnMappings;
            }
        }
    }
    public class SEOffenderWidget : SQLOffenderWidget
    {
        public SEOffenderWidget(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }
        public override string HelpTopic
        {
            get { return HelpTopics.UsingSECPUOffender; }
        }

        public override string ConfigName
        {
            get { return "SEOffenderConfig"; }
        }

        public override string TitleName
        {
            get { return "Workloads using the most SE CPU time"; }
        }

        public override string SQLText
        {
            get { return "STATUS QUERIES OFFENDER TSE"; }
        }

        public override List<string> DefaultVisibleColumns
        {
            get
            {
                List<string> defaultVisibleColumns = new List<string>();
                defaultVisibleColumns.Add("SQL_CURRENT_TIME");
                defaultVisibleColumns.Add("SQL_QUERY_ID");
                defaultVisibleColumns.Add("SQL_NUM_SE_SESSIONS");
                defaultVisibleColumns.Add("SQL_DIFF_CPU_TIME_SECS");
                defaultVisibleColumns.Add("SQL_TABLE_NAME");
                return defaultVisibleColumns;
            }
        }

        public override List<ColumnMapping> ColumnMappings
        {
            get
            {
                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("SQL_CURRENT_TIME", getDisplayColumnName("Time"), 150));
                columnMappings.Add(new ColumnMapping("SQL_QUERY_ID", getDisplayColumnName("Query ID"), 500));
                columnMappings.Add(new ColumnMapping("SQL_NUM_SE_SESSIONS", getDisplayColumnName("Number of_SE Sessions"), 150));
                columnMappings.Add(new ColumnMapping("SQL_DIFF_CPU_TIME_SECS", getDisplayColumnName("Delta SE_CPU Time (secs)"), 150));
                columnMappings.Add(new ColumnMapping("SQL_TABLE_NAME", getDisplayColumnName("Table Name"), 300));
                return columnMappings;
            }
        }
    }
    public class SlowOffenderWidget : SQLOffenderWidget
    {
        public SlowOffenderWidget(ConnectionDefinition aConnectionDefinition)
            : base(aConnectionDefinition)
        {
        }

        public override string HelpTopic
        {
            get { return HelpTopics.UsingSlowOffender; }
        }

        public override string ConfigName
        {
            get { return "SlowOffenderConfig"; }
        }

        public override string TitleName
        {
            get { return  "Slow Workloads likely blocked in SQL"; }
        }

        public override string SQLText
        {
            get { return "STATUS QUERIES OFFENDER SLOW"; }
        }

        public override List<string> DefaultVisibleColumns
        {
            get
            {
                List<string> defaultVisibleColumns = new List<string>();
                defaultVisibleColumns.Add("SQL_CURRENT_TIME");
                defaultVisibleColumns.Add("SQL_QUERY_ID");
                defaultVisibleColumns.Add("SQL_BLOCKED_IN_SQL_SECS");
                defaultVisibleColumns.Add("SQL_EXEC_STATE");
                defaultVisibleColumns.Add("SQL_QUERY_TEXT");
                return defaultVisibleColumns;
            }
        }

        public override List<ColumnMapping> ColumnMappings
        {
            get
            {
                List<ColumnMapping> columnMappings = new List<ColumnMapping>();
                columnMappings.Add(new ColumnMapping("SQL_CURRENT_TIME", getDisplayColumnName("Time"), 150));
                columnMappings.Add(new ColumnMapping("SQL_QUERY_ID", getDisplayColumnName("Query ID"), 500));
                columnMappings.Add(new ColumnMapping("SQL_BLOCKED_IN_SQL_SECS", getDisplayColumnName("Delta SQL_Blocked Time (secs)"), 150));
                columnMappings.Add(new ColumnMapping("SQL_EXEC_STATE", getDisplayColumnName("Exec State"), 150));
                columnMappings.Add(new ColumnMapping("SQL_QUERY_TEXT", getDisplayColumnName("Query Text"), 400));
                return columnMappings;
            }
        }
    }
}
