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

#include "seabed/ms.h"
#include "tmlogging.h"
#include "javaobjectinterfacetm.h"

// ===========================================================================
// ===== Class JavaObjectInterfaceTM
// ===========================================================================

JavaVM* JavaObjectInterfaceTM::jvm_  = NULL;
int JavaObjectInterfaceTM::jniHandleCapacity_ = 0;

#define DEFAULT_MAX_TM_HEAP_SIZE "2048" 
#define USE_JVM_DEFAULT_MAX_HEAP_SIZE 0
#define TRAF_DEFAULT_JNIHANDLE_CAPACITY 32
#define DEFAULT_COMPRESSED_CLASSSPACE_SIZE 128
#define DEFAULT_MAX_METASPACE_SIZE 128

  
static const char* const joiErrorEnumStr[] = 
{
  "All is well."
 ,"Error checking for existing JVMs"
 ,"Error attaching to a JVM of the wrong version"
 ,"Error attaching to an existing JVM"
 ,"Error creating a new JVM"
 ,"JNI FindClass() failed"
 ,"JNI GetMethodID() failed"
 ,"JNI NewObject() failed"
 ,"initJNIEnv() failed"
 ,"Error Unknown"
};

__thread JNIEnv* _tlp_jenv = 0;
__thread bool  _tlv_jenv_set = false;
__thread std::string *_tlp_error_msg = NULL;

