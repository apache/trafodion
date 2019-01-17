/* -*-C++-*-
**********************************************************************
*
* File:         LmLangManagerJava.cpp
* Description:  Language Manager for Java 
*
* Created:      07/01/1999
* Language:     C++
*
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
#include "Platform.h"
  #include <seabed/fs.h>
  #include <seabed/ms.h>
  #include "lmjni.h"
#include "ComSmallDefs.h"
#include "ComRtUtils.h"
#include "sqlcli.h"
#include "Globals.h"

#include "LmLangManagerJava.h"
#include "LmJavaOptions.h"
#include "LmCommon.h"
#include "LmJavaHooks.h"
#include "LmJavaType.h"
#include "LmRoutineJavaObj.h"
#include "LmContManager.h"
#include "LmJavaExceptionReporter.h"
#include "UdrFFDC.h"




#include "Measure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LmExtFunc.h"
#include "LmAssert.h"
#include "LmDebug.h"
#include "LmUtility.h"

// A helper function to check if the DEFINE name is defined. DEFINE value
// and it's length are returned if definValue & defineValueLen pointers
// are not NULL 
// Returns TRUE if define is set
//         FALSE otherwise

NABoolean getDefineSetting(const char *defineName,
	                	  char *defineValue,
			       	  short *defineValueLen)
{
  return FALSE;
}

//////////////////////////////////////////////////////////////////////
//
// Global declarations
//
//////////////////////////////////////////////////////////////////////
JavaVM *LmSqlJVM = NULL; // The per process JVM. The JVM is allocated as
                         // a global so that (1) it remains after all
                         // LMJ instances are gone, (2) "utilites" can
                         // potentially access the JVM (future).

//////////////////////////////////////////////////////////////////////
//
// Local declarations
//
//////////////////////////////////////////////////////////////////////
// Error for overflow situation
#define ERR_LM_PARAM_OVERFLOW(result, pos)		\
if (result == LM_PARAM_OVERFLOW)			\
{							\
  *da << DgSqlCode(-LME_DATA_OVERFLOW)			\
      << DgInt0(pos);					\
  result = LM_ERR;					\
}

// Warning for overflow situation
#define WARN_LM_PARAM_OVERFLOW(result, pos)		\
if (result == LM_PARAM_OVERFLOW)			\
{							\
  *da << DgSqlCode(LME_DATA_OVERFLOW_WARN)		\
      << DgInt0(pos);					\
  result = LM_OK;					\
}


// Defines complete.


//////////////////////////////////////////////////////////////////////
//
// Class LmLanguageManagerJava
//
//////////////////////////////////////////////////////////////////////
ComUInt32 LmLanguageManagerJava::maxLMJava_ = 0;
ComUInt32 LmLanguageManagerJava::numLMJava_ = 0;


LmLanguageManagerJava::LmLanguageManagerJava(LmResult &result,
                                             NABoolean commandLineMode,
                                             ComUInt32 maxLMJava,
                                             LmJavaOptions *userOptions,
                                             ComDiagsArea *diagsArea)
  : LmLanguageManager(commandLineMode),
    contManager_(NULL),
    jniEnv_(NULL),
    threadId_(0),
    userName_(NULL),
    userPassword_(NULL),
    datasourceName_(NULL),
    jdbcSupportsRS_(FALSE),
    jdbcSPJRSVer_(0),
    diagsArea_(diagsArea),
    setDefaultCatSchFlag_(commandLineMode ? FALSE : TRUE),
    enableType2Conn_(FALSE),
    mapDefaultConnToType2Conn_(FALSE)
{
  setRoutineIsActive(FALSE);
  LmJavaHooks::init_vfprintfHook();

    NABoolean status = lmSetSignalHandlersToDefault();
    if (status != TRUE) {
      result = LM_ERR;
      if (diagsArea)
        *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
    		  << DgString0(": Could not set signal handlers to default before JVM creation");
      return;
    }
    LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Set signal handlers to default before JVM creation");
  
    initialize(result, maxLMJava, userOptions, diagsArea);
    
    status = lmRestoreUdrTrapSignalHandlers(TRUE);
    if (status != TRUE) {
      result = LM_ERR;
      if (diagsArea)
        *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
      	  	    << DgString0(": Could not restore UDR trap signal handlers after JVM creation");
      return;
    }
    LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored UDR trap signal handlers after JVM creation");


  // If we captured any JVM messages, add them to the diags area
  // 
  // $$$$ NOTE: Executor does not yet support warnings in the LOAD
  // reply. We need to fix that. Once fixed, then if the JVM was
  // successfully started but JVM messages were also captured, then
  // we can return the JVM messages as warnings. For now, if the JVM
  // was successfully started, then such messages will not go into
  // the diags area but we will instead send them to stderr.
  const char *jvmMessages = LmJavaHooks::get_vfprintfText();
  if (jvmMessages && jvmMessages[0])
  {
    if (diagsArea && result != LM_OK)
    {
      *diagsArea << DgSqlCode(-LME_JVM_CONSOLE_OUTPUT)
                 << DgString0(jvmMessages);
    }
    else
    {
       fprintf(stderr, jvmMessages);
       fflush(stderr);
    }
  }
}

void LmLanguageManagerJava::initialize(LmResult &result,
                                       ComUInt32 maxLMJava,
                                       LmJavaOptions *userOptions,
                                       ComDiagsArea *diagsArea)
{
  // Initialize Java class handles to NULL.
  loaderClass_  = utilityClass_   = stringClass_    = bigintClass_   = NULL;
  systemClass_  = bigdecClass_    = dateClass_      = timeClass_     = NULL;
  stampClass_   = intClass_       = longClass_      = floatClass_    = NULL;
  doubleClass_  = resultSetClass_ = jdbcMxT2Driver_   = connClass_     = NULL;
  lmCharsetClass_ = lmDrvrClass_ = jdbcMxT4Driver_ = NULL;

  // Initialize method and field IDs to NULL, to make sure they do not
  // contain garbage values. Some of the error handling routines
  // called by this constructor will use a method ID if it has a
  // non-NULL value.
  loadClassId_ =
  verifyMethodId_ = createCLId_ =
  utilityInitId_ = classCacheSizeId_ = classCacheEnforceId_ =
  systemGetPropId_ = systemSetPropId_ = systemClearPropId_ = 
  bigdecCtorId_ = bigdecStrId_ = bigdecUnscaleId_ = 
  bigintIntValueId_ = bigintLongValueId_ =
  dateStrId_ = dateValId_ = timeStrId_ = 
  timeValId_ = stampStrId_ = stampValId_ = 
  intCtorId_ = intValId_ = longCtorId_ = 
  longValId_ = floatCtorId_ = floatValId_ = 
  doubleCtorId_ = doubleValId_ =
  bytesToUnicodeId_ =  unicodeToBytesId_ =
  utilityGetRSInfoId_ = utilityInitRSId_ =
  utilityGetT4RSInfoId_ = utilityInitT4RSId_ = utilityGetConnTypeId_ =
  lmObjMethodInvokeClass_ = makeNewObjId_ = setRuntimeInfoId_ =
  routineMethodInvokeId_ = routineReturnInfoClass_ =
  returnInfoStatusField_ = returnInfoSQLStateField_ = returnInfoMessageField_ =
  returnInfoRIIField_ = returnInfoRPIField_ =
  udrClass_ = udrQueueStateField_ =
  rsCloseId_ = rsBeforeFirstId_ = rsNextId_ = rsWasNullId_ = rsGetWarningsId_ =
  rsGetShortId_ = rsGetIntId_ = rsGetLongId_ = rsGetFloatId_ =
  rsGetDoubleId_ = rsGetStringId_ = rsGetObjectId_ = rsGetBigDecimalId_ =
  rsGetDateId_ = rsGetTimeId_ = rsGetTimestampId_ =
  driverInitId_ = connCloseId_ = connIsClosedId_ =
  hpT4ConnSuspUdrXnId_ = NULL;
  result = LM_OK;
  
  sysCatalog_ = sysSchema_ = NULL;

  // Adjust the LMJ counter.
  if (maxLMJava_ == 0)
    maxLMJava_ = maxLMJava;

  ++numLMJava_;
 
  if (numLMJava_ > maxLMJava_)
  {
    *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
		<< DgString0(": Max JVMs exceeded.");
    result = LM_ERR;
    return;
  }

  jsize nVMs = 0;
  jint jrc = JNI_GetCreatedJavaVMs (&LmSqlJVM, 1, &nVMs);
  if (jrc != JNI_OK)
    {
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
                  << DgString0(": Unable to determine whether JVMs exist.");
      result = LM_ERR;
      return;
    }
  if (nVMs == 0)
    LmSqlJVM = NULL; // JVM has not yet been started

  // Allocate the JVM or attach to it.
  if (LmSqlJVM == NULL)
  {
    // First we make a copy of the JVM startup options being passed
    // in, and do some massaging of those options before starting the
    // JVM.
    LmJavaOptions *actualJvmOptions = new (collHeap()) LmJavaOptions();
    if (userOptions)
    {
      // This call will copy userOptions into actualJvmOptions and do
      // any necessary massaging of the options. userOptions will not
      // be modified by this call.
      processJavaOptions(*userOptions, *actualJvmOptions);
    }
    
#ifdef LM_DEBUG
    actualJvmOptions->display();
    LM_DEBUG0("");
    const char *jrehome = getenv("JREHOME");
    LM_DEBUG1("JREHOME is %s", jrehome ? jrehome : "NULL");
    LM_DEBUG1("commandLineMode_ is %s",
              getCommandLineMode() ? "TRUE" : "FALSE");
    LM_DEBUG1("setDefaultCatSchFlag_ is %s",
              setDefaultCatSchFlag_ ? "TRUE" : "FALSE");

    LM_DEBUG0("");
#endif


    ULng32 numUserOpts = actualJvmOptions->entries();
    ULng32 numHookOpts = 3; // for abort, exit, vfprintf hooks
#ifdef _DEBUG
  int debugPort = 0;
  int debugTimeout = 0;
  const char *debugPortStr = getenv("JVM_DEBUG_PORT");
  if (debugPortStr != NULL)
     debugPort = atoi(debugPortStr);
  const char *debugTimeoutStr = getenv("JVM_DEBUG_TIMEOUT");
  if (debugTimeoutStr != NULL)
     debugTimeout = atoi(debugTimeoutStr);
  const char *suspendOnDebug = getenv("JVM_SUSPEND_ON_DEBUG");
  if (debugPort > 0)
      numHookOpts++;
#endif
    // We allow an environment setting to determine whether or not we
    // register for JVM callbacks. By default the registration is ON.
    if (getenv("SQLMX_NO_JVM_HOOKS"))
    {
      LM_DEBUG0("*** No JVM hook functions will be registered because");
      LM_DEBUG0("*** environment variable SQLMX_NO_JVM_HOOKS is set");
      numHookOpts = 0;
    }

    JavaVMOption *vmOptions = NULL;
    if ((numUserOpts + numHookOpts) > 0)
    {
      const ULng32 numBytes =
        (numUserOpts + numHookOpts) * sizeof(JavaVMOption);
      vmOptions = (JavaVMOption *) new (collHeap()) char[numBytes];
      memset((char *) vmOptions, 0, numBytes);
    }

    // First we initialize the hook options and then the user
    // options. By registering our hooks first, then if an error
    // occurs while processing a user option, our hooks still get
    // called because they've already been registered. This can
    // happen, for example, if a security policy file is specified in
    // a user option and there is a syntax error in that file.
    ULng32 i = 0;
    if (numHookOpts > 0)
    {
      vmOptions[i].optionString = (char *) "vfprintf";
      vmOptions[i++].extraInfo = (void *) LmJavaHooks::vfprintfHookJVM;
      vmOptions[i].optionString = (char *) "abort";
      vmOptions[i++].extraInfo = (void *) LmJavaHooks::abortHookJVM;
      vmOptions[i].optionString = (char *) "exit";
      vmOptions[i++].extraInfo = (void *) LmJavaHooks::exitHookJVM;
#ifdef _DEBUG
      char debugOptions[300];
      if (debugPort > 0)
      {
         debugPort = debugPort + (GetCliGlobals()->myPin() % 1000);
         sprintf(debugOptions,"-agentlib:jdwp=transport=dt_socket,address=%d,server=y,timeout=%d",
            debugPort,   debugTimeout);
         if (suspendOnDebug != NULL)
            strcat(debugOptions, ",suspend=y");
         else
            strcat(debugOptions, ",suspend=n");
         vmOptions[i++].optionString = debugOptions;
       }
#endif
     }

    LM_ASSERT(i == numHookOpts);

    for (i = 0; i < numUserOpts; i++)
    {
      vmOptions[i + numHookOpts].optionString =
        (char *) (actualJvmOptions->getOption(i));
    }

    JavaVMInitArgs args;
    args.nOptions = (Lng32) (numUserOpts + numHookOpts);
    args.version = JNI_VERSION_1_6;
    args.options = vmOptions;
    args.ignoreUnrecognized = JNI_TRUE;

    // Create JVM
    TIMER_ON(jvmTimer)

    LM_DEBUG0("About to call JNI_CreateJavaVM()...");
    Int32 jniResult = JNI_CreateJavaVM(&LmSqlJVM, &jniEnv_, &args);
    LM_DEBUG1("JNI_CreateJavaVM() returned %d", jniResult);

    result = (LmResult) jniResult;

    TIMER_OFF(jvmTimer, "Creating JVM")

    // Clean up structures used for JVM initialization only
    delete actualJvmOptions;
    NADELETEBASIC(vmOptions, collHeap());

    if (result < 0)
    {
      *diagsArea_ << DgSqlCode(-LME_JVM_INIT_ERROR);

      result = LM_ERR;
      return;
    }


  } // if (LmSqlJVM == NULL)

  else
  {
    JavaVMAttachArgs args;
    args.version = JNI_VERSION_1_4;
    args.name = NULL;
    args.group = NULL;

    result = (LmResult)LmSqlJVM->AttachCurrentThread(&jniEnv_, &args);
  }

  if (result != LM_OK)
  {
    *diagsArea_ << DgSqlCode(-LME_JVM_INIT_ERROR);
    result = LM_ERR;
    return;
  }

  // Asserts, just to check we don't have NULL pointers
  LM_ASSERT(LmSqlJVM != NULL);
  LM_ASSERT(jniEnv_ != NULL);

  // record the constructing thread's ID.
  threadId_ = threadId();

  jclass jc;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  //
  // Now load some important classes and methods in them.
  // Here we are not checking for the exception after each GetMethodID call.
  // But the fact is that GetMethodID returns NULL if it fails to find the
  // method and exception won't get cleared until we call ExceptionClear()
  // explicitly. So we make bunch of jni calls and at the end we check for
  // the errors(basically NULL return values).
  //

  // Instantiate Exception Mechanism class
  exceptionReporter_ = new (collHeap())
                       LmJavaExceptionReporter((LmHandle) jni,
                                               this,
                                               result,
                                               diagsArea_);
  if (result == LM_ERR)
    return;

  TIMER_ON(lmUtilTimer);
  // Load LmUtility class. We can't use loadSysClass because loadSysClass
  // uses LmUtility.
  jc = (jclass) jni->FindClass("org/trafodion/sql/udr/LmUtility");
  TIMER_OFF(lmUtilTimer, "Load LmUtility class")
  if (jc)
  {
    utilityClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (utilityClass_ != NULL)
    {
      verifyMethodId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                        "verifyMethodSignature",
  		        "(Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;III[Ljava/lang/String;[I[Ljava/lang/String;)V");
      createCLId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                    "createClassLoader",
  		    "(Ljava/lang/String;I)Lorg/trafodion/sql/udr/LmClassLoader;");
      utilityInitId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                                              "init", "()V");
      classCacheSizeId_ = jni->GetStaticFieldID((jclass)utilityClass_,
                                                "classCacheSizeKB_", "I");
      classCacheEnforceId_ = jni->GetStaticFieldID((jclass)utilityClass_,
                                                   "classCacheEnforceLimit_",
                                                   "I");
      utilityGetRSInfoId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                                              "getRSInfo", "(Ljava/lang/Object;[I[J[Ljava/lang/Object;[I[Ljava/lang/String;)V");
      utilityInitRSId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                                              "initRS", "([J)Z");
      utilityGetConnTypeId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                               "getConnectionType", "(Ljava/lang/Object;)I");
      utilityGetT4RSInfoId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                               "getT4RSInfo", "(Ljava/lang/Object;[J[Z[I[Z[Ljava/lang/String;[Ljava/lang/Object;)V");
      utilityInitT4RSId_ = jni->GetStaticMethodID((jclass)utilityClass_,
                            "initT4RS", "()Z");

      if (jni->ExceptionOccurred()     || verifyMethodId_   == NULL ||
          createCLId_          == NULL || classCacheSizeId_ == NULL ||
          classCacheEnforceId_ == NULL || utilityInitId_ == NULL    ||
          utilityGetRSInfoId_  == NULL || utilityInitRSId_ == NULL  ||
	  utilityGetConnTypeId_ == NULL || utilityGetT4RSInfoId_ == NULL ||
	  utilityInitT4RSId_ == NULL)
      {
        exceptionReporter_->insertDiags(diagsArea_,
                                        -LME_JVM_SYS_CLASS_ERROR,
                                        "org.trafodion.sql.udr.LmUtility");
        result = LM_ERR;
        return;
      }
    }
  }
  else
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_JVM_SYS_CLASS_ERROR,
                                    "org.trafodion.sql.udr.LmUtility");
    result = LM_ERR;
    return;
  }

  // Now call the LmUtility::init() method
  jni->CallStaticVoidMethod((jclass) utilityClass_,
                            (jmethodID) utilityInitId_);
  if (jni->ExceptionOccurred())
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_INTERNAL_ERROR,
                                    ": org.trafodion.sql.udr.LmUtility.init()"
                                    " raised an exception.");

    result = LM_ERR;
    return;
  }

  // Register the native methods used by LmUtility Java class
  if (registerLmUtilityMethods(jni, (jclass)utilityClass_) < 0) {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_INTERNAL_ERROR,
                                    ": Error registering native methods for "
                                    "org.trafodion.sql.udr.LmUtility class.");
    result = LM_ERR;
    return;
  }

  const char *clName = "org/trafodion/sql/udr/LmClassLoader";
  const char *name   = "org.trafodion.sql.udr.LmClassLoader";

  TIMER_ON(lmClassLoaderTimer)
  // Load LmClassLoader class and findClass() method
  jc = (jclass) loadSysClass(clName, name, diagsArea_);
  TIMER_OFF(lmClassLoaderTimer, "Load LmClassLoader class")

  if (jc != NULL)
  {
    loaderClass_ = jc;
    loadClassId_ = jni->GetMethodID(jc, "loadClass",
                                    "(Ljava/lang/String;)Ljava/lang/Class;");

    if (jni->ExceptionOccurred() || loadClassId_  == NULL)
    {
      result = exceptionReporter_->insertDiags(diagsArea_,
                                               -LME_JVM_SYS_CLASS_ERROR,
                                               name);
      return;
    }
  }
  else
  {
    result = LM_ERR;
    return;
  }

  jc = (jclass) jni->FindClass("org/trafodion/sql/udr/LmCharsetCoder");
  if (jc)
  {
    lmCharsetClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (lmCharsetClass_ != NULL)
    {
      bytesToUnicodeId_ = jni->GetStaticMethodID((jclass) lmCharsetClass_,
                             "getUnicodeStringFromBytes",
                             "([BLjava/lang/String;II)Ljava/lang/String;");

      unicodeToBytesId_= jni->GetStaticMethodID((jclass) lmCharsetClass_,
                            "getBytesFromUnicodeString",
                            "(Ljava/lang/String;Ljava/lang/String;)[B");

      if (jni->ExceptionOccurred() ||
          bytesToUnicodeId_  == NULL || unicodeToBytesId_ == NULL)
      {
        exceptionReporter_->insertDiags(diagsArea_,
                                        -LME_JVM_SYS_CLASS_ERROR,
                                        "org.trafodion.sql.udr.LmCharsetCoder");
        result = LM_ERR;
        return;
      }
    }
  }
  else
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_JVM_SYS_CLASS_ERROR,
                                    "org.trafodion.sql.udr.LmCharsetCoder");
    result = LM_ERR;
    return;
  }

  jc = (jclass) jni->FindClass("org/trafodion/sql/udr/LmUDRObjMethodInvoke");
  if (jc)
  {
    lmObjMethodInvokeClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (lmObjMethodInvokeClass_ != NULL)
    {
      makeNewObjId_ = jni->GetStaticMethodID((jclass) lmObjMethodInvokeClass_,
        "makeNewObj",
        "(Lorg/trafodion/sql/udr/UDR;[B[B)Lorg/trafodion/sql/udr/LmUDRObjMethodInvoke;");

      setRuntimeInfoId_ = jni->GetMethodID((jclass) lmObjMethodInvokeClass_,
        "setRuntimeInfo",
        "(Ljava/lang/String;II)V");

      routineMethodInvokeId_ = jni->GetMethodID((jclass) lmObjMethodInvokeClass_,
        "invokeRoutineMethod",
        "(I[B[BI[B)Lorg/trafodion/sql/udr/LmUDRObjMethodInvoke$ReturnInfo;");

      if (jni->ExceptionOccurred() ||
          makeNewObjId_ == NULL ||
          setRuntimeInfoId_ == NULL ||
          routineMethodInvokeId_ == NULL)
      {
        exceptionReporter_->insertDiags(diagsArea_,
                                        -LME_JVM_SYS_CLASS_ERROR,
                                        "org.trafodion.sql.udr.LmUDRObjMethodInvoke");
        result = LM_ERR;
        return;
      }
    }
  }
  else
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_JVM_SYS_CLASS_ERROR,
                                    "org.trafodion.sql.udr.LmUDRObjMethodInvoke");
    result = LM_ERR;
    return;
  }

  jc = (jclass) jni->FindClass("org/trafodion/sql/udr/LmUDRObjMethodInvoke$ReturnInfo");
  if (jc)
  {
    routineReturnInfoClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);

    if (routineReturnInfoClass_ != NULL)
    {
      returnInfoStatusField_ = jni->GetFieldID(
           (jclass) routineReturnInfoClass_,
           "returnStatus_",
           "I");
      returnInfoSQLStateField_ = jni->GetFieldID(
           (jclass) routineReturnInfoClass_,
           "returnedSQLState_",
           "Ljava/lang/String;");
      returnInfoMessageField_ = jni->GetFieldID(
           (jclass) routineReturnInfoClass_,
           "returnedMessage_",
           "Ljava/lang/String;");
      returnInfoRIIField_ = jni->GetFieldID(
           (jclass) routineReturnInfoClass_,
           "returnedInvocationInfo_",
           "[B");
      returnInfoRPIField_ = jni->GetFieldID(
           (jclass) routineReturnInfoClass_,
           "returnedPlanInfo_",
           "[B");

      if (jni->ExceptionOccurred() ||
          returnInfoStatusField_ == NULL ||
          returnInfoSQLStateField_ == NULL ||
          returnInfoMessageField_ == NULL ||
          returnInfoRIIField_ == NULL ||
          returnInfoRPIField_ == NULL)
      {
        exceptionReporter_->insertDiags(diagsArea_,
                                        -LME_JVM_SYS_CLASS_ERROR,
                                        "org.trafodion.sql.udr.LmUDRObjMethodInvoke$ReturnInfo");
        result = LM_ERR;
        return;
      }
    }
  }
  else
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_JVM_SYS_CLASS_ERROR,
                                    "org.trafodion.sql.udr.LmUDRObjMethodInvoke$ReturnInfo");
    result = LM_ERR;
    return;
  }

  jc = (jclass) jni->FindClass("org/trafodion/sql/udr/UDR");
  if (jc)
  {
    udrClass_ = (jclass) jni->NewGlobalRef(jc);
    jni->DeleteLocalRef(jc);
  }
  else
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_JVM_SYS_CLASS_ERROR,
                                    "org.trafodion.sql.udr.UDR");
    result = LM_ERR;
    return;
  }

  // this class is used in JNI calls from Java UDRs to C++ code that
  // gets and emits rows
  jc = (jclass) jni->FindClass("org/trafodion/sql/udr/UDR$QueueStateInfo");
  if (jc)
  {
    udrQueueStateField_ = jni->GetFieldID(
         jc,
         "queueState_",
         "I");

    if (jni->ExceptionOccurred() ||
        udrQueueStateField_ == NULL)
      {
        exceptionReporter_->insertDiags(diagsArea_,
                                        -LME_JVM_SYS_CLASS_ERROR,
                                        "org/trafodion/sql/udr/UDR$QueueStateInfo");
        result = LM_ERR;
        jni->DeleteLocalRef(jc);
        return;
      }

    jni->DeleteLocalRef(jc);
  }
  else
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_JVM_SYS_CLASS_ERROR,
                                    "org/trafodion/sql/udr/UDR$QueueStateInfo");
    result = LM_ERR;
    return;
  }

  if (initJavaClasses() == LM_ERR)
  {
    result = LM_ERR;
    return;
  }

  if (jdbcMxT2Driver_)
  {
    // Now call the LmUtility::initRS() method
    jlongArray ver = jni->NewLongArray(1);

    if (jni->ExceptionOccurred() ) {
      // JVM Out of memory
      *diagsArea_ << DgSqlCode(-LME_JVM_OUT_OF_MEMORY);
      exceptionReporter_->checkJVMException(diagsArea_, 0);
      result = LM_ERR;
      return;
    }

    jboolean jdbcRS = jni->CallStaticBooleanMethod((jclass) utilityClass_,
                            (jmethodID) utilityInitRSId_, ver);

    if (jni->ExceptionOccurred())
    {
      exceptionReporter_->insertDiags(diagsArea_,
                                      -LME_INTERNAL_ERROR,
                                      ": org.trafodion.sql.udr.LmUtility.initRS() "
                                      "raised an exception.");

      result = LM_ERR;
      jni->DeleteLocalRef(ver);
      return;
    }

    // Set a flag to indicate whether the JDBC/MX driver 
    // linked into LM has support for SPJ RS or not.
    jdbcSupportsRS_ = (jdbcRS) ? TRUE : FALSE;

    // Set a flag to indicate whether the JDBC/MX interfaces 
    // for SPJ RS is supported or not
    jlong la[1];
    jni->GetLongArrayRegion(ver, 0, 1, la);
    LM_ASSERT(jni->ExceptionOccurred() == NULL);

    jdbcSPJRSVer_ = la[0];

    jni->DeleteLocalRef(ver);

    LM_DEBUG1("jdbcSupportsRS_ is %s", jdbcSupportsRS_ ? "TRUE" : "FALSE");
    LM_DEBUG1("jdbcRSVerMismatch() returned %s", jdbcRSVerMismatch() ? "TRUE" : "FALSE");
    LM_DEBUG2("jdbcSPJRSVer_ is %d, Supported version is %d", jdbcSPJRSVer_,
                 JDBC_SPJRS_VERSION);
  }

  if (jdbcMxT4Driver_)
  {
    // Initialize stuff needed for processing T4 result sets
    jboolean t4jdbcSupportsRS =
      jni->CallStaticBooleanMethod((jclass) utilityClass_,
                                   (jmethodID) utilityInitT4RSId_);

    if (jni->ExceptionOccurred())
    {
      exceptionReporter_->insertDiags(diagsArea_,
                                      -LME_INTERNAL_ERROR,
                                      ": org.trafodion.sql.udr.LmUtility.initT4RS() "
                                      "raised an exception.");

      result = LM_ERR;
      return;
    }

    if (! t4jdbcSupportsRS)
    {
      exceptionReporter_->insertDiags(diagsArea_,
                                      -LME_INTERNAL_ERROR,
                                      ": JDBC Type 4 driver loaded by UDR Ser"
				      "ver does not support SPJ Result Sets");

      result = LM_ERR;
      return;
    }
  }

  
  // Load org.trafodion.sql.udr.LmSQLMXDriver class
 #define LM_DRIVER_CLASS_PATH  "org/trafodion/sql/udr/LmT2Driver"
 #define LM_DRIVER_CLASS  "org.trafodion.sql.udr.LmT2Driver"
  jc = (jclass) loadSysClass(LM_DRIVER_CLASS_PATH,
                             LM_DRIVER_CLASS, diagsArea_);
  if (jc != NULL)
  {
    lmDrvrClass_ = jc;
    driverInitId_ = jni->GetStaticMethodID(jc, "init", "(ZZZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

    if (jni->ExceptionOccurred() || driverInitId_ == NULL)
    {
      exceptionReporter_->insertDiags(diagsArea_,
                                      -LME_JVM_SYS_CLASS_ERROR,
                                      LM_DRIVER_CLASS);
      result = LM_ERR;		
      return;
    }
  }
  else
  {
    result = LM_ERR;
    return;
  }

  // Determine if we need to set default catalog and schema values.
  // The decision is made whether the caller has set catalog/schema
  // values in startup options.
  if (userOptions)
  {
    NABoolean foundCat = FALSE, foundSch = FALSE;
    foundCat = userOptions->getSystemProperty("catalog",
					      &sysCatalog_,
					      collHeap());
    if (foundCat)
    {
      LM_ASSERT(sysCatalog_);
    }
    else
    {
      foundCat = userOptions->getSystemProperty("jdbcmx.catalog",
						&sysCatalog_,
						collHeap());
      if (foundCat)
	    LM_ASSERT(sysCatalog_);
    }

    foundSch = userOptions->getSystemProperty("schema",
					      &sysSchema_,
					      collHeap());
    if (foundSch)
    {
      LM_ASSERT(sysSchema_);
    }
    else
    {
      foundSch = userOptions->getSystemProperty("jdbcmx.schema",
					        &sysSchema_,
					        collHeap());
      if (foundSch)
	    LM_ASSERT(sysSchema_);
    }
  }

  // Now call the LmSQLMXDriver::init() method and pass in
  //   setDefaultCatSchFlag_,
  //   enableType2Conn_, mapDefaultConnToType2Conn_,
  //   System Name, User Name, User Password, Datasource Name, Port Number.
  //
  // Environment variables SYSNAME, USERID, USERPASSWORD, MXODBC_DATASOURCE 
  // & MXODBC_PORTNUMBER will be used if they are set. These are mainly for
  // developer regressions use.
  // On Seaquest, ndcsstart script sets environment variables
  // MXUDR_MXOAS_NODE and MXUDR_MXOAS_PORTNUMBER while starting mxoas. These
  // will be used if SYSNAME and MXODBC_PORTNUMBER are not set. This
  // allows developer settings to override the values set by ndcsstart script.
  //
  short error = 0;
  jstring sysName = NULL;
  char * sName = getenv("SYSNAME");
  if (!sName)
  {

    MS_Mon_Reg_Get_Type reg_node_info;

    // check the MXUDR_MXOAS_NODE environment variable set by ndcsstart script

    error = msg_mon_reg_get(MS_Mon_ConfigType_Cluster,    // type
                            false,                        // next
                            NULL,                         // grp (CLUSTER)
                            (char *) "MXUDR_MXOAS_NODE",  // key
                            &reg_node_info);              // info

    if(error == XZFIL_ERR_OK && reg_node_info.num_returned == 1)
    {
      sysName = (jstring) jni->NewStringUTF(reg_node_info.list[0].value);
    }
    else 
    {
      // We can not return error as CREATE PROCEDURE will fail when ndcs is not started.
      // Let's use node n001 of the Seaquest cluster as the default node name.
      sysName = (jstring) jni->NewStringUTF("n001");
    }
  }
  else
  {
    sysName = (jstring) jni->NewStringUTF(sName);
  }

  // For MXCI applications, the userid that comes from executor does
  // not contain password. So we need to depend on the env setting.
  // We use USERID and USERPASSWORD env variable if they are set.
  // If they are not set then we use the values that are passed from executor.

  jstring userName = NULL;
  jstring userPassword = NULL;
  if (userName_ != NULL)
  {
    userName = (jstring) jni->NewStringUTF(userName_);
    if (userPassword_ != NULL)
      userPassword = (jstring) jni->NewStringUTF(userPassword_);
  }

  jstring dsName = NULL;
  if (datasourceName_ == NULL)
    dsName = (jstring) jni->NewStringUTF(getenv("MXODBC_DATASOURCE"));
  else
    dsName = (jstring) jni->NewStringUTF(datasourceName_);
  Int32 portNumber = 0;
  // set applicationName property for JDBC connection for identification purposes
  const int MAX_APP_NAME_LEN = 120;
  char appName[MAX_APP_NAME_LEN];

  const int MAX_PROGRAM_DIR_LEN = 1024;
  char myProgramDir[MAX_PROGRAM_DIR_LEN+1];
  short myProcessType;
  Int32 myCPU;
  char myNodeName[MAX_SEGMENT_NAME_LEN+1];
  Lng32  myNodeNum;
  short myNodeNameLen = MAX_SEGMENT_NAME_LEN;
  char myProcessName[PROCESSNAME_STRING_LEN];
  Int64 myProcessStartTime;
  pid_t myPin;
  Lng32 retStatus = ComRtGetProgramInfo(myProgramDir, MAX_PROGRAM_DIR_LEN, myProcessType, myCPU,
                                        myPin, myNodeNum, myNodeName, myNodeNameLen,
                                        myProcessStartTime, myProcessName);
  if (retStatus)
  {
    char errStr[LMJ_ERR_SIZE_256];
    sprintf (errStr, ": Error returned from ComRtGetProgramInfo. Error is:  %d.", retStatus);
    *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
                << DgString0(errStr);
    result = LM_ERR;
    return;
  }

  sprintf (appName, "UDR:%s:%03d:%02d:%04d", myProcessName, myNodeNum, myCPU, myPin);
  if (setSystemProperty("hpt4jdbc.applicationName", appName, diagsArea_) == LM_ERR) 
  {
      result = LM_ERR;
      return;
  }

  LM_DEBUG4("About to init LmSQLMXDriver, setDefaultCatSchFlag_ is %s, enableType2Conn_ is %s, mapDefaultConnToType2Conn_ is %s, port number for Type 4 connections is %d",
            setDefaultCatSchFlag_ ? "TRUE" : "FALSE",
            enableType2Conn_ ? "TRUE" : "FALSE",
            mapDefaultConnToType2Conn_ ? "TRUE" : "FALSE",
	    portNumber);

  jni->CallStaticVoidMethod((jclass) lmDrvrClass_,
                            (jmethodID) driverInitId_,
			    (setDefaultCatSchFlag_) ? 1 : 0,
                            (enableType2Conn_) ? 1 : 0,
			    (mapDefaultConnToType2Conn_) ? 1 : 0,
			    sysName,
			    userName,
			    userPassword,
			    dsName,
			    portNumber);
  jni->DeleteLocalRef(sysName);
  jni->DeleteLocalRef(userName);
  if (userPassword)
    jni->DeleteLocalRef(userPassword);
  jni->DeleteLocalRef(dsName);

  if (jni->ExceptionOccurred())
  {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_INTERNAL_ERROR,
                                    ": org.trafodion.sql.udr.LmSQLMXDriver.init() "
                                    "raised an exception.");
    result = LM_ERR;
    return;
  }

  // Register the native methods used by LmT2Driver Java class
  if (registerLmT2DriverMethods(jni, (jclass)lmDrvrClass_) < 0) {
    exceptionReporter_->insertDiags(diagsArea_,
                                    -LME_INTERNAL_ERROR,
                                    ": Error registering native methods for "
                                    "org.trafodion.sql.udr.LmSQLMXDriver class.");
    result = LM_ERR;
    return;
  }

#ifdef LM_NOCACHE
  // Used for performance testing only.
  contManager_ =  new (collHeap()) LmContainerManagerNoCache(); 
#else
  // Create the CM. 1/2 of the JVM's heap is set aside for the aggregate
  // of the LMJ's container cache.
  jint maxHeapKB = jni->GetStaticIntField((jclass) utilityClass_,
                                          (jfieldID) classCacheSizeId_);
  if (maxHeapKB < 1)
  {
    maxHeapKB = 1;
  }
  jint enforce = jni->GetStaticIntField((jclass) utilityClass_,
                                        (jfieldID) classCacheEnforceId_);
  LM_DEBUG1("LM container cache max size is %d KB", (Lng32) maxHeapKB);
  LM_DEBUG1("LM container cache limits %s be enforced",
            (enforce != 0 ? "WILL" : "WILL NOT"));
  contManager_ = new (collHeap()) 
      LmContainerManagerCache(this,
                              (maxHeapKB * 1024) / (2 * maxLMJava_),
                              (enforce != 0 ? TRUE : FALSE), diagsArea_);
#endif

  result = LM_OK;

} // LmLanguageManagerJava::initialize()


/* loadSysClass() : Utility function for loading and pining a Java object
 * so that JVM doesn't garbage collect it. Used for only system class
 * objects. System class is any class that is part of JDK that we load when
 * we start LM or the classes provided by us. Checks for the validity of
 * the object.
 * Returns null if class cannot be found.
 */
