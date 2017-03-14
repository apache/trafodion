/**************************************************************************
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
**************************************************************************/

#ifndef _SQLWRAPPER_DEFINED
#define _SQLWRAPPER_DEFINED

extern __thread Int32               sqlErrorExit[];
extern __thread short               errorIndex;

namespace SRVR {

    Int32 WSQL_EXEC_AllocDesc (
            /*INOUT*/ SQLDESC_ID * desc_id,
            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor);

    Int32 WSQL_EXEC_AllocStmt (
            /*INOUT*/ SQLSTMT_ID * new_statement_id,
            /*IN OPTIONAL*/ SQLSTMT_ID * cloned_statement);


    Int32 WSQL_EXEC_AllocStmtForRS (
            /*IN*/          SQLSTMT_ID *callStmtId,
            /*IN OPTIONAL*/ Int32        resultSetIndex,
            /*INOUT*/       SQLSTMT_ID *resultSetStmtId );

    Int32 WSQL_EXEC_ClearDiagnostics (
            /*IN*/ SQLSTMT_ID *statement_id);

    Int32 WSQL_EXEC_CloseStmt (
            /*IN*/ SQLSTMT_ID * statement_id);

    Int32 WSQL_EXEC_DeallocDesc (
            /*IN*/ SQLDESC_ID * desc_id );

    Int32 WSQL_EXEC_DeallocStmt (
            /*IN*/ SQLSTMT_ID * statement_id);

    Int32 WSQL_EXEC_DescribeStmt (
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
            /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor);

    Int32 WSQL_EXEC_Exec (
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
            /*IN*/ Int32 num_ptr_pairs,
            ...);

    Int32 WSQL_EXEC_ExecDirect(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN*/ SQLDESC_ID * sql_source,
            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
            /*IN*/ Int32 num_ptr_pairs,
            ...);

    Int32 WSQL_EXEC_ExecFetch(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
            /*IN*/ Int32 num_ptr_pairs,
            ...);

    Int32 WSQL_EXEC_ClearExecFetchClose(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
            /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
            /*IN*/ Int32 num_input_ptr_pairs,
            /*IN*/ Int32 num_output_ptr_pairs,
            /*IN*/ Int32 num_total_ptr_pairs,
            ...);

    Int32 WSQL_EXEC_Fetch(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
            /*IN*/ Int32 num_ptr_pairs,
            ...);

    Int32 WSQL_EXEC_FetchClose(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
            /*IN*/ Int32 num_ptr_pairs,
            ...);

    Int32 WSQL_EXEC_GetDescEntryCount(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ SQLDESC_ID * output_descriptor);

    Int32 WSQL_EXEC_GetDescItem(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ Int32 entry,
            /*IN* (SQLDESC_ITEM_ID) */ Int32 what_to_get,
            /*OUT OPTIONAL*/ Int32 * numeric_value,
            /*OUT OPTIONAL*/ char * string_value,
            /*IN OPTIONAL*/ Int32 max_string_len,
            /*OUT OPTIONAL*/ Int32 * len_of_item,
            /*IN OPTIONAL*/ Int32 start_from_offset);

    //LCOV_EXCL_START
    Int32 WSQL_EXEC_GetDescItems(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ SQLDESC_ITEM desc_items[],
            /*IN*/ SQLDESC_ID * value_num_descriptor,
            /*IN*/ SQLDESC_ID * output_descriptor);
    //LCOV_EXCL_STOP

    Int32 WSQL_EXEC_GetDescItems2(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ Int32 no_of_desc_items,
            /*IN*/ SQLDESC_ITEM desc_items[]);

    Int32 WSQL_EXEC_GetDiagnosticsStmtInfo(
            /*IN*/ Int32 *stmt_info_items,
            /*IN*/ SQLDESC_ID * output_descriptor);

    Int32 WSQL_EXEC_GetDiagnosticsStmtInfo2(
            /*IN OPTIONAL*/ SQLSTMT_ID * statement_id,
            /*IN* (SQLDIAG_STMT_INFO_ITEM_ID) */ Int32 what_to_get,
            /*OUT OPTIONAL*/ Int64 * numeric_value,
            /*OUT OPTIONAL*/ char * string_value,
            /*IN OPTIONAL*/ Int32 max_string_len,
            /*OUT OPTIONAL*/ Int32 * len_of_item);

