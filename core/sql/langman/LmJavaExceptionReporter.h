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
#ifndef LMJAVAEXCEPTIONREPORTER_H
#define LMJAVAEXCEPTIONREPORTER_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmJavaExceptionReporterHandler.h
* Description:  Java Exception Reporting Mechanism
*
* Created:      08/21/2003
* Language:     C++
*
*
******************************************************************************
*/

#include "LmError.h"
#include "LmCommon.h"
#include "LmRoutineJava.h"
#include "LmJavaSignature.h"

#define min(a,b)            (((a) < (b)) ? (a) : (b))

//////////////////////////////////////////////////////////////////////
//
// Forward Reference Classes
//
//////////////////////////////////////////////////////////////////////
class ComDiagsArea;
class LmRoutineJava;
class LmLanguageManagerJava;

//////////////////////////////////////////////////////////////////////
//
// Contents
//
//////////////////////////////////////////////////////////////////////
class LmJavaExceptionReporter;

//////////////////////////////////////////////////////////////////////
//
// LmJavaExceptionReporter 
// This class is encapsulates the Java Exception reporting mechanism.
// LmLanguageManagerJava uses this class to check for the exceptions
// and to report them.
//
// LmLanguageManagerJava instantiates an object of this class in its
// constructor. The constructor of LmJavaExceptionReporter loads
// java.lang.throwable, java.sql.SQLException,
// com.tandem.sqlmx.MethodValidationFailedException classes and gets
// pointers to the needed methods.
//
//////////////////////////////////////////////////////////////////////
class LmJavaExceptionReporter : public NABasicObject
{

public:
  LmJavaExceptionReporter(LmHandle jniEnv,
                  LmLanguageManagerJava *lm,
                  LmResult &result,
                  ComDiagsArea *diagsArea);

  ~LmJavaExceptionReporter();

  LmResult checkGetMethodExceptions(
    const char *rtName,
    const char *clName,
    ComDiagsArea *da);

  LmResult checkNewObjectExceptions(LmHandle jobj, ComDiagsArea *da);

  LmResult processUserException(LmRoutineJava *handle, ComDiagsArea *diagsArea);

  LmResult insertDiags(
    ComDiagsArea *da,
    Int32 eCode,
    const char *a1 = NULL,
    const char *a2 = NULL,
    LmHandle jt = NULL);

  LmResult checkJVMException(ComDiagsArea *da, LmHandle jt = NULL);

  void processJavaObjException(LmHandle returnInfoObj,
                               int returnStatus,
                               int callPhase,
                               const char *errText,
                               const char *udrName,
                               ComDiagsArea *da);

private:
  LmResult loadThrowable(ComDiagsArea *diags);

  LmResult loadSQLException(ComDiagsArea *diags);

  LmResult loadMethodValidationFailedException(ComDiagsArea *diags);

  void addJavaExceptionToDiags(LmHandle throwable, ComDiagsArea &diags);

  LmHandle getNextChainedException(LmHandle throwable);

  void reportUserSQLException(LmHandle jt, char *errText, ComDiagsArea *da);

  void reportInternalSPJException(LmHandle jt, char *errText, ComDiagsArea *da);

private:
  LmHandle jniEnv_;                // JNI handle. 
  LmLanguageManagerJava *langMan_; // Langman reference

  LmHandle throwableClass_;        // java.Lang.Throwable
  LmHandle throwableToStringId_;   // toString
  LmHandle throwableGetCauseId_;   // getCause method

  LmHandle exSQLClass_;            // java.sql.SQLException
  LmHandle exSQLStateId_;          // getSQLState method
  LmHandle exErrorCodeId_;         // getErrorCode method
  LmHandle exNextExceptionId_;     // getNextException method

  LmHandle exMetValFailedClass_;   // org.trafodion.sql.udr.
                                   //  MethodValidationFailedException
  LmHandle exGetMethodName_;       // getMethodName
  LmHandle exGetSignature_;        // getSignature

}; // class LmJavaExceptionReporter

#endif
