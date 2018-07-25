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
#ifndef JNI_H
#define JNI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <iostream>
#include <sys/types.h>
#include <sys/syscall.h>
#include "jni.h"
#include "Platform.h"
#include "NAString.h"

class LmJavaOptions;

#ifndef SEQ_TESTING
#include "ex_god.h"
#endif

extern __thread JNIEnv *jenv_;
extern __thread NAString *tsRecentJMFromJNI;
extern __thread NAString *tsSqlJniErrorStr;

void setSqlJniErrorStr(NAString &errorMsg);
void setSqlJniErrorStr(const char *errorMsg);
const char *getSqlJniErrorStr();

// This structure defines the information needed for each java method used.
struct JavaMethodInit {
    const char *jm_name;       // The method name.
    const char *jm_signature;  // The method signature.
    jmethodID   methodID;      // The JNI methodID
    NAString   *jm_full_name;
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
// ===== The JavaObjectInterface class defines an interface for using Java 
// ===== objects.
// ===== For each Java class, a new subclass of JavaObjectInterface should 
// ===== be created.
// ===========================================================================
class JavaObjectInterface
#ifndef SEQ_TESTING
  : public ExGod
#endif
{
protected:

  // Default constructor - for creating a new JVM		
  JavaObjectInterface(NAHeap *heap)
    :  heap_(heap)
      ,javaObj_(NULL)
      ,isInitialized_(false)
  {
     tid_ = syscall(SYS_gettid);
  }

  // Constructor for reusing an existing JVM.
  JavaObjectInterface(NAHeap *heap, jobject jObj)
    :  heap_(heap)
      ,javaObj_(NULL)
      ,isInitialized_(false)
  {
    tid_ = syscall(SYS_gettid);
    // When jObj is not null in the constructor
    // it is assumed that the object is created on the Java side and hence
    // just create a Global Reference in the JNI side
    if (jObj != NULL && (long)jObj != -1)
       javaObj_ = jenv_->NewGlobalRef(jObj);
    else
       javaObj_ = jObj;
  }

  // Destructor
  virtual ~JavaObjectInterface();
  
  // Create a new JVM
  static int createJVM(LmJavaOptions *options);
  
  // Initialize the JVM.
  static JOI_RetCode    initJVM(LmJavaOptions *options = NULL);
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  JOI_RetCode    init(char *className, jclass &javaclass, JavaMethodInit* JavaMethods, Int32 howManyMethods, bool methodsInitialized);

  // Get the error description.
  static char* getErrorText(JOI_RetCode errEnum);
 
  static const char *getLastError() 
    { return getSqlJniErrorStr(); }

  // Write the description of a Java error to the log file.
  static void logError(std::string &cat, const char* methodName, const char *result);
  static void logError(std::string &cat, const char* methodName, jstring jresult);
  static void logError(std::string &cat, const char* file, int line);

  static JOI_RetCode initJNIEnv();
  static char* buildClassPath();  
  
public:
  void setJavaObject(jobject jobj);
  jobject getJavaObject()
  {
    return javaObj_;
  }
  pid_t getTid()
  {
     return tid_;
  } 
  bool isInitialized()
  {
    return isInitialized_;
  }
  static NABoolean getExceptionDetails(const char *fileName, int lineNo,
                                       const char *methodName, 
                                       NABoolean noDetails = FALSE);

  static NABoolean appendExceptionMessages(jthrowable a_exception, 
                                           NAString &error_msg,
                                           NABoolean noDetails = FALSE);
  
  NAHeap *getHeap() { return heap_; }
protected:
  static JavaVM*   jvm_;
  static jclass gThrowableClass;
  static jclass gStackTraceClass;
  static jclass gOOMErrorClass;
  static jmethodID gGetStackTraceMethodID;
  static jmethodID gThrowableToStringMethodID;
  static jmethodID gStackFrameToStringMethodID;
  static jmethodID gGetCauseMethodID;
  static jint jniHandleCapacity_;

  jobject   javaObj_;
  bool      isInitialized_;
  static int debugPort_;
  static int debugTimeout_;
  pid_t     tid_;
  NAHeap    *heap_;
};

#endif
