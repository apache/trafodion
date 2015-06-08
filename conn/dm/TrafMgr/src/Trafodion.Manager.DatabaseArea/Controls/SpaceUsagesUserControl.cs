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
using System.Data;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.Framework;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class SpaceUsagesUserControl : UserControl
    {
        static readonly string SpaceUsagerPersistenceKey = "SpaceUsagePersistence"; 
        
        private ConnectionDefinition _theConnectionDefinition = null;
        private DatabaseDataProvider _theDatabaseDataProvider = null;
        private GenericUniversalWidget _theWidget = null;
        private string _theSqlText = null;
        private string _headerText = "";

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theConnectionDefinition; }
            set { _theConnectionDefinition = value; }
        }
        public string HeaderText
        {
            get { return _headerText; }
        }

        public SpaceUsagesUserControl(ConnectionDefinition aConnectionDefinition, string aSQLText, string headerText)
        {
            _theConnectionDefinition = aConnectionDefinition;
            _theSqlText = aSQLText;
            _headerText = headerText;
            InitializeComponent();
            ShowWidgets();
        }

        private void ShowWidgets()
        {
            UniversalWidgetConfig widgetConfig = WidgetRegistry.GetConfigFromPersistence(SpaceUsagerPersistenceKey);

            if (widgetConfig == null)
            {
                widgetConfig = new UniversalWidgetConfig();
                widgetConfig.Name = SpaceUsagerPersistenceKey;
                widgetConfig.ShowProviderToolBarButton = false;
                widgetConfig.ShowTimerSetupButton = false;
                widgetConfig.ShowHelpButton = true;
                widgetConfig.ShowChart = true;
                widgetConfig.SupportCharts = true;
                widgetConfig.ShowChartToolBarButton = true;
                widgetConfig.ChartPosition = UniversalWidgetConfig.ChartPositions.TOP;
                widgetConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
                widgetConfig.HelpTopic = "";
            }

            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)WidgetRegistry.GetDefaultDBConfig().DataProviderConfig;
            dbConfig.TimerPaused = true;
            dbConfig.RefreshRate = 0;
            dbConfig.SQLText = _theSqlText;
            dbConfig.ConnectionDefinition = _theConnectionDefinition;
            _theDatabaseDataProvider = new DatabaseDataProvider(dbConfig);
            widgetConfig.DataProvider = _theDatabaseDataProvider;

            ChartConfig chartConfig = GetChartConfig("NAME", "SPACE_USED");
            widgetConfig.ChartConfig = chartConfig;
            _theWidget = new GenericUniversalWidget();
            _theWidget.DataProvider = _theDatabaseDataProvider;
            ((TabularDataDisplayControl)_theWidget.DataDisplayControl).LineCountFormat = Properties.Resources.SpaceUsage; 
            _theWidget.DataDisplayControl.DataDisplayHandler = new SpaceUsagesDataHandler(this);
            _theWidget.UniversalWidgetConfiguration = widgetConfig;
            _theWidget.Dock = DockStyle.Fill;

            if (_theWidget.ChartControl.TheChart != null)
            {
                if (_theWidget.ChartControl.TheChart.ChartAreas.Count > 0)
                {
                    _theWidget.ChartControl.TheChart.ChartAreas[0].AxisY.IsLogarithmic = true;
                    _theWidget.ChartControl.TheChart.ChartAreas[0].AxisY.Minimum = Double.NaN;
                    _theWidget.ChartControl.TheChart.ChartAreas[0].AxisY.Maximum = Double.NaN;
                }
            }
            // place the widget in the host container
            _thePanel.Controls.Clear();
            _thePanel.Controls.Add(_theWidget);

            // Start data provider
            _theDatabaseDataProvider.Start();
        }

        ChartConfig GetChartConfig(string xColName, string yColName)
        {
            ChartConfig chartConfig = new ChartConfig();
            chartConfig.ChartPalette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.BrightPastel;
            ChartAreaConfig areaConfig = new ChartAreaConfig();
            chartConfig.ChartAreaConfigs.Add(areaConfig);
            areaConfig.Name = "ChartArea";
            areaConfig.EnableTitle = true;
            areaConfig.Title = "Top 10 space users";
            areaConfig.IsTitleInsideChart = false;
            areaConfig.TitleName = "Chart1";
            areaConfig.ChartFont = new System.Drawing.Font("Tahoma", 14f, System.Drawing.FontStyle.Bold);
            ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
            areaConfig.ChartSeriesConfigs.Add(seriesConfig);
            seriesConfig.ChartAreaName = areaConfig.Name;
            seriesConfig.Name = "SpaceUsage";
            seriesConfig.XValueColumnName = xColName;
            seriesConfig.YValueColumnName = yColName;
            areaConfig.XAxisTitle = "SQL Objects";
            areaConfig.YAxisTitle = "Space Used";
            seriesConfig.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Column;
            seriesConfig.Enabled = true;
            TitleConfig titleConfig = new TitleConfig("Chart1", "Top 10 space users");
            titleConfig.DockedChartAreaName = areaConfig.Name;
            chartConfig.TitleConfigs.Add(titleConfig);
            return chartConfig;
        }
    }


    #region Class RepositoryInfoDataHandler

    public class SpaceUsagesDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private SpaceUsagesUserControl m_caller = null;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public SpaceUsagesDataHandler(SpaceUsagesUserControl caller)
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
                    // do nothing.
                }
                else
                {
                    DataTable dataTable = new DataTable();
                    dataTable.Columns.Add("Name");
                    dataTable.Columns.Add("Last Collection Time", typeof(JulianTimestamp));
                    //dataTable.Columns.Add("Last Collection Time", typeof(System.DateTime));
                    //dataTable.Columns.Add(Properties.Resources.TotalCurrentRowCount, typeof(long));
                    dataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
                    dataTable.Columns.Add(Properties.Resources.TotalMaximumSize, typeof(DecimalSizeObject));
                    dataTable.Columns.Add(Properties.Resources.PercentAllocated, typeof(PercentObject));

                    for (int i = 0; i < aDataTable.Rows.Count; i++)
                    {
                        decimal currentEof = 0;
                        Object currentEOFObj = aDataTable.Rows[i]["space_used"];
                        if(currentEOFObj != System.DBNull.Value)
                        {
                            currentEof = (decimal) currentEOFObj;
                        }

                        decimal maxSize = 0;
                        Object currentMaxSizeObj = aDataTable.Rows[i]["total_max_size"];
                        if (currentMaxSizeObj != System.DBNull.Value)
                        {
                            maxSize = (decimal)currentMaxSizeObj;
                        }
                        dataTable.Rows.Add(
                            aDataTable.Rows[i]["name"] as string,
                            new JulianTimestamp((long)aDataTable.Rows[i]["last_collection_time"]),
                            new DecimalSizeObject(currentEof),
                            new DecimalSizeObject(maxSize),
                            (maxSize > 0) ? new PercentObject((double)currentEof * 100 / (double)maxSize) : new PercentObject(0)
                            );
                    }

                    base.DoPopulate(aConfig, dataTable, aDataGrid);

                    //string gridHeaderText = string.Format(
                    //    (dataTable.Rows.Count > 1) ? "There are {0} schemas in the catalog" : "there is {0} schema in the catalog", dataTable.Rows.Count);
                    aDataGrid.UpdateCountControlText(m_caller.HeaderText);
                }
            }
            finally
            {
                aDataGrid.EndUpdate();
                aDataGrid.ResizeGridColumns(aDataTable);
            }
        }

        #endregion Public methods

        #region Private methods

        #endregion Private methods
    }

    #endregion Class RepositoryInfoDataHandler

}
