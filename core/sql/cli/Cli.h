/**********************************************************************
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
**********************************************************************/
#ifndef CLI_H
#define CLI_H

/* -*-C++-*-
******************************************************************************
*
* File:         Cli.h
* Description:  Separation of Cli.cpp into a stub and client routine.
*               Originally done to help with NT work on single-threading
*               access to the CLI routines.  Will help to segregate
*               other work like DLL/Library work etc.
*               
* Created:      7/19/97
* Language:     C++
*
*
*
******************************************************************************
*/

#include <stdarg.h>
#include "Platform.h"
#include "SQLCLIdev.h"
#include "ExpLOBexternal.h"
#include "ComDiags.h"
class Statement;
class CliGlobals;
class ContextCli;
class Descriptor;

// these enums are used in method, PerformTasks.
enum CliTasks 
{
  CLI_PT_CLEAR_DIAGS         = 0x0001,
  CLI_PT_EXEC                = 0x0002,
  CLI_PT_FETCH               = 0x0004,
  CLI_PT_CLOSE               = 0x0008, 
  CLI_PT_GET_INPUT_DESC      = 0x0010,
  CLI_PT_GET_OUTPUT_DESC     = 0x0020,
  CLI_PT_SPECIAL_END_PROCESS = 0x0040,
  CLI_PT_CLOSE_ON_ERROR      = 0x0080,
  CLI_PT_PROLOGUE            = 0x0100,
  CLI_PT_EPILOGUE            = 0x0200,
  CLI_PT_OPT_STMT_INFO       = 0x0400,
  CLI_PT_CLEAREXECFETCHCLOSE = 0x0800,
  CLI_PT_SET_AQR_INFO        = 0x1000
};

// -----------------------------------------------------------------------
// Internal CLI layer, common to NT and NSK platforms. On NT, this is
// similar to the external layer, except that no try/catch block is
// activated and that calls are not protected by a semaphore. On NSK,
// the executor segment is revealed at this time and the switch to
// PRIV has occurred. The internal layer has an additional argument:
// the CLI Globals.
//
// The main reason for giving these functions extern "C" linkage is
// function redirection. This makes the function names easier to
// handle since they don't get mangled.
// -----------------------------------------------------------------------



struct DiagsConditionItem {
  Lng32 item_id;
  char *var_ptr;
  Lng32 length;
  Lng32 output_entry;
  Lng32 condition_index;

  // When MESSAGE_OCTET_LENGTH is retrieved from the diags area, 
  // its value will be dependent on the CHARACTER SET defined 
  // on the host variable to which MESSAGE_TEXT is assigned. 
  // This new data member "charset" is used to remember the 
  // CHARACTER SET defined on the host variable. 
  Lng32 charset;
};

