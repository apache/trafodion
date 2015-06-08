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
using System.Drawing;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;

namespace Trafodion.Manager.BDRArea.Controls
{
    public partial class MyWidgetControl : UserControl
    {
        #region Fields

        //A key that will be used for persistence
        private static readonly string MyWidgets = "MyWidgets";
        private static readonly string MonitorWorkloadConfigName = "BDR_MonitorWorkloadConfig";
        private UniversalWidgetConfig MonitorWorkloadConfig = null;
        private GenericUniversalWidget MonitorWorkloadWidget = null;
        private TrafodionIGrid _monitorWorkloadIGrid = null;        

        const string MinDate = "2000-01-01 00:00:00";
        const string MaxDate = "2100-12-31 23:59:59";
        const string LongDatePattern = @"\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\s*$";
        const string ShortDatePattern = @"\d{4}-\d{2}-\d{2}\s*$";
        const double OneMB = 1024d * 1024d;
        string StopLivRefreshText = "Stop Live Refresh";

        // For AggMB interval calc
        //double _AggBytesThen = 0.0;         // prev
        //double _AggBytesNow = 0.0;         // latest

        ConnectionDefinition _connectionDefinition;
        bool _IsServicesUser;
        string _UserName;
        string _RoleName;
        const string _TrafodionCatOnly = " AND SOURCE_NAME LIKE 'TRAFODION.%'";
        const string _OrderByClause = " ORDER BY START_TIME desc, SOURCE_NAME, STATUS ";

        const string _BaseQuery =
            "(SELECT " +
                   "a.QUERY_ID,START_TIME,ABORT_TIME,END_TIME,TOTAL_BLOCKS,BLOCKS_REPLICATED,PERCENT_COMPLETE," +
                   "CMP_RATIO,AVG_CMP_TIME,AVG_UNCMP_TIME," +
                   "STATUS,ERROR_INFO,queryid_extract(a.QUERY_ID,'user') UserID," +
                   "case " +
                   " when END_TIME is NULL then datediff(minute,START_TIME,(CURRENT_TIMESTAMP + interval '30' second)) " +
                   " else datediff(minute,START_TIME,END_TIME) end ET, " +
                   "VERSION,SOURCE_NAME,TARGET_NAME,SOURCE_TYPE,SOURCE_SYSTEM,TARGET_SYSTEM,PARENT_QUERY_ID," +
                   "NUM_PARTNS,PRIORITY,RATE,CONCURRENCY,PURGEDATA,COMPRESS,VALIDATE_DATA,VALIDATE_DDL,CREATE_DDL " +
                   // INCREMENTAL in M7
                   ",INCREMENTAL " +
            "FROM MANAGEABILITY.REPLICATE_SCHEMA.REPLICATE_OBJECT_STATUS a " +
            "JOIN MANAGEABILITY.REPLICATE_SCHEMA.REPLICATE_OBJECT_INFO b " +
            "ON a.QUERY_ID = b.QUERY_ID ) t ";


        // compute elapsed minutes as end_time - start_time or current_timestamp - start_time
        // Suspend-time column in the table is not impl'd.
        const string _HistoryQColumns =
            "START_TIME as \"START TIME\", " +
            "END_TIME as \"END TIME\", " +
            
            "ET as \"ET (min)\"," +
            "case when END_TIME is NULL AND PERCENT_COMPLETE > 0 " +
            "  then DATEDIFF(MINUTE, CURRENT_TIMESTAMP, DATEADD(minute, (100 * ET)/PERCENT_COMPLETE, START_TIME)) " +
            "  else NULL end as \"ETA (mins)\", " +
            
            "trim(SOURCE_SYSTEM) as \"SOURCE SYSTEM\", " +
            "trim(TARGET_SYSTEM) as \"TARGET SYSTEM\", " +
            "trim(USERID) as \"USERID\", " +
            "trim(SOURCE_NAME) as \"SOURCE NAME\", " +
            "trim(TARGET_NAME) as \"TARGET NAME\", " +
            "trim(SOURCE_TYPE) as \"SRC TYPE\", " +
            "trim(STATUS) as \"STATUS\", " +
            "trim(cast(PERCENT_COMPLETE as varchar(5))) as \"PCT DONE\", " +
            "BLOCKS_REPLICATED as \"BLKS REPD\", " +
            "TOTAL_BLOCKS as \"TOT BLKS\", " +
            "NUM_PARTNS as \"NUM PARTS\", " +
            "CMP_RATIO as \"CMPRSN RATIO\", " +
            "trim(cast(PRIORITY as varchar(10))) as \"PRI\", " +
            "RATE as \"RATE\", " +
            "trim(cast(CONCURRENCY as varchar(10))) as \"CNCRNCY\", " +
            // incremental in M7
            "trim(INCREMENTAL) as \"INCR\", " +
            "trim(PURGEDATA) as \"PGDATA\", " +
            "trim(COMPRESS) as \"CMPRS\", " +
            "trim(VALIDATE_DATA) as \"VAL DATA\", " +
            "trim(VALIDATE_DDL) as \"VAL DDL\", " +
            "trim(CREATE_DDL) as \"CRE DDL\", " +
            "trim(QUERY_ID) as \"QUERY ID\"," +
            "trim(ERROR_INFO) as \"ERROR INFO\" ";

        string _LiveQuery =
            "SELECT " + _HistoryQColumns + "FROM " +
            _BaseQuery +
            "WHERE (STATUS IN ( 'INITIATED', 'IN_PROGRESS', 'DATA_REPLICATED' ) " +
            " OR (END_TIME >= (CURRENT_TIMESTAMP - interval '2' minute))) " 
            ;

        string queryText;
        string _startFromDateText = MinDate;
        string _startToDateText = MaxDate;
        string _endFromDateText = MinDate;
        string _endToDateText = MaxDate;
        string _likeNameText = null;
        string _targetNameText = null;
        string _pctDoneText = null;
        string _blksRepdText = null;
        string _totBlksText = null;
        string _priText = null;
        string _userIDText = null;
        string _prgdataText = null;
        string _compressText = null;
        string _valdataText = null;
        string _valDdlText = null;
        string _crDdlText = null;
        string _tgtSystemText = null;
        string _concurrencyText = null;
        string _incrText = null;

        int _timeRemaining;

        // save INP BDRs to calc agg bytes - columns defined in constructor
        DataTable _prevIntervalBDR = new DataTable();

        #endregion Fields

