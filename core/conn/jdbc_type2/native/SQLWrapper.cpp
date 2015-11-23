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

#include <platform_ndcs.h>
#include <sqlcli.h>
#include "Debug.h"
#include "SQLWrapper.h"


using namespace SRVR;

// Added for exit on SQL un-recoverable errors
// The sqlErrorExit and errorIndex (to 0) should be initialized before the start
// of processing a request from the client. The RETURN macro below would
// increment errorIndex and add any SQL errors to the Int32 array[errorIndex]
__thread Int32 sqlErrorExit[8];
__thread short errorIndex;

static BYTE* allocWSQLBuffer(Int32 size);
static void releaseWSQLBuffer(BYTE *&, Int32 &);

bool sql_diagnostics = false;

#define INIT \
    Int32 retcode; \
if (sql_diagnostics) WSQL_EXEC_ClearDiagnostics(NULL);

// The SQL error if any is set here and will be picked up later
// in the server layer to check for un-recoverable fatal errors
// Fix for CR 6447 - The 2034 error will now only be added for file system error 31
// The error code will be added in GETSQLWARNINGORERROR only if we detect a error 31
#define RETURN \
    if (retcode < 0 || retcode == 100) \
{\
    sql_diagnostics = true; \
    if (retcode < 0 && errorIndex < 8 && retcode != -2034) \
    sqlErrorExit[errorIndex++] = retcode; \
}\
else \
sql_diagnostics = false; \
return retcode;

Int32 SRVR::WSQL_EXEC_AllocDesc (
        /*INOUT*/ SQLDESC_ID * desc_id,
        /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor)
{
    SRVRTRACE_ENTER(WSQL_AllocDesc);
    INIT
        retcode = SQL_EXEC_AllocDesc (
                desc_id,
                input_descriptor);
    SRVRTRACE_EXIT(WSQL_AllocDesc);
    RETURN
}

Int32 SRVR::WSQL_EXEC_AllocStmt (
        /*INOUT*/ SQLSTMT_ID * new_statement_id,
        /*IN OPTIONAL*/ SQLSTMT_ID * cloned_statement)
{
    SRVRTRACE_ENTER(WSQL_AllocStmt);
    INIT
        retcode = SQL_EXEC_AllocStmt (
                new_statement_id,
                cloned_statement);
    SRVRTRACE_EXIT(WSQL_AllocStmt);
    RETURN
}

Int32 SRVR::WSQL_EXEC_AllocStmtForRS (
        /*IN*/          SQLSTMT_ID *callStmtId,
        /*IN OPTIONAL*/ Int32        resultSetIndex,
        /*INOUT*/       SQLSTMT_ID *resultSetStmtId )
{
    // SrvrTraceFile
    SRVRTRACE_ENTER(WSQL_AllocStmtForRS);
    INIT
        retcode = SQL_EXEC_AllocStmtForRS (callStmtId, resultSetIndex, resultSetStmtId);
    // SrvrTraceFile
    SRVRTRACE_EXIT(WSQL_AllocStmtForRS);
    RETURN
}
Int32 SRVR::WSQL_EXEC_ClearDiagnostics (
        /*IN*/ SQLSTMT_ID *statement_id)
{
    SRVRTRACE_ENTER(WSQL_ClearDiagnostics);
    Int32 retcode;
    retcode = SQL_EXEC_ClearDiagnostics (
            statement_id);
    sql_diagnostics = false;
    SRVRTRACE_EXIT(WSQL_ClearDiagnostics);
    return retcode;
}

Int32 SRVR::WSQL_EXEC_CloseStmt (
        /*IN*/ SQLSTMT_ID * statement_id)
{
    SRVRTRACE_ENTER(WSQL_CloseStmt);
    INIT
        retcode = SQL_EXEC_CloseStmt (
                statement_id);
    SRVRTRACE_EXIT(WSQL_CloseStmt);
    RETURN
}

Int32 SRVR::WSQL_EXEC_DeallocDesc (
        /*IN*/ SQLDESC_ID * desc_id )
{
    SRVRTRACE_ENTER(WSQL_DeallocDesc);
    INIT
        retcode = SQL_EXEC_DeallocDesc (
                desc_id );
    SRVRTRACE_EXIT(WSQL_DeallocDesc);
    RETURN
}

