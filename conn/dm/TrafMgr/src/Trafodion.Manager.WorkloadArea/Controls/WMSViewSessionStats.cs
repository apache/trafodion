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
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSViewSessionStats : TrafodionForm
    {
        #region Constants
        private const int IGRIDINFO_MIN_WIDTH = 50;
        #endregion

        #region Members
        private string m_qid = null;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        #endregion

        public WMSViewSessionStats(ConnectionDefinition aConnectionDefinition, string qid)
        {
            InitializeComponent();
            m_qid = qid;
            oneGuiBannerControl1.ConnectionDefinition = aConnectionDefinition;

            sessionStatsIGrid.AddButtonControlToParent(DockStyle.Bottom);
            sessionStatsIGrid.AutoResizeCols = true;

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered += InvokeHandleError;
            this.Disposed += new EventHandler(WMSViewSessionStats_Disposed);
            this.timer1.Tick += new EventHandler(timer1_Tick);

            queryIdTextBox.Text = m_qid;

            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SessionStats;
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.Start();

            startTimer();
            setupProgressBar();
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

        void m_dbDataProvider_OnErrorEncountered(object sender, DataProviderEventArgs e)
        {
            stopTimer();
            MessageBox.Show("Error: " + e.Exception.Message, "", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            stopTimer();
            if (!progressBar1.IsDisposed)
            {
                progressBar1.Visible = false;
                DataTable dataTable = m_dbDataProvider.DatabaseDataTable;
                if (dataTable.Rows.Count <= 1)
                {
                    populateSingleRowSessionStats(dataTable);
                }
                else
                {
                    populateSessionStats(dataTable);
                }
            }
        }

        void WMSViewSessionStats_Disposed(object sender, EventArgs e)
        {
            stopTimer();
            m_dbDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered -= InvokeHandleError;
            m_dbDataProvider.Stop();
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

        private void populateSingleRowSessionStats(DataTable dataTable)
        {

            if (dataTable.Columns.Contains("CLUSTER_ID"))
            {
                dataTable.Columns.Remove("CLUSTER_ID");
            }
            if (dataTable.Columns.Contains("DOMAIN_ID"))
            {
                dataTable.Columns.Remove("DOMAIN_ID");
            }
            if (dataTable.Columns.Contains("SUBDOMAIN_ID"))
            {
                dataTable.Columns.Remove("SUBDOMAIN_ID");
            }
            if (dataTable.Columns.Contains("INSTANCE_ID"))
            {
                dataTable.Columns.Remove("INSTANCE_ID");
            }
            if (dataTable.Columns.Contains("TENANT_ID"))
            {
                dataTable.Columns.Remove("TENANT_ID");
            }
            DataTable sessionDataTable = new DataTable();
            sessionDataTable.Columns.Add("Name");
            sessionDataTable.Columns.Add("Value");

            if (dataTable.Rows.Count > 0)
            {
                foreach (DataColumn dataColumn in dataTable.Columns)
                {
                    sessionDataTable.Rows.Add(new object[] { dataColumn.ColumnName, dataTable.Rows[0][dataColumn] });
                }
            }
            sessionStatsIGrid.FillWithData(sessionDataTable);
        }

        private void populateSessionStats(DataTable dataTable)
        {
            try
            {
                WMSUtils.renameColumnNames(ref dataTable);
                sessionStatsIGrid.FillWithData(dataTable);
                for (int i = 0; i < sessionStatsIGrid.Cols.Count; i++)
                {
                    sessionStatsIGrid.Cols[i].AutoWidth();
                    sessionStatsIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
                for (int i = 0; i < sessionStatsIGrid.Rows.Count; i++)
                {
                    sessionStatsIGrid.Rows[i].AutoHeight();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message, "", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void closeButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.ViewSessionStatistics);
        }

    }
}