        #region Properties
        public ConnectionDefinition TheConnectionDefinition
        {
            get { return _connectionDefinition; }
            set 
            {  
                if (_connectionDefinition != value)
                {
                    if (_monitorWorkloadIGrid != null)
                    { 
                        ClearGrid();
                        _monitorWorkloadIGrid.DoubleClickHandler = this._monitorWorkloadIGrid_DoubleClick;
                        _monitorWorkloadIGrid.ColHdrClick -= _monitorWorkloadIGrid_ColHdrClick;
                        _monitorWorkloadIGrid.ColHdrDoubleClick -= _monitorWorkloadIGrid_ColHdrDoubleClick;

                    }
                    if (MonitorWorkloadWidget != null)
                    {
                        MonitorWorkloadWidget.DataProvider.Stop();
                    }
                    ResetFilters();
                    InitializeControl();
                }
                _connectionDefinition = value;
                ShowWidgets();
            }
        }

        /// <summary>
        /// Used by calendar pop-up to return Start "from" datetime
        /// </summary>
        public string SetStartFromDate
        {
            set 
            { 
                _startFromDateTextBox.Text = value;
                _startFromDateText = value;
            }
        }

        /// <summary>
        /// Used by calendar pop-up to return Start "to" datetime
        /// </summary>
        public string SetStartToDate
        {
            set 
            {
                _startToDateTextBox.Text = value;
                _startToDateText = value;
            }
        }

        /// <summary>
        /// Used by calendar pop-up to return End "from" datetime
        /// </summary>
        public string SetEndFromDate
        {
            set 
            {
                _endFromDateTextBox.Text = value;
                _endFromDateText = value;
            }
        }

        /// <summary>
        /// Used by calendar pop-up to return End "to" datetime
        /// </summary>
        public string SetEndToDate
        {
            set 
            {
                _endToDateTextBox.Text = value;
                _endToDateText = value;
            }
        }

        #endregion Properties


        // ##############################################################################################
        // My code starts here
        // ##############################################################################################
 
        /* #########################################################################
         *              CONSTRUCTOR
         * #########################################################################
         */
        public MyWidgetControl()
        {
            InitializeComponent();

            // initialize the pop-up help text for controls
            MyToolTip.SetToolTip(_startFromDateTextBox, "Start TIMESTAMP begin value");
            MyToolTip.SetToolTip(_startToDateTextBox, "Start TIMESTAMP end value");
            MyToolTip.SetToolTip(_endFromDateTextBox, "End TIMESTAMP begin value");
            MyToolTip.SetToolTip(_endToDateTextBox, "End TIMESTAMP end value");
            MyToolTip.SetToolTip(_executeButton, "Depending on context, retrieves info / stops refresh");
            MyToolTip.SetToolTip(_refreshNowButton, "Enabled when in LIVE mode: stop auto refresh");
            MyToolTip.SetToolTip(_resetFiltersButton, "Clear all data filters");
            MyToolTip.SetToolTip(_refreshTextBox, "Auto Refresh time interval (seconds)");
            MyToolTip.SetToolTip(_likenameTextBox, "Some portion of SOURCE filename");
            MyToolTip.SetToolTip(_targetNameTextBox, "Some portion of TARGET filename");
            MyToolTip.SetToolTip(_statusCheckedListBox, "Filter for the selected STATUS");
            MyToolTip.SetToolTip(_srcTypeCheckedListBox, "Filter for TAble or IndeX object types");
            MyToolTip.SetToolTip(_numPartsCheckedListBox, "Filter for number of partitions");
            MyToolTip.SetToolTip(_liveRadioButton, "Choose this for active BDR run data");
            MyToolTip.SetToolTip(_historyRadioButton, "Choose this for past BDR run data");
            MyToolTip.SetToolTip(_startCalendarButton, "Customize Starting Time Values");
            MyToolTip.SetToolTip(_endCalendarButton, "Customize Ending Time Values");
            MyToolTip.SetToolTip(oneGuiGroupBox7, "Greater than or Equal to filters");
            MyToolTip.SetToolTip(_tgtSystemTextBox, "Up to full NV seg name or SQ name");

            ConnectionDefinition.Changed += ConnectionDefinition_Changed;

            // Following is from Widgets Prgming Guide 1.4, Sect 7.7...
            // Makes the two panes of my widget independently resizable
            //Set the persistence key
            _widgetCanvas.ThePersistenceKey = MyWidgets;
            _widgetCanvas.Dock = DockStyle.Fill;

            ////set the layout manager for the canvas
            //GridLayoutManager gridLayoutManager = new GridLayoutManager(6, 1);
            //_widgetCanvas.LayoutManager = gridLayoutManager;

            ////Add the first control to the canvas
            //GridConstraint gridConstraint = new GridConstraint(0, 0, 3, 1);
            //WidgetContainer widgetContainer = new WidgetContainer(_widgetCanvas,_inputControlGroup, "Input Control");
            //widgetContainer.Name = "Input Control";
            //widgetContainer.AllowDelete = false;
            //_widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            ////Add the next control to the canvas
            //gridConstraint = new GridConstraint(3, 0, 3, 1);
            //WidgetContainer widgetContainer1 = new WidgetContainer(_widgetCanvas, _queryOutputGuiPanel1, "BDR Status Results");
            //widgetContainer1.Name = "BDR Status Results";
            //widgetContainer1.AllowDelete = false;
            //_widgetCanvas.AddWidget(widgetContainer1, gridConstraint, -1);

            ////Instruct the canvas to paint it correctly
            //_widgetCanvas.InitializeCanvas();
            //_widgetCanvas.ResetWidgetLayout();

            //oneGuiSplitContainer1.SplitterDistance = _inputControlGroup.ClientSize.Height + 2;
            // add columns to DataTable
            _prevIntervalBDR.Columns.Add("StartTime", typeof(string));
            _prevIntervalBDR.Columns.Add("SourceName", typeof(string));
            _prevIntervalBDR.Columns.Add("TargetSystem", typeof(string));
            _prevIntervalBDR.Columns.Add("BlocksReplicated", typeof(Int64));

            // define PK
            _prevIntervalBDR.PrimaryKey = new DataColumn[] { _prevIntervalBDR.Columns[0], _prevIntervalBDR.Columns[1], _prevIntervalBDR.Columns[2] };
        }

        public void InitializeControl()
        {
            _historyRadioButton.PerformClick();
        }

