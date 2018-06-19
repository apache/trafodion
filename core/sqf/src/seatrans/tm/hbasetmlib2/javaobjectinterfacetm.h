// **********************************************************************
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
// **********************************************************************
#ifndef JAVAOBJECTINTERFACETM_H
#define JAVAOBJECTINTERFACETM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <iostream>

#include "jni.h"
#include "dtm/tm_util.h"

extern __thread JNIEnv* _tlp_jenv;
extern __thread std::string *_tlp_error_msg;

// This structure defines the information needed for each java method used.
struct JavaMethodInit {
    std::string   jm_name;       // The method name.
    std::string   jm_signature;  // The method signature.
    jmethodID     methodID;      // The JNI methodID
  };

typedef enum {
  JOI_OK = 0
 ,JOI_ERROR_CHECK_JVM           // Cannot check existing JVMs
 ,JOI_ERROR_JVM_VERSION         // Attaching to JVM of wrong version.
 ,JOI_ERROR_ATTACH_JVM          // Cannot attach to an existing JVM
 ,JOI_ERROR_CREATE_JVM          // Cannot create JVM
 ,JOI_ERROR_FINDCLASS           // JNI FindClass() failed
 ,JOI_ERROR_GETMETHOD           // JNI GetMethodID() failed
 ,JOI_ERROR_NEWOBJ              // JNI NewObject() failed
 ,JOI_ERROR_INIT_JNI            // initJNIEnv failed
 ,JOI_LAST
} JOI_RetCode;

// ===========================================================================
// ===== The JavaObjectInterfaceTM class defines an interface for using Java 
// ===== objects.
// ===== For each Java class, a new subclass of JavaObjectInterfaceTM should 
// ===== be created.
// ===========================================================================
class JavaObjectInterfaceTM
{
protected:

  // Default constructor - for creating a new JVM		
  JavaObjectInterfaceTM(int debugPort = 0, int debugTimeout = 0)
    : javaObj_(NULL)
      ,needToDetach_(false)
      ,isInitialized_(false)
      ,isHBaseCompatibilityMode_(true)
      ,debugPort_(debugPort)
      ,debugTimeout_(debugTimeout)
  {}

#if 1 
  // Constructor for reusing an existing JVM.
  JavaObjectInterfaceTM(JavaVM *jvm, JNIEnv *jenv, jobject jObj = NULL)
    : javaObj_(NULL)
      ,needToDetach_(false)
      ,isInitialized_(false)
      ,isHBaseCompatibilityMode_(true)
      ,debugPort_(0)
      ,debugTimeout_(0)
  {
    _tlp_jenv = jenv;

    if(jObj != NULL)
       javaObj_ = _tlp_jenv->NewGlobalRef(jObj);
    else
       javaObj_ = jObj;
  }
#endif

  // Destructor
  virtual ~JavaObjectInterfaceTM();
  
  // Create a new JVM
  int createJVM();
  
  // Create a new JVM for debugging the Java code
  int createJVM4Debug();

  
  // Initialize the JVM.
  JOI_RetCode    initJVM();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  JOI_RetCode    init(char* className, jclass &javaclass,  JavaMethodInit* JavaMethods, int32 howManyMethods, bool methodsInitialized);

  // Get the error description.
  virtual char* getErrorText(JOI_RetCode errEnum);

  JOI_RetCode initJNIEnv();

  void setHBaseCompatibilityMode(bool val)
  {
    isHBaseCompatibilityMode_ = val;
  }

  bool isHBaseCompatibilityMode()
  {
    return isHBaseCompatibilityMode_;
  }

  char* buildClassPath();

public:
  int attachThread();
  int detachThread();

  jobject getJavaObject()
  {
    return javaObj_;
  }
  
  bool isInitialized()
  {
    return isInitialized_;
  }
  bool getExceptionDetails(JNIEnv *jenv, short *errCode = NULL);
  void appendExceptionMessages(JNIEnv *jenv, jthrowable a_exception, std::string &error_msg);
  short getExceptionErrorCode(JNIEnv *jenv, jthrowable a_exception);
  
protected:
  static JavaVM*   jvm_;
  jobject   javaObj_;
  bool      needToDetach_;
  bool      isInitialized_;
  bool      isHBaseCompatibilityMode_;
  int       debugPort_;
  int       debugTimeout_;
  static jclass gThrowableClass;
  static jclass gStackTraceClass;
  static jclass gTransactionManagerExceptionClass;
  static jmethodID gGetStackTraceMethodID;
  static jmethodID gThrowableToStringMethodID;
  static jmethodID gStackFrameToStringMethodID;
  static jmethodID gGetCauseMethodID;
  static jmethodID gGetErrorCodeMethodID;
  static jint jniHandleCapacity_;
};

void set_error_msg(std::string &error_msg); 
void set_error_msg(char *error_msg); 
#endif
