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
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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

#include "Platform.h"
#include "ExpHbaseInterface.h"
#include "str.h"

extern Int64 getTransactionIDFromContext();

// ===========================================================================
// ===== Class ExpHbaseInterface
// ===========================================================================

ExpHbaseInterface::ExpHbaseInterface(CollHeap * heap,
                                     const char * server,
                                     const char * port,
                                     const char * zkPort,
                                     int debugPort,
                                     int debugTimeout)
{
  heap_ = heap;

  if (server)
    strncpy(server_, server, sizeof(server_));
  else
    server_[0] = 0;

  if (port)
    strncpy(port_, port, sizeof(port_));
  else
    port_[0] = 0;

  if (zkPort)
    strncpy(zkPort_, zkPort, sizeof(zkPort_));
  else
    zkPort_[0] = 0;

  debugPort_ = debugPort;
  debugTimeout_ = debugTimeout;
}

ExpHbaseInterface* ExpHbaseInterface::newInstance(CollHeap* heap,
                                                  const char* server,
                                                  const char* port,
                                                  const char* interface,
                                                  const char *zkPort,
                                                  int debugPort,
                                                  int debugTimeout)
{
   return new (heap) ExpHbaseInterface_JNI(heap, server, port, TRUE,
                                            zkPort, debugPort, debugTimeout); // This is the transactional interface
}

Lng32 ExpHbaseInterface_JNI::fetchRowVec(HbaseStr &rowID)
{
   Lng32 retcode;
   jbyte *jbRowResult; 
   jbyteArray jbaRowResult;
   jboolean isCopy;
   Int32 numCols;
   Int32 rowIDLen;
   char *kvBuf;

   retcode = fetchRowVec(&jbRowResult, jbaRowResult, &isCopy);
   if (retcode == HBASE_ACCESS_SUCCESS)
   {
     kvBuf = (char *) jbRowResult;
     numCols = *(Int32 *)kvBuf;
     kvBuf += sizeof(numCols);
     if (numCols == 0)
     {
        rowID.val = NULL;
        rowID.len = 0;
     }
     else
     {
        rowIDLen = *(Int32 *)kvBuf;
        kvBuf += sizeof(rowIDLen);
        rowID.val = kvBuf;
        rowID.len = rowIDLen;
     }
     freeRowResult(jbRowResult, jbaRowResult);
  }
  else
  {
     rowID.val = NULL;
     rowID.len = 0;
  }
  return retcode;
}

