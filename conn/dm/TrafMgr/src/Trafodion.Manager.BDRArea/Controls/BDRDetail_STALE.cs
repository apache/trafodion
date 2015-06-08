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
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using BDRPlugin11.Model;


namespace BDRPlugin11.Controls
{
    public partial class BDRDetail_STALE : Form
    {
        #region Fields
        private static readonly string MyWidgets = "MyWidgets3";
        static readonly char[] delimiters = new char[] { ' ', ':' };
        string _myQID;
        string _queryText;
        ConnectionDefinition _connectionDefinition;
        bool _constructorError = false;

        // RMS query results are name-value pairs embedded in a text item.
        // The following arrays facilitate the use/don't use logic.  Presence in the array means that name-value
        // should be exported to the DataTable.  C# Split() method makes it easy to populate the candidate names and
        // values - there must be pairs.

        // Some of the "time" fields are Julian timestamps, others are elapsed usec
        // The Julians are converted to datetime strings for display
        static readonly string[] _merge10Keepers = new string[] {
                                       "CompStartTime", 
                                       "CompEndTime", 
                                       "ExeStartTime",
                                       "compElapsedTime", 
                                       "exeElapsedTime", 
                                       "numSqlProcs", 
                                       "numCpus",
                                       "exePriority", 
                                       "suspended", 
                                       "lastSuspendTime", 
                                       "sqlSrc",
                                       "AnsiName",
                                       "SourceSystem",
                                       "TargetSystem",
                                       "ObjectType",
                                       "ReplType",
                                       "NumParns",
                                       "phase1StartTime",
                                       "phase1EndTime",
                                       "phase2StartTime",
                                       "phase2EndTime",
                                       "phase3StartTime",
                                       "phase3EndTime",
                                       "phase4StartTime",
                                       "phase4EndTime",
                                       "phase5StartTime",
                                       "phase5EndTime",
                                       "PercentDone"
        };

        // keep together the next two arrays - they're related
        static readonly string[] _tdbidDetail1Keepers = new string[] {
                                       "OperCpuTime",
                                       "SourcePart",
                                       "TargetPart",
                                       "BlockLen",
                                       "TotalCompTime",
                                       "TotalCompBytes",
                                       "TotalUncompTime",
                                       "TotalUncompBytes",
                                       "TotalRowCount",
                                       "TotalBlocks",
                                       "BlocksReplicated"
        };

        // number of elements must match above "keepers"; this array tells which column in the datatable the value goes to
        // posn[0] is for ProcessName, which is not in this list; 
        static readonly int[] _tdbidDetail1ColPosition = new int[] { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 };

        DataTable _merge10DataTable = new DataTable();
        DataTable _tdbidDetail1DataTable = new DataTable();


        #endregion Fields
        // ###########################################################################################################

