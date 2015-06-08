//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
using System.Drawing;
using System.Windows.Forms;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.UniversalWidget.Controls;
using Trafodion.Manager.WorkloadArea.Model;
using TenTec.Windows.iGridLib;
using System.Text;
using System.Text.RegularExpressions;
using System.Collections.Generic;

namespace Trafodion.Manager.WorkloadArea.Controls
{
    public partial class WorkloadHistoryControl : UserControl
    {
        string _queryText = "";
        ConnectionDefinition _connectionDefinition;
        string workloadHistoryPersistenceKey = "WorkloadHistoryPersistenceKey";
        private UniversalWidgetConfig _workloadHistoryConfig = null;
        private GenericUniversalWidget _workloadHistoryWidget = null;
        private TrafodionIGrid _workloadHistoryIGrid = null;
        HistoryDataDisplayHandler _dataDisplayHandler = null;
        Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider _dateFormatProvider = new Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider();

        public Trafodion.Manager.Framework.TrafodionLongDateTimeFormatProvider TheDateTimeFormatProvider
        {
            get { return _dateFormatProvider; }
        }

        public WorkloadHistoryControl()
        {
            InitializeComponent();
        }

        public WorkloadHistoryControl(ConnectionDefinition aConnectionDefinition, string queryText)
            :this()
        {
            _connectionDefinition = aConnectionDefinition;
            _queryText = queryText;
            _queryPreviewTextBox.Text = queryText;
            ShowWidget();
        }

        void ShowWidget()
        {
            //Create the configuration. If one is persisted, we use that otherwise we create one
            UniversalWidgetConfig tempConfig = WidgetRegistry.GetConfigFromPersistence(workloadHistoryPersistenceKey);

            //if (tempConfig == null)
            {
                _workloadHistoryConfig = WidgetRegistry.GetDefaultDBConfig();
                _workloadHistoryConfig.Name = workloadHistoryPersistenceKey;
                _workloadHistoryConfig.Title = "Workload History";
                DatabaseDataProviderConfig dbConfig = _workloadHistoryConfig.DataProviderConfig as DatabaseDataProviderConfig;
                dbConfig.SQLText = string.Format("SELECT * from {0} where sql_text like '%{1}%' order by gen_ts_lct desc for read uncommitted access",
                                                                                _connectionDefinition.MetricQueryViewFull, _queryText);
                dbConfig.TimerPaused = true;
                configureTriageDataProviderConfig(dbConfig);
            }
            //else
            //{
            //    _workloadHistoryConfig = tempConfig;
            //    //configureTriageDataProviderConfig(((DatabaseDataProviderConfig)tempConfig));
            //}


            //Set the connection definition if available
            _workloadHistoryConfig.DataProviderConfig.ConnectionDefinition = _connectionDefinition;
            _workloadHistoryConfig.ShowStatusStrip = UniversalWidgetConfig.ShowStatusStripEnum.ShowDuringOperation;
            _workloadHistoryConfig.ShowRefreshTimerButton = false;
            _workloadHistoryConfig.ShowTimerSetupButton = false;
            _workloadHistoryConfig.ShowExportButtons = true;
            _workloadHistoryConfig.ShowHelpButton = true;
            _workloadHistoryConfig.ShowRowCount = true;
            _workloadHistoryConfig.HelpTopic = HelpTopics.WorkloadHistory;

            DatabaseDataProviderConfig _dbConfig = _workloadHistoryConfig.DataProviderConfig as DatabaseDataProviderConfig;
            //Create a UW using the configuration             
            _workloadHistoryWidget = new GenericUniversalWidget();
            _workloadHistoryWidget.DataProvider = new DatabaseDataProvider(_dbConfig);
            _workloadHistoryWidget.DataProvider.OnBeforeFetchingData += new DataProvider.BeforeFetchingData(DataProvider_OnBeforeFetchingData);
            ((TabularDataDisplayControl)_workloadHistoryWidget.DataDisplayControl).LineCountFormat = "Queries";
            _dataDisplayHandler = new HistoryDataDisplayHandler(this);
            //_dataProvider.ColumnsToFilterOn = _dataDisplayHandler.Columns;
            _workloadHistoryWidget.DataDisplayControl.DataDisplayHandler = _dataDisplayHandler;
            _workloadHistoryWidget.UniversalWidgetConfiguration = _workloadHistoryConfig;

            //Add the widget to the canvas
            //GridConstraint gridConstraint = new GridConstraint(0, 0, 1, 1);
            //_theCanvas.AddWidget(MonitorWorkloadWidget, MonitorWorkloadConfig.Name, MonitorWorkloadConfig.Title, gridConstraint, -1);
            _workloadHistoryWidget.Dock = DockStyle.Fill;
            _widgetPanel.Controls.Add(_workloadHistoryWidget);
            _workloadHistoryWidget.StartDataProvider();
        }

