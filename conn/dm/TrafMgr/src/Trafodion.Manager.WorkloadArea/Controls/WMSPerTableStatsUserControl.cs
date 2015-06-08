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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using Trafodion.Manager.Framework.Controls;
using System.Collections.Generic;
using TenTec.Windows.iGridLib;
using System.Drawing;
using Trafodion.Manager.Framework;
using System.Text.RegularExpressions;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WMSPerTableStatsControl : UserControl
    {
        #region Fields

        private WMSWorkloadCanvas m_parent = null;
        private string m_qid = null;
        private string m_start_ts = null;
        private string m_sqlText = null;
        private ConnectionDefinition m_connectionDefinition = null;
        private string m_title = null;
        private WMSOffenderDatabaseDataProvider m_dbDataProvider = null;
        private GenericUniversalWidget m_widget = null;
        private UniversalWidgetConfig m_widgetConfig = null;
        private ToolStripLabel m_toolStripLabel = null;
        private ToolStripButton _generateMaintainScriptIcon = null;
        private readonly Regex _regexBracket = new Regex(@"\(.*\)");

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: ConnectionDefinition 
        /// </summary>
        public ConnectionDefinition ConnectionDefinition
        {
            get { return m_connectionDefinition; }
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

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="parent"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="qid"></param>
        /// <param name="start_ts"></param>
        public WMSPerTableStatsControl(WMSWorkloadCanvas parent, ConnectionDefinition aConnectionDefinition, string qid, string start_ts)
        {
            InitializeComponent();
            m_parent = parent;
            m_qid = qid;
            m_start_ts = start_ts;
            m_connectionDefinition = aConnectionDefinition;

            if (parent is OffenderWorkloadCanvas)
                m_title = Properties.Resources.SystemOffender;
            else if (parent is MonitorWorkloadCanvas)
                m_title = Properties.Resources.LiveWorkloads;

            _theQueryIdTextBox.Text = qid;

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = "";
            dbConfig.ConnectionDefinition = aConnectionDefinition;
            m_dbDataProvider = new WMSOffenderDatabaseDataProvider(dbConfig);
            m_dbDataProvider.FetchRepositoryDataOption = WMSOffenderDatabaseDataProvider.FetchDataOption.Option_PerTableStats;
            m_dbDataProvider.QueryID = qid;
            m_dbDataProvider.START_TS = start_ts;
            m_widgetConfig = new UniversalWidgetConfig();
            m_widgetConfig.DataProvider = m_dbDataProvider;
            m_widgetConfig.ShowProviderToolBarButton = false;
            m_widgetConfig.ShowTimerSetupButton = false;
            m_widgetConfig.ShowHelpButton = true;
            m_widgetConfig.HelpTopic = HelpTopics.QueryTableStats;
            m_widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            m_widget = new GenericUniversalWidget();
            m_widget.DataProvider = m_dbDataProvider;
            m_widget.UniversalWidgetConfiguration = m_widgetConfig;
            m_widget.DataDisplayControl.DataDisplayHandler = new WMSPerTableStatsDataHandler(this);
            m_widget.Dock = DockStyle.Fill;



            _generateMaintainScriptIcon = new ToolStripButton();
            _generateMaintainScriptIcon.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            _generateMaintainScriptIcon.Image = Properties.Resources.MaintainScript;
            _generateMaintainScriptIcon.ImageTransparentColor = System.Drawing.Color.Magenta;
            _generateMaintainScriptIcon.Name = "monitorOptionsButton";
            _generateMaintainScriptIcon.Size = new System.Drawing.Size(23, 22);
            _generateMaintainScriptIcon.Text = "Generate Maintain Script";
            _generateMaintainScriptIcon.Enabled = false;
            _generateMaintainScriptIcon.Click += new EventHandler(GenerateMaintainScriptMenuItem_Click);
            m_widget.AddToolStripItem(_generateMaintainScriptIcon);

            m_dbDataProvider.OnFetchingData += (sender, e) =>
            {
                if (this.IsHandleCreated)
                {
                    this.Invoke(new MethodInvoker(() => _generateMaintainScriptIcon.Enabled = false));
                }
            };

            m_dbDataProvider.OnNewDataArrived += (sender, e) =>
            {
                if (this.IsHandleCreated)
                {
                    this.Invoke(new MethodInvoker(() => _generateMaintainScriptIcon.Enabled = true));
                }
            };

            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(m_widget);

            AddToolStripButtons();

            m_dbDataProvider.Start();
        }

        #endregion Constructors

        #region Public methods

        #endregion Public methods
        
        private List<string> ExtractTables()
        {
            List<string> tableNames = new List<string>();
            iGrid grid = ((TabularDataDisplayControl)m_widget.DataDisplayControl).DataGrid;

            foreach (iGRow row in grid.Rows)
            {
                String tableName = (string)row.Cells["TABLE_NAME"].Value;
                if (tableName != null && tableName.Trim().Length > 0)
                {
                    tableName = this._regexBracket.Replace(tableName, string.Empty).Trim().ToUpper();
                    if (!tableNames.Contains(tableName))
                    {
                        tableNames.Add(tableName);
                    }
                }
            }
            return tableNames;
        }

        void GenerateMaintainScriptMenuItem_Click(object sender, EventArgs events)
        {
            string windowTitle = string.Format(Properties.Resources.TitleMaintainScript, this.m_qid);
            List<string> tableNames = ExtractTables();
            if (tableNames != null && tableNames.Count > 0)
            {
                if (!Utilities.BringExistingWindowToFront(windowTitle, this.ConnectionDefinition))
                {
                    MaintainScript scriptUserControl = new MaintainScript(this.m_qid, tableNames, this.ConnectionDefinition);
                    Utilities.LaunchManagedWindow(windowTitle, scriptUserControl, this.ConnectionDefinition, new Size(750, 550), true);
                }
            }
            else
            {
                MessageBox.Show(this.ParentForm, Properties.Resources.MaintainScript_TableNotFound, Properties.Resources.MaintainScript_TableNotFoundCaption, MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        #region Private methods

        /// <summary>
        /// My disposing
        /// </summary>
        /// <param name="disposing"></param>
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

        /// <summary>
        /// Add the warning label to report warning when per-table stats is no longer available
        /// </summary>
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

    #region Class WMSTableStatsDataHandler

    public class WMSPerTableStatsDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private WMSPerTableStatsControl m_caller = null;
        private DataTable _lastData = null;
        private Dictionary<int, List<string>> _changedData = null;
        private MonitorWorkloadOptions _workloadOptions;
        private bool _isGridRowAddedEventRegistered = false;
        private const string LAST_UPDATED = "LAST_UPDATED";
        private const string QUERY_ID = "QUERY_ID";
        private iGCellStyle igridCellStyleForInt = null;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public WMSPerTableStatsDataHandler(WMSPerTableStatsControl caller)
        {
            m_caller = caller; 
            _workloadOptions = MonitorWorkloadOptions.GetOptions();

            InitializeIGridCellStyle();
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
            DataTable newDataTable = new DataTable();
            string lastUpdatedColName = string.IsNullOrEmpty(m_caller.ConnectionDefinition.ServerTimeZoneName) ? LAST_UPDATED :
                            string.Format("{0}_({1})", LAST_UPDATED, m_caller.ConnectionDefinition.ServerTimeZoneName);

            try
            {
                foreach (DataColumn dc in aDataTable.Columns)
                {
                    if (dc.ColumnName.Equals(QUERY_ID))
                    {
                        // skip the query id
                    }
                    else if (dc.ColumnName.Equals(LAST_UPDATED))
                        newDataTable.Columns.Add(lastUpdatedColName, System.Type.GetType("System.String"));
                    else
                        newDataTable.Columns.Add(dc.ColumnName, dc.DataType);
                }

                foreach (DataRow r in aDataTable.Rows)
                {
                    DataRow newDR = newDataTable.NewRow();
                    for (int i = 0; i < aDataTable.Columns.Count; i++)
                    {
                        try
                        {
                            string colName = aDataTable.Columns[i].ToString();
                            if (colName.Equals(QUERY_ID))
                            {
                                // skip query id
                            }
                            else if (colName.Equals(LAST_UPDATED))
                                newDR[lastUpdatedColName] = WMSUtils.convertJulianTimeStampLCT(r[LAST_UPDATED]);
                            else
                                newDR[colName] = r[colName];
                        }
                        catch (Exception ex)
                        {
                            newDR[i] = r[i];
                        }
                    }
                    newDataTable.Rows.Add(newDR);
                }


                ExtractChangedData(this._lastData, newDataTable);       // Extract changed data and record it
                RegisterRowAddedEvent(aDataGrid);                       // Register RowsAdded event for grid
                
                base.DoPopulate(aConfig, newDataTable, aDataGrid);
                StyleIGrid(aDataGrid);

            }
            finally
            {
                UnregisterRowAddedEvent(aDataGrid);                     // Unregister RowsAdded event for grid

                if (aDataGrid.Rows.Count > 0)
                {
                    aDataGrid.ResizeGridColumns(newDataTable);
                }

                if (aDataTable.Rows.Count == 0)
                {
                    m_caller.WarningMessage = Properties.Resources.WarningPerTableStatsNotAvail;
                }
                else
                {
                    m_caller.WarningMessage = null;
                }
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Highlight changed grid cells accorrding to data changes from comparison result
        /// </summary>
        /// <param name="sender">Grid</param>
        /// <param name="e"></param>
        private void DataGrid_RowsAdded(object sender, iGRowsAddedEventArgs e)
        {
            List<string> changedColumns;
            this._changedData.TryGetValue(e.RowIndex, out changedColumns);

            if (changedColumns != null)
            {
                iGRow currentGridRow = ((iGrid)sender).Rows[e.RowIndex];
                foreach (string columnName in changedColumns)
                {
                    currentGridRow.Cells[columnName].ForeColor = _workloadOptions.HighLightChangesColor;
                }
            }
        }

        /// <summary>
        /// Extract changed data by comparing last data table and new data table.       
        /// It's based on the assumption that the columns&rows of last data and new data are the same, 
        /// and only values maybe changed 
        /// </summary>
        /// <param name="lastData"></param>
        /// <param name="newData"></param>
        private void ExtractChangedData(DataTable lastData, DataTable newData)
        {
            // Record the data table for comparing of next time
            this._lastData = newData; 
            // Reset changed data
            this._changedData = null;

            // If Hightlight option is unchecked, no need to compare & extract
            if (this._workloadOptions == null || !this._workloadOptions.HighLightChanges) return;

            // If either last data or new data is empty, no need to compare & extract
            if (lastData == null || lastData.Rows == null || lastData.Rows.Count == 0
                || newData == null || newData.Rows == null || newData.Rows.Count == 0) return;
            
            this._changedData = new Dictionary<int, List<string>>();
            for (int rowIndex = 0; rowIndex <= newData.Rows.Count - 1; rowIndex++)
            {
                foreach (DataColumn column in newData.Columns)
                {
                    DataRow rowNew = newData.Rows[rowIndex];
                    DataRow rowLast = lastData.Rows[rowIndex];
                    string columnName = column.ColumnName;
                    if (Convert.ToString(rowNew[columnName]) != Convert.ToString(rowLast[columnName]))
                    {
                        List<string> changedColumns;
                        this._changedData.TryGetValue(rowIndex, out changedColumns);
                        if (changedColumns == null)
                        {
                            changedColumns = new List<string>();
                            this._changedData.Add(rowIndex, changedColumns);
                        }

                        // Add changed column name
                        string lastValue = Convert.ToString(rowNew[columnName]);
                        changedColumns.Add(columnName);
                    }
                }
            }
        }

        /// <summary>
        ///  Register RowsAdded event of Grid to highlight grid cells whose data has been changes comparing to last time.
        ///   Because there's no primary key in the fetched data table, 
        ///   once grid rows are sorted after the data table's rows has been populated to grid,
        ///   it's impossible to find the row mapping between data table's rows and grid's rows.
        ///   
        ///   DataGrid_RowsAdded will be fired 
        ///   
        ///   after this:      
        ///                // Populate data table's rows into grid without sorting
        ///                // After this method is implemented, the order of grid row will be same as data table row
        ///                TabularDataDisplayControl.PopulateGrid(aConfig, aDataTable, aDataGrid); 
        ///                
        ///   before this:
        ///                // Apply sort/filter and so on to grid
        ///                TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);
        ///                
        ///   Above tow methods are called one by one by method of TabularDataDisplayHandler:
        ///                public virtual void DoPopulate(UniversalWidgetConfig aConfig, DataTable aDataTable, TrafodionIGrid aDataGrid) 
        /// </summary>
        private void RegisterRowAddedEvent(iGrid grid)
        {
            // If there are data changes
            if (this._changedData != null && this._changedData.Count > 0)                     
            {
                grid.RowsAdded += new iGRowsAddedEventHandler(DataGrid_RowsAdded);
                this._isGridRowAddedEventRegistered = true;
            }
            else
            {
                this._isGridRowAddedEventRegistered = false;
            }
        }

        /// <summary>
        /// Unregister RowAdded Event for grid, comparing to RegisterRowAddedEvent
        /// </summary>
        /// <param name="grid"></param>
        private void UnregisterRowAddedEvent(iGrid grid)
        {
            if (this._isGridRowAddedEventRegistered)
            {
                grid.RowsAdded -= new iGRowsAddedEventHandler(DataGrid_RowsAdded);
                this._isGridRowAddedEventRegistered = false;
            }
        }

        private void InitializeIGridCellStyle() 
        {
            igridCellStyleForInt = new iGCellStyle();
            igridCellStyleForInt.FormatString = "{0:N0}";
        }

        private void StyleIGrid(iGrid igrid) 
        {
            if (igrid == null || igrid.Cols.Count == 0)
                return;
            igrid.Cols["EST_ACCESSED_ROWS"].CellStyle = igridCellStyleForInt;
            igrid.Cols["EST_USED_ROWS"].CellStyle = igridCellStyleForInt;
            igrid.Cols["ACCESSED_ROWS"].CellStyle = igridCellStyleForInt;
            igrid.Cols["USED_ROWS"].CellStyle = igridCellStyleForInt;
            igrid.Cols["MESSAGE_COUNT"].CellStyle = igridCellStyleForInt;
            igrid.Cols["MESSAGE_BYTES"].CellStyle = igridCellStyleForInt;
            igrid.Cols["PROCESS_BUSYTIME"].CellStyle = igridCellStyleForInt;
            igrid.Cols["OPEN_TIME"].CellStyle = igridCellStyleForInt;
        
        }


        #endregion Private methods
    }

    #endregion Class WMSQueryWarningDataHandler
}