        void MyDispose(bool disposing)
        {
            if (disposing)
            {
                ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
                if (MonitorWorkloadWidget != null && MonitorWorkloadWidget.DataProvider != null)
                {
                    MonitorWorkloadWidget.DataProvider.Stop();
                }
            }
        }

        public void ShowWidgets()
        {
            //Create the configuration. If one is persisted, we use that otherwise we create one
            UniversalWidgetConfig tempConfig = WidgetRegistry.GetConfigFromPersistence(MonitorWorkloadConfigName);

            if (tempConfig == null)
            {
                MonitorWorkloadConfig = WidgetRegistry.GetDefaultDBConfig();
                MonitorWorkloadConfig.Name = MonitorWorkloadConfigName;
                MonitorWorkloadConfig.Title = "BDR Dashboard";
                DatabaseDataProviderConfig dbConfig = MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.TimerPaused = true;
            }
            else
            {
                MonitorWorkloadConfig = tempConfig;
            }

            //Make this data provider's timer to continue after encountered an error. 
            MonitorWorkloadConfig.DataProviderConfig.TimerContinuesOnError = true;

            //Set the connection definition if available
            MonitorWorkloadConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            MonitorWorkloadConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.Show;
            MonitorWorkloadConfig.ShowExportButtons = true;
            MonitorWorkloadConfig.ShowRefreshButton = false;
            MonitorWorkloadConfig.ShowTimerSetupButton = false;

            //Create a UW using the configuration             
            MonitorWorkloadWidget = new GenericUniversalWidget();
            ((TabularDataDisplayControl)MonitorWorkloadWidget.DataDisplayControl).LineCountFormat = "Replication records";
            MonitorWorkloadWidget.DataProvider = new DatabaseDataProvider(MonitorWorkloadConfig.DataProviderConfig as DatabaseDataProviderConfig);
            MonitorWorkloadWidget.UniversalWidgetConfiguration = MonitorWorkloadConfig;
            MonitorWorkloadConfig.ShowHelpButton = true;
            MonitorWorkloadConfig.HelpTopic = HelpTopics.BDRDashboard;

            //Add the widget to the canvas
            //GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            //_theCanvas.AddWidget(MonitorWorkloadWidget, MonitorWorkloadConfig.Name, MonitorWorkloadConfig.Title, gridConstraint, -1);
            MonitorWorkloadWidget.Dock = DockStyle.Fill;

            _queryOutputGuiPanel1.Controls.Clear();
            _queryOutputGuiPanel1.Controls.Add(MonitorWorkloadWidget);

            //Add popup menu items to the table
            TabularDataDisplayControl dataDisplayControl = MonitorWorkloadWidget.DataDisplayControl as TabularDataDisplayControl;

            //Associate the custom data display handler for the TabularDisplay panel
            MonitorWorkloadWidget.DataDisplayControl.DataDisplayHandler = new MyMonitorDataHandler(this);

            _monitorWorkloadIGrid = ((TabularDataDisplayControl)MonitorWorkloadWidget.DataDisplayControl).DataGrid;
            _monitorWorkloadIGrid.DoubleClickHandler = this._monitorWorkloadIGrid_DoubleClick;
            _monitorWorkloadIGrid.ColHdrClick += _monitorWorkloadIGrid_ColHdrClick;
            _monitorWorkloadIGrid.ColHdrDoubleClick += _monitorWorkloadIGrid_ColHdrDoubleClick;
            //_monitorWorkloadIGrid.Paint += _monitorWorkloadIGrid_Paint;
            MonitorWorkloadWidget.ResetProviderStatus();
        }

        void _monitorWorkloadIGrid_Paint(object sender, PaintEventArgs e)
        {
            if (_monitorWorkloadIGrid.Rows.Count > 0) ColorizeDataGrid();
        }

        void _monitorWorkloadIGrid_ColHdrDoubleClick(object sender, TenTec.Windows.iGridLib.iGColHdrDoubleClickEventArgs e)
        {
            // column header click changes sort order - must reapply color
            ColorizeDataGrid();
                
        }

        void _monitorWorkloadIGrid_ColHdrClick(object sender, TenTec.Windows.iGridLib.iGColHdrClickEventArgs e)
        {
            // column header click changes sort order - must reapply color
            ColorizeDataGrid();
        }


        void _monitorWorkloadIGrid_DoubleClick(int row)
        {
            // Double click any cell in the desired row...

            // Meanwhile, the refresh timer is still running.  Maybe we should pop the following Q result into 
            // a new window of its own??  DoQuery() doesn't do that.

            // here's how to get the QID...it's cell[20] based on 0-rel position in the select list
            //MessageBox.Show("QID: " + _resultsDataGridView.Rows[e.RowIndex].Cells[20].Value); 

            // not valid for History view...no further details

            if (_historyRadioButton.Checked == true)
                MessageBox.Show("No further details available for BDR in History view", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
            {
                string _windowTitle;
                string _qid;
                string _targetname;
                Size size = new Size(1000, 800);

                //_timer.Stop();          // need to stop??
                _qid = _monitorWorkloadIGrid.Rows[row].Cells["QUERY ID"].Value.ToString();
                _windowTitle = "BDR Details for QID: " + _qid;
                _targetname = _monitorWorkloadIGrid.Rows[row].Cells["TARGET NAME"].Value.ToString();

                if (WindowsManager.Exists(TrafodionForm.TitlePrefix + _connectionDefinition.Name + " : " + _windowTitle))
                {
                    WindowsManager.BringToFront(TrafodionForm.TitlePrefix + _connectionDefinition.Name + " : " + _windowTitle);
                }
                else
                {
                    MyBDRDetail testdetails = new MyBDRDetail(_qid,_targetname, _connectionDefinition);
                    WindowsManager.PutInWindow(size, testdetails, _windowTitle, _connectionDefinition);
                }

            }
        }

        // ##############################################################################################
 
        void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason == ConnectionDefinition.Reason.Disconnected)
            {
                ClearGrid();
                ResetFilters();
            }
        }
        // ##############################################################################################

        /// <summary>
        /// Prevents displaying query results from the prior connection
        /// </summary>
        private void ClearGrid()
        {
            _monitorWorkloadIGrid.Rows.Clear();
        }
        // ##############################################################################################

