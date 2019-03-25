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
#include "Context.h"
#include "jni.h"
#include "HdfsClient_JNI.h"
#include "org_trafodion_sql_HDFSClient.h"
#include "ComCompressionInfo.h"

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
  ,"Java exception in HdfsScan::setScanRanges"
  ,"Error in HdfsScan::trafHdfsRead"
  ,"Java exception in HdfsScan::trafHdfsRead"
  , "Hdfs scan End of Ranges"
  ,"Error in HdfsScan::stop"
  ,"Java exception in HdfsScan::stop"
};

 
//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HdfsScan::~HdfsScan()
{
   if (j_buf1_ != NULL) {
      jenv_->DeleteGlobalRef(j_buf1_);
      j_buf1_ = NULL;
   }
   if (j_buf2_ != NULL) {
      jenv_->DeleteGlobalRef(j_buf2_);
      j_buf2_ = NULL;
   }
}

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
    JavaMethods_[JM_SET_SCAN_RANGES].jm_signature = "(Ljava/nio/ByteBuffer;Ljava/nio/ByteBuffer;I[Ljava/lang/String;[J[J[I[SZB)V";
    JavaMethods_[JM_TRAF_HDFS_READ].jm_name      = "trafHdfsRead";
    JavaMethods_[JM_TRAF_HDFS_READ].jm_signature = "()[I";
    JavaMethods_[JM_STOP].jm_name      = "stop";
    JavaMethods_[JM_STOP].jm_signature = "()V";
   
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
HDFS_Scan_RetCode HdfsScan::setScanRanges(ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf,  int scanBufSize, int hdfsIoByteArraySizeInKB,
      HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, int rangeTailIOSize, NABoolean sequenceFile, char recDelimiter)
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
   j_buf1_ = jenv_->NewGlobalRef(j_buf1);
   if (j_buf1_ == NULL) {
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
   j_buf2_ = jenv_->NewGlobalRef(j_buf2);
   if (j_buf2_ == NULL) {
      GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM));
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM;
   }
   jint j_hdfsIoByteArraySizeInKB = hdfsIoByteArraySizeInKB;
   jobjectArray j_filenames = NULL;
   jlongArray j_offsets = NULL;
   jlongArray j_lens = NULL;  
   jintArray j_rangenums = NULL;
   jshortArray j_compress = NULL;
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
       long len;
       if (hdfo->getBytesToRead() > (LONG_MAX-rangeTailIOSize))
           len  = LONG_MAX;
       else
           len  = hdfo->getBytesToRead()+rangeTailIOSize;
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

       if (j_compress == NULL) {
          j_compress = jenv_->NewShortArray(numRanges);
          if (jenv_->ExceptionCheck()) {
             jenv_->PopLocalFrame(NULL);
             return hdfsScanRetCode;
          }
       }
       short compressionMethod = (short)hdfo->getCompressionMethod();
       ex_assert(compressionMethod >= 0 && compressionMethod < ComCompressionInfo::SUPPORTED_COMPRESSIONS, "Illegal CompressionMethod Value");
       jenv_->SetShortArrayRegion(j_compress, rangeCount, 1, &compressionMethod);
   } 
   jboolean j_sequenceFile = sequenceFile;
   jbyte j_recDelimiter = (BYTE)recDelimiter;
   if (hdfsStats_ != NULL)
       hdfsStats_->getHdfsTimer().start();
   tsRecentJMFromJNI = JavaMethods_[JM_SET_SCAN_RANGES].jm_full_name;
   jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_SET_SCAN_RANGES].methodID, j_buf1, j_buf2, j_hdfsIoByteArraySizeInKB, 
                      j_filenames, j_offsets, j_lens, j_rangenums, j_compress, j_sequenceFile, j_recDelimiter);
   if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
   }

   if (jenv_->ExceptionCheck()) {
      getExceptionDetails(__FILE__, __LINE__, "HdfsScan::setScanRanges()");
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_SET_SCAN_RANGES_EXCEPTION;
   }
   return HDFS_SCAN_OK; 
}

