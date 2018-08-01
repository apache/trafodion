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
#ifndef HBASE_CLIENT_H
#define HBASE_CLIENT_H
#define INLINE_COLNAME_LEN 256
#define MAX_COLNAME_LEN 32767 

#include <list>
#include "Platform.h"
#include "Collections.h"
#include "NABasicObject.h"
#include "ExStats.h"
#include "JavaObjectInterface.h"
#include "Hbase_types.h"
#include "ExpHbaseDefs.h"
#include "NAMemory.h"
#include "org_trafodion_sql_HTableClient.h"

using namespace apache::hadoop::hbase::thrift;

namespace {
  typedef std::vector<Text> TextVec;
}

class ContextCli;

class HBulkLoadClient_JNI;

#define NUM_HBASE_WORKER_THREADS 4

typedef enum {
  HBC_Req_Shutdown = 0
 ,HBC_Req_Drop
} HBaseClientReqType;

class HBaseClientRequest
{
public :
  HBaseClientRequest(NAHeap *heap, HBaseClientReqType reqType); 
  ~HBaseClientRequest(); 
  bool isShutDown() { return reqType_ == HBC_Req_Shutdown; }
  void setFileName(const char *fileName); 
  NAHeap *getHeap() { return heap_; }
public :
  HBaseClientReqType reqType_;
  char *fileName_;
  NAHeap *heap_;
};

// ===========================================================================
// ===== The HTableClient class implements access to the Java 
// ===== HTableClient class.
// ===========================================================================

typedef enum {
  HTC_OK     = JOI_OK
 ,HTC_FIRST  = JOI_LAST
 ,HTC_DONE   = HTC_FIRST
 ,HTC_DONE_RESULT = 1000
 ,HTC_DONE_DATA
 ,HTC_ERROR_INIT_PARAM = HTC_FIRST+1
 ,HTC_ERROR_INIT_EXCEPTION
 ,HTC_ERROR_SETTRANS_EXCEPTION
 ,HTC_ERROR_CLEANUP_EXCEPTION
 ,HTC_ERROR_CLOSE_EXCEPTION
 ,HTC_ERROR_SCANOPEN_PARAM
 ,HTC_ERROR_SCANOPEN_EXCEPTION
 ,HTC_ERROR_FETCHROWS_EXCEPTION
 ,HTC_ERROR_SCANCLOSE_EXCEPTION
 ,HTC_ERROR_GETCLOSE_EXCEPTION
 ,HTC_ERROR_DELETEROW_PARAM
 ,HTC_ERROR_DELETEROW_EXCEPTION
 ,HTC_ERROR_CREATE_PARAM
 ,HTC_ERROR_CREATE_EXCEPTION
 ,HTC_ERROR_DROP_PARAM
 ,HTC_ERROR_DROP_EXCEPTION
 ,HTC_ERROR_EXISTS_PARAM
 ,HTC_ERROR_EXISTS_EXCEPTION
 ,HTC_ERROR_COPROC_AGGR_PARAM
 ,HTC_ERROR_COPROC_AGGR_EXCEPTION
 ,HTC_ERROR_GRANT_PARAM
 ,HTC_ERROR_GRANT_EXCEPTION
 ,HTC_ERROR_REVOKE_PARAM
 ,HTC_ERROR_REVOKE_EXCEPTION
 ,HTC_ERROR_GETHTABLENAME_EXCEPTION
 ,HTC_GET_COLNAME_EXCEPTION
 ,HTC_GET_COLVAL_EXCEPTION
 ,HTC_GET_ROWID_EXCEPTION
 ,HTC_NEXTCELL_EXCEPTION
 ,HTC_ERROR_COMPLETEASYNCOPERATION_EXCEPTION
 ,HTC_ERROR_ASYNC_OPERATION_NOT_COMPLETE
 ,HTC_ERROR_WRITETOWAL_EXCEPTION
 ,HTC_ERROR_WRITEBUFFERSIZE_EXCEPTION
 ,HTC_PREPARE_FOR_NEXTCELL_EXCEPTION
 ,HTC_LAST
} HTC_RetCode;

class HTableClient_JNI : public JavaObjectInterface
{
public:
  enum FETCH_MODE {
      UNKNOWN = 0
    , SCAN_FETCH
    , GET_ROW
    , BATCH_GET
  };

