#region Copyright info
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
#endregion Copyright info

using System;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.DatabaseArea.Queries.Controls;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSTableStatsControl : UserControl
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
        private UniversalWidgetConfig m_widgetConfig = null;
        private WMSTableStatsDisplayControl m_tableStats = null;

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="qid"></param>
        /// <param name="start_ts"></param>
        public WMSTableStatsControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            m_start_ts = start_ts;
            m_connectionDefinition = aConnectionDefinition;

            if (parent is OffenderWorkloadCanvas)
                m_title = "WMS Offender";
            else if (parent is MonitorWorkloadCanvas)
                m_title = "Live View";

            _theQueryIdTextBox.Text = qid;

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;
            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_Explain;
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.START_TS = start_ts;

            m_widgetConfig = new UniversalWidgetConfig();
            m_widgetConfig.DataProvider = m_dbDataProvider;
            m_widgetConfig.ShowProviderToolBarButton = false;
            m_widgetConfig.ShowTimerSetupButton = false;
            m_widgetConfig.ShowHelpButton = true;
            m_widgetConfig.ShowExportButtons = false;
            m_widgetConfig.HelpTopic = HelpTopics.QueryTableStats;
            m_widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            m_widget = new GenericUniversalWidget();
            m_widget.UniversalWidgetConfiguration = m_widgetConfig;

            m_tableStats = new WMSTableStatsDisplayControl();
            m_widget.DataDisplayControl = m_tableStats;
            m_widget.DataProvider = m_dbDataProvider;

            _thePanel.Controls.Clear();
            m_widget.Dock = DockStyle.Fill;
            _thePanel.Controls.Add(m_widget);

            m_dbDataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;

            m_dbDataProvider.Start();
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// My disposing
        /// </summary>
        /// <param name="disposing"></param>
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
        /// Invoker for new data arrival event
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
        /// The actual event handler for new data arrival
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void m_dbDataProvider_OnNewDataArrived(object sender, DataProviderEventArgs e)
        {
            m_tableStats.LoadQueryData(m_dbDataProvider.WorkbenchQueryData);
        }

        #endregion Private methods
    }

    #region Class WMSTableStatsDisplayControl

    public class WMSTableStatsDisplayControl : GenericDataDisplayControl
    {
        #region Fields

        private QueryPlanControl _queryPlanControl = null;

        #endregion Fields

        #region Constructors

        public WMSTableStatsDisplayControl()
        {
            _queryPlanControl = new QueryPlanControl();
            _queryPlanControl.ShowExplainPlanPanel = false;

            // Put it into the base container
            this.TheControl = _queryPlanControl;
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To load the query data directly
        /// </summary>
        /// <param name="wqd"></param>
        public void LoadQueryData(NCCWorkbenchQueryData wqd)
        {
            if (null == wqd)
            {
                return;
            }

            _queryPlanControl.LoadQueryData(wqd);
            _queryPlanControl.ShowExplainPlanPanel = false;
        }

        #endregion Public methods
    }

    #endregion Class WMSTableStatsDisplayControl
}
