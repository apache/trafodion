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
// (C) Copyright 1998-2015 Hewlett-Packard Development Company, L.P.
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

#include <boost/lexical_cast.hpp>
#include <protocol/TBinaryProtocol.h>
#include <transport/TSocket.h>
#include <transport/TTransportUtils.h>

#include "Platform.h"
#include "Collections.h"
#include "NABasicObject.h"

#include "ExpHbaseDefs.h"

#include "HBaseClient_JNI.h"

#define INLINE_COLNAME_LEN 256

class ex_globals;
class CliGlobals;
class ExHbaseAccessStats;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace apache::hadoop::hbase::thrift;

namespace {

  typedef std::vector<std::string> StrVec;
  typedef std::map<std::string,std::string> StrMap;
  typedef std::vector<ColumnDescriptor> ColVec;
  typedef std::map<std::string,ColumnDescriptor> ColMap;
  typedef std::vector<TCell> CellVec;
  typedef std::map<std::string,TCell> CellMap;
  typedef std::vector<Text> ColNames;
}

// ===========================================================================
class ExpHbaseInterface : public NABasicObject
{
 public:

  static ExpHbaseInterface* newInstance(CollHeap* heap, 
                                        const char* server = NULL, 
                                        const char *zkPort = NULL, 
                                        int debugPort = 0, 
                                        int DebugTimeout = 0);

  virtual ~ExpHbaseInterface()
  {}
  
  virtual Lng32 init(ExHbaseAccessStats *hbs) = 0;
  
  virtual Lng32 cleanup() = 0;
  virtual Lng32 cleanupClient()
  { return 0;}

  virtual Lng32 close() = 0;

  virtual Lng32 create(HbaseStr &tblName,
	               HBASE_NAMELIST& colFamNameList) = 0;
  
  virtual Lng32 create(HbaseStr &tblName,
		       NAText * hbaseCreateOptionsArray,
                       int numSplits, int keyLength,
                       const char ** splitValues) =0;

  // During a drop of seabase table or index, the object is first removed from 
  // seabase metadata. If that succeeds, the corresponding hbase object is dropped.
  // if sync is TRUE, this drop of hbase table is done in another worker thread. 
  // That speeds up the over all drop time. 
  // If a create of the same table comes in later and an error is returned
  // during create, we delay and retry for a fixed number of times since that table
  // may still be dropped by the worked thread.
  virtual Lng32 drop(HbaseStr &tblName, NABoolean async) = 0;

  // drops all objects from hbase that match the pattern
  virtual Lng32 dropAll(const char * pattern, NABoolean async) = 0;

  // make a copy of currTableName as oldTblName.
  virtual Lng32 copy(HbaseStr &currTblName, HbaseStr &oldTblName);

  virtual Lng32 exists(HbaseStr &tblName) = 0;

  // returns the next tablename. 100, at EOD.
  virtual Lng32 getTable(HbaseStr &tblName) = 0;

  virtual Lng32 scanOpen(
			 HbaseStr &tblName,
			 const Text& startRow, 
			 const Text& stopRow, 
			 const std::vector<Text> & columns,
			 const int64_t timestamp,
			 const NABoolean readUncommitted,
			 const NABoolean cacheBlocks,
			 const Lng32 numCacheRows,
                         const NABoolean preFetch,
			 const TextVec *inColNamesToFilter, 
			 const TextVec *inCompareOpList,
			 const TextVec *inColValuesToCompare,
			 Float32 samplePercent = -1.0f,
			 NABoolean useSnapshotScan = FALSE,
			 Lng32 snapTimeout = 0,
			 char * snapName = NULL,
			 char * tmpLoc = NULL,
			 Lng32 espNum=0) = 0;

  virtual Lng32 scanClose() = 0;

  Lng32 fetchAllRows(
		     HbaseStr &tblName,
		     Lng32 numCols,
		     Text &col1NameStr,
		     Text &col2NameStr,
		     Text &col3NameStr,
		     NAList<Text> &col1ValueList, // output
		     NAList<Text> &col2ValueList, // output
		     NAList<Text> &col3ValueList); // output
		     
  virtual Lng32 getRowOpen(
		HbaseStr &tblName,
		const Text &row, 
		const std::vector<Text> & columns,
		const int64_t timestamp) = 0;

  // return 1 if row exists, 0 if does not exist. -ve num in case of error.
  virtual Lng32 rowExists(
			  HbaseStr &tblName,
			  HbaseStr &row) = 0;

 virtual Lng32 getRowsOpen(
		HbaseStr &tblName,
		const std::vector<Text> & rows, 
		const std::vector<Text> & columns,
		const int64_t timestamp) = 0;

  virtual Lng32 nextRow() = 0;
  
  virtual Lng32 nextCell(HbaseStr &rowId,
          HbaseStr &colFamName,
          HbaseStr &colName,
          HbaseStr &colVal,
          Int64 &timestamp) = 0;

  virtual Lng32 getColVal(int colNo, BYTE *colVal,
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal) = 0;

  virtual Lng32 getColVal(NAHeap *heap, int colNo, BYTE **colVal,
          Lng32 &colValLen) = 0;