LmHandle LmLanguageManagerJava::loadSysClass(
  const char *clName,
  const char *name,
  ComDiagsArea *diagsArea)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
    
  LM_DEBUG1("loadSysClass: loading %s", clName);
  jobject lobj = (jobject) jni->FindClass(clName);

  if (lobj == NULL || jni->ExceptionOccurred())
  {
    exceptionReporter_->insertDiags(diagsArea, -LME_JVM_SYS_CLASS_ERROR, name);

    // Exception occurred but return a valid pointer.
    if (lobj)
    {
      jni->DeleteLocalRef(lobj);
      lobj = NULL;
    }

    return lobj;
  }

  jobject gobj = jni->NewGlobalRef(lobj);
  if (gobj == NULL)
  {
    *diagsArea << DgSqlCode(-LME_JVM_OUT_OF_MEMORY);
  }

  jni->DeleteLocalRef(lobj);
  return (LmHandle)gobj;
}

/* unloadSysClass(): Unpins the object so that JVM can garbage collect it.
 */
void LmLanguageManagerJava::unloadSysClass(LmHandle obj)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  if (obj)
    jni->DeleteGlobalRef((jobject)obj);
}


/* initJavaClasses() : pre-load the various Java System classes and
 * method IDs the LMJ uses when interfacing with the JVM.
 * Returns LM_OK  on success
 *         LM_ERR if fails to load a class or a method in it
 */