HdfsScan *HdfsScan::newInstance(NAHeap *heap, ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf,  int scanBufSize,
      int hdfsIoByteArraySizeInKB, HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, int rangeTailIOSize, 
      NABoolean sequenceFile, char recDelimiter,  
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
          hdfsScanRetCode = hdfsScan->setScanRanges(hdfsScanBuf, scanBufSize, hdfsIoByteArraySizeInKB,  
                    hdfsFileInfoArray, beginRangeNum, numRanges, rangeTailIOSize, sequenceFile, recDelimiter); 
       if (hdfsScanRetCode == HDFS_SCAN_OK) 
          hdfsScan->setHdfsStats(hdfsStats);
       else {
          NADELETE(hdfsScan, HdfsScan, heap);
          hdfsScan = NULL;
       }
   }
   return hdfsScan;
}


HDFS_Scan_RetCode HdfsScan::trafHdfsRead(int retArray[], short arrayLen)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsScan::trafHdfsRead() called.");

   if (initJNIEnv() != JOI_OK)
     return HDFS_SCAN_ERROR_TRAF_HDFS_READ_PARAM;

   if (hdfsStats_ != NULL)
       hdfsStats_->getHdfsTimer().start();
   tsRecentJMFromJNI = JavaMethods_[JM_TRAF_HDFS_READ].jm_full_name;
   jintArray j_retArray = (jintArray)jenv_->CallObjectMethod(javaObj_, JavaMethods_[JM_TRAF_HDFS_READ].methodID);
   if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
   }

   if (jenv_->ExceptionCheck()) {
      getExceptionDetails(__FILE__, __LINE__, "HdfsScan::setScanRanges()");
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_TRAF_HDFS_READ_EXCEPTION;
   }
   if (j_retArray == NULL)
      return HDFS_SCAN_EOR;

   short retArrayLen = jenv_->GetArrayLength(j_retArray);
   ex_assert(retArrayLen == arrayLen, "HdfsScan::trafHdfsRead() InternalError: retArrayLen != arrayLen");
   jenv_->GetIntArrayRegion(j_retArray, 0, 4, retArray);
   if (hdfsStats_ != NULL)
      hdfsStats_->incBytesRead(retArray[ExHdfsScanTcb::BYTES_COMPLETED]);
   return HDFS_SCAN_OK;
}

HDFS_Scan_RetCode HdfsScan::stop()
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsScan::stop() called.");

   if (initJNIEnv() != JOI_OK)
     return HDFS_SCAN_ERROR_STOP_PARAM;

   if (hdfsStats_ != NULL)
       hdfsStats_->getHdfsTimer().start();
   tsRecentJMFromJNI = JavaMethods_[JM_STOP].jm_full_name;
   jenv_->CallVoidMethod(javaObj_, JavaMethods_[JM_STOP].methodID);
   if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
   }

   if (jenv_->ExceptionCheck()) {
      getExceptionDetails(__FILE__, __LINE__, "HdfsScan::stop()");
      jenv_->PopLocalFrame(NULL);
      return HDFS_SCAN_ERROR_STOP_EXCEPTION;
   }
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
  "JNI NewStringUTF() in HdfsClient::hdfsCreate()."
 ,"Java exception in HdfsClient::hdfsCreate()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsOpen()."
 ,"Java exception in HdfsClient::hdfsOpen()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsWrite()."
 ,"Java exception in HdfsClient::hdfsWrite()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsWriteImmediate()."
 ,"Java exception in HdfsClient::hdfsWriteImmediate()."
 ,"Error in HdfsClient::hdfsRead()."
 ,"Java exception in HdfsClient::hdfsRead()."
 ,"Java exception in HdfsClient::hdfsClose()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsMergeFiles()."
 ,"Java exception in HdfsClient::hdfsMergeFiles()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsCleanUnloadPath()."
 ,"Java exception in HdfsClient::hdfsCleanUnloadPath()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsExists()."
 ,"Java exception in HdfsClient::hdfsExists()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsDeletePath()."
 ,"Java exception in HdfsClient::hdfsDeletePath()."
 ,"JNI NewStringUTF() in HdfsClient::hdfsDeleteFiles()."
 ,"Java exception in HdfsClient::hdfsDeleteFiles()."
 ,"Error in HdfsClient::setHdfsFileInfo()."
 ,"Error in HdfsClient::hdfsListDirectory()."
 ,"Java exception in HdfsClient::hdfsListDirectory()."
 ,"preparing parameters for HdfsClient::getHiveTableMaxModificationTs()."
 ,"java exception in HdfsClient::getHiveTableMaxModificationTs()."
 ,"Error in HdfsClient::getFsDefaultName()."
 ,"Java exception in HdfsClient::getFsDefaultName()."
 ,"Buffer is small in HdfsClient::getFsDefaultName()."
 ,"Error in HdfsClient::hdfsCreateDirectory()."
 ,"Java exception in HdfsClient::hdfsCreateDirectory()."
 ,"Error in HdfsClient::hdfsRename()."
 ,"Java exception in HdfsClient::hdfsRename()."
 ,"Error in HdfsClient::hdfsSize()."
 ,"Java exception in HdfsClient::hdfsSize()."
};

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
HdfsClient::~HdfsClient()
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::~HdfsClient() called.");
   deleteHdfsFileInfo();
   if (path_ != NULL) 
      NADELETEBASIC(path_, getHeap());
   path_ = NULL;
}

