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
******************************************************************************
*
* File:         LmJavaExceptionReporter.cpp
* Description:  Java Exception Reporting Mechanism
*
* Created:      08/21/2003
* Language:     C++
*
*
******************************************************************************
*/
#include "Platform.h"
  #include "lmjni.h"
#include "LmJavaExceptionReporter.h"
	
LmJavaExceptionReporter::LmJavaExceptionReporter(LmHandle jniEnv,
                                                LmLanguageManagerJava *langMan,
                                                LmResult &result,
                                                ComDiagsArea *diagsArea)
 : jniEnv_(jniEnv),
   langMan_(langMan),
   throwableClass_(NULL),
   throwableToStringId_(NULL),
   throwableGetCauseId_(NULL),
   exSQLClass_(NULL),
   exSQLStateId_(NULL),
   exErrorCodeId_(NULL),
   exNextExceptionId_(NULL),
   exMetValFailedClass_(NULL),
   exGetMethodName_(NULL),
   exGetSignature_(NULL)
{
  if (loadThrowable(diagsArea) == LM_ERR ||
      loadSQLException(diagsArea) == LM_ERR ||
      loadMethodValidationFailedException(diagsArea) == LM_ERR)
  {
    result = LM_ERR;
    return;
  }
}


LmJavaExceptionReporter::~LmJavaExceptionReporter()
{
  JNIEnv *jni = (JNIEnv*) jniEnv_;

  if (throwableClass_)
    jni->DeleteGlobalRef((jobject) throwableClass_);

  if (exSQLClass_)
    jni->DeleteGlobalRef((jobject) exSQLClass_);

  if (exMetValFailedClass_)
    jni->DeleteGlobalRef((jobject) exMetValFailedClass_);
}

//
// loadThrowable: loads java.lang.Throwable and some of its methods.
//
LmResult
LmJavaExceptionReporter::loadThrowable(ComDiagsArea *diagsArea)
{
  JNIEnv *jni = (JNIEnv *) jniEnv_;
  jclass jc = NULL;

  jc = (jclass) jni->FindClass("java/lang/Throwable");
  if (jc != NULL)
  {
    throwableClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (throwableClass_ != NULL)
    {
      throwableToStringId_ = jni->GetMethodID((jclass) throwableClass_,
                                              "toString",
                                              "()Ljava/lang/String;");
      throwableGetCauseId_ = jni->GetMethodID((jclass) throwableClass_,
                                              "getCause",
                                              "()Ljava/lang/Throwable;");
     }
  }

  // *** we tolerate throwableGetCauseId_ being NULL because
  // it is a new feature in JDK 1.4. Need to change this when
  // we move to JDK1.4
  if (throwableClass_ == NULL || throwableToStringId_ == NULL)
  {
    *diagsArea << DgSqlCode(-LME_JVM_SYS_CLASS_ERROR)
               << DgString0("java.lang.Throwable");
    return LM_ERR;
  }

  // We may get to this point with a pending exception in the JVM if
  // the getCause() method was not found. We will tolerate that
  // condition, clear the pending exception, and proceed.
  jni->ExceptionClear();
  return LM_OK;
}

//
// loadSQLException: Loads java.sql.SQLException
//
LmResult
LmJavaExceptionReporter::loadSQLException(ComDiagsArea *diagsArea)
{
  jclass jc = NULL;
  JNIEnv *jni = (JNIEnv *) jniEnv_;

  jc = (jclass) jni->FindClass("java/sql/SQLException");
  if (jc != NULL)
  {
    exSQLClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (exSQLClass_ != NULL)
    {
      exSQLStateId_ = jni->GetMethodID((jclass) exSQLClass_,
                                       "getSQLState",
                                       "()Ljava/lang/String;");
      exErrorCodeId_ = jni->GetMethodID((jclass) exSQLClass_,
                                        "getErrorCode", "()I");
      exNextExceptionId_ = jni->GetMethodID((jclass) exSQLClass_,
                                            "getNextException",
                                            "()Ljava/sql/SQLException;");
    }
  }

  if (exSQLClass_ == NULL    || exSQLStateId_ == NULL      ||
      exErrorCodeId_ == NULL || exNextExceptionId_ == NULL ||
      jni->ExceptionOccurred())
  {
    insertDiags(diagsArea, -LME_JVM_SYS_CLASS_ERROR, "java.sql.SQLException");
    return LM_ERR;
  }

  return LM_OK;
}

