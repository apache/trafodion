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

#include "ex_stdh.h"
#include "ExpHbaseInterface.h"
#include "str.h"
#include "NAStringDef.h"
#include "ex_ex.h"
#include "ExStats.h"
#include "Globals.h"
#include "SqlStats.h"
#include "CmpCommon.h"
#include "CmpContext.h"

extern Int64 getTransactionIDFromContext();

// ===========================================================================
// ===== Class ExpHbaseInterface
// ===========================================================================

ExpHbaseInterface::ExpHbaseInterface(CollHeap * heap,
                                     const char * server,
                                     const char * zkPort,
                                     int debugPort,
                                     int debugTimeout)
{
  heap_ = heap;
  hbs_ = NULL;

  if ((server) &&
      (strlen(server) <= MAX_SERVER_SIZE))
    strcpy(server_, server);
  else
    server_[0] = 0;

  if ((zkPort) &&
      (strlen(zkPort) <= MAX_PORT_SIZE))
    strcpy(zkPort_, zkPort);
  else
    zkPort_[0] = 0;

  debugPort_ = debugPort;
  debugTimeout_ = debugTimeout;
}

ExpHbaseInterface* ExpHbaseInterface::newInstance(CollHeap* heap,
                                                  const char* server,
                                                  const char *zkPort,
                                                  int debugPort,
                                                  int debugTimeout)
{
  return new (heap) ExpHbaseInterface_JNI(heap, server, TRUE, zkPort,
                                          debugPort, debugTimeout); // This is the transactional interface
}

NABoolean isParentQueryCanceled()
{
  NABoolean isCanceled = FALSE;
  CliGlobals *cliGlobals = GetCliGlobals();
  StatsGlobals *statsGlobals = cliGlobals->getStatsGlobals();
  const char *parentQid = CmpCommon::context()->sqlSession()->getParentQid();
  if (statsGlobals && parentQid)
  {
    short savedPriority, savedStopMode;
    statsGlobals->getStatsSemaphore(cliGlobals->getSemId(),
      cliGlobals->myPin(), savedPriority, savedStopMode,
      FALSE /*shouldTimeout*/);
    StmtStats *ss = statsGlobals->getMasterStmtStats(parentQid, 
      strlen(parentQid), RtsQueryId::ANY_QUERY_);
    if (ss)
    {
      ExMasterStats *masterStats = ss->getMasterStats();
      if (masterStats && masterStats->getCanceledTime() != -1)
        isCanceled = TRUE;
    }
    statsGlobals->releaseStatsSemaphore(cliGlobals->getSemId(),
       cliGlobals->myPin(), savedPriority, savedStopMode);
  }
  return isCanceled;
}

