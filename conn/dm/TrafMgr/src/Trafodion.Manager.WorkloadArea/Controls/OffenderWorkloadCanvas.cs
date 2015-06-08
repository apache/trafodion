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
using System.Collections;
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
    public partial class OffenderWorkloadCanvas : WMSWorkloadCanvas, ICloneToWindow
	{
		#region Static Members
		static readonly string OffenderWorkloadConfigName = "WMS_OffenderWorkloadConfig";
		static readonly string OffenderPersistenceKey = "OffenderOptionsPersistence";
		private static readonly string STATUS_TACLPROC = "STATUS TACLPROC 1.0.999";
		private static readonly string STATUS_TACLPROC_SQ = "STATUS PROCESS $ZWSYN";
        private static Size ChildrenWindowSize = new Size(800, 600);
#if INC_COMMANDS
        static readonly string OffenderStatusPersistenceKey = "OffenderStatusOptionsPersistence";
#endif
		#endregion

		#region Members
        public delegate void UpdateStatus(Object obj, EventArgs e);
        Persistence.PersistenceHandler _persistenceHandler = null;
		UniversalWidgetConfig OffenderWorkloadConfig = null;
		GenericUniversalWidget OffenderForCPUWidget;
		ConnectionDefinition _theConnectionDefinition;
		//SystemMonitorControl _theSystemMonitorControl;
		String _theTitle = "Offending Workloads";
		ArrayList _commands = new ArrayList();
		//SystemMonitorControl.ChangingHandler _theSystemMonitorEventHandler = null;
		Connection _conn = null;
        TrafodionIGridToolStripMenuItem _workloadDetailMenuItem = null;
        TrafodionIGridToolStripMenuItem _displayParentChildQueriesMenuItem = null;        
		TrafodionIGridToolStripMenuItem _processDetailChildMenuItem = null;
		TrafodionIGridToolStripMenuItem _processDetailParentMenuItem = null;
		TrafodionIGridToolStripMenuItem _pstateChildMenuItem = null;
		TrafodionIGridToolStripMenuItem _pstateParentMenuItem = null;
		TrafodionIGridToolStripMenuItem _childrenProcessesMenuItem = null;
        private TrafodionIGridToolStripMenuItem _reorgProgressMenuItem = null;
		TrafodionIGrid _offenderIGrid = null;
		string _processDetailChild = ""; //seg.cpu.pin
		string _processDetailParent = ""; //seg.cpu.pin
		string _pstateChild = ""; //seg.cpu.pin
		string _pstateParent = ""; //seg.cpu.pin
		string _parentProcess = ""; //eg. \wms0101.$mxoas
		string _queryID = "";
		string _processType = " PROCESS ALL";
		string _statusCommand = "STATUS CPU";
		WMSOffenderOptions _offenderOptions = null;
		bool _checkOffenderRequire = true;
		bool _offenderAvailable = false;
#if INC_COMMANDS
        WMSOffenderStatusOptions _offenderStatusOptions = null;
#endif
		#endregion

		public ConnectionDefinition ConnectionDefinition
		{
			get { return _theConnectionDefinition; }
			set
			{
				if (_theConnectionDefinition != null && OffenderForCPUWidget != null)
				{
					OffenderForCPUWidget.DataProvider.Stop();
				}

				if (_theConnectionDefinition != null && value != null && !_theConnectionDefinition.Equals(value))
				{
					_offenderIGrid.Rows.Clear();
				}
				_theConnectionDefinition = value;
				if ((OffenderWorkloadConfig != null) && (OffenderWorkloadConfig.DataProviderConfig != null))
				{
					OffenderWorkloadConfig.DataProviderConfig.ConnectionDefinition = value;
				}

				if (_theConnectionDefinition != null)
				{
					if (OffenderForCPUWidget != null)
					{
						OffenderForCPUWidget.DataProvider.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;
						OffenderForCPUWidget.DataProvider.Start();
					}
					else
					{
						ShowWidgets();
					}
				}


			}
		}


		public OffenderWorkloadCanvas()
		{
			InitializeComponent();
		}

		public OffenderWorkloadCanvas(ConnectionDefinition aConnectionDefinition)
		{
			InitializeComponent();
            _theConnectionDefinition = aConnectionDefinition;
			ShowWidgets();
		}

		private void LoadOffenderOptions()
		{
			try
			{
				this._offenderOptions = Trafodion.Manager.Framework.Persistence.Get(OffenderPersistenceKey) as WMSOffenderOptions;

				if (_offenderOptions == null)
				{
					_offenderOptions = new WMSOffenderOptions();
				}
			}
			catch (Exception ex)
			{
				_offenderOptions = new WMSOffenderOptions();
			}
			//Console.WriteLine("Load Offender Options " + Environment.NewLine + 
			//                  "\tSampleInterval " + _offenderOptions.SampleInterval + Environment.NewLine +
			//                  "\tSampleCPUs " + _offenderOptions.SampleCPUs + Environment.NewLine +
			//                  "\tSampleCache " + _offenderOptions.SampleCache + Environment.NewLine +
			//                  "\tSQLProcess " + _offenderOptions.SQLProcess + Environment.NewLine); 
#if INC_COMMANDS
            try
            {
                this._offenderStatusOptions = Trafodion.Manager.Framework.Persistence.Get(OffenderStatusPersistenceKey) as WMSOffenderStatusOptions;

                if (_offenderStatusOptions == null)
                {
                    _offenderStatusOptions = new WMSOffenderStatusOptions();
                }
            }
            catch (Exception ex)
            {
                _offenderStatusOptions = new WMSOffenderStatusOptions();
            }
            //Console.WriteLine("Load Status Offender Options " + Environment.NewLine +
            //                  "\tStatusCpu " + _offenderStatusOptions.StatusCpu + Environment.NewLine +
            //                  "\tUseCpu " + _offenderStatusOptions.UseCpu + Environment.NewLine +
            //                  "\tCpuNumber " + _offenderStatusOptions.CpuNumber + Environment.NewLine +
            //                  "\tUseSegment " + _offenderStatusOptions.UseSegment + Environment.NewLine +
            //                  "\tSegmentNumber " + _offenderStatusOptions.SegmentNumber + Environment.NewLine +
            //                  "\tSQLProcess " + _offenderStatusOptions.SQLProcess + Environment.NewLine +
            //                  "\tStatusCommand " + _offenderStatusOptions.StatusCommand + Environment.NewLine);
#endif
			_processType = _offenderOptions.SQLProcess ? " PROCESS SQL" : " PROCESS ALL";
			_statusCommand = _offenderOptions.StatusOffender == WMSOffenderOptions.STATUS_OFFENDER.CPU ? "STATUS CPU" : "STATUS MEM";
		}

		public void ShowWidgets()
		{
			//Create the configuration. If one is persisted, we use that otherwise we create one
			UniversalWidgetConfig tempConfig = WidgetRegistry.GetConfigFromPersistence(OffenderWorkloadConfigName);

			LoadOffenderOptions();

			//Register to listen to persistence events, so you can save options, when notified.
			_persistenceHandler = new Persistence.PersistenceHandler(Persistence_PersistenceHandlers);
			Persistence.PersistenceHandlers += _persistenceHandler;
            List<ColumnMapping> persistedColumnMappings = null;
			if (tempConfig == null)
			{
                OffenderWorkloadConfig = WidgetRegistry.GetDefaultDBConfig();
				OffenderWorkloadConfig.Name = OffenderWorkloadConfigName;
				OffenderWorkloadConfig.Title = _theTitle;
				DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
				dbConfig.CommandTimeout = WORKLOAD_EXEC_TIMEOUT;
				dbConfig.OpenCommand = "WMSOPEN";
				//dbConfig.SQLText = "STATUS CPU" + _processType;
				dbConfig.SQLText = _statusCommand + _processType;
				dbConfig.CloseCommand = "WMSCLOSE";
				dbConfig.RefreshRate = WORKLOAD_REFRESH_RATE;				
			}
			else
			{
                DatabaseDataProviderConfig dbConfig = tempConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.SQLText = _statusCommand + _processType;
                OffenderWorkloadConfig = tempConfig;
                persistedColumnMappings = dbConfig.ColumnMappings;
			}

            ((DatabaseDataProviderConfig)OffenderWorkloadConfig.DataProviderConfig).TimerPaused = false;

            List<string> defaultVisibleColumns = new List<string>();
            defaultVisibleColumns.Add("CPU_USAGE_PERCENT");
            defaultVisibleColumns.Add("MEMORY_USAGE_MB");
            defaultVisibleColumns.Add("ELAPSED_TIME");
            defaultVisibleColumns.Add("DATASOURCE");
            defaultVisibleColumns.Add("QUERY_NAME");
            defaultVisibleColumns.Add("QUERY_ID");
            defaultVisibleColumns.Add("QUERY_TEXT");
            defaultVisibleColumns.Add("NODE");
            defaultVisibleColumns.Add("PID");
            defaultVisibleColumns.Add("PARENT_NODE");
            defaultVisibleColumns.Add("PARENT_PID");
            defaultVisibleColumns.Add("PROCESS_TYPE");
            defaultVisibleColumns.Add("PROCESS_NAME");
            defaultVisibleColumns.Add("PROCESS_PRIORITY");
            defaultVisibleColumns.Add("PARENT_PROCESS_TYPE");
            defaultVisibleColumns.Add("PARENT_PROCESS_NAME");
            defaultVisibleColumns.Add("PARENT_PROCESS_PRIORITY");
            OffenderWorkloadConfig.DataProviderConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

            ColumnMapping cm = null;
            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping("CPU_USAGE_PERCENT", getDisplayColumnName("CPU_USAGE_PERCENT"), 50));
            columnMappings.Add(new ColumnMapping("MEMORY_USAGE_MB", getDisplayColumnName("MEMORY_USAGE_MB"), 50));
            columnMappings.Add(new ColumnMapping("ELAPSED_TIME", getDisplayColumnName("ELAPSED_TIME"), 75));
            columnMappings.Add(new ColumnMapping("DATASOURCE", getDisplayColumnName("DATASOURCE"), 150));
            columnMappings.Add(new ColumnMapping("QUERY_NAME", getDisplayColumnName("QUERY_NAME"), 150));
            columnMappings.Add(new ColumnMapping("QUERY_ID", getDisplayColumnName("QUERY_ID"), 150));
            columnMappings.Add(new ColumnMapping("QUERY_TEXT", getDisplayColumnName("QUERY_TEXT"), 150));
            columnMappings.Add(new ColumnMapping("NODE", getDisplayColumnName("NODE"), 75));
            columnMappings.Add(new ColumnMapping("PID", getDisplayColumnName("PID"), 40));
            columnMappings.Add(new ColumnMapping("PARENT_NODE", getDisplayColumnName("PARENT_NODE"), 75));
            columnMappings.Add(new ColumnMapping("PARENT_PID", getDisplayColumnName("PARENT_PID"), 75));
            columnMappings.Add(new ColumnMapping("PROCESS_TYPE", getDisplayColumnName("PROCESS_TYPE"), 75));
            columnMappings.Add(new ColumnMapping("PROCESS_NAME", getDisplayColumnName("PROCESS_NAME"), 75));
            columnMappings.Add(new ColumnMapping("PROCESS_PRIORITY", getDisplayColumnName("PROCESS_PRIORITY"), 75));
            columnMappings.Add(new ColumnMapping("PARENT_PROCESS_TYPE", getDisplayColumnName("PARENT_PROCESS_TYPE"), 75));
            columnMappings.Add(new ColumnMapping("PARENT_PROCESS_NAME", getDisplayColumnName("PARENT_PROCESS_NAME"), 75));
            columnMappings.Add(new ColumnMapping("PARENT_PROCESS_PRIORITY", getDisplayColumnName("PARENT_PROCESS_PRIORITY"), 75));
            OffenderWorkloadConfig.DataProviderConfig.ColumnMappings = columnMappings;
            ColumnMapping.Synchronize(OffenderWorkloadConfig.DataProviderConfig.ColumnMappings, persistedColumnMappings);

			//Set the connection definition if available
			OffenderWorkloadConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;

            //Set to continue the timer even when execption encountered. 
            OffenderWorkloadConfig.DataProviderConfig.TimerContinuesOnError = true;

            OffenderWorkloadConfig.ShowHelpButton = true;
            OffenderWorkloadConfig.HelpTopic = HelpTopics.UsingSystemOffender;


			//Create a UW using the configuration             
			OffenderForCPUWidget = new GenericUniversalWidget();
            OffenderForCPUWidget.DataProvider = new DatabaseDataProvider(OffenderWorkloadConfig.DataProviderConfig);
            ((TabularDataDisplayControl)OffenderForCPUWidget.DataDisplayControl).LineCountFormat = "Offending Processes";

            OffenderForCPUWidget.UniversalWidgetConfiguration = OffenderWorkloadConfig;
            OffenderForCPUWidget.Dock = DockStyle.Fill;

            DatabaseDataProviderConfig dbConfig1 = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            _theWidgetGroupBox.Controls.Add(OffenderForCPUWidget);
            SetTitle();

			//Add custom toolstrip buttons
			AddToolStripButtons();

			//Add popup menu items to the table
			TabularDataDisplayControl dataDisplayControl = OffenderForCPUWidget.DataDisplayControl as TabularDataDisplayControl;
			if (dataDisplayControl != null)
			{
				_workloadDetailMenuItem = new TrafodionIGridToolStripMenuItem();
				_workloadDetailMenuItem.Text = "Workload Detail...";
				_workloadDetailMenuItem.Enabled = false;
				_workloadDetailMenuItem.Click += new EventHandler(workloadDetailMenuItem_Click);
				dataDisplayControl.AddMenuItem(_workloadDetailMenuItem);

				dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    _displayParentChildQueriesMenuItem = new TrafodionIGridToolStripMenuItem();
                    _displayParentChildQueriesMenuItem.Text = "Display Parent/Child Queries...";
                    _displayParentChildQueriesMenuItem.Click += new EventHandler(DisplayParentChildQueriesMenuItem_Click);
                    dataDisplayControl.AddMenuItem(_displayParentChildQueriesMenuItem);
                }

                _reorgProgressMenuItem = new TrafodionIGridToolStripMenuItem();
                _reorgProgressMenuItem.Text = Properties.Resources.ReorgProgressMenuItemText;
                _reorgProgressMenuItem.Enabled = false;
                _reorgProgressMenuItem.Click += new EventHandler(reorgProgressMenuItem_Click);
                dataDisplayControl.AddMenuItem(_reorgProgressMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

				_processDetailChildMenuItem = new TrafodionIGridToolStripMenuItem();
				_processDetailChildMenuItem.Text = "Process Detail...";
				_processDetailChildMenuItem.Enabled = false;
				_processDetailChildMenuItem.Click += new EventHandler(prcoessDetailChildMenuItem_Click);
				dataDisplayControl.AddMenuItem(_processDetailChildMenuItem);

				_processDetailParentMenuItem = new TrafodionIGridToolStripMenuItem();
				_processDetailParentMenuItem.Text = "Parent Process Detail...";
				_processDetailParentMenuItem.Enabled = false;
				_processDetailParentMenuItem.Click += new EventHandler(prcoessDetailParentMenuItem_Click);
				dataDisplayControl.AddMenuItem(_processDetailParentMenuItem);

				_pstateChildMenuItem = new TrafodionIGridToolStripMenuItem();
				_pstateChildMenuItem.Text = "Pstate...";
				_pstateChildMenuItem.Enabled = false;
				_pstateChildMenuItem.Click += new EventHandler(pstateChildMenuItem_Click);
				dataDisplayControl.AddMenuItem(_pstateChildMenuItem);

				_pstateParentMenuItem = new TrafodionIGridToolStripMenuItem();
				_pstateParentMenuItem.Text = "Parent Pstate...";
				_pstateParentMenuItem.Enabled = false;
				_pstateParentMenuItem.Click += new EventHandler(pstateParentMenuItem_Click);
				dataDisplayControl.AddMenuItem(_pstateParentMenuItem);

				_childrenProcessesMenuItem = new TrafodionIGridToolStripMenuItem();
				_childrenProcessesMenuItem.Text = "Children Processes...";
				_childrenProcessesMenuItem.Enabled = false;
				_childrenProcessesMenuItem.Click += new EventHandler(childrenProcessesMenuItem_Click);
				dataDisplayControl.AddMenuItem(_childrenProcessesMenuItem);
			}

            OffenderForCPUWidget.DataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            OffenderForCPUWidget.DataProvider.OnFetchingData += InvokeHandleFetchingData;

			//Associate the custom data display handler for the TabularDisplay panel
			OffenderForCPUWidget.DataDisplayControl.DataDisplayHandler = new MyOffenderDataHandler(this);

			//Add Disposed handler to clean up, connection
			this.Disposed += new EventHandler(OffenderWorkloadCanvas_Disposed);

			_offenderIGrid = ((TabularDataDisplayControl)OffenderForCPUWidget.DataDisplayControl).DataGrid;
			_offenderIGrid.CellClick += new iGCellClickEventHandler(offenderIGrid_CellClick);
			_offenderIGrid.CellMouseDown += new iGCellMouseDownEventHandler(offenderIGrid_CellMouseDown);
			_offenderIGrid.DoubleClickHandler = this._offenderIGrid_DoubleClick;
            _offenderIGrid.RowMode = true;
            //Show for all CPU and segments
            OffenderForCPUWidget.StartDataProvider();
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
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "End System Offender Fetch", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            }
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
        
        void _offenderIGrid_DoubleClick(int row)
		{
			invokeWorkloadDetail(row);
		}

		void Persistence_PersistenceHandlers(Dictionary<string, object> aDictionary, Persistence.PersistenceOperation aPersistenceOperation)
		{
			//When framework notifies the save persistence event, do your part
			if (aPersistenceOperation == Persistence.PersistenceOperation.Save)
			{
				SavePersistence();
			}
		}

		void SavePersistence()
		{
			try
			{
				Trafodion.Manager.Framework.Persistence.Put(OffenderPersistenceKey, _offenderOptions);
				//Console.WriteLine("Save Offender Options " + Environment.NewLine +
				//                  "\tSampleInterval " + _offenderOptions.SampleInterval + Environment.NewLine +
				//                  "\tSampleCPUs " + _offenderOptions.SampleCPUs + Environment.NewLine +
				//                  "\tSampleCache " + _offenderOptions.SampleCache + Environment.NewLine +
				//                  "\tSQLProcess " + _offenderOptions.SQLProcess + Environment.NewLine);
			}
			catch (Exception)
			{
			}
#if INC_COMMANDS
            try
            {
                Trafodion.Manager.Framework.Persistence.Put(OffenderStatusPersistenceKey, _offenderStatusOptions);
                //Console.WriteLine("Save Status Offender Options " + Environment.NewLine +
                //                  "\tStatusCpu " + _offenderStatusOptions.StatusCpu + Environment.NewLine +
                //                  "\tUseCpu " + _offenderStatusOptions.UseCpu + Environment.NewLine +
                //                  "\tCpuNumber " + _offenderStatusOptions.CpuNumber + Environment.NewLine +
                //                  "\tUseSegment " + _offenderStatusOptions.UseSegment + Environment.NewLine +
                //                  "\tSegmentNumber " + _offenderStatusOptions.SegmentNumber + Environment.NewLine +
                //                  "\tSQLProcess " + _offenderStatusOptions.SQLProcess + Environment.NewLine +
                //                  "\tStatusCommand " + _offenderStatusOptions.StatusCommand + Environment.NewLine);
            }
            catch (Exception)
            {
            }
#endif
		}

		void offenderIGrid_CellClick(object sender, iGCellClickEventArgs e)
		{
			offenderIGrid_SelectCell(e.RowIndex);
		}

		void offenderIGrid_CellMouseDown(object sender, iGCellMouseDownEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
                //_offenderIGrid.PerformAction(iGActions.DeselectAllCells);

				_offenderIGrid.Rows[e.RowIndex].Cells[e.ColIndex].Selected = true;
				_offenderIGrid.SetCurCell(e.RowIndex, e.ColIndex);

				offenderIGrid_SelectCell(e.RowIndex);
			}
		}

		void offenderIGrid_SelectCell(int rowIndex)
		{
			if (rowIndex >= 0)
			{
				iGRow row = _offenderIGrid.Rows[rowIndex];
				iGRowCellCollection coll = row.Cells;
                _queryID = (string)coll["QUERY_ID"].Value;
                bool isQueryIdValid = _queryID.Length > 0;

                _reorgProgressMenuItem.Enabled 
                    = _workloadDetailMenuItem.Enabled 
                    = isQueryIdValid;

                if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ150)
                {
                    _displayParentChildQueriesMenuItem.Enabled = isQueryIdValid;
                }

				_childrenProcessesMenuItem.Enabled = false;
				_parentProcess = (string)coll["PARENT_PROCESS_NAME"].Value;
				string parentProcessType = (string)coll["PARENT_PROCESS_TYPE"].Value;
				bool serverType = parentProcessType.Equals("MXOSRVR");
				_childrenProcessesMenuItem.Enabled = (_parentProcess.Length > 0 && serverType) ? true : false;

				string processName = coll["PROCESS_NAME"].Value.ToString();
				_processDetailChildMenuItem.Enabled = (processName.Length > 0) ? true : false;
				_processDetailChild = processName;

				_pstateChildMenuItem.Enabled = (processName.Length > 0) ? true : false;
				_pstateChild = processName;


				string parent_processName = coll["PARENT_PROCESS_NAME"].Value.ToString();
				_processDetailParent = parent_processName;

				_processDetailParentMenuItem.Enabled = (parent_processName.Length > 0) ? true : false;
				_processDetailParent = parent_processName;

				_pstateParentMenuItem.Enabled = (parent_processName.Length > 0) ? true : false;
				_pstateParent = parent_processName;

                _pstateParentMenuItem.Enabled = _theConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString())
                    && _pstateParentMenuItem.Enabled;
                _pstateChildMenuItem.Enabled = _theConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_STATUS.ToString()) 
                    && _pstateChildMenuItem.Enabled;

			}
		}

		private string getDisplayColumnName(string colName)
		{
			return TrafodionIGridUtils.ConvertUnderlineToBreak(colName);
		}

		void OffenderWorkloadCanvas_Disposed(object sender, EventArgs e)
		{
            if (_conn != null && _conn.IsConnectionOpen)
			{
				//_conn.Close();
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
                if (OffenderForCPUWidget != null)
                {
                    OffenderForCPUWidget.DataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                    OffenderForCPUWidget.DataProvider.OnFetchingData -= InvokeHandleFetchingData; 
                    OffenderForCPUWidget.DataProvider.Stop();
                    //OffenderForCPUWidget.DataProvider.StopTimer();
                }
			}
		}

