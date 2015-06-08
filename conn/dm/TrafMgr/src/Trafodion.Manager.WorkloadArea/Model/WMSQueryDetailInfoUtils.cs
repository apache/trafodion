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
using System.Globalization;
using Trafodion.Manager.WorkloadArea.Controls;

namespace Trafodion.Manager.WorkloadArea.Model
{
    /// <summary>
    /// Static classes for defining the metrics.
    /// </summary>
    public class WMSQueryDetailInfoUtils
    {
        #region Fields

        public delegate object CalcRoutine(DataRow dr);
        public delegate object CalcRoutineWithOldRow(DataRow newRow, DataRow oldRow);
        public const string MetricVersion = "V03";

        private const string NoFormat = "";

        /// <summary>
        /// Types of Metrics
        /// </summary>
        public enum Metrics
        {
            TimeBased = 0, 
            Connection, 
            CompileTime, 
            RunTime
        }

        /// <summary>
        /// Data Columns for the metric data table.
        /// </summary>
        public enum DetailColumns
        {
            id = 0,
            display,
            column,
            datatype,
            method,
            call, 
            format
        }

        /// <summary>
        /// Data value acquization method.
        /// </summary>
        public enum GetMethod
        {
            DataTable = 0, 
            Calc1,
            Calc2,
            WMSTimestamp, 
            WMSTimeInt, 
            WMSDouble, 
            WMSTimeLong,
            WMSTimeSecs
        }

        /// <summary>
        /// Time-Based Metrics
        /// NOTE: Please be sure to use the defined column name as the enum. 
        /// NOTE: Remember to modify the Construct Metrics DataTable whenever this definiton is modified. 
        /// NOTE: Remember to update metric version.
        /// </summary>
        public enum TimeBasedMetrics
        {
            QUERY_START_TIME = 0,
            START_TS, 
            CALC_END_TS, 
            ELAPSED_TIME,
            WAIT_TIME,
            HOLD_TIME,
            CALC_TOTAL_QUERY_TIME,
            SQL_CPU_TIME,
            PROCESS_BUSYTIME,
            CALC_TOTAL_CPU_TIME,
            CALC_TOTAL_CPU_TIME_IN_MICRO,
            PROCESS_CREATE_TIME,
            OPEN_TIME,
            COMP_START_TIME,
            COMP_END_TIME,
            CMP_LAST_UPDATED,
            EXEC_START_TIME,
            EXEC_END_TIME,
            FIRST_ROW_RETURNED_TIME,
            EXEC_LAST_UPDATED,
            //CALC_QUERY_ELAPSED_TIME,
            ENTRY_TS,
            ENTRY_JTS,
            SUSPENDED_TS,
            RELEASED_TS,
            CANCELLED_TS,
            CALC_LAST_INTERVAL_CPU_TIME,
            CALC_PROCESSOR_USAGE,
            CALC_DELTA_PROCESSOR_TIME,
            CALC_ROWS_PER_SEC,
            CALC_IUD_PER_SEC,
            UDR_PROCESS_BUSY_TIME,
            SQL_CPU_OFFENDER_INTERVAL_SECS,
            SQL_TSE_OFFENDER_INTERVAL_SECS,
            SQL_SLOW_OFFENDER_INTERVAL_SECS            
        }

        private static DataTable TimeBasedDetails = new DataTable();

        /// <summary>
        /// Connection Metrics
        /// NOTE: Please be sure to use the defined column name as the enum.
        /// NOTE: Remember to modify the Construct Metrics DataTable whenever this definiton is modified. 
        /// NOTE: Remember to update metric version.
        /// </summary>
        public enum ConnectionFacts
        {
                QUERY_ID = 0, 
                QUERY_STATE,
                QUERY_SUBSTATE,
                APPLICATION_NAME,
                COMPUTER_NAME,
                ROLE_NAME,
                USER_NAME,
                SERVICE_NAME,
                DATASOURCE,
                PROCESS_NAME,
                STATEMENT_TYPE,
                STATEMENT_ID,
                STATEMENT_SUBTYPE,            
                QUERY_NAME,
                PARENT_QUERY_ID,
                PARENT_SYSTEM_NAME,           
                QUERY_PRIORITY,
                TRANSACTION_ID,
                CONN_RULE_NAME,
                COMP_RULE_NAME,
                EXEC_RULE_NAME,
                DB_USER_NAME,
                WARN_LEVEL,
                QUERY_TEXT,
                QUERY_TEXT_LEN
        }

        public static DataTable ConnectionFactsDetails = new DataTable();

        /// <summary>
        /// Compile-Time Metrics
        /// NOTE: Please be sure to use the defined column name as the enum.
        /// NOTE: Remember to modify the Construct Metrics DataTable whenever this definiton is modified.
        /// NOTE: Remember to update metric version.
        /// </summary>
        public enum CompileTimeMetrics
        {
                EST_COST = 0,
                EST_CARDINALITY,
                EST_CPU_TIME,
                EST_IO_TIME,
                EST_MSG_TIME,
                EST_IDLE_TIME,
                EST_TOTAL_TIME,
                EST_ACCESSED_ROWS,
                EST_USED_ROWS,
                EST_TOTAL_MEM,
                EST_MAX_CPU_BUSY,
                EST_RESRC_USAGE,
                CMP_AFFINITY_NUM,
                CMP_DOP,
                CMP_TXN_NEEDED,
                CMP_MANDATORY_X_PROD,
                CMP_MISSING_STATS,
                CMP_NUM_JOINS,
                CMP_FULL_SCAN_ON_TABLE,
                CMP_HIGH_EID_MAX_BUF_USAGE,
                CMP_ROWS_ACCESSED_FULL_SCAN,
                CMP_EID_ROWS_ACCESSED,
                CMP_EID_ROWS_USED,
                CMP_NUMBER_OF_BMOS,
                CMP_OVERFLOW_MODE,
                CMP_OVERFLOW_SIZE,
                AGGR_QUERY,
                AGGR_TOTAL_QUERIES,
                AGGR_SECS_SINCE_LAST_UPDATE,
                AGGR_SECS_TOTAL_TIME
        }

