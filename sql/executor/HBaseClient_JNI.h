// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
// **********************************************************************
#ifndef HBASE_CLIENT_H
#define HBASE_CLIENT_H
#define INLINE_COLNAME_LEN 256

#include <list>
#include "Platform.h"
#include "Collections.h"
#include "NABasicObject.h"

#include "JavaObjectInterface.h"
#include "Hbase_types.h"
#include "ExpHbaseDefs.h"
#include "NAMemory.h"
#include "HTableClient.h"

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

// ===========================================================================
// ===== The ByteArrayList class implements access to the Java 
// ===== ByteArrayList class.
// ===========================================================================

typedef enum {
  BAL_OK     = JOI_OK
 ,BAL_FIRST  = JOI_LAST
 ,BAL_ERROR_ADD_PARAM = BAL_FIRST
 ,BAL_ERROR_ADD_EXCEPTION
 ,BAL_ERROR_GET_EXCEPTION
 ,BAL_LAST
} BAL_RetCode;

class ByteArrayList : public JavaObjectInterface
{
public:
  ByteArrayList(NAHeap *heap, jobject jObj = NULL)
    :  JavaObjectInterface(heap, jObj)
  {}

  // Destructor
  virtual ~ByteArrayList();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  BAL_RetCode    init();
  
  BAL_RetCode add(const Text& str);
    
  // Add a Text vector.
  BAL_RetCode add(const TextVec& vec);

  BAL_RetCode addElement(const char * data, int keyLength);
    
  // Get a Text element
  Text* get(Int32 i);

  // Get the error description.
  virtual char* getErrorText(BAL_RetCode errEnum);

  Int32 getSize();
  Int32 getEntrySize(Int32 i);
  char* getEntry(Int32 i, char* buf, Int32 bufLen, Int32& dataLen);


private:  
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_ADD,
    JM_GET,
    JM_GETSIZE,
    JM_GETENTRY,
    JM_GETENTRYSIZE,
    JM_LAST
  };
  
  static jclass          javaClass_;  
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

// ===========================================================================
// ===== The KeyValue class implements access to the Java 
// ===== KeyValue class.
// ===========================================================================

typedef enum {
  KYV_OK     = JOI_OK
 ,KYV_FIRST  = BAL_LAST
 ,KYV_ERROR_GETBUFFER = KYV_FIRST
 ,KYV_LAST
} KYV_RetCode;

class KeyValue : public JavaObjectInterface
{
public:
  KeyValue(NAHeap *heap, jobject jObj = NULL)
    :  JavaObjectInterface(heap, jObj)
  {}
  
  // Destructor
  virtual ~KeyValue();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  KYV_RetCode    init();
  
  Int32 getBufferSize();
  char* getBuffer(char* targetBuffer, Int32 buffSize);
  Int32 getFamilyLength();
  Int32 getFamilyOffset();
  Int32 getKeyLength();
  Int32 getKeyOffset();
  Int32 getQualifierLength();
  Int32 getQualifierOffset();
  Int32 getRowLength();
  Int32 getRowOffset();
  Int32 getValueLength();
  Int32 getValueOffset();
  Int64 getTimestamp();
  
  TCell* toTCell();
      
