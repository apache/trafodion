#ifndef LMROUTINECSQLROWTM_H
#define LMROUTINECSQLROWTM_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineCSqlRowTM.h
* Description:  LmRoutine for C routines using Table Mapping Functions
* Created:      02/14/2010
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

class SqlBuffer;


class SQLLM_LIB_FUNC LmTableInfo
{
public:

  LmTableInfo()
  {
    tabIndex_ = 0;
    tableNameLen_ = 0;
    tableName_ = NULL;
    numInColumns_ = 0;
    inColumnParam_ = NULL;
    reqSqlBuffer_ = NULL;
    emitSqlBuffer_ = NULL;
    rowLength_= 0;
    lastReqBuffer_ = FALSE;

  }

  virtual ~LmTableInfo()
  {
  }

  void freeResources(NAHeap *udrHeap)
  {
    udrHeap->deallocateMemory(tableName_);
    tableName_ = NULL;

    udrHeap->deallocateMemory(inColumnParam_);
    inColumnParam_ = NULL;

    reset(udrHeap);
  }


  // Deallocate and reset members such that we are
  // ready to process reinvokes. Note that we are not
  // deleting other members that was part of load message.
  void reset(NAHeap *udrHeap);

  void setReqSqlBuffer(SqlBuffer * buf){reqSqlBuffer_ = buf;}
  void setEmitSqlBuffer(SqlBuffer * buf){emitSqlBuffer_ = buf;}
  void deleteReqSqlBuffer(void) { free(reqSqlBuffer_); reqSqlBuffer_ = NULL; }
  ComUInt32 getRequestRowSize(void) { return rowLength_;}
  NABoolean isLastReqSqlBuffer(void){ return lastReqBuffer_; }
  void setLastReqSqlBuffer(void) { lastReqBuffer_ = TRUE; }

  SqlBuffer* getReqSqlBuffer(void) { return reqSqlBuffer_;}
  SqlBuffer* getEmitSqlBuffer(void) { return emitSqlBuffer_;}
  

  ComUInt32           tabIndex_;
  ComUInt32           tableNameLen_;
  char                *tableName_;
  ComUInt32           numInColumns_;
  LmParameter         *inColumnParam_;
  ComUInt32           rowLength_;
  
  
  

private:

  SqlBuffer           *reqSqlBuffer_;
  SqlBuffer           *emitSqlBuffer_;
  NABoolean           lastReqBuffer_;

};



//////////////////////////////////////////////////////////////////////
//
// LmRoutineCSqlRowTM
//
// The LmRoutineCSqlRowTM is a concrete class used to maintain state
// for, and the invocation of, a C routine that uses the SQLROW
// parameter style. Its base class representation is returned by the
// LMC as a handle to LM clients.
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmRoutineCSqlRowTM : public LmRoutineC
{
  friend class LmLanguageManagerC;
  
public:

  virtual LmResult invokeRoutine(void *inputRow,
				 void *outputRow,
                                 ComDiagsArea *da);

  char * rowDataSpace1_;
  char * rowDataSpace2_;
  SQLUDR_GetNextRow   getNextRowPtr_;
  SQLUDR_EmitRow      emitRowptr_;

protected:
  LmRoutineCSqlRowTM(
    const char   *sqlName,
    const char   *externalName,
    const char   *librarySqlName,
    ComUInt32    numSqlParam,
    ComUInt32    numTableInfo,
    LmTableInfo  *tableInfo,
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
    LmHandle     getnextRowPtr,
    LmHandle     emitRowPtr,
    LmContainer  *container,
    ComDiagsArea *diagsArea);

  virtual ~LmRoutineCSqlRowTM();

}; // class LmRoutineCSqlRowTM

#endif
