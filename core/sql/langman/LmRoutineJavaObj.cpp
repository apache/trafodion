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

#include "LmRoutineJavaObj.h"

LmResult LmRoutineJavaObj::invokeRoutineMethod(
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
     /* IN/OUT */ ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;
  jbyteArray jii = jni->NewByteArray((jint) invocationInfoLen);
  jbyteArray jpi = jni->NewByteArray((jint) planInfoLen);

  jni->SetByteArrayRegion(jii,
                          (jsize) 0,
                          (jsize) invocationInfoLen,
                          (const jbyte *) serializedInvocationInfo);
  jni->SetByteArrayRegion(jpi,
                          (jsize) 0,
                          (jsize) planInfoLen,
                          (const jbyte *) serializedPlanInfo);

  jobject result = jni->CallObjectMethod(
       udrInvokeObject_,
       (jmethodID) getLM()->routineMethodInvokeId_,
       (jint) phase,
       jii,
       jpi,
       (jint) planNum);

  if (result == NULL || jni->ExceptionOccurred())
    {
      getLM()->exceptionReporter_->checkJVMException(da, 0);
      return LM_ERR;
    }

  // retrieve returned information
  jint returnStatus = jni->GetIntField(
       result, 
       (jfieldID) getLM()->returnInfoStatusField_);

  jbyteArray returnedII = static_cast<jbyteArray>(
       jni->GetObjectField(
            result,
            (jfieldID) getLM()->returnInfoRIIField_));
  jbyteArray returnedPI = static_cast<jbyteArray>(
       jni->GetObjectField(
            result,
            (jfieldID) getLM()->returnInfoRPIField_));

  jbyte * returnedIIChar = NULL;
  jbyte * returnedPIChar = NULL;
  jsize iiLen = 0;
  jsize piLen = 0;

  if (returnedIIChar)
    {
      returnedIIChar = jni->GetByteArrayElements(returnedII, NULL);
      iiLen = jni->GetArrayLength(returnedII);
    }

  if (returnedPI)
    {
      returnedPIChar = jni->GetByteArrayElements(returnedPI, NULL);
      piLen = jni->GetArrayLength(returnedPI);
    }

  if (returnedII)
    jni->ReleaseByteArrayElements(returnedII, returnedIIChar, JNI_ABORT);

  if (returnedPI)
    jni->ReleaseByteArrayElements(returnedPI, returnedPIChar, JNI_ABORT);

  if (returnStatus != 0)
    return LM_ERR;

  return LM_OK;
}

LmResult LmRoutineJavaObj::getRoutineInvocationInfo(
     /* IN/OUT */ char         *serializedInvocationInfo,
     /* IN */     Int32         invocationInfoMaxLen,
     /* OUT */    Int32        *invocationInfoLenOut,
     /* IN/OUT */ char         *serializedPlanInfo,
     /* IN */     Int32         planInfoMaxLen,
     /* IN */     Int32         planNum,
     /* OUT */    Int32        *planInfoLenOut,
     /* IN/OUT */ ComDiagsArea *da)
{
  return LM_ERR;
}

LmRoutineJavaObj::LmRoutineJavaObj(
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
     ComDiagsArea *diagsArea) : LmRoutineJava(sqlName,
                                              externalName,
                                              librarySqlName,
                                              0,
                                              NULL,
                                              0,
                                              NULL,
                                              COM_STYLE_JAVA_OBJ,
                                              transactionAttrs,
                                              sqlAccessMode,
                                              externalSecurity,
                                              routineOwnerId,
                                              NULL,
                                              0,
                                              0,
                                              NULL,
                                              NULL,
                                              NULL,
                                              lm,
                                              (LmHandle) NULL,
                                              container,
                                              diagsArea),
                                udrObject_(NULL),
                                udrInvokeObject_(NULL)
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;
  jbyteArray jii = jni->NewByteArray((jint) invocationInfoLen);
  jbyteArray jpi = NULL;
  jobject udrInvokeObject = NULL;

  jni->SetByteArrayRegion(jii,
                          (jsize) 0,
                          (jsize) invocationInfoLen,
                          (const jbyte *) serializedInvocationInfo);

  if (planInfoLen > 0)
    {
      jpi = jni->NewByteArray((jint) planInfoLen);

      jni->SetByteArrayRegion(jpi,
                              (jsize) 0,
                              (jsize) planInfoLen,
                              (const jbyte *) serializedPlanInfo);
    }

  if (!jni->ExceptionOccurred())
    // Do a JNI call to allocate an LmUDRObjMethodInvoke object
    udrInvokeObject = jni->CallStaticObjectMethod(
         (jclass)    getLM()->lmObjMethodInvokeClass_,
         (jmethodID) getLM()->makeNewObjId_,
         udrObject,
         jii,
         jpi);

  if (udrInvokeObject != NULL && !jni->ExceptionOccurred())
    {
      // create global references for these objects, they will
      // last as long as the LmRoutineJavaObj
      udrObject_ = jni->NewGlobalRef(udrObject);
      udrInvokeObject_ = jni->NewGlobalRef(udrInvokeObject);
    }

  if (udrObject_ == NULL || udrInvokeObject_ == NULL || jni->ExceptionOccurred())
    {
      getLM()->exceptionReporter_->insertDiags(
           diagsArea,
           -LME_INTERNAL_ERROR,
           ": Error in LmRoutineJavaObj::LmRoutineJavaObj()");
    }
}

LmRoutineJavaObj::~LmRoutineJavaObj()
{
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;

  if (udrObject_ != NULL)
    jni->DeleteGlobalRef(udrObject_);
  if (udrInvokeObject_ != NULL)
    jni->DeleteGlobalRef(udrInvokeObject_);
}

ComBoolean LmRoutineJavaObj::isValid() const
{
  return udrObject_ != NULL && udrInvokeObject_ != NULL;
}