void HdfsClient::deleteHdfsFileInfo()
{
   for (int i = 0; i < numFiles_ ; i ++) {
      NADELETEBASIC(hdfsFileInfo_[i].mName, getHeap());
      NADELETEBASIC(hdfsFileInfo_[i].mOwner, getHeap());
      NADELETEBASIC(hdfsFileInfo_[i].mGroup, getHeap());
   }
   if (hdfsFileInfo_ != NULL)
      NADELETEBASICARRAY(hdfsFileInfo_, getHeap()); 
   numFiles_ = 0;
   hdfsFileInfo_ = NULL;
}

HdfsClient *HdfsClient::newInstance(NAHeap *heap, ExHdfsScanStats *hdfsStats, HDFS_Client_RetCode &retCode, int hdfsIoByteArraySizeInKB)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::newInstance() called.");

   if (initJNIEnv() != JOI_OK)
     return NULL;
   retCode = HDFS_CLIENT_OK;
   HdfsClient *hdfsClient = new (heap) HdfsClient(heap);
   if (hdfsClient != NULL) {
       retCode = hdfsClient->init();
       if (retCode == HDFS_CLIENT_OK) {
          hdfsClient->setHdfsStats(hdfsStats);
          hdfsClient->setIoByteArraySize(hdfsIoByteArraySizeInKB);
       }
       else {
          NADELETE(hdfsClient, HdfsClient, heap);
          hdfsClient = NULL;
       }
   }
   return hdfsClient;
}

HdfsClient* HdfsClient::getInstance()
{
   ContextCli *currContext = GetCliGlobals()->currContext();
   HdfsClient *hdfsClient = currContext->getHDFSClient();
   HDFS_Client_RetCode retcode;
   if (hdfsClient == NULL) {
      NAHeap *heap = currContext->exHeap();
      hdfsClient = newInstance(heap, NULL, retcode);
      if (retcode != HDFS_CLIENT_OK)
         return NULL; 
      currContext->setHDFSClient(hdfsClient);
   }
   return hdfsClient;
}

