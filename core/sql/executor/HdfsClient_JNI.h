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
  ,HDFS_SCAN_LAST
} HDFS_Scan_RetCode;

class HdfsScan : public JavaObjectInterface
{
public:
  // Default constructor - for creating a new JVM		
  HdfsScan(NAHeap *heap)
  :  JavaObjectInterface(heap) 
  {}

  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HDFS_Scan_RetCode init();

  // Get the error description.
  static char* getErrorText(HDFS_Scan_RetCode errEnum);

  static HdfsScan *newInstance(NAHeap *heap, ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf, int scanBufSize, 
            HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, int rangeTailIOSize,
            ExHdfsScanStats *hdfsStats, HDFS_Scan_RetCode &hdfsScanRetCode);

  HDFS_Scan_RetCode setScanRanges(NAHeap *heap, ExHdfsScanTcb::HDFS_SCAN_BUF *hdfsScanBuf, int scanBufSize, 
            HdfsFileInfoArray *hdfsFileInfoArray, Int32 beginRangeNum, Int32 numRanges, 
            int rangeTailIOSize, ExHdfsScanStats *hdfsStats);

  HDFS_Scan_RetCode trafHdfsRead(NAHeap *heap, ExHdfsScanStats *hdfsStats, int retArray[], short arrayLen);

private:
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_SET_SCAN_RANGES,
    JM_TRAF_HDFS_READ,
    JM_LAST
  };
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
  HDFS_CLIENT_OK     = JOI_OK
 ,HDFS_CLIENT_FIRST  = HDFS_SCAN_LAST
 ,HDFS_CLIENT_ERROR_HDFS_CREATE_PARAM = HDFS_CLIENT_FIRST
 ,HDFS_CLIENT_ERROR_HDFS_CREATE_EXCEPTION
 ,HDFS_CLIENT_ERROR_HDFS_WRITE_PARAM
 ,HDFS_CLIENT_ERROR_HDFS_WRITE_EXCEPTION
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
 ,HDFS_CLIENT_LAST
} HDFS_Client_RetCode;

class HdfsClient : public JavaObjectInterface
{
public:
  // Default constructor - for creating a new JVM		
  HdfsClient(NAHeap *heap)
  :  JavaObjectInterface(heap) 
  {}

  static HdfsClient *newInstance(NAHeap *heap, HDFS_Client_RetCode &retCode);

  // Get the error description.
  static char* getErrorText(HDFS_Client_RetCode errEnum);
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HDFS_Client_RetCode    init();
  HDFS_Client_RetCode    hdfsCreate(const char* path, NABoolean compress);
  HDFS_Client_RetCode    hdfsWrite(const char* data, Int64 size);
  HDFS_Client_RetCode    hdfsClose();
  HDFS_Client_RetCode    hdfsMergeFiles(const NAString& srcPath,
                                 const NAString& dstPath);
  HDFS_Client_RetCode    hdfsCleanUnloadPath(const NAString& uldPath );
  HDFS_Client_RetCode    hdfsExists(const NAString& uldPath,  NABoolean & exists );
  HDFS_Client_RetCode    hdfsDeletePath(const NAString& delPath);

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_HDFS_CREATE,
    JM_HDFS_WRITE,
    JM_HDFS_CLOSE,
    JM_HDFS_MERGE_FILES,
    JM_HDFS_CLEAN_UNLOAD_PATH,
    JM_HDFS_EXISTS,
    JM_HDFS_DELETE_PATH,
    JM_LAST
  };
  
  static jclass javaClass_;
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

#endif
