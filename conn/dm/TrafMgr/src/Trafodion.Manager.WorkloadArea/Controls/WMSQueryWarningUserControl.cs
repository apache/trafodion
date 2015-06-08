using System.Collections.Generic;
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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSQueryWarningUserControl : UserControl
    {
        #region Fields

        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_title = null;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private GenericUniversalWidget m_widget = null;

        #endregion Fields

        public WMSQueryWarningUserControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;

            if (parent is OffenderWorkloadCanvas)
                m_title = Properties.Resources.SystemOffender;
            else if (parent is MonitorWorkloadCanvas)
                m_title = Properties.Resources.LiveWorkloads;

            _theQueryIdTextBox.Text = m_qid;

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;

            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_Warning;
        
            m_dbDataProvider.QueryID = qid;

            UniversalWidgetConfig widgetConfig = new UniversalWidgetConfig();
            widgetConfig.DataProvider = m_dbDataProvider;
            widgetConfig.ShowProviderToolBarButton = false;
            widgetConfig.ShowTimerSetupButton = false;
            widgetConfig.ShowHelpButton = true;
            widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            widgetConfig.HelpTopic = HelpTopics.WarnInfo;

            m_widget = new GenericUniversalWidget();
            m_widget.DataProvider = m_dbDataProvider;
            m_widget.UniversalWidgetConfiguration = widgetConfig;
            m_widget.DataDisplayControl.DataDisplayHandler = new WMSQueryWarningDataHandler(this);
            //m_widget.DataProvider = m_dbDataProvider; // needs to do it again for output display to be correct.
            m_widget.Dock = DockStyle.Fill;

            // place the widget in the host container
            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(m_widget);

            // Start data provider
            m_dbDataProvider.Start();
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
            if (disposing)
            {
                if (m_dbDataProvider != null)
                {
                    m_dbDataProvider.Stop();
                    m_dbDataProvider = null;
                }
            }
        }

        #endregion Private methods
    }

    #region Class WMSQueryWarningDataHandler

    public class WMSQueryWarningDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private WMSQueryWarningUserControl m_caller = null;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public WMSQueryWarningDataHandler(WMSQueryWarningUserControl caller)
        {
            m_caller = caller;
        }

        #endregion Constructor

        #region Public methods

        /// <summary>
        /// The DoPopulate method
        /// </summary>
        /// <param name="aConfig"></param>
        /// <param name="aDataTable"></param>
        /// <param name="aDataGrid"></param>
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            try
            {
                aDataGrid.BeginUpdate();
                List<string> cols = new List<string>();
                cols.Add("QUERY_ID");
                aDataGrid.AlwaysHiddenColumnNames = cols;
                base.DoPopulate(aConfig, aDataTable, aDataGrid);
            }
            finally
            {
                if (aDataGrid.Rows.Count > 0)
                {
                    aDataGrid.ResizeGridColumns(aDataTable);
                }

                aDataGrid.EndUpdate();
            }
        }

        #endregion Public methods

        #region Private methods

        #endregion Private methods
    }

    #endregion Class WMSQueryWarningDataHandler
}
