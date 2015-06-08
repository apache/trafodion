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
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSRepositoryInfoUserControl : UserControl
    {
        #region Fields

        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_title = null;
        private string m_stats_type = null;   // either Query stats or session stats
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private GenericUniversalWidget m_widget = null;
        private ToolStripLabel m_toolStripLabel = null;


        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: StartTime - the start time
        /// </summary>
        public string StartTime
        {
            get { return m_start_ts; }
        }

        /// <summary>
        /// Property: Warning message
        /// </summary>
        public string WarningMessage
        {
            set
            {
                if (string.IsNullOrEmpty(value))
                {
                    if (m_toolStripLabel != null)
                    {
                        m_toolStripLabel.Visible = false;
                        m_toolStripLabel.Text = "";
                    }
                }
                else
                {
                    if (m_toolStripLabel != null)
                    {
                        m_toolStripLabel.Visible = true;
                        m_toolStripLabel.Text = value;
                    }
                }
            }
        }

        #endregion Properties

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="qid"></param>
        /// <param name="start_ts"></param>
        /// <param name="sqlText"></param>
        public WMSRepositoryInfoUserControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts, string stats_type)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            m_start_ts = start_ts;
            m_stats_type = stats_type;

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
            m_dbDataProvider.FetchRepositoryDataOption = m_stats_type.Equals(Properties.Resources.QueryStatistics) ? 
                WMSOffenderDatabaseDataProvider.FetchDataOption.Option_QueryStats : WMSOffenderDatabaseDataProvider.FetchDataOption.Option_SessionStats;
        
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.START_TS = m_start_ts;

            UniversalWidgetConfig widgetConfig = new UniversalWidgetConfig();
            widgetConfig.DataProvider = m_dbDataProvider;
            widgetConfig.ShowProviderToolBarButton = false;
            widgetConfig.ShowTimerSetupButton = false;
            widgetConfig.ShowHelpButton = true;
            widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            widgetConfig.HelpTopic =
                m_stats_type.Equals(Properties.Resources.QueryStatistics) ? HelpTopics.RepositoryInfo : HelpTopics.ViewSessionStatistics;

            m_widget = new GenericUniversalWidget();
            m_widget.DataProvider = m_dbDataProvider;
            m_widget.UniversalWidgetConfiguration = widgetConfig;
            m_widget.DataDisplayControl.DataDisplayHandler = new RepositoryInfoDataHandler(this);
            //m_widget.DataProvider = m_dbDataProvider; // needs to do it again for output display to be correct.
            m_widget.Dock = DockStyle.Fill;

            // place the widget in the host container
            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(m_widget);

            AddToolStripButtons();

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


        private void AddToolStripButtons()
        {
            m_toolStripLabel = new ToolStripLabel();
            m_toolStripLabel.Name = "WarningLabel";
            m_toolStripLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
            m_toolStripLabel.ForeColor = System.Drawing.Color.Red;
            m_toolStripLabel.Text = "";
            m_toolStripLabel.Visible = false;
            m_widget.AddToolStripItem(m_toolStripLabel);
        }

        #endregion Private methods
    }

    #region Class RepositoryInfoDataHandler

    public class RepositoryInfoDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private WMSRepositoryInfoUserControl m_caller = null;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public RepositoryInfoDataHandler(WMSRepositoryInfoUserControl caller)
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
                DataRow[] dataRows;
                DataRow dataRow;

                if (aDataTable.Rows.Count == 0)
                {
                    m_caller.WarningMessage = Properties.Resources.WarningReposNotAvail; 
                }
                else
                {
                    m_caller.WarningMessage = null;
                    DataTable table = new DataTable();
                    dataRow = aDataTable.Rows[0];
                    WMSUtils.renameColumnNamesSpace(ref aDataTable);

                    table.Columns.Add("Name", typeof(string));
                    table.Columns.Add("Value", typeof(string));

                    for (int i = 0; i < aDataTable.Columns.Count; i++)
                    {
                        if (aDataTable.Columns[i].DataType == typeof(System.DateTime) ||
                            aDataTable.Columns[i].DataType == typeof(System.TimeSpan))
                        {
                            //string time = String.Format("{0:yyyy-MM-dd HH:mm:ss.ffffff}", dataRow[i]);                            
                            table.Rows.Add(new object[] { aDataTable.Columns[i].ColumnName, 
                                dataRow[i] is DateTime?
                                Trafodion.Manager.Framework.Utilities.GetTrafodionSQLLongDateTime((DateTime)dataRow[i], false):string.Empty });
                        }
                        else
                        {
                            table.Rows.Add(new object[] { aDataTable.Columns[i].ColumnName, dataRow[i] });
                        }
                    }
                    base.DoPopulate(aConfig, table, aDataGrid);
                }
            }
            finally
            {
                aDataGrid.EndUpdate();
            }
        }

        #endregion Public methods

        #region Private methods

        #endregion Private methods
    }

    #endregion Class RepositoryInfoDataHandler
}
