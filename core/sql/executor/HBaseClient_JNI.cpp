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

#include "Context.h"
#include "Globals.h"
#include "HBaseClient_JNI.h"
#include "QRLogger.h"
#include <signal.h>
#include "pthread.h"
//
// ===========================================================================
// ===== Class HBaseClient_JNI
// ===========================================================================

JavaMethodInit* HBaseClient_JNI::JavaMethods_ = NULL;
jclass HBaseClient_JNI::javaClass_ = 0;
bool HBaseClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HBaseClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

// Keep in sync with HBC_RetCode enum.
static const char* const hbcErrorEnumStr[] = 
{
  "Preparing parameters for initConnection()."
 ,"Java exception in initConnection()."
 ,"Java exception in getHTableClient()."
 ,"Java exception in releaseHTableClient()."
 ,"Preparing parameters for create()."
 ,"Java exception in create()."
 ,"Preparing parameters for alter()."
 ,"Java exception in alter()."
 ,"Preparing parameters for drop()."
 ,"Java exception in drop()."
 ,"Preparing parameters for exists()."
 ,"Java exception in exists()."
 ,"Preparing parameters for grant()."
 ,"Java exception in grant()."
 ,"Preparing parameters for revoke()."
 ,"Java exception in revoke()."
 ,"Error in Thread Create"
 ,"Error in Thread Req Alloc"
 ,"Error in Thread SIGMAS"
 ,"Error in Attach JVM"
 ,"Java exception in getHBulkLoadClient()."
 ,"Preparing parameters for estimateRowCount()."
 ,"Java exception in estimateRowCount()."
 ,"estimateRowCount() returned false."
 ,"Java exception in releaseHBulkLoadClient()."
 ,"Java exception in getBlockCacheFraction()."
 ,"Preparing parameters for getLatestSnapshot()."
 ,"Java exception in getLatestSnapshot()."
 ,"Preparing parameters for cleanSnpTmpLocation()."
 ,"Java exception in cleanSnpTmpLocation()."
 ,"Preparing parameters for setArcPerms()."
 ,"Java exception in setArcPerms()."
 ,"Java exception in startGet()."
 ,"Java exception in startGets()."
 ,"Preparing parameters for getHbaseTableInfo()."
 ,"Java exception in getHbaseTableInfo()."
 ,"preparing parameters for createCounterTable()."
 ,"java exception in createCounterTable()."
 ,"preparing parameters for incrCounter()."
 ,"java exception in incrCounter()."
 ,"Preparing parameters for getRegionsNodeName()."
 ,"Java exception in getRegionsNodeName()."
 ,"Preparing parameters for insertRow()."
 ,"Java exception in insertRow()."
 ,"Dup RowId in insertRow()."
 ,"Preparing parameters for insertRows()."
 ,"Java exception in insertRows()."
 ,"Java exception in checkAndUpdateRow()."
 ,"Preparing parameters for checkAndUpdateRow()."
 ,"Row not found in checkAndUpdateRow()."
 ,"Preparing parameters for deleteRow()."
 ,"Java exception in deleteRow()."
 ,"Preparing parameters for deleteRows()."
 ,"Java exception in deleteRows()."
 ,"Preparing parameters for checkAndDeleteRow()."
 ,"Java exception in checkAndDeleteRow()."
 ,"Row not found in checkAndDeleteRow()."
 ,"Preparing parameters for getKeys()."
 ,"Preparing parameters for listAll()."
 ,"Preparing parameters for getRegionStats()."
 ,"JNI NewStringUTF() in createSnapshot()."
 ,"Java exception in createSnapshot()."
 ,"JNI NewStringUTF() in deleteSnapshot()."
 ,"Java exception in deleteSnapshot()."
 ,"JNI NewStringUTF() in verifySnapshot()."
 ,"Java exception in verifySnapshot()."
 ,"Preparing parameters for truncate()."
 ,"Java exception in truncate()."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
// private default constructor
HBaseClient_JNI::HBaseClient_JNI(NAHeap *heap)
                 :  JavaObjectInterface(heap)
                   ,isConnected_(FALSE)
{
  for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
    threadID_[i] = 0;
  }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* HBaseClient_JNI::getErrorText(HBC_RetCode errEnum)
{
  if (errEnum < (HBC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)hbcErrorEnumStr[errEnum-HBC_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBaseClient_JNI* HBaseClient_JNI::getInstance()
{
   CliGlobals *cliGlobals = GetCliGlobals();
   NAHeap *heap = cliGlobals->getExecutorMemory();
   HBaseClient_JNI *hbaseClient_JNI;
     
   hbaseClient_JNI = cliGlobals->getHBaseClient();
   if (hbaseClient_JNI == NULL) {
      hbaseClient_JNI  = new (heap) HBaseClient_JNI(heap);
      cliGlobals->setHbaseClient(hbaseClient_JNI);
   }
   return hbaseClient_JNI;
}

void HBaseClient_JNI::deleteInstance()
{
   CliGlobals *cliGlobals = GetCliGlobals();
   HBaseClient_JNI *hbaseClient_JNI = cliGlobals->getHBaseClient();
   if (hbaseClient_JNI != NULL)
   {
      NADELETE(hbaseClient_JNI, HBaseClient_JNI, cliGlobals->getExecutorMemory());
      cliGlobals->setHbaseClient(NULL);
   }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBaseClient_JNI::~HBaseClient_JNI()
{
  //QRLogger::log(CAT_JNI_TOP, LL_DEBUG, "HBaseClient_JNI destructor called.");
  
  // worker threads need to go away and be joined. 
  if (threadID_[0])
  {
    // tell the worker threads to go away
    for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
      enqueueShutdownRequest(); 
    }

    // wait for worker threads to exit and join
    for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
      pthread_join(threadID_[i], NULL);
    }

    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&workBell_);
  }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HBaseClient";
  HBC_RetCode rc;
  
  if (isInitialized())
    return HBC_OK;
  
  if (javaMethodsInitialized_)
    return (HBC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HBC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_INIT       ].jm_name      = "init";
    JavaMethods_[JM_INIT       ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_GET_HTC    ].jm_name      = "getHTableClient";
    JavaMethods_[JM_GET_HTC    ].jm_signature = "(JLjava/lang/String;Z)Lorg/trafodion/sql/HTableClient;";
    JavaMethods_[JM_REL_HTC    ].jm_name      = "releaseHTableClient";
    JavaMethods_[JM_REL_HTC    ].jm_signature = "(Lorg/trafodion/sql/HTableClient;)V";
    JavaMethods_[JM_CREATE     ].jm_name      = "create";
    JavaMethods_[JM_CREATE     ].jm_signature = "(Ljava/lang/String;[Ljava/lang/Object;Z)Z";
    JavaMethods_[JM_CREATEK    ].jm_name      = "createk";
    JavaMethods_[JM_CREATEK    ].jm_signature = "(Ljava/lang/String;[Ljava/lang/Object;[Ljava/lang/Object;JIIZ)Z";
    JavaMethods_[JM_TRUNCABORT ].jm_name      = "registerTruncateOnAbort";
    JavaMethods_[JM_TRUNCABORT ].jm_signature = "(Ljava/lang/String;J)Z";
    JavaMethods_[JM_ALTER      ].jm_name      = "alter";
    JavaMethods_[JM_ALTER      ].jm_signature = "(Ljava/lang/String;[Ljava/lang/Object;J)Z";
    JavaMethods_[JM_DROP       ].jm_name      = "drop";
    JavaMethods_[JM_DROP       ].jm_signature = "(Ljava/lang/String;J)Z";
    JavaMethods_[JM_DROP_ALL       ].jm_name      = "dropAll";
    JavaMethods_[JM_DROP_ALL       ].jm_signature = "(Ljava/lang/String;J)Z";
    JavaMethods_[JM_LIST_ALL       ].jm_name      = "listAll";
    JavaMethods_[JM_LIST_ALL       ].jm_signature = "(Ljava/lang/String;)[[B";
    JavaMethods_[JM_GET_REGION_STATS       ].jm_name      = "getRegionStats";
    JavaMethods_[JM_GET_REGION_STATS       ].jm_signature = "(Ljava/lang/String;)[[B";
    JavaMethods_[JM_COPY       ].jm_name      = "copy";
    JavaMethods_[JM_COPY       ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;Z)Z";
    JavaMethods_[JM_EXISTS     ].jm_name      = "exists";
    JavaMethods_[JM_EXISTS     ].jm_signature = "(Ljava/lang/String;J)Z";
    JavaMethods_[JM_GRANT      ].jm_name      = "grant";
    JavaMethods_[JM_GRANT      ].jm_signature = "([B[B[Ljava/lang/Object;)Z";
    JavaMethods_[JM_REVOKE     ].jm_name      = "revoke";
    JavaMethods_[JM_REVOKE     ].jm_signature = "([B[B[Ljava/lang/Object;)Z";
    JavaMethods_[JM_GET_HBLC   ].jm_name      = "getHBulkLoadClient";
    JavaMethods_[JM_GET_HBLC   ].jm_signature = "()Lorg/trafodion/sql/HBulkLoadClient;";
    JavaMethods_[JM_EST_RC     ].jm_name      = "estimateRowCount";
    JavaMethods_[JM_EST_RC     ].jm_signature = "(Ljava/lang/String;III[J)Z";
    JavaMethods_[JM_EST_RC_COPROC     ].jm_name      = "estimateRowCountViaCoprocessor";
    JavaMethods_[JM_EST_RC_COPROC     ].jm_signature = "(Ljava/lang/String;III[J)Z";
    JavaMethods_[JM_REL_HBLC   ].jm_name      = "releaseHBulkLoadClient";
    JavaMethods_[JM_REL_HBLC   ].jm_signature = "(Lorg/trafodion/sql/HBulkLoadClient;)V";
    JavaMethods_[JM_GET_CAC_FRC].jm_name      = "getBlockCacheFraction";
    JavaMethods_[JM_GET_CAC_FRC].jm_signature = "()F";
    JavaMethods_[JM_GET_LATEST_SNP].jm_name      = "getLatestSnapshot";
    JavaMethods_[JM_GET_LATEST_SNP].jm_signature = "(Ljava/lang/String;)Ljava/lang/String;";
    JavaMethods_[JM_CLEAN_SNP_TMP_LOC].jm_name      = "cleanSnpScanTmpLocation";
    JavaMethods_[JM_CLEAN_SNP_TMP_LOC].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_SET_ARC_PERMS].jm_name      = "setArchivePermissions";
    JavaMethods_[JM_SET_ARC_PERMS].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_START_GET].jm_name      = "startGet";
    JavaMethods_[JM_START_GET].jm_signature = "(JLjava/lang/String;ZJ[B[Ljava/lang/Object;J)I";
    JavaMethods_[JM_START_GETS].jm_name      = "startGet";
    JavaMethods_[JM_START_GETS].jm_signature = "(JLjava/lang/String;ZJ[Ljava/lang/Object;[Ljava/lang/Object;J)I";
    JavaMethods_[JM_START_DIRECT_GETS].jm_name      = "startGet";
    JavaMethods_[JM_START_DIRECT_GETS].jm_signature = "(JLjava/lang/String;ZJSLjava/lang/Object;[Ljava/lang/Object;)I";
    JavaMethods_[JM_GET_HBTI].jm_name      = "getHbaseTableInfo";
    JavaMethods_[JM_GET_HBTI].jm_signature = "(Ljava/lang/String;[I)Z";
    JavaMethods_[JM_CREATE_COUNTER_TABLE ].jm_name      = "createCounterTable";
    JavaMethods_[JM_CREATE_COUNTER_TABLE ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_INCR_COUNTER         ].jm_name      = "incrCounter";
    JavaMethods_[JM_INCR_COUNTER         ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;J)J";
    JavaMethods_[JM_GET_REGN_NODES].jm_name      = "getRegionsNodeName";
    JavaMethods_[JM_GET_REGN_NODES].jm_signature = "(Ljava/lang/String;[Ljava/lang/String;)Z";
    JavaMethods_[JM_HBC_DIRECT_INSERT_ROW].jm_name      = "insertRow";
    JavaMethods_[JM_HBC_DIRECT_INSERT_ROW].jm_signature = "(JLjava/lang/String;ZJ[BLjava/lang/Object;JZSZZ)Z";
    JavaMethods_[JM_HBC_DIRECT_INSERT_ROWS].jm_name      = "insertRows";
    JavaMethods_[JM_HBC_DIRECT_INSERT_ROWS].jm_signature = "(JLjava/lang/String;ZJSLjava/lang/Object;Ljava/lang/Object;JZ)Z";
    JavaMethods_[JM_HBC_DIRECT_CHECKANDUPDATE_ROW].jm_name      = "checkAndUpdateRow";
    JavaMethods_[JM_HBC_DIRECT_CHECKANDUPDATE_ROW].jm_signature = "(JLjava/lang/String;ZJ[BLjava/lang/Object;[B[BJZZ)Z";
    JavaMethods_[JM_HBC_DELETE_ROW ].jm_name      = "deleteRow";
    JavaMethods_[JM_HBC_DELETE_ROW ].jm_signature = "(JLjava/lang/String;ZJ[B[Ljava/lang/Object;JZZ)Z";
    JavaMethods_[JM_HBC_DIRECT_DELETE_ROWS ].jm_name      = "deleteRows";
    JavaMethods_[JM_HBC_DIRECT_DELETE_ROWS ].jm_signature = "(JLjava/lang/String;ZJSLjava/lang/Object;JZ)Z";
    JavaMethods_[JM_HBC_CHECKANDDELETE_ROW ].jm_name      = "checkAndDeleteRow";
    JavaMethods_[JM_HBC_CHECKANDDELETE_ROW ].jm_signature = "(JLjava/lang/String;ZJ[B[B[BJZZ)Z";
    JavaMethods_[JM_HBC_GETSTARTKEYS ].jm_name      = "getStartKeys";
    JavaMethods_[JM_HBC_GETSTARTKEYS ].jm_signature = "(Ljava/lang/String;Z)[[B";
    JavaMethods_[JM_HBC_GETENDKEYS ].jm_name      = "getEndKeys";
    JavaMethods_[JM_HBC_GETENDKEYS ].jm_signature = "(Ljava/lang/String;Z)[[B";
    JavaMethods_[JM_HBC_CREATE_SNAPSHOT].jm_name      = "createSnapshot";
    JavaMethods_[JM_HBC_CREATE_SNAPSHOT].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_HBC_DELETE_SNAPSHOT].jm_name      = "deleteSnapshot";
    JavaMethods_[JM_HBC_DELETE_SNAPSHOT].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_HBC_VERIFY_SNAPSHOT].jm_name      = "verifySnapshot";
    JavaMethods_[JM_HBC_VERIFY_SNAPSHOT].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_TRUNCATE   ].jm_name      = "truncate";
    JavaMethods_[JM_TRUNCATE   ].jm_signature = "(Ljava/lang/String;ZJ)Z";
    rc = (HBC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == HBC_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::initConnection(const char* zkServers, const char* zkPort)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::initConnection(%s, %s) called.", zkServers, zkPort);

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_zkServers = jenv_->NewStringUTF(zkServers);
  if (js_zkServers == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INIT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INIT_PARAM;
  }
  jstring js_zkPort = jenv_->NewStringUTF(zkPort);
  if (js_zkPort == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INIT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INIT_PARAM;
  }
  tsRecentJMFromJNI = JavaMethods_[JM_INIT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_INIT].methodID, js_zkServers, js_zkPort);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::initConnection()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INIT_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::initConnection()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INIT_EXCEPTION;
  }

  isConnected_ = TRUE;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTableClient_JNI* HBaseClient_JNI::getHTableClient(NAHeap *heap, const char* tableName, bool useTRex, ExHbaseAccessStats *hbs)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::getHTableClient(%s) called.", tableName);

  if (javaObj_ == NULL || (!isInitialized()))
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HTC_EXCEPTION));
    return NULL;
  }
  if (initJNIEnv() != JOI_OK)
     return NULL;
  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HTC_EXCEPTION));
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  HTableClient_JNI *htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
  if (htc->init() != HTC_OK)
  {
     NADELETE(htc, HTableClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return NULL;
  }
  htc->setTableName(tableName);
  htc->setHbaseStats(hbs);

  tsRecentJMFromJNI = JavaMethods_[JM_GET_HTC].jm_full_name;
  jobject j_htc = jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HTC].methodID, 
	(jlong)htc, js_tblName, (jboolean)useTRex);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getHTableClient()");
    NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  if (j_htc == NULL) 
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getHTableClient()");
    NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
  }
  htc->setJavaObject(j_htc);
  if (htc->init() != HTC_OK)
  {
     jenv_->PopLocalFrame(NULL);
     releaseHTableClient(htc);
     return NULL;
  }
  jenv_->PopLocalFrame(NULL);
  return htc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::releaseHTableClient(HTableClient_JNI* htc)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::releaseHTableClient() called.");

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jobject j_htc = htc->getJavaObject();
  if (j_htc != (jobject)-1) {
      tsRecentJMFromJNI = JavaMethods_[JM_REL_HTC].jm_full_name;
      jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_REL_HTC].methodID, j_htc);
      if (jenv_->ExceptionCheck()) {
         getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::releaseHTableClient()");
         jenv_->PopLocalFrame(NULL);
         return HBC_ERROR_REL_HTC_EXCEPTION;
      }
  }
  NADELETE(htc, HTableClient_JNI, htc->getHeap()); 
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HBulkLoadClient_JNI* HBaseClient_JNI::getHBulkLoadClient(NAHeap *heap)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::getHBulkLoadClient() called.");
  if (javaObj_ == NULL || (!isInitialized()))
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HBLC_EXCEPTION));
    return NULL;
  }
  if (initJNIEnv() != JOI_OK)
     return NULL;

  tsRecentJMFromJNI = JavaMethods_[JM_GET_HBLC].jm_full_name;
  jobject j_hblc = jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HBLC].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getHBulkLoadClient()");
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
  HBulkLoadClient_JNI *hblc = new (heap) HBulkLoadClient_JNI(heap, j_hblc);
  if ( hblc->init()!= HBLC_OK)
  {
     NADELETE(hblc, HBulkLoadClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return NULL; 
  }
  jenv_->PopLocalFrame(NULL);
  return hblc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::releaseHBulkLoadClient(HBulkLoadClient_JNI* hblc)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::releaseHBulkLoadClient() called.");

  jobject j_hblc = hblc->getJavaObject();

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  tsRecentJMFromJNI = JavaMethods_[JM_REL_HBLC].jm_full_name;
  jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_REL_HBLC].methodID, j_hblc);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::releaseHBulkLoadClient()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_REL_HBLC_EXCEPTION;
  }
  NADELETE(hblc, HBulkLoadClient_JNI, hblc->getHeap());
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::create(const char* fileName, HBASE_NAMELIST& colFamilies, NABoolean isMVCC)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::create(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_PARAM;
  }
  jobjectArray j_fams = convertToStringObjectArray(colFamilies);
  if (j_fams == NULL)
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_PARAM;
  }
    
  jboolean j_isMVCC = isMVCC;
  tsRecentJMFromJNI = JavaMethods_[JM_CREATE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
        JavaMethods_[JM_CREATE].methodID, js_fileName, j_fams, j_isMVCC);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::create()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::create()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::create(const char* fileName, 
                                    NAText* createOptionsArray,
                                    int numSplits, int keyLength,
                                    const char ** splitValues,
                                    Int64 transID,
                                    NABoolean isMVCC)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::create(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_PARAM;
  }
  jobjectArray j_opts = convertToStringObjectArray(createOptionsArray, 
                   HBASE_MAX_OPTIONS);
  if (j_opts == NULL)
  {
     getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::create()");
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_PARAM;
  }

  jobjectArray j_keys = NULL;    
  if (numSplits > 0)
  {
     j_keys = convertToByteArrayObjectArray(splitValues, numSplits, keyLength);
     if (j_keys == NULL)
     {
        getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::create()");
        jenv_->PopLocalFrame(NULL);
        return HBC_ERROR_CREATE_PARAM;
     }
  }
  jlong j_tid = transID;
  jint j_numSplits = numSplits;
  jint j_keyLength = keyLength;

  jboolean j_isMVCC = isMVCC;
  tsRecentJMFromJNI = JavaMethods_[JM_CREATEK].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
          JavaMethods_[JM_CREATEK].methodID, js_fileName, j_opts, j_keys, j_tid, j_numSplits, j_keyLength, j_isMVCC);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::create()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::create()", getLastError());
    GetCliGlobals()->setJniErrorStr(getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::alter(const char* fileName,
                                   NAText* createOptionsArray,
                                   Int64 transID)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::alter(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_ALTER_PARAM;
  }
  jobjectArray j_opts = convertToStringObjectArray(createOptionsArray, 
                   HBASE_MAX_OPTIONS);
  if (jenv_->ExceptionCheck())
  {
     getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::alter()");
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_ALTER_PARAM;
  }

  jlong j_tid = transID;

  tsRecentJMFromJNI = JavaMethods_[JM_ALTER].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
          JavaMethods_[JM_ALTER].methodID, js_fileName, j_opts, j_tid);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::alter()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_ALTER_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::alter()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_ALTER_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HBaseClientRequest::HBaseClientRequest(NAHeap *heap, HBaseClientReqType reqType)
                    :  heap_(heap)
                      ,reqType_(reqType)
                      ,fileName_(NULL)
{
}

