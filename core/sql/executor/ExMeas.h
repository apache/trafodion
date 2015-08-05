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
#ifndef __EX_MEAS_H__
#define __EX_MEAS_H__

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExMeas.h
 * Description:  
 *               
 *               
 * Created:      9/25/2000
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"

#include "ComSizeDefs.h"
#include "Int64.h"
#include "BaseTypes.h"

#define EXP_SHORT_OV_ADD_SETMAX( op1,  op2) 0
#define EXP_FIXED_OV_ADD_SETMAX( op1,  op2) 0 

// Define for the max Ansiname size.
// See max Ansiname size definitions in w:/common/ComSizeDefs.h
#define MODULE_ID_SIZE ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES

// Defines for dynamic statement name

#define DYNAMIC_STMT_NAME "SQLMX^EXECUTE_"
#define DYNAMIC_STMT_NAME_SIZE 14


/*****************************************************************/
/* classes defined in this file                                  */
/*****************************************************************/
//class ExMeasStmtCntrs;
//class ExMeasProcCntrs;

/*****************************************************************/
/* ExMeasGetStatus                                               */
/* Function to check if measure is enabled.                      */
/*  stmtflag is set true if statement counter is enabled,        */
/*  procflag is set true if process counter is enabled.          */
/*****************************************************************/
Int32 ExMeasGetStatus (NABoolean &stmtflag /* out */
                   , NABoolean &procflag /* out */
                   , NABoolean &measureflag /* out */);   

/******************************************************************/
/* class ExMeasStmtCntrs                                          */
/*                                                                */
/* To store Measure Sqlstmt counters.                             */
/******************************************************************/


#include "dmeasql.h"


class ExMeasStmtCntrs 
{
public:
NA_EIDPROC
   ExMeasStmtCntrs();

NA_EIDPROC
   ~ExMeasStmtCntrs();

NA_EIDPROC
   void init(Int32 statementIndex);

NA_EIDPROC
inline void setStatementIndex (short i) { stmtCtrs_.statement_index = i; }

NA_EIDPROC
inline void setSorts (short s) { stmtCtrs_.sorts = s; }

NA_EIDPROC
inline void setRecompiles (short c) { stmtCtrs_.recompiles = c; }

NA_EIDPROC
inline void setLockWaits (short lw) { stmtCtrs_.lock_waits = lw; }

NA_EIDPROC
inline void setCalls (Int64 c) { stmtCtrs_.calls = c; }

NA_EIDPROC
inline void setElapsedBusyTime(Int64 et) { stmtCtrs_.elapsed_busy_time = et; } 

NA_EIDPROC
inline void setRowsUsed (Int64 ru) { stmtCtrs_.records_used = ru; }

NA_EIDPROC
inline void setRowsAccessed (Int64 ra) { stmtCtrs_.records_accessed = ra; }

NA_EIDPROC
inline void setDiscReads (Int64 dr) { stmtCtrs_.disc_reads = dr; }

NA_EIDPROC
inline void setMessages (Int64 m) { stmtCtrs_.messages = m; }

NA_EIDPROC
inline void setMessageBytes (Int64 mb) { stmtCtrs_.message_bytes = mb; }

NA_EIDPROC
inline void setElapsedSortTime (Int64 st) { stmtCtrs_.elapsed_sort_time = st; }

NA_EIDPROC
inline void setElapsedCompileTime(Int64 ct) { stmtCtrs_.elapsed_compile_time = ct; }

NA_EIDPROC
inline void setTimeouts (short to) { stmtCtrs_.timeouts = to; }

NA_EIDPROC
inline void setEscalations (short e) { stmtCtrs_.escalations = e; }

NA_EIDPROC
inline void incSorts (short s) 
  { stmtCtrs_.sorts = EXP_SHORT_OV_ADD_SETMAX (stmtCtrs_.sorts, s); }

NA_EIDPROC
inline void incRecompiles (short c) 
  { stmtCtrs_.recompiles = EXP_SHORT_OV_ADD_SETMAX (stmtCtrs_.recompiles, c); }

NA_EIDPROC
inline void incLockWaits (short lw) 
  { stmtCtrs_.lock_waits = EXP_SHORT_OV_ADD_SETMAX (stmtCtrs_.lock_waits, lw); }

NA_EIDPROC
inline void incCalls (Int64 c) 
  { stmtCtrs_.calls = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.calls,  c); }

NA_EIDPROC
inline void incElapseBusyTime(Int64 et) 
  { stmtCtrs_.elapsed_busy_time = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.elapsed_busy_time, et); } 

NA_EIDPROC
inline void incRowsUsed (Int64 ru) 
  { stmtCtrs_.records_used = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.records_used, ru); }

NA_EIDPROC
inline void incRowsAccessed (Int64 ra) 
  { stmtCtrs_.records_accessed = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.records_accessed, ra); }

