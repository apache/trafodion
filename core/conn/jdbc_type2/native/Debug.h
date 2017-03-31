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
//
//  Debug Routines Header
//
//  DEBUGGING SHOULD ONLY BE USED FOR DEVELOPMENT BUILDS
//
//  Debug routines are macro'ed out for non-debug builds so adding more debugging will
//    not affect the shipped product.
//
//  When debugging is selected (_DEBUG defined), debug calls are macro'ed 
//    into the code for:
//
//    1. Function Entry and Exit
//         When DEBUG_LEVEL_ENTRY is selected, FUNCTION_ENTRY() and FUNCTION_RETURN...()
//           macros log all function entries, parameters and exit return codes.
//    2. Tracing
//         DEBUG_OUT() uses the debug level to control developer tracing output.
//    3. Memory
//         Memory allocation and deallocation monitor dynamic memory allocation through the
//           use of the MEMORY_ALLOC() and MEMORY_DELETE() macros.
//

// Environment Variables:
// JDBC_DEBUG_LEVEL        - Comma delimited list of debug levels.
//                           See levelInfo[] list in Debug.cpp for valid strings.
// JDBC_DEBUG_BUFFER_LINES - If set to integral value, debug log lines are buffered to memory.
//                           The last JDBC_DEBUG_BUFFER_LINES of output can be dumped to console
//                              with the DEBUG_PRINT_BUFFER macro.

// Debug Flags:
// DEBUG_FLAG_THREADS   - Thread ID is printed in log line
// DEBUG_FLAG_DIRTY_MEM - Fills memory allocations with non-zero values
// DEBUG_FLAG_TIMER     - Replaces log timestamp with system timer (micro-second granularity)
// DEBUG_FLAG_NOTIME    - Does not print date and time in debug output
// DEBUG_FLAG_TESTWARE  - Development flag for running test code

// Debug Levels:
// DEBUG_LEVEL_ALL      - All debug levels active.  When used in DEBUG_OUT(), logs no matter what levels are set
// DEBUG_LEVEL_ENTRY    - Function entry and exits are logged.
// DEBUG_LEVEL_MEM      - Memory allocation and deallocations are logged
// DEBUG_LEVEL_CLI      - Trafodion CLI calls are logged
// DEBUG_LEVEL_DATA     - Data handling is logged
// DEBUG_LEVEL_TXN      - Transaction handling is logged
// DEBUG_LEVEL_ROWSET   - SQL Rowset handling is logged
// DEBUG_LEVEL_ERROR    - Error handling is logged
// DEBUG_LEVEL_METADATA - Metadata table/input lists are logged
// DEBUG_LEVEL_UNICODE  - Unicode related data is logged
// DEBUG_LEVEL_JAVA     - Java related structures and calls are logged
// DEBUG_LEVEL_POOLING  - Connection and Statement Pooling is logged
// DEBUG_LEVEL_STMT     - Srvrstmt management calls are logged

// Macros:
// FUNCTION_ENTRY       - Used at beginning of all functions that are using any debug macros.  Defines function name
//                          and parameters.
// FUNCTION_RETURN_NUMIERIC - Used for all "return" calls for a function returning a numeric value.  Returns a signed integral.
// FUNCTION_RETURN_INT64 - Used for all "return" calls for a function returning a 64-bit integer value.  Returns a signed integral.
// FUNCTION_RETURN_FLOAT - Used for all "return" calls for a function returning a float value.  Returns a float.
// FUNCTION_RETURN_DOUBLE - Used for all "return" calls for a function returning a double value.  Returns a double.
// FUNCTION_RETURN_PTR  - Like FUNCTION_RETURN except returns a pointer. Do not pass function directly to macro.
//						  Pass in a return code variable.
// FUNCTION_RETURN_VOID - Like FUNCTION_RETURN except for functions that return "void".
// DEBUG_OUT            - Logs information when the specified debug level is set.
//                        'args' is formatted for printf() and includes the ().
//						  DEBUG_OUT(DEBUG_LEVEL_ALL,("%s","Hello World")); 
// DEBUG_SET            - Activates the debug level(s) specified.
// DEBUG_CLEAR          - Deactivates the debug level(s) specified.
// DEBUG_ASSERT         - Asserts that the expression is true.  If not, forces program into Inspect.
// DEBUG_PRINT_BUFFER   - Prints the debug log buffered lines if JDBC_DEBUG_BUFFER_LINES set to non-zero.
//                        The buffer is cleared after the print so new debug log lines can be added.
// MEMORY_ALLOC         - Memory allocation.
//                        Equivalent to "{ alloc_var = new allocation; if (alloc_var==NULL) exit(0) }"
//                        For debug and release, will check for out of memory error and exit.
// MEMORY_ALLOC_OBJ     - Memory allocation of an object.
// MEMORY_ALLOC_ARRAY   - Memory allocation of an array.
// MEMORY_ALLOC_PERM, MEMORY_ALLOC_PERM_OBJ, MEMORY_ALLOC_PERM_ARRAY
//                      - Permanate memory allocations.  Not reported as leaks.
// MEMORY_DELETE        - Memory deallocation.
//                        Equivalent to "if (alloc_var) { delete alloc_var; alloc_var = NULL; }"
//                        For debug and release, safe to call when NULL and will NULL the pointer after delete.
// MEMORY_DELETE_OBJ    - Memory deallocation of an object.
// MEMORY_PRINT_ALLOC   - Prints the current memory allocation table.  Shows all non-deleted memory.
//                        Always printed on program exit when MEM debug level is set.
// MEMORY_DUMP          - Prints a hex dump of the memory block

#ifndef DEBUG_H
#define DEBUG_H

#include <stdlib.h>
#include "sqlcli.h"
#include <dlfcn.h>
#include <stdio.h>
#include <map>
#include <CSrvrStmt.h>
// Changes for downward compatability with Trafodion (earlier than 2.3.2)

#define CLI_VERSION_R2 90L

#define wAllocStmtForRS(callStmtId, resultSetIndex, resultSetStmtId) { \
		dlHandle dlopenhandle; 	\
		dlopenhandle = dlopen(NULL,RTLD_NOW); \
		if (dlopenhandle != NULL)	{ \
			typedef long (* funcptr)(SQLSTMT_ID *,long, SQLSTMT_ID *); \
			funcptr  func;	\
			func = (funcptr) dlsym(dlopenhandle,"SQL_EXEC_AllocStmtForRS");	\
			if (func != NULL){ \
				retcode = func (callStmtId, resultSetIndex, resultSetStmtId); } \
			else { printf("\n WARNING !!! The request to allocate a stored procedure \
					result set failed because stored procedure result sets are not \
					supported.\n"); } \
			} \
        else { printf("\n Trafodion installation not found on the system. \n"); exit; } \
		}

#if defined(_DEBUG) || defined(_BENCHMARK)
#include "org_apache_trafodion_jdbc_t2_JdbcDebug.h"
#endif
extern std::map<long,SRVR_STMT_HDL*> tempStmtIdMap;
#ifdef _DEBUG

// External prototypes
//const char *getCharsetEncoding(long charset); // SQLMXCommonFunctions.h
#if defined(_LP64)
const char *getCharsetEncoding(int charset); // SQLMXCommonFunctions.h 64 change
#else
const char *getCharsetEncoding(long charset);
#endif