HBaseClientRequest::~HBaseClientRequest()
{
  if (fileName_) {
    NADELETEBASIC(fileName_, heap_);
  }
}

void HBaseClientRequest::setFileName(const char *fileName)
{
  int len = strlen(fileName);
  fileName_ = new (heap_) char[len + 1];
  strcpy(fileName_, fileName);
}

HBC_RetCode HBaseClient_JNI::enqueueRequest(HBaseClientRequest *request)
{
  pthread_mutex_lock( &mutex_ );
  reqQueue_.push_back(request);
  pthread_cond_signal(&workBell_);
  pthread_mutex_unlock( &mutex_ );

  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::enqueueDropRequest(const char* fileName)
{
  HBaseClientRequest *request = new (heap_) HBaseClientRequest(heap_, HBC_Req_Drop);

  if (!request) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_REQ_ALLOC));
    return HBC_ERROR_THREAD_REQ_ALLOC; 
  }

  request->setFileName(fileName);

  enqueueRequest(request);

  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::enqueueShutdownRequest()
{
  HBaseClientRequest *request = new (heap_) HBaseClientRequest(heap_, HBC_Req_Shutdown);

  if (!request) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_REQ_ALLOC));
    return HBC_ERROR_THREAD_REQ_ALLOC;
  }

  enqueueRequest(request);

  return HBC_OK;
}

HBaseClientRequest* HBaseClient_JNI::getHBaseRequest() 
{
  HBaseClientRequest *request;
  reqList_t::iterator it;

  pthread_mutex_lock( &mutex_ );
  it = reqQueue_.begin();

  request = NULL;

  while (request == NULL)
  {
    if (it != reqQueue_.end())
    {
      request = *it;
      it = reqQueue_.erase(it);
    } else {
      pthread_cond_wait(&workBell_, &mutex_);
      it = reqQueue_.begin();
    }
  }

  pthread_mutex_unlock( &mutex_ );

  return request;
}

HBC_RetCode HBaseClient_JNI::performRequest(HBaseClientRequest *request, JNIEnv* jenv)
{
  switch (request->reqType_)
  {
    case HBC_Req_Drop :
	  drop(request->fileName_, jenv, 0);
	  break;
	default :
	  break;
  }

  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::doWorkInThread() 
{
  int rc;

  HBaseClientRequest *request;

  // mask all signals
  sigset_t mask;
  sigfillset(&mask);
  rc = pthread_sigmask(SIG_BLOCK, &mask, NULL);
  if (rc)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_SIGMASK));
    return HBC_ERROR_THREAD_SIGMASK;
  }

  JNIEnv* jenv; // thread local
  jint result = jvm_->GetEnv((void**) &jenv, JNI_VERSION_1_6);
  switch (result)
  {
    case JNI_OK:
	  break;

	case JNI_EDETACHED:
	  result = jvm_->AttachCurrentThread((void**) &jenv, NULL);
          if (result != JNI_OK)
          {
             GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_ATTACH_JVM));
	     return HBC_ERROR_ATTACH_JVM;
          }
      break;

	default: 
	  break;
  }

  // enter processing zone
  for (;;)
  {
    request = getHBaseRequest();

	if (request->isShutDown()) {
	  // wake up another thread as this wakeup could have consumed
	  // multiple workBell_ rings.
	  pthread_cond_signal(&workBell_);
	  break;
	} else {
      performRequest(request, jenv);
	  NADELETE(request, HBaseClientRequest, request->getHeap());
	}
  }

  jvm_->DetachCurrentThread();

  pthread_exit(0);

  return HBC_OK;
}

static void *workerThreadMain_JNI(void *arg)
{
  // parameter passed to the thread is an instance of the HBaseClient_JNI object
  HBaseClient_JNI *client = (HBaseClient_JNI *)arg;

  client->doWorkInThread();

  return NULL;
}

HBC_RetCode HBaseClient_JNI::startWorkerThreads()
{
  pthread_mutexattr_t mutexAttr;
  pthread_mutexattr_init( &mutexAttr );
  pthread_mutex_init( &mutex_, &mutexAttr );
  pthread_cond_init( &workBell_, NULL );

  int rc;
  for (int i=0; i<NUM_HBASE_WORKER_THREADS; i++) {
    rc = pthread_create(&threadID_[i], NULL, workerThreadMain_JNI, this);
    if (rc != 0)
    {
       GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_THREAD_CREATE));
       return HBC_ERROR_THREAD_CREATE;
    }
  }

  return HBC_OK;
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::drop(const char* fileName, bool async, long transID)
{
  if (async) {
    if (!threadID_[0]) {
	  startWorkerThreads();
	}
    enqueueDropRequest(fileName);
  } else {
    return drop(fileName, jenv_, transID); // not in worker thread
  }

  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::registerTruncateOnAbort(const char* fileName, Int64 transID)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::drop(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_PARAM;
  }

  jlong j_tid = transID;

  tsRecentJMFromJNI = JavaMethods_[JM_TRUNCABORT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_TRUNCABORT].methodID, js_fileName, j_tid);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::drop()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::truncate(const char* fileName, NABoolean preserveSplits, Int64 transID)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::truncate(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_TRUNCATE_PARAM;
  }
  jboolean j_preserveSplits = preserveSplits;
  jlong j_tid = transID;  
  // boolean drop(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_TRUNCATE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_TRUNCATE].methodID, js_fileName, j_preserveSplits, j_tid);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::truncate()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_TRUNCATE_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::drop(const char* fileName, JNIEnv* jenv, Int64 transID)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::drop(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_PARAM;
  }

  jlong j_tid = transID;  
  // boolean drop(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_DROP].jm_full_name;
  jboolean jresult = jenv->CallBooleanMethod(javaObj_, JavaMethods_[JM_DROP].methodID, js_fileName, j_tid);

  if (jenv->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::drop()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::dropAll(const char* pattern, bool async, Int64 transID)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::dropAll(%s) called.", pattern);

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  if (async) {
    // not supported yet.
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_PARAM;
  }

  jstring js_pattern = jenv_->NewStringUTF(pattern);
  if (js_pattern == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_PARAM;
  }

  jlong j_tid = transID;  

  // boolean drop(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_DROP_ALL].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DROP_ALL].methodID, js_pattern, j_tid);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::dropAll()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
NAArray<HbaseStr>* HBaseClient_JNI::listAll(NAHeap *heap, const char* pattern)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::listAll(%s) called.", pattern);

  if (initJNIEnv() != JOI_OK)
     return NULL;

  jstring js_pattern = jenv_->NewStringUTF(pattern);
  if (js_pattern == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_LISTALL));
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_LIST_ALL].jm_full_name;
  jarray j_hbaseTables = 
    (jarray)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_LIST_ALL].methodID, js_pattern);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::listAll()");
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  if (j_hbaseTables  == NULL) {
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  NAArray<HbaseStr> *hbaseTables;
  jint retcode = convertByteArrayObjectArrayToNAArray(heap, j_hbaseTables, &hbaseTables);
  jenv_->PopLocalFrame(NULL);
  if (retcode == 0)
     return NULL;
  else
     return hbaseTables;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
NAArray<HbaseStr>* HBaseClient_JNI::getRegionStats(NAHeap *heap, const char* tblName)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::getRegionStats(%s) called.", tblName);

  if (initJNIEnv() != JOI_OK)
     return NULL;

  jstring js_tblName = NULL;
  if (tblName) {
    js_tblName = jenv_->NewStringUTF(tblName);
    if (js_tblName == NULL) 
      {
        GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_REGION_STATS));
        jenv_->PopLocalFrame(NULL);
        return NULL;
      }
  }

  tsRecentJMFromJNI = JavaMethods_[JM_GET_REGION_STATS].jm_full_name;
  jarray j_regionInfo = 
    (jarray)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_REGION_STATS].methodID, js_tblName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getRegionStats()");
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  if (j_regionInfo == NULL) {
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }

  NAArray<HbaseStr> *regionInfo;
  jint retcode = convertByteArrayObjectArrayToNAArray(heap, j_regionInfo, &regionInfo);

  jenv_->PopLocalFrame(NULL);
  if (retcode == 0)
     return NULL;
  else
     return regionInfo;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::copy(const char* srcTblName, 
                                  const char* tgtTblName,
                                  NABoolean force)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::copy(%s,%s) called.", srcTblName, tgtTblName);

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_srcTblName = jenv_->NewStringUTF(srcTblName);
  if (js_srcTblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_PARAM;
  }

  jstring js_tgtTblName = jenv_->NewStringUTF(tgtTblName);
  if (js_tgtTblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DROP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_PARAM;
  }

  jboolean j_force = force;
  tsRecentJMFromJNI = JavaMethods_[JM_COPY].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(
       javaObj_, JavaMethods_[JM_COPY].methodID, 
       js_srcTblName, js_tgtTblName, j_force);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::copy()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::copy()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DROP_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::exists(const char* fileName, Int64 transID)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::exists(%s) called.", fileName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_fileName = jenv_->NewStringUTF(fileName);
  if (js_fileName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_EXISTS_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_EXISTS_PARAM;
  }

  jlong j_tid = transID;  

  // boolean exists(java.lang.String);
  tsRecentJMFromJNI = JavaMethods_[JM_EXISTS].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_EXISTS].methodID, js_fileName, j_tid);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::exists()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_EXISTS_EXCEPTION;
  }

  if (jresult == false) {
     jenv_->PopLocalFrame(NULL);
     return HBC_DONE;  // Table does not exist
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;  // Table exists.
}