Lng32 ExpHbaseInterface::deleteColumns(
	     HbaseStr &tblName,
	     const Text& column)
{
  Lng32 retcode = 0;

  std::vector<Text> columns;
  columns.push_back(column);

  retcode = init();
  if (retcode != HBASE_ACCESS_SUCCESS)
    return retcode;

  retcode = scanOpen(tblName, "", "", columns, -1, FALSE, FALSE, 100, NULL, NULL, NULL);
  if (retcode != HBASE_ACCESS_SUCCESS)
    return retcode;

  NABoolean done1 = FALSE;
  while (NOT done1)
    {
      retcode = fetchNextRow();
      if (retcode == HBASE_ACCESS_EOD)
	{
	  done1 = TRUE;
	  break;
	}
      
      if (retcode != HBASE_ACCESS_SUCCESS)
	{
	  // close
	  scanClose();

	  close();

	  // and done
	  return retcode;
	}
      
      NABoolean done2 = FALSE;
      HbaseStr rowID;
      while (NOT done2)
	{
	  retcode = fetchRowVec(rowID);
	  if (retcode == HBASE_ACCESS_EOD)
	    {
	      done2 = TRUE;
	      break;
	    }

	  if (retcode != HBASE_ACCESS_SUCCESS)
	    {
	      // close
	      scanClose();

	      close();

	      return retcode;
	    }
	  retcode = deleteRow(tblName, rowID, columns, -1);
	  if (retcode != HBASE_ACCESS_SUCCESS)
	    {
	      // close
	      scanClose();

	      close();

	      return retcode;
	    }
	} // while NOT done2
    } // while NOT done1

  scanClose();

  close();

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface::checkAndUpdateRow(
					   HbaseStr &tblName,
					   HbaseStr &rowID, 
					   HbaseStr &row,
					   const Text& columnToCheck,
					   const Text& colValToCheck,
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

  TextVec columns;
  // row exists, delete it
  retcode = deleteRow(tblName,
		      rowID,
		      columns,
		      timestamp);
  
  return retcode;
}

Lng32  ExpHbaseInterface::fetchAllRows(
				       HbaseStr &tblName,
				       Lng32 numInCols,
				       Text &col1NameStr,
				       Text &col2NameStr,
				       Text &col3NameStr,
				       NAList<Text> &col1ValueList, // output
				       NAList<Text> &col2ValueList, // output
				       NAList<Text> &col3ValueList) // output
{
  Lng32 retcode;

  retcode = init();
  if (retcode != HBASE_ACCESS_SUCCESS)
    return retcode;

  col1ValueList.resize(0);
  col2ValueList.resize(0);

  jbyte *jbRowResult;
  jbyteArray jbaRowResult;
  jboolean isCopy;
  char *kvBuf;
  Int32 numCols; 
  HbaseStr rowID;
  Int32 rowIDLen;
  Int32 *temp;
  char *value;
  char *buffer;
  char *colName;
  char *family;
  char inlineColName[INLINE_COLNAME_LEN+1];
  char *fullColName;
  Lng32 colNameLen;
  Int32 kvLength, valueLength, valueOffset, qualLength, qualOffset;
  Int32 familyLength, familyOffset;
  long timestamp;
  Lng32 filledCols = 0;
  Int32 allocatedLength = 0;

  const std::vector<Text> columns;
  retcode = scanOpen(tblName, "", "", columns, -1, FALSE, FALSE, 100, NULL, 
       NULL, NULL);
  while (retcode == HBASE_ACCESS_SUCCESS)
  {
     retcode = fetchNextRow();
     if (retcode != HBASE_ACCESS_SUCCESS)
	continue;

     retcode = fetchRowVec(&jbRowResult, jbaRowResult, &isCopy);
     if (retcode == HBASE_ACCESS_EOD)
	{
	  retcode = HBASE_ACCESS_SUCCESS;
	  continue;
	}
      
     if (retcode != HBASE_ACCESS_SUCCESS)
	continue;

     kvBuf = (char *) jbRowResult;
     numCols = *(Int32 *)kvBuf;
     kvBuf += sizeof(numCols);
     if (numCols == 0)
     {
        rowID.val = NULL;
        rowID.len = 0;
     }
     else
     {
        rowIDLen = *(Int32 *)kvBuf;
        kvBuf += sizeof(rowIDLen);
        rowID.val = kvBuf;
        rowID.len = rowIDLen;
        kvBuf += rowIDLen;
     }
     filledCols = 0; 
     for (Lng32 j = 0; j < numCols && filledCols != numInCols; j++)
     {
        temp = (Int32 *)kvBuf;
        kvLength = *temp++;
        valueLength = *temp++;
        valueOffset = *temp++;
        qualLength = *temp++;
        qualOffset = *temp++;
        familyLength = *temp++;
        familyOffset = *temp++;
        timestamp = *(long *)temp;
        temp += 2;
        buffer = (char *)temp;
        value = buffer + valueOffset;

        colName = (char*)buffer + qualOffset;
        family = (char *)buffer + familyOffset;
        colNameLen = familyLength + qualLength + 1; // 1 for ':'

        if (allocatedLength == 0 && colNameLen < INLINE_COLNAME_LEN)
           fullColName = inlineColName;
        else
        {
           if (colNameLen > allocatedLength)
           {
               if (allocatedLength > 0)
               {
                  NADELETEBASIC(fullColName, heap_);
               }
               fullColName = new (heap_) char[colNameLen + 1];
               allocatedLength = colNameLen;
           }
        }
        strncpy(fullColName, family, familyLength);
        fullColName[familyLength] = '\0';
        strcat(fullColName, ":");
        strncat(fullColName, colName, qualLength);
        fullColName[colNameLen] = '\0';

        colName = fullColName;

	Text colValue((char*)value, valueLength);
	if (colName == col1NameStr)
	{
           filledCols++;
	   col1ValueList.insert(colValue);
	}
	else if (colName == col2NameStr)
	{
	   col2ValueList.insert(colValue);
           filledCols++;
	}
	else if (colName == col3NameStr)
	{
	   col3ValueList.insert(colValue);
           filledCols++;
	}
        kvBuf = (char *)temp + kvLength;
     }
     freeRowResult(jbRowResult, jbaRowResult);
     if (allocatedLength > 0)
     {
         NADELETEBASIC(fullColName, heap_);
     }
  } // while
  
  if (retcode == HBASE_ACCESS_EOD)
    retcode = HBASE_ACCESS_SUCCESS;

  if (retcode == HBASE_ACCESS_SUCCESS)
    {
      retcode = scanClose();
    }
  else
    {
      scanClose();
    }
  
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

ExpHbaseInterface_JNI::ExpHbaseInterface_JNI(CollHeap* heap, const char* server, const char* port, bool useTRex,
                                             const char *zkPort, int debugPort, int debugTimeout)
     : ExpHbaseInterface(heap, server, port, zkPort, debugPort, debugTimeout)
   ,useTRex_(useTRex)
   ,client_(NULL)
   ,htc_(NULL)
   ,hblc_(NULL)
   ,retCode_(HBC_OK)
   ,lastKVBuffer_(NULL)
   ,lastKVColName_(NULL)
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
Lng32 ExpHbaseInterface_JNI::init()
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
    
  NADELETEBASIC(lastKVBuffer_, heap_);
  NADELETEBASIC(lastKVColName_, heap_);
  
  lastKVBuffer_ = NULL;
  lastKVColName_ = NULL;

  return cleanup();
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::create(HbaseStr &tblName,
				    HBASE_NAMELIST& colFamNameList)
{
  if (client_ == NULL)
  {
    if (init() != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->create(tblName.val, colFamNameList); 
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
                                    const char ** splitValues)
{
  if (client_ == NULL)
  {
    if (init() != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->create(tblName.val, hbaseCreateOptionsArray,
                             numSplits, keyLength, splitValues); 
  //close();
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_CREATE_ERROR;
}


//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::drop(HbaseStr &tblName, NABoolean async)
{
  if (client_ == NULL)
  {
    if (init() != HBASE_ACCESS_SUCCESS)
      return -HBASE_ACCESS_ERROR;
  }
    
  retCode_ = client_->drop(tblName.val, async);

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
    if (init() != HBASE_ACCESS_SUCCESS)
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
Lng32 ExpHbaseInterface_JNI::copy(HbaseStr &currTblName, HbaseStr &oldTblName)
{
  if (client_ == NULL)
  {
    if (init() != HBASE_ACCESS_SUCCESS)
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
    if (init() != HBASE_ACCESS_SUCCESS)
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
				      const std::vector<Text> & columns,
				      const int64_t timestamp,
				      const NABoolean readUncommitted,
				      const NABoolean cacheBlocks,
				      const Lng32 numCacheRows,
				      const TextVec *inColNamesToFilter, 
				      const TextVec *inCompareOpList,
				      const TextVec *inColValuesToCompare,
                                      Float32 samplePercent)
{
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }

  Int64 transID = getTransactionIDFromContext();
  if (readUncommitted)
    {
      transID = 0;
    }
  retCode_ = htc_->startScan(transID, startRow, stopRow, columns, timestamp, 
					  cacheBlocks, numCacheRows, 
					  inColNamesToFilter,
					  inCompareOpList,
					  inColValuesToCompare,
					  samplePercent);
                                          //NULL, NULL, NULL);
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_OPEN_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getLastFetchedCell(
	  HbaseStr &rowId,
	  HbaseStr &colFamName,
	  HbaseStr &colName,
	  HbaseStr &colVal,
	  Int64 &timestamp)
{
  KeyValue* kv = htc_->getLastFetchedCell();
  if (kv == NULL)
    return HBASE_ACCESS_EOD;
    
  Int32 buffSize = kv->getBufferSize()+100;
  if (lastKVBuffer_ == NULL || lastKVBufferSize_ < buffSize)
  {
    NADELETEBASIC(lastKVBuffer_, heap_);
    lastKVBuffer_ = new (heap_) char[buffSize];
    lastKVBufferSize_ = buffSize;    
  }  
  kv->getBuffer(lastKVBuffer_, lastKVBufferSize_);
  
  rowId.val      = lastKVBuffer_ + kv->getRowOffset();
  rowId.len      = kv->getRowLength();
  colFamName.val = lastKVBuffer_ + kv->getFamilyOffset();
  colFamName.len = kv->getFamilyLength();
  colName.val    = lastKVBuffer_ + kv->getQualifierOffset();
  colName.len    = kv->getQualifierLength();
  colVal.val     = lastKVBuffer_ + kv->getValueOffset();
  colVal.len     = kv->getValueLength();
  timestamp      = kv->getTimestamp();

  Int32 nameSize = colFamName.len+colName.len+1+10;
  if (lastKVColName_ == NULL || lastKVColNameSize_ < nameSize)
  {
    NADELETEBASIC(lastKVColName_, heap_);
    lastKVColName_ = new (heap_) char[nameSize];
    lastKVColNameSize_ = nameSize;    
  }  
  memcpy((char*)lastKVColName_, colFamName.val, colFamName.len);
  lastKVColName_[colFamName.len] = ':';
  memcpy(lastKVColName_+colFamName.len+1, colName.val, colName.len);
  lastKVColName_[colFamName.len+colName.len+1] = '\0';
  colName.val = (char*)lastKVColName_;
  colName.len = strlen(lastKVColName_); 
  
  NADELETE(kv, KeyValue, kv->getHeap());
  return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::scanFetch(
	  HbaseStr &rowId,
	  HbaseStr &colFamName,
	  HbaseStr &colName,
	  HbaseStr &colVal,
	  Int64 &timestamp)
{
  retCode_ = htc_->scanFetch();
  if (retCode_ != HBC_OK)
  {
    if (retCode_ == HTC_DONE)
      return HBASE_ACCESS_EOD;
    else
      return -HBASE_ACCESS_ERROR;
  }
  
  return getLastFetchedCell(rowId, colFamName, colName, colVal, timestamp);
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

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getRowOpen(
	HbaseStr &tblName,
	const Text &row, 
	const std::vector<Text> & columns,
	const int64_t timestamp)
{
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc_->startGet(transID, row, columns, timestamp); 
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_OPEN_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getRowsOpen(
	HbaseStr &tblName,
	const std::vector<Text> & rows, 
	const std::vector<Text> & columns,
	const int64_t timestamp)
{
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc_->startGets(transID, rows, columns, timestamp); 
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_OPEN_ERROR;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getFetch(
	 HbaseStr &rowId,
	 HbaseStr &colFamName,
	 HbaseStr &colName,
	 HbaseStr &colVal,
	 Int64 &timestamp)
{
  retCode_ = htc_->getFetch();
  if (retCode_ != HBC_OK)
  {
    if (retCode_ == HTC_DONE)
      return HBASE_ACCESS_EOD;
    else
      return -HBASE_ACCESS_ERROR;
  }
 
  return getLastFetchedCell(rowId, colFamName, colName, colVal, timestamp);
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::fetchNextRow()
{
  retCode_ = htc_->fetchNextRow();
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else if (retCode_ == HTC_DONE)
    return HBASE_ACCESS_EOD;
  else
    return -HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::fetchRowVec(jbyte **jbRowResult,
                            jbyteArray &jbaRowResult,
                            jboolean *isCopy)
{
  retCode_ = htc_->fetchRowVec(jbRowResult, jbaRowResult, isCopy);
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  if (retCode_ == HTC_DONE_DATA)
    return HBASE_ACCESS_EOD;
  else if (retCode_ == HTC_DONE_RESULT)
    return HBASE_ACCESS_EOR;
  else
    return -HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::freeRowResult(jbyte *jbRowResult,
                            jbyteArray &jbaRowResult)
{
  retCode_ = htc_->freeRowResult(jbRowResult, jbaRowResult);
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  else
    return -HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::deleteRow(
	  HbaseStr &tblName,
	  HbaseStr& row, 
	  const std::vector<Text> & columns,
	  const int64_t timestamp)
{
  HTableClient_JNI *htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
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
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
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
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  //  retCode_ = (HBC_RetCode)
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
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
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
	  const int64_t timestamp,
	  NABoolean autoFlush)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->insertRows(transID, rowIDLen, rowIDs, rows, timestamp, autoFlush);

  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::setWriteBufferSize(
                                HbaseStr &tblName,
                                Lng32 size)
{

  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_,tblName.val, useTRex_);
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

  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_,tblName.val, useTRex_);
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



Lng32 ExpHbaseInterface_JNI::initHBLC()
{

  Lng32  rc = init();
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
Lng32 ExpHbaseInterface_JNI::createHFile(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName)
{
  if (hblc_ == NULL)
  {
    return -HBASE_ACCESS_ERROR;
  }

  retCode_ = hblc_->createHFile(tblName, hFileLoc, hfileName);
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

   retCode_ = hblc_->addToHFile(rowIDLen, rowIDs, rows);
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
//----------------------------------------------------------------------------
// Avoid messing up the class data members (like htc_)
Lng32 ExpHbaseInterface_JNI::rowExists(
	     HbaseStr &tblName,
	     HbaseStr &row)
{
  Lng32 rc = 0;
  StrVec columns;
  
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->startGet(transID, row.val, columns, -1); 
  if (retCode_ != HBC_OK)
    return -HBASE_OPEN_ERROR;

  jbyte *jbRowResult;
  jbyteArray jbaRowResult;
  jboolean isCopy;
  retCode_ = htc->fetchRowVec(&jbRowResult, jbaRowResult, &isCopy);
  freeRowResult(jbRowResult, jbaRowResult);
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
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
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
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
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
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
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
  htc_ = client_->getHTableClient((NAHeap *)heap_, tblName, useTRex_);
  if (htc_ == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return NULL;
  }

   ByteArrayList* bal = htc_->getEndKeys();
   return bal;
}