extern "C"
{

   Lng32 SQLCLI_AllocDesc(/*IN*/          CliGlobals * cliGlobals,
			 /*INOUT*/       SQLDESC_ID * desc_id,


                         /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor);

   Lng32 SQLCLI_AllocStmt(/*IN*/          CliGlobals * cliGlobals,
			 /*INOUT*/       SQLSTMT_ID * new_statement_id,
                         /*IN OPTIONAL*/ SQLSTMT_ID * cloned_statement);

   Lng32 SQLCLI_AllocStmtForRS(/*IN*/ CliGlobals *cliGlobals,
                              /*IN*/ SQLSTMT_ID *callStmtId,
                              /*IN*/ Lng32 resultSetIndex,
                              /*INOUT*/ SQLSTMT_ID *resultSetStmtId);

   Lng32 SQLCLI_AssocFileNumber(/*IN*/    CliGlobals   * cliGlobals,
                               /*IN*/    SQLSTMT_ID * statement_id,
		               /*IN*/    short         file_number);

Int32  SQLCLI_GetDiskMaxSize (
			      /*IN*/ CliGlobals *cliGlobals,
			      /*IN*/ char *volname,
			      /*OUT*/ Int64 *totalCapacity,
			      /*OUT*/ Int64 *totalFreespace);
Int32  SQLCLI_GetListOfDisks (
			      /*IN*/ CliGlobals *cliGlobals,
			      /*IN/OUT*/ char *diskBuffer,
			      /* OUT */ Int32 *numTSEs,
			      /* OUT */ Int32 *maxTSELength,
			      /* IN/OUT */ Int32 *diskBufferLength
			      );
  Lng32 SQLCLI_BreakEnabled(/*IN*/    CliGlobals   * cliGlobals,
			   /*IN*/    UInt32 enabled );

  Lng32 SQLCLI_SPBreakRecvd(/*IN*/    CliGlobals   * cliGlobals,
			   /*OUT*/   UInt32 *breakRecvd);


   Lng32 SQLCLI_ClearDiagnostics(/*IN*/          CliGlobals *cliGlobals,
				/*IN OPTIONAL*/ SQLSTMT_ID *statement_id);

   Lng32 SQLCLI_CloseStmt(/*IN*/ CliGlobals * cliGlobals,
                         /*IN*/ SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_CreateContext(/*IN*/  CliGlobals    * cliGlobals,
                             /*OUT*/ SQLCTX_HANDLE * context_handle,
			     /*IN*/ char *sqlAuthId,
                             /*IN*/ Lng32 /* for future use */);


   Lng32 SQLCLI_CurrentContext(/*IN*/  CliGlobals    * cliGlobals,
                             /*OUT*/ SQLCTX_HANDLE * context_handle);


   Lng32 SQLCLI_DropModule(/*IN*/ CliGlobals   * cliGlobals,
			  /*IN*/ const SQLMODULE_ID * module_name);

   Lng32 SQLCLI_DeleteContext(/*IN*/  CliGlobals    * cliGlobals,
                             /*IN*/ SQLCTX_HANDLE context_handle);


   Lng32 SQLCLI_ResetContext(/*IN*/  CliGlobals    * cliGlobals,
                             /*IN*/ SQLCTX_HANDLE context_handle,
				     /*IN*/ void *contextMsg);

 
  Lng32 SQLCLI_GetUdrErrorFlags_Internal(/*IN*/ CliGlobals * cliGlobals,
					/*OUT*/ Lng32 * udrErrorFlags);


  Lng32 SQLCLI_SetUdrAttributes_Internal(/*IN*/ CliGlobals * cliGlobals,
                                        /*IN*/ Lng32 sqlAccessMode,
                                        /*IN*/ Lng32 /* for future use */);


  Lng32 SQLCLI_ResetUdrErrorFlags_Internal(/*IN*/ CliGlobals * cliGlobals);

  Lng32 SQLCLI_SetUdrRuntimeOptions_Internal(/*IN*/ CliGlobals * cliGlobals,
                                            /*IN*/ const char *options,
                                            /*IN*/ ULng32 optionsLen,
                                            /*IN*/ const char *delimiters,
                                            /*IN*/ ULng32 delimsLen);

   Lng32 SQLCLI_DeallocDesc(/*IN*/ CliGlobals * cliGlobals,
                           /*IN*/ SQLDESC_ID * desc_id);

   Lng32 SQLCLI_DeallocStmt(/*IN*/ CliGlobals * cliGlobals,
                           /*IN*/ SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_DefineDesc(/*IN*/ CliGlobals * cliGlobals,
                          /*IN*/ SQLSTMT_ID * statement_id,
         /* (SQLWHAT_DESC) *IN*/       Lng32   what_descriptor,
                          /*IN*/ SQLDESC_ID * sql_descriptor);

   Lng32 SQLCLI_DescribeStmt(/*IN*/          CliGlobals * cliGlobals,
                            /*IN*/          SQLSTMT_ID * statement_id,
                            /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                            /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor);

   Lng32 SQLCLI_DisassocFileNumber(/*IN*/          CliGlobals * cliGlobals,
                                  /*IN*/          SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_DropContext(/*IN*/  CliGlobals    * cliGlobals,
			   /*IN*/ SQLCTX_HANDLE    context_handle);

   Lng32 SQLCLI_Exec(/*IN*/          CliGlobals * cliGlobals,
                    /*IN*/          SQLSTMT_ID * statement_id,
                    /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                    /*IN*/                Lng32   num_ptr_pairs,
                    /*IN*/             va_list   ap,
                    /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);

   Lng32 SQLCLI_ExecClose(/*IN*/          CliGlobals * cliGlobals,
                         /*IN*/          SQLSTMT_ID * statement_id,
                         /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                         /*IN*/                Lng32   num_ptr_pairs,
                         /*IN*/             va_list   ap,
                         /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);

   Lng32 SQLCLI_ExecDirect(/*IN*/ CliGlobals *cliGlobals,
                          /*IN*/          SQLSTMT_ID * statement_id,
                          /*IN*/          SQLDESC_ID * sql_source,
                          /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                          /*IN*/                Lng32   num_ptr_pairs,
                          /*IN*/             va_list   ap,
                          /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);
Lng32 SQLCLI_ExecDirect2(/*IN*/           CliGlobals * cliGlobals,
               /*IN*/           SQLSTMT_ID * statement_id,
               /*IN*/           SQLDESC_ID * sql_source,
	       /*IN*/           Lng32 prepFlags,
               /*IN  OPTIONAL*/ SQLDESC_ID * input_descriptor,
               /*IN*/                 Lng32   num_ptr_pairs,
               /*IN*/              va_list   ap,
	       /*IN*/     SQLCLI_PTR_PAIRS   ptr_pairs[]
			 );
   Lng32 SQLCLI_ExecDirectDealloc(/*IN*/          CliGlobals * cliGlobals,
                                 /*IN*/          SQLSTMT_ID * statement_id,
                                 /*IN*/          SQLDESC_ID * sql_source,
                                 /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                                 /*IN*/                Lng32   num_ptr_pairs,
                                 /*IN*/             va_list   ap,
                                 /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);

   Lng32 SQLCLI_ExecFetch(/*IN*/          CliGlobals * cliGlobals,
                         /*IN*/          SQLSTMT_ID * statement_id,
                         /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                         /*IN*/                Lng32   num_ptr_pairs,
                         /*IN*/             va_list   ap,
                         /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);

   Lng32 SQLCLI_ClearExecFetchClose(/*IN*/          CliGlobals * cliGlobals,
                   /*IN*/          SQLSTMT_ID * statement_id,
                   /*IN OPTIONAL*/ SQLDESC_ID * input_descriptor,
                   /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
                   /*IN*/                Lng32   num_input_ptr_pairs,
                   /*IN*/                Lng32   num_output_ptr_pairs,
                   /*IN*/                Lng32   num_total_ptr_pairs,
                   /*IN*/             va_list   ap,
                   /*IN*/    SQLCLI_PTR_PAIRS   input_ptr_pairs[],
                   /*IN*/    SQLCLI_PTR_PAIRS   output_ptr_pairs[]);

   Lng32 SQLCLI_Fetch(/*IN*/          CliGlobals * cliGlobals,
                     /*IN*/          SQLSTMT_ID * statement_id,
                     /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
                     /*IN*/                Lng32   num_ptr_pairs,
                     /*IN*/             va_list   ap,
                     /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);

   Lng32 SQLCLI_FetchClose(/*IN*/          CliGlobals * cliGlobals,
                          /*IN*/          SQLSTMT_ID * statement_id,
                          /*IN OPTIONAL*/ SQLDESC_ID * output_descriptor,
                          /*IN*/                Lng32   num_ptr_pairs,
                          /*IN*/             va_list   ap,
                          /*IN*/    SQLCLI_PTR_PAIRS   ptr_pairs[]);


   Lng32 SQLCLI_FetchMultiple(/*IN*/          CliGlobals * cliGlobals,
                            /*IN*/           SQLSTMT_ID * statement_id,
                            /*IN  OPTIONAL*/ SQLDESC_ID * output_descriptor,
                            /*IN*/                 Lng32   rowset_size,
                            /*IN*/                 Lng32 * rowset_status_ptr,
                            /*OUT*/                Lng32 * rowset_nfetched,
                            /*IN*/                 Lng32   num_quadruple_fields,
                            /*IN*/              va_list   ap,
                            /*IN*/   SQLCLI_QUAD_FIELDS   quad_fields[]);

   Lng32 SQLCLI_Cancel(/*IN*/          CliGlobals * cliGlobals,
                      /*IN OPTIONAL*/ SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_GetDescEntryCount(/*IN*/ CliGlobals * cliGlobals,
                                 /*IN*/ SQLDESC_ID * sql_descriptor,
                                 /*IN*/ SQLDESC_ID * output_descriptor);

   Lng32 SQLCLI_GetDescItem(/*IN*/     CliGlobals * cliGlobals,
                           /*IN*/     SQLDESC_ID * sql_descriptor,
                           /*IN*/           Lng32   entry,
       /* (SQLDESC_ITEM_ID) *IN*/           Lng32   what_to_get,
                           /*OUT OPTIONAL*/ void    * numeric_value,
                           /*OUT OPTIONAL*/ char    * string_value,
                           /*IN OPTIONAL*/  Lng32   max_string_len,
                           /*OUT OPTIONAL*/ Lng32 * len_of_item,
                           /*IN OPTIONAL*/  Lng32   start_from_offset);

   Lng32 SQLCLI_GetDescItems(/*IN*/   CliGlobals * cliGlobals,
                            /*IN*/   SQLDESC_ID * sql_descriptor,
                            /*IN*/ SQLDESC_ITEM   desc_items[],
                            /*IN*/   SQLDESC_ID * value_num_descriptor,
                            /*IN*/   SQLDESC_ID * output_descriptor);

   Lng32 SQLCLI_GetDescItems2(/*IN*/ CliGlobals * cliGlobals,
			     /*IN*/ SQLDESC_ID * desc_id,
			     /*IN*/ Lng32 no_of_desc_items,
			     /*IN*/ SQLDESC_ITEM desc_items[]);

   Lng32 SQLCLI_GetDiagnosticsStmtInfo(/*IN*/ CliGlobals * cliGlobals,
                                      /*IN*/       Lng32 * stmt_info_items,
                                      /*IN*/ SQLDESC_ID * output_descriptor);

   Lng32 SQLCLI_GetDiagnosticsStmtInfo2(
	/*IN*/ CliGlobals * cliGlobals,
	/*IN OPTIONAL*/ SQLSTMT_ID * statement_id,
	/*IN* (SQLDIAG_STMT_INFO_ITEM_ID) */ Lng32 what_to_get,
	/*OUT OPTIONAL*/ void * numeric_value,
	/*OUT OPTIONAL*/ char * string_value,
	/*IN OPTIONAL*/ Lng32 max_string_len,
	/*OUT OPTIONAL*/ Lng32 * len_of_item);

   Lng32 SQLCLI_GetDiagnosticsCondInfo(
                /*IN*/             CliGlobals * cliGlobals,
                /*IN*/ SQLDIAG_COND_INFO_ITEM * cond_info_items,
                /*IN*/             SQLDESC_ID * cond_num_descriptor,
                /*IN*/             SQLDESC_ID * output_descriptor,
		/*OUT*/   IpcMessageBufferPtr   message_buffer_ptr,
		/*IN*/      IpcMessageObjSize   message_obj_size,
		/*OUT*/     IpcMessageObjSize * message_obj_size_needed,
		/*OUT*/     IpcMessageObjType * message_obj_type,
		/*OUT*/  IpcMessageObjVersion * message_obj_version,
		/*IN*/                   Lng32   condition_item_count,
		/*OUT*/                  Lng32 * condition_item_count_needed,
		/*OUT*/    DiagsConditionItem * condition_item_array,
	        /*OUT*/    SQLMXLoggingArea::ExperienceLevel * emsEventEL);

   Lng32 SQLCLI_GetPackedDiagnostics(
                /*IN*/             CliGlobals * cliGlobals,
		/*OUT*/   IpcMessageBufferPtr   message_buffer_ptr,
		/*IN*/      IpcMessageObjSize   message_obj_size,
		/*OUT*/     IpcMessageObjSize * message_obj_size_needed,
		/*OUT*/     IpcMessageObjType * message_obj_type,
		/*OUT*/  IpcMessageObjVersion * message_obj_version);


   Lng32 SQLCLI_GetSQLCODE(/*IN*/  CliGlobals * cliGlobals,
                           /*OUT*/       Lng32 * theSQLCODE);

  Lng32 SQLCLI_GetDiagnosticsCondInfo2(
                /*IN*/             CliGlobals * cliGlobals,
		/*IN* (SQLDIAG_COND_INFO_ITEM_ID) */ Lng32 what_to_get,
		/*IN*/ Lng32 conditionNum,
		/*OUT OPTIONAL*/ Lng32 * numeric_value,
		/*OUT OPTIONAL*/ char * string_value,
		/*IN OPTIONAL */ Lng32 max_string_len,
		/*OUT OPTIONAL*/ Lng32 * len_of_item);

  Lng32 SQLCLI_GetDiagnosticsCondInfo3 (
		/*IN*/ CliGlobals * cliGlobals,
		/*IN*/ Lng32 no_of_condition_items,
		/*IN*/ SQLDIAG_COND_INFO_ITEM_VALUE
			  diag_cond_info_item_values[],
		/*OUT*/ IpcMessageObjSize * message_obj_size_needed);

   Lng32 SQLCLI_GetDiagnosticsArea(
                /*IN*/             CliGlobals * cliGlobals,
		/*OUT*/   IpcMessageBufferPtr   message_buffer_ptr,
		/*IN*/      IpcMessageObjSize   message_obj_size,
		/*OUT*/     IpcMessageObjType * message_obj_type,
		/*OUT*/  IpcMessageObjVersion * message_obj_version);

  Lng32 SQLCLI_GetMainSQLSTATE(/*IN*/ CliGlobals * cliGlobals,
                /*IN*/ SQLSTMT_ID * stmtId,
		/*IN*/  Lng32 sqlcode,
		/*OUT*/ char * sqlstate /* assumed to be char[6] */);


   Lng32 SQLCLI_GetSQLSTATE(/*IN*/  CliGlobals * cliGlobals,
                           /*OUT*/       char * theSQLSTATE);

   Lng32 SQLCLI_GetCSQLSTATE(/*IN*/  CliGlobals * cliGlobals,
                            /*OUT*/       char * theSQLSTATE,
                            /*IN*/        Lng32   theSQLCODE);

   Lng32 SQLCLI_GetCobolSQLSTATE(/*IN*/  CliGlobals * cliGlobals,
                                /*OUT*/       char * theSQLSTATE,
                                /*IN*/        Lng32   theSQLCODE);

   // For internal use only -- do not document!
   Lng32 SQLCLI_GetRootTdb_Internal(/*IN*/ CliGlobals * cliGlobals,
                                   /*INOUT*/    char * roottdb_ptr,
                                   /*IN*/ Int32 roottdb_len,
				   /*INOUT*/    char * srcstr_ptr,
                                   /*IN*/ Int32 srcstr_len,
                                   /*IN*/ SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_GetRootTdbSize_Internal(/*IN*/ CliGlobals * cliGlobals,
                                       /*INOUT*/    Lng32 * roottdb_size,
				       /*INOUT*/    Lng32 * srcstr_size,
                                       /*IN*/ SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_GetCollectStatsType_Internal(
	/*IN*/ CliGlobals * cliGlobals,
	/*OUT*/ ULng32 * collectStatsType,
	/*IN*/ SQLSTMT_ID * statement_id);

   Lng32 SQLCLI_GetTotalTcbSpace(/*IN*/ CliGlobals * cliGlobals,
				/*IN*/ char * comTdb,
				/*IN*/ char * otherInfo);

   Lng32 SQLCLI_GetMPCatalog(/*IN*/ CliGlobals * cliGlobals,
                            /*IN*/  char * ANSINameObj,
                            /*OUT*/ char * MPObjName,
                            /*IN*/  Lng32   MPObjNameMaxLen,
                            /*OUT*/ Lng32 * MPObjNameLen,
                            /*OUT*/ char * MPCatalogName,
                            /*IN*/  Lng32   MPCatalogNameMaxLen,
                            /*OUT*/ Lng32 * MPCatalogNameLen);

   Lng32 SQLCLI_GetPfsSize(/*IN*/ CliGlobals *cliGlobals,
                           /*OUT*/ Int32 *pfsSize,
                           /*OUT*/ Int32 *pfsCurUse,
                           /*OUT*/ Int32 *pfsMaxUse);
   Lng32 SQLCLI_CleanUpPfsResources(/*IN*/ CliGlobals *cliGlobals);

   Lng32 SQLCLI_PerformTasks(
	/*IN*/ CliGlobals * cliGlobals,
	/*IN*/ULng32 tasks,
	/*IN*/SQLSTMT_ID * statement_id,
	/*IN  OPTIONAL*/ SQLDESC_ID * input_descriptor,
	/*IN  OPTIONAL*/ SQLDESC_ID * output_descriptor,
	/*IN*/ Lng32 num_input_ptr_pairs,
	/*IN*/ Lng32 num_output_ptr_pairs,
	/*IN*/ va_list ap,
	/*IN*/ SQLCLI_PTR_PAIRS input_ptr_pairs[],
	/*IN*/ SQLCLI_PTR_PAIRS output_ptr_pairs[]);

   Lng32 SQLCLI_Prepare(/*IN*/ CliGlobals * cliGlobals,
                       /*IN*/ SQLSTMT_ID * statement_id,
                       /*IN*/ SQLDESC_ID * sql_source);

   Lng32 SQLCLI_Prepare2(/*IN*/ CliGlobals * cliGlobals,
			/*IN*/ SQLSTMT_ID * statement_id,
			/*IN*/ SQLDESC_ID * sql_source,
			/*INOUT*/ char * gencode_ptr,
			/*IN*/    Lng32   gencode_len,
			/*INOUT*/ Lng32 * ret_gencode_len,
			/*INOUT*/ SQL_QUERY_COST_INFO *query_cost_info,
			/*INOUT*/ SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info,
			/*INOUT*/ char * uniqueStmtId,
			/*INOUT*/ Lng32 * uniqueStmtIdLen,
			/*IN*/    ULng32 flags);

   Lng32 SQLCLI_GetExplainData(
                               /*IN*/ CliGlobals * cliGlobals,
                               /*IN*/    SQLSTMT_ID * statement_id,
                               /*INOUT*/ char * explain_ptr,
                               /*IN*/    Int32 explain_len,
                               /*INOUT*/ Int32 * ret_explain_len);

   Lng32 SQLCLI_StoreExplainData(
                                 /*IN*/ CliGlobals * cliGlobals,
                                 /*IN*/ Int64 * exec_start_utc_ts,
                                 /*IN*/   char * query_id,
                                 /*INOUT*/ char * explain_ptr,
                                 /*IN*/    Int32 explain_len);
  
   Lng32 SQLCLI_ResDescName(/*IN*/          CliGlobals * cliGlobals,
                            /*INOUT*/       SQLDESC_ID * statement_id,
                            /*IN OPTIONAL*/ SQLSTMT_ID * from_statement,
                            /* (SQLWHAT_DESC) *IN OPTIONAL*/       Lng32   what_desc);
  
   Lng32 SQLCLI_ResStmtName(/*IN*/    CliGlobals * cliGlobals,
                           /*INOUT*/ SQLSTMT_ID * statement_id);


   Lng32 SQLCLI_SetCursorName(/*IN*/ CliGlobals * cliGlobals,
                             /*IN*/ SQLSTMT_ID * statement_id,
                             /*IN*/ SQLSTMT_ID * cursor_name);

Lng32 SQLCLI_SetStmtAttr( /*IN*/ CliGlobals *cliGlobals,
                         /*IN*/ SQLSTMT_ID *statement_id,
                         /*IN* SQLATTR_TYPE */ Lng32 attrName,
                         /*IN OPTIONAL*/       Lng32   numeric_value,
                         /*IN OPTIONAL*/       char * string_value);

Lng32 SQLCLI_GetSessionAttr( /*IN*/ CliGlobals *cliGlobals,
			    /*IN SESSIONATTR_TYPE  */ Lng32 attrName,
			    /*OUT OPTIONAL*/      Lng32 * numeric_value,
			    /*OUT OPTIONAL*/      char * string_value,
			    /*IN OPTIONAL*/       Lng32   max_string_len,
			    /*OUT OPTIONAL*/      Lng32 * len_of_item);

Lng32 SQLCLI_SetSessionAttr(/*IN*/ CliGlobals *cliGlobals,
			    /*IN SESSIONATTR_TYPE*/ Lng32 attrName,
			    /*IN OPTIONAL*/         Lng32 numeric_value,
			    /*IN OPTIONAL*/         char *string_value);
                            
Lng32 SQLCLI_GetAuthID(
   CliGlobals * cliGlobals,
   const char * authName,
   Lng32 & authID);
                            
Lng32 SQLCLI_GetAuthName (
    /*IN*/            CliGlobals *cliGlobals,
    /*IN*/            Lng32       auth_id,
    /*OUT*/           char       *string_value,
    /*IN*/            Lng32       max_string_len,
    /*OUT */          Lng32      &len_of_item);

Lng32 SQLCLI_GetDatabaseUserName( /*IN*/ CliGlobals *cliGlobals,
                                  /*IN*/            Lng32   user_id,
                                  /*OUT*/           char   *string_value,
                                  /*IN*/            Lng32   max_string_len,
                                  /*OUT OPTIONAL*/  Lng32  *len_of_item);

Lng32 SQLCLI_GetDatabaseUserID( /*IN*/ CliGlobals *cliGlobals,
                                /*IN*/     char   *string_value,
                                /*OUT*/    Lng32  *numeric_value);

Int32 SQLCLI_GetAuthState (
    /*IN*/            CliGlobals *cliGlobals,
    /*OUT*/           bool      &authenticationEnabled,
    /*OUT*/           bool      &authorizationEnabled,
    /*OUT*/           bool      &authorizationReady,
    /*OUT*/           bool      &auditingEnabled);

Lng32 SQLCLI_GetRoleList(
   CliGlobals *cliGlobals,
   Int32 &numEntries,
   Int32 *& roleIDs,
   Int32 *& granteeIDs);

Lng32 SQLCLI_ResetRoleList (
    /*IN*/            CliGlobals *cliGlobals);

Lng32 SQLCLI_GetUniqueQueryIdAttrs( /*IN*/ CliGlobals *cliGlobals,
				   /*IN*/    char * queryId,
				   /*IN*/    Lng32 queryIdLen,
				   /*IN*/    Lng32 no_of_attrs,
				   /*INOUT*/ UNIQUEQUERYID_ATTR queryid_attrs[]);

Lng32 SQLCLI_GetStmtAttr( /*IN*/ CliGlobals *cliGlobals,
                         /*IN*/ SQLSTMT_ID *statement_id,
                         /*IN SQLATTR_TYPE  */ Lng32 attrName,
                         /*OUT OPTIONAL*/      Lng32 * numeric_value,
                         /*OUT OPTIONAL*/      char * string_value,
                         /*IN OPTIONAL*/       Lng32   max_string_len,
                         /*OUT OPTIONAL*/      Lng32 * len_of_item);

Lng32 SQLCLI_GetStmtAttrs( /*IN*/ CliGlobals *cliGlobals,
                          /*IN*/ SQLSTMT_ID *statement_id,
                          /*IN*/           Lng32 number_of_attrs,
                          /*INOUT*/        SQLSTMT_ATTR attrs[],
                          /*OUT OPTIONAL*/ Lng32 * num_returned);
  
   Lng32 SQLCLI_SetDescEntryCount(/*IN*/ CliGlobals * cliGlobals,
                                 /*IN*/ SQLDESC_ID * sql_descriptor,
                                 /*IN*/ SQLDESC_ID * input_descriptor);

   Lng32 SQLCLI_SetDescItem(/*IN*/          CliGlobals * cliGlobals,
                           /*IN*/          SQLDESC_ID * sql_descriptor,
                           /*IN*/                Lng32   entry,
       /* (SQLDESC_ITEM_ID) *IN*/                Lng32   what_to_set,
                           /*IN OPTIONAL*/       Long    numeric_value,
                           /*IN OPTIONAL*/       char * string_value);

   Lng32 SQLCLI_SetDescItems(/*IN*/   CliGlobals * cliGlobals,
                            /*IN*/   SQLDESC_ID * sql_descriptor,
                            /*IN*/   SQLDESC_ITEM   desc_items[],
                            /*IN*/   SQLDESC_ID * value_num_descriptor,
                            /*IN*/   SQLDESC_ID * input_descriptor);

   Lng32 SQLCLI_SetDescItems2(/*IN*/   CliGlobals * cliGlobals,
			     /*IN*/   SQLDESC_ID * sql_descriptor,
			     /*IN*/   Lng32 no_of_desc_items,
			     /*IN*/   SQLDESC_ITEM   desc_items[]);

   Lng32 SQLCLI_SetDescPointers(/*IN*/       CliGlobals * cliGlobals,
                               /*IN*/       SQLDESC_ID * sql_descriptor,
                               /*IN*/             Lng32   starting_entry,
                               /*IN*/             Lng32   num_ptr_pairs,
                               /*IN*/          va_list   ap,
                               /*IN*/ SQLCLI_PTR_PAIRS   ptr_pairs[]);

   Lng32 SQLCLI_SetRowsetDescPointers(CliGlobals * cliGlobals,
                                SQLDESC_ID * desc_id, 
                                      Lng32   rowset_size,
                                      Lng32 * rowset_status_ptr,
                                      Lng32   starting_entry,
                                      Lng32   num_quadruple_fields,
                                   va_list   ap,
                        SQLCLI_QUAD_FIELDS   quad_fields[]);

   Lng32 SQLCLI_GetRowsetNumprocessed(CliGlobals * cliGlobals,
                                     SQLDESC_ID * desc_id, 
                                           Lng32 & rowset_nprocessed);

   Lng32 SQLCLI_SwitchContext(/*IN*/           CliGlobals    * cliGlobals,
                             /*IN*/           SQLCTX_HANDLE   ctxt_handle,
                              /*OUT OPTIONAL*/ SQLCTX_HANDLE * prev_ctxt_handle,
                              /*IN */ bool allowSwitchBackToDefault);

   Lng32 SQLCLI_Xact(/*IN*/                 CliGlobals * cliGlobals,
                    /*IN* (SQLTRANS_COMMAND) */  Lng32   command,
                    /*OUT OPTIONAL*/       SQLDESC_ID * transid_descriptor);
                    
Lng32 SQLCLI_SetAuthID(
   CliGlobals * cliGlobals,	      /*IN*/
   const char * externalUsername,     /*IN*/
   const char * databaseUsername,     /*IN*/
   const char * authToken,	      /*IN*/
   Int32        authTokenLen,	      /*IN*/
   Int32        effectiveUserID,      /*IN*/
   Int32        sessionUserID);	      /*IN*/
                    
/* temporary functions -- for use by sqlcat simulator only */

   Lng32 SQLCLI_AllocDescInt(/*IN*/          CliGlobals * cliGlobals,
                            /*INOUT*/       SQLDESC_ID * desc_id,
                            /*IN OPTIONAL*/       Lng32   max_entries);

   Lng32 SQLCLI_GetDescEntryCountInt(/*IN*/  CliGlobals * cliGlobals,
                                    /*IN*/  SQLDESC_ID * sql_descriptor,
                                    /*OUT*/       Lng32 * num_entries);

   Lng32 SQLCLI_SetDescEntryCountInt(/*IN*/ CliGlobals * cliGlobals,
                                    /*IN*/ SQLDESC_ID * sql_descriptor,
                                    /*IN*/       Lng32   num_entries);

   Lng32 SQLCLI_MergeDiagnostics(/*IN*/    CliGlobals   * cliGlobals,
                                /*INOUT*/ ComDiagsArea & newDiags);

   Lng32 SQLCLI_GetRowsAffected(/*IN*/ CliGlobals * cliGlobals,
                               /*?*/ SQLSTMT_ID * statement_id,
                               /*?*/      Int64 & rowsAffected);

//Function to get the length of the desc_items array
//returns the length if no error occurs, if error_occurred
//is 1 on return then return value indicates error
Lng32 SQLCLI_GetDescItemsEntryCount(
                /*IN*/ CliGlobals * cliGlobals,
                /*IN*/ SQLDESC_ID * desc_id,
                /*IN*/ SQLDESC_ITEM desc_items[],
                /*IN*/ SQLDESC_ID * value_num_descriptor,
                /*IN*/ SQLDESC_ID * output_descriptor,
                /*Out*/Int32 & error_occurred);

Lng32 SQLCLI_SetParserFlagsForExSqlComp_Internal(
     /*IN*/ CliGlobals * cliGlobals,
     /*IN*/ULng32 flagbits);

Lng32 SQLCLI_ResetParserFlagsForExSqlComp_Internal(
     /*IN*/ CliGlobals * cliGlobals,
     /*IN*/ULng32 flagbits);

Lng32 SQLCLI_AssignParserFlagsForExSqlComp_Internal(
     /*IN*/ CliGlobals * cliGlobals,
     /*IN*/ULng32 flagbits);

Lng32 SQLCLI_GetParserFlagsForExSqlComp_Internal(
     /*IN*/ CliGlobals * cliGlobals,
     /*IN*/ULng32 &flagbits);

Lng32 SQLCLI_OutputValueIntoNumericHostvar(
                /*IN*/ CliGlobals * cliGlobals,
                /*IN*/ SQLDESC_ID * output_descriptor,
		/*IN*/       Lng32   desc_entry,
		/*IN*/       Lng32   value);

Lng32 SQLCLI_GetSystemVolume_Internal(
     /*IN*/ CliGlobals * cliGlobals,
     /*INOUT*/    char * SMDLocation,
     /*INOUT*/    Lng32 *fsError);


// returns TRUE if the volume name is $SYSTEM
bool SQLCLI_IsSystemVolume_Internal (const char *const fileName);

// returns TRUE if the volume component is 7 chars (not including $)
// for examples:  $CHAR777
bool SQLCLI_IsVolume7Chars_Internal (const char *const fileName);

const char *const *const SQLCLI_GetListOfVolumes_Internal();
short SQLCLI_GetDefaultVolume_Internal(char *const outBuf,
				       const short outBufMaxLen,
				       short &defaultVolLen);

Lng32 SQLCLI_GetListOfAuditedVolumes_Internal(CliGlobals *cliGlobals,
                                             char **volNames,
                                             Lng32 *numOfVols);

Lng32 SQLCLI_GetNumOfQualifyingVolumes_Internal(CliGlobals *cliGloblas,
                                               const char *nodeName,
                                               Lng32 *numOfVols);

Lng32 SQLCLI_GetListOfQualifyingVolumes_Internal(CliGlobals *cliGloblas,
                                                const char *nodeName,
                                                Lng32 numOfVols,
                                                char **volNames,
                                                Lng32 *cpuNums,
                                                Lng32 *capacities,
                                                Lng32 *freespaces,
                                                Lng32 *largestFragments);

Lng32 SQLCLI_SetEnviron_Internal(/*IN*/ CliGlobals * cliGlobals,
				char ** envvars, Lng32 propagate);

  Lng32 SQLCLI_SetCompilerVersion_Internal(CliGlobals *cliGlobals, short mxcmpVersion, char *nodeName);

  Lng32 SQLCLI_GetCompilerVersion_Internal(CliGlobals *cliGlobals, short &mxcmpVersion, char *nodeName );

short SQLCLI_GETANSINAME(
     CliGlobals    *cliGlobals,
    const char     *guardianName,
    Lng32           guardianNameLen,
    char           *ansiName,
    Lng32           ansiNameMaxLength,
    Lng32           *returnedAnsiNameLength,
    char           *nameSpace,
    char           *partitionName,
    Lng32           partitionNameMaxLength,
    Lng32           *returnedPartitionNameLength);

Lng32 SQLCLI_DecodeAndFormatKey( 
     CliGlobals *cliGlobals,
     void  * RCB_Pointer_Addr,     // in
     void  * KeyAddr,              // in
     Int32   KeyLength,            // in
     void  * DecodedKeyBufAddr,    // in/out: where decoded key to be returned
     void  * FormattedKeyBufAddr,  // in/out: formatted key to be returned
     Int32   FormattedKeyBufLen,   // in
     Int32 * NeededKeyBufLen);     // out: required buffer size to be returned

Lng32 SQLCLI_GetPartitionKeyFromRow(
                    CliGlobals *cliGlobals,
                    void    * RCB_Pointer_Addr,
		    void    * Row_Addr,
		    Int32     Row_Length,
		    void    * KeyAddr,
		    Int32     KeyLength);

Lng32 SQLCLI_SetErrorCodeInRTS(
      CliGlobals *cliGlobals,
      SQLSTMT_ID * statement_id,
      Lng32     sqlErrorCode); 

Lng32 SQLCLI_SetSecInvalidKeys(CliGlobals *cliGlobals,
            /* IN */   Int32 numSiKeys,
           /* IN */    SQL_QIKEY siKeys[]);

Lng32 SQLCLI_GetSecInvalidKeys(CliGlobals *cliGlobals,
            /* IN */      Int64 prevTimestamp,
            /* IN/OUT */  SQL_QIKEY siKeys[],
            /* IN */      Int32 maxNumSiKeys,
            /* IN/OUT */  Int32 *returnedNumSiKeys,
            /* IN/OUT */  Int64 *maxTimestamp);

Lng32 SQLCLI_SetLobLock(CliGlobals *cliGlobals,
                        /* IN */    char * lobLockId
                        );
Lng32 SQLCLI_CheckLobLock(CliGlobals *cliGlobals,
                        /* IN */   char *lobLockId,
                        /*OUT */ NABoolean *found
                          );
Lng32 SQLCLI_GetStatistics2(CliGlobals *cliGlobals,
            /* IN */  	short statsReqType,
	    /* IN */  	char *statsReqStr,
	    /* IN */  	Lng32 statsReqStrLen,
	    /* IN */	short activeQueryNum,
	    /* IN */ 	short statsMergeType,
	    /* OUT */ 	short *statsCollectType,
	    /* IN/OUT */ 	SQLSTATS_DESC sqlstats_desc[],
	    /* IN */ 	Lng32 max_stats_desc,
	    /* OUT */	Lng32 *no_returned_stats_desc);

Lng32 SQLCLI_GetStatisticsItems(CliGlobals *cliGlobals,
            /* IN */  	short statsReqType,
	    /* IN */  	char *queryId,
	    /* IN */  	Lng32 queryIdLen,
            /* IN */	Lng32 no_of_stats_items,
	    /* IN/OUT */   SQLSTATS_ITEM sqlstats_items[]);


Lng32 SQLCLI_ProcessRetryQuery(CliGlobals *cliGlobals, 
			      SQLSTMT_ID * statement_id,
			      Lng32 sqlcode,
			      short afterPrepare, short afterExec, 
			      short afterFetch, short afterCEFC);

Lng32 SQLCLI_LocaleToUTF8(
                    CliGlobals *cliGlobals,
                    Int32 conv_charset,
                    void * Input_Buffer_Addr,
                    Int32 Input_Buffer_Length,
                    void * Output_Buffer_Addr,
                    Int32 Output_Buffer_Length,
                    void ** First_Untranslated_Char_Addr,
                    Int32 *Output_Data_Length,
                    Int32 add_null_at_end_Flag,
                    Int32 *num_translated_char);

Lng32 SQLCLI_LocaleToUTF16(
                    CliGlobals *cliGlobals,
                    Int32 conv_charset,
                    void * Input_Buffer_Addr,
                    Int32 Input_Buffer_Length,
                    void * Output_Buffer_Addr,
                    Int32 Output_Buffer_Length,
                    void ** First_Untranslated_Char_Addr,
                    Int32 *Output_Data_Length,
                    Int32 conv_flags,
                    Int32 add_null_at_end_Flag,
                    Int32 *num_translated_char);

Lng32 SQLCLI_UTF8ToLocale(
                    CliGlobals *cliGlobals,
                    Int32 conv_charset,
                    void * Input_Buffer_Addr,
                    Int32 Input_Buffer_Length,
                    void * Output_Buffer_Addr,
                    Int32 Output_Buffer_Length,
                    void ** First_Untranslated_Char_Addr,
                    Int32 *Output_Data_Length,
                    Int32 add_null_at_end_Flag,
                    Int32 allow_invalids,
                    Int32 *num_translated_char,
                    void * substitution_char_addr);

Lng32 SQLCLI_UTF16ToLocale(
                    CliGlobals *cliGlobals,
                    Int32 conv_charset,
                    void * Input_Buffer_Addr,
                    Int32 Input_Buffer_Length,
                    void * Output_Buffer_Addr,
                    Int32 Output_Buffer_Length,
                    void ** First_Untranslated_Char_Addr,
                    Int32 *Output_Data_Length,
                    Int32 conv_flags,
                    Int32 add_null_at_end_Flag,
                    Int32 allow_invalids,
                    Int32 *num_translated_char,
                    void * substitution_char_addr);

Lng32 SQLCLI_RegisterQuery(CliGlobals *cliGlobals,
                      SQLQUERY_ID *queryId,
                      Lng32 fragId,
                      Lng32 tdbId,
                      Lng32 explainTdbId,
                      short collectStatsType,
                      Lng32 instNum,
                      Lng32 tdbType,
                      char *tdbName,
                      Lng32 tdbNameLen);

Lng32 SQLCLI_DeregisterQuery(CliGlobals *cliGlobals,
                      SQLQUERY_ID *queryId,
                      Lng32 fragId);
// This method returns the pointer to the CLI ExStatistics area.
// The returned pointer is a read only pointer, its contents cannot be
// modified by the caller.
Lng32 SQLCLI_GetStatisticsArea_Internal 
(
 /* IN */    CliGlobals *cliGlobals, 
 /* IN */    short statsReqType,
 /* IN */    char *statsReqStr,
 /* IN */    Lng32 statsReqStrLen,
 /* IN */    short activeQueryNum,
 /* IN */    short statsMergeType,
 /*INOUT*/   const ExStatisticsArea* &exStatsArea
);


Lng32 SQLCLI_GetChildQueryInfo(CliGlobals *cliGlobals,
     SQLSTMT_ID * statement_id,
     char * uniqueQueryId,
     Lng32 uniqueQueryIdMaxLen,
     Lng32 * uniqueQueryIdLen,
     SQL_QUERY_COST_INFO *query_cost_info,
     SQL_QUERY_COMPILER_STATS_INFO *comp_stats_info);

Lng32 SQLCLI_LOBcliInterface
(
 /*IN*/     CliGlobals *cliGlobals,
 /*IN*/     char * inLobHandle,
 /*IN*/     Lng32  inLobHandleLen,
 /*IN*/     char * blackBox,
 /*IN*/     Int32* blackBoxLen,
 /*OUT*/    char * outLobHandle,
 /*OUT*/    Lng32 * outLobHandleLen,
 /*IN*/     LOBcliQueryType qType,
 /*IN*/     LOBcliQueryPhase qPhase,
 /*INOUT*/  Int64 * dataOffset, /* IN: for insert, 
                                   OUT: for select */
 /*IN*/     Int64 * dataLen,    /* length of data.
                                   IN: for insert, out: for select */
 /*OUT*/    Int64 * outDescPartnKey,  /* returned after insert and select */
 /*OUT*/    Int64 * outDescSyskey,    /* returned after insert and select */
 /*INOUT*/  void* *inCliInterface,
 /*IN*/     Int64 xnId,          /* xn id of the parent process, if non-zero */
 /*IN */    NABoolean lobTrace
 );
Lng32 SQLCLI_LOB_GC_Interface
(
 /*IN*/     CliGlobals *cliGlobals,
 /*IN*/     ExLobGlobals *lobGlobals, // can be passed or NULL
 /*IN*/     char * handle,
 /*IN*/     Lng32  handleLen,
 /*IN*/     char*  hdfsServer,
 /*IN*/     Lng32  hdfsPort,
 /*IN*/     char *lobLocation,
 /*IN*/    Int64 lobMaxMemChunkLen, // if passed in as 0, will use default value of 1G for the in memory buffer to do compaction.
 /*IN*/    NABoolean lobTrace
 );

Lng32 SQLCLI_LOBddlInterface
(
 /*IN*/     CliGlobals *cliGlobals,
 /*IN*/     char * schName,
 /*IN*/     Lng32  schNameLen,
 /*IN*/     Int64  objectUID,
 /*IN*/     Lng32  &numLOBs,
 /*IN*/     LOBcliQueryType qType,
 /*IN*/     short *lobNumList,
 /*IN*/     short *lobTypList,
 /*IN*/     char* *lobLocList,
 /*IN*/     char* *lobColNameList,
 /*IN*/    char *hdfsServer,
 /*IN*/    Int32 hdfsPort,
 /*IN*/     Int64 lobMaxSize,
 /*IN*/    NABoolean lobTrace
 );

Int32 SQLCLI_SWITCH_TO_COMPILER_TYPE
 (
 /*IN*/     CliGlobals* cliGlobals,
 /*IN*/     Int32 cmpCntxtType
 );  

Int32 SQLCLI_SWITCH_TO_COMPILER
 (
 /*IN*/     CliGlobals* cliGlobals,
 /*IN*/     void* cmpCntxt
 );  

Int32 SQLCLI_SWITCH_BACK_COMPILER
 (
 /*IN*/     CliGlobals* cliGlobals
 );  

Lng32 SQLCLI_SEcliInterface
(
 CliGlobals *cliGlobals,

 SECliQueryType qType,
 
 void* *cliInterface,  /* IN: if passed in and not null, use it.
                                                  OUT: if returned, save it and pass it back in */

 const char * inStrParam1 = NULL,
 const char * inStrParam2 = NULL,
 Lng32 inIntParam1 = -1,
 Lng32 inIntParam2 = -1,

 char* *outStrParam1 = NULL,
 char* *outStrParam2 = NULL,
 Lng32 *outIntParam1 = NULL

 );

  Lng32 SQLCLI_SeqGenCliInterface
  (
   CliGlobals *cliGlobals,
   
   void* *cliInterface,  /* IN: if passed in and not null, use it.
			    OUT: if returned, save it and pass it back in */
   
   void * seqGenAttrs
   );
  
  Int32 SQLCLI_GetRoutine
  (
       /* IN */     CliGlobals  *cliGlobals,
       /* IN */     const char  *serializedInvocationInfo,
       /* IN */     Int32        invocationInfoLen,
       /* IN */     const char  *serializedPlanInfo,
       /* IN */     Int32        planInfoLen,
       /* IN */     Int32        language,
       /* IN */     Int32        paramStyle,
       /* IN */     const char  *externalName,
       /* IN */     const char  *containerName,
       /* IN */     const char  *externalPath,
       /* IN */     const char  *librarySqlName,
       /* OUT */    Int32       *handle
   );

  Int32 SQLCLI_InvokeRoutine
  (
       /* IN */     CliGlobals  *cliGlobals,
       /* IN */     Int32        handle,
       /* IN */     Int32        phaseEnumAsInt,
       /* IN */     const char  *serializedInvocationInfo,
       /* IN */     Int32        invocationInfoLen,
       /* OUT */    Int32       *invocationInfoLenOut,
       /* IN */     const char  *serializedPlanInfo,
       /* IN */     Int32        planInfoLen,
       /* IN */     Int32        planNum,
       /* OUT */    Int32       *planInfoLenOut,
       /* IN */     char        *inputRow,
       /* IN */     Int32        inputRowLen,
       /* OUT */    char        *outputRow,
       /* IN */     Int32        outputRowLen
   );

  Int32 SQLCLI_GetRoutineInvocationInfo
  (
       /* IN */     CliGlobals  *cliGlobals,
       /* IN */     Int32        handle,
       /* IN/OUT */ char        *serializedInvocationInfo,
       /* IN */     Int32        invocationInfoMaxLen,
       /* OUT */    Int32       *invocationInfoLenOut,
       /* IN/OUT */ char        *serializedPlanInfo,
       /* IN */     Int32        planInfoMaxLen,
       /* IN */     Int32        planNum,
       /* OUT */    Int32       *planInfoLenOut
   );

  Int32 SQLCLI_PutRoutine
  (
       /* IN */     CliGlobals*  cliGlobals,
       /* IN */     Int32        handle
   );

} // end extern "C"

class StrTarget {
private:
  CollHeap *heap_;
  char *str_;
  Lng32 externalCharset_;      // these are really enum CharInfo::CharSet,
  Lng32 internalCharset_;      // defined in common/charinfo.h
  Lng32 cnvExternalCharset_;   // these are really enum cnv_charset,
  Lng32 cnvInternalCharset_;   // defined in common/csconvert.h
public:
  StrTarget();
  StrTarget(Descriptor *desc, Lng32 entry);
  ~StrTarget()
    {
      if (str_)
	heap_->deallocateMemory(str_);
    }
  void init(Descriptor *desc, Lng32 entry);
  void init(char  *source,
            Lng32 sourceLen,
            Lng32 sourceType,
            Lng32 externalCharset,
            Lng32 internalCharset,
            CollHeap *heap,
            ComDiagsArea *&diagsArea);
  char * getStr() { return str_; }
  Lng32 getIntCharSet() { return internalCharset_; }
  
};

#endif /* CLI_H */
