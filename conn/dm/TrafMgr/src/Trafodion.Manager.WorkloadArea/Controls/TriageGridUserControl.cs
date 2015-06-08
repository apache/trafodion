#region Copyright info
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
#endregion Copyright info
using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    /// <summary>
    /// The class that encapsualtes the universal widget to display the triage grid
    /// </summary>
    public partial class TriageGridUserControl : UserControl
    {
        #region Constants
        private static readonly string TriageGridConfigName = "Triage_Grid_Widget";
        public const String RUNNING = "RUNNING";
        public const String ABNORMALLY_TERMINATED = "ABNORMALLY TERMINATED";
        public const String SUSPENDED = "SUSPENDED";
        public const String HOLD = "HOLD";
        public const String COMPLETED = "COMPLETED";
        public const String ERROR = "ERROR";
        #endregion

        #region member variables
        UniversalWidgetConfig _widgetConfig = null;
        GenericUniversalWidget _widget;
        ConnectionDefinition _theConnectionDefinition;
        String _theTitle = "Traige Queries";
        ArrayList _commands = new ArrayList();
        TriageGridDataDisplayHandler _dataDisplayHandler = null;
        TriageDataProvider _dataProvider = null;
        DataTable _dataStore = null;
        ToolStrip _theToolStrip = null;
        ToolStripButton _sqlPreviewButton = new ToolStripButton();
        ToolStripButton _showControlButton = new ToolStripButton();
        ToolStripButton _showHiddenButton = new ToolStripButton();
        ToolStripButton _hideStatementsButton = new ToolStripButton();
        ToolStripButton _getSessionsButton = new ToolStripButton();
        ToolStripButton _clearButton = new ToolStripButton();
        ToolStripButton _loadWhiteboardButton = new ToolStripButton();
        ToolStripButton _clientRuleButton = new ToolStripButton();

        //Hashtable hiddenQIDs_ht = new Hashtable();
        TriageHelper _theTriageHelper = new TriageHelper();
        private iGRow _highlightedQuery = null;
        private readonly Size childrenWindowSize = new Size(800, 600);

        //TODO: This is a test only feature. This is to be refactored
        //The reference will have to be removed
        private TriageChartControl _theChartControl = null;

        private TrafodionIGridToolStripMenuItem WorkspaceContextMenuGenerateSingleScript = null;
        private TrafodionIGridToolStripMenuItem WorkspaceContextMenuClearTriageSpace = null;
        private TrafodionIGridToolStripMenuItem WorkspaceContextMenuDeleteSelectedRows = null;
        private TrafodionIGridToolStripMenuItem _displaySQLPlanMenuItem = null;
        private TrafodionIGridToolStripMenuItem _cancelSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _holdSelectedQueryMenuItem = null;
        private TrafodionIGridToolStripMenuItem _releaseSelectedQueryMenuItem = null;
        public delegate void UpdateStatus(Object obj, EventArgs e);
        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();
        #endregion

        #region Constructors
        public TriageGridUserControl()
        {
            InitializeComponent();
        }
        public TriageGridUserControl(ConnectionDefinition aConnectionDefinition)
            : this()
        {
            ConnectionDefinition = aConnectionDefinition;
            ShowWidgets();
        }
        #endregion

        #region Properties

        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }


        public TriageChartControl ChartControl
        {
            get { return _theChartControl; }
            set { _theChartControl = value; }
        }

        public ConnectionDefinition ConnectionDefinition
        {
            get { return _theConnectionDefinition; }
            set
            {
                if (_theConnectionDefinition != null && _widgetConfig != null)
                {
                    _widget.DataProvider.Stop();
                }

                _theConnectionDefinition = value;
                if ((_widgetConfig != null) && (_widgetConfig.DataProviderConfig != null))
                {
                    _widgetConfig.DataProviderConfig.ConnectionDefinition = _theConnectionDefinition;
                }
                doClearTriageSpace();
            }
        }

        public DataProvider DataProvider
        {
            get { return _dataProvider; }
        }

        public UniversalWidgetConfig UniversalWidgetConfiguration
        {
            get
            {
                return _widgetConfig;
            }
            set { _widgetConfig = value; }
        }

        public GenericUniversalWidget GridWidget
        {
            get { return _widget; }
            set { _widget = value; }
        }

        public IDataDisplayHandler DataDisplayHandler
        {
            get { return _dataDisplayHandler; }
            set { }
        }

        public DrillDownManager DrillDownManager
        {
            get;
            set;
        }

        public DataTable DataStore
        {
            get { return _dataStore; }
            set { _dataStore = value; }
        }

        public string WindowTitle
        {
            get { return _theTitle; }
        }

        public TriageHelper TriageHelper
        {
            get { return _theTriageHelper; }
            set { _theTriageHelper = value; }
        }

        public bool ClientRulesEnabled
        {
            get
            {
                if (this.ConnectionDefinition == null)
                    return false;
                else
                    return true;
            }
        }

        #endregion Properties

        #region Public methods
        public void RefreshWithNewFilter(string aFilter)
        {
            TriageDataProvider _dataProvider = (TriageDataProvider)_widget.DataProvider;
            _dataProvider.IsSessionData = false;
            _dataProvider.FilterString = aFilter;
            _theTriageHelper.HiddenQIDs_ht.Clear();
            _dataProvider.Start();
        }

        public void UpdateGraphWidget(DataTable adatatable)
        {
            if (_theChartControl != null)
            {
                _theChartControl.UpdateGraphWidget(adatatable);
            }
        }

        public void HighlightQuery(String queryId, String queryStartTime)
        {
            if (null != this._highlightedQuery)
            {
                //Set 'original' background color to restore to
                Color originalBackCol = Color.White;

                //if there is no background color setting assigned by the Rules filter
                try
                {
                    if (this.ClientRulesEnabled && !this._highlightedQuery.Cells["BackgroundColor"].Text.Equals(null))
                    {
                        string backCol = ((string)this._highlightedQuery.Cells["BackgroundColor"].Text);
                        if (backCol != null && backCol != "")
                            originalBackCol = Color.FromArgb(int.Parse(backCol));
                    }

                }
                catch (Exception)
                {
                }

                //Update the background color, then re-apply cell specific color settings
                TriageGridDataDisplayHandler.ChangeDataRowBackColor(this._highlightedQuery, originalBackCol);
                TriageGridDataDisplayHandler.CheckChangeCellRowColor(this._highlightedQuery);
            }

            this._highlightedQuery = null;


            if (null == queryId)
                return;

            iGrid TriageIGridControl = ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid;

            int cnt = TriageIGridControl.Rows.Count;
            for (int i = 0; i < TriageIGridControl.Rows.Count; i++)
            {
                String qId = TriageIGridControl.Rows[i].Cells["QUERY_ID"].Value.ToString();
                String qStartTime = TriageIGridControl.Rows[i].Cells["START_TIME"].Value.ToString();
                if (qId.Trim().Equals(queryId.Trim()) && qStartTime.Trim().Equals(queryStartTime.Trim()))
                {
                    this._highlightedQuery = TriageIGridControl.Rows[i];
                    break;
                }
            }

            if (null != this._highlightedQuery)
            {
                TriageGridDataDisplayHandler.ChangeDataRowBackColor(this._highlightedQuery, Color.Gold);
                TriageIGridControl.SetCurRow(-1);
                TriageIGridControl.SetCurRow(this._highlightedQuery.Index);
            }
        }

        public bool ShowSqlPreview()
        {
            return _sqlPreviewButton.Checked;
        }

        public bool isHighlightedQueryRunning()
        {
            try
            {
                if (null != this._highlightedQuery)
                {
                    String state = this._highlightedQuery.Cells["STATE"].Value.ToString().ToUpper();
                    if (RUNNING.Equals(state))
                        return true;

                }

            }
            catch (Exception)
            {
            }

            return false;
        }

        //enables and disables buttons based on the user selection of the rows
        public void enableDisableButtons()
        {
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            iGrid theGrid = dataDisplayControl.DataGrid;
            _sqlPreviewButton.Enabled = true;
            _getSessionsButton.Enabled = true;
            _clearButton.Enabled = true;
            ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid.Enabled = true;

            bool enableFlag = false;
            if (theGrid != null)
            {
                if (getSelectedVisibleRowCount() > 0)
                    enableFlag = true;
            }
            WorkspaceContextMenuGenerateSingleScript.Enabled = enableFlag;
            WorkspaceContextMenuDeleteSelectedRows.Enabled = enableFlag;
            _loadWhiteboardButton.Enabled = enableFlag;
            _hideStatementsButton.Enabled = enableFlag;
            _getSessionsButton.Enabled = enableFlag;
            _showHiddenButton.Enabled = (_theTriageHelper.HiddenQIDs_ht.Count > 0);


            _cancelSelectedQueryMenuItem.Enabled = false;
            _holdSelectedQueryMenuItem.Enabled = false;
            _releaseSelectedQueryMenuItem.Enabled = false;
            if (hasQueriesWithState(TriageGridUserControl.RUNNING))
            {
                _cancelSelectedQueryMenuItem.Enabled = true;
                _holdSelectedQueryMenuItem.Enabled = true;
            }
            else if (hasQueriesWithState(TriageGridUserControl.SUSPENDED) || hasQueriesWithState(TriageGridUserControl.HOLD))
            {
                _cancelSelectedQueryMenuItem.Enabled = true;
                _releaseSelectedQueryMenuItem.Enabled = true;
            }
            _theTriageHelper.EnablePropertyGridButtons();
        }

        #endregion

        #region Private Methods
        void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_widget != null && _widget.DataProvider != null)
                {
                    _widget.DataProvider.Stop();
                }
            }
        }
        private void InvokeHandleError(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleError), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        private void InvokeHandleNewDataArrived(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleNewDataArrived), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }
        private void InvokeHandleFetchingData(Object obj, EventArgs e)
        {
            try
            {
                if (IsHandleCreated)
                {
                    Invoke(new UpdateStatus(HandleFetchingData), new object[] { obj, e });
                }
            }
            catch (Exception ex)
            {
                //may be object is already disposed. Write a message to trace log to identify this stack trace
                Trafodion.Manager.Framework.Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, TraceOptions.TraceArea.WMS,
                    "Unexpected error. It is possible that the object has been disposed already." + Environment.NewLine + ex.StackTrace);
            }
        }


        private bool hasQueriesWithState(String state)
        {
            bool ret = false;
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            TrafodionIGrid theGrid = dataDisplayControl.DataGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int i in theGrid.SelectedRowIndexes)
                {
                    iGRow row = theGrid.Rows[i];
                    if ((row.Visible) && (row.Cells["STATE"].Value.ToString().ToUpper().Equals(state.ToUpper())))
                    {
                        ret = true;
                        break;
                    }
                }
            }
            return ret;

        }

        private String getSelectedIGridContents(TrafodionIGrid theGrid, bool generateSQLScript)
        {
            if (null == theGrid)
                return "";

            if (0 == theGrid.SelectedRowIndexes.Count)
            {
                if (0 == theGrid.Rows.Count)
                    return null;

                MessageBox.Show("\nWarning:  No Row(s) have been selected for copying.\n\n",
                               "No Row(s) Selected Warning",
                               MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return null;
            }

            StringBuilder headersSB = new StringBuilder();
            StringBuilder rowsSB = new StringBuilder();
            int nRows = theGrid.Rows.Count;
            int nCols = theGrid.Cols.Count;

            int i = 0;
            bool[] isColumnForIndexVisible = new bool[nCols];
            bool isTheFirstColumn = true;
            bool isNPASkin = true;// this._wsOptions.NPASkin;

            for (i = 0; i < nCols; i++)
            {
                // Always copy the SQL_TEXT field -- but only for NPA Skin.
                bool isSQLTextColumn = theGrid.Cols[i].Key.Equals("SQL_TEXT");
                isColumnForIndexVisible[i] = theGrid.Cols[i].Visible || isSQLTextColumn;

                // Need to copy 'em all for Gary's excel macro for R2.3 -- so just turn visible to true.
                isColumnForIndexVisible[i] = true;
                if (!isNPASkin && isSQLTextColumn)
                    isColumnForIndexVisible[i] = false;
                else if (ClientQueryRuler.isAQueryRulerColumn(theGrid.Cols[i].Key))
                    isColumnForIndexVisible[i] = false;

                if (!isSQLTextColumn && generateSQLScript)
                    isColumnForIndexVisible[i] = false;

                if (generateSQLScript || (false == isColumnForIndexVisible[i]))
                    continue;

                if (!isTheFirstColumn)
                    headersSB.Append('\t');

                headersSB.Append(theGrid.Cols[i].Text);
                isTheFirstColumn = false;
            }

            for (i = 0; i < theGrid.Rows.Count; i++)
            {
                if (theGrid.Rows[i].Visible && theGrid.SelectedRowIndexes.Contains(i))
                {
                    isTheFirstColumn = true;


                    if (generateSQLScript)
                    {
                        iGRow aRow = theGrid.Rows[i];
                        String headingComments = getQueryRowInfoNCIComments(aRow);
                        rowsSB.Append(headingComments);
                    }

                    for (int j = 0; j < nCols; j++)
                    {
                        if (false == isColumnForIndexVisible[j])
                            continue;

                        if (!isTheFirstColumn)
                            rowsSB.Append('\t');

                        String colValue = theGrid.Cells[i, j].Text;

                        if (theGrid.Cols[j].Key.Equals("SQL_TEXT"))
                        {
                            if (generateSQLScript)
                            {
                                colValue = System.Text.RegularExpressions.Regex.Replace(colValue, @"(\r\n|\r|\n)", "\r\n");
                                colValue = System.Text.RegularExpressions.Regex.Replace(colValue, @"(\r\n)", Environment.NewLine);
                            }
                            else
                                colValue = System.Text.RegularExpressions.Regex.Replace(colValue, @"(\r|\n|\t)", " ");

                            //  Add a trailing semi-colon when we copy to the clipboard so that its easy to run
                            //  this query externally in a script file.
                            if (false == colValue.Trim().EndsWith(";"))
                                colValue = colValue + ";";
                        }

                        rowsSB.Append(colValue);
                        isTheFirstColumn = false;
                    }

                    rowsSB.Append(Environment.NewLine);

                    if (generateSQLScript)
                        rowsSB.Append(Environment.NewLine);

                }

            }

            String copiedData = "";
            if (0 < rowsSB.Length)
                copiedData = headersSB.ToString() + Environment.NewLine + rowsSB.ToString();

            return copiedData;
        }

        private String getQueryRowInfoNCIComments(iGRow aRow)
        {
            StringBuilder headingComments = new StringBuilder();

            String queryID = "";
            String queryElapsedTime = "";

            try
            {
                queryID = aRow.Cells["QUERY_ID"].Value.ToString().Trim();
                queryElapsedTime = aRow.Cells["ELAPSED_TIME"].Value.ToString().Trim();
            }
            catch (Exception)
            {
            }


            String queryStartTime = "";
            try
            {
                DateTime startTime = (DateTime)aRow.Cells["START_TIME"].Value;
                queryStartTime = startTime.ToString("ddd, dd MMM yyyy HH:mm:ss");
            }
            catch (Exception)
            {
                queryStartTime = "";
            }

            String queryEndTime = "";
            try
            {
                DateTime endTime = (DateTime)aRow.Cells["END_TIME"].Value;
                queryEndTime = endTime.ToString("ddd, dd MMM yyyy HH:mm:ss");
            }
            catch (Exception)
            {
                queryEndTime = "";
            }

            headingComments.Append(Environment.NewLine);
            headingComments.Append("--  ").Append(Environment.NewLine);
            headingComments.Append("--  Query ID:  ").Append(queryID).Append(Environment.NewLine);
            headingComments.Append("--  Run Time:  ").Append(queryElapsedTime).Append(" seconds  ");
            headingComments.Append("[").Append(queryStartTime).Append("  TO  ");
            headingComments.Append(queryEndTime).Append("]").Append(Environment.NewLine);
            headingComments.Append("--   ").Append(Environment.NewLine);

            return headingComments.ToString();
        }

        private void ShowWidgets()
        {

            //Create a UW using the configuration
            _widgetConfig = WidgetRegistry.GetConfigFromPersistence(TriageGridConfigName);
            TriageDataProviderConfig dbConfig = null;
            List<ColumnMapping> persistedColumnMappings = null;
            if (_widgetConfig == null)
            {
                _widgetConfig = new UniversalWidgetConfig();
                _widgetConfig.Name = TriageGridConfigName;
                _widgetConfig.Title = _theTitle;
                _widgetConfig.ShowProperties = false;
                _widgetConfig.ShowToolBar = true;
                _widgetConfig.ShowChart = false;
                _widgetConfig.ShowRowCount = false;
                _widgetConfig.ShowTimerSetupButton = false;
                _widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            else
            {
                if (_widgetConfig.DataProviderConfig != null)
                {
                    persistedColumnMappings = _widgetConfig.DataProviderConfig.ColumnMappings;
                }
            }

            _widgetConfig.Name = TriageGridConfigName;
            _widgetConfig.ShowRowCount = true;
            _widgetConfig.ShowTimerSetupButton = false;
            _widgetConfig.ShowHelpButton = true;
            _widgetConfig.ShowProviderStatus = false;
            _widgetConfig.HelpTopic = HelpTopics.UsingTriageSpace;

            if (_widgetConfig.DataProviderConfig == null)
            {
                _widgetConfig.DataProviderConfig = new TriageDataProviderConfig();
            }
            dbConfig = (TriageDataProviderConfig)_widgetConfig.DataProviderConfig;


            dbConfig.MaxRowCount = 0;
            confureTriageDataProviderConfig(dbConfig, persistedColumnMappings);

            //set the connection definition and the data provider
            dbConfig.ConnectionDefinition = _theConnectionDefinition;

            _dataProvider = (TriageDataProvider)dbConfig.GetDataProvider();

            //Create the Universal widget to diaplay the data
            _widget = new GenericUniversalWidget();
            _widget.DataProvider = _dataProvider;
            _widget.UniversalWidgetConfiguration = _widgetConfig;

            //Set the display properties of the widget and add it to the control
            ((TabularDataDisplayControl)_widget.DataDisplayControl).LineCountFormat = "Running (0) Completed (0) Error (0)";
            _dataDisplayHandler = new TriageGridDataDisplayHandler(this);
            //_dataProvider.ColumnsToFilterOn = _dataDisplayHandler.Columns;
            _widget.DataDisplayControl.DataDisplayHandler = _dataDisplayHandler;
            _widget.UniversalWidgetConfiguration = _widgetConfig;
            _widget.Dock = DockStyle.Fill;
            this._theUniversalWidgetPanel.Controls.Add(_widget);

            AddToolStripButtons();

            //Add popup menu items to the table
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;

            if (dataDisplayControl != null)
            {
                WorkspaceContextMenuGenerateSingleScript = new TrafodionIGridToolStripMenuItem();
                WorkspaceContextMenuGenerateSingleScript.Text = "Single Consolidated Script...";
                WorkspaceContextMenuGenerateSingleScript.Enabled = false;
                WorkspaceContextMenuGenerateSingleScript.Click += new EventHandler(WorkspaceContextMenuGenerateSingleScript_Click);
                dataDisplayControl.AddMenuItem(WorkspaceContextMenuGenerateSingleScript);

                WorkspaceContextMenuClearTriageSpace = new TrafodionIGridToolStripMenuItem();
                WorkspaceContextMenuClearTriageSpace.Text = "Clear &Triage Space";
                WorkspaceContextMenuClearTriageSpace.Enabled = true;
                WorkspaceContextMenuClearTriageSpace.Click += new EventHandler(WorkspaceContextMenuClearTriageSpace_Click);
                dataDisplayControl.AddMenuItem(WorkspaceContextMenuClearTriageSpace);


                WorkspaceContextMenuDeleteSelectedRows = new TrafodionIGridToolStripMenuItem();
                WorkspaceContextMenuDeleteSelectedRows.Text = "Delete Selected Rows";
                WorkspaceContextMenuDeleteSelectedRows.Enabled = false;
                WorkspaceContextMenuDeleteSelectedRows.Click += new EventHandler(WorkspaceContextMenuDeleteSelectedRows_Click);
                dataDisplayControl.AddMenuItem(WorkspaceContextMenuDeleteSelectedRows);

                _displaySQLPlanMenuItem = new TrafodionIGridToolStripMenuItem();
                _displaySQLPlanMenuItem.Text = "Display SQL Plan...";
                _displaySQLPlanMenuItem.Enabled = false;
                _displaySQLPlanMenuItem.Click += new EventHandler(displaySQLPlanMenuItem_Click);
                dataDisplayControl.AddMenuItem(_displaySQLPlanMenuItem);

                dataDisplayControl.AddToolStripSeparator(new ToolStripSeparator());

                _cancelSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _cancelSelectedQueryMenuItem.Text = "Cancel Selected Queries...";
                _cancelSelectedQueryMenuItem.Enabled = false;
                _cancelSelectedQueryMenuItem.Click += new EventHandler(cancelSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_cancelSelectedQueryMenuItem);

                _holdSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _holdSelectedQueryMenuItem.Text = "Hold Selected Queries...";
                _holdSelectedQueryMenuItem.Enabled = false;
                _holdSelectedQueryMenuItem.Click += new EventHandler(holdSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_holdSelectedQueryMenuItem);

                _releaseSelectedQueryMenuItem = new TrafodionIGridToolStripMenuItem();
                _releaseSelectedQueryMenuItem.Text = "Release Selected Queries...";
                _releaseSelectedQueryMenuItem.Enabled = false;
                _releaseSelectedQueryMenuItem.Click += new EventHandler(releaseSelectedQueryMenuItem_Click);
                dataDisplayControl.AddMenuItem(_releaseSelectedQueryMenuItem);

            }
            //Add event handlers to deal with data provider events
            AddHandlers();

            dataDisplayControl.DataGrid.CellMouseDown += new iGCellMouseDownEventHandler(DataGrid_CellMouseDown);
            dataDisplayControl.DataGrid.SelectionChanged += new EventHandler(DataGrid_SelectionChanged);

            //Set client threshold columns to be Alwasys Hidden Columns, so that they won't showup in Show/Hide Grid Columns
            List<string> alwaysHiddenColumnNames = new List<string>();
            alwaysHiddenColumnNames.Add("ForegroundColor");
            alwaysHiddenColumnNames.Add("BackgroundColor");
            alwaysHiddenColumnNames.Add("ViolatorColor");
            alwaysHiddenColumnNames.Add("ViolatorNames");
            alwaysHiddenColumnNames.Add("ElapsedTimeTicks");
            alwaysHiddenColumnNames.Add("WaitTimeTicks");
            dataDisplayControl.DataGrid.AlwaysHiddenColumnNames.AddRange(alwaysHiddenColumnNames);

            //((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid.CellClick += new TenTec.Windows.iGridLib.iGCellClickEventHandler(DataGrid_CellClick);

            //Show for all events
            //_widget.StartDataProvider();

            //Add Disposed handler to clean up, connection
            this.Disposed += new EventHandler(TriageGridUserControl_Disposed);

            //Force it to create all controls.
            this.CreateControl();
            enableDisableButtons();
        }

        void confureTriageDataProviderConfig(TriageDataProviderConfig dbConfig, List<ColumnMapping> persistedColumnMappings)
        {
            List<string> defaultVisibleColumns = new List<string>();
            defaultVisibleColumns.Add("ELAPSED_TIME");
            defaultVisibleColumns.Add("ESTIMATED_COST");
            defaultVisibleColumns.Add("START_TIME");
            defaultVisibleColumns.Add("END_TIME");
            defaultVisibleColumns.Add("STATE");
            defaultVisibleColumns.Add("CLIENT_ID");
            defaultVisibleColumns.Add("DATASOURCE");
            defaultVisibleColumns.Add("USER_NAME");
            defaultVisibleColumns.Add("APPLICATION_NAME");
            defaultVisibleColumns.Add("MASTER_PROCESS_ID");
            defaultVisibleColumns.Add("LOCKWAITS");
            defaultVisibleColumns.Add("ROWS_ACCESSED");
            defaultVisibleColumns.Add("QUERY_ID");
            defaultVisibleColumns.Add("NUM_ROWS_IUD");
            defaultVisibleColumns.Add("ERROR_CODE");
            defaultVisibleColumns.Add("DISK_READS");
            //defaultVisibleColumns.Add("LOCKESCALATIONS");
            defaultVisibleColumns.Add("MESSAGES_TO_DISK");
            defaultVisibleColumns.Add("MESSAGE_BYTES_TO_DISK");
            defaultVisibleColumns.Add("SQL_TYPE");
            dbConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping("ELAPSED_TIME", getDisplayColumnName("ELAPSED_TIME"), 80));
            columnMappings.Add(new ColumnMapping("ESTIMATED_COST", getDisplayColumnName("ESTIMATED_COST"), 80));
            columnMappings.Add(new ColumnMapping("START_TIME", getDisplayColumnName("START_TIME"), 80));
            columnMappings.Add(new ColumnMapping("END_TIME", getDisplayColumnName("END_TIME"), 80));
            columnMappings.Add(new ColumnMapping("STATE", getDisplayColumnName("STATE"), 80));
            columnMappings.Add(new ColumnMapping("CLIENT_ID", getDisplayColumnName("CLIENT_ID"), 80));
            //columnMappings.Add(new ColumnMapping("DATASOURCE", getDisplayColumnName("DATASOURCE"), 80));
            columnMappings.Add(new ColumnMapping("USER_NAME", getDisplayColumnName("USER_NAME"), 80));
            columnMappings.Add(new ColumnMapping("APPLICATION_NAME", getDisplayColumnName("APPLICATION_NAME"), 80));
            columnMappings.Add(new ColumnMapping("MASTER_PROCESS_ID", getDisplayColumnName("MASTER_PROCESS_ID"), 80));
            columnMappings.Add(new ColumnMapping("LOCKWAITS", getDisplayColumnName("LOCKWAITS"), 80));
            columnMappings.Add(new ColumnMapping("ROWS_ACCESSED", getDisplayColumnName("ROWS_ACCESSED"), 80));
            columnMappings.Add(new ColumnMapping("QUERY_ID", getDisplayColumnName("QUERY_ID"), 80));
            columnMappings.Add(new ColumnMapping("NUM_ROWS_IUD", getDisplayColumnName("NUM_ROWS_IUD"), 80));
            columnMappings.Add(new ColumnMapping("ERROR_CODE", getDisplayColumnName("ERROR_CODE"), 80));
            columnMappings.Add(new ColumnMapping("DISK_READS", getDisplayColumnName("DISK_READS"), 80));
            //columnMappings.Add(new ColumnMapping("LOCKESCALATIONS", getDisplayColumnName("LOCKESCALATIONS"), 80));
            columnMappings.Add(new ColumnMapping("MESSAGES_TO_DISK", getDisplayColumnName("MESSAGES_TO_DISK"), 80));
            columnMappings.Add(new ColumnMapping("MESSAGE_BYTES_TO_DISK", getDisplayColumnName("MESSAGE_BYTES_TO_DISK"), 80));
            columnMappings.Add(new ColumnMapping("SQL_TYPE", getDisplayColumnName("SQL_TYPE"), 80));
            dbConfig.ColumnMappings = columnMappings;
            ColumnMapping.Synchronize(dbConfig.ColumnMappings, persistedColumnMappings);
        }

        void DataGrid_SelectionChanged(object sender, EventArgs e)
        {
            enableDisableButtons();
        }

        private void WorkspaceContextMenuClearTriageSpace_Click(object sender, EventArgs e)
        {
            clearTriageSpace();
        }

        private void WorkspaceContextMenuDeleteSelectedRows_Click(object sender, EventArgs e)
        {
            deleteSelectedTriageRows();
        }

        private void displaySQLPlanMenuItem_Click(object sender, EventArgs events)
        {
            //DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
            TrafodionIGridToolStripMenuItem mi = sender as TrafodionIGridToolStripMenuItem;
            if (mi != null)
            {
                TrafodionIGridEventObject eventObj = mi.TrafodionIGridEventObject;
                if (eventObj != null)
                {
                    TrafodionIGrid iGrid = eventObj.TheGrid;
                    int row = eventObj.Row;
                    string query_id = iGrid.Cells[row, WmsCommand.COL_QUERY_ID].Value as string;
                    string query_text = iGrid.Cells[row, WmsCommand.COL_SQL_TEXT].Value as string;
                    int length = query_text == null ? 0 : query_text.Length;
                    string mxosrvrStartTime = "";
                    if (iGrid.Cols.KeyExists(WmsCommand.COL_QUERY_START_TIME))
                    {
                        mxosrvrStartTime = iGrid.Cells[row, WmsCommand.COL_QUERY_START_TIME].Value as string;
                    }
                    string dataSource = "";
                    //if (iGrid.Cols.KeyExists("DATASOURCE"))
                    //{
                      //  dataSource = iGrid.Cells[row, "DATASOURCE"].Value as string;
                    //}
                    string title = string.Format(Properties.Resources.TitleQueryPlan, query_id);
                    WorkloadPlanControl wpc = new WorkloadPlanControl(_theConnectionDefinition, query_id, mxosrvrStartTime, query_text, length, true, dataSource,false);
                    Utilities.LaunchManagedWindow(title, wpc, _theConnectionDefinition, new Size(800, 600), true);
                }
            }
        }

        private void DataGrid_CellMouseDown(object sender, iGCellMouseDownEventArgs e)
        {
            if (e.RowIndex >= 0 && e.Button == MouseButtons.Right)
            {
                enableDisableButtons();
                string queryId = (string)((iGrid)sender).Rows[e.RowIndex].Cells[WmsCommand.COL_QUERY_ID].Value;
                _displaySQLPlanMenuItem.Enabled = queryId.Length > 0;
            }
        }

        private void WorkspaceContextMenuGenerateSingleScript_Click(object sender, EventArgs e)
        {
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            String sqlTextScript = getSelectedIGridContents(dataDisplayControl.DataGrid, true);
            TriageHelper.saveQueriesToScriptFile(sqlTextScript);
        }


        void holdSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            bool queryHeld = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            TrafodionIGrid theGrid = dataDisplayControl.DataGrid;

            Hashtable states_ht = new Hashtable();
            states_ht.Add(RUNNING, RUNNING);
            states_ht.Add("RELEASED", "RELEASED");
            states_ht.Add("WAITING", "WAITING");
            bool allRowsMatched = true;
            String notAllRunningQueries = "";

            Hashtable runningQueryRows = getSelectedQueriesMatchingState(theGrid, states_ht, out allRowsMatched);
            if (!allRowsMatched)
                notAllRunningQueries = "\nNot all the selected queries are in running state. " +
                                       "Only the running queries will be put on hold (suspended). \n";

            StringBuilder qryList = new StringBuilder();
            int nQueries = 0;
            foreach (DictionaryEntry de in runningQueryRows)
            {
                nQueries++;
                if (20 >= nQueries)
                    qryList.Append("\n").Append(de.Key.ToString());
            }

            if (20 < nQueries)
                qryList.Append("\n     ...  and " + (nQueries - 20) + " more ... ");


            if (0 >= qryList.Length)
                return;
            DialogResult yesNo = MessageBox.Show(notAllRunningQueries +
                                                 "\nPlease confirm that you wish to suspend (hold) the following queries \n" +
                                                 qryList.ToString() + "  ?\n", "Suspend (Hold) Selected Queries",
                                                 MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (DialogResult.No == yesNo)
                return;

            Connection connection = null;
            Cursor = Cursors.WaitCursor;
            foreach (DictionaryEntry de in runningQueryRows)
            {
                try
                {
                    iGRow aRow = (iGRow)de.Value;
                    string query_id = aRow.Cells["QUERY_ID"].Value.ToString();
                    string query_state = aRow.Cells["STATE"].Value.ToString();

                    //Skip queries that have completed or already on hold
                    if (query_state.Equals(TriageGridUserControl.COMPLETED) || query_state.Equals(TriageGridUserControl.ERROR) || query_state.Equals(TriageGridUserControl.HOLD) || query_state.Equals(TriageGridUserControl.SUSPENDED))
                        continue;

                    try
                    {
                        Trafodion.Manager.WorkloadArea.Model.Queries.ManageQuery(_theConnectionDefinition, query_id, WmsCommand.QUERY_ACTION.HOLD_QUERY);
                        queryHeld = true;
                        try
                        {
                            aRow.Cells["STATE"].Value = SUSPENDED;
                        }
                        catch (Exception)
                        {
                        }

                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { query_id, ex.Message });
                    }
                }
                catch (Exception)
                {
                }
            }
            Cursor = Cursors.Default;


            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("One or more queries were not placed on hold", errorTable, System.Drawing.SystemIcons.Warning);
                mmd.ShowDialog();
            }
        }

        void cancelSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            bool queryCancelled = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            TrafodionIGrid theGrid = dataDisplayControl.DataGrid;

            Hashtable states_ht = new Hashtable();
            states_ht.Add(RUNNING, RUNNING);
            states_ht.Add(SUSPENDED, SUSPENDED);
            bool allRowsMatched = true;
            String notAllRunningQueries = "";

            Hashtable runningQueryRows = getSelectedQueriesMatchingState(theGrid, states_ht, out allRowsMatched);
            if (!allRowsMatched)
                notAllRunningQueries = "\nNot all the selected queries are in running state. " +
                                       "Only the running queries will cancelled. \n";

            StringBuilder qryList = new StringBuilder();
            int nQueries = 0;
            foreach (DictionaryEntry de in runningQueryRows)
            {
                nQueries++;
                if (20 >= nQueries)
                    qryList.Append("\n").Append(de.Key.ToString());
            }

            if (20 < nQueries)
                qryList.Append("\n     ...  and " + (nQueries - 20) + " more ... ");


            if (0 >= qryList.Length)
                return;
            DialogResult yesNo = MessageBox.Show(notAllRunningQueries +
                                                 "\nPlease confirm that you wish to cancel the following queries \n" +
                                                 qryList.ToString() + "  ?\n", "Cancel Selected Queries",
                                                 MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (DialogResult.No == yesNo)
                return;

            Connection connection = null;
            Cursor = Cursors.WaitCursor;
            foreach (DictionaryEntry de in runningQueryRows)
            {
                try
                {
                    iGRow aRow = (iGRow)de.Value;
                    string query_id = aRow.Cells["QUERY_ID"].Value.ToString();
                    string query_state = aRow.Cells["STATE"].Value.ToString().ToUpper();

                    //Skip queries that have completed or already on hold
                    if (query_state.Equals(TriageGridUserControl.COMPLETED) || query_state.Equals(TriageGridUserControl.ERROR))
                        continue;

                    try
                    {
                        Trafodion.Manager.WorkloadArea.Model.Queries.ManageQuery(_theConnectionDefinition, query_id, WmsCommand.QUERY_ACTION.CANCEL_QUERY);
                        queryCancelled = true;
                        try
                        {
                            aRow.Cells["STATE"].Value = "Completed";
                        }
                        catch (Exception)
                        {
                        }

                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { query_id, ex.Message });
                    }
                }
                catch (Exception)
                {
                }
            }
            Cursor = Cursors.Default;


            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("One or more queries were not cancelled", errorTable, System.Drawing.SystemIcons.Warning);
                mmd.ShowDialog();
            }

        }

        void releaseSelectedQueryMenuItem_Click(object sender, EventArgs events)
        {
            bool queryCancelled = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            TrafodionIGrid theGrid = dataDisplayControl.DataGrid;

            Hashtable states_ht = new Hashtable();
            states_ht.Add(SUSPENDED, SUSPENDED);
            bool allRowsMatched = true;
            String notAllRunningQueries = "";

            Hashtable runningQueryRows = getSelectedQueriesMatchingState(theGrid, states_ht, out allRowsMatched);
            if (!allRowsMatched)
                notAllRunningQueries = "\nNot all the selected queries are in suspended state. " +
                                       "Only the Suspended queries will released. \n";

            StringBuilder qryList = new StringBuilder();
            int nQueries = 0;
            foreach (DictionaryEntry de in runningQueryRows)
            {
                nQueries++;
                if (20 >= nQueries)
                    qryList.Append("\n").Append(de.Key.ToString());
            }

            if (20 < nQueries)
                qryList.Append("\n     ...  and " + (nQueries - 20) + " more ... ");


            if (0 >= qryList.Length)
                return;
            DialogResult yesNo = MessageBox.Show(notAllRunningQueries +
                                                 "\nPlease confirm that you wish to release the following queries \n" +
                                                 qryList.ToString() + "  ?\n", "Release Selected Queries",
                                                 MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
            if (DialogResult.No == yesNo)
                return;

            Connection connection = null;
            Cursor = Cursors.WaitCursor;
            foreach (DictionaryEntry de in runningQueryRows)
            {
                try
                {
                    iGRow aRow = (iGRow)de.Value;
                    string query_id = aRow.Cells["QUERY_ID"].Value.ToString();
                    string query_state = aRow.Cells["STATE"].Value.ToString().ToUpper();

                    //Skip queries that have completed or already on hold
                    if (query_state.Equals(TriageGridUserControl.COMPLETED) || query_state.Equals(TriageGridUserControl.ERROR))
                        continue;

                    try
                    {
                        Trafodion.Manager.WorkloadArea.Model.Queries.ManageQuery(_theConnectionDefinition, query_id, WmsCommand.QUERY_ACTION.RELEASE_QUERY);
                        queryCancelled = true;
                        try
                        {
                            aRow.Cells["STATE"].Value = "Running";
                        }
                        catch (Exception)
                        {
                        }

                    }
                    catch (Exception ex)
                    {
                        errorTable.Rows.Add(new object[] { query_id, ex.Message });
                    }
                }
                catch (Exception)
                {
                }
            }
            Cursor = Cursors.Default;


            if (errorTable.Rows.Count > 0)
            {
                TrafodionMultipleMessageDialog mmd = new TrafodionMultipleMessageDialog("One or more queries were not released", errorTable, System.Drawing.SystemIcons.Warning);
                mmd.ShowDialog();
            }

        }


        //private bool canRoleHoldOrReleaseAQuery(String role)
        //{
        //    if (_theConnectionDefinition != null)
        //    {
        //        return (_theConnectionDefinition.IsWmsAdminRole);
        //    }

        //    return false;
        //    //NCCUtils.isRoleServices(myRoleName) || NCCUtils.isRoleDBA(myRoleName) ||
        //    //        (isWMSAdminRole() && (null != this.RepositoryVersion) &&
        //    //         NCCUtils.isWMSOnR24AndLater(this.RepositoryVersion.WMSVersion))
        //}


        private Hashtable getSelectedQueriesMatchingState(TrafodionIGrid theGrid, Hashtable theStates_ht, out bool allMatched)
        {
            Hashtable matchingQueries_ht = new Hashtable();
            allMatched = true;

            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int i in theGrid.SelectedRowIndexes)
                {
                    try
                    {
                        iGRow row = theGrid.Rows[i];
                        String qryState = row.Cells["STATE"].Value.ToString();
                        if (false == theStates_ht.ContainsKey(qryState.Trim().ToUpper()))
                        {
                            allMatched = false;
                            continue;
                        }

                        String qID = row.Cells["QUERY_ID"].Value.ToString();
                        matchingQueries_ht.Add(qID, row);
                    }
                    catch (Exception)
                    {
                    }
                }
            }
            return matchingQueries_ht;
        }

        void TriageGridUserControl_Disposed(object sender, EventArgs e)
        {
            RemoveHandlers();
        }

        //associates the data provider handlers
        private void AddHandlers()
        {
            if (_dataProvider != null)
            {
                //Associate the event handlers
                _dataProvider.OnErrorEncountered += InvokeHandleError;
                _dataProvider.OnFetchCancelled += InvokeHandleError;
                _dataProvider.OnNewDataArrived += InvokeHandleNewDataArrived;
                _dataProvider.OnFetchingData += InvokeHandleFetchingData;

            }
        }

        //removes the data provider handlers
        private void RemoveHandlers()
        {
            if (_dataProvider != null)
            {
                //Remove the event handlers
                _dataProvider.OnErrorEncountered -= InvokeHandleError;
                _dataProvider.OnFetchCancelled -= InvokeHandleError;
                _dataProvider.OnNewDataArrived -= InvokeHandleNewDataArrived;
                _dataProvider.OnFetchingData -= InvokeHandleFetchingData;
            }
        }

        // Methods to deal with DataProvider events
        private void HandleError(Object obj, EventArgs e)
        {

            //In case of error, the data provider and the UI can get out of sync.
            //So it's best to reset the UI to keep everything consistent.
            doClearTriageSpace();

            enableDisableButtons();
        }

        private void HandleNewDataArrived(Object obj, EventArgs e)
        {
            enableDisableButtons();
        }

        private void HandleFetchingData(Object obj, EventArgs e)
        {
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            dataDisplayControl.PersistConfiguration();
            enableDisableButtons(false);
        }

        private void enableDisableButtons(bool enable)
        {
            _sqlPreviewButton.Enabled = enable;
            _showControlButton.Enabled = enable;
            _showHiddenButton.Enabled = enable && (_theTriageHelper.HiddenQIDs_ht.Count > 0);
            _hideStatementsButton.Enabled = enable;
            _getSessionsButton.Enabled = enable;
            _clearButton.Enabled = enable;
            _loadWhiteboardButton.Enabled = enable;
            ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid.Enabled = enable;

        }


        private int getSelectedVisibleRowCount()
        {
            int selectedCount = 0;
            TabularDataDisplayControl dataDisplayControl = _widget.DataDisplayControl as TabularDataDisplayControl;
            TrafodionIGrid theGrid = dataDisplayControl.DataGrid;
            if (theGrid.SelectedRowIndexes.Count > 0)
            {
                foreach (int i in theGrid.SelectedRowIndexes)
                {
                    iGRow row = theGrid.Rows[i];
                    if (row.Visible)
                    {
                        selectedCount++;
                    }
                }
                return selectedCount;
            }
            return selectedCount;
        }

        private string getDisplayColumnName(string colName)
        {
            return colName.Replace("_", " ");
        }

        private void AddToolStripButtons()
        {

            _sqlPreviewButton.Text = "Preview SQL";
            _sqlPreviewButton.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
            _sqlPreviewButton.CheckOnClick = true;
            _sqlPreviewButton.ImageScaling = ToolStripItemImageScaling.None;
            _sqlPreviewButton.Checked = false; // _monitorWorkloadOptions.SQLPreview;
            _sqlPreviewButton.Image = _sqlPreviewButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            _sqlPreviewButton.CheckedChanged += new EventHandler(_sqlPreviewButton_CheckedChanged);
            _widget.AddToolStripItem(_sqlPreviewButton);


            //_showControlButton.Text = "Show Control";
            //_showControlButton.DisplayStyle = ToolStripItemDisplayStyle.ImageAndText;
            //_showControlButton.CheckOnClick = true;
            //_showControlButton.ImageScaling = ToolStripItemImageScaling.None;
            //_showControlButton.Checked = false; // _monitorWorkloadOptions.SQLPreview;
            //_showControlButton.Image = _showControlButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            //_showControlButton.CheckedChanged += new EventHandler(_showControlButton_CheckedChanged);
            //_widget.AddToolStripItem(_showControlButton);

            _widget.AddToolStripItem(new ToolStripSeparator());


            _clientRuleButton.Text = "Configure Client Thresholds";
            _clientRuleButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            _clientRuleButton.Image = Trafodion.Manager.Properties.Resources.AlterIcon;
            _clientRuleButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _clientRuleButton.Name = "_clientRuleButton";
            _clientRuleButton.Click += new EventHandler(_clientRuleButton_Click);
            //_clientRuleButton.Enabled = false;
            _widget.AddToolStripItem(_clientRuleButton);

            _showHiddenButton.Text = "Show Hidden";
            _showHiddenButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _showHiddenButton.Image = (System.Drawing.Image)Properties.Resources.showhidden;
            _showHiddenButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _showHiddenButton.Name = "_showHiddenButton";
            _showHiddenButton.Click += new EventHandler(_showHiddenButton_Click);
            _widget.AddToolStripItem(_showHiddenButton);


            _hideStatementsButton.Text = "Hide";
            _hideStatementsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _hideStatementsButton.Image = (System.Drawing.Image)Properties.Resources.hidestatements;
            _hideStatementsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _hideStatementsButton.Name = "_hideStatementsButton";
            _hideStatementsButton.Click += new EventHandler(_hideStatementsButton_Click);
            _widget.AddToolStripItem(_hideStatementsButton);

            _widget.AddToolStripItem(new ToolStripSeparator());


            _getSessionsButton.Text = "Get Sessions";
            _getSessionsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _getSessionsButton.Image = (System.Drawing.Image)Properties.Resources.bookcase;
            _getSessionsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _getSessionsButton.Name = "_getSessionsButton";
            _getSessionsButton.Click += new EventHandler(_getSessionsButton_Click);
            _widget.AddToolStripItem(_getSessionsButton);

            _widget.AddToolStripItem(new ToolStripSeparator());

            _loadWhiteboardButton.Text = "Load Whiteboard";
            _loadWhiteboardButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _loadWhiteboardButton.Image = (System.Drawing.Image)global::Trafodion.Manager.Properties.Resources.WhiteBoardIcon;
            _loadWhiteboardButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _loadWhiteboardButton.Name = "_loadWhiteboardButton";
            _loadWhiteboardButton.Click += new EventHandler(_loadWhiteboardButton_Click);
            _widget.AddToolStripItem(_loadWhiteboardButton);

            _widget.AddToolStripItem(new ToolStripSeparator());

            _clearButton.Text = "Clear";
            _clearButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
            _clearButton.Image = (System.Drawing.Image)Properties.Resources.resetTriage;
            _clearButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            _clearButton.Name = "_clearButton";
            _clearButton.Click += new EventHandler(_clearButton_Click);
            _widget.AddToolStripItem(_clearButton);

        }

        void _sqlPreviewButton_CheckedChanged(object sender, EventArgs e)
        {
            //_monitorWorkloadOptions.SQLPreview = _sqlPreviewButton.Checked;
            _sqlPreviewButton.Image = _sqlPreviewButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            TriageGridDataDisplayHandler displayHandler = _widget.DataDisplayControl.DataDisplayHandler as TriageGridDataDisplayHandler;
            displayHandler.showHidePreviewSQL(_sqlPreviewButton.Checked);
        }

        void _showControlButton_CheckedChanged(object sender, EventArgs e)
        {
            //_monitorWorkloadOptions.SQLPreview = _sqlPreviewButton.Checked;
            _showControlButton.Image = _showControlButton.Checked ? global::Trafodion.Manager.Properties.Resources.Checkbox_Checked : global::Trafodion.Manager.Properties.Resources.Checkbox_UnChecked;
            //showHidePreviewSQL(_sqlPreviewButton.Checked);
        }

        void _clientRuleButton_Click(object sender, EventArgs e)
        {
            ClientQueryRuler.Instance.OpenRuleManager();
        }

        void _showHiddenButton_Click(object sender, EventArgs e)
        {
            showHiddenStatements();
            enableDisableButtons();
        }

        void _hideStatementsButton_Click(object sender, EventArgs e)
        {
            hideSelectedStatements();
            enableDisableButtons();
        }

        void _getSessionsButton_Click(object sender, EventArgs e)
        {
            TrafodionIGrid theGrid = ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid;
            //the first parameter determines whether the iGrid is from LiveView iGrid or not
            getSessionQueries(false, theGrid);
        }

        void _loadWhiteboardButton_Click(object sender, EventArgs e)
        {
            TrafodionIGrid theGrid = ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid;
            if (theGrid.SelectedRowIndexes.Count <= 0)
            {
                return;
            }

            Hashtable htStatements = new Hashtable();
            //ArrayList statements = new ArrayList();

            foreach (int i in theGrid.SelectedRowIndexes)
            {
                iGRow row = theGrid.Rows[i];
                string queryID = row.Cells["QUERY_ID"].Value as string;
                string queryText = row.Cells["SQL_TEXT"].Value as string;
                DateTime startTime = (DateTime)row.Cells["START_TIME"].Value;

                if (!String.IsNullOrEmpty(queryID) && !String.IsNullOrEmpty(queryText) && !htStatements.ContainsKey(queryID))
                {
                    StatementDefinition statement = new StatementDefinition(queryID, queryID, queryText);
                    statement.StartTime = startTime;
                    htStatements.Add(queryID, statement);
                }
            }

            if (htStatements.Count > 0)
            {
                ArrayList statements = new ArrayList();
                foreach (object obj in htStatements.Values)
                {
                    statements.Add(obj);
                }

                ITrafodionMain TrafodionMain = TrafodionContext.Instance.TheTrafodionMain;
                TrafodionMain.LaunchSQLWhiteboard(statements, "Triage Space");
            }
        }

        void _clearButton_Click(object sender, EventArgs e)
        {
            clearTriageSpace();
        }

        private void hideSelectedStatements()
        {
            TrafodionIGrid TriageIGridControl = ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid;
            foreach (int i in TriageIGridControl.SelectedRowIndexes)
            {
                iGRow row = TriageIGridControl.Rows[i];
                row.Visible = false;

                String queryID = row.Cells["QUERY_ID"].Value.ToString().Trim();
                String entryID = row.Cells["ENTRY_ID"].Value.ToString().Trim();

                String htKey = queryID + "^_^" + entryID;
                if (!_theTriageHelper.HiddenQIDs_ht.ContainsKey(htKey))
                    _theTriageHelper.HiddenQIDs_ht.Add(htKey, queryID);
            }
            _dataDisplayHandler.UpdateGraphWidget();
        }

        private void clearHiddenStatements()
        {
            _theTriageHelper.HiddenQIDs_ht.Clear();
            _dataDisplayHandler.UpdateGraphWidget();
        }

        private void showHiddenStatements()
        {
            iGrid TriageIGridControl = ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid;
            for (int i = 0; i < TriageIGridControl.Rows.Count; i++)
            {
                TriageIGridControl.Rows[i].Visible = true;
            }
            clearHiddenStatements();
            //showHideControlStatements();
        }

        private void clearTriageSpace()
        {
            DialogResult yesNo = MessageBox.Show("\nWarning: Clearing the Triage Space will " +
                                                 "remove all the currently triaged queries.\n\n" +
                                                 "Please confirm that you wish to clear the Triage Space? \n\n",
                                                 "Clear Triage Space Confirmation",
                                                 MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation);
            if (DialogResult.No == yesNo)
                return;

            doClearTriageSpace();
        }

        private void doClearTriageSpace()
        {
            _theTriageHelper.HiddenQIDs_ht.Clear();
            DataTable currentData = null;

            if (_theChartControl != null)
                _theChartControl.UpdateGraphWidget(null);

            if (DataProvider != null)
            {
                ((TriageDataProvider)DataProvider).IsSessionData = false;
            }

            if (_widget != null)
            {

                if (_widget.DataProvider != null)
                {
                    //if the data provider is running, stop it
                    //_widget.DataProvider.Stop();

                    //Clean up the cached data
                    currentData = ((DatabaseDataProvider)_widget.DataProvider).GetDataTable();
                    if (null != currentData)
                    {
                        currentData.Clear();
                    }
                }
                if ((_widget.DataDisplayControl != null) && (_widget.DataDisplayControl.DataDisplayHandler != null))
                {
                    if (((TriageGridDataDisplayHandler)_widget.DataDisplayControl.DataDisplayHandler).DisplayDataTable != null)
                    {
                        ((TriageGridDataDisplayHandler)_widget.DataDisplayControl.DataDisplayHandler).DisplayDataTable.Clear();
                    }

                    //((TriageGridDataDisplayHandler)_widget.DataDisplayControl.DataDisplayHandler).DoPopulate(_widget.UniversalWidgetConfiguration,
                    //    currentData,
                    if (((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid != null)
                    {
                        ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid.Rows.Clear();
                    }
                }
            }
        }

        private void deleteSelectedTriageRows()
        {
            TrafodionIGrid TriageIGridControl = ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid;

            if ((0 >= TriageIGridControl.SelectedRowIndexes.Count))
                return;

            DialogResult yesNo = MessageBox.Show("\nWarning: The selected queries will be removed " +
                                                 "from the Triage Space.\n\n" +
                                                 "Please confirm that you wish to delete them from the Triage Space? \n\n",
                                                 "Triage Space Delete Confirmation",
                                                 MessageBoxButtons.YesNo, MessageBoxIcon.Exclamation);
            if (DialogResult.No == yesNo)
                return;

            try
            {
                Hashtable rowsToDelete_ht = new Hashtable();
                foreach (int i in TriageIGridControl.SelectedRowIndexes)
                {
                    iGRow row = TriageIGridControl.Rows[i];

                    try
                    {
                        String qryID = row.Cells["QUERY_ID"].Value.ToString().Trim();
                        String entryID = row.Cells["ENTRY_ID"].Value.ToString().Trim();
                        String htKey = qryID + "^_^" + entryID;

                        if (rowsToDelete_ht.ContainsKey(htKey))
                            continue;

                        rowsToDelete_ht.Add(htKey, row.Index);

                    }
                    catch (Exception)
                    {
                    }
                }
                DataTable dataTable = _dataProvider.GetDataTable();
                deleteQueryRows(dataTable, rowsToDelete_ht);
                //if ((null != this._filteredDataTable) &&
                //    (this._filteredDataTable != this.m_dataTable))
                //    deleteQueryRows(this._filteredDataTable, rowsToDelete_ht);

            }
            catch (Exception e)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "deleteSelectedTriageRows(): Exception = " + e.Message);

            }

            this.DataDisplayHandler.DoPopulate(_widgetConfig, _dataProvider.GetDataTable(), ((TabularDataDisplayControl)_widget.DataDisplayControl).DataGrid);
        }

        private void deleteQueryRows(DataTable the_dt, Hashtable selectedRows_ht)
        {
            String htKey = "";

            List<DataRow> rows2Delete = new List<DataRow>();

            // Build the list of rows to delete.
            foreach (DataRow row in the_dt.Rows)
            {
                try
                {
                    String qryID = (String)row["QUERY_ID"];
                    String entryID = row["ENTRY_ID"].ToString().Trim();

                    htKey = qryID.Trim() + "^_^" + entryID;
                    if (selectedRows_ht.ContainsKey(htKey))
                        rows2Delete.Add(row);

                }
                catch (Exception e)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Monitoring,
                        TriageHelper.TRACE_SUB_AREA_NAME,
                        "deleteQueryRows: Error deleting row with key = '" + htKey + "'. Exception = " + e.Message);
                }

            }


            // And now delete the rows.
            foreach (DataRow theRow in rows2Delete)
            {
                try
                {
                    the_dt.Rows.Remove(theRow);

                }
                catch (Exception e)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Monitoring,
                        TriageHelper.TRACE_SUB_AREA_NAME,
                        "deleteQueryRows: Error deleting row with key = '" +
                                                    theRow["QUERY_ID"] + "^_^" +
                                                    theRow["ENTRY_ID"].ToString().Trim() +
                                                    "'. Exception = " + e.Message);

                }
            }

        }

        public void getSessionQueries(bool useLiveViewGrid, TrafodionIGrid theGrid)
        {
            StringBuilder inClause = new StringBuilder();
            TriageDataProvider dataProvider = (TriageDataProvider)_widget.DataProvider;

            if (0 >= theGrid.SelectedRowIndexes.Count)
                return;

            if (useLiveViewGrid)
            {
                StringBuilder queryIDList = generateInClauseForSelectedIDs(theGrid, "QUERY_ID", 1);
                inClause = generateInClauseForSelectedQueryIDs(queryIDList);
                String sessionFilter = " SESSION_ID IN (" + inClause.ToString() + ")";
                //RefreshWithNewFilter(sessionFilter);
                dataProvider.GetSessionStatements(sessionFilter);
            }
            else
            {
                inClause = generateInClauseForSelectedIDs(theGrid, "SESSION_ID", 1);
                String sessionFilter = " SESSION_ID IN (" + inClause.ToString() + ")";
                dataProvider.GetSessionStatements(sessionFilter);
            }

            //String sessionFilter = " SESSION_ID IN (" + inClause.ToString() + ")";
            //dataProvider.GetSessionStatements(sessionFilter);
        }

        //Here is where actually Load Live View Queries to Triage IGrid
        public void loadSelectedLiveViewQueries(TrafodionIGrid theIGrid)
        {
            if (0 >= theIGrid.SelectedRowIndexes.Count)
            {
                return;
            }
            StringBuilder inClause = generateInClauseForSelectedIDs(theIGrid, "QUERY_ID", 1);
            TriageDataProvider dataProvider = (TriageDataProvider)_widget.DataProvider;
            String theFilter = " QSTATS.QUERY_ID IN (" + inClause.ToString() + ")";
            RefreshWithNewFilter(theFilter);
        }

        private StringBuilder generateInClauseForSelectedQueryIDs(StringBuilder qIDs)
        {
            StringBuilder sb = new StringBuilder();

            sb.Append("SELECT  distinct SESSION_ID  FROM  ").Append(this.ConnectionDefinition.MetricQueryViewFull).Append("  SESSQSTATS");
            sb.Append("\n\t\t\t WHERE  SESSQSTATS.QUERY_ID in (" + qIDs.ToString() + ") ");
            sb.Append("\n\t\t\t FOR READ UNCOMMITTED ACCESS ");

            return sb;
        }

        public StringBuilder generateInClauseForSelectedIDs(TrafodionIGrid theGrid, String columnName, int numIndentTabs)
        {
            StringBuilder inClause = new StringBuilder();
            Hashtable theIDs_ht = new Hashtable();

            String indentPrefix = "";
            for (int idx = 0; idx < numIndentTabs; idx++)
                indentPrefix += "        ";

            String columnNameCharsetPrefix = "_" + TrafodionSystem.FindTrafodionSystem(this.ConnectionDefinition).GetColumnEncoding("MANAGEABILITY",
                "INSTANCE_REPOSITORY", this.ConnectionDefinition.MetricQueryView, TrafodionView.ObjectType, "QUERY_ID"); ;

            foreach (int i in theGrid.SelectedRowIndexes)
            {
                iGRow aRow = theGrid.Rows[i];

                if (false == aRow.Visible)
                    continue;

                String theID = aRow.Cells[columnName].Value.ToString().Trim();
                if (!theIDs_ht.ContainsKey(theID))
                {
                    theIDs_ht.Add(theID, theID);
                    if (0 < inClause.Length)
                        inClause.Append(", ");

                    inClause.Append("\n" + indentPrefix + columnNameCharsetPrefix + "'" + theID + "'");
                }
            }
            return inClause;
        }

        #endregion
    }

    /// <summary>
    /// The Custom data display handler for Triage
    /// </summary>
    public class TriageGridDataDisplayHandler : TabularDataDisplayHandler
    {
        #region Constants

        private const int NORMAL_ROW_HEIGHT = 19;
        private const int PREVIEW_ROW_HEIGHT = 38;
        public const String SUSPEND_RESUME_KILL_SPJ_APPLICATION_ID = "querykiller";
        public const String SUSPEND_RESUME_KILL_SPJ_CLIENT_ID = "spj.querykiller";

        #endregion

        #region Member variables
        TriageGridUserControl _theTriageGridUserControl = null;
        String numRunning = "0";
        String numCompleted = "0";
        String numWaitingOrInError = "0";
        String numHold = "0";
        private static Hashtable stateMapping_ht = new Hashtable();
        private static Hashtable _mappingHT = new Hashtable();
        //Hashtable hiddenQIDs_ht = new Hashtable();
        DataTable currentDataTable = null;
        private int _iGridNormalRowHeight = NORMAL_ROW_HEIGHT;
        private int _iGridRowHeightWithPreview = PREVIEW_ROW_HEIGHT;
        private TenTec.Windows.iGridLib.iGCellStyle iGrid3RowTextColCellStyle;
        private Color m_sqlPreviewColor = Color.DimGray;
        private bool _timeOffsetGMTAdd = false;
        private TimeSpan _displayTimeOffsetFromGMT = TimeSpan.FromTicks(0);
        private bool _savedTrafodionTimeOffsetGMTAdd = false;
        private TimeSpan _savedTrafodionDisplayTimeOffsetFromGMT = TimeSpan.FromTicks(0);
        #endregion

        #region constructors
        public TriageGridDataDisplayHandler()
        {

            stateMapping_ht.Clear();
            stateMapping_ht.Add("COMPLETED", "Completed");
            stateMapping_ht.Add("INIT", "Running");
            stateMapping_ht.Add("START", "Running");
            stateMapping_ht.Add("EXECUTING", "Running");
            stateMapping_ht.Add("ERROR", "Error");

            _mappingHT.Clear();
            _mappingHT.Add("SELECT", "SELECT");
            _mappingHT.Add("SEL", "SELECT");
            _mappingHT.Add("INSERT", "INSERT");
            _mappingHT.Add("INS", "INSERT");
            _mappingHT.Add("UPDATE", "UPDATE");
            _mappingHT.Add("UPD", "UPDATE");
            _mappingHT.Add("DELETE", "DELETE");
            _mappingHT.Add("DEL", "DELETE");
            _mappingHT.Add("MERGE", "MERGE");
            _mappingHT.Add("VALUES", "VALUES");

            _mappingHT.Add("CALL", "CALL");
            _mappingHT.Add("PREPARE", "PREPARE");
            _mappingHT.Add("EXECUTE", "EXECUTE");

            _mappingHT.Add("GRANT", "PRIVILEGES [GRANT]");
            _mappingHT.Add("REVOKE", "PRIVILEGES [REVOKE]");

            _mappingHT.Add("CONTROL", "CONTROL");
            _mappingHT.Add("SHOWCONTROL", "SHOWCONTROL");
            _mappingHT.Add("SHOWLABEL", "SHOWLABEL");
            _mappingHT.Add("SHOWSHAPE", "SHOWSHAPE");

            _mappingHT.Add("GET", "GET");
            _mappingHT.Add("SET", "SET");

            _mappingHT.Add("ALTER", "DDL [ALTER]");
            _mappingHT.Add("CREATE", "DDL [CREATE]");
            _mappingHT.Add("DROP", "DDL [DROP]");

            _mappingHT.Add("INVOKE", "DDL");
            _mappingHT.Add("SHOWDDL", "DDL");

            _mappingHT.Add("EXPLAIN", "EXPLAIN");

            _mappingHT.Add("BEGIN", "TRANSACTION [BEGIN]");
            _mappingHT.Add("COMMIT", "TRANSACTION [COMMIT]");
            _mappingHT.Add("ROLLBACK", "TRANSACTION [ROLLBACK]");
            _mappingHT.Add("BT", "TRANSACTION [BEGIN]");
            _mappingHT.Add("ET", "TRANSACTION [COMMIT]");

            _mappingHT.Add("LOCK", "LOCK");
            _mappingHT.Add("UNLOCK", "UNLOCK");

            _mappingHT.Add("INFOSTATS", "MANAGEMENT");
            _mappingHT.Add("MAINTAIN", "MANAGEMENT [MAINTAIN]");
            _mappingHT.Add("MANAGE", "MANAGEMENT");
            _mappingHT.Add("ONLINEDBDUMP", "MANAGEMENT [SCHEDULE]");
            _mappingHT.Add("POPULATE", "MANAGEMENT [POPULATE]");
            _mappingHT.Add("PURGEDATA", "MANAGEMENT");
            _mappingHT.Add("RECOVER", "MANAGEMENT");
            _mappingHT.Add("REFRESH", "MANAGEMENT [REFRESH]");
            _mappingHT.Add("REORG", "MANAGEMENT [REORG]");
            _mappingHT.Add("SCHEDULE", "MANAGEMENT [SCHEDULE]");
            _mappingHT.Add("VERIFY", "MANAGEMENT");

            /*  Entries for SQL and WMS Statement Types. */
            _mappingHT.Add("SELECT_UNIQUE", "SELECT");
            _mappingHT.Add("SELECT_NON_UNIQUE", "SELECT");
            _mappingHT.Add("INSERT_UNIQUE", "INSERT");
            _mappingHT.Add("INSERT_NON_UNIQUE", "INSERT");
            _mappingHT.Add("UPDATE_UNIQUE", "UPDATE");
            _mappingHT.Add("UPDATE_NON_UNIQUE", "UPDATE");
            _mappingHT.Add("DELETE_UNIQUE", "DELETE");
            _mappingHT.Add("DELETE_NON_UNIQUE", "DELETE");

            _mappingHT.Add("SET_TRANSACTION", "SET");
            _mappingHT.Add("SET_CATALOG", "SET");
            _mappingHT.Add("SET_SCHEMA", "SET");

            _mappingHT.Add("CALL_NO_RESULT_SETS", "CALL");
            _mappingHT.Add("CALL_WITH_RESULT_SETS", "CALL");
            _mappingHT.Add("SP_RESULT_SET", "CALL");

            computeRowHeightForSQLPreview();
        }

        public TriageGridDataDisplayHandler(TriageGridUserControl aTriageGridUserControl)
            : this()
        {
            _theTriageGridUserControl = aTriageGridUserControl;
        }
        #endregion

        #region properties
        public String NumRunning
        {
            get { return numRunning; }
            set { numRunning = value; }
        }

        public String NumCompleted
        {
            get { return numCompleted; }
            set { numCompleted = value; }
        }

        public String NumWaitingOrInError
        {
            get { return numWaitingOrInError; }
            set { numWaitingOrInError = value; }
        }

        public String NumHold
        {
            get { return numHold; }
            set { numHold = value; }
        }

        public DataTable DisplayDataTable
        {
            get { return currentDataTable; }
            set { currentDataTable = value; }
        }
        #endregion

        #region Public methods

        public void showHidePreviewSQL(bool previewSQLText)
        {
            iGrid thegrid = ((TabularDataDisplayControl)_theTriageGridUserControl.GridWidget.DataDisplayControl).DataGrid;
            if (previewSQLText)
            {
                thegrid.RowMode = true;
                thegrid.RowTextVisible = true;
                thegrid.RowTextStartColNear = 1;
            }
            else
            {
                thegrid.RowMode = false;
                thegrid.RowTextVisible = false;
            }

            foreach (iGRow row in thegrid.Rows)
            {
                if (previewSQLText)
                {
                    row.Height = _iGridRowHeightWithPreview;
                    string sqlPreview = row.Cells[TriageDataProvider.SQL_TEXT].Value.ToString();
                    sqlPreview = sqlPreview.Replace("\r\n", " ");
                    sqlPreview = sqlPreview.Replace("\r", " ");
                    sqlPreview = sqlPreview.Replace("\n", " ");
                    row.RowTextCell.Value = sqlPreview;
                    row.RowTextCell.Style = iGrid3RowTextColCellStyle;
                    row.RowTextCell.ForeColor = m_sqlPreviewColor;
                }
                else
                {
                    row.Height = _iGridNormalRowHeight;
                }
            }
        }
        public void applyFilterToTriageData()
        {
            //get the current filter criteria

        }

        public DataTable GetFilteredData(String theFilter, Hashtable showQueries_ht)
        {
            DataTable originalTable = ((DatabaseDataProvider)_theTriageGridUserControl.DataProvider).GetDataTable();
            if (originalTable != null)
            {
                DataTable m_dataTable = getDataTableCopy(originalTable);
                DataTable dtFiltered = null;
                showQueries_ht.Clear();

                if ((null != theFilter) && (0 < theFilter.Length) && (null != m_dataTable) && (m_dataTable.Columns.Count > 0))
                {
                    dtFiltered = m_dataTable.Clone();
                    showQueries_ht.Add("MXID01DUMMY^_^1234567", null);
                    theFilter = theFilter.Replace("UPPER(APPLICATION_NAME)", "APPLICATION_NAME");
                    theFilter = theFilter.Replace("UPPER(CLIENT_NAME)", "CLIENT_NAME");
                    theFilter = theFilter.Replace("UPPER(USER_NAME)", "USER_NAME");
                    DataRow[] dataRows = m_dataTable.Select(theFilter, "START_TIME");
                    foreach (DataRow row in dataRows)
                    {
                        try
                        {
                            String qid = (String)row["QUERY_ID"];
                            String entryID = "";
                            //if (this._wsOptions.NPASkin)
                            entryID = row["ENTRY_ID"].ToString().Trim();

                            String htKey = qid.Trim() + "^_^" + entryID;
                            if (_theTriageGridUserControl.TriageHelper.HiddenQIDs_ht.ContainsKey(htKey) || showQueries_ht.ContainsKey(htKey))
                                continue;

                            showQueries_ht.Add(htKey, row);
                            dtFiltered.ImportRow(row);

                        }
                        catch (Exception)
                        {
                        }
                    }
                }
                else
                {
                    dtFiltered = m_dataTable;
                }

                return dtFiltered;
            }
            else
            {
                return null;
            }
        }
        public void applyFilterToTriageData(String theFilter)
        {
            Hashtable showQueries_ht = new Hashtable();
            DataTable dtFiltered = GetFilteredData(theFilter, showQueries_ht);
            if (dtFiltered != null)
            {
                TabularDataDisplayControl dataDisplayControl = (TabularDataDisplayControl)_theTriageGridUserControl.GridWidget.DataDisplayControl;
                dataDisplayControl.PersistConfiguration();
                foreach (iGRow gridRow in dataDisplayControl.DataGrid.Rows)
                {
                    String qryID = gridRow.Cells["QUERY_ID"].Value.ToString().Trim();
                    String entryID = gridRow.Cells["ENTRY_ID"].Value.ToString().Trim();
                    String gridRowKey = qryID + "^_^" + entryID;

                    gridRow.Visible = (0 >= showQueries_ht.Count) || showQueries_ht.ContainsKey(gridRowKey);
                }

                showQueries_ht.Clear();
                populate(_theTriageGridUserControl.UniversalWidgetConfiguration, dtFiltered, ((TabularDataDisplayControl)_theTriageGridUserControl.GridWidget.DataDisplayControl).DataGrid);
                //currentDataTable = dtFiltered;
            }
        }

        public void UpdateGraphWidget()
        {
            UpdateGraphWidget(currentDataTable);
        }
        //Populates the UI
        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            bool isSession = ((TriageDataProvider)_theTriageGridUserControl.DataProvider).IsSessionData;
            lock (this)
            {
                if (isSession)
                {
                    if (currentDataTable == null)
                    {
                        currentDataTable = aDataTable;
                    }
                    ShowSessionQueries(aConfig, aDataTable, aDataGrid);
                    ((TriageDataProvider)_theTriageGridUserControl.DataProvider).IsSessionData = false;
                }
                else
                {
                    //populate(aConfig, aDataTable, aDataGrid);
                    _theTriageGridUserControl.TriageHelper.TriageFilterPropertyGrid.applyFilterToDataTable();
                }
            }
        }

        public static void CheckChangeCellRowColor(iGRow row)
        {

            try
            {
                string CellCols = row.Cells["ViolatorColor"].Value.ToString();
                if (CellCols != "")
                {
                    string CellCol = "";
                    String CellNames = row.Cells["ViolatorNames"].Value.ToString();
                    string[] CellNamesArray = CellNames.Split('|');
                    string[] ColorArray = CellCols.Split('|');

                    for (int i = 0; i < ColorArray.Length; i++)  //foreach (string nameArrays in CellNamesArray)
                    {
                        string nameArrays = CellNamesArray[i];
                        CellCol = ColorArray[i];
                        string[] CellNameArray = nameArrays.Split(',');
                        foreach (string name in CellNameArray)
                        {
                            if (name != "")
                                ChangeCellRowColor(row, name, Color.FromArgb(int.Parse(CellCol)));
                        }
                    }
                }

            }
            catch (Exception)
            {
            }

        }

        public static void ChangeCellRowColor(iGRow row, string name, Color c)
        {
            try
            {
                row.Cells[name].BackColor = c;

            }
            catch (Exception)
            {
            }
        }

        public static void ChangeDataRowBackColor(iGRow row, Color c)
        {
            try
            {
                int len = row.Cells.Count;
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].BackColor = c;
            }
            catch (Exception)
            {
            }
        }

        public static void ChangeDataRowColor(iGRow row, Color c)
        {
            int len = row.Cells.Count;

            try
            {
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].ForeColor = c;

            }
            catch (Exception)
            {
            }

        }
        #endregion Public methods

        #region Private methods

        private void setupDisplayTimeOffsetFromGMT(DateTime lctTime, DateTime gmtTime, bool saveInfo)
        {
            if (0 >= lctTime.CompareTo(gmtTime))
            {
                this._displayTimeOffsetFromGMT = lctTime.Subtract(gmtTime);
                this._timeOffsetGMTAdd = true;
            }
            else
            {
                this._displayTimeOffsetFromGMT = gmtTime.Subtract(lctTime);
                this._timeOffsetGMTAdd = false;
            }


            if (saveInfo)
            {
                this._savedTrafodionDisplayTimeOffsetFromGMT = this._displayTimeOffsetFromGMT;
                this._savedTrafodionTimeOffsetGMTAdd = this._timeOffsetGMTAdd;
            }

        }
        private String getInViewQueryGridRowKey(iGrid TriageIGridControl)
        {
            String gridRowKey = "";

            if (null == TriageIGridControl.CurCell)
                return gridRowKey;


            try
            {
                iGRow currentRow = TriageIGridControl.CurCell.Row;
                String qryID = currentRow.Cells["QUERY_ID"].Value.ToString().Trim();
                String entryID = currentRow.Cells["ENTRY_ID"].Value.ToString().Trim();
                gridRowKey = qryID + "^_^" + entryID;

            }
            catch (Exception)
            {
            }

            return gridRowKey;
        }
        private void calculateDisplayTimeOffsetFromGMT(bool fetchTimeInfo)
        {
            DateTime lctTime = DateTime.Now;
            DateTime savedLctTime = lctTime;
            DateTime nowAtGMT = lctTime.ToUniversalTime();

            if (fetchTimeInfo)
            {
                try
                {
                    String timeStr = _theTriageGridUserControl.TriageHelper.GetTrafodionSystemTime();
                    lctTime = DateTime.Parse(timeStr);

                }
                catch (Exception)
                {
                }

                setupDisplayTimeOffsetFromGMT(lctTime, DateTime.Now.ToUniversalTime(), true);
            }


            //if (false == this._wsOptions.ShowTimesLocally)
            //{
            //    this._displayTimeOffsetFromGMT = this._savedTrafodionDisplayTimeOffsetFromGMT;
            //    this._timeOffsetGMTAdd = this._savedTrafodionTimeOffsetGMTAdd;
            //    return;
            //}

            lctTime = savedLctTime;
            setupDisplayTimeOffsetFromGMT(lctTime, nowAtGMT, false);
        }

        public DateTime getDisplayTimeFromGMT(DateTime t)
        {
            if (t != null)
            {
                if (this._timeOffsetGMTAdd)
                    return t.Add(this._displayTimeOffsetFromGMT);

                return t.Subtract(this._displayTimeOffsetFromGMT);
            }
            return t;
        }

        private void ShowSessionQueries(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                        Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            DataTable dt = aDataTable;
            foreach (DataRow row in dt.Rows)
            {
                try
                {
                    String queryID = (String)row["QUERY_ID"];
                    String entryID = row["ENTRY_ID"].ToString().Trim();
                    String htKey = queryID.Trim() + "^_^" + entryID;
                    _theTriageGridUserControl.TriageHelper.HiddenQIDs_ht.Remove(htKey);

                }
                catch (Exception)
                {
                }
            }

            addToTriageDataGrid(dt);
            //base.DoPopulate(aConfig, DisplayDataTable, aDataGrid);
            populate(aConfig, DisplayDataTable, aDataGrid);
        }
        private void addToTriageDataGrid(DataTable dt)
        {

            DataTable triageLoadDataTable = null;
            DataTable _filteredDataTable = DisplayDataTable;
            addOrUpdateTriageSpaceQueries(dt, _filteredDataTable);

        }

        private void addOrUpdateTriageSpaceQueries(DataTable fetched_dt, DataTable destination_dt)
        {
            if ((null == destination_dt) || (fetched_dt == destination_dt))
                return;

            try
            {
                Hashtable existingQueries_ht = new Hashtable();
                foreach (DataRow aRow in destination_dt.Rows)
                {
                    String qryID = aRow["QUERY_ID"].ToString();
                    String entryID = aRow["ENTRY_ID"].ToString();

                    String htKey = qryID.Trim() + "^_^" + entryID.Trim();
                    if (!existingQueries_ht.ContainsKey(htKey))
                        existingQueries_ht.Add(htKey, aRow);

                }

                int numConsecutiveErrorsUsingItemArray = 0;

                foreach (DataRow fetchedRow in fetched_dt.Rows)
                {
                    String fetchedQryKey = "";

                    try
                    {
                        String fetchedQryID = fetchedRow["QUERY_ID"].ToString();
                        String fetchedEntryID = fetchedRow["ENTRY_ID"].ToString();

                        fetchedQryKey = fetchedQryID.Trim() + "^_^" + fetchedEntryID.Trim();

                        if (existingQueries_ht.ContainsKey(fetchedQryKey))
                        {
                            DataRow existingRow = (DataRow)existingQueries_ht[fetchedQryKey];
                            existingRow.ItemArray = fetchedRow.ItemArray;
                        }
                        else
                        {
                            DataRow brandNewRow = destination_dt.NewRow();

                            if (5 > numConsecutiveErrorsUsingItemArray)
                            {
                                try
                                {
                                    brandNewRow.ItemArray = fetchedRow.ItemArray;
                                    destination_dt.Rows.Add(brandNewRow);

                                    // Reset the error counter, its not a consecutive error.
                                    numConsecutiveErrorsUsingItemArray = 0;
                                    continue;

                                }
                                catch (Exception)
                                {
                                    numConsecutiveErrorsUsingItemArray++;
                                }
                            }

                            foreach (DataColumn dc in fetched_dt.Columns)
                            {
                                try
                                {
                                    if (destination_dt.Columns.Contains(dc.ColumnName))
                                        brandNewRow[dc.ColumnName] = fetchedRow[dc.ColumnName];

                                }
                                catch (Exception)
                                {
                                }
                            }
                            destination_dt.Rows.Add(brandNewRow);
                        }
                    }
                    catch (Exception exc)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.Monitoring,
                            TriageHelper.TRACE_SUB_AREA_NAME,
                            "addOrUpdateTriageSpaceQueries(): Error adding " +
                                                    "or updating a query. QID_EID='" +
                                                    fetchedQryKey + "'. Error details = " + exc.Message);

                    }
                }
            }
            catch (Exception e)
            {
                Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.Monitoring,
                            TriageHelper.TRACE_SUB_AREA_NAME,
                            "addOrUpdateTriageSpaceQueries(): Error adding " +
                                                "or updating queries, details = " + e.Message);
            }
        }

        private void UpdateGraphWidget(DataTable aDataTable)
        {
            _theTriageGridUserControl.UpdateGraphWidget(aDataTable);
        }

        private void computeRowHeightForSQLPreview()
        {
            iGrid3RowTextColCellStyle = new iGCellStyle();
            this.iGrid3RowTextColCellStyle.ContentIndent = new iGIndent(3, 3, 3, 3);
            this.iGrid3RowTextColCellStyle.TextFormatFlags = iGStringFormatFlags.WordWrap;
            this.iGrid3RowTextColCellStyle.Font = new System.Drawing.Font("Tahoma", 8.25F, FontStyle.Regular);
            this.iGrid3RowTextColCellStyle.ForeColor = Color.Gray;

            TrafodionIGrid tempGrid = new TrafodionIGrid();
            TenTec.Windows.iGridLib.iGCol sqlTextCol = tempGrid.Cols.Add("SQL_TEXT", "SQL_TEXT");

            TenTec.Windows.iGridLib.iGRow row = tempGrid.Rows.Add();
            iGCell cell = row.Cells[0];

            cell.Value = "SET SCHEMA  TRAFODION.USER_SCHEMA";

            tempGrid.RowTextVisible = true;
            row.RowTextCell.Style = tempGrid.GroupRowLevelStyles[0];

            row.RowTextCell.Value = " First Line #1 " + Environment.NewLine + " Second Line #2 ";
            row.AutoHeight();
            this._iGridRowHeightWithPreview = (int)(1.15 * row.Height);

            tempGrid.RowTextVisible = false;
            row.AutoHeight();
            this._iGridNormalRowHeight = (int)(1.15 * row.Height);
        }

        public static DataTable GetProcessedDataTable(DataTable rawDataTable)
        {
            foreach (DataRow dr in rawDataTable.Rows)
            {
                calculateSingleRowPerQueryCounters(dr);
            }
            DataTable aggrDataTable = getSingleRowPerQueryWithSQLText(rawDataTable);
            cleanupRunningQueriesDataTable(aggrDataTable);
            return aggrDataTable;
        }

        public static DataTable GetDataTableForDisplay(DataTable rawDataTable)
        {
            DataTable aggrDataTable = getSingleRowPerQueryWithSQLText(rawDataTable);
            cleanupRunningQueriesDataTable(aggrDataTable);
            return aggrDataTable;
        }

        private void setupQueriesInTriageIGrid(iGrid TriageIGridControl)
        {
            MonitorWorkloadOptions options = MonitorWorkloadOptions.GetOptions();
            bool showRowText = (_theTriageGridUserControl != null) ? _theTriageGridUserControl.ShowSqlPreview() : false;
            TriageIGridControl.RowTextVisible = showRowText;

            setIGridColumnFormatTypes(TriageIGridControl);

            foreach (iGRow row in TriageIGridControl.Rows)
            {
                try
                {
                    row.RowTextCell.Value = row.Cells["SQL_TEXT"].Value;
                    row.RowTextCell.ForeColor = Color.Gray;

                    String errorCode = "";
                    try
                    {
                        errorCode = row.Cells["ERROR_CODE"].Value.ToString();
                    }
                    catch (Exception)
                    {
                    }

                    if (queryHadErrors(errorCode))
                        ChangeDataRowColor(row, options.RejectedColor);
                    else
                    {
                        String qryState = row.Cells["STATE"].Value.ToString().Trim().ToUpper();
                        if (TriageGridUserControl.RUNNING.Equals(qryState))
                            ChangeDataRowColor(row, options.ExecutingColor);
                        else if ("ABNORMALLY TERMINATED".Equals(qryState))
                            ChangeDataRowColor(row, options.RejectedColor);
                        else if (TriageGridUserControl.HOLD.Equals(qryState) || TriageGridUserControl.SUSPENDED.Equals(qryState))
                            ChangeDataRowColor(row, options.HoldingColor);
                        else if (TriageGridUserControl.HOLD.Equals(qryState) || TriageGridUserControl.COMPLETED.Equals(qryState))
                            ChangeDataRowColor(row, options.CompletedColor);

                    }

                    if (showRowText)
                        row.Height = this._iGridRowHeightWithPreview;
                    else
                        row.Height = this._iGridNormalRowHeight;

                    if (this._theTriageGridUserControl.ClientRulesEnabled)
                    {
                        //Check for Threshold Color indicators on this row
                        String BackCol = row.Cells["BackgroundColor"].Value.ToString();
                        if (BackCol != "")
                            ChangeDataRowBackColor(row, Color.FromArgb(int.Parse(BackCol)));

                        String ForeCol = row.Cells["ForegroundColor"].Value.ToString();
                        if (ForeCol != "")
                            ChangeDataRowColor(row, Color.FromArgb(int.Parse(ForeCol)));

                        //then apply the color settings for each cell.
                        CheckChangeCellRowColor(row);
                    }

                }
                catch (Exception)
                {
                }
            }

        }

        private void setupRowColors()
        {

        }

        private void setIGridColumnFormatTypes(iGrid TriageIGridControl)
        {
            String[] numericColsList = {"START_PRIORITY", "QUERY_ELAPSED_TIME", "MASTER_EXECUTOR_TIME", 
										"DISK_READS", /*"LOCKESCALATIONS",*/ "LOCKWAITS", "MESSAGE_BYTES_TO_DISK", 
									    "MESSAGES_TO_DISK", "ROWS_ACCESSED", "ROWS_RETRIEVED", "NUM_ROWS_IUD", 
                                        //"TOTAL_EXECUTES", "TOTAL_ELAPSED_TIME", "TOTAL_EXECUTION_TIME",
 
										 /*  WMS columns.  */
										 "CARDINALITY_ESTIMATE", "USED_ROWS", 
										 "ROWS_ACCESS_ESTIMATE", "ROWS_USAGE_ESTIMATE", "TOTAL_PROCESSOR_TIME",
										 "DELTA_PROCESSOR_TIME", "DELTA_SPACE_ALLOC_MB", "DELTA_SPACE_USED_MB",
									     "DELTA_DISK_READS", "DELTA_IUD",
										 "RESOURCE_USAGE_ESTIMATE", "CMP_AFFINITY_NUM", "CMP_DOP",
									     "CMP_TXN_NEEDED", "CMP_MANDATORY_X_PROD", "CMP_MISSING_STATS",
										 "CMP_NUM_JOINS", "CMP_FULL_SCAN_ON_TABLE", "COMPILATION_HIGH_ESAM_MAX_BUFFER_USAGE",
										 "TOTAL_MEMORY_ALLOCATED", "TOTAL_SPACE_ALLOC_KB", "TOTAL_SPACE_USED_KB",
                                         "STATS_BYTES", "DISK_IOS", "LOCK_WAITS", "LOCK_ESCALATIONS", "OPENS",
										 "PROCESSES_CREATED", "DISK_PROCESS_BUSYTIME", "SQL_CPU_TIME", 
										 "PROCESS_CREATE_TIME", "OPEN_TIME", 
										 "SQL_SPACE_ALLOC", "SQL_SPACE_USED", "SQL_HEAP_ALLOC", "SQL_HEAP_USED",
										 "EID_SPACE_ALLOC", "EID_SPACE_USED", "EID_HEAP_ALLOC", "EID_HEAP_USED",
										 "TOTAL_PROCESSOR_TIME_MICROSECONDS" };


            Hashtable formatColsHT = new Hashtable();
            String integerValueNumberFormat = TriageHelper.getNumberFormatForCurrentLocale(0);

            foreach (String colName in numericColsList)
                formatColsHT.Add(colName, integerValueNumberFormat);

            String precision2NumberFormat = TriageHelper.getLocaleNumberFormat(2, true);
            formatColsHT.Add("TOTAL_MEMORY_ESTIMATE", precision2NumberFormat);
            formatColsHT.Add("CMP_ROWS_ACCESSED_FULL_SCAN", precision2NumberFormat);
            formatColsHT.Add("CMP_EID_ROWS_ACCESSED", precision2NumberFormat);
            formatColsHT.Add("CMP_EID_ROWS_USED", precision2NumberFormat);
            formatColsHT.Add("ROWS/SECOND", precision2NumberFormat);
            formatColsHT.Add("IUD/SECOND", precision2NumberFormat);

            formatColsHT.Add("PROCESSOR_USAGE/SECOND", precision2NumberFormat);

            formatColsHT.Add("SPACE_ALLOCATION_GB", precision2NumberFormat);
            formatColsHT.Add("SPACE_USAGE_GB", precision2NumberFormat);

            String precision4NumberFormat = TriageHelper.getLocaleNumberFormat(4, true);
            formatColsHT.Add("ESTIMATED_COST", precision4NumberFormat);
            formatColsHT.Add("CPU_TIME_ESTIMATE", precision4NumberFormat);
            formatColsHT.Add("IO_TIME_ESTIMATE", precision4NumberFormat);
            formatColsHT.Add("MSG_TIME_ESTIMATE", precision4NumberFormat);
            formatColsHT.Add("IDLE_TIME_ESTIMATE", precision4NumberFormat);

            String precision8NumberFormat = TriageHelper.getLocaleNumberFormat(8, true);
            formatColsHT.Add("ESTIMATED_TOTAL_TIME", precision8NumberFormat);


            foreach (iGCol col in TriageIGridControl.Cols)
            {
                String name = col.Key;
                if (formatColsHT.ContainsKey(name))
                {
                    String fmt = (String)formatColsHT[name];
                    if ((null != fmt) && (0 < fmt.Trim().Length))
                        col.CellStyle.FormatString = fmt;

                }
            }

        }

        private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            //foreach (DataRow dr in aDataTable.Rows)
            //{
            //    calculateSingleRowPerQueryCounters(dr);
            //}

            try
            {
                numRunning = aDataTable.Compute("COUNT(STATE)", "STATE = 'Running'").ToString();

                String completedFilter = "STATE = 'Completed' AND (ERROR_CODE = '0'  OR  ERROR_CODE = '')";
                numCompleted = aDataTable.Compute("COUNT(STATE)", completedFilter).ToString();

                //if (this._wsOptions.HideRecentlyCompletedQueries)
                //    numCompleted = _numRecentlyCompletedQueries.ToString();

                String waitingOrInErrorFilter = "STATE = 'Waiting'  OR  " +
                                                "STATE = 'Error'  OR  " +
                                                "STATE = 'Abnormally Terminated'  OR  " +
                                                "(STATE = 'Completed'  AND  ERROR_CODE <> '0'  AND  ERROR_CODE <> '')";

                numWaitingOrInError = aDataTable.Compute("COUNT(STATE)", waitingOrInErrorFilter).ToString();
                numHold = aDataTable.Compute("COUNT(STATE)", "STATE = 'Hold'  OR  STATE = 'Suspended'").ToString();

            }
            catch (Exception e)
            {

            }

            //DataTable aggrDataTable = GetDataTableForDisplay(aDataTable);

            //getSingleRowPerQueryWithSQLText(aDataTable);
            //cleanupRunningQueriesDataTable(aggrDataTable);
            DataTable aggrDataTable = getDataTableCopy(aDataTable);
            //calculateDisplayTimeOffsetFromGMT(true);
            //foreach (DataRow dr in aggrDataTable.Rows)
            //{
            //    DateTime gmtTime;
            //    if (dr["START_TIME"] is DateTime)
            //    {
            //        gmtTime = (DateTime)dr["START_TIME"];
            //        dr["START_TIME"] = getDisplayTimeFromGMT(gmtTime);
            //    }

            //    if (dr["ENTRY_TIME"] is DateTime)
            //    {
            //        gmtTime = (DateTime)dr["ENTRY_TIME"];
            //        dr["ENTRY_TIME"] = getDisplayTimeFromGMT(gmtTime);
            //    }

            //    if (dr["END_TIME"] is DateTime)
            //    {
            //        gmtTime = (DateTime)dr["END_TIME"];
            //        dr["END_TIME"] = getDisplayTimeFromGMT(gmtTime);
            //    }
            //}

            if (this._theTriageGridUserControl.ClientRulesEnabled && (null != aggrDataTable))
            {
                ClientQueryRuler.Instance.SetupOperatingTable(this._theTriageGridUserControl.ConnectionDefinition, aggrDataTable);
                if (aggrDataTable.Rows.Count > 0)
                {
                    ClientQueryRuler.Instance.EvaluateEnabledRules(this._theTriageGridUserControl.ConnectionDefinition, false);
                    ClientQueryRuler.Instance.RuleViolatorPopup.RuleViolatorCellClick += new EventHandler(RuleViolatorPopup_RuleViolatorCellClick);
                }
            }


            base.DoPopulate(aConfig, aggrDataTable, aDataGrid);

            FormatIGridDatetimeColumns(aDataGrid);

            setupQueriesInTriageIGrid(aDataGrid);
            _theTriageGridUserControl.enableDisableButtons();
            string gridHeaderText;
            gridHeaderText = string.Format("Running ({0}) Completed ({1}) Error ({2})",
                                            numRunning,
                                            numCompleted,
                                            numWaitingOrInError);
            aDataGrid.UpdateCountControlText(gridHeaderText);
            UpdateGraphWidget(aggrDataTable);
            currentDataTable = aggrDataTable;
        }

        private void FormatIGridDatetimeColumns(TrafodionIGrid aDataGrid)
        {

            foreach (iGCol column in aDataGrid.Cols)
            {

                if (column.CellStyle.ValueType == typeof(System.DateTime))
                {
                    //Display datetime using TrafodionManager standard datetime format                   
                    column.CellStyle.FormatProvider = _theTriageGridUserControl.TheDateTimeFormatProvider;
                    column.CellStyle.FormatString = "{0}";
                }
            }
        }
        private void RuleViolatorPopup_RuleViolatorCellClick(object sender, EventArgs e)
        {
            if (e is Trafodion.Manager.WorkloadArea.Controls.ClientRuleViolators.ViolatorCellClickEventArgs)
            {
                Trafodion.Manager.WorkloadArea.Controls.ClientRuleViolators.ViolatorCellClickEventArgs violatorCellClickEventArgs =
                    e as Trafodion.Manager.WorkloadArea.Controls.ClientRuleViolators.ViolatorCellClickEventArgs;
                if (violatorCellClickEventArgs.IsLiveView == false)
                {
                    this._theTriageGridUserControl.HighlightQuery(violatorCellClickEventArgs.QueryID, violatorCellClickEventArgs.QueryStartTime);
                }
            }
        }


        private static String mapQueryStateForSingleRowPerQuery(String queryStatus)
        {

            try
            {
                String theState = queryStatus.Trim().ToUpper();
                if (stateMapping_ht.ContainsKey(theState))
                    return stateMapping_ht[theState].ToString();

            }
            catch (Exception)
            {
            }

            return queryStatus;
        }

        private static void calculateSingleRowPerQueryCounters(DataRow theRow)
        {
            String theState = theRow["STATE"].ToString();
            if (false == "Error".Equals(theState))
            {
                theState = mapQueryStateForSingleRowPerQuery(theRow["QUERY_STATUS"].ToString());

                try
                {
                    // Fixup 1 row per query data issues.
                    if (TriageGridUserControl.RUNNING.Equals(theState.ToUpper().Trim()) &&
                        "CLOSE".Equals(theRow["QUERY_EXECUTION_STATE"].ToString().ToUpper().Trim()))
                        theState = "Completed";

                    if ((null != theRow["END_TIME"]) &&
                        !String.IsNullOrEmpty(theRow["END_TIME"].ToString().Trim()))
                        theState = "Completed";

                }
                catch (Exception)
                {
                }

                theRow["STATE"] = theState;
            }

            try
            {
                String killerName = theRow["APPLICATION_NAME"].ToString().Trim();
                if (killerName.Equals(SUSPEND_RESUME_KILL_SPJ_APPLICATION_ID))
                    theRow["STATE"] = "Abnormally Terminated";

            }
            catch (Exception)
            {
            }


            String elapsedTime = "";
            try
            {
                if ((theRow["END_TIME"] is DateTime) && (theRow["START_TIME"] is DateTime))
                {
                    DateTime endTime = (DateTime)theRow["END_TIME"];
                    DateTime startTime = (DateTime)theRow["START_TIME"];
                    elapsedTime = TriageHelper.getFormattedElapsedTime(startTime, endTime);
                }

            }
            catch (Exception)
            {
            }

            theRow["ELAPSED_TIME"] = elapsedTime;
        }

        private static DataTable getDataTableCopy(DataTable dt)
        {
            DataTable aggregatedDataTable = cloneDataTableColumns(dt);
            if (dt != null)
            {
                DataRow aggregatedRow = null;
                for (int idx = 0; idx < dt.Rows.Count; idx++)
                {
                    DataRow row = dt.Rows[idx];
                    aggregatedRow = aggregatedDataTable.NewRow();
                    for (int itemIdx = 0; itemIdx < row.ItemArray.Length; itemIdx++)
                    {
                        try
                        {
                            aggregatedRow[row.Table.Columns[itemIdx].ColumnName] = row[itemIdx];
                        }
                        catch (Exception)
                        {
                        }
                    }
                    aggregatedDataTable.Rows.Add(aggregatedRow);
                    aggregatedRow = null;
                }
            }
            return aggregatedDataTable;
        }

        private static DataTable getSingleRowPerQueryWithSQLText(DataTable dt)
        {
            DataTable aggregatedDataTable = cloneDataTableColumns(dt);
            String previousSessionID = "";
            String previousQID = "";
            int prevSequenceNum = -1;
            DataRow aggregatedRow = null;
            DateTime currentTimeAtGMT = DateTime.Now.ToUniversalTime();

            Hashtable skipIndex_ht = new Hashtable();

            try
            {
                DataColumn sqlTextCol = dt.Columns["SQL_TEXT"];
                skipIndex_ht.Add(sqlTextCol.Ordinal.ToString(), sqlTextCol.ColumnName);

            }
            catch (Exception)
            {
            }

            foreach (DataColumn dc in dt.Columns)
            {
                if (false == aggregatedDataTable.Columns.Contains(dc.ColumnName))
                    skipIndex_ht.Add(dc.Ordinal.ToString(), dc.ColumnName);

            }

            for (int idx = 0; idx < dt.Rows.Count; idx++)
            {
                DataRow row = dt.Rows[idx];

                try
                {
                    String sessionID = (String)row["SESSION_ID"];
                    String qid = (String)row["QUERY_ID"];
                    int sequence_num = 0;// Int16.Parse(row["SEQUENCE_NUM"].ToString());
                    //DateTime queryStartTime = (DateTime) row["ENTRY_TIME"];
                    DateTime queryStartTime = new DateTime();
                    //if (!row.IsNull("ENTRY_TIME"))
                    //{
                      //  queryStartTime = (DateTime)row["ENTRY_TIME"];
                    //}
                    sessionID = sessionID.Trim();
                    qid = qid.Trim();
                    if (0 >= qid.Length)
                        continue;   //  Skip over empty QIDs. Data quality issues. 

                    if ((false == qid.Equals(previousQID)) || (0 == sequence_num))
                    {
                        /**
						 *  The Query ID is different - a new query, so we need to get the end of 
						 *  the previous query. We do this only if we have an aggregatedRow though.
						 */
                        if (null != aggregatedRow)
                        {
                            calculateSingleRowPerQueryCounters(aggregatedRow);
                            aggregatedDataTable.Rows.Add(aggregatedRow);
                            aggregatedRow = null;
                            prevSequenceNum = -1;
                            previousQID = "";
                            previousSessionID = "";
                        }

                        if (null == aggregatedRow)
                            aggregatedRow = aggregatedDataTable.NewRow();

                        prevSequenceNum = sequence_num;
                        previousSessionID = sessionID;
                        previousQID = qid;

                        for (int itemIdx = 0; itemIdx < row.ItemArray.Length; itemIdx++)
                        {
                            try
                            {
                                if (false == skipIndex_ht.ContainsKey(itemIdx.ToString()))
                                    aggregatedRow[row.Table.Columns[itemIdx].ColumnName] = row[itemIdx];

                            }
                            catch (Exception)
                            {
                            }

                        }
                    }  /*  End of IF it is a query START event.  */

                    assembleQueryStartDetails(row, aggregatedRow, queryStartTime);
                }
                catch (Exception e)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Monitoring,
                        TriageHelper.TRACE_SUB_AREA_NAME,
                        "getSingleRowPerQueryWithSQLText  exception : " + e.ToString());

                    Console.WriteLine(e.ToString());
                    aggregatedRow = null;
                    prevSequenceNum = -1;
                    previousQID = "";
                }

            }


            if (null != aggregatedRow)
            {
                /**
                 *  Have one row without and ending record -- see if we can find it. If not, then add
                 *  a row indicating a running query. 
                 */
                calculateSingleRowPerQueryCounters(aggregatedRow);
                aggregatedDataTable.Rows.Add(aggregatedRow);
                aggregatedRow = null;
            }


            return aggregatedDataTable;

        }

        private static void assembleQueryStartDetails(DataRow row, DataRow aggregatedRow, DateTime currentSysTime)
        {
            if ((null == row) || (null == aggregatedRow))
                return;


            int sequence_num = 0;// Int32.Parse(row["SEQUENCE_NUM"].ToString());

            String theQueryStringBits = "";
            theQueryStringBits = row["SQL_TEXT"].ToString();
#if DISABLE_CHUNKING
			theQueryStringBits = row["SQL_TEXT"].ToString();
#else
            //  Work around an ODBC DriverManager or .NET bug.
            //  DriverManager/ODBC.NET provider uses 1K buffers for Unicode - so split the text up.
            /*for (int idx = 1; idx <= TriageDataProvider.MAX_SQLTEXT_CHUNKS; idx++)
            {
                int chunkStart = (idx - 1) * TriageDataProvider.SQLTEXT_CHUNK_SIZE;
                theQueryStringBits += row["SQL_TEXT CHUNK_ID_" + idx].ToString();
            }
            */
#endif


            if (0 < sequence_num)
            {
                Object sqlText = aggregatedRow["SQL_TEXT"];

                if ((null != sqlText) && (0 < sqlText.ToString().Length))
                {
                    aggregatedRow["SQL_TEXT"] += theQueryStringBits;
                    return;
                }

                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "**** Aaaaargh!!  Repository Data Quality issue. SQLText is blank " +
                    "(missing sequence # 0 record). \nQuery ID = " + row["QUERY_ID"] +
                    " Sequence #" + sequence_num + "  should have appended BUT now setting " +
                    "query text to \n" + theQueryStringBits);
            }

            aggregatedRow["QUERY_ID"] = row["QUERY_ID"];
            aggregatedRow["SESSION_ID"] = row["SESSION_ID"];
            aggregatedRow["CURRENT_SYSTEM_TIME"] = currentSysTime;
            //aggregatedRow["START_TIME"] = (DateTime)row["ENTRY_TIME"];
            aggregatedRow["ELAPSED_TIME"] = "";
            aggregatedRow["STATE"] = "Running";
            aggregatedRow["USER_NAME"] = row["USER_NAME"];
            aggregatedRow["APPLICATION_NAME"] = row["APPLICATION_NAME"];
            aggregatedRow["CLIENT_ID"] = row["CLIENT_ID"];
            //aggregatedRow["DATASOURCE"] = row["DATASOURCE"];

            aggregatedRow["SQL_TYPE"] = mapQueryToStatementType(theQueryStringBits);

            String errCode = row["ERROR_CODE"].ToString();
            errCode = Regex.Replace(errCode, @"\D", "");
            if (queryHadErrors(errCode))
                aggregatedRow["STATE"] = "Error";

            aggregatedRow["ERROR_CODE"] = errCode;
            aggregatedRow["ESTIMATED_COST"] = row["ESTIMATED_COST"];
            aggregatedRow["START_PRIORITY"] = row["START_PRIORITY"];
            aggregatedRow["QUERY_ELAPSED_TIME"] = row["QUERY_ELAPSED_TIME"];
            aggregatedRow["MASTER_EXECUTOR_TIME"] = row["MASTER_EXECUTOR_TIME"];
            aggregatedRow["DISK_READS"] = row["DISK_READS"];
            //aggregatedRow["LOCKESCALATIONS"] = row["LOCKESCALATIONS"];
            aggregatedRow["LOCKWAITS"] = row["LOCKWAITS"];
            aggregatedRow["MESSAGE_BYTES_TO_DISK"] = row["MESSAGE_BYTES_TO_DISK"];
            aggregatedRow["MESSAGES_TO_DISK"] = row["MESSAGES_TO_DISK"];
            aggregatedRow["ROWS_ACCESSED"] = row["ROWS_ACCESSED"];
            aggregatedRow["ROWS_RETRIEVED"] = row["ROWS_RETRIEVED"];
            aggregatedRow["NUM_ROWS_IUD"] = row["NUM_ROWS_IUD"];
            //aggregatedRow["TOTAL_EXECUTES"] = row["TOTAL_EXECUTES"];
            aggregatedRow["SQL_TEXT"] = theQueryStringBits;
            //aggregatedRow["SEGMENT_ID"] = row["SEGMENT_ID"];
            aggregatedRow["NODE_ID"] = row["NODE_ID"];
            //aggregatedRow["PIN"] = row["PIN"];
            aggregatedRow["MASTER_PROCESS_ID"] = row["MASTER_PROCESS_ID"];
            //Change Process_Name to Process_ID Column Name Master_Process_Name is changed to Master_Process_ID from 2011.1.9
            aggregatedRow["PROCESS_ID"] = row["PROCESS_ID"];
            aggregatedRow["ENTRY_ID"] = row["ENTRY_ID"];
            //aggregatedRow["ENTRY_TIME"] = row["ENTRY_TIME"];
            aggregatedRow["SEQUENCE_NUM"] = 0;
            //aggregatedRow["STATEMENT_STATUS"] = row["STATEMENT_STATUS"];
            //aggregatedRow["TOTAL_ELAPSED_TIME"] = row["TOTAL_ELAPSED_TIME"];
            //aggregatedRow["TOTAL_EXECUTION_TIME"] = row["TOTAL_EXECUTION_TIME"];

        }

        private static String mapQueryToStatementType(String qryStart)
        {
            if (null == qryStart)
                return "UNKNOWN";

            qryStart = qryStart.Trim();
            qryStart = Regex.Replace(qryStart, @"(/\*(.*)?\*/|--(.*)?\n)", "");
            qryStart = qryStart.Trim().ToUpper();

            String[] tokens = qryStart.Split(new char[] { '\t', '\r', '\n', ' ', '{' },
                                             StringSplitOptions.RemoveEmptyEntries);

            if (tokens.Length > 0)
            {
                String cmd = tokens[0].ToUpper();

                if (cmd.Equals("LOCKING"))
                {
                    String sqlObjectTypeRE = @"((DATABASE|TABLE|VIEW|\s*)(.*)?|ROW)";
                    String sqlObjectLockModeRE = @"(ACCESS|EXCLUSIVE|EXCL|SHARE|READ|READ\s+OVERRIDE|WRITE|CHECKSUM)";
                    String sqlObjectLockRE = @"(LOCKING\s+" + sqlObjectTypeRE + @"\s+(FOR|IN)\s+" +
                                              sqlObjectLockModeRE + @"\s+(MODE\s+NOWAIT|\s*))";

                    try
                    {
                        qryStart = Regex.Replace(qryStart, sqlObjectLockRE, "");
                        tokens = qryStart.Split(new char[] { '\t', '\r', '\n', ' ', '{' });
                        if (tokens.Length > 0)
                            cmd = tokens[0].ToUpper();

                    }
                    catch (Exception)
                    {
                    }

                }


                if (cmd.Equals("UPDATE") && tokens.Length > 1 &&
                    tokens[1].ToUpper().Equals("STATISTICS"))
                    return "MANAGEMENT [UPDATE STATISTICS]";

                if (cmd.StartsWith("VALUES("))
                    return "VALUES";

                if (cmd.Equals("CREATE") && tokens.Length > 1 &&
                    tokens[1].ToUpper().Equals("VOLATILE"))
                    return "DDL [CREATE VOLATILE]";
                if (cmd.StartsWith("SQL_"))
                {
                    cmd = cmd.Substring(4);
                }


                String mappedSQLType = (String)_mappingHT[cmd];
                if (null != mappedSQLType)
                    return mappedSQLType;


                return "UNMAPPED [" + cmd + "]";

            }

            return "UNMAPPED [" + qryStart.Substring(0, Math.Min(qryStart.Length, 10)) + "]";
        }

        private static bool queryHadErrors(String errorCode)
        {
            if (0 >= errorCode.Length)
                return false;

            try
            {
                int err = int.Parse(errorCode);
                if (0 != err)
                    return true;

            }
            catch (Exception)
            {
            }

            return false;
        }

        private static void assembleMissingQueryEndDetails(DataRow aggregatedRow, DateTime probableEndTime)
        {
            /**
             *  Query had no ending row or end event and its part of the same session
             *  as the next query that was run.
             */
            try
            {
                aggregatedRow["END_TIME"] = probableEndTime;
                DateTime startTime = (DateTime)aggregatedRow["START_TIME"];
                aggregatedRow["ELAPSED_TIME"] = TriageHelper.getFormattedElapsedTime(startTime, probableEndTime);
                aggregatedRow["STATE"] = "Completed [Missing END]";

            }
            catch (Exception e)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "Error assembling missing query end row - error = " + e.Message);
            }

        }

        private static void assembleAbnormalQueryEndDetails(DataRow aggregatedRow, DateTime currentSysTime)
        {
            /**
             *  Query had no ending row or end event -- so if its past 1 day, 
             *  flag is as state missing completion.
             */
            try
            {
                DateTime startTime = (DateTime)aggregatedRow["START_TIME"];
                TimeSpan timeDiff = currentSysTime.Subtract(startTime);
                if (timeDiff.TotalSeconds > TriageHelper.TWELVE_HOURS.TotalSeconds)
                    aggregatedRow["STATE"] = "Abnormally Terminated";

            }
            catch (Exception e)
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "Error assembling abnormal query end row - error = " + e.Message);
            }

        }

        private static void assembleQueryEndDetails(DataRow endrow, DataRow aggregatedRow)
        {

            Object sqlText = aggregatedRow["SQL_TEXT"];
            if ((null == sqlText) || (0 >= sqlText.ToString().Length))
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "Have only the ending record -- fill in stuff for the starting record. \n" +
                                                "Query ID = " + endrow["QUERY_ID"]);

                aggregatedRow["QUERY_ID"] = endrow["QUERY_ID"];
                aggregatedRow["SESSION_ID"] = endrow["SESSION_ID"];
                //aggregatedRow["CURRENT_SYSTEM_TIME"] = (DateTime)endrow["ENTRY_TIME"];
                aggregatedRow["ELAPSED_TIME"] = "";
                aggregatedRow["USER_NAME"] = endrow["USER_NAME"];
                aggregatedRow["APPLICATION_NAME"] = endrow["APPLICATION_NAME"];
                aggregatedRow["CLIENT_ID"] = endrow["CLIENT_ID"];
                //aggregatedRow["DATASOURCE"] = endrow["DATASOURCE"];

            }


            aggregatedRow["STATE"] = "Completed";

            String errCode = (String)endrow["ERROR_CODE"];
            errCode = Regex.Replace(errCode, @"\D", "");
            if (queryHadErrors(errCode))
                aggregatedRow["STATE"] = "Error";

            aggregatedRow["ERROR_CODE"] = errCode;

            try
            {
                String killerName = endrow["APPLICATION_NAME"].ToString().Trim();
                if (killerName.Equals(SUSPEND_RESUME_KILL_SPJ_APPLICATION_ID))
                    aggregatedRow["STATE"] = "Abnormally Terminated";

            }
            catch (Exception)
            {
            }

            aggregatedRow["QUERY_ELAPSED_TIME"] = endrow["QUERY_ELAPSED_TIME"];
            aggregatedRow["MASTER_EXECUTOR_TIME"] = endrow["MASTER_EXECUTOR_TIME"];
            aggregatedRow["DISK_READS"] = endrow["DISK_READS"];
            //aggregatedRow["LOCKESCALATIONS"] = endrow["LOCKESCALATIONS"];
            aggregatedRow["LOCKWAITS"] = endrow["LOCKWAITS"];
            aggregatedRow["MESSAGE_BYTES_TO_DISK"] = endrow["MESSAGE_BYTES_TO_DISK"];
            aggregatedRow["MESSAGES_TO_DISK"] = endrow["MESSAGES_TO_DISK"];
            aggregatedRow["ROWS_ACCESSED"] = endrow["ROWS_ACCESSED"];
            aggregatedRow["ROWS_RETRIEVED"] = endrow["ROWS_RETRIEVED"];
            aggregatedRow["NUM_ROWS_IUD"] = endrow["NUM_ROWS_IUD"];
            //aggregatedRow["TOTAL_EXECUTES"] = endrow["TOTAL_EXECUTES"];
            //aggregatedRow["TOTAL_ELAPSED_TIME"] = endrow["TOTAL_ELAPSED_TIME"];
            //aggregatedRow["TOTAL_EXECUTION_TIME"] = endrow["TOTAL_EXECUTION_TIME"];

            DateTime endTime = (DateTime)endrow["ENTRY_TIME"];
            aggregatedRow["END_TIME"] = endTime;

            DateTime startTime = (DateTime)aggregatedRow["START_TIME"];
            String elapsedTime = TriageHelper.getFormattedElapsedTime(startTime, endTime);
            aggregatedRow["ELAPSED_TIME"] = elapsedTime;

            if ((0 > elapsedTime.Length))
            {
                Logger.OutputToLog(
                    TraceOptions.TraceOption.DEBUG,
                    TraceOptions.TraceArea.Monitoring,
                    TriageHelper.TRACE_SUB_AREA_NAME,
                    "getAggregatedQueryRow: Elapsed time is not set for QID = " +
                                                endrow["QUERY_ID"] + " (" + aggregatedRow["QUERY_ID"] + ")");
            }

        }

        private static DataTable cloneDataTableColumns(DataTable dt)
        {
            DataTable lore = new DataTable();
            if (dt != null)
            {
                try
                {
                    foreach (DataColumn col in dt.Columns)
                    {

#if DISABLE_CHUNKING
					// Don't need to do anything.
#else
                        //  Work around an ODBC DriverManager or .NET bug.
                        //  DriverManager/ODBC.NET provider uses 1K buffers for Unicode - 
                        //  so remove the columns added to split the text up.
                        if (col.ColumnName.StartsWith("SQL_TEXT CHUNK_ID_"))
                            continue;

#endif

                        DataColumn colCopy = new DataColumn(col.ColumnName, col.DataType,
                                                            col.Expression, col.ColumnMapping);
                        lore.Columns.Add(colCopy);
                    }

                }
                catch (Exception e)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Monitoring,
                        TriageHelper.TRACE_SUB_AREA_NAME,
                        "Query cloneDataTableColumns() error = " + e.Message +
                                                    "\n\t Doing this via Clone() and Clear().");

                    lore = dt.Clone();
                    lore.Rows.Clear();
                }
            }
            return lore;  //  A clone of Data!! :^)
        }

        private static DataTable cleanupRunningQueriesDataTable(DataTable dt)
        {

            foreach (DataRow dr in dt.Rows)
            {
                try
                {
                    String sqlText = dr["SQL_TEXT"].ToString();
                    dr["SQL_TYPE"] = mapQueryToStatementType(sqlText);

                    String errCode = (String)dr["ERROR_CODE"];
                    errCode = Regex.Replace(errCode, @"\D", "");
                    if (queryHadErrors(errCode))
                        dr["STATE"] = "Error";

                    dr["ERROR_CODE"] = errCode;


                    String elapsedTime = "";
                    try
                    {
                        DateTime startTime = (DateTime)dr["START_TIME"];
                        DateTime endTime = DateTime.Now.ToUniversalTime();
                        try
                        {
                            if (0 < dr["END_TIME"].ToString().Length)
                                endTime = (DateTime)dr["END_TIME"];

                        }
                        catch (Exception)
                        {

                        }

                        elapsedTime = TriageHelper.getFormattedElapsedTime(startTime, endTime);
                    }
                    catch (Exception)
                    {
                        elapsedTime = "";
                    }
                    dr["ELAPSED_TIME"] = elapsedTime;
                }
                catch (Exception ex)
                {
                    //clean up failed for a query.  
                    //Log the error to error log and continue with the next query.
                    string queryId = "";
                    if (dt.Columns.Contains("QUERY_ID"))
                    {
                        if (dr["QUERY_ID"] != null)
                        {
                            queryId = dr["QUERY_ID"] as string;
                        }
                    }

                    string errorMessage = string.Format("Cleanup failed for query {0}: {1}", queryId, ex.Message);
                    Logger.OutputErrorLog(errorMessage);
                }
            }
            return dt;
        }
        #endregion
    }
}
