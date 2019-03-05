//******************************************************************************
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
//******************************************************************************
/* -*-C++-*-
****************************************************************************
*
* File:         ExpErrorEnums.h (previously part of /exp/experror.h)
* Description:
*
****************************************************************************
*/

#ifndef EXP_ERRORENUMS_H
#define EXP_ERRORENUMS_H

// -----------------------------------------------------------------------
// List of all errors generated in the SQL executor code
// -----------------------------------------------------------------------

enum ExeErrorCode
{
  EXE_OK				= 0,     // no error

  EXE_FIRST_ERROR			= 8000,
  EXE_INTERNAL_ERROR			= 8001,
  EXE_NOWAIT_OP_INCOMPLETE		= 8002,
  EXE_OUTPUT_DESCRIPTOR_LOCKED   	= 8003,
  EXE_CURSOR_ALREADY_OPEN		= 8004,
  EXE_CURSOR_NOT_OPEN			= 8005,
  EXE_STREAM_TIMEOUT                    = 8006,
  EXE_CANCELED		                = 8007,
  EXE_INVALID_CAT_NAME			= 8008,
  EXE_INVALID_SCH_NAME			= 8009,
  EXE_INFO_DEFAULT_CAT_SCH		= 8010,
  EXE_BLOCK_CARDINALITY_VIOLATION       = 8011,
  EXE_INFO_CQD_NAME_VALUE_PAIRS		= 8012,
  EXE_CURSOR_NOT_FETCHED                = 8013,
  EXE_CS_EOD                            = 8014,
  EXE_CS_EOD_ROLLBACK_ERROR		= 8015,
  EXE_VERSION_ERROR			= 8016,	
  EXE_NO_EXPLAIN_INFO                   = 8017,
  EXE_PARTN_SKIPPED                     = 8018,
  EXE_EXPLAIN_BAD_DATA			= 8019,

  EXE_INITIALIZE_MAINTAIN               = 8020,

  EXE_QUERY_LIMITS_CPU                  = 8023,
  EXE_QUERY_LIMITS_CPU_DEBUG            = 8024,
  EXE_QUERY_LIMITS_CPU_DP2              = 8025,

  EXE_CANCEL_QID_NOT_FOUND              = 8026,
  EXE_CANCEL_TIMEOUT                    = 8027,
  EXE_CANCEL_PROCESS_NOT_FOUND          = 8028,
  EXE_CANCEL_NOT_AUTHORIZED             = 8029,
  EXE_CANCEL_NOT_POSSIBLE               = 8031,

  EXE_NO_QID_EXPLAIN_INFO               = 8032,
  EXE_EXPLAIN_PLAN_TOO_LARGE            = 8033,
  EXE_DEFAULT_VALUE_INCONSISTENT_ERROR  = 8034,
  EXE_HIVE_TRUNCATE_ERROR               = 8035,
  EXE_ERROR_WHILE_LOGGING               = 8036,

  // ---------------------------------------------------------------------
  // Data integrity errors
  // ---------------------------------------------------------------------
  EXE_INVALID_DEFINE_OR_ENVVAR          = 8100,
  EXE_TABLE_CHECK_CONSTRAINT            = 8101, // SQLSTATE 23000
  EXE_DUPLICATE_RECORD                  = 8102, // SQLSTATE 23000
  EXE_RI_CONSTRAINT_VIOLATION           = 8103, // SQLSTATE 23000
  EXE_CHECK_OPTION_VIOLATION_CASCADED   = 8104, // SQLSTATE 44000
  EXE_CHECK_OPTION_VIOLATION            = 8105, // SQLSTATE 44000
  EXE_CURSOR_UPDATE_CONFLICT            = 8106,
  EXE_HALLOWEEN_INSERT_AUTOCOMMIT       = 8107,
  EXE_INVALID_SESSION_DEFAULT           = 8109,
  EXE_DUPLICATE_ENTIRE_RECORD           = 8110,
  EXE_MAX_ERROR_ROWS_EXCEEDED           = 8113,
  EXE_ERROR_ROWS_FOUND                  = 8114,
  EXE_LAST_INTEGRITY_ERROR              = 8139,

