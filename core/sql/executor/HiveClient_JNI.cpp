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

#include "QRLogger.h"
#include "Globals.h"
#include "Context.h"
#include "jni.h"
#include "HiveClient_JNI.h"

// ===========================================================================
// ===== Class HiveClient_JNI
// ===========================================================================

JavaMethodInit* HiveClient_JNI::JavaMethods_ = NULL;
jclass HiveClient_JNI::javaClass_ = 0;
bool HiveClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HiveClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const hvcErrorEnumStr[] = 
{
  "Preparing parameters for HiveClient."
 ,"Java exception in close()."
 ,"Preparing parameters for exists()."
 ,"Java exception in exists()."
 ,"Preparing parameters for getHiveTableStr()."
 ,"Java exception in getHiveTableStr()."
 ,"Preparing parameters for getHiveTableParameters()."
 ,"Java exception in getHiveTableParameters()."
 ,"Preparing parameters for getRedefTime()."
 ,"Java exception in getRedefTime()."
 ,"Java exception in getAllSchemas()."
 ,"Preparing parameters for getAllTables()."
 ,"Java exception in getAllTables()."
 ,"Preparing parameters for executeHiveSQL()."
 ,"Java exception in executeHiveSQL()."
};



//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* HiveClient_JNI::getErrorText(HVC_RetCode errEnum)
{
  if (errEnum < (HVC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)hvcErrorEnumStr[errEnum-HVC_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HiveClient_JNI* HiveClient_JNI::getInstance()
{
   HVC_RetCode hvcRetcode = HVC_OK;

   ContextCli *currContext = GetCliGlobals()->currContext();
   HiveClient_JNI *hiveClient_JNI = currContext->getHiveClient();
   if (hiveClient_JNI == NULL)
   { 
       NAHeap *heap = currContext->exHeap();
       hiveClient_JNI = new (heap) HiveClient_JNI(heap);
       if ((hvcRetcode = hiveClient_JNI->init()) == HVC_OK)
          currContext->setHiveClient(hiveClient_JNI);
       else {
          NADELETE(hiveClient_JNI, HiveClient_JNI, heap);
          hiveClient_JNI = NULL;
       }
   }
   return hiveClient_JNI;
}

void HiveClient_JNI::deleteInstance()
{
  ContextCli *currContext = GetCliGlobals()->currContext();
  HiveClient_JNI *hiveClient_JNI = currContext->getHiveClient();
  if (hiveClient_JNI != NULL)
  {
     NAHeap *heap = currContext->exHeap();
     NADELETE(hiveClient_JNI, HiveClient_JNI, heap);
     currContext->setHiveClient(NULL);
  }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HiveClient_JNI::~HiveClient_JNI()
{
   if (isInitialized())	
      close(); // error handling
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HiveClient";
  HVC_RetCode rc;

  if (isInitialized())
    return HVC_OK;
  
  if (javaMethodsInitialized_)
    return (HVC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HVC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_CLOSE      ].jm_name      = "close";	
    JavaMethods_[JM_CLOSE      ].jm_signature = "()Z";
    JavaMethods_[JM_EXISTS     ].jm_name      = "exists";
    JavaMethods_[JM_EXISTS     ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_GET_HVT    ].jm_name      = "getHiveTableString";
    JavaMethods_[JM_GET_HVT    ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";
    JavaMethods_[JM_GET_HVP    ].jm_name      = "getHiveTableParameters";
    JavaMethods_[JM_GET_HVP    ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";
    JavaMethods_[JM_GET_RDT    ].jm_name      = "getRedefTime";
    JavaMethods_[JM_GET_RDT    ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)J";
    JavaMethods_[JM_GET_ASH     ].jm_name      = "getAllSchemas";
    JavaMethods_[JM_GET_ASH     ].jm_signature = "()[Ljava/lang/Object;";
    JavaMethods_[JM_GET_ATL    ].jm_name      = "getAllTables";
    JavaMethods_[JM_GET_ATL    ].jm_signature = "(Ljava/lang/String;)[Ljava/lang/Object;";
    JavaMethods_[JM_EXEC_HIVE_SQL].jm_name = "executeHiveSQL";
    JavaMethods_[JM_EXEC_HIVE_SQL].jm_signature = "(Ljava/lang/String;)V";

    rc = (HVC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == HVC_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::exists(const char* schName, const char* tabName)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HiveClient_JNI::exists(%s, %s) called.", schName, tabName);
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;
  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_EXISTS_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_EXISTS_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_EXISTS_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_EXISTS_PARAM;
  }

  // boolean exists(java.lang.String, java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_EXISTS].jm_full_name;
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_EXISTS].methodID, js_schName, js_tabName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::exists()");
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_EXISTS_EXCEPTION;
  }

  if (jresult == false) {
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE;  // Table does not exist
  }

  jenv_->PopLocalFrame(NULL);
  return HVC_OK;  // Table exists.
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::getHiveTableStr(const char* schName, 
                                            const char* tabName, 
                                            Text& hiveTblStr)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getHiveTableStr(%s, %s, %s).", schName, tabName, hiveTblStr.data());
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;
  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_HVT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_HVT_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_HVT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_HVT_PARAM;
  }

  // java.lang.String getHiveTableString(java.lang.String, java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_GET_HVT].jm_full_name;
  jstring jresult = (jstring)jenv_->CallStaticObjectMethod(javaClass_, 
                                            JavaMethods_[JM_GET_HVT].methodID, 
                                            js_schName, js_tabName);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::getHiveTableStr()");
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_HVT_EXCEPTION;
  }
 
  if (jresult == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE;
  }
  if (jenv_->GetStringLength(jresult) <= 0)
  { 
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE; // Table does not exist
  }
    
  // Not using UFTchars and NAWString for now.
  const char* char_result = jenv_->GetStringUTFChars(jresult, 0);
  hiveTblStr += char_result ; // deep copy. hiveTblStr is assumed to be empty.
  jenv_->ReleaseStringUTFChars(jresult, char_result);

  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getHiveTableStr(%s, %s, %s).", schName, tabName, hiveTblStr.data());
  jenv_->PopLocalFrame(NULL);
  return HVC_OK;  // Table exists.
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::getHiveTableParameters(const char *schName, const char *tabName, Text& hiveParamsStr)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getHiveTableParameters(%s).", hiveParamsStr.data());
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;
  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_HVT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_HVP_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_HVT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_HVP_PARAM;
  }

  // java.lang.String getHiveTableParameters();
  tsRecentJMFromJNI = JavaMethods_[JM_GET_HVP].jm_full_name;
  jstring jresult = (jstring)jenv_->CallStaticObjectMethod(javaClass_, 
                                                     JavaMethods_[JM_GET_HVP].methodID, js_schName, js_tabName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::getHiveTableParameters()");
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_HVT_EXCEPTION;
  }
 
  if (jresult == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE;
  }
  if (jenv_->GetStringLength(jresult) <= 0)
  { 
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE; // Table does not exist
  }
    
  // Not using UFTchars and NAWString for now.
  const char* char_result = jenv_->GetStringUTFChars(jresult, 0);
  hiveParamsStr += char_result ; // deep copy. hiveParamsStr is assumed to be empty.
  jenv_->ReleaseStringUTFChars(jresult, char_result);

  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getHiveTableParameters(%s).", hiveParamsStr.data());
  jenv_->PopLocalFrame(NULL);
  return HVC_OK;  // Table exists.
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////  
HVC_RetCode HiveClient_JNI::getRedefTime(const char* schName, 
                                         const char* tabName, 
                                         Int64& redefTime)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getRedefTime(%s, %s, %lld).", schName, tabName, redefTime);
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;

  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_REDEFTIME_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_REDEFTIME_PARAM;
  }
  jstring js_tabName = jenv_->NewStringUTF(tabName);
  if (js_tabName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_REDEFTIME_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_REDEFTIME_PARAM;
  }

  //  jlong getRedefTime(java.lang.String, java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_GET_RDT].jm_full_name;
  jlong jresult = jenv_->CallStaticLongMethod(javaClass_, 
                                        JavaMethods_[JM_GET_RDT].methodID, 
                                        js_schName, js_tabName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::getRedefTime()");
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_REDEFTIME_EXCEPTION;
  }

  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Exit HiveClient_JNI::getRedefTime(%s, %s, %lld).", schName, tabName, redefTime);

  if (jresult < 0) {
    jenv_->PopLocalFrame(NULL);
    return HVC_DONE; // Table does not exist
  }

  redefTime = jresult ;
  jenv_->PopLocalFrame(NULL);
  return HVC_OK;  // Table exists.
  
}