        private void ResetFilters()
        {
            // internal areas
            _likeNameText = null;
            _targetNameText = null;
            _startFromDateText = MinDate;
            _startToDateText = MaxDate;
            _endFromDateText = MinDate;
            _endToDateText = MaxDate;
            _pctDoneText = null;
            _blksRepdText = null;
            _totBlksText = null;
            //_numPartsText = null;
            _priText = null;
            _userIDText = null;
            _prgdataText = null;
            _compressText = null;
            _valdataText = null;
            _valDdlText = null;
            _crDdlText = null;
            _tgtSystemText = null;
            _concurrencyText = null;

            // in the GUI
            _startFromDateTextBox.Text = "";
            _startToDateTextBox.Text = "";
            _likenameTextBox.Text = "";
            _targetNameTextBox.Text = "";
            _endFromDateTextBox.Text = "";
            _endToDateTextBox.Text = "";
            _pctDoneTextBox.Text = "";
            _blksRepdTextBox.Text = "";
            _totBlksTextBox.Text = "";
            //_numPartsTextBox.Text = "";
            _priTextBox.Text = "";
            _userIDTextBox.Text = "";
            _tgtSystemTextBox.Text = "";
            _cncrncyTextBox.Text = "";
            _pgdataOffRadioButton.Checked = true;
            _cmprsOffRadioButton.Checked = true;
            _valDataOffRadioButton.Checked = true;
            _valDdlOffRadioButton.Checked = true;
            _crDdlOffRadioButton.Checked = true;

            // turn off the <= / >= buttons
            _pctDoneGERadioButton.Checked = false;
            _pctDoneLERadioButton.Checked = false;
            _blksRepdGERadioButton.Checked = false;
            _blksRepdLERadioButton.Checked = false;
            _totBlksGERadioButton.Checked = false;
            _totBlksLERadioButton.Checked = false;
            _cncrncyGERadioButton.Checked = false;
            _cncrncyLERadioButton.Checked = false;
            _priGERadioButton.Checked = false;
            _priLERadioButton.Checked = false;

            for (int i = 0; i <= _statusCheckedListBox.Items.Count - 1; i++)
                { _statusCheckedListBox.SetItemChecked(i,false);
                }          // uncheck all

            for (int i = 0; i <= _srcTypeCheckedListBox.Items.Count - 1; i++)
                { _srcTypeCheckedListBox.SetItemChecked(i, false); }

            for (int i = 0; i <= _numPartsCheckedListBox.Items.Count - 1; i++)
                { _numPartsCheckedListBox.SetItemChecked(i, false); }

        }
        // ##############################################################################################

        private void ColorizeDataGrid()
        { 
            int rownum;

            Cursor.Current = Cursors.WaitCursor;

            // colorize the datagridview
            for (rownum = 0; rownum < (_monitorWorkloadIGrid.Rows.Count); rownum++)
            {
                switch (_monitorWorkloadIGrid.Rows[rownum].Cells["STATUS"].Value.ToString())
                {
                    case "ABORTED":
                    case "FAILED":
                        _monitorWorkloadIGrid.Rows[rownum].BackColor = Color.Red;
                        break;

                    case "COMPLETED":
                        _monitorWorkloadIGrid.Rows[rownum].BackColor = Color.LawnGreen;
                        break;

                    case "IN_PROGRESS":
                    case "DATA_REPLICATED":
                        _monitorWorkloadIGrid.Rows[rownum].BackColor = Color.Yellow;
                        break;

                    case "INITIATED":
                        _monitorWorkloadIGrid.Rows[rownum].BackColor = Color.DodgerBlue;
                        break;
                }
            } // for

            Cursor.Current = Cursors.Default;

        }
        // ##############################################################################################

        private void DoQuery()
        {
            //MessageBox.Show(queryText);
            //Cursor.Current = Cursors.WaitCursor;

            //try
            //{
                
                //SqlQuery sqlQuery = new SqlQuery(_connectionDefinition);
            //    DataTable dataTable = sqlQuery.GetQueryResults(queryText);
            //    _monitorWorkloadIGrid.FillWithData(dataTable);
            //}
            //catch (Exception ex)
            //{
            //    MessageBox.Show(ex.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            //}
            //finally
            //{
            //    Cursor.Current = Cursors.Default;
            //}

            ((DatabaseDataProviderConfig)MonitorWorkloadWidget.DataProvider.DataProviderConfig).SQLText = queryText;
            MonitorWorkloadWidget.DataProvider.Start();

        }
        // ###########################################################################################################

