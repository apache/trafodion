#ifndef LMROUTINEJAVAOBJ_H
#define LMROUTINEJAVAOBJ_H
/* -*-C++-*-
**********************************************************************
*
* File:         LmRoutineJavaObj.h
* Description:  
*
* Created:      04/30/2015
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

#include "LmRoutineJava.h"
#include "LmLangManagerJava.h"
#include "sqludr.h"

//////////////////////////////////////////////////////////////////////
//
// LmRoutineJava
//
// The LmRoutineJavaObj is a concrete class used to maintain state for,
// and the invocation of, a fixed set of methods for objects
// of a class derived from tmudr::UDR.
//////////////////////////////////////////////////////////////////////
class LmRoutineJavaObj : public LmRoutineJava
{
friend class LmLanguageManagerJava;
friend class LmJavaExceptionReporter;

public:

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

protected:

  LmRoutineJavaObj(
       const char   *sqlName,
       const char   *externalName,
       const char   *librarySqlName,
       ComRoutineTransactionAttributes transactionAttrs,
       ComRoutineSQLAccess sqlAccessMode,
       ComRoutineExternalSecurity externalSecurity,
       Int32    routineOwnerId,
       const char   *serializedInvocationInfo,
       int           invocationInfoLen,
       const char   *serializedPlanInfo,
       int           planInfoLen,
       LmLanguageManagerJava *lm,
       jobject       udrObject,
       LmContainer  *container,
       ComDiagsArea *diagsArea);

  ~LmRoutineJavaObj();

  // check whether the constructor successfully created the Java objects
  virtual ComBoolean isValid() const;

  // global reference to UDR object, of the user-defined class
  // that implements the UDR
  jobject udrObjectG_;

  // global reference to the LmUDRObjMethodInvoke object that
  // is used to unpack/pack information and invoke routines, this
  // is the one we actually call through JNI
  jobject udrInvokeObjectG_;

  // local reference to a byte array with the last returned
  // serialized UDRInvocationInfo and UDRPlanInfo objects and
  // their lengths
  jbyteArray returnedIIL_;
  jbyteArray returnedPIL_;
  jsize iiLen_;
  jsize piLen_;
}; // class LmRoutineJava

#endif
