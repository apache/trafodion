//**********************************************************************
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
#include "jni.h"
#include "HdfsClient_JNI.h"

// ===========================================================================
// ===== Class HdfsScan
// ===========================================================================

JavaMethodInit* HdfsScan::JavaMethods_ = NULL;
jclass HdfsScan::javaClass_ = 0;
bool HdfsScan::javaMethodsInitialized_ = false;
pthread_mutex_t HdfsScan::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const hdfsScanErrorEnumStr[] = 
{
   "Error in HdfsScan::setScanRanges"
  ,"Java Exception in HdfsScan::setScanRanges"
  ,"Error in HdfsScan::trafHdfsRead"
  ,"Java Exceptiokn in HdfsScan::trafHdfsRead"
  , "Hdfs scan End of Ranges"
};

 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HDFS_Scan_RetCode HdfsScan::init()
{
  static char className[]="org/trafodion/sql/HdfsScan";
  HDFS_Scan_RetCode rc; 

  if (javaMethodsInitialized_)
    return (HDFS_Scan_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_); 
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HDFS_Scan_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_SET_SCAN_RANGES].jm_name      = "setScanRanges";
    JavaMethods_[JM_SET_SCAN_RANGES].jm_signature = "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;[Ljava/lang/String;[J[J[I)V";
    JavaMethods_[JM_TRAF_HDFS_READ].jm_name      = "trafHdfsRead";
    JavaMethods_[JM_TRAF_HDFS_READ].jm_signature = "()[I";
   
    rc = (HDFS_Scan_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == HDFS_SCAN_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}
        
char* HdfsScan::getErrorText(HDFS_Scan_RetCode errEnum)
{
  if (errEnum < (HDFS_Scan_RetCode)JOI_LAST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else
    return (char*)hdfsScanErrorEnumStr[errEnum-HDFS_SCAN_FIRST];
}

/////////////////////////////////////////////////////////////////////////////
HDFS_Scan_RetCode HdfsScan::setScanRanges(NAHeap *heap, ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf,  int scanBufSize,
      HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, int rangeTailIOSize,
      ExHdfsScanStats *hdfsStats)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsScan::setScanRanges() called.");

  if (initJNIEnv() != JOI_OK)
     return HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM;

   jobject j_buf1 = jenv_->NewDirectByteBuffer(hdfsScanBuf[0].buf_, scanBufSize);
   if (j_buf1 == NULL) {
      GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM));
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM;
   }

   jobject j_buf2 = jenv_->NewDirectByteBuffer(hdfsScanBuf[1].buf_, scanBufSize);
   if (j_buf2 == NULL) {
      GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM));
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM;
   }
   jobjectArray j_filenames = NULL;
   jlongArray j_offsets = NULL;
   jlongArray j_lens = NULL;  
   jintArray j_rangenums = NULL;
   HdfsFileInfo *hdfo;
   jstring j_obj;

   HDFS_Scan_RetCode hdfsScanRetCode =  HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM;
   int arrayLen = hdfsFileInfoArray->entries();
   
   for (Int32 i = beginRangeNum, rangeCount=0; i < arrayLen; i++, rangeCount++) {
       if (rangeCount >= numRanges)
           break;
       hdfo = hdfsFileInfoArray->at(i);
       j_obj = jenv_->NewStringUTF(hdfo->fileName());
       if (jenv_->ExceptionCheck()) {
          jenv_->PopLocalFrame(NULL);
          return hdfsScanRetCode;
       }
       if (j_filenames == NULL) {
          j_filenames = jenv_->NewObjectArray(numRanges, jenv_->GetObjectClass(j_obj), NULL);
          if (jenv_->ExceptionCheck()) {
             jenv_->PopLocalFrame(NULL);
             return hdfsScanRetCode;
          }
       }
       jenv_->SetObjectArrayElement(j_filenames, rangeCount, (jobject)j_obj);
       jenv_->DeleteLocalRef(j_obj);

       if (j_offsets == NULL) {
          j_offsets = jenv_->NewLongArray(numRanges);
          if (jenv_->ExceptionCheck()) {
             jenv_->PopLocalFrame(NULL);
             return hdfsScanRetCode;
          }
       }
       long offset = hdfo->getStartOffset(); 
       jenv_->SetLongArrayRegion(j_offsets, rangeCount, 1, &offset);

       if (j_lens == NULL) {
          j_lens = jenv_->NewLongArray(numRanges);
          if (jenv_->ExceptionCheck()) {
             jenv_->PopLocalFrame(NULL);
             return hdfsScanRetCode;
          }
       }
       long len = hdfo->getBytesToRead()+rangeTailIOSize;
       jenv_->SetLongArrayRegion(j_lens, rangeCount, 1, &len);

       if (j_rangenums == NULL) {
          j_rangenums = jenv_->NewIntArray(numRanges);
          if (jenv_->ExceptionCheck()) {
             jenv_->PopLocalFrame(NULL);
             return hdfsScanRetCode;
          }
       }
       jint tdbRangeNum = i;
       jenv_->SetIntArrayRegion(j_rangenums, rangeCount, 1, &tdbRangeNum);
   } 

   if (hdfsStats)
       hdfsStats->getHdfsTimer().start();
   tsRecentJMFromJNI = JavaMethods_[JM_SET_SCAN_RANGES].jm_full_name;
   jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_SET_SCAN_RANGES].methodID, j_buf1, j_buf2, j_filenames, j_offsets, j_lens, j_rangenums);
   if (hdfsStats) {
      hdfsStats->incMaxHdfsIOTime(hdfsStats->getHdfsTimer().stop());
      hdfsStats->incHdfsCalls();
   }

   if (jenv_->ExceptionCheck()) {
      getExceptionDetails();
      logError(CAT_SQL_HDFS, __FILE__, __LINE__);
      logError(CAT_SQL_HDFS, "HdfsScan::setScanRanges()", getLastError());
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_SET_SCAN_RANGES_EXCEPTION;
   }
   return HDFS_SCAN_OK; 
}