jclass JavaObjectInterfaceTM::gThrowableClass = NULL;
jclass JavaObjectInterfaceTM::gStackTraceClass = NULL;
jclass JavaObjectInterfaceTM::gTransactionManagerExceptionClass = NULL;
jmethodID JavaObjectInterfaceTM::gGetStackTraceMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gThrowableToStringMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gStackFrameToStringMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gGetCauseMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gGetErrorCodeMethodID = NULL;

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* JavaObjectInterfaceTM::getErrorText(JOI_RetCode errEnum)
{
   if (errEnum >= JOI_LAST) {
      abort();
   }
   else
      return (char*)joiErrorEnumStr[errEnum];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
JavaObjectInterfaceTM::~JavaObjectInterfaceTM()
{
   if (_tlp_jenv && javaObj_)
      _tlp_jenv->DeleteGlobalRef(javaObj_);
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* JavaObjectInterfaceTM::buildClassPath()
{
  char* classPath = getenv("CLASSPATH");
  int32 size = strlen(classPath) + 1024;
  char* classPathBuffer = (char*)malloc(size);
  
  strcpy(classPathBuffer, "-Djava.class.path=");
  strcat(classPathBuffer, classPath);

  return classPathBuffer;
}

int JavaObjectInterfaceTM::attachThread() {
  
      jint result = jvm_->AttachCurrentThread((void**) &_tlp_jenv, NULL);   
      if (result != JNI_OK)
        return JOI_ERROR_ATTACH_JVM;
      
//      needToDetach_ = true;
      _tlv_jenv_set = true;
      return JNI_OK;
}

int JavaObjectInterfaceTM::detachThread() {
      jint result = jvm_->DetachCurrentThread();   
      if (result != JNI_OK)
        return JOI_ERROR_ATTACH_JVM;

      _tlv_jenv_set = false;
      _tlp_jenv = 0;
      return JNI_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Create a new JVM instance.
//////////////////////////////////////////////////////////////////////////////
int JavaObjectInterfaceTM::createJVM()
{
  JavaVMInitArgs jvm_args;
  JavaVMOption jvm_options[10];

  char* classPathArg = buildClassPath();
  char debugOptions[300];
  int numJVMOptions = 0;

  const char *maxHeapSize = getenv("DTM_JVM_MAX_HEAP_SIZE_MB");  
  char heapOptions[100];  
  int heapSize;  
  if (maxHeapSize == NULL) {  
     maxHeapSize = DEFAULT_MAX_TM_HEAP_SIZE;  
  }  
  heapSize = atoi(maxHeapSize);  
  if (heapSize != USE_JVM_DEFAULT_MAX_HEAP_SIZE) {  
      sprintf(heapOptions, "-Xmx%sm", maxHeapSize);  
      jvm_options[numJVMOptions++].optionString = heapOptions;  
  }  

  jvm_options[numJVMOptions++].optionString = classPathArg;
  jvm_options[numJVMOptions++].optionString = (char *) "-XX:-LoopUnswitching";
  //  jvm_options[numJVMOptions++].optionString = (char *) "-Xcheck:jni";
  int debugPort = 0;
  int my_nid;
  int my_pid;
  static const char *debugPortStr = getenv("JVM_DEBUG_PORT");
  static const char *suspendOnDebug = getenv("JVM_SUSPEND_ON_DEBUG");
  if (debugPortStr != NULL)
     debugPort = atoi(debugPortStr);
  if (debugPort > 0) {
     const char *debugTimeoutStr = getenv("JVM_DEBUG_TIMEOUT");
     if (debugTimeoutStr != NULL)
        debugTimeout_ = atoi(debugTimeoutStr);
     debugPort_ = debugPort;
     msg_mon_get_process_info(NULL, &my_nid, &my_pid);
     // to allow debugging multiple processes at the same time,
     // specify a port that is a multiple of 1000 and the code will
     // add pid mod 1000 to the port number to use
     if (debugPort_ % 1000 == 0)
         debugPort_ += (my_pid % 1000);
  }
  if (debugPort_ > 0)
    {
      sprintf(debugOptions,"-agentlib:jdwp=transport=dt_socket,address=%d,server=y,timeout=%d,suspend=%s",
                                  debugPort_, debugTimeout_, 
                                  (suspendOnDebug != NULL ? "y" : "n"));
      jvm_options[numJVMOptions++].optionString = debugOptions;
    }

  char compressedClassSpaceSizeOptions[64];
  int compressedClassSpaceSize = 0;
  const char *compressedClassSpaceSizeStr = getenv("JVM_COMPRESSED_CLASS_SPACE_SIZE");
  if (compressedClassSpaceSizeStr)
    compressedClassSpaceSize = atoi(compressedClassSpaceSizeStr);
  if (compressedClassSpaceSize <= 0)
     compressedClassSpaceSize = DEFAULT_COMPRESSED_CLASSSPACE_SIZE;
  sprintf(compressedClassSpaceSizeOptions, "-XX:CompressedClassSpaceSize=%dm", compressedClassSpaceSize);
  jvm_options[numJVMOptions].optionString = compressedClassSpaceSizeOptions;
  numJVMOptions++;

  char maxMetaspaceSizeOptions[64];
  int maxMetaspaceSize = 0;
  const char *maxMetaspaceSizeStr = getenv("JVM_MAX_METASPACE_SIZE");
  if (maxMetaspaceSizeStr)
    maxMetaspaceSize = atoi(maxMetaspaceSizeStr);
  if (maxMetaspaceSize <= 0)
     maxMetaspaceSize = DEFAULT_MAX_METASPACE_SIZE;
  sprintf(maxMetaspaceSizeOptions, "-XX:MaxMetaspaceSize=%dm", maxMetaspaceSize);
  jvm_options[numJVMOptions].optionString = maxMetaspaceSizeOptions;
  numJVMOptions++;
  
  jvm_args.version            = JNI_VERSION_1_6;
  jvm_args.options            = jvm_options;
  jvm_args.nOptions           = numJVMOptions;
  jvm_args.ignoreUnrecognized = true;

  int ret = JNI_CreateJavaVM(&jvm_, (void**)&_tlp_jenv, &jvm_args);
  if (ret != 0) {
    abort();
  }
  _tlv_jenv_set = true;
  free(classPathArg);
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// Create a new JVM instance, ready for attaching a debugger.
//////////////////////////////////////////////////////////////////////////////
int JavaObjectInterfaceTM::createJVM4Debug()
{
  JavaVMInitArgs jvm_args;
  JavaVMOption jvm_options[3];

  char* classPathArg = buildClassPath();
  
  jvm_options[0].optionString = classPathArg;
  jvm_options[1].optionString = (char*)"-Xdebug";
  jvm_options[2].optionString = (char*)"-Xrunjdwp:transport=dt_socket,address=8998,server=y";
  
  jvm_args.version            = JNI_VERSION_1_6;
  jvm_args.options            = jvm_options;
  jvm_args.nOptions           = 3;
  jvm_args.ignoreUnrecognized = 1;

  int ret = JNI_CreateJavaVM(&jvm_, (void**)&_tlp_jenv, &jvm_args);
  if (ret != 0) {
    abort();
  }
  _tlv_jenv_set = true;
  free(classPathArg);
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
JOI_RetCode JavaObjectInterfaceTM::initJVM()
{
  jint result;

  if ((_tlp_jenv != 0) && (_tlv_jenv_set)) {
    return JOI_OK;
  }

  if (jvm_ == NULL)
  {
    jsize jvm_count = 0;
    // Is there an existing JVM already created?
    result = JNI_GetCreatedJavaVMs (&jvm_, 1, &jvm_count);
    if (result != JNI_OK) {
      set_error_msg(getErrorText(JOI_ERROR_CHECK_JVM));
      return JOI_ERROR_CHECK_JVM;      
    }
      
    if (jvm_count == 0)
    {
      // No - create a new one.
      result = createJVM();
      if (result != JNI_OK) {
        set_error_msg(getErrorText(JOI_ERROR_CHECK_JVM));
        return JOI_ERROR_CREATE_JVM;
      }
      needToDetach_ = false;
        
    }
    char *jniHandleCapacityStr =  getenv("TRAF_JNIHANDLE_CAPACITY");
    if (jniHandleCapacityStr != NULL)
       jniHandleCapacity_ = atoi(jniHandleCapacityStr);
    if (jniHandleCapacity_ == 0)
        jniHandleCapacity_ = TRAF_DEFAULT_JNIHANDLE_CAPACITY;
  }
  if (_tlp_jenv == NULL) {
  // We found a JVM, can we use it?
  result = jvm_->GetEnv((void**) &_tlp_jenv, JNI_VERSION_1_6);
  switch (result)
  {
    case JNI_OK:
      break;
    
    case JNI_EDETACHED:
      result = jvm_->AttachCurrentThread((void**) &_tlp_jenv, NULL);   
      if (result != JNI_OK)
      {
        set_error_msg(getErrorText(JOI_ERROR_ATTACH_JVM));
        return JOI_ERROR_ATTACH_JVM;
      }
      
      needToDetach_ = true;
      break;
       
    case JNI_EVERSION:
      set_error_msg(getErrorText(JOI_ERROR_JVM_VERSION));
      return JOI_ERROR_JVM_VERSION;
      break;
      
    default:
      set_error_msg(getErrorText(JOI_ERROR_ATTACH_JVM));
      return JOI_ERROR_ATTACH_JVM;
      break;
  }
  _tlv_jenv_set = true;
  }
  jclass lJavaClass;
  if (gThrowableClass == NULL)
  {
     lJavaClass = _tlp_jenv->FindClass("java/lang/Throwable");
     if (lJavaClass != NULL)
     {
        gThrowableClass  = (jclass)_tlp_jenv->NewGlobalRef(lJavaClass);
        _tlp_jenv->DeleteLocalRef(lJavaClass);
        gGetStackTraceMethodID  = _tlp_jenv->GetMethodID(gThrowableClass,
                      "getStackTrace",
                      "()[Ljava/lang/StackTraceElement;");
        gThrowableToStringMethodID = _tlp_jenv->GetMethodID(gThrowableClass,
                      "toString",
                      "()Ljava/lang/String;");
        gGetCauseMethodID = _tlp_jenv->GetMethodID(gThrowableClass,
                      "getCause",
                      "()Ljava/lang/Throwable;");
     }
  }
  if (gStackTraceClass == NULL)
  {
     lJavaClass =  (jclass)_tlp_jenv->FindClass("java/lang/StackTraceElement");
     if (lJavaClass != NULL)
     {
        gStackTraceClass = (jclass)_tlp_jenv->NewGlobalRef(lJavaClass);
        _tlp_jenv->DeleteLocalRef(lJavaClass);
        gStackFrameToStringMethodID  = _tlp_jenv->GetMethodID(gStackTraceClass,
                      "toString",
                      "()Ljava/lang/String;");
     }
  }
  
  if (gTransactionManagerExceptionClass == NULL)
  {
     lJavaClass =  (jclass)_tlp_jenv->FindClass("org/trafodion/dtm/TransactionManagerException");
     if (lJavaClass != NULL)
     {
        gTransactionManagerExceptionClass = (jclass)_tlp_jenv->NewGlobalRef(lJavaClass);
        _tlp_jenv->DeleteLocalRef(lJavaClass);
        gGetErrorCodeMethodID  = _tlp_jenv->GetMethodID(gTransactionManagerExceptionClass,
                        "getErrorCode",
                         "()S");
     }
  } 
  return JOI_OK;
}
 

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
JOI_RetCode JavaObjectInterfaceTM::init(char*           className, 
                                      jclass          &javaClass,
                                      JavaMethodInit* JavaMethods, 
                                      int32           howManyMethods,
                                      bool            methodsInitialized)
{
  if (isInitialized_)
    return JOI_OK;
  
  JOI_RetCode retCode = JOI_OK;
    
  // Make sure the JVM environment is set up correctly.
  retCode = initJVM();
  if (retCode != JOI_OK)
    return retCode;
        
  if (methodsInitialized == false || javaObj_ == NULL)
  {
    // Initialize the class pointer
    jclass lv_javaClass = _tlp_jenv->FindClass(className); 
    if (getExceptionDetails(NULL)) {
      tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"JavaObjectInterfaceTM::init()", (char *)_tlp_error_msg->c_str(), -1LL);
      return JOI_ERROR_FINDCLASS;
    }
    javaClass = (jclass)_tlp_jenv->NewGlobalRef(lv_javaClass);
    _tlp_jenv->DeleteLocalRef(lv_javaClass);
 
    // Initialize the method pointers.
    if (!methodsInitialized)
    {
      for (int i=0; i<howManyMethods; i++)
      {
        JavaMethods[i].methodID = _tlp_jenv->GetMethodID(javaClass, 
                                                     JavaMethods[i].jm_name.data(), 
                                                     JavaMethods[i].jm_signature.data());
        if (getExceptionDetails(NULL)) {
           tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"JNIEnv->GetMethodID()", 
                  (char *)_tlp_error_msg->c_str(), -1LL);
           return JOI_ERROR_GETMETHOD;
        }
      }
    }
    
    if (javaObj_ == NULL)
    {
      // Allocate an object of the Java class, and call its constructor.
      // The constructor must be the first entry in the methods array.
      javaObj_ = _tlp_jenv->NewObject(javaClass, JavaMethods[0].methodID);
      if (getExceptionDetails(NULL)) {
         tm_log_write(DTM_TM_JNI_ERROR, SQ_LOG_ERR, (char *)"JavaObjectInterfaceTM::init()", 
             (char *)_tlp_error_msg->c_str(), -1LL);
         _tlp_jenv->DeleteLocalRef(javaClass);  
         return JOI_ERROR_NEWOBJ;
      }
      javaObj_ = _tlp_jenv->NewGlobalRef(javaObj_);
    }
       
    _tlp_jenv->DeleteLocalRef(javaClass);  
  }  
  
  isInitialized_ = true;
  return JOI_OK;
}

JOI_RetCode JavaObjectInterfaceTM::initJNIEnv()
{
  JOI_RetCode retcode;
  if (_tlp_jenv == NULL) {
     if ((retcode = initJVM()) != JOI_OK)
         return retcode;
  }
  if (_tlp_jenv->PushLocalFrame(jniHandleCapacity_) != 0) {
    return JOI_ERROR_INIT_JNI;
  }
  return JOI_OK;
}

void set_error_msg(std::string &error_msg) 
{
   if (_tlp_error_msg != NULL)
      delete _tlp_error_msg;
   _tlp_error_msg = new std::string(error_msg); 
}

void set_error_msg(char *error_msg) 
{
   if (_tlp_error_msg != NULL)
      delete _tlp_error_msg;
   _tlp_error_msg = new std::string(error_msg); 
}



bool  JavaObjectInterfaceTM::getExceptionDetails(JNIEnv *jenv, short *errCode)
{
   std::string error_msg;

   if (jenv == NULL)
       jenv = _tlp_jenv;
   if (jenv == NULL)
   {
      error_msg = "Internal Error - Unable to obtain jenv";
      set_error_msg(error_msg);
      return false;
   }
   
   if(!jenv->ExceptionCheck())
   {
     //jenv_->ExceptionCheck() is a light weight call
     //to determine exception exists. Return here if 
     //no exception object to probe.
     return false;
   }
   
   if (gThrowableClass == NULL)
   {
      jenv->ExceptionDescribe();
      error_msg = "Internal Error - Unable to find Throwable class";
      set_error_msg(error_msg);
      return false;
   }
   
   jthrowable a_exception = jenv->ExceptionOccurred();
   if (a_exception == NULL)
   {
       error_msg = "No java exception was thrown";
       set_error_msg(error_msg);
       return false;
   }
   
   if(errCode)
   {
     *errCode = getExceptionErrorCode(jenv, a_exception);
   }
   
   error_msg = "";
   appendExceptionMessages(jenv, a_exception, error_msg);
   set_error_msg(error_msg);
   jenv->ExceptionClear();
   return true;
}

void JavaObjectInterfaceTM::appendExceptionMessages(JNIEnv *jenv, jthrowable a_exception, std::string &error_msg)
{
    jstring msg_obj =
       (jstring) jenv->CallObjectMethod(a_exception,
                                         gThrowableToStringMethodID);
    const char *msg_str;
    if (msg_obj != NULL)
    {
       msg_str = jenv->GetStringUTFChars(msg_obj, 0);
       error_msg += msg_str;
       jenv->ReleaseStringUTFChars(msg_obj, msg_str);
       jenv->DeleteLocalRef(msg_obj);
    }
    else
       msg_str = "Exception is thrown, but tostring is null";


    // Get the stack trace
    jobjectArray frames =
        (jobjectArray) jenv->CallObjectMethod(
                                        a_exception,
                                        gGetStackTraceMethodID);
    if (frames == NULL)
       return;
    jsize frames_length = jenv->GetArrayLength(frames);

    jsize i = 0;
    for (i = 0; i < frames_length; i++)
    {
       jobject frame = jenv->GetObjectArrayElement(frames, i);
       msg_obj = (jstring) jenv->CallObjectMethod(frame,
                                            gStackFrameToStringMethodID);
       if (msg_obj != NULL)
       {
          msg_str = jenv->GetStringUTFChars(msg_obj, 0);
          error_msg += "\n";
          error_msg += msg_str;
          jenv->ReleaseStringUTFChars(msg_obj, msg_str);
          jenv->DeleteLocalRef(msg_obj);
          jenv->DeleteLocalRef(frame);
       }
    }
    jthrowable j_cause = (jthrowable)jenv->CallObjectMethod(a_exception, gGetCauseMethodID);
    if (j_cause != NULL) {
       error_msg += " Caused by \n";
       appendExceptionMessages(jenv, j_cause, error_msg);
    }
    jenv->DeleteLocalRef(a_exception);
}

short JavaObjectInterfaceTM::getExceptionErrorCode(JNIEnv *jenv, jthrowable a_exception)
{
  jshort errCode = JOI_OK;
  if(a_exception != NULL)
  {
    //Only TransactionManagerException class has errorcode defined.
    if(jenv->IsInstanceOf(a_exception, gTransactionManagerExceptionClass))
    {
      errCode =(jshort) jenv->CallShortMethod(a_exception,
                                              gGetErrorCodeMethodID);
      if(jenv->ExceptionCheck())
      {
        jenv->ExceptionClear();
      }
    }
  }
  return errCode;
}
