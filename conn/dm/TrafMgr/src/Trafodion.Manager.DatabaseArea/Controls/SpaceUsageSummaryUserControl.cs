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
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class SpaceUsageSummaryUserControl : UserControl
    {
        #region private member variables
        private DatabaseObjectsControl _databaseObjectsControl = null;
        private DatabaseDataProvider _theDatabaseDataProvider = null;
        protected GenericUniversalWidget _theWidget = null;
        private string _theSqlText = null;
        private string _headerText = "";
        MSChartControl _theChartControl = null;
        DataTable _chartDataTable = null;
        protected TrafodionObject _theTrafodionObject;
        string _objectType = "";
        TrafodionIGridHyperlinkCellManager _hyperLinkCellManager = new TrafodionIGridHyperlinkCellManager();
        int _hyperLinkColumnNumber = 0;
        TrafodionIGrid _dataGrid = null;
        protected IDataDisplayHandler _dataDisplayHandler = null;
        protected FetchMode _fetchMode = FetchMode.Repository;

        public enum FetchMode { Repository = 0, Live = 1 };

        #endregion private member variables

        #region properties

        public DatabaseObjectsControl TheDatabaseObjectsControl
        {
            get { return _databaseObjectsControl; }
        }
        public TrafodionIGrid TheDataGrid
        {
            get { return _dataGrid; }
        }
        public TrafodionObject TheTrafodionObject
        {
            get { return _theTrafodionObject; }
        }
        public TrafodionIGridHyperlinkCellManager HyperLinkCellManager
        {
            get { return _hyperLinkCellManager; }
        }

        public int HyperLinkColumnNumber
        {
            get { return _hyperLinkColumnNumber; }
            set { _hyperLinkColumnNumber = value; }
        }

        /// <summary>
        /// The collection time offset from GMT time
        /// </summary>
        public TimeSpan CollectionTimeOffset
        {
            get { return (_fetchMode == SpaceUsageSummaryUserControl.FetchMode.Repository) ? TimeSpan.FromSeconds(0) : _theTrafodionObject.ConnectionDefinition.ServerGMTOffset; }
        }

        virtual protected string PersistenceKey
        {
            get { return "SpaceUsageSummaryPersistenceKey"; }
        }

        virtual public string SQLText
        {
            get { return _theSqlText; }
            set { _theSqlText = value; }
        }

        virtual public string HeaderText
        {
            get { return _headerText; }
            set { _headerText = value; }
        }

        virtual protected string ObjectType
        {
            get { return _objectType; }
            set { _objectType = value; }
        }

        virtual protected bool ShowTopNPanel
        {
            get { return true; }
        }

        public MSChartControl TheChartControl
        {
            get 
            {
                if (_theChartControl == null)
                {
                    _theChartControl = CreateChartControl();
                }
                return _theChartControl; 
            }
        }

        protected TrafodionNumericUpDown TopNumericUpDown
        {
            get { return _topNumericUpDown; }
        }

        public DataTable TheChartDataTable
        {
            get { return _chartDataTable; }
            set { _chartDataTable = value; }
        }

        virtual protected bool SupportsLiveFetch
        {
            get { return false; }
        }

        protected virtual IDataDisplayHandler TheDataDisplayHandler
        {
            get
            {
                if (_dataDisplayHandler == null)
                {
                    _dataDisplayHandler = new SpaceUsagesSummaryDataHandler(this);
                }
                return _dataDisplayHandler;
            }
        }

        protected DatabaseDataProvider DataProvider
        {
            get { return _theDatabaseDataProvider; }
            set 
            {
                _theDatabaseDataProvider = value;
                if (_theDatabaseDataProvider != null)
                {
                    _theWidget.DataProvider = _theDatabaseDataProvider;
                }
            }
        }

        virtual protected string HelpTopicName
        {
            get { return ""; }
        }

        #endregion properties

        public SpaceUsageSummaryUserControl()
        {
            InitializeComponent();
            this._layoutToolStrip.Visible = false;
        }
        public SpaceUsageSummaryUserControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionObject aTrafodionObject)
        {
            _databaseObjectsControl = aDatabaseObjectsControl;
            _theTrafodionObject = aTrafodionObject;
            InitializeComponent();
            this._layoutToolStrip.Visible = false;
            InitializeWidget();
        }

        #region private methods

        private void InitializeWidget()
        {

            UniversalWidgetConfig widgetConfig = WidgetRegistry.GetConfigFromPersistence(PersistenceKey);
            if (widgetConfig == null)
            {
                widgetConfig = WidgetRegistry.GetDefaultDBConfig();
                widgetConfig.Name = PersistenceKey;                
                widgetConfig.ShowProviderToolBarButton = false;
                widgetConfig.ShowTimerSetupButton = false;
                widgetConfig.ShowHelpButton = true;
                widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            }
            widgetConfig.HelpTopic = HelpTopicName;

            DatabaseDataProviderConfig dbConfig = widgetConfig.DataProviderConfig as DatabaseDataProviderConfig;
            if (dbConfig == null)
            {
                dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            }
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.ConnectionDefinition = _theTrafodionObject.ConnectionDefinition;
            _theDatabaseDataProvider = new DatabaseDataProvider(dbConfig);
            widgetConfig.DataProvider = _theDatabaseDataProvider;

            if (_theDatabaseDataProvider.DataProviderConfig.ColumnSortObjects == null)
            {
                _theDatabaseDataProvider.DataProviderConfig.ColumnSortObjects = new List<ColumnSortObject>();
            }

            if (SupportsLiveFetch)
            {
                widgetConfig.ShowRefreshButton = false;
            }

            _theWidget = new GenericUniversalWidget();
            _theWidget.DataProvider = _theDatabaseDataProvider;
            ((TabularDataDisplayControl)_theWidget.DataDisplayControl).LineCountFormat = Properties.Resources.SpaceUsage;
            _theWidget.DataDisplayControl.DataDisplayHandler = TheDataDisplayHandler;
            _theWidget.UniversalWidgetConfiguration = widgetConfig;
            _theWidget.Dock = DockStyle.Fill;

            if (SupportsLiveFetch)
            {
                ToolStripButton refetchRepositoryButton = new ToolStripButton();
                refetchRepositoryButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
                refetchRepositoryButton.Image = global::Trafodion.Manager.Properties.Resources.RefreshIcon;
                refetchRepositoryButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                refetchRepositoryButton.Name = "refetchRepositoryButton";
                refetchRepositoryButton.Size = new System.Drawing.Size(60, 22);
                refetchRepositoryButton.Text = "Repository Data";
                refetchRepositoryButton.Click += new EventHandler(refetchRepositoryButton_Click);
                _theWidget.AddToolStripItem(refetchRepositoryButton);

                ToolStripButton refetchLiveButton = new ToolStripButton();
                refetchLiveButton.Text = "Current Usage";
                refetchLiveButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.ImageAndText;
                refetchLiveButton.Image = global::Trafodion.Manager.Properties.Resources.PieChartIcon;
                refetchLiveButton.ImageTransparentColor = System.Drawing.Color.Magenta;
                refetchLiveButton.Name = "refetchLiveButton";
                refetchLiveButton.Click += new EventHandler(refetchLiveButton_Click);
                _theWidget.AddToolStripItem(refetchLiveButton);
            }

            // place the widget in the host container
            TheChartControl.Dock = DockStyle.Fill;
            _theChartPanel.Controls.Add(TheChartControl);

            GridLayoutManager gridLayoutManager = new GridLayoutManager(2, 1);
            gridLayoutManager.CellSpacing = 4;
            widgetCanvas.LayoutManager = gridLayoutManager;

            GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            WidgetContainer widgetContainer = new WidgetContainer(widgetCanvas, _theWidget, "Summary");
            widgetContainer.Name = "SQL Space Usage Summary";
            widgetContainer.AllowDelete = false;
            widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);

            gridConstraint = new GridConstraint(1, 0, 1, 1);
            widgetContainer = new WidgetContainer(widgetCanvas, _theDisplayPanel, " ");
            widgetContainer.Name = "Top SQL Space Users";
            widgetContainer.AllowDelete = false;
            widgetCanvas.AddWidget(widgetContainer, gridConstraint, -1);
            widgetCanvas.Dock = DockStyle.Fill;

            _dataGrid = ((TabularDataDisplayControl)_theWidget.DataDisplayControl).DataGrid;
            _dataGrid.CellClick += _dataGrid_CellClick;

        }

        void _dataGrid_CellClick(object sender, iGCellClickEventArgs e)
        {
            if (e.ColIndex == HyperLinkColumnNumber)
            {
                try
                {
                    HandleHyperLink(e.RowIndex, e.ColIndex);
                }
                catch (Exception ex) { }
            }            
        }

        private void SpaceUsageSummaryUserControl_Load(object sender, EventArgs e)
        {
            spaceUsersLabel.Text = ObjectType;
            _theTopNPanel.Visible = ShowTopNPanel;
            if (_theDatabaseDataProvider != null)
            {
                // Start data provider
                ((DatabaseDataProviderConfig)_theDatabaseDataProvider.DataProviderConfig).SQLText = _theSqlText;
                _theDatabaseDataProvider.Start();
            }
        }

        void MyDispose(bool disposing)
        {
            if (_theDatabaseDataProvider != null)
            {
                _theDatabaseDataProvider.Stop();
            }
            if (_hyperLinkCellManager != null)
            {
                _hyperLinkCellManager.Detach();
            }
            if (_dataGrid != null)
            {
                _dataGrid.CellClick -= _dataGrid_CellClick;
            }
        }

        void refetchRepositoryButton_Click(object sender, EventArgs e)
        {
            _fetchMode = FetchMode.Repository;
            RefetchFromRepository();
        }

        void refetchLiveButton_Click(object sender, EventArgs e)
        {
            _fetchMode = FetchMode.Live;
            RefetchLive();
        }


        private void _topNumericUpDown_ValueChanged(object sender, EventArgs e)
        {
            UpdateChart();
        }

        #endregion private methods        

        #region methods overridable by subclasses

        protected virtual void HandleHyperLink(int rowIndex, int colIndex)
        {

        }

        protected virtual MSChartControl CreateChartControl()
        {
            ChartConfig chartConfig = new ChartConfig();
            chartConfig.ChartPalette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.BrightPastel;
            ChartAreaConfig areaConfig = new ChartAreaConfig();
            chartConfig.ChartAreaConfigs.Add(areaConfig);
            areaConfig.Name = "ChartArea";
            areaConfig.EnableTitle = true;
            areaConfig.Title = "Top 10 " + ObjectType;
            areaConfig.IsTitleInsideChart = false;
            areaConfig.TitleName = "Chart1";
            areaConfig.ChartFont = new System.Drawing.Font("Tahoma", 14f, System.Drawing.FontStyle.Bold);
            ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
            areaConfig.ChartSeriesConfigs.Add(seriesConfig);
            seriesConfig.ChartAreaName = areaConfig.Name;
            seriesConfig.Name = "SpaceUsage";
            seriesConfig.XValueColumnName = "Name";
            seriesConfig.YValueColumnName = "CurrentSize";
            seriesConfig.ToolTip = "#VALX #VALY(MB)";
            areaConfig.YAxisTitle = "Space Used (MB)";
            //areaConfig.XAxisLabelStyleFont = new System.Drawing.Font("Tahoma", 10f, System.Drawing.FontStyle.Regular);
            //areaConfig.YAxisLabelStyleFont = new System.Drawing.Font("Tahoma", 10f, System.Drawing.FontStyle.Regular);
            seriesConfig.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Column;
            seriesConfig.Enabled = true;
            TitleConfig titleConfig = new TitleConfig("Chart1", "Top 10 " + ObjectType);
            titleConfig.DockedChartAreaName = areaConfig.Name;
            chartConfig.TitleConfigs.Add(titleConfig);

            MSChartControl chartControl = new MSChartControl(chartConfig);

            return chartControl;
        }
        
        virtual protected void RefetchFromRepository()
        {
            _theWidget.DataProvider.Stop();
            _theWidget.StartDataProvider();
        }

        virtual protected void RefetchLive()
        {
            _theWidget.DataProvider.Stop();
            _theWidget.StartDataProvider();
        }

        public virtual void UpdateChart()
        {
            if (TheChartDataTable.Rows.Count > 0)
            {
                if (TheChartDataTable.Rows.Count < 100)
                    _topNumericUpDown.Maximum = TheChartDataTable.Rows.Count;

                int numberObjects = (int)_topNumericUpDown.Value;
                DataTable topNDataTable = TheChartDataTable.Clone();
                DataRow[] topNRows = TheChartDataTable.Select("CurrentSize >= 0", "CurrentSize DESC", DataViewRowState.CurrentRows).Take(numberObjects).ToArray();
                foreach (DataRow row in topNRows)
                {
                    topNDataTable.Rows.Add(row.ItemArray);
                }
                TheChartControl.ChartConfiguration.TitleConfigs[0].Text = string.Format("Top {0} {1}", numberObjects, ObjectType);
                TheChartControl.PopulateChart(topNDataTable);
                //TheChartControl.TheChart.ChartAreas[0].AxisX.LabelStyle.Angle = -10;
                TheChartControl.Update();
            }
            else
            {

            }
        }
        #endregion methods overridable by subclasses

        private void _lockLayoutToolStripButton_Click(object sender, EventArgs e)
        {
            if (this._lockLayoutToolStripButton.Text.StartsWith("Lock"))
            {
                widgetCanvas.LockMenuItem.PerformClick();
                _lockLayoutToolStripButton.Text = "Unlock Layout";
                //_lockLayoutToolStripButton.Image = Trafodion.Manager.Properties.Resources.Unlocked;
            }
            else
            {
                widgetCanvas.LockMenuItem.PerformClick();
                _lockLayoutToolStripButton.Text = "Lock Layout";
                //_lockLayoutToolStripButton.Image = Trafodion.Manager.Properties.Resources.Locked;
            }
        }

        private void _resetLayoutToolStripButton_Click(object sender, EventArgs e)
        {
            widgetCanvas.ResetWidgetLayout();
        }

    }

    #region Class SpaceUsagesSummaryDataHandler

    public class SpaceUsagesSummaryDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private SpaceUsageSummaryUserControl m_caller = null;
        bool hyperLinkInitialized = false;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public SpaceUsagesSummaryDataHandler(SpaceUsageSummaryUserControl caller)
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
            DataTable gridDataTable = new DataTable();
            DataTable chartDataTable = new DataTable();
            chartDataTable.Columns.Add("Name");
            chartDataTable.Columns.Add("CurrentSize", typeof(decimal)); 
            
            try
            {
                aDataGrid.BeginUpdate();
                DataRow[] dataRows;
                DataRow dataRow;

                if (aDataTable.Rows.Count == 0)
                {
                    // do nothing.
                }
                else
                {
                    gridDataTable.Columns.Add("Name");
                    gridDataTable.Columns.Add("Oldest Collection Time", typeof(JulianTimestamp));
                    gridDataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
                    //gridDataTable.Columns.Add("% Used", typeof(PercentObject));
                    TimeSpan serverGMTOffset = m_caller.CollectionTimeOffset;
                    string timeZoneName = m_caller.TheTrafodionObject.ConnectionDefinition.ServerTimeZoneName;

                    for (int i = 0; i < aDataTable.Rows.Count; i++)
                    {
                        string name = aDataTable.Rows[i]["name"] as string;
                        name = TrafodionName.ExternalForm(name);

                        decimal currentEof = 0;
                        Object currentEOFObj = aDataTable.Rows[i]["space_used"];
                        if(currentEOFObj != System.DBNull.Value)
                        {
                            currentEof = (decimal) currentEOFObj;
                        }

                        Object julianValue = aDataTable.Rows[i]["last_collection_time"];
                        long lastCollectionTime = 0;
                        if(julianValue != System.DBNull.Value)
                        {
                            lastCollectionTime = (long)julianValue;
                        }
                        gridDataTable.Rows.Add(
                            name.Trim(),
                            (lastCollectionTime == 0 && currentEof > 0) ? new JulianTimestamp(lastCollectionTime, "Partial", serverGMTOffset, timeZoneName) : new JulianTimestamp(lastCollectionTime, serverGMTOffset, timeZoneName),
                            new DecimalSizeObject(currentEof)
                            );

                        chartDataTable.Rows.Add(name.Trim(), currentEof/(1024 * 1024));
                    }

                    base.DoPopulate(aConfig, gridDataTable, aDataGrid);
                    if (aConfig.DataProviderConfig.ColumnSortObjects != null && aConfig.DataProviderConfig.ColumnSortObjects.Count == 0)
                    {
                        if (aDataGrid.Cols.KeyExists(Properties.Resources.TotalCurrentSize))
                        {
                            aConfig.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(aDataGrid.Cols[Properties.Resources.TotalCurrentSize].Index, 0, (int)iGSortOrder.Descending));
                        }
                    }

                    aConfig.DataProviderConfig.DefaultVisibleColumnNames = DefaultVisibleColumn(gridDataTable);
                    TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);

                    aDataGrid.UpdateCountControlText(m_caller.HeaderText);
                    if (!hyperLinkInitialized && m_caller.TheDatabaseObjectsControl != null)
                    {
                        m_caller.HyperLinkCellManager.Attach(aDataGrid, 0);
                        hyperLinkInitialized = true;
                    }
                }
            }
            finally
            {
                aDataGrid.EndUpdate();
                aDataGrid.Cols.AutoWidth();
            }
            m_caller.TheChartDataTable = chartDataTable;
            m_caller.UpdateChart();
        }

        #endregion Public methods

        #region Private methods
        private List<string> DefaultVisibleColumn(DataTable gridDataTable)
        {
            List<string> list = new List<string>();
            foreach (DataColumn dr in gridDataTable.Columns)
            {
                list.Add(dr.ColumnName);
            }
            return list;
        }
        #endregion Private methods
    }

    #endregion Class SpaceUsagesSummaryDataHandler
}