HdfsScan *HdfsScan::newInstance(NAHeap *heap, ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf,  int scanBufSize,
      HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, int rangeTailIOSize, 
      ExHdfsScanStats *hdfsStats, HDFS_Scan_RetCode &hdfsScanRetCode)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsScan::newInstance() called.");

   if (initJNIEnv() != JOI_OK)
     return NULL;
   hdfsScanRetCode = HDFS_SCAN_OK;
   HdfsScan *hdfsScan = new (heap) HdfsScan(heap);
   if (hdfsScan != NULL) {
       hdfsScanRetCode = hdfsScan->init();
       if (hdfsScanRetCode == HDFS_SCAN_OK) 
          hdfsScanRetCode = hdfsScan->setScanRanges(heap, hdfsScanBuf, scanBufSize, 
                    hdfsFileInfoArray, beginRangeNum, numRanges, rangeTailIOSize, hdfsStats); 
       if (hdfsScanRetCode != HDFS_SCAN_OK) {
          NADELETE(hdfsScan, HdfsScan, heap);
          hdfsScan = NULL;
       }
   }
   return hdfsScan;
}


HDFS_Scan_RetCode HdfsScan::trafHdfsRead(NAHeap *heap, ExHdfsScanStats *hdfsStats, int retArray[], short arrayLen)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsScan::trafHdfsRead() called.");

   if (initJNIEnv() != JOI_OK)
     return HDFS_SCAN_ERROR_TRAF_HDFS_READ_PARAM;

   if (hdfsStats)
       hdfsStats->getHdfsTimer().start();
   tsRecentJMFromJNI = JavaMethods_[JM_TRAF_HDFS_READ].jm_full_name;
   jintArray j_retArray = (jintArray)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_TRAF_HDFS_READ].methodID);
   if (hdfsStats) {
      hdfsStats->incMaxHdfsIOTime(hdfsStats->getHdfsTimer().stop());
      hdfsStats->incHdfsCalls();
   }

   if (jenv_->ExceptionCheck()) {
      getExceptionDetails();
      logError(CAT_SQL_HDFS, __FILE__, __LINE__);
      logError(CAT_SQL_HDFS, "HdfsScan::setScanRanges()", getLastError());
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_TRAF_HDFS_READ_EXCEPTION;
   }
   if (j_retArray == NULL)
      return HDFS_SCAN_EOR;
   short retArrayLen = jenv_->GetArrayLength(j_retArray);
   ex_assert(retArrayLen == arrayLen, "HdfsScan::trafHdfsRead() InternalError: retArrayLen != arrayLen");
   jenv_->GetIntArrayRegion(j_retArray, 0, 4, retArray);
   return HDFS_SCAN_OK;
}

// ===========================================================================
// ===== Class HdfsClient
// ===========================================================================