// All valid debug levels and flags.
// High byte is reserved for flags.
#define DEBUG_FLAG_THREADS		org_apache_trafodion_jdbc_t2_JdbcDebug_debugFlagThreads
#define DEBUG_FLAG_DIRTY_MEM	org_apache_trafodion_jdbc_t2_JdbcDebug_debugFlagDirtyMem
#define DEBUG_FLAG_TIMER		org_apache_trafodion_jdbc_t2_JdbcDebug_debugFlagTimer
#define DEBUG_FLAG_NOTIME		org_apache_trafodion_jdbc_t2_JdbcDebug_debugFlagNoTime
#define DEBUG_FLAG_TESTWARE		org_apache_trafodion_jdbc_t2_JdbcDebug_debugFlagTestware
#define DEBUG_LEVEL_ENTRY		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry
#define DEBUG_LEVEL_JAVA		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelJava
#define DEBUG_LEVEL_MEM			org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelMem
#define DEBUG_LEVEL_MEMLEAK		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelMemLeak
#define DEBUG_LEVEL_CLI			org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI
#define DEBUG_LEVEL_DATA		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelData
#define DEBUG_LEVEL_TXN			org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelTxn
#define DEBUG_LEVEL_ROWSET		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelRowset
#define DEBUG_LEVEL_ERROR		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelError
#define DEBUG_LEVEL_METADATA	org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelMetadata
#define DEBUG_LEVEL_UNICODE		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelUnicode
#define DEBUG_LEVEL_POOLING		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelPooling
#define DEBUG_LEVEL_STMT		org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelStmt
#define DEBUG_LEVEL_ALL			org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelAll

#define MEMORY_ALLOC_MEMORY		0
#define MEMORY_ALLOC_OBJECT		1

void DebugOutput(const char *msg,
				 const char *filename, unsigned long line);
const char *DebugFormat(const char *fmt, ...);
void DebugSetLevel(unsigned long debug_level);
void DebugClearLevel(unsigned long debug_level);
void DebugFunctionEntry(const char *function_name, unsigned long level,
						const char *params,
                        const char *filename, unsigned long line);
void DebugFunctionReturnNumeric(long rc, const char *msg, bool pop_stack,
						        const char *filename, unsigned long line);
void DebugFunctionReturnInt64(long long rc, const char *msg, bool pop_stack,
						        const char *filename, unsigned long line);
void DebugFunctionReturnDouble(double rc, const char *msg, bool pop_stack,
						        const char *filename, unsigned long line);
void DebugFunctionReturnPtr(void *rc, const char *msg, bool pop_stack,
                            const char *filename, unsigned long line);
void DebugFunctionReturn(const char *tag, const char *comment,
						 bool pop_stack, const char *call_type,
						 const char *filename, unsigned long line);
bool DebugActive(unsigned long debug_level,
				 const char *filename, unsigned long line);
const char *DebugString(const char *text);
const char *DebugString(const char *text, size_t len);
const char *DebugJString(void *jenv_ptr, void * jstring_ptr);
void DebugTransTag(const char * filename, unsigned long line);
const char *DebugBoolStr(bool isTrue);
const char *DebugTimestampStr(long long timestamp);
void DebugAssertFailed(const char *message,
					   const char *filename, unsigned long line);
void DebugMemoryAlloc(void *alloc_var, char alloc_type, unsigned long alloc_size,
					  bool perm, const char *filename, unsigned long line);
void DebugMemoryCheckAlloc(void *alloc_var,const char *filename, unsigned long line);
void DebugMemoryDelete(void *alloc_var,char alloc_type,const char *filename, unsigned long line);
void DebugPrintMemoryAlloc(const char *filename, unsigned long line);
void DebugPrintBuffer(void);
void DebugPrintMemoryDump(unsigned long debug_level,
						  const char *addr, unsigned long len,
						  const char *filename, unsigned long line);

const char *CliDebugStatementType(int stmtType);
const char *CliDebugSqlQueryStatementType(int stmtType);
const char *CliDebugSqlStatementType(int stmtType);
void CliDebugShowServerStatement(void *vSrvrStmt,
                                 const char *filename, unsigned long line);
const char *CliDebugSqlAttrType(int code);
const char *CliDebugSqlTypeCode(int code);
const char *CliDebugSqlError(long retcode);
const char *CliDebugSqlValueStr(const void *data_value);
const char *CliDebugDescTypeStr(int descType);
const char *CliDebugDescItemStr(long descItem);
void CliDebugShowDesc(void *vSrvrStmt, int descType,
                      const char *filename, unsigned long line);
void CliDebugShowQuad(void *vSrvrStmt, int descType,
                      const char *filename, unsigned long line);
const char *DebugSqlAttrTypeStr(long attrName);
const char *DebugSqlDiagCondStr(long cond);
const char *DebugSqlWhatDescStr(long what_desc);
void DebugShowOutputValueList(void *output_value_list, long colCount, const char *fcn_name,
		                      const char *filename, unsigned long line);
