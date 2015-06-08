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
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using System.Data;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSQueryPlanOrTextUserControl : UserControl
    {
        #region Fields

        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_sqlText = null;
        private string m_title = null;
        private bool m_getSqlPlan = true;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private GenericUniversalWidget m_widget = null;
        private WMSQueryPlanOrTextDisplayControl m_queryPlanOrTextDisplayControl = null;

        #endregion Fields

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="qid"></param>
        /// <param name="start_ts"></param>
        /// <param name="sqlText"></param>
        public WMSQueryPlanOrTextUserControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts, string sqlText, bool aToGetPlan)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            m_start_ts = start_ts;
            m_sqlText = sqlText;
            m_getSqlPlan = aToGetPlan;

            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Live View";

            _theQueryIdTextBox.Text = qid;

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "dummy query";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.FetchRepositoryDataOption = 
                (m_getSqlPlan) ? WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLPlan : WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SQLText;
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.START_TS = start_ts;

            UniversalWidgetConfig widgetConfig = new UniversalWidgetConfig();
            widgetConfig.DataProvider = m_dbDataProvider;
            widgetConfig.ShowToolBar = false;
            widgetConfig.ShowProviderToolBarButton = false;

            m_widget = new GenericUniversalWidget();
            m_widget.UniversalWidgetConfiguration = widgetConfig;

            m_queryPlanOrTextDisplayControl = new WMSQueryPlanOrTextDisplayControl(m_qid);
            m_widget.DataDisplayControl = m_queryPlanOrTextDisplayControl;
            m_widget.DataProvider = m_dbDataProvider; // needs to do it again for output display to be correct.
            m_widget.Dock = DockStyle.Fill;

            // place the widget in the host container
            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(m_widget);

            // Registers for events
            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

            // Start data provider
            m_dbDataProvider.Start();

            //// Mannually, control the widget didsplay
            //m_widget.StatusLabel.Text = "Fetching Data";
            //m_widget.StatusProgressBar.Visible = true;
        }

        #region Public methods

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// My disposing
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void MyDispose(bool disposing)
        {
            if (m_dbDataProvider != null)
            {
                m_dbDataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                m_dbDataProvider.Stop();
                m_dbDataProvider = null;
            }
        }

        /// <summary>
        /// New data arrived event invoker
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="e"></param>
        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            if (IsHandleCreated)
            {
                Invoke(new HandleEvents(m_dbDataProvider_OnNewDataArrived), new object[] { obj, (DataProviderEventArgs)e });
            }
        }

        /// <summary>
        /// New data arrived event handler
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            //m_widget.StatusLabel.Text = "Successful           ";
            //m_widget.StatusProgressBar.Visible = false;
            if (m_getSqlPlan)
            {
                m_queryPlanOrTextDisplayControl.LoadSQLPlan(m_dbDataProvider.DatabaseDataTable);
            }
            else
            {
                if (m_dbDataProvider.QueryText != null && m_dbDataProvider.QueryText.Length > 0)
                {
                    m_queryPlanOrTextDisplayControl.LoadSQLText(m_dbDataProvider.QueryText);
                }
                else
                {
                    m_queryPlanOrTextDisplayControl.LoadSQLText(null);
                }
            }
        }

        #endregion Private methods

        #region Class WMSQueryPlanDisplayControl
    }

    public class WMSQueryPlanOrTextDisplayControl : GenericDataDisplayControl
    {

        #region Fields

        private SqlStatementTextBox m_queryPlanTextBox = null;
        private string m_qid = null;

        #endregion Fields

        #region Constructors

        public WMSQueryPlanOrTextDisplayControl(string aQueryID)
        {
            // Sets up the textbox for query plan
            m_qid = aQueryID;
            m_queryPlanTextBox = new SqlStatementTextBox();
            m_queryPlanTextBox.WordWrap = true;
            m_queryPlanTextBox.ReadOnly = true;
            m_queryPlanTextBox.Text = "";
            m_queryPlanTextBox.AppendText("-- QUERY_ID: " + m_qid + Environment.NewLine);
            m_queryPlanTextBox.Dock = DockStyle.Fill;

            // Put it into the base container
            this.TheControl = m_queryPlanTextBox;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To load the fetched query plan to the container
        /// </summary>
        /// <param name="aDataTable"></param>
        public void LoadSQLPlan(DataTable aDataTable)
        {
            if (aDataTable == null)
            {
                return;
            }

            // Find the plan
            bool found = false;
            string planInfo = "";
            if (aDataTable != null)
            {
                foreach (DataRow r in aDataTable.Rows)
                {
                    object[] cols = r.ItemArray;
                    planInfo = (string)cols[0];
                    found = true;
                    break;
                }
            }

            m_queryPlanTextBox.AppendText(Environment.NewLine);

            // Load the plan to the textbox
            if (found)
            {
                m_queryPlanTextBox.AppendText(planInfo + Environment.NewLine);
            }
            else
            {
                m_queryPlanTextBox.AppendText(Environment.NewLine + "SQL plan not available for the selected query" + Environment.NewLine);
            }
        }

        /// <summary>
        /// To load the fetched query text to the container
        /// </summary>
        /// <param name="aQueryText"></param>
        public void LoadSQLText(string aQueryText)
        {
            m_queryPlanTextBox.AppendText(Environment.NewLine);

            if (!string.IsNullOrEmpty(aQueryText))
            {
                m_queryPlanTextBox.AppendText(aQueryText + Environment.NewLine);
            }
            else
            {
                m_queryPlanTextBox.AppendText(Environment.NewLine + "SQL Text not available for the selected query" + Environment.NewLine);
            }
        }

        #endregion Public methods

        #endregion  Class WMSQueryPlanDisplayControl
    }
}
