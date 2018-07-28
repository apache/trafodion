/* -*-C++-*-
****************************************************************************
*
* File:             ExpHbaseInterface.h
* Description:  Interface to Hbase world
* Created:        5/26/2013
* Language:      C++
*
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
*
****************************************************************************
*/

#ifndef EXP_HBASE_INTERFACE_H
#define EXP_HBASE_INTERFACE_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>

#include <iostream>

#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#include "Platform.h"
#include "Collections.h"
#include "NABasicObject.h"

#include "ExpHbaseDefs.h"

#include "HBaseClient_JNI.h"
#include "HiveClient_JNI.h"
#include "HiveClient_JNI.h"

#define INLINE_COLNAME_LEN 256

class ex_globals;
class CliGlobals;

Int64 getTransactionIDFromContext();


// ===========================================================================
class ExpHbaseInterface : public NABasicObject
{
 public:
  NAHeap *getHeap() { return (NAHeap*)heap_; }

  static ExpHbaseInterface* newInstance(CollHeap* heap, 
                                        const char* server = NULL, 
                                        const char *zkPort = NULL);

  virtual ~ExpHbaseInterface()
  {}
  
  virtual Lng32 init(ExHbaseAccessStats *hbs = NULL) = 0;
  
  virtual Lng32 cleanup() = 0;

  virtual Lng32 close() = 0;

  virtual Lng32 create(HbaseStr &tblName,
	               HBASE_NAMELIST& colFamNameList,
                       NABoolean isMVCC) = 0;
  
  virtual Lng32 create(HbaseStr &tblName,
		       NAText * hbaseCreateOptionsArray,
                       int numSplits, int keyLength,
                       const char ** splitValues,
                       NABoolean useHbaseXn,
                       NABoolean isMVCC) =0;

  virtual Lng32 alter(HbaseStr &tblName,
		      NAText * hbaseCreateOptionsArray,
                      NABoolean useHbaseXn) = 0;

  // During upsert using load, register truncate on abort will be used
  virtual Lng32 registerTruncateOnAbort(HbaseStr &tblName, 
                                        NABoolean useHbaseXn) = 0;

  // During a drop of seabase table or index, the object is first removed from 
  // seabase metadata. If that succeeds, the corresponding hbase object is dropped.
  // if sync is TRUE, this drop of hbase table is done in another worker thread. 
  // That speeds up the over all drop time. 
  // If a create of the same table comes in later and an error is returned
  // during create, we delay and retry for a fixed number of times since that table
  // may still be dropped by the worked thread.
  virtual Lng32 drop(HbaseStr &tblName, NABoolean async, NABoolean useHbaseXn) = 0;
  virtual Lng32 truncate(HbaseStr &tblName, NABoolean preserveSplits, NABoolean useHbaseXn) = 0;

  // drops all objects from hbase that match the pattern
  virtual Lng32 dropAll(const char * pattern, NABoolean async, NABoolean useHbaseXn) = 0;

  // retrieve all objects from hbase that match the pattern
  virtual NAArray<HbaseStr> *listAll(const char * pattern) = 0;

  // make a copy of srcTblName as tgtTblName
  // if force is true, remove target before copying.
  virtual Lng32 copy(HbaseStr &srcTblName, HbaseStr &tgtTblName,
                     NABoolean force = FALSE);

  virtual Lng32 exists(HbaseStr &tblName) = 0;

  // returns the next tablename. 100, at EOD.
  virtual Lng32 getTable(HbaseStr &tblName) = 0;

  virtual Lng32 scanOpen(
			 HbaseStr &tblName,
			 const Text& startRow, 
			 const Text& stopRow, 
			 const LIST(HbaseStr) & columns,
			 const int64_t timestamp,
			 const NABoolean readUncommitted,
			 const NABoolean cacheBlocks,
			 const NABoolean smallScanner,
			 const Lng32 numCacheRows,
             const NABoolean preFetch,
			 const LIST(NAString) *inColNamesToFilter, 
			 const LIST(NAString) *inCompareOpList,
			 const LIST(NAString) *inColValuesToCompare,
	         Float32 dopParallelScanner = 0.0f,
			 Float32 samplePercent = -1.0f,
			 NABoolean useSnapshotScan = FALSE,
			 Lng32 snapTimeout = 0,
			 char * snapName = NULL,
			 char * tmpLoc = NULL,
			 Lng32 espNum=0,
                         Lng32 versions = 0) = 0;

  virtual Lng32 scanClose() = 0;

