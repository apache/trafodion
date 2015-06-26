/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
********************************************************************/
/**************************************************************************
**************************************************************************/
//
#ifndef _SQLWRAPPER_DEFINED
#define _SQLWRAPPER_DEFINED

namespace SRVR {

Int32 WSQL_EXEC_AddModule (
		/*IN*/ SQLMODULE_ID * module_name);

Int32 WSQL_EXEC_AllocDesc (
		/*INOUT*/ SQLDESC_ID * desc_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor);

//LCOV_EXCL_START
Int32 WSQL_EXEC_AssocFileNumber (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ short        file_number);

//LCOV_EXCL_STOP

Int32 WSQL_EXEC_AllocStmt (
		/*INOUT*/ SQLSTMT_ID * new_statement_id,
		/*IN OPTIONAL*/ SQLSTMT_ID * cloned_statement);


Int32 WSQL_EXEC_AllocStmtForRS (
		/*IN*/          SQLSTMT_ID *callStmtId,
        /*IN OPTIONAL*/ Int32        resultSetIndex,
        /*INOUT*/       SQLSTMT_ID *resultSetStmtId );

Int32 WSQL_EXEC_ClearDiagnostics (
		/*IN*/ SQLSTMT_ID *statement_id);

Int32 WSQL_EXEC_CLI_VERSION ();

Int32 WSQL_EXEC_CloseStmt (
		/*IN*/ SQLSTMT_ID * statement_id);

//LCOV_EXCL_START
Int32 WSQL_EXEC_CreateContext(
		/*OUT*/ SQLCTX_HANDLE * context_handle,
		/*IN OPTIONAL*/ char* sqlAuthId , 
		/*IN OPTIONAL*/ Int32 /* for future use */);
  
Int32 WSQL_EXEC_CurrentContext(
		/*OUT*/ SQLCTX_HANDLE * contextHandle);

Int32 WSQL_EXEC_DeleteContext(
		/*IN*/ SQLCTX_HANDLE contextHandle);

Int32 WSQL_EXEC_ResetContext(
		/*IN*/ SQLCTX_HANDLE contextHandle, 
		/*IN*/ void *contextMsg);
//LCOV_EXCL_STOP

Int32 WSQL_EXEC_DeallocDesc (
		/*IN*/ SQLDESC_ID * desc_id );

Int32 WSQL_EXEC_DeallocStmt (
		/*IN*/ SQLSTMT_ID * statement_id);

//LCOV_EXCL_START
Int32 WSQL_EXEC_DefineDesc (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN* (SQLWHAT_DESC) */ Int32 what_descriptor,
		/*IN*/ SQLDESC_ID * sql_descriptor);
//LCOV_EXCL_STOP

Int32 WSQL_EXEC_DescribeStmt (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN OPTIONAL*/ SQLDESC_ID * output_descriptor);

//LCOV_EXCL_START
Int32 WSQL_EXEC_DisassocFileNumber(/*IN*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_DropContext (
		/*IN*/ SQLCTX_HANDLE context_handle );
//LCOV_EXCL_STOP

Int32 WSQL_EXEC_Exec (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
                ...);

Int32 WSQL_EXEC_ExecClose (
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

//LCOV_EXCL_START
Int32 WSQL_EXEC_ExecDirectDealloc(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ SQLDESC_ID * sql_source,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
                ...);
//LCOV_EXCL_STOP

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

//LCOV_EXCL_START
Int32 WSQL_EXEC_FetchMultiple(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN  OPTIONAL*/ SQLDESC_ID * output_descriptor,
		/*IN*/                 Int32   rowset_size,
		/*IN*/                 Int32 * rowset_status_ptr,
		/*OUT*/                Int32 * rowset_nfetched,
		/*IN*/                 Int32   num_quadruple_fields,
//LCOV_EXCL_STOP
                                            ...);
Int32 WSQL_EXEC_Cancel (
		/*IN OPTIONAL*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_GetSessionAttr(
		/*IN* (SESSIONATTR_TYPE) */ Int32 attrName,
		/*OUT OPTIONAL*/        Int32 * numeric_value,
		/*OUT OPTIONAL*/        char * string_value,
		/*IN OPTIONAL*/         Int32   max_string_len,
		/*OUT OPTIONAL*/        Int32 * len_of_item);

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

//LCOV_EXCL_START
/* This function retrieves the SQLSTATE from the statement diagnostics area if possible */  
Int32 WSQL_EXEC_GetMainSQLSTATE(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/  Int32 sqlcode,
		/*OUT*/ char * sqlstate /* assumed to be char[6] */);

Int32 WSQL_EXEC_GetCSQLSTATE(
		/*OUT*/ char * sqlstate /* assumed to be char[6] */,
		/*IN*/  Int32 sqlcode);

Int32 WSQL_EXEC_GetCobolSQLSTATE(
		/*OUT*/ char * sqlstate /* assumed to be char[5] */,
		/*IN*/  Int32 sqlcode);

Int32 WSQL_EXEC_GetSQLSTATE(
		/*OUT*/ char * sqlstate /* assumed to be char[6] */);

//LCOV_EXCL_STOP

// The signature in the CLI for SQL_EXEC_GetStatistics has been changed 
// resulting in build errors in SrvrCore. Since these functions are not 
// used anyways it was decided that we comment these out.
//Int32 WSQL_EXEC_GetStatistics(
//		/*IN OPTIONAL*/ SQLSTMT_ID * statement_id);

//Int32 WSQL_EXEC_GETSTATISTICS(
//		/*IN OPTIONAL*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_GetStmtAttr(
		/*IN*/                  SQLSTMT_ID * statement_id,
		/*IN* (SQLATTR_TYPE) */ Int32 attrName,
		/*OUT OPTIONAL*/        Int32 * numeric_value,
		/*OUT OPTIONAL*/        char * string_value,
		/*IN OPTIONAL*/         Int32   max_string_len,
		/*OUT OPTIONAL*/        Int32 * len_of_item);


Int32 WSQL_EXEC_GoAway(
		/*IN*/ SQLDESC_ID * tableNameDesc);

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

Int32 WSQL_EXEC_ResDescName(
		/*INOUT*/ SQLDESC_ID * statement_id,
		/*IN OPTIONAL*/ SQLSTMT_ID * from_statement,
		/*IN OPTIONAL (SQLWHAT_DESC) */ Int32 what_desc);

Int32 WSQL_EXEC_ResStmtName(
		/*INOUT*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_SetCursorName(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ SQLSTMT_ID * cursor_name);

Int32 WSQL_EXEC_SetDescEntryCount(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ SQLDESC_ID * input_descriptor);

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

Int32 WSQL_EXEC_SetDescPointers(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 starting_entry,
		/*IN*/ Int32 num_ptr_pairs,
                ...);

Int32 WSQL_EXEC_SetRowsetDescPointers(
		SQLDESC_ID * desc_id,
		Int32    rowset_size,
		Int32    *rowset_status_ptr,
		Int32    starting_entry,
		Int32    num_quadruple_fields,
		...);

Int32 WSQL_EXEC_SetStmtAttr(
		/*IN*/                  SQLSTMT_ID * statement_id,
		/*IN* (SQLATTR_TYPE) */ Int32 attrName,
		/*IN OPTIONAL*/         Int32 numeric_value,
		/*IN OPTIONAL*/         char * string_value);

Int32 WSQL_EXEC_SwitchContext(
		/*IN*/ SQLCTX_HANDLE context_handle,
		/*OUT OPTIONAL*/ SQLCTX_HANDLE * prev_context_handle);

Int32 WSQL_EXEC_Xact(
		/*IN* (SQLTRANS_COMMAND) */ Int32 command,
		/*OUT OPTIONAL*/ SQLDESC_ID * transid_descriptor); 

Int32 WSQL_EXEC_SetAuthID(
   const char * externalUsername,
   const char * databaseUsername,
   const char * authToken,
   Int32        authTokenLen,
   Int32        effectiveUserID,
   Int32        sessionUserID);

Int32 WSQL_EXEC_GetUniqueQueryIdAttrs(
        /*IN*/    char * uniqueQueryId,
		/*IN*/    Int32 uniqueQueryIdLen,
		/*IN*/    Int32 no_of_attrs,
		/*INOUT*/ UNIQUEQUERYID_ATTR unique_queryid_attrs[]);

/* temporary functions -- for use by sqlcat simulator only */

Int32 WSQL_EXEC_AllocDesc (
		/*INOUT*/ SQLDESC_ID * desc_id,
		/*IN OPTIONAL*/ Int32 max_entries);

Int32 WSQL_EXEC_GetDescEntryCount(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*OUT*/ Int32 * num_entries);

Int32 WSQL_EXEC_SetDescEntryCount(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 num_entries);

//LCOV_EXCL_START
Int32 WSQL_EXEC_ADDMODULE (
		/*IN*/ SQLMODULE_ID * module_name);

Int32 WSQL_EXEC_ALLOCDESC (
		/*INOUT*/ SQLDESC_ID * desc_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor);

Int32 WSQL_EXEC_ASSOCFILENUMBER (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ short        file_number);

Int32 WSQL_EXEC_ALLOCSTMT (
		/*INOUT*/ SQLSTMT_ID * new_statement_id,
		/*IN OPTIONAL*/ SQLSTMT_ID * cloned_statement);

Int32 WSQL_EXEC_CLEARDIAGNOSTICS (
		/*IN*/ SQLSTMT_ID *statement_id);

Int32 WSQL_EXEC_CLOSESTMT (
		/*IN*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_CREATECONTEXT(
		/*OUT*/ SQLCTX_HANDLE * context_handle,
		/*IN OPTIONAL*/ char* sqlAuthId, 
		/*IN OPTIONAL*/ Int32 /* for future use */);

Int32 WSQL_EXEC_CURRENTCONTEXT(
		/*OUT*/ SQLCTX_HANDLE * contextHandle);

Int32 WSQL_EXEC_DELETECONTEXT(
		/*IN*/ SQLCTX_HANDLE contextHandle);

Int32 WSQL_EXEC_RESETCONTEXT(
		/*IN*/ SQLCTX_HANDLE contextHandle, 
		/*IN*/ void *contextMsg);

Int32 WSQL_EXEC_DEALLOCDESC (
		/*IN*/ SQLDESC_ID * desc_id );

Int32 WSQL_EXEC_DEALLOCSTMT (
		/*IN*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_DEFINEDESC (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN* (SQLWHAT_DESC) */ Int32 what_descriptor,
		/*IN*/ SQLDESC_ID * sql_descriptor);

Int32 WSQL_EXEC_DESCRIBESTMT (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN OPTIONAL*/ SQLDESC_ID * output_descriptor);

Int32 WSQL_EXEC_DISASSOCFILENUMBER(/*IN*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_DROPCONTEXT (
		/*IN*/ SQLCTX_HANDLE context_handle );

Int32 WSQL_EXEC_EXEC (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_EXECCLOSE (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_EXECDIRECT (
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ SQLDESC_ID * sql_source,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

		Int32 WSQL_EXEC_EXECDIRECTDEALLOC(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ SQLDESC_ID * sql_source,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_EXECFETCH(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_CLEAREXECFETCHCLOSE(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
		/*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
		/*IN*/ Int32 num_input_ptr_pairs,
		/*IN*/ Int32 num_output_ptr_pairs,
		/*IN*/ Int32 num_total_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS input_ptr_pairs[],
		/*IN*/ struct SQLCLI_PTR_PAIRS output_ptr_pairs[]);

Int32 WSQL_EXEC_FETCH(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_FETCHCLOSE(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_FETCHMULTIPLE(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN  OPTIONAL*/ SQLDESC_ID * output_descriptor,
		/*IN*/                 Int32   rowset_size,
		/*IN*/                 Int32 * rowset_status_ptr,
		/*OUT*/                Int32 * rowset_nfetched,
		/*IN*/                 Int32   num_quadruple_fields,
		/*IN*/   struct SQLCLI_QUAD_FIELDS   quad_fields[]);

Int32 WSQL_EXEC_CANCEL (
		/*IN OPTIONAL*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_GETDESCENTRYCOUNT(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ SQLDESC_ID * output_descriptor);

Int32 WSQL_EXEC_GETDESCITEM(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 entry,
		/*IN* (SQLDESC_ITEM_ID) */ Int32 what_to_get,
		/*OUT OPTIONAL*/ Int32 * numeric_value,
		/*OUT OPTIONAL*/ char * string_value,
		/*IN OPTIONAL*/ Int32 max_string_len,
		/*OUT OPTIONAL*/ Int32 * len_of_item,
		/*IN OPTIONAL*/ Int32 start_from_offset);

Int32 WSQL_EXEC_GETDESCITEMS(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ SQLDESC_ITEM desc_items[],
		/*IN*/ SQLDESC_ID * value_num_descriptor,
		/*IN*/ SQLDESC_ID * output_descriptor);

Int32 WSQL_EXEC_GETDESCITEMS2(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 no_of_desc_items,
		/*IN*/ SQLDESC_ITEM desc_items[]);

Int32 WSQL_EXEC_GETDIAGNOSTICSSTMTINFO(
		/*IN*/ Int32 *stmt_info_items,
		/*IN*/ SQLDESC_ID * output_descriptor);

Int32 WSQL_EXEC_GETDIAGNOSTICSSTMTINFO2(
		/*IN OPTIONAL*/ SQLSTMT_ID * statement_id,
		/*IN* (SQLDIAG_STMT_INFO_ITEM_ID) */ Int32 what_to_get,
		/*OUT OPTIONAL*/ Int32 * numeric_value,
		/*OUT OPTIONAL*/ char * string_value,
		/*IN OPTIONAL*/ Int32 max_string_len,
		/*OUT OPTIONAL*/ Int32 * len_of_item);

Int32 WSQL_EXEC_GETDIAGNOSTICSCONDINFO2(
		/*IN* (SQLDIAG_COND_INFO_ITEM_ID) */ Int32 what_to_get,
		/*IN*/ Int32 conditionNum,
		/*OUT OPTIONAL*/ Int32 * numeric_value,
		/*OUT OPTIONAL*/ char * string_value,
		/*IN OPTIONAL */ Int32 max_string_len,
		/*OUT OPTIONAL*/ Int32 * len_of_item);

Int32 WSQL_EXEC_GETDIAGNOSTICSCONDINFO(
		/*IN*/ SQLDIAG_COND_INFO_ITEM *cond_info_items,
		/*IN*/ SQLDESC_ID * cond_num_descriptor,
		/*IN*/ SQLDESC_ID * output_descriptor);

Int32 WSQL_EXEC_GETMAINSQLSTATE(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/  Int32 sqlcode,
		/*OUT*/ char * sqlstate /* assumed to be char[6] */);

Int32 WSQL_EXEC_GETCSQLSTATE(
		/*OUT*/ char * sqlstate /* assumed to be char[6] */,
		/*IN*/  Int32 sqlcode);

Int32 WSQL_EXEC_GETCOBOLSQLSTATE(
		/*OUT*/ char * sqlstate /* assumed to be char[5] */,
		/*IN*/  Int32 sqlcode);

Int32 WSQL_EXEC_GETSQLSTATE(
		/*OUT*/ char * sqlstate /* assumed to be char[6] */);

Int32 WSQL_EXEC_GETSTMTATTR(
		/*IN*/                  SQLSTMT_ID * statement_id,
		/*IN* (SQLATTR_TYPE) */ Int32 attrName,
		/*OUT OPTIONAL*/        Int32 * numeric_value,
		/*OUT OPTIONAL*/        char * string_value,
		/*IN OPTIONAL*/         Int32   max_string_len,
		/*OUT OPTIONAL*/        Int32 * len_of_item);

Int32 WSQL_EXEC_GETMPCATALOG(
		/*IN*/    char * AnsiObjName,
		/*INOUT*/ char * MPObjName,
		/*IN*/    Int32   MPObjNameMaxLen,
		/*INOUT*/ Int32 * MPObjNameLen,
		/*OUT*/   char * MPCatalogName,
		/*IN*/    Int32   MPCatalogNameMaxLen,
		/*OUT*/   Int32 * MPCatalogNameLen);

Int32 WSQL_EXEC_GOAWAY(
		/*IN*/ SQLDESC_ID * tableNameDesc);

Int32 WSQL_EXEC_PREPARE(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ SQLDESC_ID * sql_source);

Int32 WSQL_EXEC_RESDESCNAME(
		/*INOUT*/ SQLDESC_ID * statement_id,
		/*IN OPTIONAL*/ SQLSTMT_ID * from_statement,
		/*IN OPTIONAL (SQLWHAT_DESC) */ Int32 what_desc);

Int32 WSQL_EXEC_RESSTMTNAME(
		/*INOUT*/ SQLSTMT_ID * statement_id);

Int32 WSQL_EXEC_SETCURSORNAME(
		/*IN*/ SQLSTMT_ID * statement_id,
		/*IN*/ SQLSTMT_ID * cursor_name);

Int32 WSQL_EXEC_SETDESCENTRYCOUNT(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ SQLDESC_ID * input_descriptor);

Int32 WSQL_EXEC_SETDESCITEM(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 entry,
		/*IN* (SQLDESC_ITEM_ID) */ Int32 what_to_set,
		/*IN OPTIONAL*/ long numeric_value,
		/*IN OPTIONAL*/ char * string_value);

Int32 WSQL_EXEC_SETDESCITEMS(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ SQLDESC_ITEM desc_items[],
		/*IN*/ SQLDESC_ID * value_num_descriptor,
		/*IN*/ SQLDESC_ID * input_descriptor);

Int32 WSQL_EXEC_SETDESCITEMS2(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 no_of_desc_items,
		/*IN*/ SQLDESC_ITEM desc_items[]);

Int32 WSQL_EXEC_SETDESCPOINTERS(
		/*IN*/ SQLDESC_ID * sql_descriptor,
		/*IN*/ Int32 starting_entry,
		/*IN*/ Int32 num_ptr_pairs,
		/*IN*/ struct SQLCLI_PTR_PAIRS ptr_pairs[]);

Int32 WSQL_EXEC_SETROWSETDESCPOINTERS(
		SQLDESC_ID * desc_id,
		Int32    rowset_size,
		Int32    *rowset_status_ptr,
		Int32    starting_entry,
		Int32    num_quadruple_fields,
		struct SQLCLI_QUAD_FIELDS    quad_fields[]);

Int32 WSQL_EXEC_SETSTMTATTR(
		/*IN*/                  SQLSTMT_ID * statement_id,
		/*IN* (SQLATTR_TYPE) */ Int32 attrName,
		/*IN OPTIONAL*/         Int32 numeric_value,
		/*IN OPTIONAL*/         char * string_value);

Int32 WSQL_EXEC_SWITCHCONTEXT(
		/*IN*/ SQLCTX_HANDLE context_handle,
		/*OUT OPTIONAL*/ SQLCTX_HANDLE * prev_context_handle);

Int32 WSQL_EXEC_XACT(
		/*IN* (SQLTRANS_COMMAND) */ Int32 command,
		/*OUT OPTIONAL*/ SQLDESC_ID * transid_descriptor); 

// #ifdef _TMP_SQ_SECURITY
Int32 WSQL_EXEC_SETAUTHID(	//ACH delete?
		/*IN*/                 char   * authID,
		/*IN SQLAUTHID_TYPE */ Int32     authIDType,
		/* OUT OPTIONAL defaultRole*/ char *defaultRole,
		/* OUT OPTIONAL  role len*/ short *defaultRoleLen);
// #endif

//LCOV_EXCL_STOP

}

#endif