Int32 SRVR::WSQL_EXEC_DeallocStmt (
        /*IN*/ SQLSTMT_ID * statement_id)
{
    SRVRTRACE_ENTER(WSQL_DeallocStmt);
    INIT
        retcode = SQL_EXEC_DeallocStmt (
                statement_id);
    SRVRTRACE_EXIT(WSQL_DeallocStmt);
    RETURN
}

Int32 SRVR::WSQL_EXEC_DescribeStmt (
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
        /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor)
{
    SRVRTRACE_ENTER(WSQL_DescribeStmt);
    INIT
        retcode = SQL_EXEC_DescribeStmt (
                statement_id,
                input_descriptor,
                output_descriptor);
    SRVRTRACE_EXIT(WSQL_DescribeStmt);
    RETURN
}

Int32 SRVR::WSQL_EXEC_Exec (
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
        /*IN*/ Int32 num_ptr_pairs,
        ...)
{
    SRVRTRACE_ENTER(WSQL_Exec);
    INIT
        va_list argptr;
    va_start(argptr, num_ptr_pairs);
    struct SQLCLI_PTR_PAIRS* ptr_pairs = NULL;

    if (num_ptr_pairs > 0)
    {
        ptr_pairs = (struct SQLCLI_PTR_PAIRS*)allocWSQLBuffer(num_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS));
        if (ptr_pairs == NULL)
            return 999;
    }
    for (int i=0; i < num_ptr_pairs; i++)
    {
        ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }
    if (num_ptr_pairs == 0)
        retcode = SQL_EXEC_Exec (
                statement_id,
                input_descriptor,
                0);
    else
        retcode = SQL_EXEC_EXEC (
                statement_id,
                input_descriptor,
                num_ptr_pairs,
                ptr_pairs);

    SRVRTRACE_EXIT(WSQL_Exec);
    RETURN
}

Int32 SRVR::WSQL_EXEC_ExecDirect(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN*/ SQLDESC_ID * sql_source,
        /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
        /*IN*/ Int32 num_ptr_pairs,
        ...)
{
    SRVRTRACE_ENTER(WSQL_ExecDirect);
    INIT
        va_list argptr;
    va_start(argptr, num_ptr_pairs);
    struct SQLCLI_PTR_PAIRS* ptr_pairs = NULL;

    if (num_ptr_pairs > 0)
    {
        ptr_pairs = (struct SQLCLI_PTR_PAIRS*)allocWSQLBuffer(num_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS));
        if (ptr_pairs == NULL)
            return 999;
    }
    for (int i=0; i < num_ptr_pairs; i++)
    {
        ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }
    UInt32 flags = 0;
    if (num_ptr_pairs == 0)
        retcode = SQL_EXEC_ExecDirect2 (
                statement_id,
                sql_source,
                flags,
                input_descriptor,
                0);
    else
        retcode = SQL_EXEC_EXECDIRECT (
                statement_id,
                sql_source,
                input_descriptor,
                num_ptr_pairs,
                ptr_pairs);

    SRVRTRACE_EXIT(WSQL_ExecDirect);
    RETURN
}

Int32 SRVR::WSQL_EXEC_ExecFetch(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
        /*IN*/ Int32 num_ptr_pairs,
        ...)
{
    SRVRTRACE_ENTER(WSQL_ExecFetch);
    INIT
        va_list argptr;
    va_start(argptr, num_ptr_pairs);
    struct SQLCLI_PTR_PAIRS* ptr_pairs = NULL;

    if (num_ptr_pairs > 0)
    {
        ptr_pairs = (struct SQLCLI_PTR_PAIRS*)allocWSQLBuffer(num_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS));
        if (ptr_pairs == NULL)
            return 999;
    }
    for (int i=0; i < num_ptr_pairs; i++)
    {
        ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }

    if (num_ptr_pairs == 0)
        retcode = SQL_EXEC_ExecFetch(
                statement_id,
                input_descriptor,
                0);
    else
        retcode = SQL_EXEC_EXECFETCH(
                statement_id,
                input_descriptor,
                num_ptr_pairs,
                ptr_pairs);

    SRVRTRACE_EXIT(WSQL_ExecFetch);
    RETURN
}