const char *DebugJavaObjectInfo(void *jenv, void *jobj);
const char *WrapperDataTypeStr(jbyte dataType);
#define FUNCTION_ENTRY(function_name,params) DebugFunctionEntry(function_name, 0, DebugFormat params , __FILE__, __LINE__)
#define FUNCTION_ENTRY_LEVEL_LOC(level,function_name,params,filename,line) DebugFunctionEntry(function_name, level, DebugFormat params , filename, line)
#define FUNCTION_ENTRY_LEVEL(level,function_name,params) FUNCTION_ENTRY_LEVEL_LOC(level,function_name,params,__FILE__,__LINE__)
#define FUNCTION_RETURN_NUMERIC_LOC(rc,comment,filename,line) { long debugRetNumRc = (long) (rc); DebugFunctionReturnNumeric(debugRetNumRc,DebugFormat comment, true, filename, line); return(debugRetNumRc); }
#define FUNCTION_RETURN_NUMERIC(rc,comment) FUNCTION_RETURN_NUMERIC_LOC(rc,comment,__FILE__,__LINE__)
#define FUNCTION_RETURN_INT64_LOC(rc,comment,filename,line) { long long debugRetNumRc = (long long) (rc); DebugFunctionReturnInt64(debugRetNumRc,DebugFormat comment, true, filename, line); return(debugRetNumRc); }
#define FUNCTION_RETURN_INT64(rc,comment) FUNCTION_RETURN_INT64_LOC(rc,comment,__FILE__,__LINE__)
#define FUNCTION_RETURN_FLOAT_LOC(rc,comment,filename,line) { float debugRetNumRc = (float) (rc); DebugFunctionReturnDouble(debugRetNumRc,DebugFormat comment, true, filename, line); return(debugRetNumRc); }
#define FUNCTION_RETURN_FLOAT(rc,comment) FUNCTION_RETURN_FLOAT_LOC(rc,comment,__FILE__,__LINE__)
#define FUNCTION_RETURN_DOUBLE_LOC(rc,comment,filename,line) { double debugRetNumRc = (double) (rc); DebugFunctionReturnDouble(debugRetNumRc,DebugFormat comment, true, filename, line); return(debugRetNumRc); }
#define FUNCTION_RETURN_DOUBLE(rc,comment) FUNCTION_RETURN_DOUBLE_LOC(rc,comment,__FILE__,__LINE__)
#define FUNCTION_RETURN_PTR_LOC(rc,comment,filename,line) { DebugFunctionReturnPtr((void *) (rc),DebugFormat comment , true, filename, line); return(rc); }
#define FUNCTION_RETURN_PTR(rc,comment) FUNCTION_RETURN_PTR_LOC(rc,comment,__FILE__,__LINE__)
#define FUNCTION_RETURN_VOID_LOC(comment,filename,line) { DebugFunctionReturn("VOID", DebugFormat comment , true, "RETURNING", filename, line) ; return; }
#define FUNCTION_RETURN_VOID(comment) FUNCTION_RETURN_VOID_LOC(comment,__FILE__,__LINE__)
#define FUNCTION_RETURN_TYPE(type_str,rc,comment) { DebugFunctionReturn(type_str, DebugFormat comment , true, "RETURNING", __FILE__, __LINE__) ; return(rc); }
#define DEBUG_OUT_LOC(selected_level,args, filename, line) { if (DebugActive(selected_level,filename,line)) DebugOutput (DebugFormat args , filename, line); }
#define DEBUG_OUT(selected_level,args) DEBUG_OUT_LOC(selected_level, args, __FILE__, __LINE__)
#define DEBUG_SET(level) DebugSetLevel(level)
#define DEBUG_CLEAR(level) DebugClearLevel(level)
#define DEBUG_ASSERT(expr,message) if (!(expr)) DebugAssertFailed(DebugFormat message , __FILE__, __LINE__) 
#define DEBUG_PRINT_BUFFER() DebugPrintBuffer()
#define MEMORY_ALLOC_SELECT(alloc_var,allocation,perm) { DebugMemoryCheckAlloc((void *) (alloc_var),__FILE__,__LINE__); (alloc_var) = new allocation; DebugMemoryAlloc((void *) (alloc_var),MEMORY_ALLOC_MEMORY,sizeof(allocation),perm,__FILE__,__LINE__); }
#define MEMORY_ALLOC_SELECT_ARRAY(alloc_var,allocation,count,perm) { size_t lcount = count; DebugMemoryCheckAlloc((void *) (alloc_var),__FILE__,__LINE__); (alloc_var) = new allocation[count]; DebugMemoryAlloc((void *) (alloc_var),MEMORY_ALLOC_MEMORY,sizeof(allocation)*lcount,perm,__FILE__,__LINE__); }
#define MEMORY_ALLOC_SELECT_OBJ(alloc_var,allocation,perm) { DebugMemoryCheckAlloc((void *) (alloc_var),__FILE__,__LINE__); (alloc_var) = new allocation; DebugMemoryAlloc((void *) (alloc_var),MEMORY_ALLOC_OBJECT,0,perm,__FILE__,__LINE__); }
#define MEMORY_ALLOC(alloc_var,allocation) MEMORY_ALLOC_SELECT(alloc_var,allocation,false)
#define MEMORY_ALLOC_ARRAY(alloc_var,allocation,count) MEMORY_ALLOC_SELECT_ARRAY(alloc_var,allocation,count,false)
#define MEMORY_ALLOC_OBJ(alloc_var,allocation) MEMORY_ALLOC_SELECT_OBJ(alloc_var,allocation,false)
#define MEMORY_ALLOC_PERM(alloc_var,allocation) MEMORY_ALLOC_SELECT(alloc_var,allocation,true)
#define MEMORY_ALLOC_PERM_ARRAY(alloc_var,allocation,count) MEMORY_ALLOC_SELECT_ARRAY(alloc_var,allocation,count,true)
#define MEMORY_ALLOC_PERM_OBJ(alloc_var,allocation) MEMORY_ALLOC_SELECT_OBJ(alloc_var,allocation,true)
#define MEMORY_DELETE(alloc_var) if (alloc_var) { DebugMemoryDelete((void *) (alloc_var),MEMORY_ALLOC_MEMORY,__FILE__,__LINE__); delete (alloc_var); (alloc_var) = NULL; }
#define MEMORY_DELETE_ARRAY(alloc_var) if (alloc_var) { DebugMemoryDelete((void *) (alloc_var),MEMORY_ALLOC_MEMORY,__FILE__,__LINE__); delete [] (alloc_var); (alloc_var) = NULL; }
#define MEMORY_DELETE_OBJ(alloc_var) if (alloc_var) { DebugMemoryDelete((void *) (alloc_var),MEMORY_ALLOC_OBJECT,__FILE__,__LINE__); delete (alloc_var); (alloc_var) = NULL; }
#define MEMORY_PRINT_ALLOC() DebugPrintMemoryAlloc(__FILE__,__LINE__)
#define MEMORY_DUMP(level,addr,len) DebugPrintMemoryDump(level, (const char *)(addr),len,__FILE__,__LINE__)

#define CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt) CliDebugShowServerStatement((void *)pSrvrStmt,__FILE__,__LINE__)
#define CLI_DEBUG_SHOW_DESC(pSrvrStmt, descType) CliDebugShowDesc((void *)pSrvrStmt,descType,__FILE__,__LINE__)
#define CLI_DEBUG_SHOW_QUAD(pSrvrStmt, descType) CliDebugShowQuad((void *)pSrvrStmt,descType,__FILE__,__LINE__)
#define CLI_DEBUG_RETURN_SQL_LOC(code,filename,line) { long debugSqlRc = (long) (code); FUNCTION_RETURN_NUMERIC_LOC(debugSqlRc,("(%s)",CliDebugSqlError(debugSqlRc)),filename,line); }
#define CLI_DEBUG_RETURN_SQL(code) CLI_DEBUG_RETURN_SQL_LOC(code,__FILE__,__LINE__)
#define CLI_SQL_VALUE_STR(data_value) CliDebugSqlValueStr((const void *)data_value)
#define DEBUG_SHOW_OUTPUT_VALUE_LIST(output_value_list, colCount, fcn_name) DebugShowOutputValueList(output_value_list, colCount, fcn_name,__FILE__,__LINE__)
#define DEBUG_TRANSTAG() DebugTransTag(__FILE__, __LINE__)

#define BENCHMARK_ADD_COUNTER(index,name,value)

#else /* !_DEBUG */

