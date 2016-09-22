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

#include "javaobjectinterfacetm.h"

// ===========================================================================
// ===== Class JavaObjectInterfaceTM
// ===========================================================================

JavaVM* JavaObjectInterfaceTM::jvm_  = NULL;

#define DEFAULT_MAX_TM_HEAP_SIZE "2048" 
#define USE_JVM_DEFAULT_MAX_HEAP_SIZE 0
  
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
 ,"Error Unknown"
};

__thread JNIEnv* _tlp_jenv = 0;
__thread bool  _tlv_jenv_set = false;
__thread std::string *_tlp_error_msg = NULL;

jclass JavaObjectInterfaceTM::gThrowableClass = NULL;
jclass JavaObjectInterfaceTM::gStackTraceClass = NULL;
jmethodID JavaObjectInterfaceTM::gGetStackTraceMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gThrowableToStringMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gStackFrameToStringMethodID = NULL;
jmethodID JavaObjectInterfaceTM::gGetCauseMethodID = NULL;

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* JavaObjectInterfaceTM::getErrorText(JOI_RetCode errEnum)
{
   if (errEnum >= JOI_LAST) {
      fprintf(stderr,"getErrorText called with out of bounds index %d.\n",errEnum);
      fflush(stderr);
      abort();
      //return (char*)joiErrorEnumStr[JOI_LAST];
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

  //printf("In JavaObjectInterfaceTM::createJVM\n");

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

  if (debugPort_ > 0)
    {
      sprintf(debugOptions,"-agentlib:jdwp=transport=dt_socket,address=%d,server=y,timeout=%d,suspend=y",
                                                                       debugPort_,         debugTimeout_);
      jvm_options[numJVMOptions++].optionString = debugOptions;
    }
  
  jvm_args.version            = JNI_VERSION_1_6;
  jvm_args.options            = jvm_options;
  jvm_args.nOptions           = numJVMOptions;
  jvm_args.ignoreUnrecognized = true;

  int ret = JNI_CreateJavaVM(&jvm_, (void**)&_tlp_jenv, &jvm_args);
  if (ret != 0) {
    abort();
  }
  _tlv_jenv_set = true;
  fflush(stdout);
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
    if (result != JNI_OK)
      return JOI_ERROR_CHECK_JVM;      
      
    if (jvm_count == 0)
    {
      // No - create a new one.
      result = createJVM();
      if (result != JNI_OK)
        return JOI_ERROR_CREATE_JVM;
      needToDetach_ = false;
        
    }
  }
  if (_tlp_jenv == NULL) {
  // We found a JVM, can we use it?
  result = jvm_->GetEnv((void**) &_tlp_jenv, JNI_VERSION_1_6);
  switch (result)
  {
    case JNI_OK:
      break;
    
    case JNI_EDETACHED:
      fprintf(stderr,"initJVM: Detached, Try 2 attach\n");
      result = jvm_->AttachCurrentThread((void**) &_tlp_jenv, NULL);   
      if (result != JNI_OK)
      {
        fprintf(stderr,"initJVM: Error in attaching\n");
        return JOI_ERROR_ATTACH_JVM;
      }
      
      needToDetach_ = true;
      break;
       
    case JNI_EVERSION:
      return JOI_ERROR_JVM_VERSION;
      break;
      
    default:
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
    jclass javaClass = _tlp_jenv->FindClass(className); 
    if (_tlp_jenv->ExceptionCheck()) 
    {
      fprintf(stderr,"FindClass failed. javaClass %p.\n", javaClass);
      _tlp_jenv->ExceptionDescribe();
      _tlp_jenv->ExceptionClear();
      return JOI_ERROR_FINDCLASS;
    }
    
    if (javaClass == 0) 
    {
      return JOI_ERROR_FINDCLASS;
    }
    
    // Initialize the method pointers.
    if (!methodsInitialized)
    {
      for (int i=0; i<howManyMethods; i++)
      {
        JavaMethods[i].methodID = _tlp_jenv->GetMethodID(javaClass, 
                                                     JavaMethods[i].jm_name.data(), 
                                                     JavaMethods[i].jm_signature.data());
        if (JavaMethods[i].methodID == 0 || _tlp_jenv->ExceptionCheck())
        { 
          fprintf(stderr,"GetMethodID failed returning error. javaClass %p, i %d, "
                 "name %s, signature %s.\n", javaClass, i, 
                 JavaMethods[i].jm_name.data(), JavaMethods[i].jm_signature.data());
          _tlp_jenv->ExceptionDescribe();
          _tlp_jenv->ExceptionClear();
          _tlp_jenv->DeleteLocalRef(javaClass);  
          return JOI_ERROR_GETMETHOD;
        }      
      }
    }
    
    if (javaObj_ == NULL)
    {
      // Allocate an object of the Java class, and call its constructor.
      // The constructor must be the first entry in the methods array.
      javaObj_ = _tlp_jenv->NewObject(javaClass, JavaMethods[0].methodID);
      if (javaObj_ == 0 || _tlp_jenv->ExceptionCheck())
      { 
        _tlp_jenv->ExceptionDescribe();
        _tlp_jenv->ExceptionClear();
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

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
void JavaObjectInterfaceTM::logError(const char* cat, const char* methodName, jstring jresult)
{
  if (jresult == NULL) {}
  else
  {
    const char* char_result = _tlp_jenv->GetStringUTFChars(jresult, 0);
    _tlp_jenv->ReleaseStringUTFChars(jresult, char_result);
    _tlp_jenv->DeleteLocalRef(jresult);  
  }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
void JavaObjectInterfaceTM::logError(const char* cat, const char* file, int line)
{
}

void set_error_msg(std::string &error_msg) 
{
   if (_tlp_error_msg != NULL)
      delete _tlp_error_msg;
   _tlp_error_msg = new std::string(error_msg); 
}


bool  JavaObjectInterfaceTM::getExceptionDetails(JNIEnv *jenv)
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
   if (gThrowableClass == NULL)
   {
      jenv->ExceptionDescribe();
      error_msg = "Internal Error - Unable to find Throwable class";
      set_error_msg(error_msg);
      return false;
   }
   jthrowable a_exception = jenv->ExceptionOccurred();
   if (a_exception != NULL)
       jenv->ExceptionClear();
   else
   {
       error_msg = "No java exception was thrown";
       set_error_msg(error_msg);
       return false;
   }
   error_msg = "";
   appendExceptionMessages(jenv, a_exception, error_msg);
   error_msg += "\n";
   set_error_msg(error_msg);
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
       // Start the error message in a new line
       error_msg = "\n";
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
    error_msg += "\n";
    jthrowable j_cause = (jthrowable)jenv->CallObjectMethod(a_exception, gGetCauseMethodID);
    if (j_cause != NULL) {
       error_msg += "Caused by ";
       appendExceptionMessages(jenv, j_cause, error_msg);
    }
}