  Lng32 fetchAllRows(
		     HbaseStr &tblName,
		     Lng32 numCols,
		     HbaseStr &col1NameStr,
		     HbaseStr &col2NameStr,
		     HbaseStr &col3NameStr,
		     LIST(NAString) &col1ValueList, // output
		     LIST(NAString) &col2ValueList, // output
		     LIST(NAString) &col3ValueList); // output

  // return 1 if table is empty, 0 if not empty. -ve num in case of error
  virtual Lng32 isEmpty(HbaseStr &tblName) = 0;
		     
  virtual Lng32 getRowOpen(
		HbaseStr &tblName,
		const HbaseStr &row, 
		const LIST(HbaseStr) & columns,
		const int64_t timestamp) = 0;

 virtual Lng32 getRowsOpen(
		HbaseStr &tblName,
		const LIST(HbaseStr) *rows, 
		const LIST(HbaseStr) & columns,
		const int64_t timestamp) = 0;

  virtual Lng32 nextRow() = 0;
  
  virtual Lng32 nextCell(HbaseStr &rowId,
          HbaseStr &colFamName,
          HbaseStr &colName,
          HbaseStr &colVal,
          Int64 &timestamp) = 0;

  virtual Lng32 completeAsyncOperation(Int32 timeout, NABoolean *resultArray, Int16 resultArrayLen) = 0;

  virtual Lng32 getColVal(int colNo, BYTE *colVal,
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal) = 0;

  virtual Lng32 getColVal(NAHeap *heap, int colNo, BYTE **colVal,
          Lng32 &colValLen) = 0;

  virtual Lng32 getColName(int colNo,
              char **outColName,
              short &colNameLen,
              Int64 &timestamp) = 0;
 
  virtual Lng32 getNumCellsPerRow(int &numCells) = 0;
 
  virtual Lng32 getRowID(HbaseStr &rowID) = 0;
 
  virtual Lng32 deleteRow(
		  HbaseStr tblName,
		  HbaseStr row, 
		  const LIST(HbaseStr) *columns,
		  NABoolean useHbaseXn,
                  NABoolean useRegionXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation) = 0;



  virtual Lng32 deleteRows(
		  HbaseStr tblName,
                  short rowIDLen,
		  HbaseStr rowIDs,
		  NABoolean useHbaseXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation) = 0;


  virtual Lng32 checkAndDeleteRow(
				  HbaseStr &tblName,
				  HbaseStr& row, 
				  HbaseStr& columnToCheck,
				  HbaseStr& colValToCheck,
                                  NABoolean useHbaseXn,
                                  NABoolean useRegionXn,
				  const int64_t timestamp) = 0;



  virtual Lng32 deleteColumns(
		  HbaseStr &tblName,
		  HbaseStr & column) = 0;

  virtual Lng32 insertRow(
		  HbaseStr tblName,
		  HbaseStr rowID, 
		  HbaseStr row,
		  NABoolean useHbaseXn,
                  NABoolean useRegionXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation) = 0;

 virtual Lng32 getRowsOpen(
          HbaseStr tblName,
          short rowIDLen,
          HbaseStr rowIDs,
          const LIST(HbaseStr) & columns) = 0;

 virtual Lng32 insertRows(
		  HbaseStr tblName,
                  short rowIDLen,
                  HbaseStr rowIDs,
                  HbaseStr rows,
		  NABoolean useHbaseXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation) = 0; 
 
 virtual Lng32 setWriteBufferSize(
                 HbaseStr &tblName,
                 Lng32 size)=0;
 
 virtual Lng32 setWriteToWAL(
                 HbaseStr &tblName,
                 NABoolean v)=0;
 
 virtual  Lng32 initHBLC(ExHbaseAccessStats* hbs = NULL)=0;
 virtual  Lng32 initHive() = 0;

 virtual Lng32 initHFileParams(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName,
                           Int64 maxHFileSize,
                           const char* sampleTblName,
                           const char* hiveDDL) = 0;

 virtual Lng32 addToHFile(short rowIDLen,
                          HbaseStr &rowIDs,
                          HbaseStr &rows) = 0;
 
 virtual Lng32 closeHFile(HbaseStr &tblName) = 0;

 virtual Lng32 doBulkLoad(HbaseStr &tblName,
                          Text& location,
                          Text& tableName,
                          NABoolean quasiSecure,
                          NABoolean snapshot) = 0;

 virtual Lng32 bulkLoadCleanup(HbaseStr &tblName,
                          Text& location) = 0;