#ifdef _BENCHMARK
#include "Benchmark.h"
#else /* !_BENCHMARK */
#define FUNCTION_ENTRY(function_name, args)
#define FUNCTION_ENTRY_LEVEL(level,function_name,params)
#define FUNCTION_ENTRY_LEVEL_LOC(level,function_name,params,filename,line)
#define FUNCTION_RETURN_NUMERIC(rc,comment) return(rc)
#define FUNCTION_RETURN_NUMERIC_LOC(rc,comment,filename,line) return(rc)
#define FUNCTION_RETURN_INT64(rc,comment) return(rc)
#define FUNCTION_RETURN_INT64_LOC(rc,comment,filename,line) return(rc)
#define FUNCTION_RETURN_FLOAT(rc,comment) return(rc)
#define FUNCTION_RETURN_FLOAT_LOC(rc,comment,filename,line) return(rc)
#define FUNCTION_RETURN_DOUBLE(rc,comment) return(rc)
#define FUNCTION_RETURN_DOUBLE_LOC(rc,comment,filename,line) return(rc)
#define FUNCTION_RETURN_PTR(rc,comment) return(rc)
#define FUNCTION_RETURN_PTR_LOC(rc,comment,filename,line) return(rc)
#define FUNCTION_RETURN_VOID(comment) return
#define FUNCTION_RETURN_VOID_LOC(comment,filename,line) return
#define FUNCTION_RETURN_TYPE(type_str,rc,comment) return(rc)
#define CLI_DEBUG_RETURN_SQL(code) return(code)
#define CLI_DEBUG_RETURN_SQL_LOC(code,filename,line) return(code)
#define BENCHMARK_ADD_COUNTER(index,name,value)
#endif

#define DEBUG_OUT_LOC(selected_level,args, filename, line)
#define DEBUG_OUT(selected_level,args)
#define DEBUG_SET(level)
#define DEBUG_CLEAR(level)
#define DEBUG_ASSERT(expr,message)
#define DEBUG_PRINT_BUFFER()
#define MEMORY_ALLOC(alloc_var,allocation) { alloc_var = new allocation; if (alloc_var==NULL) exit(0); }
#define MEMORY_ALLOC_ARRAY(alloc_var,allocation,count) { alloc_var = new allocation[count]; if (alloc_var==NULL) exit(0); }
#define MEMORY_ALLOC_OBJ(alloc_var,allocation) MEMORY_ALLOC(alloc_var,allocation)
#define MEMORY_ALLOC_PERM(alloc_var,allocation) MEMORY_ALLOC(alloc_var,allocation)
#define MEMORY_ALLOC_PERM_ARRAY(alloc_var,allocation,count) MEMORY_ALLOC_ARRAY(alloc_var,allocation,count)
#define MEMORY_ALLOC_PERM_OBJ(alloc_var,allocation) MEMORY_ALLOC(alloc_var,allocation)
#define MEMORY_DELETE(alloc_var) if (alloc_var) { delete alloc_var; alloc_var = NULL; }
#define MEMORY_DELETE_ARRAY(alloc_var) if (alloc_var) { delete [] alloc_var; alloc_var = NULL; }
#define MEMORY_DELETE_OBJ(alloc_var) MEMORY_DELETE(alloc_var)
#define MEMORY_PRINT_ALLOC()
#define MEMORY_DUMP(level,addr,len)

#define CLI_DEBUG_SHOW_SERVER_STATEMENT(pSrvrStmt)
#define CLI_DEBUG_SHOW_DESC(pSrvrStmt, descType)
#define CLI_DEBUG_SHOW_QUAD(pSrvrStmt, descType)
#define CLI_SQL_VALUE_STR(data_value)
#define DEBUG_SHOW_OUTPUT_VALUE_LIST(output_value_list, colCount, fcn_name)
#define DEBUG_TRANSTAG()

#endif /* !_DEBUG */

#if defined(_DEBUG) || defined(_BENCHMARK)

