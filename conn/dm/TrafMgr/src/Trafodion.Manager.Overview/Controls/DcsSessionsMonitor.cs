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
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.OverviewArea.Models;
using Trafodion.Manager.OverviewArea.Controls;

namespace Trafodion.Manager.OverviewArea.Controls
{
    /// <summary>
    /// HDcsSessionsMonitor 
    /// </summary>
    public partial class DcsSessionsMonitor : UserControl, ICloneToWindow
    {
        #region Fields

        private static readonly string DcsSessionsMonitorConfigName = "DcsSessionsMonitor";
        public const int DefaultRefreshRate = 3000; // 3 seconds
        private const string TRACE_SUB_AREA_NAME = "DcsSessions";

        private ConnectionDefinition _theConnectionDefinition = null;
        //private DisplayConnectorsUserControl _theDisplayConnectorsUserControl = null;
        private string _theTitle = null;
        private delegate void HandleEvents(Object obj, DataProviderEventArgs e);
        private BackgroundWorker _backgroundWorker = null;
        private bool _started = false;
        private int _refreshRate = DefaultRefreshRate;
        TrafodionIGridToolStripMenuItem _cancelJobMenuItem = new TrafodionIGridToolStripMenuItem();

        private UniversalWidgetConfig _config1 = null;
        private GenericUniversalWidget _widget1 = null;
        private JsonDataProvider _dataProvider = null;
        private TrafodionIGrid _jobsDataGrid = null;

        public JsonDataProvider TheDataProvider
        {
            get { return _dataProvider; }
            set { _dataProvider = value; }
        }
        TrafodionIGrid _jobsGrid = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: The connection definition
        /// </summary>
        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }

        /// <summary>
        /// Property: The window title
        /// </summary>
        public string WindowTitle
        {
            get { return _theTitle; }
        }

        #endregion Properties

        #region Constructors

        /// <summary>
        /// Constructor for a brand new creation
        /// </summary>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aTitle"></param>
        public DcsSessionsMonitor(ConnectionDefinition aConnectionDefinition)
        {
            _theConnectionDefinition = aConnectionDefinition;
            InitializeComponent();
            ShowWidgets();
        }