 virtual Lng32  incrCounter( const char * tabName, const char * rowId,
                             const char * famName, const char * qualName ,
                             Int64 incr, Int64 & count)=0;
 virtual Lng32  createCounterTable( const char * tabName,
                                   const char * famName)=0;
  virtual Lng32 checkAndInsertRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
                                  NABoolean useHbaseXn,
                                  NABoolean useRegionXn,
				  const int64_t timestamp,
                                  NABoolean asyncOperation,
				  Int16 colIndexToCheck) = 0;

  
  virtual Lng32 checkAndUpdateRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
				  HbaseStr& columnToCheck,
				  HbaseStr& colValToCheck,
                                  NABoolean useHbaseXn,
                                  NABoolean useRegionXn,
				  const int64_t timestamp,
                                  NABoolean asyncOperation) = 0;

 
  virtual Lng32 getClose() = 0;

  virtual Lng32 coProcAggr(
			 HbaseStr &tblName,
			 Lng32 aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
			 const Text& startRow, 
			 const Text& stopRow, 
			 const Text &colFamily,
			 const Text &colName,
			 const NABoolean cacheBlocks,
			 const Lng32 numCacheRows,
			 Text &aggrVal); // returned value
 
  virtual Lng32 grant(
		      const Text& user, 
		      const Text& tblName,
		      const std::vector<Text> & actionCodes) = 0;

  virtual Lng32 revoke(
		      const Text& user, 
		      const Text& tblName,
		      const std::vector<Text> & actionCodes) = 0;

  virtual NAArray<HbaseStr>* getRegionBeginKeys(const char*) = 0;
  virtual NAArray<HbaseStr>* getRegionEndKeys(const char*) = 0;

  virtual Lng32 estimateRowCount(HbaseStr& tblName,
                                 Int32 partialRowSize,
                                 Int32 numCols,
                                 Int32 retryLimitMilliSeconds,
                                 NABoolean useCoprocessor,
                                 Int64& estRC,
                                 Int32& breadCrumb) = 0;
  virtual Lng32 getLatestSnapshot(const char * tableName, char *& snapshotName, NAHeap * heap) = 0;
  virtual Lng32 cleanSnpTmpLocation( const char * path)=0;
  virtual Lng32 setArchivePermissions( const char * tbl)=0;

  virtual Lng32 getBlockCacheFraction(float& frac) = 0;
  virtual Lng32 getHbaseTableInfo(const HbaseStr& tblName,
                                  Int32& indexLevels,
                                  Int32& blockSize) = 0;

  virtual Lng32 getRegionsNodeName(const HbaseStr& tblName,
                                   Int32 partns,
                                   ARRAY(const char *)& nodeNames) = 0;
  // get regions and size
  virtual NAArray<HbaseStr> *getRegionStats(const HbaseStr& tblName) = 0;

  virtual NAArray<HbaseStr> *getClusterStats(Int32 &numEntries) = 0;

  // Snapshots
  virtual Lng32 createSnapshot( const NAString&  tableName, const NAString&  snapshotName) = 0;
  virtual Lng32 deleteSnapshot( const NAString&  snapshotName) = 0;
  virtual Lng32 verifySnapshot( const NAString&  tableName, const NAString&  snapshotName,
                                                NABoolean & exist) = 0;

protected:
  enum 
    {
      MAX_SERVER_SIZE = 999,
      MAX_PORT_SIZE = 99
    };

  ExpHbaseInterface(CollHeap * heap,
                    const char * server = NULL,
                    const char * zkPort = NULL);
  
  CollHeap * heap_;
  ExHbaseAccessStats * hbs_;
  char server_[MAX_SERVER_SIZE+1];
  char zkPort_[MAX_PORT_SIZE+1];
};

char * getHbaseErrStr(Lng32 errEnum);

// ===========================================================================
class ExpHbaseInterface_JNI : public ExpHbaseInterface
{
 public:

  ExpHbaseInterface_JNI(CollHeap* heap,
                        const char* server, bool useTRex,
                        const char *zkPort);
  
  virtual ~ExpHbaseInterface_JNI();
  
  virtual Lng32 init(ExHbaseAccessStats *hbs = NULL);
  
  virtual Lng32 cleanup();

  virtual Lng32 close();

  virtual Lng32 create(HbaseStr &tblName,
	               HBASE_NAMELIST& colFamNameList,
                       NABoolean isMVCC);

  virtual Lng32 create(HbaseStr &tblName,
	               NAText* hbaseCreateOptionsArray,
                       int numSplits, int keyLength,
                       const char ** splitValues,
                       NABoolean useHbaseXn,
                       NABoolean isMVCC);

  virtual Lng32 alter(HbaseStr &tblName,
		      NAText * hbaseCreateOptionsArray,
                      NABoolean useHbaseXn);