//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::grant(const Text& user, const Text& tblName, const TextVec& actions)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::grant(%s, %s, %s) called.", user.data(), tblName.data(), actions.data());
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  int len = user.size();
  jbyteArray jba_user = jenv_->NewByteArray(len);
  if (jba_user == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GRANT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GRANT_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_user, 0, len, (const jbyte*)user.data());

  len = tblName.size();
  jbyteArray jba_tblName = jenv_->NewByteArray(len);
  if (jba_tblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GRANT_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GRANT_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_tblName, 0, len, (const jbyte*)tblName.data());

  jobjectArray j_actionCodes = NULL;
  if (!actions.empty())
  {
    QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "  Adding %d actions.", actions.size());
    j_actionCodes = convertToStringObjectArray(actions);
    if (j_actionCodes == NULL)
    {
       getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::grant()");
       jenv_->PopLocalFrame(NULL);
       return HBC_ERROR_GRANT_PARAM;
    }
  }
  tsRecentJMFromJNI = JavaMethods_[JM_GRANT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
       JavaMethods_[JM_GRANT].methodID, jba_user, jba_tblName, j_actionCodes);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::grant()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GRANT_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::grant()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GRANT_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// Estimate row count for tblName by adding the entry counts from the trailer
// block of each HFile for the table, and dividing by the number of columns.
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::estimateRowCount(const char* tblName,
                                              Int32 partialRowSize,
                                              Int32 numCols,
                                              Int32 retryLimitMilliSeconds,
                                              NABoolean useCoprocessor,
                                              Int64& rowCount,
                                              Int32& breadCrumb)
{
  // Note: Please use HBC_ERROR_ROWCOUNT_EST_EXCEPTION only for
  // those error returns that call getExceptionDetails(). This
  // tells the caller that Java exception information is available.

  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::estimateRowCount(%s) called.", tblName);
  breadCrumb = 1;
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  breadCrumb = 3;
  jstring js_tblName = jenv_->NewStringUTF(tblName);
  if (js_tblName == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_ROWCOUNT_EST_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_ROWCOUNT_EST_PARAM;
  }

  jint jPartialRowSize = partialRowSize;
  jint jNumCols = numCols;
  jint jRetryLimitMilliSeconds = retryLimitMilliSeconds;
  jlongArray jRowCount = jenv_->NewLongArray(1);
  enum JAVA_METHODS method = (useCoprocessor ? JM_EST_RC_COPROC : JM_EST_RC);
  tsRecentJMFromJNI = JavaMethods_[method].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[method].methodID,
                                              js_tblName, jPartialRowSize,
                                              jNumCols, jRetryLimitMilliSeconds, jRowCount);
  jboolean isCopy;
  jlong* arrayElems = jenv_->GetLongArrayElements(jRowCount, &isCopy);
  rowCount = *arrayElems;
  if (isCopy == JNI_TRUE)
    jenv_->ReleaseLongArrayElements(jRowCount, arrayElems, JNI_ABORT);

  breadCrumb = 4;
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::estimateRowCount()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_ROWCOUNT_EST_EXCEPTION;
  }

  breadCrumb = 5;
  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::estimateRowCount() returned false", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_ROWCOUNT_EST_FALSE;
  }

  breadCrumb = 6;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;  // Table exists.
}

HBC_RetCode HBaseClient_JNI::getLatestSnapshot(const char * tblName, char *& snapshotName, NAHeap * heap)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::getLatestSnapshot(%s) called.", tblName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_tblName = jenv_->NewStringUTF(tblName);
  if (js_tblName == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_LATEST_SNP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_LATEST_SNP_PARAM;
  }
  tsRecentJMFromJNI = JavaMethods_[JM_GET_LATEST_SNP].jm_full_name;
  jstring jresult = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_LATEST_SNP].methodID,js_tblName);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getLatestSnapshot()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_LATEST_SNP_EXCEPTION;
  }

  if (jresult == NULL)
    snapshotName = NULL;
  else
  {
    char * tmp = (char*)jenv_->GetStringUTFChars(jresult, NULL);
    snapshotName = new (heap) char[strlen(tmp)+1];
    strcpy(snapshotName, tmp);
    jenv_->ReleaseStringUTFChars(jresult, tmp);
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;  
}
HBC_RetCode HBaseClient_JNI::cleanSnpTmpLocation(const char * path)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::cleanSnpTmpLocation(%s) called.", path);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CLEAN_SNP_TMP_LOC_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CLEAN_SNP_TMP_LOC_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_CLEAN_SNP_TMP_LOC].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_CLEAN_SNP_TMP_LOC].methodID,js_path);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::cleanupSnpTmpLocation()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CLEAN_SNP_TMP_LOC_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::setArchivePermissions(const char * tbl)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::setArchivePermissions(%s) called.", tbl);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_tbl = jenv_->NewStringUTF(tbl);
  if (js_tbl == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_SET_ARC_PERMS_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_SET_ARC_PERMS_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_SET_ARC_PERMS].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_SET_ARC_PERMS].methodID,js_tbl);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::setArchivePermissions()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_SET_ARC_PERMS_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::getBlockCacheFraction(float& frac)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, 
                 "HBaseClient_JNI::getBlockCacheFraction() called.");
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  tsRecentJMFromJNI = JavaMethods_[JM_GET_CAC_FRC].jm_full_name;
  jfloat jresult = jenv_->CallFloatMethod(javaObj_, 
                                          JavaMethods_[JM_GET_CAC_FRC].methodID);
  
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getBlockCacheFraction()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_CACHE_FRAC_EXCEPTION;
  }
  frac = jresult;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
JavaMethodInit* HBulkLoadClient_JNI::JavaMethods_ = NULL;
jclass HBulkLoadClient_JNI::javaClass_ = 0;
bool HBulkLoadClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HBulkLoadClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;


static const char* const hblcErrorEnumStr[] = ///need to update content
{
    "preparing parameters for init."
   ,"java exception in init."
   ,"java exception in cleanup."
   ,"java exception in close"
   ,"java exception in create_hfile()."
   ,"java exception in create_hfile()."
   ,"preparing parameters for add_to_hfile()."
   ,"java exception in add_to_hfile()."
   ,"preparing parameters for hblc_error_close_hfile()."
   ,"java exception in close_hfile()."
   ,"java exception in do_bulkload()."
   ,"java exception in do_bulkload()."
   ,"preparing parameters for bulkload_cleanup()."
   ,"java exception in bulkload_cleanup()."
   ,"preparing parameters for init_hblc()."
   ,"java exception in init_hblc()."
};
HBLC_RetCode HBulkLoadClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HBulkLoadClient";
  HBLC_RetCode rc;

  if (isInitialized())
    return HBLC_OK;

  if (javaMethodsInitialized_)
    return (HBLC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HBLC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];

    JavaMethods_[JM_CTOR       ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR       ].jm_signature = "()V";
    JavaMethods_[JM_INIT_HFILE_PARAMS     ].jm_name      = "initHFileParams";
    JavaMethods_[JM_INIT_HFILE_PARAMS     ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_CLOSE_HFILE      ].jm_name      = "closeHFile";
    JavaMethods_[JM_CLOSE_HFILE      ].jm_signature = "()Z";
    JavaMethods_[JM_DO_BULK_LOAD     ].jm_name      = "doBulkLoad";
    JavaMethods_[JM_DO_BULK_LOAD     ].jm_signature = "(Ljava/lang/String;Ljava/lang/String;ZZ)Z";
    JavaMethods_[JM_BULK_LOAD_CLEANUP].jm_name      = "bulkLoadCleanup";
    JavaMethods_[JM_BULK_LOAD_CLEANUP].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_ADD_TO_HFILE_DB  ].jm_name      = "addToHFile";
    JavaMethods_[JM_ADD_TO_HFILE_DB  ].jm_signature = "(SLjava/lang/Object;Ljava/lang/Object;)Z";

    rc = (HBLC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == HBLC_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

char* HBulkLoadClient_JNI::getErrorText(HBLC_RetCode errEnum)
{
  if (errEnum < (HBLC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else
    return (char*)hblcErrorEnumStr[errEnum-HBLC_FIRST-1];
}

HBLC_RetCode HBulkLoadClient_JNI::initHFileParams(
                        const HbaseStr &tblName,
                        const Text& hFileLoc,
                        const Text& hfileName,
                        Int64 maxHFileSize,
                        const char* sampleTblName,
                        const char* hiveDDL)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::initHFileParams(%s, %s, %s, %ld, %s, %s) called.",
                hFileLoc.data(), hfileName.data(), tblName.val, maxHFileSize, sampleTblName, hiveDDL);

  if (initJNIEnv() != JOI_OK)
     return HBLC_ERROR_INIT_PARAM;

  jstring js_hFileLoc = jenv_->NewStringUTF(hFileLoc.c_str());
   if (js_hFileLoc == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBLC_ERROR_CREATE_HFILE_PARAM;
   }
  jstring js_hfileName = jenv_->NewStringUTF(hfileName.c_str());
   if (js_hfileName == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBLC_ERROR_CREATE_HFILE_PARAM;
   }
   jstring js_tabName = jenv_->NewStringUTF(tblName.val);
    if (js_tabName == NULL)
    {
      GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
      jenv_->PopLocalFrame(NULL);
      return HBLC_ERROR_CREATE_HFILE_PARAM;
    }
  jstring js_sampleTblName = jenv_->NewStringUTF(sampleTblName);
  if (js_sampleTblName == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CREATE_HFILE_PARAM;
  }
  jstring js_hiveDDL = jenv_->NewStringUTF(hiveDDL);
  if (js_hiveDDL == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_CREATE_HFILE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CREATE_HFILE_PARAM;
  }
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::initHFileParams()");
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CREATE_HFILE_EXCEPTION;
  }

  jlong j_maxSize = maxHFileSize;

  tsRecentJMFromJNI = JavaMethods_[JM_INIT_HFILE_PARAMS].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_INIT_HFILE_PARAMS].methodID, js_hFileLoc,
                                              js_hfileName, j_maxSize, js_tabName, js_sampleTblName, js_hiveDDL);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::initHFileParams()");
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CREATE_HFILE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBulkLoadClient_JNI::initHFileParams()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CREATE_HFILE_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBLC_OK;
}

HBLC_RetCode HBulkLoadClient_JNI::addToHFile( short rowIDLen, HbaseStr &rowIDs,
            HbaseStr &rows, ExHbaseAccessStats *hbs)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::addToHFile called.");
  if (initJNIEnv() != JOI_OK)
     return HBLC_ERROR_INIT_PARAM;

  jobject jRowIDs = jenv_->NewDirectByteBuffer(rowIDs.val, rowIDs.len);
  if (jRowIDs == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_ADD_TO_HFILE_EXCEPTION));
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }
  jobject jRows = jenv_->NewDirectByteBuffer(rows.val, rows.len);
  if (jRows == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_ADD_TO_HFILE_EXCEPTION));
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }

  jshort j_rowIDLen = rowIDLen;

  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_ADD_TO_HFILE_DB].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
            JavaMethods_[JM_ADD_TO_HFILE_DB].methodID, 
            j_rowIDLen, jRowIDs, jRows);
  if (hbs)
  {
    hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
    hbs->incHbaseCalls();
  }
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::addToHFile()");
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBulkLoadClient_JNI::addToHFile()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_ADD_TO_HFILE_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBLC_OK;
}

HBLC_RetCode HBulkLoadClient_JNI::closeHFile(
                        const HbaseStr &tblName)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::closeHFile(%s) called.", tblName.val);
  if (initJNIEnv() != JOI_OK)
     return HBLC_ERROR_INIT_PARAM;

  tsRecentJMFromJNI = JavaMethods_[JM_CLOSE_HFILE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_CLOSE_HFILE].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::closeHFile()");
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CLOSE_HFILE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBulkLoadClient_JNI::closeHFile()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_CLOSE_HFILE_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBLC_OK;
}


HBLC_RetCode HBulkLoadClient_JNI::doBulkLoad(
                             const HbaseStr &tblName,
                             const Text& prepLocation,
                             const Text& tableName,
                             NABoolean quasiSecure,
                             NABoolean snapshot)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::doBulkLoad(%s, %s, %s) called.", tblName.val, prepLocation.data(), tableName.data());
  if (initJNIEnv() != JOI_OK)
     return HBLC_ERROR_INIT_PARAM;

  jstring js_PrepLocation = jenv_->NewStringUTF(prepLocation.c_str());
   if (js_PrepLocation == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_DO_BULKLOAD_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBLC_ERROR_DO_BULKLOAD_PARAM;
   }
  jstring js_TableName = jenv_->NewStringUTF(tableName.c_str());
   if (js_TableName == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_DO_BULKLOAD_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBLC_ERROR_DO_BULKLOAD_PARAM;
   }

  jboolean j_quasiSecure = quasiSecure;

  jboolean j_snapshot = snapshot;
  tsRecentJMFromJNI = JavaMethods_[JM_DO_BULK_LOAD].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_DO_BULK_LOAD].methodID, js_PrepLocation, js_TableName, j_quasiSecure, j_snapshot);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::doBulkLoad()");
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_DO_BULKLOAD_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::doBulkLoad()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_DO_BULKLOAD_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBLC_OK;
}

HBLC_RetCode HBulkLoadClient_JNI::bulkLoadCleanup(
                             const HbaseStr &tblName,
                             const Text& location)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBulkLoadClient_JNI::bulkLoadCleanup(%s, %s) called.", tblName.val, location.data());
  if (initJNIEnv() != JOI_OK)
     return HBLC_ERROR_INIT_PARAM;

  jstring js_location = jenv_->NewStringUTF(location.c_str());
  if (js_location == NULL)
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBLC_ERROR_BULKLOAD_CLEANUP_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_BULK_LOAD_CLEANUP].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_BULK_LOAD_CLEANUP].methodID, js_location);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::bulkLoadCleanup()");
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBulkLoadClient_JNI::bulkLoadCleanup()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBLC_ERROR_BULKLOAD_CLEANUP_PARAM;
  }
  jenv_->PopLocalFrame(NULL);
  return HBLC_OK;
}


 //////////////////////////////////////////////////////////////////////////////
 //
 //////////////////////////////////////////////////////////////////////////////
 HBC_RetCode  HBaseClient_JNI::incrCounter( const char * tabName, const char * rowId, const char * famName, const char * qualName , Int64 incr, Int64 & count)
 {
   QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::incrCounter().");
   if (initJNIEnv() != JOI_OK)
      return HBC_ERROR_INIT_PARAM;

   jstring js_tabName = jenv_->NewStringUTF(tabName);
   if (js_tabName == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_INCR_COUNTER_PARAM;
   }
   jstring js_rowId = jenv_->NewStringUTF(rowId);
   if (js_rowId == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_INCR_COUNTER_PARAM;
   }

   jstring js_famName = jenv_->NewStringUTF(famName);
   if (js_famName == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_INCR_COUNTER_PARAM;
   }

   jstring js_qualName = jenv_->NewStringUTF(qualName);
   if (js_qualName == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_INCR_COUNTER_PARAM;
   }

   jlong j_incr = incr;

   tsRecentJMFromJNI = JavaMethods_[JM_INCR_COUNTER].jm_full_name;
   jlong jcount = jenv_->CallLongMethod(javaObj_, JavaMethods_[JM_INCR_COUNTER].methodID, js_tabName, js_rowId, js_famName, js_qualName, j_incr);

   count = jcount;
   if (jenv_->ExceptionCheck())
   {
     getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::incrCounter()");
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_INCR_COUNTER_EXCEPTION;
   }

   jenv_->PopLocalFrame(NULL);
   return HBC_OK;
 }

 //////////////////////////////////////////////////////////////////////////////
 //
 //////////////////////////////////////////////////////////////////////////////
 HBC_RetCode  HBaseClient_JNI::createCounterTable( const char * tabName,  const char * famName)
 {
   QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::createCounterTable().");
   if (initJNIEnv() != JOI_OK)
      return HBC_ERROR_INIT_PARAM;

   jstring js_tabName = jenv_->NewStringUTF(tabName);
   if (js_tabName == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_COUNTER_PARAM;
   }
   jstring js_famName = jenv_->NewStringUTF(famName);
   if (js_famName == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_INCR_COUNTER_PARAM;
   }
   tsRecentJMFromJNI = JavaMethods_[JM_CREATE_COUNTER_TABLE].jm_full_name;
   jboolean jresult = jenv_->CallLongMethod(javaObj_, JavaMethods_[JM_CREATE_COUNTER_TABLE].methodID, js_tabName, js_famName);

   if (jenv_->ExceptionCheck())
   {
     getExceptionDetails(__FILE__, __LINE__, "HBulkLoadClient_JNI::createCounterTable()");
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_COUNTER_EXCEPTION;
   }
   if (jresult == false)
   {
     logError(CAT_SQL_HBASE, "HBaseClient_JNI::createCounterTable()", getLastError());
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_COUNTER_EXCEPTION;
   }
   jenv_->PopLocalFrame(NULL);
   return HBC_OK;
 }