  // ---------------------------------------------------------------------
  // Some internal testing "errors"
  // ---------------------------------------------------------------------
  EXE_CANCEL_INJECTED                   = 8140,
  EXE_ERROR_INJECTED                    = 8141,

  // ---------------------------------------------------------------------
  // Miscellaneous.
  // ---------------------------------------------------------------------
  EXE_CLEANUP_ESP                       = 8143,
  EXE_CORRUPT_PARTITION                 = 8144,

  // ---------------------------------------------------------------------
  // generic error for the specified statement/feature/option.
  // Error detail included in string param.
  // ---------------------------------------------------------------------
  EXE_STMT_NOT_SUPPORTED                = 8145,

  //----------------------------------------------------------------------
  // Late-name resolution and late-binding/similarity check errors.
  //----------------------------------------------------------------------
  EXE_NAME_MAPPING_ERROR                = 8300,
  EXE_NAME_MAPPING_FS_ERROR             = 8301,
  EXE_NAME_MAPPING_NO_PART_AVAILABLE    = 8302,
  EXE_NAME_MAPPING_BAD_ANCHOR           = 8303,

  //----------------------------------------------------------------------
  // Available for future use.
  //----------------------------------------------------------------------
  CLI_ASSIGN_INCOMPATIBLE_CHARSET       = 8350,

  //----------------------------------------------------------------------
  // Expressions errors
  //----------------------------------------------------------------------
  EXE_INVALID_DEFINE_CLASS_ERROR        = 8400,
  EXE_CARDINALITY_VIOLATION		= 8401,
  EXE_STRING_OVERFLOW			= 8402,
  EXE_SUBSTRING_ERROR			= 8403,
  EXE_TRIM_ERROR			= 8404,
  EXE_CONVERTTIMESTAMP_ERROR		= 8405,
  EXE_JULIANTIMESTAMP_ERROR		= 8407,
  EXE_CONVERSION_ERROR                  = 8408,
  EXE_INVALID_ESCAPE_CHARACTER		= 8409,
  EXE_INVALID_ESCAPE_SEQUENCE		= 8410,
  EXE_NUMERIC_OVERFLOW			= 8411,
  EXE_MISSING_NULL_TERMINATOR		= 8412,
  EXE_CONVERT_STRING_ERROR		= 8413,
  EXE_CONVERT_NOT_SUPPORTED		= 8414,
  EXE_CONVERT_DATETIME_ERROR		= 8415,
  EXE_DATETIME_FIELD_OVERFLOW           = 8416,
  EXE_USER_FUNCTION_ERROR		= 8417,
  EXE_USER_FUNCTION_NOT_SUPP		= 8418,
  EXE_DIVISION_BY_ZERO			= 8419,
  EXE_MISSING_INDICATOR_VARIABLE        = 8420,
  EXE_ASSIGNING_NULL_TO_NOT_NULL        = 8421,
  EXE_CONVERT_INTERVAL_ERROR            = 8422,
  EXE_FIELD_NUM_OVERFLOW                = 8423,
  EXE_MATH_FUNC_NOT_SUPPORTED           = 8424,
  EXE_DEFAULT_VALUE_ERROR               = 8425,
  EXE_INVALID_BOOLEAN_VALUE             = 8426,
  EXE_SORT_ERROR                        = 8427,
  EXE_BAD_ARG_TO_MATH_FUNC              = 8428,
  EXE_MAPPED_FUNCTION_ERROR             = 8429,
  EXE_GETBIT_ERROR			= 8430,
  EXE_IS_BITWISE_AND_ERROR		= 8431,
  EXE_UNSIGNED_OVERFLOW                 = 8432,
  EXE_INVALID_CHARACTER                 = 8433,
  EXE_HIVE_DATA_MOD_CHECK_ERROR         = 8436,
  EXE_HISTORY_BUFFER_TOO_SMALL		= 8440,
  EXE_OLAP_OVERFLOW_NOT_SUPPORTED       = 8441,
  EXE_ERROR_FROM_LOB_INTERFACE          = 8442,
  EXE_INVALID_LOB_HANDLE                = 8443,
  EXE_ERROR_HDFS_SCAN                   = 8447,
  EXE_INVALID_INTERVAL_RESULT           = 8453,
  EXE_HIVE_ROW_TOO_LONG                 = 8457,
  EXE_LAST_EXPRESSIONS_ERROR		= 8499,