  HTableClient_JNI(NAHeap *heap, jobject jObj = NULL)
  :  JavaObjectInterface(heap, jObj)
  {
     heap_ = heap;
     tableName_ = NULL;
     jKvValLen_ = NULL;
     jKvValOffset_ = NULL;
     jKvQualLen_ = NULL;
     jKvQualOffset_ = NULL;
     jKvFamLen_ = NULL;
     jKvFamOffset_ = NULL;
     jTimestamp_ = NULL;
     jKvBuffer_ = NULL;
     jKvFamArray_ = NULL;
     jKvQualArray_ = NULL;
     jRowIDs_ = NULL;
     jKvsPerRow_ = NULL;
     currentRowNum_ = -1;
     currentRowCellNum_ = -1;
     prevRowCellNum_ = 0;
     numRowsReturned_ = 0;
     numColsInScan_ = 0;
     colNameAllocLen_ = 0;
     inlineColName_[0] = '\0';
     colName_ = NULL;
     numReqRows_ = -1;
     cleanupDone_ = FALSE;
     hbs_ = NULL;
     p_kvValLen_ = NULL;
     p_kvValOffset_ = NULL;
     p_kvFamLen_ = NULL;
     p_kvFamOffset_ = NULL;
     p_kvQualLen_ = NULL;
     p_kvQualOffset_ = NULL;
     p_timestamp_ = NULL;
     jba_kvBuffer_ = NULL;
     jba_kvFamArray_ = NULL;
     jba_kvQualArray_ = NULL;
     jba_rowID_ = NULL;
     fetchMode_ = UNKNOWN;
     p_rowID_ = NULL;
     p_kvsPerRow_ = NULL;
     numCellsReturned_ = 0;
     numCellsAllocated_ = 0;
     rowIDLen_ = 0;
  }

  // Destructor
  virtual ~HTableClient_JNI();
  
  HTC_RetCode init();
  
  HTC_RetCode startScan(Int64 transID, const Text& startRowID, const Text& stopRowID, const LIST(HbaseStr) & cols, Int64 timestamp, bool cacheBlocks, bool smallScanner, Lng32 numCacheRows,
                        NABoolean preFetch,
			const LIST(NAString) *inColNamesToFilter, 
			const LIST(NAString) *inCompareOpList,
			const LIST(NAString) *inColValuesToCompare,
            Float32 dopParallelScanner = 0.0f,
			Float32 samplePercent = -1.0f,
			NABoolean useSnapshotScan = FALSE,
			Lng32 snapTimeout = 0,
			char * snapName = NULL,
			char * tmpLoc = NULL,
			Lng32 espNum = 0,
                        Lng32 versions = 0);
  HTC_RetCode deleteRow(Int64 transID, HbaseStr &rowID, const LIST(HbaseStr) *columns, Int64 timestamp);
  HTC_RetCode setWriteBufferSize(Int64 size);
  HTC_RetCode setWriteToWAL(bool vWAL);
  HTC_RetCode coProcAggr(Int64 transID, 
			 int aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
			 const Text& startRow, 
			 const Text& stopRow, 
			 const Text &colFamily,
			 const Text &colName,
			 const NABoolean cacheBlocks,
			 const Lng32 numCacheRows,
			 Text &aggrVal); // returned value
  void setResultInfo( jintArray jKvValLen, jintArray jKvValOffset,
        jintArray jKvQualLen, jintArray jKvQualOffset,
        jintArray jKvFamLen, jintArray jKvFamOffset,
        jlongArray jTimestamp, 
        jobjectArray jKvBuffer, 
        jobjectArray jKvFamArray, jobjectArray jKvQualArray, jobjectArray jRowIDs,
        jintArray jKvsPerRow, jint numCellsReturned, jint numRowsReturned);
  void getResultInfo();
  void cleanupResultInfo();
  HTC_RetCode fetchRows();
  HTC_RetCode nextRow();
  HTC_RetCode getColName(int colNo,
              char **colName,
              short &colNameLen,
              Int64 &timestamp);
  HTC_RetCode getColVal(int colNo,
              BYTE *colVal,
              Lng32 &colValLen,
              NABoolean nullable,
              BYTE &nullVal);
  HTC_RetCode getColVal(NAHeap *heap,
              int colNo,
              BYTE **colVal,
              Lng32 &colValLen);
  HTC_RetCode getNumCellsPerRow(int &numCells);
  HTC_RetCode getRowID(HbaseStr &rowID);
  HTC_RetCode nextCell(HbaseStr &rowId,
                 HbaseStr &colFamName,
                 HbaseStr &colName,
                 HbaseStr &colVal,
                 Int64 &timestamp);
  HTC_RetCode completeAsyncOperation(int timeout, NABoolean *resultArray, short resultArrayLen);
  HTC_RetCode prepareForNextCell(int idx);