void HdfsClient::deleteInstance(HdfsClient *hdfsClient)
{
  hdfsClient->hdfsClose();
  NADELETE(hdfsClient, HdfsClient, hdfsClient->getHeap());
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
    JavaMethods_[JM_HDFS_CREATE     ].jm_signature = "(Ljava/lang/String;ZZZ)Z";
    JavaMethods_[JM_HDFS_OPEN       ].jm_name      = "hdfsOpen";
    JavaMethods_[JM_HDFS_OPEN       ].jm_signature = "(Ljava/lang/String;Z)Z";
    JavaMethods_[JM_HDFS_WRITE      ].jm_name      = "hdfsWrite";
    JavaMethods_[JM_HDFS_WRITE      ].jm_signature = "([B)I";
    JavaMethods_[JM_HDFS_WRITE_IMMEDIATE].jm_name      = "hdfsWriteImmediate";
    JavaMethods_[JM_HDFS_WRITE_IMMEDIATE].jm_signature = "([BZ)J";
    JavaMethods_[JM_HDFS_READ       ].jm_name      = "hdfsRead";
    JavaMethods_[JM_HDFS_READ       ].jm_signature = "(JLjava/nio/ByteBuffer;)I";
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
    JavaMethods_[JM_HDFS_DELETE_FILES].jm_name      = "hdfsDeleteFiles";
    JavaMethods_[JM_HDFS_DELETE_FILES].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_LIST_DIRECTORY].jm_name      = "hdfsListDirectory";
    JavaMethods_[JM_HDFS_LIST_DIRECTORY].jm_signature = "(Ljava/lang/String;J)I";
    JavaMethods_[JM_HIVE_TBL_MAX_MODIFICATION_TS].jm_name      = "getHiveTableMaxModificationTs";
    JavaMethods_[JM_HIVE_TBL_MAX_MODIFICATION_TS ].jm_signature = "(Ljava/lang/String;I)J";
    JavaMethods_[JM_GET_FS_DEFAULT_NAME].jm_name      = "getFsDefaultName";
    JavaMethods_[JM_GET_FS_DEFAULT_NAME].jm_signature = "()Ljava/lang/String;";
    JavaMethods_[JM_HDFS_CREATE_DIRECTORY].jm_name      = "hdfsCreateDirectory";
    JavaMethods_[JM_HDFS_CREATE_DIRECTORY].jm_signature = "(Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_RENAME].jm_name      = "hdfsRename";
    JavaMethods_[JM_HDFS_RENAME].jm_signature = "(Ljava/lang/String;Ljava/lang/String;)Z";
    JavaMethods_[JM_HDFS_SIZE].jm_name      = "hdfsSize";
    JavaMethods_[JM_HDFS_SIZE].jm_signature = "()J";
    JavaMethods_[JM_HDFS_SIZE_FOR_FILE].jm_name      = "hdfsSize";
    JavaMethods_[JM_HDFS_SIZE_FOR_FILE].jm_signature = "(Ljava/lang/String;)J";
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

void HdfsClient::setPath(const char *path)
{
   if (path_ != NULL) 
      NADELETEBASIC(path_, getHeap());
   size_t len = strlen(path);
   path_ = new (getHeap()) char[len+1];
   strcpy(path_, path); 
}

HDFS_Client_RetCode HdfsClient::hdfsCreate(const char* path, NABoolean overwrite, NABoolean append, NABoolean compress)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsCreate(%s) called.", path);

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM;
  setPath(path);
  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM;
  }

  jboolean j_compress = compress;
  jboolean j_overwrite = overwrite;
  jboolean j_append = append;

  if (hdfsStats_ != NULL)
     hdfsStats_->getHdfsTimer().start();

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_CREATE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_CREATE].methodID, js_path, j_overwrite, j_append, j_compress);
  if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
  }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsCreate()");
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
  setPath(path);
  jstring js_path = jenv_->NewStringUTF(path);
  if (js_path == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM;
  }

  jboolean j_compress = compress;
  if (hdfsStats_ != NULL)
     hdfsStats_->getHdfsTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_OPEN].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_OPEN].methodID, js_path, j_compress);
  if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
  }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsOpen()");
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

Int64 HdfsClient::hdfsSize(HDFS_Client_RetCode &hdfsClientRetcode)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsSize() called.");
   
  if (initJNIEnv() != JOI_OK) {
     hdfsClientRetcode = HDFS_CLIENT_ERROR_SIZE_PARAM;
     return -1;
  }

  if (hdfsStats_ != NULL)
     hdfsStats_->getHdfsTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_SIZE].jm_full_name;
  jlong jresult = jenv_->CallLongMethod(javaObj_, JavaMethods_[JM_HDFS_SIZE].methodID);
  if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
  }

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsSize()");
    jenv_->PopLocalFrame(NULL);
    hdfsClientRetcode = HDFS_CLIENT_ERROR_SIZE_EXCEPTION;
    return -1;
  }
  hdfsClientRetcode = HDFS_CLIENT_OK;
  return jresult;
}