        void configureTriageDataProviderConfig(DatabaseDataProviderConfig dbConfig)
        {
            List<string> defaultVisibleColumns = new List<string>();
            defaultVisibleColumns.Add("ELAPSED_TIME");
            defaultVisibleColumns.Add("ESTIMATED_COST");
            defaultVisibleColumns.Add("START_TIME");
            defaultVisibleColumns.Add("END_TIME");
            defaultVisibleColumns.Add("STATE");
            defaultVisibleColumns.Add("CLIENT_ID");
            defaultVisibleColumns.Add("DATASOURCE");
            defaultVisibleColumns.Add("USER_NAME");
            defaultVisibleColumns.Add("APPLICATION_NAME");
            defaultVisibleColumns.Add("MASTER_PROCESS_ID");
            defaultVisibleColumns.Add("LOCKWAITS");
            defaultVisibleColumns.Add("ROWS_ACCESSED");
            defaultVisibleColumns.Add("QUERY_ID");
            defaultVisibleColumns.Add("NUM_ROWS_IUD");
            defaultVisibleColumns.Add("ERROR_CODE");
            defaultVisibleColumns.Add("DISK_IOS");
            //defaultVisibleColumns.Add("LOCKESCALATIONS");
            defaultVisibleColumns.Add("MESSAGES_TO_DISK");
            defaultVisibleColumns.Add("MESSAGE_BYTES_TO_DISK");
            dbConfig.DefaultVisibleColumnNames = defaultVisibleColumns;

            List<ColumnMapping> columnMappings = new List<ColumnMapping>();
            columnMappings.Add(new ColumnMapping("ELAPSED_TIME", "Elapsed_Time", 80));
            columnMappings.Add(new ColumnMapping("ESTIMATED_COST", "Estimated_Cost", 80));
            columnMappings.Add(new ColumnMapping("START_TIME", "Start Time", 80));
            columnMappings.Add(new ColumnMapping("END_TIME", "End Time", 80));
            columnMappings.Add(new ColumnMapping("STATE", "State", 80));
            columnMappings.Add(new ColumnMapping("CLIENT_ID", "Client ID", 80));
            columnMappings.Add(new ColumnMapping("DATASOURCE", "DataSource", 80));
            columnMappings.Add(new ColumnMapping("USER_NAME", "User Name", 80));
            columnMappings.Add(new ColumnMapping("APPLICATION_NAME", "Application_Name", 80));
            columnMappings.Add(new ColumnMapping("MASTER_PROCESS_ID", "Master_Process ID", 80));
            columnMappings.Add(new ColumnMapping("LOCKWAITS", "Lock_Waits", 80));
            columnMappings.Add(new ColumnMapping("ROWS_ACCESSED", "Rows_Accessed", 80));
            columnMappings.Add(new ColumnMapping("QUERY_ID", "Query ID", 80));
            columnMappings.Add(new ColumnMapping("NUM_ROWS_IUD", "Num_Rows_IUD", 80));
            columnMappings.Add(new ColumnMapping("ERROR_CODE", "Error Code", 80));
            columnMappings.Add(new ColumnMapping("DISK_IOS", "Disk I/Os", 80));
            //columnMappings.Add(new ColumnMapping("LOCKESCALATIONS", "Lock_Escalations", 80));
            columnMappings.Add(new ColumnMapping("MESSAGES_TO_DISK", "Messages_To_Disk", 80));
            columnMappings.Add(new ColumnMapping("MESSAGE_BYTES_TO_DISK", "Message_Bytes_To_Disk", 80));
            dbConfig.ColumnMappings = columnMappings;
        }