  //  HTC_RetCode codeProcAggrGetResult();

  const char *getTableName();
  std::string* getHTableName();

  // Get the error description.
  static char* getErrorText(HTC_RetCode errEnum);

  void setTableName(const char *tableName)
  {
    Int32 len = strlen(tableName);

    if (tableName_ != NULL)
    {
        NADELETEBASIC(tableName_, heap_);
        tableName_ = NULL;
    }
    tableName_ = new (heap_) char[len+1];
    strcpy(tableName_, tableName);
  } 

  void setHbaseStats(ExHbaseAccessStats *hbs)
  {
    hbs_ = hbs;
  }
  void setNumColsInScan(int numColsInScan) {
     numColsInScan_ = numColsInScan;
  }
  void setNumReqRows(int numReqRows) {
     numReqRows_ = numReqRows;
  }
  void setFetchMode(HTableClient_JNI::FETCH_MODE  fetchMode) {
     fetchMode_ = fetchMode;
  }
  void setNumRowsReturned(int numRowsReturned) {
     numRowsReturned_ = numRowsReturned; 
  }
  HTableClient_JNI::FETCH_MODE getFetchMode() {
     return fetchMode_;
  }

private:
  enum JAVA_METHODS {
    JM_SCAN_OPEN 
   ,JM_DELETE    
   ,JM_COPROC_AGGR
   ,JM_GET_NAME
   ,JM_GET_HTNAME
   ,JM_SET_WB_SIZE
   ,JM_SET_WRITE_TO_WAL
   ,JM_FETCH_ROWS
   ,JM_COMPLETE_PUT
   ,JM_LAST
  };
  char *tableName_; 
  jintArray jKvValLen_;
  jintArray jKvValOffset_;
  jintArray jKvQualLen_;
  jintArray jKvQualOffset_;
  jintArray jKvFamLen_;
  jintArray jKvFamOffset_;
  jlongArray jTimestamp_;
  jobjectArray jKvBuffer_;
  jobjectArray jKvFamArray_;
  jobjectArray jKvQualArray_;
  jobjectArray jRowIDs_;
  jintArray jKvsPerRow_;
  jint *p_kvValLen_;
  jint *p_kvValOffset_;
  jint *p_kvQualLen_;
  jint *p_kvQualOffset_;
  jint *p_kvFamLen_;
  jint *p_kvFamOffset_;
  jlong *p_timestamp_;
  jbyteArray jba_kvBuffer_;
  jbyteArray jba_kvFamArray_;
  jbyteArray jba_kvQualArray_;
  jbyteArray jba_rowID_;
  jbyte *p_rowID_;
  jint *p_kvsPerRow_;
  jint numRowsReturned_;
  int currentRowNum_;
  int currentRowCellNum_;
  int numColsInScan_;
  int numReqRows_;
  int numCellsReturned_;
  int numCellsAllocated_;
  int prevRowCellNum_;
  int rowIDLen_;
  char *colName_;
  char inlineColName_[INLINE_COLNAME_LEN+1];
  short colNameAllocLen_;
  FETCH_MODE fetchMode_; 
  NABoolean cleanupDone_;
  ExHbaseAccessStats *hbs_;
  static jclass          javaClass_;  
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

// ===========================================================================
// ===== The HBaseClient_JNI class implements access to the Java 
// ===== HBaseClient_JNI class.
// ===========================================================================

// Keep in sync with hbcErrorEnumStr array.
typedef enum {
  HBC_OK     = JOI_OK
 ,HBC_FIRST  = HTC_LAST
 ,HBC_DONE   = HBC_FIRST
 ,HBC_ERROR_INIT_PARAM
 ,HBC_ERROR_INIT_EXCEPTION
 ,HBC_ERROR_GET_HTC_EXCEPTION
 ,HBC_ERROR_REL_HTC_EXCEPTION
 ,HBC_ERROR_CREATE_PARAM
 ,HBC_ERROR_CREATE_EXCEPTION
 ,HBC_ERROR_ALTER_PARAM
 ,HBC_ERROR_ALTER_EXCEPTION
 ,HBC_ERROR_DROP_PARAM
 ,HBC_ERROR_DROP_EXCEPTION
 ,HBC_ERROR_LIST_PARAM
 ,HBC_ERROR_LIST_EXCEPTION
 ,HBC_ERROR_EXISTS_PARAM
 ,HBC_ERROR_EXISTS_EXCEPTION
 ,HBC_ERROR_GRANT_PARAM
 ,HBC_ERROR_GRANT_EXCEPTION
 ,HBC_ERROR_REVOKE_PARAM
 ,HBC_ERROR_REVOKE_EXCEPTION
 ,HBC_ERROR_THREAD_CREATE
 ,HBC_ERROR_THREAD_REQ_ALLOC
 ,HBC_ERROR_THREAD_SIGMASK
 ,HBC_ERROR_ATTACH_JVM
 ,HBC_ERROR_GET_HBLC_EXCEPTION
 ,HBC_ERROR_ROWCOUNT_EST_PARAM
 ,HBC_ERROR_ROWCOUNT_EST_EXCEPTION
 ,HBC_ERROR_ROWCOUNT_EST_FALSE
 ,HBC_ERROR_REL_HBLC_EXCEPTION
 ,HBC_ERROR_GET_CACHE_FRAC_EXCEPTION
 ,HBC_ERROR_GET_LATEST_SNP_PARAM
 ,HBC_ERROR_GET_LATEST_SNP_EXCEPTION
 ,HBC_ERROR_CLEAN_SNP_TMP_LOC_PARAM
 ,HBC_ERROR_CLEAN_SNP_TMP_LOC_EXCEPTION
 ,HBC_ERROR_SET_ARC_PERMS_PARAM
 ,HBC_ERROR_SET_ARC_PERMS_EXCEPTION
 ,HBC_ERROR_STARTGET_EXCEPTION
 ,HBC_ERROR_STARTGETS_EXCEPTION
 ,HBC_ERROR_GET_HBTI_PARAM
 ,HBC_ERROR_GET_HBTI_EXCEPTION
 ,HBC_ERROR_CREATE_COUNTER_PARAM
 ,HBC_ERROR_CREATE_COUNTER_EXCEPTION
 ,HBC_ERROR_INCR_COUNTER_PARAM
 ,HBC_ERROR_INCR_COUNTER_EXCEPTION
 ,HBC_ERROR_INSERTROW_PARAM
 ,HBC_ERROR_INSERTROW_EXCEPTION
 ,HBC_ERROR_INSERTROW_DUP_ROWID
 ,HBC_ERROR_INSERTROWS_PARAM
 ,HBC_ERROR_INSERTROWS_EXCEPTION
 ,HBC_ERROR_CHECKANDUPDATEROW_PARAM
 ,HBC_ERROR_CHECKANDUPDATEROW_EXCEPTION
 ,HBC_ERROR_CHECKANDUPDATEROW_NOTFOUND
 ,HBC_ERROR_DELETEROW_PARAM
 ,HBC_ERROR_DELETEROW_EXCEPTION
 ,HBC_ERROR_DELETEROWS_PARAM
 ,HBC_ERROR_DELETEROWS_EXCEPTION
 ,HBC_ERROR_CHECKANDDELETEROW_PARAM
 ,HBC_ERROR_CHECKANDDELETEROW_EXCEPTION
 ,HBC_ERROR_CHECKANDDELETEROW_NOTFOUND
 ,HBC_ERROR_GETKEYS
 ,HBC_ERROR_LISTALL
 ,HBC_ERROR_REGION_STATS
 ,HBC_ERROR_CREATE_SNAPSHOT_PARAM
 ,HBC_ERROR_CREATE_SNAPSHOT_EXCEPTION
 ,HBC_ERROR_DELETE_SNAPSHOT_PARAM
 ,HBC_ERROR_DELETE_SNAPSHOT_EXCEPTION
 ,HBC_ERROR_VERIFY_SNAPSHOT_PARAM
 ,HBC_ERROR_VERIFY_SNAPSHOT_EXCEPTION
 ,HBC_ERROR_TRUNCATE_PARAM
 ,HBC_ERROR_TRUNCATE_EXCEPTION
 ,HBC_LAST
} HBC_RetCode;

class HBaseClient_JNI : public JavaObjectInterface
{
public:
  static HBaseClient_JNI* getInstance();
  static void deleteInstance();