        private static DataTable CompileTimeDetails = new DataTable();

        /// <summary>
        /// Run-Time Metrics
        /// NOTE: Please be sure to use the defined column name as the enum.
        /// NOTE: Remember to modify the Construct Metrics DataTable whenever this definiton is modified.  
        /// NOTE: Remember to update metric version.
        /// </summary>
        public enum RunTimeMetrics
        {
                EXEC_STATE = 0,
                TOTAL_CHILD_COUNT,
                ACTIVE_CHILD_COUNT, 
                SQL_ERROR_CODE,
                ACCESSED_ROWS,
                USED_ROWS,
                SQL_SPACE_ALLOC,
                SQL_SPACE_USED,
                EID_SPACE_ALLOC,
                EID_SPACE_USED,
                CALC_TOTAL_SPACE_ALLOC,
                CALC_TOTAL_SPACE_USED,
                CALC_DELTA_SPACE_ALLOC,
                CALC_DELTA_SPACE_USED,
                CALC_DELTA_DISK_IOS,
                CALC_DELTA_IUD,
                SQL_HEAP_ALLOC,
                SQL_HEAP_USED,
                EID_HEAP_ALLOC,
                EID_HEAP_USED,
                TOTAL_MEM_ALLOC,
                MAX_MEM_USED,
                ROWS_RETURNED,
                REQ_MESSAGE_COUNT,
                REQ_MESSAGE_BYTES,
                REPLY_MESSAGE_COUNT,
                REPLY_MESSAGE_BYTES,
                STATS_ERROR_CODE,
                STATS_BYTES,
                DISK_IOS,
                LOCK_WAITS,
                LOCK_ESCALATIONS,
                OPENS,
                LAST_ERROR_BEFORE_AQR,
                AQR_NUM_RETRIES,
                DELAY_BEFORE_AQR,
                NUM_NODES,
                NUM_SQL_PROCESSES,
                MESSAGE_COUNT,
                MESSAGE_BYTES,
                NUM_ROWS_IUD,
                PROCESSES_CREATED,
                OVF_FILE_COUNT, 
                OVF_SPACE_ALLOCATED,
                OVF_SPACE_USED,
                OVF_BLOCK_SIZE,
                OVF_WRITE_READ_COUNT,
                OVF_WRITE_COUNT,
                OVF_BUFFER_BLOCKS_WRITTEN,
                OVF_BUFFER_BYTES_WRITTEN,
                OVF_READ_COUNT,
                OVF_BUFFER_BLOCKS_READ,
                OVF_BUFFER_BYTES_READ
        }

        private static DataTable RunTimeDetails = new DataTable();

        private static String integerValueNumberFormat = TriageHelper.getNumberFormatForCurrentLocale(0);
        private static String precision0NumberFormat = TriageHelper.getLocaleNumberFormat(0, false);
        private static String precision2NumberFormat = TriageHelper.getLocaleNumberFormat(2, true);
        private static String precision4NumberFormat = TriageHelper.getLocaleNumberFormat(4, true);

        #endregion Fields

        #region Constructors

        /// <summary>
        /// Static constructor
        /// </summary>
        static WMSQueryDetailInfoUtils()
        {
            ConstructTimeBasedMetricsTable();
            ConstructConnectionFactsTable();
            ConstructCompileTimeMetricsTable();
            ConstructRunTimeMetricsTable();
        }

        /// <summary>
        /// Construct the metric data table's column definitions.
        /// </summary>
        /// <param name="dt"></param>
        private static void AddColumnDefinitions(DataTable dt)
        {
            dt.PrimaryKey = new DataColumn[]{dt.Columns.Add(DetailColumns.id.ToString(), typeof(int))};
            dt.Columns.Add(DetailColumns.display.ToString(), typeof(string));
            dt.Columns.Add(DetailColumns.column.ToString(), typeof(string));
            dt.Columns.Add(DetailColumns.datatype.ToString(), typeof(Type));
            dt.Columns.Add(DetailColumns.method.ToString(), typeof(GetMethod));
            dt.Columns.Add(DetailColumns.call.ToString(), typeof(object));
            dt.Columns.Add(DetailColumns.format.ToString(), typeof(string));
        }
      
