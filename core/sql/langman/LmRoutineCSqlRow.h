#ifndef LMROUTINECSQLROW_H
#define LMROUTINECSQLROW_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineCSqlRow.h
* Description:  LmRoutine for C routines using parameter style SQLROW
* Created:      08/02/2009
* Language:     C++
*
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

#include "LmRoutineC.h"
#include "sqludr.h"

//////////////////////////////////////////////////////////////////////
//
// LmRoutineCSqlRow
//
// The LmRoutineCSqlRow is a concrete class used to maintain state
// for, and the invocation of, a C routine that uses the SQLROW
// parameter style. Its base class representation is returned by the
// LMC as a handle to LM clients.
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmRoutineCSqlRow : public LmRoutineC
{
  friend class LmLanguageManagerC;
  
public:
  virtual LmResult invokeRoutine(void *inputRow,
				 void *outputRow,
                                 ComDiagsArea *da);

protected:
  LmRoutineCSqlRow(
    const char   *sqlName,
    const char   *externalName,
    const char   *librarySqlName,
    ComUInt32    numSqlParam,
    char         *routineSig,
    ComUInt32    maxResultSets,
    ComRoutineTransactionAttributes transactionAttrs,
    ComRoutineSQLAccess sqlAccessMode,
    ComRoutineExternalSecurity externalSecurity,
    Int32        routineOwnerId,
    const char   *parentQid,
    ComUInt32    inputRowLen,
    ComUInt32    outputRowLen,
    const char   *currentUserName,
    const char   *sessionUserName,
    LmParameter  *parameters,
    LmLanguageManagerC *lm,
    LmHandle     routine,
    LmContainer  *container,
    ComDiagsArea *diagsArea);

  virtual ~LmRoutineCSqlRow();

}; // class LmRoutineCSqlRow

#endif