#define CLI_SetStmtAttr(statement_id, attrName, numeric_value, string_value) CliDebug_SetStmtAttr(statement_id, attrName, numeric_value, string_value, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_SetStmtAttr(SQLSTMT_ID *statement_id, long attrName, long numeric_value, char *string_value, const char *filename, unsigned long line);
#define CLI_SetDescItem(sql_descriptor, entry, what_to_set, numeric_value, string_value) CliDebug_SetDescItem(sql_descriptor, entry, what_to_set, numeric_value, string_value, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_SetDescItem(SQLDESC_ID *sql_descriptor, long entry, long what_to_set, long numeric_value, char * string_value, const char *filename, unsigned long line);
#define CLI_SETROWSETDESCPOINTERS(desc_id, rowset_size, rowset_status_ptr, starting_entry, num_quadruple_fields, quad_fields) CliDebug_SETROWSETDESCPOINTERS(desc_id, rowset_size, rowset_status_ptr, starting_entry, num_quadruple_fields, quad_fields, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_SETROWSETDESCPOINTERS(SQLDESC_ID * desc_id, long rowset_size, int *rowset_status_ptr, long starting_entry, long num_quadruple_fields, struct SQLCLI_QUAD_FIELDS quad_fields[], const char *filename, unsigned long line);
#define CLI_GetDescItems2(sql_descriptor, no_of_desc_items, desc_items) CliDebug_GetDescItems2(sql_descriptor, no_of_desc_items, desc_items, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_GetDescItems2(SQLDESC_ID * sql_descriptor, long no_of_desc_items, SQLDESC_ITEM desc_items[], const char *filename, unsigned long line);
#define CLI_SetCursorName(statement_id, cursor_name) CliDebug_SetCursorName(statement_id, cursor_name, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_SetCursorName(SQLSTMT_ID * statement_id, SQLSTMT_ID * cursor_name, const char *filename, unsigned long line);
#define CLI_ExecFetch(statement_id, input_descriptor, num_ptr_pairs) CliDebug_ExecFetch(statement_id, input_descriptor, num_ptr_pairs, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_ExecFetch(SQLSTMT_ID *statement_id, SQLDESC_ID *input_descriptor, long num_ptr_pairs, const char *filename, unsigned long line);
#define CLI_CloseStmt(statement_id) CliDebug_CloseStmt(statement_id, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_CloseStmt (SQLSTMT_ID *statement_id, const char *filename, unsigned long line);
#define CLI_GetDiagnosticsCondInfo2(what_to_get, conditionNum, numeric_value, string_value, max_string_len, len_of_item) CliDebug_GetDiagnosticsCondInfo2(what_to_get, conditionNum, numeric_value, string_value, max_string_len, len_of_item, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_GetDiagnosticsCondInfo2(long what_to_get, long conditionNum, int *numeric_value, char * string_value, long max_string_len, int *len_of_item, const char *filename, unsigned long line);
#define CLI_SwitchContext(context_handle, prev_context_handle) CliDebug_SwitchContext(context_handle, prev_context_handle, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_SwitchContext(SQLCTX_HANDLE context_handle, SQLCTX_HANDLE * prev_context_handle, const char *filename, unsigned long line);
#define CLI_DeleteContext(contextHandle) CliDebug_DeleteContext(contextHandle, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_DeleteContext(SQLCTX_HANDLE contextHandle, const char *filename, unsigned long line);
#define CLI_CreateContext(context_handle, sqlAuthId, suppressAutoXactStart) CliDebug_CreateContext(context_handle, sqlAuthId, suppressAutoXactStart, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_CreateContext(SQLCTX_HANDLE *context_handle, char* sqlAuthId, long suppressAutoXactStart, const char *filename, unsigned long line);
#define CLI_CurrentContext(contextHandle) CliDebug_CurrentContext(contextHandle,__FILE__,__LINE__)
SQLCLI_LIB_FUNC long CliDebug_CurrentContext(SQLCTX_HANDLE *contextHandle, const char *filename, unsigned long line);
#define CLI_ClearDiagnostics(statement_id) CliDebug_ClearDiagnostics(statement_id, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_ClearDiagnostics (SQLSTMT_ID *statement_id, const char *filename, unsigned long line);
#define CLI_GetDiagnosticsStmtInfo2(statement_id, what_to_get, numeric_value, string_value, max_string_len, len_of_item) CliDebug_GetDiagnosticsStmtInfo2(statement_id, what_to_get, numeric_value, string_value, max_string_len, len_of_item, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_GetDiagnosticsStmtInfo2(SQLSTMT_ID *statement_id, long what_to_get, void *numeric_value, char *string_value, long max_string_len, int *len_of_item, const char *filename, unsigned long line);
#define CLI_DeallocDesc(desc_id) CliDebug_DeallocDesc(desc_id, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_DeallocDesc(SQLDESC_ID *desc_id, const char *filename, unsigned long line);
#define CLI_DeallocStmt(statement_id) CliDebug_DeallocStmt(statement_id, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_DeallocStmt(SQLSTMT_ID *statement_id, const char *filename, unsigned long line);
#define CLI_ExecDirect(statement_id, sql_source, input_descriptor, num_ptr_pairs) CliDebug_ExecDirect(statement_id, sql_source, input_descriptor, num_ptr_pairs, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_ExecDirect(SQLSTMT_ID *statement_id, SQLDESC_ID *sql_source, SQLDESC_ID *input_descriptor, long num_ptr_pairs, const char *filename, unsigned long line);
#define CLI_Exec(statement_id, input_descriptor, num_ptr_pairs) CliDebug_Exec(statement_id, input_descriptor, num_ptr_pairs, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_Exec(SQLSTMT_ID *statement_id, SQLDESC_ID *input_descriptor,long num_ptr_pairs, const char *filename, unsigned long line);
#define CLI_Prepare(statement_id, sql_source) CliDebug_Prepare(statement_id, sql_source, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_Prepare(SQLSTMT_ID *statement_id, SQLDESC_ID *sql_source, const char *filename, unsigned long line);
#define CLI_DescribeStmt(statement_id, input_descriptor, output_descriptor) CliDebug_DescribeStmt(statement_id, input_descriptor, output_descriptor, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_DescribeStmt(SQLSTMT_ID *statement_id, SQLDESC_ID *input_descriptor, SQLDESC_ID *output_descriptor, const char *filename, unsigned long line);
#define CLI_GetDescEntryCount(sql_descriptor, num_entries) CliDebug_GetDescEntryCount(sql_descriptor, num_entries, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_GetDescEntryCount(SQLDESC_ID *sql_descriptor, int *num_entries, const char *filename, unsigned long line);
#define CLI_Fetch(statement_id, output_descriptor, num_ptr_pairs) CliDebug_Fetch(statement_id, output_descriptor, num_ptr_pairs, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_Fetch(SQLSTMT_ID *statement_id, SQLDESC_ID *output_descriptor, long num_ptr_pairs, const char *filename, unsigned long line);
#define CLI_GetDescItem(sql_descriptor, entry, what_to_get, numeric_value, string_value, max_string_len, len_of_item, start_from_offset) CliDebug_GetDescItem(sql_descriptor, entry, what_to_get, numeric_value, string_value, max_string_len, len_of_item, start_from_offset, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_GetDescItem(SQLDESC_ID *sql_descriptor, long entry, long what_to_get, int *numeric_value, char *string_value, long max_string_len, int *len_of_item, long start_from_offset, const char *filename, unsigned long line);
#define CLI_Cancel(statement_id) CliDebug_Cancel(statement_id, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_Cancel(SQLSTMT_ID *statement_id, const char *filename, unsigned long line);
#define CLI_AllocStmt(new_statement_id,cloned_statement) CliDebug_AllocStmt(new_statement_id,cloned_statement, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_AllocStmt(SQLSTMT_ID * new_statement_id, SQLSTMT_ID *cloned_statement, const char *filename, unsigned long line);
#define CLI_ResDescName(statement_id, from_statement, what_desc) CliDebug_ResDescName(statement_id, from_statement, what_desc, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_ResDescName(SQLDESC_ID *statement_id, SQLSTMT_ID *from_statement, long what_desc, const char *filename, unsigned long line);
#define CLI_AllocDesc(desc_id, input_descriptor) CliDebug_AllocDesc(desc_id, input_descriptor, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_AllocDesc(SQLDESC_ID *desc_id, SQLDESC_ID *input_descriptor, const char *filename, unsigned long line);
#define CLI_AssocFileNumber(statement_id, file_number) CliDebug_AssocFileNumber(statement_id, file_number, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_AssocFileNumber(SQLSTMT_ID *statement_id, short file_number, const char *filename, unsigned long line);
#define CLI_CLI_VERSION() CliDebug_CLI_VERSION(__FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_CLI_VERSION(const char *filename, unsigned long line);
#define CLI_ClearExecFetchClose(statement_id, input_descriptor, output_descriptor, num_input_ptr_pairs, num_output_ptr_pairs, num_total_ptr_pairs) CliDebug_ClearExecFetchClose(statement_id, input_descriptor, output_descriptor, num_input_ptr_pairs, num_output_ptr_pairs, num_total_ptr_pairs,__FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_ClearExecFetchClose(SQLSTMT_ID *statement_id, SQLDESC_ID *input_descriptor, SQLDESC_ID* output_descriptor, long num_input_ptr_pairs, long num_output_ptr_pairs, long num_total_ptr_pairs,const char *filename, unsigned long line);
#define CLI_GetStmtAttr(statement_id, attrName, numeric_value, string_value, max_string_len, len_of_item) CliDebug_GetStmtAttr(statement_id, attrName, numeric_value, string_value, max_string_len, len_of_item, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_GetStmtAttr(SQLSTMT_ID *statement_id, long attrName, int *numeric_value, char *string_value, long max_string_len, int *len_of_item, const char *filename, unsigned long line);
#define CLI_AllocStmtForRS(callStmtId, resultSetIndex, resultSetStmtId) CliDebug_AllocStmtForRS(callStmtId, resultSetIndex, resultSetStmtId, __FILE__, __LINE__)
SQLCLI_LIB_FUNC long CliDebug_AllocStmtForRS(SQLSTMT_ID *callStmtId, long resultSetIndex, SQLSTMT_ID *resultSetStmtId, const char *filename, unsigned long line);

#define JNI_GetArrayLength(jenv, array) JNIDebug_GetArrayLength(jenv, array, __FILE__, __LINE__)
jsize JNIDebug_GetArrayLength(JNIEnv *jenv, jarray array, const char *filename, unsigned long line);

#define JNI_GetObjectArrayElement(jenv, array, index) JNIDebug_GetObjectArrayElement(jenv, array, index, __FILE__, __LINE__)
jobject JNIDebug_GetObjectArrayElement(JNIEnv *jenv, jobjectArray array, jsize index, const char *filename, unsigned long line);
#define JNI_GetObjectField(jenv, jobj, fieldID) JNIDebug_GetObjectField(jenv, jobj, fieldID, __FILE__, __LINE__)
jobject JNIDebug_GetObjectField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, const char *filename, unsigned long line);