LmResult LmLanguageManagerJava::initJavaClasses()
{
  jclass jc;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  TIMER_ON(set2ClsTimer)

  // Load java.lang.String
  jc = (jclass) loadSysClass("java/lang/String", "java.lang.String", diagsArea_);
  if (jc != NULL)
  {
    stringClass_ = jc;
    stringSubstringId_ = jni->GetMethodID(jc, "substring",
		                          "(II)Ljava/lang/String;");

    if (jni->ExceptionOccurred() || stringSubstringId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.lang.String");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.lang.System
  jc = (jclass) loadSysClass("java/lang/System", "java.lang.System", diagsArea_);
  if (jc != NULL)
  {
    systemClass_ = jc;
    systemGetPropId_ = jni->GetStaticMethodID(jc, "getProperty",
			    "(Ljava/lang/String;)Ljava/lang/String;");

    systemSetPropId_ = jni->GetStaticMethodID(jc, "setProperty",
 		       "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");

    systemClearPropId_ = jni->GetStaticMethodID(jc, "clearProperty",
			    "(Ljava/lang/String;)Ljava/lang/String;");

    if (jni->ExceptionOccurred() 
        || systemGetPropId_ == NULL 
        || systemSetPropId_ == NULL 
        || systemClearPropId_ == NULL
       )
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.lang.System");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.math.BigDecimal
  jc = (jclass) loadSysClass("java/math/BigDecimal", "java.math.BigDecimal", diagsArea_);
  if (jc != NULL)
  {
    bigdecClass_ = jc;
    bigdecCtorId_ = jni->GetMethodID(jc, "<init>", "(Ljava/lang/String;)V");
    bigdecStrId_ = jni->GetMethodID(jc, "toString", "()Ljava/lang/String;");
    bigdecUnscaleId_ = jni->GetMethodID(jc, "unscaledValue", "()Ljava/math/BigInteger;");

    if (jni->ExceptionOccurred() || bigdecCtorId_ == NULL ||
        bigdecStrId_ == NULL || bigdecUnscaleId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR, 
                                            "java.math.BigDecimal");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.math.BigInteger
  jc = (jclass) loadSysClass("java/math/BigInteger", "java.math.BigInteger", diagsArea_);
  if (jc != NULL)
  {
    bigintClass_ = jc;
    bigintIntValueId_ = jni->GetMethodID(jc, "intValue", "()I");
  
    if (jni->ExceptionOccurred() || bigintIntValueId_ == NULL) 
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                            -LME_JVM_SYS_CLASS_ERROR,
                                            "java.math.BigInteger");
    }
    bigintLongValueId_ = jni->GetMethodID(jc, "longValue", "()J");
    if (jni->ExceptionOccurred() || bigintLongValueId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                            -LME_JVM_SYS_CLASS_ERROR,
                                            "java.math.BigInteger");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.sql.Date
  jc = (jclass) loadSysClass("java/sql/Date", "java.sql.Date", diagsArea_);
  if (jc != NULL)
  {
    dateClass_ = jc;
    dateStrId_ = jni->GetMethodID(jc, "toString", "()Ljava/lang/String;");
    dateValId_ = jni->GetStaticMethodID(jc, "valueOf", "(Ljava/lang/String;)Ljava/sql/Date;");

    if (jni->ExceptionOccurred() || dateStrId_ == NULL || dateValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_, 
                                            -LME_JVM_SYS_CLASS_ERROR,
                                             "java.sql.Date");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.sql.Time
  jc = (jclass) loadSysClass("java/sql/Time", "java.sql.Time", diagsArea_);
  if (jc != NULL)
  {
    timeClass_ = jc;
    timeStrId_ = jni->GetMethodID(jc, "toString", "()Ljava/lang/String;");
    timeValId_ = jni->GetStaticMethodID(jc, "valueOf", "(Ljava/lang/String;)Ljava/sql/Time;");

    if (jni->ExceptionOccurred() || timeStrId_ == NULL || timeValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.sql.Time");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.sql.Timestamp
  jc = (jclass) loadSysClass("java/sql/Timestamp", "java.sql.Timestamp", diagsArea_);
  if (jc != NULL)
  {
    stampClass_ = jc;
    stampStrId_ = jni->GetMethodID(jc, "toString", "()Ljava/lang/String;");
    stampValId_ = jni->GetStaticMethodID(jc, "valueOf",
                       "(Ljava/lang/String;)Ljava/sql/Timestamp;");

    if (stampStrId_ == NULL || stampValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.sql.Timestamp");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.lang.Integer
  jc = (jclass) loadSysClass("java/lang/Integer", "java.lang.Integer", diagsArea_);
  if (jc != NULL)
  {
    intClass_ = jc;
    intCtorId_ = jni->GetMethodID(jc, "<init>", "(I)V");
    intValId_ = jni->GetMethodID(jc, "intValue", "()I");

    if (jni->ExceptionOccurred() || intCtorId_ == NULL || intValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.lang.Integer");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.lang.Long
  jc = (jclass) loadSysClass("java/lang/Long", "java.lang.Long", diagsArea_);
  if (jc != NULL)
  {
    longClass_ = jc;
    longCtorId_ = jni->GetMethodID(jc, "<init>", "(J)V");
    longValId_ = jni->GetMethodID(jc, "longValue", "()J");

    if (jni->ExceptionOccurred() || longCtorId_ == NULL || longValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.lang.Long");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.lang.Float
  jc = (jclass) loadSysClass("java/lang/Float", "java.lang.Float", diagsArea_);
  if (jc != NULL)
  {
    floatClass_ = jc;
    floatCtorId_ = jni->GetMethodID(jc, "<init>", "(F)V");
    floatValId_ = jni->GetMethodID(jc, "floatValue", "()F");

    if (jni->ExceptionOccurred() || floatCtorId_ == NULL || floatValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.lang.Float");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.lang.Double
  jc = (jclass) loadSysClass("java/lang/Double", "java.lang.Double", diagsArea_);
  if (jc != NULL)
  {
    doubleClass_ = jc;
    doubleCtorId_ = jni->GetMethodID(jc, "<init>", "(D)V");
    doubleValId_ = jni->GetMethodID(jc, "doubleValue", "()D");

    if (jni->ExceptionOccurred() || doubleCtorId_ == NULL || doubleValId_ == NULL)
    {
      return exceptionReporter_->insertDiags(diagsArea_,
                                             -LME_JVM_SYS_CLASS_ERROR,
                                             "java.lang.Double");
    }
  }
  else
  {
    return LM_ERR;
  }

  // Load java.sql.ResultSet
  resultSetClass_ = loadSysClass("java/sql/ResultSet",
                                 "java.sql.ResultSet", diagsArea_);
  if (resultSetClass_ == NULL)
    return LM_ERR;

  rsCloseId_ = jni->GetMethodID((jclass)resultSetClass_, "close", "()V");
  rsBeforeFirstId_ = jni->GetMethodID((jclass)resultSetClass_,
                                      "beforeFirst",
                                      "()V");
  rsNextId_ = jni->GetMethodID((jclass)resultSetClass_, "next", "()Z");
  rsWasNullId_ = jni->GetMethodID((jclass)resultSetClass_, "wasNull", "()Z");
  rsGetWarningsId_ = jni->GetMethodID((jclass)resultSetClass_,
                                      "getWarnings",
                                      "()Ljava/sql/SQLWarning;");

  rsGetShortId_ = jni->GetMethodID((jclass)resultSetClass_, "getShort", "(I)S");
  rsGetIntId_ = jni->GetMethodID((jclass)resultSetClass_, "getInt", "(I)I");
  rsGetLongId_ = jni->GetMethodID((jclass)resultSetClass_, "getLong", "(I)J");
  rsGetFloatId_ = jni->GetMethodID((jclass)resultSetClass_, "getFloat", "(I)F");
  rsGetDoubleId_ = jni->GetMethodID((jclass)resultSetClass_, "getDouble", "(I)D");
  rsGetBigDecimalId_ = jni->GetMethodID((jclass)resultSetClass_, "getBigDecimal", "(I)Ljava/math/BigDecimal;");
  rsGetStringId_ = jni->GetMethodID((jclass)resultSetClass_,
                                    "getString",
                                    "(I)Ljava/lang/String;");
  rsGetObjectId_ = jni->GetMethodID((jclass)resultSetClass_,
                                    "getObject",
                                    "(I)Ljava/lang/Object;");

  rsGetDateId_ = jni->GetMethodID((jclass)resultSetClass_,
                                  "getDate",
                                  "(I)Ljava/sql/Date;");
  rsGetTimeId_ = jni->GetMethodID((jclass)resultSetClass_,
                                  "getTime",
                                  "(I)Ljava/sql/Time;");
  rsGetTimestampId_ = jni->GetMethodID((jclass)resultSetClass_,
                                       "getTimestamp",
                                       "(I)Ljava/sql/Timestamp;");
  if (jni->ExceptionOccurred() ||
      rsCloseId_ == NULL || rsGetWarningsId_ == NULL ||
      rsGetShortId_ == NULL || rsGetIntId_ == NULL ||
      rsGetLongId_ == NULL || rsGetFloatId_ == NULL ||
      rsGetObjectId_ == NULL || rsGetStringId_ == NULL ||
      rsGetBigDecimalId_ == NULL || rsWasNullId_ == NULL ||
      rsGetDateId_ == NULL || rsGetTimeId_ == NULL ||
      rsBeforeFirstId_ == NULL || rsNextId_ == NULL ||
      rsGetDoubleId_ == NULL || rsGetTimestampId_ == NULL)
  {
	return exceptionReporter_->insertDiags(diagsArea_,
                                           -LME_JVM_SYS_CLASS_ERROR,
                                           "java.sql.ResultSet");
  }



  // Load java.sql.Connection
  connClass_ = loadSysClass("java/sql/Connection",
                                 "java.sql.Connection", diagsArea_);
  if (connClass_ == NULL)
    return LM_ERR;

  connCloseId_ = jni->GetMethodID((jclass)connClass_,
                                              "close", "()V");
  connIsClosedId_ = jni->GetMethodID((jclass)connClass_,
                                              "isClosed", "()Z");
  if (jni->ExceptionOccurred() || connCloseId_ == NULL ||
		                          connIsClosedId_ == NULL)
  {
    return exceptionReporter_->insertDiags(diagsArea_,
                                           -LME_JVM_SYS_CLASS_ERROR,
                                           "java.sql.Connection");
  }

  TIMER_OFF(set2ClsTimer, "Load Set 2 of classes");
    enableType2Conn_ = TRUE;
    mapDefaultConnToType2Conn_ = TRUE;
    jdbcMxT2Driver_ = loadSysClass("org/apache/trafodion/jdbc/t2/T2Driver",
	                         "org.apache.trafodion.jdbc.t2.T2Driver", diagsArea_);
    if (jdbcMxT2Driver_ == NULL)
      return LM_ERR;

  return LM_OK;
}

LmLanguageManagerJava::~LmLanguageManagerJava()
{
  JNIEnv* jni = (JNIEnv*)jniEnv_;

  // Adjust the LMJ counter.
  --numLMJava_;

  if (userName_)
    NADELETEBASIC(userName_, collHeap());

  if (userPassword_)
    NADELETEBASIC(userPassword_, collHeap());

  if (datasourceName_)
    NADELETEBASIC(datasourceName_, collHeap());

  if( sysCatalog_ )
	NADELETEBASIC(sysCatalog_, collHeap());
  if( sysSchema_ )
	NADELETEBASIC(sysSchema_, collHeap());

  // Ensure the LMJ was constructed properly.
  if (jni == NULL)
    return;

  // De-allocate instance objects and unref Java class objects.
  delete contManager_;

  delete exceptionReporter_;

  unloadSysClass(loaderClass_);
  unloadSysClass(utilityClass_);
  unloadSysClass(lmCharsetClass_);
  unloadSysClass(stringClass_);
  unloadSysClass(systemClass_);
  unloadSysClass(bigdecClass_);
  unloadSysClass(dateClass_);
  unloadSysClass(timeClass_);
  unloadSysClass(stampClass_);
  unloadSysClass(intClass_);
  unloadSysClass(longClass_);
  unloadSysClass(floatClass_);
  unloadSysClass(doubleClass_);
  unloadSysClass(resultSetClass_);
  unloadSysClass(connClass_);
  unloadSysClass(lmObjMethodInvokeClass_);
  unloadSysClass(routineReturnInfoClass_);

  unloadSysClass(jdbcMxT2Driver_);
  unloadSysClass(jdbcMxT4Driver_);

  // Detach from JVM.
  LmSqlJVM->DetachCurrentThread();
}

void LmLanguageManagerJava::destroyVM()
{
  LM_ASSERT(numLMJava_ == 0);

  if (LmSqlJVM)
  {
    lmRestoreJavaSignalHandlers();
    // Bring down the JVM
    LmSqlJVM->DestroyJavaVM();
    LmSqlJVM = NULL;
    lmRestoreUdrTrapSignalHandlers(TRUE);
  }

}

//////////////////////////////////////////////////////////////////////
// LM service: validateRoutine
//////////////////////////////////////////////////////////////////////
LmResult LmLanguageManagerJava::validateRoutine(
  ComUInt32     numSqlParam,
  ComFSDataType paramType[],
  ComUInt32     paramSubType[],
  ComColumnDirection direction[],
  const char    *routineName,
  const char    *containerName,
  const char    *externalPath,
  char          *sigBuf,
  ComUInt32     sigLen,
  ComFSDataType resultType,
  ComUInt32     resultSubType,
  ComUInt32     numResultSets,
  const char    *metaContainerName,
  const char    *optionalSig,
  ComDiagsArea  *diagsArea)
{
  LmContainer *container;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;
  ComBoolean isUdrForJavaMain = FALSE;

  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);

  if (startService(da) == LM_ERR)
    return LM_ERR;

  // Get the requested container from the CM.
  result = contManager_->getContainer(containerName, externalPath, 
                                      &container, da);

  if (result == LM_ERR)
    return LM_ERR;

  //
  // Create a Java signature for the specfied routine based upon
  // the various SQL attributes such as parameter type and mode.
  //
  // Special treatment is given if 'UDR is for main()' (UDR-MAIN) 
  // because main() takes only one parameter.
  //
  isUdrForJavaMain = ((str_cmp_ne(routineName, "main") == 0) ? TRUE: FALSE);
  LmJavaSignature javaSig(NULL, collHeap());
  result = javaSig.createSig(paramType, paramSubType, direction,
                             numSqlParam, resultType, resultSubType,
                             numResultSets, optionalSig, isUdrForJavaMain,
                             sigBuf, sigLen, da);
  if (result == LM_ERR)
    return LM_ERR;

  //
  // Though UDR-MAIN accepts 0 or more parameters, the underlying Java
  // method takes only one parameter.
  //
  ComUInt32 numJavaParam;
  if (isUdrForJavaMain)
    numJavaParam = 1;
  else
    numJavaParam = numSqlParam;

  // Using sigBuf signature, call LmUtility@verifyMethodSignature()
  jvalue jval[4];
  jval[0].l = (jobject) container->getHandle();
  jval[1].l = (jstring) jni->NewStringUTF(routineName);
  LmResult midChk = exceptionReporter_->checkNewObjectExceptions(jval[1].l, da);
  if(midChk == LM_ERR)
    return LM_ERR;
  jval[2].l = (jstring) jni->NewStringUTF(sigBuf);
  midChk = exceptionReporter_->checkNewObjectExceptions(jval[2].l, da);
  if(midChk == LM_ERR){
    jni->DeleteLocalRef(jval[1].l);
    return LM_ERR;
  }
  jval[3].i = (jint) numJavaParam;


  jni->CallStaticVoidMethodA((jclass)utilityClass_,
                             (jmethodID)verifyMethodId_,
                             jval);

  jni->DeleteLocalRef(jval[1].l);
  jni->DeleteLocalRef(jval[2].l);

  // verifyMethodSignature() throws exception on failure.
  if (jni->ExceptionOccurred() != NULL)
  {
    LmJavaSignature lmSig(sigBuf, collHeap());
    ComSInt32 size = lmSig.getUnpackedSignatureSize();
    ComUInt32 totLen = str_len(routineName) + size;
    char *signature = new (collHeap()) char[totLen + 1];

    sprintf(signature, "%s", routineName);
    lmSig.unpackSignature(signature + str_len(routineName));
    signature[totLen] = '\0';

    result = exceptionReporter_->insertDiags(da,
                                             -LME_ROUTINE_NOT_FOUND,
                                             signature,
                                             containerName);

    if (signature) NADELETEBASIC(signature, collHeap());
  }
  else
  {
    result = LM_OK;
  }

  // De-ref the container.
  if (container) contManager_->putContainer(container);

  return result;
}

//////////////////////////////////////////////////////////////////////
// LM serivce: getRoutine.
//////////////////////////////////////////////////////////////////////
LmResult LmLanguageManagerJava::getRoutine(
  ComUInt32    numSqlParam,
  LmParameter  parameters[],
  ComUInt32   numTableInfo,
  LmTableInfo tableInfo[],
  LmParameter  *returnValue,
  ComRoutineParamStyle paramStyle,
  ComRoutineTransactionAttributes transactionAttrs,
  ComRoutineSQLAccess sqlAccessMode,
  const char   *parentQid,
  ComUInt32    inputRowLen,
  ComUInt32    outputRowLen,
  const char   *sqlName,
  const char   *externalName,
  const char   *routineSig,
  const char   *containerName,
  const char   *externalPath,
  const char   *librarySqlName,
  const char   *currentUserName,
  const char   *sessionUserName,
  ComRoutineExternalSecurity externalSecurity,
  Int32        routineOwnerId,
  LmRoutine    **handle,
  LmHandle     getNextRowPtr,
  LmHandle     emitRowPtr,
  ComUInt32    maxResultSets,
  ComDiagsArea *diagsArea)
{
  NABoolean status = lmRestoreJavaSignalHandlers();
  if (status != TRUE) {
    if (diagsArea)
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
   	          << DgString0(": Could not restore Java signal handlers before entering Java in getRoutine");
    return LM_ERR;
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored Java signal handlers before entering Java in getRoutine");

  JNIEnv *jni = (JNIEnv*)jniEnv_;
  *handle = NULL;
  LmContainer *container = NULL;
  LmResult result = LM_OK;

  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);
  
  if (startService(da) == LM_ERR)
    result = LM_ERR;

  if (result != LM_ERR) 
    // Get the requested container from the CM.
    result = contManager_->getContainer(containerName, externalPath,
                                        &container, da);
  
  if (result != LM_ERR)
  {
    // Get the method ID from the container for the requested routine.
    jmethodID jm = jni->GetStaticMethodID((jclass)container->getHandle(), 
                                          externalName, routineSig);
  
    // Handle the result of the method ID look-up.
    if (jm != NULL)
    {
      // allocate an LM handle for the Java method.
      LmRoutineJava *h = new (collHeap()) 
        LmRoutineJava(sqlName, externalName, librarySqlName, numSqlParam, returnValue,
                      maxResultSets, (char *)routineSig,
                      COM_STYLE_JAVA_CALL,
                      transactionAttrs,
                      sqlAccessMode,
                      externalSecurity, 
		      routineOwnerId,
                      parentQid, inputRowLen, outputRowLen,
                      currentUserName, sessionUserName, 
                      parameters, this, jm, container, da);

      // Verify the handle.
      if (h == NULL || !h->isValid())
      {
        // DiagsArea is already filled by LmRoutineJava.
        if (h)
          delete h;

        if (container)
          contManager_->putContainer(container);

        result = LM_ERR;
      }
      else
      {
        *handle = h;
      }
    }
    else // Look-up failed. Set Diags area and return error.
    {
      ComSInt32 size = 0;
      LmJavaSignature *lmSig = new (collHeap()) LmJavaSignature(routineSig,
                                                                collHeap());
      size = lmSig->getUnpackedSignatureSize();
      ComUInt32 totLen = str_len(externalName) + size;
      char *signature = new (collHeap()) char[totLen + 1];
  
      sprintf(signature, "%s", externalName);
      lmSig->unpackSignature(signature + str_len(externalName));
      signature[totLen] = '\0';
      NADELETEBASIC(lmSig, collHeap());

      result = exceptionReporter_->checkGetMethodExceptions(signature,
                                                            containerName,
                                                            da);

      // Upon failure, de-ref the container.
      if (container)
        contManager_->putContainer(container);
  
      if (signature)
        NADELETEBASIC(signature, collHeap());
      result = LM_ERR;
    }
  }

  status = lmRestoreUdrTrapSignalHandlers(TRUE);
  if (status != TRUE) {
    if (diagsArea)
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
   	          << DgString0(": Could not restore UDR trap signal handlers after returning from Java in getRoutine");
    return   LM_ERR; 
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored UDR trap signal handlers after returning from Java in getRoutine");

  return result;
}

LmResult LmLanguageManagerJava::getObjRoutine(
     const char            *serializedInvocationInfo,
     int                    invocationInfoLen,
     const char            *serializedPlanInfo,
     int                    planInfoLen,
     ComRoutineLanguage     language,
     ComRoutineParamStyle   paramStyle,
     const char            *externalName,
     const char            *containerName,
     const char            *externalPath,
     const char            *librarySqlName,
     LmRoutine            **handle,
     ComDiagsArea          *diagsArea)
{
  *handle = NULL;

  NABoolean status = lmRestoreJavaSignalHandlers();
  if (status != TRUE) {
    if (diagsArea)
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
   	          << DgString0(": Could not restore Java signal handlers before entering Java in getObjRoutine");
    return LM_ERR;
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored Java signal handlers before entering Java in getObjRoutine");

  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmContainer *container = NULL;
  LmResult result = LM_OK;

  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);
  
  if (startService(da) == LM_ERR)
    result = LM_ERR;

  if (result != LM_ERR) 
    // Get the requested container from the CM.
    result = contManager_->getContainer(containerName, externalPath,
                                        &container, da);
  
  if (result != LM_ERR)
  {
    // Get the method ID from the container for the requested routine.
    jmethodID jm = jni->GetMethodID((jclass)container->getHandle(), 
                                    externalName,
                                    "()V");
  
    // Handle the result of the method ID look-up.
    if (jm != NULL)
    {
      tmudr::UDRInvocationInfo *invocationInfo = NULL;
      ComRoutineTransactionAttributes transactionAttrs = COM_NO_TRANSACTION_REQUIRED;
      ComRoutineSQLAccess sqlAccessMode = COM_NO_SQL;
      ComRoutineExternalSecurity externalSecurity = COM_ROUTINE_EXTERNAL_SECURITY_DEFINER;

      // call the default constructor for the specified class, this
      // creates a user-defined object derived from class
      // org.trafodion.sql.udr.UDR. We pass this object to the
      // Routine constructor, which will allocate a global reference
      // for it and dispose of it when it is destroyed. We make this
      // call outside the LmRoutineJavaObj constructor so that it
      // can also be done for validation.

      jobject newUDRObj = jni->NewObject((jclass)container->getHandle(), jm);

      if (newUDRObj == NULL || jni->ExceptionOccurred())
        {
          if (diagsArea)
            exceptionReporter_->insertDiags(
                 diagsArea,
                 -LME_UDR_METHOD_ERROR,
                 "constructor",
                 // Details: field will be blank, detail info
                 // is provided in a separate error entry.
                 containerName);
          result = LM_ERR;
        }

      if (invocationInfoLen > 0 && result == LM_OK)
        {
          // unpack invocation info to get to some info for the routine
          invocationInfo = new tmudr::UDRInvocationInfo();

          try
            {
              invocationInfo->deserializeObj(serializedInvocationInfo,
                                             invocationInfoLen);
            }
          catch (...)
            {
              *da << DgSqlCode(-LME_INTERNAL_ERROR)
                  << DgString0(": Error unpacking info in LmLanguageManagerJava::getObjRoutine()");
              result = LM_ERR;
            }

          if (result == LM_OK)
            {
              // for now we don't allow a choice, once we do we need to translate enums
              LM_ASSERT(invocationInfo->sqlTransactionType_ ==
                        tmudr::UDRInvocationInfo::REQUIRES_NO_TRANSACTION);
              LM_ASSERT(invocationInfo->sqlAccessType_ ==
                        tmudr::UDRInvocationInfo::CONTAINS_NO_SQL);
              LM_ASSERT(invocationInfo->sqlRights_ ==
                        tmudr::UDRInvocationInfo::INVOKERS_RIGHTS);

              // allocate an LM handle for the Java method.
              LmRoutineJavaObj *h = new (collHeap()) 
                LmRoutineJavaObj(invocationInfo->getUDRName().c_str(),
                                 externalName,
                                 librarySqlName,
                                 transactionAttrs,
                                 sqlAccessMode,
                                 externalSecurity, 
                                 0, // routine owner id is 0 for now
                                 serializedInvocationInfo,
                                 invocationInfoLen,
                                 serializedPlanInfo,
                                 planInfoLen,
                                 this,
                                 newUDRObj,
                                 container,
                                 da);

              // Verify the handle.
              if (h == NULL || !h->isValid())
                {
                  // DiagsArea is already filled by LmRoutineJava.
                  if (h)
                    delete h;

                  // no need to deallocate newUDRObj, since it is
                  // just a local reference

                  result = LM_ERR;
                }
              else
                {
                  *handle = h;
                }
            }

          if (invocationInfo)
            delete invocationInfo;
        } // invocation info was specified
      else if (result == LM_OK)
        {
          // Invocation info length is 0, this happens during
          // validation of a routine at DDL time. Return success
          // but don't create a routine object.
        }
    }
    else // Look-up failed. Set Diags area and return error.
    {
      result = exceptionReporter_->checkGetMethodExceptions(externalName,
                                                            containerName,
                                                            da);
      result = LM_ERR;
    }
  }

  status = lmRestoreUdrTrapSignalHandlers(TRUE);
  if (status != TRUE) {
    if (diagsArea)
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
   	          << DgString0(": Could not restore UDR trap signal handlers after returning from Java in getObjRoutine");
    result = LM_ERR; 
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored UDR trap signal handlers after returning from Java in getObjRoutine");

  // avoid leaking a container in the error case
  if (result != LM_OK && container)
    contManager_->putContainer(container);

  return result;
}

//////////////////////////////////////////////////////////////////////
// LM serivce: putRoutine.
//////////////////////////////////////////////////////////////////////
LmResult LmLanguageManagerJava::putRoutine(
  LmRoutine    *handle,
  ComDiagsArea *diagsArea)
{
  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);

  if (startService(da) == LM_ERR)
    return LM_ERR;

  NABoolean status = lmRestoreJavaSignalHandlers();
  if (status != TRUE) {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
   	<< DgString0(": Could not restore Java signal handlers before entering Java in putRoutine");
    return LM_ERR;
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored Java signal handlers before entering Java in putRoutine");

  LmResult result = LM_OK;
  // Verify the specified handle.
  LmRoutineJava *h = (LmRoutineJava*)handle;

  if (h == NULL)
  {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": Invalid Routine Handle.");
    result = LM_ERR;
  }
  else
  {
    // De-ref the container.
    if (h->container()) contManager_->putContainer(h->container());

    // De-allocate the handle.
    delete h;
  }

  status = lmRestoreUdrTrapSignalHandlers(TRUE);
  if (status != TRUE) {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
   	<< DgString0(": Could not restore UDR trap signal handlers after returning from  Java in putRoutine");
    return  LM_ERR; 
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored UDR trap signal handlers after returning from Java in putRoutine");

  return result;
}

//////////////////////////////////////////////////////////////////////
// LM serivce: invokeRoutine.
//////////////////////////////////////////////////////////////////////
LmResult LmLanguageManagerJava::invokeRoutine(
  LmRoutine    *handle,
  void *inputRow,
  void *outputRow,
  ComDiagsArea *diagsArea)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);

  LmRoutineJava *routine = (LmRoutineJava *) handle;
  LmResult result = LM_OK;

  if (startService(da) == LM_ERR)
    return LM_ERR;

  // Verify the specified routine handle.
  if (routine == NULL || !routine->isValid())
  {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": Invalid Routine Handle.");
    return LM_ERR;
  }

  NABoolean status = lmRestoreJavaSignalHandlers();
  if (status != TRUE) {
    if (diagsArea)
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
   	          << DgString0(": Could not restore Java signal handlers before entering Java in invokeRoutine");
    return LM_ERR;
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored Java signal handlers before entering Java in invokeRoutine");

  if (routine->getParamStyle() == COM_STYLE_JAVA_OBJ)
    {
      LmRoutineJavaObj *javaObjRoutine = static_cast<LmRoutineJavaObj *>(routine);
      Int32 dummy1, dummy2;

      // this path is only used for the run-time call, where we
      // have already received the UDRInvocationInfo/UDRPlanInfo
      result = javaObjRoutine->invokeRoutineMethod(
           tmudr::UDRInvocationInfo::RUNTIME_WORK_CALL,
           NULL,    // invocation info already there
           0,
           &dummy1, // expecting no updated invocation info
           NULL,    // plan info is already there or not used
           0,
           -1,
           &dummy2, // expecting no updated plan info
           (char *) inputRow,
           -1,      // use row length set earlier
           (char *) outputRow,
           -1,      // use row length set earlier
           da);
    } // Java object routine, used for TMUDFs
  else
    {
      // this is a Java routine with Java parameters, used for SPJs

      LmParameter *lmParams = routine->getLmParams();

      // Map the in-bound parameters from SQL to Java values.
      result = processInParameters(routine, lmParams, inputRow, da);
      if (result == LM_ERR)
        {
          processParametersDone(routine, lmParams);
        }
      else
        {
          // Before invoking method, make sure we don't have any pending exceptions.
          if (jni->ExceptionOccurred() != NULL)
            {
              exceptionReporter_->insertDiags(da,
                                              -LME_INTERNAL_ERROR,
                                              ": There is a pending exception.");

              processParametersDone(routine, lmParams);
              result = LM_ERR;
            }
        }

      if (result != LM_ERR)
        {
          // Invoke the Java method.
          routine->setDefaultCatSchFlag(setDefaultCatSchFlag_);
          result = routine->invokeRoutine(inputRow, outputRow, da);

          // Handle any uncaught Java exceptions.
          result = exceptionReporter_->processUserException(routine, da);

          NABoolean javaException = (result == LM_OK ? FALSE : TRUE);
  
          // If there are any uncaught Java exceptions then there may be 
          // Java result set and connection objects created in the SPJ
          // method that may still be valid and they need to be cleaned up.
          // We need to call processOutParameters() even when exceptions
          // are thrown so that we have the necessary LM objects created 
          // which are required for the cleanup process.

          // Map the out-bound parameters from Java to SQL values.
          result = processOutParameters(routine, lmParams, outputRow,
                                        javaException, da);

          // De-ref any Java objects resulting from the type mappings.
          processParametersDone(routine, lmParams);
        }
    } // Java routine with Java parameters

  status = lmRestoreUdrTrapSignalHandlers(TRUE);
  if (status != TRUE) {
    if (diagsArea)
      *diagsArea_ << DgSqlCode(-LME_INTERNAL_ERROR)
   	          << DgString0(": Could not restore UDR trap signal handlers after returning from Java in invokeRoutine");
    return LM_ERR; 
  }
  LM_DEBUG_SIGNAL_HANDLERS("[SIGNAL] Restored UDR trap signal handlers after returning from Java in invokeRoutine");

  return result;
}

//////////////////////////////////////////////////////////////////////
// CM support methods: createLoader, deleteLoader, loadContainer,
// and unloadContainer.
//////////////////////////////////////////////////////////////////////
/* createLoader(): creates a LmClassLoader object for a given directory
 * path 'externalPath'
 * Returns: pinned ref to LmClassLoader object on SUCCESS
 *          NULL   on FAILURE
 */
LmHandle LmLanguageManagerJava::createLoader(
  const char   *externalPath,
  ComDiagsArea *da)
{
  ComUInt32 i;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jobject jobj;
  jvalue jval[2];

  // Strip the file protocol from the path.
  i = skipURLProtocol(externalPath);

  //
  // The LmClassLoader takes two parameters: path, and debug flag.
  // debug flag is not used currently. May be we can make use of it
  // in future.
  //
  jval[0].l = jni->NewStringUTF(&externalPath[i]);
  LmResult midChk = exceptionReporter_->checkNewObjectExceptions(jval[0].l, da);
  if(midChk == LM_ERR)
    return NULL;
  jval[1].i = 0;

  // Create an LmClassLoader
  jobj = jni->CallStaticObjectMethodA((jclass)utilityClass_,
                                      (jmethodID)createCLId_,
                                      jval);
  jni->DeleteLocalRef(jval[0].l);

  // Check for Exceptions.
  if (jni->ExceptionOccurred() != NULL)
  {
    exceptionReporter_->insertDiags(da, -LME_CLASSLOADER_ERROR);
    return NULL;
  }

  // pin the ClassLoader Object. And delete local reference.
  jobject globalobj = jni->NewGlobalRef(jobj);
  jni->DeleteLocalRef(jobj);

  return globalobj;
}

/* deleteLoader() : De-ref the loader so that the JVM can garbage collect it.
 */
void LmLanguageManagerJava::deleteLoader(LmHandle loader)
{
  unloadSysClass((jobject)loader);
}

/* loadcontainer() : Load the 'containerName' class file from 'externalPath'
 * directory using extLoader(LmClassLoader object corresponding to path).
 * Returns: pinned ref to the class object on success
 *          NULL on FAILURE 
 */
LmHandle LmLanguageManagerJava::loadContainer(
  const char   *containerName,
  const char   *externalPath,
  LmHandle     extLoader,
  ComUInt32    *containerSize,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jobject jobj, container;
  jstring jstr;

  if (extLoader == NULL)
  {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": Invalid ClassLoader handle is passed to load a class file.");
    return NULL;
  }

  jstr = jni->NewStringUTF(containerName);
  LmResult midChk = exceptionReporter_->checkNewObjectExceptions(jstr, da);
  if(midChk == LM_ERR)
    return NULL;

  // Call the ClassLoader's loadClass() method to get the container.
  jobj = jni->CallObjectMethod((jobject)extLoader,
                               (jmethodID)loadClassId_,
                               jstr);

  jni->DeleteLocalRef(jstr);

  if (jni->ExceptionOccurred() == NULL)
  {
    // pin the object for the container
    container = jni->NewGlobalRef(jobj);
    jni->DeleteLocalRef(jobj);

    // Call the LmClassLoader's size method to get its current capacity.
    *containerSize = 100; // currently not implemented,
                          // could add an interface to verifyClassIsInFile
  }
  else
  {
    container = NULL;

    exceptionReporter_->insertDiags(da,
                                    -LME_CONT_NOT_FOUND,
                                    containerName,
                                    externalPath);
  }

  return container;
}

/* unloadContainer() : De-ref the container. Note that it is still
 * referenced via the base class loader.
 */
void LmLanguageManagerJava::unloadContainer(LmHandle container)
{ 
  if (container)
    ((JNIEnv*)jniEnv_)->DeleteGlobalRef((jobject)container);
}


//
// processInParameters(): Maps IN/INOUT params from SQL to Java values.
// For all types except primitive numeric types(eg int, float),
// NULL value is ok. In this case, NULL reference is sent to the
// Java method.  If NULL is passed in as input to primitive type,
// then error will be returned. 
//
LmResult LmLanguageManagerJava::processInParameters(
  LmRoutineJava *handle, 
  LmParameter   params[],
  void          *inputRow,
  ComDiagsArea  *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jvalue *jval = (jvalue*)handle->javaParams_;
  LmResult result = LM_OK;
  ComBoolean isUdrForJavaMain = handle->isUdrForJavaMain();

  for (Int32 i = 0; i < (Int32)handle->numSqlParam_; i++)
  {
    LmParameter *p = &params[i];

    if (p->direction() == COM_OUTPUT_COLUMN)
      continue;

    void *thisParamDataPtr = (void*) ((char*)inputRow + p->inDataOffset());

    // Check for null value
    NABoolean valIsNull = p->isNullInput((char *) inputRow);

    switch (LmJavaType(p).getType())
    {
      case LmJavaType::JT_SHORT:
      {
	if (valIsNull)
	{
	  *da << DgSqlCode(-LME_NULL_NOT_ALLOWED)
	      << DgInt0(i+1);
	  return LM_ERR;
	}

	jshort j = *(jshort*) thisParamDataPtr;

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].s = j;
	}
	else
	{
	  jni->SetShortArrayRegion((jshortArray)jval[i].l, 0, 1, &j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	}
      }
      break;

      case LmJavaType::JT_INT:
      {
	if (valIsNull)
	{
	  *da << DgSqlCode(-LME_NULL_NOT_ALLOWED)
	      << DgInt0(i+1);
	  return LM_ERR;
	}

	jint j = *(jint*) thisParamDataPtr;

	if (p->direction() == COM_INPUT_COLUMN)
	{
  	  jval[i].i = j;
	}
	else
	{
	  jni->SetIntArrayRegion((jintArray)jval[i].l, 0, 1, &j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	}
      }
      break;

      case LmJavaType::JT_LONG:
      {
	if (valIsNull)
	{
	  *da << DgSqlCode(-LME_NULL_NOT_ALLOWED)
	      << DgInt0(i+1);
	  return LM_ERR;
	}

	jlong j = *(jlong*) thisParamDataPtr;

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].j = j;
	}
	else
	{
	  jni->SetLongArrayRegion((jlongArray)jval[i].l, 0, 1, &j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	}
      }
      break; 

      case LmJavaType::JT_FLOAT:
      {
	if (valIsNull)
	{
	  *da << DgSqlCode(-LME_NULL_NOT_ALLOWED)
	      << DgInt0(i+1);
	  return LM_ERR;
	}

	jfloat j = *(jfloat*) thisParamDataPtr;

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].f = j;
	}
	else
	{
	  jni->SetFloatArrayRegion((jfloatArray)jval[i].l, 0, 1, &j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	}
      }
      break; 

      case LmJavaType::JT_DOUBLE:
      {
	if (valIsNull)
	{
	  *da << DgSqlCode(-LME_NULL_NOT_ALLOWED)
	      << DgInt0(i+1);
	  return LM_ERR;
	}

	jdouble j = *(jdouble*) thisParamDataPtr;

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].d = j;
	}
	else
	{
	  jni->SetDoubleArrayRegion((jdoubleArray)jval[i].l, 0, 1, &j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	}
      }
      break; 

      case LmJavaType::JT_LANG_STRING:
      {
	jstring j = NULL;

	if (! valIsNull)
	{
          convertToString(p, inputRow, (LmHandle *) &j, da);
          if (result == LM_ERR)
            return result;
        }

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  //
	  // Special treatment for UDR-MAIN
	  //
	  // Java method(main()) has only one parameter which is String[]
	  // So copy all the actual parameters into this array
	  //
	  if (isUdrForJavaMain)
	  {
	    jni->SetObjectArrayElement((jobjectArray)jval[0].l, i, j);
	    LM_ASSERT(jni->ExceptionOccurred() == NULL); 
	    jni->DeleteLocalRef((jobject)j);
	  }
	  else
	  {
	    jval[i].l = j;
	  }
	}
	else 
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;

      case LmJavaType::JT_MATH_BIGDEC:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToBigdec(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;

      case LmJavaType::JT_SQL_DATE:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToDate(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;

      case LmJavaType::JT_SQL_TIME:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToTime(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break; 

      case LmJavaType::JT_SQL_TIMESTAMP:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToTimestamp(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break; 

      case LmJavaType::JT_LANG_INTEGER:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToInteger(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;

      case LmJavaType::JT_LANG_LONG:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToLong(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;

      case LmJavaType::JT_LANG_FLOAT:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToFloat(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;

      case LmJavaType::JT_LANG_DOUBLE:
      {
	LmHandle j = NULL;

	if (! valIsNull)
	{
	  result = convertToDouble(p, inputRow, &j, da);
	  if (result == LM_ERR)
	    return result;
	}

	if (p->direction() == COM_INPUT_COLUMN)
	{
	  jval[i].l = (jobject)j;
	}
	else
	{
	  jni->SetObjectArrayElement((jobjectArray)jval[i].l, 0, (jobject)j);
	  LM_ASSERT(jni->ExceptionOccurred() == NULL);
	  jni->DeleteLocalRef((jobject)j);
	}
      }
      break;
     
      default:
      {
	char errStr[LMJ_ERR_SIZE_256];
	sprintf (errStr, ". Unknown parameter type encountered at parameter position %d.", i+1);
	*da << DgSqlCode(-LME_INTERNAL_ERROR)
	    << DgString0(errStr);
	return LM_ERR;
      }
    } // Switch

  } // For
  return LM_OK;
}

/* processOutParameters: Maps OUT/INOUT params from Java to SQL values.
 */
LmResult LmLanguageManagerJava::processOutParameters(
  LmRoutineJava *handle,
  LmParameter   params[],
  void          *outputRow,
  NABoolean     uncaughtExp,
  ComDiagsArea  *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jvalue *jval = (jvalue*)handle->javaParams_;
  jobject jobj;
  LmResult result = LM_OK;
  Int32 i;

  // Do not process the OUT/INOUT Sql params when there
  // are uncaught exceptions from SPJ invocation
  if (!uncaughtExp) {

  for (i = 0; i < (Int32)handle->numSqlParam_ && result == LM_OK; i++)
  {
    LmParameter *p = &params[i];

    if (p->direction() == COM_INPUT_COLUMN)
      continue;

    void *thisParamDataPtr = (void*) ((char*)outputRow + p->outDataOffset());
    ComBoolean valIsNull = FALSE;

    switch (LmJavaType(p).getType())
    {
      case LmJavaType::JT_SHORT:
      {
        jni->GetShortArrayRegion((jshortArray)jval[i].l,
                                 0, 1, (jshort*)thisParamDataPtr);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);
      }
      break;
      
      case LmJavaType::JT_INT:
      {
        jni->GetIntArrayRegion((jintArray)jval[i].l,
			       0, 1, (jint*)thisParamDataPtr);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);
      }
      break;

      case LmJavaType::JT_LONG:
      {
        jni->GetLongArrayRegion((jlongArray)jval[i].l,
				0, 1, (jlong*)thisParamDataPtr);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);
      }
      break;

      case LmJavaType::JT_FLOAT:
      {
        jni->GetFloatArrayRegion((jfloatArray)jval[i].l,
                                 0, 1, (jfloat*)thisParamDataPtr);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);
      }
      break;

      case LmJavaType::JT_DOUBLE:
      {
        jni->GetDoubleArrayRegion((jdoubleArray)jval[i].l,
                                  0, 1, (jdouble*)thisParamDataPtr);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);
      }
      break;

      case LmJavaType::JT_LANG_STRING:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

        valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromString(p, outputRow, jobj, da);
          jni->DeleteLocalRef(jobj);
	  WARN_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_MATH_BIGDEC:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromBigdec(p, outputRow, jobj);
          jni->DeleteLocalRef(jobj);
	  ERR_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_SQL_DATE:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromDate(p, outputRow, jobj);
          jni->DeleteLocalRef(jobj);
	  WARN_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_SQL_TIME:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromTime(p, outputRow, jobj);
          jni->DeleteLocalRef(jobj);
	  WARN_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_SQL_TIMESTAMP:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromTimestamp(p, outputRow, jobj);
          jni->DeleteLocalRef(jobj);
	  WARN_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_LANG_INTEGER:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromInteger(p, outputRow, jobj);
          jni->DeleteLocalRef(jobj);
	  ERR_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_LANG_LONG:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromLong(p, outputRow, jobj);
          jni->DeleteLocalRef(jobj);
	  ERR_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_LANG_FLOAT:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromFloat(p, outputRow, jobj, da);
          jni->DeleteLocalRef(jobj);
	  ERR_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      case LmJavaType::JT_LANG_DOUBLE:
      {
        jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
	LM_ASSERT(jni->ExceptionOccurred() == NULL);

	valIsNull = (jobj == NULL) ? TRUE : FALSE;

	if (! valIsNull)
	{
	  result = convertFromDouble(p, outputRow, jobj, da);
          jni->DeleteLocalRef(jobj);
	  ERR_LM_PARAM_OVERFLOW(result, i+1);
	}
      }
      break;

      default:
      {
        char errStr[LMJ_ERR_SIZE_256];
        sprintf(errStr,
                ": Unknown parameter type encountered at parameter position %d.",
                i+1);
        *da << DgSqlCode(-LME_INTERNAL_ERROR)
            << DgString0(errStr);
        result = LM_ERR;
      }
      break;
      
    } // switch (LmJavaType(p)->getType())

    // Set Null indicator bytes with correct byte value. The
    // setNullOutput() method is safe to call even for NOT NULL
    // parameters.
    p->setNullOutput((char *) outputRow, valIsNull);

  } // for each parameter

  } // if (uncaughtExp)

  // Now process result set parameters if any returned by the routine.
  // The below result set processing logic needs to execute even when 
  // 'result' != LM_OK since we will need to create LmResultSet objects
  // which will help us close any open result sets and the associated 
  // connections.
  LmResult rsResult = LM_OK;

  if (handle->maxResultSets_ > 0) {
    for (i = (Int32)handle->numSqlParam_; 
         i < (Int32)handle->numParamsInSig_; i++)
    {
      jobj = jni->GetObjectArrayElement((jobjectArray)jval[i].l, 0);
      LM_ASSERT(jni->ExceptionOccurred() == NULL);

      // If errors occurred in previous iterations then we have
      // still go ahead and delete the remaining result set objects.
      if (rsResult != LM_OK && jobj != NULL) {
        jni->DeleteLocalRef(jobj);
        continue;
      }

      // Check if the linked-in JDBC/MX Type 2 driver supports SPJ RS and if it
      // does then make sure there is no version mismatch in driver's interfaces.
      // If the SPJ method returns a NULL in all of it's RS parameters
      // then there are no RS to process and the above check can be waived.
      if (jobj != NULL && enableType2Conn_ &&
          (!jdbcSupportsRS_ || jdbcRSVerMismatch()))
      {
        if (!jdbcSupportsRS_)
          *da << DgSqlCode(-LME_JDBC_SUPPORT_SPJRS_ERROR)
              << DgString0(handle->getNameForDiags());
        else
          *da << DgSqlCode(-LME_JDBC_SPJRS_VERSION_ERROR)
              << DgString0(handle->getNameForDiags())
              << DgInt0((Lng32) jdbcSPJRSVer_)
              << DgInt1((Lng32) JDBC_SPJRS_VERSION);

        rsResult = LM_ERR;
        jni->DeleteLocalRef(jobj);
        continue;
      }
      rsResult = handle->populateResultSetInfo(jobj, i, da);
      jni->DeleteLocalRef(jobj);
    }

    // Check if the SPJ method returned more result sets than it's
    // declared maximum
    if (result == LM_OK && !uncaughtExp &&
          handle->getNumResultSets() > handle->maxResultSets_) {
      *da << DgSqlCode(LME_TOO_MANY_RS)
          << DgString0(handle->getNameForDiags())
          << DgInt0((Lng32) handle->maxResultSets_)
          << DgInt1((Lng32) handle->getNumResultSets());

      handle->deleteLmResultSetsOverMax(da);
    }
  }

  // We will now go ahead and close any default connections 
  // that may have been opened within the SPJ method but
  // there are no result sets associated with that connection.
  handle->closeDefConnWithNoRS(da);

  // If there are uncaught exceptions from the SPJ invocation
  // or if there were errors processing the routine's out params
  // then do result sets clean up and return LM_ERR status.
  if (result != LM_OK || rsResult != LM_OK || uncaughtExp) {
    handle->cleanupResultSets(da);
    result = LM_ERR;
  }

  return result;
}

/* processParametersDone(): De-ref Java objects resulting from IN mode 
 * type mappings. Note that de-ref for INOUT param objects is done in 
 * processInParameters after the object is set as an array element.
 */
void LmLanguageManagerJava::processParametersDone( 
  LmRoutineJava *handle,
  LmParameter   params[])
{

  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jvalue *jval = (jvalue*)handle->javaParams_;
  ComBoolean isUdrForJavaMain = handle->isUdrForJavaMain();

  // 
  // The objects for IN params for UDr-MAIN are allocated by the
  // constructor of LmRoutineJava. So let LmRoutineJava deallocate them.
  //
  if (isUdrForJavaMain)
  {
    return;
  }

  for (Int32 i = 0; i < (Int32)handle->numSqlParam_; i++)
  { 
    LmParameter *p = &params[i];

    // No de-ref required for OUT/INOUT.
    if (p->isOut())
      continue;

    // De-ref objects created for IN mode parameters.
    if (LmJavaType(p).isJavaTypeObject())
    {
      if (jval[i].l != NULL)
	jni->DeleteLocalRef(jval[i].l);
    }

    // Each IN mode parameter's Java value is nulled so that INOUT/OUT
    // parameter arrays can be recognized and de-ref in the LmRoutineJava
    // destructor.
    jval[i].l = NULL;
  }
}


/*
 * convert<To|From><Type>: See comments in LmLangManagerJava.h
 */
LmResult
LmLanguageManagerJava::convertToString(
  LmParameter  *param,
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jobject jobj;

  ComUInt32 len = param->actualInDataSize(inputRow);

  char *inputChars = ((char *)inputRow) + param->inDataOffset();

  NABoolean singleCharWorkAround = FALSE;
  if (len == 1 && param->encodingCharSet() == CharInfo::SJIS)
  {
    // Following is a workaround for a problem when a single ASCII byte
    // is passed when SJIS charset is in effect. In this case a String
    // with no bytes is being created.
    //
    // The workaround is to pass atleast two bytes, so we are adding
    // a null terminator. Later, the null terminator is removed by
    // calling subtring() method.
    //
    // FIXME: As of August 2009, this problem still exists in NSJ5. If
    // we move to a newer NSJ we should once again verify whether the
    // problem still exists.

    char *localInputChars = inputChars;
    inputChars = new (collHeap()) char[len+1];
    singleCharWorkAround = TRUE;
    str_cpy_all(inputChars, localInputChars, (Lng32) len);
    inputChars[len] = '\0';
    len++;
  }

  // Allocate Byte Array in JVM.
  jbyteArray javaBytes = jni->NewByteArray((Lng32)len);
  LmResult result = exceptionReporter_->
                      checkNewObjectExceptions(javaBytes, da);
  if (result == LM_ERR)
  {
    // Deallocate inputChars if it was allocated
    if (singleCharWorkAround)
      NADELETEBASIC(inputChars, collHeap());

    return LM_ERR;
  }

  // Set the Byte array with the input bytes
  jni->SetByteArrayRegion(javaBytes, 0, (Lng32)len, (jbyte *)inputChars);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);

  // Deallocate inputChars if it was allocated
  if (singleCharWorkAround)
    NADELETEBASIC(inputChars, collHeap());

  // Set charset of input
  jstring inputCharset = NULL;
  switch (param->encodingCharSet())
  {
    case CharInfo::ISO88591:
      inputCharset = jni->NewStringUTF("ISO88591");
      break;

    case CharInfo::UNICODE:
      inputCharset = jni->NewStringUTF("UCS2");
      break;

    case CharInfo::SJIS:
      inputCharset = jni->NewStringUTF("SJIS");
      break;

    case CharInfo::UTF8:
      inputCharset = jni->NewStringUTF("UTF8");
      break;

    default:
      jni->DeleteLocalRef(javaBytes);
      LM_ASSERT(0);
      break;
  }

  // Create String object from the byte array
  jobj = (jstring)jni->CallStaticObjectMethod((jclass) lmCharsetClass_,
                                              (jmethodID)bytesToUnicodeId_,
                                              javaBytes, inputCharset,
					      (jint)0, (jint)len);

  jni->DeleteLocalRef(inputCharset);
  jni->DeleteLocalRef(javaBytes);

  result = exceptionReporter_->checkNewObjectExceptions(jobj, da);
  if (result == LM_ERR)
    return LM_ERR;

  // Workaround for single ASCII char for SJIS charset string
  if (singleCharWorkAround)
    jobj = jni->CallObjectMethod(jobj, (jmethodID) stringSubstringId_, 0 , 1);

  *obj = jobj;
  return LM_OK;
}

LmResult LmLanguageManagerJava::convertFromString(
  LmParameter  *param, 
  void         *outputRow,
  LmHandle     obj,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jstring inputStr = (jstring)obj;
  LmResult result = LM_OK;
  jbyteArray  javaBytesArray;

  // Set charset of the output bytes
  jstring outputCharset = NULL;
  CharInfo::CharSet charset = param->encodingCharSet();
  switch (charset)
  {
    case CharInfo::ISO88591:
      outputCharset = jni->NewStringUTF("ISO88591");
      break;

    case CharInfo::UNICODE:
      outputCharset = jni->NewStringUTF("UCS2");
      break;

    case CharInfo::SJIS:
      outputCharset = jni->NewStringUTF("SJIS");
      break;

    case CharInfo::UTF8:
      outputCharset = jni->NewStringUTF("UTF8");
      break;

    default:
      LM_ASSERT(0);
  }

  // Get the bytes from the String in outputCharset charset
  javaBytesArray = (jbyteArray)
      jni->CallStaticObjectMethod((jclass) lmCharsetClass_,
                                  (jmethodID) unicodeToBytesId_,
                                  inputStr, outputCharset);

  jni->DeleteLocalRef(outputCharset);

  if(!javaBytesArray){
    exceptionReporter_->checkJVMException(da, NULL);
    return LM_ERR;
  }

  jsize len = jni->GetArrayLength(javaBytesArray);
  char *rawBytes = new (collHeap()) char[len];
  jni->GetByteArrayRegion(javaBytesArray, 0, len, (signed char *)rawBytes);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);

  if (charset == CharInfo::SJIS || charset == CharInfo::UTF8)
  {
    // In some cases, JVM returns NULL Characters at the end for
    // SJIS and UTF8 strings. One example is when single byte SJIS
    // characters are used, JVM returns two bytes for each character.
    //
    // Here, we adjust 'len' to ignore the extra NULL Chars that exist
    // at the end of the string.
    Int32 index;
    for (index=len-1; (index >= 0 && rawBytes[index] == '\0'); index--);

    len = index + 1;
  }

  result = param->setOutChar(outputRow, rawBytes, len);

  NADELETEBASIC(rawBytes, collHeap());
  jni->DeleteLocalRef(javaBytesArray);

  return result;
}

LmResult LmLanguageManagerJava::convertFromInterval(
  LmParameter *param,
  void *outputRow,
  LmHandle obj,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jstring jstr = (jstring)obj;
  LmResult result = LM_OK;
  jbyteArray  javaBytesArray;

  jstring outputCharset = jni->NewStringUTF("ISO88591");
  javaBytesArray = (jbyteArray)
        jni->CallStaticObjectMethod((jclass) lmCharsetClass_,
                                    (jmethodID) unicodeToBytesId_,
                                    jstr, outputCharset);
  jni->DeleteLocalRef(outputCharset);

  if(!javaBytesArray){
    exceptionReporter_->checkJVMException(da, NULL);
    return LM_ERR;
  }

  jsize len = 0;
  len = jni->GetArrayLength(javaBytesArray);
  if (len <= 0)
  {
     exceptionReporter_->checkJVMException(da, NULL);
     jni->DeleteLocalRef(javaBytesArray);
     return LM_ERR;
  }  
  char *rawBytes = new (collHeap()) char[len];
  jni->GetByteArrayRegion(javaBytesArray, 0, len, (signed char *)rawBytes);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);

  result = param->setOutInterval(outputRow, rawBytes, len);

  NADELETEBASIC(rawBytes, collHeap());
  jni->DeleteLocalRef(javaBytesArray);

  return result;
}

LmResult LmLanguageManagerJava::convertToBigdec(
  LmParameter  *param, 
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;
  char tempbuf[131];

  char *inputChars = ((char *)inputRow) + param->inDataOffset();
  ComUInt32 len = param->actualInDataSize(inputRow);

  // Bigdecimal value will never be more than 130 bytes long
  // This includes the optional sign byte and decimal point
  LM_ASSERT(len < 131);

  // First create the java.lang.String from the input bytes
  str_cpy_all(tempbuf, inputChars, (Lng32) len);
  tempbuf[len] = '\0';
  jstring jstr = jni->NewStringUTF(tempbuf);
  if (exceptionReporter_->checkNewObjectExceptions(jstr, da) == LM_ERR)
    return LM_ERR;
  
  // Create java.math.BigDecimal object from java.lang.String value
  jobj = jni->NewObject((jclass)bigdecClass_, (jmethodID)bigdecCtorId_, jstr);
  result = exceptionReporter_->checkNewObjectExceptions(jobj, da);

  jni->DeleteLocalRef(jstr);
  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromBigdec(
  LmParameter *param, 
  void        *outputRow,
  LmHandle    obj,
  ComBoolean resultSet_decimal, // useful to decide setting Numeric or Decimal
                                // type. The default value FALSE will set 
                                // Numeric type.
  ComBoolean copyBinary, // flag to copy binary value into target
                         // This is true while copying RS bigdec values
  ComDiagsArea *da)
{
  jobject jobj = (jobject)obj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jstring jstr;
  LmResult result = LM_OK;

  jstr = (jstring)jni->CallObjectMethod(jobj, (jmethodID)bigdecStrId_);
  if (!jstr)
  {
    exceptionReporter_->checkJVMException(da, NULL);
    return LM_ERR;
  }

  const char *str = jni->GetStringUTFChars(jstr, NULL);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  if (resultSet_decimal)
  {
     result = param->setOutDecimal(outputRow, str, collHeap(), da);
  }
  else
  {
     result = param->setOutNumeric(outputRow, str, copyBinary, collHeap(), da);
  }

  jni->ReleaseStringUTFChars(jstr, str);
  jni->DeleteLocalRef(jstr);

  return result;
}

LmResult LmLanguageManagerJava::convertToDate(
  LmParameter  *param, 
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jstring jstr; 
  LmResult result = LM_OK;
  char tempbuf[64];

  char *inputChars = ((char *)inputRow) + param->inDataOffset();
  ComUInt32 len = param->actualInDataSize(inputRow);

  // NOTE: jdbc only handles default date: yyyy-mm-dd.
  len = (len <= 10)? len : 10;

  // First create the java.lang.String from the input bytes
  str_cpy_all(tempbuf, inputChars, (Lng32) len);
  tempbuf[len] = '\0';
  jstr = jni->NewStringUTF(tempbuf);
  if (exceptionReporter_->checkNewObjectExceptions(jstr, da) == LM_ERR)
    return LM_ERR;

  // Create java.sql.Date object from java.lang.String value
  jobj = jni->CallStaticObjectMethod((jclass)dateClass_,
				     (jmethodID)dateValId_, jstr);
  result = exceptionReporter_->checkJVMException(da);

  jni->DeleteLocalRef(jstr);
  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromDate(
  LmParameter  *param, 
  void         *outputRow,
  LmHandle     obj,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jobject jobj = (jobject)obj;
  jstring jstr;
  LmResult result = LM_OK;
  const char *str;

  jstr = (jstring)jni->CallObjectMethod(jobj, (jmethodID)dateStrId_);
  if (!jstr)
  {
    exceptionReporter_->checkJVMException(da, NULL);
    return LM_ERR;
  }

  str = jni->GetStringUTFChars(jstr, NULL);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  result = param->setOutDate(outputRow, str);

  jni->ReleaseStringUTFChars(jstr, str);
  jni->DeleteLocalRef(jstr);

  return result;
}

LmResult LmLanguageManagerJava::convertToTime(
  LmParameter  *param,
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;
  char tempbuf[64];

  char *inputChars = ((char *)inputRow) + param->inDataOffset();
  ComUInt32 len = param->actualInDataSize(inputRow);

  // NOTE: jdbc only handles default time: hh:mm:ss.msssss (less .mssss).
  len = (len <= 8)? len : 8;

  // First create the java.lang.String from the input bytes
  str_cpy_all(tempbuf, inputChars, (Lng32) len);
  tempbuf[len] = '\0';
  jstring jstr = jni->NewStringUTF(tempbuf);
  if (exceptionReporter_->checkNewObjectExceptions(jstr, da) == LM_ERR)
    return LM_ERR;

  // Create java.sql.Time object from java.lang.String value
  jobj = jni->CallStaticObjectMethod((jclass)timeClass_,
				     (jmethodID)timeValId_, jstr);
  result = exceptionReporter_->checkJVMException(da);

  jni->DeleteLocalRef(jstr);
  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromTime(
  LmParameter  *param, 
  void         *outputRow,
  LmHandle     obj,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jobject jobj = (jobject)obj;
  jstring jstr;
  LmResult result = LM_OK;
  const char *str;

  jstr = (jstring)jni->CallObjectMethod(jobj, (jmethodID)timeStrId_);
  if (!jstr)
  {
    exceptionReporter_->checkJVMException(da, NULL);
    return LM_ERR;
  }

  str = jni->GetStringUTFChars(jstr, NULL);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  result = param->setOutTime(outputRow, str);

  jni->ReleaseStringUTFChars(jstr, str);
  jni->DeleteLocalRef(jstr);

  return result;
}

LmResult LmLanguageManagerJava::convertToTimestamp(
  LmParameter  *param, 
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;
  char tempbuf[64];

  char *inputChars = ((char *)inputRow) + param->inDataOffset();
  ComUInt32 len = param->actualInDataSize(inputRow);

  // NOTE: jdbc only handles default timestamp: yyyy-mm-dd hh:mm:ss.nssssssss.
  len = (len <= 29) ? len :  29;

  // First create the java.lang.String from the input bytes
  str_cpy_all(tempbuf, inputChars, (Lng32) len);
  tempbuf[len] = '\0';
  jstring jstr = jni->NewStringUTF(tempbuf);
  if (exceptionReporter_->checkNewObjectExceptions(jstr, da) == LM_ERR)
    return LM_ERR;

  // Create java.sql.Timestamp object from java.lang.String value
  jobj = jni->CallStaticObjectMethod((jclass)stampClass_,
				     (jmethodID)stampValId_, jstr);
  result = exceptionReporter_->checkJVMException(da);

  jni->DeleteLocalRef(jstr);
  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromTimestamp(
  LmParameter  *param, 
  void         *outputRow,
  LmHandle     obj,
  ComDiagsArea *da)
{
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  jobject jobj = (jobject)obj;
  jstring jstr;
  LmResult result = LM_OK;
  const char *str;

  jstr = (jstring)jni->CallObjectMethod(jobj, (jmethodID)stampStrId_);
  if (!jstr)
  {
    exceptionReporter_->checkJVMException(da, NULL);
    return LM_ERR;
  }
  str = jni->GetStringUTFChars(jstr, NULL);
  LM_ASSERT(jni->ExceptionOccurred() == NULL);
  result = param->setOutTimestamp(outputRow, str);

  jni->ReleaseStringUTFChars(jstr, str);
  jni->DeleteLocalRef(jstr);

  return result;
}

LmResult LmLanguageManagerJava::convertToInteger(
  LmParameter  *param, 
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jint val = *(jint*) ((char *)inputRow + param->inDataOffset());
  jobj = jni->NewObject((jclass)intClass_, (jmethodID)intCtorId_, val);
  result = exceptionReporter_->checkNewObjectExceptions(jobj, da); 

  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromInteger(
  LmParameter *param, 
  void        *outputRow,
  LmHandle    obj)
{
  jobject jobj = (jobject)obj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jint r = jni->CallIntMethod(jobj, (jmethodID)intValId_);
  result = param->setOutInteger(outputRow, r);

  return result;
}

LmResult LmLanguageManagerJava::convertToLong(
  LmParameter  *param, 
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jlong val = *(jlong*) ((char *)inputRow + param->inDataOffset());
  jobj = jni->NewObject((jclass)longClass_, (jmethodID)longCtorId_, val);
  result = exceptionReporter_->checkNewObjectExceptions(jobj, da); 

  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromLong(
  LmParameter *param, 
  void        *outputRow,
  LmHandle    obj)
{
  jobject jobj = (jobject)obj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jlong r = jni->CallLongMethod(jobj, (jmethodID)longValId_);
  result = param->setOutLargeInt(outputRow, r);

  return result;
}

LmResult LmLanguageManagerJava::convertToFloat(
  LmParameter  *param, 
  void *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jfloat val = *(jfloat*) ((char *)inputRow + param->inDataOffset());

  jvalue args[1];
  args[0].f = val;
  jobj = jni->NewObjectA((jclass)floatClass_, 
			 (jmethodID)floatCtorId_, args);
  result = exceptionReporter_->checkNewObjectExceptions(jobj, da); 

  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromFloat(
  LmParameter  *param,
  void         *outputRow,
  LmHandle     obj,
  ComDiagsArea *da)
{
  jobject jobj = (jobject)obj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jfloat r = jni->CallFloatMethod(jobj, (jmethodID)floatValId_);
  result = param->setOutReal(outputRow, r);

  return result;
}

LmResult LmLanguageManagerJava::convertToDouble(
  LmParameter  *param, 
  void         *inputRow,
  LmHandle     *obj,
  ComDiagsArea *da)
{
  jobject jobj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jdouble val = *(jdouble*) ((char *)inputRow + param->inDataOffset());

  jvalue args[1];
  args[0].d = val;
  jobj = jni->NewObjectA((jclass)doubleClass_, 
			 (jmethodID)doubleCtorId_, args);
  result = exceptionReporter_->checkNewObjectExceptions(jobj, da);
  *obj = jobj;
  return result;
}

LmResult LmLanguageManagerJava::convertFromDouble(
  LmParameter  *param, 
  void         *outputRow,
  LmHandle     obj,
  ComDiagsArea *da)
{
  jobject jobj = (jobject)obj;
  JNIEnv *jni = (JNIEnv*)jniEnv_;
  LmResult result = LM_OK;

  jdouble r = jni->CallDoubleMethod(jobj, (jmethodID)doubleValId_);
  result = param->setOutDouble(outputRow, r);

  return result;
}

/* startService(): Prepare for next service call and verify that serivce
 * can be started.
 */
LmResult LmLanguageManagerJava::startService(ComDiagsArea *da)
{
  if (LmSqlJVM != NULL && jniEnv_ != NULL && threadId_ == threadId())
    return LM_OK;
  else
  {
    *da << DgSqlCode(-LME_INTERNAL_ERROR)
        << DgString0(": Language Manager is not started yet.");  
    return LM_ERR;
  }
}

/* threadId(): Returns the current thread's ID. OS-dependent.
 */
ComSInt32 LmLanguageManagerJava::threadId()
{
  return GetCurrThreadId() ;
}


/* getSystemProperty(): Get the value of a Java system property.
 * Returns: LM_ERR if exceptions occur,
 *          LM_OK  Otherwise.
 * propertyIsSet is set to TRUE/FALSE depending on property is set or not.
 * value is set to the value of the property if the property is set.
 */
LmResult LmLanguageManagerJava::getSystemProperty(
  const char   *key,
  char         *value,
  ComUInt32    bufferLen,
  ComBoolean   &propertyIsSet,
  ComDiagsArea *diagsArea)
{
  JNIEnv *jni = (JNIEnv*) jniEnv_;
  jstring jstrIn = NULL, jstrOut = NULL;
  const char *cstrOut = NULL;

  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);

  propertyIsSet = FALSE;
  jstrIn = jni->NewStringUTF(key);
  LmResult midChk = exceptionReporter_->checkNewObjectExceptions(jstrIn, da);
  if(midChk == LM_ERR)
    return LM_ERR;

  jstrOut = (jstring) jni->CallStaticObjectMethod(
             (jclass) systemClass_, (jmethodID) systemGetPropId_,jstrIn);
  jni->DeleteLocalRef(jstrIn);
  if (exceptionReporter_->checkJVMException(da, 0) == LM_ERR)
    return LM_ERR;
  
  if (jstrOut == NULL)
    return LM_OK;

  cstrOut = jni->GetStringUTFChars(jstrOut, NULL);
  if (cstrOut == NULL)
  {
    *da << DgSqlCode (-LME_INTERNAL_ERROR)
        << DgString0 (" while getting system property.");
    jni->DeleteLocalRef(jstrOut);
    return LM_ERR;
  }

  ComUInt32 len = (ComUInt32) str_len(cstrOut);
  if (len < bufferLen)
    str_cpy_all(value, cstrOut, (Lng32) (len + 1));
  else if (bufferLen > 0)
  {
    str_cpy_all(value, cstrOut, (Lng32) (bufferLen - 1));
    value[bufferLen - 1] = 0;
  }

  propertyIsSet = TRUE;
  jni->ReleaseStringUTFChars(jstrOut, cstrOut);
  jni->DeleteLocalRef(jstrOut);
  return LM_OK;
}

/* setSystemProperty(): Set the value of a Java system property.
 * Returns LM_OK if the operation is successful.
 */
LmResult LmLanguageManagerJava::setSystemProperty(
  const char   *key,
  const char   *value,
  ComDiagsArea *diagsArea)
{
  JNIEnv *jni = (JNIEnv*) jniEnv_;
  jstring jstrKey = NULL, jstrVal = NULL, jstrResult = NULL;

  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);

  jstrKey = jni->NewStringUTF(key);
  LmResult midChk = exceptionReporter_->checkNewObjectExceptions(jstrKey, da);
  if(midChk == LM_ERR){
    if(jstrKey)
      jni->DeleteLocalRef(jstrKey);
    return LM_ERR;
  }
  jstrVal = jni->NewStringUTF(value);
  midChk = exceptionReporter_->checkNewObjectExceptions(jstrVal, da);
  if(midChk == LM_ERR){
    jni->DeleteLocalRef(jstrKey);
    if(jstrVal)
      jni->DeleteLocalRef(jstrVal);
    return LM_ERR;
  }

  jstrResult = (jstring) jni->CallStaticObjectMethod(
      (jclass)systemClass_, (jmethodID)systemSetPropId_, jstrKey, jstrVal);

  jni->DeleteLocalRef(jstrResult);
  jni->DeleteLocalRef(jstrVal);
  jni->DeleteLocalRef(jstrKey);
  if (exceptionReporter_->checkJVMException(da, 0) == LM_ERR)
    return LM_ERR;

  return LM_OK;

}

/* clearSystemProperty(): Clear the value of a Java system property.
 * Returns LM_OK if the operation is successful.
 * This is used for clearing the sqlmx.udr.defAuthToken property
 * for Definer Rights SPJ. The property is first cleared in 
 * LmSQLMXDriver code, but it is also done here in case the SPJ method
 * does not call getConnection.
 * There is no exception checking done here for JNI calls
 * as that would result in showing internal error in clearSystemProperty
 * if there was any exception thrown while invoking the SPJ method.
 */
LmResult LmLanguageManagerJava::clearSystemProperty(
  const char   *key,
  ComDiagsArea *diagsArea)
{
  JNIEnv *jni = (JNIEnv*) jniEnv_;
  jstring jstrKey = NULL, jstrResult = NULL;

  ComDiagsArea *da = NULL;
  da = ((diagsArea != NULL) ? diagsArea : diagsArea_);

  jstrKey = jni->NewStringUTF(key);
  if(jstrKey == NULL)
    return LM_ERR;

  jstrResult = (jstring) jni->CallStaticObjectMethod(
      (jclass)systemClass_, (jmethodID)systemClearPropId_, jstrKey);

  jni->DeleteLocalRef(jstrKey);
  jni->DeleteLocalRef(jstrResult);

  return LM_OK;

}

// Here is where we massage user-specified JVM options into the set
// that we actually want to pass to the JVM. Note that userOptions
// will not be modified by this method.
void
LmLanguageManagerJava::processJavaOptions(const LmJavaOptions &userOptions,
                                          LmJavaOptions &jvmOptions)
{
  jvmOptions.merge(userOptions);

  // Check if userid information is passed from executor.
  jvmOptions.removeSystemProperty("sqlmx.udr.username", &userName_, collHeap());

  // Check if password information is passed from executor.
  jvmOptions.removeSystemProperty("sqlmx.udr.password", &userPassword_, collHeap());

  // Check if datasource information is passed from executor.
  jvmOptions.removeSystemProperty("sqlmx.datasource",
                                  &datasourceName_,
     	                          collHeap());

  // The system property sqlmx.udr.jrehome has special meaning. If
  // specified it tells us to set the JREHOME environment variable. We
  // look for it here and if found, we set JREHOME and also leave the
  // property definition intact.
  char *userJreHomeValue = NULL;
  NABoolean foundJreHome =
    jvmOptions.removeSystemProperty("sqlmx.udr.jrehome",
                                    &userJreHomeValue,
                                    collHeap());
  if (foundJreHome)
  {
    LM_ASSERT(userJreHomeValue);
    const char *envPrefix = "JREHOME=";

    char *envString = new (collHeap())
      char[str_len(envPrefix) + str_len(userJreHomeValue) + 10];
    str_sprintf(envString, "%s%s", envPrefix, userJreHomeValue);
    putenv(envString);
    NADELETEBASIC(envString, collHeap());

    jvmOptions.addSystemProperty("sqlmx.udr.jrehome", userJreHomeValue);
    NADELETEBASIC(userJreHomeValue, collHeap());
  }

  // On Yosemite, we need to set environment to pick up correct JVM
  // and threads library files. JVM library is included in _RLD_LIB_PATH
  // and threads library is included in _RLD_FIRST_LIB_PATH.
  // SQL/JRT says this property should be defined in any JVM running
  // "within a DBMS". The standard says its value should be
  // "jdbc:default:connection". The connections created using the 
  // above URL (default connections) are only supported from within
  // the SPJ environment via the LmSQLMXDriver Java class.
  jvmOptions.addSystemProperty("sqlj.defaultconnection",
                               "jdbc:default:connection");


} // processJavaOptions()