/*		private void HandleNewDataArrived(Object obj, EventArgs e)
		{
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   "End System Offender Fetch", DateTime.Now.ToString(Utilities.DateTimeLongFormat24HourString));
            }
            //string query = SetTitle(e as DataProviderEventArgs);
			DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
			addCommandHistory(dbConfig.SQLText, "WMS");

            string[] watchedReorgQueries = GetWatchedReorgQueryList(dbConfig.ConnectionDefinition);
            foreach (string queryId in watchedReorgQueries)
            {
                string[] ids = queryId.Split('@');
                string qid = ids[0];
                string start_ts = (ids.Length > 1) ? ids[1] : "";

                ReorgProgressUserControl reorgWindow = GetWatchedReorgProgressWindow(dbConfig.ConnectionDefinition, qid, start_ts);
                if (reorgWindow != null)
                {
                    reorgWindow.DoRefresh();
                }
            }
		}
        */
        //sets the title with the query that got executed
        private void SetTitle()
        {
            _theWidgetGroupBox.Text = _theTitle + String.Format(" ({0})", ((DatabaseDataProviderConfig)OffenderWorkloadConfig.DataProviderConfig).SQLText);
        }

        public void HandleMouseClickCPU(ShowOffenderEventArgs args)
		{
            OffenderForCPUWidget.DataProvider.Stop();
            DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            if (args.TheCommandType == ShowOffenderEventArgs.CommandType.MEM || args.TheCommandType == ShowOffenderEventArgs.CommandType.SWAP)
            {
                dbConfig.SQLText = "STATUS MEM" + _processType;
            }
            else
            {
                dbConfig.SQLText = "STATUS CPU" + _processType;
            }
            SetTitle();
            OffenderForCPUWidget.StartDataProvider();
        }

		/// <summary>
		/// Add custom tool strip buttons pertaining to WMS Offender
		/// </summary>
		private void AddToolStripButtons()
		{

			ToolStripButton alterOffenderButton = new ToolStripButton();
			alterOffenderButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			alterOffenderButton.Image = (System.Drawing.Image)Properties.Resources.AlterIcon;
			alterOffenderButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			alterOffenderButton.Name = "alterOffenderButton";
			alterOffenderButton.Size = new System.Drawing.Size(23, 22);
			alterOffenderButton.Text = "Alter Offender parameters";
			alterOffenderButton.Click += new EventHandler(alterOffenderButton_Click);
            alterOffenderButton.Enabled = this.ConnectionDefn.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString());
			OffenderForCPUWidget.AddToolStripItem(alterOffenderButton);
			//OffenderForCPUWidget.AddToolStripItem(new ToolStripSeparator());