  // Destructor
  virtual ~HBaseClient_JNI();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HBC_RetCode init();
  
  HBC_RetCode initConnection(const char* zkServers, const char* zkPort); 
  bool isConnected() 
  {
    return isConnected_;
  }

  HTableClient_JNI* getHTableClient(NAHeap *heap, const char* tableName, 
               bool useTRex, ExHbaseAccessStats *hbs);
  HBulkLoadClient_JNI* getHBulkLoadClient(NAHeap *heap);
  HBC_RetCode releaseHBulkLoadClient(HBulkLoadClient_JNI* hblc);
  HBC_RetCode releaseHTableClient(HTableClient_JNI* htc);
  HBC_RetCode create(const char* fileName, HBASE_NAMELIST& colFamilies, NABoolean isMVCC);
  HBC_RetCode create(const char* fileName, NAText*  hbaseOptions, 
                     int numSplits, int keyLength, const char** splitValues, Int64 transID, NABoolean isMVCC);
  HBC_RetCode alter(const char* fileName, NAText*  hbaseOptions, Int64 transID);
  HBC_RetCode registerTruncateOnAbort(const char* fileName, Int64 transID);
  HBC_RetCode truncate(const char* fileName, NABoolean preserveSplits, Int64 transID);
  HBC_RetCode drop(const char* fileName, bool async, Int64 transID);
  HBC_RetCode drop(const char* fileName, JNIEnv* jenv, Int64 transID); // thread specific
  HBC_RetCode dropAll(const char* pattern, bool async, Int64 transID);
  HBC_RetCode copy(const char* srcTblName, const char* tgtTblName,
                   NABoolean force);
  NAArray<HbaseStr>* listAll(NAHeap *heap, const char* pattern);
  NAArray<HbaseStr>* getRegionStats(NAHeap *heap, const char* tblName);

