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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSWarnInfo : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Constants
        private const int IGRIDINFO_MIN_WIDTH = 50;
        #endregion

        #region Members
        private WMSWorkloadCanvas m_parent = null;
        private ConnectionDefinition m_cd = null;
        private string m_qid = null;
        private string m_title = "";
        #endregion

        public WMSWarnInfo(WMSWorkloadCanvas parent, ConnectionDefinition cd, string qid)
        {
            InitializeComponent();
            m_parent = parent;
            m_cd = cd;
            oneGuiBannerControl1.ConnectionDefinition = cd;

            m_qid = qid;
            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Monitor Workload";

            CenterToParent();

            populateWarnInfo();
        }

        private void populateWarnInfo()
        {
            bool wmsOpened = false;
            Connection conn = null;
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

                    string sql = "STATUS QUERY " + m_qid + " WARN";
                    DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    WMSUtils.renameColumnNames(ref dataTable);
                    warnInfoIGrid.FillWithData(dataTable);
                    for (int i = 0; i < warnInfoIGrid.Cols.Count; i++)
                    {
                        warnInfoIGrid.Cols[i].AutoWidth();
                        warnInfoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                    }
                    warnInfoIGrid.Cols[0].Visible = false;
                    queryIdTextBox.Text = m_qid;

                    for (int i = 0; i < warnInfoIGrid.Rows.Count; i++)
                    {
                        warnInfoIGrid.Rows[i].AutoHeight();
                    }
                }
                else
                {
                    MessageBox.Show("Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Error unable to obtain warning info: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.WarnInfo);
        }

        private void closeButton_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}