HBulkLoadClient_JNI::~HBulkLoadClient_JNI()
{
  //QRLogger::log(CAT_JNI_TOP, LL_DEBUG, "HBulkLoadClient_JNI destructor called.");
}


////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::revoke(const Text& user, const Text& tblName, const TextVec& actions)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::revoke(%s, %s, %s) called.", user.data(), tblName.data(), actions.data());
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  int len = user.size();
  jbyteArray jba_user = jenv_->NewByteArray(len);
  if (jba_user == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_REVOKE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_REVOKE_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_user, 0, len, (const jbyte*)user.data());

  len = tblName.size();
  jbyteArray jba_tblName = jenv_->NewByteArray(len);
  if (jba_tblName == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_REVOKE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_REVOKE_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_tblName, 0, len, (const jbyte*)tblName.data());

  jobjectArray j_actionCodes = NULL;
  if (!actions.empty())
  {
    QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "  Adding %d actions.", actions.size());
    j_actionCodes = convertToStringObjectArray(actions);
    if (j_actionCodes == NULL)
    {
       getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::revoke()");
       jenv_->PopLocalFrame(NULL);
       return HBC_ERROR_REVOKE_PARAM;
    }
  }
  tsRecentJMFromJNI = JavaMethods_[JM_REVOKE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
       JavaMethods_[JM_REVOKE].methodID, jba_user, jba_tblName, j_actionCodes);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::revoke()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_REVOKE_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::revoke()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_REVOKE_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}


////////////////////////////////////////////////////////////////////
void HBaseClient_JNI::logIt(const char* str)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, str);
}

HTableClient_JNI *HBaseClient_JNI::startGet(NAHeap *heap, const char* tableName, bool useTRex, 
            ExHbaseAccessStats *hbs, Int64 transID, const HbaseStr& rowID, 
            const LIST(HbaseStr) & cols, Int64 timestamp) 
{
  if (javaObj_ == NULL || (!isInitialized())) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HTC_EXCEPTION));
    return NULL;
  }

  HTableClient_JNI *htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
  if (htc->init() != HTC_OK) {
     NADELETE(htc, HTableClient_JNI, heap);
     return NULL;
  }
  htc->setTableName(tableName);
  htc->setHbaseStats(hbs);
  htc->setFetchMode(HTableClient_JNI::GET_ROW);

  if (initJNIEnv() != JOI_OK)
     return NULL;

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_STARTGET_EXCEPTION));
    NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
  int len = rowID.len;
  jbyteArray jba_rowID = jenv_->NewByteArray(len);
  if (jba_rowID == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_STARTGET_EXCEPTION));
     NADELETE(htc, HTableClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return NULL;;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, len, (const jbyte*)rowID.val);
  jobjectArray j_cols = NULL;
  if (!cols.isEmpty()) {
     j_cols = convertToByteArrayObjectArray(cols);
     if (j_cols == NULL) {
        getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::startGet()");
        NADELETE(htc, HTableClient_JNI, heap);
        jenv_->PopLocalFrame(NULL);
        return NULL;
     }  
     htc->setNumColsInScan(cols.entries());
  }
  else
     htc->setNumColsInScan(0);
  htc->setNumReqRows(1);
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  
  if (hbs)
    hbs->getHbaseTimer().start();

  tsRecentJMFromJNI = JavaMethods_[JM_START_GET].jm_full_name;
  jint jresult = jenv_->CallIntMethod(javaObj_, 
            JavaMethods_[JM_START_GET].methodID, 
	(jlong)htc, js_tblName, (jboolean)useTRex, j_tid, jba_rowID, 
            j_cols, j_ts);
  if (hbs) {
      hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
      hbs->incHbaseCalls();
  }

  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::startGet()");
    jenv_->PopLocalFrame(NULL);
    releaseHTableClient(htc);
    return NULL;
  }

  if (jresult == 0) 
     htc->setNumRowsReturned(-1);
  else
     htc->setNumRowsReturned(1);
  jenv_->PopLocalFrame(NULL);
  return htc;
}

HTableClient_JNI *HBaseClient_JNI::startGets(NAHeap *heap, const char* tableName, bool useTRex,
            ExHbaseAccessStats *hbs, Int64 transID, const LIST(HbaseStr) *rowIDs, 
            short rowIDLen, const HbaseStr *rowIDsInDB,
            const LIST(HbaseStr) & cols, Int64 timestamp)
{
  if (javaObj_ == NULL || (!isInitialized())) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HTC_EXCEPTION));
    return NULL;
  }

  HTableClient_JNI *htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
  if (htc->init() != HTC_OK) {
     NADELETE(htc, HTableClient_JNI, heap);
     return NULL;
  }
  htc->setTableName(tableName);
  htc->setHbaseStats(hbs);
  htc->setFetchMode(HTableClient_JNI::BATCH_GET);

  if (initJNIEnv() != JOI_OK)
     return NULL;

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_STARTGET_EXCEPTION));
    NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
  jobjectArray j_cols = NULL;
  if (!cols.isEmpty()) {
     j_cols = convertToByteArrayObjectArray(cols);
     if (j_cols == NULL) {
        getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::startGets()");
        NADELETE(htc, HTableClient_JNI, heap);
        jenv_->PopLocalFrame(NULL);
        return NULL;
     }  
     htc->setNumColsInScan(cols.entries());
  }
  else
     htc->setNumColsInScan(0);
  jobjectArray	j_rows = NULL;
  jobject       jRowIDs = NULL;

  if (rowIDs != NULL) {
     j_rows = convertToByteArrayObjectArray(*rowIDs);
     if (j_rows == NULL) {
        getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::startGets()");
        NADELETE(htc, HTableClient_JNI, heap);
        jenv_->PopLocalFrame(NULL);
        return NULL;
     }  
     htc->setNumReqRows(rowIDs->entries());
  } else {
     jRowIDs = jenv_->NewDirectByteBuffer(rowIDsInDB->val, rowIDsInDB->len);
     if (jRowIDs == NULL) {
        GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_STARTGET_EXCEPTION));
        NADELETE(htc, HTableClient_JNI, heap);
        jenv_->PopLocalFrame(NULL);
        return NULL;
     }
     // Need to swap the bytes 
     short numReqRows = *(short *)rowIDsInDB->val;
     htc->setNumReqRows(bswap_16(numReqRows));
  }

  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jshort jRowIDLen = rowIDLen; 
  if (hbs)
    hbs->getHbaseTimer().start();

  jint jresult;
  if (rowIDs != NULL) {
     tsRecentJMFromJNI = JavaMethods_[JM_START_GETS].jm_full_name;
     jresult = jenv_->CallIntMethod(javaObj_, 
            JavaMethods_[JM_START_GETS].methodID, 
	(jlong)htc, js_tblName, (jboolean)useTRex, j_tid, j_rows, 
            j_cols, j_ts);
  } else {
     tsRecentJMFromJNI = JavaMethods_[JM_START_DIRECT_GETS].jm_full_name;
     jresult = jenv_->CallIntMethod(javaObj_, 
            JavaMethods_[JM_START_DIRECT_GETS].methodID, 
	(jlong)htc, js_tblName, (jboolean)useTRex, j_tid, jRowIDLen, jRowIDs, 
            j_cols);
  }
  if (hbs) {
      hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
      hbs->incHbaseCalls();
  }

  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::startGets()");
    jenv_->PopLocalFrame(NULL);
    releaseHTableClient(htc);
    return NULL;
  }

  if (jresult == 0) 
     htc->setNumRowsReturned(-1);
  else
     htc->setNumRowsReturned(jresult);
  jenv_->PopLocalFrame(NULL);
  return htc;
}

HBC_RetCode HBaseClient_JNI::getRegionsNodeName(const char* tblName,
                                                Int32 partns,
                                                ARRAY(const char *)& nodeNames)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::getRegionsNodeName(%s) called.", tblName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_tblName = jenv_->NewStringUTF(tblName);
  if (js_tblName == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HBTI_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_HBTI_PARAM;
  }

  jobjectArray jNodeNames = jenv_->NewObjectArray(partns,
                                                  jenv_->FindClass("java/lang/String"),
                                                  jenv_->NewStringUTF(""));
                              
  tsRecentJMFromJNI = JavaMethods_[JM_GET_REGN_NODES].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_GET_REGN_NODES].methodID,
                                              js_tblName, jNodeNames);
  NAHeap *heap = getHeap();
  if (jresult) {
    jstring strObj = NULL;
    char* node = NULL;
    for(int i=0; i < partns; i++) {
      strObj = (jstring) jenv_->GetObjectArrayElement(jNodeNames, i);
      node = (char*)jenv_->GetStringUTFChars(strObj, NULL);
      char* copyNode = new (heap) char[strlen(node)+1];
      strcpy(copyNode, node);
      nodeNames.insertAt(i, copyNode);
    }
    //jenv_->ReleaseObjectArrayElements(jNodeNames, strObj, JNI_ABORT);
    jenv_->ReleaseStringUTFChars(strObj, node);
  }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getRegionsNodeName()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_HBTI_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::getRegionsNodeName()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_HBTI_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;  // Table exists.

}
//////////////////////////////////////////////////////////////////////////////
// Get Hbase Table information. Currently the following info is requested:
// 1. index levels : This info is obtained from trailer block of Hfiles of randomly chosen region
// 2. block size : This info is obtained for HColumnDescriptor
////////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::getHbaseTableInfo(const char* tblName,
                                              Int32& indexLevels,
                                              Int32& blockSize)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::getHbaseTableInfo(%s) called.", tblName);
  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_tblName = jenv_->NewStringUTF(tblName);
  if (js_tblName == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GET_HBTI_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_HBTI_PARAM;
  }

  jintArray jHtabInfo = jenv_->NewIntArray(2);
  tsRecentJMFromJNI = JavaMethods_[JM_GET_HBTI].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_GET_HBTI].methodID,
                                              js_tblName, jHtabInfo);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getHbaseTableInfo()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_GET_HBTI_EXCEPTION;
  }
  jboolean isCopy;
  jint* arrayElems = jenv_->GetIntArrayElements(jHtabInfo, &isCopy);
  indexLevels = arrayElems[0];
  blockSize = arrayElems[1];
  if (isCopy == JNI_TRUE)
     jenv_->ReleaseIntArrayElements(jHtabInfo, arrayElems, JNI_ABORT);
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;  // Table exists.
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::insertRow(NAHeap *heap, const char *tableName,
      ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, HbaseStr rowID,
      HbaseStr row, Int64 timestamp, bool checkAndPut, bool asyncOperation,
      bool useRegionXn, short colIndexToCheck, HTableClient_JNI **outHtc)
{
  
  HTableClient_JNI *htc = NULL;

  if (asyncOperation) {
     htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
     if (htc->init() != HTC_OK) {
         NADELETE(htc, HTableClient_JNI, heap);
         return HBC_ERROR_INSERTROW_EXCEPTION;
     }
     htc->setTableName(tableName);
     htc->setHbaseStats(hbs);
  }

  if (initJNIEnv() != JOI_OK) {
    if (htc != NULL) 
       NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INIT_PARAM;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INSERTROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROW_PARAM;
  }
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INSERTROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);
  jobject jRow = jenv_->NewDirectByteBuffer(row.val, row.len);
  if (jRow == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INSERTROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROW_PARAM;
  }
  jlong j_htc = (long)htc;
  jboolean j_useTRex = useTRex;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean j_checkAndPut = checkAndPut;
  jboolean j_asyncOperation = asyncOperation;
  jboolean j_useRegionXn = useRegionXn;
  jshort j_colIndexToCheck = colIndexToCheck;

  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HBC_DIRECT_INSERT_ROW].jm_full_name;
  jboolean jresult = 
    jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HBC_DIRECT_INSERT_ROW].methodID, 
                             j_htc, js_tblName, j_useTRex, j_tid, jba_rowID, jRow, j_ts, j_checkAndPut, j_colIndexToCheck, j_asyncOperation, j_useRegionXn);
  if (hbs) {
      hbs->incHbaseCalls();
      if (!asyncOperation)
         hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
  }
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::insertRow()");
    jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    return HBC_ERROR_INSERTROW_EXCEPTION;
  }
  if (hbs)
    hbs->incBytesRead(rowID.len + row.len);
  if (jresult == false) {
     jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INSERTROW_DUP_ROWID;
  }
  *outHtc = htc;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::insertRows(NAHeap *heap, const char *tableName,
      ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, short rowIDLen, HbaseStr rowIDs,
      HbaseStr rows, Int64 timestamp, bool asyncOperation,
      HTableClient_JNI **outHtc)
{
  
  HTableClient_JNI *htc = NULL;

  if (asyncOperation) {
     htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
     if (htc->init() != HTC_OK) {
         NADELETE(htc, HTableClient_JNI, heap);
         return HBC_ERROR_INSERTROWS_EXCEPTION;
     }
     htc->setTableName(tableName);
     htc->setHbaseStats(hbs);
  }
  if (initJNIEnv() != JOI_OK) {
     if (htc != NULL) 
       NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INIT_PARAM;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INSERTROWS_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROWS_PARAM;
  }
  jobject jRowIDs = jenv_->NewDirectByteBuffer(rowIDs.val, rowIDs.len);
  if (jRowIDs == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INSERTROWS_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROWS_PARAM;
  }
  
  jobject jRows = jenv_->NewDirectByteBuffer(rows.val, rows.len);
  if (jRows == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_INSERTROWS_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROWS_PARAM;
  }
  jboolean j_useTRex = useTRex;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jlong j_htc = (long)htc;
  jshort j_rowIDLen = rowIDLen;
  jboolean j_asyncOperation = asyncOperation;
 
  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HBC_DIRECT_INSERT_ROWS].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HBC_DIRECT_INSERT_ROWS].methodID, 
               	j_htc, js_tblName, j_useTRex, j_tid, j_rowIDLen, jRowIDs, jRows, j_ts, j_asyncOperation);
  if (hbs) {
      hbs->incHbaseCalls();
      if (!asyncOperation)
         hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
  }
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::insertRows()");
    jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    return HBC_ERROR_INSERTROWS_EXCEPTION;
  }
  if (jresult == false) {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::insertRows()", getLastError());
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_INSERTROWS_EXCEPTION;
  }
  if (hbs)
    hbs->incBytesRead(rowIDs.len + rows.len);
  *outHtc = htc;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}