  // ---------------------------------------------------------------------
  // File System and DP2 errors.
  // ---------------------------------------------------------------------
  EXE_ERROR_FROM_DP2			= 8550,
  EXE_ERROR_FROM_FS2			= 8551,
  EXE_FS2_FETCH_VERSION_ERROR		= 8552,
  EXE_ERROR_STREAM_OVERFLOW             = 8553,
  EXE_EID_INTERNAL_ERROR                = 8555,
  EXE_HBASE_ACCESS_ERROR                = 8556,
  EXE_LOB_CONCURRENT_ACCESS_ERROR       = 8558,
  EXE_LAST_ERROR_FROM_FS_DP2		= 8569,

  // ---------------------------------------------------------------------
  // Build-time and other catastophic errors
  // ---------------------------------------------------------------------
  EXE_NO_MEM_TO_BUILD			= 8570,
  EXE_NO_MEM_TO_EXEC 			= 8571,
  EXE_CANNOT_CONTINUE                   = 8572,
  // unused                             = 8573,
  
  // ------------------------------------------------------------
  // Error 8574, lost open. Could result in reopening the table.
  // Error 8575, could result in recompilation.
  // Warning 8576, statement was recompiled.
  // ------------------------------------------------------------
  EXE_LOST_OPEN                         = 8574,
  EXE_TIMESTAMP_MISMATCH                = 8575,
  EXE_RECOMPILE                         = 8576,
  EXE_TABLE_NOT_FOUND                   = 8577,
  EXE_SIM_CHECK_PASSED                  = 8578,
  EXE_SIM_CHECK_FAILED                  = 8579,
  EXE_PARTITION_UNAVAILABLE             = 8580,
  EXE_NO_MEM_FOR_IN_MEM_JOIN		= 8581,
  EXE_USER_PREPARE_NEEDED               = 8582,
  EXE_RELEASE_WORK_TIMEOUT              = 8584,
  EXE_SCHEMA_SECURITY_CHANGED           = 8585,
  EXE_ASSIGN_ESPS_ERROR                 = 8586,
  EXE_MERGE_STMT_ERROR                  = 8595,
  EXE_ESP_CHANGE_PRIORITY_FAILED        = 8596,
  EXE_RECOMPILE_AUTO_QUERY_RETRY        = 8597,
  EXE_VIEW_NOT_FOUND                    = 8598,
  EXE_MV_UNAVILABLE                     = 12073,

  //-------------------------------------------------------------
  // Errors codes for concurrency control.
  //-------------------------------------------------------------
  EXE_FIRST_CONCURRENCY_CONTROL_ERROR	= 8600,
  EXE_LOCK_UNLOCK_ERROR			= 8601,
  EXE_FILESYSTEM_ERROR			= 8602,
  EXE_BEGIN_TRANSACTION_ERROR		= 8603,
  EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS	= 8604,
  EXE_COMMIT_TRANSACTION_ERROR		= 8605,
  EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS	= 8606,
  EXE_ROLLBACK_TRANSACTION_ERROR	= 8607,
  EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS	= 8608,
  EXE_ROLLBACK_TRANSACTION_WAITED_ERROR	= 8609,
  EXE_ROLLBACK_WAITED_ERROR_TRANS_SUBSYS= 8610,
  EXE_SET_TRANS_ERROR_FROM_TRANS_SUBSYS	= 8612,
  EXE_CANT_COMMIT_OR_ROLLBACK           = 8613,
  EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS = 8614,
  EXE_CANT_BEGIN_USER_TRANS_WITH_LRU    = 8615,
  EXE_COMMIT_CONFLICT_FROM_TRANS_SUBSYS	= 8616,
  EXE_LAST_CONCURRENCY_CONTROL_ERROR	= 8629,

