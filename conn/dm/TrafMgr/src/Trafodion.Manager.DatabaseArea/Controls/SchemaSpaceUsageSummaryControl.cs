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
using Trafodion.Manager.DatabaseArea.Controls.Tree;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class SchemaSpaceUsageSummaryControl : SpaceUsageSummaryUserControl, ICloneToWindow
    {

        public override string HeaderText
        {
            get
            {
                return "Current Schema Size : {0}";
            }
        }

        protected override bool ShowTopNPanel
        {
            get
            {
                return false;
            }
        }

        protected override bool SupportsLiveFetch
        {
            get
            {
                return true;
            }
        }

        protected override IDataDisplayHandler TheDataDisplayHandler
        {
            get
            {
                if (_dataDisplayHandler == null)
                {
                    _dataDisplayHandler = new SchemaSpaceUsageDataHandler(this);
                }
                return _dataDisplayHandler;
            }
        }

        protected override string HelpTopicName
        {
            get
            {
                return HelpTopics.SchemaSpaceUsage;
            }
        }

        protected override string PersistenceKey
        {
            get
            {
                return "SchemaSpaceUsageSummaryPersistenceKey";
            }
        }

        public SchemaSpaceUsageSummaryControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSchema aTrafodionSchema)
            : base(aDatabaseObjectsControl, aTrafodionSchema)
        {
            SQLText = Model.Queries.GetSchemaSpaceUsageQueryString(aTrafodionSchema, false);
            
        }

        public SchemaSpaceUsageSummaryControl(SchemaSpaceUsageSummaryControl clone)
            : this(null, (TrafodionSchema)clone.TheTrafodionObject)
        {            

        }

        #region methods overriden from base class


        protected override void HandleHyperLink(int rowIndex, int colIndex)
        {
            string childFolderName = TheDataGrid.Rows[rowIndex].Cells[colIndex].Value as string;
            if (!string.IsNullOrEmpty(childFolderName))
            {
                TreeNode currentSelectedNode = TheDatabaseObjectsControl.TheDatabaseTreeView.SelectedNode;
                if (currentSelectedNode != null && currentSelectedNode is SchemaFolder)
                {
                    currentSelectedNode.Expand();
                    TreeNode childNode = TheDatabaseObjectsControl.TheDatabaseTreeView.FindChildNode(currentSelectedNode, childFolderName);
                    if (childNode != null)
                    {
                        TheDatabaseObjectsControl.TheDatabaseTreeView.SelectedNode = childNode;
                    }
                }
            }
        }

        protected override MSChartControl CreateChartControl()
        {
            ChartConfig chartConfig = new ChartConfig();
            chartConfig.ChartPalette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.BrightPastel;
            ChartAreaConfig areaConfig = new ChartAreaConfig();
            chartConfig.ChartAreaConfigs.Add(areaConfig);
            areaConfig.Name = "ChartArea";
            areaConfig.EnableTitle = true;
            areaConfig.Title = "Schema Space Distribution";
            areaConfig.IsTitleInsideChart = false;
            areaConfig.TitleName = "Chart1";
            areaConfig.ChartFont = new System.Drawing.Font("Tahoma", 10f, System.Drawing.FontStyle.Bold);
            areaConfig.Enable3D = true;

            ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
            areaConfig.ChartSeriesConfigs.Add(seriesConfig);
            seriesConfig.ChartAreaName = areaConfig.Name;
            seriesConfig.Name = "SpaceUsage";
            seriesConfig.XValueColumnName = "Type";
            seriesConfig.YValueColumnName = "Space";
            areaConfig.XAxisTitle = "SQL Objects";
            areaConfig.YAxisTitle = "Space Used";
            seriesConfig.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Pie;
            seriesConfig.Palette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.BrightPastel;
            seriesConfig.Enabled = true;
            seriesConfig.IsValueShownAsLabel = true;
            seriesConfig.Label = "#VALX #PERCENT";
            TitleConfig titleConfig = new TitleConfig("Chart1", "Schema Space Distribution");
            titleConfig.DockedChartAreaName = areaConfig.Name;
            chartConfig.TitleConfigs.Add(titleConfig);

            MSChartControl chartControl = new MSChartControl(chartConfig);

            return chartControl;
        }

        override protected void RefetchFromRepository()
        {
            _theWidget.DataProvider.Stop();
            DatabaseDataProviderConfig dbConfig = _theWidget.DataProvider.DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = Model.Queries.GetSchemaSpaceUsageQueryString(_theTrafodionObject as TrafodionSchema, false);
            _theWidget.StartDataProvider();
        }

        override protected void RefetchLive()
        {
            _theWidget.DataProvider.Stop();
            DatabaseDataProviderConfig dbConfig = _theWidget.DataProvider.DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = Model.Queries.GetSchemaSpaceUsageQueryString(_theTrafodionObject as TrafodionSchema, true);
            _theWidget.StartDataProvider();
        }

        public override void UpdateChart()
        {
            if (TheChartDataTable.Rows.Count > 0)
            {
                TheChartControl.PopulateChart(TheChartDataTable);
                TheChartControl.TheChart.ChartAreas[0].Area3DStyle.Inclination = 0;
                TheChartControl.TheChart.Series[0]["PieLabelStyle"] = "Outside";
                TheChartControl.TheChart.Series[0]["PieDrawingStyle"] = "SoftEdge";
                TheChartControl.TheChart.Series[0]["PieLineColor"] = "Black";
            }
        }

        #endregion methods overriden from base class

        #region ICloneToWindow Members
        public Control Clone()
        {
            return new SchemaSpaceUsageSummaryControl(this);
        }

        public string WindowTitle
        {
            get { return _theTrafodionObject.VisibleAnsiName + " " + Properties.Resources.SpaceUsage;  }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theTrafodionObject.ConnectionDefinition; }
        }

        #endregion ICloneToWindow Members
    }

    #region Class SchemaSpaceUsageyDataHandler

    public class SchemaSpaceUsageDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private SchemaSpaceUsageSummaryControl m_caller = null;
        bool hyperLinkInitialized = false;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public SchemaSpaceUsageDataHandler(SchemaSpaceUsageSummaryControl caller)
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
            DataTable chartDataTable = new DataTable();
            chartDataTable.Columns.Add("Type");
            chartDataTable.Columns.Add("Space");

            DataTable gridDataTable = new DataTable();

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
                    gridDataTable.Columns.Add("Object Type");
                    gridDataTable.Columns.Add("Oldest Collection Time", typeof(JulianTimestamp));
                    gridDataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
                    gridDataTable.Columns.Add(Properties.Resources.TotalCompressedSize, typeof(string));
                    gridDataTable.Columns.Add("% of Current Schema Size", typeof(PercentObject));

                    decimal totalSpaceUsed = 0;
                    decimal totalMax = 0;
                    TimeSpan serverGMTOffset = m_caller.CollectionTimeOffset;
                    string timeZoneName = m_caller.TheTrafodionObject.ConnectionDefinition.ServerTimeZoneName;
                    int iCompressionType = -1;
                    bool hasCompressedData = false;

                    for (int i = 0; i < aDataTable.Rows.Count; i++)
                    {
                        if (aDataTable.Columns.Contains("compression_type"))
                        {
                            iCompressionType = Convert.ToInt32(aDataTable.Rows[i]["compression_type"]);
                            if (!hasCompressedData) hasCompressedData = iCompressionType > 0;
                        }

                        decimal currentEof = 0;
                        Object currentEOFObj = aDataTable.Rows[i]["space_used"];
                        if (currentEOFObj != System.DBNull.Value)
                        {
                            currentEof = (decimal)currentEOFObj;
                            totalSpaceUsed += currentEof;
                        }
                        decimal compressedEof = 0;
                        Object compressedEOFObj = aDataTable.Rows[i]["compressed_space_used"];
                        if (compressedEOFObj != System.DBNull.Value)
                        {
                            compressedEof = (decimal)compressedEOFObj;
                        }
                        decimal maxSize = 0;
                        Object currentMaxSizeObj = aDataTable.Rows[i]["total_max_size"];
                        if (currentMaxSizeObj != System.DBNull.Value)
                        {
                            maxSize = (decimal)currentMaxSizeObj;
                            totalMax += maxSize;
                        }
                        Object julianValue = aDataTable.Rows[i]["last_collection_time"];
                        long lastCollectionTime = 0;
                        if (julianValue != System.DBNull.Value)
                        {
                            lastCollectionTime = (long)julianValue;
                        }
                        double percentAllocated = (maxSize > 0) ? ((double)currentEof / (double)maxSize) : 0;

                        gridDataTable.Rows.Add(
                            aDataTable.Rows[i]["object_type"] as string,
                            (lastCollectionTime == 0 && currentEof > 0) ? new JulianTimestamp(lastCollectionTime, "Partial", serverGMTOffset, timeZoneName) : new JulianTimestamp(lastCollectionTime, serverGMTOffset, timeZoneName),
                            new DecimalSizeObject(currentEof),
                            iCompressionType > 0 ? new DecimalSizeObject(compressedEof).ToString() : "N/A",
                             new PercentObject(0)
                            );

                    }

                    for (int i = 0; i < gridDataTable.Rows.Count; i++)
                    {
                        double currentSize = (double)((DecimalSizeObject)gridDataTable.Rows[i][Properties.Resources.TotalCurrentSize]).TheValue;
                        double percentOfSchemaSize = (totalSpaceUsed > 0) ? (currentSize / (double)totalSpaceUsed) : 0;
                        gridDataTable.Rows[i]["% of Current Schema Size"] = new PercentObject(percentOfSchemaSize * 100);
                        chartDataTable.Rows.Add(new object[] { aDataTable.Rows[i]["object_type"], percentOfSchemaSize });
                    }

                    IEnumerable<string> objectTypes =
                        from DataRow row in chartDataTable.Rows
                        select row[0] as string;

                    if (!objectTypes.Contains("Tables"))
                    {
                        chartDataTable.Rows.Add("Tables", 0);
                        gridDataTable.Rows.Add("Tables", new JulianTimestamp(0), new DecimalSizeObject(0), "N/A", new PercentObject(0));
                    }
                    if (!objectTypes.Contains("Materialized Views"))
                    {
                        chartDataTable.Rows.Add(new object[] { "Materialized Views", 0 });
                        gridDataTable.Rows.Add("Materialized Views", new JulianTimestamp(0), new DecimalSizeObject(0), "N/A", new PercentObject(0));
                    }
                    if (!objectTypes.Contains("Indexes"))
                    {
                        chartDataTable.Rows.Add(new object[] { "Indexes", 0 });
                        gridDataTable.Rows.Add("Indexes", new JulianTimestamp(0), new DecimalSizeObject(0), "N/A", new PercentObject(0));
                    }
                    if (!objectTypes.Contains("Internal Objects"))
                    {
                        chartDataTable.Rows.Add(new object[] { "Internal Objects", 0 });
                        gridDataTable.Rows.Add("Internal Objects", new JulianTimestamp(0), new DecimalSizeObject(0), "N/A", new PercentObject(0));
                    }

                    if (!hasCompressedData)
                    {
                        if (gridDataTable.Columns.Contains(Properties.Resources.TotalCompressedSize))
                        {
                            gridDataTable.Columns.Remove(Properties.Resources.TotalCompressedSize);
                        }
                    }

                    base.DoPopulate(aConfig, gridDataTable, aDataGrid);

                    string gridHeaderText = string.Format("Current Schema Size : {0}", new DecimalSizeObject(totalSpaceUsed).ToString());

                    aDataGrid.UpdateCountControlText(gridHeaderText);
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
            }

            if (aConfig.DataProviderConfig.ColumnSortObjects != null && aConfig.DataProviderConfig.ColumnSortObjects.Count == 0)
            {
                if (aDataGrid.Cols.KeyExists(Properties.Resources.TotalCurrentSize))
                {
                    aConfig.DataProviderConfig.ColumnSortObjects.Add(new ColumnSortObject(aDataGrid.Cols[Properties.Resources.TotalCurrentSize].Index, 0, (int)iGSortOrder.Descending));
                }
            }

            aConfig.DataProviderConfig.DefaultVisibleColumnNames = DefaultVisibleColumn(gridDataTable);
            TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);
            aDataGrid.Cols.AutoWidth();
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

    #endregion Class SchemaSpaceUsageDataHandler

}