//
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::checkAndUpdateRow(NAHeap *heap, const char *tableName,
                                               ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, HbaseStr rowID,
                                               HbaseStr row, HbaseStr columnToCheck, HbaseStr columnValToCheck,
                                               Int64 timestamp, bool asyncOperation, bool useRegionXn,
                                               HTableClient_JNI **outHtc)
{
  
  HTableClient_JNI *htc = NULL;

  if (asyncOperation) {
     htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
     if (htc->init() != HTC_OK) {
         NADELETE(htc, HTableClient_JNI, heap);
         return HBC_ERROR_CHECKANDUPDATEROW_EXCEPTION;
     }
     htc->setTableName(tableName);
     htc->setHbaseStats(hbs);
  }
  if (initJNIEnv() != JOI_OK) {
    if (htc != NULL) 
       NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INIT_PARAM;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDUPDATEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDUPDATEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);
  
  jobject jRow = jenv_->NewDirectByteBuffer(row.val, row.len);
  if (jRow == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDUPDATEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDUPDATEROW_PARAM;
  }

  jbyteArray jba_columnToCheck = jenv_->NewByteArray(columnToCheck.len);
  if (jba_columnToCheck == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDUPDATEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_columnToCheck, 0, columnToCheck.len, (const jbyte*)columnToCheck.val);

  jbyteArray jba_columnValToCheck = jenv_->NewByteArray(columnValToCheck.len);
  if (jba_columnValToCheck == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDUPDATEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDUPDATEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_columnValToCheck, 0, columnValToCheck.len, (const jbyte*)columnValToCheck.val);
  jlong j_htc = (long)htc;
  jboolean j_useTRex = useTRex;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean j_asyncOperation = asyncOperation;
  jboolean j_useRegionXn = useRegionXn;

  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HBC_DIRECT_CHECKANDUPDATE_ROW].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HBC_DIRECT_CHECKANDUPDATE_ROW].methodID, 
                                              j_htc, js_tblName, j_useTRex, j_tid, jba_rowID, jRow,
                                              jba_columnToCheck, jba_columnValToCheck,  j_ts, 
                                              j_asyncOperation, j_useRegionXn);
  if (hbs) {
      hbs->incHbaseCalls();
      if (!asyncOperation)
         hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
  }
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::cheakAndUpdateRow()");
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDUPDATEROW_EXCEPTION;
  }
  if (hbs)
    hbs->incBytesRead(rowID.len + row.len);
  if (jresult == false) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDUPDATEROW_NOTFOUND));
     jenv_->PopLocalFrame(NULL);
     if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_CHECKANDUPDATEROW_NOTFOUND;
  }
  *outHtc = htc;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::deleteRow(NAHeap *heap, const char *tableName,
                                       ExHbaseAccessStats *hbs, bool useTRex, 
                                       Int64 transID, HbaseStr rowID, 
                                       const LIST(HbaseStr) *cols, 
                                       Int64 timestamp, 
                                       bool asyncOperation, bool useRegionXn,
                                       HTableClient_JNI **outHtc)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::deleteRow(%ld, %s) called.", transID, rowID.val);

  HTableClient_JNI *htc = NULL;

  if (asyncOperation) {
     htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
     if (htc->init() != HTC_OK) {
         NADELETE(htc, HTableClient_JNI, heap);
         return HBC_ERROR_DELETEROW_EXCEPTION;
     }
     htc->setTableName(tableName);
     htc->setHbaseStats(hbs);
  }
  if (initJNIEnv() != JOI_OK) {
    if (htc != NULL) 
       NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INIT_PARAM;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DELETEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DELETEROW_PARAM;
  }
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DELETEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_DELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);
  jobjectArray j_cols = NULL;
  if (cols != NULL && !cols->isEmpty()) {
     j_cols = convertToByteArrayObjectArray(*cols);
     if (j_cols == NULL) {
        getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::deleteRow()");
        if (htc != NULL)
           NADELETE(htc, HTableClient_JNI, heap);
        jenv_->PopLocalFrame(NULL);
        return HBC_ERROR_DELETEROW_PARAM;
     }
  }  
  jlong j_htc = (jlong)htc;
  jboolean j_useTRex = useTRex;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean j_asyncOperation = asyncOperation;
  jboolean j_useRegionXn = useRegionXn;
  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HBC_DELETE_ROW].jm_full_name;
  jboolean jresult = 
    jenv_->CallBooleanMethod(javaObj_, 
                             JavaMethods_[JM_HBC_DELETE_ROW].methodID, j_htc, js_tblName, j_useTRex, j_tid, jba_rowID, 
                             j_cols, j_ts, j_asyncOperation, j_useRegionXn);
  if (hbs) {
      hbs->incHbaseCalls();
      if (!asyncOperation) 
         hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
  }
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::deleteRow()");
    jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    return HBC_ERROR_DELETEROW_EXCEPTION;
  }

  if (jresult == false) {
     logError(CAT_SQL_HBASE, "HBaseClient_JNI::deleteRow()", getLastError());
     jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_DELETEROW_EXCEPTION;
  }

  if (hbs)
    hbs->incBytesRead(rowID.len);
  *outHtc = htc;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::deleteRows(NAHeap *heap, const char *tableName,
      ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, short rowIDLen, HbaseStr rowIDs, 
      Int64 timestamp, bool asyncOperation, HTableClient_JNI **outHtc)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::deleteRows(%ld, %s) called.", transID, rowIDs.val);

  HTableClient_JNI *htc = NULL;

  if (asyncOperation) {
      htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
     if (htc->init() != HTC_OK) {
         NADELETE(htc, HTableClient_JNI, heap);
         return HBC_ERROR_DELETEROWS_EXCEPTION;
     }
     htc->setTableName(tableName);
     htc->setHbaseStats(hbs);
  }
  if (initJNIEnv() != JOI_OK) {
    if (htc != NULL) 
       NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INIT_PARAM;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DELETEROWS_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DELETEROWS_PARAM;
  }

  jobject jRowIDs = jenv_->NewDirectByteBuffer(rowIDs.val, rowIDs.len);
  if (jRowIDs == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DELETEROWS_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DELETEROWS_PARAM;
  }
  jlong j_htc = (jlong)htc;
  jboolean j_useTRex = useTRex;
  jlong j_tid = transID;  
  jshort j_rowIDLen = rowIDLen;
  jlong j_ts = timestamp;
  jboolean j_asyncOperation = asyncOperation;
  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HBC_DIRECT_DELETE_ROWS].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
          JavaMethods_[JM_HBC_DIRECT_DELETE_ROWS].methodID, j_htc, js_tblName, j_useTRex, j_tid, 
          j_rowIDLen, jRowIDs, j_ts, j_asyncOperation);
  if (hbs) {
      hbs->incHbaseCalls();
      if (!asyncOperation) 
         hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
  }
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::deleteRows()");
    jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    return HBC_ERROR_DELETEROWS_EXCEPTION;
  }
  if (jresult == false) {
    logError(CAT_SQL_HBASE, "HBaseClient_JNI::deleteRows()", getLastError());
    jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    return HBC_ERROR_DELETEROWS_EXCEPTION;
  }
  if (hbs)
    hbs->incBytesRead(rowIDs.len);
  *outHtc = htc;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}
//
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HBC_RetCode HBaseClient_JNI::checkAndDeleteRow(NAHeap *heap, const char *tableName,
                                               ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, HbaseStr rowID, 
                                               HbaseStr columnToCheck, HbaseStr columnValToCheck,
                                               Int64 timestamp, bool asyncOperation, bool useRegionXn,
                                               HTableClient_JNI **outHtc)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::checkAndDeleteRow(%ld, %s) called.", transID, rowID.val);

  HTableClient_JNI *htc = NULL;

  if (asyncOperation) {
     htc = new (heap) HTableClient_JNI(heap, (jobject)-1);
     if (htc->init() != HTC_OK) {
         NADELETE(htc, HTableClient_JNI, heap);
         return HBC_ERROR_CHECKANDDELETEROW_EXCEPTION;
     }
     htc->setTableName(tableName);
     htc->setHbaseStats(hbs);
  }
  if (initJNIEnv() != JOI_OK) {
    if (htc != NULL) 
       NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_INIT_PARAM;
  }

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDDELETEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDDELETEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);
  jbyteArray jba_columnToCheck = jenv_->NewByteArray(columnToCheck.len);
  if (jba_columnToCheck == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDDELETEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_columnToCheck, 0, columnToCheck.len, (const jbyte*)columnToCheck.val);
  jbyteArray jba_columnValToCheck = jenv_->NewByteArray(columnValToCheck.len);
  if (jba_columnValToCheck == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CHECKANDDELETEROW_PARAM));
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CHECKANDDELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_columnValToCheck, 0, columnValToCheck.len, (const jbyte*)columnValToCheck.val);
  jlong j_htc = (jlong)htc;
  jboolean j_useTRex = useTRex;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  jboolean j_asyncOperation = asyncOperation;
  jboolean j_useRegionXn = useRegionXn;
  if (hbs)
    hbs->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HBC_CHECKANDDELETE_ROW].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
                                              JavaMethods_[JM_HBC_CHECKANDDELETE_ROW].methodID, j_htc, js_tblName, j_useTRex, j_tid, jba_rowID, 
                                              jba_columnToCheck, jba_columnValToCheck, j_ts, 
                                              j_asyncOperation, j_useRegionXn);
  if (hbs) {
      hbs->incHbaseCalls();
      if (!asyncOperation) 
         hbs->incMaxHbaseIOTime(hbs->getHbaseTimer().stop());
  }
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::checkAndDeleteRow()");
    jenv_->PopLocalFrame(NULL);
    if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
    return HBC_ERROR_CHECKANDDELETEROW_EXCEPTION;
  }

  if (jresult == false) {
     jenv_->PopLocalFrame(NULL);
     if (htc != NULL)
        NADELETE(htc, HTableClient_JNI, heap);
     return HBC_ERROR_CHECKANDDELETEROW_NOTFOUND;
  }

  if (hbs)
    hbs->incBytesRead(rowID.len);
  *outHtc = htc;
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

NAArray<HbaseStr>* HBaseClient_JNI::getStartKeys(NAHeap *heap, const char *tableName, bool useTRex)
{
   return HBaseClient_JNI::getKeys(JM_HBC_GETSTARTKEYS, heap, tableName, useTRex);
}

NAArray<HbaseStr>* HBaseClient_JNI::getEndKeys(NAHeap *heap, const char * tableName, bool useTRex)
{
   return HBaseClient_JNI::getKeys(JM_HBC_GETENDKEYS, heap, tableName, useTRex);
}

NAArray<HbaseStr>* HBaseClient_JNI::getKeys(Int32 funcIndex, NAHeap *heap, const char *tableName, bool useTRex)
{
  if (initJNIEnv() != JOI_OK) 
     return NULL;

  jstring js_tblName = jenv_->NewStringUTF(tableName);
  if (js_tblName == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_GETKEYS));
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
  jboolean j_useTRex = useTRex;
  tsRecentJMFromJNI = JavaMethods_[funcIndex].jm_full_name;
  jarray j_keyArray=
     (jarray)jenv_->CallObjectMethod(javaObj_, JavaMethods_[funcIndex].methodID, js_tblName, j_useTRex);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::getKeys()");
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
  NAArray<HbaseStr> *retArray;
  jint retcode = convertByteArrayObjectArrayToNAArray(heap, j_keyArray, &retArray);
  jenv_->PopLocalFrame(NULL);
  if (retcode == 0)
     return NULL;
  else
     return retArray;  
}

HBC_RetCode HBaseClient_JNI::createSnapshot( const NAString&  tableName, const NAString&  snapshotName)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::createSnapshot(%s, %s) called.",
      tableName.data(), snapshotName.data());

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_tableName = jenv_->NewStringUTF(tableName.data());
  if (js_tableName == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_SNAPSHOT_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_SNAPSHOT_PARAM;
  }
  jstring js_snapshotName= jenv_->NewStringUTF(snapshotName.data());
  if (js_snapshotName == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_CREATE_SNAPSHOT_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_CREATE_SNAPSHOT_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HBC_CREATE_SNAPSHOT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HBC_CREATE_SNAPSHOT].methodID, js_tableName, js_snapshotName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::createSnapshot()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_CREATE_SNAPSHOT_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::verifySnapshot( const NAString&  tableName, const NAString&  snapshotName,
                                                NABoolean & exist)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::verifySnapshot(%s, %s) called.",
      tableName.data(), snapshotName.data());

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_tableName = jenv_->NewStringUTF(tableName.data());
  if (js_tableName == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_VERIFY_SNAPSHOT_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_VERIFY_SNAPSHOT_PARAM;
  }
  jstring js_snapshotName= jenv_->NewStringUTF(snapshotName.data());
  if (js_snapshotName == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_VERIFY_SNAPSHOT_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_VERIFY_SNAPSHOT_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HBC_VERIFY_SNAPSHOT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HBC_VERIFY_SNAPSHOT].methodID, js_tableName, js_snapshotName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::verifySnapshot()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_VERIFY_SNAPSHOT_EXCEPTION;
  }

  exist = jresult;

  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

HBC_RetCode HBaseClient_JNI::deleteSnapshot( const NAString&  snapshotName)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HBaseClient_JNI::deleteSnapshot(%s) called.",
      snapshotName.data());

  if (initJNIEnv() != JOI_OK)
     return HBC_ERROR_INIT_PARAM;

  jstring js_snapshotName= jenv_->NewStringUTF(snapshotName.data());
  if (js_snapshotName == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HBC_ERROR_DELETE_SNAPSHOT_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HBC_ERROR_DELETE_SNAPSHOT_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HBC_DELETE_SNAPSHOT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HBC_DELETE_SNAPSHOT].methodID, js_snapshotName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HBaseClient_JNI::deleteSnapshot()");
    jenv_->PopLocalFrame(NULL);
    return HBC_ERROR_DELETE_SNAPSHOT_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HBC_OK;
}

// ===========================================================================
// ===== Class HTableClient
// ===========================================================================

