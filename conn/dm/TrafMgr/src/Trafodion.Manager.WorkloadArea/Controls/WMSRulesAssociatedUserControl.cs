//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2015 Hewlett-Packard Development Company, L.P.
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
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// User control for Rules associated widget
    /// Note: The entire user control was taken strickly from the original RulesAssociated form.  
    ///       If time permits, it is recommended to convert this widget to use Universal Widget, which runs the IOs
    ///       in the background.  Currently, the IOs are in the GUI thread and could potentially 
    ///       block the entire application. 
    /// </summary>
    public partial class WMSRulesAssociatedUserControl : UserControl
    {
        #region Members
        private WMSWorkloadCanvas m_parent = null;
        private ConnectionDefinition m_cd = null;
        private string m_serviceName = null;
        private string m_selectedRuleName = null;
        private string m_selectedServiceName = null;
        private string m_title = "";
        #endregion

        public WMSRulesAssociatedUserControl(WMSWorkloadCanvas parent, ConnectionDefinition cd, string serviceName)
        {
            InitializeComponent();
            m_parent = parent;
            m_cd = cd;
            m_serviceName = serviceName;
            this.Text = "Rules Associated With Service " + serviceName;
            if (parent is OffenderWorkloadCanvas)
                m_title = Properties.Resources.SystemOffender;
            else if (parent is MonitorWorkloadCanvas)
                m_title = Properties.Resources.LiveWorkloads;
            this.ruleAssignedIGrid.CellClick += new iGCellClickEventHandler(iGridRuleAssigned_SelectionChanged);
            showRulesToSelectedService();
        }

        void iGridRuleAssigned_SelectionChanged(object sender, EventArgs e)
        {
            this.ruleInfoButton.Enabled = (this.ruleAssignedIGrid.SelectedRowIndexes.Count > 0) ? true : false;
            this.serviceInfoButton.Enabled = (this.ruleAssignedIGrid.SelectedRowIndexes.Count > 0) ? true : false;

            iGRow row = ruleAssignedIGrid.Rows[ruleAssignedIGrid.SelectedRowIndexes[0]];
            iGRowCellCollection coll = row.Cells;
            m_selectedRuleName = (string)coll["RULE_NAME"].Value;
            m_selectedServiceName = (string)coll["SERVICE_NAME"].Value;
        }

        void iGridRuleAssigned_SelectionChanged(object sender, iGCellClickEventArgs e)
        {
            this.ruleInfoButton.Enabled = (e.RowIndex >= 0) ? true : false;
            this.serviceInfoButton.Enabled = (e.RowIndex >= 0) ? true : false;

            if (e.RowIndex >= 0)
            {
                iGRow row = ruleAssignedIGrid.Rows[e.RowIndex];
                iGRowCellCollection coll = row.Cells;
                string rule_name = "RULE" + Environment.NewLine + "NAME";
                m_selectedRuleName = (string)coll[rule_name].Value;
                string service_name = "SERVICE" + Environment.NewLine + "NAME";
                m_selectedServiceName = (string)coll[service_name].Value;
            }
        }

        private void showRulesToSelectedService()
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

                    string sql = "STATUS WMS CONN";
                    DataTable dtConn = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    string sql2 = "STATUS SERVICE " + m_serviceName + " COMP";
                    DataTable dtComp = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql2);
                    string sql3 = "STATUS SERVICE " + m_serviceName + " EXEC";
                    DataTable dtExec = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql3);
                    DataTable dtNew = WMSUtils.getAssoicatedDataTable(m_serviceName, dtConn, dtComp, dtExec);
                    WMSUtils.renameColumnNames(ref dtNew);
                    ruleAssignedIGrid.FillWithData(dtNew);
                    for (int i = 0; i < ruleAssignedIGrid.Cols.Count; i++)
                    {
                        ruleAssignedIGrid.Cols[i].AutoWidth();
                        ruleAssignedIGrid.Cols[i].MinWidth = (i == 0) ? 100 : 250;
                    }
                    for (int i = 0; i < ruleAssignedIGrid.Rows.Count; i++)
                    {
                        ruleAssignedIGrid.Rows[i].AutoHeight();
                    }
                }
                else
                {
                    MessageBox.Show("Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("OdbcException " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void showRule()
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

                    string sql = "STATUS RULE " + m_selectedRuleName;
                    DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    WMSUtils.renameColumnNames(ref dataTable);
                    infoIGrid.FillWithData(dataTable);
                    for (int i = 0; i < infoIGrid.Cols.Count; i++)
                    {
                        infoIGrid.Cols[i].AutoWidth();
                        infoIGrid.Cols[i].MinWidth = 50;
                    }
                    for (int i = 0; i < infoIGrid.Rows.Count; i++)
                    {
                        infoIGrid.Rows[i].AutoHeight();
                    }
                }
                else
                {
                    MessageBox.Show("Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("OdbcException " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void showService()
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

                    string sql = "STATUS SERVICE " + m_selectedServiceName;
                    DataTable dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    WMSUtils.renameColumnNames(ref dataTable);
                    infoIGrid.FillWithData(dataTable);
                    for (int i = 0; i < infoIGrid.Cols.Count; i++)
                    {
                        infoIGrid.Cols[i].AutoWidth();
                        infoIGrid.Cols[i].MinWidth = 50;
                    }
                    for (int i = 0; i < infoIGrid.Rows.Count; i++)
                    {
                        infoIGrid.Rows[i].AutoHeight();
                    }
                }
                else
                {
                    MessageBox.Show("Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show("OdbcException " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void buttonServiceInfo_Click(object sender, EventArgs e)
        {
            showService();
        }

        private void buttonRuleInfo_Click(object sender, EventArgs e)
        {
            showRule();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.RulesAssociated);
        }
    }
}