Int32 HdfsClient::hdfsWrite(const char* data, Int64 len, HDFS_Client_RetCode &hdfsClientRetcode, int maxChunkSize)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsWrite(%ld) called.", len);

  if (initJNIEnv() != JOI_OK) {
     hdfsClientRetcode = HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION;
     return 0;
  }
  Int64 lenRemain = len;
  Int64 writeLen;
  Int64 chunkLen = (maxChunkSize > 0 ? maxChunkSize : (ioByteArraySizeInKB_ > 0 ? ioByteArraySizeInKB_ * 1024 : 0));
  Int64 offset = 0;
  jint bytesWritten;
  do 
  {
     if ((chunkLen > 0) && (lenRemain > chunkLen))
        writeLen = chunkLen; 
     else
        writeLen = lenRemain;
     //Write the requisite bytes into the file
     jbyteArray jbArray = jenv_->NewByteArray(writeLen);
     if (!jbArray) {
        GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_WRITE_PARAM));
        jenv_->PopLocalFrame(NULL);
        hdfsClientRetcode =  HDFS_CLIENT_ERROR_HDFS_WRITE_PARAM;
        return 0;
     }
     jenv_->SetByteArrayRegion(jbArray, 0, writeLen, (const jbyte*)(data+offset));

     if (hdfsStats_ != NULL)
         hdfsStats_->getHdfsTimer().start();

     tsRecentJMFromJNI = JavaMethods_[JM_HDFS_WRITE].jm_full_name;
     // Java method returns the cumulative bytes written
     bytesWritten = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_HDFS_WRITE].methodID, jbArray);

     if (hdfsStats_ != NULL) {
         hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
         hdfsStats_->incHdfsCalls();
         hdfsStats_->incBytesRead(writeLen);
     }
     if (jenv_->ExceptionCheck())
     {
        getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsWrite()");
        jenv_->PopLocalFrame(NULL);
        hdfsClientRetcode = HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION;
        return 0;
     }
     lenRemain -= writeLen;
     offset += writeLen;
  } while (lenRemain > 0);
  jenv_->PopLocalFrame(NULL);
  hdfsClientRetcode = HDFS_CLIENT_OK;
  return bytesWritten; 
}

Int64 HdfsClient::hdfsWriteImmediate(const char* data, Int64 len, HDFS_Client_RetCode &hdfsClientRetcode, int maxChunkSize, NABoolean doRetry)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsWriteImmediate(%ld) called.", len);

  if (initJNIEnv() != JOI_OK) {
     hdfsClientRetcode = HDFS_CLIENT_ERROR_HDFS_WRITE_IMMEDIATE_EXCEPTION;
     return 0;
  }
  Int64 lenRemain = len;
  Int64 writeLen;
  Int64 chunkLen = (maxChunkSize > 0 ? maxChunkSize : (ioByteArraySizeInKB_ > 0 ? ioByteArraySizeInKB_ * 1024 : 0));
  Int64 offset = 0;
  jlong writeOffset = -1;
  jlong chunkWriteOffset;
  jboolean j_doRetry = doRetry;
  do 
  {
     if ((chunkLen > 0) && (lenRemain > chunkLen))
        writeLen = chunkLen; 
     else
        writeLen = lenRemain;
     //Write the requisite bytes into the file
     jbyteArray jbArray = jenv_->NewByteArray(writeLen);
     if (!jbArray) {
        GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_WRITE_IMMEDIATE_PARAM));
        jenv_->PopLocalFrame(NULL);
        hdfsClientRetcode =  HDFS_CLIENT_ERROR_HDFS_WRITE_IMMEDIATE_PARAM;
        return 0;
     }
     jenv_->SetByteArrayRegion(jbArray, 0, writeLen, (const jbyte*)(data+offset));

     if (hdfsStats_ != NULL)
         hdfsStats_->getHdfsTimer().start();

     tsRecentJMFromJNI = JavaMethods_[JM_HDFS_WRITE_IMMEDIATE].jm_full_name;
     // Java method returns the cumulative bytes written
     chunkWriteOffset = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_HDFS_WRITE_IMMEDIATE].methodID, jbArray,j_doRetry);
     if (writeOffset == -1)
        writeOffset = chunkWriteOffset;

     if (hdfsStats_ != NULL) {
         hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
         hdfsStats_->incHdfsCalls();
         hdfsStats_->incBytesRead(writeLen);
     }
     if (jenv_->ExceptionCheck())
     {
        getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsWrite()");
        jenv_->PopLocalFrame(NULL);
        hdfsClientRetcode = HDFS_CLIENT_ERROR_HDFS_WRITE_IMMEDIATE_EXCEPTION;
        return 0;
     }
     lenRemain -= writeLen;
     offset += writeLen;
  } while (lenRemain > 0);
  jenv_->PopLocalFrame(NULL);
  hdfsClientRetcode = HDFS_CLIENT_OK;
  return writeOffset;
}

