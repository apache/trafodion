// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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

/*
 ********************************************************************************
 * 
 * File:         CmpSeabaseDDLrepos.h
 * Description:  This file describes repository tables used by Trafodion.
 *                
 * *****************************************************************************
 */

#ifndef _CMP_SEABASE_REPOS_H_
#define _CMP_SEABASE_REPOS_H_

#define TRAF_METRIC_QUERY_TABLE  "METRIC_QUERY_TABLE"
#define TRAF_METRIC_SESSION_TABLE  "METRIC_SESSION_TABLE"
#define TRAF_METRIC_QUERY_AGGR_TABLE  "METRIC_QUERY_AGGR_TABLE"

#define TRAF_METRIC_QUERY_VIEW "METRIC_QUERY_VIEW"

//----------------------------------------------------------------
//-- METRIC_QUERY_TABLE
//----------------------------------------------------------------
static const QString createMetricQueryTable[] =
{
{" create table %s.\"%s\"."TRAF_METRIC_QUERY_TABLE" "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable not serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable not serialized, "},
 {" component_id                           integer unsigned default null not serialized, "},
 {" process_id                             integer default null not serialized, "},
 {" thread_id                              integer unsigned default null not serialized, "},
 {" node_id                                integer unsigned default null not serialized, "},   
 {" pnid_id                                integer unsigned default null not serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable not serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null not serialized, "}, 
 {" sequence_number                        integer unsigned default null not serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null not serialized, "},
 {" exec_start_utc_ts                      timestamp(6) no default not null not droppable not serialized, "}, 
 {" query_id                               char(160) character set iso88591 casespecific no default not null not droppable not serialized, "},
 {" user_name                              char(256 bytes) character set utf8 casespecific default null not serialized, "},
 {" role_name                              char(256 bytes) character set utf8 casespecific default null not serialized, "},
 {" start_priority                         integer unsigned default null not serialized, "},
 {" master_process_id                      char(64) character set iso88591 casespecific       default null not serialized, "},
 {" session_id                             char(108) character set iso88591 casespecific      default null not serialized, "},
 {" client_name                            varchar(1024) character set iso88591 casespecific  default null not serialized, "},
 {" application_name                       char(130) character set iso88591 casespecific      default null not serialized, "},
 {" statement_id                           char(160) character set iso88591 casespecific      default null not serialized, "},
 {" statement_type                         char(36) character set iso88591 casespecific       default null not serialized, "},
 {" statement_subtype                      char(36) character set iso88591 casespecific       default null not serialized, "},
 {" submit_utc_ts				   timestamp(6) default null not serialized, "},
 {" compile_start_utc_ts			   timestamp(6) default null not serialized, "},
 {" compile_end_utc_ts			   timestamp(6) default null not serialized, "},
 {" compile_elapsed_time			   largeint default null not serialized, "},
 {" cmp_affinity_num				   largeint default null not serialized, "},
 {" cmp_dop					   largeint default null not serialized, "},
 {" cmp_txn_needed				   largeint default null not serialized, "},
 {" cmp_mandatory_x_prod			   largeint default null not serialized, "},
 {" cmp_missing_stats				   largeint default null not serialized, "},
 {" cmp_num_joins				   largeint default null not serialized, "},
 {" cmp_full_scan_on_table			   largeint default null not serialized, "},
 {" cmp_rows_accessed_full_scan		   double precision default null not serialized, "},
 {" est_accessed_rows       		   double precision default null not serialized, "}, 
 {" est_used_rows				   double precision default null not serialized, "}, 
 {" cmp_compiler_id				   char(28) character set iso88591 casespecific default null not serialized, "},
 {" cmp_cpu_path_length			   largeint default null not serialized, "},
 {" cmp_cpu_binder				   largeint default null not serialized, "},
 {" cmp_cpu_normalizer			   largeint default null not serialized, "},
 {" cmp_cpu_analyzer				   largeint default null not serialized, "},
 {" cmp_cpu_optimizer				   largeint default null not serialized, "},
 {" cmp_cpu_generator				   largeint default null not serialized, "},
 {" cmp_metadata_cache_hits			   largeint default null not serialized, "},
 {" cmp_metadata_cache_lookups		   largeint default null not serialized, "},
 {" cmp_query_cache_status	largeint                    default null not serialized, "},
 {" cmp_histogram_cache_hits	largeint                default null not serialized, "},
 {" cmp_histogram_cache_lookups	largeint                default null not serialized, "},
 {" cmp_stmt_heap_size	largeint                        default null not serialized, "},
 {" cmp_context_heap_size	largeint                    default null not serialized, "},
 {" cmp_optimization_tasks	largeint                    default null not serialized, "},
 {" cmp_optimization_contexts	largeint                default null not serialized, "},
 {" cmp_is_recompile	smallint                        default null not serialized, "},
 {" est_num_seq_ios	double precision                  	default null not serialized, "},
 {" est_num_rand_ios	double precision                default null not serialized, "},
 {" est_cost	double precision                  		default null not serialized, "},
 {" est_cardinality	double precision                  	default null not serialized, "},
 {" est_io_time	double precision                  		default null not serialized, "},
 {" est_msg_time	double precision                  	default null not serialized, "},
 {" est_idle_time	double precision                  	default null not serialized, "},
 {" est_cpu_time	double precision                  	default null not serialized, "},
 {" est_total_time	double precision                  	default null not serialized, "},
 {" est_total_mem	double precision                  	default null not serialized, "},
 {" est_resource_usage	largeint                        default null not serialized, "},
 {" aggregate_option	char(3)   character set iso88591 casespecific    	default null not serialized, "},
 {" cmp_number_of_bmos	integer                         default null not serialized, "},
 {" cmp_overflow_mode	char(10) character set iso88591 casespecific      	default null not serialized, "},
 {" cmp_overflow_size	largeint                        default null not serialized, "},
 {" aggregate_total     largeint                        default null not serialized, "},
 {" stats_error_code                       integer                           default null not serialized, "},
 {" query_elapsed_time                     largeint                          default null not serialized, "},
 {" sql_process_busy_time                  largeint                          default null not serialized, "},
 {" disk_process_busy_time                 largeint                          default null not serialized, "},
 {" disk_ios                               largeint                          default null not serialized, "},
 {" num_sql_processes                      largeint                          default null not serialized, "},
 {" sql_space_allocated                    largeint                          default null not serialized, "},
 {" sql_space_used                         largeint                          default null not serialized, "},
 {" sql_heap_allocated                     largeint                          default null not serialized, "},
 {" sql_heap_used                          largeint                          default null not serialized, "},
 {" total_mem_alloc                        largeint                          default null not serialized, "},
 {" max_mem_used                           largeint                          default null not serialized, "},
 {" transaction_id                         char(25) character set iso88591 casespecific       default null not serialized, "},
 {" num_request_msgs                       largeint                          default null not serialized, "},
 {" num_request_msg_bytes                  largeint                          default null not serialized, "},
 {" num_reply_msgs                         largeint                          default null not serialized, "},
 {" num_reply_msg_bytes                    largeint                          default null not serialized, "},
 {" first_result_return_utc_ts             timestamp(6)                      default null not serialized, "},
 {" rows_returned_to_master                largeint                          default null not serialized, "},
 {" parent_query_id                        char(160) character set iso88591 casespecific      default null not serialized, "},
 {" parent_system_name                     char(128) character set iso88591 casespecific      default null not serialized, "},
 {" exec_end_utc_ts                        timestamp(6)                      default null not serialized, "},
 {" master_execution_time                  largeint                          default null not serialized, "},
 {" master_elapsed_time                    largeint                          default null not serialized, "},
 {" error_code                             integer                           default null not serialized, "},
 {" sql_error_code                         integer                           default null not serialized, "},
 {" error_text                             varchar(2000) character set utf8 casespecific default null not serialized, "},
 {" query_text                             varchar(50000) character set utf8 casespecific default null not serialized, "},
 {" explain_plan                           varchar(50000) character set utf8 casespecific default null not serialized, "},
 {" last_error_before_aqr                  integer                           default null not serialized, "},
 {" delay_time_before_aqr_sec              largeint                          default null not serialized, "},
 {" total_num_aqr_retries                  largeint                          default null not serialized, "},
 {" msg_bytes_to_disk                      largeint                          default null not serialized, "},
 {" msgs_to_disk                           largeint                          default null not serialized, "},
 {" rows_accessed                          largeint                          default null not serialized, "},
 {" rows_retrieved                         largeint                          default null not serialized, "},
 {" num_rows_iud                           largeint                          default null not serialized, "},
 {" processes_created                      largeint                          default null not serialized, "},
 {" process_create_busy_time               largeint                          default null not serialized, "},
 {" ovf_file_count                         largeint                          default null not serialized, "},
 {" ovf_space_allocated                    largeint                          default null not serialized, "},
 {" ovf_space_used                         largeint                          default null not serialized, "},
 {" ovf_block_size                         largeint                          default null not serialized, "},
 {" ovf_write_read_count                   largeint                          default null not serialized, "},
 {" ovf_write_count                        largeint                          default null not serialized, "},
 {" ovf_buffer_blocks_written              largeint                          default null not serialized, "},
 {" ovf_buffer_bytes_written               largeint                          default null not serialized, "},
 {" ovf_read_count                         largeint                          default null not serialized, "},
 {" ovf_buffer_blocks_read                 largeint                          default null not serialized, "},
 {" ovf_buffer_bytes_read                  largeint                          default null not serialized, "},
 {" num_nodes                              largeint                          default null not serialized, "},
 {" udr_process_busy_time                  largeint                          default null not serialized, "},
 {" pertable_stats                         integer                           default null not serialized "},
 {" "},
 {" ) "},
 {" primary key ( exec_start_utc_ts, query_id ) "},
 // {" hash partition by ( exec_start_utc_ts )
 {" ; "}
 };

//----------------------------------------------------------------
// -- METRIC_SESSION_TABLE
// ----------------------------------------------------------------
static const QString createMetricSessionTable[] =
{
 {" create table %s.\"%s\"."TRAF_METRIC_SESSION_TABLE"  "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable not serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable not serialized, "},
 {" component_id                           integer unsigned default null not serialized, "},
 {" process_id                             integer default null not serialized, "},
 {" thread_id                              integer unsigned default null not serialized, "},
 {" node_id                                integer unsigned default null not serialized, "},   
 {" pnid_id                                integer unsigned default null not serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable not serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null not serialized, "}, 
 {" sequence_number                        integer unsigned default null not serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null not serialized, "},
 {" session_id                             char(108)     character set iso88591 casespecific no default not null not droppable not serialized, "},
 {" session_status                         char(5)       character set iso88591 casespecific  default null not serialized, "},
 {" session_start_utc_ts                   timestamp(6)                      default null not serialized, "},
 {" session_end_utc_ts                     timestamp(6)                      default null not serialized, "},
 {" user_id                                largeint                          default null not serialized, "},
 {" user_name                              char(256 bytes)     character set utf8 casespecific  default null not serialized, "},
 {" role_name                              char(256 bytes)     character set utf8 casespecific  default null not serialized, "},
 {" client_name                            varchar(256) character set utf8 casespecific  default null not serialized, "},
 {" client_user_name                       char(256 bytes)     character set utf8 casespecific  default null not serialized, "},
 {" application_name                       char(130)     character set iso88591 casespecific  default null not serialized, "},
 {" total_execution_time                   largeint                          default null not serialized, "},
 {" total_elapsed_time                     largeint                          default null not serialized, "},
 {" total_insert_stmts_executed            largeint                          default null not serialized, "},
 {" total_delete_stmts_executed            largeint                          default null not serialized, "},
 {" total_update_stmts_executed            largeint                          default null not serialized, "},
 {" total_select_stmts_executed            largeint                          default null not serialized, "},
 {" total_catalog_stmts                    largeint                          default null not serialized, "},     
 {" total_prepares                         largeint                          default null not serialized, "},
 {" total_executes                         largeint                          default null not serialized, "},
 {" total_fetches                          largeint                          default null not serialized, "},
 {" total_closes                           largeint                          default null not serialized, "},
 {" total_execdirects                      largeint                          default null not serialized, "},
 {" total_errors                           largeint                          default null not serialized, "},
 {" total_warnings                         largeint                          default null not serialized, "},
 {" total_login_elapsed_time_mcsec         largeint                          default null not serialized, "},
 {" ldap_login_elapsed_time_mcsec          largeint                          default null not serialized, "},
 {" sql_user_elapsed_time_mcsec            largeint                          default null not serialized, "},
 {" search_connection_elapsed_time_mcsec   largeint                          default null not serialized, "},
 {" search_elapsed_time_mcsec              largeint                          default null not serialized, "},
 {" authentication_connection_elapsed_time_mcsec     largeint                default null not serialized, "},
 {" authentication_elapsed_time_mcsec      largeint                          default null not serialized "},
 {" ) "},
 {" primary key ( session_id ) salt using 8 partitions "},
 {" ; "},
 };

 //----------------------------------------------------------------
 //- METRIC_QUERY_AGGREGATION_TABLE
 //----------------------------------------------------------------
static const QString createMetricQueryAggrTable[] =
{
 {" create table %s.\"%s\"."TRAF_METRIC_QUERY_AGGR_TABLE"  "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable not serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable not serialized, "},
 {" component_id                           integer unsigned default null not serialized, "},
 {" process_id                             integer default null not serialized, "},
 {" thread_id                              integer unsigned default null not serialized, "},
 {" node_id                                integer unsigned default null not serialized, "},   
 {" pnid_id                                integer unsigned default null not serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable not serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null not serialized, "}, 
 {" sequence_number                        integer unsigned default null not serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null not serialized, "},
 {" session_id                             char(108)     character set iso88591 casespecific no default not null not droppable not serialized, "},
 {" aggregation_start_utc_ts               timestamp(6) no default not null not droppable not serialized, "},
 {" aggregation_end_utc_ts                 timestamp(6)                      default null not serialized, "},
 {" user_id                                largeint                          default null not serialized, "},
 {" user_name                              char(256 bytes)     character set utf8 casespecific  default null not serialized, "},
 {" role_name                              char(256 bytes)     character set utf8 casespecific  default null not serialized, "},
 {" client_name                            varchar(256) character set utf8 casespecific  default null not serialized, "},
 {" client_user_name                       char(256 bytes)     character set utf8 casespecific  default null not serialized, "},
 {" application_name                       char(130)     character set iso88591 casespecific  default null not serialized, "},
 {" total_est_rows_accessed			   largeint default null not serialized, "},
 {" total_est_rows_used			   largeint default null not serialized, "},
 {" total_rows_retrieved			   largeint default null not serialized, "},
 {" total_num_rows_iud			   largeint default null not serialized, "},
 {" total_selects			   	   largeint default null not serialized, "},
 {" total_inserts			   largeint default null not serialized, "},
 {" total_updates			   largeint default null not serialized, "},
 {" total_deletes			   largeint default null not serialized, "},
 {" delta_estimated_rows_accessed			   largeint default null not serialized, "},
 {" delta_estimated_rows_used			   largeint default null not serialized, "},
 {" delta_rows_accessed			   largeint default null not serialized, "},
 {" delta_rows_retrieved			   largeint default null not serialized, "},
 {" delta_num_rows_uid			   largeint default null not serialized, "},
 {" delta_total_selects			   largeint default null not serialized, "},
 {" delta_total_inserts			   largeint default null not serialized, "},
 {" delta_total_updates			   largeint default null not serialized, "},
 {" delta_total_deletes			   largeint default null not serialized "},
 {" ) "},
 {" primary key ( aggregation_start_utc_ts, session_id ) salt using 8 partitions "},
 {" ; "}
};

struct ReposTableInfo
{
  const char * tableName;

  const QString *tableDefnQuery;

  Lng32 sizeOfDefnArr;
};
 
static const ReposTableInfo allReposTablesInfo[] = {
  {
    TRAF_METRIC_QUERY_TABLE,
    createMetricQueryTable,
    sizeof(createMetricQueryTable)
  },
  {
    TRAF_METRIC_SESSION_TABLE,
    createMetricSessionTable,
    sizeof(createMetricSessionTable)
  },
  {
    TRAF_METRIC_QUERY_AGGR_TABLE,
    createMetricQueryAggrTable,
    sizeof(createMetricQueryAggrTable)
  }
};

#endif