Int32 SRVR::WSQL_EXEC_ClearExecFetchClose(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
        /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
        /*IN*/ Int32 num_input_ptr_pairs,
        /*IN*/ Int32 num_output_ptr_pairs,
        /*IN*/ Int32 num_total_ptr_pairs,
        ...)
{
    SRVRTRACE_ENTER(WSQL_ClearExecFetchClose);
    Int32 retcode;
    sql_diagnostics = 0;
    int i;
    va_list argptr;
    va_start(argptr, num_total_ptr_pairs);
    struct SQLCLI_PTR_PAIRS* input_ptr_pairs = NULL;
    struct SQLCLI_PTR_PAIRS* output_ptr_pairs = NULL;

    if (num_input_ptr_pairs > 0 || num_output_ptr_pairs > 0)
    {
        Int32 input_len = num_input_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS);
        input_len = ((input_len + 8 - 1) >> 3) << 3;
        Int32 output_len = num_output_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS);
        input_ptr_pairs = (struct SQLCLI_PTR_PAIRS* )allocWSQLBuffer(input_len + output_len);
        if (input_ptr_pairs == NULL)
            return 999;
        output_ptr_pairs = (struct SQLCLI_PTR_PAIRS* )((intptr_t)input_ptr_pairs + input_len);
    }
    for (i=0; i < num_input_ptr_pairs; i++)
    {
        input_ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        input_ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }
    for (i=0; i < num_output_ptr_pairs; i++)
    {
        output_ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        output_ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }

    if (num_total_ptr_pairs == 0)
        retcode = SQL_EXEC_ClearExecFetchClose (
                statement_id,
                input_descriptor,
                output_descriptor,
                0,
                0,
                0);
    else
        retcode = SQL_EXEC_CLEAREXECFETCHCLOSE (
                statement_id,
                input_descriptor,
                output_descriptor,
                num_input_ptr_pairs,
                num_output_ptr_pairs,
                num_total_ptr_pairs,
                input_ptr_pairs,
                output_ptr_pairs);

    SRVRTRACE_EXIT(WSQL_ClearExecFetchClose);

    // Added for exit on an un-recoverable SQL error
    // Since we are not calling the RETURN macro here (which sets the sqlErrorExit)
    // we will have to explictly set it.
    // The 2034 error will now only be added for file system error 31
    // The error code will be added in GETSQLWARNINGORERROR only if we detect a error 31
    if( retcode < 0 && errorIndex < 8 && retcode != -2034)
        sqlErrorExit[errorIndex++] = retcode;

    return retcode;
}


Int32 SRVR::WSQL_EXEC_Fetch(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
        /*IN*/ Int32 num_ptr_pairs,
        ...)
{
    SRVRTRACE_ENTER(WSQL_Fetch);
    INIT
        va_list argptr;
    va_start(argptr, num_ptr_pairs);
    struct SQLCLI_PTR_PAIRS* ptr_pairs = NULL;

    if (num_ptr_pairs > 0)
    {
        ptr_pairs = (struct SQLCLI_PTR_PAIRS*)allocWSQLBuffer(num_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS)); // has this ever been freed?
        if (ptr_pairs == NULL)
            return 999;
    }
    for (int i=0; i < num_ptr_pairs; i++)
    {
        ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }

    if (num_ptr_pairs == 0)
        retcode = SQL_EXEC_Fetch(
                statement_id,
                output_descriptor,
                0);
    else
        retcode = SQL_EXEC_FETCH(
                statement_id,
                output_descriptor,
                num_ptr_pairs,
                ptr_pairs);

    SRVRTRACE_EXIT(WSQL_Fetch);
    RETURN
}

