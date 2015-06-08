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
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.MetricMiner.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSQueryPlan : UserControl
    {
        #region Fields

        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_sqlText = null;
        private ConnectionDefinition m_connectionDefinition = null;
        private string m_title = null;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private GenericUniversalWidget m_widget = null;
        private TrafodionRichTextBox m_queryPlanTextBox = null;
        private QueryPlanDataDisplayControl m_queryPlanDataDisplayControl = null;
        private UniversalWidgetConfig m_widgetConfig = null;


        #endregion Fields

        public WMSQueryPlan(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts, string sqlText)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            m_start_ts = start_ts;
            m_sqlText = sqlText;
            m_connectionDefinition = aConnectionDefinition;

            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Live View";

            GridLayoutManager gridLayoutManager = new GridLayoutManager(1, 1);
            gridLayoutManager.CellSpacing = 4;
            this._theCanvas.LayoutManager = gridLayoutManager;

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
            //m_dbDataProvider.OnErrorEncountered += InvokeHandleError;
            this.Disposed += new EventHandler(WMSQueryPlan_Disposed);
            //this.timer1.Tick += new EventHandler(timer1_Tick);

            m_queryPlanTextBox = new TrafodionRichTextBox();
            m_queryPlanTextBox.Text = "";
            m_queryPlanTextBox.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            m_queryPlanTextBox.AppendText("-- INFO: Fetching Data");
            m_queryPlanTextBox.Dock = DockStyle.Fill;

            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLPlan;
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.EXEC_START_UTC_TS = start_ts;

            m_widgetConfig = new UniversalWidgetConfig();
            m_widgetConfig.DataProvider = m_dbDataProvider;
            m_widgetConfig.ShowToolBar = false;
            m_widgetConfig.ShowProviderToolBarButton = false;
            m_widget = new GenericUniversalWidget();
            m_widget.UniversalWidgetConfiguration = m_widgetConfig;
            m_widget.Controls.Clear();
            m_widget.Controls.Add(m_queryPlanTextBox);
            m_widget.Dock = DockStyle.Fill;
            this.Controls.Add(m_widget);

            //Add the widget to the canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer container1 = _theCanvas.AddWidget(m_widget, "Query Plan from WMS", "Query Plan from WMS", gridConstraint, -1);

            m_dbDataProvider.Start();
        }




        #region Public methods

        #endregion Public methods

        #region Private methods

        private void WMSQueryPlan_Disposed(object sender, EventArgs e)
        {
            m_dbDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
            //m_dbDataProvider.OnErrorEncountered -= InvokeHandleError;
            m_dbDataProvider.Stop();
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
            MessageBox.Show("Error: " + e.Exception.Message, m_title, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            getSQLPlan();
        }

        private void getSQLText()
        {
            this.Text = "SQL text - " + m_qid;
            m_queryPlanTextBox.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            if (m_dbDataProvider.QueryText != null && m_dbDataProvider.QueryText.Length > 0)
            {
                m_queryPlanTextBox.AppendText("-- INFO: " + m_dbDataProvider.ViewName + Environment.NewLine);
                m_queryPlanTextBox.AppendText(Environment.NewLine + m_dbDataProvider.QueryText + Environment.NewLine);
            }
            else
            {
                m_queryPlanTextBox.AppendText("-- INFO: PREVIEW" + Environment.NewLine);
                m_queryPlanTextBox.AppendText(Environment.NewLine + m_sqlText + Environment.NewLine);
            }
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
            m_queryPlanTextBox.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            if (found)
            {
                m_queryPlanTextBox.AppendText(planInfo + Environment.NewLine);
            }
            else
            {
                m_queryPlanTextBox.AppendText(Environment.NewLine + "SQL plan not available for the selected query" + Environment.NewLine);
            }
            this.Show();
        }

        private void helpButton_Click(object sender, EventArgs e)
        {
            //if (m_isSqlText)
            //{
            //    TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.SQLText);
            //}
            //else
            //{
            //    TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.PlanText);
            //}
        }
        #endregion Private methods
    }
}