  // Get the error description.
  virtual char* getErrorText(KYV_RetCode errEnum);

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0, 
    JM_BUFFER, 
    JM_FAM_LEN,
    JM_FAM_OFF,
    JM_KEY_LEN,
    JM_KEY_OFF,
    JM_QUA_LEN,
    JM_QUA_OFF,
    JM_ROW_LEN,
    JM_ROW_OFF,
    JM_VAL_LEN,
    JM_VAL_OFF,
    JM_TS,    
    JM_LAST
  };
  
  static jclass          javaClass_;  
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
};

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
 ,HTC_FIRST  = KYV_LAST
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
 ,HTC_ERROR_SCANFETCH_EXCEPTION
 ,HTC_ERROR_FETCHROWS_EXCEPTION
 ,HTC_ERROR_SCANCLOSE_EXCEPTION
 ,HTC_ERROR_GETROWOPEN_PARAM
 ,HTC_ERROR_GETROWOPEN_EXCEPTION
 ,HTC_ERROR_GETROWSOPEN_PARAM
 ,HTC_ERROR_GETROWSOPEN_EXCEPTION
 ,HTC_ERROR_GETFETCH_EXCEPTION
 ,HTC_ERROR_GETCLOSE_EXCEPTION
 ,HTC_ERROR_DELETEROW_PARAM
 ,HTC_ERROR_DELETEROW_EXCEPTION
 ,HTC_ERROR_DELETEROWS_PARAM
 ,HTC_ERROR_DELETEROWS_EXCEPTION
 ,HTC_ERROR_CHECKANDDELETEROW_PARAM
 ,HTC_ERROR_CHECKANDDELETEROW_EXCEPTION
 ,HTC_ERROR_CHECKANDDELETE_ROW_NOTFOUND
 ,HTC_ERROR_INSERTROW_PARAM
 ,HTC_ERROR_INSERTROW_EXCEPTION
 ,HTC_ERROR_INSERTROWS_PARAM
 ,HTC_ERROR_INSERTROWS_EXCEPTION
 ,HTC_ERROR_CHECKANDINSERTROW_PARAM
 ,HTC_ERROR_CHECKANDINSERTROW_EXCEPTION
 ,HTC_ERROR_CHECKANDINSERT_DUP_ROWID
 ,HTC_ERROR_CHECKANDINSERT_NOTRANS
 ,HTC_ERROR_CHECKANDUPDATEROW_PARAM
 ,HTC_ERROR_CHECKANDUPDATEROW_EXCEPTION
 ,HTC_ERROR_CHECKANDUPDATE_ROW_NOTFOUND
 ,HTC_ERROR_CHECKANDUPDATE_NOTRANS
 ,HTC_ERROR_CREATE_PARAM
 ,HTC_ERROR_CREATE_EXCEPTION
 ,HTC_ERROR_DROP_PARAM
 ,HTC_ERROR_DROP_EXCEPTION
 ,HTC_ERROR_EXISTS_PARAM
 ,HTC_ERROR_EXISTS_EXCEPTION
 ,HTC_ERROR_COPROC_AGGR_PARAM
 ,HTC_ERROR_COPROC_AGGR_EXCEPTION
 ,HTC_ERROR_COPROC_AGGR_GET_RESULT_PARAM
 ,HTC_ERROR_COPROC_AGGR_GET_RESULT_EXCEPTION
 ,HTC_ERROR_GRANT_PARAM
 ,HTC_ERROR_GRANT_EXCEPTION
 ,HTC_ERROR_REVOKE_PARAM
 ,HTC_ERROR_REVOKE_EXCEPTION
 ,HTC_GETENDKEYS
 ,HTC_ERROR_GETHTABLENAME_EXCEPTION
 ,HTC_ERROR_FLUSHTABLE_EXCEPTION
 ,HTC_GET_COLNAME_EXCEPTION
 ,HTC_GET_COLVAL_EXCEPTION
 ,HTC_SET_JNIOBJECT_EXCEPTION
 ,HTC_LAST
} HTC_RetCode;

