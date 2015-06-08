#region Copyright info
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
#endregion Copyright info
using System;
using System.Collections;
using System.Text;
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.UniversalWidget;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.WorkloadArea.Model
{
    public class TriageDataProvider : DatabaseDataProvider
    {
        public static int MAX_SQLTEXT_CHUNKS = 8;
        public static int SQLTEXT_CHUNK_SIZE = 2000;
        public const string SQL_TEXT = "SQL_TEXT";
        private bool _sessionData = false;
        
        #region Fields
        TriageFilterInfo _theTriageFilterModel;
        string _theFilterString = "(1 = 1)\n";
        #endregion Fields

        #region Properties

        public TriageFilterInfo TriageFilterModel
        {
            get { return _theTriageFilterModel; }
            set { _theTriageFilterModel = value; }
        }

        public string FilterString
        {
            get { return _theFilterString; }
            set { _theFilterString = value; }
        }

        public bool IsSessionData
        {
            get { return _sessionData; }
            set { _sessionData = value; }
        }

        #endregion Properties

        #region Constructors

        public TriageDataProvider(DataProviderConfig aDataProviderConfig)
            : base(aDataProviderConfig)
        {
        }

        #endregion Constructors
        public override void  DoPrefetchSetup(Hashtable predefinedParametersHash)
        {
            DatabaseDataProviderConfig dbConfig = DataProviderConfig as DatabaseDataProviderConfig;
            dbConfig.SQLText = getSQL(FilterString);
 	        base.DoPrefetchSetup(predefinedParametersHash);
        }

        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            base.DoFetchData(worker, e);
            //Save the formatted table. We will overwrite the data table
            _theDataTable = Trafodion.Manager.WorkloadArea.Controls.TriageGridDataDisplayHandler.GetProcessedDataTable(_theDataTable);
        }


        private string getSQL(string filterString)
        {
            StringBuilder sb = new StringBuilder();

            sb.Append((this.DataProviderConfig.MaxRowCount > 0) ? string.Format(" SELECT [First {0}] \n", this.DataProviderConfig.MaxRowCount) : " SELECT \n");
            sb.Append(" QSTATS.SESSION_ID                               as  SESSION_ID,    ");
            sb.Append(" QSTATS.QUERY_ID                                 as  QUERY_ID, ");
            sb.Append(" QSTATS.EXEC_START_UTC_TS                        as  START_TIME,    ");
            sb.Append(" QSTATS.EXEC_END_UTC_TS                          as  END_TIME, ");
            sb.Append(" QSTATS.USER_NAME                                as  USER_NAME,     ");
            sb.Append(" QSTATS.DISK_PROCESS_BUSY_TIME                   as  DISK_PROCESS_BUSY_TIME,  ");
            sb.Append(" QSTATS.MASTER_EXECUTION_TIME                    as  MASTER_EXECUTOR_TIME,  ");
            sb.Append(" QSTATS.ROLE_NAME                                as  ROLE_NAME,     ");
            sb.Append(" QSTATS.START_PRIORITY                           as  START_PRIORITY,     ");
            sb.Append(" QSTATS.MASTER_PROCESS_ID                        as  MASTER_PROCESS_ID,  ");
            sb.Append(" QSTATS.CLIENT_NAME                              as  CLIENT_ID,     ");
            sb.Append(" QSTATS.APPLICATION_NAME                         as  APPLICATION_NAME,     ");
            sb.Append(" QSTATS.COMPILE_START_UTC_TS                     as  COMPILATION_START_TIME, ");
            sb.Append(" QSTATS.COMPILE_END_UTC_TS                       as  COMPILATION_END_TIME,   ");
            sb.Append(" QSTATS.COMPILE_ELAPSED_TIME                     as  COMPILATION_TIME,   ");
            sb.Append(" QSTATS.CMP_AFFINITY_NUM                         as  CMP_AFFINITY_NUM,   ");
            sb.Append(" QSTATS.CMP_TXN_NEEDED                           as  CMP_TXN_NEEDED,     ");
            sb.Append(" QSTATS.CMP_MANDATORY_X_PROD                     as  COMPILATION_MANDATORY_CROSS_PRODUCT,  ");
            sb.Append(" QSTATS.CMP_MISSING_STATS                        as  CMP_MISSING_STATS,  ");
            sb.Append(" QSTATS.CMP_NUM_JOINS                            as  CMP_NUM_JOINS, ");
            sb.Append(" QSTATS.CMP_FULL_SCAN_ON_TABLE                   as  CMP_FULL_SCAN_ON_TABLE, ");
            sb.Append(" QSTATS.CMP_ROWS_ACCESSED_FULL_SCAN              as  CMP_ROWS_ACCESSED_FULL_SCAN, ");
            sb.Append(" QSTATS.EST_COST                                 as  ESTIMATED_COST,     ");
            sb.Append(" QSTATS.EST_CARDINALITY                          as  CARDINALITY_ESTIMATE,   ");
            sb.Append(" QSTATS.EST_ACCESSED_ROWS                        as  ROWS_ACCESS_ESTIMATE,   ");
            sb.Append(" QSTATS.EST_USED_ROWS                            as  ROWS_USAGE_ESTIMATE,    ");
            sb.Append(" QSTATS.EST_IO_TIME                              as  IO_TIME_ESTIMATE,   ");
            sb.Append(" QSTATS.EST_MSG_TIME                             as  MSG_TIME_ESTIMATE,  ");
            sb.Append(" QSTATS.EST_IDLE_TIME                            as  IDLE_TIME_ESTIMATE, ");
            sb.Append(" QSTATS.EST_TOTAL_TIME                           as  ESTIMATED_TOTAL_TIME,   ");
            sb.Append(" QSTATS.EST_TOTAL_MEM                            as  TOTAL_MEMORY_ESTIMATE,  ");
            sb.Append(" QSTATS.EST_RESOURCE_USAGE                       as  RESOURCE_USAGE_ESTIMATE,     ");
            sb.Append(" QSTATS.QUERY_STATUS                             as  QUERY_STATUS,  ");
            sb.Append(" QSTATS.QUERY_SUB_STATUS                         as  SUBSTATE, ");
            sb.Append(" QSTATS.QUERY_STATUS                               as  EXEC_STATE,    ");
            sb.Append(" QSTATS.STATS_ERROR_CODE                         as  STATS_ERROR_CODE,   ");
            sb.Append(" CAST(QSTATS.QUERY_ELAPSED_TIME/1000000          as DEC(18,2))   as QUERY_ELAPSED_TIME,    ");
            sb.Append(" QSTATS.SQL_PROCESS_BUSY_TIME                    as  SQL_CPU_TIME,  ");
            sb.Append(" QSTATS.DISK_PROCESS_BUSY_TIME/1000000           as  DISK_PROCESS_BUSYTIME_SEC,   ");
            sb.Append(" QSTATS.DISK_IOS                                 as  DISK_READS, ");
            sb.Append(" QSTATS.NUM_SQL_PROCESSES                        as  NUM_SQL_PROCESSES,  ");
            sb.Append(" QSTATS.SQL_SPACE_ALLOCATED                      as  SQL_SPACE_ALLOC,    ");
            sb.Append(" QSTATS.SQL_SPACE_USED                           as  SQL_SPACE_USED,     ");
            sb.Append(" QSTATS.SQL_HEAP_ALLOCATED                       as  SQL_HEAP_ALLOC,     ");
            sb.Append(" QSTATS.SQL_HEAP_USED                            as  SQL_HEAP_USED, ");
            sb.Append(" QSTATS.TOTAL_MEM_ALLOC                          as  TOTAL_MEMORY_ALLOCATED,  ");
            sb.Append(" QSTATS.MAX_MEM_USED                             as  MAX_MEMORY_USAGE,   ");
            sb.Append(" QSTATS.TRANSACTION_ID                           as  TRANSACTION_ID,     ");
            sb.Append(" QSTATS.NUM_REQUEST_MSGS                         as  NUM_SQL_REQUEST_MSGS,   ");
            sb.Append(" QSTATS.NUM_REQUEST_MSG_BYTES                    as  NUM_SQL_REQUEST_MSG_BYTES,   ");
            sb.Append(" QSTATS.NUM_REPLY_MSGS                           as  NUM_SQL_REPLY_MSGS, ");
            sb.Append(" QSTATS.NUM_REPLY_MSG_BYTES                      as  NUM_SQL_REPLY_MSG_BYTES,     ");
            sb.Append(" QSTATS.FIRST_RESULT_RETURN_UTC_TS               as  FETCH_START_TIME,   ");
            sb.Append(" QSTATS.ROWS_RETURNED_TO_MASTER                  as  NUM_ROWS_FETCHED,   ");
            sb.Append(" QSTATS.PARENT_QUERY_ID                          as  PARENT_QUERY_ID,    ");
            sb.Append(" CAST(QSTATS.MASTER_EXECUTION_TIME/1000000       as DEC(18,2))   as MASTER_EXECUTOR_TIME_SEC,  ");
            sb.Append(" CAST(QSTATS.ERROR_CODE     as VARCHAR(16) )     as  SQL_ERROR_CODE,     ");
            sb.Append(" QSTATS.MSG_BYTES_TO_DISK                        as  MESSAGE_BYTES_TO_DISK,  ");
            sb.Append(" QSTATS.MSGS_TO_DISK                             as  MESSAGES_TO_DISK,   ");
            sb.Append(" QSTATS.ROWS_ACCESSED                            as  ROWS_ACCESSED, ");
            sb.Append(" QSTATS.ROWS_RETRIEVED                           as  ROWS_RETRIEVED,     ");
            sb.Append(" QSTATS.NUM_ROWS_IUD                             as  NUM_ROWS_IUD,  ");
            sb.Append(" QSTATS.PROCESSES_CREATED                        as  PROCESSES_CREATED,  ");
            sb.Append(" CAST(QSTATS.PROCESS_CREATE_BUSY_TIME/1000000    as DEC(18,2))   as PROCESS_CREATION_TIME, ");
            sb.Append(" QUERY_TEXT as  SQL_TEXT, ");
            sb.Append(" QSTATS.PROCESS_ID                               as  PROCESS_ID,    ");
            sb.Append(" QSTATS.PROCESS_NAME                             as  PROCESS_NAME,  ");
            sb.Append(" JULIANTIMESTAMP(QSTATS.EXEC_START_UTC_TS)       as  ENTRY_ID, ");
            sb.Append(" ''                               as  ELAPSED_TIME,  ");
            sb.Append(" ''                                               as  STATE,  ");
            sb.Append(" CASE WHEN  TRIM(CAST(QSTATS.ERROR_CODE as VARCHAR(16) )) = ''  OR  TRIM(CAST(QSTATS.ERROR_CODE as VARCHAR(16) )) = '0'  THEN  ' '  ");
            sb.Append("    ELSE  TRIM(CAST(QSTATS.ERROR_CODE as VARCHAR(16) ))  ");
            sb.Append(" END  as  ERROR_CODE,  ");
            sb.Append(" ''                                               as  SQL_TYPE,  ");
            sb.Append(" QSTATS.NODE_ID                                   as  NODE_ID,   ");
            sb.Append(" current_timestamp                                 as  CURRENT_SYSTEM_TIME,  ");
            sb.Append(" QSTATS.AGGREGATE_OPTION                         as  AGGREGATE_OPTION,  ");
            sb.Append(" QSTATS.AGGREGATE_TOTAL                          as  AGGREGATE_TOTAL,  ");
            sb.Append(" QSTATS.CMP_DOP                                  as  COMPILATION_DOP,  ");
            sb.Append(" QSTATS.CMP_NUMBER_OF_BMOS                       as  CMP_NUMBER_OF_BMOS,  ");
            sb.Append(" QSTATS.CMP_OVERFLOW_MODE                        as  CMP_OVERFLOW_MODE,  ");
            sb.Append(" QSTATS.CMP_OVERFLOW_SIZE                        as  CMP_OVERFLOW_SIZE,  ");
            sb.Append(" QSTATS.DELAY_TIME_BEFORE_AQR_SEC                as  DELAY_TIME_BEFORE_AQR_SEC,  ");
            sb.Append(" QSTATS.ERROR_TEXT                               as  ERROR_TEXT,  ");
            sb.Append(" QSTATS.EST_CPU_TIME                             as  EST_CPU_TIME,  ");
            sb.Append(" QSTATS.INSTANCE_ID                              as  INSTANCE_ID,  ");
            sb.Append(" QSTATS.IP_ADDRESS_ID                            as  IP_ADDRESS_ID,  ");
            sb.Append(" QSTATS.LAST_ERROR_BEFORE_AQR                    as  LAST_ERROR_BEFORE_AQR,  ");
            sb.Append(" QSTATS.OVF_FILE_COUNT                           as  OVF_FILE_COUNT,  ");
            sb.Append(" QSTATS.OVF_SPACE_ALLOCATED                      as  OVF_SPACE_ALLOCATED,  ");
            sb.Append(" QSTATS.OVF_SPACE_USED                           as  OVF_SPACE_USED,  ");
            sb.Append(" QSTATS.OVF_BLOCK_SIZE                           as  OVF_BLOCK_SIZE,  ");
            sb.Append(" QSTATS.OVF_WRITE_READ_COUNT                     as  OVF_WRITE_READ_COUNT,  ");
            sb.Append(" QSTATS.OVF_WRITE_COUNT                          as  OVF_WRITE_COUNT,  ");
            sb.Append(" QSTATS.OVF_BUFFER_BLOCKS_WRITTEN                as  OVF_BUFFER_BLOCKS_WRITTEN,  ");
            sb.Append(" QSTATS.OVF_BUFFER_BYTES_WRITTEN                 as  OVF_BUFFER_BYTES_WRITTEN ,  ");
            sb.Append(" QSTATS.OVF_READ_COUNT                           as  OVF_READ_COUNT ,  ");
            sb.Append(" QSTATS.OVF_BUFFER_BLOCKS_READ                   as  OVF_BUFFER_BLOCKS_READ,  ");
            sb.Append(" QSTATS.OVF_BUFFER_BYTES_READ                    as  OVF_BUFFER_BYTES_READ,  ");
            sb.Append(" QSTATS.PNID_ID                                  as  PNID_ID,  ");
            sb.Append(" QSTATS.STATEMENT_ID                             as  STATEMENT_ID,  ");
            sb.Append(" QSTATS.STATEMENT_TYPE                           as  STATEMENT_TYPE,  ");
            sb.Append(" QSTATS.SUBMIT_UTC_TS                            as  SUBMIT_UTC_TS,  ");
            sb.Append(" QSTATS.TOTAL_NUM_AQR_RETRIES                    as  TOTAL_NUM_AQR_RETRIES ");
            sb.Append(" FROM \"_REPOS_\".METRIC_QUERY_TABLE QSTATS ");
            sb.Append(" WHERE 1=1 \n");
            if ((filterString != null) && (filterString.Trim().Length > 0))
            {
                sb.Append("     AND  " + filterString);
            }
            sb.Append(" ORDER BY QUERY_ID, START_TIME  \n");
			sb.Append(" FOR READ UNCOMMITTED ACCESS");

			return sb.ToString();

        }

        public void GetSessionStatements(String statementsFilter)
        {
            _theFilterString = statementsFilter;
            _sessionData = true;
            Start();
        }
    }
    [Serializable]
    public class TriageDataProviderConfig : DatabaseDataProviderConfig
    {
        public TriageDataProviderConfig()
        {
        }

        //[XmlIgnore]
        [NonSerialized]
        protected TriageDataProvider _theDataProvider = null;
        //[XmlIgnore]
        public override DataProviderTypes DataProviderType
        {
            get { return DataProviderTypes.DB; }
        }

        /// <summary>
        /// Returns a new DatabaseDataProvider using this config
        /// </summary>
        /// <returns></returns>
        public override DataProvider GetDataProvider()
        {
            if (_theDataProvider == null)
            {
                _theDataProvider = new TriageDataProvider(this);
            }
            return _theDataProvider;
        }

    }                                                                                                                  
}                                                                                                                 
 