  //-------------------------------------------------------------
  // Error codes for bulk replicate
  //-------------------------------------------------------------
  EXE_REPL_TO_UNSUPPORTED_TGT_SYS       = 8645,
  EXE_BDR_ALREADY_INITIALIZED           = 8646,
  EXE_BDR_SERVICE_PROCESS_COMM_ERROR    = 8647,
  EXE_REPL_TARGET_REPL_PROCESS_COMM_ERROR = 8648,
  EXE_REPL_SRC_TGT_PARTN_MISMATCH       = 8650,
  EXE_REPL_SRC_TGT_DDL_MISMATCH         = 8651,
  EXE_REPL_SRC_TGT_VERSION_MISMATCH     = 8652,
  EXE_BDR_REPL_PROCESS_COMM_ERROR       = 8653,
  EXE_REPL_QUERY_ID_NOT_FOUND           = 8654,
  EXE_REPL_COULD_NOT_ABORT_QUERY        = 8655,
  EXE_REPL_QUERY_WAS_ABORTED            = 8656,
  EXE_REPL_COULD_NOT_RECOVER            = 8657,
  EXE_REPL_INVALID_IPADDR_OR_PORTNUM    = 8658,

  //-------------------------------------------------------------
  // Errors codes for suspend/resume. 
  //-------------------------------------------------------------
  EXE_SUSPEND_AUDIT                     = 8670,
  EXE_SUSPEND_LOCKS                     = 8671,
  EXE_SUSPEND_QID_NOT_ACTIVE            = 8672,
  EXE_SUSPEND_GUARDIAN_ERROR_1          = 8673,
  EXE_SUSPEND_GUARDIAN_ERROR_2          = 8674,
  EXE_SUSPEND_SQL_ERROR                 = 8675,
  EXE_SUSPEND_ALREADY_SUSPENDED         = 8676,
  EXE_SUSPEND_NOT_SUSPENDED             = 8677,

  //-------------------------------------------------------------
  // Errors codes translate function.
  //-------------------------------------------------------------
  EXE_INVALID_CHAR_IN_TRANSLATE_FUNC    = 8690,

  //-------------------------------------------------------------
  // Errors codes split_part function.
  //-------------------------------------------------------------
  EXE_INVALID_FIELD_POSITION            = 8691,
  
  // ---------------------------------------------------------------------
  // Parallel execution
  // ---------------------------------------------------------------------
  EXE_PARALLEL_EXECUTION_ERROR		= 8700,
  EXE_PARALLEL_EXTRACT_OPEN_ERROR       = 8701,
  EXE_PARALLEL_EXTRACT_CONNECT_ERROR    = 8702,

  //----------------------------------------------------------------------
  // Errors generated in the CLI code
  //----------------------------------------------------------------------
  CLI_FIRST_ERROR			= 8730,
  CLI_PROBLEM_READING_USERS             = 8731,
  CLI_USER_NOT_REGISTERED               = 8732,
  CLI_USER_NOT_VALID                    = 8733,
  CLI_INVALID_QUERY_PRIVS               = 8734,
 
  CLI_INVALID_ROWS_AFFECTED             = 8737,
  CLI_DDL_REDEFINED                     = 8738,

  CLI_UNUSED                            = 8740,
  CLI_CANNOT_EXECUTE_IN_MEM_DEFN        = 8741,
  CLI_GET_METADATA_INFO_ERROR           = 8742,
  CLI_ROUTINE_INVALID                   = 8743,
  CLI_ROUTINE_DEALLOC_ERROR             = 8744,
  CLI_ROUTINE_INVOCATION_ERROR          = 8745,
  CLI_ROUTINE_INVOCATION_INFO_ERROR     = 8746,

  CLI_DUPLICATE_DESC			= 8801,
  CLI_DUPLICATE_STMT			= 8802,
  CLI_DESC_NOT_EXISTS			= 8803,
  CLI_STMT_NOT_EXISTS			= 8804,
  CLI_NOT_DYNAMIC_DESC			= 8805,
  CLI_NOT_DYNAMIC_STMT			= 8806,
  CLI_DATA_OUTOFRANGE			= 8807,
  CLI_MODULEFILE_CORRUPTED		= 8808,
  CLI_MODULEFILE_OPEN_ERROR		= 8809,