class HTableClient_JNI : public JavaObjectInterface
{
public:
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
     jRowIDs_ = NULL;
     jKvsPerRow_ = NULL;
     currentRowNum_ = -1;
     prevRowCellNum_ = 0;
     numRowsReturned_ = 0;
     numColsInScan_ = 0;
     colNameAllocLen_ = 0;
     inlineColName_[0] = '\0';
     colName_ = NULL;
     numReqRows_ = -1;
     cleanupDone_ = FALSE;
     p_kvValLen_ = NULL;
     p_kvValOffset_ = NULL;
     p_kvFamLen_ = NULL;
     p_kvFamOffset_ = NULL;
     p_kvQualLen_ = NULL;
     p_kvQualOffset_ = NULL;
     p_timestamp_ = NULL;
     jba_kvBuffer_ = NULL;
     jba_rowID_ = NULL;
     fetchMode_ = UNKNOWN;
     p_rowID_ = NULL;
     p_kvsPerRow_ = NULL;
     numCellsReturned_ = 0;
     numCellsAllocated_ = 0;
  }

  // Destructor
  virtual ~HTableClient_JNI();
  
  HTC_RetCode init();
  
  HTC_RetCode startScan(Int64 transID, const Text& startRowID, const Text& stopRowID, const TextVec& cols, Int64 timestamp, bool cacheBlocks, Lng32 numCacheRows,
                        NABoolean preFetch,
			const TextVec *inColNamesToFilter, 
			const TextVec *inCompareOpList,
			const TextVec *inColValuesToCompare,
			Float32 samplePercent = -1.0f);
  HTC_RetCode startGet(Int64 transID, const Text& rowID, const TextVec& cols, 
		Int64 timestamp, NABoolean directRow);
  HTC_RetCode startGets(Int64 transID, const TextVec& rowIDs, const TextVec& cols, 
		Int64 timestamp, NABoolean directRow);
  HTC_RetCode scanFetch();
  HTC_RetCode getFetch();
  KeyValue*   getLastFetchedCell();
  HTC_RetCode deleteRow(Int64 transID, HbaseStr &rowID, const TextVec& columns, Int64 timestamp);
  HTC_RetCode deleteRows(Int64 transID, short rowIDLen, HbaseStr &rowIDs, Int64 timestamp);
  HTC_RetCode checkAndDeleteRow(Int64 transID, HbaseStr &rowID, const Text &columnToCheck, const Text &colValToCheck, Int64 timestamp);
  HTC_RetCode insertRow(Int64 transID, HbaseStr &rowID, HbaseStr &row,
       Int64 timestamp);
  HTC_RetCode insertRows(Int64 transID, short rowIDLen, HbaseStr &rowIDs, HbaseStr &rows, Int64 timestamp, bool autoFlush);
  HTC_RetCode setWriteBufferSize(Int64 size);
  HTC_RetCode setWriteToWAL(bool vWAL);
  HTC_RetCode checkAndInsertRow(Int64 transID, HbaseStr &rowID, HbaseStr &row, Int64 timestamp);
  HTC_RetCode checkAndUpdateRow(Int64 transID, HbaseStr &rowID, HbaseStr &row,
       const Text &columnToCheck, const Text &colValToCheck, Int64 timestamp);
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
        jobjectArray jKvBuffer, jobjectArray jRowIDs,
        jintArray jKvsPerRow, jint numCellsReturned);
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
  HTC_RetCode getNumCols(int &numCols);
  HTC_RetCode getRowID(HbaseStr &rowID);
    
  //  HTC_RetCode codeProcAggrGetResult();

  const char *getTableName();
  std::string* getHTableName();

  // Get the error description.
  virtual char* getErrorText(HTC_RetCode errEnum);

  ByteArrayList* getEndKeys();

  HTC_RetCode flushTable(); 
  void setTableName(const char *tableName)
  {
    Int32 len = strlen(tableName);

    tableName_ = new (heap_) char[len+1];
    strcpy(tableName_, tableName);
  } 
  HTC_RetCode setJniObject();

