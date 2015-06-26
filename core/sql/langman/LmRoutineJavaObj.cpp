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
  LmResult result = LM_OK;
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

  // call the Java method that will ultimately call the user code
  jobject jniResult = jni->CallObjectMethod(
       udrInvokeObjectG_,
       (jmethodID) getLM()->routineMethodInvokeId_,
       (jint) phase,
       jii,
       jpi,
       (jint) planNum);

  if (jniResult == NULL || jni->ExceptionOccurred())
    {
      getLM()->exceptionReporter_->checkJVMException(da, 0);
      if (da && da->mainSQLCODE() >= 0)
        {
          // an error without a diagnostic message from Java
          char buf[4];
          snprintf(buf, sizeof(buf), "%d", static_cast<int>(phase));
          *da << DgSqlCode(-LME_OBJECT_INTERFACE_ERROR)
              << DgString0(getNameForDiags())
              << DgString0(buf)
              << DgString0("LmRoutineJavaObj::invokeRoutineMethod()")
              << DgString0("JNI call returned a NULL object");
        }
      result = LM_ERR;
    }

  // delete input parameter objects
  jni->DeleteLocalRef(jii);
  jni->DeleteLocalRef(jpi);

  // clear return info from any previous calls
  if (returnedIIL_)
    {
      jni->DeleteLocalRef(returnedIIL_);
      returnedIIL_ = NULL;
    }
  if (returnedPIL_)
    {
      jni->DeleteLocalRef(returnedPIL_);
      returnedPIL_ = NULL;
    }

  iiLen_ = piLen_ = 0;

  if (result == LM_OK)
    {
      // retrieve returned information
      jint returnStatus = jni->GetIntField(
           jniResult, 
           (jfieldID) getLM()->returnInfoStatusField_);

      if (returnStatus != 0)
        {
          if (da)
            {
              // Report the return status. For now this is considered an
              // internal error. User errors should be reported through the
              // Java exception mechanism.
              char buf1[4];
              snprintf(buf1, sizeof(buf1), "%d", static_cast<int>(phase));
              char buf2[80];
              snprintf(buf2, sizeof(buf2), "Return status %d from JNI call", returnStatus);

              *da << DgSqlCode(-LME_OBJECT_INTERFACE_ERROR)
                  << DgString0(getNameForDiags())
                  << DgString0(buf1)
                  << DgString0("LmRoutineJavaObj::invokeRoutineMethod()")
                  << DgString0(buf2);
            }
          result = LM_ERR;
        }
      else
        {
          returnedIIL_ = static_cast<jbyteArray>(
               jni->GetObjectField(
                    jniResult,
                    (jfieldID) getLM()->returnInfoRIIField_));
          returnedPIL_ = static_cast<jbyteArray>(
               jni->GetObjectField(
                    jniResult,
                    (jfieldID) getLM()->returnInfoRPIField_));

          if (returnedIIL_)
            iiLen_ = jni->GetArrayLength(returnedIIL_);

          if (returnedPIL_)
            piLen_ = jni->GetArrayLength(returnedPIL_);
        }
    }

  if (invocationInfoLenOut)
    *invocationInfoLenOut = static_cast<Int32>(iiLen_);

  if (planInfoLenOut)
    *planInfoLenOut = static_cast<Int32>(piLen_);

  return result;
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
  JNIEnv *jni = (JNIEnv*)getLM()->jniEnv_;
  LmResult result = LM_OK;

  // return the serialized invocation info
  if (returnedIIL_)
    {
      if (invocationInfoMaxLen < iiLen_ && serializedInvocationInfo)
        {
          if (da)
            *da << DgSqlCode(-LME_OBJECT_INTERFACE_ERROR)
                << DgString0(getNameForDiags())
                << DgString1("unknown")
                << DgString2("getRoutineInvocationInfo()")
                << DgString3("Buffer too short for invocation info");
          iiLen_ = 0;
          result = LM_ERR;
        }
      else if (serializedInvocationInfo)
        {
          jbyte * returnedIIChar = jni->GetByteArrayElements(returnedIIL_, NULL);
          str_cpy_all(serializedInvocationInfo,
                      reinterpret_cast<const char *>(returnedIIChar),
                      iiLen_);
          jni->ReleaseByteArrayElements(returnedIIL_, returnedIIChar, JNI_ABORT);
        }
      jni->DeleteLocalRef(returnedIIL_);
      returnedIIL_ = NULL;
    }

  if (invocationInfoLenOut)
    *invocationInfoLenOut = iiLen_;
  iiLen_ = 0;

  // return the serialized plan info
  if (returnedPIL_)
    {
      if (planInfoMaxLen < piLen_ && serializedPlanInfo)
        {
          if (da)
            *da << DgSqlCode(-LME_OBJECT_INTERFACE_ERROR)
                << DgString0(getNameForDiags())
                << DgString1("unknown")
                << DgString2("getRoutineInvocationInfo()")
                << DgString3("Buffer too short for plan info");
          piLen_ = 0;
          result = LM_ERR;
        }
      else if (serializedPlanInfo)
        {
          jbyte * returnedPIChar = jni->GetByteArrayElements(returnedPIL_, NULL);
          str_cpy_all(serializedPlanInfo,
                      reinterpret_cast<const char *>(returnedPIChar),
                      piLen_);
          jni->ReleaseByteArrayElements(returnedPIL_, returnedPIChar, JNI_ABORT);
        }
      jni->DeleteLocalRef(returnedPIL_);
      returnedPIL_ = NULL;
    }

  if (planInfoLenOut)
    *planInfoLenOut = piLen_;
  piLen_ = 0;

  return result;
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
                                udrObjectG_(NULL),
                                udrInvokeObjectG_(NULL),
                                returnedIIL_(NULL),
                                returnedPIL_(NULL),
                                iiLen_(0),
                                piLen_(0)

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

  jni->DeleteLocalRef(jii);
  if (planInfoLen > 0)
    jni->DeleteLocalRef(jpi);

  if (udrInvokeObject != NULL && !jni->ExceptionOccurred())
    {
      // create global references for these objects, they will
      // last as long as the LmRoutineJavaObj
      udrObjectG_ = jni->NewGlobalRef(udrObject);
      jni->DeleteLocalRef(udrObject);
      udrInvokeObjectG_ = jni->NewGlobalRef(udrInvokeObject);
      jni->DeleteLocalRef(udrInvokeObject);
    }

  if (udrObjectG_ == NULL || udrInvokeObjectG_ == NULL || jni->ExceptionOccurred())
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

  if (udrObjectG_ != NULL)
    jni->DeleteGlobalRef(udrObjectG_);
  if (udrInvokeObjectG_ != NULL)
    jni->DeleteGlobalRef(udrInvokeObjectG_);
  if (returnedIIL_)
    jni->DeleteLocalRef(returnedIIL_);
  if (returnedPIL_)
    jni->DeleteLocalRef(returnedPIL_);
}

ComBoolean LmRoutineJavaObj::isValid() const
{
  return udrObjectG_ != NULL && udrInvokeObjectG_ != NULL;
}
