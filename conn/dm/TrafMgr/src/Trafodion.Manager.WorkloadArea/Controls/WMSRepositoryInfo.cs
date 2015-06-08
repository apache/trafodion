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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSRepositoryInfo : Trafodion.Manager.Framework.Controls.TrafodionForm
    {
        #region Constants
        private const int IGRIDINFO_MIN_WIDTH = 50;
        #endregion

        #region Members
        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private string m_title = "";
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        #endregion

        public WMSRepositoryInfo(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            oneGuiBannerControl1.ConnectionDefinition = aConnectionDefinition;

            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Monitor Workload";

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            m_dbDataProvider.OnErrorEncountered += InvokeHandleError;
            this.Disposed += new EventHandler(WMSRepositoryInfo_Disposed);
            this.timer1.Tick += new EventHandler(timer1_Tick);

            queryIdTextBox.Text = m_qid;

            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_QueryStats;
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

        void WMSRepositoryInfo_Disposed(object sender, EventArgs e)
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
                DataTable dataTable = m_dbDataProvider.DatabaseDataTable;
                if (dataTable.Rows.Count == 1)
                {
                    populateSingleRowRepInfo(dataTable);
                }
                else
                {
                    populateRepInfo(dataTable);
                }
            }
        }

        private void populateSingleRowRepInfo(DataTable dataTable)
        {
            repositoryInfoIGrid.Rows.Clear();
            repositoryInfoIGrid.Cols.Clear();
            iGColPattern intColPattern = new iGColPattern();
            intColPattern.CellStyle.ValueType = System.Type.GetType("System.Int16");
            intColPattern.CellStyle.TextAlign = iGContentAlignment.MiddleRight;
            iGColPattern stringColPattern = new iGColPattern();
            stringColPattern.CellStyle.ValueType = System.Type.GetType("System.String");
            stringColPattern.CellStyle.TextAlign = iGContentAlignment.MiddleLeft;
            repositoryInfoIGrid.Cols.Add("COLUMN NUMBER", 100, intColPattern);
            repositoryInfoIGrid.Cols.Add("COLUMN NAME", 200, stringColPattern);
            repositoryInfoIGrid.Cols.Add("COLUMN VALUE", 400, stringColPattern);
            try
            {
                WMSUtils.renameColumnNamesSpace(ref dataTable);
                //for (int r = 0; r < dataTable.Rows.Count; r++)
                {
                    for (int c = 0; c < dataTable.Columns.Count; c++)
                    {
                        repositoryInfoIGrid.Rows.Add();
                        repositoryInfoIGrid.Cells[c, 0].Value = c + 1;
                        repositoryInfoIGrid.Cells[c, 1].Value = dataTable.Columns[c].ColumnName;
                        repositoryInfoIGrid.Cells[c, 2].Value = dataTable.Rows[0][c];
                    }
                    //break;
                }
                for (int i = 0; i < repositoryInfoIGrid.Cols.Count; i++)
                {
                    repositoryInfoIGrid.Cols[i].AutoWidth();
                    repositoryInfoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
                for (int i = 0; i < repositoryInfoIGrid.Rows.Count; i++)
                {
                    repositoryInfoIGrid.Rows[i].AutoHeight();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void populateRepInfo(DataTable dataTable)
        {
            try 
            {
                WMSUtils.renameColumnNames(ref dataTable);
                repositoryInfoIGrid.FillWithData(dataTable);
                for (int i = 0; i < repositoryInfoIGrid.Cols.Count; i++)
                {
                    repositoryInfoIGrid.Cols[i].AutoWidth();
                    repositoryInfoIGrid.Cols[i].MinWidth = IGRIDINFO_MIN_WIDTH;
                }
                for (int i = 0; i < repositoryInfoIGrid.Rows.Count; i++)
                {
                    repositoryInfoIGrid.Rows[i].AutoHeight();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show("Exception: " + ex.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void closeButton_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.RepositoryInfo);
        }
    }
}
