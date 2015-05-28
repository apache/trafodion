#ifndef LMROUTINECPPOBJ_H
#define LMROUTINECPPOBJ_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineCppObj.h
* Description:  LmRoutine for C++ routines
* Created:      01/22/2015
* Language:     C++
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/

#include "LmRoutineC.h"
#include "sqludr.h"

class SqlBuffer;


//////////////////////////////////////////////////////////////////////
//
// LmRoutineCppObj
//
// The LmRoutineCppObj is a concrete class used to maintain state
// for, and the invocation of, a C++ routine that uses the object
// interface (class TMUDRInterface).
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmRoutineCppObj : public LmRoutineC
{
  friend class LmLanguageManagerC;
  
public:

  virtual LmResult invokeRoutine(
       void *inputRow,
       void *outputRow,
       ComDiagsArea *da);
  virtual LmResult invokeRoutineMethod(
       tmudr::UDRInvocationInfo::CallPhase phase,
       void *parameterRow,
       ComDiagsArea *da);
  LmResult validateWall(char *userBuf,
                        int userBufLen,
                        ComDiagsArea *da,
                        const char *bufferName);
  LmResult validateWalls(ComDiagsArea *da = NULL);

protected:
  LmRoutineCppObj(
    tmudr::UDRInvocationInfo *invocationInfo,
    tmudr::UDRPlanInfo       *planInfo,
    tmudr::UDR               *interfaceObj,
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
    ComUInt32    inputParamRowLen,
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

  virtual ~LmRoutineCppObj();

 private:

  void setUpWall(char *userBuf, int userBufLen);

  tmudr::UDRInvocationInfo *invocationInfo_;
  tmudr::UDRPlanInfo       *planInfo_;
  tmudr::UDR               *interfaceObj_;

  // number, lengths and pointers to row buffers
  int paramRowLen_;
  char *paramRow_;
  int numInputTables_;
  int *inputRowLengths_;
  int outputRowLen_;
  char **inputRows_;
  char *outputRow_;
}; // class LmRoutineCppObj

#endif