        // Because History and Live share filters, add them here
        // Note this adds the " AND..." conditions only -- "WHERE..." is on the caller as well other specifics
        private bool PredicateCheck()
        {
            // check user:  regular user...only TRAFODION catalog names; Services or above...all entries
            //    test differs between NV & SQ
            if ((!string.IsNullOrEmpty(_UserName) && !_UserName.ToUpper().Equals("DB__ROOT"))
                &&
                (!string.IsNullOrEmpty(_RoleName) && !_RoleName.ToUpper().Equals("DB__USERADMIN") && !_RoleName.ToUpper().Equals("DB__ROOTROLE")))
            {
                // regular SQ user - only TRAFODION cat
                queryText = queryText + _TrafodionCatOnly;
            }
            
            // Filter:  Start_time
            assignDateText(_startFromDateTextBox, ref _startFromDateText);
            assignDateText(_startToDateTextBox, ref _startToDateText);
            if (_startFromDateText != MinDate)
            {
                queryText = queryText + " AND START_TIME BETWEEN timestamp '" + _startFromDateText + "'" +
                                                          " AND timestamp '" + _startToDateText + "'";
            }

            // Filter:  End_time
            if (_endFromDateTextBox.Enabled == true)
            {
                assignDateText(_endFromDateTextBox, ref _endFromDateText);
                assignDateText(_endToDateTextBox, ref _endToDateText);
                if (_endFromDateText != MinDate)
                {
                    queryText = queryText + " AND END_TIME BETWEEN timestamp '" + _endFromDateText + "'" +
                                                            " AND timestamp '" + _endToDateText + "'";
                }
            }

            //reset values
            _startFromDateText = MinDate;
            _startToDateText = MaxDate;
            _endFromDateText = MinDate;
            _endToDateText = MaxDate;

            // Filter:  Source name
            if (_likeNameText != null)
            { queryText = queryText + _likeNameText; }

            // Filter:  Target name
            if (_targetNameText != null)
            { queryText = queryText + _targetNameText; }

            // Filter:  Target system
            if (_tgtSystemText != null)
            { queryText = queryText + _tgtSystemText; }

            // Filter: % Done
            if (_pctDoneText != null)
            {
                if (_pctDoneLERadioButton.Checked == true)
                    queryText = queryText + " AND PERCENT_COMPLETE <= " + _pctDoneText;
                else
                    if (_pctDoneGERadioButton.Checked == true)
                        queryText = queryText + " AND PERCENT_COMPLETE >= " + _pctDoneText;
                    else
                    {
                        MessageBox.Show("Missing comparison operator for % Done", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return true;
                    }
            }

            // Filter: Blocks Rep'd
            if (_blksRepdText != null)
            {
                if (_blksRepdLERadioButton.Checked == true)
                    queryText = queryText + " AND BLOCKS_REPLICATED <= " + _blksRepdText;
                else
                    if (_blksRepdGERadioButton.Checked == true)
                        queryText = queryText + " AND BLOCKS_REPLICATED >= " + _blksRepdText;
                    else
                    {
                        MessageBox.Show("Missing comparison operator for Blocks Rep'd", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return true;
                    }
            }

            // Filter:  Total blocks
            if (_totBlksText != null)
            {
                if (_totBlksLERadioButton.Checked == true)
                    queryText = queryText + " AND TOTAL_BLOCKS <= " + _totBlksText;
                else
                    if (_totBlksGERadioButton.Checked == true)
                        queryText = queryText + " AND TOTAL_BLOCKS >= " + _totBlksText;
                    else
                    {
                        MessageBox.Show("Missing comparison operator for Total Blks", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return true;
                    }
            }

            // Filter:  Number of partitions
            if (_numPartsCheckedListBox.CheckedItems.Count != 0)   // some items checked, build IN-list
            {
                string inlist = " AND NUM_PARTNS IN (";
                for (int i = 0; i <= _numPartsCheckedListBox.CheckedItems.Count - 1; i++)
                {
                    if (i > 0) inlist = inlist + ",";
                    inlist = inlist + _numPartsCheckedListBox.CheckedItems[i].ToString();
                }
                inlist = inlist + ")";
                queryText = queryText + inlist;
            }

            // Filter:  UserID
            if (_userIDText != null)
            { queryText = queryText + _userIDText; }

            // Filter:  Concurrency
            if (_concurrencyText != null)
            {
                if (_cncrncyLERadioButton.Checked == true)
                    queryText = queryText + " AND CONCURRENCY <= " + _concurrencyText;
                else
                    if (_cncrncyGERadioButton.Checked == true)
                        queryText = queryText + " AND CONCURRENCY >= " + _concurrencyText;
                    else
                    {
                        MessageBox.Show("Missing comparison operator for Concurrency", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return true;
                    }
            }

            // Filter:  Priority
            if (_priText != null)
            {
                if (_priLERadioButton.Checked == true)
                    queryText = queryText + " AND PRIORITY <= " + _priText;
                else
                    if (_priGERadioButton.Checked == true)
                        queryText = queryText + " AND PRIORITY >= " + _priText;
                    else
                    {
                        MessageBox.Show("Missing comparison operator for Priority", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        return true;
                    }
            }

            // Filter: Source Type
            if (_srcTypeCheckedListBox.CheckedItems.Count != 0)   // some items checked, build IN-list
            {
                string inlist = " AND SOURCE_TYPE IN (";
                for (int i = 0; i <= _srcTypeCheckedListBox.CheckedItems.Count - 1; i++)
                {
                    if (i > 0) inlist = inlist + ",";
                    inlist = inlist + "'" + _srcTypeCheckedListBox.CheckedItems[i].ToString() + "'";
                }
                inlist = inlist + ")";
                queryText = queryText + inlist;
            }


            // Filters:  all the Y/N flags
            if (_prgdataText != null) queryText = queryText + _prgdataText;
            if (_compressText != null) queryText = queryText + _compressText;
            if (_valdataText != null) queryText = queryText + _valdataText;
            if (_valDdlText != null) queryText = queryText + _valDdlText;
            if (_crDdlText != null) queryText = queryText + _crDdlText;
            if (_incrText != null) queryText = queryText + _incrText;
            
            // success
            return false;
        }
        // ###########################################################################################################

        /// <summary>
        /// assign the value of textbox to datatext if the value in the textbox is valid
        /// </summary>
        /// <param name="textBox"></param>
        /// <param name="dateText"></param>
        private void assignDateText(TrafodionTextBox textBox, ref String dateText)
        {
            if (Regex.IsMatch(textBox.Text, LongDatePattern))
                dateText = textBox.Text;
            else
                if (Regex.IsMatch(textBox.Text, ShortDatePattern))
                {
                    dateText = textBox.Text + " 00:00:00";
                    textBox.Text = dateText;
                }
        }


        private void _executeButton_Click(object sender, EventArgs e)
        {
            _IsServicesUser = _connectionDefinition.IsServicesUser;
            _UserName = _connectionDefinition.DatabaseUserName;
            _RoleName = _connectionDefinition.RoleName;

            if (_executeButton.Text == this.StopLivRefreshText)
            {
                _timer.Enabled = false;
                _timer.Stop();
                _executeButton.Text = "Live View";
                _refreshTextBox.Enabled = true;        // allow changing refresh interval
                _refreshNowButton.Enabled = false;
                _timeRemainingTextBox.Text = _refreshTextBox.Text;
                return;
            }
            else
                if (_historyRadioButton.Checked)
                {
                    _timer.Enabled = false;

                    // History query is constructed from a pre-defined set of columns
                    // plus optional predicates that user specifies as filters;
                    // If there are no filters, WHERE syntax is not needed and all the BDR rows are used,
                    // else each filter condition is added as an AND predicate
                    queryText = "SELECT " + _HistoryQColumns +
                                "FROM " + _BaseQuery;
                    // Add predicates...
                    // ((((((((((((((((((((((((((((((((
                    queryText = queryText + "WHERE STATUS NOT IN ('IN_PROGRESS','INITIATED') ";  // these status = LIVE      

                    // Filter: BDR Status
                    if (_statusCheckedListBox.CheckedItems.Count != 0)   // some items checked, build IN-list
                    {
                        string inlist = " AND STATUS IN (";
                        for (int i = 0; i <= _statusCheckedListBox.CheckedItems.Count - 1; i++)
                        {
                            if (i > 0) inlist = inlist + ",";
                            inlist = inlist + "'" + _statusCheckedListBox.CheckedItems[i].ToString() + "'";
                        }
                        inlist = inlist + ")";
                        queryText = queryText + inlist;
                    }
                    // Filters shared with LIVE
                    if (PredicateCheck()) return;       // bad predicate - exit

                    // )))))))))))))))))))))))))))))))))))
                    // end of adding predicates

                    queryText = queryText + _OrderByClause;

                } // (_historyRadioButton.Checked)
                else
                    if (_liveRadioButton.Checked)
                    {
                        queryText = _LiveQuery;
                        // Filters shared with HISTORY
                        if (PredicateCheck()) return;   // bad predicate - exit

                        // finally...
                        queryText = queryText + _OrderByClause;

                        _executeButton.Text = this.StopLivRefreshText;
                        _refreshTextBox.Enabled = false;        // disable changing refresh interval
                        _refreshNowButton.Enabled = true;
                        _timeRemaining = Convert.ToInt16(_refreshTextBox.Text);
                        _timer.Interval = 1000;      // one second updates to display
                    } // (_liveRadioButton.Checked)

            DoQuery();
        }


        // ###########################################################################################################
        // The following 4 datetime text boxes can be manually entered.  Handle all the same way.
        // Check for all spaces
        // Check for yyyy-mm-dd only, if so, extend with 00:00:00
        // Check for yyyy-mm-dd hh:mm:ss format
        // Otherwise it's bad

        // rather than handle keypress and textchanged events, validate when user LEAVES this control
        private void _startFromDateTextBox_Leave(object sender, EventArgs e)
        {
            string buffer = _startFromDateTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            {
                if (Regex.IsMatch(_startFromDateTextBox.Text, LongDatePattern))
                    _startFromDateText = _startFromDateTextBox.Text;
                else
                    if (Regex.IsMatch(_startFromDateTextBox.Text, ShortDatePattern))
                    {
                        _startFromDateText = _startFromDateTextBox.Text + " 00:00:00";
                        _startFromDateTextBox.Text = _startFromDateText;
                    }
                    else
                    {
                        _startFromDateTextBox.BackColor = Color.Crimson;
                        MessageBox.Show("Bad Date/Time format for Start Time From", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        _startFromDateTextBox.Focus();
                    }
            }

        }

        // ###########################################################################################################
        private void _startToDateTextBox_Leave(object sender, EventArgs e)
        {
            string buffer = _startToDateTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            {
                if (Regex.IsMatch(_startToDateTextBox.Text, LongDatePattern))
                    _startToDateText = _startToDateTextBox.Text; 
                else
                    if (Regex.IsMatch(_startToDateTextBox.Text, ShortDatePattern))
                    {
                        _startToDateText = _startToDateTextBox.Text + " 00:00:00";
                        _startToDateTextBox.Text = _startToDateText;
                    }
                    else
                    {
                        _startToDateTextBox.BackColor = Color.Crimson;
                        MessageBox.Show("Bad Date/Time format for Start Time To", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        _startToDateTextBox.Focus();
                    }
            }

        }
        // ###########################################################################################################

        private void _endFromDateTextBox_Leave(object sender, EventArgs e)
        {
            string buffer = _endFromDateTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            {
                if (Regex.IsMatch(_endFromDateTextBox.Text, LongDatePattern))
                    _endFromDateText = _endFromDateTextBox.Text; 
                else
                    if (Regex.IsMatch(_endFromDateTextBox.Text, ShortDatePattern))
                    {
                        _endFromDateText = _endFromDateTextBox.Text + " 00:00:00";
                        _endFromDateTextBox.Text = _endFromDateText;
                    }
                    else
                    {
                        _endFromDateTextBox.BackColor = Color.Crimson;
                        MessageBox.Show("Bad Date/Time format for End Time From", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        _endFromDateTextBox.Focus();
                    }
            }
        }
        // ###########################################################################################################

        private void _endToDateTextBox_Leave(object sender, EventArgs e)
        {
            string buffer = _endToDateTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            {
                if (Regex.IsMatch(_endToDateTextBox.Text, LongDatePattern))
                    _endToDateText = _endToDateTextBox.Text; 
                else
                    if (Regex.IsMatch(_endToDateTextBox.Text, ShortDatePattern))
                    {
                        _endToDateText = _endToDateTextBox.Text + " 00:00:00";
                        _endToDateTextBox.Text = _endToDateText;
                    }
                    else
                    {
                        _endToDateTextBox.BackColor = Color.Crimson;
                        MessageBox.Show("Bad Date/Time format for End Time To", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        _endToDateTextBox.Focus();
                    }
            }
        }
        // ###########################################################################################################
 
        private void DateTextBox_KeyPress(object sender, KeyPressEventArgs e)
        {
            TrafodionTextBox k = (TrafodionTextBox)sender;
            k.BackColor = Color.White;
        }
        // ###########################################################################################################

        private void _likenameTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _likenameTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _likeNameText = " AND SOURCE_NAME LIKE UPSHIFT('%" + _likenameTextBox.Text + "%')"; }
            else
            { _likeNameText = null; }

        }
        // ###########################################################################################################

        private void _refreshTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _refreshTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            {
                _timeRemaining = Convert.ToInt16(_refreshTextBox.Text);
                _timeRemainingTextBox.Text = _refreshTextBox.Text;
            }
            else
            {
                MessageBox.Show("Delay value cannot be empty", "Data Error!");
            }

        }
        // ###########################################################################################################

        private void _historyRadioButton_Click(object sender, EventArgs e)
        {
            SetDefaultStartDates();

             _executeButton.Text = "Get History";

            // off for history
            _timer.Enabled = false;
            _timer.Stop();
            _timeRemainingTextBox.Text = "off";
            _timeRemainingTextBox.BackColor = System.Drawing.Color.LightGray;
            _refreshTextBox.Enabled = false;
            _refreshNowButton.Enabled = false;
            _aggMBGroupBox.Visible = false;
            _aggMBTextBox.Visible = false;

            // on for history
            _statusCheckedListBox.Enabled = true;
            _likenameTextBox.Enabled = true;
            _startFromDateTextBox.Enabled = true;
            _startToDateTextBox.Enabled = true;
            _endFromDateTextBox.Enabled = true;
            _endToDateTextBox.Enabled = true;
            _targetNameTextBox.Enabled = true;
            _pctDoneTextBox.Enabled = true;
            _blksRepdTextBox.Enabled = true;
            _totBlksTextBox.Enabled = true;
            _priTextBox.Enabled = true;
            oneGuiGroupBox1.Enabled = true;
            _startCalendarButton.Enabled = true;
            _endCalendarButton.Enabled = true;


        }
        // ###########################################################################################################

        private void _liveRadioButton_Click(object sender, EventArgs e)
        {
            _executeButton.Text = "Get Live View";
            _timeRemainingTextBox.Text = _refreshTextBox.Text;
            _timeRemainingTextBox.BackColor = System.Drawing.Color.LightYellow;
            _refreshTextBox.Enabled = true;
            _tgtSystemTextBox.Enabled = true;
            _aggMBGroupBox.Visible = true;
            _aggMBTextBox.Visible = true;

            // disable
            _refreshNowButton.Enabled = false;
            _endFromDateTextBox.Enabled = false;
            _endToDateTextBox.Enabled = false;
            _statusCheckedListBox.Enabled = false;
            _endCalendarButton.Enabled = false;

        }
        // ###########################################################################################################

        private void timer_Tick(object sender, EventArgs e)
        {
            if (_timeRemaining == 0)
            {
                _timer.Enabled = false;
                _timer.Stop();
                // restore _timeRemaining
                _timeRemaining = Convert.ToInt16(_refreshTextBox.Text);
                _timeRemainingTextBox.Text = Convert.ToString(_timeRemaining);

                // Before firing the query, check if the connection is still there - user could have disconn'd
                // while the timer was running

                if (_connectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)     // conn bye-bye
                {
                    ResetFilters();
                    return;
                }

                // run what's in queryText
                DoQuery();
            }
            else
            {
                _timeRemaining--;
                _timeRemainingTextBox.Text = Convert.ToString(_timeRemaining);
            }
        }
        // ###########################################################################################################

        private void _refreshNowButton_Click(object sender, EventArgs e)
        {
            // Refresh Now...
            //   stop timer
            //   restore original time remaining
            //   run live query
            //   start timer
            // This could be the first button clicked after switching to Live, so behave like Live was requested

            _timer.Stop();
            _timeRemaining = Convert.ToInt16(_refreshTextBox.Text);
            _timeRemainingTextBox.Text = _refreshTextBox.Text;
            _executeButton.Text = this.StopLivRefreshText;
            _refreshTextBox.Enabled = false;        // disable changing refresh interval
            _timer.Interval = 1000;                 // one second updates to display
            DoQuery();                              // queryText still has it
        }
        // ###########################################################################################################

        // ###########################################################################################################

        private void startCalendarButton_Click(object sender, EventArgs e)
        {
            MyCalendarPopUp localCalendar = new MyCalendarPopUp(this, "START");
            localCalendar.ShowDialog();
        }
        // ###########################################################################################################

        private void _endCalendarButton_Click(object sender, EventArgs e)
        {
            MyCalendarPopUp localCalendar = new MyCalendarPopUp(this, "END");
            localCalendar.ShowDialog();
        }

        // ###########################################################################################################

        private void _targetNameTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _targetNameTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _targetNameText = " AND TARGET_NAME LIKE UPSHIFT('%" + _targetNameTextBox.Text + "%')"; }
            else
            { _targetNameText = null; }
        }
        // ###########################################################################################################

        private void _resetFiltersButton_Click(object sender, EventArgs e)
        {
            ResetFilters();
        }
        // ###########################################################################################################

        public void updateCounters()
        {
            ColorizeDataGrid();

            if (_liveRadioButton.Checked || _executeButton.Text.Equals(StopLivRefreshText))
            {
                _timer.Enabled = true;
                _timer.Start();
            }

            double CmpRatio;
            double _AggBytesNow = 0.0;        

            object[] PKCols = new object[3];

            
            // this is needed for history and live view to reformat the dates.
            //_monitorWorkloadIGrid.Cols["START TIME"].CellStyle.Format = "yyyy'-'MM'-'dd' 'HH':'mm':'ss";
            //_monitorWorkloadIGrid.Cols["END TIME"].CellStyle.Format = "yyyy'-'MM'-'dd' 'HH':'mm':'ss";

            // Compute MB rate based on num blocks and cmp ratio in the prior interval. 
            // for each item in the grid that is IN_PROGRESS, look for a matching item in _prevIntervalBDR DataTable.
            // If found, take difference in blocks rep'd and calc bytes rep'd
            // If not found, calc bytes rep'd, if compression ratio not zero; add to PREV DataTable regardless
            // Since Live View also shows 'completed within past 2 minutes', if completed, delete entry from the PREV DataTable.

            for (int i = 0; i < _monitorWorkloadIGrid.Rows.Count; i++)
            {
                PKCols[0] = _monitorWorkloadIGrid.Rows[i].Cells["START TIME"].Value.ToString();
                PKCols[1] = _monitorWorkloadIGrid.Rows[i].Cells["SOURCE NAME"].Value.ToString();
                PKCols[2] = _monitorWorkloadIGrid.Rows[i].Cells["TARGET SYSTEM"].Value.ToString();
                
                DataRow _foundRow = _prevIntervalBDR.Rows.Find(PKCols);

                if (_monitorWorkloadIGrid.Rows[i].Cells["STATUS"].Value.ToString() == "IN_PROGRESS")
                {
                    CmpRatio = Convert.ToDouble(_monitorWorkloadIGrid.Rows[i].Cells["CMPRSN RATIO"].Value);

                    if (_foundRow != null )         // got it
                    {                    
                        if (CmpRatio != 0.0)
                        { 
                            // display these values...
                            //MessageBox.Show("prev: " + _foundRow.Field<Int64>(3).ToString() +
                            //                "\nNow: " + _resultsDataGridView.Rows[i].Cells["BLKS REPD"].Value.ToString());

                            _AggBytesNow +=  Convert.ToDouble(
                                                (Convert.ToInt64(_monitorWorkloadIGrid.Rows[i].Cells["BLKS REPD"].Value) 
                                                - _foundRow.Field<Int64>(3)) * 32768 / CmpRatio );
                        }
                        _foundRow["BlocksReplicated"] = _monitorWorkloadIGrid.Rows[i].Cells["BLKS REPD"].Value;

                    }
                    else                            // not found
                    {
                        if (CmpRatio != 0.0)
                        {
                            _AggBytesNow += Convert.ToDouble(
                                              Convert.ToInt64(_monitorWorkloadIGrid.Rows[i].Cells["BLKS REPD"].Value) * 32768 / CmpRatio);
                        }

                        _prevIntervalBDR.Rows.Add(PKCols[0], PKCols[1], PKCols[2], Convert.ToInt64(_monitorWorkloadIGrid.Rows[i].Cells["BLKS REPD"].Value));
                    }
                    
                    
                }
                else
                    if (_monitorWorkloadIGrid.Rows[i].Cells["STATUS"].Value.ToString() == "COMPLETED") 
                    {
                        if (_foundRow != null) _foundRow.Delete();
                    }

            }

            // _refreshTextBox holds the refresh timer value
            _aggMBTextBox.Text = ((_AggBytesNow ) / OneMB / Convert.ToDouble(_refreshTextBox.Text)).ToString("N1");

        }
        // ###########################################################################################################

        private void _pctDoneTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _pctDoneTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _pctDoneText = _pctDoneTextBox.Text; }
            else
            { _pctDoneText = null; }
        }
        // ###########################################################################################################

        private void _blksRepdTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _blksRepdTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _blksRepdText = _blksRepdTextBox.Text; }
            else
            { _blksRepdText = null; }
        }
        // ###########################################################################################################