    Int32 WSQL_EXEC_GetDiagnosticsCondInfo(
            /*IN*/ SQLDIAG_COND_INFO_ITEM *cond_info_items,
            /*IN*/ SQLDESC_ID * cond_num_descriptor,
            /*IN*/ SQLDESC_ID * output_descriptor);

    Int32 WSQL_EXEC_GetDiagnosticsCondInfo2(
            /*IN* (SQLDIAG_COND_INFO_ITEM_ID) */ Int32 what_to_get,
            /*IN*/ Int32 conditionNum,
            /*OUT OPTIONAL*/ Int32 * numeric_value,
            /*OUT OPTIONAL*/ char * string_value,
            /*IN OPTIONAL */ Int32 max_string_len,
            /*OUT OPTIONAL*/ Int32 * len_of_item);

    Int32 WSQL_EXEC_GetStmtAttr(
            /*IN*/                  SQLSTMT_ID * statement_id,
            /*IN* (SQLATTR_TYPE) */ Int32 attrName,
            /*OUT OPTIONAL*/        Int32 * numeric_value,
            /*OUT OPTIONAL*/        char * string_value,
            /*IN OPTIONAL*/         Int32   max_string_len,
            /*OUT OPTIONAL*/        Int32 * len_of_item);

    Int32 WSQL_EXEC_Prepare(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN*/ SQLDESC_ID * sql_source);

    Int32 WSQL_EXEC_Prepare2(
            /*IN*/    SQLSTMT_ID * statement_id,
            /*IN*/    SQLDESC_ID * sql_source,
            /*INOUT*/ char * gencode_ptr,
            /*IN*/    Int32 gencode_len,
            /*INOUT*/ Int32 * ret_gencode_len,
            /*INOUT*/ SQL_QUERY_COST_INFO *query_cost_info,
            /*INOUT*/ SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info,
            /*INOUT*/ char * uniqueStmtId,
            /*INOUT*/ Int32 * uniqueStmtIdLen,
            /*IN*/    UInt32 flags = 0);

    Int32 WSQL_EXEC_SetCursorName(
            /*IN*/ SQLSTMT_ID * statement_id,
            /*IN*/ SQLSTMT_ID * cursor_name);

    Int32 WSQL_EXEC_SetDescItem(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ Int32 entry,
            /*IN* (SQLDESC_ITEM_ID) */ Int32 what_to_set,
            /*IN OPTIONAL*/ long numeric_value,
            /*IN OPTIONAL*/ char * string_value);

    Int32 WSQL_EXEC_SetDescItems(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ SQLDESC_ITEM desc_items[],
            /*IN*/ SQLDESC_ID * value_num_descriptor,
            /*IN*/ SQLDESC_ID * input_descriptor);

    Int32 WSQL_EXEC_SetDescItems2(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*IN*/ Int32 no_of_desc_items,
            /*IN*/ SQLDESC_ITEM desc_items[]);

    Int32 WSQL_EXEC_SetStmtAttr(
            /*IN*/                  SQLSTMT_ID * statement_id,
            /*IN* (SQLATTR_TYPE) */ Int32 attrName,
            /*IN OPTIONAL*/         Int32 numeric_value,
            /*IN OPTIONAL*/         char * string_value);

    /* temporary functions -- for use by sqlcat simulator only */

    Int32 WSQL_EXEC_AllocDesc (
            /*INOUT*/ SQLDESC_ID * desc_id,
            /*IN OPTIONAL*/ Int32 max_entries);

    Int32 WSQL_EXEC_GetDescEntryCount(
            /*IN*/ SQLDESC_ID * sql_descriptor,
            /*OUT*/ Int32 * num_entries);

    Int32 WSQL_EXEC_SETROWSETDESCPOINTERS(
            SQLDESC_ID * desc_id,
            Int32    rowset_size,
            Int32    *rowset_status_ptr,
            Int32    starting_entry,
            Int32    num_quadruple_fields,
            struct SQLCLI_QUAD_FIELDS    quad_fields[]);

}

#endif