#if INC_COMMANDS
            ToolStripButton statusCpuButton = new ToolStripButton();
            statusCpuButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            statusCpuButton.Image = (System.Drawing.Image)Properties.Resources.CPU;
            statusCpuButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            statusCpuButton.Name = "statusCpuButton";
            statusCpuButton.Size = new System.Drawing.Size(23, 22);
            statusCpuButton.Text = "Busiest workload - all CPU";
            statusCpuButton.Click += new EventHandler(statusCpuButton_Click);
            OffenderForCPUWidget.AddToolStripItem(statusCpuButton);

            ToolStripButton statusMemButton = new ToolStripButton();
            statusMemButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            statusMemButton.Image = (System.Drawing.Image)Properties.Resources.Memory;
            statusMemButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            statusMemButton.Name = "statusMemButton";
            statusMemButton.Size = new System.Drawing.Size(23, 22);
            statusMemButton.Text = "High memory usage - all CPU";
            statusMemButton.Click += new EventHandler(statusMemButton_Click);
            OffenderForCPUWidget.AddToolStripItem(statusMemButton);
 
            ToolStripButton statusCommandButton = new ToolStripButton();
            statusCommandButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            statusCommandButton.Image = (System.Drawing.Image)Properties.Resources.StatusCommand;
            statusCommandButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            statusCommandButton.Name = "statusCommandButton";
            statusCommandButton.Size = new System.Drawing.Size(23, 22);
            statusCommandButton.Text = "Status command";
            statusCommandButton.Click += new EventHandler(statusCommandButton_Click);
            OffenderForCPUWidget.AddToolStripItem(statusCommandButton);
            //OffenderForCPUWidget.AddToolStripItem(new ToolStripSeparator());
