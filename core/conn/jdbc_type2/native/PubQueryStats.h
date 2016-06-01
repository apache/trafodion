// ===============================================================================================
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
// ===============================================================================================
#ifndef QPID_QUERY_STATS_DEFINED
#define QPID_QUERY_STATS_DEFINED

#include <string>
using namespace std;

enum pub_struct_type
{
    PUB_TYPE_INIT = 0,
    PUB_TYPE_SESSION_END,
    PUB_TYPE_STATEMENT_NEW_QUERYEXECUTION,
    PUB_TYPE_STATEMENT_UPDATE_QUERYEXECUTION,
    PUB_TYPE_SESSION_START_AGGREGATION,
    PUB_TYPE_SESSION_UPDATE_AGGREGATION,
    PUB_TYPE_SESSION_END_AGGREGATION,
};

enum statistics_type
{
    STATISTICS_AGGREGATED = 0,
    STATISTICS_SESSION,
    STATISTICS_QUERY,
};

typedef struct _SESSION_END
{
    unsigned int m_instance_id;
    unsigned int m_tenant_id;
    unsigned int m_component_id;
    int m_process_id;
    unsigned int m_thread_id;
    unsigned int m_node_id;
    unsigned int m_pnid_id;
    unsigned int m_host_id;
    string m_ip_address_id;
    unsigned int m_sequence_number;
    string m_process_name;
    string m_sessionId;
    string m_session_status;
    long long m_session_start_utc_ts;
    long long m_session_end_utc_ts;
    long m_user_id;
    string m_user_name;
    string m_role_name;
    string m_client_name;
    string m_client_user_name;
    string m_application_name;
    long long m_total_odbc_exection_time;
    long long m_total_odbc_elapsed_time;
    long m_total_insert_stmts_executed;
    long m_total_delete_stmts_executed;
    long m_total_update_stmts_executed;
    long m_total_select_stmts_executed;
    long m_total_catalog_stmts;
    long m_total_prepares;
    long m_total_executes;
    long m_total_fetches;
    long m_total_closes;
    long m_total_execdirects;
    long m_total_errors;
    long m_total_warnings;
    long long m_total_login_elapsed_time_mcsec;
    long long m_ldap_login_elapsed_time_mcsec;
    long long m_sql_user_elapsed_time_mcsec;
    long long m_search_connection_elapsed_time_mcsec;
    long long m_search_elapsed_time_mcsec;
    long long m_authentication_connection_elapsed_time_mcsec;
    long long m_authentication_elapsed_time_mcsec;
}SESSION_END, *pSESSION_END;

