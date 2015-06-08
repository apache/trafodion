using System;
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
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.OverviewArea.Models
{
    /// <summary>
    /// Model handling color scheme for systems
    /// </summary>
    class SystemMetricChartConfigModel
    {
        #region Fields

        // the persistence key 
        public const string SystemMetricColorTablePersistenceKey = "SystemMetricColorTablePersistenceKey_2";
        public const string HealthStatesTablePersistentKey = "HealthStatesTablePersistentKey_1";
        public const string DynamicSystemMetricColorTablePersistenceKey = "DynamicSystemMetricColorTablePersistenceKey";

        // Column defintion for the color table
        public enum SystemMetricsColorTableColumns
        {
            ColMetric = 0,
            ColMetricBackColor,
            ColMetricColor,
            ColMetricGridLineColor,
            ColMetricCursorColor,
            ColMetricThresholdColor,
            ColMetricAverageLineColor,
            ColMetricMaxLineColor,
            ColMetricMinLineColor,
            ColDisplayStatus,
            ColThreshold,
            Col100PercentSetting
        };

        //Column definition for health states table
        public enum HealthStatesTableColumns
        {
            ColHealthLayer,
            ColDisplayStatus
        };

        // Default color scheme
        public static Color DefaultMetricBackColor = Color.Black;
        public static Color DefaultMetricColor = Color.LimeGreen;
        public static Color DefaultMetricGridLineColor = Color.YellowGreen;
        public static Color DefaultMetricCursorColor = Color.RoyalBlue;
        public static Color DefaultMetricThresholdColor = Color.Red;
        public static Color DefaultMetricMaxLineColor = Color.Lime;
        public static Color DefaultMetricMinLineColor = Color.LightCyan;
        public static Color DefaultMetricAverageLineColor = Color.LightSeaGreen;
        public const bool DefaultDisplayStatus = true;
        public const int DefaultCursorValue = 0;

        private static SystemMetricChartConfigModel _theInstance = new SystemMetricChartConfigModel();

        private Dictionary<string, DataTable> _theSystemMetricsColorTables = null;
        private Dictionary<string, DataTable> _theHealthStatesTables = null;
        private Dictionary<string, DataTable> _theDynamicSystemMetricsColorTables = null;
        private Dictionary<string, DataTable> _theDynamicSystemMetricTotalTables = null;

        #endregion Fields

        #region Properties

        /// <summary>
        /// Property: Instance - the singleton instance
        /// </summary>
        public static SystemMetricChartConfigModel Instance
        {
            get { return _theInstance; }
        }


        public Dictionary<string, DataTable> TheSystemMetricsColorTables
        {
            get { return _theSystemMetricsColorTables; }
            set { _theSystemMetricsColorTables = value; }
        }

        public Dictionary<string, DataTable> TheHealthStatesTables 
        {
            get { return _theHealthStatesTables; }
            set { _theHealthStatesTables = value; }
        }

        public Dictionary<string, DataTable> TheDynamicSystemMetricTotalTables
        {
            get { return _theDynamicSystemMetricTotalTables; }
            set { _theDynamicSystemMetricTotalTables = value; }
        }

        #endregion Properties

        #region Constructor

        /// <summary>
        /// The default constructor
        /// </summary>
        private SystemMetricChartConfigModel()
        {
            ConnectionDefinition.Changed +=  ConnectionDefinition_Changed;
        }

        ~SystemMetricChartConfigModel()
        {
            ConnectionDefinition.Changed -= ConnectionDefinition_Changed;
        }
        #endregion Constructor

        #region Public methods

        /// <summary>
        /// Get the back color of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricBackColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            //return (Color)dr[SystemMetricsColorTableColumns.ColMetricBackColor.ToString()];
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricBackColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the color of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            //return (Color)dr[SystemMetricsColorTableColumns.ColMetricColor.ToString()];
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the color of an average line 
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricAverageLineColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataTable chartColorTable = GetSystemMetricColorTable(aConnectionDefinitionName);
            DataRow dr = chartColorTable.Rows.Find(m);
            if (!dr.Table.Columns.Contains(SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString()))
            {
                chartColorTable.Columns.Add(SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString(), typeof(string));
                foreach (DataRow datarow in chartColorTable.Rows)
                {
                    datarow[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString()] = SystemMetricChartConfigModel.DefaultMetricAverageLineColor.Name;
                }
            }
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the color of a max line 
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricMaxLineColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataTable chartColorTable = GetSystemMetricColorTable(aConnectionDefinitionName);
            DataRow dr = chartColorTable.Rows.Find(m);
            if (!dr.Table.Columns.Contains(SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString()))
            {
                chartColorTable.Columns.Add(SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString(), typeof(string));
                foreach (DataRow datarow in chartColorTable.Rows)
                {
                    datarow[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString()] = SystemMetricChartConfigModel.DefaultMetricMaxLineColor.Name;
                }
            }
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the color of a min line 
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricMinLineColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataTable chartColorTable = GetSystemMetricColorTable(aConnectionDefinitionName);
            DataRow dr = chartColorTable.Rows.Find(m);
            if (!dr.Table.Columns.Contains(SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString()))
            {
                chartColorTable.Columns.Add(SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString(), typeof(string));
                foreach (DataRow datarow in chartColorTable.Rows)
                {
                    datarow[SystemMetricChartConfigModel.SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString()] = SystemMetricChartConfigModel.DefaultMetricMinLineColor.Name;
                }
            }
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the grid line color of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricGridLineColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            //return (Color)dr[SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()];
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the cursor color of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricCursorColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            //return (Color)dr[SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()];
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the threshold color of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public Color GetSystemMetricThresholdColor(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            //return (Color)dr[SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()];
            return Color.FromName(dr[SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()].ToString());
        }

        /// <summary>
        /// Get the display status of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public bool GetSystemMetricDisplayStatus(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            return (bool)dr[SystemMetricsColorTableColumns.ColDisplayStatus.ToString()];
        }

        /// <summary>
        /// Get the threshold value of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public int GetSystemMetricThreshold(string aConnectionDefinitionName, SystemMetricModel.SystemMetrics m)
        {
            DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            return (int)dr[SystemMetricsColorTableColumns.ColThreshold.ToString()];
        }

        /// <summary>
        /// Update a system metric's color scheme. 
        /// Note: You need to persist all of the changes after you have done all of the updates.
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="m"></param>
        /// <param name="aBackColor"></param>
        /// <param name="aColor"></param>
        /// <param name="aGridLineColor"></param>
        /// <param name="aCursorColor"></param>
        /// <param name="aThresholdColor"></param>
		/// <returns>true for changed,false for no any change</returns>
        public bool UpdateSystemMetricColors(DataTable aDataTable, SystemMetricModel.SystemMetrics m, Color aBackColor, Color aColor, Color aGridLineColor, 
            Color aCursorColor, Color aThresholdColor,Color aMaxLineColor,
            Color aMinLineColor,Color aAvgLineColor)
        {
            //DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            //dr[SystemMetricsColorTableColumns.ColMetricBackColor.ToString()] = aBackColor;
            //dr[SystemMetricsColorTableColumns.ColMetricColor.ToString()] = aColor;
            //dr[SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()] = aGridLineColor;
            //dr[SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()] = aCursorColor;
            //dr[SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()] = aThresholdColor;
            bool blnReturn = false;

            DataRow dr = aDataTable.Rows.Find(m);
            if (!dr[SystemMetricsColorTableColumns.ColMetricBackColor.ToString()].ToString().Equals(
                aBackColor.Name,StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricBackColor.ToString()] = aBackColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricColor.ToString()].ToString().Equals(
                aColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricColor.ToString()] = aColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()].ToString().Equals(
                aGridLineColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString()] = aGridLineColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()].ToString().Equals(
                aCursorColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricCursorColor.ToString()] = aCursorColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()].ToString().Equals(
                aThresholdColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString()] = aThresholdColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString()].ToString().Equals(
                aMaxLineColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString()] = aMaxLineColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString()].ToString().Equals(
                aMinLineColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString()] = aMinLineColor.Name;
                blnReturn = true;
            }

            if (!dr[SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString()].ToString().Equals(
                aAvgLineColor.Name, StringComparison.OrdinalIgnoreCase))
            {
                dr[SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString()] = aAvgLineColor.Name;
                blnReturn = true;
            }

            return blnReturn;
            
        }

        /// <summary>
        /// Update a system metric's display status. 
        /// Note: You need to persist all of the changes after you have done all of the updates.
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="m"></param>
        public void UpdateSystemMetricDisplayStatus(DataTable aDataTable, SystemMetricModel.SystemMetrics m, bool argDisplayStatus)
        {
            //DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            DataRow dr = aDataTable.Rows.Find(m);
            dr[SystemMetricsColorTableColumns.ColDisplayStatus.ToString()] = argDisplayStatus;            
            //Persistence.Put(SystemMetricColorTablePersistenceKey, _theSystemMetricsColorTables);
        }

        /// <summary>
        /// Update a system metric's threshold (using cursor value). 
        /// Note: You need to persist all of the changes after you have done all of the updates.
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="m"></param>
        public void UpdateSystemMetricThreshold(DataTable aDataTable, SystemMetricModel.SystemMetrics m, int argThreshold)
        {
            //DataRow dr = GetSystemMetricColorTable(aConnectionDefinitionName).Rows.Find(m);
            DataRow dr = aDataTable.Rows.Find(m);
            dr[SystemMetricsColorTableColumns.ColThreshold.ToString()] = argThreshold;
            //Persistence.Put(SystemMetricColorTablePersistenceKey, _theSystemMetricsColorTables);
        }

        /// <summary>
        /// To persist all of the above changes.
        /// </summary>
        public void PersistSystemMetricColorsAndHealthStates()
        {
            Persistence.Put(SystemMetricColorTablePersistenceKey, _theSystemMetricsColorTables);
            Persistence.Put(HealthStatesTablePersistentKey, _theHealthStatesTables);
        }
        /// <summary>
        /// To persist all of the above changes.
        /// </summary>
        public void PersistDynamicSystemMetricColors()
        {
            Persistence.Put(DynamicSystemMetricColorTablePersistenceKey, _theDynamicSystemMetricsColorTables);

        }

        /// <summary>
        /// Get the associate system metric color table for a system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <returns></returns>
        public DataTable GetSystemMetricColorTable(string aConnectionDefinitionName)
        {
            if (_theSystemMetricsColorTables == null)
            {
                _theSystemMetricsColorTables = Persistence.Get(SystemMetricColorTablePersistenceKey) as Dictionary<string, DataTable>;
                if (_theSystemMetricsColorTables == null)
                {
                    _theSystemMetricsColorTables = new Dictionary<string, DataTable>();
                }
            }

            DataTable table = ConstructorDefaultSystemMetricColorTable();            

            if (_theSystemMetricsColorTables.ContainsKey(aConnectionDefinitionName))
            {
                DataTable persistedTable = _theSystemMetricsColorTables[aConnectionDefinitionName];
                RecognizeNewMetric(table, persistedTable);
                Sort(persistedTable);
                return persistedTable;
            }
            else
            {
                _theSystemMetricsColorTables.Add(aConnectionDefinitionName, table);
                Sort(table);
                return table;
            }
        }

        private void RecognizeNewMetric(DataTable dtDefaultConfig, DataTable dtPersistedConfig)
        {
            foreach (DataRow dr in dtDefaultConfig.Rows)
            {
                SystemMetricModel.SystemMetrics metricKey = (SystemMetricModel.SystemMetrics)dr[0];
                DataRow[] searchRows = dtPersistedConfig.Select(string.Format("{0} = {1}", SystemMetricsColorTableColumns.ColMetric.ToString(), (int)metricKey));
                if (searchRows == null || searchRows.Length == 0)
                {
                    if (dtPersistedConfig.Rows.Count > 0)
                    {
                        DataRow sampleMetricRow = dtPersistedConfig.Rows[0];
                        DataRow newMetricRow = dtPersistedConfig.NewRow();
                        foreach (DataColumn configColumn in dtPersistedConfig.Columns)
                        {
                            if ( dtDefaultConfig.Columns.Contains(configColumn.ColumnName))
                            {
                                newMetricRow[configColumn.ColumnName] = dr[configColumn.ColumnName];
                            }
                            else
                            {
                                newMetricRow[configColumn.ColumnName] = sampleMetricRow[configColumn.ColumnName];
                            }
                        }

                        dtPersistedConfig.Rows.Add(newMetricRow);
                    }
                }
            }
        }

        /// <summary>
        /// Sort metric order based on requiement for newly-added metric, rather than the natural oder of enum.
        /// We can only append the newly-added metric to the end of enum, rather than inserting it to the middle,
        /// because this will mess up the persisted metric information,
        /// which use the emum number as the metric key, such as 0, 1, 2, 3, 4, 5 ...
        /// </summary>
        /// <param name="dtMetricConfig"></param>
        private void Sort(DataTable dtMetricConfig)
        {
            List<object[]> rowValues = new List<object[]>();
            foreach( SystemMetricModel.SystemMetrics metric in SystemMetricModel.OrderedSystemMetrics)
            {
                DataRow[] metricConfigRows = dtMetricConfig.Select(string.Format("{0} = {1}", SystemMetricsColorTableColumns.ColMetric.ToString(),(int)metric));
                if ( metricConfigRows != null && metricConfigRows.Length > 0 )
                {
                    rowValues.Add(metricConfigRows[0].ItemArray);
                }
            }

            dtMetricConfig.Rows.Clear();
            foreach (object[] cellValues in rowValues)
            {
                dtMetricConfig.Rows.Add(cellValues);
            }
        }
        
        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Event handler for connection defition changes
        /// </summary>
        /// <param name="aSender"></param>
        /// <param name="aConnectionDefinition"></param>
        /// <param name="aReason"></param>
        private void ConnectionDefinition_Changed(object aSender, ConnectionDefinition aConnectionDefinition, ConnectionDefinition.Reason aReason)
        {
            if (aReason == ConnectionDefinition.Reason.Removed)
            {
                if (_theSystemMetricsColorTables.ContainsKey(aConnectionDefinition.Name))
                {
                    // The system has been removed, remove the color table too.
                    _theSystemMetricsColorTables.Remove(aConnectionDefinition.Name);
                    Persistence.Put(SystemMetricColorTablePersistenceKey, _theSystemMetricsColorTables);
                    _theHealthStatesTables.Remove(aConnectionDefinition.Name);
                    Persistence.Put(HealthStatesTablePersistentKey, _theHealthStatesTables);
                }
            }
        }

        

        /// <summary>
        /// Construct the metric data table's column definitions.
        /// </summary>
        /// <param name="dt"></param>
        private void AddSystemMetricColorTableColumns(DataTable dt)
        {
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetric.ToString(), typeof(SystemMetricModel.SystemMetrics));
            
            //Change type from Color to string, because when write to persistent file, color object in datatable is not persistent
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricBackColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricGridLineColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricCursorColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricThresholdColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricMaxLineColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricMinLineColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColMetricAverageLineColor.ToString(), typeof(string));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColDisplayStatus.ToString(), typeof(bool));
            dt.Columns.Add(SystemMetricsColorTableColumns.ColThreshold.ToString(), typeof(int));

            DataColumn[] key = new DataColumn[1];
            key[0] = dt.Columns[SystemMetricsColorTableColumns.ColMetric.ToString()];
            dt.PrimaryKey = key;
        }      

        /// <summary>
        /// construct the default system metric color table
        /// </summary>
        private DataTable ConstructorDefaultSystemMetricColorTable()
        {
            DataTable colorTable = new DataTable();
            AddSystemMetricColorTableColumns(colorTable);

            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Core, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Memory, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Swap, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.File_System, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Load_Avg, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Network_Rcv, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Network_Txn, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Disk, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Tse, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            colorTable.Rows.Add(new object[] { SystemMetricModel.SystemMetrics.Virtual_Memory, DefaultMetricBackColor.Name, DefaultMetricColor.Name, DefaultMetricGridLineColor.Name, DefaultMetricCursorColor.Name, DefaultMetricThresholdColor.Name, DefaultMetricMaxLineColor.Name, DefaultMetricMinLineColor.Name, DefaultMetricAverageLineColor.Name, DefaultDisplayStatus, DefaultCursorValue });
            
            return colorTable;
        }

        /// <summary>
        /// construct the default HealthStates Table table
        /// </summary>
        private DataTable ConstructorDefaultHealthStatesTable()
        {
            DataTable healthStatesTable = new DataTable();
            //Add Columns
            healthStatesTable.Columns.Add(HealthStatesTableColumns.ColHealthLayer.ToString(), typeof(int));
            healthStatesTable.Columns.Add(HealthStatesTableColumns.ColDisplayStatus.ToString(), typeof(bool));

            DataColumn[] key = new DataColumn[1];
            key[0] = healthStatesTable.Columns[HealthStatesTableColumns.ColHealthLayer.ToString()];
            healthStatesTable.PrimaryKey = key;

            //Initialize values
            foreach (SystemMetricModel.HealthLayer healthLayer in Enum.GetValues(typeof(SystemMetricModel.HealthLayer))) 
            {
                healthStatesTable.Rows.Add(new object[]{healthLayer, true});                
            }

            return healthStatesTable;
        }

        /// <summary>
        /// Get the associate system metric color table for a system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <returns></returns>
        public DataTable GetHealthStatesTable(string aConnectionDefinitionName)
        {
            if (_theHealthStatesTables == null)
            {
                _theHealthStatesTables = Persistence.Get(HealthStatesTablePersistentKey) as Dictionary<string, DataTable>;
                if (_theHealthStatesTables == null)
                {
                    _theHealthStatesTables = new Dictionary<string, DataTable>();
                }
            }

            if (_theHealthStatesTables.ContainsKey(aConnectionDefinitionName))
            {
                return _theHealthStatesTables[aConnectionDefinitionName];
            }
            else
            {
                DataTable table = ConstructorDefaultHealthStatesTable();
                TheHealthStatesTables.Add(aConnectionDefinitionName, table);
                return table;
            }
        }


        /// <summary>
        /// Get the display status of a system metric for a specific system
        /// </summary>
        /// <param name="aConnectionDefinitionName"></param>
        /// <param name="m"></param>
        /// <returns></returns>
        public bool GetHealthStatesLayerDisplayStatus(string aConnectionDefinitionName, SystemMetricModel.HealthLayer h)
        {
            DataRow dr = GetHealthStatesTable(aConnectionDefinitionName).Rows.Find(h);
            return (bool)dr[HealthStatesTableColumns.ColDisplayStatus.ToString()];
        }


        /// <summary>
        /// Update a HealthStates Layer's display status.         
        /// </summary>
        /// <param name="aDataTable"></param>
        /// <param name="m"></param>
        public void UpdateHealthStatesLayerDisplayStatus(DataTable aDataTable, SystemMetricModel.HealthLayer h, bool argDisplayStatus)
        {
            //DataRow dr = GetHealthStatesTable(aConnectionDefinitionName).Rows.Find(h);
            DataRow dr = aDataTable.Rows.Find(h);
            dr[HealthStatesTableColumns.ColDisplayStatus.ToString()] = argDisplayStatus;            
        }

        #endregion Private methods

    }
}