//
// loadMethodValidationFailedException: Loads org.trafodion.sql.udr
//  MethodValidationFailedException class
//
LmResult
LmJavaExceptionReporter::loadMethodValidationFailedException(
  ComDiagsArea *diagsArea)
{
  jclass jc = NULL;
  JNIEnv *jni = (JNIEnv *) jniEnv_;

  jc = (jclass)
        jni->FindClass("org/trafodion/sql/udr/MethodValidationFailedException");

  if (jc != NULL)
  {
    exMetValFailedClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (exMetValFailedClass_ != NULL)
    {
      exGetMethodName_ = jni->GetMethodID((jclass) exMetValFailedClass_,
                                          "getMethodName",
                                          "()Ljava/lang/String;");
      exGetSignature_ = jni->GetMethodID((jclass) exMetValFailedClass_,
                                         "getMethodSignature",
                                         "()Ljava/lang/String;");
    }
  }

  if (exGetMethodName_ == NULL || exGetSignature_ == NULL  ||
      jni->ExceptionOccurred())
  {
    insertDiags(diagsArea,
                -LME_JVM_SYS_CLASS_ERROR,
                "org.trafodion.sql.udr.MethodValidationFailedException");
    return LM_ERR;
  }

  return LM_OK;
}

//
// checkJVMException(): Raises an error for JVM's exception
// if there is any and adds a condition for each JVM exception.
// If 'exception' parameter is not NULL that will be used as exception.
//
// Returns: LM_OK  if there is no exception
//          LM_ERR if there is an exception
//
LmResult
LmJavaExceptionReporter::checkJVMException(ComDiagsArea *da,
                                           LmHandle exception)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jthrowable jex = NULL;
  LmResult result = LM_OK;

  // This boolean will track whether this method owns a reference to
  // the current exception object, or whether the caller owns the
  // reference. This method must release any of its own references
  // before it returns.
  NABoolean callerOwnsTheCurrentException;

  if (exception == NULL)
  {
    jex = jni->ExceptionOccurred();
    callerOwnsTheCurrentException = FALSE;
  }
  else
  {
    jex = (jthrowable) exception;
    callerOwnsTheCurrentException = TRUE;
  }

  if (jex != NULL)
  {
    result = LM_ERR;
  }

  //
  // ExceptionDescribe() prints the exception information including
  // the stack trace to standard error. We want to do this so that
  // user can see the stack. But right now we see two errors if we
  // enable this
  // 1. ":" missing in exception message java.lang.NullPointerException
  //     TEST501
  // 2. 11229 error with message "Exception in thread "main" "
  // So we are commenting calls to ExceptionDescribe().
  //
  // jni->ExceptionDescribe();
  jni->ExceptionClear();

  // Each iteration of this while loop will add a diags condition for
  // the current exception jex, then move jex to point to the next
  // exception in the chain.
  while (jex != NULL)
  {
    addJavaExceptionToDiags(jex, *da);
    jthrowable next = (jthrowable) getNextChainedException(jex);

    if (!callerOwnsTheCurrentException)
    {
      jni->DeleteLocalRef(jex);
    }

    jex = next;
    callerOwnsTheCurrentException = FALSE;
  }

  jni->ExceptionClear();
  return result;
}


//
// checkNewObjectExceptions(): checks for exceptions after a JNI
// call to create a new Java Object, populate diags with exception
// messages.
//
// Returns  LM_OK  if the jobj is not NULL and there is no
//                 exception occurred
//          LM_ERR otherwise
//
LmResult
LmJavaExceptionReporter::checkNewObjectExceptions(LmHandle jobj,
                                                  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jthrowable jt = jni->ExceptionOccurred();
  // jni->ExceptionDescribe();
  jni->ExceptionClear();

  if(jt == NULL)
  {
    if(jobj)
    {
      return LM_OK;
    }
    else
    {
      *da << DgSqlCode(-LME_INTERNAL_ERROR)
          << DgString0(": a JNI error was encountered but no exception was found in the Java Virtual machine.");
      return LM_ERR;
    }
  }
  else
  {
    jstring jstr =
      (jstring) jni->CallObjectMethod(jt, (jmethodID) throwableToStringId_);
    if (jstr)
    {
      const char *msg = jni->GetStringUTFChars(jstr, NULL);
      if ((str_cmp(msg, "java.lang.OutOfMemoryError", 28) == 0))
      {
        *da << DgSqlCode(-LME_JVM_OUT_OF_MEMORY)
            << DgString0(msg);
      }
      else
      {
        *da << DgSqlCode(-LME_JAVA_EXCEPTION)
            << DgString0(msg);
      }

      jni->ReleaseStringUTFChars(jstr, msg);
      jni->DeleteLocalRef(jstr);
    }
    else
    {
      *da << DgSqlCode(-LME_JAVA_EXCEPTION);
    }

    jthrowable next = (jthrowable) getNextChainedException(jt);
    if (next)
    {
      checkJVMException(da, next);
      jni->DeleteLocalRef(next);
    }

    jni->DeleteLocalRef(jt);
    jni->ExceptionClear();
    return LM_ERR;
  }
}