        /// <summary>
        /// Constructor for cloning
        /// </summary>
        /// <param name="aDcsSessionsMonitor"></param>
        public DcsSessionsMonitor(DcsSessionsMonitor aDcsSessionsMonitor)
        {
            _theConnectionDefinition = aDcsSessionsMonitor.ConnectionDefn;
            InitializeComponent();
            ShowWidgets(aDcsSessionsMonitor);
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// The interface method for ICloneWindow.
        /// </summary>
        /// <returns></returns>
        public Control Clone()
        {
            DcsSessionsMonitor theDcsSessionsMonitor = new DcsSessionsMonitor(this);
            return theDcsSessionsMonitor;
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// To dispose everything here
        /// </summary>
        /// <param name="disposing"></param>
        private void MyDispose(bool disposing)
        {
            if (disposing)
            {
                if (_widget1 != null && _widget1.DataProvider != null)
                {
                    _widget1.DataProvider.Stop();
                }
            }
        }
        /// <summary>
        /// Show all widgets
        /// </summary>
        private void ShowWidgets()
        {
            ShowWidgets(null);
        }

        /// <summary>
        /// Create all of the widgets
        /// </summary>
        private void ShowWidgets(DcsSessionsMonitor clone)
        {
            UniversalWidgetConfig tempConfig = WidgetRegistry.GetConfigFromPersistence(DcsSessionsMonitorConfigName);
            DcsConnectionOptions dcsConnectionOptions = DcsConnectionOptions.GetOptions(this.ConnectionDefn);

           // if (tempConfig == null)
            {
                _config1 = new UniversalWidgetConfig();
                WebDataProviderConfig webConfig = new JsonDataProviderConfig();
                webConfig.ConnectionDefinition = this.ConnectionDefn;

                webConfig.TheRootURL = dcsConnectionOptions.WebServerURL;
                webConfig.TheURL = string.Format("http://{0}/v1/servers/dcs/connections", dcsConnectionOptions.WebServerURL);
                webConfig.RefreshRate = 180;
                webConfig.MethodType = "GET";
                webConfig.ContentType = "application/json";
                //webConfig.PostParameters = hadoopOptions.PostParameters;
                _config1.DataProviderConfig = webConfig;
                _config1.ShowToolBar = true;
                _config1.ShowProviderToolBarButton = false;
                _config1.ShowProviderStatus = false;
                _config1.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.Show;
                _config1.DataProviderConfig = webConfig;
            }
            /*else
            {
                _config1 = tempConfig;
            }*/
            _config1.ShowRefreshButton = true;
            _config1.DataProviderConfig.TimerPaused = false;

            _dataProvider = new JsonDataProvider(_config1.DataProviderConfig);

            _widget1 = new GenericUniversalWidget();
            ((TabularDataDisplayControl)_widget1.DataDisplayControl).LineCountFormat = "";
            _widget1.DataProvider = _dataProvider;
            _widget1.UniversalWidgetConfiguration = _config1;
            _jobsGrid = ((TabularDataDisplayControl)_widget1.DataDisplayControl).DataGrid;
            //_jobsGrid.DoubleClickHandler = this._jobsGrid_DoubleClick;
            _widget1.DataDisplayControl.DataProvider = _dataProvider;
            _widget1.DataDisplayControl.DataDisplayHandler = new DcsSessionsDataHandler(this);

            _jobsDataGrid = ((TabularDataDisplayControl)_widget1.DataDisplayControl).DataGrid;
            _jobsDataGrid.CellMouseDown += DataGrid_CellMouseDown;
            ToolStripButton monitorOptionsButton = new ToolStripButton();
            monitorOptionsButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
            monitorOptionsButton.Image = Properties.Resources.ConfigureIcon;
            monitorOptionsButton.ImageTransparentColor = System.Drawing.Color.Magenta;
            monitorOptionsButton.Name = "monitorOptionsButton";
            monitorOptionsButton.Size = new System.Drawing.Size(23, 22);
            monitorOptionsButton.Text = "Options";
            monitorOptionsButton.Click += new EventHandler(monitorOptionsButton_Click);
            _widget1.AddToolStripItem(monitorOptionsButton);

/*            _cancelJobMenuItem = new TrafodionIGridToolStripMenuItem();
            _cancelJobMenuItem.Text = "Cancel Job";
            _cancelJobMenuItem.Click += new EventHandler(cancelJobMenuItem_Click);
 * 
            ((TabularDataDisplayControl)_widget1.DataDisplayControl).AddMenuItem(_cancelJobMenuItem);
 * */
  
            ((TabularDataDisplayControl)_widget1.DataDisplayControl).LineCountFormat = "DCS Sessions";
            _widget1.Dock = DockStyle.Fill;
            _widget1.BackColor = Color.Blue;
            _theCanvas.Controls.Add(_widget1);
            _widget1.StartDataProvider();

        }

        void DataGrid_CellMouseDown(object sender, iGCellMouseDownEventArgs e)
        {
            if (e.RowIndex >= 0 && e.Button == MouseButtons.Right)
            {
                iGRow row = _jobsDataGrid.Rows[e.RowIndex];
                iGRowCellCollection coll = row.Cells;

                if (e.ColIndex >= 0)
                {
                    row.Cells[e.ColIndex].Selected = true;
                }
                string job_state = (string)coll["State"].Value;

                _cancelJobMenuItem.Enabled = !(job_state.Equals("Running", StringComparison.InvariantCultureIgnoreCase) ||
                                                job_state.Equals("Preparing", StringComparison.InvariantCultureIgnoreCase));
            }
        }

        void monitorOptionsButton_Click(object sender, EventArgs e)
        {
            _dataProvider.Stop();

            DcsConnectionOptionsDialog hod = new DcsConnectionOptionsDialog(this.ConnectionDefn);
            if (hod.ShowDialog() == DialogResult.OK)
            {
                DcsConnectionOptions dcsConnectionOptions = DcsConnectionOptions.GetOptions(this.ConnectionDefn);
                if (dcsConnectionOptions != null)
                {
                    ((WebDataProviderConfig)_config1.DataProviderConfig).TheRootURL = dcsConnectionOptions.WebServerURL;
                    ((WebDataProviderConfig)_config1.DataProviderConfig).TheURL = string.Format("http://{0}/v1/servers/dcs/connections", dcsConnectionOptions.WebServerURL);
                    _dataProvider.Start();
                }
            }
        }

        void cancelJobMenuItem_Click(object sender, EventArgs e)
        {
            JsonDataProviderConfig dbConfig = TheDataProvider.DataProviderConfig as JsonDataProviderConfig;
            bool queryCancelled = false;
            DataTable errorTable = new DataTable();
            errorTable.Columns.Add("Name");
            errorTable.Columns.Add("Exception");

/*            if (MessageBox.Show(Utilities.GetForegroundControl(), "Are you sure you want to cancel the selected jobs?",
                    "Confirm Cancel", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                Cursor = Cursors.WaitCursor;
                foreach (int rowIndex in _jobsDataGrid.SelectedRowIndexes)
                {
                    string jobId = _jobsDataGrid.Cells[rowIndex, "ID"].Value as string;
                    string jobState = _jobsDataGrid.Cells[rowIndex, "State"].Value as string;
                    //skip the completed queries
                    if (jobState.Equals("Running", StringComparison.InvariantCultureIgnoreCase) ||
                                                jobState.Equals("Preparing", StringComparison.InvariantCultureIgnoreCase))
                    {
                        continue;
                    }

                    try
                    {
                        DcsConnectionOptions hadoopOptions = DcsConnectionOptions.GetOptions(this.ConnectionDefn);
                        WebDataProviderConfig sourceConfig = (WebDataProviderConfig)this._dataProvider.DataProviderConfig;
                        WebDataProviderConfig dbProviderConfig = new WebDataProviderConfig();
                        dbProviderConfig.ConnectionDefinition = sourceConfig.ConnectionDefinition;
                        dbProviderConfig.TheURL = string.Format("http://{0}/openedbm/restful-services/hadoopservice/jobs/{1}/cancel", hadoopOptions.WebServerURL, jobId);
                        dbProviderConfig.MethodType = "GET";
                        string response = WebDataProvider.DoWebRequest(dbProviderConfig);
                    }
                    catch (Exception ex)
                    {
                    }
                }
                Cursor = Cursors.Default;
            }*/
        }

        void _jobsGrid_DoubleClick(int row)
        {
            invokeJobDetail(row);
        }

        void invokeJobDetail(int row)
        {

        }
        /// <summary>
        /// Clean up all displays
        /// </summary>
        private void CleanupAllDisplays()
        {
            // nothing at this time
        }

        /// <summary>
        /// To refresh the monitoring displays
        /// </summary>
        private void DoRefresh()
        {
            //DataTable table = _theDisplayConnectorsUserControl.DataTable;
            //table.Rows.Clear();
        }

        #endregion Private Methods
    }
}

#region Class DcsSessionsDataHandler

/// <summary>
/// The class for handling received publication data
/// </summary>
public class DcsSessionsDataHandler : TabularDataDisplayHandler
{
    #region Fields