#define JNI_GetByteArrayElements(jenv, array, isCopy) JNIDebug_GetByteArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jbyte *JNIDebug_GetByteArrayElements(JNIEnv *jenv, jbyteArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseByteArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseByteArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseByteArrayElements(JNIEnv *jenv, jbyteArray array, jbyte *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetShortArrayElements(jenv, array, isCopy) JNIDebug_GetShortArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jshort *JNIDebug_GetShortArrayElements(JNIEnv *env, jshortArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseShortArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseShortArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseShortArrayElements(JNIEnv *jenv, jshortArray array, jshort *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetIntArrayElements(jenv, array, isCopy) JNIDebug_GetIntArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jint *JNIDebug_GetIntArrayElements(JNIEnv *env, jintArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseIntArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseIntArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseIntArrayElements(JNIEnv *jenv, jintArray array, jint *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetLongArrayElements(jenv, array, isCopy) JNIDebug_GetLongArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jlong *JNIDebug_GetLongArrayElements(JNIEnv *env, jlongArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseLongArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseLongArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseLongArrayElements(JNIEnv *jenv, jlongArray array, jlong *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetFloatArrayElements(jenv, array, isCopy) JNIDebug_GetFloatArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jfloat *JNIDebug_GetFloatArrayElements(JNIEnv *env, jfloatArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseFloatArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseFloatArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseFloatArrayElements(JNIEnv *jenv, jfloatArray array, jfloat *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetDoubleArrayElements(jenv, array, isCopy) JNIDebug_GetDoubleArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jdouble *JNIDebug_GetDoubleArrayElements(JNIEnv *env, jdoubleArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseDoubleArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseDoubleArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseDoubleArrayElements(JNIEnv *jenv, jdoubleArray array, jdouble *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetBooleanArrayElements(jenv, array, isCopy) JNIDebug_GetBooleanArrayElements(jenv, array, isCopy, __FILE__, __LINE__)
jboolean *JNIDebug_GetBooleanArrayElements(JNIEnv *env, jbooleanArray array, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseBooleanArrayElements(jenv, array, elems, mode) JNIDebug_ReleaseBooleanArrayElements(jenv, array, elems, mode, __FILE__, __LINE__)
void JNIDebug_ReleaseBooleanArrayElements(JNIEnv *jenv, jbooleanArray array, jboolean *elems, jint mode, const char *filename, unsigned long line);

#define JNI_GetStringUTFChars(jenv, jstr, isCopy) JNIDebug_GetStringUTFChars(jenv, jstr, isCopy, __FILE__, __LINE__)
const char *JNIDebug_GetStringUTFChars(JNIEnv *jenv, jstring jstr, jboolean *isCopy, const char *filename, unsigned long line);
#define JNI_ReleaseStringUTFChars(jenv, jstr, chars) JNIDebug_ReleaseStringUTFChars(jenv, jstr, chars, __FILE__, __LINE__)
void JNIDebug_ReleaseStringUTFChars(JNIEnv *jenv, jstring jstr, const char* chars, const char *filename, unsigned long line);

#define JNI_GetMethodID(jenv, jcls, name, sig) JNIDebug_GetMethodID(jenv, jcls, name, sig, __FILE__, __LINE__)
jmethodID JNIDebug_GetMethodID(JNIEnv *jenv, jclass jcls, const char *name, const char *sig, const char *filename, unsigned long line);
#define JNI_GetFieldID(jenv, jcls, name, sig) JNIDebug_GetFieldID(jenv, jcls, name, sig, __FILE__, __LINE__)
jfieldID JNIDebug_GetFieldID(JNIEnv *jenv, jclass jcls, const char *name, const char *sig, const char *filename, unsigned long line);

#define JNI_GetObjectClass(jenv, jobj) JNIDebug_GetObjectClass(jenv, jobj, __FILE__, __LINE__)
jclass JNIDebug_GetObjectClass(JNIEnv *jenv, jobject jobj, const char *filename, unsigned long line);

#define JNI_IsInstanceOf(jenv, jobj, clazz) JNIDebug_IsInstanceOf(jenv, jobj, clazz, __FILE__, __LINE__)
jboolean JNIDebug_IsInstanceOf(JNIEnv *jenv, jobject jobj, jclass clazz, const char *filename, unsigned long line);

#define JNI_SetObjectField(jenv, jobj, fieldID, val) JNIDebug_SetObjectField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetObjectField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jobject val, const char *filename, unsigned long line);

#define JNI_SetBooleanField(jenv, jobj, fieldID, val) JNIDebug_SetBooleanField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetBooleanField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jboolean val, const char *filename, unsigned long line);

#define JNI_SetByteField(jenv, jobj, fieldID, val) JNIDebug_SetByteField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetByteField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jbyte val, const char *filename, unsigned long line);

#define JNI_SetShortField(jenv, jobj, fieldID, val) JNIDebug_SetShortField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetShortField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jshort val, const char *filename, unsigned long line);

#define JNI_SetIntField(jenv, jobj, fieldID, val) JNIDebug_SetIntField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetIntField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jint val, const char *filename, unsigned long line);

#define JNI_SetLongField(jenv, jobj, fieldID, val) JNIDebug_SetLongField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetLongField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jlong val, const char *filename, unsigned long line);

#define JNI_SetFloatField(jenv, jobj, fieldID, val) JNIDebug_SetFloatField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetFloatField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jfloat val, const char *filename, unsigned long line);

#define JNI_SetDoubleField(jenv, jobj, fieldID, val) JNIDebug_SetDoubleField(jenv, jobj, fieldID, val, __FILE__, __LINE__)
void JNIDebug_SetDoubleField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jdouble val, const char *filename, unsigned long line);

#define JNI_SetObjectArrayElement(jenv, array, index, val) JNIDebug_SetObjectArrayElement(jenv, array, index, val, __FILE__, __LINE__)
void JNIDebug_SetObjectArrayElement(JNIEnv *jenv, jobjectArray array, jsize index, jobject val, const char *filename, unsigned long line);

#define JNI_SetBooleanArrayRegion(jenv, array, start, len, buf) JNIDebug_SetBooleanArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetBooleanArrayRegion(JNIEnv *jenv, jbooleanArray array, jsize start, jsize len, jboolean *buf, const char *filename, unsigned long line);

#define JNI_SetByteArrayRegion(jenv, array, start, len, buf) JNIDebug_SetByteArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetByteArrayRegion(JNIEnv *jenv, jbyteArray array, jsize start, jsize len, jbyte *buf, const char *filename, unsigned long line);

#define JNI_SetCharArrayRegion(jenv, array, start, len, buf) JNIDebug_SetCharArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetCharArrayRegion(JNIEnv *jenv, jcharArray array, jsize start, jsize len, jchar *buf, const char *filename, unsigned long line);

#define JNI_SetShortArrayRegion(jenv, array, start, len, buf) JNIDebug_SetShortArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetShortArrayRegion(JNIEnv *jenv, jshortArray array, jsize start, jsize len, jshort *buf, const char *filename, unsigned long line);

#define JNI_SetIntArrayRegion(jenv, array, start, len, buf) JNIDebug_SetIntArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetIntArrayRegion(JNIEnv *jenv, jintArray array, jsize start, jsize len, jint *buf, const char *filename, unsigned long line);