  CLI_NO_ERROR_IN_DIAGS                 = 8810,

  CLI_STMT_NOT_OPEN			= 8811,
  CLI_STMT_NOT_CLOSE			= 8812,
  CLI_STMT_CLOSE			= 8813,
  CLI_TRANS_MODE_MISMATCH		= 8814,
  CLI_TCB_EXECUTE_ERROR			= 8816,
  CLI_TCB_FETCH_ERROR			= 8817,
  CLI_TDB_DESCRIBE_ERROR		= 8818,
  CLI_BEGIN_TRANSACTION_ERROR		= 8819,
  CLI_COMMIT_TRANSACTION_ERROR		= 8820,

  CLI_ROLLBACK_TRANSACTION_ERROR	= 8821,
  CLI_STMT_NOT_PREPARED			= 8822,
  CLI_IO_REQUESTS_PENDING		= 8823,
  CLI_NO_MODULE_NAME			= 8824,
  CLI_MODULE_ALREADY_ADDED		= 8825,
  CLI_ADD_MODULE_ERROR			= 8826,
  CLI_SEND_REQUEST_ERROR		= 8827,
  CLI_OUT_OF_MEMORY			= 8828,
  CLI_INVALID_DESC_ENTRY		= 8829,
  CLI_NO_CURRENT_CONTEXT		= 8830,

  CLI_MODULE_NOT_ADDED			= 8831,
  CLI_TRANSACTION_NOT_STARTED		= 8832,
  CLI_INVALID_SQLTRANS_COMMAND		= 8833,
  CLI_NO_INSTALL_DIR			= 8834,
  CLI_INVALID_DESC_INFO_REQUEST         = 8835,
  CLI_INVALID_UPDATE_COLUMN             = 8836,
  CLI_INVALID_USERID   			= 8837,
  CLI_RECEIVE_ERROR   			= 8838,
  CLI_VALIDATE_TRANSACTION_ERROR        = 8839,
  CLI_SELECT_INTO_ERROR                 = EXE_CARDINALITY_VIOLATION,
  CLI_INVALID_OBJECTNAME		= 8840,

  CLI_USER_ENDED_EXE_XN                 = 8841,
  CLI_NON_UPDATABLE_SELECT_CURSOR       = 8842,
  CLI_ITEM_NUM_OUT_OF_RANGE             = 8843,
  CLI_USER_ENDED_XN_CLEANUP             = 8844,
  CLI_INTERR_NULL_TCB                   = 8845,
  CLI_EMPTY_SQL_STMT                    = 8846,
  // unused      			= 8847,
  CLI_CANCEL_REJECTED                   = 8848,
  CLI_NON_CURSOR_UPDEL_TABLE            = 8850,
  CLI_USER_MEMORY_IN_EXECUTOR_SEGMENT   = 8851,
  CLI_CURSOR_CANNOT_BE_HOLDABLE         = 8852,
  CLI_INVALID_ATTR_NAME                 = 8853,
  CLI_INVALID_ATTR_VALUE                = 8854,
  CLI_CURSOR_ATTR_CANNOT_BE_SET         = 8855,
  CLI_ARRAY_MAXSIZE_INVALID_ENTRY       = 8856,
  CLI_LOCAL_AUTHENTICATION              = 8857,
  CLI_INVALID_SQL_ID			= 8858,
  CLI_UPDATE_PENDING                    = 8859,
  // ---------------------------------------------------------------------
  // Module versioning errors
  // ---------------------------------------------------------------------
  CLI_MODULE_HDR_VERSION_ERROR          = 8860,
  CLI_MOD_DLT_HDR_VERSION_ERROR         = 8861,
  CLI_MOD_DLT_ENT_VERSION_ERROR         = 8862,
  CLI_MOD_DESC_HDR_VERSION_ERROR        = 8863,
  CLI_MOD_DESC_ENT_VERSION_ERROR        = 8864,
  CLI_MOD_PLT_HDR_VERSION_ERROR         = 8865,
  CLI_MOD_PLT_ENT_VERSION_ERROR         = 8866,
  CLI_READ_ERROR                        = 8867,
  CLI_CREATE_CONTEXT_EXE_TRANSACTION    = 8868,
  CLI_INVALID_QFO_NUMBER                = 8869,
  CLI_STATEMENT_WITH_NO_QFO             = 8870,
  CLI_NOWAIT_TAG_NOT_SPECIFIED          = 8871,
  CLI_OPERATION_WITH_PENDING_OPS        = 8872,
  CLI_STATEMENT_ASSOCIATED_WITH_QFO     = 8873,

