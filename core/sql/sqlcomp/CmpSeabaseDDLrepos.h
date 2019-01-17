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
#include "CmpSeabaseDDLupgrade.h"
#define TRAF_METRIC_QUERY_VIEW "METRIC_QUERY_VIEW"

///////////////////////////////////////////////////////////////////////////////
// *** Current Definition ***
//
// Current repository tables definition for Metadata Version 2.1
//  (Major version = 2, Minor version = 1)
///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------
//-- METRIC_QUERY_TABLE
//----------------------------------------------------------------
static const QString createMetricQueryTable[] =
{
{" create table %s.\"%s\"." REPOS_METRIC_QUERY_TABLE" "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable serialized, "},
 {" component_id                           integer unsigned default null serialized, "},
 {" process_id                             integer default null serialized, "},
 {" thread_id                              integer unsigned default null serialized, "},
 {" node_id                                integer unsigned default null serialized, "},   
 {" pnid_id                                integer unsigned default null serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null serialized, "}, 
 {" sequence_number                        integer unsigned default null serialized, "},             
 {" process_name                           char(32) character set iso88591 casespecific default null serialized, "},
 {" exec_start_utc_ts                      timestamp(6) no default not null not droppable not serialized, "}, 
 {" query_id                               char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific no default not null not droppable serialized, "},
 {" query_signature_id                     char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific default null serialized, "},
 {" user_name                              char(256 bytes) character set utf8 casespecific default null serialized, "},
 {" role_name                              char(256 bytes) character set utf8 casespecific default null serialized, "},
 {" start_priority                         integer unsigned default null serialized, "},
 {" master_process_id                      char(64) character set iso88591 casespecific       default null serialized, "},
 {" session_id                             char(108) character set iso88591 casespecific      default null serialized, "},
 {" client_name                            varchar(1024) character set iso88591 casespecific  default null serialized, "},
 {" application_name                       char(130) character set iso88591 casespecific      default null serialized, "},
 {" statement_id                           char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific      default null serialized, "},
 {" statement_type                         char(36) character set iso88591 casespecific       default null serialized, "},
 {" statement_subtype                      char(36) character set iso88591 casespecific       default null serialized, "},
 {" submit_utc_ts				   timestamp(6) default null not serialized, "},
 {" compile_start_utc_ts			   timestamp(6) default null not serialized, "},
 {" compile_end_utc_ts			   timestamp(6) default null not serialized, "},
 {" compile_elapsed_time			   largeint default null serialized, "},
 {" cmp_affinity_num				   largeint default null serialized, "},
 {" cmp_dop					   largeint default null serialized, "},
 {" cmp_txn_needed				   largeint default null serialized, "},
 {" cmp_mandatory_x_prod			   largeint default null serialized, "},
 {" cmp_missing_stats				   largeint default null serialized, "},
 {" cmp_num_joins				   largeint default null serialized, "},
 {" cmp_full_scan_on_table			   largeint default null serialized, "},
 {" cmp_rows_accessed_full_scan		   double precision default null not serialized, "},
 {" est_accessed_rows       		   double precision default null not serialized, "}, 
 {" est_used_rows				   double precision default null not serialized, "}, 
 {" cmp_compiler_id				   char(28) character set iso88591 casespecific default null serialized, "},
 {" cmp_cpu_path_length			   largeint default null serialized, "},
 {" cmp_cpu_binder				   largeint default null serialized, "},
 {" cmp_cpu_normalizer			   largeint default null serialized, "},
 {" cmp_cpu_analyzer				   largeint default null serialized, "},
 {" cmp_cpu_optimizer				   largeint default null serialized, "},
 {" cmp_cpu_generator				   largeint default null serialized, "},
 {" cmp_metadata_cache_hits			   largeint default null serialized, "},
 {" cmp_metadata_cache_lookups		   largeint default null serialized, "},
 {" cmp_query_cache_status	largeint                    default null serialized, "},
 {" cmp_histogram_cache_hits	largeint                default null serialized, "},
 {" cmp_histogram_cache_lookups	largeint                default null serialized, "},
 {" cmp_stmt_heap_size	largeint                        default null serialized, "},
 {" cmp_context_heap_size	largeint                    default null serialized, "},
 {" cmp_optimization_tasks	largeint                    default null serialized, "},
 {" cmp_optimization_contexts	largeint                default null serialized, "},
 {" cmp_is_recompile	smallint                        default null serialized, "},
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
 {" est_resource_usage	largeint                        default null serialized, "},
 {" aggregate_option	char(3)   character set iso88591 casespecific    	default null serialized, "},
 {" cmp_number_of_bmos	integer                         default null serialized, "},
 {" cmp_overflow_mode	char(10) character set iso88591 casespecific      	default null serialized, "},
 {" cmp_overflow_size	largeint                        default null serialized, "},
 {" aggregate_total     largeint                        default null serialized, "},
 {" stats_error_code                       integer                           default null serialized, "},
 {" query_elapsed_time                     largeint                          default null serialized, "},
 {" sql_process_busy_time                  largeint                          default null serialized, "},
 {" disk_process_busy_time                 largeint                          default null serialized, "},
 {" disk_ios                               largeint                          default null serialized, "},
 {" num_sql_processes                      largeint                          default null serialized, "},
 {" sql_space_allocated                    largeint                          default null serialized, "},
 {" sql_space_used                         largeint                          default null serialized, "},
 {" sql_heap_allocated                     largeint                          default null serialized, "},
 {" sql_heap_used                          largeint                          default null serialized, "},
 {" total_mem_alloc                        largeint                          default null serialized, "},
 {" max_mem_used                           largeint                          default null serialized, "},
 {" transaction_id                         char(25) character set iso88591 casespecific       default null serialized, "},
 {" num_request_msgs                       largeint                          default null serialized, "},
 {" num_request_msg_bytes                  largeint                          default null serialized, "},
 {" num_reply_msgs                         largeint                          default null serialized, "},
 {" num_reply_msg_bytes                    largeint                          default null serialized, "},
 {" first_result_return_utc_ts             timestamp(6)                      default null not serialized, "},
 {" rows_returned_to_master                largeint                          default null serialized, "},
 {" parent_query_id                        char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific      default null serialized, "},
 {" parent_system_name                     char(128) character set iso88591 casespecific      default null serialized, "},
 {" exec_end_utc_ts                        timestamp(6)                      default null not serialized, "},
 {" master_execution_time                  largeint                          default null serialized, "},
 {" master_elapsed_time                    largeint                          default null serialized, "},
{" query_status                            char(21) character set utf8 casespecific default null serialized, "},
 {" query_sub_status                       char(30) character set utf8 casespecific default null serialized, "},
 {" error_code                             integer                           default null serialized, "},
 {" sql_error_code                         integer                           default null serialized, "},
 {" error_text                             varchar(2000) character set utf8 casespecific default null serialized, "},
 {" query_text                             varchar(50000) character set utf8 casespecific default null serialized, "},
 {" explain_plan                           varchar(" REPOS_MAX_EXPLAIN_PLAN_LEN_STR") character set iso88591 casespecific default null serialized, "},
 {" last_error_before_aqr                  integer                           default null serialized, "},
 {" delay_time_before_aqr_sec              largeint                          default null serialized, "},
 {" total_num_aqr_retries                  largeint                          default null serialized, "},
 {" msg_bytes_to_disk                      largeint                          default null serialized, "},
 {" msgs_to_disk                           largeint                          default null serialized, "},
 {" rows_accessed                          largeint                          default null serialized, "},
 {" rows_retrieved                         largeint                          default null serialized, "},
 {" num_rows_iud                           largeint                          default null serialized, "},
 {" processes_created                      largeint                          default null serialized, "},
 {" process_create_busy_time               largeint                          default null serialized, "},
 {" ovf_file_count                         largeint                          default null serialized, "},
 {" ovf_space_allocated                    largeint                          default null serialized, "},
 {" ovf_space_used                         largeint                          default null serialized, "},
 {" ovf_block_size                         largeint                          default null serialized, "},
 {" ovf_write_read_count                   largeint                          default null serialized, "},
 {" ovf_write_count                        largeint                          default null serialized, "},
 {" ovf_buffer_blocks_written              largeint                          default null serialized, "},
 {" ovf_buffer_bytes_written               largeint                          default null serialized, "},
 {" ovf_read_count                         largeint                          default null serialized, "},
 {" ovf_buffer_blocks_read                 largeint                          default null serialized, "},
 {" ovf_buffer_bytes_read                  largeint                          default null serialized, "},
 {" num_nodes                              largeint                          default null serialized, "},
 {" udr_process_busy_time                  largeint                          default null serialized, "},
 {" pertable_stats                         integer                           default null serialized, "},
 {" last_updated_time                      timestamp(6)                      default null not serialized"},
 {" "},
 {" ) "},
 {" primary key ( exec_start_utc_ts, query_id ) salt using 8 partitions on ( query_id ) "},
 {" hbase_options ( DATA_BLOCK_ENCODING = 'FAST_DIFF', COMPRESSION = 'GZ' ) "},
 {" attribute hbase format "},
 {" ; "}
 };

//----------------------------------------------------------------
// -- METRIC_SESSION_TABLE
// ----------------------------------------------------------------
static const QString createMetricSessionTable[] =
{
 {" create table %s.\"%s\"." REPOS_METRIC_SESSION_TABLE"  "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable serialized, "},
 {" component_id                           integer unsigned default null serialized, "},
 {" process_id                             integer default null serialized, "},
 {" thread_id                              integer unsigned default null serialized, "},
 {" node_id                                integer unsigned default null serialized, "},   
 {" pnid_id                                integer unsigned default null serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null serialized, "}, 
 {" sequence_number                        integer unsigned default null serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null serialized, "},
 {" session_id                             char(108)     character set iso88591 casespecific no default not null not droppable serialized, "},
 {" session_status                         char(5)       character set iso88591 casespecific  default null serialized, "},
 {" session_start_utc_ts               timestamp(6) no default not null not droppable not serialized, "},
 {" session_end_utc_ts                     timestamp(6)                      default null not serialized, "},
 {" user_id                                largeint                          default null serialized, "},
 {" user_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" role_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" client_name                            varchar(256) character set utf8 casespecific  default null serialized, "},
 {" client_user_name                       char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" application_name                       char(130)     character set iso88591 casespecific  default null serialized, "},
 {" profile_name                           varchar(130) character set utf8 casespecific  default null serialized, "},
 {" sla_name                               varchar(130) character set utf8 casespecific  default null serialized, "},
 {" total_execution_time                   largeint                          default null serialized, "},
 {" total_elapsed_time                     largeint                          default null serialized, "},
 {" total_insert_stmts_executed            largeint                          default null serialized, "},
 {" total_delete_stmts_executed            largeint                          default null serialized, "},
 {" total_update_stmts_executed            largeint                          default null serialized, "},
 {" total_select_stmts_executed            largeint                          default null serialized, "},
 {" total_catalog_stmts                    largeint                          default null serialized, "},     
 {" total_prepares                         largeint                          default null serialized, "},
 {" total_executes                         largeint                          default null serialized, "},
 {" total_fetches                          largeint                          default null serialized, "},
 {" total_closes                           largeint                          default null serialized, "},
 {" total_execdirects                      largeint                          default null serialized, "},
 {" total_errors                           largeint                          default null serialized, "},
 {" total_warnings                         largeint                          default null serialized, "},
 {" total_login_elapsed_time_mcsec         largeint                          default null serialized, "},
 {" ldap_login_elapsed_time_mcsec          largeint                          default null serialized, "},
 {" sql_user_elapsed_time_mcsec            largeint                          default null serialized, "},
 {" search_connection_elapsed_time_mcsec   largeint                          default null serialized, "},
 {" search_elapsed_time_mcsec              largeint                          default null serialized, "},
 {" authentication_connection_elapsed_time_mcsec     largeint                default null serialized, "},
 {" authentication_elapsed_time_mcsec      largeint                          default null serialized "},
 {" ) "},
 {" primary key (session_start_utc_ts, session_id ) salt using 8 partitions on ( session_id ) "},
 {" hbase_options ( DATA_BLOCK_ENCODING = 'FAST_DIFF', COMPRESSION = 'GZ' ) "},
 {" attribute hbase format "},
 {" ; "},
 };

 //----------------------------------------------------------------
 //- METRIC_QUERY_AGGREGATION_TABLE
 //----------------------------------------------------------------
static const QString createMetricQueryAggrTable[] =
{
 {" create table %s.\"%s\"." REPOS_METRIC_QUERY_AGGR_TABLE"  "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable serialized, "},
 {" component_id                           integer unsigned default null serialized, "},
 {" process_id                             integer default null serialized, "},
 {" thread_id                              integer unsigned default null serialized, "},
 {" node_id                                integer unsigned default null serialized, "},   
 {" pnid_id                                integer unsigned default null serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null serialized, "}, 
 {" sequence_number                        integer unsigned default null serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null serialized, "},
 {" session_id                             char(108)     character set iso88591 casespecific no default not null not droppable serialized, "},
 {" session_start_utc_ts                   timestamp(6) no default not null not droppable not serialized, "},
 {" aggregation_last_update_utc_ts         timestamp(6)                      default null not serialized, "},
 {" aggregation_last_elapsed_time          largeint                          default null serialized, "},
 {" user_id                                largeint                          default null serialized, "},
 {" user_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" role_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" client_name                            varchar(256) character set utf8 casespecific  default null serialized, "},
 {" client_user_name                       char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" application_name                       char(130)     character set iso88591 casespecific  default null serialized, "},
 {" total_est_rows_accessed		   largeint default null serialized, "},
 {" total_est_rows_used			   largeint default null serialized, "},
 {" total_rows_retrieved		   largeint default null serialized, "},
 {" total_num_rows_iud			   largeint default null serialized, "},
 {" total_selects			   largeint default null serialized, "},
 {" total_inserts			   largeint default null serialized, "},
 {" total_updates			   largeint default null serialized, "},
 {" total_deletes			   largeint default null serialized, "},
 {" total_ddl_stmts			   largeint default null serialized, "},
 {" total_util_stmts			   largeint default null serialized, "},
 {" total_catalog_stmts			   largeint default null serialized, "},
 {" total_other_stmts			   largeint default null serialized, "},
 {" total_insert_errors			   largeint default null serialized, "},
 {" total_delete_errors			   largeint default null serialized, "},
 {" total_update_errors			   largeint default null serialized, "},
 {" total_select_errors			   largeint default null serialized, "},
 {" total_ddl_errors			   largeint default null serialized, "},
 {" total_util_errors			   largeint default null serialized, "},
 {" total_catalog_errors		   largeint default null serialized, "},
 {" total_other_errors			   largeint default null serialized, "},
 {" delta_estimated_rows_accessed	   largeint default null serialized, "},
 {" delta_estimated_rows_used		   largeint default null serialized, "},
 {" delta_rows_accessed			   largeint default null serialized, "},
 {" delta_rows_retrieved	           largeint default null serialized, "},
 {" delta_num_rows_iud			   largeint default null serialized, "},
 {" delta_selects			   largeint default null serialized, "},
 {" delta_inserts			   largeint default null serialized, "},
 {" delta_updates			   largeint default null serialized, "},
 {" delta_deletes			   largeint default null serialized, "},
 {" delta_ddl_stmts			   largeint default null serialized, "},
 {" delta_util_stmts			   largeint default null serialized, "},
 {" delta_catalog_stmts			   largeint default null serialized, "},
 {" delta_other_stmts			   largeint default null serialized, "},
 {" delta_insert_errors			   largeint default null serialized, "},
 {" delta_delete_errors			   largeint default null serialized, "},
 {" delta_update_errors			   largeint default null serialized, "},
 {" delta_select_errors			   largeint default null serialized, "},
 {" delta_ddl_errors			   largeint default null serialized, "},
 {" delta_util_errors			   largeint default null serialized, "},
 {" delta_catalog_errors		   largeint default null serialized, "},
 {" delta_other_errors			   largeint default null serialized, "},
 {" average_response_time                  largeint default null serialized, "},
 {" throughput_per_second                  largeint default null serialized "},
 {" ) "},
 {" primary key ( session_start_utc_ts, session_id ) salt using 8 partitions on ( session_id ) "},
 {" hbase_options ( DATA_BLOCK_ENCODING = 'FAST_DIFF', COMPRESSION = 'GZ' ) "},
 {" attribute hbase format "},
 {" ; "}
};

//----------------------------------------------------------------
//-- METRIC_TEXT_TABLE
//----------------------------------------------------------------
static const QString createMetricTextTable[] =
  {
    {" create table %s.\"%s\"." REPOS_METRIC_TEXT_TABLE" "},
    {" ( "},
    {"  query_id     char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific no default not null not droppable serialized, "},
    {"   text_type int not null serialized, "},
    {"   sub_id int not null serialized, "},
    {"   seq_num int not null serialized, "},
    {"   flags largeint not null serialized, "},
    {"   text varchar(50000 bytes) character set iso88591 not null serialized "},
    {" ) "},
    {" primary key (query_id, text_type, sub_id, seq_num) "},
    {" attribute hbase format "},
    {" ; "}
  };


/////////////////////////////////////////////////////////////////////
//
// Information about changed old metadata tables from which upgrade
// is being done to the current version.
// These definitions have changed in the current version of code.
// 
// Old definitions have the form (for ex for METRIC_QUERY_TABLE table):
//            createOldTrafv??MetricQueryTable[]
// v?? is the old version.
//
// When definitions change, make new entries between
// START_OLD_MD_v?? and END_OLD_MD_v??.
// Do not remove older entries. We want to keep them around for
// historical purpose.
//
// Change entries in allReposUpgradeInfo[] struct in this file
// to reflect the 'old' repository tables.
//
//////////////////////////////////////////////////////////////////////

#define TRAF_METRIC_QUERY_TABLE_OLD_REPOS REPOS_METRIC_QUERY_TABLE"_OLD_REPOS"
#define TRAF_METRIC_SESSION_TABLE_OLD_REPOS REPOS_METRIC_SESSION_TABLE"_OLD_REPOS"
#define TRAF_METRIC_QUERY_AGGR_TABLE_OLD_REPOS REPOS_METRIC_QUERY_AGGR_TABLE"_OLD_REPOS"

//----------------------------------------------------------------
//-- METRIC_QUERY_TABLE
//----------------------------------------------------------------
static const QString createOldTrafv11MetricQueryTable[] =
{
{" create table %s.\"%s\"." TRAF_METRIC_QUERY_TABLE_OLD_REPOS" "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable serialized, "},
 {" component_id                           integer unsigned default null serialized, "},
 {" process_id                             integer default null serialized, "},
 {" thread_id                              integer unsigned default null serialized, "},
 {" node_id                                integer unsigned default null serialized, "},   
 {" pnid_id                                integer unsigned default null serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null serialized, "}, 
 {" sequence_number                        integer unsigned default null serialized, "},             
 {" process_name                           char(32) character set iso88591 casespecific default null serialized, "},
 {" exec_start_utc_ts                      timestamp(6) no default not null not droppable not serialized, "}, 
 {" query_id                               char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific no default not null not droppable serialized, "},
 {" user_name                              char(256 bytes) character set utf8 casespecific default null serialized, "},
 {" role_name                              char(256 bytes) character set utf8 casespecific default null serialized, "},
 {" start_priority                         integer unsigned default null serialized, "},
 {" master_process_id                      char(64) character set iso88591 casespecific       default null serialized, "},
 {" session_id                             char(108) character set iso88591 casespecific      default null serialized, "},
 {" client_name                            varchar(1024) character set iso88591 casespecific  default null serialized, "},
 {" application_name                       char(130) character set iso88591 casespecific      default null serialized, "},
 {" statement_id                           char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific      default null serialized, "},
 {" statement_type                         char(36) character set iso88591 casespecific       default null serialized, "},
 {" statement_subtype                      char(36) character set iso88591 casespecific       default null serialized, "},
 {" submit_utc_ts				   timestamp(6) default null not serialized, "},
 {" compile_start_utc_ts			   timestamp(6) default null not serialized, "},
 {" compile_end_utc_ts			   timestamp(6) default null not serialized, "},
 {" compile_elapsed_time			   largeint default null serialized, "},
 {" cmp_affinity_num				   largeint default null serialized, "},
 {" cmp_dop					   largeint default null serialized, "},
 {" cmp_txn_needed				   largeint default null serialized, "},
 {" cmp_mandatory_x_prod			   largeint default null serialized, "},
 {" cmp_missing_stats				   largeint default null serialized, "},
 {" cmp_num_joins				   largeint default null serialized, "},
 {" cmp_full_scan_on_table			   largeint default null serialized, "},
 {" cmp_rows_accessed_full_scan		   double precision default null not serialized, "},
 {" est_accessed_rows       		   double precision default null not serialized, "}, 
 {" est_used_rows				   double precision default null not serialized, "}, 
 {" cmp_compiler_id				   char(28) character set iso88591 casespecific default null serialized, "},
 {" cmp_cpu_path_length			   largeint default null serialized, "},
 {" cmp_cpu_binder				   largeint default null serialized, "},
 {" cmp_cpu_normalizer			   largeint default null serialized, "},
 {" cmp_cpu_analyzer				   largeint default null serialized, "},
 {" cmp_cpu_optimizer				   largeint default null serialized, "},
 {" cmp_cpu_generator				   largeint default null serialized, "},
 {" cmp_metadata_cache_hits			   largeint default null serialized, "},
 {" cmp_metadata_cache_lookups		   largeint default null serialized, "},
 {" cmp_query_cache_status	largeint                    default null serialized, "},
 {" cmp_histogram_cache_hits	largeint                default null serialized, "},
 {" cmp_histogram_cache_lookups	largeint                default null serialized, "},
 {" cmp_stmt_heap_size	largeint                        default null serialized, "},
 {" cmp_context_heap_size	largeint                    default null serialized, "},
 {" cmp_optimization_tasks	largeint                    default null serialized, "},
 {" cmp_optimization_contexts	largeint                default null serialized, "},
 {" cmp_is_recompile	smallint                        default null serialized, "},
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
 {" est_resource_usage	largeint                        default null serialized, "},
 {" aggregate_option	char(3)   character set iso88591 casespecific    	default null serialized, "},
 {" cmp_number_of_bmos	integer                         default null serialized, "},
 {" cmp_overflow_mode	char(10) character set iso88591 casespecific      	default null serialized, "},
 {" cmp_overflow_size	largeint                        default null serialized, "},
 {" aggregate_total     largeint                        default null serialized, "},
 {" stats_error_code                       integer                           default null serialized, "},
 {" query_elapsed_time                     largeint                          default null serialized, "},
 {" sql_process_busy_time                  largeint                          default null serialized, "},
 {" disk_process_busy_time                 largeint                          default null serialized, "},
 {" disk_ios                               largeint                          default null serialized, "},
 {" num_sql_processes                      largeint                          default null serialized, "},
 {" sql_space_allocated                    largeint                          default null serialized, "},
 {" sql_space_used                         largeint                          default null serialized, "},
 {" sql_heap_allocated                     largeint                          default null serialized, "},
 {" sql_heap_used                          largeint                          default null serialized, "},
 {" total_mem_alloc                        largeint                          default null serialized, "},
 {" max_mem_used                           largeint                          default null serialized, "},
 {" transaction_id                         char(25) character set iso88591 casespecific       default null serialized, "},
 {" num_request_msgs                       largeint                          default null serialized, "},
 {" num_request_msg_bytes                  largeint                          default null serialized, "},
 {" num_reply_msgs                         largeint                          default null serialized, "},
 {" num_reply_msg_bytes                    largeint                          default null serialized, "},
 {" first_result_return_utc_ts             timestamp(6)                      default null not serialized, "},
 {" rows_returned_to_master                largeint                          default null serialized, "},
 {" parent_query_id                        char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific      default null serialized, "},
 {" parent_system_name                     char(128) character set iso88591 casespecific      default null serialized, "},
 {" exec_end_utc_ts                        timestamp(6)                      default null not serialized, "},
 {" master_execution_time                  largeint                          default null serialized, "},
 {" master_elapsed_time                    largeint                          default null serialized, "},
{" query_status                            char(21) character set utf8 casespecific default null serialized, "},
 {" query_sub_status                       char(30) character set utf8 casespecific default null serialized, "},
 {" error_code                             integer                           default null serialized, "},
 {" sql_error_code                         integer                           default null serialized, "},
 {" error_text                             varchar(2000) character set utf8 casespecific default null serialized, "},
 {" query_text                             varchar(50000) character set utf8 casespecific default null serialized, "},
 {" explain_plan                           varchar(200000) character set iso88591 casespecific default null serialized, "},
 {" last_error_before_aqr                  integer                           default null serialized, "},
 {" delay_time_before_aqr_sec              largeint                          default null serialized, "},
 {" total_num_aqr_retries                  largeint                          default null serialized, "},
 {" msg_bytes_to_disk                      largeint                          default null serialized, "},
 {" msgs_to_disk                           largeint                          default null serialized, "},
 {" rows_accessed                          largeint                          default null serialized, "},
 {" rows_retrieved                         largeint                          default null serialized, "},
 {" num_rows_iud                           largeint                          default null serialized, "},
 {" processes_created                      largeint                          default null serialized, "},
 {" process_create_busy_time               largeint                          default null serialized, "},
 {" ovf_file_count                         largeint                          default null serialized, "},
 {" ovf_space_allocated                    largeint                          default null serialized, "},
 {" ovf_space_used                         largeint                          default null serialized, "},
 {" ovf_block_size                         largeint                          default null serialized, "},
 {" ovf_write_read_count                   largeint                          default null serialized, "},
 {" ovf_write_count                        largeint                          default null serialized, "},
 {" ovf_buffer_blocks_written              largeint                          default null serialized, "},
 {" ovf_buffer_bytes_written               largeint                          default null serialized, "},
 {" ovf_read_count                         largeint                          default null serialized, "},
 {" ovf_buffer_blocks_read                 largeint                          default null serialized, "},
 {" ovf_buffer_bytes_read                  largeint                          default null serialized, "},
 {" num_nodes                              largeint                          default null serialized, "},
 {" udr_process_busy_time                  largeint                          default null serialized, "},
 {" pertable_stats                         integer                           default null serialized "},
 {" "},
 {" ) "},
 {" primary key ( exec_start_utc_ts, query_id ) salt using 8 partitions on ( query_id ) "},
 {" hbase_options ( DATA_BLOCK_ENCODING = 'FAST_DIFF', COMPRESSION = 'GZ' ) "},
 {" ; "}
 };

//----------------------------------------------------------------
// -- METRIC_SESSION_TABLE
// ----------------------------------------------------------------
static const QString createOldTrafv11MetricSessionTable[] =
{
 {" create table %s.\"%s\"." TRAF_METRIC_SESSION_TABLE_OLD_REPOS"  "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable serialized, "},
 {" component_id                           integer unsigned default null serialized, "},
 {" process_id                             integer default null serialized, "},
 {" thread_id                              integer unsigned default null serialized, "},
 {" node_id                                integer unsigned default null serialized, "},   
 {" pnid_id                                integer unsigned default null serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null serialized, "}, 
 {" sequence_number                        integer unsigned default null serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null serialized, "},
 {" session_id                             char(108)     character set iso88591 casespecific no default not null not droppable serialized, "},
 {" session_status                         char(5)       character set iso88591 casespecific  default null serialized, "},
 {" session_start_utc_ts               timestamp(6) no default not null not droppable not serialized, "},
 {" session_end_utc_ts                     timestamp(6)                      default null not serialized, "},
 {" user_id                                largeint                          default null serialized, "},
 {" user_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" role_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" client_name                            varchar(256) character set utf8 casespecific  default null serialized, "},
 {" client_user_name                       char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" application_name                       char(130)     character set iso88591 casespecific  default null serialized, "},
 {" total_execution_time                   largeint                          default null serialized, "},
 {" total_elapsed_time                     largeint                          default null serialized, "},
 {" total_insert_stmts_executed            largeint                          default null serialized, "},
 {" total_delete_stmts_executed            largeint                          default null serialized, "},
 {" total_update_stmts_executed            largeint                          default null serialized, "},
 {" total_select_stmts_executed            largeint                          default null serialized, "},
 {" total_catalog_stmts                    largeint                          default null serialized, "},     
 {" total_prepares                         largeint                          default null serialized, "},
 {" total_executes                         largeint                          default null serialized, "},
 {" total_fetches                          largeint                          default null serialized, "},
 {" total_closes                           largeint                          default null serialized, "},
 {" total_execdirects                      largeint                          default null serialized, "},
 {" total_errors                           largeint                          default null serialized, "},
 {" total_warnings                         largeint                          default null serialized, "},
 {" total_login_elapsed_time_mcsec         largeint                          default null serialized, "},
 {" ldap_login_elapsed_time_mcsec          largeint                          default null serialized, "},
 {" sql_user_elapsed_time_mcsec            largeint                          default null serialized, "},
 {" search_connection_elapsed_time_mcsec   largeint                          default null serialized, "},
 {" search_elapsed_time_mcsec              largeint                          default null serialized, "},
 {" authentication_connection_elapsed_time_mcsec     largeint                default null serialized, "},
 {" authentication_elapsed_time_mcsec      largeint                          default null serialized "},
 {" ) "},
{" primary key (session_start_utc_ts, session_id ) salt using 8 partitions on ( session_id ) "},
 {" hbase_options ( DATA_BLOCK_ENCODING = 'FAST_DIFF', COMPRESSION = 'GZ' ) "},
 {" ; "},
 };

 //----------------------------------------------------------------
 //- METRIC_QUERY_AGGREGATION_TABLE
 //----------------------------------------------------------------
static const QString createOldTrafv11MetricQueryAggrTable[] =
{
 {" create table %s.\"%s\"." TRAF_METRIC_QUERY_AGGR_TABLE_OLD_REPOS"  "},
 {" ( "},
 {" instance_id                            integer unsigned no default not null not droppable serialized, "},
 {" tenant_id                              integer unsigned no default not null not droppable serialized, "},
 {" component_id                           integer unsigned default null serialized, "},
 {" process_id                             integer default null serialized, "},
 {" thread_id                              integer unsigned default null serialized, "},
 {" node_id                                integer unsigned default null serialized, "},   
 {" pnid_id                                integer unsigned default null serialized, "}, 
 {" host_id                                integer unsigned no default not null not droppable serialized, "},
 {" ip_address_id                          char(32) character set iso88591 casespecific default null serialized, "}, 
 {" sequence_number                        integer unsigned default null serialized, "},                 
 {" process_name                           char(32) character set iso88591 casespecific default null serialized, "},
 {" session_id                             char(108)     character set iso88591 casespecific no default not null not droppable serialized, "},
 {" session_start_utc_ts               timestamp(6) no default not null not droppable not serialized, "},
 {" aggregation_last_update_utc_ts         timestamp(6)                      default null not serialized, "},
 {" aggregation_last_elapsed_time          largeint                          default null serialized, "},
 {" user_id                                largeint                          default null serialized, "},
 {" user_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" role_name                              char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" client_name                            varchar(256) character set utf8 casespecific  default null serialized, "},
 {" client_user_name                       char(256 bytes)     character set utf8 casespecific  default null serialized, "},
 {" application_name                       char(130)     character set iso88591 casespecific  default null serialized, "},
 {" total_est_rows_accessed			   largeint default null serialized, "},
 {" total_est_rows_used			   largeint default null serialized, "},
 {" total_rows_retrieved			   largeint default null serialized, "},
 {" total_num_rows_iud			   largeint default null serialized, "},
 {" total_selects			   	   largeint default null serialized, "},
 {" total_inserts			   largeint default null serialized, "},
 {" total_updates			   largeint default null serialized, "},
 {" total_deletes			   largeint default null serialized, "},

 {" total_ddl_stmts			   largeint default null serialized, "},
 {" total_util_stmts			   largeint default null serialized, "},
 {" total_catalog_stmts			   largeint default null serialized, "},
 {" total_other_stmts			   largeint default null serialized, "},

 {" total_insert_errors			          largeint default null serialized, "},
 {" total_delete_errors			          largeint default null serialized, "},
 {" total_update_errors			   largeint default null serialized, "},
 {" total_select_errors			   largeint default null serialized, "},

 {" total_ddl_errors			          largeint default null serialized, "},
 {" total_util_errors			          largeint default null serialized, "},
 {" total_catalog_errors			   largeint default null serialized, "},
 {" total_other_errors			   largeint default null serialized, "},

 {" delta_estimated_rows_accessed			   largeint default null serialized, "},
 {" delta_estimated_rows_used			   largeint default null serialized, "},
 {" delta_rows_accessed			   largeint default null serialized, "},
 {" delta_rows_retrieved			   largeint default null serialized, "},
 {" delta_num_rows_iud			   largeint default null serialized, "},

 {" delta_selects			   largeint default null serialized, "},
 {" delta_inserts			   largeint default null serialized, "},
 {" delta_updates			   largeint default null serialized, "},
 {" delta_deletes			   largeint default null serialized, "},

 {" delta_ddl_stmts			          largeint default null serialized, "},
 {" delta_util_stmts			          largeint default null serialized, "},
 {" delta_catalog_stmts			   largeint default null serialized, "},
 {" delta_other_stmts			   largeint default null serialized, "},

 {" delta_insert_errors			          largeint default null serialized, "},
 {" delta_delete_errors			          largeint default null serialized, "},
 {" delta_update_errors			   largeint default null serialized, "},
 {" delta_select_errors			   largeint default null serialized, "},

 {" delta_ddl_errors			          largeint default null serialized, "},
 {" delta_util_errors			          largeint default null serialized, "},
 {" delta_catalog_errors			   largeint default null serialized, "},
 {" delta_other_errors			   largeint default null serialized "},

 {" ) "},
 {" primary key ( session_start_utc_ts, session_id ) salt using 8 partitions on ( session_id ) "},
 {" hbase_options ( DATA_BLOCK_ENCODING = 'FAST_DIFF', COMPRESSION = 'GZ' ) "},
 {" ; "}
};

//----------------------------------------------------------------
//-- METRIC_TEXT_TABLE
//----------------------------------------------------------------
static const QString createOldTrafv11MetricTextTable[] =
  {
    {" create table %s.\"%s\"." REPOS_METRIC_TEXT_TABLE" "},  // table didn't change when moving from 1.1 to 2.1
    {" ( "},
    {"  query_id                               char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific no default not null not droppable serialized, "},
    {"   text_type int not null serialized, "},
    {"   sub_id int not null serialized, "},
    {"   seq_num int not null serialized, "},
    {"   flags largeint not null serialized, "},
    {"   text varchar(50000 bytes) character set iso88591 not null serialized "},
    {" ) "},
    {" primary key (query_id, text_type, sub_id, seq_num) "},
    {" ; "}
  };

// The following are the repository definitions for Metadata version 2.3 (before we 
// reset the numbering for Trafodion; 2.3 preceeds Trafodion 1.1). These are
// kept solely for historical purposes.

static const QString createOldv23ReposMetricQueryTable[] =
{
 {" create table %s.\"%s\"." TRAF_METRIC_QUERY_TABLE_OLD_REPOS" "},
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
 {" query_id                               char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific no default not null not droppable not serialized, "},
 {" user_name                              char(256 bytes) character set utf8 casespecific default null not serialized, "},
 {" role_name                              char(256 bytes) character set utf8 casespecific default null not serialized, "},
 {" start_priority                         integer unsigned default null not serialized, "},
 {" master_process_id                      char(64) character set iso88591 casespecific       default null not serialized, "},
 {" session_id                             char(108) character set iso88591 casespecific      default null not serialized, "},
 {" client_name                            varchar(1024) character set iso88591 casespecific  default null not serialized, "},
 {" application_name                       char(130) character set iso88591 casespecific      default null not serialized, "},
 {" statement_id                           char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific      default null not serialized, "},
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
 {" parent_query_id                        char(" MAX_QUERY_ID_LEN_STR") character set iso88591 casespecific      default null not serialized, "},
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
 {" ; "}
 };

static const QString createOldv23ReposMetricSessionTable[] =
{
 {" create table %s.\"%s\"." TRAF_METRIC_SESSION_TABLE_OLD_REPOS"  "},
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
 //- Old METRIC_QUERY_AGGREGATION_TABLE
 //----------------------------------------------------------------
static const QString createOldv23ReposMetricQueryAggrTable[] =
{
 {" create table %s.\"%s\"." TRAF_METRIC_QUERY_AGGR_TABLE_OLD_REPOS"  "},
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
static const MDUpgradeInfo allReposUpgradeInfo[] = {
 { REPOS_METRIC_QUERY_TABLE,  TRAF_METRIC_QUERY_TABLE_OLD_REPOS,
   createMetricQueryTable,  sizeof(createMetricQueryTable),
   NULL,0,
   NULL, 0,
   FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
 // REPOS_METRIC_SESSION_TABLE
  { REPOS_METRIC_SESSION_TABLE,  TRAF_METRIC_SESSION_TABLE_OLD_REPOS,
    createMetricSessionTable,  sizeof(createMetricSessionTable),
    NULL,0,
    NULL, 0,
    FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
  // REPOS_METRIC_QUERY_AGGR_TABLE
  { REPOS_METRIC_QUERY_AGGR_TABLE,  TRAF_METRIC_QUERY_AGGR_TABLE_OLD_REPOS,
    createMetricQueryAggrTable,  sizeof(createMetricQueryAggrTable),
    NULL,0,
    NULL, 0,
    FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
  // REPOS_METRIC_TEXT_TABLE  
  { REPOS_METRIC_TEXT_TABLE,  NULL,
    createMetricTextTable,  sizeof(createMetricTextTable),
    NULL, 0,
    NULL, 0,
    FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}
  
};
static const MDUpgradeInfo allReposv110Tov210UpgradeInfo[] = {

  // TRAF_METRIC_QUERY_TABLE
  { REPOS_METRIC_QUERY_TABLE,  TRAF_METRIC_QUERY_TABLE_OLD_REPOS,
    createMetricQueryTable,  sizeof(createMetricQueryTable),
    createOldTrafv11MetricQueryTable,  sizeof(createOldTrafv11MetricQueryTable),
    NULL, 0,
    TRUE, 

    // new table columns
    "instance_id,"
    "tenant_id,"
    "component_id,"
    "process_id,"
    "thread_id,"
    "node_id,"
    "pnid_id,"
    "host_id,"
    "ip_address_id,"
    "sequence_number,"
    "process_name,"
    "exec_start_utc_ts,"
    "query_id,"
    "query_signature_id,"
    "user_name,"
    "role_name,"
    "start_priority,"
    "master_process_id,"
    "session_id,"
    "client_name,"
    "application_name,"
    "statement_id,"
    "statement_type,"
    "statement_subtype,"
    "submit_utc_ts,"
    "compile_start_utc_ts,"
    "compile_end_utc_ts,"
    "compile_elapsed_time,"
    "cmp_affinity_num,"
    "cmp_dop,"
    "cmp_txn_needed,"
    "cmp_mandatory_x_prod,"
    "cmp_missing_stats,"
    "cmp_num_joins,"
    "cmp_full_scan_on_table,"
    "cmp_rows_accessed_full_scan,"
    "est_accessed_rows,"
    "est_used_rows,"
    "cmp_compiler_id,"
    "cmp_cpu_path_length,"
    "cmp_cpu_binder,"
    "cmp_cpu_normalizer,"
    "cmp_cpu_analyzer,"
    "cmp_cpu_optimizer,"
    "cmp_cpu_generator,"
    "cmp_metadata_cache_hits,"
    "cmp_metadata_cache_lookups,"
    "cmp_query_cache_status,"
    "cmp_histogram_cache_hits,"
    "cmp_histogram_cache_lookups,"
    "cmp_stmt_heap_size,"
    "cmp_context_heap_size,"
    "cmp_optimization_tasks,"
    "cmp_optimization_contexts,"
    "cmp_is_recompile,"
    "est_num_seq_ios,"
    "est_num_rand_ios,"
    "est_cost,"
    "est_cardinality,"
    "est_io_time,"
    "est_msg_time,"
    "est_idle_time,"
    "est_cpu_time,"
    "est_total_time,"
    "est_total_mem,"
    "est_resource_usage,"
    "aggregate_option,"
    "cmp_number_of_bmos,"
    "cmp_overflow_mode,"
    "cmp_overflow_size,"
    "aggregate_total,"
    "stats_error_code,"
    "query_elapsed_time,"
    "sql_process_busy_time,"
    "disk_process_busy_time,"
    "disk_ios,"
    "num_sql_processes,"
    "sql_space_allocated,"
    "sql_space_used,"
    "sql_heap_allocated,"
    "sql_heap_used,"
    "total_mem_alloc,"
    "max_mem_used,"
    "transaction_id,"
    "num_request_msgs,"
    "num_request_msg_bytes,"
    "num_reply_msgs,"
    "num_reply_msg_bytes,"
    "first_result_return_utc_ts,"
    "rows_returned_to_master,"
    "parent_query_id,"
    "parent_system_name,"
    "exec_end_utc_ts,"
    "master_execution_time,"
    "master_elapsed_time,"
    "query_status,"
    "query_sub_status,"
    "error_code,"
    "sql_error_code,"
    "error_text,"
    "query_text,"
    "explain_plan,"
    "last_error_before_aqr,"
    "delay_time_before_aqr_sec,"
    "total_num_aqr_retries,"
    "msg_bytes_to_disk,"
    "msgs_to_disk,"
    "rows_accessed,"
    "rows_retrieved,"
    "num_rows_iud,"
    "processes_created,"
    "process_create_busy_time,"
    "ovf_file_count,"
    "ovf_space_allocated,"
    "ovf_space_used,"
    "ovf_block_size,"
    "ovf_write_read_count,"
    "ovf_write_count,"
    "ovf_buffer_blocks_written,"
    "ovf_buffer_bytes_written,"
    "ovf_read_count,"
    "ovf_buffer_blocks_read,"
    "ovf_buffer_bytes_read,"
    "num_nodes,"
    "udr_process_busy_time,"
    "pertable_stats,"
    "last_updated_time",

    // values from old table to be inserted into new
    "instance_id,"
    "tenant_id,"
    "component_id,"
    "process_id,"
    "thread_id,"
    "node_id,"
    "pnid_id,"
    "host_id,"
    "ip_address_id,"
    "sequence_number,"
    "process_name,"
    "exec_start_utc_ts,"
    "query_id,"
    "NULL,"
    "user_name,"
    "role_name,"
    "start_priority,"
    "master_process_id,"
    "session_id,"
    "client_name,"
    "application_name,"
    "statement_id,"
    "statement_type,"
    "statement_subtype,"
    "submit_utc_ts,"
    "compile_start_utc_ts,"
    "compile_end_utc_ts,"
    "compile_elapsed_time,"
    "cmp_affinity_num,"
    "cmp_dop,"
    "cmp_txn_needed,"
    "cmp_mandatory_x_prod,"
    "cmp_missing_stats,"
    "cmp_num_joins,"
    "cmp_full_scan_on_table,"
    "cmp_rows_accessed_full_scan,"
    "est_accessed_rows,"
    "est_used_rows,"
    "cmp_compiler_id,"
    "cmp_cpu_path_length,"
    "cmp_cpu_binder,"
    "cmp_cpu_normalizer,"
    "cmp_cpu_analyzer,"
    "cmp_cpu_optimizer,"
    "cmp_cpu_generator,"
    "cmp_metadata_cache_hits,"
    "cmp_metadata_cache_lookups,"
    "cmp_query_cache_status,"
    "cmp_histogram_cache_hits,"
    "cmp_histogram_cache_lookups,"
    "cmp_stmt_heap_size,"
    "cmp_context_heap_size,"
    "cmp_optimization_tasks,"
    "cmp_optimization_contexts,"
    "cmp_is_recompile,"
    "est_num_seq_ios,"
    "est_num_rand_ios,"
    "est_cost,"
    "est_cardinality,"
    "est_io_time,"
    "est_msg_time,"
    "est_idle_time,"
    "est_cpu_time,"
    "est_total_time,"
    "est_total_mem,"
    "est_resource_usage,"
    "aggregate_option,"
    "cmp_number_of_bmos,"
    "cmp_overflow_mode,"
    "cmp_overflow_size,"
    "aggregate_total,"
    "stats_error_code,"
    "query_elapsed_time,"
    "sql_process_busy_time,"
    "disk_process_busy_time,"
    "disk_ios,"
    "num_sql_processes,"
    "sql_space_allocated,"
    "sql_space_used,"
    "sql_heap_allocated,"
    "sql_heap_used,"
    "total_mem_alloc,"
    "max_mem_used,"
    "transaction_id,"
    "num_request_msgs,"
    "num_request_msg_bytes,"
    "num_reply_msgs,"
    "num_reply_msg_bytes,"
    "first_result_return_utc_ts,"
    "rows_returned_to_master,"
    "parent_query_id,"
    "parent_system_name,"
    "exec_end_utc_ts,"
    "master_execution_time,"
    "master_elapsed_time,"
    "query_status,"
    "query_sub_status,"
    "error_code,"
    "sql_error_code,"
    "error_text,"
    "query_text,"
    "explain_plan,"
    "last_error_before_aqr,"
    "delay_time_before_aqr_sec,"
    "total_num_aqr_retries,"
    "msg_bytes_to_disk,"
    "msgs_to_disk,"
    "rows_accessed,"
    "rows_retrieved,"
    "num_rows_iud,"
    "processes_created,"
    "process_create_busy_time,"
    "ovf_file_count,"
    "ovf_space_allocated,"
    "ovf_space_used,"
    "ovf_block_size,"
    "ovf_write_read_count,"
    "ovf_write_count,"
    "ovf_buffer_blocks_written,"
    "ovf_buffer_bytes_written,"
    "ovf_read_count,"
    "ovf_buffer_blocks_read,"
    "ovf_buffer_bytes_read,"
    "num_nodes,"
    "udr_process_busy_time,"
    "pertable_stats,"
    "NULL",

    NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},
 
  // REPOS_METRIC_SESSION_TABLE
  { REPOS_METRIC_SESSION_TABLE,  TRAF_METRIC_SESSION_TABLE_OLD_REPOS,
    createMetricSessionTable,  sizeof(createMetricSessionTable),
    createOldTrafv11MetricSessionTable,  sizeof(createOldTrafv11MetricSessionTable),
    NULL, 0,
    TRUE, 

    // new table columns
    "instance_id,"
    "tenant_id,"
    "component_id,"
    "process_id,"
    "thread_id,"
    "node_id,"   
    "pnid_id,"
    "host_id,"
    "ip_address_id," 
    "sequence_number,"                 
    "process_name,"
    "session_id,"
    "session_status,"
    "session_start_utc_ts,"
    "session_end_utc_ts,"
    "user_id,"
    "user_name,"
    "role_name,"
    "client_name,"
    "client_user_name,"
    "application_name,"
    "profile_name,"
    "sla_name,"
    "total_execution_time,"
    "total_elapsed_time,"
    "total_insert_stmts_executed,"
    "total_delete_stmts_executed,"
    "total_update_stmts_executed,"
    "total_select_stmts_executed,"
    "total_catalog_stmts,"
    "total_executes,"
    "total_fetches,"
    "total_closes,"
    "total_execdirects,"
    "total_errors,"
    "total_warnings,"
    "total_login_elapsed_time_mcsec,"
    "ldap_login_elapsed_time_mcsec,"
    "sql_user_elapsed_time_mcsec,"
    "search_connection_elapsed_time_mcsec,"
    "search_elapsed_time_mcsec,"
    "authentication_connection_elapsed_time_mcsec,"
    "authentication_elapsed_time_mcsec",   

    // values from old table to be inserted into new
    "instance_id,"
    "tenant_id,"
    "component_id,"
    "process_id,"
    "thread_id,"
    "node_id,"   
    "pnid_id,"
    "host_id,"
    "ip_address_id," 
    "sequence_number,"                 
    "process_name,"
    "session_id,"
    "session_status,"
    "session_start_utc_ts,"
    "session_end_utc_ts,"
    "user_id,"
    "user_name,"
    "role_name,"
    "client_name,"
    "client_user_name,"
    "application_name,"
    "NULL,"
    "NULL,"
    "total_execution_time,"
    "total_elapsed_time,"
    "total_insert_stmts_executed,"
    "total_delete_stmts_executed,"
    "total_update_stmts_executed,"
    "total_select_stmts_executed,"
    "total_catalog_stmts,"
    "total_executes,"
    "total_fetches,"
    "total_closes,"
    "total_execdirects,"
    "total_errors,"
    "total_warnings,"
    "total_login_elapsed_time_mcsec,"
    "ldap_login_elapsed_time_mcsec,"
    "sql_user_elapsed_time_mcsec,"
    "search_connection_elapsed_time_mcsec,"
    "search_elapsed_time_mcsec,"
    "authentication_connection_elapsed_time_mcsec,"
    "authentication_elapsed_time_mcsec",  

    NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  // REPOS_METRIC_QUERY_AGGR_TABLE
  { REPOS_METRIC_QUERY_AGGR_TABLE,  TRAF_METRIC_QUERY_AGGR_TABLE_OLD_REPOS,
    createMetricQueryAggrTable,  sizeof(createMetricQueryAggrTable),
    createOldTrafv11MetricQueryAggrTable,  sizeof(createOldTrafv11MetricQueryAggrTable),
    NULL, 0,
    TRUE, 

    // new table columns
    "instance_id, tenant_id, component_id, process_id, thread_id, node_id, pnid_id, host_id,"
    "ip_address_id, sequence_number, process_name, session_id, "
    "session_start_utc_ts, aggregation_last_update_utc_ts, aggregation_last_elapsed_time," 
    "user_id, user_name, role_name, client_name, client_user_name,"
    "application_name, total_est_rows_accessed, total_est_rows_used, total_rows_retrieved,"
    "total_num_rows_iud, total_selects, total_inserts, total_updates, total_deletes,"

    "total_ddl_stmts, total_util_stmts, total_catalog_stmts, total_other_stmts, "
    "total_insert_errors, total_delete_errors, total_update_errors, total_select_errors, "
    "total_ddl_errors, total_util_errors, total_catalog_errors, total_other_errors, "

    "delta_estimated_rows_accessed, delta_estimated_rows_used, delta_rows_accessed,"
    "delta_rows_retrieved, delta_num_rows_iud, "
    "delta_selects, delta_inserts, delta_updates, delta_deletes,"

    "delta_ddl_stmts, delta_util_stmts, delta_catalog_stmts, delta_other_stmts, "
    "delta_insert_errors, delta_delete_errors, delta_update_errors, delta_select_errors, "
    "delta_ddl_errors, delta_util_errors, delta_catalog_errors, delta_other_errors, "
    
    "average_response_time, throughput_per_second ",

    // values from old table to be inserted into new
    "instance_id, tenant_id, component_id, process_id, thread_id, node_id, pnid_id, host_id,"
    "ip_address_id, sequence_number, process_name, session_id, "
    "session_start_utc_ts, aggregation_last_update_utc_ts, aggregation_last_elapsed_time," 
    "user_id, user_name, role_name, client_name, client_user_name,"
    "application_name, total_est_rows_accessed, total_est_rows_used, total_rows_retrieved,"
    "total_num_rows_iud, total_selects, total_inserts, total_updates, total_deletes,"

    "total_ddl_stmts, total_util_stmts, total_catalog_stmts, total_other_stmts, "
    "total_insert_errors, total_delete_errors, total_update_errors, total_select_errors, "
    "total_ddl_errors, total_util_errors, total_catalog_errors, total_other_errors, "

    "delta_estimated_rows_accessed, delta_estimated_rows_used, delta_rows_accessed,"
    "delta_rows_retrieved, delta_num_rows_iud, "
    "delta_selects, delta_inserts, delta_updates, delta_deletes,"

    "delta_ddl_stmts, delta_util_stmts, delta_catalog_stmts, delta_other_stmts, "
    "delta_insert_errors, delta_delete_errors, delta_update_errors, delta_select_errors, "
    "delta_ddl_errors, delta_util_errors, delta_catalog_errors, delta_other_errors, "

    "NULL, NULL",

    NULL, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE},

  // REPOS_METRIC_TEXT_TABLE  
  { REPOS_METRIC_TEXT_TABLE,  NULL,
    createMetricTextTable,  sizeof(createMetricTextTable),
    NULL, 0,
    NULL, 0,
    FALSE, NULL, NULL, NULL, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}
  
};

#endif

