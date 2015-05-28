#ifndef LMROUTINEJAVA_H
#define LMROUTINEJAVA_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineJava.h
* Description:  
*
* Created:      08/22/2003
* Language:     C++
*
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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

#include "ComSmallDefs.h"
#include "NABoolean.h"
#include "LmRoutine.h"
#include "LmLangManagerJava.h"
#include "LmParameter.h"
#include "LmJavaType.h"
#include "LmContManager.h"
#include "LmResultSetJava.h"
#include "LmConnection.h"

//////////////////////////////////////////////////////////////////////
//
// LmRoutineJava
//
// The LmRoutineJava is a concrete class used to maintain state for,
// and the invocation of, a static Java method. Its base class
// representation is returned by the LMJ as a handle to LM clients.
//
//////////////////////////////////////////////////////////////////////
class LmRoutineJava : public LmRoutine
{
friend class LmLanguageManagerJava;
friend class LmJavaExceptionReporter;

public:

  // Deletes the given LmResultSet object.
  virtual void cleanupLmResultSet(LmResultSet *resultSet,
                          ComDiagsArea *diagsArea = NULL);

  // Deletes the LmResultSet object at a given index.
  void cleanupLmResultSet(ComUInt32 index,
                          ComDiagsArea *diagsArea = NULL);

  // Deletes the LmResultSetJava objects in the resultSetList_
  void cleanupResultSets(ComDiagsArea *diagsArea = NULL);

  // Main routine invocation method and its support methods.
  virtual LmResult invokeRoutine(void *inputRow,
				 void *outputRow,
                                 ComDiagsArea *da);

  // A flag to indicate whether the default catalog and schema should
  // be set in the Java environment prior to invokeRoutine
  void setDefaultCatSchFlag(NABoolean value) { defaultCatSch_ = value; }
  NABoolean getDefaultCatSchFlag() const { return defaultCatSch_; }

protected:
  LmRoutineJava(
    const char   *sqlName,
    const char   *externalName,
    const char   *librarySqlName,
    ComUInt32     numSqlParam,
    LmParameter  *returnValue,
    ComUInt32     maxResultSets,
    char         *routineSig,
    ComRoutineParamStyle paramStyle,
    ComRoutineTransactionAttributes transactionAttrs,
    ComRoutineSQLAccess sqlAccessMode,
    ComRoutineExternalSecurity externalSecurity,
    Int32    routineOwnerId,
    const char   *parentQid,
    ComUInt32     inputRowLen,
    ComUInt32     outputRowLen,
    const char   *currentUserName,
    const char   *sessionUserName,
    LmParameter  *parameters,
    LmLanguageManagerJava *lm,
    LmHandle      routine,
    LmContainer  *container,
    ComDiagsArea *diagsArea);

  virtual ~LmRoutineJava();

  // Utilities.
  LmHandle getContainerHandle()
    { return container()->getHandle(); }

  inline void setUdrForJavaMain(ComBoolean main)
    { udrForJavaMain_ = main; }

  inline ComBoolean isUdrForJavaMain() const
    { return udrForJavaMain_; }

  inline void setIsInternalSPJ(ComBoolean internal)
    { udrForInternalSPJ_ = internal; }

  inline ComBoolean isInternalSPJ() const
    { return udrForInternalSPJ_; }

  virtual ComBoolean isValid();

  // This method invokes a static Java method that returns void. In
  // the future if we need to support Java methods that return a value
  // (for example, for Java UDFs) new methods can be created.
  LmResult voidRoutine(void*, LmParameter*);

  // This method creates a LmResultSet object for the passed in 
  // Java result set only when the result set is not null, not closed
  // and not a duplicate. The newly created LmResultSet object is
  // added to a NAList data structure.
  LmResult populateResultSetInfo(LmHandle rs, Int32 paramPos, ComDiagsArea *da);

  // Checks if the passed in Java result set object is already
  // part of a LmResultSet object in the result set list.
  NABoolean isDuplicateRS( LmHandle newRs );

  // Deletes the LmResultSetJava objects in resultSetList_
  // over and above the routine's decalred max result set value
  void deleteLmResultSetsOverMax(ComDiagsArea *diagsArea);

  void closeDefConnWithNoRS(ComDiagsArea *diagsArea = NULL);

  virtual LmLanguageManagerJava *getLM()
  { return (LmLanguageManagerJava *) lm_; }

private:
  LmResult generateDefAuthToken(char *defAuthToken, ComDiagsArea *da);

private:
  void *javaParams_;               // Java method parameters (array of jvalue).
  LmJavaType::Type retType_;       // Routine return type.
  ComBoolean udrForJavaMain_;      // Routine is for Java main()
  ComBoolean udrForInternalSPJ_;   // Routine for internal SPJ

  NABoolean  defaultCatSch_;       // flag that tells if cat & sch
                                   // need to be set

  // List of LmConnection objects for default and non-default
  // Java connection objects created during the SPJ's invocation.
  NAList<LmConnection*> connectionList_;

}; // class LmRoutineJava

#endif