#define JNI_SetLongArrayRegion(jenv, array, start, len, buf) JNIDebug_SetLongArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetLongArrayRegion(JNIEnv *jenv, jlongArray array, jsize start, jsize len, jlong *buf, const char *filename, unsigned long line);

#define JNI_SetFloatArrayRegion(jenv, array, start, len, buf) JNIDebug_SetFloatArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetFloatArrayRegion(JNIEnv *jenv, jfloatArray array, jsize start, jsize len, jfloat *buf, const char *filename, unsigned long line);

#define JNI_SetDoubleArrayRegion(jenv, array, start, len, buf) JNIDebug_SetDoubleArrayRegion(jenv, array, start, len, buf, __FILE__, __LINE__)
void JNIDebug_SetDoubleArrayRegion(JNIEnv *jenv, jdoubleArray array, jsize start, jsize len, jdouble *buf, const char *filename, unsigned long line);

#define JNI_NewByteArray(jenv, len) JNIDebug_NewByteArray(jenv, len, __FILE__, __LINE__)
jbyteArray JNIDebug_NewByteArray(JNIEnv *jenv, jsize len, const char *filename, unsigned long line);

#define JNI_NewIntArray(jenv, len) JNIDebug_NewIntArray(jenv, len, __FILE__, __LINE__)
jintArray JNIDebug_NewIntArray(JNIEnv *jenv, jsize len, const char *filename, unsigned long line);

#define JNI_NewObjectArray(jenv, len, clazz, init) JNIDebug_NewObjectArray(jenv, len, clazz, init, __FILE__, __LINE__)
jobjectArray JNIDebug_NewObjectArray(JNIEnv *jenv, jsize len, jclass clazz, jobject init, const char *filename, unsigned long line);

#define JNI_ExceptionClear(jenv) JNIDebug_ExceptionClear(jenv, __FILE__, __LINE__)
void JNIDebug_ExceptionClear(JNIEnv *jenv, const char *filename, unsigned long line);

#define JNI_Throw(jenv,obj) JNIDebug_Throw(jenv, obj, __FILE__, __LINE__)
jint JNIDebug_Throw(JNIEnv *jenv, jthrowable obj, const char *filename, unsigned long line);

#define JNI_FindClass(jenv,name) JNIDebug_FindClass(jenv, name, __FILE__, __LINE__)
jclass JNIDebug_FindClass(JNIEnv *jenv, const char *name, const char *filename, unsigned long line);

#define JNI_NewGlobalRef(jenv,lobj) JNIDebug_NewGlobalRef(jenv, lobj, __FILE__, __LINE__)
jobject JNIDebug_NewGlobalRef(JNIEnv *jenv, jobject lobj, const char *filename, unsigned long line);

#else  /* !(defined(_DEBUG) || defined(_BENCHMARK)) */

#define CLI_SetStmtAttr(statement_id, attrName, numeric_value, string_value) SQL_EXEC_SetStmtAttr(statement_id, attrName, numeric_value, string_value)
#define CLI_SetDescItem(sql_descriptor, entry, what_to_set, numeric_value, string_value) SQL_EXEC_SetDescItem(sql_descriptor, entry, what_to_set, numeric_value, string_value)
#define CLI_SETROWSETDESCPOINTERS(desc_id, rowset_size, rowset_status_ptr, starting_entry, num_quadruple_fields, quad_fields) SQL_EXEC_SETROWSETDESCPOINTERS(desc_id, rowset_size, rowset_status_ptr, starting_entry, num_quadruple_fields, quad_fields)
#define CLI_GetDescItems2(sql_descriptor, no_of_desc_items, desc_items) SQL_EXEC_GetDescItems2(sql_descriptor, no_of_desc_items, desc_items)
#define CLI_SetCursorName(statement_id, cursor_name) SQL_EXEC_SetCursorName(statement_id, cursor_name)
#define CLI_ExecFetch(statement_id, input_descriptor, num_ptr_pairs) SQL_EXEC_ExecFetch(statement_id, input_descriptor, num_ptr_pairs)
#define CLI_CloseStmt(statement_id) SQL_EXEC_CloseStmt(statement_id)
#define CLI_GetDiagnosticsCondInfo2(what_to_get, conditionNum, numeric_value, string_value, max_string_len, len_of_item) SQL_EXEC_GetDiagnosticsCondInfo2(what_to_get, conditionNum, numeric_value, string_value, max_string_len, len_of_item)
#define CLI_SwitchContext(context_handle, prev_context_handle) SQL_EXEC_SwitchContext(context_handle, prev_context_handle)
#define CLI_DeleteContext(contextHandle) SQL_EXEC_DeleteContext(contextHandle)
#define CLI_CreateContext(context_handle, sqlAuthId, suppressAutoXactStart) SQL_EXEC_CreateContext(context_handle, sqlAuthId, suppressAutoXactStart)
#define CLI_CurrentContext(contextHandle) SQL_EXEC_CurrentContext(contextHandle)
#define CLI_ClearDiagnostics(statement_id) SQL_EXEC_ClearDiagnostics(statement_id)
#define CLI_GetDiagnosticsStmtInfo2(statement_id, what_to_get, numeric_value, string_value, max_string_len, len_of_item) SQL_EXEC_GetDiagnosticsStmtInfo2(statement_id, what_to_get, numeric_value, string_value, max_string_len, len_of_item)
#define CLI_DeallocDesc(desc_id) SQL_EXEC_DeallocDesc(desc_id)
#define CLI_DeallocStmt(statement_id) SQL_EXEC_DeallocStmt(statement_id)
#define CLI_ExecDirect(statement_id, sql_source, input_descriptor, num_ptr_pairs) SQL_EXEC_ExecDirect(statement_id, sql_source, input_descriptor, num_ptr_pairs)
#define CLI_Exec(statement_id, input_descriptor, num_ptr_pairs) SQL_EXEC_Exec(statement_id, input_descriptor, num_ptr_pairs)
#define CLI_Prepare(statement_id, sql_source) SQL_EXEC_Prepare(statement_id, sql_source)
#define CLI_DescribeStmt(statement_id, input_descriptor, output_descriptor) SQL_EXEC_DescribeStmt(statement_id, input_descriptor, output_descriptor)
#define CLI_GetDescEntryCount(sql_descriptor, num_entries) SQL_EXEC_GetDescEntryCount(sql_descriptor, num_entries)
#define CLI_Fetch(statement_id, output_descriptor, num_ptr_pairs) SQL_EXEC_Fetch(statement_id, output_descriptor, num_ptr_pairs)
#define CLI_GetDescItem(sql_descriptor, entry, what_to_get, numeric_value, string_value, max_string_len, len_of_item, start_from_offset) SQL_EXEC_GetDescItem(sql_descriptor, entry, what_to_get, numeric_value, string_value, max_string_len, len_of_item, start_from_offset)
#define CLI_Cancel(statement_id) SQL_EXEC_Cancel(statement_id)
#define CLI_AllocStmt(new_statement_id,cloned_statement) SQL_EXEC_AllocStmt(new_statement_id,cloned_statement)
#define CLI_ResDescName(statement_id, from_statement, what_desc) SQL_EXEC_ResDescName(statement_id, from_statement, what_desc)
#define CLI_AllocDesc(desc_id, input_descriptor) SQL_EXEC_AllocDesc(desc_id, input_descriptor)
#define CLI_AssocFileNumber(statement_id, file_number) SQL_EXEC_AssocFileNumber(statement_id, file_number)
#define CLI_CLI_VERSION() SQL_EXEC_CLI_VERSION()
#define CLI_ClearExecFetchClose(statement_id, input_descriptor, output_descriptor, num_input_ptr_pairs, num_output_ptr_pairs, num_total_ptr_pairs) SQL_EXEC_ClearExecFetchClose(statement_id, input_descriptor, output_descriptor, num_input_ptr_pairs, num_output_ptr_pairs, num_total_ptr_pairs)
#define CLI_GetStmtAttr(statement_id, attrName, numeric_value, string_value, max_string_len, len_of_item) SQL_EXEC_GetStmtAttr(statement_id, attrName, numeric_value, string_value, max_string_len, len_of_item)
#define CLI_AllocStmtForRS(callStmtId, resultSetIndex, resultSetStmtId) wAllocStmtForRS(callStmtId, resultSetIndex, resultSetStmtId)

