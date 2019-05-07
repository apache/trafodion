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
#ifndef EX_HBASE_ACCESS_H
#define EX_HBASE_ACCESS_H

//
// Task Definition Block
//
#include "ComTdbHbaseAccess.h"
#include "ExStats.h"
#include "ex_queue.h"
#include "ExpHbaseDefs.h"
#include "ExpHbaseInterface.h"
#include "ComKeySingleSubset.h"
#include "ComKeyMDAM.h"
#include "key_range.h"
#include "key_single_subset.h"
#include "ex_mdam.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExHbaseAccessTdb;
class ExHbaseAccessTcb;

// -----------------------------------------------------------------------
// Classes referenced in this file
// -----------------------------------------------------------------------
class ex_tcb;
class ExpHbaseInterface;
class ExHbaseAccessSelectTcb;
class ExHbaseAccessUMDTcb;
class HdfsClient;

#define INLINE_ROWID_LEN 255
// -----------------------------------------------------------------------
// ExHbaseAccessTdb
// -----------------------------------------------------------------------
class ExHbaseAccessTdb : public ComTdbHbaseAccess
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExHbaseAccessTdb()
  {}

  virtual ~ExHbaseAccessTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the appropriate ComTdb subclass instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExHbaseAccessTcb  : public ex_tcb
{
  friend class ExHbaseTaskTcb;
  friend class ExHbaseScanTaskTcb;
  friend class ExHbaseScanRowwiseTaskTcb;
  friend class ExHbaseGetTaskTcb;
  friend class ExHbaseGetRowwiseTaskTcb;
  friend class ExHbaseScanSQTaskTcb;
  friend class ExHbaseGetSQTaskTcb;
  friend class ExHbaseUMDtrafUniqueTaskTcb;
  friend class ExHbaseUMDnativeUniqueTaskTcb;
  friend class ExHbaseUMDtrafSubsetTaskTcb;
  friend class ExHbaseUMDnativeSubsetTaskTcb;
  friend class ExHbaseAccessUMDTcb;

public:
  //  ExHbaseAccessTcb( const ExHbaseAccessTdb &tdb,
  ExHbaseAccessTcb( const ComTdbHbaseAccess &tdb,
		    ex_globals *glob );

  ~ExHbaseAccessTcb();

  void freeResources();
  virtual void registerSubtasks();  // register work procedures with scheduler

  virtual ExWorkProcRetcode work(); 
  
  ex_queue_pair getParentQueue() const { return qparent_;}
  virtual Int32 numChildren() const { return 0; }
  virtual const ex_tcb* getChild(Int32 /*pos*/) const { return NULL; }
  virtual NABoolean needStatsEntry();
  virtual ExOperStats *doAllocateStatsEntry(CollHeap *heap,
					    ComTdb *tdb);
  virtual ex_tcb_private_state *allocatePstates(
    Lng32 &numElems,
    Lng32 &pstateLength);

  ExHbaseAccessStats *getHbaseAccessStats()
  {
    if (getStatsEntry())
      return getStatsEntry()->castToExHbaseAccessStats();
    else
      return NULL;
  }

  // Overridden by ExHbaseAccessSelectTcb, for which sampling may be used.
  virtual Float32 getSamplePercentage() const
  {
    return -1.0f;
  }

  Lng32 numRowsInDirectBuffer() { return directBufferRowNum_; }

  static void incrErrorCount( ExpHbaseInterface * ehi,Int64 & totalExceptionCount,
                               const char * tabName, const char * rowId);

  static void getErrorCount( ExpHbaseInterface * ehi,Int64 & totalExceptionCount,
                               const char * tabName, const char * rowId);

  void handleException(NAHeap *heap,
                          char *loggingDdata,
                          Lng32 loggingDataLen,
                          ComCondition *errorCond);

  static void buildLoggingPath(const char * loggingLocation,
                               char *logId,
                               const char *tableName,
                               char * currCmdLoggingLocation);
  static void buildLoggingFileName(NAHeap *heap,
                               const char * currCmdLoggingLocation,
                               const char *tableName,
                               const char * loggingFileNamePrefix,
                               Lng32 instId,
                               char *& loggingFileName);
  static short setupError(NAHeap *heap, ex_queue_pair &qparent, Lng32 retcode, const char * str, const char * str2 = NULL);

  int compareRowIds();

protected:

  /////////////////////////////////////////////////////
  // Private methods.
  /////////////////////////////////////////////////////

  inline ExHbaseAccessTdb &hbaseAccessTdb() const
    { return (ExHbaseAccessTdb &) tdb; }

  inline ex_expr *convertExpr() const 
    { return hbaseAccessTdb().convertExpr_; }

  inline ex_expr *updateExpr() const 
    { return hbaseAccessTdb().updateExpr_; }

  inline ex_expr *mergeInsertExpr() const 
    { return hbaseAccessTdb().mergeInsertExpr_; }

  inline ex_expr *lobDelExpr() const 
    { return hbaseAccessTdb().mergeInsertExpr_; }

  inline ex_expr *mergeInsertRowIdExpr() const 
    { return hbaseAccessTdb().mergeInsertRowIdExpr_; }

  inline ex_expr *mergeUpdScanExpr() const 
    { return hbaseAccessTdb().mergeUpdScanExpr_; }

  inline ex_expr *returnFetchExpr() const 
    { return hbaseAccessTdb().returnFetchExpr_; }

  inline ex_expr *returnUpdateExpr() const 
    { return hbaseAccessTdb().returnUpdateExpr_; }

  inline ex_expr *returnMergeInsertExpr() const 
    { return hbaseAccessTdb().returnMergeInsertExpr_; }

  inline ex_expr *scanExpr() const 
    { return hbaseAccessTdb().scanExpr_; }

  inline ex_expr *rowIdExpr() const 
    { return hbaseAccessTdb().rowIdExpr_; }

  inline ex_expr *encodedKeyExpr() const 
    { return hbaseAccessTdb().encodedKeyExpr_; }

  inline ex_expr *keyColValExpr() const 
    { return hbaseAccessTdb().keyColValExpr_; }

  inline ex_expr *insDelPreCondExpr() const
    { return hbaseAccessTdb().insDelPreCondExpr_; }

  inline ex_expr *insConstraintExpr() const
    { return hbaseAccessTdb().insConstraintExpr_; }

  inline ex_expr *updConstraintExpr() const
    { return hbaseAccessTdb().updConstraintExpr_; }

  inline ex_expr *hbaseFilterValExpr() const 
    { return hbaseAccessTdb().hbaseFilterExpr_; }

  ex_expr * beginKeyExpr() const
  { 
    return (keySubsetExeExpr() ? keySubsetExeExpr()->bkPred() : NULL);
  }

  ex_expr * endKeyExpr() const
  { 
    return (keySubsetExeExpr() ? keySubsetExeExpr()->ekPred() : NULL);
  }
  
  keySingleSubsetEx * keySubsetExeExpr() const
  {
    return keySubsetExeExpr_;
  }
 
  keyMdamEx * keyMdamExeExpr() const
  {
    return keyMdamExeExpr_;
  }
  
  keyRangeEx * keyExeExpr() const
  {
    if (keySubsetExeExpr_)
      return keySubsetExeExpr_;
    else if (keyMdamExeExpr())
      return keyMdamExeExpr();
    else
      return NULL;
  }

  short allocateUpEntryTupps
    (Lng32 tupp1index, Lng32 tupp1length, Lng32 tupp2index, Lng32 tupp2length,
     NABoolean isVarchar,
     short * rc);
  
  short moveRowToUpQueue(const char * row, Lng32 len, 
			 short * rc, NABoolean isVarchar);
  short moveRowToUpQueue(short * rc);
  
  short raiseError(Lng32 errcode, 
                   Lng32 * intParam1 = NULL,
                   const char * str1 = NULL, 
                   const char * str2 = NULL);

  short setupError(Lng32 retcode, const char * str, const char * str2 = NULL);
  short handleError(short &rc);
  short handleDone(ExWorkProcRetcode &rc, Int64 rowsAffected = 0);
  short createColumnwiseRow();
  short createRowwiseRow();
  Lng32 createSQRowFromHbaseFormat(Int64 *lastestRowTimestamp = NULL);
  Lng32 createSQRowFromHbaseFormatMulti();
  Lng32 createSQRowFromAlignedFormat(Int64 *lastestRowTimestamp = NULL);
  NABoolean missingValuesInAlignedFormatRow(
       ExpTupleDesc * tdesc, char * alignedRow, Lng32 alignedRowLen);

  short copyCell();
  Lng32 createSQRowDirect(Int64 *lastestRowTimestamp = NULL);
  Lng32 setupSQMultiVersionRow();

  void extractColNameFields
    (char * inValPtr, short &colNameLen, char* &colName);

  short getColPos(char * colName, Lng32 colNameLen, Lng32 &idx);
  short applyPred(ex_expr * expr,UInt16 tuppIndex = 0,
		  char * tuppRow = NULL);
  void setupPrevRowId();
  short extractColFamilyAndName(char * input, Text &colFam, Text &colName);
  short evalKeyColValExpr(HbaseStr &columnToCheck, HbaseStr &colValToCheck);
  short evalInsDelPreCondExpr();
  short evalConstraintExpr(ex_expr *expr, UInt16 tuppIndex = 0,
                  char * tuppRow = NULL);
  short evalEncodedKeyExpr();
  short evalRowIdExpr(NABoolean isVarchar);
  short evalRowIdAsciiExpr(NABoolean noVarchar = FALSE);
  short evalRowIdAsciiExpr(const char * inputRowIdVals,
			   char * rowIdBuf, // input: buffer where rowid is created
			   char* &outputRowIdPtr,  // output: ptr to rowid.
			   Lng32 excludeKey,
			   Lng32 &outputRowIdLen);
  Lng32 setupUniqueRowIdsAndCols
    (ComTdbHbaseAccess::HbaseGetRows*hgr);
  Lng32 setupSubsetRowIdsAndCols
    (ComTdbHbaseAccess::HbaseScanRows* hsr);
  Lng32 genAndAssignSyskey(UInt16 tuppIndex, char * tuppRow);

  short createDirectAlignedRowBuffer( UInt16 tuppIndex, 
                                      char * tuppRow,
                                      Queue * listOfColNames, 
                                      NABoolean isUpdate,
                                      std::vector<UInt32> * posVec );

  short createDirectRowBuffer(UInt16 tuppIndex, char * tuppRow, 
                        Queue * listOfColNames,
                        Queue * listOfOmittedColNames,
			NABoolean isUpdate = FALSE,
			std::vector<UInt32> * posVec = NULL,
			double sampleRate = 0.0);
  short createDirectRowBuffer(Text &colFamily,
                         Text &colName,
                         Text &colVal);
  short createDirectRowwiseBuffer(char * inputRow);

  Lng32 initNextKeyRange(sql_buffer_pool *pool = NULL, atp_struct * atp = NULL);
  Lng32 setupUniqueKeyAndCols(NABoolean doInit);
  keyRangeEx::getNextKeyRangeReturnType  setupSubsetKeys
    (NABoolean fetchRangeHadRows = FALSE);
  Lng32 setupSubsetKeysAndCols();

  Lng32 setupListOfColNames(Queue * listOfColNames, LIST(HbaseStr) &columns);
  Lng32 setupListOfColNames(Queue * listOfColNames, LIST(NAString) &columns);

  short setupHbaseFilterPreds();
  void setRowID(char *rowId, Lng32 rowIdLen);
  void allocateDirectBufferForJNI(UInt32 rowLen);
  void allocateDirectRowBufferForJNI(short numCols, 
                          short maxRows = 1);
  short patchDirectRowBuffers();
  short patchDirectRowIDBuffers();
  void allocateDirectRowIDBufferForJNI(short maxRows = 1);
  Lng32 copyColToDirectBuffer( BYTE *rowCurPtr, 
                char *colName, short colNameLen,
                NABoolean prependNullVal, char nullVal, 
                char *colVal, Int32 colValLen);
  short copyRowIDToDirectBuffer(HbaseStr &rowID);

  short numColsInDirectBuffer()
  {
    if (row_.val != NULL)
       return bswap_16(*(short *)row_.val);
    else
        return 0;
  }
  /////////////////////////////////////////////////////
  //
  // Private data.
  /////////////////////////////////////////////////////

  ex_queue_pair  qparent_;
  Int64 matches_;
  atp_struct     * workAtp_;

  sql_buffer_pool * pool_;    

  ExpHbaseInterface * ehi_;

  HBASE_NAMELIST hnl_;
  HbaseStr table_;
  HbaseStr rowId_;

  HbaseStr colFamName_;
  HbaseStr colName_;

  HbaseStr colVal_;
  Int64 colTS_;

  Text beginRowId_;
  Text endRowId_;

  LIST(HbaseStr) rowIds_;
  LIST(HbaseStr) columns_;
  LIST(HbaseStr) deletedColumns_;
  LIST(NAString) hbaseFilterColumns_;
  LIST(NAString) hbaseFilterOps_;
  LIST(NAString) hbaseFilterValues_;

  char * asciiRow_;
  char * convertRow_;
  UInt32 convertRowLen_;
  char * updateRow_;
  char * mergeInsertRow_;
  char * rowIdRow_;
  char * rowIdAsciiRow_;
  char * beginRowIdRow_;
  char * endRowIdRow_;
  char * asciiRowMissingCols_;
  long * latestTimestampForCols_;
  long * latestVersionNumForCols_;
  char * beginKeyRow_;
  char * endKeyRow_;
  char * encodedKeyRow_;
  char * keyColValRow_;
  char * hbaseFilterValRow_;

  char * rowwiseRow_;
  Lng32 rowwiseRowLen_;
  HbaseStr prevRowId_;
  Lng32 prevRowIdMaxLen_;
  NABoolean isEOD_;

  ComTdbHbaseAccess::HbaseScanRows * hsr_;
  ComTdbHbaseAccess::HbaseGetRows * hgr_;

  keySingleSubsetEx * keySubsetExeExpr_;
  keyMdamEx * keyMdamExeExpr_;

  Lng32 currRowidIdx_;

  HbaseStr rowID_;

  // length of aligned format row created by eval method. 
  ULng32 insertRowlen_; 

  Lng32 rowIDAllocatedLen_;
  char *rowIDAllocatedVal_;
  // Direct Buffer to store multiple rowIDs
  // Format for rowID direct buffer
  // numRowIds + rowIDSuffix + rowId + rowIDSuffix + rowId + …
  // rowId len is passed as a parameter to Java functions
  // rowIDSuffix is '0' then the subsequent rowID is of length 
  //                             =  passed rowID len
  //                '1' then the subsequent rowID is of length
  //                              = passed rowID len+1
  BYTE *directRowIDBuffer_;
  Lng32 directRowIDBufferLen_;
  // Current row num in the Direct Buffers
  short directBufferRowNum_;
  // rowID of the current row
  HbaseStr dbRowID_;
  // Structure to keep track of current position in the rowIDs direct buffer
  HbaseStr rowIDs_;
  // Direct buffer for one or more rows containing the column names and
  // values
  //  Format for Row direct buffer
  //   numCols
  //                __
  //    colNameLen    |
  //    colName       |   For each non-null column value
  //    colValueLen   |
  //    colValue   __ |
  //                                
  // The colValue will have one byte null indicator 0 or -1 if column is nullabl
  // The numRowIds, numCols, colNameLen and colValueLen are byte swapped because
  // Java always use BIG_ENDIAN format. But, the column value need not be byte
  // swapped because column value is always interpreted in the JNI side.
  //
  BYTE *directRowBuffer_;
  Lng32 directRowBufferLen_;
  short directBufferMaxRows_;
  // Structure to keep track of current row
  HbaseStr row_;
  // Structure to keep track of current position in direct row buffer
  HbaseStr rows_;

  struct ColValVec
  {
    Lng32 numVersions_;
    char ** versionArray_;
    Int64 * timestampArray_;
    Int64 * versionNumArray_;
  };

  ColValVec * colValVec_;
  Lng32 colValVecSize_;
  Lng32 colValEntry_;
  Int16 asyncCompleteRetryCount_;
  NABoolean *resultArray_;
  NABoolean asyncOperation_;
  Int32 asyncOperationTimeout_;
  ComDiagsArea *loggingErrorDiags_;
  HdfsClient *logFileHdfsClient_;
  char *loggingFileName_;
  NABoolean loggingFileCreated_ ;

  // Redefined and used by ExHbaseAccessBulkLoadPrepSQTcb.

  virtual HdfsClient *sampleFileHdfsClient() const { return NULL; }
};

