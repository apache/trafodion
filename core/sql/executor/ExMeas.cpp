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
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExMeas.cpp
 * Description:  methods for classes ExMeasStmtCntrs and ExMeasProcCntrs
 *               
 *               
 * Created:      9/26/2000
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ExMeas.h"
#include "str.h"
#include "ComCextdecs.h"

#include "dmeasql.h"


#include "seabed/trace.h"

#include "ex_ex.h"

/******************************************************************/
// class ExMeasStmtCntrs    
/******************************************************************/
 
void ExMeasStmtCntrs::init(Int32 statementIndex)
{
#pragma nowarn(1506)   // warning elimination 
  stmtCtrs_.statement_index = statementIndex;
#pragma warn(1506)  // warning elimination 
  stmtCtrs_.sorts = 0;
  stmtCtrs_.recompiles = 0;
  stmtCtrs_.lock_waits = 0;
  stmtCtrs_.calls = 0;
  stmtCtrs_.elapsed_busy_time = 0;
  stmtCtrs_.records_accessed = 0;
  stmtCtrs_.records_used = 0;
  stmtCtrs_.disc_reads = 0;
  stmtCtrs_.messages = 0;
  stmtCtrs_.message_bytes = 0;
  stmtCtrs_.elapsed_sort_time = 0;
  stmtCtrs_.elapsed_compile_time = 0;
  stmtCtrs_.timeouts = 0;
  stmtCtrs_.escalations = 0;
};  

ExMeasStmtCntrs::ExMeasStmtCntrs()
{
  init(0);
#ifdef NA_LINUX_DEBUG
  trace_printf("ExMeasStmtCntrsBump(%p) ctor\n", this);
#endif
}

ExMeasStmtCntrs::~ExMeasStmtCntrs()
{
#ifdef NA_LINUX_DEBUG
  trace_printf("ExMeasStmtCntrsBump(%p) dtor\n", this);
#endif
};
  
// Update Measure statement counters.
Int32 ExMeasStmtCntrs::ExMeasStmtCntrsBump(Int32 statementCount, char *moduleName, 
					 Int32 moduleNameLen)
{

#ifdef NA_LINUX_DEBUG
  trace_printf("ExMeasStmtCntrsBump (%p): %d %s\n", this, statementCount, moduleName);
#endif
  return 0;
}




/******************************************************************/
// class ExMeasProcCntrs    
/******************************************************************/
ExMeasProcCntrs::ExMeasProcCntrs()
{
  procCtrs_.sql_obj_recompiles= 0;
  procCtrs_.sql_obj_recompile_time = 0;
  procCtrs_.sql_stmt_recompiles = 0;
  procCtrs_.sql_stmt_recompile_time = 0;
  procCtrs_.sql_newprocesses = 0;
  procCtrs_.sql_newprocess_time = 0;
  procCtrs_.opens = 0;
  procCtrs_.open_time = 0;
#ifdef NA_LINUX_DEBUG
  trace_printf("ExMeasProcCntrs(%p) ctor\n", this);
#endif
};

ExMeasProcCntrs::~ExMeasProcCntrs()
{
#ifdef NA_LINUX_DEBUG
  trace_printf("ExMeasProcCntrs(%p) dtor\n", this);
#endif
};

Int32 ExMeasProcCntrs::ExMeasProcCntrsBump ()
{
#ifdef NA_LINUX_DEBUG
  trace_printf("ExMeasProcCntrsBump\n");
#endif
  return 0;
}

Int32 ExMeasGetStatus (NABoolean &stmtflag /* out */
                   , NABoolean &procflag /* out */
                   , NABoolean &measureflag /* out */)
{
  Int32 status = 0;
  static short dSqlMeasureEnabled = 0;
  static bool  bCheckedMeasureEnv = false;

  if (!bCheckedMeasureEnv) {
    const char * pEnvStr = getenv("SQL_MEASURE_ENABLE");
    if (pEnvStr != NULL) {
      dSqlMeasureEnabled = atoi(pEnvStr);
    }
    bCheckedMeasureEnv = true;
  }
  if (dSqlMeasureEnabled) {
      stmtflag = 1;
      procflag = 1;
      measureflag = 1;
  } else {
      stmtflag = 0;
      procflag = 0;
      measureflag = 0;
  }
  
  return 0;
}