//
// addJavaExceptionToDiags(): Adds a single diags condition
// describing an uncaught Java exception.
//
// Returns  Nothing
//
void
LmJavaExceptionReporter::addJavaExceptionToDiags(LmHandle throwable,
                                                 ComDiagsArea &diags)
{
  JNIEnv *jni = (JNIEnv *) jniEnv_;
  jthrowable t = (jthrowable) throwable;
  const char *msg;

  if (t)
  {
    if (!throwableToStringId_)
    {
      // We should never get here. For some reason the Throwable class
      // and its methods have not been loaded yet.
      msg = ": A Java exception was thrown but the Language Manager was unable to retrieve the message text.";
      diags << DgSqlCode(-LME_INTERNAL_ERROR) << DgString0(msg);
    }
    else
    {
      jstring jstr =
        (jstring) jni->CallObjectMethod(t, (jmethodID) throwableToStringId_);

      if (jstr != NULL)
      {
        msg = jni->GetStringUTFChars(jstr, NULL);
        diags << DgSqlCode(-LME_JVM_EXCEPTION) << DgString0(msg);
        jni->ReleaseStringUTFChars(jstr, msg);
        jni->DeleteLocalRef(jstr);
      }
      else
      {
        // An exception occurred but it contains no message
        msg = ": A Java exception with no message text was thrown.";
        diags << DgSqlCode(-LME_INTERNAL_ERROR) << DgString0(msg);
      }
    }
  }

  jni->ExceptionClear();
}


//
// getNextChainedException(): Return a reference to the
// next chained Java exception. This method attempts to
// avoid method lookups by name if possible, using cached
// class references and method IDs if they have non-NULL values.
//
// Returns LmHandle to next exception
//         NULL if there is no next exception
//
LmHandle
LmJavaExceptionReporter::getNextChainedException(LmHandle throwable)
{
  if (throwable == NULL)
  {
    return NULL;
  }

  JNIEnv *jni = (JNIEnv *) jniEnv_;
  LmHandle result = NULL;
  jthrowable t = (jthrowable) throwable;
  jmethodID methodID;

  jclass classRef = jni->GetObjectClass((jobject) t);
  jni->ExceptionClear();

  if (classRef)
  {
    // 1. Is this a SQLException?
    if (exSQLClass_ != NULL &&
        jni->IsInstanceOf(t, (jclass) exSQLClass_) == JNI_TRUE)
    {
      methodID = (jmethodID) exNextExceptionId_;
    }
    else
    {
      methodID = (jmethodID) jni->GetMethodID(classRef, "getNextException",
                                              "()Ljava/sql/SQLException;");
    }
    jni->ExceptionClear();

    // 2. If not, does this object have a getCause() method?
    //    The getCause() chaining framework is new in JDK 1.4
    if (methodID == NULL)
    {
      if (throwableClass_ != NULL)
      {
        methodID = (jmethodID) throwableGetCauseId_;
      }
      else
      {
        methodID = (jmethodID) jni->GetMethodID(classRef, "getCause",
                                                "()Ljava/lang/Throwable;");
      }
      jni->ExceptionClear();
    }

    // 3. If not, does the object happen to have a getException() method?
    //    Some Throwable subclasses supported this method for exception
    //    chaining before the getCause() framework arrived in JDK 1.4.
    if (methodID == NULL)
    {
      methodID = (jmethodID) jni->GetMethodID(classRef, "getException",
                                              "()Ljava/lang/Throwable;");
      jni->ExceptionClear();
    }

    if (methodID)
    {
      result = (LmHandle) jni->CallObjectMethod(t, methodID);
      jni->ExceptionClear();
    }

    jni->DeleteLocalRef(classRef);

  } // if (classRef)

  jni->ExceptionClear();
  return result;
}