#define JNI_GetArrayLength(jenv, array) jenv->GetArrayLength(array)
#define JNI_GetObjectArrayElement(jenv, array, index) jenv->GetObjectArrayElement(array, index)
#define JNI_GetObjectField(jenv, jobj, fieldID) jenv->GetObjectField(jobj, fieldID)
#define JNI_GetByteArrayElements(jenv, array, isCopy) jenv->GetByteArrayElements(array, isCopy)
#define JNI_ReleaseByteArrayElements(jenv, array, elems, mode) jenv->ReleaseByteArrayElements(array, elems, mode)
#define JNI_GetShortArrayElements(jenv, array, isCopy) jenv->GetShortArrayElements(array, isCopy)
#define JNI_ReleaseShortArrayElements(jenv, array, elems, mode) jenv->ReleaseShortArrayElements(array, elems, mode)
#define JNI_GetIntArrayElements(jenv, array, isCopy) jenv->GetIntArrayElements(array, isCopy)
#define JNI_ReleaseIntArrayElements(jenv, array, elems, mode) jenv->ReleaseIntArrayElements(array, elems, mode)
#define JNI_GetLongArrayElements(jenv, array, isCopy) jenv->GetLongArrayElements(array, isCopy)
#define JNI_ReleaseLongArrayElements(jenv, array, elems, mode) jenv->ReleaseLongArrayElements(array, elems, mode)
#define JNI_GetFloatArrayElements(jenv, array, isCopy) jenv->GetFloatArrayElements(array, isCopy)
#define JNI_ReleaseFloatArrayElements(jenv, array, elems, mode) jenv->ReleaseFloatArrayElements(array, elems, mode)
#define JNI_GetDoubleArrayElements(jenv, array, isCopy) jenv->GetDoubleArrayElements(array, isCopy)
#define JNI_ReleaseDoubleArrayElements(jenv, array, elems, mode) jenv->ReleaseDoubleArrayElements(array, elems, mode)
#define JNI_GetBooleanArrayElements(jenv, array, isCopy) jenv->GetBooleanArrayElements(array, isCopy)
#define JNI_ReleaseBooleanArrayElements(jenv, array, elems, mode) jenv->ReleaseBooleanArrayElements(array, elems, mode)
#define JNI_GetStringUTFChars(jenv, jstr, isCopy) jenv->GetStringUTFChars(jstr, isCopy)
#define JNI_ReleaseStringUTFChars(jenv, jstr, chars) jenv->ReleaseStringUTFChars(jstr, chars)
#define JNI_GetMethodID(jenv, jcls, name, sig) jenv->GetMethodID(jcls, name, sig)
#define JNI_GetFieldID(jenv, jcls, name, sig) jenv->GetFieldID(jcls, name, sig)
#define JNI_GetObjectClass(jenv, jobj) jenv->GetObjectClass(jobj)
#define JNI_IsInstanceOf(jenv, jobj, clazz) jenv->IsInstanceOf(jobj, clazz)
#define JNI_SetObjectField(jenv, jobj, fieldID, val) jenv->SetObjectField(jobj, fieldID, val)
#define JNI_SetBooleanField(jenv, jobj, fieldID, val) jenv->SetBooleanField(jobj, fieldID, val)
#define JNI_SetByteField(jenv, jobj, fieldID, val) jenv->SetByteField(jobj, fieldID, val)
#define JNI_SetShortField(jenv, jobj, fieldID, val) jenv->SetShortField(jobj, fieldID, val)
#define JNI_SetIntField(jenv, jobj, fieldID, val) jenv->SetIntField(jobj, fieldID, val)
#define JNI_SetLongField(jenv, jobj, fieldID, val) jenv->SetLongField(jobj, fieldID, val)
#define JNI_SetFloatField(jenv, jobj, fieldID, val) jenv->SetFloatField(jobj, fieldID, val)
#define JNI_SetDoubleField(jenv, jobj, fieldID, val) jenv->SetDoubleField(jobj, fieldID, val)
#define JNI_SetObjectArrayElement(jenv, array, index, val) jenv->SetObjectArrayElement(array, index, val)
#define JNI_SetBooleanArrayRegion(jenv, array, start, len, buf) jenv->SetBooleanArrayRegion(array, start, len, buf)
#define JNI_SetByteArrayRegion(jenv, array, start, len, buf) jenv->SetByteArrayRegion(array, start, len, buf)
#define JNI_SetCharArrayRegion(jenv, array, start, len, buf) jenv->SetCharArrayRegion(array, start, len, buf)
#define JNI_SetShortArrayRegion(jenv, array, start, len, buf) jenv->SetShortArrayRegion(array, start, len, buf)
#define JNI_SetIntArrayRegion(jenv, array, start, len, buf) jenv->SetIntArrayRegion(array, start, len, buf)
#define JNI_SetLongArrayRegion(jenv, array, start, len, buf) jenv->SetLongArrayRegion(array, start, len, buf)
#define JNI_SetFloatArrayRegion(jenv, array, start, len, buf) jenv->SetFloatArrayRegion(array, start, len, buf)
#define JNI_SetDoubleArrayRegion(jenv, array, start, len, buf) jenv->SetDoubleArrayRegion(array, start, len, buf)
#define JNI_NewByteArray(jenv, len) jenv->NewByteArray(len)
#define JNI_NewIntArray(jenv, len) jenv->NewIntArray(len)
#define JNI_NewObjectArray(jenv, len, clazz, init) jenv->NewObjectArray(len, clazz, init)
#define JNI_ExceptionClear(jenv) jenv->ExceptionClear()
#define JNI_Throw(jenv,obj) jenv->Throw(obj)
#define JNI_FindClass(jenv,name) jenv->FindClass(name)
#define JNI_NewGlobalRef(jenv,lobj) jenv->NewGlobalRef(lobj)

#endif /* !(defined(_DEBUG) || defined(_BENCHMARK)) */

#endif /* DEBUG_H */