typedef struct _STATEMENT_QUERYEXECUTION
{
    ~_STATEMENT_QUERYEXECUTION()
    { if (m_explain_plan != 0) delete [] m_explain_plan;
      m_explain_plan = 0;
    };

    unsigned int m_instance_id;
    unsigned int m_tenant_id;
    unsigned int m_component_id;
    int m_process_id;
    unsigned int m_thread_id;
    unsigned int m_node_id;
    unsigned int m_pnid_id;
    unsigned int m_host_id;
    string m_ip_address_id;
    unsigned int m_sequence_number;
    string m_process_name;
    long long m_exec_start_utc_ts;
    string m_query_id;
    string m_user_name;
    string m_role_name;
    unsigned int m_start_priority;
    string m_master_process_id;
    string m_session_id;
    string m_client_name;
    string m_application_name;
    string m_statement_id;
    string m_statement_type;
    string m_statement_subtype;
    long long m_submit_utc_ts;
    long long m_compile_start_utc_ts;
    long long m_compile_end_utc_ts;
    long long m_compile_elapsed_time;
    long m_cmp_affinity_num;
    long m_cmp_dop;
    long m_cmp_txn_needed;
    long m_cmp_mandatory_x_prod;
    long m_cmp_missing_stats;
    long m_cmp_num_joins;
    long m_cmp_full_scan_on_table;
    double m_cmp_rows_accessed_full_scan;
    double m_est_accessed_rows;
    double m_est_used_rows;
    string m_cmp_compiler_id;
    long m_cmp_cpu_path_length;
    long m_cmp_cpu_binder;
    long m_cmp_cpu_normalizer;
    long m_cmp_cpu_analyzer;
    long m_cmp_cpu_optimizer;
    long m_cmp_cpu_generator;
    long m_cmp_metadata_cache_hits;
    long m_cmp_metadata_cache_lookups;
    long m_cmp_query_cache_status;
    long m_cmp_histogram_cache_hits;
    long m_cmp_histogram_cache_lookups;
    long m_cmp_stmt_heap_size;
    long m_cmp_context_heap_size;
    long m_cmp_optimization_tasks;
    long m_cmp_optimization_contexts;
    short m_cmp_is_recompile;
    double m_est_num_seq_ios;
    double m_est_num_rand_ios;
    double m_est_cost;
    double m_est_cardinality;
    double m_est_io_time;
    double m_est_msg_time;
    double m_est_idle_time;
    double m_est_cpu_time;
    double m_est_total_time;
    double m_est_total_mem;
    long m_est_resource_usage;
    string m_aggregation_option;
    int m_cmp_number_of_bmos;
    string m_cmp_overflow_mode;
    long m_cmp_overflow_size;
    long m_aggregate_total;
    int m_stats_error_code;
    long long m_query_elapsed_time;
    long long m_sql_process_busy_time;
    long long m_disk_process_busy_time;
    long m_disk_ios;
    long m_num_sql_processes;
    long m_sql_space_allocated;
    long m_sql_space_used;
    long m_sql_heap_allocated;
    long m_sql_heap_used;
    long m_total_mem_alloc;
    long m_max_mem_used;
    string m_transaction_id;
    long m_num_request_msgs;
    long m_num_request_msg_bytes;
    long m_num_reply_msgs;
    long m_num_reply_msg_bytes;
    long long m_first_result_return_utc_ts;
    long m_rows_returned_to_master;
    string m_parent_query_id;
    string m_parent_system_name;
    long long m_exec_end_utc_ts;
    long m_master_execution_time;
    long m_master_elapse_time;
    string m_query_status;
    string m_query_sub_status;
    int m_error_code;
    int m_sql_error_code;
    string m_error_text;
    string m_query_text;
    char *m_explain_plan;
    int m_explain_plan_len;
    int m_last_error_before_aqr;
    long m_delay_time_before_aqr_sec;
    long m_total_num_aqr_retries;
    long m_msg_bytes_to_disk;
    long m_msgs_to_disk;
    long m_rows_accessed;
    long m_rows_retrieved;
    long m_num_rows_iud;
    long m_processes_created;
    long m_process_create_busy_time;
    long m_ovf_file_count;
    long m_ovf_space_allocated;
    long m_ovf_space_used;
    long m_ovf_block_size;
    long m_ovf_write_read_count;
    long m_ovf_write_count;
    long m_ovf_buffer_blocks_written;
    long m_ovf_buffer_bytes_written;
    long m_ovf_read_count;
    long m_ovf_buffer_blocks_read;
    long m_ovf_buffer_bytes_read;
    long m_num_nodes;
    long long m_udr_process_busy_time;
    int m_pertable_stats;
}STATEMENT_QUERYEXECUTION, *pSTATEMENT_QUERYEXECUTION;

typedef struct _SESSION_AGGREGATION
{
    unsigned int m_instance_id;
    unsigned int m_tenant_id;
    unsigned int m_component_id;
    int m_process_id;
    unsigned int m_thread_id;
    unsigned int m_node_id;
    unsigned int m_pnid_id;
    unsigned int m_host_id;
    string m_ip_address_id;
    unsigned int m_sequence_number;
    string m_process_name;
    string m_sessionId;
    long long m_session_start_utc_ts;
    long long m_aggregation_last_update_utc_ts;
    long m_aggregation_last_elapsed_time;
    long m_user_id;
    string m_user_name;
    string m_role_name;
    string m_client_name;
    string m_client_user_name;
    string m_application_name;
    long m_total_est_rows_accessed;
    long m_total_est_rows_used;
    long m_total_rows_retrieved;
    long m_total_num_rows_iud;
    long m_total_selects;
    long m_total_inserts;
    long m_total_updates;
    long m_total_deletes;
    long m_total_ddl_stmts;
    long m_total_util_stmts;
    long m_total_catalog_stmts;
    long m_total_other_stmts;
    long m_delta_estimated_rows_accessed;
    long m_delta_estimated_rows_used;
    long m_delta_rows_accessed;
    long m_delta_rows_retrieved;
    long m_delta_num_rows_iud;
    long m_delta_total_selects;
    long m_delta_total_inserts;
    long m_delta_total_updates;
    long m_delta_total_deletes;
    long m_delta_total_ddl_stmts;
    long m_delta_total_util_stmts;
    long m_delta_total_catalog_stmts;
    long m_delta_total_other_stmts;
    long m_total_select_errors;
    long m_total_insert_errors;
    long m_total_update_errors;
    long m_total_delete_errors;
    long m_total_ddl_errors;
    long m_total_util_errors;
    long m_total_catalog_errors;
    long m_total_other_errors;
    long m_delta_select_errors;
    long m_delta_insert_errors;
    long m_delta_update_errors;
    long m_delta_delete_errors;
    long m_delta_ddl_errors;
    long m_delta_util_errors;
    long m_delta_catalog_errors;
    long m_delta_other_errors;
}SESSION_AGGREGATION, *pSESSION_AGGREGATION;

#endif
