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
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
	public partial class WMSOffenderAlterParams : Trafodion.Manager.Framework.Controls.TrafodionForm
	{
		#region Members
		private OffenderWorkloadCanvas m_parent = null;
		private ConnectionDefinition m_cd = null;
		private WMSOffenderOptions m_options = null;
		private string m_sql = null;
		#endregion

		#region Properties
		public WMSOffenderOptions WMSOffenderOptions
		{
			get { return m_options; }
		}

		public string WMSCommand
		{
			get { return m_sql; }
		}

		public string ProcessType
		{
			get { return m_options.SQLProcess ? " PROCESS SQL" : " PROCESS ALL"; }
		}

		public string StatusCommand
		{
			get { return m_options.StatusOffender == WMSOffenderOptions.STATUS_OFFENDER.CPU ? "STATUS CPU" : "STATUS MEM"; }
		}
		#endregion

		public WMSOffenderAlterParams(OffenderWorkloadCanvas parent, ConnectionDefinition cd, WMSOffenderOptions options)
		{
			InitializeComponent();
			m_parent = parent;
			m_cd = cd;
			m_options = options;
			this.Text = "Alter Offender Parameters";
			this.sampleCPUNumericUpDown.Visible = false;
			this.sampleCPULabel.Visible = false;
			this.cpuLinkLabel.Visible = false;
			getWMSInfo();
			this.Load += new EventHandler(handleInfoChanged);
			this.sampleIntervalNumericUpDown.ValueChanged += new EventHandler(handleInfoChanged);
			this.sampleCPUNumericUpDown.ValueChanged += new EventHandler(handleInfoChanged);
			this.sampleCacheNumericUpDown.ValueChanged += new EventHandler(handleInfoChanged);
			this.intervalLinkLabel.MouseHover += new EventHandler(intervalLinkLabel_MouseHover);
			this.cpuLinkLabel.MouseHover += new EventHandler(cpuLinkLabel_MouseHover);
			this.cacheLinkLabel.MouseHover += new EventHandler(cacheLinkLabel_MouseHover);
			this.processLinkLabel.MouseHover += new EventHandler(processLinkLabel_MouseHover);
			if (m_options.SQLProcess)
			{
				sqlRadioButton.Checked = true;
			}
			else
			{
				allRadioButton.Checked = true;
			}

			if (m_options.StatusOffender == WMSOffenderOptions.STATUS_OFFENDER.CPU)
			{
				cpuRadioButton.Checked = true;
			}
			else
			{
				memoryRadioButton.Checked = true;
			}
		}

		void processLinkLabel_MouseHover(object sender, EventArgs e)
		{
			string caption =
				"Select one of these options:\r\n" +
				"- ALL to display all processes\r\n" +
				"- SQL to display SQL processes such as MXUDR, MXESP, MXCI, and MXOSRVR\r\n" +
				"The default value is ALL.";
			this.toolTip1.IsBalloon = true;
			this.toolTip1.AutoPopDelay = 30 * 1000;
			this.toolTip1.SetToolTip(processLinkLabel, caption);
		}

		void cacheLinkLabel_MouseHover(object sender, EventArgs e)
		{
			string segment_node = "node";
			string caption =
				"The number of offending processes that WMS displays per " + segment_node + ".\r\n" +
				"The default value is 10, and the range is 10 to 100.";
			this.toolTip1.IsBalloon = true;
			this.toolTip1.AutoPopDelay = 30 * 1000;
			this.toolTip1.SetToolTip(cacheLinkLabel, caption);
		}

		void cpuLinkLabel_MouseHover(object sender, EventArgs e)
		{
			int def_value = 2;
			int min_value = 2;
			int max_value = 256;

			string caption =
				"The number of CPUs that WMS checks during one sample.\r\n" +
				"The default value is " + def_value + ", and the range is " + min_value + " to " + max_value + ".";
			this.toolTip1.IsBalloon = true;
			this.toolTip1.AutoPopDelay = 30 * 1000;
			this.toolTip1.SetToolTip(cpuLinkLabel, caption);
		}

		void intervalLinkLabel_MouseHover(object sender, EventArgs e)
		{
			string caption =
				"How often, in seconds, WMS gets the offender's sample data.\r\n" +
				"The defaut value is 10, and the range is from 10 to 60 seconds.";
			this.toolTip1.IsBalloon = true;
			this.toolTip1.AutoPopDelay = 30 * 1000;
			this.toolTip1.SetToolTip(intervalLinkLabel, caption);
		}

		void handleInfoChanged(object sender, EventArgs e)
		{
			string sql = getCommandPreview();
			this.commandPreviewTextBox.Text = sql;
		}

		private string getCommandPreview()
		{
			StringBuilder sb = new StringBuilder();
			sb.Append("ALTER WMS OFFENDER").Append(Environment.NewLine);
			sb.Append("   SAMPLE_INTERVAL ").Append(sampleIntervalNumericUpDown.Value.ToString()).Append(Environment.NewLine);
			sb.Append("  ,SAMPLE_CACHE ").Append(sampleCacheNumericUpDown.Value.ToString()).Append(Environment.NewLine);
			return sb.ToString();
		}

		private void getWMSInfo()
		{
			DataTable dataTable = null;
			Connection conn = null;
			bool wmsOpened = false;
			OdbcConnection odbcCon = null;
			OdbcCommand command = null;

			try
			{
				Cursor.Current = Cursors.WaitCursor;
				conn = m_parent.GetConnection(m_cd);
				if (conn != null)
				{
					odbcCon = conn.OpenOdbcConnection;
					command = new OdbcCommand();
					command.Connection = odbcCon;
					command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
					command.CommandText = "WMSOPEN";
					command.ExecuteNonQuery();
					wmsOpened = true;

					string sql = "STATUS WMS";
					dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
					if (!setWMSFields(dataTable))
					{
                        MessageBox.Show("WMS Info not found", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
				}
				else
				{
                    MessageBox.Show("Error unable to obtain OdbcConnection", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
				}
			}
			catch (OdbcException ex)
			{
                MessageBox.Show("Exception: " + ex.Message, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
			}
			finally
			{
				if (wmsOpened)
				{
					command.CommandText = "WMSCLOSE";
					command.ExecuteNonQuery();
				}
				if (conn != null)
				{
					conn.Close();
				}
				Cursor.Current = Cursors.Default;
			}
		}

		private bool setWMSFields(DataTable dataTable)
		{
			if (dataTable != null && dataTable.Rows.Count > 0)
			{
				foreach (DataRow r in dataTable.Rows)
				{
					string processType = r["OFNDR_PROCESS_TYPE"].ToString();
					string sampleInterval = r["OFNDR_SAMPLE_INTERVAL"].ToString();
					string sampleCache = r["OFNDR_SAMPLE_CACHE"].ToString();
					sampleIntervalNumericUpDown.Value = Int32.Parse(sampleInterval);
					sampleCacheNumericUpDown.Value = Int32.Parse(sampleCache);

					return true;
				}
			}
			return false;
		}

		private void alterButton_Click(object sender, EventArgs e)
		{
            DialogResult result = MessageBox.Show("Altering these values will affect all users of System Offender. Are you sure you want to do this?", Properties.Resources.SystemOffender, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
			if (result == DialogResult.Yes)
			{
				Connection conn = null;
				bool wmsOpened = false;
				OdbcConnection odbcCon = null;
				OdbcCommand command = null;

				try
				{
					Cursor.Current = Cursors.WaitCursor;
					conn = m_parent.GetConnection(m_cd);
					if (conn != null)
					{
						odbcCon = conn.OpenOdbcConnection;
						command = new OdbcCommand();
						command.Connection = odbcCon;
						command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
						command.CommandText = "WMSOPEN";
						command.ExecuteNonQuery();
						wmsOpened = true;

						m_options.SQLProcess = this.sqlRadioButton.Checked ? true : false;
						m_options.StatusOffender = this.cpuRadioButton.Checked ? WMSOffenderOptions.STATUS_OFFENDER.CPU : WMSOffenderOptions.STATUS_OFFENDER.MEMORY;
						m_sql = getCommandPreview();
						command.CommandText = m_sql;
						command.ExecuteNonQuery();
						getWMSInfo();
						this.commandPreviewTextBox.Text = m_sql;
						m_options.SampleInterval = (int)this.sampleIntervalNumericUpDown.Value;
						m_options.SampleCPUs = (int)this.sampleCPUNumericUpDown.Value;
						m_options.SampleCache = (int)this.sampleCacheNumericUpDown.Value;
                        MessageBox.Show("The server and client parameters are altered successfully.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Information);
						DialogResult = DialogResult.OK;
						this.Close();
					}
					else
					{
                        MessageBox.Show("Error unable to obtain OdbcConnection", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
				}
				catch (OdbcException ex)
				{
					if (ex.Message.Contains("Must be an administrator"))
					{
                        MessageBox.Show("User does not have the privilege to alter the server parameters, only the client parameter is altered.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Information);
						DialogResult = DialogResult.OK;
						this.Close();
					}
					else
					{
                        MessageBox.Show("Exception: " + ex.Message, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
				}
				finally
				{
					if (wmsOpened)
					{
						command.CommandText = "WMSCLOSE";
						command.ExecuteNonQuery();
					}
					if (conn != null)
					{
						conn.Close();
					}
					Cursor.Current = Cursors.Default;
				}
			}
		}

		private void resetButton_Click(object sender, EventArgs e)
		{
            DialogResult result = MessageBox.Show("Resetting these values will affect all users of System Offender. Are you sure you want to do this?", Properties.Resources.SystemOffender, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
			if (result == DialogResult.Yes)
			{
				Connection conn = null;
				bool wmsOpened = false;
				OdbcConnection odbcCon = null;
				OdbcCommand command = null;

				try
				{
					conn = m_parent.GetConnection(m_cd);
					if (conn != null)
					{

						odbcCon = conn.OpenOdbcConnection;
						command = new OdbcCommand();
						command.Connection = odbcCon;
						command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
						command.CommandText = "WMSOPEN";
						command.ExecuteNonQuery();
						wmsOpened = true;

						m_options.SQLProcess = false;
						m_options.StatusOffender = WMSOffenderOptions.STATUS_OFFENDER.CPU;
						this.allRadioButton.Checked = true;
						this.cpuRadioButton.Checked = true;
						m_sql = "ALTER WMS OFFENDER RESET";
						command.CommandText = m_sql;
						command.ExecuteNonQuery();
						getWMSInfo();
						this.commandPreviewTextBox.Text = m_sql;
						m_options.SampleInterval = (int)this.sampleIntervalNumericUpDown.Value;
						m_options.SampleCPUs = (int)this.sampleCPUNumericUpDown.Value;
						m_options.SampleCache = (int)this.sampleCacheNumericUpDown.Value;
                        MessageBox.Show("The server and client parameters are altered successfully.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Information);
						DialogResult = DialogResult.OK;
						this.Close();
					}
					else
					{
                        MessageBox.Show("Error unable to obtain OdbcConnection", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
				}
				catch (OdbcException ex)
				{
					if (ex.Message.Contains("Must be an administrator"))
					{
                        MessageBox.Show("User does not have the privilege to reset the server parameters, only the client parameter is altered.", Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Information);
						DialogResult = DialogResult.OK;
						this.Close();
					}
					else
					{
                        MessageBox.Show("Exception: " + ex.Message, Properties.Resources.SystemOffender, MessageBoxButtons.OK, MessageBoxIcon.Error);
					}
				}
				finally
				{
					if (wmsOpened)
					{
						command.CommandText = "WMSCLOSE";
						command.ExecuteNonQuery();
					}
					if (conn != null)
					{
						conn.Close();
					}
				}
			}
		}

		private void cancelButton_Click(object sender, EventArgs e)
		{
			this.Close();
		}

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.AlterOffender);
        }

	}
}