  virtual Lng32 registerTruncateOnAbort(HbaseStr &tblName, NABoolean useHbaseXn);
  virtual Lng32 truncate(HbaseStr &tblName, NABoolean preserveSplits, NABoolean useHbaseXn);
  virtual Lng32 drop(HbaseStr &tblName, NABoolean async, NABoolean useHbaseXn);
  virtual Lng32 dropAll(const char * pattern, NABoolean async, NABoolean useHbaseXn);

  virtual NAArray<HbaseStr>* listAll(const char * pattern);

  // make a copy of srcTblName as tgtTblName
  // if force is true, remove target before copying.
  virtual Lng32 copy(HbaseStr &srcTblName, HbaseStr &tgtTblName,
                     NABoolean force = FALSE);

  // -1, if table exists. 0, if doesn't. -ve num, error.
  virtual Lng32 exists(HbaseStr &tblName);

  // returns the next tablename. 100, at EOD.
  virtual Lng32 getTable(HbaseStr &tblName);

  virtual Lng32 scanOpen(
			 HbaseStr &tblName,
			 const Text& startRow, 
			 const Text& stopRow, 
			 const LIST(HbaseStr) & columns,
			 const int64_t timestamp,
			 const NABoolean readUncommitted,
			 const NABoolean cacheBlocks,
			 const NABoolean smallScanner,
			 const Lng32 numCacheRows,
             const NABoolean preFetch,
			 const LIST(NAString) *inColNamesToFilter, 
			 const LIST(NAString) *inCompareOpList,
			 const LIST(NAString) *inColValuesToCompare,
	         Float32 DOPparallelScanner = 0.0f,
			 Float32 samplePercent = -1.0f,
			 NABoolean useSnapshotScan = FALSE,
			 Lng32 snapTimeout = 0,
			 char * snapName = NULL,
			 char * tmpLoc = NULL,
			 Lng32 espNum = 0,
                         Lng32 versions = 0);

  virtual Lng32 scanClose();

  // return 1 if table is empty, 0 if not empty. -ve num in case of error
  virtual Lng32 isEmpty(HbaseStr &tblName);

  virtual Lng32 getRowOpen(
		HbaseStr &tblName,
		const HbaseStr &row, 
		const LIST(HbaseStr) & columns,
		const int64_t timestamp);
 
 virtual Lng32 getRowsOpen(
		HbaseStr &tblName,
		const LIST(HbaseStr) *rows, 
		const LIST(HbaseStr) & columns,
		const int64_t timestamp);

  virtual Lng32 nextRow();

  virtual Lng32 nextCell( HbaseStr &rowId,
          HbaseStr &colFamName,
          HbaseStr &colName,
          HbaseStr &colVal,
          Int64 &timestamp);
 
  virtual Lng32 completeAsyncOperation(Int32 timeout, NABoolean *resultArray, Int16 resultArrayLen);

  virtual Lng32 getColVal(int colNo, BYTE *colVal,
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal);

  virtual Lng32 getColVal(NAHeap *heap, int colNo, BYTE **colVal,
          Lng32 &colValLen);

  virtual Lng32 getColName(int colNo,
              char **outColName,
              short &colNameLen,
              Int64 &timestamp);

  virtual Lng32 getNumCellsPerRow(int &numCells);

  virtual Lng32 getRowID(HbaseStr &rowID);

  virtual Lng32 deleteRow(
		  HbaseStr tblName,
		  HbaseStr row, 
		  const LIST(HbaseStr) *columns,
		  NABoolean useHbaseXn,
                  NABoolean useRegionXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation);


  virtual Lng32 deleteRows(
		  HbaseStr tblName,
                  short rowIDLen,
		  HbaseStr rowIDs,
		  NABoolean useHbaseXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation);


  virtual Lng32 checkAndDeleteRow(
				  HbaseStr &tblName,
				  HbaseStr& row, 
				  HbaseStr& columnToCheck,
				  HbaseStr& colValToCheck,
                                  NABoolean useHbaseXn,     
                                  NABoolean useRegionXn,
				  const int64_t timestamp);


  virtual Lng32 deleteColumns(
		  HbaseStr &tblName,
		  HbaseStr & column);

  virtual Lng32 insertRow(
		  HbaseStr tblName,
		  HbaseStr rowID, 
                  HbaseStr row,
		  NABoolean useHbaseXn,
                  NABoolean useRegionXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation);

 virtual Lng32 getRowsOpen(
          HbaseStr tblName,
          short rowIDLen,
          HbaseStr rowIDs,
          const LIST(HbaseStr) & columns);

