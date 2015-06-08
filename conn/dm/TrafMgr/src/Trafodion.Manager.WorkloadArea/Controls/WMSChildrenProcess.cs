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
using System.Data.Odbc;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;
using ZedGraph;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
	public partial class WMSChildrenProcess : Trafodion.Manager.Framework.Controls.TrafodionForm
	{
		#region Constants
		private const int IGRIDINFO_MIN_WIDTH = 40;
		private const int MAX_CPUS = 16;
		private const int MAX_SEGMENTS = 32;
		private const int MAX_CPU_USAGE = 90;
		#endregion

		#region Members
		private WMSWorkloadCanvas m_parent = null;
		ConnectionDefinition m_aConnectionDefinition = null;
		private string m_query_id = null;
		private string m_query_text = null;
		private string m_process_name = null;
		private DataTable m_dataTable = null;
		private DataTable m_dataTable2 = null;
		private int m_numSegments = 0;
		private int m_numCpus = 0;
		//sq-offender
		//private int[][] m_total_esp_each_seg_cpu = new int[MAX_SEGMENTS][];
		//private int[][] m_total_esp_usage = new int[MAX_SEGMENTS][];
		//private int[] m_total_esp_by_seg = new int[MAX_SEGMENTS];
		//private int[] m_total_esp_by_cpu = new int[MAX_CPUS];
		private int[][] m_total_esp_each_seg_cpu = null;
		private int[][] m_total_esp_usage = null;
		private int[] m_total_esp_by_seg = null;
		private int[] m_total_esp_by_cpu = null;
		private int m_grand_total_esp = 0;
		private ZedGraphControl m_zedGraphControl;
		private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
		private string m_processID = "";
		private ContextMenu m_contextMenu = null;
		private MenuItem m_processDetailMenuItem = null;
		private MenuItem m_pstateChildMenuItem = null;
		private UniversalWidgetConfig MonitorWorkloadConfig = null;
		private Connection _conn = null;
		private string m_title = "";
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
		#endregion

		public WMSChildrenProcess(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition,
			string query_id, string query_text, string process_name)
		{
			InitializeComponent();
			m_parent = parent;
			m_aConnectionDefinition = aConnectionDefinition;
            oneGuiBannerControl1.ConnectionDefinition = aConnectionDefinition;
			m_query_id = query_id;
			m_query_text = query_text;
			m_process_name = process_name;
			if (parent is OffenderWorkloadCanvas)
				m_title = "WMS Offender";
			else if (parent is MonitorWorkloadCanvas)
				m_title = "Monitor Workload";

			DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
			dbConfig.TimerPaused = true;
			dbConfig.RefreshRate = 0;
			dbConfig.SQLText = "";
			dbConfig.ConnectionDefinition = aConnectionDefinition;

			helpFilterLinkLabel.MouseHover += new EventHandler(helpFilterLinkLabel_MouseHover);
			viewInfoTabControl.SelectedIndexChanged += new EventHandler(viewInfoTabControl_SelectedIndexChanged);
			summaryIGrid.CellMouseEnter += new iGCellMouseEnterLeaveEventHandler(summaryIGrid_CellMouseEnter);
			detailedIGrid.CellMouseEnter += new iGCellMouseEnterLeaveEventHandler(detailedIGrid_CellMouseEnter);
			detailedIGrid.CellMouseDown += new iGCellMouseDownEventHandler(detailedIGrid_CellMouseDown);
			this.Resize += new EventHandler(WMSChildrenProcess_Resize);

			m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered += InvokeHandleError;


			this.Disposed += new EventHandler(WMSChildrenProcess_Disposed);
			this.timer1.Tick += new EventHandler(timer1_Tick);

			queryIdTextBox.Text = m_query_id;
			queryTextTextBox.Text = m_query_text;
			parentProcessTextBox.Text = m_process_name;
			enableFilterControls(false);

			m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_WmsDetail;
			m_dbDataProvider.Start();
			//Console.WriteLine("Start DataProvider - FetchDataOption.Option_WmsDetail");

			startTimer();
			setupProgressBar();

			if (m_aConnectionDefinition.IsTrafodion)
			{
				enableFilterControls(true);
				this.viewInfoTabControl.Controls.Remove(this.summaryTabPage);
				this.viewInfoTabControl.Controls.Remove(this.graphTabPage);
			}

            CenterToParent();
		}

        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            if (IsHandleCreated)
            {
                Invoke(new HandleEvents(m_dbDataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
            }
        }

        private void InvokeHandleError(Object obj, EventArgs e)
        {
            if (IsHandleCreated)
            {
                Invoke(new HandleEvents(m_dbDataProvider_OnErrorEncountered), new object[] { obj, (DataProviderEventArgs)e });
            }
        }

		void detailedIGrid_CellMouseDown(object sender, iGCellMouseDownEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				detailedIGrid.Rows[e.RowIndex].Cells[e.ColIndex].Selected = true;
				detailedIGrid.SetCurCell(e.RowIndex, e.ColIndex);

				if (e.RowIndex >= 0)
				{
					iGRow row = detailedIGrid.Rows[e.RowIndex];
					iGRowCellCollection coll = row.Cells;
					if (m_aConnectionDefinition.IsTrafodion)
					{
						//m_processID = coll["PROCESS\r\nNAME"].Value.ToString();
						int node = int.Parse(coll["NODE"].Value.ToString());
						int pid = int.Parse(coll["PID"].Value.ToString());
						m_processID = node + "." + pid;
					}
					else
					{
						int segment = int.Parse(coll["SEGMENT"].Value.ToString());
						int cpu = int.Parse(coll["CPU"].Value.ToString());
						int pin = int.Parse(coll["PIN"].Value.ToString());
						m_processID = segment + "." + cpu + "." + pin;
					}

					m_contextMenu = new ContextMenu();
					m_processDetailMenuItem = new MenuItem();
					m_processDetailMenuItem.Text = "Process Detail...";
					m_processDetailMenuItem.Click += new EventHandler(m_processDetailMenuItem_Click);
					m_contextMenu.MenuItems.Add(m_processDetailMenuItem);

					m_pstateChildMenuItem = new MenuItem();
					m_pstateChildMenuItem.Text = "Pstate...";
					m_pstateChildMenuItem.Click += new EventHandler(m_pstateChildMenuItem_Click);
					m_contextMenu.MenuItems.Add(m_pstateChildMenuItem);

					detailedIGrid.ContextMenu = m_contextMenu;
				}
			}
		}

		public Connection GetConnection(ConnectionDefinition connectionDefinition)
		{
			Connection connection = null;
			connection = new Connection(connectionDefinition);
			return connection;
		}

		void m_pstateChildMenuItem_Click(object sender, EventArgs e)
		{
			if (_conn == null)
			{
				_conn = GetConnection(m_aConnectionDefinition);
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
					command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
					command.CommandText = "WMSOPEN";
					command.ExecuteNonQuery();
					wmsOpened = true;
					string process_id = m_processID;
					string sql;
					if (this.m_aConnectionDefinition.IsTrafodion)
						sql = "STATUS PROCESS " + process_id + " PSTATE" ;
					else
						sql = "STATUS PSTATE " + process_id;
					DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    WMSTaclProc tp = new WMSTaclProc(_conn.TheConnectionDefinition, "Pstate", false, dataTable, process_id);
					tp.Show();
				}
				catch (OdbcException ex)
				{
					if (ex.Message.Contains("Must be an administrator"))
					{
						MessageBox.Show("User does not have the privilege, must be an administrator", m_title, MessageBoxButtons.OK, MessageBoxIcon.Information);
					}
					else
					{
						MessageBox.Show("Error: Unable to obtain PSTATE information for the selected row", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
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

		void m_processDetailMenuItem_Click(object sender, EventArgs e)
		{
			if (_conn == null)
			{
				_conn = GetConnection(m_aConnectionDefinition);
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
					command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
					command.CommandText = "WMSOPEN";
					command.ExecuteNonQuery();
					wmsOpened = true;
					string process_id = m_processID;
					string sql;
					if (this.m_aConnectionDefinition.IsTrafodion)
						sql = "STATUS PROCESS " + process_id;
					else
						sql = "STATUS TACLPROC " + process_id;
					DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    WMSTaclProc tp = new WMSTaclProc(_conn.TheConnectionDefinition, "Process Detail", false, dataTable, process_id);
					tp.Show();
				}
				catch (OdbcException ex)
				{
					MessageBox.Show("Error: Unable to obtain process information for the selected row", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

		private void setupProgressBar()
		{
			progressBar1.Value = 0;
			progressBar1.Maximum = 30;
			progressBar1.Minimum = 0;
			progressBar1.Step = 1;
		}

		void timer1_Tick(object sender, EventArgs e)
		{
			if (timer1.Enabled)
			{
				progressBar1.PerformStep();
				if (progressBar1.Value == progressBar1.Maximum)
				{
					progressBar1.Value = 0;
				}
			}
		}

		private void startTimer()
		{
			timer1.Interval = 100;
			if (timer1.Enabled)
			{
				timer1.Stop();
			}
			timer1.Start();
		}

		private void stopTimer()
		{
			if (timer1.Enabled)
			{
				timer1.Stop();
			}
		}

		void WMSChildrenProcess_Disposed(object sender, EventArgs e)
		{
			stopTimer();
            m_dbDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered -= InvokeHandleError;
			m_dbDataProvider.Stop();
		}

		void m_dbDataProvider_OnErrorEncountered(object sender, DataProviderEventArgs e)
		{
			stopTimer();
			MessageBox.Show("Error: " + e.Exception.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
		{
			if (!progressBar1.IsDisposed)
			{
				if (m_dbDataProvider.FetchRepositoryDataOption == WMSOffenderDatabaseDataProvider.FetchDataOption.Option_WmsDetail)
				{
					m_dataTable2 = m_dbDataProvider.DatabaseDataTable;
					//Console.WriteLine("DataProvider Returned - FetchDataOption.Option_WmsDetail (Rows.Count=" + m_dataTable2.Rows.Count + ")");
					m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_ChildrenProcesses;
					m_dbDataProvider.ProcessName = m_process_name;
					m_dbDataProvider.Start();
					//Console.WriteLine("Start DataProvider - FetchDataOption.Option_ChildrenProcesses");
				}
				else if (m_dbDataProvider.FetchRepositoryDataOption == WMSOffenderDatabaseDataProvider.FetchDataOption.Option_ChildrenProcesses)
				{
					stopTimer();
					progressBar1.Visible = false;
					m_dataTable = m_dbDataProvider.DatabaseDataTable;
					//Console.WriteLine("DataProvider Returned - FetchDataOption.Option_ChildrenProcesses (Rows.Count=" + m_dataTable.Rows.Count + ")");

					DataView dv = new DataView(m_dataTable);
					//dv.Sort = "SEGMENT ASC, CPU ASC, PROGRAM_NAME ASC, PIN ASC";
					m_dataTable = dv.ToTable();
					WMSUtils.renameColumnNames(ref m_dataTable);
					if (!m_aConnectionDefinition.IsTrafodion)
					{
						orderSummaryView(m_dataTable);
						CreateGraph_GradientByZBars(m_zedGraphControl);
					}
					orderDetailedView(m_dataTable);
					SetSize();
				}
			}
		}

		private bool getSegmentAndNode(DataTable dataTable, ref int numSegments, ref int numCpus)
		{
			bool result = false;
			DataView dv = new DataView(dataTable);

			if (m_aConnectionDefinition.IsTrafodion)
			{
				dv.Sort = "NODE DESC, CPU DESC";
				dataTable = dv.ToTable();

				if (dataTable.Rows.Count > 0)
				{
					numSegments = (int)dataTable.Rows[0]["NODE"] + 1;
					numCpus = (int)dataTable.Rows[0]["CPU"] + 1;
					result = true;
				}
			}
			else
			{
				dv.Sort = "SEGMENT DESC, NODE DESC";
				dataTable = dv.ToTable();

				if (dataTable.Rows.Count > 0)
				{
					numSegments = (int)dataTable.Rows[0]["SEGMENT"];
					numCpus = (int)dataTable.Rows[0]["NODE"] + 1;
					result = true;
				}
			}

			return result;
		}

		void detailedIGrid_CellMouseEnter(object sender, iGCellMouseEnterLeaveEventArgs e)
		{
			string text = "";
			this.toolTip1.SetToolTip(detailedIGrid, text);
			if (e.RowIndex >= 0 && e.ColIndex >= 0)
			{
				TenTec.Windows.iGridLib.iGCol col = detailedIGrid.Cols[e.ColIndex];
				string colCpuUsage = "CPU" + Environment.NewLine + "USAGE";
				if (col.Key.Equals(colCpuUsage) && (int)detailedIGrid.Cells[e.RowIndex, e.ColIndex].Value > 0)
				{
					text = "The Detailed Info displays list of children processes used by the parent process\r\n" +
						   "   - the CPU USAGE shown in BLUE indicates the CPU usage is under " + MAX_CPU_USAGE + "%\r\n" +
						   "   - the CPU USAGE shown in RED indicates the CPU usage is " + MAX_CPU_USAGE + "% or above";
					this.toolTip1.IsBalloon = true;
					this.toolTip1.AutoPopDelay = 30 * 1000;
					this.toolTip1.SetToolTip(detailedIGrid, text);
				}
			}
		}

		void summaryIGrid_CellMouseEnter(object sender, iGCellMouseEnterLeaveEventArgs e)
		{
			string text = "";
			this.toolTip1.SetToolTip(summaryIGrid, text);
			//don't include row heading && total
			if (e.RowIndex >= 0 && e.RowIndex < summaryIGrid.Rows.Count - 1 &&
				e.ColIndex > 0 && e.ColIndex < summaryIGrid.Cols.Count - 1)
			{
				TenTec.Windows.iGridLib.iGCol col = summaryIGrid.Cols[e.ColIndex];
				if ((int)summaryIGrid.Cells[e.RowIndex, e.ColIndex].Value > 0)
				{
					text = "The Summary Info displays a snapshot of ESPs used by the parent process\r\n" +
						   "   - The number of ESPs shown in BLUE indicates all ESP for the CPU is under " + MAX_CPU_USAGE + "%\r\n" +
						   "   - The number of ESPs shown in RED indicates one or more ESP for the CPU is " + MAX_CPU_USAGE + "% or above";
					this.toolTip1.IsBalloon = true;
					this.toolTip1.AutoPopDelay = 30 * 1000;
					this.toolTip1.SetToolTip(summaryIGrid, text);
				}
			}
		}

		private void enableFilterControls(bool enable)
		{
			filterLabel.Enabled = enable;
			filterNumericUpDown.Enabled = enable;
			refreshButton.Enabled = enable;
			helpFilterLinkLabel.Enabled = enable;
		}

		void viewInfoTabControl_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (this.viewInfoTabControl.SelectedTab.Name.Equals(summaryTabPage.Name) || this.viewInfoTabControl.SelectedTab.Name.Equals(graphTabPage.Name))
			{
				enableFilterControls(false);
			}
			else if (this.viewInfoTabControl.SelectedTab.Name.Equals(detailedTabPage.Name))
			{
				enableFilterControls(true);
			}
		}

		void helpFilterLinkLabel_MouseHover(object sender, EventArgs e)
		{
			string caption =
				"Use Filter CPU Value to list all children processes in the Detailed Info tab page with a specified CPU usage or higher\r\n" +
				"Examples below:-\r\n" +
				"   - The value 0 (zero) will list all processes\r\n" +
				"   - The value 90 will list all processes with the CPU usage of 90% or higher";
			this.toolTip1.IsBalloon = true;
			this.toolTip1.AutoPopDelay = 30 * 1000;
			this.toolTip1.SetToolTip(helpFilterLinkLabel, caption);
		}

		private void orderDetailedView(DataTable dataTable)
		{
			detailedIGrid.FillWithData(dataTable);
			int filterValue = (int)this.filterNumericUpDown.Value;
			for (int r = 0; r < detailedIGrid.Rows.Count; r++)
			{
				string colCpuUsage = "CPU" + Environment.NewLine + "USAGE";
				int value = int.Parse(detailedIGrid.Rows[r].Cells[colCpuUsage].Value.ToString());
				if (value < filterValue)
					detailedIGrid.Rows[r].Visible = false;
				detailedIGrid.Rows[r].AutoHeight();
			}
			setColorGridDetailed(detailedIGrid);
			for (int i = 0; i < detailedIGrid.Cols.Count; i++)
			{
				detailedIGrid.Cols[i].AutoWidth();
				detailedIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
			}
		}

		private void orderSummaryView(DataTable dataTable)
		{
			//for (int s = 0; s < MAX_SEGMENTS; s++)
			//{
			//    m_total_esp_each_seg_cpu[s] = new int[MAX_CPUS];
			//    m_total_esp_usage[s] = new int[MAX_CPUS];
			//}
			int numSegments = 0;
			int numCpus = 0;
			//if (getSegmentAndNode(m_dataTable2, ref numSegments, ref numCpus))
            if (getSegmentAndNode(m_dataTable2, ref numSegments, ref numCpus))
			{
				m_numSegments = numSegments;
				m_numCpus = numCpus;
			}

			Type typeInt32 = System.Type.GetType("System.Int32");
			Type typeString = System.Type.GetType("System.String");

			DataTable dtNew = new DataTable();
			string text = m_aConnectionDefinition.IsTrafodion ? "NODE" : "SEGEMENT";
			//string[] columns = { text,
			//                     "CPU 0", "CPU 1", "CPU 2", "CPU 3", "CPU 4", "CPU 5", "CPU 6", "CPU 7", 
			//                     "CPU 8", "CPU 9", "CPU 10", "CPU 11", "CPU 12", "CPU 13", "CPU 14", "CPU 15", 
			//                     "TOTAL" 
			//                   };
			//Type[] types = { typeInt32,
			//                 typeInt32, typeInt32, typeInt32, typeInt32, typeInt32, typeInt32, typeInt32, typeInt32,
			//                 typeInt32, typeInt32, typeInt32, typeInt32, typeInt32, typeInt32, typeInt32, typeInt32,
			//                 typeInt32
			//               };
			string[] columns = new string[m_numCpus + 2]; // for NODE/SEGMENT and TOTAL
			Type[] types = new Type[m_numCpus + 2]; // for NODE/SEGMENT and TOTAL
			for (int c = 0; c < columns.Length; c++)
			{
				if (c == 0)
					columns[c] = text;
				else if (c == columns.Length - 1)
					columns[c] = "TOTAL";
				else
					columns[c] = "CPU " + (c - 1);

				types[c] = typeInt32;
			}
			for (int c = 0; c < columns.Length; c++)
			{
				DataColumn dcNew = new DataColumn(columns[c], types[c]);
				dtNew.Columns.Add(dcNew);
			}

			m_total_esp_each_seg_cpu = new int[m_numSegments][];
			m_total_esp_usage = new int[m_numSegments][];
			for (int s = 0; s < m_numSegments; s++)
			{
				m_total_esp_each_seg_cpu[s] = new int[m_numCpus];
				m_total_esp_usage[s] = new int[m_numCpus];
			}

			for (int r = 0; r < dataTable.Rows.Count; r++)
			{
				int segment = 0;
				int cpu = 0;
				if (m_aConnectionDefinition.IsTrafodion)
				{
					segment = (int)dataTable.Rows[r]["NODE"];
					cpu = (int)dataTable.Rows[r]["CPU"];
				}
				else
				{
					segment = (int)dataTable.Rows[r]["SEGMENT"];
					cpu = (int)dataTable.Rows[r]["CPU"];
				}
				string colCpuUsage = "CPU" + Environment.NewLine + "USAGE";
				int cpu_usage = (int)dataTable.Rows[r][colCpuUsage];
				string colProcessType = "PROCESS" + Environment.NewLine + "TYPE";
				string process_type = (string)dataTable.Rows[r][colProcessType];
				if (m_aConnectionDefinition.IsTrafodion)
				{
					if (process_type.Contains("ESP"))
					{
						m_total_esp_each_seg_cpu[segment][cpu]++;
						if (m_total_esp_usage[segment][cpu] <= cpu_usage)
							m_total_esp_usage[segment][cpu] = cpu_usage;
						m_grand_total_esp++;
					}
				}
				else
				{
					if (process_type.Contains("ESP"))
					{
						m_total_esp_each_seg_cpu[segment - 1][cpu]++;
						if (m_total_esp_usage[segment - 1][cpu] <= cpu_usage)
							m_total_esp_usage[segment - 1][cpu] = cpu_usage;
						m_grand_total_esp++;
					}
				}
				//The m_numSegments should always be >= segment and m_numCpus should also always be >= cpu + 1
				if (m_numSegments < segment)
					m_numSegments = segment;
				if (m_numCpus < cpu + 1)
					m_numCpus = cpu + 1;
			}

			m_total_esp_by_seg = new int[m_numSegments];
			m_total_esp_by_cpu = new int[m_numCpus];

			//total ESP per segment
			for (int s = 0; s < m_numSegments; s++)
			{
				for (int c = 0; c < numCpus; c++)
				{
					m_total_esp_by_seg[s] += m_total_esp_each_seg_cpu[s][c];
				}
			}

			//total ESP per CPU
			for (int c = 0; c < numCpus; c++)
			{
				for (int s = 0; s < m_numSegments; s++)
				{
					m_total_esp_by_cpu[c] += m_total_esp_each_seg_cpu[s][c];
				}
			}

			for (int s = 0; s < m_numSegments; s++)
			{
				DataRow drNew = dtNew.NewRow();
				for (int c = 0; c < numCpus; c++)
				{
					drNew[0] = m_aConnectionDefinition.IsTrafodion ? s : (s + 1);
					drNew[c + 1] = m_total_esp_each_seg_cpu[s][c];
				}
				drNew[columns.Length - 1] = m_total_esp_by_seg[s];
				dtNew.Rows.Add(drNew);
			}

			DataRow drNew2 = dtNew.NewRow();
			for (int c = 0; c < numCpus; c++)
			{
				drNew2[c + 1] = m_total_esp_by_cpu[c];
			}
			drNew2[columns.Length - 1] = m_grand_total_esp;
			dtNew.Rows.Add(drNew2);

			summaryIGrid.FillWithData(dtNew);
			setColorGridSummary(summaryIGrid, m_total_esp_usage);
			for (int i = 0; i < summaryIGrid.Cols.Count; i++)
			{
				summaryIGrid.Cols[i].AutoWidth();
				summaryIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
			}
			for (int i = 0; i < summaryIGrid.Rows.Count; i++)
			{
				summaryIGrid.Rows[i].AutoHeight();
			}
		}

		private void setColorGridSummary(iGrid igrid, int[][] total_esp_usage)
		{
			for (int r = 0; r < igrid.Rows.Count - 1; r++)
			{
				for (int c = 1; c < igrid.Cols.Count - 1; c++)
				{
					if (r + 1 > m_numSegments)
					{
						igrid.Rows[r].Cells[c].BackColor = Color.LightGray;
					}
					if (c > m_numCpus)
					{
						igrid.Rows[r].Cells[c].BackColor = Color.LightGray;
					}
					int numEsp = Int16.Parse(igrid.Rows[r].Cells[c].Text);
					if (numEsp > 0)
					{
						if (total_esp_usage[r][c - 1] >= MAX_CPU_USAGE)
							igrid.Rows[r].Cells[c].ForeColor = Color.Red;
						else
							igrid.Rows[r].Cells[c].ForeColor = Color.Blue;
					}
				}
			}
		}

		private void setColorGridDetailed(iGrid igrid)
		{
			for (int r = 0; r < igrid.Rows.Count; r++)
			{
				string colCpuUsage = "CPU" + Environment.NewLine + "USAGE";
				int cpu_usage = (int)igrid.Rows[r].Cells[colCpuUsage].Value;
				if (cpu_usage > 0)
				{
					if (cpu_usage >= MAX_CPU_USAGE)
						igrid.Rows[r].Cells[colCpuUsage].ForeColor = Color.Red;
					else
						igrid.Rows[r].Cells[colCpuUsage].ForeColor = Color.Blue;
				}
			}
		}

		private void refreshButton_Click(object sender, EventArgs e)
		{
			orderDetailedView(m_dataTable);
		}

		void WMSChildrenProcess_Resize(object sender, EventArgs e)
		{
			SetSize();
		}

		private void SetSize()
		{
			m_zedGraphControl.Location = new Point(10, 10);
			// Leave a small margin around the outside of the control
			m_zedGraphControl.Size = new Size(viewInfoTabControl.Width - 20, viewInfoTabControl.Height - 40);
		}

		private void CreateGraph_GradientByZBars(ZedGraphControl z1)
		{
			GraphPane myPane = z1.GraphPane;
			myPane.Title.Text = "No. of ESPs for each CPU used - " + m_process_name +
								" (Total " + m_grand_total_esp + " ESPs)";
			if (m_aConnectionDefinition.IsTrafodion)
				myPane.XAxis.Title.Text = "Node,CPU";
			else
				myPane.XAxis.Title.Text = "Segment,CPU";
			myPane.YAxis.Title.Text = "No. of ESPs";
			string[] textLabels = new string[m_numSegments * m_numCpus];
			for (int i = 0; i < m_numSegments * m_numCpus; i++)
			{
				int seg = m_aConnectionDefinition.IsTrafodion ? (i / m_numCpus) : (i / m_numCpus + 1);
				int cpu = i % m_numCpus;
				textLabels[i] = "" + seg + "," + cpu;
			}
			myPane.XAxis.Scale.TextLabels = textLabels;
			myPane.XAxis.Type = AxisType.Text;

			PointPairList list = new PointPairList();
			//double colorStep = 4.0 / m_numCpus;

			for (int i = 0; i < m_numSegments * m_numCpus; i++)
			{
				double x = (double)i + 1;
				int seg = i / m_numCpus;
				int cpu = i % m_numCpus;
				double y = (double)m_total_esp_each_seg_cpu[seg][cpu];
				//double z = (i / m_numCpus) * colorStep;
				double z = m_total_esp_usage[seg][cpu] >= MAX_CPU_USAGE ? 0.0 : 1.0;
				list.Add(x, y, z);
			}

			BarItem myCurve = myPane.AddBar("", list, Color.Blue);
			//Color[] colors = { Color.Red, Color.Yellow, Color.Green, Color.Blue, Color.Purple };
			Color[] colors = { Color.Red, Color.Blue };
			myCurve.Bar.Fill = new Fill(colors);
			myCurve.Bar.Fill.Type = FillType.GradientByZ;

			myCurve.Bar.Fill.RangeMin = 0.0;
			//myCurve.Bar.Fill.RangeMax = 4;
			myCurve.Bar.Fill.RangeMax = 1.0;

			myPane.Chart.Fill = new Fill(Color.White, Color.FromArgb(220, 220, 255), 45);
			myPane.Fill = new Fill(Color.White, Color.FromArgb(255, 255, 225), 45);

			// Tell ZedGraph to calculate the axis ranges
			z1.AxisChange();
		}

        private void closeButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ChildrenProcesses);
        }
	}
}
