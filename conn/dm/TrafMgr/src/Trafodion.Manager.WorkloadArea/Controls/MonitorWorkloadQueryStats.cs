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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class MonitorWorkloadQueryStats : UserControl
    {
        #region Members
        private MonitorWorkloadCanvas m_parent = null;
        private ConnectionDefinition m_cd = null;
        private double m_totalExec = 0.0;
        private double m_totalWait = 0.0;
        private double m_totalHold = 0.0;
        private double m_totalSuspend = 0.0;
        private double m_totalCancel = 0.0;
        private double m_totalReject = 0.0;
        private double m_totalComplete = 0.0;
        private string m_startTime = "";
        #endregion

        public MonitorWorkloadQueryStats(MonitorWorkloadCanvas parent, ConnectionDefinition cd)
        {
            InitializeComponent();
            m_parent = parent;
            m_cd = cd;
            _theToolTip.SetToolTip(totalExecCheckBox, "Cumulative number of queries that were in the executing state since the start of statistics collection");
            _theToolTip.SetToolTip(totalCancelCheckBox, "Cumulative number of queries that were cancelled since the start of statistics collection");
            _theToolTip.SetToolTip(totalCompleteCheckBox, "Cumulative number of queries that completed since the start of statistics collection");
            _theToolTip.SetToolTip(totalHoldCheckBox, "Cumulative number of queries that were in the holding state since the start of statistics collection");
            _theToolTip.SetToolTip(totalRejectCheckBox, "Cumulative number of queries that were rejected since the start of statistics collection");
            _theToolTip.SetToolTip(totalSuspendCheckBox, "Cumulative number of queries that were in the suspended state since the start of statistics collection");
            _theToolTip.SetToolTip(totalWaitCheckBox, "Cumulative number of queries that were in the waiting state since the start of statistics collection");

            populateQueryStats();
        }

        private void populateQueryStats()
        {
            bool wmsOpened = false;
            Connection conn = null;
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

                    string sql = "STATUS WMS STATS";
                    DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    foreach (DataRow dr in dataTable.Rows)
                    {
                        m_totalExec = Double.Parse(dr["TOTAL_EXEC"].ToString());
                        m_totalWait = Double.Parse(dr["TOTAL_WAIT"].ToString());
                        m_totalHold = Double.Parse(dr["TOTAL_HOLD"].ToString());
                        m_totalSuspend = Double.Parse(dr["TOTAL_SUSPEND"].ToString());
                        m_totalCancel = Double.Parse(dr["TOTAL_CANCEL"].ToString());
                        m_totalReject = Double.Parse(dr["TOTAL_REJECT"].ToString());
                        m_totalComplete = Double.Parse(dr["TOTAL_COMPLETE"].ToString());
                        m_startTime = dr["START_TIME"].ToString();
                        CreateChart();
                        break;
                    }
                    if (dataTable.Columns.Contains("NODE_NAME"))
                    {
                        dataTable.Columns.Remove("NODE_NAME");
                    }
                    _statsGrid.FillWithData(dataTable);
                    _statsGrid.ResizeGridColumns(dataTable, 7, 20);
                }
                else
                {
                    MessageBox.Show("Error unable to obtain OdbcConnection", Properties.Resources.LiveWorkloads , MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (OdbcException ex)
            {
                if (ex.Message.Contains("Must be an administrator"))
                {
                    MessageBox.Show("User does not have the privilege, must be an administrator", Properties.Resources.LiveWorkloads, MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    MessageBox.Show("Error: Unable to obtain WMS statistics", Properties.Resources.LiveWorkloads, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void cancelButton_Click(object sender, EventArgs e)
        {
            if (Parent != null && Parent is Form)
            {
                ((Form)Parent).Close();
            }
        }

        public void Refresh()
        {
            _statsGrid.Rows.Clear();
            populateQueryStats();
        }

        private void refreshButton_Click(object sender, EventArgs e)
        {
            Refresh();
        }

        private void resetStatsButton_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show("Are you sure you want to reset WMS statistics?", "Monitor Query Stats", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
            {
                bool wmsOpened = false;
                Connection conn = null;
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

                        command.CommandText = "ALTER WMS RESET STATS";
                        command.ExecuteNonQuery();
                        MessageBox.Show("The WMS stats is reset successfully.", "Monitor Query Stats", MessageBoxButtons.OK, MessageBoxIcon.Information);
                        populateQueryStats();
                    }
                    else
                    {
                        MessageBox.Show("Error unable to obtain OdbcConnection", "Monitor Query Stats", MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        // Call this method from the Form_Load method, passing your ZedGraphControl
        public void CreateChart()
        {
            DataTable data = new DataTable();
            data.Columns.Add("x_axis", typeof(string));
            data.Columns.Add("y_axis", typeof(double));

            // Add some pie slices
            if (totalExecCheckBox.Checked)
            {
                string execute = "Executed";
                addRowToTable(data, execute, m_totalExec);
            }
            if (totalCompleteCheckBox.Checked)
            {
                string complete = "Completed";
                addRowToTable(data, complete, m_totalComplete);

            }
            if (totalCancelCheckBox.Checked)
            {
                string cancel = "Canceled";
                addRowToTable(data, cancel, m_totalCancel);

            }
            if (totalRejectCheckBox.Checked)
            {
                string reject = "Rejected";
                addRowToTable(data, reject, m_totalReject);

            }
            if (totalWaitCheckBox.Checked)
            {
                string wait = "Waiting";
                addRowToTable(data, wait, m_totalWait);

            }
            if (totalHoldCheckBox.Checked)
            {
                string hold = "Holding";
                addRowToTable(data, hold, m_totalHold);

            }
            if (totalSuspendCheckBox.Checked)
            {
                string suspend = "Suspended";
                addRowToTable(data, suspend, m_totalSuspend);
            }

            _theChart.PopulateChart(data, "Cumulative WMS Statistics", "Statistics since " + m_startTime.Substring(0, 19));
        }

        private void addRowToTable(DataTable data, String title, double value)
        {
            data.Rows.Add(new object[] { title, value });
        }

        private void totalExecCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void totalWaitCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void totalHoldCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void totalSuspendCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void totalRejectCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void totalCancelCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void totalCompleteCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CreateChart();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.WMSStatistics);
        }
    }

}