  CLI_SAVEPOINT_ROLLBACK_FAILED         = 8874,
  CLI_SAVEPOINT_ROLLBACK_DONE           = 8875,
  CLI_PARTIAL_UPDATED_DATA              = 8876,
  CLI_AUTO_BEGIN_TRANSACTION_ERROR      = 8877,
  CLI_DEFAULT_CONTEXT_NOT_ALLOWED       = 8878,
  CLI_BUFFER_TOO_SMALL                  = 8879,
  CLI_REMOVE_CURRENT_CONTEXT            = 8880,
  CLI_CONTEXT_NOT_FOUND                 = 8881,
  CLI_NO_SQL_ACCESS_MODE_VIOLATION      = 8882,
  CLI_NOT_CHECK_VIOLATION               = 8883,
  CLI_NO_TRANS_STMT_VIOLATION           = 8884,

  CLI_SEND_ARKCMP_CONTROL		= 8885,

  CLI_INTERR_ON_CONTEXT_SWITCH          = 8886,
   
  CLI_GENCODE_BUFFER_TOO_SMALL          = 8887,

  CLI_IUD_IN_PROGRESS			= 8888,

  CLI_RS_PROXY_BUFFER_SMALL_OR_NULL     = 8889,

  CLI_ARKCMP_INIT_FAILED		= 8890,
  CLI_NOT_ASCII_CHAR_TYPE		= 8891,
  CLI_STMT_EXCEEDS_DESC_COUNT           = 8892,
  CLI_STMT_DESC_COUNT_MISMATCH          = 8893,
  CLI_RESERVED_ARGUMENT                 = 8894,
  CLI_INVALID_CHARSET_FOR_DESCRIPTOR    = 8895,
  CLI_CHARSET_MISMATCH                  = 8896,

  CLI_INTERNAL_ERROR			= 8898,
  CLI_LAST_ERROR			= 8899,

  // ---------------------------------------------------------------------
  // Diagnostic message errors
  // ---------------------------------------------------------------------
  CLI_MSG_CHAR_SET_NOT_SUPPORTED        = 8900,

  // ---------------------------------------------------------------------
  // Execution errors for user-defined functions and procedures
  // ---------------------------------------------------------------------
  EXE_UDR_SERVER_WENT_AWAY              = 8901,
  EXE_UDR_INVALID_HANDLE                = 8902,
  EXE_UDR_ATTEMPT_TO_KILL               = 8903,
  EXE_UDR_REPLY_ERROR                   = 8904,
  EXE_UDR_ACCESS_VIOLATION              = 8905,
  EXE_UDR_INVALID_OR_CORRUPT_REPLY      = 8906,
  EXE_UDR_RESULTSETS_NOT_SUPPORTED      = 8907,
  EXE_UDR_RS_ALLOC_RS_NOT_SUPPORTED     = 8908,
  EXE_UDR_RS_ALLOC_STMT_NOT_CALL        = 8909,
  //                                      8910 is used by some other feature
  //                                      8911 is used by some other feature
  EXE_UDR_RS_ALLOC_INTERNAL_ERROR       = 8912,
  EXE_UDR_RS_PREPARE_NOT_ALLOWED        = 8913,
  EXE_UDR_RS_REOPEN_NOT_ALLOWED         = 8914,
  EXE_UDR_RS_NOT_AVAILABLE              = 8915,
  EXE_UDR_RS_ALLOC_INVALID_INDEX        = 8916,
  EXE_UDR_RS_ALLOC_ALREADY_EXISTS       = 8917,
  EXE_RTS_NOT_STARTED                   = 8918,
  EXE_RTS_INVALID_QID                   = 8919,
  EXE_RTS_INVALID_CPU_PID               = 8920,
  EXE_RTS_TIMED_OUT                     = 8921,
  EXE_RTS_REQ_PARTIALY_SATISFIED        = 8922,
  EXE_RTS_QID_NOT_FOUND                 = 8923,
  CLI_MERGED_STATS_NOT_AVAILABLE        = 8924,
  CLI_QID_NOT_MATCHING                  = 8925,
  EXE_STAT_NOT_FOUND                    = 8926,
  EXE_ERROR_IN_STAT_ITEM                = 8927,
  CLI_INSUFFICIENT_STATS_DESC           = 8928,
  CLI_INSUFFICIENT_SIKEY_BUFF           = 8929,