JavaMethodInit* HdfsClient::JavaMethods_ = NULL;
jclass HdfsClient::javaClass_ = 0;
bool HdfsClient::javaMethodsInitialized_ = false;
pthread_mutex_t HdfsClient::javaMethodsInitMutex_ = PTHREAD_MUTEX_INITIALIZER;

static const char* const hdfsClientErrorEnumStr[] = 
{
  "JNI NewStringUTF() in hdfsCreate()."
 ,"Java exception in hdfsCreate()."
  "JNI NewStringUTF() in hdfsOpen()."
 ,"Java exception in hdfsOpen()."
 ,"JNI NewStringUTF() in hdfsWrite()."
 ,"Java exception in hdfsWrite()."
 ,"Java exception in hdfsClose()."
 ,"JNI NewStringUTF() in hdfsMergeFiles()."
 ,"Java exception in hdfsMergeFiles()."
 ,"JNI NewStringUTF() in hdfsCleanUnloadPath()."
 ,"Java exception in hdfsCleanUnloadPath()."
 ,"JNI NewStringUTF() in hdfsExists()."
 ,"Java exception in hdfsExists()."
 ,"JNI NewStringUTF() in hdfsDeletePath()."
 ,"Java exception in hdfsDeletePath()."
 ,"Error in setHdfsFileInfo()."
 ,"Error in hdfsListDirectory()."
 ,"Java exception in hdfsListDirectory()."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HdfsClient::~HdfsClient()
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::~HdfsClient() called.");
   deleteHdfsFileInfo();
}

void HdfsClient::deleteHdfsFileInfo()
{
   for (int i = 0; i < numFiles_ ; i ++) {
      NADELETEBASIC(hdfsFileInfo_[i].mName, getHeap());
      NADELETEBASIC(hdfsFileInfo_[i].mOwner, getHeap());
      NADELETEBASIC(hdfsFileInfo_[i].mGroup, getHeap());
   }
   NADELETEBASIC(hdfsFileInfo_, getHeap()); 
   numFiles_ = 0;
   hdfsFileInfo_ = NULL;
}

HdfsClient *HdfsClient::newInstance(NAHeap *heap, HDFS_Client_RetCode &retCode)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::newInstance() called.");

   if (initJNIEnv() != JOI_OK)
     return NULL;
   retCode = HDFS_CLIENT_OK;
   HdfsClient *hdfsClient = new (heap) HdfsClient(heap);
   if (hdfsClient != NULL) {
       retCode = hdfsClient->init();
       if (retCode != HDFS_CLIENT_OK) {
          NADELETE(hdfsClient, HdfsClient, heap);
          hdfsClient = NULL;
       }
   }
   return hdfsClient;
}

HDFS_Client_RetCode HdfsClient::init()
{
  static char className[]="org/trafodion/sql/HDFSClient";
  HDFS_Client_RetCode rc;
  
  if (javaMethodsInitialized_)
    return (HDFS_Client_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
  else
  {
    pthread_mutex_lock(&javaMethodsInitMutex_);
    if (javaMethodsInitialized_)
    {
      pthread_mutex_unlock(&javaMethodsInitMutex_);
      return (HDFS_Client_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    }
    JavaMethods_ = new JavaMethodInit[JM_LAST];
    
    JavaMethods_[JM_CTOR      ].jm_name      = "<init>";
    JavaMethods_[JM_CTOR      ].jm_signature = "()V";
    JavaMethods_[JM_HDFS_CREATE     ].jm_name      = "hdfsCreate";
    JavaMethods_[JM_HDFS_CREATE     ].jm_signature = "(Ljava/lang/String;Z)Z";
    JavaMethods_[JM_HDFS_OPEN       ].jm_name      = "hdfsOpen";
    JavaMethods_[JM_HDFS_OPEN       ].jm_signature = "(Ljava/lang/String;Z)Z";
    JavaMethods_[JM_HDFS_WRITE      ].jm_name      = "hdfsWrite";
    JavaMethods_[JM_HDFS_WRITE      ].jm_signature = "([BJ)Z";
    JavaMethods_[JM_HDFS_CLOSE      ].jm_name      = "hdfsClose";
    JavaMethods_[JM_HDFS_CLOSE      ].jm_signature = "()Z";
    JavaMethods_[JM_HDFS_MERGE_FILES].jm_name      = "hdfsMergeFiles";
    JavaMethods_[JM_HDFS_MERGE_FILES].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_CLEAN_UNLOAD_PATH].jm_name      = "hdfsCleanUnloadPath";
    JavaMethods_[JM_HDFS_CLEAN_UNLOAD_PATH].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_EXISTS].jm_name      = "hdfsExists";
    JavaMethods_[JM_HDFS_EXISTS].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_DELETE_PATH].jm_name      = "hdfsDeletePath";
    JavaMethods_[JM_HDFS_DELETE_PATH].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_LIST_DIRECTORY].jm_name      = "hdfsListDirectory";
    JavaMethods_[JM_HDFS_LIST_DIRECTORY].jm_signature = "(Ljava/lang/String;J)I";
    rc = (HDFS_Client_RetCode)JavaObjectInterface::init(className, javaClass_, JavaMethods_, (Int32)JM_LAST, javaMethodsInitialized_);
    if (rc == HDFS_CLIENT_OK)
       javaMethodsInitialized_ = TRUE;
    pthread_mutex_unlock(&javaMethodsInitMutex_);
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
char* HdfsClient::getErrorText(HDFS_Client_RetCode errEnum)
{
  if (errEnum < (HDFS_Client_RetCode)HDFS_CLIENT_FIRST)
    return JavaObjectInterface::getErrorText((JOI_RetCode)errEnum);
  else
    return (char*)hdfsClientErrorEnumStr[errEnum-HDFS_CLIENT_FIRST];
}