JavaMethodInit* HTableClient_JNI::JavaMethods_ = NULL;
jclass HTableClient_JNI::javaClass_ = 0;
bool HTableClient_JNI::javaMethodsInitialized_ = false;
pthread_mutex_t HTableClient_JNI::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const htcErrorEnumStr[] = 
{
  "Preparing parameters for initConnection()."
 ,"Java exception in initConnection()."
 ,"Java exception in setTransactionID()."
 ,"Java exception in cleanup()."
 ,"Java exception in close()."
 ,"Preparing parameters for scanOpen()."
 ,"Java exception in scanOpen()."
 ,"Java exception in fetchRows()."
 ,"Java exception in scanClose()."
 ,"Java exception in getClose()."
 ,"Preparing parameters for deleteRow()."
 ,"Java exception in deleteRow()."
 ,"Preparing parameters for create()."
 ,"Java exception in create()."
 ,"Preparing parameters for drop()."
 ,"Java exception in drop()."
 ,"Preparing parameters for exists()."
 ,"Java exception in exists()."
 ,"Preparing parameters for coProcAggr()."
 ,"Java exception in coProcAggr()."
 ,"Preparing parameters for grant()."
 ,"Java exception in grant()."
 ,"Preparing parameters for revoke()."
 ,"Java exception in revoke()."
 ,"Java exception in getendkeys()."
 ,"Java exception in getHTableName()."
 ,"Java exception in getColName()."
 ,"Java exception in getColValue()."
 ,"Java exception in getRowID()."
 ,"Java exception in nextCell()."
 ,"Java exception in completeAsyncOperation()."
 ,"Async Hbase Operation not yet complete."
 ,"Java exception in setWriteToWal()."
 ,"Java exception in setWriteBufferSize()."
 ,"Java exception in prepareForNextCell()."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
char* HTableClient_JNI::getErrorText(HTC_RetCode errEnum)
{
  if (errEnum < (HTC_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else    
    return (char*)htcErrorEnumStr[errEnum-HTC_FIRST-1];
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTableClient_JNI::~HTableClient_JNI()
{
  //QRLogger::log(CAT_JNI_TOP, LL_DEBUG, "HTableClient destructor called.");
  cleanupResultInfo();
  if (tableName_ != NULL)
  {
     NADELETEBASIC(tableName_, heap_);
  }
  if (colNameAllocLen_ != 0)
     NADELETEBASIC(colName_, heap_);
  if (numCellsAllocated_ > 0)
  {
      NADELETEBASIC(p_kvValLen_, heap_);
      NADELETEBASIC(p_kvValOffset_, heap_);
      NADELETEBASIC(p_kvFamLen_, heap_);
      NADELETEBASIC(p_kvFamOffset_, heap_);
      NADELETEBASIC(p_kvQualLen_, heap_);
      NADELETEBASIC(p_kvQualOffset_, heap_);
      NADELETEBASIC(p_timestamp_, heap_);
      numCellsAllocated_ = 0;
  }
}
 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::init()
{
  static char className[]="org/trafodion/sql/HTableClient";
  HTC_RetCode rc;
  
  if (isInitialized())
    return HTC_OK;
  
  if (javaMethodsInitialized_)
    return (HTC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HTC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    JavaMethods_[JM_SCAN_OPEN  ].jm_name      = "startScan";
    JavaMethods_[JM_SCAN_OPEN  ].jm_signature = "(J[B[B[Ljava/lang/Object;JZZI[Ljava/lang/Object;[Ljava/lang/Object;[Ljava/lang/Object;FFZZILjava/lang/String;Ljava/lang/String;II)Z";
    JavaMethods_[JM_DELETE     ].jm_name      = "deleteRow";
    JavaMethods_[JM_DELETE     ].jm_signature = "(J[B[Ljava/lang/Object;JZZ)Z";
    JavaMethods_[JM_COPROC_AGGR     ].jm_name      = "coProcAggr";
    JavaMethods_[JM_COPROC_AGGR     ].jm_signature = "(JI[B[B[B[BZI)[B";
    JavaMethods_[JM_GET_NAME   ].jm_name      = "getTableName";
    JavaMethods_[JM_GET_NAME   ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_GET_HTNAME ].jm_name      = "getTableName";
    JavaMethods_[JM_GET_HTNAME ].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_SET_WB_SIZE ].jm_name      = "setWriteBufferSize";
    JavaMethods_[JM_SET_WB_SIZE ].jm_signature = "(J)Z";
    JavaMethods_[JM_SET_WRITE_TO_WAL ].jm_name      = "setWriteToWAL";
    JavaMethods_[JM_SET_WRITE_TO_WAL ].jm_signature = "(Z)Z";
    JavaMethods_[JM_FETCH_ROWS ].jm_name      = "fetchRows";
    JavaMethods_[JM_FETCH_ROWS ].jm_signature = "()I";
    JavaMethods_[JM_COMPLETE_PUT ].jm_name      = "completeAsyncOperation";
    JavaMethods_[JM_COMPLETE_PUT ].jm_signature = "(I[Z)Z";
   
    rc = (HTC_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == HTC_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::startScan(Int64 transID, const Text& startRowID, 
   const Text& stopRowID, const LIST(HbaseStr) & cols, Int64 timestamp, 
   bool cacheBlocks, bool smallScanner, Lng32 numCacheRows, NABoolean preFetch,
					const LIST(NAString) *inColNamesToFilter, 
					const LIST(NAString) *inCompareOpList,
					const LIST(NAString) *inColValuesToCompare,
                    Float32 dopParallelScanner,
					Float32 samplePercent,
					NABoolean useSnapshotScan,
					Lng32 snapTimeout,
					char * snapName ,
					char * tmpLoc,
					Lng32 espNum,
                                        Lng32 versions)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HTableClient_JNI::startScan() called.");

  if (initJNIEnv() != JOI_OK) 
     return HTC_ERROR_INIT_PARAM;

  int len = startRowID.size();
  jbyteArray jba_startRowID = jenv_->NewByteArray(len);
  if (jba_startRowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_SCANOPEN_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_SCANOPEN_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_startRowID, 0, len, (const jbyte*)startRowID.data());

  len = stopRowID.size();
  jbyteArray jba_stopRowID = jenv_->NewByteArray(len);
  if (jba_stopRowID == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_SCANOPEN_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_SCANOPEN_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_stopRowID, 0, len, (const jbyte*)stopRowID.data());

  jobjectArray j_cols = NULL;
  if (!cols.isEmpty())
  {
    j_cols = convertToByteArrayObjectArray(cols);
    if (j_cols == NULL)
    {
       getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::startScan()");
       jenv_->PopLocalFrame(NULL);
       return HTC_ERROR_SCANOPEN_PARAM;
    }
    numColsInScan_ = cols.entries();
  }
  else
     numColsInScan_ = 0;
  jlong j_tid = transID;  
  jlong j_ts = timestamp;

  jboolean j_cb = cacheBlocks;
  jboolean j_smallScanner = smallScanner;
  jboolean j_preFetch = preFetch;
  jint j_ncr = numCacheRows;
  numReqRows_ = numCacheRows;
  currentRowNum_ = -1;
  currentRowCellNum_ = -1;

  jobjectArray j_colnamestofilter = NULL;
  if ((inColNamesToFilter) && (!inColNamesToFilter->isEmpty()))
  {
    j_colnamestofilter = convertToByteArrayObjectArray(*inColNamesToFilter);
    if (j_colnamestofilter == NULL)
    {
       getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::startScan()");
       jenv_->PopLocalFrame(NULL);
       return HTC_ERROR_SCANOPEN_PARAM;
    }
  }

  jobjectArray j_compareoplist = NULL;
  if ((inCompareOpList) && (! inCompareOpList->isEmpty()))
  {
     j_compareoplist = convertToByteArrayObjectArray(*inCompareOpList);
     if (j_compareoplist == NULL)
     {
        getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::startScan()");
        jenv_->PopLocalFrame(NULL);
        return HTC_ERROR_SCANOPEN_PARAM;
     }
  }

  jobjectArray j_colvaluestocompare = NULL;
  if ((inColValuesToCompare) && (!inColValuesToCompare->isEmpty()))
  {
     j_colvaluestocompare = convertToByteArrayObjectArray(*inColValuesToCompare);
     if (j_colvaluestocompare == NULL)
     {
        getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::startScan()");
        jenv_->PopLocalFrame(NULL);
        return HTC_ERROR_SCANOPEN_PARAM;
     }
  }
  jfloat j_dopParallelScanner = dopParallelScanner;
  jfloat j_smplPct = samplePercent;
  jboolean j_useSnapshotScan = useSnapshotScan;
  jint j_snapTimeout = snapTimeout;
  jint j_espNum = espNum;
  jint j_versions = versions;

  jstring js_snapName = jenv_->NewStringUTF(snapName != NULL ? snapName : "Dummy");
   if (js_snapName == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_SCANOPEN_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HTC_ERROR_SCANOPEN_PARAM;
   }
  jstring js_tmp_loc = jenv_->NewStringUTF(tmpLoc != NULL ? tmpLoc : "Dummy");
   if (js_tmp_loc == NULL)
   {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_SCANOPEN_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HTC_ERROR_SCANOPEN_PARAM;
   }

  if (hbs_)
      hbs_->getHbaseTimer().start();

  tsRecentJMFromJNI = JavaMethods_[JM_SCAN_OPEN].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(
                                              javaObj_, 
                                              JavaMethods_[JM_SCAN_OPEN].methodID, 
                                              j_tid, jba_startRowID, jba_stopRowID, j_cols, j_ts, j_cb, j_smallScanner, j_ncr,
                                              j_colnamestofilter, j_compareoplist, j_colvaluestocompare, 
                                              j_dopParallelScanner,
                                              j_smplPct, j_preFetch, j_useSnapshotScan,
                                              j_snapTimeout, js_snapName, js_tmp_loc, j_espNum,
                                              j_versions);

  if (hbs_)
  {
    hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
    hbs_->incHbaseCalls();
  }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::scanOpen()");
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_SCANOPEN_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HTableClient_JNI::scanOpen()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_SCANOPEN_EXCEPTION;
  }
  fetchMode_ = SCAN_FETCH;
  jenv_->PopLocalFrame(NULL);
  return HTC_OK;
}
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::deleteRow(Int64 transID, HbaseStr &rowID, const LIST(HbaseStr) *cols, Int64 timestamp)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HTableClient_JNI::deleteRow(%ld, %s) called.", transID, rowID.val);

  if (initJNIEnv() != JOI_OK) 
     return HTC_ERROR_INIT_PARAM;

  jbyteArray jba_rowID = jenv_->NewByteArray(rowID.len);
  if (jba_rowID == NULL) 
  {
     GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_DELETEROW_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HTC_ERROR_DELETEROW_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_rowID, 0, rowID.len, (const jbyte*)rowID.val);
  jobjectArray j_cols = NULL;
  if (cols != NULL && !cols->isEmpty())
  {
     j_cols = convertToByteArrayObjectArray(*cols);
     if (j_cols == NULL)
     {
        getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::deleteROw()");
        jenv_->PopLocalFrame(NULL);
        return HTC_ERROR_DELETEROW_PARAM;
     }
  }  
  jlong j_tid = transID;  
  jlong j_ts = timestamp;
  if (hbs_)
    hbs_->getHbaseTimer().start();
  jboolean j_asyncOperation = FALSE;
  jboolean j_useRegionXn = FALSE;
  tsRecentJMFromJNI = JavaMethods_[JM_DELETE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, 
                                              JavaMethods_[JM_DELETE].methodID, j_tid, jba_rowID, j_cols, j_ts, j_asyncOperation, j_useRegionXn);
  if (hbs_)
    {
      hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
      hbs_->incHbaseCalls();
    }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::deleteROw()");
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_DELETEROW_EXCEPTION;
  }

  if (jresult == false) 
  {
    logError(CAT_SQL_HBASE, "HTableClient_JNI::deleteRow()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_DELETEROW_EXCEPTION;
  }

  if (hbs_)
    hbs_->incBytesRead(rowID.len);
  jenv_->PopLocalFrame(NULL);
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HTC_RetCode HTableClient_JNI::setWriteBufferSize(Int64 size)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HTableClient_JNI::setWriteBufferSize() called.");
  if (initJNIEnv() != JOI_OK) 
     return HTC_ERROR_INIT_PARAM;

  jlong j_size = size;

  tsRecentJMFromJNI = JavaMethods_[JM_SET_WB_SIZE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_SET_WB_SIZE].methodID, j_size);


  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::setWriteBufferSize()");
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_WRITEBUFFERSIZE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HTableClient_JNI::setWriteBufferSize()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_WRITEBUFFERSIZE_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HTC_OK;
}

HTC_RetCode HTableClient_JNI::setWriteToWAL(bool WAL)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HTableClient_JNI::setWriteToWAL() called.");

  jboolean j_WAL = WAL;

  if (initJNIEnv() != JOI_OK) 
     return HTC_ERROR_INIT_PARAM;

  tsRecentJMFromJNI = JavaMethods_[JM_SET_WRITE_TO_WAL].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_SET_WRITE_TO_WAL].methodID, j_WAL);


  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::setWriteToWAL)");
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_WRITETOWAL_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HBASE, "HTableClient_JNI::setWriteToWAL()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_WRITETOWAL_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HTC_OK;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
const char *HTableClient_JNI::getTableName()
{
  return tableName_;
}
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
std::string* HTableClient_JNI::getHTableName()
{
  if (initJNIEnv() != JOI_OK) 
     return NULL;

  tsRecentJMFromJNI = JavaMethods_[JM_GET_HTNAME].jm_full_name;
  jstring js_name = (jstring)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_GET_HTNAME].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::getHTableName()");
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
 
  if (js_name == NULL) {
    jenv_->PopLocalFrame(NULL);
    return NULL;
  }
    
  const char* char_result = jenv_->GetStringUTFChars(js_name, 0);
  std::string* tableName = new (heap_) std::string(char_result);
  jenv_->ReleaseStringUTFChars(js_name, char_result);
  jenv_->PopLocalFrame(NULL);
  return tableName;
}

HTC_RetCode HTableClient_JNI::coProcAggr(Int64 transID, 
					 int aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
					 const Text& startRowID, 
					 const Text& stopRowID, 
					 const Text &colFamily,
					 const Text &colName,
					 const NABoolean cacheBlocks,
					 const Lng32 numCacheRows,
					 Text &aggrVal) // returned value
{

  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HTableClient_JNI::coProcAggr called.");

  int len = 0;

  if (initJNIEnv() != JOI_OK) 
     return HTC_ERROR_INIT_PARAM;

  len = startRowID.size();
  jbyteArray jba_startrowid = jenv_->NewByteArray(len);
  if (jba_startrowid == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_startrowid, 0, len, 
			    (const jbyte*)startRowID.data());

  len = stopRowID.size();
  jbyteArray jba_stoprowid = jenv_->NewByteArray(len);
  if (jba_stoprowid == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_stoprowid, 0, len, 
			    (const jbyte*)stopRowID.data());
 
  len = colFamily.size();
  jbyteArray jba_colfamily = jenv_->NewByteArray(len);
  if (jba_colfamily == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_colfamily, 0, len, 
			    (const jbyte*)colFamily.data());
 
  len = colName.size();
  jbyteArray jba_colname = jenv_->NewByteArray(len);
  if (jba_colname == NULL) 
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }
  jenv_->SetByteArrayRegion(jba_colname, 0, len, 
			    (const jbyte*)colName.data());

  jlong j_tid = transID;  
  jint j_aggrtype = aggrType;

  jboolean j_cb = cacheBlocks;
  jint j_ncr = numCacheRows;

  if (hbs_)
    hbs_->getHbaseTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_COPROC_AGGR].jm_full_name;
  jarray jresult = (jarray)jenv_->CallObjectMethod(javaObj_, 
              JavaMethods_[JM_COPROC_AGGR].methodID, j_tid, 
              j_aggrtype, jba_startrowid, jba_stoprowid, jba_colfamily, 
              jba_colname, j_cb, j_ncr);
  if (hbs_)
    {
      hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
      hbs_->incHbaseCalls();
    }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::coProcAggr()");
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_COPROC_AGGR_EXCEPTION;
  }

  Text *val = NULL;
  if (jresult != NULL)
  {
     jbyte *result = jenv_->GetByteArrayElements((jbyteArray)jresult, NULL);
     int len = jenv_->GetArrayLength(jresult);
     val = new (heap_) Text((char *)result, len);
     jenv_->ReleaseByteArrayElements((jbyteArray)jresult, result, JNI_ABORT);
  }
  if (val == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HTC_ERROR_COPROC_AGGR_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HTC_ERROR_COPROC_AGGR_PARAM;
  }  
  aggrVal = *val;

  jenv_->PopLocalFrame(NULL);
  return HTC_OK;
}

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jint JNICALL Java_org_trafodion_sql_HTableClient_setResultInfo
  (JNIEnv *jenv, jobject jobj, jlong jniObject, 
	jintArray jKvValLen, jintArray jKvValOffset, 
        jintArray jKvQualLen, jintArray jKvQualOffset,
        jintArray jKvFamLen, jintArray jKvFamOffset, 
        jlongArray jTimestamp, 
        jobjectArray jKvBuffer, 
        jobjectArray jKvFamArray, jobjectArray jkvQualArray, jobjectArray jRowIDs,
        jintArray jKvsPerRow, jint numCellsReturned, jint numRowsReturned)
{
   HTableClient_JNI *htc = (HTableClient_JNI *)jniObject;
   if (htc->getFetchMode() == HTableClient_JNI::GET_ROW ||
          htc->getFetchMode() == HTableClient_JNI::BATCH_GET)
      htc->setJavaObject(jobj);
   htc->setResultInfo(jKvValLen, jKvValOffset,
                jKvQualLen, jKvQualOffset, jKvFamLen, jKvFamOffset,
                jTimestamp, jKvBuffer, jKvFamArray, jkvQualArray,
                jRowIDs, jKvsPerRow, numCellsReturned, numRowsReturned);  
   return 0;
}