  virtual Lng32 getColName(int colNo,
              char **outColName,
              short &colNameLen,
              Int64 &timestamp) = 0;
 
  virtual Lng32 getNumCols(int &numCols) = 0;
 
  virtual Lng32 getRowID(HbaseStr &rowID) = 0;
 
  virtual Lng32 deleteRow(
		  HbaseStr &tblName,
		  HbaseStr& row, 
		  const std::vector<Text> & columns,
		  NABoolean noXn,
		  const int64_t timestamp) = 0;



  virtual Lng32 deleteRows(
		  HbaseStr &tblName,
                  short rowIDLen,
		  HbaseStr &rowIDs,
		  NABoolean noXn,
		  const int64_t timestamp) = 0;


  virtual Lng32 checkAndDeleteRow(
				  HbaseStr &tblName,
				  HbaseStr& row, 
				  const Text& columnToCheck,
				  const Text& colValToCheck,
                                  NABoolean noXn,
				  const int64_t timestamp);



  virtual Lng32 deleteColumns(
		  HbaseStr &tblName,
		  const Text & column) = 0;

  virtual Lng32 insertRow(
		  HbaseStr &tblName,
		  HbaseStr& rowID, 
		  HbaseStr& row,
		  NABoolean noXn,
		  const int64_t timestamp) = 0;

 virtual Lng32 insertRows(
		  HbaseStr &tblName,
                  short rowIDLen,
                  HbaseStr &rowIDs,
                  HbaseStr &rows,
		  NABoolean noXn,
		  const int64_t timestamp,
		  NABoolean autoFlush = TRUE) = 0; // by default, flush rows after put
 
 virtual Lng32 setWriteBufferSize(
                 HbaseStr &tblName,
                 Lng32 size)=0;
 
 virtual Lng32 setWriteToWAL(
                 HbaseStr &tblName,
                 NABoolean v)=0;
 
 virtual  Lng32 initHBLC(ExHbaseAccessStats* hbs = NULL)=0;

 virtual Lng32 initHFileParams(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName,
                           Int64 maxHFileSize) = 0;

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
  virtual Lng32 checkAndInsertRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
                                  NABoolean noXn,
				  const int64_t timestamp) = 0;

  
  virtual Lng32 checkAndUpdateRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
				  const Text& columnToCheck,
				  const Text& colValToCheck,
                                  NABoolean noXn,				
                                 
				  const int64_t timestamp);

 
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

  virtual ByteArrayList* getRegionInfo(const char*) = 0;
  virtual Lng32 flushTable() = 0;
  static Lng32 flushAllTables();

  virtual Lng32 estimateRowCount(HbaseStr& tblName,
                                 Int32 partialRowSize,
                                 Int32 numCols,
                                 Int64& estRC) = 0;
  virtual Lng32 getLatestSnapshot(const char * tableName, char *& snapshotName, NAHeap * heap) = 0;
  virtual Lng32 cleanSnpTmpLocation( const char * path)=0;
  virtual Lng32 setArchivePermissions( const char * tbl)=0;

  virtual Lng32 getBlockCacheFraction(float& frac) = 0;

protected:
  enum 
    {
      MAX_SERVER_SIZE = 999,
      MAX_PORT_SIZE = 99
    };

  ExpHbaseInterface(CollHeap * heap,
                    const char * server = NULL,
                    const char * zkPort = NULL,
                    int debugPort = 0,
                    int debugTimeout = 0);
  
  CollHeap * heap_;
  ExHbaseAccessStats * hbs_;
  char server_[MAX_SERVER_SIZE+1];
  char zkPort_[MAX_PORT_SIZE+1];
  int  debugPort_;
  int  debugTimeout_;
};

char * getHbaseErrStr(Lng32 errEnum);

// ===========================================================================
class ExpHbaseInterface_JNI : public ExpHbaseInterface
{
 public:

  ExpHbaseInterface_JNI(CollHeap* heap,
                        const char* server, bool useTRex,
                        const char *zkPort, int debugPort, int debugTimeout);
  
  virtual ~ExpHbaseInterface_JNI();
  
  virtual Lng32 init(ExHbaseAccessStats *hbs);
  
  virtual Lng32 cleanup();

  virtual Lng32 cleanupClient();

  virtual Lng32 close();

  virtual Lng32 create(HbaseStr &tblName,
	               HBASE_NAMELIST& colFamNameList);

  virtual Lng32 create(HbaseStr &tblName,
	               NAText* hbaseCreateOptionsArray,
                       int numSplits, int keyLength,
                       const char ** splitValues);

  virtual Lng32 drop(HbaseStr &tblName, NABoolean async);
  virtual Lng32 dropAll(const char * pattern, NABoolean async);

  // make a copy of currTableName as oldTblName.
  virtual Lng32 copy(HbaseStr &currTblName, HbaseStr &oldTblName);

  // -1, if table exists. 0, if doesn't. -ve num, error.
  virtual Lng32 exists(HbaseStr &tblName);

  // returns the next tablename. 100, at EOD.
  virtual Lng32 getTable(HbaseStr &tblName);