HDFS_Client_RetCode HdfsClient::hdfsCreate(const char* path, NABoolean compress)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsCreate(%s) called.", path);

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM;

  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM;
  }

  jboolean j_compress = compress;

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_CREATE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_CREATE].methodID, js_path, j_compress);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsCreate()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CREATE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsCreate()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM;
  }

  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsOpen(const char* path, NABoolean compress)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsOpen(%s) called.", path);

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM;

  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM;
  }

  jboolean j_compress = compress;

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_OPEN].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_OPEN].methodID, js_path, j_compress);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsOpen()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_OPEN_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsOpen()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM;
  }

  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}


HDFS_Client_RetCode HdfsClient::hdfsWrite(const char* data, Int64 len)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsWrite(%ld) called.", len);

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION;

  //Write the requisite bytes into the file
  jbyteArray jbArray = jenv_->NewByteArray( len);
  if (!jbArray) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_WRITE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_WRITE_PARAM;
  }
  jenv_->SetByteArrayRegion(jbArray, 0, len, (const jbyte*)data);

  jlong j_len = len;
  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_WRITE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_WRITE].methodID,jbArray , j_len);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsWrite()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsWrite()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION;
  }


  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
HDFS_Client_RetCode HdfsClient::hdfsClose()
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::close() called.");

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_CLOSE_EXCEPTION;

  // String close();
  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_CLOSE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_CLOSE].methodID);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsClose()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CLOSE_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsClose()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CLOSE_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsCleanUnloadPath( const NAString& uldPath)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsCleanUnloadPath(%s) called.",
                                                                             uldPath.data());
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_CLEANUP_PARAM;
  jstring js_UldPath = jenv_->NewStringUTF(uldPath.data());
  if (js_UldPath == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_CLEANUP_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CLEANUP_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_CLEAN_UNLOAD_PATH].jm_full_name;
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_HDFS_CLEAN_UNLOAD_PATH].methodID, js_UldPath);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsCleanUnloadPath()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CLEANUP_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsMergeFiles( const NAString& srcPath,
                                                const NAString& dstPath)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsMergeFiles(%s, %s) called.",
                  srcPath.data(), dstPath.data());

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_EXCEPTION;

  jstring js_SrcPath = jenv_->NewStringUTF(srcPath.data());

  if (js_SrcPath == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM;
  }
  jstring js_DstPath= jenv_->NewStringUTF(dstPath.data());
  if (js_DstPath == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM;
  }


  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_MERGE_FILES].jm_full_name;
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_HDFS_MERGE_FILES].methodID, js_SrcPath, js_DstPath);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsMergeFiles()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsMergeFiles()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_EXCEPTION;
  } 

  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsDeletePath( const NAString& delPath)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsDeletePath(%s called.",
                  delPath.data());
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_EXCEPTION;

  jstring js_delPath = jenv_->NewStringUTF(delPath.data());
  if (js_delPath == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_PARAM;
  }


  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_DELETE_PATH].jm_full_name;
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_HDFS_DELETE_PATH].methodID, js_delPath);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsDeletePath()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsDeletePath()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_EXCEPTION;
  }

  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsListDirectory(const char *pathStr, HDFS_FileInfo **hdfsFileInfo, int *numFiles)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsListDirectory(%s) called.", pathStr);

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_LIST_DIR_PARAM;

  jstring js_pathStr = jenv_->NewStringUTF(pathStr);
  if (js_pathStr == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_LIST_DIR_PARAM;
  }
  jlong jniObj = (long)this;
  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_LIST_DIRECTORY].jm_full_name;
  
  jint retNumFiles = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_HDFS_LIST_DIRECTORY].methodID, 
          js_pathStr, jniObj);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsListDirectory()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_LIST_DIR_EXCEPTION;
  } 
  *numFiles = retNumFiles;
  *hdfsFileInfo = hdfsFileInfo_;
  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsExists( const NAString& uldPath, NABoolean & exist)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsExists(%s) called.",
                                                      uldPath.data());

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_EXISTS_EXCEPTION;

  jstring js_UldPath = jenv_->NewStringUTF(uldPath.data());
  if (js_UldPath == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_EXISTS_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_EXISTS].jm_full_name;
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_HDFS_EXISTS].methodID, js_UldPath);
  exist = jresult;
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails();
    logError(CAT_SQL_HDFS, __FILE__, __LINE__);
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsExists()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_EXISTS_EXCEPTION;
  } 
  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::setHdfsFileInfo(JNIEnv *jenv, jint numFiles, jint fileNo, jboolean isDir, 
          jstring filename, jlong modTime, jlong len, jshort numReplicas, jlong blockSize, 
          jstring owner, jstring group, jshort permissions, jlong accessTime)
{
   HDFS_FileInfo *hdfsFileInfo;

   if (fileNo == 0 && hdfsFileInfo_ != NULL)
      deleteHdfsFileInfo();

   if (hdfsFileInfo_ == NULL) {
       hdfsFileInfo_ = new (getHeap()) HDFS_FileInfo[numFiles];
       numFiles_ = numFiles;
   }

   if (fileNo >= numFiles_)
      return HDFS_CLIENT_ERROR_SET_HDFSFILEINFO;
   hdfsFileInfo = &hdfsFileInfo_[fileNo];
   if (isDir)
      hdfsFileInfo->mKind = HDFS_DIRECTORY_KIND;
   else
      hdfsFileInfo->mKind = HDFS_FILE_KIND;
   hdfsFileInfo->mLastMod = modTime;
   hdfsFileInfo->mSize = len;  
   hdfsFileInfo->mReplication = numReplicas;
   hdfsFileInfo->mBlockSize = blockSize;
   hdfsFileInfo->mPermissions = permissions;
   hdfsFileInfo->mLastAccess = accessTime;
   jint tempLen = jenv->GetStringUTFLength(filename);
   hdfsFileInfo->mName = new (getHeap()) char[tempLen+1];   
   strncpy(hdfsFileInfo->mName, jenv->GetStringUTFChars(filename, NULL), tempLen);
   hdfsFileInfo->mName[tempLen] = '\0';
   tempLen = jenv->GetStringUTFLength(owner);
   hdfsFileInfo->mOwner = new (getHeap()) char[tempLen+1];   
   strncpy(hdfsFileInfo->mOwner, jenv->GetStringUTFChars(owner, NULL), tempLen);
   hdfsFileInfo->mOwner[tempLen] = '\0';
   tempLen = jenv->GetStringUTFLength(group);
   hdfsFileInfo->mGroup = new (getHeap()) char[tempLen+1];   
   strncpy(hdfsFileInfo->mGroup, jenv->GetStringUTFChars(group, NULL), tempLen);
   hdfsFileInfo->mGroup[tempLen] = '\0';
   return HDFS_CLIENT_OK;
}


#ifdef __cplusplus
extern "C" {
#endif

jint JNICALL Java_org_trafodion_sql_HDFSClient_sendFileStatus
  (JNIEnv *jenv, jobject j_obj, jlong hdfsClientJniObj, jint numFiles, jint fileNo, jboolean isDir, 
          jstring filename, jlong modTime, jlong len, jshort numReplicas, jlong blockSize, 
          jstring owner, jstring group, jshort permissions, jlong accessTime)
{
   HDFS_Client_RetCode retcode;
   HdfsClient *hdfsClient = (HdfsClient *)hdfsClientJniObj;
   retcode =  hdfsClient->setHdfsFileInfo(jenv, numFiles, fileNo, isDir, filename, modTime, len, numReplicas, blockSize, owner,
            group, permissions, accessTime);
   return (jint) retcode;
}

#ifdef __cplusplus
}
#endif