Int32 HdfsClient::hdfsRead(Int64 pos, const char* data, Int64 len, HDFS_Client_RetCode &hdfsClientRetcode)
{
   QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsRead(%ld) called.", len);

   if (initJNIEnv() != JOI_OK) {
      hdfsClientRetcode = HDFS_CLIENT_ERROR_HDFS_READ_EXCEPTION;
      return 0;
   }
   jobject j_buf = jenv_->NewDirectByteBuffer((BYTE *)data, len);
   if (j_buf == NULL) {
      GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_READ_PARAM));
      jenv_->PopLocalFrame(NULL);
      return HDFS_CLIENT_ERROR_HDFS_READ_PARAM;
   }
  if (hdfsStats_ != NULL)
     hdfsStats_->getHdfsTimer().start();

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_READ].jm_full_name;
  jint bytesRead = 0;
  jlong j_pos = pos;
  bytesRead = jenv_->CallIntMethod(javaObj_, JavaMethods_[JM_HDFS_READ].methodID, j_pos, j_buf);

  if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
      hdfsStats_->incBytesRead(bytesRead);
  }
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsRead()");
    jenv_->PopLocalFrame(NULL);
    hdfsClientRetcode = HDFS_CLIENT_ERROR_HDFS_READ_EXCEPTION;
    return 0;
  }
  jenv_->PopLocalFrame(NULL);
  hdfsClientRetcode = HDFS_CLIENT_OK;
  return bytesRead; 
}

HDFS_Client_RetCode HdfsClient::hdfsClose()
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::close() called.");

  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_CLOSE_EXCEPTION;

  // String close();
  if (hdfsStats_ != NULL)
     hdfsStats_->getHdfsTimer().start();
  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_CLOSE].jm_full_name;
  jboolean jresult = jenv_->CallBooleanMethod(javaObj_, JavaMethods_[JM_HDFS_CLOSE].methodID);

  if (hdfsStats_ != NULL) {
      hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
      hdfsStats_->incHdfsCalls();
  }
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsClose()");
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

Int64 HdfsClient::hdfsSize(const char *filename, HDFS_Client_RetCode &hdfsClientRetcode)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsSize() called.");
   
  if (initJNIEnv() != JOI_OK) {
     hdfsClientRetcode = HDFS_CLIENT_ERROR_SIZE_PARAM;
     return -1;
  }
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_SIZE_PARAM;

  jstring j_filename = jenv_->NewStringUTF(filename);
  if (j_filename == NULL) {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_SIZE_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_SIZE_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_SIZE_FOR_FILE].jm_full_name;
  jlong jresult = jenv_->CallStaticLongMethod(javaClass_, JavaMethods_[JM_HDFS_SIZE_FOR_FILE].methodID, j_filename);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsSize()");
    jenv_->PopLocalFrame(NULL);
    hdfsClientRetcode = HDFS_CLIENT_ERROR_SIZE_EXCEPTION;
    return -1;
  }
  hdfsClientRetcode = HDFS_CLIENT_OK;
  return jresult;
}