Int32 ExpHbaseInterface_JNI::deleteColumns(
	     HbaseStr &tblName,
	     HbaseStr& column)
{
  Int32 retcode = 0;

  LIST(HbaseStr) columns(heap_);
  columns.insert(column);
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  Int64 transID = getTransactionIDFromContext();

  int numReqRows = 100;
  retcode = htc_->startScan(transID, "", "", columns, -1, FALSE, numReqRows, FALSE, 
       NULL, NULL, NULL, NULL);
  if (retcode != HTC_OK)
    return retcode;

  NABoolean done = FALSE;
  HbaseStr rowID;
  do {
     // Added the for loop to consider using deleteRows
     // to delete the column for all rows in the batch 
     for (int rowNo = 0; rowNo < numReqRows; rowNo++)
     {
         retcode = htc_->nextRow();
         if (retcode != HTC_OK)
         {
            done = TRUE;
	    break;
         }
         retcode = htc_->getRowID(rowID);
         if (retcode != HBASE_ACCESS_SUCCESS)
         {
            done = TRUE; 
            break;
         }
         retcode = htc_->deleteRow(transID, rowID, columns, -1);
         if (retcode != HTC_OK) 
         {
            done = TRUE;
            break;
	 }
     }
  } while (!(done || isParentQueryCanceled()));
  scanClose();
  if (retcode == HTC_DONE)
     return HBASE_ACCESS_SUCCESS;
  return HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface::checkAndUpdateRow(
					   HbaseStr &tblName,
					   HbaseStr &rowID, 
					   HbaseStr &row,
					   const Text& columnToCheck,
					   const Text& colValToCheck,
                                           NABoolean noXn,
					   const int64_t timestamp)
{
  Lng32 retcode = 0;

  retcode = rowExists(tblName, rowID);
  if (retcode < 0)
    return retcode;

  if (retcode == 0) // row does not exist
    {
      // return warning
      return HBASE_ROW_NOTFOUND_ERROR;
    }

  // if exists, update it
  retcode = insertRow(tblName,
		      rowID,
		      row,
		      FALSE,
		      timestamp);
  
  return retcode;
}

Lng32 ExpHbaseInterface::checkAndDeleteRow(
					   HbaseStr &tblName,
					   HbaseStr &rowID, 
					   const Text& columnToCheck,
					   const Text& colValToCheck,
                                           NABoolean noXn,
					   const int64_t timestamp)
{
  Lng32 retcode = 0;

  retcode = rowExists(tblName, rowID);
  if (retcode < 0)
    return retcode;

  if (retcode == 0) // row does not exist
    {
      // return warning
      return HBASE_ROW_NOTFOUND_ERROR;
    }

  LIST(HbaseStr) columns(heap_);
  // row exists, delete it
  retcode = deleteRow(tblName,
		      rowID,
		      columns,
                      noXn,
		      timestamp);

  
  return retcode;
}

Lng32  ExpHbaseInterface::fetchAllRows(
				       HbaseStr &tblName,
				       Lng32 numInCols,
				       HbaseStr &col1NameStr,
				       HbaseStr &col2NameStr,
				       HbaseStr &col3NameStr,
				       LIST(NAString) &col1ValueList, // output
				       LIST(NAString) &col2ValueList, // output
				       LIST(NAString) &col3ValueList) // output
{
  Lng32 retcode;

  retcode = init(hbs_);
  if (retcode != HBASE_ACCESS_SUCCESS)
    return retcode;

  Int32 colValLen;
  char *colName;
  short colNameLen;
  Int64 timestamp;
  LIST(HbaseStr) columns(heap_);

  switch (numInCols)
  {
     case 1:
        columns.insert(col1NameStr);  // copy to new element in the list
        col1ValueList.clear();
        break;
     case 2:
        columns.insert(col1NameStr);  // copy to new element in the list
        columns.insert(col2NameStr);  // copy to new element in the list
        col1ValueList.clear();
        col2ValueList.clear();
        break;
     case 3:
        columns.insert(col1NameStr);  // copy to new element in the list
        columns.insert(col2NameStr);  // copy to new element in the list
        columns.insert(col3NameStr);  // copy to new element in the list
        col1ValueList.clear();
        col2ValueList.clear();
        col3ValueList.clear();
        break;
  }

  retcode = scanOpen(tblName, "", "", columns, -1, FALSE, FALSE, 100, TRUE, NULL, 
       NULL, NULL, NULL);
  if (retcode != HBASE_ACCESS_SUCCESS)
    return retcode;
  while (retcode == HBASE_ACCESS_SUCCESS)
  {
     retcode = nextRow();
     if (retcode != HBASE_ACCESS_SUCCESS)
        break;
     int numCols;
     retcode = getNumCols(numCols);
     if (retcode == HBASE_ACCESS_SUCCESS)
     {	
        for (int colNo = 0; colNo < numCols; colNo++)
        {
           retcode = getColName(colNo, &colName, colNameLen, timestamp);
           if (retcode != HBASE_ACCESS_SUCCESS)
              break;
           BYTE *colVal = NULL;
           colValLen = 0;
           retcode = getColVal((NAHeap *)heap_, colNo, &colVal, colValLen);
           if (retcode != HBASE_ACCESS_SUCCESS) 
              break; 
           NAString colValue((char *)colVal, colValLen);
           NADELETEBASIC(colVal, heap_);
	   if (colNameLen == col1NameStr.len &&
	       memcmp(colName, col1NameStr.val, col1NameStr.len) == 0)
	      col1ValueList.insert(colValue);
	   else if (colNameLen == col2NameStr.len &&
                    memcmp(colName, col2NameStr.val, col2NameStr.len) == 0)
	      col2ValueList.insert(colValue);
	   else if (colNameLen == col3NameStr.len &&
                    memcmp(colName, col3NameStr.val, col3NameStr.len) == 0)
	      col3ValueList.insert(colValue);
        }
     }
  } // while
  scanClose();
  if (retcode == HBASE_ACCESS_EOD)
     retcode = HBASE_ACCESS_SUCCESS;
  return retcode;
}

Lng32 ExpHbaseInterface::copy(HbaseStr &currTblName, HbaseStr &oldTblName)
{
  return -HBASE_COPY_ERROR;
}

Lng32 ExpHbaseInterface::coProcAggr(
				    HbaseStr &tblName,
				    Lng32 aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
				    const Text& startRow, 
				    const Text& stopRow, 
				    const Text &colFamily,
				    const Text &colName,
				    const NABoolean cacheBlocks,
				    const Lng32 numCacheRows,
				    Text &aggrVal) // returned value
{
  return -HBASE_OPEN_ERROR;
}

Lng32 ExpHbaseInterface_JNI::flushTable()
{
  HTC_RetCode retCode = HTC_OK;
  if (htc_ != NULL)
     retCode = htc_->flushTable();

  if (retCode != HTC_OK)
    return HBASE_ACCESS_ERROR;
  
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface::flushAllTables()
{
  HBC_RetCode retCode = HBaseClient_JNI::flushAllTablesStatic();

  if (retCode != HBC_OK)
    return HBASE_ACCESS_ERROR;
  
  return HBASE_ACCESS_SUCCESS;
}

char * getHbaseErrStr(Lng32 errEnum)
{
  return (char*)hbaseErrorEnumStr[errEnum - (Lng32)HBASE_MIN_ERROR_NUM];
}


// ===========================================================================
// ===== Class ExpHbaseInterface_JNI
// ===========================================================================

ExpHbaseInterface_JNI::ExpHbaseInterface_JNI(CollHeap* heap, const char* server, bool useTRex,
                                             const char *zkPort, int debugPort, int debugTimeout)
     : ExpHbaseInterface(heap, server, zkPort, debugPort, debugTimeout)
   ,useTRex_(useTRex)
   ,client_(NULL)
   ,htc_(NULL)
   ,hblc_(NULL)
   ,hive_(NULL)
   ,retCode_(HBC_OK)
{
//  HBaseClient_JNI::logIt("ExpHbaseInterface_JNI::constructor() called.");
}

//----------------------------------------------------------------------------
ExpHbaseInterface_JNI::~ExpHbaseInterface_JNI()
{
//  HBaseClient_JNI::logIt("ExpHbaseInterface_JNI::destructor() called.");
  if (client_)
  {
    if (htc_ != NULL)
    {
      // Return open table object to the pool for reuse.
      client_->releaseHTableClient(htc_);
      htc_ = NULL;
    }
    
    if (hblc_ !=NULL)
      hblc_ = NULL;

    client_ = NULL;
  }
}
  
//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::init(ExHbaseAccessStats *hbs)
{
  //HBaseClient_JNI::logIt("ExpHbaseInterface_JNI::init() called.");
  // Cannot use statement heap - objects persist accross statements.
  if (client_ == NULL)
  {
    HBaseClient_JNI::logIt("ExpHbaseInterface_JNI::init() creating new client.");
    client_ = HBaseClient_JNI::getInstance(debugPort_, debugTimeout_);
    
    if (client_->isInitialized() == FALSE)
    {
      HBC_RetCode retCode = client_->init();
      if (retCode != HBC_OK)
        return -HBASE_ACCESS_ERROR;
    }
    
    if (client_->isConnected() == FALSE)
    {
      HBC_RetCode retCode = client_->initConnection(server_,
                                                    zkPort_);
      if (retCode != HBC_OK)
        return -HBASE_ACCESS_ERROR;
    }
  }
  
  hbs_ = hbs;  // save for ExpHbaseInterface_JNI member function use
               // and eventually give to HTableClient_JNI

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::initHive()
{
  if (hive_ == NULL)
  {
    hive_ = HiveClient_JNI::getInstance();
    
    if (hive_->isInitialized() == FALSE)
    {
      HVC_RetCode retCode = hive_->init();
      if (retCode != HVC_OK)
        return -HBASE_ACCESS_ERROR;
    }
  }
  return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::cleanup()
{
  //  HBaseClient_JNI::logIt("ExpHbaseInterface_JNI::cleanup() called.");
  if (client_)
  {
    // Return client object to the pool for reuse.    
    if (htc_)
    {
      client_->releaseHTableClient(htc_);
      htc_ = NULL;    
    }

    if (hblc_)
    {
      client_->releaseHBulkLoadClient(hblc_);
      hblc_ = NULL;
    }

  }
  return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::cleanupClient()
{
  if (client_)
  {
    client_->cleanup();
  }
  return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::close()
{
//  HBaseClient_JNI::logIt("ExpHbaseInterface_JNI::close() called.");
  if (client_ == NULL)
    return HBASE_ACCESS_SUCCESS;
  return cleanup();
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::create(HbaseStr &tblName,
				    HBASE_NAMELIST& colFamNameList,
                                    NABoolean isMVCC)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->create(tblName.val, colFamNameList, isMVCC); 
  //close();
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_CREATE_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::create(HbaseStr &tblName,
				    NAText * hbaseCreateOptionsArray,
                                    int numSplits, int keyLength,
                                    const char ** splitValues,
                                    NABoolean noXn,
                                    NABoolean isMVCC)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
  
  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();
 
  retCode_ = client_->create(tblName.val, hbaseCreateOptionsArray,
                             numSplits, keyLength, splitValues, transID,
                             isMVCC);
  //close();
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_CREATE_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::alter(HbaseStr &tblName,
				   NAText * hbaseCreateOptionsArray,
                                   NABoolean noXn)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
  
  Int64 transID = 0;
  if (!noXn)
    transID = getTransactionIDFromContext();
 
  retCode_ = client_->alter(tblName.val, hbaseCreateOptionsArray, transID);

  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_ALTER_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::registerTruncateOnAbort(HbaseStr &tblName, NABoolean noXn)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }

  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();

  retCode_ = client_->registerTruncateOnAbort(tblName.val, transID);

  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_DROP_ERROR;
}


//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::drop(HbaseStr &tblName, NABoolean async, NABoolean noXn)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
   
  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();

  retCode_ = client_->drop(tblName.val, async, transID);

  //close();
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_DROP_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::dropAll(const char * pattern, NABoolean async)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->dropAll(pattern, async);

  //close();
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_DROP_ERROR;
}

//----------------------------------------------------------------------------
ByteArrayList* ExpHbaseInterface_JNI::listAll(const char * pattern)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return NULL;
  }
    
  ByteArrayList* bal = client_->listAll(pattern);
  if (bal == NULL)
    return NULL;

  return bal;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::copy(HbaseStr &currTblName, HbaseStr &oldTblName)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->copy(currTblName.val, oldTblName.val);

  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_COPY_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::exists(HbaseStr &tblName)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->exists(tblName.val); 
  //close();
  if (retCode_ == HBC_OK)
    return -1;   // Found.
  else if (retCode_ == HBC_DONE)
    return 0;  // Not found
  else
    return -HBASE_ACCESS_ERROR;
}

//----------------------------------------------------------------------------
// returns the next tablename. 100, at EOD.
Lng32 ExpHbaseInterface_JNI::getTable(HbaseStr &tblName)
{
  return -HBASE_ACCESS_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::scanOpen(
				      HbaseStr &tblName,
				      const Text& startRow, 
				      const Text& stopRow, 
				      const LIST(HbaseStr) & columns,
				      const int64_t timestamp,
				      const NABoolean noXn,
				      const NABoolean cacheBlocks,
				      const Lng32 numCacheRows,
                                      const NABoolean preFetch,
				      const LIST(NAString) *inColNamesToFilter,
				      const LIST(NAString) *inCompareOpList,
				      const LIST(NAString) *inColValuesToCompare,
				      Float32 samplePercent,
				      NABoolean useSnapshotScan,
				      Lng32 snapTimeout,
				      char * snapName,
				      char * tmpLoc,
				      Lng32 espNum  )
{
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }

  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();
  retCode_ = htc_->startScan(transID, startRow, stopRow, columns, timestamp, 
					  cacheBlocks, numCacheRows, 
                                          preFetch,
					  inColNamesToFilter,
					  inCompareOpList,
					  inColValuesToCompare,
					  samplePercent,
					  useSnapshotScan,
					  snapTimeout,
					  snapName,
					  tmpLoc,
					  espNum);
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_OPEN_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::scanClose()
{
  if (htc_)
  {
    client_->releaseHTableClient(htc_);
    htc_ = NULL;
  }
    
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::getHTable(HbaseStr &tblName)
{
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc_ == NULL) {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getRowOpen(
	HbaseStr &tblName,
	const HbaseStr &row, 
	const LIST(HbaseStr) & columns,
	const int64_t timestamp)
{
  Int64 transID = getTransactionIDFromContext();
  htc_ = client_->startGet((NAHeap *)heap_, (char *)tblName.val, useTRex_, hbs_, 
                       transID, row, columns, timestamp);
  if (htc_ == NULL) {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  return HBASE_ACCESS_SUCCESS;
}


//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getRowsOpen(
	HbaseStr &tblName,
	const LIST(HbaseStr) & rows, 
	const LIST(HbaseStr) & columns,
	const int64_t timestamp)
{
  Int64 transID = getTransactionIDFromContext();
  htc_ = client_->startGets((NAHeap *)heap_, (char *)tblName.val, useTRex_, hbs_, 
                       transID, rows, columns, timestamp);
  if (htc_ == NULL) {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::deleteRow(
	  HbaseStr &tblName,
	  HbaseStr& row, 
	  const LIST(HbaseStr) & columns,
	  NABoolean noXn,
	  const int64_t timestamp)

{
  HTableClient_JNI *htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  


  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();
  retCode_ = htc->deleteRow(transID, row, columns, timestamp);



  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}
//
//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::deleteRows(
	  HbaseStr &tblName,
          short rowIDLen,
	  HbaseStr &rowIDs,
	  NABoolean noXn,
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  

  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();

 
  retCode_ = htc->deleteRows(transID, rowIDLen, rowIDs, timestamp);



  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::checkAndDeleteRow(
	  HbaseStr &tblName,
	  HbaseStr& row, 
	  const Text& columnToCheck,
	  const Text& colValToCheck,
	  NABoolean noXn,
	  const int64_t timestamp)

{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();
  HTC_RetCode rc = htc->checkAndDeleteRow(transID, row, columnToCheck, colValToCheck,
					  timestamp);

  client_->releaseHTableClient(htc);

  if (rc == HTC_ERROR_CHECKANDDELETE_ROW_NOTFOUND)
    return HBASE_ROW_NOTFOUND_ERROR;

  retCode_ = rc;

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}
//
//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::insertRow(
	  HbaseStr &tblName,
	  HbaseStr &rowID, 
          HbaseStr &row,
	  NABoolean noXn,
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();

  retCode_ = htc->insertRow(transID, rowID, row, timestamp);


  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::insertRows(
	  HbaseStr &tblName,
          short rowIDLen,
          HbaseStr &rowIDs,
          HbaseStr &rows,
	  NABoolean noXn,
	  const int64_t timestamp,
	  NABoolean autoFlush)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }

  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();
  retCode_ = htc->insertRows(transID, rowIDLen, rowIDs, rows, timestamp, autoFlush);

  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getRows(
          short rowIDLen,
          HbaseStr &rowIDs,
	  const LIST(HbaseStr) & columns)
{
  ex_assert(htc_ != NULL, "htc_ is null");
  Int64 transID;
  transID = getTransactionIDFromContext();
  retCode_ = htc_->getRows(transID, rowIDLen, rowIDs, columns);


  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::setWriteBufferSize(
                                HbaseStr &tblName,
                                Lng32 size)
{

  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_,tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }

  retCode_ = htc->setWriteBufferSize(size);

  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}
Lng32 ExpHbaseInterface_JNI::setWriteToWAL(
                                HbaseStr &tblName,
                                NABoolean WAL )
{

  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_,tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }

  retCode_ = htc->setWriteToWAL(WAL);

  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}



Lng32 ExpHbaseInterface_JNI::initHBLC(ExHbaseAccessStats* hbs)
{

  Lng32  rc = init(hbs);
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  if (hblc_ == NULL)
  {
    hblc_ = client_->getHBulkLoadClient((NAHeap *)heap_);
    if (hblc_ == NULL)
    {
      retCode_ = HBLC_ERROR_INIT_HBLC_EXCEPTION;
      return HBASE_INIT_HBLC_ERROR;
    }
  }

  return HBLC_OK;
}
Lng32 ExpHbaseInterface_JNI::initHFileParams(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName,
                           Int64 maxHFileSize,
                           const char* sampleTblName,
                           const char* hiveDDL)
{
  if (hblc_ == NULL)
  {
    return -HBASE_ACCESS_ERROR;
  }

  retCode_ = hblc_->initHFileParams(tblName, hFileLoc, hfileName, maxHFileSize, sampleTblName, hiveDDL);
  //close();
  if (retCode_ == HBLC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_CREATE_HFILE_ERROR;

}

 Lng32 ExpHbaseInterface_JNI::addToHFile( short rowIDLen,
                                          HbaseStr &rowIDs,
                                          HbaseStr &rows)
 {
   if (hblc_ == NULL || client_ == NULL)
   {
     return -HBASE_ACCESS_ERROR;
   }

   retCode_ = hblc_->addToHFile(rowIDLen, rowIDs, rows, hbs_);
   if (retCode_ == HBLC_OK)
     return HBASE_ACCESS_SUCCESS;
   else
     return -HBASE_ADD_TO_HFILE_ERROR;
 }


 Lng32 ExpHbaseInterface_JNI::closeHFile(HbaseStr &tblName)
 {
   if (hblc_ == NULL || client_ == NULL)
   {
     return -HBASE_ACCESS_ERROR;
   }

   retCode_ = hblc_->closeHFile(tblName);
   //close();
   if (retCode_ == HBLC_OK)
     return HBASE_ACCESS_SUCCESS;
   else
     return -HBASE_CLOSE_HFILE_ERROR;
 }

 Lng32 ExpHbaseInterface_JNI::doBulkLoad(HbaseStr &tblName,
                          Text& location,
                          Text& tableName,
                          NABoolean quasiSecure,
                          NABoolean snapshot)
 {
   if (hblc_ == NULL || client_ == NULL)
   {
     return -HBASE_ACCESS_ERROR;
   }

   retCode_ = hblc_->doBulkLoad(tblName, location, tableName, quasiSecure,snapshot);

   if (retCode_ == HBLC_OK)
     return HBASE_ACCESS_SUCCESS;
   else
     return -HBASE_DOBULK_LOAD_ERROR;
 }
 
 Lng32 ExpHbaseInterface_JNI::bulkLoadCleanup(
                                HbaseStr &tblName,
                                Text& location)
 {
   if (hblc_ == NULL || client_ == NULL)
   {
     return -HBASE_ACCESS_ERROR;
   }

   retCode_ = hblc_->bulkLoadCleanup(tblName, location);

   if (retCode_ == HBLC_OK)
     return HBASE_ACCESS_SUCCESS;
   else
     return -HBASE_CLEANUP_HFILE_ERROR;
 }
 ///////////////////

 //////////////////////////////////////////////////////////////////////////////
 //
 //////////////////////////////////////////////////////////////////////////////
 Lng32 ExpHbaseInterface_JNI::hdfsCreateFile(const char* path)
 {
   if (hive_ == NULL) {
      retCode_ = initHive();
      if (retCode_ != HVC_OK)
         return retCode_;
   }

    retCode_ = hive_->hdfsCreateFile( path);

    if (retCode_ == HVC_OK)
      return HBASE_ACCESS_SUCCESS;
    else
      return -HVC_ERROR_HDFS_CREATE_EXCEPTION;
 }

 Lng32  ExpHbaseInterface_JNI::incrCounter( const char * tabName, const char * rowId,
                             const char * famName, const char * qualName ,
                             Int64 incr, Int64 & count)
 {
    if (client_ == NULL) {
      retCode_ = init();
      if (retCode_ != HBC_OK)
         return -HBASE_ACCESS_ERROR;
    }
    retCode_ = client_->incrCounter( tabName, rowId, famName, qualName , incr, count);

    if (retCode_ == HBC_OK)
      return HBASE_ACCESS_SUCCESS;
    else
      return -HBC_ERROR_INCR_COUNTER_EXCEPTION;
 }

 Lng32  ExpHbaseInterface_JNI::createCounterTable( const char * tabName,  const char * famName)
 {
    if (client_ == NULL) {
      retCode_ = init();
      if (retCode_ != HBC_OK)
         return -HBASE_ACCESS_ERROR;
   }

   retCode_ = client_->createCounterTable( tabName, famName);

   if (retCode_ == HBC_OK)
     return HBASE_ACCESS_SUCCESS;
   else
      return -HBC_ERROR_CREATE_COUNTER_EXCEPTION;
 }
 //////////////////////////////////////////////////////////////////////////////
 //
 //////////////////////////////////////////////////////////////////////////////
 Lng32 ExpHbaseInterface_JNI::hdfsWrite(const char* data, Int64 len)
 {
   if (hive_ == NULL) {
      retCode_ = initHive();
      if (retCode_ != HVC_OK)
         return retCode_;
   }
   retCode_ = hive_->hdfsWrite( data, len);

   if (retCode_ == HVC_OK)
      return HBASE_ACCESS_SUCCESS;
    else
      return -HVC_ERROR_HDFS_WRITE_EXCEPTION;
 }

 //////////////////////////////////////////////////////////////////////////////
 //
 //////////////////////////////////////////////////////////////////////////////
 Lng32 ExpHbaseInterface_JNI::hdfsClose()
 {
   if (hive_ == NULL) {
      retCode_ = initHive();
      if (retCode_ != HVC_OK)
         return retCode_;
   }

   retCode_ = hive_->hdfsClose();

   if (retCode_ == HVC_OK)
      return HVC_OK;
    else
      return -HVC_ERROR_HDFS_CLOSE_EXCEPTION;
 }
/*
 Lng32 ExpHbaseInterface_JNI::hdfsCleanPath( const std::string& path)
 {
   if (hblc_ == NULL) {
      retCode_ = initHBLC();
      if (retCode_ != HBLC_OK)
         return -HBASE_ACCESS_ERROR;
   }

   retCode_ = hblc_->hdfsCleanPath(path);

   if (retCode_ == HBLC_OK)
      return HBLC_OK;
    else
      return -HBLC_ERROR_HDFS_CLOSE_EXCEPTION;
 }
*/

Lng32 ExpHbaseInterface_JNI::isEmpty(
                                     HbaseStr &tblName)
{
  Lng32 retcode;

  retcode = init(hbs_);
  if (retcode != HBASE_ACCESS_SUCCESS)
    return -HBASE_OPEN_ERROR;
  
  LIST(HbaseStr) columns(heap_);

  retcode = scanOpen(tblName, "", "", columns, -1, FALSE, FALSE, 100, TRUE, NULL, 
       NULL, NULL, NULL);
  if (retcode != HBASE_ACCESS_SUCCESS)
    return -HBASE_OPEN_ERROR;

  retcode = nextRow();

  scanClose();
  if (retcode == HBASE_ACCESS_EOD)
    return 1; // isEmpty
  else if (retcode == HBASE_ACCESS_SUCCESS)
    return 0; // not empty

  return -HBASE_ACCESS_ERROR; // error
}

//----------------------------------------------------------------------------
// Avoid messing up the class data members (like htc_)
Lng32 ExpHbaseInterface_JNI::rowExists(
	     HbaseStr &tblName,
	     HbaseStr &row)
{
  Lng32 rc = 0;
  LIST(HbaseStr) columns(heap_);
  
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->startGet(transID, row, columns, -1); 
  if (retCode_ != HBC_OK)
    return -HBASE_OPEN_ERROR;

  retCode_ = htc_->nextRow();
  client_->releaseHTableClient(htc);

  if (retCode_ == HTC_OK)
    return 1; // exists
  else if (retCode_ == HTC_DONE_DATA || retCode_ == HTC_DONE_RESULT)
    return 0; // does not exist
  else
    return -HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::checkAndInsertRow(
	  HbaseStr &tblName,
	  HbaseStr &rowID, 
	  HbaseStr &row,
	  NABoolean noXn,
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  

  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();
 

  HTC_RetCode rc = htc->checkAndInsertRow(transID, rowID, row, timestamp);



  client_->releaseHTableClient(htc);

  retCode_ = rc;

  if (rc == HTC_ERROR_CHECKANDINSERT_DUP_ROWID)
     return HBASE_DUP_ROW_ERROR;
  if (retCode_ != HBC_OK)
     return -HBASE_ACCESS_ERROR;
  else
     return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::checkAndUpdateRow(
	  HbaseStr &tblName,
	  HbaseStr &rowID, 
	  HbaseStr &row,
	  const Text& columnToCheck,
	  const Text& colValToCheck,
          NABoolean noXn,
	  const int64_t timestamp)

{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID;
  if (noXn)
    transID = 0;
  else
    transID = getTransactionIDFromContext();

  HTC_RetCode rc = htc->checkAndUpdateRow(transID, rowID, row, 
					  columnToCheck, colValToCheck,
					  timestamp);

  client_->releaseHTableClient(htc);

  if (rc == HTC_ERROR_CHECKANDUPDATE_ROW_NOTFOUND)
    return HBASE_ROW_NOTFOUND_ERROR;

  retCode_ = rc;

   if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::coProcAggr(
				    HbaseStr &tblName,
				    Lng32 aggrType, // 0:count, 1:min, 2:max, 3:sum, 4:avg
				    const Text& startRow, 
				    const Text& stopRow, 
				    const Text &colFamily,
				    const Text &colName,
				    const NABoolean cacheBlocks,
				    const Lng32 numCacheRows,
				    Text &aggrVal) // returned value
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_, hbs_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }

  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->coProcAggr(
			  transID, aggrType, startRow, stopRow,
			  colFamily, colName, cacheBlocks, numCacheRows,
			  aggrVal);

  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}
 
//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getClose()
{
  if (htc_)
  {
    client_->releaseHTableClient(htc_);
    htc_ = NULL;
  }
  
  return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::grant(
				   const Text& user, 
				   const Text& tblName,
				   const std::vector<Text> & actionCodes)
{
  retCode_ = client_->grant(user, tblName, actionCodes);
  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::revoke(
				   const Text& user, 
				   const Text& tblName,
				   const std::vector<Text> & actionCodes)
{
  retCode_ = client_->revoke(user, tblName, actionCodes);
  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

ByteArrayList* ExpHbaseInterface_JNI::getRegionInfo(const char* tblName)
{ 
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName, useTRex_, hbs_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return NULL;
  }

   ByteArrayList* bal = htc_->getEndKeys();
   return bal;
}

Lng32 ExpHbaseInterface_JNI::getColVal(int colNo, BYTE *colVal,
          Lng32 &colValLen, NABoolean nullable, BYTE &nullVal)
{
  HTC_RetCode retCode = HTC_OK;
  if (htc_ != NULL)
     retCode = htc_->getColVal(colNo, colVal, colValLen, nullable,
                    nullVal);
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;

  if (retCode != HTC_OK)
    return HBASE_ACCESS_ERROR;
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::getColVal(NAHeap *heap, int colNo, 
          BYTE **colVal, Lng32 &colValLen)
{
  HTC_RetCode retCode = HTC_OK;
  if (htc_ != NULL)
     retCode = htc_->getColVal(heap, colNo, colVal, colValLen);
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;

  if (retCode != HTC_OK)
    return HBASE_ACCESS_ERROR;
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::getRowID(HbaseStr &rowID)
{
  HTC_RetCode retCode = HTC_OK;
  if (htc_ != NULL)
     retCode = htc_->getRowID(rowID);
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;

  if (retCode != HTC_OK)
    return HBASE_ACCESS_ERROR;
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::getNumCols(int &numCols)
{
  HTC_RetCode retCode = HTC_OK;
  if (htc_ != NULL)
     retCode = htc_->getNumCols(numCols);
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;
 
  if (retCode == HTC_OK)
     return HBASE_ACCESS_SUCCESS;
  else if (retCode == HTC_DONE_DATA)
     return HBASE_ACCESS_NO_ROW;
 return HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::getColName(int colNo,
              char **outColName,
              short &colNameLen,
              Int64 &timestamp)
{
  HTC_RetCode retCode = HTC_OK;
  if (htc_ != NULL)
     retCode = htc_->getColName(colNo, outColName, colNameLen, timestamp);
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;

  if (retCode != HTC_OK)
    return HBASE_ACCESS_ERROR;
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::nextRow()
{
  if (htc_ != NULL)
     retCode_ = htc_->nextRow();
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;

  if (retCode_ == HTC_OK)
    return HBASE_ACCESS_SUCCESS;
  else if (retCode_ == HTC_DONE)
    return HBASE_ACCESS_EOD;
  else if (retCode_ == HTC_DONE_RESULT)
    return HBASE_ACCESS_EOR;
  else
    return -HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::nextCell(HbaseStr &rowId,
          HbaseStr &colFamName,
          HbaseStr &colName,
          HbaseStr &colVal,
          Int64 &timestamp)
{
  if (htc_ != NULL)
     retCode_ = htc_->nextCell(rowId, colFamName,
                    colName, colVal, timestamp);
  else
     return HBC_ERROR_GET_HTC_EXCEPTION;

  if (retCode_ == HTC_OK)
    return HBASE_ACCESS_SUCCESS;
  else if (retCode_ == HTC_DONE)
    return HBASE_ACCESS_EOD;
  else
    return -HBASE_ACCESS_ERROR;
}


// Get an estimate of the number of rows in table tblName. Pass in the
// fully qualified table name and the number of columns in the table.
// The row count estimate is returned in estRC.
Lng32 ExpHbaseInterface_JNI::estimateRowCount(HbaseStr& tblName,
                                              Int32 partialRowSize,
                                              Int32 numCols,
                                              Int64& estRC)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }

  estRC = 0;
  retCode_ = client_->estimateRowCount(tblName.val, partialRowSize, numCols, estRC);
  return retCode_;
}

// get nodeNames of regions. this information will be used to co-locate ESPs
Lng32 ExpHbaseInterface_JNI::getRegionsNodeName(const HbaseStr& tblName,
                                                Int32 partns,
                                                ARRAY(const char *)& nodeNames)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }

  retCode_ = client_->getRegionsNodeName(tblName.val, partns, nodeNames);
  return retCode_;
}


// Get Hbase Table information. This will be generic function to get needed information
// from Hbase layer. Currently index level and blocksize is being requested for use in
// costing code, but can be extended in the future so that we only make one JNI call.
Lng32 ExpHbaseInterface_JNI::getHbaseTableInfo(const HbaseStr& tblName,
                                               Int32& indexLevels,
                                               Int32& blockSize)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }

  retCode_ = client_->getHbaseTableInfo(tblName.val, indexLevels, blockSize);
  return retCode_;
}

Lng32 ExpHbaseInterface_JNI::getLatestSnapshot( const char * tabname, char *& snapshotName, NAHeap * heap)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }

  retCode_ = client_->getLatestSnapshot(tabname, snapshotName, heap);
  return retCode_;
}
Lng32 ExpHbaseInterface_JNI::cleanSnpTmpLocation( const char * path)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
  retCode_ = client_->cleanSnpTmpLocation(path);
  return retCode_;
}

Lng32  ExpHbaseInterface_JNI::setArchivePermissions( const char * tabName)
{
  if (client_ == NULL)
  {
    if (init(hbs_) != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
  retCode_ = client_->setArchivePermissions(tabName);
  return retCode_;
}

Lng32 ExpHbaseInterface_JNI::getBlockCacheFraction(float& frac)
{
  if (client_ == NULL)
    return -HBASE_ACCESS_ERROR ;

  retCode_ = client_->getBlockCacheFraction(frac);
  return retCode_;
}