//
// insertDiags() : Inserts conditions into diags area. 
// This method works only with error messages that take string
// parameters. Typically this method is called when the calling
// method wants to insert a condition as main SQLCode and one
// condition for every exception in the exception chain. Currently
// this method can accept at most 2 params to the condition item.
// This method calls checkJVMException() to insert conditions
// for JVM exceptions.
//
// Returns: LM_ERR unconditionally
//
LmResult
LmJavaExceptionReporter::insertDiags(ComDiagsArea *diags,
                                     Int32 errCode,
                                     const char *arg1,
                                     const char *arg2,
                                     LmHandle jt)
{
  Int32 numArgs;

  numArgs = (arg2 == NULL)? ((arg1 == NULL)? 0 : 1) : 2;

  // This is mainSQLCode in the diags area
  switch (numArgs)
  {
    case 0:
    {
      *diags << DgSqlCode(errCode);
       break;
    }

    case 1:
    {
      *diags << DgSqlCode(errCode)
             << DgString0(arg1);
       break;
    }

    case 2:
    {
      *diags << DgSqlCode(errCode)
             << DgString0(arg1)
             << DgString1(arg2);
      break;
    }
  }

  // Insert a diags condition for every exception in exception chain.
  checkJVMException(diags, jt);

  return LM_ERR;
}


//
// checkGetMethodExceptions() : checks for possible exceptions after
// a call to jni GetMethodID(). GetMethodID() can throw
// NoSuchMethodError, ExceptionInInitializerError, or OutOfMemoryError.
//
LmResult
LmJavaExceptionReporter::checkGetMethodExceptions(const char   *routineName,
                                                  const char   *className,
                                                  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jthrowable jt;

  if ((jt = jni->ExceptionOccurred()) == NULL)
  {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": A JNI error was encountered but no exception was found in the Java Virtual Machine.");
     return LM_ERR;
  }

  // jni->ExceptionDescribe();
  jni->ExceptionClear();

  jstring jstr = (jstring)
    jni->CallObjectMethod(jt, (jmethodID) throwableToStringId_);

  if (jstr != NULL)
  {
    const char *msg = jni->GetStringUTFChars(jstr, NULL);

    if ((str_cmp(msg, "java.lang.ExceptionInInitializerError", 37)) == 0)
    {
      insertDiags(da, -LME_CLASS_INIT_FAIL, className, NULL, jt);
    }
    else if ((str_cmp(msg, "java.lang.OutOfMemoryError", 28) == 0))
    {
      insertDiags(da, -LME_JVM_OUT_OF_MEMORY, NULL, NULL, jt);
    }
    else
    {
      insertDiags(da, -LME_ROUTINE_NOT_FOUND, routineName, className, jt);
    }

    jni->ReleaseStringUTFChars(jstr, msg);
    jni->DeleteLocalRef(jstr);
  }
  else
  {
    // Exception occurred, but no message
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": Unknown error occurred.");
  }

  jni->DeleteLocalRef(jt);
  jni->ExceptionClear();
  return LM_ERR;
}

//
// processUserException(): Processes possible uncaught Java exceptions.
// If no exception occurred, returns OK. In the event of an exception,
// diagsArea is filled with proper error message.
//
// The SQLSTATE value is set according to section 13 'Status Code' of JRT.
//
// 38000 - For standard java exceptions.
// 38??? - For SQL exceptions(java.sql.Exception) when Class = 38
//         and Subclass != 000. So valid SQLSTATE values from Java method
//         are between 38001 to 38999
// 39001 - For all other SQL exceptions(java.sql.Exception) ie. for
//         SQL exceptions with invalid SQLSTATE value
//
LmResult
LmJavaExceptionReporter::processUserException(LmRoutineJava *routine_handle,
                                              ComDiagsArea *da)
{
  jthrowable jex;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  char errText[LMJ_ERR_SIZE_512];

  // Get Java exception if one occurred.
  if ((jex = jni->ExceptionOccurred()) == NULL)
    return LM_OK;

  // jni->ExceptionDescribe();
  jni->ExceptionClear();

  jstring jstr = (jstring)
    jni->CallObjectMethod(jex, (jmethodID) throwableToStringId_);

  if (jstr != NULL)
  {
    // Record exception error text.
    const char *msg = jni->GetStringUTFChars(jstr, NULL);

    Int32 len = min(str_len(msg), (LMJ_ERR_SIZE_512 - 1));
    str_cpy_all(errText, msg, len);
    errText[len] = '\0';

    if (str_cmp(errText, "java.sql", 8) == 0 ||
        str_cmp(errText, "com.hp.jdbc.HPT4Exception", 25) == 0)
    {
      reportUserSQLException(jex, errText, da);
    }
    else if (routine_handle->isInternalSPJ())
    {
      reportInternalSPJException(jex, errText, da);
    }
    else
    {
       *da << DgSqlCode(-LME_JAVA_EXCEPTION)
           << DgString0(errText);
    }

    jni->ReleaseStringUTFChars(jstr, msg);
    jni->DeleteLocalRef(jstr);
  }

  // Now insert the remaining error messages into diags
  jthrowable next = (jthrowable) getNextChainedException(jex);
  if (next)
  {
    checkJVMException(da, next);
    jni->DeleteLocalRef(next);
  }

  jni->DeleteLocalRef(jex);
  jni->ExceptionClear();

  return LM_ERR;
}