    private Trafodion.Manager.OverviewArea.Controls.DcsSessionsMonitor _theCanvas;
    private int limit = 1000;

    #endregion Fields

    #region Constructors

    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="aPubsCanvas"></param>
    public DcsSessionsDataHandler(Trafodion.Manager.OverviewArea.Controls.DcsSessionsMonitor aCanvas)
    {
        _theCanvas = aCanvas;
    }

    #endregion Constructor

    #region Public methods

    /// <summary>
    /// For applying filtering
    /// </summary>
    /// <param name="aConfig"></param>
    /// <param name="aDataTable"></param>
    /// <param name="aDataGrid"></param>
    public void PopulateFromCache(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                    Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
    {
        lock (this)
        {
            aDataGrid.Rows.Clear();
            populate(aConfig, aDataTable, aDataGrid);
        }
    }

    /// <summary>
    /// Do populate
    /// </summary>
    /// <param name="aConfig"></param>
    /// <param name="aDataTable"></param>
    /// <param name="aDataGrid"></param>
    public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                    Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
    {
        lock (this)
        {
            populate(aConfig, aDataTable, aDataGrid);
        }
    }

    #endregion Public methods

    #region Private methods

    /// <summary>
    /// To adjust grid
    /// </summary>
    /// <param name="aDataTable"></param>
    /// <param name="aDataGrid"></param>
    private void DeleteRowsIfNeeded(System.Data.DataTable aDataTable, Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
    {
        int currentRowCount = aDataGrid.Rows.Count;
        int rowsToBeAdded = (aDataTable != null) ? aDataTable.Rows.Count : 0;
        if ((currentRowCount + rowsToBeAdded) > limit)
        {
            int deleteCount = (currentRowCount + rowsToBeAdded) - limit;
            aDataGrid.Rows.RemoveRange((currentRowCount - deleteCount), deleteCount);
        }
    }

    /// <summary>
    /// the real populate routine
    /// </summary>
    /// <param name="aConfig"></param>
    /// <param name="aDataTable"></param>
    /// <param name="aDataGrid"></param>
    private void populate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                    Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
    {
        if (null == aDataTable)
        {
            return;
        }

        long recorded_ts = 0;
        DataTable dataTable = new DataTable();

        foreach (DataColumn dc in aDataTable.Columns)
        {
            if (dc.ColumnName.Contains("time_ts_utc") ||
                dc.ColumnName.Contains("time_ts_lct") ||
                dc.ColumnName.Equals("RECORDED_TS", StringComparison.OrdinalIgnoreCase))
            {
                dataTable.Columns.Add(dc.ColumnName, typeof(DateTime));
            }
            else
            {
                dataTable.Columns.Add(dc.ColumnName, dc.DataType);
            }
        }

        foreach (DataRow dr in aDataTable.Rows)
        {
            DataRow row = dataTable.NewRow();
            foreach (DataColumn dc in aDataTable.Columns)
            {
                if (dc.ColumnName.Contains("time_ts_utc"))
                {
                    row[dc.ColumnName] = GetFormattedTimeFromUnixTimestampUTC((long)dr[dc.ColumnName] / 1000);
                }
                else if (dc.ColumnName.Contains("time_ts_lct"))
                {
                    row[dc.ColumnName] = GetFormattedTimeFromUnixTimestampLCT((long)dr[dc.ColumnName] / 1000);
                }
                else if (dc.ColumnName.Equals("RECORDED_TS", StringComparison.OrdinalIgnoreCase))
                {
                    if (recorded_ts == 0)
                    {
                        recorded_ts = (long)dr[dc.ColumnName] * 1000;
                    }
                    row[dc.ColumnName] = GetFormattedTimeFromUnixTimestampUTC((long)dr[dc.ColumnName] * 1000);

                }
                else
                {
                    row[dc.ColumnName] = dr[dc.ColumnName];
                }
            }
            dataTable.Rows.Add(row);
        }

        if (aDataGrid.Rows.Count == 0)
        {
            base.DoPopulate(aConfig, dataTable, aDataGrid);
        }
        else
        {
            //if (dataTable.Rows.Count > 0)
            //{
            //    DeleteRowsIfNeeded(dataTable, aDataGrid);
            //    aDataGrid.InsertRows(0, dataTable);
            //}
            base.DoPopulate(aConfig, dataTable, aDataGrid);
        }

        string gridHeaderText;
        if (aDataGrid.Rows.Count == 0)
        {
            gridHeaderText = "There are no dcs servers";
        }
        else if (aDataGrid.Rows.Count == 1)
        {
            gridHeaderText = "There is 1 dcs server";
        }
        else
        {
            gridHeaderText = String.Format("There are {0} DCS servers", aDataGrid.Rows.Count);
        }
        aDataGrid.ResizeGridColumns(dataTable);
        aDataGrid.UpdateCountControlText(gridHeaderText);
    }

    #endregion Private methods

    /// <summary>
    /// Formats the datetime to a human readable form with a input from Unix ls
    /// </summary>
    /// <param name="aTime"></param>
    /// <returns>DateTime Short format used by Trafodion Database Manager</returns>
    static public DateTime GetFormattedTimeFromUnixTimestampUTC(long aTime)
    {
        long secondsSinceEpoch = aTime / 1000;
        DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + 621355968000000000, DateTimeKind.Utc);
        return theDateTime;
    }

    /// <summary>
    /// To format datatime for LCT timestamps
    /// </summary>
    /// <param name="aTime"></param>
    /// <returns></returns>
    static public DateTime GetFormattedTimeFromUnixTimestampLCT(long aTime)
    {
        long secondsSinceEpoch = aTime / 1000;
        DateTime theDateTime = new DateTime((secondsSinceEpoch * 10000000) + 621355968000000000, DateTimeKind.Local);
        return theDateTime;
    }
}

#endregion Class PublicationDataHandler

