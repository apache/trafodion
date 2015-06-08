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
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSChildrenProcessesUserControl : UserControl
    {
        #region Fields

        private ConnectionDefinition m_connectionDefinition = null;
        private string m_title = "";
        private WMSWorkloadCanvas m_parent = null;
        private string m_query_id = null;
        private string m_process_name = null;

        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private UniversalWidgetConfig m_widgetConfig = null;
        private GenericUniversalWidget m_widget = null;

        private TrafodionIGridToolStripMenuItem m_processDetailMenuItem = null;
        private TrafodionIGridToolStripMenuItem m_pstateChildMenuItem = null;

        #endregion Fields

        #region Properties

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="query_id"></param>
        /// <param name="process_name"></param>
        public WMSChildrenProcessesUserControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string query_id, string process_name)
        {
            InitializeComponent();
            m_parent = parent;
            m_connectionDefinition = aConnectionDefinition;
            m_query_id = query_id;
            m_process_name = process_name;
            if (parent is OffenderWorkloadCanvas)
                m_title = Properties.Resources.SystemOffender;
            else if (parent is MonitorWorkloadCanvas)
                m_title = Properties.Resources.LiveWorkloads;

            // Set up the Process name/id display
            ID = m_process_name; 

            ShowWidgets();
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Property: ID - of the parent process, it could be process name or process id
        /// </summary>
        public string ID
        {
            get { return _theIDTextBox.Text; }
            set { _theIDTextBox.Text = value; }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To create the widget and start the data provider
        /// </summary>
        private void ShowWidgets()
        {
            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = m_connectionDefinition;
            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_ChildrenProcesses;
            m_dbDataProvider.QueryID = m_query_id;
            m_dbDataProvider.ProcessName = m_process_name;

            m_widgetConfig = new UniversalWidgetConfig();
            m_widgetConfig.DataProvider = m_dbDataProvider;
            m_widgetConfig.ShowProviderToolBarButton = false;
            m_widgetConfig.ShowTimerSetupButton = false;
            m_widgetConfig.ShowHelpButton = true;
            m_widgetConfig.ShowExportButtons = false;
            m_widgetConfig.HelpTopic = HelpTopics.ChildrenProcesses;
            m_widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            m_widget = new GenericUniversalWidget();
            m_widget.DataProvider = m_dbDataProvider;
            ((TabularDataDisplayControl)m_widget.DataDisplayControl).LineCountFormat = Properties.Resources.InitialChildrenCount;
            //((TabularDataDisplayControl)m_widget.DataDisplayControl).DataGrid.AutoResizeCols = true;

            m_widget.UniversalWidgetConfiguration = m_widgetConfig;
            m_widget.DataDisplayControl.DataDisplayHandler = new ChildrenProcessesDataHandler(this);
            m_widget.Dock = DockStyle.Fill;

            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(m_widget);

            this.Disposed += WMSChildrenProcessesUserControl_Disposed;

             //Add popup menu items to the table
            TabularDataDisplayControl dataDisplayControl = m_widget.DataDisplayControl as TabularDataDisplayControl;

            if (dataDisplayControl != null)
            {
                m_processDetailMenuItem = new TrafodionIGridToolStripMenuItem();
                m_processDetailMenuItem.Text = "Process Detail...";
                m_processDetailMenuItem.Click += m_processDetailMenuItem_Click;
                dataDisplayControl.AddMenuItem(m_processDetailMenuItem);

                m_pstateChildMenuItem = new TrafodionIGridToolStripMenuItem();
                m_pstateChildMenuItem.Text = "Pstate...";
                m_pstateChildMenuItem.Click += m_pstateChildMenuItem_Click;
                dataDisplayControl.AddMenuItem(m_pstateChildMenuItem);
            }


            m_dbDataProvider.Start();

        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void WMSChildrenProcessesUserControl_Disposed(object sender, EventArgs e)
        {
            if (m_dbDataProvider != null)
            {
                m_dbDataProvider.Stop();
            }

            if (m_pstateChildMenuItem != null)
            {
                m_pstateChildMenuItem.Click -= m_pstateChildMenuItem_Click;
            }

            if (m_processDetailMenuItem != null)
            {
                m_processDetailMenuItem.Click -= m_processDetailMenuItem_Click;
            }
        }

        /// <summary>
        /// Event handler for starting up pstate info for a selected child
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void  m_pstateChildMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string prcess_name = iGrid.Cells[row, "PROCESS_NAME"].Value as string;
                    string title = string.Format(Properties.Resources.TitlePstate, prcess_name);
                    WMSPStateUserControl pstate = new WMSPStateUserControl(title, m_connectionDefinition, prcess_name, true);
                    Trafodion.Manager.Framework.Utilities.LaunchManagedWindow(title, pstate, m_connectionDefinition, this.Size, true);
                }
            }
        }

        /// <summary>
        /// Event handler for starting up process detail for a selected child
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void  m_processDetailMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string process_name = iGrid.Cells[row, "PROCESS_NAME"].Value as string;
                    string title = string.Format(Properties.Resources.TitleProcessDetails, process_name);
                    WMSProcessDetailsUserControl processDetail = new WMSProcessDetailsUserControl(title, m_connectionDefinition, process_name, true);
                    Trafodion.Manager.Framework.Utilities.LaunchManagedWindow(title, processDetail, m_connectionDefinition, this.Size, true);
                }
            }
        } 

        #endregion Private methods
    }

    #region Class ChildrenProcessesDataHandler

    /// <summary>
    /// Class for Children process data handler
    /// </summary>
    public class ChildrenProcessesDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private WMSChildrenProcessesUserControl m_caller = null;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public ChildrenProcessesDataHandler(WMSChildrenProcessesUserControl caller)
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
                base.DoPopulate(aConfig, aDataTable, aDataGrid);
            }
            finally
            {
                string gridHeaderText;
                if (aDataGrid.Rows.Count == 0)
                {
                    gridHeaderText = string.Format(Properties.Resources.ChildCount, 0);
                }
                else if (aDataGrid.Rows.Count == 1)
                {
                    gridHeaderText = string.Format(Properties.Resources.ChildCount, 1);
                }
                else
                {
                    gridHeaderText = string.Format(Properties.Resources.ChildrenCount,aDataGrid.Rows.Count);
                }

                aDataGrid.ResizeGridColumns(aDataTable);
                aDataGrid.UpdateCountControlText(gridHeaderText);
                aDataGrid.EndUpdate();
            }
        }

        #endregion Public methods

        #region Private methods

        #endregion Private methods
    }

    #endregion Class ChildrenProcessesDataHandler
}