JNIEXPORT jint JNICALL Java_org_trafodion_sql_HTableClient_setJavaObject
  (JNIEnv *jenv, jobject jobj, jlong jniObject)
{
   HTableClient_JNI *htc = (HTableClient_JNI *)jniObject;
   htc->setJavaObject(jobj);
   return 0;
}

JNIEXPORT void JNICALL Java_org_trafodion_sql_HTableClient_cleanup
  (JNIEnv *jenv, jobject jobj, jlong jniObject)
{
   HTableClient_JNI *htc = (HTableClient_JNI *)jniObject;
   NADELETE(htc, HTableClient_JNI, htc->getHeap()); 
}

#ifdef __cplusplus
}
#endif

void HTableClient_JNI::setResultInfo( jintArray jKvValLen, jintArray jKvValOffset,
        jintArray jKvQualLen, jintArray jKvQualOffset,
        jintArray jKvFamLen, jintArray jKvFamOffset,
        jlongArray jTimestamp, 
        jobjectArray jKvBuffer, 
        jobjectArray jKvFamArray, jobjectArray jKvQualArray, jobjectArray jRowIDs,
        jintArray jKvsPerRow, jint numCellsReturned, jint numRowsReturned)
{
   if (numRowsReturned_ > 0)
      cleanupResultInfo();
   NABoolean exceptionFound = FALSE;
   if (numCellsReturned != 0) {
      jKvValLen_ = (jintArray)jenv_->NewGlobalRef(jKvValLen);
      if (jenv_->ExceptionCheck())
          exceptionFound = TRUE;
      if (! exceptionFound) {
         jKvValOffset_ = (jintArray)jenv_->NewGlobalRef(jKvValOffset);
         if (jenv_->ExceptionCheck())
             exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvQualLen_ = (jintArray)jenv_->NewGlobalRef(jKvQualLen);
         if (jenv_->ExceptionCheck())
             exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvQualOffset_ = (jintArray)jenv_->NewGlobalRef(jKvQualOffset);
         if (jenv_->ExceptionCheck())
             exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvFamLen_ = (jintArray)jenv_->NewGlobalRef(jKvFamLen);
         if (jenv_->ExceptionCheck())
            exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvFamOffset_ = (jintArray)jenv_->NewGlobalRef(jKvFamOffset);
         if (jenv_->ExceptionCheck())
            exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jTimestamp_ = (jlongArray)jenv_->NewGlobalRef(jTimestamp);
         if (jenv_->ExceptionCheck())
            exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvBuffer_ = (jobjectArray)jenv_->NewGlobalRef(jKvBuffer);
         if (jenv_->ExceptionCheck())
            exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvFamArray_ = (jobjectArray)jenv_->NewGlobalRef(jKvFamArray);
         if (jenv_->ExceptionCheck())
            exceptionFound = TRUE;
      }
      if (! exceptionFound) {
         jKvQualArray_ = (jobjectArray)jenv_->NewGlobalRef(jKvQualArray);
         if (jenv_->ExceptionCheck())
            exceptionFound = TRUE;
      }
   }
   if (! exceptionFound) {
      jRowIDs_ = (jobjectArray)jenv_->NewGlobalRef(jRowIDs);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   if (! exceptionFound) {
      jKvsPerRow_ = (jintArray)jenv_->NewGlobalRef(jKvsPerRow);
      if (jenv_->ExceptionCheck())
         exceptionFound = TRUE;
   }
   numCellsReturned_ = numCellsReturned;
   numRowsReturned_ = numRowsReturned;
   prevRowCellNum_ = 0;
   currentRowNum_ = -1;
   cleanupDone_ = FALSE;
   ex_assert(! exceptionFound, "Exception in HTableClient_JNI::setResultInfo");
   return;
} 

void HTableClient_JNI::cleanupResultInfo()
{
   if (cleanupDone_)
      return;
   if (jKvValLen_ != NULL) {
      jenv_->DeleteGlobalRef(jKvValLen_);
      jKvValLen_ = NULL;
   }
   if (jKvValOffset_ != NULL) {
      jenv_->DeleteGlobalRef(jKvValOffset_);
      jKvValOffset_ = NULL;
   }
   if (jKvQualLen_ != NULL) {
      jenv_->DeleteGlobalRef(jKvQualLen_);
      jKvQualLen_ = NULL;
   }
   if (jKvQualOffset_ != NULL) {
      jenv_->DeleteGlobalRef(jKvQualOffset_);
      jKvQualOffset_ = NULL;
   }
   if (jKvFamLen_ != NULL) {
      jenv_->DeleteGlobalRef(jKvFamLen_);
      jKvFamLen_ = NULL;
   }
   if (jKvFamOffset_ != NULL) {
      jenv_->DeleteGlobalRef(jKvFamOffset_);
      jKvFamOffset_ = NULL;
   }
   if (jTimestamp_ != NULL) {
      jenv_->DeleteGlobalRef(jTimestamp_);
      jTimestamp_ = NULL;
   }
   if (jKvBuffer_ != NULL) {
      jenv_->DeleteGlobalRef(jKvBuffer_);
      jKvBuffer_ = NULL;
   }
   if (jKvFamArray_ != NULL) {
      jenv_->DeleteGlobalRef(jKvFamArray_);
      jKvFamArray_ = NULL;
   }
   if (jKvQualArray_ != NULL) {
      jenv_->DeleteGlobalRef(jKvQualArray_);
      jKvQualArray_ = NULL;
   }
   if (jRowIDs_ != NULL) {
      jenv_->DeleteGlobalRef(jRowIDs_);
      jRowIDs_ = NULL;
   }
   if (jba_kvBuffer_ != NULL)
   {
      jenv_->DeleteGlobalRef(jba_kvBuffer_);
      jba_kvBuffer_ = NULL;
   }
   if (jba_kvFamArray_ != NULL)
   {
      jenv_->DeleteGlobalRef(jba_kvFamArray_);
      jba_kvFamArray_ = NULL;
   }
   if (jba_kvQualArray_ != NULL)
   {
      jenv_->DeleteGlobalRef(jba_kvQualArray_);
      jba_kvQualArray_ = NULL;
   }
   if (p_rowID_ != NULL)
   {
      jenv_->ReleaseByteArrayElements(jba_rowID_, p_rowID_, JNI_ABORT);
      p_rowID_ = NULL;
      jenv_->DeleteGlobalRef(jba_rowID_);
   }
   if (p_kvsPerRow_ != NULL)
   {
      jenv_->ReleaseIntArrayElements(jKvsPerRow_, p_kvsPerRow_, JNI_ABORT);
       p_kvsPerRow_ = NULL;
   }
   jenv_->DeleteGlobalRef(jKvsPerRow_);
   cleanupDone_ = TRUE;
   return;
}


HTC_RetCode HTableClient_JNI::nextRow()
{
    HTC_RetCode retCode;

    ex_assert(fetchMode_ != UNKNOWN, "invalid fetchMode_");
    switch (fetchMode_) {
       case GET_ROW:
          if (numRowsReturned_ == -1)
             return HTC_DONE;
          if (currentRowNum_ == -1)
          {
             getResultInfo();
             return HTC_OK;
	  }
          else
          {
             cleanupResultInfo();   
             return HTC_DONE;
          }
          break;
       case BATCH_GET:
          if (numRowsReturned_ == -1)
             return HTC_DONE_RESULT;
          if (currentRowNum_ == -1)
          {
             getResultInfo();
             return HTC_OK;
          }
          else
          if ((currentRowNum_+1) >= numRowsReturned_)
          {
             cleanupResultInfo();   
             return HTC_DONE_RESULT;
          }
          break;
       default:
          break;
    }
    if (fetchMode_ == SCAN_FETCH && (currentRowNum_ == -1 || ((currentRowNum_+1) >= numRowsReturned_)))
    {
        if (currentRowNum_ != -1 && (numRowsReturned_ < numReqRows_))
        {
            cleanupResultInfo();
            return HTC_DONE;
        }   
        retCode = fetchRows();
        if (retCode != HTC_OK)
        {
           cleanupResultInfo();
           return retCode;
        }
        getResultInfo();
    }
    else
    {
        // Add the number of previous cells returned
        jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
        prevRowCellNum_ += kvsPerRow;  
        currentRowNum_++;
        currentRowCellNum_ = 0;
    }
    // clean the rowID of the previous row
    if (p_rowID_ != NULL)
    {
       jenv_->ReleaseByteArrayElements(jba_rowID_, p_rowID_, JNI_ABORT);
       p_rowID_ = NULL;
       jenv_->DeleteGlobalRef(jba_rowID_);
    }
    return HTC_OK;
}

void HTableClient_JNI::getResultInfo()
{
   // Allocate Buffer and copy the cell info
   int numCellsNeeded;
   if (numCellsReturned_ == 0)
   {
      p_kvsPerRow_ = jenv_->GetIntArrayElements(jKvsPerRow_, NULL);
      currentRowNum_ = 0;
      currentRowCellNum_ = 0;
      prevRowCellNum_ = 0;
      return;
   }
   if (numCellsAllocated_ == 0 || 
		numCellsAllocated_ < numCellsReturned_) {
      NAHeap *heap = getHeap();
      if (numCellsAllocated_ > 0) {
          NADELETEBASIC(p_kvValLen_, heap);
          NADELETEBASIC(p_kvValOffset_, heap);
          NADELETEBASIC(p_kvFamLen_, heap);
          NADELETEBASIC(p_kvFamOffset_, heap);
          NADELETEBASIC(p_kvQualLen_, heap);
          NADELETEBASIC(p_kvQualOffset_, heap);
          NADELETEBASIC(p_timestamp_, heap);
          numCellsNeeded = numCellsReturned_;
       }
       else {  
          if (numColsInScan_ == 0)
              numCellsNeeded = numCellsReturned_;
          else    
              numCellsNeeded = 2 * numReqRows_ * numColsInScan_;
       }
       p_kvValLen_ = new (heap) jint[numCellsNeeded];
       p_kvValOffset_ = new (heap) jint[numCellsNeeded];
       p_kvFamLen_ = new (heap) jint[numCellsNeeded];
       p_kvFamOffset_ = new (heap) jint[numCellsNeeded];
       p_kvQualLen_ = new (heap) jint[numCellsNeeded];
       p_kvQualOffset_ = new (heap) jint[numCellsNeeded];
       p_timestamp_ = new (heap) jlong[numCellsNeeded];
       numCellsAllocated_ = numCellsNeeded;
    }
    jenv_->GetIntArrayRegion(jKvValLen_, 0, numCellsReturned_, p_kvValLen_);
    jenv_->GetIntArrayRegion(jKvValOffset_, 0, numCellsReturned_, p_kvValOffset_);
    jenv_->GetIntArrayRegion(jKvQualLen_, 0, numCellsReturned_, p_kvQualLen_);
    jenv_->GetIntArrayRegion(jKvQualOffset_, 0, numCellsReturned_, p_kvQualOffset_);
    jenv_->GetIntArrayRegion(jKvFamLen_, 0, numCellsReturned_, p_kvFamLen_);
    jenv_->GetIntArrayRegion(jKvFamOffset_, 0, numCellsReturned_, p_kvFamOffset_);
    jenv_->GetLongArrayRegion(jTimestamp_, 0, numCellsReturned_, p_timestamp_);
    p_kvsPerRow_ = jenv_->GetIntArrayElements(jKvsPerRow_, NULL);
    currentRowNum_ = 0;
    currentRowCellNum_ = 0;
    prevRowCellNum_ = 0;
}

HTC_RetCode HTableClient_JNI::prepareForNextCell(int idx)
{
    jobject kvBufferObj;

    if (jba_kvFamArray_ != NULL)
    {
       jenv_->DeleteGlobalRef(jba_kvFamArray_);
       jba_kvFamArray_ = NULL;
    }
    kvBufferObj = jenv_->GetObjectArrayElement(jKvFamArray_, idx);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::prepareForNextCell()");
      return HTC_PREPARE_FOR_NEXTCELL_EXCEPTION;
    }
    jba_kvFamArray_ = (jbyteArray)jenv_->NewGlobalRef(kvBufferObj);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::prepareForNextCell()");
      return HTC_PREPARE_FOR_NEXTCELL_EXCEPTION;
    }
    jenv_->DeleteLocalRef(kvBufferObj);

    if (jba_kvQualArray_ != NULL)
    {
       jenv_->DeleteGlobalRef(jba_kvQualArray_);
       jba_kvQualArray_ = NULL;
    }
    kvBufferObj = jenv_->GetObjectArrayElement(jKvQualArray_, idx);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::prepareForNextCell()");
      return HTC_PREPARE_FOR_NEXTCELL_EXCEPTION;
    }
    jba_kvQualArray_ = (jbyteArray)jenv_->NewGlobalRef(kvBufferObj);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::prepareForNextCell()");
      return HTC_PREPARE_FOR_NEXTCELL_EXCEPTION;
    }
    jenv_->DeleteLocalRef(kvBufferObj);

    if (jba_kvBuffer_ != NULL)
    {
       jenv_->DeleteGlobalRef(jba_kvBuffer_);
       jba_kvBuffer_ = NULL;
    }
    kvBufferObj = jenv_->GetObjectArrayElement(jKvBuffer_, idx);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::prepareForNextCell()");
      return HTC_PREPARE_FOR_NEXTCELL_EXCEPTION;
    }
    jba_kvBuffer_ = (jbyteArray)jenv_->NewGlobalRef(kvBufferObj);
    if (jenv_->ExceptionCheck())
    {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::prepareForNextCell()");
      return HTC_PREPARE_FOR_NEXTCELL_EXCEPTION;
    }
    jenv_->DeleteLocalRef(kvBufferObj);

    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::getColName(int colNo,
              char **outColName, 
              short &colNameLen,
              Int64 &timestamp)
{
    HTC_RetCode retcode;

    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (kvsPerRow == 0 || colNo >= kvsPerRow)
    {
       *outColName == NULL;
       timestamp = 0;
       return HTC_OK;
    }
    int idx = prevRowCellNum_ + colNo;
    ex_assert((idx < numCellsReturned_), "Buffer overflow");
    jint kvQualLen = p_kvQualLen_[idx];
    jint kvQualOffset = p_kvQualOffset_[idx];
    jint kvFamLen = p_kvFamLen_[idx];
    jint kvFamOffset = p_kvFamOffset_[idx];

    if ((retcode = prepareForNextCell(idx)) != HTC_OK)
       return retcode;
   
    colNameLen = kvQualLen + kvFamLen + 1; // 1 for ':'
    char * colName;
    if (colNameAllocLen_ == 0  && colNameLen <= INLINE_COLNAME_LEN)
    	colName = inlineColName_;
    else
    {
        if (colNameLen > colNameAllocLen_)
        {
	   if (colNameAllocLen_ != 0)
              NADELETEBASIC(colName_, heap_);
           colName_ = new (heap_) char[colNameLen+1];
           colNameAllocLen_ = colNameLen;
        }
        colName = colName_; 
    }
    jenv_->GetByteArrayRegion(jba_kvFamArray_, kvFamOffset, kvFamLen, 
            (jbyte *)colName);
    colName[kvFamLen] = ':';
    char *temp = colName+ kvFamLen+1;
    jenv_->GetByteArrayRegion(jba_kvQualArray_, kvQualOffset, kvQualLen, 
            (jbyte *)temp);
    timestamp = p_timestamp_[idx];
    *outColName = colName;
    if (hbs_)
      hbs_->incBytesRead(sizeof(timestamp) + colNameLen);
    return HTC_OK; 
}

