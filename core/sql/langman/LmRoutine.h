#ifndef LMROUTINE_H
#define LMROUTINE_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutine.h.h
* Description:
*
* Created:      08/22/2003
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

// Some data members such as the NAList come from outside this
// DLL. The Windows compiler generates a warning about them requiring
// a DLL interface in order to be used by LmRoutine clients. We
// will suppress such warnings.

#include "LmCommon.h"
#include "ComSmallDefs.h"
#include "LmAssert.h"
#include "LmResultSet.h"
#include "LmContManager.h"

// Some data members such as the NAList come from outside this
// DLL. The Windows compiler generates a warning about them requiring
// a DLL interface in order to be used by LmRoutine clients. We
// will suppress such warnings.

//////////////////////////////////////////////////////////////////////
//
// Forward References
//
//////////////////////////////////////////////////////////////////////
class LmContainer;
class LmLanguageManager;
class LmParameter;

//////////////////////////////////////////////////////////////////////
//
// LmRoutine
//
// The LmRoutine class represents a handle to an external routine,
// being return by the LM to a client so the client can execute the
// the routine using the invokeRoutine service. Internally the class
// records invocation state related to the external routine as determined
// by a specific LM, possibly via derivation.
//
//////////////////////////////////////////////////////////////////////
class SQLLM_LIB_FUNC LmRoutine : public NABasicObject
{
public:

  // Returns the number of result sets that were returned by the 
  // routine
  ComUInt32 getNumResultSets() const { return resultSetList_.entries(); }

  // Gets the LmResultSet object from the resultSetList_ given a 
  // index as the input. Asserts on an invalid index.
  LmResultSet *getLmResultSet(ComUInt32 index) const
  { 
    LM_ASSERT(index < resultSetList_.entries());  
    return resultSetList_[index]; 
  }

  ComRoutineTransactionAttributes getTransactionAttrs() const { return transactionAttrs_; }
  ComRoutineSQLAccess getSqlAccessMode() const { return sqlAccessMode_; }

  const char *getSqlName() const { return sqlName_; }

  const char *getLibrarySqlName() const { return librarySqlName_; }

  // Deletes the given LmResultSet object.
  // Any error during the close will be reported in the 
  // diagsArea, if available. Asserts on an invalid pointer.
  virtual void cleanupLmResultSet(LmResultSet *resultSet,
                                  ComDiagsArea *diagsArea = NULL) = 0;
  
  // Deletes the LmResultSet object at a given index.
  // Any error during the close will be reported in the 
  // diagsArea, if available. Asserts on an invalid index.
  virtual void cleanupLmResultSet(ComUInt32 index,
                                  ComDiagsArea *diagsArea = NULL) = 0;
  
  // Deletes all the LmResultSet objects from the resultSetList_ 
  // and empties the list. Any error during the cleanup process 
  // will be reported in the diagsArea, if available.
  virtual void cleanupResultSets(ComDiagsArea *diagsArea = NULL) = 0;
  
  virtual const char *getParentQid() { return parentQid_; }

  virtual LmResult setRuntimeInfo(
       const char   *parentQid,
       int           numTotalInstances,
       int           myInstanceNum,
       ComDiagsArea *da);

  // Main routine invocation method
  virtual LmResult invokeRoutine(void *inputRow,
				 void *outputRow,
                                 ComDiagsArea *da) = 0;
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
  virtual LmResult getRoutineInvocationInfo
    (
         /* IN/OUT */ char         *serializedInvocationInfo,
         /* IN */     Int32         invocationInfoMaxLen,
         /* OUT */    Int32        *invocationInfoLenOut,
         /* IN/OUT */ char         *serializedPlanInfo,
         /* IN */     Int32         planInfoMaxLen,
         /* IN */     Int32         planNum,
         /* OUT */    Int32        *planInfoLenOut,
         /* IN/OUT */ ComDiagsArea *da);

  virtual LmResult setFunctionPtrs(SQLUDR_GetNextRow getNextRowPtr,
                                   SQLUDR_EmitRow emitRowPtr,
                                   ComDiagsArea *da);

  ComRoutineLanguage getLanguage() const { return language_; }
  ComRoutineParamStyle getParamStyle() const { return paramStyle_; }

  ComRoutineExternalSecurity getExternalSecurity() const
                             { return externalSecurity_; }
  Int32 getRoutineOwnerId() const { return routineOwnerId_; }


  LmParameter *getLmParams() const { return lmParams_; }

  Int32 getInputParamRowLen() const { return inputParamRowLen_; }
  Int32 getOutputRowLen() const { return outputRowLen_; }

  // Returns the routine name to use for error reporting.
  // For command-line operations the sqlName_ is not available 
  // and so we'll use the externalName_ instead.
  const char *getNameForDiags();

  virtual ~LmRoutine();

  virtual LmLanguageManager *getLM() { return lm_; }

  LmContainer* container()
    { return (LmContainer*)container_; }

  LmHandle getContainerHandle()
    { return ((LmContainer*)container_)->getHandle(); }

  // make a final call to the UDF or do the equivalent, if needed
  virtual LmResult handleFinalCall(ComDiagsArea *diagsArea = NULL) = 0;

protected:
  LmRoutine(LmHandle container,
            LmHandle routine,
            const char *sqlName,
            const char *externalName,
            const char *librarySqlName,
            ComUInt32 numParam,
            ComUInt32 maxResultSets,
            ComRoutineLanguage language,
            ComRoutineParamStyle paramStyle,
            ComRoutineTransactionAttributes transactionAttrs,
            ComRoutineSQLAccess sqlAccessMode,
            ComRoutineExternalSecurity externalSecurity,
            Int32 routineOwnerId,
            const char *parentQid,
            Int32 inputParamRowLen,
            Int32 outputRowLen,
            const char   *currentUserName,
            const char   *sessionUserName,
            LmParameter *lmParams,
            LmLanguageManager *lm);

  LmLanguageManager *lm_;       // Owning LM
  LmHandle container_;          // Handle to the external routine's container.
  LmHandle routine_;            // Handle to the external routine.
  ComUInt32 numSqlParam_;       // Number of parameters.
  ComUInt32 numParamsInSig_;    // Number of params in the method signature 
  ComUInt32 maxResultSets_;     // Maximum number of resultsets that
                                // this routine can return

  NAList<LmResultSet*> resultSetList_;  // LmResultSet objects. Only
                                        // Non-Null, Not-Closed &
                                        // Non-Duplicate result sets
                                        // are stored.

  ComRoutineTransactionAttributes transactionAttrs_;//check if transaction is required
	
  ComRoutineSQLAccess  sqlAccessMode_;  // level of SQL access that is
                                        // allowed within external routine

  char  *externalName_;     // Name of the C/Java function or method
  char  *sqlName_;          // Routine's three part ANSI name
  char  *librarySqlName_;   // Routine Library's three part ANSI name
  const char *parentQid_;   // Query ID of the parent SQL statement

  ComRoutineLanguage language_;
  ComRoutineParamStyle paramStyle_;

  ComRoutineExternalSecurity externalSecurity_;
  Int32 routineOwnerId_;

  Int32 inputParamRowLen_;
  Int32 outputRowLen_;

  const char *udrCatalog_;	   // Default catalog value
  const char *udrSchema_;	   // Default schema value

  LmParameter *lmParams_;

private:
  // Do not implement a default constructor
  LmRoutine();

}; // class LmRoutine


#endif
