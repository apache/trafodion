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
#ifndef HDFS_CLIENT_H
#define HDFS_CLIENT_H

#include "JavaObjectInterface.h"
#include "ExHdfsScan.h"

// ===========================================================================
// ===== The native HdfsScan class implements access to the Java methods 
// ===== org.trafodion.sql.HdfsScan class.
// ===========================================================================

typedef enum {
   HDFS_SCAN_OK     = JOI_OK
  ,HDFS_SCAN_FIRST = JOI_LAST
  ,HDFS_SCAN_ERROR_SET_SCAN_RANGES_PARAM = HDFS_SCAN_FIRST
  ,HDFS_SCAN_ERROR_SET_SCAN_RANGES_EXCEPTION
  ,HDFS_SCAN_ERROR_TRAF_HDFS_READ_PARAM
  ,HDFS_SCAN_ERROR_TRAF_HDFS_READ_EXCEPTION
  ,HDFS_SCAN_EOR
  ,HDFS_SCAN_ERROR_STOP_PARAM
  ,HDFS_SCAN_ERROR_STOP_EXCEPTION
  ,HDFS_SCAN_LAST
} HDFS_Scan_RetCode;

class HdfsScan : public JavaObjectInterface
{
public:
  HdfsScan(NAHeap *heap)
  :  JavaObjectInterface(heap) 
  , hdfsStats_(NULL)
  , j_buf1_(NULL)
  , j_buf2_(NULL)
  {}

  ~HdfsScan();

  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HDFS_Scan_RetCode init();
  void setHdfsStats(ExHdfsScanStats *hdfsStats)
  { hdfsStats_ = hdfsStats; } 

  // Get the error description.
  static char* getErrorText(HDFS_Scan_RetCode errEnum);

  static HdfsScan *newInstance(NAHeap *heap, ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf, int scanBufSize, int hdfsIoByteArraySizeInKB, 
            HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, int rangeTailIOSize, NABoolean sequenceFile,
            char recDelimiter, ExHdfsScanStats *hdfsStats, HDFS_Scan_RetCode &hdfsScanRetCode);

  HDFS_Scan_RetCode setScanRanges(ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf, int scanBufSize, int hdfsIoByteArraySizeInKB, 
            HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, 
            int rangeTailIOSize, NABoolean sequenceFile, char recDelimiter);

  HDFS_Scan_RetCode trafHdfsRead(int retArray[], short arrayLen);

  HDFS_Scan_RetCode stop();

private:
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_SET_SCAN_RANGES,
    JM_TRAF_HDFS_READ,
    JM_STOP,
    JM_LAST
  };
  jobject j_buf1_;
  jobject j_buf2_;
  ExHdfsScanStats *hdfsStats_;
  static jclass javaClass_;
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

// ===========================================================================
// ===== The native HdfsClient class implements access to the Java 
// ===== org.trafodion.sql.HdfsClient class.
// ===========================================================================

typedef enum {
     HDFS_FILE_KIND
   , HDFS_DIRECTORY_KIND 
} HDFS_FileType;

typedef struct {
   HDFS_FileType mKind; /* file or directory */
   char *mName;         /* the name of the file */
   Int64 mLastMod;      /* the last modification time for the file in seconds */
   Int64 mSize;       /* the size of the file in bytes */
   short mReplication;    /* the count of replicas */
   Int64 mBlockSize;  /* the block size for the file */
   char *mOwner;        /* the owner of the file */
   char *mGroup;        /* the group associated with the file */
   short mPermissions;  /* the permissions associated with the file */
   Int64 mLastAccess;    /* the last access time for the file in seconds */
} HDFS_FileInfo;


typedef enum {
  HDFS_CLIENT_OK     = JOI_OK
 ,HDFS_CLIENT_FIRST  = HDFS_SCAN_LAST
 ,HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM = HDFS_CLIENT_FIRST
 ,HDFS_CLIENT_ERROR_HDFS_CREATE_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_OPEN_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_OPEN_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_WRITE_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_WRITE_IMMEDIATE_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_WRITE_IMMEDIATE_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_READ_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_READ_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_CLOSE_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_MERGE_FILES_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_CLEANUP_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_CLEANUP_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_EXISTS_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_EXISTS_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_EXISTS_FILE_EXISTS
 ,HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_DELETE_PATH_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_DELETE_FILES_EXCEPTION
 ,HDFS_CLIENT_ERROR_SET_HDFSFILEINFO
 ,HDFS_CLIENT_ERROR_HDFS_LIST_DIR_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_LIST_DIR_EXCEPTION
 ,HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_PARAM
 ,HDFS_CLIENT_ERROR_HIVE_TBL_MAX_MODIFICATION_TS_EXCEPTION
 ,HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_PARAM
 ,HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_EXCEPTION
 ,HDFS_CLIENT_ERROR_GET_FS_DEFAULT_NAME_BUFFER_TOO_SMALL
 ,HDFS_CLIENT_ERROR_CREATE_DIRECTORY_PARAM
 ,HDFS_CLIENT_ERROR_CREATE_DIRECTORY_EXCEPTION
 ,HDFS_CLIENT_ERROR_RENAME_PARAM
 ,HDFS_CLIENT_ERROR_RENAME_EXCEPTION
 ,HDFS_CLIENT_ERROR_SIZE_PARAM
 ,HDFS_CLIENT_ERROR_SIZE_EXCEPTION
 ,HDFS_CLIENT_LAST
} HDFS_Client_RetCode;