//////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////// 
HVC_RetCode HiveClient_JNI::getAllSchemas(NAHeap *heap, LIST(Text *)& schNames)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getAllSchemas(%p) called.", (void *) &schNames);
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;

  tsRecentJMFromJNI = JavaMethods_[JM_GET_ASH].jm_full_name;
  jarray j_schNames= 
     (jarray)jenv_->CallStaticObjectMethod(javaClass_, JavaMethods_[JM_GET_ASH].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::getAllSchemas()");
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_ALLSCH_EXCEPTION;
  }

  int numSchemas = convertStringObjectArrayToList(heap, j_schNames,
                   schNames);           
  if (numSchemas == 0) {
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE;
  }
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, 
       "Exit HiveClient_JNI::getAllSchemas(%p) called.", (void *) &schNames);
  jenv_->PopLocalFrame(NULL);
  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////// 
HVC_RetCode HiveClient_JNI::executeHiveSQL(const char* hiveSQL)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HiveClient_JNI::executeHiveSQL(%s) called.", hiveSQL);
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;

  jstring js_hiveSQL = jenv_->NewStringUTF(hiveSQL);
  if (js_hiveSQL == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_ALLTBL_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_EXECUTE_HIVE_SQL_PARAM;
  }
  
  tsRecentJMFromJNI = JavaMethods_[JM_EXEC_HIVE_SQL].jm_full_name;
  jenv_->CallStaticVoidMethod(javaClass_, JavaMethods_[JM_EXEC_HIVE_SQL].methodID, js_hiveSQL);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::executeHiveSQL()",
                        TRUE /*dont return stack details*/);
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_EXECUTE_HIVE_SQL_EXCEPTION;
  }

  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, 
       "Exit HiveClient_JNI::executeHiveSQL(%s) called.", hiveSQL);
  jenv_->PopLocalFrame(NULL);
  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HVC_RetCode HiveClient_JNI::getAllTables(NAHeap *heap, const char* schName, 
                                         LIST(Text *)& tblNames)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HiveClient_JNI::getAllTables(%s, %p) called.", schName, (void *) &tblNames);
  if (initJNIEnv() != JOI_OK)
     return HVC_ERROR_INIT_PARAM;
  if (getInstance() == NULL)
     return HVC_ERROR_INIT_PARAM;

  jstring js_schName = jenv_->NewStringUTF(schName);
  if (js_schName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_GET_ALLTBL_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_ALLTBL_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_GET_ATL].jm_full_name;
  jarray j_tblNames = 
    (jarray)jenv_->CallStaticObjectMethod(javaClass_, JavaMethods_[JM_GET_ATL].methodID, 
                            js_schName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::getAllTables()");
    jenv_->PopLocalFrame(NULL);
    return HVC_ERROR_GET_ALLTBL_EXCEPTION;
  }

  if (j_tblNames == NULL) 	
  {	
     GetCliGlobals()->setJniErrorStr(getErrorText(HVC_ERROR_EXISTS_EXCEPTION));
     jenv_->PopLocalFrame(NULL);	
     return HVC_ERROR_EXISTS_EXCEPTION;	
  }

  int numTables = convertStringObjectArrayToList(heap, j_tblNames,
                   tblNames);           
  if (numTables == 0) {
     jenv_->PopLocalFrame(NULL);
     return HVC_DONE;
  }
  jenv_->PopLocalFrame(NULL);
  return HVC_OK;
}

//////////////////////////////////////////////////////////////////////////////	
// 	
//////////////////////////////////////////////////////////////////////////////	
HVC_RetCode HiveClient_JNI::close()	
{	
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HiveClient_JNI::close() called.");	
	
  if (initJNIEnv() != JOI_OK)	
     return HVC_ERROR_INIT_PARAM;	
	
  // boolean close();	
  tsRecentJMFromJNI = JavaMethods_[JM_CLOSE].jm_full_name;	
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_CLOSE].methodID);	
  if (jenv_->ExceptionCheck())	
  {	
    getExceptionDetails(__FILE__, __LINE__, "HiveClient_JNI::close()");	
    jenv_->PopLocalFrame(NULL);	
    return HVC_ERROR_CLOSE_EXCEPTION;	
  }	
  	
  if (jresult == false) 	
  {	
    logError(CAT_SQL_HBASE, "HiveClient_JNI::close()", getLastError());	
    jenv_->PopLocalFrame(NULL);	
    return HVC_ERROR_CLOSE_EXCEPTION;	
  }	
	
  jenv_->PopLocalFrame(NULL);	
  return HVC_OK;	
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////  
void HiveClient_JNI::logIt(const char* str)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, str);
}