HDFS_Client_RetCode HdfsClient::hdfsCleanUnloadPath( const NAString& uldPath)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsCleanUnloadPath(%s) called.",
                                                                             uldPath.data());
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_CLEANUP_PARAM;
  if (getInstance() == NULL)
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
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsCleanUnloadPath()");
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
     return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM;
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
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsMergeFiles()");
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
     return HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_PARAM;

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
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsDeletePath()");
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

HDFS_Client_RetCode HdfsClient::hdfsDeleteFiles(const NAString& dirPath, const char *startingFileName)
{
  QRLogger::log(CAT_SQL_HDFS, LL_DEBUG, "HdfsClient::hdfsDeleteFiles(%s, %s) called.",
                  dirPath.data(), startingFileName);
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM;

  jstring js_dirPath = jenv_->NewStringUTF(dirPath.data());
  if (js_dirPath == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM;
  }

  jstring js_startingFileName = jenv_->NewStringUTF(startingFileName);
  if (js_startingFileName == NULL) {
     GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM));
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_DELETE_FILES].jm_full_name;
  jboolean jresult = jenv_->CallStaticBooleanMethod(javaClass_, JavaMethods_[JM_HDFS_DELETE_FILES].methodID, 
                     js_dirPath, js_startingFileName);

  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsDeleteFiles()");
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_EXCEPTION;
  }

  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsDeleteFiles()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_EXCEPTION;
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
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsListDirectory()");
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
     return HDFS_CLIENT_ERROR_HDFS_EXISTS_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_HDFS_EXISTS_PARAM;

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
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsExists()");
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HDFS_EXISTS_EXCEPTION;
  } 
  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::getHiveTableMaxModificationTs( Int64& maxModificationTs, const char * tableDirPaths,  int levelDeep)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HDFSClient_JNI::getHiveTableMaxModificationTs(%s) called.",tableDirPaths);
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_PARAM; 
  jstring js_tableDirPaths = jenv_->NewStringUTF(tableDirPaths);
  if (js_tableDirPaths == NULL)
  {
    GetCliGlobals()->setJniErrorStr(getErrorText(HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_PARAM));
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_PARAM;
  }

  jint jlevelDeep = levelDeep;
  tsRecentJMFromJNI = JavaMethods_[JM_HIVE_TBL_MAX_MODIFICATION_TS].jm_full_name;
  jlong jresult = jenv_->CallStaticLongMethod(javaClass_,
                                          JavaMethods_[JM_HIVE_TBL_MAX_MODIFICATION_TS].methodID,
										  js_tableDirPaths, jlevelDeep);
  jenv_->DeleteLocalRef(js_tableDirPaths);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::getHiveTableMaxModificationTS()");
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_EXCEPTION;
  }
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG,
       "Exit HDFSClient_JNI::getHiveTableMaxModificationTs() called.");
  maxModificationTs = jresult;
  jenv_->PopLocalFrame(NULL);

  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::getFsDefaultName(char* buf, int buf_len)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HDFSClient_JNI::getFsDefaultName() called.");
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_PARAM;

  tsRecentJMFromJNI = JavaMethods_[JM_GET_FS_DEFAULT_NAME].jm_full_name;
  jstring jresult = 
        (jstring)jenv_->CallStaticObjectMethod(javaClass_,
                              JavaMethods_[JM_GET_FS_DEFAULT_NAME].methodID);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::getFsDefaultName()");
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_EXCEPTION;
  }
  const char* char_result = jenv_->GetStringUTFChars(jresult, 0);

  HDFS_Client_RetCode retcode = HDFS_CLIENT_OK;
  if ( buf_len >= strlen(char_result) ) {
     strcpy(buf, char_result);
  } else
     retcode = HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_BUFFER_TOO_SMALL;

  jenv_->ReleaseStringUTFChars(jresult, char_result);
  jenv_->PopLocalFrame(NULL);

  return retcode;
}