class HdfsClient : public JavaObjectInterface
{
public:
  HdfsClient(NAHeap *heap)
  :  JavaObjectInterface(heap) 
    , path_(NULL)
    , hdfsFileInfo_(NULL) 
    , numFiles_(0)
    , totalBytesWritten_(0)
    , hdfsStats_(NULL)
  {
  }
 
  ~HdfsClient();
  static HdfsClient *newInstance(NAHeap *heap, ExHdfsScanStats *hdfsStats, HDFS_Client_RetCode &retCode, int hdfsIoByteArraySizeInKB = 0);
  static HdfsClient *getInstance();
  static void deleteInstance(HdfsClient *hdfsClient);
  void setIoByteArraySize(int size)
      { ioByteArraySizeInKB_ = size; }

  // Get the error description.
  static char* getErrorText(HDFS_Client_RetCode errEnum);
  void setHdfsStats(ExHdfsScanStats *hdfsStats)
  { hdfsStats_ = hdfsStats; } 
  HDFS_Client_RetCode    init();
  HDFS_Client_RetCode    hdfsCreate(const char* path, NABoolean overwrite, NABoolean append, NABoolean compress);
  HDFS_Client_RetCode    hdfsOpen(const char* path, NABoolean compress);
  Int64                  hdfsSize(HDFS_Client_RetCode &hdfsClientRetcode);
  Int32                  hdfsWrite(const char* data, Int64 size, HDFS_Client_RetCode &hdfsClientRetcode, int maxChunkSize = 0);
  Int64                  hdfsWriteImmediate(const char* data, Int64 size, HDFS_Client_RetCode &hdfsClientRetcode, int maxChunkSize = 0, NABoolean doRetry = FALSE);
  Int32                  hdfsRead(Int64 offset, const char* data, Int64 size, HDFS_Client_RetCode &hdfsClientRetcode);
  HDFS_Client_RetCode    hdfsClose();
  HDFS_Client_RetCode    setHdfsFileInfo(JNIEnv *jenv, jint numFiles, jint fileNo, jboolean isDir, 
          jstring filename, jlong modTime, jlong len, jshort numReplicas, jlong blockSize, 
          jstring owner, jstring group, jshort permissions, jlong accessTime);
  HDFS_Client_RetCode    hdfsListDirectory(const char *pathStr, HDFS_FileInfo **hdfsFileInfo, int *numFiles);
  static Int64                  hdfsSize(const char *filename, HDFS_Client_RetCode &hdfsClientRetcode);
  static HDFS_Client_RetCode    hdfsMergeFiles(const NAString& srcPath, const NAString& dstPath);
  static HDFS_Client_RetCode    hdfsCleanUnloadPath(const NAString& uldPath );
  static HDFS_Client_RetCode    hdfsExists(const NAString& uldPath,  NABoolean & exists );
  static HDFS_Client_RetCode    hdfsDeletePath(const NAString& delPath);
  static HDFS_Client_RetCode    hdfsDeleteFiles(const NAString& dirPath, const char *startingFileName);
  static HDFS_Client_RetCode    getHiveTableMaxModificationTs(Int64& maxModificationTs, const char * tableDirPaths,  int levelDeep);
   // Get the hdfs URL.
  // buffer is the buffer pre-allocated to hold the result
  // buf_len is the length of the buffer in bytes
  static HDFS_Client_RetCode    getFsDefaultName(char* buffer, Int32 buf_len);
  static HDFS_Client_RetCode    hdfsCreateDirectory(const NAString& path);
  static HDFS_Client_RetCode    hdfsRename(const NAString& fromPath, const NAString& toPath);

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_HDFS_CREATE,
    JM_HDFS_OPEN,
    JM_HDFS_WRITE,
    JM_HDFS_WRITE_IMMEDIATE,
    JM_HDFS_READ,
    JM_HDFS_CLOSE,
    JM_HDFS_MERGE_FILES,
    JM_HDFS_CLEAN_UNLOAD_PATH,
    JM_HDFS_EXISTS,
    JM_HDFS_DELETE_PATH,
    JM_HDFS_DELETE_FILES,
    JM_HDFS_LIST_DIRECTORY,
    JM_HIVE_TBL_MAX_MODIFICATION_TS,
    JM_GET_FS_DEFAULT_NAME,
    JM_HDFS_CREATE_DIRECTORY,
    JM_HDFS_RENAME,
    JM_HDFS_SIZE,
    JM_HDFS_SIZE_FOR_FILE,
    JM_LAST
  };

  void deleteHdfsFileInfo();
  void setPath(const char *path);

  HDFS_FileInfo *hdfsFileInfo_; 
  int numFiles_;
  char *path_;
  Int64 totalBytesWritten_;
  Int32 ioByteArraySizeInKB_;
  ExHdfsScanStats *hdfsStats_;
  static jclass javaClass_;
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};


#endif
