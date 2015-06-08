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
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using TenTec.Windows.iGridLib;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using System.Windows.Forms;
using System.Collections.Generic;

namespace Trafodion.Manager.DatabaseArea.Controls
{
    public partial class IndexesSpaceUsageSummaryControl : SpaceUsageSummaryUserControl, ICloneToWindow
    {
        protected override string ObjectType
        {
            get { return Properties.Resources.Indexes; }
        }
        public override string HeaderText
        {
            get { return "SQL Space Usage of {0} " + Properties.Resources.Indexes; }
        }
        protected override bool SupportsLiveFetch
        {
            get { return true; }
        }
        protected override string PersistenceKey
        {
            get { return "IndexesSpaceUsageSummaryPersistence"; }
        }

        protected override IDataDisplayHandler TheDataDisplayHandler
        {
            get
            {
                if (_dataDisplayHandler == null)
                {
                    _dataDisplayHandler = new IndexesSpaceUsageSummaryDataHandler(this);
                }
                return _dataDisplayHandler;
            }
        }
        protected override string HelpTopicName
        {
            get
            {
                return HelpTopics.IndexesSpaceUsage;
            }
        }

        public IndexesSpaceUsageSummaryControl(DatabaseObjectsControl aDatabaseObjectsControl, TrafodionObject aTrafodionObject)
            : base(aDatabaseObjectsControl, aTrafodionObject)
        {
            SQLText = Model.Queries.GetIndexesSpaceUsageSummaryQueryString(aTrafodionObject, false);
        }

        public IndexesSpaceUsageSummaryControl(IndexesSpaceUsageSummaryControl clone)
            : this(null, clone.TheTrafodionObject)
        {            
        }

        protected override void HandleHyperLink(int rowIndex, int colIndex)
        {
            string indexName = TheDataGrid.Rows[rowIndex].Cells[colIndex].Value as string;
            if (!string.IsNullOrEmpty(indexName))
            {
                string internalName = TrafodionName.InternalForm(indexName).Trim();
                TrafodionIndex index = null;
                if (_theTrafodionObject is TrafodionSchema)
                {
                    index = ((TrafodionSchema)_theTrafodionObject).FindIndex(internalName);
                }
                if (_theTrafodionObject is TrafodionSchemaObject)
                {
                    index = ((TrafodionSchemaObject)_theTrafodionObject).TheTrafodionSchema.FindIndex(internalName);
                }

                if (index != null && TheDatabaseObjectsControl != null)
                {
                    TheDatabaseObjectsControl.TheDatabaseTreeView.SelectTrafodionObject(index);
                }
            }
        }

        override protected void RefetchFromRepository()
        {
            _theWidget.DataProvider.Stop();
            DatabaseDataProviderConfig dbConfig = _theWidget.DataProvider.DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = Model.Queries.GetIndexesSpaceUsageSummaryQueryString(_theTrafodionObject, false);
            _theWidget.StartDataProvider();
        }

        override protected void RefetchLive()
        {
            _theWidget.DataProvider.Stop();
            DatabaseDataProviderConfig dbConfig = _theWidget.DataProvider.DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = Model.Queries.GetIndexesSpaceUsageSummaryQueryString(_theTrafodionObject, true);
            _theWidget.StartDataProvider();
        }

        #region ICloneToWindow Members
        public Control Clone()
        {
            return new IndexesSpaceUsageSummaryControl(this);            
        }
        
        public string WindowTitle
        {
            get { return _theTrafodionObject.VisibleAnsiName+" "+Properties.Resources.SpaceUsage;; }
        }

        public ConnectionDefinition ConnectionDefn
        {
            get { return _theTrafodionObject.ConnectionDefinition; }
        }

        #endregion ICloneToWindow Members
    } 

    #region Class IndexesSpaceUsageSummaryDataHandler

    public class IndexesSpaceUsageSummaryDataHandler : TabularDataDisplayHandler
    {
        #region Fields

        private IndexesSpaceUsageSummaryControl m_caller = null;
        bool hyperLinkInitialized = false;

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="aStatsType"></param>
        public IndexesSpaceUsageSummaryDataHandler(IndexesSpaceUsageSummaryControl caller)
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
                    gridDataTable.Columns.Add("Object Name");
                    gridDataTable.Columns.Add("Last Collection Time", typeof(JulianTimestamp));
                    gridDataTable.Columns.Add("Row Count", typeof(long));
                    gridDataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
                    gridDataTable.Columns.Add(Properties.Resources.TotalCompressedSize, typeof(string));
                    gridDataTable.Columns.Add(Properties.Resources.TotalMaximumSize, typeof(DecimalSizeObject));
                    gridDataTable.Columns.Add(Properties.Resources.PercentAllocated, typeof(PercentObject));