NA_EIDPROC
inline void incDiscReads (Int64 dr) 
  { stmtCtrs_.disc_reads = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.disc_reads, dr); }

NA_EIDPROC
inline void incMessages (Int64 m) 
  { stmtCtrs_.messages = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.messages, m); }

NA_EIDPROC
inline void incMessageBytes (Int64 mb) 
  { stmtCtrs_.message_bytes = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.message_bytes, mb); }

NA_EIDPROC
inline void incElapsedSortTime (Int64 st) 
  { stmtCtrs_.elapsed_sort_time = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.elapsed_sort_time, st); }

NA_EIDPROC
inline void incElapsedCompileTime(Int64 ct) 
  { stmtCtrs_.elapsed_compile_time = EXP_FIXED_OV_ADD_SETMAX (stmtCtrs_.elapsed_compile_time, ct); }

NA_EIDPROC
inline void incTimeouts (short to) 
  { stmtCtrs_.timeouts = EXP_SHORT_OV_ADD_SETMAX (stmtCtrs_.timeouts, to); }

NA_EIDPROC
inline void incEscalations (short e) 
  { stmtCtrs_.escalations = EXP_SHORT_OV_ADD_SETMAX (stmtCtrs_.escalations, e); }

// Update Measure statement counters.
NA_EIDPROC
Int32 ExMeasStmtCntrsBump (Int32 statementCount, char *moduleName, Int32 moduleNameLen);

private:
    SQLSTMT_CTRS_DEF stmtCtrs_; 

};


/******************************************************************/
/* class ExMeasProcCntrs                                          */
/*                                                                */
/* To store Measure Sqlproc counters. Should have the same layout */
/* as the Measure SQLPROC^CTRS^DEF.                               */
/******************************************************************/

class ExMeasProcCntrs
{
public:
NA_EIDPROC
   ExMeasProcCntrs();

NA_EIDPROC
   ~ExMeasProcCntrs();

NA_EIDPROC
inline void setSqlObjRecompiles(short rc) { procCtrs_.sql_obj_recompiles = rc; }

NA_EIDPROC
inline void setSqlObjRecompileTime(Int64 rct) { procCtrs_.sql_obj_recompile_time = rct; } 

NA_EIDPROC
inline void setSqlStmtRecompiles(short rc) { procCtrs_.sql_stmt_recompiles = rc; }

NA_EIDPROC
inline void setSqlStmtRecompileTime(Int64 rct) { procCtrs_.sql_stmt_recompile_time = rct; }

NA_EIDPROC
inline void setSqlNewprocesses(short n) { procCtrs_.sql_newprocesses = n; }

NA_EIDPROC
inline void setSqlNewprocesstime(Int64 nt) { procCtrs_.sql_newprocess_time = nt; }

NA_EIDPROC
inline void setOpens (short op) { procCtrs_.opens = op; }

NA_EIDPROC
inline void setOpenTime (Int64 ot) { procCtrs_.open_time = ot; }

NA_EIDPROC
inline void incSqlObjRecompiles(short rc) 
  { procCtrs_.sql_obj_recompiles = EXP_SHORT_OV_ADD_SETMAX (procCtrs_.sql_obj_recompiles, rc); }

NA_EIDPROC
inline void incSqlObjRecompileTime(Int64 rct) 
  { procCtrs_.sql_obj_recompile_time = EXP_FIXED_OV_ADD_SETMAX (procCtrs_.sql_obj_recompile_time, rct ); } 

NA_EIDPROC
inline void incSqlStmtRecompiles(short rc) 
  { procCtrs_.sql_stmt_recompiles = EXP_SHORT_OV_ADD_SETMAX (procCtrs_.sql_stmt_recompiles, rc); }

NA_EIDPROC
inline void incSqlStmtRecompileTime(Int64 rct) 
  { procCtrs_.sql_stmt_recompile_time = EXP_FIXED_OV_ADD_SETMAX (procCtrs_.sql_stmt_recompile_time, rct ); }

NA_EIDPROC
inline void incNewprocess (short np) 
  { procCtrs_.sql_newprocesses =  EXP_SHORT_OV_ADD_SETMAX (procCtrs_.sql_newprocesses, np ); }

NA_EIDPROC
inline void incNewprocessTime (Int64 nt) 
  { procCtrs_.sql_newprocess_time = EXP_FIXED_OV_ADD_SETMAX (procCtrs_.sql_newprocess_time, nt); }

NA_EIDPROC
inline void incOpens (short op) 
  { procCtrs_.opens =  EXP_SHORT_OV_ADD_SETMAX (procCtrs_.opens, op ); }

NA_EIDPROC
inline void incOpenTime (Int64 ot) { procCtrs_.open_time = procCtrs_.open_time + ot; }

// Update Measure process counters.
NA_EIDPROC
Int32 ExMeasProcCntrsBump ();

private:
    SQLPROC_CTRS_DEF procCtrs_;

};

#endif