 virtual Lng32 insertRows(
		  HbaseStr tblName,
                  short rowIDLen,
                  HbaseStr rowIDs,
                  HbaseStr rows,
		  NABoolean useHbaseXn,
		  const int64_t timestamp,
                  NABoolean asyncOperation); 
  
  virtual Lng32 setWriteBufferSize(
                  HbaseStr &tblName,
                  Lng32 size);
  
  virtual Lng32 setWriteToWAL(
                  HbaseStr &tblName,
                  NABoolean v);

virtual  Lng32 initHBLC(ExHbaseAccessStats* hbs = NULL);
virtual  Lng32 initHive();

virtual Lng32 initHFileParams(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName,
                           Int64 maxHFileSize,
                           const char* sampleTblName,
                           const char* hiveDDL);
 virtual Lng32 addToHFile(short rowIDLen,
                          HbaseStr &rowIDs,
                          HbaseStr &rows);

 virtual Lng32 closeHFile(HbaseStr &tblName);

 virtual Lng32 doBulkLoad(HbaseStr &tblName,
                          Text& location,
                          Text& tableName,
                          NABoolean quasiSecure,
                          NABoolean snapshot);
 
 virtual Lng32 bulkLoadCleanup(HbaseStr &tblName,
                          Text& location);
 virtual Lng32  incrCounter( const char * tabName, const char * rowId,
                             const char * famName, const char * qualName ,
                             Int64 incr, Int64 & count);
 virtual Lng32  createCounterTable( const char * tabName,
                                   const char * famName);

  virtual Lng32 checkAndInsertRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
                                  NABoolean useHbaseXn,
                                  NABoolean useRegionXn,
				  const int64_t timestamp,
                  		  NABoolean asyncOperation,
				  Int16 colIndexToCheck);



  virtual Lng32 checkAndUpdateRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
				  HbaseStr& columnToCheck,
				  HbaseStr& colValToCheck,
                                  NABoolean useHbaseXn,			
                                  NABoolean useRegionXn,
				  const int64_t timestamp,
                                  NABoolean asyncOperation);



  virtual Lng32 coProcAggr(
			 HbaseStr &tblName,
			 Lng32 aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
			 const Text& startRow, 
			 const Text& stopRow, 
			 const Text &colFamily,
			 const Text &colName,
			 const NABoolean cacheBlocks,
			 const Lng32 numCacheRows,
			 Text &aggrVal); // returned value
 
  virtual Lng32 getClose();

  virtual Lng32 grant(
		      const Text& user, 
		      const Text& tblName,
  		      const std::vector<Text> & actionCodes);

  virtual Lng32 revoke(
		      const Text& user, 
		      const Text& tblName,
  		      const std::vector<Text> & actionCodes);

  virtual NAArray<HbaseStr>* getRegionBeginKeys(const char*);
  virtual NAArray<HbaseStr>* getRegionEndKeys(const char*);

  virtual Lng32 estimateRowCount(HbaseStr& tblName,
                                 Int32 partialRowSize,
                                 Int32 numCols,
                                 Int32 retryLimitMilliSeconds,
                                 NABoolean useCoprocessor,
                                 Int64& estRC,
                                 Int32& breadCrumb);

  virtual Lng32 getLatestSnapshot(const char * tableName, char *& snapshotName, NAHeap * heap);
  virtual Lng32 cleanSnpTmpLocation( const char * path);
  virtual Lng32  setArchivePermissions( const char * tabName) ;

  virtual Lng32 getBlockCacheFraction(float& frac) ;
  virtual Lng32 getHbaseTableInfo(const HbaseStr& tblName,
                                  Int32& indexLevels,
                                  Int32& blockSize) ;
  virtual Lng32 getRegionsNodeName(const HbaseStr& tblName,
                                   Int32 partns,
                                   ARRAY(const char *)& nodeNames) ;

  virtual NAArray<HbaseStr>* getRegionStats(const HbaseStr& tblName);
  virtual NAArray<HbaseStr>* getClusterStats(Int32 &numEntries);

  virtual Lng32 createSnapshot( const NAString&  tableName, const NAString&  snapshotName);
  virtual Lng32 deleteSnapshot( const NAString&  snapshotName);
  virtual Lng32 verifySnapshot( const NAString&  tableName, const NAString&  snapshotName,
                                                NABoolean & exist);

private:
  bool  useTRex_;
  HBaseClient_JNI* client_;
  HTableClient_JNI* htc_;
  HBulkLoadClient_JNI* hblc_;
  HiveClient_JNI* hive_;
  HTableClient_JNI *asyncHtc_;
  Int32  retCode_;
};

#endif