        public void SetAvgMinMaxElapsedTime(double avgElapsedtime, double minElapsedTime, double maxElapsedTime)
        {
            _avgElapsedTimeTextBox.Text = WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(avgElapsedtime));
            _minElapsedTimeTextBox.Text = WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(minElapsedTime));
            _maxElapsedTimeTextBox.Text = WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(maxElapsedTime));
        }

        void DataProvider_OnBeforeFetchingData(object sender, DataProviderEventArgs e)
        {
            DatabaseDataProviderConfig dbConfig = (DatabaseDataProviderConfig)_workloadHistoryWidget.DataProvider.DataProviderConfig;
            StringBuilder sb = new StringBuilder();
            sb.Append((dbConfig.MaxRowCount > 0) ? string.Format(" SELECT [First {0}] \n", dbConfig.MaxRowCount) : " SELECT \n");
            sb.Append("     QSTATS.SESSION_ID                               as  SESSION_ID,                                   \n");
            sb.Append("     QSTATS.QUERY_ID                                 as  QUERY_ID,                                     \n");
            sb.Append("     QSTATS.EXEC_START_LCT_TS                        as  START_TIME,                                   \n");
            sb.Append("     QSTATS.EXEC_END_LCT_TS                          as  END_TIME,                                     \n");
            sb.Append("     QSTATS.USER_NAME                                as  USER_NAME,                                    \n");
            sb.Append("     QSTATS.MASTER_EXECUTION_TIME                    as  MASTER_EXECUTION_TIME,                        \n");
            sb.Append("     QSTATS.ROLE_NAME                                as  ROLE_NAME,                                    \n");
            sb.Append("     QSTATS.START_PRIORITY                           as  START_PRIORITY,                               \n");
            sb.Append("     QSTATS.MASTER_PROCESS_ID                        as  MASTER_PROCESS_ID,                            \n");
            sb.Append("     QSTATS.CLIENT_NAME                              as  CLIENT_ID,                                    \n");
            sb.Append("     QSTATS.APPLICATION_NAME                         as  APPLICATION_NAME,                             \n");
            sb.Append("     QSTATS.DATASOURCE                               as  DATASOURCE,                                   \n");
            sb.Append("     QSTATS.SERVICE_NAME                             as  SERVICE_NAME,                                 \n");
            sb.Append("     QSTATS.COMPILE_START_LCT_TS                     as  COMPILATION_START_TIME,                       \n");
            sb.Append("     QSTATS.COMPILE_END_LCT_TS                       as  COMPILATION_END_TIME,                         \n");
            sb.Append("     QSTATS.COMPILE_ELAPSED_TIME                     as  COMPILATION_TIME,                             \n");
            sb.Append("     QSTATS.CMP_AFFINITY_NUM                         as  CMP_AFFINITY_NUM,                             \n");
            sb.Append("     QSTATS.CMP_TXN_NEEDED                           as  CMP_TXN_NEEDED,                               \n");
            sb.Append("     QSTATS.CMP_MANDATORY_X_PROD                     as  COMPILATION_MANDATORY_CROSS_PRODUCT,          \n");  
            sb.Append("     QSTATS.CMP_MISSING_STATS                        as  CMP_MISSING_STATS,                            \n");
            sb.Append("     QSTATS.CMP_NUM_JOINS                            as  CMP_NUM_JOINS,                                \n");
            sb.Append("     QSTATS.CMP_FULL_SCAN_ON_TABLE                   as  CMP_FULL_SCAN_ON_TABLE,                       \n");
            sb.Append("     QSTATS.CMP_ROWS_ACCESSED_FULL_SCAN              as  CMP_ROWS_ACCESSED_FULL_SCAN,                  \n");
            sb.Append("     QSTATS.CMP_EID_ROWS_ACCESSED                    as  COMPILATION_DISK_PROCESS_ROWS_ACCESSED,       \n");     
            sb.Append("     QSTATS.CMP_EID_ROWS_USED                        as  COMPILATION_DISK_PROCESS_ROWS_USED,           \n"); 
            sb.Append("     QSTATS.EST_COST                                 as  ESTIMATED_COST,                               \n");
            sb.Append("     QSTATS.EST_CARDINALITY                          as  CARDINALITY_ESTIMATE,                         \n");
            sb.Append("     QSTATS.EST_ACCESSED_ROWS                        as  ROWS_ACCESS_ESTIMATE,                         \n");
            sb.Append("     QSTATS.EST_USED_ROWS                            as  ROWS_USAGE_ESTIMATE,                          \n");
            sb.Append("     QSTATS.EST_IO_TIME                              as  IO_TIME_ESTIMATE,                             \n");
            sb.Append("     QSTATS.EST_MSG_TIME                             as  MSG_TIME_ESTIMATE,                            \n");     
            sb.Append("     QSTATS.EST_IDLE_TIME                            as  IDLE_TIME_ESTIMATE,                           \n");
            sb.Append("     QSTATS.EST_TOTAL_TIME                           as  ESTIMATED_TOTAL_TIME,                         \n");
            sb.Append("     QSTATS.EST_TOTAL_MEM                            as  TOTAL_MEMORY_ESTIMATE,                        \n");
            sb.Append("     QSTATS.EST_RESOURCE_USAGE                       as  RESOURCE_USAGE_ESTIMATE,                      \n");
            sb.Append("     QSTATS.QUERY_STATUS                             as  STATE,                                        \n");
            sb.Append("     QSTATS.QUERY_SUB_STATUS                         as  SUBSTATE,                                     \n");
            sb.Append("     QSTATS.EXEC_STATE                               as  EXEC_STATE,                                   \n");
            sb.Append("     QSTATS.WARN_LEVEL                               as  WARNING_LEVEL,                                \n");
            sb.Append("     QSTATS.STATS_ERROR_CODE                         as  STATS_ERROR_CODE,                             \n");
            sb.Append("     QSTATS.WAIT_TIME                                as  WAIT_TIME,                                    \n");
            sb.Append("     QSTATS.HOLD_TIME                                as  HOLD_TIME,                                    \n");
            sb.Append("     QSTATS.QUERY_ELAPSED_TIME                       as  ELAPSED_TIME,                                 \n");
            sb.Append("     QSTATS.SQL_PROCESS_BUSY_TIME                    as  SQL_CPU_TIME,                                 \n");
            sb.Append("     QSTATS.DISK_PROCESS_BUSY_TIME                   as  DISK_PROCESS_BUSY_TIME,                       \n");
            sb.Append("     QSTATS.DISK_IOS                                 as  DISK_IOS,                                     \n");
            sb.Append("     QSTATS.NUM_SQL_PROCESSES                        as  NUM_SQL_PROCESSES,                            \n");
            sb.Append("     QSTATS.SQL_SPACE_ALLOCATED                      as  SQL_SPACE_ALLOC,                              \n");
            sb.Append("     QSTATS.SQL_SPACE_USED                           as  SQL_SPACE_USED,                               \n");
            sb.Append("     QSTATS.SQL_HEAP_ALLOCATED                       as  SQL_HEAP_ALLOC,                               \n");
            sb.Append("     QSTATS.SQL_HEAP_USED                            as  SQL_HEAP_USED,                                \n");
            sb.Append("     QSTATS.EID_SPACE_ALLOCATED                      as  EID_SPACE_ALLOC,                              \n");
            sb.Append("     QSTATS.EID_SPACE_USED                           as  EID_SPACE_USED,                               \n");
            sb.Append("     QSTATS.EID_HEAP_ALLOCATED                       as  EID_HEAP_ALLOC,                               \n");
            sb.Append("     QSTATS.EID_HEAP_USED                            as  EID_HEAP_USED,                                \n");
            sb.Append("     QSTATS.TOTAL_MEM_ALLOC                          as  TOTAL_MEMORY_ALLOCATED,                       \n");
            sb.Append("     QSTATS.MAX_MEM_USED                             as  MAX_MEMORY_USAGE,                             \n");
            sb.Append("     QSTATS.TRANSACTION_ID                           as  TRANSACTION_ID,                               \n");
            sb.Append("     QSTATS.NUM_REQUEST_MSGS                         as  NUM_SQL_REQUEST_MSGS,                         \n");
            sb.Append("     QSTATS.NUM_REQUEST_MSG_BYTES                    as  NUM_SQL_REQUEST_MSG_BYTES,                    \n");
            sb.Append("     QSTATS.NUM_REPLY_MSGS                           as  NUM_SQL_REPLY_MSGS,                           \n");
            sb.Append("     QSTATS.NUM_REPLY_MSG_BYTES                      as  NUM_SQL_REPLY_MSG_BYTES,                      \n");
            sb.Append("     QSTATS.FIRST_RESULT_RETURN_LCT_TS               as  FETCH_START_TIME,                             \n");
            sb.Append("     QSTATS.ROWS_RETURNED_TO_MASTER                  as  NUM_ROWS_FETCHED,                             \n");
            sb.Append("     QSTATS.PARENT_QUERY_ID                          as  PARENT_QUERY_ID,                              \n");
            sb.Append("     CAST(QSTATS.ERROR_CODE     as VARCHAR(16) )     as  SQL_ERROR_CODE,                               \n");
            //sb.Append("     QSTATS.LOCK_ESCALATIONS                         as  LOCKESCALATIONS,                              \n");
            sb.Append("     QSTATS.LOCK_WAITS                               as  LOCKWAITS,                                    \n");
            sb.Append("     QSTATS.MSG_BYTES_TO_DISK                        as  MESSAGE_BYTES_TO_DISK,                        \n");
            sb.Append("     QSTATS.MSGS_TO_DISK                             as  MESSAGES_TO_DISK,                             \n");
            sb.Append("     QSTATS.ROWS_ACCESSED                            as  ROWS_ACCESSED,                                \n");
            sb.Append("     QSTATS.ROWS_RETRIEVED                           as  ROWS_RETRIEVED,                               \n");
            sb.Append("     QSTATS.NUM_ROWS_IUD                             as  NUM_ROWS_IUD,                                 \n");
            sb.Append("     QSTATS.NUM_OPENS                                as  NUM_OPEN_FILES,                               \n");
            sb.Append("     CAST(QSTATS.OPEN_BUSY_TIME/1000000 as DEC(18,2)) as  OPEN_BUSY_TIME,                              \n");
            sb.Append("     QSTATS.PROCESSES_CREATED                        as  PROCESSES_CREATED,                            \n");
            sb.Append("     CAST(QSTATS.PROCESS_CREATE_BUSY_TIME/1000000    as DEC(18,2))   as PROCESS_CREATION_TIME,         \n");   
            sb.Append("     QSTATS.PROCESS_ID                               as  PROCESS_ID,                                   \n");
            sb.Append("     QSTATS.PROCESS_NAME                             as  PROCESS_NAME,                                 \n");
            sb.Append("     ISNULL(QSTATS.EXEC_START_LCT_TS, QSTATS.ENTRY_ID_LCT_TS) as ENTRY_TIME,                           \n");
            sb.Append("     CASE WHEN  TRIM(CAST(QSTATS.ERROR_CODE as VARCHAR(16) )) = ''  OR  TRIM(CAST(QSTATS.ERROR_CODE as VARCHAR(16) )) = '0'  THEN  ' ' \n");
            sb.Append("        ELSE  TRIM(CAST(QSTATS.ERROR_CODE as VARCHAR(16) )) \n");
            sb.Append("     END \n");
            sb.Append("                                                     as  ERROR_CODE, \n");
            sb.Append("     QSTATS.SQL_TEXT                                 as  SQL_TEXT, \n");
            sb.Append("     QSTATS.NODE_ID                                  as  NODE_ID,  \n");
            sb.Append("     current_timestamp                               as  CURRENT_SYSTEM_TIME, \n");
            sb.Append("     QSTATS.AGGREGATE_OPTION                         as  AGGREGATE_OPTION, \n");
            sb.Append("     QSTATS.AGGREGATE_TOTAL                          as  AGGREGATE_TOTAL, \n");
            sb.Append("     QSTATS.CMP_DOP                                  as  COMPILATION_DOP, \n");
            sb.Append("     QSTATS.CMP_HIGH_EID_MAX_BUF_USAGE               as  CMP_HIGH_EID_MAX_BUF_USAGE, \n");
            sb.Append("     QSTATS.CMP_NUMBER_OF_BMOS                       as  CMP_NUMBER_OF_BMOS, \n");
            sb.Append("     QSTATS.CMP_OVERFLOW_MODE                        as  CMP_OVERFLOW_MODE, \n");
            sb.Append("     QSTATS.CMP_OVERFLOW_SIZE                        as  CMP_OVERFLOW_SIZE, \n");
            sb.Append("     QSTATS.COMPILATION_RULE                         as  COMPILATION_RULE, \n");
            sb.Append("     QSTATS.COMPILE_START_UTC_TS                     as  COMPILE_START_UTC_TS, \n");
            sb.Append("     QSTATS.COMPILE_END_UTC_TS                       as  COMPILE_END_UTC_TS, \n");
            sb.Append("     QSTATS.CONNECTION_RULE                          as  CONNECTION_RULE, \n");
            sb.Append("     QSTATS.DELAY_TIME_BEFORE_AQR_SEC                as  DELAY_TIME_BEFORE_AQR_SEC, \n");
            sb.Append("     QSTATS.ENTRY_ID_LCT_TS                          as  ENTRY_ID_LCT_TS, \n");
            sb.Append("     QSTATS.ERROR_TEXT                               as  ERROR_TEXT, \n");
            sb.Append("     QSTATS.EST_CPU_TIME                             as  EST_CPU_TIME, \n");
            sb.Append("     QSTATS.EXEC_START_UTC_TS                        as  EXEC_START_UTC_TS, \n");
            sb.Append("     QSTATS.EXEC_END_UTC_TS                          as  EXEC_END_UTC_TS, \n");
            sb.Append("     QSTATS.EXECUTION_RULE                           as  EXECUTION_RULE, \n");
            sb.Append("     QSTATS.FIRST_RESULT_RETURN_UTC_TS               as  FIRST_RESULT_RETURN_UTC_TS, \n");
            sb.Append("     QSTATS.INSTANCE_ID                              as  INSTANCE_ID, \n");
            sb.Append("     QSTATS.IP_ADDRESS_ID                            as  IP_ADDRESS_ID, \n");
            sb.Append("     QSTATS.LAST_ERROR_BEFORE_AQR                    as  LAST_ERROR_BEFORE_AQR, \n");
            sb.Append("     QSTATS.OVF_FILE_COUNT                           as  OVF_FILE_COUNT, \n");
            sb.Append("     QSTATS.OVF_SPACE_ALLOCATED                      as  OVF_SPACE_ALLOCATED, \n");
            sb.Append("     QSTATS.OVF_SPACE_USED                           as  OVF_SPACE_USED, \n");
            sb.Append("     QSTATS.OVF_BLOCK_SIZE                           as  OVF_BLOCK_SIZE, \n");
            sb.Append("     QSTATS.OVF_WRITE_READ_COUNT                     as  OVF_WRITE_READ_COUNT, \n");
            sb.Append("     QSTATS.OVF_WRITE_COUNT                          as  OVF_WRITE_COUNT, \n"); 
            sb.Append("     QSTATS.OVF_BUFFER_BLOCKS_WRITTEN                as  OVF_BUFFER_BLOCKS_WRITTEN, \n");
            sb.Append("     QSTATS.OVF_BUFFER_BYTES_WRITTEN                 as  OVF_BUFFER_BYTES_WRITTEN , \n");
            sb.Append("     QSTATS.OVF_READ_COUNT                           as  OVF_READ_COUNT , \n");
            sb.Append("     QSTATS.OVF_BUFFER_BLOCKS_READ                   as  OVF_BUFFER_BLOCKS_READ, \n");
            sb.Append("     QSTATS.OVF_BUFFER_BYTES_READ                    as  OVF_BUFFER_BYTES_READ, \n");
            sb.Append("     QSTATS.PNID_ID                                  as  PNID_ID, \n");
            sb.Append("     QSTATS.STATEMENT_ID                             as  STATEMENT_ID, \n");
            sb.Append("     QSTATS.STATEMENT_TYPE                           as  STATEMENT_TYPE, \n");
            sb.Append("     QSTATS.SUBMIT_UTC_TS                            as  SUBMIT_UTC_TS, \n");
            sb.Append("     QSTATS.SUBMIT_LCT_TS                            as  SUBMIT_LCT_TS, \n");
            sb.Append("     QSTATS.TOTAL_NUM_AQR_RETRIES                    as  TOTAL_NUM_AQR_RETRIES, \n");
            sb.Append("     QSTATS.NUM_NODES, \n");
            sb.Append("     QSTATS.PARENT_SYSTEM_NAME, \n");
            sb.Append("     QSTATS.PERTABLE_STATS, \n");
            sb.Append("     QSTATS.STATEMENT_SUBTYPE, \n");
            sb.Append("     QSTATS.UDR_PROCESS_BUSY_TIME \n");
            sb.Append(" FROM {0} QSTATS \n");
            sb.Append(" WHERE  QSTATS.SQL_TEXT LIKE '%{1}%'      \n");
            sb.Append(" ORDER BY START_TIME DESC \n");
			sb.Append(" FOR READ UNCOMMITTED ACCESS");

			string sqlText = sb.ToString();
            string previewText = _queryPreviewTextBox.Text.Replace("'", "''");
            dbConfig.SQLText = string.Format(sqlText, dbConfig.ConnectionDefinition.MetricQueryViewFull, previewText);
        }
    }

    public class HistoryDataDisplayHandler : TabularDataDisplayHandler
    {
        WorkloadHistoryControl _workloadHistoryControl = null;
        public HistoryDataDisplayHandler(WorkloadHistoryControl aWorkloadHistoryControl)
        {
            _workloadHistoryControl = aWorkloadHistoryControl;
        }

        public override void DoPopulate(UniversalWidgetConfig aConfig, System.Data.DataTable aDataTable,
                                                Trafodion.Manager.Framework.Controls.TrafodionIGrid aDataGrid)
        {
            base.DoPopulate(aConfig, aDataTable, aDataGrid);
            FormatIGridDatetimeColumns(aDataGrid);
            setupQueriesInIGrid(aDataGrid);
            string gridHeaderText = string.Format("There are {0} occurrences of the query", aDataGrid.Rows.Count);
            aDataGrid.UpdateCountControlText(gridHeaderText);
        }

        private void FormatIGridDatetimeColumns(TrafodionIGrid aDataGrid)
        {

            foreach (iGCol column in aDataGrid.Cols)
            {

                if (column.CellStyle.ValueType == typeof(System.DateTime))
                {
                    //Display datetime using TrafodionManager standard datetime format                   
                    column.CellStyle.FormatProvider = _workloadHistoryControl.TheDateTimeFormatProvider;
                    column.CellStyle.FormatString = "{0}";
                }
            }
        }

        private bool ParseNulltoZero(object time, out string outElapsedTime)
        {
            if (time == null || time.ToString().Trim().Equals(string.Empty))
            {
                outElapsedTime= "0";
                return true;
            }
            else
            {
                outElapsedTime = time.ToString();
                return false;
            }
        }

        private void setupQueriesInIGrid(iGrid aDataGrid)
        {
            MonitorWorkloadOptions options = MonitorWorkloadOptions.GetOptions();
            setIGridColumnFormatTypes(aDataGrid);
            double minElapsedTime = 0;
            double maxElaspsedTime = 0;
            double totalElapsedTime = 0;
            string elapsedTime = string.Empty;
            bool isEmptyElapsedTime = false;
            if (aDataGrid.Rows.Count > 0)
            {
                isEmptyElapsedTime = ParseNulltoZero(aDataGrid.Rows[0].Cells["ELAPSED_TIME"].Value, out elapsedTime);
                minElapsedTime = maxElaspsedTime = Int64.Parse(elapsedTime) / 1000000; ;
            }

            foreach (iGRow row in aDataGrid.Rows)
            {
                try
                {
                    String errorCode = "";
                    try
                    {
                        isEmptyElapsedTime = ParseNulltoZero(row.Cells["ELAPSED_TIME"].Value, out elapsedTime);
                        double queryElapsedTime = Int64.Parse(elapsedTime) / 1000000;
                        if (queryElapsedTime < minElapsedTime)
                        {
                            minElapsedTime = queryElapsedTime;
                        }

                        if(queryElapsedTime > maxElaspsedTime)
                        {
                            maxElaspsedTime = queryElapsedTime;
                        }

                        totalElapsedTime += queryElapsedTime;

                        row.Cells["ELAPSED_TIME"].Value = isEmptyElapsedTime ? string.Empty : WMSUtils.getFormattedElapsedTime(TimeSpan.FromSeconds(queryElapsedTime));

                        errorCode = row.Cells["ERROR_CODE"].Value.ToString();
                        if (queryHadErrors(errorCode)) 
                            row.Cells["STATE"].Value = "Error";
                    }
                    catch (Exception ex)
                    {
                    }

                    if (queryHadErrors(errorCode))
                        ChangeDataRowColor(row, options.RejectedColor);
                    else
                    {
                        String qryState = row.Cells["STATE"].Value.ToString().Trim().ToUpper();
                        if (TriageGridUserControl.RUNNING.Equals(qryState))
                            ChangeDataRowColor(row, options.ExecutingColor);
                        else if ("ABNORMALLY TERMINATED".Equals(qryState))
                            ChangeDataRowColor(row, options.RejectedColor);
                        else if (TriageGridUserControl.HOLD.Equals(qryState) || TriageGridUserControl.SUSPENDED.Equals(qryState))
                            ChangeDataRowColor(row, options.HoldingColor);
                        else if (TriageGridUserControl.HOLD.Equals(qryState) || TriageGridUserControl.COMPLETED.Equals(qryState))
                            ChangeDataRowColor(row, options.CompletedColor);

                    }
                }
                catch (Exception)
                {
                }
            }

            if (aDataGrid.Rows.Count > 0)
            {
                _workloadHistoryControl.SetAvgMinMaxElapsedTime(totalElapsedTime / aDataGrid.Rows.Count, minElapsedTime, maxElaspsedTime);
            }
            else
            {
                _workloadHistoryControl.SetAvgMinMaxElapsedTime(0, 0, 0);
            }

        }

        public static void ChangeDataRowBackColor(iGRow row, Color c)
        {
            try
            {
                int len = row.Cells.Count;
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].BackColor = c;
            }
            catch (Exception)
            {
            }
        }
        public static void ChangeDataRowColor(iGRow row, Color c)
        {
            int len = row.Cells.Count;

            try
            {
                for (int idx = 0; idx < len; idx++)
                    row.Cells[idx].ForeColor = c;

            }
            catch (Exception)
            {
            }

        }

        private static bool queryHadErrors(String errorCode)
        {
            if (0 >= errorCode.Length)
                return false;

            try
            {
                int err = int.Parse(errorCode);
                if (0 != err)
                    return true;

            }
            catch (Exception)
            {
            }

            return false;
        }

        private void setupRowColors()
        {

        }

        private void setIGridColumnFormatTypes(iGrid TriageIGridControl)
        {
            String[] numericColsList = {"EST_CPU_TIME","START_PRIORITY", "ELAPSED_TIME", "MASTER_EXECUTOR_TIME", 
										"LOCKESCALATIONS", "LOCKWAITS", "MESSAGE_BYTES_TO_DISK", 
									    "MESSAGES_TO_DISK", "ROWS_ACCESSED", "ROWS_RETRIEVED", "NUM_ROWS_IUD", 
										 "CARDINALITY_ESTIMATE", "USED_ROWS", 
										 "ROWS_ACCESS_ESTIMATE", "ROWS_USAGE_ESTIMATE", "TOTAL_PROCESSOR_TIME",
										 "DELTA_PROCESSOR_TIME", "DELTA_SPACE_ALLOC_MB", "DELTA_SPACE_USED_MB",
									     "DELTA_DISK_IOS", "DELTA_IUD",
										 "RESOURCE_USAGE_ESTIMATE", "CMP_AFFINITY_NUM", "CMP_DOP",
									     "CMP_TXN_NEEDED", "CMP_MANDATORY_X_PROD", "CMP_MISSING_STATS",
										 "CMP_NUM_JOINS", "CMP_FULL_SCAN_ON_TABLE", "COMPILATION_HIGH_ESAM_MAX_BUFFER_USAGE",
										 "TOTAL_MEMORY_ALLOCATED", "TOTAL_SPACE_ALLOC_KB", "TOTAL_SPACE_USED_KB",
                                         "STATS_BYTES", "DISK_IOS", "LOCK_WAITS", "LOCK_ESCALATIONS", "OPENS",
										 "PROCESSES_CREATED", "DISK_PROCESS_BUSYTIME", "SQL_CPU_TIME", 
										 "PROCESS_CREATE_TIME", "OPEN_TIME", 
										 "SQL_SPACE_ALLOC", "SQL_SPACE_USED", "SQL_HEAP_ALLOC", "SQL_HEAP_USED",
										 "EID_SPACE_ALLOC", "EID_SPACE_USED", "EID_HEAP_ALLOC", "EID_HEAP_USED",
										 "TOTAL_PROCESSOR_TIME_MICROSECONDS", "CMP_ROWS_ACCESSED_FULL_SCAN", "CMP_EID_ROWS_ACCESSED",
                                         "CMP_EID_ROWS_USED"
                                       };


            Hashtable formatColsHT = new Hashtable();
            String integerValueNumberFormat = TriageHelper.getNumberFormatForCurrentLocale(0);

            foreach (String colName in numericColsList)
                formatColsHT.Add(colName, integerValueNumberFormat);

            String precision2NumberFormat = TriageHelper.getLocaleNumberFormat(2, true);
            formatColsHT.Add("TOTAL_MEMORY_ESTIMATE", precision2NumberFormat);
            formatColsHT.Add("ROWS/SECOND", precision2NumberFormat);
            formatColsHT.Add("IUD/SECOND", precision2NumberFormat);
            formatColsHT.Add("ESTIMATED_COST", precision2NumberFormat);
            formatColsHT.Add("CPU_TIME_ESTIMATE", precision2NumberFormat);
            formatColsHT.Add("IO_TIME_ESTIMATE", precision2NumberFormat);
            formatColsHT.Add("MSG_TIME_ESTIMATE", precision2NumberFormat);
            formatColsHT.Add("IDLE_TIME_ESTIMATE", precision2NumberFormat);
            formatColsHT.Add("ESTIMATED_TOTAL_TIME", precision2NumberFormat);
            formatColsHT.Add("PROCESSOR_USAGE/SECOND", precision2NumberFormat);
            formatColsHT.Add("SPACE_ALLOCATION_GB", precision2NumberFormat);
            formatColsHT.Add("SPACE_USAGE_GB", precision2NumberFormat);

            foreach (iGCol col in TriageIGridControl.Cols)
            {
                String name = col.Key;
                if (formatColsHT.ContainsKey(name))
                {
                    String fmt = (String)formatColsHT[name];
                    if ((null != fmt) && (0 < fmt.Trim().Length))
                        col.CellStyle.FormatString = fmt;

                }
            }

        }

    }
}