  HBC_RetCode exists(const char* fileName, Int64 transID);
  HBC_RetCode grant(const Text& user, const Text& tableName, const TextVec& actionCodes); 
  HBC_RetCode revoke(const Text& user, const Text& tableName, const TextVec& actionCodes);
  HBC_RetCode estimateRowCount(const char* tblName, Int32 partialRowSize,
                               Int32 numCols, Int32 retryLimitMilliSeconds, NABoolean useCoprocessor,
                               Int64& rowCount, Int32 & breadCrumb);
  HBC_RetCode getLatestSnapshot(const char * tabname, char *& snapshotName, NAHeap * heap);
  HBC_RetCode cleanSnpTmpLocation(const char * path);
  HBC_RetCode setArchivePermissions(const char * path);
  HBC_RetCode getBlockCacheFraction(float& frac);
  HBC_RetCode getHbaseTableInfo(const char* tblName, Int32& indexLevels, Int32& blockSize);
  HBC_RetCode getRegionsNodeName(const char* tblName, Int32 partns, ARRAY(const char *)& nodeNames);

  // req processing in worker threads
  HBC_RetCode enqueueRequest(HBaseClientRequest *request);
  HBC_RetCode enqueueShutdownRequest();
  HBC_RetCode enqueueDropRequest(const char *fileName);
  HBC_RetCode doWorkInThread();
  HBC_RetCode startWorkerThreads();
  HBC_RetCode performRequest(HBaseClientRequest *request, JNIEnv* jenv);
  HBaseClientRequest* getHBaseRequest();
  bool workerThreadsStarted() { return (threadID_[0] ? true : false); }
  // Get the error description.
  static char* getErrorText(HBC_RetCode errEnum);
  
  static void logIt(const char* str);