private:
  NAString getLastJavaError();

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_GET_ERROR 
   ,JM_SCAN_OPEN 
   ,JM_GET_OPEN  
   ,JM_GETS_OPEN 
   ,JM_SCAN_FETCH
   ,JM_GET_FETCH 
   ,JM_GET_CELL  
   ,JM_DELETE    
   ,JM_CHECKANDDELETE
   ,JM_CHECKANDUPDATE
   ,JM_COPROC_AGGR
   ,JM_GET_NAME
   ,JM_GET_HTNAME
   ,JM_GETENDKEYS
   ,JM_FLUSHT
   ,JM_SET_WB_SIZE
   ,JM_SET_WRITE_TO_WAL
   ,JM_DIRECT_INSERT
   ,JM_DIRECT_CHECKANDINSERT
   ,JM_DIRECT_INSERT_ROWS
   ,JM_DIRECT_DELETE_ROWS
   ,JM_FETCH_ROWS
   ,JM_SET_JNIOBJECT
   ,JM_LAST
  };
  enum FETCH_MODE {
      UNKNOWN = 0
    , SCAN_FETCH
    , GET_ROW
    , BATCH_GET
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
  jbyteArray jba_rowID_;
  jbyte *p_rowID_;
  jint *p_kvsPerRow_;
  jint numRowsReturned_;
  int currentRowNum_;
  int numColsInScan_;
  int numReqRows_;
  int numCellsReturned_;
  int numCellsAllocated_;
  int prevRowCellNum_;
  char *colName_;
  char inlineColName_[INLINE_COLNAME_LEN+1];
  short colNameAllocLen_;
  FETCH_MODE fetchMode_; 
  NABoolean cleanupDone_;
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
 ,HBC_ERROR_CLEANUP_EXCEPTION
 ,HBC_ERROR_GET_HTC_EXCEPTION
 ,HBC_ERROR_REL_HTC_EXCEPTION
 ,HBC_ERROR_CREATE_PARAM
 ,HBC_ERROR_CREATE_EXCEPTION
 ,HBC_ERROR_DROP_PARAM
 ,HBC_ERROR_DROP_EXCEPTION
 ,HBC_ERROR_EXISTS_PARAM
 ,HBC_ERROR_EXISTS_EXCEPTION
 ,HBC_ERROR_FLUSHALL_PARAM
 ,HBC_ERROR_FLUSHALL_EXCEPTION
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
 ,HBC_LAST
} HBC_RetCode;

class HBaseClient_JNI : public JavaObjectInterface
{
public:
  static HBaseClient_JNI* getInstance(int debugPort, int debugTimeout);
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

  HBC_RetCode cleanup();
  HTableClient_JNI* getHTableClient(NAHeap *heap, const char* tableName, 
               bool useTRex);
  HBulkLoadClient_JNI* getHBulkLoadClient(NAHeap *heap);
  HBC_RetCode releaseHTableClient(HTableClient_JNI* htc);
  HBC_RetCode create(const char* fileName, HBASE_NAMELIST& colFamilies);
  HBC_RetCode create(const char* fileName, NAText*  hbaseOptions, 
                     int numSplits, int keyLength, const char** splitValues);
  HBC_RetCode drop(const char* fileName, bool async);
  HBC_RetCode drop(const char* fileName, JNIEnv* jenv); // thread specific
  HBC_RetCode dropAll(const char* pattern, bool async);
  HBC_RetCode copy(const char* currTblName, const char* oldTblName);
  static HBC_RetCode flushAllTablesStatic();
  HBC_RetCode flushAllTables();
  HBC_RetCode exists(const char* fileName);
  HBC_RetCode grant(const Text& user, const Text& tableName, const TextVec& actionCodes); 
  HBC_RetCode revoke(const Text& user, const Text& tableName, const TextVec& actionCodes);
  HBC_RetCode estimateRowCount(const char* tblName, Int32 partialRowSize,
                               Int32 numCols, Int64& rowCount);

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
  virtual char* getErrorText(HBC_RetCode errEnum);
  
  static void logIt(const char* str);

private:   
  // private default constructor
  HBaseClient_JNI(NAHeap *heap, int debugPort, int debugTimeout);

private:
  NAString  getLastJavaError();

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_GET_ERROR 
   ,JM_INIT
   ,JM_CLEANUP   
   ,JM_GET_HTC
   ,JM_REL_HTC
   ,JM_CREATE
   ,JM_CREATEK
   ,JM_DROP
   ,JM_DROP_ALL
   ,JM_COPY
   ,JM_EXISTS
   ,JM_FLUSHALL
   ,JM_GRANT
   ,JM_REVOKE
   ,JM_GET_HBLC
   ,JM_EST_RC
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
// ===== The HiveClient_JNI class implements access to the Java 
// ===== HiveClient class.
// ===========================================================================

typedef enum {
  HVC_OK     = JOI_OK
 ,HVC_FIRST  = HBC_LAST
 ,HVC_DONE   = HVC_FIRST
 ,HVC_ERROR_INIT_PARAM
 ,HVC_ERROR_INIT_EXCEPTION
 ,HVC_ERROR_CLOSE_EXCEPTION
 ,HVC_ERROR_EXISTS_PARAM
 ,HVC_ERROR_EXISTS_EXCEPTION
 ,HVC_ERROR_GET_HVT_PARAM
 ,HVC_ERROR_GET_HVT_EXCEPTION
 ,HVC_ERROR_GET_REDEFTIME_PARAM
 ,HVC_ERROR_GET_REDEFTIME_EXCEPTION
 ,HVC_ERROR_GET_ALLSCH_EXCEPTION
 ,HVC_ERROR_GET_ALLTBL_PARAM
 ,HVC_ERROR_GET_ALLTBL_EXCEPTION
 ,HVC_LAST
} HVC_RetCode;

class HiveClient_JNI : public JavaObjectInterface
{
public:
  