        private void _totBlksTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _totBlksTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _totBlksText = _totBlksTextBox.Text; }
            else
            { _totBlksText = null; }
        }
        // ###########################################################################################################

        private void _priTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _priTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _priText = _priTextBox.Text; }
            else
            { _priText = null; }
        }
        // ###########################################################################################################

        private void _cncrncyTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _cncrncyTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _concurrencyText = _cncrncyTextBox.Text; }
            else
            { _concurrencyText = null; }
        }
        // ###########################################################################################################

        private void _userIDTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _userIDTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _userIDText = " AND USERID LIKE UPSHIFT('%" + _userIDTextBox.Text + "%')"; }
            else
            { _userIDText = null; }
        }
        // ###########################################################################################################

        private void _pgdataYRadioButton_Click(object sender, EventArgs e)
        {
            _prgdataText = " AND PURGEDATA = 'Y'";
        }

        private void _pgdataNRadioButton_Click(object sender, EventArgs e)
        {
            _prgdataText = " AND PURGEDATA = 'N'";
        }

        private void _pgdataOffRadioButton_Click(object sender, EventArgs e)
        {
            _prgdataText = null;
        }
        // ###########################################################################################################

        private void _cmprsYRadioButton_Click(object sender, EventArgs e)
        {
            _compressText = " AND COMPRESS = 'Y'";
        }

        private void _cmprsNRadioButton_Click(object sender, EventArgs e)
        {
            _compressText = " AND COMPRESS = 'N'";
        }

        private void _cmprsOffRadioButton_Click(object sender, EventArgs e)
        {
            _compressText = null;
        }
        // ###########################################################################################################

        private void _valDataYRadioButton_Click(object sender, EventArgs e)
        {
            _valdataText = " AND VALIDATE_DATA = 'Y'";
        }

        private void _valDataNRadioButton_Click(object sender, EventArgs e)
        {
            _valdataText = " AND VALIDATE_DATA = 'N'";
        }

        private void _valDataOffRadioButton_Click(object sender, EventArgs e)
        {
            _valdataText = null;
        }
        // ###########################################################################################################

        private void _valDdlYRadioButton_Click(object sender, EventArgs e)
        {
            _valDdlText = " AND VALIDATE_DDL = 'Y'";
        }

        private void _valDdlNRadioButton_Click(object sender, EventArgs e)
        {
            _valDdlText = " AND VALIDATE_DDL = 'N'";
        }

        private void _valDdlOffRadioButton_Click(object sender, EventArgs e)
        {
            _valDdlText = null;
        }
        // ###########################################################################################################

        private void _crDdlYRadioButton_Click(object sender, EventArgs e)
        {
            _crDdlText = " AND CREATE_DDL = 'Y'";
        }

        private void _crDdlNRadioButton_Click(object sender, EventArgs e)
        {
            _crDdlText = " AND CREATE_DDL = 'N'";
        }

        private void _crDdlOffRadioButton_Click(object sender, EventArgs e)
        {
            _crDdlText = null;
        }
        // ###########################################################################################################

        private void _incrYRadioButton_Click(object sender, EventArgs e)
        {
            _incrText = " AND INCREMENTAL = 'Y'";
        }

        private void _incrNRadioButton_Click(object sender, EventArgs e)
        {
            _incrText = " AND INCREMENTAL = 'N'";
        }

        private void _incrOffRadioButton_Click(object sender, EventArgs e)
        {
            _incrText = null;
        }
        // ###########################################################################################################

        private void _tgtSystemTextBox_TextChanged(object sender, EventArgs e)
        {
            string buffer = _tgtSystemTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            { _tgtSystemText = " AND TARGET_SYSTEM LIKE UPSHIFT('" + _tgtSystemTextBox.Text + "%')"; }
            else
            { _tgtSystemText = null; }

        }

        private void _srcTypeCheckedListBox_SelectedIndexChanged(object sender, EventArgs e)
        {
        // no action...check the list when needed
        }

        private void MyWidgetControl_Load(object sender, EventArgs e)
        {
            SetDefaultStartDates();
            oneGuiSplitContainer1.SplitterDistance = oneGuiSplitContainer1.Size.Height / 3;
        }

        void SetDefaultStartDates()
        {
            DateTime today = DateTime.UtcNow;

            _startFromDateTextBox.Text = today.Date.ToString("yyyy-MM-dd HH:mm:ss");
            _startToDateTextBox.Text = today.ToString("yyyy-MM-dd HH:mm:ss");
        }

        private void MyWidgetControl_SizeChanged(object sender, EventArgs e)
        {
            oneGuiSplitContainer1.SplitterDistance = oneGuiSplitContainer1.Size.Height / 3;
        }
    }

    public class MyMonitorDataHandler : TabularDataDisplayHandler
    {
        private MyWidgetControl _myWidgetControl = null;

        public MyMonitorDataHandler(MyWidgetControl aMonitorWorkloadCanvas)
        {
            _myWidgetControl = aMonitorWorkloadCanvas;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable, TrafodionIGrid aDataGrid)
        {
            base.DoPopulate(aConfig, aDataTable, aDataGrid);
            aDataGrid.UpdateCountControlText("There are {0} replication records");
            _myWidgetControl.updateCounters();
        }

    }

}
