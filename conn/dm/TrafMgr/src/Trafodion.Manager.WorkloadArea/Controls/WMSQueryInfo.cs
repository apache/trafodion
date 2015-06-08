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
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSQueryInfo : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members
        private WMSWorkloadCanvas m_parent = null;
        private DatabaseDataProviderConfig m_dbConfig = null;
        private ConnectionDefinition m_cd = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private DataTable m_dataTable = null;
        private DataTable m_previousDataTable = null;
        private int m_refreshRate = 10;
        private int m_tickCount = 0;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private string m_title = "";
        WidgetCanvas widgetCanvas = null;
        WidgetContainer queryTextContainer = null;
        private DataTable sessionStatsDataTable = null;

        #endregion

        public WMSQueryInfo(WMSWorkloadCanvas parent, DatabaseDataProviderConfig dbConfig, ConnectionDefinition cd, string qid, string start_ts, DataTable dataTable)
        {
            InitializeComponent();
            m_parent = parent;
            m_dbConfig = dbConfig;
            m_cd = cd;
            m_qid = qid;
            m_start_ts = start_ts;
            m_dataTable = dataTable;
            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Monitor Workload";

            detailsPanel.Controls.Remove(_headerPanel);
            detailsPanel.Controls.Remove(compilerStatsPanel);
            detailsPanel.Controls.Remove(runTimeStatsPanel);
            detailsPanel.Controls.Remove(_buttonsPanel);

            widgetCanvas = new WidgetCanvas();
            widgetCanvas.Dock = DockStyle.Fill;
            widgetCanvas.ThePersistenceKey = "WorkloadDetail";

            detailsPanel.Controls.Add(widgetCanvas);

            GridLayoutManager gridLayoutManager = new GridLayoutManager(10, 7);
            //gridLayoutManager.CellSpacing = 4;
            widgetCanvas.LayoutManager = gridLayoutManager;

            GridConstraint gridConstraint = new GridConstraint(0, 0, 2, 6);
            queryTextContainer = new WidgetContainer(widgetCanvas, this.queryTextPanel, "Query Text");
            queryTextContainer.Name = "Query Text";
            queryTextContainer.AllowDelete = false;
            widgetCanvas.AddWidget(queryTextContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(0, 6, 5, 1);
            WidgetContainer buttonsContainer = new WidgetContainer(widgetCanvas, _buttonsPanel,"");
            buttonsContainer.Name = "Buttons";
            buttonsContainer.AllowDelete = false;
            buttonsContainer.AllowMaximize = false;
            widgetCanvas.AddWidget(buttonsContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(2, 0, 3, 6);
            WidgetContainer compStatsContainer = new WidgetContainer(widgetCanvas, this.compilerStatsPanel, "Compiler Statistics");
            compStatsContainer.Name = "Compiler Statistics";
            compStatsContainer.AllowDelete = false;
            widgetCanvas.AddWidget(compStatsContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(5, 0, 5, 7);
            WidgetContainer runtimeStatsContainer = new WidgetContainer(widgetCanvas, this.runTimeStatsPanel, "Runtime Statistics");
            runtimeStatsContainer.Name = "Runtime Statistics";
            runtimeStatsContainer.AllowDelete = false;
            widgetCanvas.AddWidget(runtimeStatsContainer, gridConstraint, -1);

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            
            refreshTimer.Tick += new EventHandler(refreshTimer_Tick);
            FormClosed += new FormClosedEventHandler(WMSQueryInfo_FormClosed);
            warnIndLinkLabel.MouseHover += new EventHandler(warnIndLinkLabel_MouseHover);
            totalMemAllocLinkLabel.MouseHover += new EventHandler(totalMemAllocLinkLabel_MouseHover);
            maxMemUsedLinkLabel.MouseHover += new EventHandler(maxMemUsedLinkLabel_MouseHover);
            totalProcessorLinkLabel.MouseHover += new EventHandler(totalProcessorLinkLabel_MouseHover);
            lastProcessorLinkLabel.MouseHover += new EventHandler(lastProcessorLinkLabel_MouseHover);
            deltaProcessorLinkLabel.MouseHover += new EventHandler(deltaProcessorLinkLabel_MouseHover);
            processorUsageLinkLabel.MouseHover += new EventHandler(processorUsageLinkLabel_MouseHover);
            totalElapsedTimeLinkLabel.MouseHover += new EventHandler(totalElapsedTimeLinkLabel_MouseHover);
            //MaximumSize = new Size(this.Width, int.MaxValue);

            _systemNameTextBox.Text = cd.Name;

            widgetCanvas.InitializeCanvas();

            layoutCheckBox.Checked = widgetCanvas.Locked;

            getWMSInfo(false);
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return m_cd; }
        }
        public string QueryId
        {
            get { return m_qid; }
        }

        void totalElapsedTimeLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Total Query Time =\r\n" +
                "\tElapsed Time +\r\n" +
                "\tWait Time +\r\n" +
                "\tHold Time\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(totalElapsedTimeLinkLabel, caption);
        }

        void processorUsageLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Processor Usage Per Second = \r\n" +
                "\tTotal CPU Time / Elapsed Time\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(processorUsageLinkLabel, caption);
        }

        void deltaProcessorLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Delta Processor Time = \r\n" +
                "\tTotal Processor Time - Last Interval Processor Time\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(deltaProcessorLinkLabel, caption);
        }

        void lastProcessorLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Last Interval of the Total Processor Time (hh:mm:ss.sss) format\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(lastProcessorLinkLabel, caption);
        }

        void totalProcessorLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Total Processor Time (hh:mm:ss.sss) format\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(totalProcessorLinkLabel, caption);
        }

        void maxMemUsedLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Max Mem Used =\r\n" +
                "\tSQL Space Alloc +\r\n" +
                "\tSQL Heap Alloc +\r\n" +
                "\tEID Space Alloc +\r\n" +
                "\tEID Heap Alloc\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(maxMemUsedLinkLabel, caption);
        }

        void totalMemAllocLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "Total Mem Alloc =\r\n" +
                "\tSQL Space Used +\r\n" +
                "\tSQL Heap Used\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(totalMemAllocLinkLabel, caption);
        }

        void warnIndLinkLabel_MouseHover(object sender, EventArgs e)
        {
            string caption =
                "The color of the circle maps to a warn level:\r\n" +
                "\tGreen\t= NOWARN\r\n" +
                "\tYellow\t= LOW\r\n" +
                "\tOrange\t= MEDIUM\r\n" +
                "\tRed\t= HIGH\r\n";
            this.toolTip1.IsBalloon = true;
            this.toolTip1.AutoPopDelay = 30 * 1000;
            this.toolTip1.SetToolTip(warnIndLinkLabel, caption);
        }

        private bool serviceEnable(string serviceName, string type)
        {
            bool enable = false;
            DataTable dataTable = null;
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

                    dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, "STATUS SERVICE " + serviceName);
                    if (dataTable != null)
                    {
                        foreach (DataRow r in dataTable.Rows)
                        {
                            if (serviceName.Equals(r["SERVICE_NAME"]))
                            {
                                if (type.Equals("PLAN"))
                                {
                                    if (r["SQL_PLAN"].Equals("PLAN"))
                                        enable = true;
                                }
                                else if (type.Equals("TEXT"))
                                {
                                    if (r["SQL_TEXT"].Equals("TEXT"))
                                        enable = true;
                                }
                            }
                            break;
                        }
                    }
                }
                else
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            catch (OdbcException ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "Exception: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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
            
            return enable;
        }

        private DataRow getDataRow()
        {
            if (m_dataTable.Rows.Count > 0)
            {
                foreach (DataRow r in m_dataTable.Rows)
                {
                    string qid = r["QUERY_ID"].ToString();
                    string start_ts = r["START_TS"].ToString();
                    if (qid.Equals(m_qid) && start_ts.Equals(m_start_ts))
                    {
                        return r;
                    }
                }
            }

            return null;
        }

        public void LoadData(DataTable dataTable)
        {
            m_dataTable = dataTable;
            getWMSInfo(false, true);
        }

        private void getWMSInfo(bool loadTable)
        {
            getWMSInfo(loadTable, false);
        }

        private void getWMSInfo(bool loadTable, bool saveDataTable)
        {
            Connection conn = null;
            bool wmsOpened = false;
            OdbcConnection odbcCon = null;
            OdbcCommand command = null;

            try
            {
                this.queryIdTextBox.Text = m_qid;

                if (loadTable)
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
                        
                        string sql = "STATUS QUERY " + m_qid + " MERGED";
                        m_dataTable = WMSOdbcAccess.getDataTableFromSQL(odbcCon, command, sql);
                    }
                    else
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }

                DataRow dr = getDataRow();
                if (setStatusTextFields(dr) && setStatsTextFields(dr))
                {
                    ;
                }
                else
                {
                    stopTimer();

                    //MessageBox.Show(Utilities.GetForegroundControl(), "Error: Query statistics not available for the selected query", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    if (!stateTextBox.Text.Equals(WmsCommand.QUERY_STATE_COMPLETED) && !stateTextBox.Text.Equals(WmsCommand.QUERY_STATE_REJECTED))
                    {
                        setTextChangeIndicator(stateTextBox, "COMPLETED");
                        stateTextBox.Text = "COMPLETED";
                        ResetData();
                        MessageBox.Show(Utilities.GetForegroundControl(), "Query statistics no longer available for the selected query in WMS. It may be that the query has already completed.", "WMS Offender", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    }
                }

                //sqlPlanButton.Enabled = serviceEnable(serviceTextBox.Text, "PLAN");
                sqlPlanButton.Enabled = true;
                //sqlTextButton.Enabled = serviceEnable(serviceTextBox.Text, "TEXT");
                sqlTextButton.Enabled = true;

                enableDisableCancel();

                if (saveDataTable)
                {
                    m_previousDataTable = m_dataTable.Copy();
                }
            }
            catch (OdbcException ex)
            {
                stopTimer();
                MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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

        private void enableDisableCancel()
        {
            cancelQueryButton.Enabled = false;
            if (stateTextBox.Text.Equals("EXECUTING"))
            {
                cancelQueryButton.Enabled = true;
            }
            else if (stateTextBox.Text.Equals("WAITING"))
            {
                cancelQueryButton.Enabled = true;
            }
            else if (stateTextBox.Text.Equals("HOLDING"))
            {
                cancelQueryButton.Enabled = true;
            }
            else if (stateTextBox.Text.Equals("SUSPENDED"))
            {
                cancelQueryButton.Enabled = true;
            }
        }

        private void enableWarnIndicator(bool enable)
        {
            warnIndPictureBox.Enabled = enable;
            warnIndButton.Enabled = enable;
        }

        public void ResetData()
        {
            serverTextBox.Text = "";
            roleTextBox.Text = "";
            applicationTextBox.Text = "";
            computerTextBox.Text = "";
            userTextBox.Text = "";
            dataSourceTextBox.Text = "";
            queryTextBox.Text = "";
            estCostTextBox.Text = "";
            cpuExecTextBox.Text = "";
            ioTextBox.Text = "";
            messageTextBox.Text = "";
            idleTextBox.Text = "";
            totalTextBox.Text = "";
            cardinalityTextBox.Text = "";
            startTimeTextBox.Text = "";
            entryTimeTextBox.Text = "";
            subStateTextBox.Text = "";
            estMemoryTextBox.Text = "";
            resUsageTextBox.Text = "";
            affinityTextBox.Text = "";
            missingStatsTextBox.Text = "";
            numberJoinsTextBox.Text = "";
            tableFullScanTextBox.Text = "";
            dp2MaxBuffTextBox.Text = "";

            rowsFullScanTextBox.Text = "";
            dp2RowsAccessedTextBox.Text = "";
            dp2RowsUsedTextBox.Text = "";

            dopTextBox.Text = "";
            txnNeededTextBox.Text = "";
            manXProdTextBox.Text = "";

            warnLevelTextBox.Text = "";
            warnIndPictureBox.Image = this.warnIndImageList.Images[0];
            connRuleTextBox.Text = "";
            compRuleTextBox.Text = "";
            execRuleTextBox.Text = "";

            stmtIdTextBox.Text = "";
            stmtTypeTextBox.Text = "";

            compStartTextBox.Text = "";
            compEndTextBox.Text = "";
            execStartTextBox.Text = "";
            execEndTextBox.Text = "";
            execLastUpdatedTextBox.Text = "";
            execStateTextBox.Text = "";
            elapsedTextBox.Text = "";
            waitTextBox.Text = "";
            holdTextBox.Text = "";
            accessedRowsTextBox.Text = "";
            usedRowsTextBox.Text = "";
            messageCountTextBox.Text = "";
            messageBytesTextBox.Text = "";
            statsBytesTextBox.Text = "";
            diskIOsTextBox.Text = "";
            lockWaitsTextBox.Text = "";
            lockEscalationsTextBox.Text = "";
            sqlCodeTextBox.Text = "";
            numRowsIUDTextBox.Text = "";
            statsCodeTextBox.Text = "";
            sqlSpaceAllocTextBox.Text = "";
            sqlSpaceUsedTextBox.Text = "";
            sqlHeapAllocTextBox.Text = "";
            sqlHeapUsedTextBox.Text = "";
            eidSpaceAllocTextBox.Text = "";
            eidSpaceUsedTextBox.Text = "";
            eidHeapAllocTextBox.Text = "";
            eidHeapUsedTextBox.Text = "";

            estAccessedRowsTextBox.Text = "";
            estUsedRowsTextBox.Text = "";

            parentQIDTextBox.Text = "";
            priorityTextBox.Text = "";
            txnIDTextBox.Text = "";
            numSQLProcessesTextBox.Text = "";
            processesCreatedTextBox.Text = "";

            rowsReturnedTextBox.Text = "";
            reqMsgCountTextBox.Text = "";
            reqMsgBytesTextBox.Text = "";
            replyMsgCountTextBox.Text = "";
            replyMsgBytesTextBox.Text = "";

            totalMemAllocTextBox.Text = "";
            maxMemUsedTextBox.Text = "";

            errorAQRTextBox.Text = "";
            numRetriesAQRTextBox.Text = "";
            delayAQRTextBox.Text = "";

            aggrQueryTextBox.Text = "";
            aggrQueryLabel.Enabled = aggrQueryTextBox.Enabled = false;
            aggrTotalQueriesTextBox.Text = "";
            aggrTotalQueriesLabel.Enabled = aggrTotalQueriesTextBox.Enabled = false;
            aggrSecsSinceLastUpdateTextBox.Text = "";
            aggrSecsSinceLastUpdateLabel.Enabled = aggrSecsSinceLastUpdateTextBox.Enabled = false;
            aggrSecsTotalTimeTextBox.Text = "";
            aggrSecsTotalTimeLabel.Enabled = aggrSecsTotalTimeTextBox.Enabled = false;

            opensTextBox.Text = "";
            firstRowReturnedTextBox.Text = "";
            sqlCPUTextBox.Text = "";
            processBusyTextBox.Text = "";
            openTimeTextBox.Text = "";
            processCreateTimeTextBox.Text = "";

            totalElapsedTimeTextBox.Text = "";

            processorUsageTextBox.Text = "";

            totalSpaceAllocatedTextBox.Text = "";

            totalSpaceUsedTextBox.Text = "";
            rowsPerSecondTextBox.Text = "";
            iudPerSecondTextBox.Text = "";
            totalProcessorTextBox.Text = "";

            lastProcessorTextBox.Text = "";
            deltaProcessorLTextBox.Text = "";
            deltaSpaceAllocatedTextBox.Text = "";
            deltaSpaceUsedTextBox.Text = "";
            deltaDiskReadsTextBox.Text = "";
            deltaIUDTextBox.Text = "";

            cmpNumberOfBMOsTextBox.Text = "";
            cmpOverflowModeTextBox.Text = "";
            cmpOverflowSizeTextBox.Text = "";
            overflowSpaceAllocatedTextBox.Text = "";
            overflowSpaceUsedTextBox.Text = "";
            overflowBlockSizeTextBox.Text = "";
            overflowIOsTextBox.Text = "";
            overflowMessageToTextBox.Text = "";
            overflowMessageBytesOutTextBox.Text = "";
            overflowMessageOutTextBox.Text = "";
            overflowMessageBytesOutTextBox.Text = "";
        }

        private bool setStatusTextFields(DataRow r)
        {
            if (r != null)
            {
                setTextChangeIndicator(stateTextBox, r["QUERY_STATE"].ToString());
                stateTextBox.Text = r["QUERY_STATE"].ToString();
                if (stateTextBox.Text.Equals(WmsCommand.QUERY_STATE_COMPLETED) || stateTextBox.Text.Equals(WmsCommand.QUERY_STATE_REJECTED))
                {
                    stopTimer();
                }
                serviceTextBox.Text = r["SERVICE_NAME"].ToString();
                estCostTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_COST"].ToString()));
                cpuExecTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_CPU_TIME"].ToString()));
                ioTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_IO_TIME"].ToString()));
                messageTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_MSG_TIME"].ToString()));
                idleTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_IDLE_TIME"].ToString()));
                totalTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_TOTAL_TIME"].ToString()));
                cardinalityTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["EST_CARDINALITY"].ToString()));
                serverTextBox.Text = r["PROCESS_NAME"].ToString();
                queryTextBox.Text = r["QUERY_NAME"].ToString();
                roleTextBox.Text = r["ROLE_NAME"].ToString();
                startTimeTextBox.Text = r["START_TS"].ToString();
                entryTimeTextBox.Text = r["ENTRY_TS"].ToString();
                applicationTextBox.Text = r["APPLICATION_NAME"].ToString();
                computerTextBox.Text = r["COMPUTER_NAME"].ToString();
                userTextBox.Text = r["USER_NAME"].ToString();
                dataSourceTextBox.Text = r["DATASOURCE"].ToString();
                subStateTextBox.Text = r["QUERY_SUBSTATE"].ToString();
                estMemoryTextBox.Text = String.Format("{0:F}", Double.Parse(r["EST_TOTAL_MEM"].ToString()));
                resUsageTextBox.Text = r["EST_RESRC_USAGE"].ToString();
                affinityTextBox.Text = r["CMP_AFFINITY_NUM"].ToString();
                missingStatsTextBox.Text = r["CMP_MISSING_STATS"].ToString();
                numberJoinsTextBox.Text = r["CMP_NUM_JOINS"].ToString();
                tableFullScanTextBox.Text = r["CMP_FULL_SCAN_ON_TABLE"].ToString();
                if (m_cd.IsTrafodion)
                {
                    dp2MaxBuffTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["CMP_HIGH_EID_MAX_BUF_USAGE"].ToString()));
                    dp2RowsAccessedTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["CMP_EID_ROWS_ACCESSED"].ToString()));
                    dp2RowsUsedTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["CMP_EID_ROWS_USED"].ToString()));
                }
                else
                {
                    dp2MaxBuffTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["CMP_HIGH_DP2_MAX_BUF_USAGE"].ToString()));
                    dp2RowsAccessedTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["CMP_DP2_ROWS_ACCESSED"].ToString()));
                    dp2RowsUsedTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["CMP_DP2_ROWS_USED"].ToString()));
                }

                rowsFullScanTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["CMP_ROWS_ACCESSED_FULL_SCAN"].ToString()));

                dopTextBox.Text = r["CMP_DOP"].ToString();
                txnNeededTextBox.Text = r["CMP_TXN_NEEDED"].ToString();
                manXProdTextBox.Text = r["CMP_MANDATORY_X_PROD"].ToString();

                warnLevelTextBox.Text = r["WARN_LEVEL"].ToString();
                if (warnLevelTextBox.Text.Equals("") || warnLevelTextBox.Text.Equals("NOWARN"))
                {
                    warnIndPictureBox.Image = this.warnIndImageList.Images[0];
                    enableWarnIndicator(false);
                }
                else if (warnLevelTextBox.Text.Equals("LOW"))
                {
                    warnIndPictureBox.Image = this.warnIndImageList.Images[1];
                    enableWarnIndicator(true);
                }
                else if (warnLevelTextBox.Text.Equals("MEDIUM"))
                {
                    warnIndPictureBox.Image = this.warnIndImageList.Images[2];
                    enableWarnIndicator(true);
                }
                else if (warnLevelTextBox.Text.Equals("HIGH"))
                {
                    warnIndPictureBox.Image = this.warnIndImageList.Images[3];
                    enableWarnIndicator(true);
                }

                try
                {

                    connRuleTextBox.Text = r["CONN_RULE_NAME"].ToString();
                }
                catch (Exception ex)
                {
                    connRuleTextBox.Text = "";
                    connRuleLabel.Enabled = connRuleTextBox.Enabled = false;
                }
                try
                {

                    compRuleTextBox.Text = r["COMP_RULE_NAME"].ToString();
                }
                catch (Exception ex)
                {
                    compRuleTextBox.Text = "";
                    compRuleLabel.Enabled = compRuleTextBox.Enabled = false;
                }
                try
                {

                    execRuleTextBox.Text = r["EXEC_RULE_NAME"].ToString();
                }
                catch (Exception ex)
                {
                    execRuleTextBox.Text = "";
                    execRuleLabel.Enabled = execRuleTextBox.Enabled = false;
                }

                return true;
            }

            return false;
        }

        private bool setStatsTextFields(DataRow r)
        {
            if (r != null)
            {
                setTextChangeIndicator(stmtIdTextBox, r["STATEMENT_ID"].ToString());
                stmtIdTextBox.Text = r["STATEMENT_ID"].ToString();
                setTextChangeIndicator(stmtTypeTextBox, r["STATEMENT_TYPE"].ToString());
                stmtTypeTextBox.Text = r["STATEMENT_TYPE"].ToString();
                setTextChangeIndicator(compStartTextBox, WMSUtils.convertJulianTimeStamp(r["COMP_START_TIME"]));
                compStartTextBox.Text = WMSUtils.convertJulianTimeStamp(r["COMP_START_TIME"]);
                setTextChangeIndicator(compEndTextBox, WMSUtils.convertJulianTimeStamp(r["COMP_END_TIME"]));
                compEndTextBox.Text = WMSUtils.convertJulianTimeStamp(r["COMP_END_TIME"]);
                setTextChangeIndicator(execStartTextBox, WMSUtils.convertJulianTimeStamp(r["EXEC_START_TIME"]));
                execStartTextBox.Text = WMSUtils.convertJulianTimeStamp(r["EXEC_START_TIME"]);
                setTextChangeIndicator(execEndTextBox, WMSUtils.convertJulianTimeStamp(r["EXEC_END_TIME"]));
                execEndTextBox.Text = WMSUtils.convertJulianTimeStamp(r["EXEC_END_TIME"]);
                execLastUpdatedTextBox.Text = WMSUtils.convertJulianTimeStamp(r["EXEC_LAST_UPDATED"]);
                setTextChangeIndicator(execStateTextBox, r["EXEC_STATE"].ToString());
                execStateTextBox.Text = r["EXEC_STATE"].ToString();
                setTextChangeIndicator(elapsedTextBox, WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(Int64.Parse(r["ELAPSED_TIME"].ToString()) / 1000000)));
                elapsedTextBox.Text = WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(Int64.Parse(r["ELAPSED_TIME"].ToString()) / 1000000));
                setTextChangeIndicator(waitTextBox, WMSUtils.formatInt2Time(Int64.Parse(r["WAIT_TIME"].ToString())));
                waitTextBox.Text = WMSUtils.formatInt2Time(Int64.Parse(r["WAIT_TIME"].ToString()));
                setTextChangeIndicator(holdTextBox, WMSUtils.formatInt2Time(Int64.Parse(r["HOLD_TIME"].ToString())));
                holdTextBox.Text = WMSUtils.formatInt2Time(Int64.Parse(r["HOLD_TIME"].ToString()));
                setTextChangeIndicator(accessedRowsTextBox, String.Format("{0:#,0}", Double.Parse(r["ACCESSED_ROWS"].ToString())));
                accessedRowsTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["ACCESSED_ROWS"].ToString()));
                setTextChangeIndicator(usedRowsTextBox, String.Format("{0:#,0}", Double.Parse(r["USED_ROWS"].ToString())));
                usedRowsTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["USED_ROWS"].ToString()));
                setTextChangeIndicator(messageCountTextBox, String.Format("{0:#,0}", Int64.Parse(r["MESSAGE_COUNT"].ToString())));
                messageCountTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["MESSAGE_COUNT"].ToString()));
                setTextChangeIndicator(messageBytesTextBox, String.Format("{0:#,0}", Int64.Parse(r["MESSAGE_BYTES"].ToString())));
                messageBytesTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["MESSAGE_BYTES"].ToString()));
                setTextChangeIndicator(statsBytesTextBox, String.Format("{0:#,0}", Int64.Parse(r["STATS_BYTES"].ToString())));
                statsBytesTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["STATS_BYTES"].ToString()));
                setTextChangeIndicator(diskIOsTextBox, String.Format("{0:#,0}", Int64.Parse(r["DISK_IOS"].ToString())));
                diskIOsTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["DISK_IOS"].ToString()));
                setTextChangeIndicator(lockWaitsTextBox, String.Format("{0:D}", Int64.Parse(r["LOCK_WAITS"].ToString())));
                lockWaitsTextBox.Text = String.Format("{0:D}", Int64.Parse(r["LOCK_WAITS"].ToString()));
                setTextChangeIndicator(lockEscalationsTextBox, String.Format("{0:D}", Int64.Parse(r["LOCK_ESCALATIONS"].ToString())));
                lockEscalationsTextBox.Text = String.Format("{0:D}", Int64.Parse(r["LOCK_ESCALATIONS"].ToString()));
                setTextChangeIndicator(sqlCodeTextBox, String.Format("{0:D}", Int64.Parse(r["SQL_ERROR_CODE"].ToString())));
                sqlCodeTextBox.Text = String.Format("{0:D}", Int64.Parse(r["SQL_ERROR_CODE"].ToString()));
                setTextChangeIndicator(numRowsIUDTextBox, String.Format("{0:#,0}", Int64.Parse(r["NUM_ROWS_IUD"].ToString())));
                numRowsIUDTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["NUM_ROWS_IUD"].ToString()));
                setTextChangeIndicator(statsCodeTextBox, String.Format("{0:D}", Int64.Parse(r["STATS_ERROR_CODE"].ToString())));
                statsCodeTextBox.Text = String.Format("{0:D}", Int64.Parse(r["STATS_ERROR_CODE"].ToString()));
                setTextChangeIndicator(sqlSpaceAllocTextBox, String.Format("{0:#,0}", Int64.Parse(r["SQL_SPACE_ALLOC"].ToString())));
                sqlSpaceAllocTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["SQL_SPACE_ALLOC"].ToString()));
                setTextChangeIndicator(sqlSpaceUsedTextBox, String.Format("{0:#,0}", Int64.Parse(r["SQL_SPACE_USED"].ToString())));
                sqlSpaceUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["SQL_SPACE_USED"].ToString()));
                setTextChangeIndicator(sqlHeapAllocTextBox, String.Format("{0:#,0}", Int64.Parse(r["SQL_HEAP_ALLOC"].ToString())));
                sqlHeapAllocTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["SQL_HEAP_ALLOC"].ToString()));
                setTextChangeIndicator(sqlHeapUsedTextBox, String.Format("{0:#,0}", Int64.Parse(r["SQL_HEAP_USED"].ToString())));
                sqlHeapUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["SQL_HEAP_USED"].ToString()));
                setTextChangeIndicator(eidSpaceAllocTextBox, String.Format("{0:#,0}", Int64.Parse(r["EID_SPACE_ALLOC"].ToString())));
                eidSpaceAllocTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["EID_SPACE_ALLOC"].ToString()));
                setTextChangeIndicator(eidSpaceUsedTextBox, String.Format("{0:#,0}", Int64.Parse(r["EID_SPACE_USED"].ToString())));
                eidSpaceUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["EID_SPACE_USED"].ToString()));
                setTextChangeIndicator(eidHeapAllocTextBox, String.Format("{0:#,0}", Int64.Parse(r["EID_HEAP_ALLOC"].ToString())));
                eidHeapAllocTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["EID_HEAP_ALLOC"].ToString()));
                setTextChangeIndicator(eidHeapUsedTextBox, String.Format("{0:#,0}", Int64.Parse(r["EID_HEAP_USED"].ToString())));
                eidHeapUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["EID_HEAP_USED"].ToString()));

                setTextChangeIndicator(estAccessedRowsTextBox, String.Format("{0:#,0}", Double.Parse(r["EST_ACCESSED_ROWS"].ToString())));
                estAccessedRowsTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["EST_ACCESSED_ROWS"].ToString()));
                setTextChangeIndicator(estUsedRowsTextBox, String.Format("{0:#,0}", Double.Parse(r["EST_USED_ROWS"].ToString())));
                estUsedRowsTextBox.Text = String.Format("{0:#,0}", Double.Parse(r["EST_USED_ROWS"].ToString()));

                setTextChangeIndicator(parentQIDTextBox, r["PARENT_QUERY_ID"].ToString());
                parentQIDTextBox.Text = r["PARENT_QUERY_ID"].ToString();
                setTextChangeIndicator(priorityTextBox, String.Format("{0:#,0}", Int64.Parse(r["QUERY_PRIORITY"].ToString())));
                priorityTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["QUERY_PRIORITY"].ToString()));
                setTextChangeIndicator(txnIDTextBox, r["TRANSACTION_ID"].ToString());
                txnIDTextBox.Text = r["TRANSACTION_ID"].ToString();
                setTextChangeIndicator(numSQLProcessesTextBox, String.Format("{0:#,0}", Int64.Parse(r["NUM_SQL_PROCESSES"].ToString())));
                numSQLProcessesTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["NUM_SQL_PROCESSES"].ToString()));
                setTextChangeIndicator(processesCreatedTextBox, String.Format("{0:#,0}", Int64.Parse(r["PROCESSES_CREATED"].ToString())));
                processesCreatedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["PROCESSES_CREATED"].ToString()));

                setTextChangeIndicator(rowsReturnedTextBox, String.Format("{0:#,0}", Int64.Parse(r["ROWS_RETURNED"].ToString())));
                rowsReturnedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["ROWS_RETURNED"].ToString()));
                setTextChangeIndicator(reqMsgCountTextBox, String.Format("{0:#,0}", Int64.Parse(r["REQ_MESSAGE_COUNT"].ToString())));
                reqMsgCountTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["REQ_MESSAGE_COUNT"].ToString()));
                setTextChangeIndicator(reqMsgBytesTextBox, String.Format("{0:#,0}", Int64.Parse(r["REQ_MESSAGE_BYTES"].ToString())));
                reqMsgBytesTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["REQ_MESSAGE_BYTES"].ToString()));
                setTextChangeIndicator(replyMsgCountTextBox, String.Format("{0:#,0}", Int64.Parse(r["REPLY_MESSAGE_COUNT"].ToString())));
                replyMsgCountTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["REPLY_MESSAGE_COUNT"].ToString()));
                setTextChangeIndicator(replyMsgBytesTextBox, String.Format("{0:#,0}", Int64.Parse(r["REPLY_MESSAGE_BYTES"].ToString())));
                replyMsgBytesTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["REPLY_MESSAGE_BYTES"].ToString()));

                setTextChangeIndicator(totalMemAllocTextBox, String.Format("{0:#,0}", Int64.Parse(r["TOTAL_MEM_ALLOC"].ToString())));
                totalMemAllocTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["TOTAL_MEM_ALLOC"].ToString()));
                setTextChangeIndicator(maxMemUsedTextBox, String.Format("{0:#,0}", Int64.Parse(r["MAX_MEM_USED"].ToString())));
                maxMemUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["MAX_MEM_USED"].ToString()));

                setTextChangeIndicator(errorAQRTextBox, String.Format("{0:D}", Int64.Parse(r["LAST_ERROR_BEFORE_AQR"].ToString())));
                errorAQRTextBox.Text = String.Format("{0:D}", Int64.Parse(r["LAST_ERROR_BEFORE_AQR"].ToString()));
                setTextChangeIndicator(numRetriesAQRTextBox, String.Format("{0:D}", Int64.Parse(r["AQR_NUM_RETRIES"].ToString())));
                numRetriesAQRTextBox.Text = String.Format("{0:D}", Int64.Parse(r["AQR_NUM_RETRIES"].ToString()));
                setTextChangeIndicator(delayAQRTextBox, String.Format("{0:D}", Int64.Parse(r["DELAY_BEFORE_AQR"].ToString())));
                delayAQRTextBox.Text = String.Format("{0:D}", Int64.Parse(r["DELAY_BEFORE_AQR"].ToString()));

                if (this.m_cd.IsTrafodion)
                {
                    _overflowGroupBox.Visible = true;
                    setTextChangeIndicator(cmpNumberOfBMOsTextBox, String.Format("{0:#,0}", Int64.Parse(r["CMP_NUMBER_OF_BMOS"].ToString())));
                    cmpNumberOfBMOsTextBox.Text = String.Format("{0:#,0}", Int32.Parse(r["CMP_NUMBER_OF_BMOS"].ToString()));
                    cmpOverflowModeTextBox.Text = r["CMP_OVERFLOW_MODE"].ToString();
                    setTextChangeIndicator(cmpOverflowSizeTextBox, String.Format("{0:#,0}", Int64.Parse(r["CMP_OVERFLOW_SIZE"].ToString())));
                    cmpOverflowSizeTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["CMP_OVERFLOW_SIZE"].ToString()));
                    setTextChangeIndicator(overflowSpaceAllocatedTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_SPACE_ALLOCATED"].ToString())));
                    overflowSpaceAllocatedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_SPACE_ALLOCATED"].ToString()));

                    setTextChangeIndicator(overflowSpaceUsedTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_SPACE_USED"].ToString())));
                    overflowSpaceUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_SPACE_USED"].ToString()));

                    setTextChangeIndicator(overflowBlockSizeTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_BLOCK_SIZE"].ToString())));
                    overflowBlockSizeTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_BLOCK_SIZE"].ToString()));

                    setTextChangeIndicator(overflowIOsTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_IOS"].ToString())));
                    overflowIOsTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_IOS"].ToString()));

                    setTextChangeIndicator(overflowMessageToTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_TO"].ToString())));
                    overflowMessageToTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_TO"].ToString()));

                    setTextChangeIndicator(overflowMessageBytesToTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_BYTES_TO"].ToString())));
                    overflowMessageBytesToTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_BYTES_TO"].ToString()));

                    setTextChangeIndicator(overflowMessageOutTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_OUT"].ToString())));
                    overflowMessageOutTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_OUT"].ToString()));

                    setTextChangeIndicator(overflowMessageBytesOutTextBox, String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_BYTES_OUT"].ToString())));
                    overflowMessageBytesOutTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["OVF_MESSAGE_BYTES_OUT"].ToString()));
                }
                else
                {
                    _overflowGroupBox.Visible = false; //Hide overflow for Trafodion
                }

                try
                {
                    setTextChangeIndicator(aggrQueryTextBox, r["AGGR_QUERY"].ToString());
                    aggrQueryTextBox.Text = r["AGGR_QUERY"].ToString();
                }
                catch (Exception ex)
                {
                    aggrQueryTextBox.Text = "";
                    aggrQueryLabel.Enabled = aggrQueryTextBox.Enabled = false;
                }
                try
                {
                    setTextChangeIndicator(aggrTotalQueriesTextBox, String.Format("{0:#,0}", Int64.Parse(r["AGGR_TOTAL_QUERIES"].ToString())));
                    aggrTotalQueriesTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["AGGR_TOTAL_QUERIES"].ToString()));
                }
                catch (Exception ex)
                {
                    aggrTotalQueriesTextBox.Text = "";
                    aggrTotalQueriesLabel.Enabled = aggrTotalQueriesTextBox.Enabled = false;
                }
                try
                {
                    setTextChangeIndicator(aggrSecsSinceLastUpdateTextBox, String.Format("{0:#,0}", Int64.Parse(r["AGGR_SECS_SINCE_LAST_UPDATE"].ToString())));
                    aggrSecsSinceLastUpdateTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["AGGR_SECS_SINCE_LAST_UPDATE"].ToString()));
                }
                catch (Exception ex)
                {
                    aggrSecsSinceLastUpdateTextBox.Text = "";
                    aggrSecsSinceLastUpdateLabel.Enabled = aggrSecsSinceLastUpdateTextBox.Enabled = false;
                }
                try
                {
                    setTextChangeIndicator(aggrSecsTotalTimeTextBox, String.Format("{0:#,0}", Int64.Parse(r["AGGR_SECS_TOTAL_TIME"].ToString())));
                    aggrSecsTotalTimeTextBox.Text = String.Format("{0:#,0}", Int64.Parse(r["AGGR_SECS_TOTAL_TIME"].ToString()));
                }
                catch (Exception ex)
                {
                    aggrSecsTotalTimeTextBox.Text = "";
                    aggrSecsTotalTimeLabel.Enabled = aggrSecsTotalTimeTextBox.Enabled = false;
                }
               
                queryTextTextBox.Text = r["QUERY_TEXT"].ToString();
                queryLengthTextBox.Text = String.Format("{0:D}", Int64.Parse(r["QUERY_TEXT_LEN"].ToString()));
                setTextChangeIndicator(opensTextBox, String.Format("{0:D}", Int64.Parse(r["OPENS"].ToString())));
                opensTextBox.Text = String.Format("{0:D}", Int64.Parse(r["OPENS"].ToString()));
                setTextChangeIndicator(firstRowReturnedTextBox, WMSUtils.convertJulianTimeStamp(r["FIRST_ROW_RETURNED_TIME"]));
                firstRowReturnedTextBox.Text = WMSUtils.convertJulianTimeStamp(r["FIRST_ROW_RETURNED_TIME"]);
                setTextChangeIndicator(sqlCPUTextBox, String.Format("{0:#,0.000}", Double.Parse(r["SQL_CPU_TIME"].ToString()) / 1000000));
                sqlCPUTextBox.Text = String.Format("{0:#,0.000}", Double.Parse(r["SQL_CPU_TIME"].ToString()) / 1000000);
                setTextChangeIndicator(processBusyTextBox, String.Format("{0:#,0.000}", Double.Parse(r["PROCESS_BUSYTIME"].ToString()) / 1000000));
                processBusyTextBox.Text = String.Format("{0:#,0.000}", Double.Parse(r["PROCESS_BUSYTIME"].ToString()) / 1000000);
                setTextChangeIndicator(openTimeTextBox, String.Format("{0:#,0.000}", Double.Parse(r["OPEN_TIME"].ToString()) / 1000000));
                openTimeTextBox.Text = String.Format("{0:#,0.000}", Double.Parse(r["OPEN_TIME"].ToString()) / 1000000);
                setTextChangeIndicator(processCreateTimeTextBox, String.Format("{0:#,0.000}", Double.Parse(r["PROCESS_CREATE_TIME"].ToString()) / 1000000));
                processCreateTimeTextBox.Text = String.Format("{0:#,0.000}", Double.Parse(r["PROCESS_CREATE_TIME"].ToString()) / 1000000);

                Int64 elapsed_time = Int64.Parse(r["ELAPSED_TIME"].ToString()) / 1000000;
                Int64 wait_time = Int64.Parse(r["WAIT_TIME"].ToString());
                Int64 hold_time = Int64.Parse(r["HOLD_TIME"].ToString());
                Int64 total_time = elapsed_time + wait_time + hold_time;
                setTextChangeIndicator(totalElapsedTimeTextBox, WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(total_time)));
                totalElapsedTimeTextBox.Text = WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(total_time));

                double totalCPUTimeMicroSecs = Double.Parse(r["SQL_CPU_TIME"].ToString()) +
                                       Double.Parse(r["PROCESS_BUSYTIME"].ToString()) +
                                       Double.Parse(r["OPEN_TIME"].ToString()) +
                                       Double.Parse(r["PROCESS_CREATE_TIME"].ToString());

                Int64 elapsedTime = Int64.Parse(r["ELAPSED_TIME"].ToString()) / 1000000;
                double processorUsageSec = 0.0;
                if (elapsedTime > 0)
                {
                    processorUsageSec = (totalCPUTimeMicroSecs / (1000 * 1000)) / elapsedTime;
                }
                setTextChangeIndicator(processorUsageTextBox, String.Format("{0:0.000}", processorUsageSec));
                processorUsageTextBox.Text = String.Format("{0:0.000}", processorUsageSec);

                try
                {
                    Int64 totalSpaceAllocated = getTotalSpaceAllocated(r);
                    setTextChangeIndicator(totalSpaceAllocatedTextBox, String.Format("{0:#,0}", Int64.Parse(totalSpaceAllocated.ToString())));
                    totalSpaceAllocatedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(totalSpaceAllocated.ToString()));

                    Int64 totalSpaceUsed = getTotalSpaceUsed(r);
                    setTextChangeIndicator(totalSpaceUsedTextBox, String.Format("{0:#,0}", Int64.Parse(totalSpaceUsed.ToString())));
                    totalSpaceUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(totalSpaceUsed.ToString()));
                }
                catch (Exception) { }
                try
                {
                    Int64 usedRows = Int64.Parse(r["USED_ROWS"].ToString());
                    double rowsPerSecond = 0;
                    if (usedRows > 0)
                    {
                        rowsPerSecond = usedRows / elapsedTime;
                    }
                    setTextChangeIndicator(rowsPerSecondTextBox, String.Format("{0:#,0}", Int64.Parse(rowsPerSecond.ToString())));
                    rowsPerSecondTextBox.Text = String.Format("{0:#,0}", Int64.Parse(rowsPerSecond.ToString()));
                }
                catch (Exception ex) { }

                try
                {
                    Int64 IUDRows = Int64.Parse(r["NUM_ROWS_IUD"].ToString());
                    double iudsPerSecond = 0;
                    if (IUDRows > 0)
                    {
                        iudsPerSecond = IUDRows / elapsedTime;
                    }
                    setTextChangeIndicator(iudPerSecondTextBox, String.Format("{0:#,0}", Int64.Parse(iudsPerSecond.ToString())));
                    iudPerSecondTextBox.Text = String.Format("{0:#,0}", Int64.Parse(iudsPerSecond.ToString()));
                }
                catch (Exception ex) { }

                string totalProcessTime = "00:00:00.000";
                try
                {
                    DateTime now = DateTime.Now;
                    DateTime startTime = now.Subtract(TimeSpan.FromMilliseconds(totalCPUTimeMicroSecs / 1000));
                    totalProcessTime = WMSUtils.getFormattedElapsedTime(startTime, now);
                }
                catch (Exception ex)
                { 
                }
                setTextChangeIndicator(totalProcessorTextBox, totalProcessTime);
                totalProcessorTextBox.Text = totalProcessTime;

                string lastProcessor = "00:00:00.000";
                string deltaProcessor = "00:00:00.000";
                Int64 deltaSpaceUsed = 0;
                Int64 deltaSpaceAllocated = 0;
                Int64 deltaIUD = 0;
                Int64 deltaIOs = 0;

                if (m_previousDataTable != null)
                {
                    DataRow prevStatsRow = null;
                    DataRow[] theRows = new DataRow[0];
                    //fetch the DataRow of the previous DataTable
                    try
                    {
                        //theRows = m_previousDataTable.Select("TRIM(QUERY_ID) = '" + m_qid.Trim() + "'");
                        string start_ts = r["START_TS"].ToString();
                        string filter = "QUERY_ID = '" + m_qid + "' AND START_TS = '" + start_ts + "'";
                        theRows = m_previousDataTable.Select(filter);
                        if (theRows.Length > 0)
                        {
                            prevStatsRow = theRows[0];
                        }
                    }
                    catch (Exception ex)
                    { }

                    if (prevStatsRow != null)
                    {
                        //get the last interval processor time of the previous DataTable
                        double previousTotalCPUTimeMicroSecs = Double.Parse(prevStatsRow["SQL_CPU_TIME"].ToString()) +
                                               Double.Parse(prevStatsRow["PROCESS_BUSYTIME"].ToString()) +
                                               Double.Parse(prevStatsRow["OPEN_TIME"].ToString()) +
                                               Double.Parse(prevStatsRow["PROCESS_CREATE_TIME"].ToString());
                        try
                        {
                            DateTime now = DateTime.Now;
                            DateTime startTime2 = now.Subtract(TimeSpan.FromMilliseconds(previousTotalCPUTimeMicroSecs / 1000));
                            lastProcessor = WMSUtils.getFormattedElapsedTime(startTime2, now);
                        }
                        catch (Exception ex)
                        { }

                        //get the delta processor time of the previous DataTable
                        try
                        {
                            double cpuTimeMicroSec = totalCPUTimeMicroSecs - previousTotalCPUTimeMicroSecs;
                            TimeSpan timeDiff = TimeSpan.FromMilliseconds(cpuTimeMicroSec / 1000);
                            deltaProcessor = String.Format("{0:00}", timeDiff.Hours) + ":" +
                                             String.Format("{0:00}", timeDiff.Minutes) + ":" +
                                             String.Format("{0:00}", timeDiff.Seconds) + "." +
                                             String.Format("{0:000}", timeDiff.Milliseconds);
                        }
                        catch (Exception ex)
                        { }

                        try
                        {
                            deltaSpaceAllocated = getTotalSpaceAllocated(r) - getTotalSpaceAllocated(prevStatsRow);
                        }
                        catch (Exception ex)
                        { }
                        try
                        {
                            deltaSpaceUsed = getTotalSpaceUsed(r) - getTotalSpaceUsed(prevStatsRow);
                        }
                        catch (Exception ex)
                        { }

                        try
                        {
                            Int64 diskIOs = Int64.Parse(r["DISK_IOS"].ToString());
                            Int64 prevDiskIOS = Int64.Parse(prevStatsRow["DISK_IOS"].ToString());
                            deltaIOs = diskIOs - prevDiskIOS;
                        }
                        catch (Exception) { }
                        try
                        {
                            Int64 currentIUD = Int64.Parse(r["NUM_ROWS_IUD"].ToString());
                            Int64 prevIUD = Int64.Parse(prevStatsRow["NUM_ROWS_IUD"].ToString());
                            deltaIUD = currentIUD - prevIUD;
                        }
                        catch (Exception) { }
                    }
                }
                setTextChangeIndicator(lastProcessorTextBox, lastProcessor);
                lastProcessorTextBox.Text = lastProcessor;
                setTextChangeIndicator(deltaProcessorLTextBox, deltaProcessor);
                deltaProcessorLTextBox.Text = deltaProcessor;
                setTextChangeIndicator(deltaSpaceAllocatedTextBox, String.Format("{0:#,0}", Int64.Parse(deltaSpaceAllocated.ToString())));
                deltaSpaceAllocatedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(deltaSpaceAllocated.ToString()));
                setTextChangeIndicator(deltaSpaceUsedTextBox, String.Format("{0:#,0}", Int64.Parse(deltaSpaceUsed.ToString())));
                deltaSpaceUsedTextBox.Text = String.Format("{0:#,0}", Int64.Parse(deltaSpaceUsed.ToString()));

                setTextChangeIndicator(deltaDiskReadsTextBox, String.Format("{0:#,0}", Int64.Parse(deltaIOs.ToString())));
                deltaDiskReadsTextBox.Text = String.Format("{0:#,0}", Int64.Parse(deltaIOs.ToString()));

                setTextChangeIndicator(deltaIUDTextBox, String.Format("{0:#,0}", Int64.Parse(deltaIUD.ToString())));
                deltaIUDTextBox.Text = String.Format("{0:#,0}", Int64.Parse(deltaIUD.ToString()));

                return true;
            }
            return false;
        }

        private Int64 getTotalSpaceAllocated(DataRow r)
        {
            Int64 totalSpaceAllocated = 0;
            try
            {
                Int64 sqlSpaceAllocated = Int64.Parse(r["SQL_SPACE_ALLOC"].ToString());
                Int64 sqlHeapAllocated = Int64.Parse(r["SQL_HEAP_ALLOC"].ToString());
                Int64 eidSpaceAllocated = Int64.Parse(r["EID_SPACE_ALLOC"].ToString());
                Int64 eidHeapAllocated = Int64.Parse(r["EID_HEAP_ALLOC"].ToString());
                totalSpaceAllocated = sqlSpaceAllocated + sqlHeapAllocated + eidHeapAllocated + eidSpaceAllocated;
            }
            catch (Exception) { }

            return totalSpaceAllocated;
        }

        private Int64 getTotalSpaceUsed(DataRow r)
        {
            Int64 totalSpaceAllocated = 0;
            try
            {
                Int64 sqlSpaceUsed = Int64.Parse(r["SQL_SPACE_USED"].ToString());
                Int64 sqlHeapUsed = Int64.Parse(r["SQL_HEAP_USED"].ToString());
                Int64 eidSpaceUsed = Int64.Parse(r["EID_SPACE_USED"].ToString());
                Int64 eidHeapUsed = Int64.Parse(r["EID_HEAP_USED"].ToString());
                totalSpaceAllocated = sqlSpaceUsed + sqlHeapUsed + eidSpaceUsed + eidHeapUsed;
            }
            catch (Exception) { }

            return totalSpaceAllocated;
        }

        private void buttonSQLText_Click(object sender, EventArgs e)
        {
            try
            {
                WMSPlanText sqlText = new WMSPlanText(m_parent, m_dbConfig.ConnectionDefinition, m_qid, m_start_ts, queryTextTextBox.Text, true);
                sqlText.Show();
            }
            catch (OdbcException ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void buttonSQLPlan_Click(object sender, EventArgs e)
        {
            try
            {
                WMSPlanText sqlPlan = new WMSPlanText(m_parent, m_dbConfig.ConnectionDefinition, m_qid, m_start_ts, queryTextTextBox.Text, false);
                sqlPlan.Show();
            }
            catch (OdbcException ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void buttonCancelQuery_Click(object sender, EventArgs e)
        {
            DialogResult result = MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to cancel the query?", m_title, MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            if (result == DialogResult.Yes)
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
                        command.CommandText = "CANCEL QUERY " + m_qid;
                        command.ExecuteNonQuery();
                        MessageBox.Show(Utilities.GetForegroundControl(), "The query was cancelled successfully", m_title, MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                    else
                    {
                        MessageBox.Show(Utilities.GetForegroundControl(), "Error unable to obtain OdbcConnection", m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
                    }
                }
                catch (OdbcException ex)
                {
                    MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
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
                    getWMSInfo(true);
                    Cursor.Current = Cursors.Default;
                }
            }
        }

        private void buttonRefresh_Click(object sender, EventArgs e)
        {
            getWMSInfo(true);
        }

        private void setTextChangeIndicator(TextBox textBox, String text)
        {
            Color bc = textBox.BackColor;
            if (!textBox.Text.Equals(text) && textBox.Text.Length > 0)
            {
                if (m_parent is MonitorWorkloadCanvas)
                    textBox.ForeColor = MonitorWorkloadOptions.GetOptions().HighLightChangesColor;
                else
                    textBox.ForeColor = Color.Blue;
                textBox.BackColor = bc;
            }
            else
            {
                textBox.ForeColor = Color.Black;
                textBox.BackColor = bc;
            }
        }

        //private void setTextChangeIndicator(Trafodion.Manager.Framework.Controls.TrafodionRichTextBox textBox, String text)
        //{
        //    Color bc = textBox.BackColor;
        //    if (!textBox.Text.Equals(text) && textBox.Text.Length > 0)
        //    {
        //        textBox.ForeColor = Color.Blue;
        //        textBox.BackColor = bc;
        //    }
        //    else
        //    {
        //        textBox.ForeColor = Color.Black;
        //        textBox.BackColor = bc;
        //    }
        //}

        void WMSQueryInfo_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (refreshTimer.Enabled)
            {
                refreshTimer.Stop();
            }
            if (m_parent != null)
            {
                m_parent.RemoveQueryFromWatch(this);
            }

            if (widgetCanvas != null)
            {
                widgetCanvas.SaveToPersistence();
            }
        }

        private void startTimer()
        {
            refreshTimer.Interval = 1000;
            if (!refreshTimer.Enabled)
            {
                refreshTimer.Start();
            }
            if (!autoRefreshCheckBox.Checked)
            {
                autoRefreshCheckBox.Checked = true;
            }
            refreshProgressBar.Visible = true;
            setProgressBar();
        }

        private void stopTimer()
        {
            if (refreshTimer.Enabled)
            {
                refreshTimer.Stop();
            }
            if (autoRefreshCheckBox.Checked)
            {
                autoRefreshCheckBox.Checked = false;
            }
            refreshProgressBar.Visible = false;
        }

        private void setProgressBar()
        {
            refreshProgressBar.Value = 0;
            refreshProgressBar.Minimum = 0;
            refreshProgressBar.Maximum = m_refreshRate;
            refreshProgressBar.Step = 1;
            refreshProgressBar.PerformStep();
        }

        void refreshTimer_Tick(object sender, EventArgs e)
        {
            if (m_tickCount < m_refreshRate)
            {
                refreshProgressBar.PerformStep();
                m_tickCount++;
            }
            else
            {
                getWMSInfo(true);
                setProgressBar();
                m_tickCount = 0;
            }
        }

        private void refreshRateTrackBar_Scroll(object sender, EventArgs e)
        {
            m_refreshRate = refreshRateTrackBar.Value;
            refreshRateValueLabel.Text = m_refreshRate.ToString() + " sec";
            setProgressBar();
            m_tickCount = 0;
        }

        private void autoRefreshCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (autoRefreshCheckBox.Checked)
            {
                startTimer();
            }
            else
            {
                stopTimer();
            }
        }

        private void warnIndButton_Click(object sender, EventArgs e)
        {
            try
            {
                WMSWarnInfo warnInfo = new WMSWarnInfo(m_parent, m_cd, m_qid);
                warnInfo.Show();
            }
            catch (OdbcException ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void ruleAssociatedButton_Click(object sender, EventArgs e)
        {
            string serviceName = serviceTextBox.Text;
            if (!string.IsNullOrEmpty(serviceName))
            {
                WMSRulesAssociated ra = new WMSRulesAssociated(m_parent, m_cd, serviceName);
                ra.Show();
            }
        }

        private void repositoryInfoButton_Click(object sender, EventArgs e)
        {
            try
            {
                WMSRepositoryInfo repositoryInfo = new WMSRepositoryInfo(m_parent, m_dbConfig.ConnectionDefinition, m_qid);
                repositoryInfo.Show();
            }
            catch (OdbcException ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void label86_Click(object sender, EventArgs e)
        {

        }

        private void runTimeStatsPanel_Paint(object sender, PaintEventArgs e)
        {

        }

        private void layoutCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (widgetCanvas != null)
            {
                widgetCanvas.Locked = layoutCheckBox.Checked;
            }
            if (!layoutCheckBox.Checked)
            {
                widgetCanvas.ResetWidgetLayout();
            }
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.WorkloadDetail);
        }

        //View Session Statistics Click
        private void viewSessionStatsButton_Click(object sender, EventArgs e)
        {
            try
            {
                WMSViewSessionStats repositoryInfo = new WMSViewSessionStats(m_dbConfig.ConnectionDefinition, m_qid);
                repositoryInfo.Show();
            }
            catch (OdbcException ex)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), "OdbcException: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }
}