  HTableClient_JNI *startGet(NAHeap *heap, const char* tableName, bool useTRex, 
            ExHbaseAccessStats *hbs, Int64 transID, const HbaseStr& rowID, 
            const LIST(HbaseStr) & cols, Int64 timestamp);
  HTableClient_JNI *startGets(NAHeap *heap, const char* tableName, bool useTRex, 
            ExHbaseAccessStats *hbs, Int64 transID, const LIST(HbaseStr) *rowIDs, 
            short rowIDLen, const HbaseStr *rowIDsInDB, 
            const LIST(HbaseStr) & cols, Int64 timestamp);
  HBC_RetCode incrCounter( const char * tabName, const char * rowId, const char * famName, 
                 const char * qualName , Int64 incr, Int64 & count);
  HBC_RetCode createCounterTable( const char * tabName,  const char * famName);
  HBC_RetCode insertRow(NAHeap *heap, const char *tableName,
                        ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, 
                        HbaseStr rowID,
                        HbaseStr row, Int64 timestamp,bool checkAndPut, 
                        bool asyncOperation, bool useRegionXn, 
			short colIndexToCheck,
                        HTableClient_JNI **outHtc);
  HBC_RetCode insertRows(NAHeap *heap, const char *tableName,
      ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, short rowIDLen, HbaseStr rowIDs,
      HbaseStr rows, Int64 timestamp,  bool asyncOperation,
      HTableClient_JNI **outHtc);
  HBC_RetCode checkAndUpdateRow(NAHeap *heap, const char *tableName,
                                ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, 
                                HbaseStr rowID,
                                HbaseStr row, HbaseStr columnToCheck, HbaseStr columnValToCheck,
                                Int64 timestamp, bool asyncOperation, bool useRegionXn,
                                HTableClient_JNI **outHtc);
  HBC_RetCode deleteRow(NAHeap *heap, const char *tableName,
      ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, HbaseStr rowID, 
                        const LIST(HbaseStr) *cols, 
                        Int64 timestamp, 
                        bool asyncOperation, 
                        bool useRegionXn,
                        HTableClient_JNI **outHtc);
  HBC_RetCode deleteRows(NAHeap *heap, const char *tableName,
      ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, short rowIDLen, HbaseStr rowIDs, 
      Int64 timestamp, bool asyncOperation, HTableClient_JNI **outHtc);
  HBC_RetCode checkAndDeleteRow(NAHeap *heap, const char *tableName,
                                ExHbaseAccessStats *hbs, bool useTRex, Int64 transID, HbaseStr rowID, 
                                HbaseStr columnToCheck, HbaseStr columnValToCheck,
                                Int64 timestamp, bool asyncOperation, bool useRegionXn,
                                HTableClient_JNI **outHtc);
  NAArray<HbaseStr>* getStartKeys(NAHeap *heap, const char *tableName, bool useTRex);
  NAArray<HbaseStr>* getEndKeys(NAHeap *heap, const char * tableName, bool useTRex);
  HBC_RetCode    createSnapshot( const NAString&  tableName, const NAString&  snapshotName);
  HBC_RetCode    deleteSnapshot( const NAString&  snapshotName);
  HBC_RetCode    verifySnapshot( const NAString&  tableName, const NAString&  snapshotName, NABoolean & exist);

private:   
  // private default constructor
  HBaseClient_JNI(NAHeap *heap);
  NAArray<HbaseStr>* getKeys(Int32 funcIndex, NAHeap *heap, const char *tableName, bool useTRex);

private:
  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_INIT
   ,JM_GET_HTC
   ,JM_REL_HTC
   ,JM_CREATE
   ,JM_CREATEK
   ,JM_TRUNCABORT
   ,JM_ALTER
   ,JM_DROP
   ,JM_DROP_ALL
   ,JM_LIST_ALL
   ,JM_GET_REGION_STATS
   ,JM_COPY
   ,JM_EXISTS
   ,JM_GRANT
   ,JM_REVOKE
   ,JM_GET_HBLC
   ,JM_EST_RC
   ,JM_EST_RC_COPROC
   ,JM_REL_HBLC
   ,JM_GET_CAC_FRC
   ,JM_GET_LATEST_SNP
   ,JM_CLEAN_SNP_TMP_LOC
   ,JM_SET_ARC_PERMS
   ,JM_START_GET
   ,JM_START_GETS
   ,JM_START_DIRECT_GETS
   ,JM_GET_HBTI
   ,JM_CREATE_COUNTER_TABLE  
   ,JM_INCR_COUNTER
   ,JM_GET_REGN_NODES
   ,JM_HBC_DIRECT_INSERT_ROW
   ,JM_HBC_DIRECT_INSERT_ROWS
   ,JM_HBC_DIRECT_CHECKANDUPDATE_ROW
   ,JM_HBC_DELETE_ROW
   ,JM_HBC_DIRECT_DELETE_ROWS
   ,JM_HBC_CHECKANDDELETE_ROW
   ,JM_HBC_GETSTARTKEYS
   ,JM_HBC_GETENDKEYS
   ,JM_HBC_CREATE_SNAPSHOT
   ,JM_HBC_DELETE_SNAPSHOT
   ,JM_HBC_VERIFY_SNAPSHOT
   ,JM_TRUNCATE
   ,JM_LAST
  };
  static jclass          javaClass_; 
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
  bool isConnected_;