        /// <summary>
        /// Cconstruct the time-based metrics datatable
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructTimeBasedMetricsTable()
        {
            AddColumnDefinitions(TimeBasedDetails);

            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.QUERY_START_TIME, "Start Time", TimeBasedMetrics.QUERY_START_TIME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.START_TS, "Submit Time", TimeBasedMetrics.START_TS.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_END_TS, "End Time", TimeBasedMetrics.EXEC_END_TIME.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.ELAPSED_TIME, "Elapsed Time", TimeBasedMetrics.ELAPSED_TIME.ToString(), typeof(long), GetMethod.WMSTimeLong, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.WAIT_TIME, "Wait Time", TimeBasedMetrics.WAIT_TIME.ToString(), typeof(int), GetMethod.WMSTimeInt, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.HOLD_TIME, "Hold Time", TimeBasedMetrics.HOLD_TIME.ToString(), typeof(int), GetMethod.WMSTimeInt, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_TOTAL_QUERY_TIME, "Total Query Time", TimeBasedMetrics.CALC_TOTAL_QUERY_TIME.ToString(), typeof(long), GetMethod.Calc1, new CalcRoutine(GetTotalQueryTime), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.SQL_CPU_TIME, "SQL CPU Time", TimeBasedMetrics.SQL_CPU_TIME.ToString(), typeof(long), GetMethod.WMSTimeLong, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.PROCESS_BUSYTIME, "Disk CPU Time", TimeBasedMetrics.PROCESS_BUSYTIME.ToString(), typeof(long), GetMethod.WMSTimeLong, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_TOTAL_CPU_TIME, "Total Processor Time", TimeBasedMetrics.CALC_TOTAL_CPU_TIME.ToString(), typeof(long), GetMethod.Calc1, new CalcRoutine(GetTotalCPUTime), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_TOTAL_CPU_TIME_IN_MICRO, "Total Processor Time Microseconds", TimeBasedMetrics.CALC_TOTAL_CPU_TIME_IN_MICRO.ToString(), typeof(long), GetMethod.Calc1, new CalcRoutine(GetTotalCPUTimeInMicros), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.PROCESS_CREATE_TIME, "Process Create Time", TimeBasedMetrics.PROCESS_CREATE_TIME.ToString(), typeof(long), GetMethod.WMSTimeLong, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.OPEN_TIME, "Open Time", TimeBasedMetrics.OPEN_TIME.ToString(), typeof(long), GetMethod.WMSTimeLong, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.COMP_START_TIME, "Comp Start Time", TimeBasedMetrics.COMP_START_TIME.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.COMP_END_TIME, "Comp End Time", TimeBasedMetrics.COMP_END_TIME.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CMP_LAST_UPDATED, "Comp Last Updated Time", TimeBasedMetrics.CMP_LAST_UPDATED.ToString(), typeof(long), GetMethod.WMSTimestamp, null, integerValueNumberFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.EXEC_START_TIME, "Exec Start Time", TimeBasedMetrics.EXEC_START_TIME.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.EXEC_END_TIME, "Exec End Time", TimeBasedMetrics.EXEC_END_TIME.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.FIRST_ROW_RETURNED_TIME, "First Row Returned Time", TimeBasedMetrics.FIRST_ROW_RETURNED_TIME.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.EXEC_LAST_UPDATED, "Exec Last Updated Time", TimeBasedMetrics.EXEC_LAST_UPDATED.ToString(), typeof(long), GetMethod.WMSTimestamp, null, NoFormat });
            //TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_QUERY_ELAPSED_TIME, "Query Elapsed Time", TimeBasedMetrics.CALC_QUERY_ELAPSED_TIME.ToString(), typeof(long), GetMethod.Calc });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.ENTRY_TS, "Entry Ts", TimeBasedMetrics.ENTRY_TS.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.ENTRY_JTS, "Entry Jts", TimeBasedMetrics.ENTRY_JTS.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.SUSPENDED_TS, "Suspended Time", TimeBasedMetrics.SUSPENDED_TS.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.RELEASED_TS, "Released Time", TimeBasedMetrics.RELEASED_TS.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CANCELLED_TS, "Cancelled Time", TimeBasedMetrics.CANCELLED_TS.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_LAST_INTERVAL_CPU_TIME, "Last Interval Processor Time", TimeBasedMetrics.CALC_LAST_INTERVAL_CPU_TIME.ToString(), typeof(long), GetMethod.Calc2, new CalcRoutineWithOldRow(GetLastIntervalCPUTime), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_PROCESSOR_USAGE, "Processor Usage/Second", TimeBasedMetrics.CALC_PROCESSOR_USAGE.ToString(), typeof(double), GetMethod.Calc1, new CalcRoutine(GetProcessorUsagePerSecond), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_DELTA_PROCESSOR_TIME, "Delta Processor Time", TimeBasedMetrics.CALC_DELTA_PROCESSOR_TIME.ToString(), typeof(long), GetMethod.Calc2, new CalcRoutineWithOldRow(GetDeltaProcessorTime), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.UDR_PROCESS_BUSY_TIME, "UDR Process Busy Time", TimeBasedMetrics.UDR_PROCESS_BUSY_TIME.ToString(), typeof(long), GetMethod.DataTable, null, NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_ROWS_PER_SEC, "Rows/Second", TimeBasedMetrics.CALC_ROWS_PER_SEC.ToString(), typeof(double), GetMethod.Calc1, new CalcRoutine(GetRowsPerSecond), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.CALC_IUD_PER_SEC, "IUD/Second", TimeBasedMetrics.CALC_IUD_PER_SEC.ToString(), typeof(double), GetMethod.Calc1, new CalcRoutine(GetIUDPerSecond), NoFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.SQL_CPU_OFFENDER_INTERVAL_SECS, "CPU Offender Interval (secs)", TimeBasedMetrics.SQL_CPU_OFFENDER_INTERVAL_SECS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.SQL_TSE_OFFENDER_INTERVAL_SECS, "TSE Offender Interval (secs)", TimeBasedMetrics.SQL_TSE_OFFENDER_INTERVAL_SECS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            TimeBasedDetails.Rows.Add(new object[] { (int)TimeBasedMetrics.SQL_SLOW_OFFENDER_INTERVAL_SECS, "Slow Offender Interval (secs)", TimeBasedMetrics.SQL_SLOW_OFFENDER_INTERVAL_SECS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });

        }


        /// <summary>
        /// Construct the Connection metrics datatable.
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructConnectionFactsTable()
        {
            AddColumnDefinitions(ConnectionFactsDetails);

            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_ID, "Query ID", ConnectionFacts.QUERY_ID.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_STATE, "State", ConnectionFacts.QUERY_STATE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_SUBSTATE, "Substate", ConnectionFacts.QUERY_SUBSTATE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.APPLICATION_NAME, "Application", ConnectionFacts.APPLICATION_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.COMPUTER_NAME, "Computer", ConnectionFacts.COMPUTER_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.ROLE_NAME, "Role Name", ConnectionFacts.ROLE_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.USER_NAME, "User Name", ConnectionFacts.USER_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.SERVICE_NAME, "Service", ConnectionFacts.SERVICE_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.DATASOURCE, "Datasource", ConnectionFacts.DATASOURCE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.PROCESS_NAME, "Process Name", ConnectionFacts.PROCESS_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.STATEMENT_TYPE, "Statement Type", ConnectionFacts.STATEMENT_TYPE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.STATEMENT_SUBTYPE, "Statement Sub Type", ConnectionFacts.STATEMENT_SUBTYPE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.STATEMENT_ID, "Statement ID", ConnectionFacts.STATEMENT_ID.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_NAME, "Query Name", ConnectionFacts.QUERY_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.PARENT_QUERY_ID, "Parent Query ID", ConnectionFacts.PARENT_QUERY_ID.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.PARENT_SYSTEM_NAME, "Parent System Name", ConnectionFacts.PARENT_SYSTEM_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_PRIORITY, "Query Priority", ConnectionFacts.QUERY_PRIORITY.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.TRANSACTION_ID, "Transaction ID", ConnectionFacts.TRANSACTION_ID.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_TEXT_LEN, "Query Text Length", ConnectionFacts.QUERY_TEXT_LEN.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.CONN_RULE_NAME, "Conn Rule Name", ConnectionFacts.CONN_RULE_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.COMP_RULE_NAME, "Comp Rule Name", ConnectionFacts.COMP_RULE_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.EXEC_RULE_NAME, "Exec Rule Name", ConnectionFacts.EXEC_RULE_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.DB_USER_NAME, "Database User Name", ConnectionFacts.DB_USER_NAME.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.WARN_LEVEL, "Warn Level", ConnectionFacts.WARN_LEVEL.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            ConnectionFactsDetails.Rows.Add(new object[] { (int)ConnectionFacts.QUERY_TEXT, "Query Text", ConnectionFacts.QUERY_TEXT.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
        }

        /// <summary>
        /// To construct the compile-time metrics datatable.
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructCompileTimeMetricsTable()
        {
            AddColumnDefinitions(CompileTimeDetails);

            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_COST, "Est Cost", CompileTimeMetrics.EST_COST.ToString(), typeof(double), GetMethod.DataTable, null, precision2NumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_CARDINALITY, "Est Cardinality", CompileTimeMetrics.EST_CARDINALITY.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_CPU_TIME, "Est CPU Time", CompileTimeMetrics.EST_CPU_TIME.ToString(), typeof(double), GetMethod.WMSTimeSecs, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_IO_TIME, "Est IO Time", CompileTimeMetrics.EST_IO_TIME.ToString(), typeof(double), GetMethod.WMSTimeSecs, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_MSG_TIME, "Est Message Time", CompileTimeMetrics.EST_MSG_TIME.ToString(), typeof(double), GetMethod.WMSTimeSecs, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_IDLE_TIME, "Est Idle Time", CompileTimeMetrics.EST_IDLE_TIME.ToString(), typeof(double), GetMethod.WMSTimeSecs, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_TOTAL_TIME, "Est Total Time", CompileTimeMetrics.EST_TOTAL_TIME.ToString(), typeof(double), GetMethod.WMSTimeSecs, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_ACCESSED_ROWS, "Est Accessed Rows", CompileTimeMetrics.EST_ACCESSED_ROWS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_USED_ROWS, "Est Used Rows", CompileTimeMetrics.EST_USED_ROWS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_TOTAL_MEM, "Est Total Memory (KB)", CompileTimeMetrics.EST_TOTAL_MEM.ToString(), typeof(double), GetMethod.DataTable, null, precision2NumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_MAX_CPU_BUSY, "Est Max CPU Busy", CompileTimeMetrics.EST_MAX_CPU_BUSY.ToString(), typeof(double), GetMethod.DataTable, null, precision2NumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.EST_RESRC_USAGE, "Est Resource Usage", CompileTimeMetrics.EST_RESRC_USAGE.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_AFFINITY_NUM, "Cmp Affinity Num", CompileTimeMetrics.CMP_AFFINITY_NUM.ToString(), typeof(long), GetMethod.DataTable, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_DOP, "Cmp Degree Of Parallelism", CompileTimeMetrics.CMP_DOP.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_TXN_NEEDED, "Cmp Txn Needed", CompileTimeMetrics.CMP_TXN_NEEDED.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_MANDATORY_X_PROD, "Cmp Mandatory Cross Product", CompileTimeMetrics.CMP_MANDATORY_X_PROD.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_MISSING_STATS, "Cmp Missing Stats", CompileTimeMetrics.CMP_MISSING_STATS.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_NUM_JOINS, "Cmp Num Joins", CompileTimeMetrics.CMP_NUM_JOINS.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_FULL_SCAN_ON_TABLE, "Cmp Full Scan On Table", CompileTimeMetrics.CMP_FULL_SCAN_ON_TABLE.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_HIGH_EID_MAX_BUF_USAGE, "Cmp High EID Max Buffer Usage", CompileTimeMetrics.CMP_HIGH_EID_MAX_BUF_USAGE.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_ROWS_ACCESSED_FULL_SCAN, "Cmp Rows Accessed Full Scan", CompileTimeMetrics.CMP_ROWS_ACCESSED_FULL_SCAN.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_EID_ROWS_ACCESSED, "Cmp Disk Process Rows Accessed", CompileTimeMetrics.CMP_EID_ROWS_ACCESSED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_EID_ROWS_USED, "Cmp Disk Process Rows Used", CompileTimeMetrics.CMP_EID_ROWS_USED.ToString(), typeof(double), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_NUMBER_OF_BMOS, "Cmp Number of BMOS", CompileTimeMetrics.CMP_NUMBER_OF_BMOS.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_OVERFLOW_MODE, "Cmp Overflow Mode", CompileTimeMetrics.CMP_OVERFLOW_MODE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.CMP_OVERFLOW_SIZE, "Cmp Overflow Size", CompileTimeMetrics.CMP_OVERFLOW_SIZE.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.AGGR_QUERY, "Is Aggr Query", CompileTimeMetrics.AGGR_QUERY.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.AGGR_TOTAL_QUERIES, "Aggr Total Queries", CompileTimeMetrics.AGGR_TOTAL_QUERIES.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.AGGR_SECS_SINCE_LAST_UPDATE, "Aggr Secs Since Last Update", CompileTimeMetrics.AGGR_SECS_SINCE_LAST_UPDATE.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            CompileTimeDetails.Rows.Add(new object[] { (int)CompileTimeMetrics.AGGR_SECS_TOTAL_TIME, "Aggr Secs Total Time", CompileTimeMetrics.AGGR_SECS_TOTAL_TIME.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            
        }

        /// <summary>
        /// Consturct the run-time metrics datatable.
        /// NOTE: This has to follow the order of the ENUM definition. 
        /// NOTE: Remember to update metric version. (esp. the display name got changed)
        /// </summary>
        private static void ConstructRunTimeMetricsTable()
        {
            AddColumnDefinitions(RunTimeDetails);

            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.EXEC_STATE, "Exec State", RunTimeMetrics.EXEC_STATE.ToString(), typeof(string), GetMethod.DataTable, null, NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.TOTAL_CHILD_COUNT, "Total Child Count", RunTimeMetrics.TOTAL_CHILD_COUNT.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.ACTIVE_CHILD_COUNT, "Active Child Count", RunTimeMetrics.ACTIVE_CHILD_COUNT.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.SQL_ERROR_CODE, "Sql Error Code", RunTimeMetrics.SQL_ERROR_CODE.ToString(), typeof(int), GetMethod.DataTable, null, NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.ACCESSED_ROWS, "Accessed Rows", RunTimeMetrics.ACCESSED_ROWS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.USED_ROWS, "Used Rows", RunTimeMetrics.USED_ROWS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.SQL_SPACE_ALLOC, "SQL Space Alloc (KB)", RunTimeMetrics.SQL_SPACE_ALLOC.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.SQL_SPACE_USED, "SQL Space Used (KB)", RunTimeMetrics.SQL_SPACE_USED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.EID_SPACE_ALLOC, "EID Space Alloc (KB)", RunTimeMetrics.EID_SPACE_ALLOC.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.EID_SPACE_USED, "EID Space Used (KB)", RunTimeMetrics.EID_SPACE_USED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.CALC_DELTA_SPACE_ALLOC, "Delta Space Alloc (KB)", RunTimeMetrics.CALC_DELTA_SPACE_ALLOC.ToString(), typeof(long), GetMethod.Calc2, new CalcRoutineWithOldRow(GetDeltaSpaceAlloc), NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.CALC_DELTA_SPACE_USED, "Delta Space Used (KB)", RunTimeMetrics.CALC_DELTA_SPACE_USED.ToString(), typeof(long), GetMethod.Calc2, new CalcRoutineWithOldRow(GetDeltaSpaceUsed), NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.CALC_TOTAL_SPACE_ALLOC, "Total Space Alloc (KB)", RunTimeMetrics.CALC_TOTAL_SPACE_ALLOC.ToString(), typeof(long), GetMethod.Calc1, new CalcRoutine(GetTotalSpaceAlloc), NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.CALC_TOTAL_SPACE_USED, "Total Space Used (KB)", RunTimeMetrics.CALC_TOTAL_SPACE_USED.ToString(), typeof(long), GetMethod.Calc1, new CalcRoutine(GetTotalSpaceUsed), NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.SQL_HEAP_ALLOC, "SQL Heap Alloc (KB)", RunTimeMetrics.SQL_HEAP_ALLOC.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.SQL_HEAP_USED, "SQL Heap Used (KB)", RunTimeMetrics.SQL_HEAP_USED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.EID_HEAP_ALLOC, "EID Heap Alloc (KB)", RunTimeMetrics.EID_HEAP_ALLOC.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.EID_HEAP_USED, "EID Heap Used (KB)", RunTimeMetrics.EID_HEAP_USED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.TOTAL_MEM_ALLOC, "Total Mem Alloc (KB)", RunTimeMetrics.TOTAL_MEM_ALLOC.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.MAX_MEM_USED, "Max Mem Used (KB)", RunTimeMetrics.MAX_MEM_USED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.ROWS_RETURNED, "Rows Returned", RunTimeMetrics.ROWS_RETURNED.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.REQ_MESSAGE_COUNT, "Req Message Count", RunTimeMetrics.REQ_MESSAGE_COUNT.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.REQ_MESSAGE_BYTES, "Req Message Bytes", RunTimeMetrics.REQ_MESSAGE_BYTES.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.REPLY_MESSAGE_COUNT, "Reply Message Count", RunTimeMetrics.REPLY_MESSAGE_COUNT.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.REPLY_MESSAGE_BYTES, "Reply Message Bytes", RunTimeMetrics.REPLY_MESSAGE_BYTES.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.STATS_ERROR_CODE, "Stats Error Code", RunTimeMetrics.STATS_ERROR_CODE.ToString(), typeof(int), GetMethod.DataTable, null, NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.STATS_BYTES, "Stats Bytes", RunTimeMetrics.STATS_BYTES.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.DISK_IOS, "Disk IOs", RunTimeMetrics.DISK_IOS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.CALC_DELTA_DISK_IOS, "Delta Disk IOS", RunTimeMetrics.CALC_DELTA_DISK_IOS.ToString(), typeof(long), GetMethod.Calc2, new CalcRoutineWithOldRow(GetDeltaIOs), NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.LOCK_WAITS, "Lock Waits", RunTimeMetrics.LOCK_WAITS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.LOCK_ESCALATIONS, "Lock Escalations", RunTimeMetrics.LOCK_ESCALATIONS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OPENS, "Opens", RunTimeMetrics.OPENS.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.LAST_ERROR_BEFORE_AQR, "Last Error Before AQR", RunTimeMetrics.LAST_ERROR_BEFORE_AQR.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.AQR_NUM_RETRIES, "AQR Num Retries", RunTimeMetrics.AQR_NUM_RETRIES.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.DELAY_BEFORE_AQR, "Delay Before AQR", RunTimeMetrics.DELAY_BEFORE_AQR.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.NUM_NODES, "Num Nodes", RunTimeMetrics.NUM_NODES.ToString(), typeof(long), GetMethod.DataTable, null, NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.NUM_SQL_PROCESSES, "Num SQL Processes", RunTimeMetrics.NUM_SQL_PROCESSES.ToString(), typeof(int), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.MESSAGE_COUNT, "Messages To Disk", RunTimeMetrics.MESSAGE_COUNT.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.MESSAGE_BYTES, "Msg Bytes to Disk", RunTimeMetrics.MESSAGE_BYTES.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.NUM_ROWS_IUD, "Num Rows IUD", RunTimeMetrics.NUM_ROWS_IUD.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.CALC_DELTA_IUD, "Delta IUD", RunTimeMetrics.CALC_DELTA_IUD.ToString(), typeof(long), GetMethod.Calc2, new CalcRoutineWithOldRow(GetDeltaIUD), NoFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.PROCESSES_CREATED, "Processes Created", RunTimeMetrics.PROCESSES_CREATED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_FILE_COUNT, "Overflow File Count", RunTimeMetrics.OVF_FILE_COUNT.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_SPACE_ALLOCATED, "Overflow Space Alloc (KB)", RunTimeMetrics.OVF_SPACE_ALLOCATED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_SPACE_USED, "Overflow Space Used (KB)", RunTimeMetrics.OVF_SPACE_USED.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_BLOCK_SIZE, "Overflow Block Size (KB)", RunTimeMetrics.OVF_BLOCK_SIZE.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_WRITE_READ_COUNT, "Overflow Write Read Count", RunTimeMetrics.OVF_WRITE_READ_COUNT.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_WRITE_COUNT, "Overflow Write Count", RunTimeMetrics.OVF_WRITE_COUNT.ToString(), typeof(double), GetMethod.DataTable, null, precision0NumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_BUFFER_BLOCKS_WRITTEN, "Overflow Buffer Blocks Written", RunTimeMetrics.OVF_BUFFER_BLOCKS_WRITTEN.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_BUFFER_BYTES_WRITTEN, "Overflow Buffer Bytes Written (KB)", RunTimeMetrics.OVF_BUFFER_BYTES_WRITTEN.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_READ_COUNT, "Overflow Read Count", RunTimeMetrics.OVF_READ_COUNT.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_BUFFER_BLOCKS_READ, "Overflow Buffer Blocks Read", RunTimeMetrics.OVF_BUFFER_BLOCKS_READ.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
            RunTimeDetails.Rows.Add(new object[] { (int)RunTimeMetrics.OVF_BUFFER_BYTES_READ, "Overflow Buffer Bytes Read (KB)", RunTimeMetrics.OVF_BUFFER_BYTES_READ.ToString(), typeof(long), GetMethod.DataTable, null, integerValueNumberFormat });
        }

        #endregion Constructors

        #region Public methods

        /// <summary>
        /// To get all of the display names for a given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static List<string> GetMetricDisplayNames(Metrics m)
        {
            List<string> names = new List<string>();
            foreach (DataRow dr in GetDetailTable(m).Rows)
            {
                names.Add(dr[DetailColumns.display.ToString()] as string);
            }

            return names;
        }

        /// <summary>
        /// Get the datatable for the given metric
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        public static DataTable GetDetailTable(Metrics m)
        {
            switch (m)
            {
                case Metrics.CompileTime: 
                    return CompileTimeDetails;

                case Metrics.Connection:
                    return ConnectionFactsDetails;

                case Metrics.RunTime:
                    return RunTimeDetails;

                case Metrics.TimeBased:
                    return TimeBasedDetails;
            }

            return null;
        }

        /// <summary>
        /// Get a specific display name for a given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <returns></returns>
        public static string GetDisplayName(Metrics m, int idx)
        {
            DataTable dt = GetDetailTable(m);
            return dt.Rows.Find(idx)[DetailColumns.display.ToString()] as string;
        }

        /// <summary>
        /// Get a specific column name for a given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <returns></returns>
        public static string GetColumnName(Metrics m, int idx)
        {
            DataTable dt = GetDetailTable(m);
            return dt.Rows.Find(idx)[DetailColumns.column.ToString()] as string;
        }

        /// <summary>
        /// Get the data type for a spcific column in the given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <returns></returns>
        public static Type GetDataType(Metrics m, int idx)
        {
            DataTable dt = GetDetailTable(m);
            return dt.Rows.Find(idx)[DetailColumns.datatype.ToString()] as Type;
        }

        /// <summary>
        /// Get the value caculation method for a given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <returns></returns>
        public static GetMethod GetMethodType(Metrics m, int idx)
        {
            DataTable dt = GetDetailTable(m);
            return (GetMethod)dt.Rows.Find(idx)[DetailColumns.method.ToString()];
        }

        /// <summary>
        /// Get the foramt string for a given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <returns></returns>
        public static string GetFormat(Metrics m, int idx)
        {
            DataTable dt = GetDetailTable(m);
            return (string)dt.Rows.Find(idx)[DetailColumns.format.ToString()];
        }

        /// <summary>
        /// Get the method delegate for a given metric.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <returns></returns>
        public static object GetDelegate(Metrics m, int idx)
        {
            DataTable dt = GetDetailTable(m);
            object obj = dt.Rows.Find(idx)[DetailColumns.call.ToString()];
            if (obj != System.DBNull.Value)
            {
                return obj;
            }

            return null;
        }

        /// <summary>
        /// Metric data conversion.
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <param name="dr"></param>
        /// <returns></returns>
        public static object ConvertMetrics(Metrics m, int idx, DataRow dr)
        {
            return ConvertMetrics(m, idx, dr, null);
        }

        /// <summary>
        /// Convert a metric to its output display form
        /// </summary>
        /// <param name="m"></param>
        /// <param name="idx"></param>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        public static object ConvertMetrics(Metrics m, int idx, DataRow newRow, DataRow oldRow)
        {
            String columnName = WMSQueryDetailInfoUtils.GetColumnName(m, idx);

            switch (GetMethodType(m, idx))
            {
                case GetMethod.DataTable:
                    {
                        return Convert.ChangeType(newRow[columnName], GetDataType(m, idx));
                    }

                case GetMethod.WMSTimestamp:
                    {
                        return WMSUtils.convertJulianTimeStamp(Convert.ChangeType(newRow[columnName], GetDataType(m, idx)));
                    }

                case GetMethod.WMSTimeInt:
                    {
                        return WMSUtils.formatInt2Time((int)newRow[columnName]);
                    }

                case GetMethod.WMSDouble:
                    {
                        return String.Format("{0:F}", (double)newRow[columnName]);
                    }

                case GetMethod.WMSTimeLong:
                    {
                        return WMSUtils.FormatTimeFromMilliseconds(((long)newRow[columnName]) / 1000);
                    }
                case GetMethod.WMSTimeSecs:
                    {
                        return WMSUtils.FormatTimeFromMilliseconds((double)newRow[columnName] * 1000);
                    }

                case GetMethod.Calc1:
                    {
                        CalcRoutine call = (CalcRoutine)WMSQueryDetailInfoUtils.GetDelegate(m, idx);
                        if (call != null)
                        {
                            return call(newRow);
                        }

                        return "";
                    }

                case GetMethod.Calc2:
                    {
                        CalcRoutineWithOldRow call = (CalcRoutineWithOldRow)WMSQueryDetailInfoUtils.GetDelegate(m, idx);
                        if (call != null && oldRow != null)
                        {
                            return call(newRow, oldRow);
                        }

                        return "";
                    }
                default:
                    return "";
            }
        }

        #endregion Public methods

        #region Private methods

        /// <summary>
        /// Delegate function for End time caculation.
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string EndTimeCalc(DataRow dr)
        {
            string queryState = dr["QUERY_STATE"] as string;
            if (!string.IsNullOrEmpty(queryState) && (queryState.Equals("COMPLETED")||queryState.Equals("REJECTED")))
            {
                string start_time = dr["QUERY_START_TIME"] as string;
                DateTime t = DateTime.ParseExact(start_time, "yyyy-MM-dd HH:mm:ss.ffffff", CultureInfo.InvariantCulture);

                string end_time = dr["EXEC_END_TIME"] as string;
                return  String.IsNullOrEmpty(end_time)? "" : end_time;
/*
                long elapsed = (long)dr["ELAPSED_TIME"];
                TimeSpan ts = new TimeSpan(elapsed * 10);
                t += ts;
                return String.Format("{0:yyyy-MM-dd HH:mm:ss.ffffff}", t);*/

            }
            else
            {
                return"";
            }
        }

        /// <summary>
        /// Return the total space allocation
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static long TotalSpaceAlloc(DataRow dr)
        {
            long totalSpaceAllocated = 0;
            try
            {
                long sqlSpaceAllocated = (long)dr["SQL_SPACE_ALLOC"];
                long sqlHeapAllocated = (long)dr["SQL_HEAP_ALLOC"];
                long eidSpaceAllocated = (long)dr["EID_SPACE_ALLOC"];
                long eidHeapAllocated = (long)dr["EID_HEAP_ALLOC"];
                totalSpaceAllocated = sqlSpaceAllocated + sqlHeapAllocated + eidHeapAllocated + eidSpaceAllocated;
            }
            catch (Exception) { }

            return totalSpaceAllocated;
        }

        /// <summary>
        /// Return the total space used
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static long TotalSpaceUsed(DataRow dr)
        {
            long totalSpaceUsed = 0;
            try
            {
                long sqlSpaceUsed = (long)dr["SQL_SPACE_USED"];
                long sqlHeapUsed = (long)dr["SQL_HEAP_USED"];
                long eidSpaceUsed = (long)dr["EID_SPACE_USED"];
                long eidHeapUsed = (long)dr["EID_HEAP_USED"];
                totalSpaceUsed = sqlSpaceUsed + sqlHeapUsed + eidSpaceUsed + eidHeapUsed;
            }
            catch (Exception) { }

            return totalSpaceUsed;
        }

        /// <summary>
        /// Return the total cpu utilization in micro secs
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static long TotalCPUTimeMicroSecs(DataRow dr)
        {
            long totalCPUTimeMicroSecs = 0;
            try
            {
                totalCPUTimeMicroSecs = (long)dr["SQL_CPU_TIME"] +
                                        (long)dr["PROCESS_BUSYTIME"] +
                                        (long)dr["OPEN_TIME"] +
                                        (long)dr["PROCESS_CREATE_TIME"];
            }
            catch (Exception)
            { }

            return totalCPUTimeMicroSecs;
        }

        /// <summary>
        /// Delegate function for total space allocation.
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetTotalSpaceAlloc(DataRow dr)
        {
            long totalSpaceAllocated = TotalSpaceAlloc(dr);
            return string.Format(integerValueNumberFormat, totalSpaceAllocated);
        }

        /// <summary>
        /// Delegate function for getting total space used.
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetTotalSpaceUsed(DataRow dr)
        {
            long totalSpaceUsed = TotalSpaceUsed(dr);
            return string.Format(integerValueNumberFormat, totalSpaceUsed);
        }

        /// <summary>
        /// Delegate function for getting total cpu time in microseconds
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetTotalCPUTimeInMicros(DataRow dr)
        {
            long totalCPUTimeMicroSecs = TotalCPUTimeMicroSecs(dr);
            return string.Format(integerValueNumberFormat, totalCPUTimeMicroSecs);
        }

        /// <summary>
        /// Delegate function for getting total cpu time
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetTotalCPUTime(DataRow dr)
        {
            long totalCPUTimeMicroSecs = TotalCPUTimeMicroSecs(dr);
            return WMSUtils.FormatTimeFromMilliseconds(totalCPUTimeMicroSecs / 1000);
        }

        /// <summary>
        /// Delegate function for getting total query time
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetTotalQueryTime(DataRow dr)
        {
            try
            {
                long elapsed_time = ((long)dr["ELAPSED_TIME"]) / 1000;
                int wait_time = (int)dr["WAIT_TIME"] * 1000;
                int hold_time = (int)dr["HOLD_TIME"] * 1000;
                return WMSUtils.FormatTimeFromMilliseconds(elapsed_time + wait_time + hold_time);
            }
            catch (Exception)
            {}

            return "";
        }

        /// <summary>
        /// Delegate function for getting processor usage
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetProcessorUsagePerSecond(DataRow dr)
        {
            double processorUsageSec = 0.00;

            try
            {
                double totalCPUTimeMicroSecs = TotalCPUTimeMicroSecs(dr);
                long elapsedTime = (long)dr["ELAPSED_TIME"];
                if (elapsedTime > 0)
                {
                    processorUsageSec = totalCPUTimeMicroSecs/elapsedTime;
                }
            }
            catch (Exception)
            {}

            return string.Format(precision2NumberFormat, processorUsageSec);
        }

        /// <summary>
        /// Delegate function for getting rows per second
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetRowsPerSecond(DataRow dr)
        {
            long rowsPerSecond = 0;
            try
            {
                long usedRows = (long)dr["USED_ROWS"];
                long elapsedTime = (long)dr["ELAPSED_TIME"]/1000000;
                if (elapsedTime > 0)
                {
                    rowsPerSecond = usedRows / elapsedTime;
                }
            }
            catch (Exception ex) { }

            return string.Format(integerValueNumberFormat, rowsPerSecond);
        }

        /// <summary>
        /// Delegate function for getting IUD/Second
        /// </summary>
        /// <param name="dr"></param>
        /// <returns></returns>
        private static string GetIUDPerSecond(DataRow dr)
        {
            double iudsPerSecond = 0.00;
            try
            {
                long IUDRows = (long)dr["NUM_ROWS_IUD"];
                long elapsedTime = (long)dr["ELAPSED_TIME"]/1000000;
                if (elapsedTime > 0)
                {
                    iudsPerSecond = IUDRows / elapsedTime;
                }
            }
            catch (Exception ex) { }

            return string.Format(precision2NumberFormat, iudsPerSecond);
        }

        /// <summary>
        /// Delegate function for getting the last interval CPU utilization 
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private static string GetLastIntervalCPUTime(DataRow newRow, DataRow oldRow)
        {
            //get the last interval processor time of the previous DataTable
            double previousTotalCPUTimeMicroSecs = TotalCPUTimeMicroSecs(oldRow);
            return WMSUtils.FormatTimeFromMilliseconds(previousTotalCPUTimeMicroSecs/1000);
        }

        /// <summary>
        /// Delegate function for getting the delta disk IOs
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private static string GetDeltaIOs(DataRow newRow, DataRow oldRow)
        {
            long deltaIOs = 0;
            try
            {
                long diskIOs = (long)newRow["DISK_IOS"];
                long prevDiskIOS = (long)oldRow["DISK_IOS"];
                deltaIOs = diskIOs - prevDiskIOS;
            }
            catch (Exception) { }

            return string.Format(integerValueNumberFormat, deltaIOs);
        }

        /// <summary>
        /// Delegate function for getting delta IUDs
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private static string GetDeltaIUD(DataRow newRow, DataRow oldRow)
        {
            long deltaIUD = 0;

            try
            {
                long currentIUD = (long)newRow["NUM_ROWS_IUD"];
                long prevIUD = (long)oldRow["NUM_ROWS_IUD"];
                deltaIUD = currentIUD - prevIUD;
            }
            catch (Exception) { }
            return string.Format(integerValueNumberFormat, deltaIUD);
        }

        /// <summary>
        /// Delegate function for getting the delta processor time
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private static string GetDeltaProcessorTime(DataRow newRow, DataRow oldRow)
        {
            //get the current interval processor time
            double currentTotalCPUTimeMicroSecs = TotalCPUTimeMicroSecs(newRow);

            //get the last interval processor time of the previous DataTable
            double previousTotalCPUTimeMicroSecs = TotalCPUTimeMicroSecs(oldRow);
            return WMSUtils.FormatTimeFromMilliseconds((currentTotalCPUTimeMicroSecs - previousTotalCPUTimeMicroSecs) / 1000);
        }

        /// <summary>
        /// Delegate function for getting the delta space allocation
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private static string GetDeltaSpaceAlloc(DataRow newRow, DataRow oldRow)
        {
            long currentAlloc = TotalSpaceAlloc(newRow);
            long previousAlloc = TotalSpaceAlloc(oldRow);
            return string.Format(integerValueNumberFormat, currentAlloc - previousAlloc);
        }

        /// <summary>
        /// Delegate function for getting the delta space used.
        /// </summary>
        /// <param name="newRow"></param>
        /// <param name="oldRow"></param>
        /// <returns></returns>
        private static string GetDeltaSpaceUsed(DataRow newRow, DataRow oldRow)
        {
            long currentUsed = TotalSpaceUsed(newRow);
            long previousUsed = TotalSpaceUsed(oldRow);
            return string.Format(integerValueNumberFormat, currentUsed - previousUsed);
        }

        #endregion Private methods
    }
}