        #region Properties
        /*
        public ConnectionDefinition TheConnectionDefinition
        {
            //get { return _connectionDefinition; }
            set
            {
                _connectionDefinition = value;
            }
        }
        */
        #endregion Properties
        // ###########################################################################################################
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aQID"></param>
        /// <param name="aConnectionDef"></param>
        public BDRDetail_STALE(string aQID, ConnectionDefinition aConnectionDef)
        {
            InitializeComponent();
            // set the window title
            _myQID = aQID;
            this.Text = "BDR Details for QID: " + _myQID;
            _connectionDefinition = aConnectionDef;
            // add to datagridview
            _merge10DataGridView.AddCountControlToParent("There are {0} rows", DockStyle.Top);
            _merge10DataGridView.AddButtonControlToParent(DockStyle.Bottom);

            _partitionsDataGridView.AddCountControlToParent("There are {0} rows", DockStyle.Top);
            _partitionsDataGridView.AddButtonControlToParent(DockStyle.Bottom);

            _widgetCanvas3.ThePersistenceKey = MyWidgets;
            _widgetCanvas3.Dock = DockStyle.Fill;
            GridLayoutManager gridLayoutManager = new GridLayoutManager(1, 2);   // 1 row n col
            _widgetCanvas3.LayoutManager = gridLayoutManager;
            //Add the first control to the canvas
            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer widgetContainer = new WidgetContainer(_widgetCanvas3, _merge10GroupBox, "Progress");
            widgetContainer.Name = "Progess";
            widgetContainer.AllowDelete = false;
            _widgetCanvas3.AddWidget(widgetContainer, gridConstraint, -1);

            //Add the next control to the canvas
            gridConstraint = new GridConstraint(0, 1, 1, 1);
            widgetContainer = new WidgetContainer(_widgetCanvas3, _partitionsGroupBox, "Active Partitions");
            widgetContainer.Name = "Partitions";
            widgetContainer.AllowDelete = false;
            _widgetCanvas3.AddWidget(widgetContainer, gridConstraint, -1);

            // set the columns for the DataTables
            _merge10DataTable.Columns.Add("Attribute", typeof(string));
            _merge10DataTable.Columns.Add("Value", typeof(string));

            _tdbidDetail1DataTable.Columns.Add("ProcessName", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("OperCpuTime", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("SourcePart", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TargetPart", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("BlockLen", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TotalCompTime", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TotalCompBytes", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TotalUncompTime", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TotalUncompBytes", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TotalRowCount", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("TotalBlocks", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("BlocksReplicated", typeof(string));
            // computed columns
            _tdbidDetail1DataTable.Columns.Add("% Done", typeof(string));
            _tdbidDetail1DataTable.Columns.Add("Blks/sec", typeof(string));

            // if Merge10 query fails - no need to continue with window
            if (SqlMerge10()) _constructorError = true ;
            SqlTdbidDetail1();

        }
        // ###########################################################################################################

        /// <summary>
        /// Execute MERGE=10 query and parse out the interesting name-value pairs
        /// DataTable the result; map to datagridview
        /// </summary>
        private bool SqlMerge10()
        {
            string[] words;
            string datetimestring;
            
            Cursor.Current = Cursors.WaitCursor;

            try
            {
                SqlQuery sqlQuery = new SqlQuery(_connectionDefinition);
                _queryText = "SELECT variable_info from table(statistics(NULL, 'QID=" +
                             _myQID +
                             ",MERGE=10'))";
                             
                DataTable tempdata = sqlQuery.GetQueryResults(_queryText);

                foreach (DataRow r in tempdata.Rows)
                {
                    words = r.Field<string>(0).Split(delimiters, System.StringSplitOptions.RemoveEmptyEntries);
                    if (words[0].ToUpper() == "STATSROWTYPE" && words[1] == "1") continue;
                    //if (words[0].ToUpper == "STATSROWTYPE" && words[1] == "20") continue;   //?????

                    foreach (string attribute in _merge10Keepers)
                    {
                        for (int i=0;i<words.Length;i++)
                        {
                            if (attribute == words[i])
                            {   // add the row to the result table, but first...
                                // if the attribute has "time" in it, the value is a juliantimestamp
                                //  convert this to a date/time string
                                if (attribute.ToUpper().Contains("STARTTIME") ||
                                    attribute.ToUpper().Contains("ENDTIME")   ||
                                    attribute.ToUpper().Contains("SUSPENDTIME") )
                                {
                                    long k = Convert.ToInt64(words[i + 1]);
                                    // time value could be missing
                                    if (k == -1) _merge10DataTable.Rows.Add(words[i], words[i + 1]);
                                    else
                                        {
                                            datetimestring = Trafodion.Manager.Framework.Utilities.FormattedJulianTimestamp(k);
                                            _merge10DataTable.Rows.Add(words[i], datetimestring);
                                        }
                                }
                                else
                                    _merge10DataTable.Rows.Add(words[i], words[i + 1]);
                            }
                        }
                    }
                }

                _merge10DataGridView.DataSource = _merge10DataTable;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                //MessageBox.Show(ex.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                Cursor.Current = Cursors.Default;
                return true;
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }
            return false;
        }
        // ###########################################################################################################

        /// <summary>
        /// Execute TDBID_DETAIL=1 query for status of the running partitions
        /// Compute a couple of new columns (%done, byte rate); 
        /// DataTable result; datagridview; colorize
        /// </summary>
        private void SqlTdbidDetail1()
        {
            string[] words;
            object[] obj = new object[_tdbidDetail1Keepers.Length];
            Cursor.Current = Cursors.WaitCursor;

            try
            {
                SqlQuery sqlQuery = new SqlQuery(_connectionDefinition);
                _queryText = "SELECT text,variable_info from table(statistics(NULL, 'QID=" +
                             _myQID +
                             ",TDBID_DETAIL=1'))";

                DataTable tempdata = sqlQuery.GetQueryResults(_queryText);

                foreach (DataRow r in tempdata.Rows)
                {
                    words = r.Field<string>(1).Split(delimiters, System.StringSplitOptions.RemoveEmptyEntries);
                    if (words[0].ToUpper() == "STATSROWTYPE" && words[1] == "1") continue;
                    //if (words[0].ToUpper == "STATSROWTYPE" && words[1] == "20") continue;   //?????

                    foreach (string attribute in _tdbidDetail1Keepers)
                        // many fields from the result column turn into columns in the datatable
                        // how to construct a DataRow??
                    {
                        for (int i = 0; i < words.Length; i++)
                        {
                            if (attribute == words[i])
                            {  // add words[i+1] to the list
                                obj[_tdbidDetail1ColPosition[i]] = words[i + 1]; 
                            }
                        }
                    }
                    // compute the %done, rate, etc..
                    /* TBD
                     */

                    _tdbidDetail1DataTable.Rows.Add(obj);
                }

                _partitionsDataGridView.DataSource = _tdbidDetail1DataTable;
            }
            catch (Exception ex)
            {
                //MessageBox.Show(ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                MessageBox.Show(ex.ToString(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
            finally
            {
                Cursor.Current = Cursors.Default;
            }

        }

        private void MyBDRDetail_Shown(object sender, EventArgs e)
        {
            if (_constructorError)
            {
                MessageBox.Show("Because the requested Query_ID was not found, there is no data to display.  This window will close.",
                                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                this.Dispose();
                this.Close();
            }
        }
        // ###########################################################################################################

    }
}
