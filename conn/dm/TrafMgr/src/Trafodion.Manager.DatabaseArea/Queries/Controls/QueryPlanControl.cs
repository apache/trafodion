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

using System;
using System.Collections;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.NCC;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Queries.Controls
{
    /// <summary>
    /// Explain plan control
    /// </summary>
    public partial class QueryPlanControl : UserControl
    {
        #region Fields

        /// <summary>
        /// Private members
        /// </summary>
        private NCCQueryPlan _nccQueryPlan = null;
        private NCCWorkbenchQueryData _wbqd = null;
        private string _theSqlText = null;
        private NCCWorkbenchQueryData.ExplainResult _explainResult = NCCWorkbenchQueryData.ExplainResult.Successful;
        private string _fetchExplainPlanError = null;
        private string _fetchTableStatsError = null;
        private TrafodionToolStripMenuItem _showHelpMenuItem; 

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property SQLText: the Sql statement
        /// </summary>
        public string SQLText
        {
            get { return _theSqlText; }
            set { _theSqlText = value; }
        }

        /// <summary>
        /// Property Result: indicates whether there is an error in the evaluation
        /// </summary>
        public NCCWorkbenchQueryData.ExplainResult Result
        {
            get { return _explainResult; }
            set { _explainResult = value; }
        }

        /// <summary>
        /// Property FetchExplainPlanError: the error text while fetching explain plan
        /// </summary>
        public String FetchExplainPlanError
        {
            get { return _fetchExplainPlanError; }
            set { _fetchExplainPlanError = value; }
        }

        /// <summary>
        /// Property FetchTableStatsError: the error text while fetching table stats
        /// </summary>
        public string FetchTableStatsError
        {
            get { return _fetchTableStatsError; }
            set { _fetchTableStatsError = value; }
        }

        /// <summary>
        /// Property ShowExplainPlanPanel: indicates whether to show or hide explain plan
        /// </summary>
        public bool ShowExplainPlanPanel
        {
            get { return !_queryPlanSplitContainer.Panel1Collapsed; }
            set { _queryPlanSplitContainer.Panel1Collapsed = !value; }
        }

        /// <summary>
        /// Property ShowTableStatsPanel: indicates whether to show or hide table stats
        /// </summary>
        public bool ShowTableStatsPanel
        {
            get { return !_queryPlanSplitContainer.Panel2Collapsed; }
            set { _queryPlanSplitContainer.Panel2Collapsed = !value; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Default constructor 
        /// </summary>
        public QueryPlanControl()
        {
            InitializeComponent();
            this.TrafodionPanel1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.TrafodionSplitContainer2.Panel2Collapsed = true;
            _showHelpMenuItem = new TrafodionToolStripMenuItem();
            _showHelpMenuItem.Text = "Help";
            _showHelpMenuItem.Click += new EventHandler(_showHelpMenuItem_Click);
            this.TrafodionGroupBox1.MouseUp += new MouseEventHandler(TrafodionGroupBox1_MouseUp);
            this._iGridTableDetails.HelpTopic = HelpTopics.TableStats;
        }

        void _showHelpMenuItem_Click(object sender, EventArgs e)
        {
            TrafodionHelpProvider.Instance.ShowHelpTopic(HelpTopics.TableStats);
        }

        void TrafodionGroupBox1_MouseUp(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                TrafodionContextMenuStrip contextMenuStrip = new TrafodionContextMenuStrip();
                contextMenuStrip.Items.Add(_showHelpMenuItem);
                contextMenuStrip.Show(TrafodionGroupBox1, new System.Drawing.Point(e.X, e.Y));
            }
        }

        /// <summary>
        /// MyDispose - clean up
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing && (_nccQueryPlan != null))
            {
                _nccQueryPlanControl.Dispose();
                if (_iGridTableDetails != null)
                {
                    _iGridTableDetails.Rows.Clear();
                    _iGridTableDetails.Dispose();
                }
            }
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// Load the retrived query data to the grids. 
        /// </summary>
        /// <param name="wbqd"></param>
        public void LoadQueryData(NCCWorkbenchQueryData wbqd)
        {
            _wbqd = wbqd;
            Result = wbqd.Result;
            FetchExplainPlanError = wbqd.FetchPlanError;
            FetchTableStatsError = wbqd.FetchTableStatsError;
            if (Result != NCCWorkbenchQueryData.ExplainResult.Get_Explain_Error)
            {
                LoadTreeGrid(wbqd);
                DisplaySQLTableItems(wbqd);
            }    

            if (!String.IsNullOrEmpty(FetchTableStatsError))
            {
                // There are error messages while retrieving table stats. 
                TrafodionSplitContainer2.Panel2Collapsed = false;
                DisplayTableStatsMessage(FetchTableStatsError);
            }
            else
            {
                if (_iGridTableDetails.Rows.Count > 0)
                {
                    _queryPlanSplitContainer.Panel2Collapsed = false;
                    TrafodionSplitContainer2.Panel2Collapsed = true;
                }
                else
                {
                    if (wbqd.SkipTableStats)
                    {
                        TrafodionSplitContainer2.Panel2Collapsed = true;
                        _queryPlanSplitContainer.Panel2Collapsed = true;
                        DisplayTableStatsMessage("stats not selected");
                    }
                    else
                    {
                        _queryPlanSplitContainer.Panel2Collapsed = false;
                        TrafodionSplitContainer2.Panel2Collapsed = false;
                        DisplayTableStatsMessage("No stats available");
                    }
                }
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Load query data to build the explain plan tree grid.
        /// </summary>
        /// <param name="wbqd"></param>
        private void LoadTreeGrid(NCCWorkbenchQueryData wbqd)
        {
            _nccQueryPlanControl.LoadTreeGrid(wbqd);
        }

        /// <summary>
        /// Diaplay SQL table stats.
        /// </summary>
        /// <param name="wbqd"></param>
        private void DisplaySQLTableItems(NCCWorkbenchQueryData wbqd)
        {
            ArrayList qpa = wbqd.QueryPlanArray;
            this._iGridTableDetails.Rows.Clear();
            this._iGridTableDetails.GroupObject.Clear();
            this._iGridTableDetails.GroupObject.Add("TableName");

            ArrayList TablesDescribed = new ArrayList();
            foreach (NCCWorkbenchQueryData.QueryPlanData qpd in qpa)
            {
                String theTableName = GetTableNameFromQueryPlanData(qpd);
                if (0 < theTableName.Length)
                {
                    if (TablesDescribed.Contains(theTableName))
                        continue;

                    GetTableDetails(theTableName, wbqd);
                    TablesDescribed.Add(theTableName);
                }
            }
            this._iGridTableDetails.Group();
        }
        private String GetTableNameFromQueryPlanData(NCCWorkbenchQueryData.QueryPlanData qpd)
        {
            String tableName = "";
            try
            {
                tableName = qpd.tableName.Trim();
                if (tableName.EndsWith(")"))
                {
                    int startIdx = tableName.IndexOf("(");
                    int endIdx = tableName.Length - 2;
                    tableName = tableName.Substring(startIdx + 1, (endIdx - startIdx));
                }
            }
            catch (Exception)
            { }
            return tableName;
        }

        /// <summary>
        /// Get values as 64-bit number from row info hash table.
        /// </summary>
        /// <param name="rowInfo"></param>
        /// <param name="colName"></param>
        /// <returns></returns>
        private Object GetValueAs64BitNumber(Hashtable rowInfo, String colName)
        {
            Object theValue = rowInfo[colName];
            try
            {
                UInt64 intVal = UInt64.Parse(theValue.ToString());
                theValue = intVal;
            }
            catch (Exception)
            { }

            return theValue;
        }

        /// <summary>
        /// Add table detailed stats into the table stats grid.  
        /// Note: the table stats should have been retrieved from the backend and 
        ///       stored in the _tableDetails_ht hash table. 
        /// </summary>
        /// <param name="tableName"></param>
        private void GetTableDetails(string tableName, NCCWorkbenchQueryData wbqd)
        {
            if (false == wbqd.TableDetailsHashtable.Contains(tableName))
                return;

            ArrayList tableInfo = (ArrayList)wbqd.TableDetailsHashtable[tableName];

            foreach (Hashtable rowInfo in tableInfo)
            {
                iGRow newRow = _iGridTableDetails.Rows.Add();
                newRow.Cells["TableName"].Value = "  " + tableName;
                newRow.Cells["UEC"].Value = GetValueAs64BitNumber(rowInfo, "UEC");
                newRow.Cells["Colnames"].Value = rowInfo["Colnames"];
                newRow.Cells["HistID"].Value = GetValueAs64BitNumber(rowInfo, "HistID");
                newRow.Cells["Ints"].Value = GetValueAs64BitNumber(rowInfo, "Ints");
                newRow.Cells["Rowcount"].Value =GetValueAs64BitNumber(rowInfo, "Rowcount");
            }
        }

        /// <summary>
        /// Display error messages from retrieving table stats.
        /// </summary>
        /// <param name="error"></param>
        private void DisplayTableStatsMessage(string error)
        {
            TrafodionRichTextBox theTextBox = new TrafodionRichTextBox();
            theTextBox.Text = error;
            theTextBox.WordWrap = true;
            theTextBox.Dock = DockStyle.Fill;
            _tableDetailsPanel.Controls.Add(theTextBox);
        }

        #endregion Private methods
    }
}