class ExHbaseTaskTcb : public ExGod
{
 public:
  ExHbaseTaskTcb(ExHbaseAccessTcb * tcb,  NABoolean rowsetTcb = FALSE);

  virtual ExWorkProcRetcode work(short &retval);

  virtual void init() {};

 protected:

  ExHbaseAccessTcb * tcb_;

  // To allow cancel, some tasks will need to return to the 
  // scheduler occasionally.
  Lng32 batchSize_;
  NABoolean rowsetTcb_;
  Lng32 remainingInBatch_;
};

class ExHbaseScanTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseScanTaskTcb(ExHbaseAccessSelectTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SCAN_OPEN
    , NEXT_ROW
    , NEXT_CELL 
    , SCAN_CLOSE
    , CREATE_ROW
    , RETURN_ROW
    , APPLY_PRED
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseScanRowwiseTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseScanRowwiseTaskTcb(ExHbaseAccessSelectTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SCAN_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , SCAN_CLOSE
    , APPEND_ROW
    , CREATE_ROW
    , RETURN_ROW
    , APPLY_PRED
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseScanSQTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseScanSQTaskTcb(ExHbaseAccessSelectTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  Lng32 getProbeResult(char* &keyData);

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SCAN_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , SCAN_CLOSE
    , APPEND_ROW
    , SETUP_MULTI_VERSION_ROW
    , CREATE_ROW
    , RETURN_ROW
    , APPLY_PRED
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseGetTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseGetTaskTcb(ExHbaseAccessSelectTcb * tcb);

  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , GET_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , GET_CLOSE 
    , CREATE_ROW
    , APPLY_PRED
    , RETURN_ROW
    , COLLECT_STATS
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseGetRowwiseTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseGetRowwiseTaskTcb(ExHbaseAccessSelectTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , GET_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , GET_CLOSE 
    , APPEND_ROW
    , CREATE_ROW
    , APPLY_PRED
    , RETURN_ROW
    , COLLECT_STATS
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseGetSQTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseGetSQTaskTcb(ExHbaseAccessTcb * tcb, NABoolean rowsetTcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , GET_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , GET_CLOSE 
    , APPEND_ROW
    , CREATE_ROW
    , APPLY_PRED
    , RETURN_ROW
    , COLLECT_STATS
    , HANDLE_ERROR
    , DONE
    , ALL_DONE
  } step_;
};

class ExHbaseAccessSelectTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessSelectTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 

  virtual Float32 getSamplePercentage() const
    {
      return samplePercentage_;
    }

 private:
  enum {
    NOT_STARTED
    , SELECT_INIT
    , SETUP_SCAN
    , PROCESS_SCAN
    , NEXT_SCAN
    , SETUP_GET
    , PROCESS_GET
    , NEXT_GET
    , SETUP_SCAN_KEY
    , SETUP_GET_KEY
    , PROCESS_SCAN_KEY
    , PROCESS_GET_KEY
    , SELECT_CLOSE
    , SELECT_CLOSE_NO_ERROR
    , HANDLE_ERROR
    , HANDLE_ERROR_NO_CLOSE
    , DONE
  } step_;

 protected:
  Float32 samplePercentage_;    // -1.0 if sampling not used

  ExHbaseScanTaskTcb * scanTaskTcb_;
  ExHbaseScanRowwiseTaskTcb * scanRowwiseTaskTcb_;
  ExHbaseGetTaskTcb * getTaskTcb_;
  ExHbaseGetRowwiseTaskTcb * getRowwiseTaskTcb_;
  ExHbaseScanSQTaskTcb * scanSQTaskTcb_;
  ExHbaseGetSQTaskTcb * getSQTaskTcb_;

  ExHbaseTaskTcb * scanTask_;
  ExHbaseTaskTcb * getTask_;

};

class ExHbaseAccessMdamSelectTcb  : public ExHbaseAccessSelectTcb
{
public:
  ExHbaseAccessMdamSelectTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 private:
  enum {
    NOT_STARTED
    , SELECT_INIT
    , INIT_NEXT_KEY_RANGE
    , GET_NEXT_KEY_RANGE
    , PROCESS_PROBE_RANGE
    , PROCESS_FETCH_RANGE
    , SELECT_CLOSE
    , SELECT_CLOSE_NO_ERROR
    , HANDLE_ERROR
    , HANDLE_ERROR_NO_CLOSE
    , DONE
  } step_;

  // The indicator below tells if we have a key range to fetch
  // with. If we have a key range it could be fetch key range or a
  // probe key range.
  //  keyRangeEx::getNextKeyRangeReturnType keyRangeType_; 
  
  // the key range iterator likes to know if the fetch ranges they
  // have provided were non-empty; we keep track of that here
  NABoolean fetchRangeHadRows_;       

  Int64 matchesBeforeFetch_;
};

class ExHbaseAccessDeleteTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessDeleteTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 private:
  enum {
    NOT_STARTED
    , DELETE_INIT
    , SETUP_DELETE
    , GET_NEXT_ROWID
    , GET_NEXT_COL
    , PROCESS_DELETE
    , ADVANCE_NEXT_ROWID
    , GET_NEXT_ROW
    , DELETE_CLOSE
    , HANDLE_ERROR
    , DONE
  } step_;

 protected:
  ComTdbHbaseAccess::HbaseGetRows * hgr_;

  const char * currColName_;
  
};

class ExHbaseAccessDeleteSubsetTcb  : public ExHbaseAccessDeleteTcb
{
public:
  ExHbaseAccessDeleteSubsetTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 private:
  enum {
    NOT_STARTED
    , SCAN_INIT
    , SCAN_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , CREATE_ROW
    , APPLY_PRED
    , SCAN_CLOSE
    , DELETE_ROW
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseAccessInsertTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessInsertTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 

 protected:
  enum {
    NOT_STARTED
    , INSERT_INIT
    , SETUP_INSERT
    , EVAL_INSERT_EXPR
    , EVAL_CONSTRAINT
    , CREATE_MUTATIONS
    , EVAL_ROWID_EXPR
    , CHECK_AND_INSERT
    , PROCESS_INSERT
    , PROCESS_INSERT_AND_CLOSE
    , RETURN_ROW
    , INSERT_CLOSE
    , HANDLE_EXCEPTION
    , HANDLE_ERROR
    , DONE
    , ALL_DONE
    , COMPLETE_ASYNC_INSERT
    , CLOSE_AND_DONE

  } step_;

  //  const char * insRowId_;
  Text insRowId_;

  Text insColFam_;
  Text insColNam_;
  Text insColVal_;
  const Int64 * insColTS_;
};

class ExHbaseAccessInsertRowwiseTcb  : public ExHbaseAccessInsertTcb
{
public:
  ExHbaseAccessInsertRowwiseTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 protected:
};

class ExHbaseAccessInsertSQTcb  : public ExHbaseAccessInsertTcb
{
public:
  ExHbaseAccessInsertSQTcb( const ExHbaseAccessTdb &tdb,
			    ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 private:
  Int64 insColTSval_;

};

class ExHbaseAccessUpsertVsbbSQTcb  : public ExHbaseAccessInsertTcb
{
public:
  ExHbaseAccessUpsertVsbbSQTcb( const ExHbaseAccessTdb &tdb,
				ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 protected:
  Int64 insColTSval_;
  Lng32 currRowNum_;

  queue_index prevTailIndex_;
  queue_index nextRequest_;    // next request in down queue

  Lng32 numRetries_;

  Lng32 rowsInserted_;
  int lastHandledStep_;
  Lng32 numRowsInVsbbBuffer_;
};


class ExHbaseAccessBulkLoadPrepSQTcb: public ExHbaseAccessUpsertVsbbSQTcb
{
  public:
    ExHbaseAccessBulkLoadPrepSQTcb( const ExHbaseAccessTdb &tdb,
                                ex_globals *glob );
    virtual ~ExHbaseAccessBulkLoadPrepSQTcb();

    virtual ExWorkProcRetcode work();

  protected:
    virtual HdfsClient *sampleFileHdfsClient() const
    {
      return sampleFileHdfsClient_;
    }

   private:
    void getHiveCreateTableDDL(NAString& hiveSampleTblNm, NAString& ddlText);

    short createLoggingRow( UInt16 tuppIndex,  char * tuppRow, char * targetRow, int &targetRowLen);

    NABoolean hFileParamsInitialized_;  ////temporary-- need better mechanism later
    Text   familyLocation_;
    Text   importLocation_;
    Text   hFileName_;

    ComCondition * lastErrorCnd_;
    std::vector<UInt32> posVec_;

    char * prevRowId_;
    char * loggingRow_;


    // HDFS file system and output file ptrs used for ustat sample table.
    HdfsClient *sampleFileHdfsClient_;
};
// UMD SQ: UpdMergeDel on Trafodion table
class ExHbaseUMDtrafSubsetTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseUMDtrafSubsetTaskTcb(ExHbaseAccessUMDTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SCAN_INIT
    , SCAN_OPEN
    , NEXT_ROW
    , CREATE_FETCHED_ROW
    , CREATE_UPDATED_ROW
    , EVAL_INS_CONSTRAINT
    , EVAL_UPD_CONSTRAINT
    , CREATE_MUTATIONS
    , APPLY_PRED
    , APPLY_MERGE_UPD_SCAN_PRED
    , RETURN_ROW
    , SCAN_CLOSE
    , SCAN_CLOSE_AND_INIT
    , UPDATE_ROW
    , DELETE_ROW
    , EVAL_RETURN_ROW_EXPRS
    , RETURN_UPDATED_ROWS
    , HANDLE_ERROR
    , DONE
  } step_;

};

// UMD: UpdMergeDel on native Hbase table
class ExHbaseUMDnativeSubsetTaskTcb  : public ExHbaseUMDtrafSubsetTaskTcb
{
public:
  ExHbaseUMDnativeSubsetTaskTcb(ExHbaseAccessUMDTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SCAN_INIT
    , SCAN_OPEN
    , NEXT_ROW
    , NEXT_CELL
    , APPEND_CELL_TO_ROW
    , CREATE_FETCHED_ROWWISE_ROW
    , CREATE_UPDATED_ROWWISE_ROW
    , CREATE_MUTATIONS
    , APPLY_PRED
    , SCAN_CLOSE
    , UPDATE_ROW
    , DELETE_ROW
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseUMDtrafUniqueTaskTcb  : public ExHbaseTaskTcb
{
public:
  ExHbaseUMDtrafUniqueTaskTcb(ExHbaseAccessUMDTcb * tcb);

  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SETUP_UMD
    , GET_NEXT_ROWID
    , NEXT_ROW 
    , CREATE_FETCHED_ROW
    , CREATE_UPDATED_ROW
    , CREATE_MERGE_INSERTED_ROW
    , CREATE_MUTATIONS
    , EVAL_INS_CONSTRAINT
    , EVAL_UPD_CONSTRAINT
    , APPLY_PRED
    , APPLY_MERGE_UPD_SCAN_PRED
    , RETURN_ROW
    , GET_CLOSE
    , UPDATE_ROW
    , INSERT_ROW
    , CHECK_AND_INSERT_ROW
    , NEXT_ROW_AFTER_UPDATE
    , DELETE_ROW
    , CHECK_AND_DELETE_ROW
    , CHECK_AND_UPDATE_ROW
    , EVAL_RETURN_ROW_EXPRS
    , RETURN_UPDATED_ROWS
    , HANDLE_ERROR
    , DONE
  } step_;

 protected:
  NABoolean rowUpdated_;
  Int64 latestRowTimestamp_;
  HbaseStr columnToCheck_;
  HbaseStr colValToCheck_;
};

// UMD: unique UpdMergeDel on native Hbase table
class ExHbaseUMDnativeUniqueTaskTcb  : public ExHbaseUMDtrafUniqueTaskTcb
{
public:
  ExHbaseUMDnativeUniqueTaskTcb(ExHbaseAccessUMDTcb * tcb);
  
  virtual ExWorkProcRetcode work(short &retval); 

  virtual void init();

 private:
  enum {
    NOT_STARTED
    , SETUP_UMD
    , SCAN_INIT
    , GET_NEXT_ROWID
    , NEXT_ROW
    , NEXT_CELL
    , APPEND_CELL_TO_ROW
    , CREATE_FETCHED_ROW
    , CREATE_UPDATED_ROW
    , CREATE_MUTATIONS
    , APPLY_PRED
    , GET_CLOSE
    , UPDATE_ROW
    , DELETE_ROW
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseAccessUMDTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessUMDTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 private:
  enum {
    NOT_STARTED
    , UMD_INIT
    , SETUP_SUBSET
    , PROCESS_SUBSET
    , NEXT_SUBSET
    , SETUP_UNIQUE
    , PROCESS_UNIQUE
    , NEXT_UNIQUE
    , SETUP_SUBSET_KEY
    , PROCESS_SUBSET_KEY
    , SETUP_UNIQUE_KEY
    , PROCESS_UNIQUE_KEY
    , UMD_CLOSE
    , UMD_CLOSE_NO_ERROR
    , HANDLE_ERROR
    , DONE
  } step_;

  ExHbaseUMDtrafSubsetTaskTcb * umdSQSubsetTaskTcb_;
  ExHbaseUMDtrafUniqueTaskTcb * umdSQUniqueTaskTcb_;

  ComTdbHbaseAccess::HbaseScanRows * hsr_;
  ComTdbHbaseAccess::HbaseGetRows * hgr_;

  enum 
  {
    UMD_SUBSET_TASK = 0,
    UMD_UNIQUE_TASK = 1,
    UMD_SUBSET_KEY_TASK = 2,
    UMD_UNIQUE_KEY_TASK = 3,
    UMD_MAX_TASKS = 4
  };
  NABoolean tasks_[UMD_MAX_TASKS];
};

class ExHbaseAccessSQRowsetTcb  : public ExHbaseAccessTcb
{
public:
  enum {
    NOT_STARTED
    , RS_INIT
    , SETUP_UMD
    , SETUP_SELECT
    , CREATE_UPDATED_ROW
    , PROCESS_DELETE_AND_CLOSE
    , EVAL_CONSTRAINT
    , PROCESS_UPDATE_AND_CLOSE
    , PROCESS_SELECT
    , NEXT_ROW
    , RS_CLOSE
    , HANDLE_ERROR
    , DONE
    , ALL_DONE
    , ROW_DONE
    , CREATE_ROW
    , APPLY_PRED
    , RETURN_ROW
    , COMPLETE_ASYNC_OPERATION
    , CLOSE_AND_DONE
  } step_;

  ExHbaseAccessSQRowsetTcb( const ExHbaseAccessTdb &tdb,
			    ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
  Lng32 setupRowIds();
  Lng32 setupUniqueKey();
 private:
  Lng32 currRowNum_;

  queue_index prevTailIndex_;
  queue_index nextRequest_;    // next request in down queue

  Lng32 numRetries_;
  int lastHandledStep_;
  Lng32 numRowsInVsbbBuffer_;
};

class ExHbaseAccessDDLTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessDDLTcb( const ExHbaseAccessTdb &tdb,
		       ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 

  short createHbaseTable(HbaseStr &table, 
		      const char * cf1, const char * cf2, const char * cf3);

  short dropMDtable(const char * name);

 private:
  enum {
    NOT_STARTED
    , CREATE_TABLE
    , DROP_TABLE
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseAccessInitMDTcb  : public ExHbaseAccessDDLTcb
{
public:
  ExHbaseAccessInitMDTcb( const ExHbaseAccessTdb &tdb,
			  ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 

 private:
  enum {
    NOT_STARTED
    , INIT_MD
    , UPDATE_MD
    , DROP_MD
    , HANDLE_ERROR
    , DONE
  } step_;

};

class ExHbaseAccessGetTablesTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessGetTablesTcb( const ExHbaseAccessTdb &tdb,
		       ex_globals *glob );
  
  virtual ExWorkProcRetcode work(); 
 private:
  enum {
    NOT_STARTED
    , GET_TABLE
    , RETURN_ROW
    , CLOSE
    , HANDLE_ERROR
    , DONE
  } step_;

};

/////////////////////////////////////////////////////////////////
// ExHbaseCoProcAggrTdb
/////////////////////////////////////////////////////////////////
class ExHbaseCoProcAggrTdb : public ComTdbHbaseCoProcAggr
{
public:

  // ---------------------------------------------------------------------
  // Constructor is only called to instantiate an object used for
  // retrieval of the virtual table function pointer of the class while
  // unpacking. An empty constructor is enough.
  // ---------------------------------------------------------------------
  ExHbaseCoProcAggrTdb()
  {}

  virtual ~ExHbaseCoProcAggrTdb()
  {}

  // ---------------------------------------------------------------------
  // Build a TCB for this TDB. Redefined in the Executor project.
  // ---------------------------------------------------------------------
  virtual ex_tcb *build(ex_globals *globals);

private:
  // ---------------------------------------------------------------------
  // !!!!!!! IMPORTANT -- NO DATA MEMBERS ALLOWED IN EXECUTOR TDB !!!!!!!!
  // *********************************************************************
  // The Executor TDB's are only used for the sole purpose of providing a
  // way to supplement the Compiler TDB's (in comexe) with methods whose
  // implementation depends on Executor objects. This is done so as to
  // decouple the Compiler from linking in Executor objects unnecessarily.
  //
  // When a Compiler generated TDB arrives at the Executor, the same data
  // image is "cast" as an Executor TDB after unpacking. Therefore, it is
  // a requirement that a Compiler TDB has the same object layout as its
  // corresponding Executor TDB. As a result of this, all Executor TDB's
  // must have absolutely NO data members, but only member functions. So,
  // if you reach here with an intention to add data members to a TDB, ask
  // yourself two questions:
  //
  // 1. Are those data members Compiler-generated?
  //    If yes, put them in the appropriate ComTdb subclass instead.
  //    If no, they should probably belong to someplace else (like TCB).
  // 
  // 2. Are the classes those data members belong defined in the executor
  //    project?
  //    If your answer to both questions is yes, you might need to move
  //    the classes to the comexe project.
  // ---------------------------------------------------------------------
};

class ExHbaseCoProcAggrTcb  : public ExHbaseAccessTcb
{
 private:
  enum {
    NOT_STARTED
    , COPROC_INIT
    , COPROC_EVAL
    , RETURN_ROW
    , HANDLE_ERROR
    , DONE
  } step_;

public:
  ExHbaseCoProcAggrTcb( const ComTdbHbaseCoProcAggr &tdb,
		    ex_globals *glob );

  //  virtual void registerSubtasks();  // register work procedures with scheduler

  virtual ExWorkProcRetcode work(); 

  inline ExHbaseCoProcAggrTdb &hbaseAccessTdb() const
    { return (ExHbaseCoProcAggrTdb &) tdb; }

 private:
  Lng32 aggrIdx_;
};


class ExHbaseAccessBulkLoadTaskTcb  : public ExHbaseAccessTcb
{
public:
  ExHbaseAccessBulkLoadTaskTcb( const ExHbaseAccessTdb &tdb,
                       ex_globals *glob );

  virtual ExWorkProcRetcode work();

 private:
  enum {
      NOT_STARTED
    , GET_NAME
    , LOAD_CLEANUP
    , COMPLETE_LOAD
    , LOAD_CLOSE
    , LOAD_CLOSE_AND_DONE
    , HANDLE_ERROR
    , HANDLE_ERROR_AND_CLOSE
    , DONE
  } step_;

  Text hBulkLoadPrepPath_;

};


#endif
