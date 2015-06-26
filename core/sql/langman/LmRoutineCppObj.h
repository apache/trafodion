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
class SQLLM_LIB_FUNC LmRoutineCppObj : public LmRoutine
{
  friend class LmLanguageManagerC;
  
public:

  tmudr::UDRInvocationInfo *getInvocationInfo() { return invocationInfo_; }
  virtual const char *getParentQid();
  virtual void setRuntimeInfo(
       const char   *parentQid,
       int           totalNumInstances,
       int           myInstanceNum);
  virtual LmResult invokeRoutine(
       void         *inputRow,
       void         *outputRow,
       ComDiagsArea *da);
  virtual LmResult invokeRoutineMethod(
       /* IN */     tmudr::UDRInvocationInfo::CallPhase phase,
       /* IN */     const char   *serializedInvocationInfo,
       /* IN */     Int32         invocationInfoLen,
       /* OUT */    Int32        *invocationInfoLenOut,
       /* IN */     const char   *serializedPlanInfo,
       /* IN */     Int32         planInfoLen,
       /* IN */     Int32         planNum,
       /* OUT */    Int32        *planInfoLenOut,
       /* IN */     char         *inputRow,
       /* IN */     Int32         inputRowLen,
       /* OUT */    char         *outputRow,
       /* IN */     Int32         outputRowLen,
       /* IN/OUT */ ComDiagsArea *da);
  virtual LmResult getRoutineInvocationInfo(
       /* IN/OUT */ char         *serializedInvocationInfo,
       /* IN */     Int32         invocationInfoMaxLen,
       /* OUT */    Int32        *invocationInfoLenOut,
       /* IN/OUT */ char         *serializedPlanInfo,
       /* IN */     Int32         planInfoMaxLen,
       /* IN */     Int32         planNum,
       /* OUT */    Int32        *planInfoLenOut,
       /* IN/OUT */ ComDiagsArea *da);
  void setFunctionPtrs(SQLUDR_GetNextRow getNextRowPtr,
                       SQLUDR_EmitRow emitRowPtr);
  LmResult validateWall(char *userBuf,
                        int userBufLen,
                        ComDiagsArea *da,
                        const char *bufferName);
  LmResult validateWalls(ComDiagsArea *da = NULL);

  // The following pure virtual methods must be implemented even
  // though we do not currently support result sets for C++ routines
  void cleanupLmResultSet(LmResultSet *resultSet,
                          ComDiagsArea *diagsArea = NULL) {}
  void cleanupLmResultSet(ComUInt32 index,
                          ComDiagsArea *diagsArea = NULL) {}
  void cleanupResultSets(ComDiagsArea *diagsArea = NULL) {}

  virtual LmResult handleFinalCall(ComDiagsArea *diagsArea = NULL);

protected:
  LmRoutineCppObj(
    tmudr::UDRInvocationInfo *invocationInfo,
    tmudr::UDRPlanInfo       *planInfo,
    tmudr::UDR               *interfaceObj,
    const char   *sqlName,
    const char   *externalName,
    const char   *librarySqlName,
    ComUInt32    maxResultSets,
    ComRoutineTransactionAttributes transactionAttrs,
    ComRoutineSQLAccess sqlAccessMode,
    ComRoutineExternalSecurity externalSecurity,
    Int32        routineOwnerId,
    LmLanguageManagerC *lm,
    LmContainer  *container,
    ComDiagsArea *diagsArea);

  virtual ~LmRoutineCppObj();
  LmResult dealloc(ComDiagsArea *diagsArea);

 private:

  void setUpWall(char *userBuf, int userBufLen);

  tmudr::UDRInvocationInfo     *invocationInfo_;
  NAArray<tmudr::UDRPlanInfo *> planInfos_;
  tmudr::UDR                   *interfaceObj_;

  // number, lengths and pointers to row buffers
  char *paramRow_; // of length inputParamRowLen_ (stored in base class)
  int numInputTables_;
  int *inputRowLengths_;
  char **inputRows_;
  char *outputRow_; // of length outputRowLen_ (stored in base class)
}; // class LmRoutineCppObj

#endif