  static HiveClient_JNI* getInstance();
  static void deleteInstance();

  // Destructor
  virtual ~HiveClient_JNI();
  
  // Initialize JVM and all the JNI configuration.
  // Must be called.
  HVC_RetCode init();
  
  HVC_RetCode initConnection(const char* metastoreURI); 
  bool isConnected() 
  {
    return isConnected_;
  }

  HVC_RetCode close();
  HVC_RetCode exists(const char* schName, const char* tabName);
  HVC_RetCode getHiveTableStr(const char* schName, const char* tabName, 
                              Text& hiveTblStr);
  HVC_RetCode getRedefTime(const char* schName, const char* tabName, 
                           Int64& redefTime);
  HVC_RetCode getAllSchemas(LIST(Text *)& schNames);
  HVC_RetCode getAllTables(const char* schName, LIST(Text *)& tblNames);

  // Get the error description.
  virtual char* getErrorText(HVC_RetCode errEnum);
  
  static void logIt(const char* str);

private:   
  // Private Default constructor		
  HiveClient_JNI(NAHeap *heap)
  :  JavaObjectInterface(heap)
  , isConnected_(FALSE)
  {}

private:
  NAString getLastJavaError();

private:  
  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_GET_ERROR 
   ,JM_INIT
   ,JM_CLOSE
   ,JM_EXISTS     
   ,JM_GET_HVT
   ,JM_GET_RDT
   ,JM_GET_ASH
   ,JM_GET_ATL
   ,JM_LAST
  };
  static jclass          javaClass_; 
  static JavaMethodInit* JavaMethods_;
  static bool javaMethodsInitialized_;
  // this mutex protects both JaveMethods_ and javaClass_ initialization
  static pthread_mutex_t javaMethodsInitMutex_;
  bool isConnected_;
};

// ===========================================================================
// ===== The HBulkLoadClient_JNI class implements access to the Java
// ===== HBulkLoadClient class.
// ===========================================================================

typedef enum {
  HBLC_OK     = JOI_OK
 ,HBLC_FIRST  = HVC_LAST
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

  HBLC_RetCode initHFileParams(const HbaseStr &tblName, const Text& hFileLoc, const Text& hfileName, Int64 maxHFileSize);

  HBLC_RetCode addToHFile( short rowIDLen, HbaseStr &rowIDs, HbaseStr &rows);

  HBLC_RetCode closeHFile(const HbaseStr &tblName);

  HBLC_RetCode doBulkLoad(const HbaseStr &tblName, const Text& location, const Text& tableName, NABoolean quasiSecure, NABoolean snapshot);

  HBLC_RetCode  bulkLoadCleanup(const HbaseStr &tblName, const Text& location);

  // Get the error description.
  virtual char* getErrorText(HBLC_RetCode errEnum);


private:
  NAString getLastJavaError();


  enum JAVA_METHODS {
    JM_CTOR = 0
   ,JM_GET_ERROR
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

jobjectArray convertToByteArrayObjectArray(const TextVec &vec);
jobjectArray convertToByteArrayObjectArray(const char **array,
                   int numElements, int elementLen);
jobjectArray convertToStringObjectArray(const TextVec &vec);
jobjectArray convertToStringObjectArray(const HBASE_NAMELIST& nameList);
jobjectArray convertToStringObjectArray(const NAText *text, int arrayLen);
int convertStringObjectArrayToList(NAHeap *heap, jarray j_objArray, 
                                         LIST(Text *)&list);
#endif


