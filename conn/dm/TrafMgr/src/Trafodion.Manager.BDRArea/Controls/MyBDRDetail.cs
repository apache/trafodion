// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
using System;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using Trafodion.Manager.BDRArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.BDRArea.Controls
{
    public partial class MyBDRDetail : UserControl
    {
        #region Fields
        private static readonly string MyWidgets = "MyWidgets";
        static readonly char[] delimiters = new char[] { ' ', ':' };
        const double OneMB = 1024d * 1024d;
        const int BDRStatusInPhase2 = 9;
        string _myQID;
        string _queryText;
        ConnectionDefinition _connectionDefinition;
        bool _constructorError = false;
        long _exeElapsedTime;           // usec of exec; from Merge=10 result
        long _totalUncompBytes;
        long _totalCompBytes;
        double _bsfCompression = 0.0;
        int _refreshVSN = 0;
        int _whatPhaseBDR = 0;
        int _sqlError = 0;
        int _timeRemaining = 10;
        int _refreshInterval = 10;
        bool _firstFetch = true;
        TrafodionProgressUserControl _oneGuiProgressUserControl = null;
        string _targetObjectName = string.Empty;

        // RMS query results are name-value pairs embedded in a text item.
        // The following arrays facilitate the use/don't use logic.  Presence in the array means that name-value
        // should be exported to the DataTable.  C# Split() method makes it easy to populate the candidate names and
        // values - there must be pairs.

        // Some of the "time" fields are Julian timestamps, others are elapsed usec
        // The Julians are converted to datetime strings for display
        // First column is value from query; second column is value to display, third column for display order
        static readonly string[,] _merge10Keepers = new string[,] {

            //                          Attr name         |  Displayed              |  Display order
            //                        ====================+=========================+=================
                                       {"AnsiName",         "Source Object Name",       "1"},
                                       {"Status",           "Status",                   "2"},
                                       {"SqlErrorCode",     "SQL Error Code",           "3"},
                                       //                                               "4"  is in oddballs
                                       {"CompletedPartns",  "Partitions Completed",     "5"},
                                       {"ObjectType",       "Object Type",              "6"},
                                       {"ReplType",         "Replication Type",         "7"},
                                       {"SourceSystem",     "Source System",            "8"},
                                       {"TargetSystem",     "Target System",            "9"},
                                       {"NumParns",         "Number of Partitions",     "10"},
                                       //                                               "11"  is in oddballs
                                       {"CompStartTime",    "Compile Start Time",       "12"},
                                       {"CompEndTime",      "Compile End Time",         "13"}, 
                                       {"ExeStartTime",     "Execute Start Time",       "14"},
                                       {"ExeEndTime",       "Execute End Time",         "15"},
                                       {"compElapsedTime",  "Compile Elapsed Time",     "16"}, 
                                       {"exeElapsedTime",   "Execute Elapsed Time",     "17"}, 
                                       //in M6 - one of these will be found
                                       {"RowsReplicated",   "Rows Replicated",          "18"},    
                                       // replaces Rows Rep'd...in M7
                                       // in M7
                                       {"RowsRead",         "Rows Read",                "18"},
                                       {"TotalBlocks",      "Total Blocks",             "19"},
                                       {"BlocksReplicated", "Blocks Replicated",        "20"},
                                       {"BlocksRead",       "Blocks Read",              "21"},
                                       {"TotalCompTime",    "Total Compress Time",      "22"},
                                       {"TotalCompBytes",   "Total Compressed Bytes",   "23"},
                                       {"TotalUncompTime",  "Total Uncompress Time",    "24"},
                                       {"TotalUncompBytes", "Total Uncompressed Bytes", "25"},
                                       {"numSqlProcs",      "Number SQL Processes",     "26"}, 
                                       {"numCpus",          "Number CPUs",              "27"},

                                       {"parentQid",        "Parent QID",               "28"},
                                       {"childQid",         "Child QID",                "29"},
                                       {"exePriority",      "Execution Priority",       "30"}, 
                                       {"suspended",        "Suspended",                "31"}, 
                                       {"lastSuspendTime",  "Last Suspend Time",        "32"}, 
                                       {"phase0StartTime",  "Phase 0 Start Time",       "33"},
                                       {"phase0EndTime",    "Phase 0 End Time",         "34"},
                                       {"phase1StartTime",  "Phase 1 Start Time",       "35"},
                                       {"phase1EndTime",    "Phase 1 End Time",         "36"},
                                       {"phase2StartTime",  "Phase 2 Start Time",       "37"},
                                       {"phase2EndTime",    "Phase 2 End Time",         "38"},
                                       {"phase3StartTime",  "Phase 3 Start Time",       "39"},
                                       {"phase3EndTime",    "Phase 3 End Time",         "40"},
                                       {"phase4StartTime",  "Phase 4 Start Time",       "41"},
                                       {"phase4EndTime",    "Phase 4 End Time",         "42"},
                                       {"phase5StartTime",  "Phase 5 Start Time",       "43"},
                                       {"phase5EndTime",    "Phase 5 End Time",         "44"},
                                       {"phase6StartTime",  "Phase 6 Start Time",       "45"},
                                       {"phase6EndTime",    "Phase 6 End Time",         "46"},
                                       //"sqlSrc",                                      "47"     // special handling
                                       {"endendend",        "list ender",               "99"}
        };

        // PercentDone occurs in Type=19 & 20 - want the one in Type=19
        // Concurrency can occur in the SqlSrc command string in Type=15 - don't want that; want the one in Type=19
        // So have to make special checks
        static readonly string[,] _merge10OddBalls = new string[,] {
                                       {"PercentDone",      "Percent Done",             "4"},
                                       {"Concurrency",      "Concurrency",              "11"}
        };

        static readonly string[] _statusValues = new string [] {
                                        "INITIATED",            // 0
                                        "IN_PROGRESS",          // 1
                                        "DATA_REPLICATED",      // 2
                                        "COMPLETED",            // 3
                                        "FAILED",               // 4
                                        "ABORTED",              // 5
                                        "RECOVERED",            // 6
                                        "IN_PHASE0",            // 7
                                        "IN_PHASE1",            // 8
                                        "IN_PHASE2",            // 9
                                        "IN_PHASE3",            // 10
                                        "IN_PHASE4",            // 11
                                        "IN_PHASE5",            // 12
                                        "IN_PHASE6"             // 13
        };


        static readonly string[] _tdbidDetail1Keepers = new string[] {
                                       "ProcessName",                  // not in query col2, but here as place-holder
                                       "SourcePart",             // 1
                                       "TargetPart",             // 2
                                       "PercentDone",            // 3 *   // M7
                                       "TotalBlocks",            // 4 
                                       "BlocksRead",             // 5 *   // M7
                                       "BlocksReplicated",       // 6 *
                                       "RowsRead",               // 7 *   // M7
                                            /*"RowsReplicated",         // 7 *   // M6   */
                                       "TotalCompBytes",         // 8 *
                                       "TotalUncompBytes",       // 9 *
                                       "BlockLen",               // 10
                                       "OperCpuTime",            // 11 *
                                       "TotalCompTime",          // 12 *    
                                       "TotalUncompTime",        // 13 * 
                                       "REFRESHVSN"              // this will never appear in input; it's here as space holder
                                                                 //  use to version check what's been saved against what RMS returns
        };


        DataTable _merge10DataTable = new DataTable();
        DataTable _tdbidDetail1DataTable = new DataTable();


        #endregion Fields
        // ###########################################################################################################

        #region Properties

        #endregion Properties

        // ###########################################################################################################

        public MyBDRDetail(string aQID,string aTargetObjectName, ConnectionDefinition aConnectionDef)
        {
            InitializeComponent();

            _myQID = aQID;
            _targetObjectName = aTargetObjectName;
            _connectionDefinition = aConnectionDef;

            oneGuiPanel2.Dock = DockStyle.Fill;

            // canvas setup
            _bdrStatswidgetCanvas.ThePersistenceKey = MyWidgets;
            _bdrStatswidgetCanvas.Dock = DockStyle.Fill;
            GridLayoutManager gridLayoutManager = new GridLayoutManager(1, 6);   // 10 row n col
            _bdrStatswidgetCanvas.LayoutManager = gridLayoutManager;
            
            //Add the first control to the canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 2);
            WidgetContainer widgetContainer = new WidgetContainer(_bdrStatswidgetCanvas, _merge10DataGridView, "Progress");
            widgetContainer.Name = "Progess";
            widgetContainer.AllowDelete = false;
            _bdrStatswidgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);
            
            //Add the next control to the canvas
            gridConstraint = new GridConstraint(0, 2, 1, 4);
            widgetContainer = new WidgetContainer(_bdrStatswidgetCanvas, _partitionsDataGridView, "Active Partitions");
            widgetContainer.Name = "Partitions";
            widgetContainer.AllowDelete = false;
            _bdrStatswidgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            // add to datagridview
            //_merge10DataGridView.AddCountControlToParent("There are {0} rows", DockStyle.Top);
            _merge10DataGridView.AddButtonControlToParent(DockStyle.Bottom);

            _partitionsDataGridView.AddCountControlToParent("There are {0} active partitions", DockStyle.Top);
            _partitionsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

            // canvas layout complete

            // set the columns for the DataTables
            _merge10DataTable.Columns.Add("Attribute", typeof(string));
            _merge10DataTable.Columns.Add("Value", typeof(string));
            _merge10DataTable.Columns.Add("Sequence", typeof(Int16));
            
            // ordering of these columns must match ordering in the "keepers" list defined above
            // position within the array is used when the value is added to the row

            // FYI: when export grid to XLS, Int64 & long result in strings (left justified) and Excel flags the cell
            //      int exports OK.
            //      Risky to reduce the data type just to make Excel happy
            _tdbidDetail1DataTable.Columns.Add("Process Name", typeof(string));         // 0
            _tdbidDetail1DataTable.Columns.Add("SourcePart", typeof(string));           // 1
            _tdbidDetail1DataTable.Columns.Add("TargetPart", typeof(string));           // 2
            _tdbidDetail1DataTable.Columns.Add("Pct Done", typeof(int));                // 3*
            _tdbidDetail1DataTable.Columns.Add("Total Blocks", typeof(Int64));          // 4
            _tdbidDetail1DataTable.Columns.Add("Blocks Read", typeof(Int64));           // 5 *  // M7
            _tdbidDetail1DataTable.Columns.Add("Blocks Repd", typeof(Int64));           // 6*
            _tdbidDetail1DataTable.Columns.Add("Rows Read", typeof(Int64));             // 7*    // M7
                    /*_tdbidDetail1DataTable.Columns.Add("Rows Replicated", typeof(Int64));       // 7*    // M6  */
            _tdbidDetail1DataTable.Columns.Add("Total Comp MB", typeof(string));        // 8*
            _tdbidDetail1DataTable.Columns.Add("Total Uncomp MB", typeof(string));      // 9*
            _tdbidDetail1DataTable.Columns.Add("Block Len", typeof(int));               // 10
            _tdbidDetail1DataTable.Columns.Add("Oper Cpu Sec", typeof(string));         // 11*
            _tdbidDetail1DataTable.Columns.Add("Total Comp Sec", typeof(string));       // 12*
            _tdbidDetail1DataTable.Columns.Add("Total Uncomp Sec", typeof(double));     // 13* 

            // need this to track updates
            _tdbidDetail1DataTable.Columns.Add("RefreshVSN", typeof(int));              // 14

            // define SourcePart as PK for the details so we can update 
            _tdbidDetail1DataTable.PrimaryKey = new DataColumn[] { _tdbidDetail1DataTable.Columns[1] };

            _refreshTextBox.Text = _timeRemaining.ToString();
            _timeRemainingTextBox.Text = _refreshTextBox.Text;


        }
        // ###########################################################################################################

        private void DoSqlMerge10()
        {
            toolStripProgressBar1.Visible = true;
            TrafodionProgressArgs args = new TrafodionProgressArgs("Fetching header information", this, "FetchSQLMerge10", new object[0]);
            _oneGuiProgressUserControl = new TrafodionProgressUserControl(args);
            _oneGuiProgressUserControl.ProgressCompletedEvent += SQLMerge10_ProgressCompletedEvent;
        }

        public DataTable FetchSQLMerge10()
        {
                BDRSystemModel sqlQuery = BDRSystemModel.FindSystemModel(_connectionDefinition);
                
                _queryText = "SELECT variable_info from table(statistics(NULL, 'QID=" +
                             _myQID +
                             ",MERGE=10'))";
             return sqlQuery.GetQueryResults(_queryText);

        }
        void SQLMerge10_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            _oneGuiProgressUserControl.ProgressCompletedEvent -= SQLMerge10_ProgressCompletedEvent;
            toolStripProgressBar1.Visible = false;

            int result = -1;
            if (e.Error != null)
            {
                if (!e.Error.Message.ToUpper().Contains("NOT FOUND"))       // not found is OK
                {
                    MessageBox.Show(e.Error.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
            else
            {
                DataTable tmpData = new DataTable();
                if (e.ReturnValue is DataTable)
                {
                    tmpData = e.ReturnValue as DataTable;
                    result = SqlMerge10(e.ReturnValue as DataTable);
                }
            }

            if (_firstFetch && result != 0)
            {
                this.oneGuiPanel2.Controls.Remove(_bdrStatswidgetCanvas);        // remove canvas from panel2
                Trafodion.Manager.Framework.Controls.TrafodionTextBox errorTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();       // new control
                this.oneGuiPanel2.Controls.Add(errorTextBox);                          // put in panel2
                // change the look
                errorTextBox.Dock = DockStyle.Fill;
                errorTextBox.Text = "Requested Query_ID was not found, no data is available.  Please close this window.";
                errorTextBox.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold);
                errorTextBox.BackColor = Color.Red;

                _refreshGuiButton.Enabled = false;
            }
            else
            {
                if (_whatPhaseBDR == 3)     // it's finished
                {
                    ColorizeDataGrid();
                    _refreshGuiButton.Text = "Resume Refresh";
                    MessageBox.Show("The replication has completed - no new statistics are available.  \nAuto refresh has been stopped.  The details window can be closed.",
                                    "FYI", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }
                else
                {
                    DoFetchSqlTbidDetail();
                }
            }
            _firstFetch = false;

        }
        
        /* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
         * Two queries are used to get RMS stats:
         *   MERGE=10
         *   TDBID_DETAIL=1
         * MERGE10 results consider as "header" - BDR progress
         * _DETAIL1 results treat as "detail" - partition details
         *    One row per active part + 1 is returned;  the *+1* row has no useful info and we ignore it
         * 
         * Merge10 results in *SqlErrorCode* and *Status* indicate whether _Detail1 is necessary:
         *   :  Before Phase 2, no details are available, but the data handling can proceed as normal
         *   :  In Phase 2, header and details are present...normal processing
         *   :  Phase 3, *Status* indicates Phase3 and no error in *SqlErrorCode*
         *               detail returns the *+1* row
         *   :  Post-phase 3, Transform might run (Phase 4), but this s/b indicated in *Status*
         *               detail returns *+1* row or none at all
         *   :  DONE, *Status* indicates completed and *SqlErrorCode* has 100
         *               NO detail rows
         * $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
         */

        /// <summary>
        /// Execute MERGE=10 query and parse out the interesting name-value pairs
        /// DataTable the result; map to datagridview
        /// </summary>
        /// <returns>
        /// true if error occurred
        /// </returns>
        private int SqlMerge10(DataTable tempdata)
        {
            string[] words;
            string datetimestring;

            _statsTimeTextBox.Text = DateTime.UtcNow.ToString("yyyy-MM-dd HH:mm:ss");

            // This query always returns something...clear the previous data
            _merge10DataTable.Clear();

            try
            {
                string replicateCmd= null;


                // parse the RMS glob and format into _merge10DataTable
                foreach (DataRow r in tempdata.Rows)
                {
                    words = r.Field<string>(0).Split(delimiters, System.StringSplitOptions.RemoveEmptyEntries);
                    if (words[0].ToUpper() == "STATSROWTYPE" && words[1] == "1") continue;

                    // stattype 15 has SQLSRC...special handling
                    if (words[0].ToUpper() == "STATSROWTYPE" && words[1] == "15")
                    {
                        Match m = Regex.Match (r.Field<string>(0), @"sqlsrc: (.+)", RegexOptions.IgnoreCase); 
                        replicateCmd = m.Groups[1].ToString();
                        //MessageBox.Show("Rep Cmd: " + replicateCmd);
                    }

                    #region regular_check
                    for (int n = 0; n < _merge10Keepers.GetLength(0); n++)
                    {
                        for (int i = 0; i < words.Length; i++)
                        {
                            if (_merge10Keepers[n,0].ToUpper() == words[i].ToUpper())
                            {   // add the row to the result table, but first...
                                // if the attribute has "time" in it, the value is a juliantimestamp
                                //  convert this to a date/time string
                                if (_merge10Keepers[n, 0].ToUpper().Contains("STARTTIME") ||
                                    _merge10Keepers[n, 0].ToUpper().Contains("ENDTIME") ||
                                    _merge10Keepers[n, 0].ToUpper().Contains("SUSPENDTIME"))
                                {
                                    long k = Convert.ToInt64(words[i + 1]);
                                    // time value could be missing
                                    if (k == -1) _merge10DataTable.Rows.Add(_merge10Keepers[n, 1], words[i + 1], Convert.ToInt16(_merge10Keepers[n, 2]));
                                    else
                                    {
                                        datetimestring = Trafodion.Manager.Framework.Utilities.FormattedJulianTimestamp(k);
                                        _merge10DataTable.Rows.Add(_merge10Keepers[n, 1], datetimestring, Convert.ToInt16(_merge10Keepers[n, 2]));
                                    }
                                }
                                else
                                {
                                        switch (words[i].ToUpper())
                                        {
                                            case "EXEELAPSEDTIME":
                                                _exeElapsedTime = Convert.ToInt64(words[i + 1]);
                                                // convert to sec (truncated) and put it back
                                                words[i + 1] = (Convert.ToDouble(_exeElapsedTime) / 1000000d).ToString("N1") + " seconds";
                                                break;

                                            case "COMPELAPSEDTIME":
                                                long _compElapsedTime = Convert.ToInt64(words[i + 1]);
                                                words[i + 1] = (Convert.ToDouble(_compElapsedTime) / 1000000d).ToString("N1") + " seconds";
                                                break;

                                            case "NUMPARNS":
                                                _numPartsTextBox.Text = words[i + 1];
                                                break;

                                            case "COMPLETEDPARTNS":
                                                _partsCompletedTextBox.Text = words[i + 1];
                                                break;

                                            case "TOTALCOMPBYTES":
                                                _totalCompBytes = Convert.ToInt64(words[i + 1]);
                                                words[i + 1] = (Convert.ToDouble(_totalCompBytes) / OneMB).ToString("N1") + " MB";
                                                break;

                                            case "TOTALUNCOMPBYTES":
                                                _totalUncompBytes = Convert.ToInt64(words[i + 1]);
                                                words[i + 1] = (Convert.ToDouble(_totalUncompBytes) / OneMB).ToString("N1") + " MB";
                                                break;

                                            case "TOTALCOMPTIME":
                                            case "TOTALUNCOMPTIME":
                                                words[i + 1] = ((Convert.ToDouble(words[i + 1]) / 1000000d)).ToString("N1") + " seconds";
                                                break;

                                            case "ROWSREAD":
                                            case "ROWSREPLICATED":
                                            case "TOTALBLOCKS":
                                            case "BLOCKSREPLICATED":
                                            case "BLOCKSREAD":
                                                words[i + 1] = (Convert.ToInt64(words[i + 1])).ToString("N0");
                                                break;

                                            case "STATUS":
                                                _whatPhaseBDR = Convert.ToInt16(words[i + 1]);
                                                words[i + 1] = _statusValues[_whatPhaseBDR];
                                                break;

                                            case "SQLERRORCODE":
                                                // save this result...it's the actual error indication
                                                _sqlError = Convert.ToInt16(words[i + 1]);
                                                break;

                                        }

                                        _merge10DataTable.Rows.Add(_merge10Keepers[n, 1], words[i + 1], Convert.ToInt16(_merge10Keepers[n, 2]));
                                }


                            } // endif
                        }  // for words parsed
                    }  // for n
                    #endregion regular_check

                    #region oddball_check
                    // OddBalls check
                    if (words[0].ToUpper() == "STATSROWTYPE" && words[1] == "19")
                    {
                        for (int k=0; k <_merge10OddBalls.GetLength(0); k++)
                        {
                            for (int j = 0; j < words.Length; j++)
                            {
                                if (_merge10OddBalls[k, 0].ToUpper() == words[j].ToUpper())
                                    _merge10DataTable.Rows.Add(_merge10OddBalls[k, 1], words[j + 1], Convert.ToInt16(_merge10OddBalls[k, 2]));

                                switch (words[j].ToUpper())
                                {
                                    case "PERCENTDONE":
                                        _pctDoneTextBox.Text = words[j + 1];
                                        break;
                                }
                            }
                        }
                    }
                    #endregion oddball_check
                }  // foreach datarow

                // add the target Object Name string...sequence at 47,end of all attributes
                _merge10DataTable.Rows.Add("Target Object Name", _targetObjectName, 47);

                // add the command string...sequence at end of list
                _merge10DataTable.Rows.Add("SQL Command", replicateCmd,99);
                
    
                // done...put into gridview
                _merge10DataGridView.DataSource = _merge10DataTable;

                // Fill the text boxes in the "banner" area at the top...
                
                // about this rate calc...
                // totalcompbytes and exeElapsed come from BDR Master.  BPS calc is OK when MAX CONCURRENCY; otherwise
                // total bytes includes parts that have finished, thus reducing BPS, which is intended to show xfer rate
                // of the parts that are active.  There is no exe timer for the individual parts, so BPS for concurrency
                // less than MAX is tainted.
                if (_exeElapsedTime > 0)
                {
                    double bps = Convert.ToDouble(_totalCompBytes) / OneMB / (Convert.ToDouble(_exeElapsedTime) / 1000000D);
                    _byteRateCompTextBox.Text = bps.ToString("N2");
                     bps = Convert.ToDouble(_totalUncompBytes) / OneMB / (Convert.ToDouble(_exeElapsedTime) / 1000000D);
                    _byteRateUncompTextBox.Text = bps.ToString("N2");
                }
                else
                    _byteRateCompTextBox.Text = 0.ToString("N0");

                if (_totalCompBytes > 0)
                {
                    double compRatio = Convert.ToDouble(_totalUncompBytes) / Convert.ToDouble(_totalCompBytes);
                    if (compRatio > _bsfCompression) _bsfCompression = compRatio;
                    _bestCompressionTextBox.Text = _bsfCompression.ToString("N1");
                }
            }
            catch (Exception ex)
            {
                Cursor.Current = Cursors.Default;
                if (ex.Message.ToUpper().Contains("NOT FOUND")) return -1;      // not found is OK

                MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return -1;
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
            return _sqlError;
        }
        // ###########################################################################################################

        void DoFetchSqlTbidDetail()
        {
            toolStripProgressBar1.Visible = true;
            TrafodionProgressArgs args = new TrafodionProgressArgs("Fetching header information", this, "FetchSqlTbidDetail", new object[0]);
            _oneGuiProgressUserControl = new TrafodionProgressUserControl(args);
            _oneGuiProgressUserControl.ProgressCompletedEvent += FetchSqlTdbidDetail1_ProgressCompletedEvent;
        }

        public DataTable FetchSqlTbidDetail()
        {
            BDRSystemModel sqlQuery = BDRSystemModel.FindSystemModel(_connectionDefinition);
            _queryText = "SELECT text,variable_info from table(statistics(NULL, 'QID=" +
                         _myQID +
                         ",TDBID_DETAIL=1'))";

            // DataTable to catch the 2 columns of RMS output
            return sqlQuery.GetQueryResults(_queryText);
        }

        void FetchSqlTdbidDetail1_ProgressCompletedEvent(object sender, TrafodionProgressCompletedArgs e)
        {
            toolStripProgressBar1.Visible = false;
            _oneGuiProgressUserControl.ProgressCompletedEvent -= FetchSqlTdbidDetail1_ProgressCompletedEvent;
            if (e.Error != null)
            {
                MessageBox.Show(e.Error.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            else
            {
                if (e.ReturnValue is DataTable)
                {
                    SqlTdbidDetail1(e.ReturnValue as DataTable);
                    _timer.Enabled = true;
                    _timer.Start();
                }
            }
        }

        /// <summary>
        /// Execute TDBID_DETAIL=1 query for status of the running partitions
        /// Compute a couple of new columns (%done, byte rate); 
        /// DataTable result; datagridview; colorize
        /// </summary>
        private void SqlTdbidDetail1(DataTable tempdata)
        {
            string[] words;
            object[] obj = new object[_tdbidDetail1Keepers.Length];

            try
            {
               
                // first column of result is ProcessName, second column needs parsing into piece parts
                foreach (DataRow r in tempdata.Rows)
                {
                    words = r.Field<string>(1).Split(delimiters, System.StringSplitOptions.RemoveEmptyEntries);
                    // one rows has all counters=0...it has no entry for source & target...skip it
                    if (Array.IndexOf(words,"SourcePart") == (Array.IndexOf(words,"TargetPart") - 1)) continue;
                    
                    Match m = Regex.Match(r.Field<string>(0), @"(\S+)");
                    obj[0] = m.Groups[1].ToString();

                    foreach (string attribute in _tdbidDetail1Keepers)
                    //  fields from the result column turn into columns in the datatable
                    //  construct a DataRow
                    {
                        for (int i = 0; i < words.Length; i++)
                        {
                            if (attribute.ToUpper() == words[i].ToUpper())
                            {  // add words[i+1] to the insert value list

                                // handling specific words
                                switch (words[i].ToUpper())
                                {
                                    case "TOTALCOMPBYTES":      // change to MB
                                    case "TOTALUNCOMPBYTES":    // change to MB
                                        obj[Array.IndexOf(_tdbidDetail1Keepers, attribute)] = (Convert.ToDouble(words[i + 1]) / OneMB).ToString(".000");
                                        break;

                                    case "OPERCPUTIME":         // change usec to sec
                                    case "TOTALCOMPTIME":
                                    case "TOTALUNCOMPTIME":
                                        obj[Array.IndexOf(_tdbidDetail1Keepers,attribute)] = (Convert.ToDouble( words[i + 1]) / 1000000d).ToString(".000");
                                        break;

                                    case "TOTALBLOCKS":
                                    case "BLOCKSREPLICATED":
                                    case "ROWSREPLICATED":
                                        obj[Array.IndexOf(_tdbidDetail1Keepers, attribute)] = Convert.ToInt64(words[i + 1]);
                                        break;
                                 

                                    default:
                                        obj[Array.IndexOf(_tdbidDetail1Keepers,attribute)] = words[i + 1];
                                        break;
                                } // switch

                            }

                        }
                    }

                    // add new and update existing
                    try
                    {
                        obj[Array.IndexOf(_tdbidDetail1Keepers, "REFRESHVSN")] = _refreshVSN;
                        _tdbidDetail1DataTable.Rows.Add(obj);
                    }
                    catch (Exception x)
                    {
                        if (x.Message.ToUpper().Contains("UNIQUE"))
                        {
                            //update
                            DataRow[] updateRow = _tdbidDetail1DataTable.Select("SourcePart = '" + obj[Array.IndexOf(_tdbidDetail1Keepers, "SourcePart")].ToString() + "'");


                            // BlocksRead & RowsRead & %Done stat M7
                            updateRow[0]["Blocks Read"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "BlocksRead")];
                            updateRow[0]["Rows Read"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "RowsRead")];
                            updateRow[0]["Pct Done"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "PercentDone")];

                            updateRow[0]["Blocks Repd"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "BlocksReplicated")];
                            updateRow[0]["Total Comp MB"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "TotalCompBytes")];
                            updateRow[0]["Total Uncomp MB"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "TotalUncompBytes")];
                            updateRow[0]["Oper Cpu Sec"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "OperCpuTime")];
                            updateRow[0]["Total Comp Sec"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "TotalCompTime")];
                            updateRow[0]["Total Uncomp Sec"] = obj[Array.IndexOf(_tdbidDetail1Keepers, "TotalUncompTime")];
                            updateRow[0]["RefreshVSN"] = _refreshVSN;
                        }
                        else    // something wrong
                        {
                            MessageBox.Show(x.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                            break;
                        }
                    }

                } // foreach datarow

                _partitionsDataGridView.DataSource = _tdbidDetail1DataTable;
            }
            catch (Exception ex)
            {
                //MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                MessageBox.Show(ex.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        // ###########################################################################################################

        private void MyBDRDetail_Shown(object sender, EventArgs e)
        {
            // LOAD event  set to call this
            // if Merge10 query fails - no need to continue with window
            DoSqlMerge10();
            //if (SqlMerge10() != 0)
            //{
            //    this.oneGuiPanel2.Controls.Remove(_bdrStatswidgetCanvas);        // remove canvas from panel2
            //    Trafodion.Manager.Framework.Controls.TrafodionTextBox errorTextBox = new Trafodion.Manager.Framework.Controls.TrafodionTextBox();       // new control
            //    this.oneGuiPanel2.Controls.Add(errorTextBox);                          // put in panel2
            //    // change the look
            //    errorTextBox.Dock = DockStyle.Fill;
            //    errorTextBox.Text = "Requested Query_ID was not found, no data is available.  Please close this window.";
            //    errorTextBox.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold);
            //    errorTextBox.BackColor = Color.Red;

            //    _refreshGuiButton.Enabled = false;
            //}
            //else
            //{
            //    SqlTdbidDetail1();
            //    _timer.Start();
            //    _refreshGuiButton.Text = "Pause Refresh";
            //}

        }

        // ###########################################################################################################

        private void _refreshGuiButton_Click(object sender, EventArgs e)
        {
            if (_refreshGuiButton.Text == "Pause Refresh")
            {
                _refreshGuiButton.Text = "Resume Refresh";
                _timer.Stop();
            }
            else
                if (_refreshGuiButton.Text == "Resume Refresh")
                {
                    _refreshGuiButton.Text = "Pause Refresh";
                    _timer.Start();
                }
        }

        // ###########################################################################################################

        private void ColorizeDataGrid()
        {
            // colorize the partitions datagridview
            for (int rownum = 0; rownum < _partitionsDataGridView.Rows.Count ; rownum++)
            {
                if (Convert.ToInt64( _partitionsDataGridView.Rows[rownum].Cells["Total Blocks"].Value) >
                      Convert.ToInt64(_partitionsDataGridView.Rows[rownum].Cells["Blocks Repd"].Value))
                {
                    _partitionsDataGridView.Rows[rownum].DefaultCellStyle.BackColor = Color.Yellow;
                }
                else
                    if (Convert.ToInt64(_partitionsDataGridView.Rows[rownum].Cells["Total Blocks"].Value) ==
                          Convert.ToInt64(_partitionsDataGridView.Rows[rownum].Cells["Blocks Repd"].Value))
                {
                    _partitionsDataGridView.Rows[rownum].DefaultCellStyle.BackColor = Color.LawnGreen;
                }

                // check for parts that did not see an update at last refresh
                // if it was yellow, make it gray - must have completed
                if (Convert.ToInt64(_partitionsDataGridView.Rows[rownum].Cells["RefreshVSN"].Value) < _refreshVSN)
                {
                    if (_partitionsDataGridView.Rows[rownum].DefaultCellStyle.BackColor == Color.Yellow)
                        _partitionsDataGridView.Rows[rownum].DefaultCellStyle.BackColor = Color.Gray;
                }

            } // for

        }
        // ###########################################################################################################

        private void _partitionsDataGridView_DataBindingComplete(object sender, DataGridViewBindingCompleteEventArgs e)
        {
            // A partition could have completed since the last refresh.  If that part wasn't reported in the details,
            // its RefreshVSN will be less than the current value.  Colorize will note the distinction.
 
            // Column "Total Uncomp Sec" has no meaningful data for this release...don't show this column
            _partitionsDataGridView.Columns["Total Uncomp Sec"].Visible = false;

            _partitionsDataGridView.Columns["RefreshVSN"].Visible = false;

            // M7...prefer not show this...Blocks Read good enuf
            //_partitionsDataGridView.Columns["Rows Read"].Visible = false;         

            // set column attributes
            _partitionsDataGridView.Columns[0].Width = 70;
            _partitionsDataGridView.Columns[1].Width = 130;
            _partitionsDataGridView.Columns[2].Width = 130;
            _partitionsDataGridView.Columns[3].Width = 35;
            _partitionsDataGridView.Columns[4].Width = 50;
            _partitionsDataGridView.Columns[5].Width = 50;
            _partitionsDataGridView.Columns[6].Width = 50;
            _partitionsDataGridView.Columns[7].Width = 50;
            _partitionsDataGridView.Columns[8].Width = 50;
            _partitionsDataGridView.Columns[9].Width = 50;
            _partitionsDataGridView.Columns[10].Width = 50;
            _partitionsDataGridView.Columns[11].Width = 50;
            _partitionsDataGridView.Columns[12].Width = 50;
            _partitionsDataGridView.Columns[13].Width = 50;

            _partitionsDataGridView.Columns[3].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[4].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[5].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[6].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[7].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[8].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[9].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[10].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[11].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;
            _partitionsDataGridView.Columns[12].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleRight;

            ColorizeDataGrid();
        }
        // ###########################################################################################################

        private void _pctDoneTextBox_TextChanged(object sender, EventArgs e)
        {
            if (_pctDoneTextBox.Text.ToString() == "100") _pctDoneTextBox.BackColor = Color.LawnGreen;
        }
        // ###########################################################################################################

        private void _merge10DataGridView_DataBindingComplete(object sender, DataGridViewBindingCompleteEventArgs e)
        {
            // Sequence column is not visible, but sort on it
            _merge10DataGridView.Columns["Sequence"].Visible = false;
            _merge10DataGridView.Sort(_merge10DataGridView.Columns["Sequence"], ListSortDirection.Ascending);

            // Adjust the appearance of some grid cells
            _merge10DataGridView.Columns[0].Width = 200;
            _merge10DataGridView.Columns[1].Width = 200;

            // Handle specific row conditions...
            for (int rownum = 0; rownum < _merge10DataGridView.Rows.Count; rownum++)
            {
                // Find the row for "AnsiName" and BOLD the name
                if (_merge10DataGridView.Rows[rownum].Cells["Attribute"].Value.ToString() == "ANSI Object Name")
                {
                    _merge10DataGridView.Rows[rownum].Cells["Value"].Style.Font = new Font("Tahoma", 9, FontStyle.Bold);
                    
                }

                // Highlight SQL Error when NOT ZERO
                if (_merge10DataGridView.Rows[rownum].Cells["Attribute"].Value.ToString() == "SQL Error Code" &&
                    _merge10DataGridView.Rows[rownum].Cells["Value"].Value.ToString() != "0")
                {
                    if (Convert.ToInt16(_merge10DataGridView.Rows[rownum].Cells["Value"].Value) < 0)
                        _merge10DataGridView.Rows[rownum].Cells["Value"].Style.BackColor = Color.Red;
                    else
                        _merge10DataGridView.Rows[rownum].Cells["Value"].Style.BackColor = Color.PaleGoldenrod;

                }

                // Highlight Status 
                if (_merge10DataGridView.Rows[rownum].Cells["Attribute"].Value.ToString() == "Status" &&
                    _merge10DataGridView.Rows[rownum].Cells["Value"].Value.ToString() == "IN_PROGRESS")
                {
                    _merge10DataGridView.Rows[rownum].Cells["Value"].Style.BackColor = Color.Yellow;
                }
                else
                    if (_merge10DataGridView.Rows[rownum].Cells["Attribute"].Value.ToString() == "Status" &&
                        ( _merge10DataGridView.Rows[rownum].Cells["Value"].Value.ToString() == "FAILED" ||
                          _merge10DataGridView.Rows[rownum].Cells["Value"].Value.ToString() == "ABORTED"))
                    {
                        _merge10DataGridView.Rows[rownum].Cells["Value"].Style.BackColor = Color.Red;
                    }
                    else
                        if (_merge10DataGridView.Rows[rownum].Cells["Attribute"].Value.ToString() == "Status" &&
                            _merge10DataGridView.Rows[rownum].Cells["Value"].Value.ToString() == "COMPLETED" )
                        {
                            _merge10DataGridView.Rows[rownum].Cells["Value"].Style.BackColor = Color.LawnGreen;
                        }

                // Hide End Time when Start Time = -1
                // Assuming that Start/End times are contiguous rows...
                if (_merge10DataGridView.Rows[rownum].Cells["Attribute"].Value.ToString().Contains("Start Time") &&
                    _merge10DataGridView.Rows[rownum].Cells["Value"].Value.ToString() == "-1")
                {
                    _merge10DataGridView.Rows[rownum].Visible = false;
                    if(rownum+1 < _merge10DataGridView.Rows.Count)
                    _merge10DataGridView.Rows[rownum+1].Visible = false;
                }

            }

        }
        // ###########################################################################################################

        private void _timer_Tick(object sender, EventArgs e)
        {
            if (_timeRemaining == 0)
            {
                _timer.Enabled = false;
                _timer.Stop();
                // restore _timeRemaining
                _timeRemaining = _refreshInterval;
                _timeRemainingTextBox.Text = Convert.ToString(_timeRemaining);

                // Before firing the query, might want to check the connection...

                if (_connectionDefinition.TheState != ConnectionDefinition.State.TestSucceeded)     // conn bye-bye
                {
                    MessageBox.Show("Connection not available.  Auto refresh will be disabled.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return;
                }

                // When Refresh  while Phase2 or earlier, both query results will be handled OK.
                // Past Phase2, there won't be any partition details so handle differently
                _refreshVSN++;
                DoSqlMerge10();
            }
            else
            {
                _timeRemaining--;
                _timeRemainingTextBox.Text = Convert.ToString(_timeRemaining);
            }

        }
        // ##############################################################################################
        // Because textbox value changes with each keystroke, don't use the textbox events.  Rather click this
        // SET button to transfer what's in the textbox to the internal variable
        private void _setRefreshIntervalButton_Click(object sender, EventArgs e)
        {
            string buffer = _refreshTextBox.Text.Replace(" ", "");
            if (buffer != String.Empty)
            {
                _refreshInterval = Convert.ToInt16(_refreshTextBox.Text);
            }
            else
            {
                MessageBox.Show("Refresh value cannot be empty", "Data Error!");
            }


        }

        private void _theHelpButton_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.BDRDetails);
        }
       

    }
}