  CLI_CONSUMER_QUERY_BUF_TOO_SMALL      = 8930, // For parallel extract

  EXE_SG_MAXVALUE_EXCEEDED              = 8934, // For sequence generator
  EXE_SG_UPDATE_FAILURE                 = 8935,

  EXE_REORG_STATISTICS_FETCH_ERROR      = 8936,
  EXE_RTS_INVALID_QID_INTERNAL          = 8937, 

  EXE_UDR_INVALID_DATA                  = 8940,

  CLI_SESSION_ATTR_BUFFER_TOO_SMALL     = 8941,
  CLI_USERNAME_BUFFER_TOO_SMALL         = 8942,
  
  EXE_ROWLENGTH_EXCEEDS_BUFFER          = 8943,

  //-------------------------------------------------------------
  // Error codes for bulk replicate - Part 2
  //-------------------------------------------------------------
  EXE_INTERNALLY_GENERATED_COMMAND      = 8950,

 
  //fast transport

  EXE_EXTRACT_ERROR_CREATING_FILE       = 8960,
  EXE_EXTRACT_ERROR_WRITING_TO_FILE     = 8961,
  EXE_EXTRACT_CANNOT_ALLOCATE_BUFFER    = 8962,
  EXE_EXTRACT_BACKUP_SWITCHED           = 8963,
  EXE_LOAD_NON_EMPTY_TABLE              = 8964,
  EXE_UNLOAD_FILE_EXISTS                = 8965,
  // ---------------------------------------------------------------------
  // SeaMonster
  // ---------------------------------------------------------------------
  EXE_SM_FUNCTION_ERROR                 = 8951,
  EXE_SM_CONTROL_CONN_ERROR             = 8952,
  EXE_SM_FIXUP_REPLY_TIMEOUT            = 8953,

  // ---------------------------------------------------------------------
  // Built-in Function ( encrpt/decrypt )
  // ---------------------------------------------------------------------
  EXE_AES_INVALID_IV                    = 8954,
  EXE_ERR_PARAMCOUNT_FOR_FUNC           = 8955,
  EXE_OPTION_IGNORED                    = 8956,
  EXE_OPENSSL_ERROR                     = 8957,

  // Execution errors related to JSon parser
  // ---------------------------------------------------------------------
  EXE_JSON_INVALID_TOKEN                  = 8971,
  EXE_JSON_INVALID_VALUE                  = 8972,
  EXE_JSON_INVALID_STRING                 = 8973,
  EXE_JSON_INVALID_ARRAY_START            = 8974,
  EXE_JSON_INVALID_ARRAY_NEXT             = 8975,
  EXE_JSON_INVALID_OBJECT_START           = 8976,
  EXE_JSON_INVALID_OBJECT_LABEL           = 8977,
  EXE_JSON_INVALID_OBJECT_NEXT            = 8978,
  EXE_JSON_INVALID_OBJECT_COMMA           = 8979,
  EXE_JSON_INVALID_END                    = 8980,
  EXE_JSON_END_PREMATURELY                = 8981,
  EXE_JSON_UNEXPECTED_ERROR               = 8982,