  pthread_t threadID_[NUM_HBASE_WORKER_THREADS];
  pthread_mutex_t mutex_;  
  pthread_cond_t workBell_;
  typedef list<HBaseClientRequest *> reqList_t;
  reqList_t reqQueue_; 
};

// ===========================================================================
// ===== The HBulkLoadClient_JNI class implements access to the Java
// ===== HBulkLoadClient class.
// ===========================================================================

typedef enum {
  HBLC_OK     = JOI_OK
 ,HBLC_FIRST  = HTC_LAST
 ,HBLC_DONE   = HBLC_FIRST
 ,HBLC_ERROR_INIT_PARAM
 ,HBLC_ERROR_INIT_EXCEPTION
 ,HBLC_ERROR_CLEANUP_EXCEPTION
 ,HBLC_ERROR_CLOSE_EXCEPTION
 ,HBLC_ERROR_CREATE_HFILE_PARAM
 ,HBLC_ERROR_CREATE_HFILE_EXCEPTION
 ,HBLC_ERROR_ADD_TO_HFILE_PARAM
 ,HBLC_ERROR_ADD_TO_HFILE_EXCEPTION
 ,HBLC_ERROR_CLOSE_HFILE_PARAM
 ,HBLC_ERROR_CLOSE_HFILE_EXCEPTION
 ,HBLC_ERROR_DO_BULKLOAD_PARAM
 ,HBLC_ERROR_DO_BULKLOAD_EXCEPTION
 ,HBLC_ERROR_BULKLOAD_CLEANUP_PARAM
 ,HBLC_ERROR_BULKLOAD_CLEANUP_EXCEPTION
 ,HBLC_ERROR_INIT_HBLC_PARAM
 ,HBLC_ERROR_INIT_HBLC_EXCEPTION
 ,HBLC_LAST
} HBLC_RetCode;


class HBulkLoadClient_JNI : public JavaObjectInterface
{
public:

  HBulkLoadClient_JNI(NAHeap *heap,jobject jObj = NULL)
  :  JavaObjectInterface(heap, jObj)
  {
    heap_= heap;
  }
  // Destructor
  virtual ~HBulkLoadClient_JNI();

  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HBLC_RetCode init();

  HBLC_RetCode initHFileParams(const HbaseStr &tblName, const Text& hFileLoc, const Text& hfileName, Int64 maxHFileSize,
                              const char* sampleTblName, const char* hiveDDL);

  HBLC_RetCode addToHFile( short rowIDLen, HbaseStr &rowIDs, HbaseStr &rows, ExHbaseAccessStats *hbs);

  HBLC_RetCode closeHFile(const HbaseStr &tblName);

  HBLC_RetCode doBulkLoad(const HbaseStr &tblName, const Text& location, const Text& tableName, NABoolean quasiSecure, NABoolean snapshot);

  HBLC_RetCode  bulkLoadCleanup(const HbaseStr &tblName, const Text& location);
  // Get the error description.
  static char* getErrorText(HBLC_RetCode errEnum);


private:
  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_INIT_HFILE_PARAMS
   ,JM_CLOSE_HFILE
   ,JM_DO_BULK_LOAD
   ,JM_BULK_LOAD_CLEANUP
   ,JM_ADD_TO_HFILE_DB
   ,JM_LAST
  };
  static jclass          javaClass_;
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;

};

jobjectArray convertToByteArrayObjectArray(const LIST(NAString) &vec);
jobjectArray convertToByteArrayObjectArray(const LIST(HbaseStr) &vec);
jobjectArray convertToByteArrayObjectArray(const char **array,
                   int numElements, int elementLen);
jobjectArray convertToStringObjectArray(const TextVec &vec);
jobjectArray convertToStringObjectArray(const HBASE_NAMELIST& nameList);
jobjectArray convertToStringObjectArray(const NAText *text, int arrayLen);
int convertStringObjectArrayToList(NAHeap *heap, jarray j_objArray, 
                                         LIST(Text *)&list);
int convertByteArrayObjectArrayToNAArray(NAHeap *heap, jarray j_objArray, 
             NAArray<HbaseStr> **retArray);
void deleteNAArray(CollHeap *heap, NAArray<HbaseStr> *array);
#endif