HTC_RetCode HTableClient_JNI::getColVal(int colNo, BYTE *colVal, 
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (kvsPerRow == 0 || colNo >= kvsPerRow)
       return HTC_GET_COLVAL_EXCEPTION;
    int idx = prevRowCellNum_ + colNo;
    ex_assert((idx < numCellsReturned_), "Buffer overflow");
    jint kvValLen = p_kvValLen_[idx];
    jint kvValOffset = p_kvValOffset_[idx];
    Lng32 copyLen;
    Lng32 dataLen;
    jbyte nullByte;
    // If the column is nullable, get the first byte
    // The first byte determines if the column is null(0xff) or not (0)
    if (nullable)
    {
      dataLen = kvValLen - 1; 
      copyLen = MINOF(dataLen, colValLen);
      jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset, 1, &nullByte); 
      jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset+1, copyLen, 
                                (jbyte *)colVal); 
    }
    else 
    {
      dataLen = kvValLen;
      copyLen = MINOF(dataLen, colValLen);
      nullByte = 0;
      jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset, copyLen,
                                (jbyte *)colVal); 
    }
    nullVal = nullByte;
    if (dataLen > colValLen)
      colValLen = dataLen;
    else
      colValLen = copyLen;
    if (hbs_)
      hbs_->incBytesRead(copyLen);
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::getColVal(NAHeap *heap, int colNo, BYTE **colVal, 
          Lng32 &colValLen)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (kvsPerRow == 0 || colNo >= kvsPerRow)
       return HTC_GET_COLVAL_EXCEPTION;
    int idx = prevRowCellNum_ + colNo;
    ex_assert((idx < numCellsReturned_), "Buffer overflow");
    jint kvValLen = p_kvValLen_[idx];
    jint kvValOffset = p_kvValOffset_[idx];
   
    BYTE *colValTmp;
    int colValLenTmp;
    if (heap == NULL)
    {
       colValTmp = *colVal; 
       colValLenTmp = colValLen;
       if (colValLenTmp > kvValLen)
          colValLenTmp = kvValLen;
    }
    else
    {
       colValTmp = new (heap) BYTE[kvValLen];
       colValLenTmp = kvValLen;
    }
    jenv_->GetByteArrayRegion(jba_kvBuffer_, kvValOffset, colValLenTmp,
             (jbyte *)colValTmp); 
    *colVal = colValTmp;
    colValLen = colValLenTmp;
    if (hbs_)
      hbs_->incBytesRead(colValLen);
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::getNumCellsPerRow(int &numCells)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    numCells = kvsPerRow;
    if (numCells == 0)
       return HTC_DONE_DATA;
    else
       return HTC_OK;
}  


HTC_RetCode HTableClient_JNI::getRowID(HbaseStr &rowID)
{
    jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
    if (p_rowID_ != NULL)
    {
      jenv_->ReleaseByteArrayElements(jba_rowID_, p_rowID_, JNI_ABORT);
      p_rowID_ = NULL;
      jenv_->DeleteGlobalRef(jba_rowID_);
    }

    if (kvsPerRow == 0) 
    {
       rowID.len = 0;
       rowID.val = NULL;
    }
    else
    {
       jobject rowIDObj;
       rowIDObj = jenv_->GetObjectArrayElement(jRowIDs_, currentRowNum_);
       jba_rowID_ = (jbyteArray)jenv_->NewGlobalRef(rowIDObj);
       if (jenv_->ExceptionCheck())
       {
          getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::getRowID()");
          return HTC_GET_ROWID_EXCEPTION;
       }
       jenv_->DeleteLocalRef(rowIDObj);
       p_rowID_ = jenv_->GetByteArrayElements(jba_rowID_, NULL);
       rowIDLen_ = jenv_->GetArrayLength(jba_rowID_); 
       rowID.len = rowIDLen_;
       rowID.val = (char *)p_rowID_;
    }
    return HTC_OK;
}

HTC_RetCode HTableClient_JNI::fetchRows()
{
   QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "HTableClient_JNI::fetchRows() called.");

   if (initJNIEnv() != JOI_OK) 
      return HTC_ERROR_INIT_PARAM;

   jlong jniObject = (jlong)this;
   if (hbs_)
     hbs_->getHbaseTimer().start();
   tsRecentJMFromJNI = JavaMethods_[JM_FETCH_ROWS].jm_full_name;
   jint jRowsReturned = jenv_->CallIntMethod(javaObj_, 
             JavaMethods_[JM_FETCH_ROWS].methodID,
             jniObject);
   if (hbs_)
    {
      hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
      hbs_->incHbaseCalls();
    }

   if (jenv_->ExceptionCheck())
   {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::fetchRows()");
      jenv_->PopLocalFrame(NULL);
      return HTC_ERROR_FETCHROWS_EXCEPTION;
   }

   numRowsReturned_ = jRowsReturned;
   if (numRowsReturned_ == 0) {
      jenv_->PopLocalFrame(NULL);
      return HTC_DONE;
   }
   if (hbs_)
      hbs_->incAccessedRows(numRowsReturned_);
   jenv_->PopLocalFrame(NULL);
   return HTC_OK; 
}

HTC_RetCode HTableClient_JNI::nextCell(
        	 HbaseStr &rowId,
                 HbaseStr &colFamName,
                 HbaseStr &colQualName,
                 HbaseStr &colVal,
                 Int64 &timestamp)
{
   HTC_RetCode retcode;
   jint kvsPerRow = p_kvsPerRow_[currentRowNum_];
   if (currentRowCellNum_ >= kvsPerRow)
   {
      currentRowCellNum_ = -1;
      return HTC_DONE;
   }
   if (p_rowID_ != NULL)
   {
      rowId.val = (char *)p_rowID_;
      rowId.len = rowIDLen_;
   }
   else
   {
      retcode = getRowID(rowId);
      if (retcode != HTC_OK)
         return retcode;
   }
   int idx = prevRowCellNum_ + currentRowCellNum_;
   ex_assert((idx < numCellsReturned_), "Buffer overflow");
   jint kvQualLen = p_kvQualLen_[idx];
   jint kvQualOffset = p_kvQualOffset_[idx];
   jint kvFamLen = p_kvFamLen_[idx];
   jint kvFamOffset = p_kvFamOffset_[idx];

   if ((retcode = prepareForNextCell(idx)) != HTC_OK)
       return retcode;

   int colNameLen = kvQualLen + kvFamLen + 1; // 1 for ':'
   char * colName;
   if (colNameAllocLen_ == 0  && colNameLen <= INLINE_COLNAME_LEN)
      colName = inlineColName_;
   else
   {
      if (colNameLen > colNameAllocLen_)
      {
         if (colNameAllocLen_ != 0)
             NADELETEBASIC(colName_, heap_);
         colName_ = new (heap_) char[colNameLen+1];
         colNameAllocLen_ = colNameLen;
      }
      colName = colName_;
   }
   jenv_->GetByteArrayRegion(jba_kvFamArray_, kvFamOffset, kvFamLen,
            (jbyte *)colName);
   colName[kvFamLen] = ':';
   colFamName.val = colName;
   colFamName.len = kvFamLen; 
   char *temp = colName+ kvFamLen+1;
   jenv_->GetByteArrayRegion(jba_kvQualArray_, kvQualOffset, kvQualLen,
            (jbyte *)temp);
   colQualName.val = temp;
   colQualName.len = kvQualLen;
   timestamp = p_timestamp_[idx];
   retcode = getColVal(NULL, currentRowCellNum_, (BYTE **)&colVal.val,
                         colVal.len);
   if (retcode != HTC_OK)
      return retcode;
   currentRowCellNum_++;
   return HTC_OK;
}

HTC_RetCode HTableClient_JNI::completeAsyncOperation(Int32 timeout, NABoolean *resultArray, Int16 resultArrayLen)
{
  HTC_RetCode retcode;

  if (initJNIEnv() != JOI_OK) {
     if (hbs_)
        hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
     return HTC_ERROR_COMPLETEASYNCOPERATION_EXCEPTION;
  }
  jint jtimeout = timeout;
  jbooleanArray jresultArray =  jenv_->NewBooleanArray(resultArrayLen);
  if (jenv_->ExceptionCheck()) {
      getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::completeAsyncOperation()");
      jenv_->PopLocalFrame(NULL);
      if (hbs_)
         hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
      return HTC_ERROR_COMPLETEASYNCOPERATION_EXCEPTION;
  }
  tsRecentJMFromJNI = JavaMethods_[JM_COMPLETE_PUT].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_COMPLETE_PUT].methodID,
                               jtimeout, jresultArray);
  if (jenv_->ExceptionCheck()) {
    getExceptionDetails(__FILE__, __LINE__, "HTableClient_JNI::completeAsyncOperation()");
    jenv_->PopLocalFrame(NULL);
    if (hbs_)
       hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
    return HTC_ERROR_COMPLETEASYNCOPERATION_EXCEPTION;
  }
  if (jresult == false) {
     jenv_->PopLocalFrame(NULL);
     return HTC_ERROR_ASYNC_OPERATION_NOT_COMPLETE;
  }
  if (hbs_)
     hbs_->incMaxHbaseIOTime(hbs_->getHbaseTimer().stop());
  jboolean *returnArray = jenv_->GetBooleanArrayElements(jresultArray, NULL);
  for (int i = 0; i < resultArrayLen; i++) 
      resultArray[i] = returnArray[i]; 
  jenv_->ReleaseBooleanArrayElements(jresultArray, returnArray, JNI_ABORT);
  jenv_->PopLocalFrame(NULL);
  return HTC_OK;
}

jobjectArray convertToByteArrayObjectArray(const LIST(NAString) &vec)
{
   int vecLen = vec.entries();
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( ; i < vec.entries(); i++)
   {
       const NAString *naStr = &vec.at(i);
       jbyteArray j_obj = jenv_->NewByteArray(naStr->length());
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       jenv_->SetByteArrayRegion(j_obj, 0, naStr->length(), (const jbyte *)naStr->data());
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(vecLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

jobjectArray convertToByteArrayObjectArray(const LIST(HbaseStr) &vec)
{
   int vecLen = vec.entries();
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( ; i < vec.entries(); i++)
   {
       const HbaseStr *hbStr = &vec.at(i);
       jbyteArray j_obj = jenv_->NewByteArray(hbStr->len);
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       jenv_->SetByteArrayRegion(j_obj, 0, hbStr->len, (const jbyte *)hbStr->val);
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(vecLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

jobjectArray convertToByteArrayObjectArray(const char **array,
                   int numElements, int elementLen)
{
   int i = 0;
   jobjectArray j_objArray = NULL;
   for (i = 0; i < numElements; i++)
   {
       jbyteArray j_obj = jenv_->NewByteArray(elementLen);
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       jenv_->SetByteArrayRegion(j_obj, 0, elementLen,
             (const jbyte *)(array[i]));
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(numElements,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

jobjectArray convertToStringObjectArray(const TextVec &vec)
{
   int vecLen = vec.size();
   int i = 0;
   jobjectArray j_objArray = NULL;
   for (std::vector<Text>::const_iterator it = vec.begin(); 
           it != vec.end(); ++it, i++)
   {
       jstring j_obj = jenv_->NewStringUTF((*it).data());
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(vecLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

jobjectArray convertToStringObjectArray(const HBASE_NAMELIST& nameList)
{
   int listLen = nameList.entries();
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( i = 0; i < listLen ; i++)
   {
       jstring j_obj = jenv_->NewStringUTF(nameList.at(i).val);
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(listLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

jobjectArray convertToStringObjectArray(const NAText *textArray, int arrayLen)
{
   int i = 0;
   jobjectArray j_objArray = NULL;
   for ( i = 0; i < arrayLen ; i++)
   {
       jstring j_obj = jenv_->NewStringUTF(textArray[i].c_str());
       if (jenv_->ExceptionCheck())
       {
          if (j_objArray != NULL)
             jenv_->DeleteLocalRef(j_objArray);
          return NULL; 
       }
       if (j_objArray == NULL)
       {
          j_objArray = jenv_->NewObjectArray(arrayLen,
                 jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck())
          {
             jenv_->DeleteLocalRef(j_obj);
             return NULL;
          }
       }
       jenv_->SetObjectArrayElement(j_objArray, i, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);
   }
   return j_objArray;
}

int convertStringObjectArrayToList(NAHeap *heap, jarray j_objArray, 
                                         LIST(Text *)&list)
{

    if (j_objArray == NULL)
        return 0;
    int arrayLen = jenv_->GetArrayLength(j_objArray);
    jstring j_str;
    const char *str;
    jboolean isCopy;

    for (int i = 0; i < arrayLen; i++)
    {
        j_str = (jstring)jenv_->GetObjectArrayElement((jobjectArray)j_objArray, i);
        str = jenv_->GetStringUTFChars(j_str, &isCopy);
        list.insert(new (heap) Text(str));
        jenv_->ReleaseStringUTFChars(j_str, str);        
    }
    return arrayLen;
}


jint convertByteArrayObjectArrayToNAArray(NAHeap *heap, jarray j_objArray, NAArray<HbaseStr> **retArray)
{
    if (j_objArray == NULL) {
       *retArray = NULL;
       return 0;
    }
    int arrayLen = jenv_->GetArrayLength(j_objArray);
    jbyteArray j_ba;
    jint j_baLen;
    BYTE *ba;
    jboolean isCopy;
    HbaseStr element; 
    NAArray<HbaseStr> *tmpArray = new (heap) NAArray<HbaseStr> (heap, arrayLen); 
    for (int i = 0; i < arrayLen; i++) {
        j_ba = (jbyteArray)jenv_->GetObjectArrayElement((jobjectArray)j_objArray, i);
        j_baLen = jenv_->GetArrayLength(j_ba);
        ba = new (heap) BYTE[j_baLen];
        jenv_->GetByteArrayRegion(j_ba, 0, j_baLen, (jbyte *)ba); 
        element.len = j_baLen;
        element.val = (char *)ba;
        tmpArray->insert(i,element);
    }
    *retArray = tmpArray;
    return arrayLen;
}

void deleteNAArray(CollHeap *heap, NAArray<HbaseStr> *array)
{
  
  if (array == NULL)
     return;
  CollIndex entryCount = array->entries();
  for (CollIndex i = 0 ; i < entryCount; i++) {
      NADELETEBASIC(array->at(i).val, heap);
  }
  NADELETE(array, NAArray, heap);
}
