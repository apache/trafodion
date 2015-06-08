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
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSPlanText : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Members
        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_sqlText = null;
        private bool m_isSqlText = false;
        private string m_title = "";
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        #endregion

        #region Constructors

        public WMSPlanText(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts, string sqlText, bool isSqlText)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            m_start_ts = start_ts;
            m_sqlText = sqlText;
            m_isSqlText = isSqlText;
            oneGuiBannerControl1.ConnectionDefinition = aConnectionDefinition;

            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Live View";

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered += InvokeHandleError;
            this.Disposed += new EventHandler(WMSPlanText_Disposed);
            this.timer1.Tick += new EventHandler(timer1_Tick);

            textBoxPlanText.Text = "";
            textBoxPlanText.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            textBoxPlanText.AppendText("-- INFO: Fetching Data");
            
            if (isSqlText)
            {
                m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText;
            }
            else
            {
                m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLPlan;
            }
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.EXEC_START_UTC_TS = start_ts;
            m_dbDataProvider.Start();

            startTimer();
            setupProgressBar();
            CenterToParent();
        }
        #endregion

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

        void WMSPlanText_Disposed(object sender, EventArgs e)
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
            stopTimer();
            if (!progressBar1.IsDisposed)
            {
                progressBar1.Visible = false;
                textBoxPlanText.Text = "";
                if (m_isSqlText)
                {
                    getSQLText();
                }
                else
                {
                    getSQLPlan();
                }
            }
        }

        private void getSQLText()
        {
            this.Text = "SQL text - " + m_qid;
            textBoxPlanText.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            if (m_dbDataProvider.QueryText != null && m_dbDataProvider.QueryText.Length > 0)
            {
                textBoxPlanText.AppendText("-- INFO: " + m_dbDataProvider.ViewName + Environment.NewLine);
                textBoxPlanText.AppendText(Environment.NewLine + m_dbDataProvider.QueryText + Environment.NewLine);
            }
            else
            {
                textBoxPlanText.AppendText("-- INFO: PREVIEW" + Environment.NewLine);
                textBoxPlanText.AppendText(Environment.NewLine + m_sqlText + Environment.NewLine);
            }
            this.Show();
        }

        private void getSQLPlan()
        {
            this.Text = "SQL plan - " + m_qid;
            DataTable dataTable = m_dbDataProvider.DatabaseDataTable;
            bool found = false;
            string planInfo = "";
            if (dataTable != null)
            {
                foreach (DataRow r in dataTable.Rows)
                {
                    object[] cols = r.ItemArray;
                    planInfo = (string)cols[0];
                    found = true;
                    break;
                }
            }
            textBoxPlanText.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            if (found)
            {
                textBoxPlanText.AppendText(planInfo + Environment.NewLine);
            }
            else
            {
                textBoxPlanText.AppendText(Environment.NewLine + "SQL plan not available for the selected query" + Environment.NewLine);
            }
            this.Show();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            if (m_isSqlText)
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SQLText);
            }
            else
            {
                TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.PlanText);
            }
        }

        private void closeButton_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}
