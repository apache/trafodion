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
#ifndef _DMEASQL
#define _DMEASQL

//#pragma columns 79
#pragma page "dmeasql.h - T9086 Measure / SQL Interface Declarations"


//#ifndef _rosgenh_
//#define _rosgenh_
//#include "rosgen.h"
//#endif  


typedef struct SQLSTMT_CTRS_DEF {
  char          run_unit[128] ;
  Int32           statement_index; //indexed statement number
  Int32           sorts;
  Int32           recompiles;
  Int32           lock_waits;
  Int64         calls;
  Int64         elapsed_busy_time;
  Int64         records_used;
  Int64         records_accessed;
  Int64         disc_reads;
  Int64         messages;
  Int64         message_bytes;
  Int64         elapsed_sort_time;
  Int64         elapsed_compile_time;
  Int32           timeouts;
  Int32           escalations;
  char          _filler[4];
} SQLSTMT_CTRS_DEF;

typedef struct SQLPROC_CTRS_DEF {
  Int32    sql_obj_recompiles;
  Int64  sql_obj_recompile_time;
  Int32    sql_stmt_recompiles;
  Int64  sql_stmt_recompile_time;
  Int32    sql_newprocesses;
  Int64  sql_newprocess_time;
  Int32    opens;
  Int64  open_time;
} SQLPROC_CTRS_DEF;

//#pragma section MEASCHECKITOUT
//extern _tal _callable int_16 MEASCHECKITOUT();

enum {MEASURE_NOTHING = 0};
enum {MEASURE_STMT_ONLY = 1};
enum {MEASURE_PROC_ONLY = 2};
enum {MEASURE_PROC_AND_STMTS = 3};

//#pragma section MEASSQLPROCBUMP
//extern _tal _callable _resident int_16 MEASSQLPROCBUMP
// (SQLPROC_CTRS_DEF  *sqlproc_ctrs);

//#pragma section MEASSQLSTMTBUMP
//extern _tal _callable _resident _extensible int_16 MEASQLSTMTBUMP
//  (SQLSTMT_CTRS_DEF *sqlstmt_ctrs,
//   int_16  num_stmts,
//   int_16  total_proc,
//   int_16  caller);

enum {SQL_MP_CALLER = 1};   // parameters to be passed in MEASQLSTMTBUMP
enum {SQL_ARK_CALLER = 2};

#endif  /* _DMEASQL defined */