  virtual Lng32 scanOpen(
			 HbaseStr &tblName,
			 const Text& startRow, 
			 const Text& stopRow, 
			 const std::vector<Text> & columns,
			 const int64_t timestamp,
			 const NABoolean readUncommitted,
			 const NABoolean cacheBlocks,
			 const Lng32 numCacheRows,
                         const NABoolean preFetch,
			 const TextVec *inColNamesToFilter, 
			 const TextVec *inCompareOpList,
			 const TextVec *inColValuesToCompare,
			 Float32 samplePercent = -1.0f,
			 NABoolean useSnapshotScan = FALSE,
			 Lng32 snapTimeout = 0,
			 char * snapName = NULL,
			 char * tmpLoc = NULL,
			 Lng32 espNum = 0);

  virtual Lng32 scanClose();

  virtual Lng32 getRowOpen(
		HbaseStr &tblName,
		const Text &row, 
		const std::vector<Text> & columns,
		const int64_t timestamp);
 
  // return 1 if row exists, 0 if does not exist. -ve num in case of error.
  virtual Lng32 rowExists(
			  HbaseStr &tblName,
			  HbaseStr &row);

 virtual Lng32 getRowsOpen(
		HbaseStr &tblName,
		const std::vector<Text> & rows, 
		const std::vector<Text> & columns,
		const int64_t timestamp);

  virtual Lng32 nextRow();

  virtual Lng32 nextCell( HbaseStr &rowId,
          HbaseStr &colFamName,
          HbaseStr &colName,
          HbaseStr &colVal,
          Int64 &timestamp);

  virtual Lng32 getColVal(int colNo, BYTE *colVal,
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal);

  virtual Lng32 getColVal(NAHeap *heap, int colNo, BYTE **colVal,
          Lng32 &colValLen);

  virtual Lng32 getColName(int colNo,
              char **outColName,
              short &colNameLen,
              Int64 &timestamp);

  virtual Lng32 getNumCols(int &numCols);

  virtual Lng32 getRowID(HbaseStr &rowID);

  virtual Lng32 deleteRow(
		  HbaseStr &tblName,
		  HbaseStr &row, 
		  const std::vector<Text> & columns,
		  NABoolean noXn,
		  const int64_t timestamp);


  virtual Lng32 deleteRows(
		  HbaseStr &tblName,
                  short rowIDLen,
		  HbaseStr &rowIDs,
		  NABoolean noXn,		 		  
		  const int64_t timestamp);


  virtual Lng32 checkAndDeleteRow(
				  HbaseStr &tblName,
				  HbaseStr& row, 
				  const Text& columnToCheck,
				  const Text& colValToCheck,
                                  NABoolean noXn,     
				  const int64_t timestamp);


  virtual Lng32 deleteColumns(
		  HbaseStr &tblName,
		  const Text & column);

  virtual Lng32 insertRow(
		  HbaseStr &tblName,
		  HbaseStr& rowID, 
                  HbaseStr& row,
		  NABoolean noXn,
		  const int64_t timestamp);

 virtual Lng32 insertRows(
		  HbaseStr &tblName,
                  short rowIDLen,
                  HbaseStr &rowIDs,
                  HbaseStr &rows,
		  NABoolean noXn,
		  const int64_t timestamp,
		  NABoolean autoFlush = TRUE); // by default, flush rows after put
  
  virtual Lng32 setWriteBufferSize(
                  HbaseStr &tblName,
                  Lng32 size);
  
  virtual Lng32 setWriteToWAL(
                  HbaseStr &tblName,
                  NABoolean v);

virtual  Lng32 initHBLC(ExHbaseAccessStats* hbs = NULL);

virtual Lng32 initHFileParams(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName,
                           Int64 maxHFileSize);
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

  virtual Lng32 checkAndInsertRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
                                  NABoolean noXn,
				  const int64_t timestamp);



  virtual Lng32 checkAndUpdateRow(
				  HbaseStr &tblName,
				  HbaseStr& rowID, 
				  HbaseStr& row,
				  const Text& columnToCheck,
				  const Text& colValToCheck,
                                  NABoolean noXn,			
				  const int64_t timestamp);



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


  virtual ByteArrayList* getRegionInfo(const char*);
  virtual Lng32 flushTable();
  virtual Lng32 estimateRowCount(HbaseStr& tblName,
                                 Int32 partialRowSize,
                                 Int32 numCols,
                                 Int64& estRC);

  virtual Lng32 getLatestSnapshot(const char * tableName, char *& snapshotName, NAHeap * heap);
  virtual Lng32 cleanSnpTmpLocation( const char * path);
  virtual Lng32  setArchivePermissions( const char * tabName) ;

  virtual Lng32 getBlockCacheFraction(float& frac) ;

private:
  bool  useTRex_;
  HBaseClient_JNI* client_;
  HTableClient_JNI* htc_;
  HBulkLoadClient_JNI* hblc_;
  Int32  retCode_;
  char* lastKVBuffer_;
  Int32 lastKVBufferSize_;
  char* lastKVColName_;
  Int32 lastKVColNameSize_;
};

#endif