//
// reportUserSQLException(): populates the diags for SQLException.
// Look at the comments of processUserException().
//
void
LmJavaExceptionReporter::reportUserSQLException(LmHandle jt,
                                                char *errText,
                                                ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jthrowable jex = (jthrowable) jt;
  char sqlState[6];

  // Get the error code, SQLState
  Int32 errcode = (Int32) jni->CallIntMethod((jobject)jex,
                                         (jmethodID)exErrorCodeId_);
  jstring jsstate = (jstring)
                    jni->CallObjectMethod(jex, (jmethodID)exSQLStateId_);
  if (jsstate != NULL)
  {
    const char *sstate = jni->GetStringUTFChars(jsstate, NULL);
    // Check for the validity of the SQLSTATE
    if (sstate != NULL &&
        str_len(sstate) >= 5 &&
        str_cmp(&sstate[0],  "38", 2) == 0 &&
        str_cmp(&sstate[2], "000", 3) != 0)
    {
      // Valid sqlstate. Report this as LME_CUSTOM_ERROR
      // with sqlState as SQLSTATE of LME_CUSTOM_ERROR.
      str_cpy_all(sqlState, sstate, 5);
      sqlState[5] = '\0';
      *da << DgSqlCode(-LME_CUSTOM_ERROR)
          << DgString0(errText)
          << DgString1(sqlState)
          << DgCustomSQLState(sqlState);
    }
    else
    {
      // Invalid sqlstate. Report this value wrapped in the message
      // of LME_JAVA_SQL_EXCEPTION_INVALID. The sqlstate of the reported
      // message is 39001.
      Int32 min_len = min(str_len(sstate), 5); // interested only upto 5 chars
      str_cpy_all(sqlState, sstate, min_len);
      sqlState[min_len] = '\0';
      *da << DgSqlCode(-LME_JAVA_SQL_EXCEPTION_INVALID)
          << DgInt0(errcode)
          << DgString0(sqlState)
          << DgString1(errText);
    }

    if (sstate != NULL)
      jni->ReleaseStringUTFChars(jsstate, sstate);
    jni->DeleteLocalRef(jsstate);
  } // if (jsstate != NULL)
  else
  {
    // sqlstate is not specified.
    // LME_JAVA_SQL_EXCEPTION_INVALID is reported
    sqlState[0] = '\0';
    *da << DgSqlCode(-LME_JAVA_SQL_EXCEPTION_INVALID)
        << DgInt0(errcode)
        << DgString0(sqlState)
        << DgString1(errText);
  }

  return;
}