  // ---------------------------------------------------------------------
  // Scratch file I/O errors (10100 - 10199)
  // ---------------------------------------------------------------------
  EXE_SCR_IO_CREATE                     = 10101,
  EXE_SCR_IO_OPEN                       = 10102,
  EXE_SCR_IO_CLOSE                      = 10103,
  EXE_SCR_IO_WRITE                      = 10104,
  EXE_SCR_IO_READ                       = 10105,

  EXE_SCR_IO_SETMODE                    = 10110,
  EXE_SCR_IO_AWAITIOX                   = 10111,
  EXE_SCR_IO_POSITION                   = 10112,
  EXE_SCR_IO_GETINFO                    = 10113,
  EXE_SCR_IO_GETINFOLIST                = 10114,
  EXE_SCR_IO_GETINFOLISTBYNAME          = 10115,
  EXE_SCR_IO_GET_PHANDLE                = 10116,
  EXE_SCR_IO_DECOMPOSE_PHANDLE          = 10117,
  EXE_SCR_IO_FILENAME_FINDSTART         = 10118,
  EXE_SCR_IO_FILENAME_FINDNEXT          = 10119,

  EXE_SCR_IO_CREATEDIR                  = 10130,
  EXE_SCR_IO_CREATEFILE                 = 10131,
  EXE_SCR_IO_GETTMPFNAME                = 10132,
  EXE_SCR_IO_CLOSEHANDLE                = 10133,
  EXE_SCR_IO_WRITEFILE                  = 10134,
  EXE_SCR_IO_SETFILEPOINTER             = 10135,
  EXE_SCR_IO_CREATEEVENT                = 10136,
  EXE_SCR_IO_WAITMULTOBJ                = 10137,
  EXE_SCR_IO_WAITSINGLEOBJ              = 10138,
  EXE_SCR_IO_GETOVERLAPPEDRESULT        = 10139,
  EXE_SCR_IO_RESETEVENT                 = 10140,
  EXE_SCR_IO_GETDISKFREESPACE           = 10141,

  EXE_SCR_IO_NO_DISKS                   = 10150,
  EXE_SCR_IO_THRESHOLD                  = 10151,
  EXE_SCR_IO_INVALID_BLOCKNUM           = 10152,
  EXE_SCR_IO_UNMAPPED_BLOCKNUM          = 10153,

  // ---------------------------------------------------------------------
  // Execution errors related to Materialized Views
  // ---------------------------------------------------------------------
  CLI_MV_EXECUTE_UNINITIALIZED          = 12301,
 
  // ---------------------------------------------------------------------
  // Execution errors related to Rowsets
  // ---------------------------------------------------------------------
  EXE_ROWSET_INDEX_OUTOF_RANGE = 30008,
  EXE_ROWSET_OVERFLOW          = 30009,
  EXE_ROWSET_CORRUPTED         = 30010,

  EXE_ROWSET_NEGATIVE_SIZE     = 30013,
  EXE_ROWSET_WRONG_SIZETYPE    = 30014,
  EXE_ROWSET_VARDATA_OR_INDDATA_ERROR     = 30018,
  EXE_ROWSET_SCALAR_ARRAY_MISMATCH = 30019,
  EXE_NONFATAL_ERROR_SEEN = 30022,
  EXE_ROWSET_ROW_COUNT_ARRAY_WRONG_SIZE = 30023,
  EXE_ROWSET_ROW_COUNT_ARRAY_NOT_AVAILABLE = 30024,
  EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER = 30029,
  EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED = 30031,
  CLI_STMT_NEEDS_PREPARE = 30032,
  EXE_NONFATAL_ERROR_ON_ALL_ROWS = 30035,

  CLI_ROWWISE_ROWSETS_NOT_SUPPORTED = 30036,

  CLI_RWRS_DECOMPRESS_ERROR        = 30045,
  CLI_RWRS_DECOMPRESS_LENGTH_ERROR = 30046,
  CLI_NAR_ERROR_DETAILS            = 30047,

  // ---------------------------------------------------------------------
  // the trailer (use temporarily for new errors that aren't added yet)
  // ---------------------------------------------------------------------
  EXE_NEW_ERROR				= 8999

};
                                                               
#endif /* EXP_ERRORENUMS_H */