                    if (m_caller.TheTrafodionObject != null &&
                        m_caller.TheTrafodionObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                    {
                        gridDataTable.Columns.Add("Overhead", typeof(DecimalSizeObject));
                        gridDataTable.Columns.Add("Access Count", typeof(long));

                    }

                    TimeSpan serverGMTOffset = m_caller.CollectionTimeOffset;
                    string timeZoneName = m_caller.TheTrafodionObject.ConnectionDefinition.ServerTimeZoneName;
                    decimal totalSpaceUsed = 0;
                    decimal totalMax = 0;
                    int iCompressionType = -1;
                    bool hasCompressedData = false;

                    for (int i = 0; i < aDataTable.Rows.Count; i++)
                    {
                        string objectName = aDataTable.Rows[i]["object_name"] as string;
                        objectName = TrafodionName.ExternalForm(objectName);

                        if (aDataTable.Columns.Contains("compression_type"))
                        {
                            iCompressionType = Convert.ToInt32(aDataTable.Rows[i]["compression_type"]);
                            if (!hasCompressedData) hasCompressedData = iCompressionType > 0;
                        }

                        Object collectiontimeValue = aDataTable.Rows[i]["last_collection_time"];
                        long lastCollectionTime = 0;
                        if (collectiontimeValue != System.DBNull.Value)
                        {
                            lastCollectionTime = (long)collectiontimeValue;
                        }

                        long rowCount = 0;
                        object rowCountValue = aDataTable.Rows[i]["row_count"];
                        if (rowCountValue != System.DBNull.Value)
                        {
                            rowCount = (long)rowCountValue;
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


                        double percentAllocated = (maxSize > 0) ? ((double)currentEof / (double)maxSize) : 0;

                        if (m_caller.TheTrafodionObject != null &&
                            m_caller.TheTrafodionObject.ConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
                        {
                            decimal accessCount = 0;
                            Object accessCountObj = aDataTable.Rows[i]["access_counter"];
                            if (accessCountObj != System.DBNull.Value)
                            {
                                accessCount = (decimal)accessCountObj;
                            }

                            decimal spaceOverhead = 0;
                            Object spaceOverheadObj = aDataTable.Rows[i]["overhead"];
                            if (spaceOverheadObj != System.DBNull.Value)
                            {
                                spaceOverhead = (decimal)spaceOverheadObj;
                            }

                            gridDataTable.Rows.Add(
                                objectName.Trim(),
                                new JulianTimestamp(lastCollectionTime, serverGMTOffset, timeZoneName),
                                rowCount,
                                new DecimalSizeObject(currentEof),
                                iCompressionType > 0 ?new DecimalSizeObject(compressedEof).ToString():"N/A",
                                new DecimalSizeObject(maxSize),
                                new PercentObject(percentAllocated * 100),
                                new DecimalSizeObject(spaceOverhead),
                                (long)accessCount
                                );

                        }
                        else
                        {
                            gridDataTable.Rows.Add(
                                objectName.Trim(),
                                new JulianTimestamp(lastCollectionTime, serverGMTOffset, timeZoneName),
                                rowCount,
                                new DecimalSizeObject(currentEof),
                                new DecimalSizeObject(compressedEof),
                                new DecimalSizeObject(maxSize),
                                new PercentObject(percentAllocated * 100)
                                );

                        }


                        chartDataTable.Rows.Add(objectName.Trim(), currentEof / (1024 * 1024));

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
                    if (!hyperLinkInitialized && m_caller.TheDatabaseObjectsControl != null)
                    {
                        m_caller.HyperLinkCellManager.Attach(aDataGrid, 0);
                        hyperLinkInitialized = true;
                    }

                    aConfig.DataProviderConfig.DefaultVisibleColumnNames = DefaultVisibleColumn(gridDataTable);
                    TabularDataDisplayControl.ApplyColumnAttributes(aDataGrid, aConfig.DataProviderConfig);
                    aDataGrid.UpdateCountControlText(m_caller.HeaderText);
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

    #endregion Class IndexesSpaceUsageSummaryDataHandler

}