HDFS_Client_RetCode HdfsClient::hdfsCreateDirectory(const NAString &dirName)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HDFSClient_JNI::createDirectory() called.");
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_CREATE_DIRECTORY_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_CREATE_DIRECTORY_PARAM;

  jstring js_dirName = jenv_->NewStringUTF(dirName.data());
  if (js_dirName == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_CREATE_DIRECTORY_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_CREATE_DIRECTORY].jm_full_name;
  jstring jresult = 
        (jstring)jenv_->CallStaticObjectMethod(javaClass_,
                              JavaMethods_[JM_HDFS_CREATE_DIRECTORY].methodID, js_dirName);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsCreateDirectory()");
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_CREATE_DIRECTORY_EXCEPTION;
  }
  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsCreateDirectory()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_CREATE_DIRECTORY_EXCEPTION;
  }
  jenv_->PopLocalFrame(NULL);
  return HDFS_CLIENT_OK;
}

HDFS_Client_RetCode HdfsClient::hdfsRename(const NAString &fromPath, const NAString &toPath)
{
  QRLogger::log(CAT_SQL_HBASE, LL_DEBUG, "Enter HDFSClient_JNI::hdfsRename() called.");
  if (initJNIEnv() != JOI_OK)
     return HDFS_CLIENT_ERROR_RENAME_PARAM;
  if (getInstance() == NULL)
     return HDFS_CLIENT_ERROR_RENAME_PARAM;

  jstring js_fromPath = jenv_->NewStringUTF(fromPath.data());
  if (js_fromPath == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_RENAME_PARAM;
  }

  jstring js_toPath = jenv_->NewStringUTF(toPath.data());
  if (js_toPath == NULL) {
     jenv_->PopLocalFrame(NULL);
     return HDFS_CLIENT_ERROR_RENAME_PARAM;
  }

  tsRecentJMFromJNI = JavaMethods_[JM_HDFS_RENAME].jm_full_name;
  jstring jresult = 
        (jstring)jenv_->CallStaticObjectMethod(javaClass_,
                              JavaMethods_[JM_HDFS_RENAME].methodID, js_fromPath, js_toPath);
  if (jenv_->ExceptionCheck())
  {
    getExceptionDetails(__FILE__, __LINE__, "HdfsClient::hdfsRename()");
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_RENAME_EXCEPTION;
  }
  if (jresult == false)
  {
    logError(CAT_SQL_HDFS, "HdfsClient::hdfsRename()", getLastError());
    jenv_->PopLocalFrame(NULL);
    return HDFS_CLIENT_ERROR_RENAME_EXCEPTION;
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
   const char *temp = jenv->GetStringUTFChars(filename, NULL); 
   strncpy(hdfsFileInfo->mName, temp, tempLen);
   hdfsFileInfo->mName[tempLen] = '\0';
   jenv_->ReleaseStringUTFChars(filename, temp); 
   tempLen = jenv->GetStringUTFLength(owner);
   hdfsFileInfo->mOwner = new (getHeap()) char[tempLen+1];   
   temp = jenv->GetStringUTFChars(owner, NULL);
   strncpy(hdfsFileInfo->mOwner, temp, tempLen);
   hdfsFileInfo->mOwner[tempLen] = '\0';
   jenv_->ReleaseStringUTFChars(owner, temp); 
   tempLen = jenv->GetStringUTFLength(group);
   hdfsFileInfo->mGroup = new (getHeap()) char[tempLen+1];   
   temp = jenv->GetStringUTFChars(group, NULL); 
   strncpy(hdfsFileInfo->mGroup, temp, tempLen);
   hdfsFileInfo->mGroup[tempLen] = '\0';
   jenv_->ReleaseStringUTFChars(group, temp); 
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

JNIEXPORT jint JNICALL Java_org_trafodion_sql_HDFSClient_copyToByteBuffer
  (JNIEnv *jenv, jobject j_obj, jobject j_buf, jint offset, jbyteArray j_bufArray, jint copyLen)
{
   void *bufBacking;
   
   bufBacking =  jenv->GetDirectBufferAddress(j_buf);
   if (bufBacking == NULL)
      return -1;
   jlong capacity = jenv->GetDirectBufferCapacity(j_buf);
   jbyte *byteBufferAddr = (jbyte *)bufBacking + offset; 
   if ((offset + copyLen) > capacity)
      return -2; 
   jenv->GetByteArrayRegion(j_bufArray, 0, copyLen, byteBufferAddr);
   return 0;
}

#ifdef __cplusplus
}
#endif