Int32 SRVR::WSQL_EXEC_FetchClose(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
        /*IN*/ Int32 num_ptr_pairs,
        ...)
{
    SRVRTRACE_ENTER(WSQL_FetchClose);
    INIT
        va_list argptr;
    va_start(argptr, num_ptr_pairs);
    struct SQLCLI_PTR_PAIRS* ptr_pairs = NULL;

    if (num_ptr_pairs > 0)
    {
        ptr_pairs = (struct SQLCLI_PTR_PAIRS*)allocWSQLBuffer(num_ptr_pairs * sizeof(struct SQLCLI_PTR_PAIRS)); // has this ever been freed?
        if (ptr_pairs == NULL)
            return 999;
    }
    for (int i=0; i < num_ptr_pairs; i++)
    {
        ptr_pairs[i].var_ptr = va_arg(argptr, void*);
        ptr_pairs[i].ind_ptr = va_arg(argptr, void*);
    }

    if (num_ptr_pairs == 0)
        retcode = SQL_EXEC_FetchClose(
                statement_id,
                output_descriptor,
                0);
    else
        retcode = SQL_EXEC_FETCHCLOSE(
                statement_id,
                output_descriptor,
                num_ptr_pairs,
                ptr_pairs);

    SRVRTRACE_EXIT(WSQL_FetchClose);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDescEntryCount(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ SQLDESC_ID * output_descriptor)
{
    SRVRTRACE_ENTER(WSQL_GetDescEntryCount);
    INIT
        retcode = SQL_EXEC_GetDescEntryCount(
                sql_descriptor,
                output_descriptor);
    SRVRTRACE_EXIT(WSQL_GetDescEntryCount);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDescItem(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ Int32 entry,
        /*IN* (SQLDESC_ITEM_ID) */ Int32 what_to_get,
        /*OUT OPTIONAL*/ Int32 * numeric_value,
        /*OUT OPTIONAL*/ char * string_value,
        /*IN OPTIONAL*/ Int32 max_string_len,
        /*OUT OPTIONAL*/ Int32 * len_of_item,
        /*IN OPTIONAL*/ Int32 start_from_offset)
{
    SRVRTRACE_ENTER(WSQL_GetDescItem);
    INIT
        retcode = SQL_EXEC_GetDescItem(
                sql_descriptor,
                entry,
                what_to_get,
                numeric_value,
                string_value,
                max_string_len,
                len_of_item,
                start_from_offset);
    SRVRTRACE_EXIT(WSQL_GetDescItem);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDescItems(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ SQLDESC_ITEM desc_items[],
        /*IN*/ SQLDESC_ID * value_num_descriptor,
        /*IN*/ SQLDESC_ID * output_descriptor)
{
    SRVRTRACE_ENTER(WSQL_GetDescItems);
    INIT
        retcode = SQL_EXEC_GetDescItems(
                sql_descriptor,
                desc_items,
                value_num_descriptor,
                output_descriptor);
    SRVRTRACE_EXIT(WSQL_GetDescItems);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDescItems2(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ Int32 no_of_desc_items,
        /*IN*/ SQLDESC_ITEM desc_items[])
{
    SRVRTRACE_ENTER(WSQL_GetDescItems2);
    INIT
        retcode = SQL_EXEC_GetDescItems2(
                sql_descriptor,
                no_of_desc_items,
                desc_items);
    SRVRTRACE_EXIT(WSQL_GetDescItems2);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDiagnosticsStmtInfo(
        /*IN*/ Int32 *stmt_info_items,
        /*IN*/ SQLDESC_ID * output_descriptor)
{
    SRVRTRACE_ENTER(WSQL_GetDiagnosticsStmtInfo);
    Int32 retcode;
    retcode = SQL_EXEC_GetDiagnosticsStmtInfo(
            stmt_info_items,
            output_descriptor);
    SRVRTRACE_EXIT(WSQL_GetDiagnosticsStmtInfo);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDiagnosticsStmtInfo2(
        /*IN OPTIONAL*/ SQLSTMT_ID * statement_id,
        /*IN* (SQLDIAG_STMT_INFO_ITEM_ID) */ Int32 what_to_get,
        /*OUT OPTIONAL*/ Int64 * numeric_value,
        /*OUT OPTIONAL*/ char * string_value,
        /*IN OPTIONAL*/ Int32 max_string_len,
        /*OUT OPTIONAL*/ Int32 * len_of_item)
{
    SRVRTRACE_ENTER(WSQL_GetDiagnosticsStmtInfo2);
    Int32 retcode;
    retcode = SQL_EXEC_GetDiagnosticsStmtInfo2(
            statement_id,
            what_to_get,
            numeric_value,
            string_value,
            max_string_len,
            len_of_item);
    SRVRTRACE_EXIT(WSQL_GetDiagnosticsStmtInfo2);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDiagnosticsCondInfo(
        /*IN*/ SQLDIAG_COND_INFO_ITEM *cond_info_items,
        /*IN*/ SQLDESC_ID * cond_num_descriptor,
        /*IN*/ SQLDESC_ID * output_descriptor)
{
    SRVRTRACE_ENTER(WSQL_GetDiagnosticsCondInfo);
    Int32 retcode;
    retcode = SQL_EXEC_GetDiagnosticsCondInfo(
            cond_info_items,
            cond_num_descriptor,
            output_descriptor);
    SRVRTRACE_EXIT(WSQL_GetDiagnosticsCondInfo);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDiagnosticsCondInfo2(
        /*IN* (SQLDIAG_COND_INFO_ITEM_ID) */ Int32 what_to_get,
        /*IN*/ Int32 conditionNum,
        /*OUT OPTIONAL*/ Int32 * numeric_value,
        /*OUT OPTIONAL*/ char * string_value,
        /*IN OPTIONAL */ Int32 max_string_len,
        /*OUT OPTIONAL*/ Int32 * len_of_item)
{
    SRVRTRACE_ENTER(WSQL_GetDiagnosticsCondInfo2);
    Int32 retcode;
    retcode = SQL_EXEC_GetDiagnosticsCondInfo2(
            what_to_get,
            conditionNum,
            numeric_value,
            string_value,
            max_string_len,
            len_of_item);
    SRVRTRACE_EXIT(WSQL_GetDiagnosticsCondInfo2);
    RETURN
}


Int32 SRVR::WSQL_EXEC_GetStmtAttr(
        /*IN*/                  SQLSTMT_ID * statement_id,
        /*IN* (SQLATTR_TYPE) */ Int32 attrName,
        /*OUT OPTIONAL*/        Int32 * numeric_value,
        /*OUT OPTIONAL*/        char * string_value,
        /*IN OPTIONAL*/         Int32   max_string_len,
        /*OUT OPTIONAL*/        Int32 * len_of_item)
{
    SRVRTRACE_ENTER(WSQL_GetStmtAttr);
    INIT
        retcode = SQL_EXEC_GetStmtAttr(
                statement_id,
                attrName,
                numeric_value,
                string_value,
                max_string_len,
                len_of_item);
    SRVRTRACE_EXIT(WSQL_GetStmtAttr);
    RETURN
}

//LCOV_EXCL_START
Int32 SRVR::WSQL_EXEC_Prepare(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN*/ SQLDESC_ID * sql_source)
{
    SRVRTRACE_ENTER(WSQL_Prepare);
    INIT
        retcode = SQL_EXEC_Prepare(
                statement_id,
                sql_source);
    SRVRTRACE_EXIT(WSQL_Prepare);
    RETURN
}
//LCOV_EXCL_STOP

Int32 SRVR::WSQL_EXEC_Prepare2(
        /*IN*/    SQLSTMT_ID * statement_id,
        /*IN*/    SQLDESC_ID * sql_source,
        /*INOUT*/ char * gencode_ptr,
        /*IN*/    Int32 gencode_len,
        /*INOUT*/ Int32 * ret_gencode_len,
        /*INOUT*/ SQL_QUERY_COST_INFO *query_cost_info,
        /*INOUT*/ SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info,
        /*INOUT*/ char * uniqueQueryId,
        /*INOUT*/ Int32 * uniqueQueryIdLen,
        /*IN*/    UInt32 flags)
{
    SRVRTRACE_ENTER(WSQL_Prepare);
    INIT
        retcode = SQL_EXEC_Prepare2(
                statement_id,
                sql_source,
                gencode_ptr,
                gencode_len,
                ret_gencode_len,
                query_cost_info,
                comp_stats_info,
                uniqueQueryId,
                uniqueQueryIdLen,
                flags);
    SRVRTRACE_EXIT(WSQL_Prepare);
    RETURN
}

Int32 SRVR::WSQL_EXEC_SetCursorName(
        /*IN*/ SQLSTMT_ID * statement_id,
        /*IN*/ SQLSTMT_ID * cursor_name)
{
    SRVRTRACE_ENTER(WSQL_SetCursorName);
    INIT
        retcode = SQL_EXEC_SetCursorName(
                statement_id,
                cursor_name);
    SRVRTRACE_EXIT(WSQL_SetCursorName);
    RETURN
}

Int32 SRVR::WSQL_EXEC_SetDescItem(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ Int32 entry,
        /*IN* (SQLDESC_ITEM_ID) */ Int32 what_to_set,
        /*IN OPTIONAL*/ long numeric_value,
        /*IN OPTIONAL*/ char * string_value)
{
    SRVRTRACE_ENTER(WSQL_SetDescItem);
    INIT
        retcode = SQL_EXEC_SetDescItem(
                sql_descriptor,
                entry,
                what_to_set,
                numeric_value,
                string_value);
    SRVRTRACE_EXIT(WSQL_SetDescItem);
    RETURN
}

//LCOV_EXCL_START
Int32 SRVR::WSQL_EXEC_SetDescItems(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ SQLDESC_ITEM desc_items[],
        /*IN*/ SQLDESC_ID * value_num_descriptor,
        /*IN*/ SQLDESC_ID * input_descriptor)
{
    SRVRTRACE_ENTER(WSQL_SetDescItems);
    INIT
        retcode = SQL_EXEC_SetDescItems(
                sql_descriptor,
                desc_items,
                value_num_descriptor,
                input_descriptor);
    SRVRTRACE_EXIT(WSQL_SetDescItems);
    RETURN
}

Int32 SRVR::WSQL_EXEC_SetDescItems2(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*IN*/ Int32 no_of_desc_items,
        /*IN*/ SQLDESC_ITEM desc_items[])
{
    SRVRTRACE_ENTER(WSQL_SetDescItems2);
    INIT
        retcode = SQL_EXEC_SetDescItems2(
                sql_descriptor,
                no_of_desc_items,
                desc_items);
    SRVRTRACE_EXIT(WSQL_SetDescItems2);
    RETURN
}

Int32 SRVR::WSQL_EXEC_SETROWSETDESCPOINTERS(SQLDESC_ID * desc_id,
        Int32    rowset_size,
        Int32    *rowset_status_ptr,
        Int32    starting_entry,
        Int32    num_quadruple_fields,
        struct SQLCLI_QUAD_FIELDS    quad_fields[])
{
    SRVRTRACE_ENTER(WSQL_SETROWSETDESCPOINTERS);
    INIT
        retcode = SQL_EXEC_SETROWSETDESCPOINTERS(
                desc_id,
                rowset_size,
                rowset_status_ptr,
                starting_entry,
                num_quadruple_fields,
                quad_fields);
    SRVRTRACE_EXIT(WSQL_SETROWSETDESCPOINTERS);
    RETURN
}

Int32 SRVR::WSQL_EXEC_SetStmtAttr(
        /*IN*/                  SQLSTMT_ID * statement_id,
        /*IN* (SQLATTR_TYPE) */ Int32 attrName,
        /*IN OPTIONAL*/         Int32 numeric_value,
        /*IN OPTIONAL*/         char * string_value)
{
    SRVRTRACE_ENTER(WSQL_SetStmtAttr);
    INIT
        retcode = SQL_EXEC_SetStmtAttr(
                statement_id,
                attrName,
                numeric_value,
                string_value);
    SRVRTRACE_EXIT(WSQL_SetStmtAttr);
    RETURN
}

/* temporary functions -- for use by sqlcat simulator only */

Int32 SRVR::WSQL_EXEC_AllocDesc (
        /*INOUT*/ SQLDESC_ID * desc_id,
        /*IN OPTIONAL*/ Int32 max_entries)
{
    SRVRTRACE_ENTER(WSQL_AllocDesc_max);
    INIT
        retcode = SQL_EXEC_AllocDesc (
                desc_id,
                max_entries);
    SRVRTRACE_EXIT(WSQL_AllocDesc_max);
    RETURN
}

Int32 SRVR::WSQL_EXEC_GetDescEntryCount(
        /*IN*/ SQLDESC_ID * sql_descriptor,
        /*OUT*/ Int32 * num_entries)
{
    SRVRTRACE_ENTER(WSQL_GetDescEntryCount_num);
    INIT
        retcode = SQL_EXEC_GetDescEntryCount(
                sql_descriptor,
                num_entries);
    SRVRTRACE_EXIT(WSQL_GetDescEntryCount_num);
    RETURN
}

static BYTE* allocWSQLBuffer(Int32 size)
{
    BYTE *pWSQLBuffer = NULL;
    Int32 WSQLBufferLen = 0;

    if (size > WSQLBufferLen)
    {
        Int32 len = size + size/2;
        releaseWSQLBuffer(pWSQLBuffer, WSQLBufferLen);
        MEMORY_ALLOC_ARRAY(pWSQLBuffer, BYTE, len);
        if (pWSQLBuffer != NULL)
            WSQLBufferLen = len;
    }
    return pWSQLBuffer;
}

static void releaseWSQLBuffer(BYTE *&pWSQLBuffer, Int32 &WSQLBufferLen)
{
    if (pWSQLBuffer != NULL)
    {
        MEMORY_DELETE_ARRAY(pWSQLBuffer);
        pWSQLBuffer = NULL;
        WSQLBufferLen = 0;
    }
    return;
}