//
// reportInternalSPJException(): populates the diags for
// MethodValidationException. This exception is thrown by
// the internal SPJ VALIDATEROUTINE.
//
void
LmJavaExceptionReporter::reportInternalSPJException(LmHandle jt,
                                                    char *errText,
                                                    ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jthrowable jex = (jthrowable) jt;

  jstring metNameStr = NULL;
  jstring metSigStr = NULL;
 
  if (exMetValFailedClass_ != NULL &&
      jni->IsInstanceOf(jex, (jclass) exMetValFailedClass_))
  {
    metNameStr = (jstring)
      jni->CallObjectMethod(jex, (jmethodID) exGetMethodName_);
    metSigStr = (jstring)
      jni->CallObjectMethod(jex, (jmethodID) exGetSignature_);
  }

  if (metNameStr != NULL && metSigStr != NULL)
  {
    const char *metName = jni->GetStringUTFChars(metNameStr, NULL);
    const char *metSig = jni->GetStringUTFChars(metSigStr, NULL);

    LmJavaSignature lmSig(metSig, collHeap());
    ComSInt32 unpackedSigSize = lmSig.getUnpackedSignatureSize();
    ComUInt32 totLen = str_len(metName) + unpackedSigSize;
    char *signature = new (collHeap()) char[totLen + 1];

    sprintf(signature, "%s", metName);
    lmSig.unpackSignature(signature + str_len(metName));
    signature[totLen] = '\0';

    *da << DgSqlCode(-LME_VALIDATION_FAILED)
        << DgString0(signature)
        << DgString1(errText);

    jni->ReleaseStringUTFChars(metNameStr, metName);
    jni->DeleteLocalRef(metNameStr);
    jni->ReleaseStringUTFChars(metSigStr, metSig);
    jni->DeleteLocalRef(metSigStr);

    if (signature) NADELETEBASIC(signature, collHeap());

  }
  else
  {
    *da << DgSqlCode(-LME_JAVA_EXCEPTION)
        << DgString0(errText);

    if (metNameStr) jni->DeleteLocalRef(metNameStr);
    if (metSigStr) jni->DeleteLocalRef(metSigStr);
  }

}

//
// reportJavaObjException(): populates the diags for
// a return status returned in an LmUDRObjMethodInvoke.ReturnInfo
// object used in the Java object interface
void
LmJavaExceptionReporter::processJavaObjException(
     LmHandle returnInfoObj,
     int returnStatus,
     int callPhase,
     const char *errText,
     const char *udrName,
     ComDiagsArea *da)
{
  if (da)
    {
      JNIEnv *jni = (JNIEnv*)jniEnv_;
      const char *sqlState = NULL;
      const char *message = NULL;
      jobject jniResult = (jobject) returnInfoObj;
      jstring returnedSQLState =
        static_cast<jstring>(jni->GetObjectField(
           jniResult, 
           (jfieldID) langMan_->getReturnInfoSQLStateField()));
      jstring returnedMessage =
        static_cast<jstring>(jni->GetObjectField(
           jniResult, 
           (jfieldID) langMan_->getReturnInfoMessageField()));

      if (returnedSQLState != NULL)
        sqlState = jni->GetStringUTFChars(returnedSQLState, NULL);
      if (returnedMessage != NULL)
        message = jni->GetStringUTFChars(returnedMessage, NULL);

      if (sqlState || message)
        {
          const char *diagsMessage =
            (message ? message : "no message provided");
          const char *diagsSQLState =
            (sqlState ? sqlState : "no SQLSTATE provided");

          // Check the returned SQLSTATE value and raise appropriate
          // SQL code. Valid SQLSTATE values begin with "38" except "38000"
          if (sqlState &&
              (strncmp(diagsSQLState, "38", 2) == 0) &&
              (strncmp(diagsSQLState, "38000", 5) != 0))
            {
              *da << DgSqlCode(-LME_CUSTOM_ERROR)
                  << DgString0(diagsMessage)
                  << DgString1(diagsSQLState);
              *da << DgCustomSQLState(diagsSQLState);
            }
          else
            {
              *da << DgSqlCode(-LME_UDF_ERROR)
                  << DgString0(udrName)
                  << DgString1(diagsSQLState)
                  << DgString2(diagsMessage);
            }
        }
      else
        {
          // Report the return status as an internal error, since
          // we didn't get a UDRException. This should be rare, since
          // Java exceptions are caught above.
          char buf1[4];
          snprintf(buf1, sizeof(buf1), "%d", callPhase);
          char buf2[80];
          snprintf(buf2, sizeof(buf2), "Return status %d from JNI call", returnStatus);

          *da << DgSqlCode(-LME_OBJECT_INTERFACE_ERROR)
              << DgString0(udrName)
              << DgString1(buf1)
              << DgString2(errText)
              << DgString3(buf2);
        }
      if (sqlState)
        jni->ReleaseStringUTFChars(returnedSQLState, sqlState);
      if (message)
        jni->ReleaseStringUTFChars(returnedMessage, message);
      if (returnedSQLState)
        jni->DeleteLocalRef(returnedSQLState);
      if (returnedMessage)
        jni->DeleteLocalRef(returnedMessage);
    }
}
