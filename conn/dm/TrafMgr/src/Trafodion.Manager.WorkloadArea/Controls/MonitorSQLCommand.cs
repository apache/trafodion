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
using System.Data.Odbc;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class MonitorSQLCommand : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members
        private WMSWorkloadCanvas m_parent = null;
        private ConnectionDefinition m_aConnectionDefinition = null;
        private string m_action = null;
        private string m_sql = null;
        #endregion

        public MonitorSQLCommand(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string action, string sql, bool enable)
        {
            InitializeComponent();
            m_parent = parent;
            m_aConnectionDefinition = aConnectionDefinition;
            m_action = action;
            m_sql = sql;
            this.Text = action;
            //this.okButton.Text = action;
            this.commandTextBox.Text = sql;
            this.sqlCommandTextBox.Enabled = this.sqlCommandTextBox.Visible = groupBox1.Visible = enable;
        }

        private void okButton_Click(object sender, EventArgs e)
        {
            Connection conn = new Connection(m_aConnectionDefinition);
            if (conn != null)
            {
                OdbcConnection odbcCon = conn.OpenOdbcConnection;
                OdbcCommand command = new OdbcCommand();
                bool wmsOpened = false;
                try
                {
                    command.Connection = odbcCon;
                    command.CommandTimeout = WMSWorkloadCanvas.WORKLOAD_EXEC_TIMEOUT;
                    command.CommandText = "WMSOPEN";
                    command.ExecuteNonQuery();
                    wmsOpened = true;
                    StringBuilder sb = new StringBuilder();
                    sb.Append(m_sql);
                    string sqlCmd = sqlCommandTextBox.Text.Trim();
                    if (sqlCmd != null && sqlCmd.Length > 0)
                    {
                        sb.Append(" SQL_CMD ");
                        sb.Append("\"");
                        sb.Append(sqlCmd);
                        if (!sqlCmd.EndsWith(";"))
                            sb.Append(";");
                        sb.Append("\"");
                    }
                    string cmd = sb.ToString();
                    command.CommandText = cmd;
                    command.ExecuteNonQuery();
                    MessageBox.Show(m_action + " executed successfully.", Properties.Resources.LiveWorkloads, MessageBoxButtons.OK, MessageBoxIcon.Information);
                    DialogResult = DialogResult.OK;
                    this.Close();
                }
                catch (OdbcException ex)
                {
                    string message = "Error: executing " + m_action + Environment.NewLine + ex.Message + Environment.NewLine;
                    MessageBox.Show(message, Properties.Resources.LiveWorkloads, MessageBoxButtons.OK, MessageBoxIcon.Error);
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
    }
}
