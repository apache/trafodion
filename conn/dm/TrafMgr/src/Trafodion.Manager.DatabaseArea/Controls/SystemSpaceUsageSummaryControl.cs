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
using System.Linq;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.UniversalWidget.Controls;
using System.Data;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework;
using TenTec.Windows.iGridLib;
using System;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using Trafodion.Manager.Framework.Connections;
using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public class SystemSpaceUsageSummaryControl : SpaceUsageSummaryUserControl, ICloneToWindow
    {
        protected override string ObjectType
        {
            get
            {
                return "Catalogs";
            }
        }

        public override string HeaderText
        {
            get
            {
                return "System has {0} catalogs and uses {1} of total SQL space {2}";
            }
        }

        protected override IDataDisplayHandler TheDataDisplayHandler
        {
            get
            {
                if (_dataDisplayHandler == null)
                {
                    _dataDisplayHandler = new SystemSpaceUsageDataHandler(this);
                }
                return _dataDisplayHandler;
            }
        }

        protected override string HelpTopicName
        {
            get
            {
                return HelpTopics.SystemSQLSpaceUsage;
            }
        }

        protected override string PersistenceKey
        {
            get
            {
                return "SystemSpaceUsageSummaryPersistenceKey";
            }
        }

        public SystemSpaceUsageSummaryControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionSystem aTrafodionSystem)
            : base(aDatabaseObjectsControl, aTrafodionSystem)
        {
            this.DataProvider = new SystemSpaceDataProvider(this.DataProvider.DataProviderConfig);
            SQLText = Model.Queries.GetSystemLevelSpaceUsageQueryString();
        }

        public SystemSpaceUsageSummaryControl(SystemSpaceUsageSummaryControl clone)
            : this(null, (TrafodionSystem)clone.TheTrafodionObject)
        {
            this.DataProvider = new SystemSpaceDataProvider(this.DataProvider.DataProviderConfig);
            SQLText = Model.Queries.GetSystemLevelSpaceUsageQueryString();
        }

        protected override void HandleHyperLink(int rowIndex, int colIndex)
        {
            string catalogName = TheDataGrid.Rows[rowIndex].Cells[colIndex].Value as string;
            if (!string.IsNullOrEmpty(catalogName))
            {
                TrafodionCatalog catalog = ((TrafodionSystem)_theTrafodionObject).FindCatalog(catalogName.Trim());
                if (catalog != null && TheDatabaseObjectsControl != null)
                {
                    TheDatabaseObjectsControl.TheDatabaseTreeView.SelectTrafodionObject(catalog);
                }
            }

        }

        protected override UniversalWidget.Controls.MSChartControl CreateChartControl()
        {
            ChartConfig chartConfig = new ChartConfig();
            chartConfig.ChartPalette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.BrightPastel;
            ChartAreaConfig areaConfig = new ChartAreaConfig();
            chartConfig.ChartAreaConfigs.Add(areaConfig);
            areaConfig.Name = "ChartArea";
            areaConfig.EnableTitle = true;
            areaConfig.Title = "SQL Space Distribution";
            areaConfig.IsTitleInsideChart = false;
            areaConfig.TitleName = "Chart1";
            areaConfig.ChartFont = new System.Drawing.Font("Tahoma", 10f, System.Drawing.FontStyle.Bold);
            areaConfig.Enable3D = true;

            ChartSeriesConfig seriesConfig = new ChartSeriesConfig();
            areaConfig.ChartSeriesConfigs.Add(seriesConfig);
            seriesConfig.ChartAreaName = areaConfig.Name;
            seriesConfig.Name = "SpaceUsage";
            seriesConfig.XValueColumnName = "Name";
            seriesConfig.YValueColumnName = "PercentSystemSize";
            areaConfig.XAxisTitle = "Catalogs";
            areaConfig.YAxisTitle = "Space Used";
            seriesConfig.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Pie;
            seriesConfig.Palette = System.Windows.Forms.DataVisualization.Charting.ChartColorPalette.BrightPastel;
            seriesConfig.Enabled = true;
            seriesConfig.IsValueShownAsLabel = true;
            seriesConfig.Label = "#VALX #PERCENT";
            TitleConfig titleConfig = new TitleConfig("Chart1", "SQL Space Distribution");
            titleConfig.DockedChartAreaName = areaConfig.Name;
            chartConfig.TitleConfigs.Add(titleConfig);

            MSChartControl chartControl = new MSChartControl(chartConfig);

            return chartControl;            
        }

        public override void UpdateChart()
        {
            if (TheChartDataTable.Rows.Count > 0)
            {

                DecimalSizeObject systemSize = new DecimalSizeObject(TrafodionSystem.FindTrafodionSystem(this._theTrafodionObject.ConnectionDefinition).TotalSQLSpace);
                decimal systemSpaceMB = (decimal)systemSize.TheValue / (1024 * 1024);

                if (TheChartDataTable.Rows.Count < 100)
                    TopNumericUpDown.Maximum =  TheChartDataTable.Rows.Count;

                int numberObjects = (int)TopNumericUpDown.Value;
                decimal totalSize = 0;
                foreach (DataRow row in TheChartDataTable.Rows)
                {
                    decimal size = (decimal)row["CurrentSize"];
                    totalSize += size;
                }
                DataTable topNDataTable = TheChartDataTable.Clone();
                DataRow[] topNRows = TheChartDataTable.Select("CurrentSize >= 0", "CurrentSize DESC", DataViewRowState.CurrentRows).Take(numberObjects).ToArray();

                decimal totalDisplaySize = 0;
                foreach (DataRow row in topNRows)
                {
                    topNDataTable.Rows.Add(row.ItemArray);
                    decimal size = (decimal)row["CurrentSize"]; 
                    totalDisplaySize += size;
                }

                if (totalDisplaySize < totalSize)
                {
                    decimal otherSizeMB = totalSize - totalDisplaySize;
                    topNDataTable.Rows.Add(new object[] { "Others", 0, otherSizeMB / systemSpaceMB });
                }

                if (totalSize < systemSpaceMB)
                {
                    decimal freeSizeMB = systemSpaceMB - totalSize;
                    topNDataTable.Rows.Add(new object[] { "Free", 0, freeSizeMB / systemSpaceMB });
                }

                TheChartControl.ChartConfiguration.TitleConfigs[0].Text = string.Format("Top {0} {1}", numberObjects, ObjectType);
                TheChartControl.PopulateChart(topNDataTable);
                //TheChartControl.TheChart.ChartAreas[0].AxisX.LabelStyle.Angle = -10;
                TheChartControl.TheChart.ChartAreas[0].Area3DStyle.Inclination = 0;
                TheChartControl.TheChart.Series[0]["PieLabelStyle"] = "Outside";
                TheChartControl.TheChart.Series[0]["PieDrawingStyle"] = "SoftEdge";
                TheChartControl.TheChart.Series[0]["PieLineColor"] = "Black";
                TheChartControl.Update();
            }
            else
            {

            } 
        }

        #region ICloneToWindow Members
        public Control Clone()
        {
            return new SystemSpaceUsageSummaryControl(this);
        }

        public string WindowTitle
        {
            get { return "All Catalogs" + " " + Properties.Resources.SpaceUsage; ; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theTrafodionObject.ConnectionDefinition; }
        }

        #endregion ICloneToWindow Members
    }
    #region Class SystemSpaceUsageDataHandler

    public class SystemSpaceUsageDataHandler : TabularDataDisplayHandler
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
        public SystemSpaceUsageDataHandler(SpaceUsageSummaryUserControl caller)
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
            chartDataTable.Columns.Add("PercentSystemSize", typeof(decimal));
            long systemSpaceMB = 0;
            decimal totalUsedSize = 0;
            try
            {
                DecimalSizeObject systemSize = new DecimalSizeObject(TrafodionSystem.FindTrafodionSystem(aConfig.DataProviderConfig.ConnectionDefinition).TotalSQLSpace);
                systemSpaceMB = (long)systemSize.TheValue/ (1024 * 1024);


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
                    gridDataTable.Columns.Add(Properties.Resources.TotalCompressedSize, typeof(string));
                    gridDataTable.Columns.Add("% of Total SQL Space", typeof(PercentObject));

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
                        string name = aDataTable.Rows[i]["name"] as string;
                        name = TrafodionName.ExternalForm(name);

                        decimal currentEof = 0;
                        Object currentEOFObj = aDataTable.Rows[i]["space_used"];
                        if (currentEOFObj != System.DBNull.Value)
                        {
                            currentEof = (decimal)currentEOFObj;
                        }

                        decimal compressedEof = 0;
                        Object compressedEOFObj = aDataTable.Rows[i]["compressed_space_used"];
                        if (compressedEOFObj != System.DBNull.Value)
                        {
                            compressedEof = (decimal)compressedEOFObj;
                        }

                        Object julianValue = aDataTable.Rows[i]["last_collection_time"];
                        long lastCollectionTime = 0;
                        if (julianValue != System.DBNull.Value)
                        {
                            lastCollectionTime = (long)julianValue;
                        }
                        totalUsedSize += currentEof;

                        decimal currentMB = currentEof / (1024 * 1024);

                        double percentOfSystemSize = (double)currentMB / systemSpaceMB;
                        gridDataTable.Rows.Add(
                            name.Trim(),
                            (lastCollectionTime == 0 && currentEof > 0) ? new JulianTimestamp(lastCollectionTime, "Partial", serverGMTOffset, timeZoneName) : new JulianTimestamp(lastCollectionTime, serverGMTOffset, timeZoneName),
                            new DecimalSizeObject(currentEof),
                            iCompressionType>0?new DecimalSizeObject(compressedEof).ToString():"N/A",
                            new PercentObject(percentOfSystemSize * 100)
                            );

                        chartDataTable.Rows.Add(name.Trim(), currentMB, percentOfSystemSize);
                    }
                    //remove compressed space usage column
                    if (m_caller is SystemSpaceUsageSummaryControl)
                    {
                        if (gridDataTable.Columns.Contains(Properties.Resources.TotalCompressedSize))
                        {
                            gridDataTable.Columns.Remove(Properties.Resources.TotalCompressedSize);
                        }
                    }

                    if (!hasCompressedData)
                    {
                        if (gridDataTable.Columns.Contains(Properties.Resources.TotalCompressedSize))
                        {
                            gridDataTable.Columns.Remove(Properties.Resources.TotalCompressedSize);
                        }
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
                    string headerText = string.Format(m_caller.HeaderText, aDataGrid.Rows.Count, new DecimalSizeObject(totalUsedSize).ToString(), systemSize.ToString());
                    aDataGrid.UpdateCountControlText(headerText);
                    if (!hyperLinkInitialized && m_caller.TheDatabaseObjectsControl!=null)
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

    #endregion Class SystemSpaceUsageDataHandler
}