#endif
			ToolStripButton historyButton = new ToolStripButton();
			historyButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
			historyButton.Image = (System.Drawing.Image)Properties.Resources.History;
			historyButton.ImageTransparentColor = System.Drawing.Color.Magenta;
			historyButton.Name = "historyButton";
			historyButton.Size = new System.Drawing.Size(23, 22);
			historyButton.Text = "History";
			historyButton.Click += new EventHandler(historyButton_Click);
			OffenderForCPUWidget.AddToolStripItem(historyButton);

            if (this.ConnectionDefn.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ151)
            {
                ToolStripButton getTransButton = new ToolStripButton();
                getTransButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
                getTransButton.Image = (System.Drawing.Image)Properties.Resources.GetTrans;
                getTransButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                getTransButton.Name = "getTransButton";
                getTransButton.Size = new System.Drawing.Size(23, 22);
                getTransButton.Text = Properties.Resources.GetTransactionToolTip;
                getTransButton.Click += new EventHandler(getTransButton_Click);
                OffenderForCPUWidget.AddToolStripItem(getTransButton);
            }
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

		public bool isOffenderAvailable(out string msg)
		{
			bool avail = false;
			msg = "";
			DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
			if (_conn == null)
			{
				_conn = GetConnection(dbConfig.ConnectionDefinition);
			}
			if (_conn != null)
			{
				OdbcConnection odbcCon = _conn.OpenOdbcConnection;
				OdbcCommand command = new OdbcCommand();
				bool wmsOpened = false;
				try
				{
					command.Connection = odbcCon;
					command.CommandTimeout = WORKLOAD_EXEC_TIMEOUT;
					command.CommandText = "WMSOPEN";
					command.ExecuteNonQuery();
					wmsOpened = true;
					string sql = STATUS_TACLPROC_SQ;
					DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
					avail = true;
				}
				catch (OdbcException ex)
				{
					MessageBox.Show(ex.Message, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
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
				}
			}
			return avail;
		}

		private void invokeWorkloadDetail(int row)
		{
            string query_id = _offenderIGrid.Cells[row, "QUERY_ID"].Value as string;
            if (string.IsNullOrEmpty(query_id)) //If query id null or blank, no details to fetch.
                return;

            DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
			if (_conn == null)
			{
				_conn = GetConnection(dbConfig.ConnectionDefinition);
			}
			if (_conn != null)
			{
				OdbcConnection odbcCon = _conn.OpenOdbcConnection;
				OdbcCommand command = new OdbcCommand();
				bool wmsOpened = false;
				try
				{
					Cursor.Current = Cursors.WaitCursor;
					command.Connection = odbcCon;
					command.CommandTimeout = WORKLOAD_EXEC_TIMEOUT;
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

        private void reorgProgressMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    invokeReorgProgress(eventObj.Row);
                }
            }

        }

        private void invokeReorgProgress(int row)
        {
            if (row < 0)
                return;

            try
            {
                
                
                string query_id = _offenderIGrid.Cells[row, "QUERY_ID"].Value as string;
                if (string.IsNullOrEmpty(query_id)) //If query id null or blank, no details to fetch.
                    return;


                string title = string.Format(Properties.Resources.TitleReorgProgress, query_id);

                ReorgProgressUserControl reorgProgressWindow = GetWatchedReorgWindow(ConnectionDefn, query_id);


                if (reorgProgressWindow != null)
                {
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
                    Size size = new Size(827, 520);

                    ReorgProgressUserControl reorgProgress = new ReorgProgressUserControl(this, _theConnectionDefinition, query_id);
                    AddQueryToWatchReorg(reorgProgress);

                    Utilities.LaunchManagedWindow(title, reorgProgress, true, _theConnectionDefinition, size, false);
                }
               
            }
            catch (Exception ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(),
                    "Unable to obtain reorg progress information for the selected row due to the following exception - " + ex.Message,
                    Properties.Resources.TitleLiveView, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        void DisplayParentChildQueriesMenuItem_Click(object sender, EventArgs events)
        {
            TrafodionIGridEventObject eventObject = ((TrafodionIGridToolStripMenuItem)sender).TrafodionIGridEventObject;
            iGRow eventRow = eventObject.TheGrid.Rows[eventObject.Row];

            string queryId = (string)eventRow.Cells[WmsCommand.COL_QUERY_ID].Value;
            string windowTitle = string.Format(Properties.Resources.TitleParentChildQueries, queryId);
            ParentChildQueriesUserControl parentChildQueriesUserControl = new ParentChildQueriesUserControl(_theConnectionDefinition, queryId, this.ParentForm.ClientSize);
            Utilities.LaunchManagedWindow(windowTitle, parentChildQueriesUserControl, _theConnectionDefinition, ChildrenWindowSize, true);
        }

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void prcoessDetailChildMenuItem_Click(object sender, EventArgs events)
		{
			prcoessDetailMenuItem_Click(sender, events, false);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void prcoessDetailParentMenuItem_Click(object sender, EventArgs events)
		{
			prcoessDetailMenuItem_Click(sender, events, true);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void prcoessDetailMenuItem_Click(object sender, EventArgs events, bool parentProcess)
		{
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        string process_id = parentProcess ? _processDetailParent : _processDetailChild;
                        string title = string.Format((parentProcess ? Properties.Resources.TitleParentProcessDetails : Properties.Resources.TitleProcessDetails), process_id);
                        WMSProcessDetailsUserControl process =
                            new WMSProcessDetailsUserControl((parentProcess ? Properties.Resources.ParentProcessDetails : Properties.Resources.ProcessDetails),
                                                             _theConnectionDefinition,
                                                             process_id,
                                                             true);
                        Utilities.LaunchManagedWindow(title, process, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to parent process details information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleSystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void pstateChildMenuItem_Click(object sender, EventArgs events)
		{
			pstateMenuItem_Click(sender, events, false);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void pstateParentMenuItem_Click(object sender, EventArgs events)
		{
			pstateMenuItem_Click(sender, events, true);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void pstateMenuItem_Click(object sender, EventArgs events, bool parentProcess)
		{
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        string process_id = parentProcess ? _pstateParent : _pstateChild;
                        string title = string.Format((parentProcess ? Properties.Resources.TitleParentPstate : Properties.Resources.TitlePstate), process_id);
                        WMSPStateUserControl pstate = new WMSPStateUserControl((parentProcess ? Properties.Resources.ParentPstate : Properties.Resources.Pstate),
                                                                               _theConnectionDefinition,
                                                                               process_id,
                                                                               true);
                        Utilities.LaunchManagedWindow(title, pstate, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(),
                            "Unable to obtain Pstate information for the selected row due to the following exception - " + ex.Message,
                            Properties.Resources.TitleSystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="events"></param>
		void childrenProcessesMenuItem_Click(object sender, EventArgs events)
		{
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    try
                    {
                        TrafodionIGrid iGrid = eventObj.TheGrid;
                        int row = eventObj.Row;
                        string query_id = iGrid.Cells[row, "QUERY_ID"].Value as string;
                        string process_name = _processDetailParent;
                        string title = string.Format(Properties.Resources.TitleChildrenProcesses, process_name);
                        WMSChildrenProcessesUserControl children = new WMSChildrenProcessesUserControl(this, _theConnectionDefinition, query_id, process_name);
                        Utilities.LaunchManagedWindow(title, children, _theConnectionDefinition, ChildrenWindowSize, true);
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), 
                            "Unable to obtain children processes information for the selected row due to the following exception - " + ex.Message, 
                            Properties.Resources.TitleSystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
            }
		}

		void alterOffenderButton_Click(object sender, EventArgs events)
		{
            if (_theConnectionDefinition != null && _theConnectionDefinition.ComponentPrivilegeExists("WMS", WmsCommand.WMS_PRIVILEGE.ADMIN_ALTER.ToString()))
            {
                DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
                WMSOffenderAlterParams alter = new WMSOffenderAlterParams(this, dbConfig.ConnectionDefinition, _offenderOptions);
                DialogResult result = alter.ShowDialog();
                if (result == DialogResult.OK)
                {
                    OffenderForCPUWidget.DataProvider.Stop();

                    _statusCommand = alter.StatusCommand;
                    //Check for CPU/MEM in SQLText and replace with user selected command option.
                    int inx = dbConfig.SQLText.IndexOf("CPU");
                    if (inx != -1)
                    {
                        dbConfig.SQLText = _statusCommand;
                    }
                    inx = dbConfig.SQLText.IndexOf("MEM");
                    if (inx != -1)
                    {
                        dbConfig.SQLText = _statusCommand;
                    }

                    _processType = alter.ProcessType;
                    //Check for PROCESS in SQLText and replace with user selected process option.
                    inx = dbConfig.SQLText.IndexOf("PROCESS");
                    if (inx != -1)
                    {
                        string sqlText = dbConfig.SQLText.Substring(0, inx - 1);
                        dbConfig.SQLText = sqlText + _processType;
                    }
                    else
                    {
                        dbConfig.SQLText += _processType;
                    }

                    addCommandHistory(alter.WMSCommand, "WMS");

                    SetTitle();
                    OffenderForCPUWidget.DataProvider.Start();
                }
            }
            else
            {
                MessageBox.Show("Only DB__ROOT and DB__USERADMINUSER users can view and alter offender configuration", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
		}

		void alterOffender(string sql)
		{
			DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
			ConnectionDefinition connDef = dbConfig.ConnectionDefinition;
			if (_conn == null)
			{
				_conn = GetConnection(dbConfig.ConnectionDefinition);
			}
			if (_conn != null)
			{
				OdbcConnection odbcCon = _conn.OpenOdbcConnection;
				OdbcCommand command = new OdbcCommand();
				bool wmsOpened = false;
				try
				{
					Cursor.Current = Cursors.WaitCursor;
					command.Connection = odbcCon;
					command.CommandTimeout = WORKLOAD_EXEC_TIMEOUT;
					command.CommandText = "WMSOPEN";
					command.ExecuteNonQuery();
					wmsOpened = true;

					command.CommandText = sql;
					command.ExecuteNonQuery();
                    MessageBox.Show("WMS offender parameters altered successfully.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Information);
					addCommandHistory(sql, "WMS");
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

#if INC_COMMANDS
        void statusCpuButton_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = "STATUS CPU";
            //OffenderWorkloadConfig.PassedParameters = args;
            OffenderForCPUWidget.StartDataProvider();
            //addCommandHistory(dbConfig.SQLText, "WMS");
        }

        void statusMemButton_Click(object sender, EventArgs events)
        {
            DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = "STATUS MEM";
            //OffenderWorkloadConfig.PassedParameters = args;
            OffenderForCPUWidget.StartDataProvider();
            //addCommandHistory(dbConfig.SQLText, "WMS");
        }

        void statusCommandButton_Click(object sender, EventArgs events)
        {
            WMSOffenderStatusCommand dlg = new WMSOffenderStatusCommand(_offenderStatusOptions);
            DialogResult result = dlg.ShowDialog();
            if (result == DialogResult.OK)
            {
                string cmd = dlg.WMSOffenderStatusOptions.StatusCommand;
                DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.SQLText = cmd;
                //OffenderWorkloadConfig.PassedParameters = args;
                OffenderForCPUWidget.StartDataProvider();
                //addCommandHistory(dbConfig.SQLText, "WMS");
            }
        }
#endif

		void historyButton_Click(object sender, EventArgs events)
		{
			WMSHistory dlg = new WMSHistory(null, ref _commands);
			DialogResult result = dlg.ShowDialog();
			if (result == DialogResult.OK)
			{
				string cmd = dlg.WMSCommand.Trim();
				if (cmd.ToUpper().StartsWith("STATUS"))
				{
                    OffenderForCPUWidget.DataProvider.Stop();
					DatabaseDataProviderConfig dbConfig = OffenderWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
					dbConfig.SQLText = cmd;
                    SetTitle();
					//OffenderWorkloadConfig.PassedParameters = args;
					OffenderForCPUWidget.StartDataProvider();
					//addCommandHistory(dbConfig.SQLText, "WMS");
				}
				else
				{
					alterOffender(cmd);
				}
			}
		}

        private void getTransButton_Click(object sender, EventArgs events)
        {
            TransactionUserControl parentChildQueriesUserControl = new TransactionUserControl(_theConnectionDefinition);
            Utilities.LaunchManagedWindow(Properties.Resources.TitleTransactionsOfAllNodes, parentChildQueriesUserControl, _theConnectionDefinition, TransactionUserControl.IdealWindowSize, true);
        }

		private void addCommandHistory(string sql, string type)
		{
			_commands.Add(new WMSHistoryModel(_commands.Count, sql, type, DateTime.Now));
		}


        #region ICloneToWindow Members

        public Control Clone()
        {
            return new OffenderWorkloadCanvas(_theConnectionDefinition);
        }

        public string WindowTitle
        {
            get { return Properties.Resources.SystemOffender; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
        }

        #endregion ICloneToWindow Members
    }

	public class MyOffenderDataHandler : TabularDataDisplayHandler
	{
		private OffenderWorkloadCanvas _offenderWorkloadCanvas;

		public MyOffenderDataHandler(OffenderWorkloadCanvas aOffenderWorkloadCanvas)
		{
			_offenderWorkloadCanvas = aOffenderWorkloadCanvas;
		}

		public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
										Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
		{
			//Format Elapsed, Wait and Hold Times
			DataTable newDataTable = new DataTable();
			foreach (DataColumn dc in aDataTable.Columns)
			{
				if (dc.ColumnName.Equals("ELAPSED_TIME") || dc.ColumnName.Equals("WAIT_TIME") || dc.ColumnName.Equals("HOLD_TIME"))
					newDataTable.Columns.Add(dc.ColumnName, System.Type.GetType("System.String"));
				else
					newDataTable.Columns.Add(dc.ColumnName, dc.DataType);
			}

			foreach (DataRow dr in aDataTable.Rows)
			{
				DataRow newDR = newDataTable.NewRow();
				for (int i = 0; i < aDataTable.Columns.Count; i++)
				{
					string colName = aDataTable.Columns[i].ToString();
					if (colName.Equals("ELAPSED_TIME") || colName.Equals("WAIT_TIME") || colName.Equals("HOLD_TIME"))
					{
						newDR[i] = WMSUtils.formatInt2Time(Int32.Parse(dr[i].ToString()));
					}
					else
					{
						newDR[i] = dr[i];
					}
				}
				newDataTable.Rows.Add(newDR);
			}
			base.DoPopulate(aConfig, newDataTable, aDataGrid);

            aDataGrid.UpdateCountControlText("There are {0} offending processes");
        }

	}

    public class ShowOffenderEventArgs : EventArgs
    {
        public enum CommandType
        {
            CPU = 0,
            MEM = 1,
            SWAP = 2
        }

        private CommandType _theCommandType;

        /// <summary>
        /// Property: Offender command type
        /// </summary>
        public CommandType TheCommandType
        {
            get { return _theCommandType; }
            set { _theCommandType = value; }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public ShowOffenderEventArgs(CommandType aCommandType)
        {
            _theCommandType = aCommandType;
        }
    }
}
