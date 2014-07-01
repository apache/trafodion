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
  if (interface==NULL || !strcmp(interface, "THRIFT"))
    return new (heap) ExpHbaseInterface_Thrift(heap, server, port);
  else if (!strcmp(interface, "JNI"))
    return new (heap) ExpHbaseInterface_JNI(heap, server, port, FALSE,
                                            zkPort, debugPort, debugTimeout);
  else  
    return new (heap) ExpHbaseInterface_JNI(heap, server, port, TRUE,
                                            zkPort, debugPort, debugTimeout); // This is the transactional interface
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
      
      TRowResult rowResult;
      
      NABoolean done2 = FALSE;
      while (NOT done2)
	{
	  retcode = fetchRowVec(rowResult);
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
          HbaseStr rowID;
          rowID.val = (char *)rowResult.row.data();	  
          rowID.len = rowResult.row.size();
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

Lng32 ExpHbaseInterface::checkAndInsertRow(
					   HbaseStr &tblName,
					   HbaseStr &row, 
					   MutationVec & mutations,
					   const int64_t timestamp)
{
  Lng32 retcode = 0;

  retcode = rowExists(tblName, row);
  if (retcode == 1) // row exists
    {
      // return error
      return HBASE_DUP_ROW_ERROR;
    }

  if (retcode != HBASE_ACCESS_SUCCESS)
    return retcode;

  retcode = insertRow(tblName,
		      row,
		      mutations,
		      timestamp);
  
  return retcode;
}

Lng32 ExpHbaseInterface::checkAndUpdateRow(
					   HbaseStr &tblName,
					   HbaseStr &row, 
					   MutationVec & mutations,
					   const Text& columnToCheck,
					   const Text& colValToCheck,
					   const int64_t timestamp)
{
  Lng32 retcode = 0;

  retcode = rowExists(tblName, row);
  if (retcode < 0)
    return retcode;

  if (retcode == 0) // row does not exist
    {
      // return warning
      return HBASE_ROW_NOTFOUND_ERROR;
    }

  // if exists, update it
  retcode = insertRow(tblName,
		      row,
		      mutations,
		      timestamp);
  
  return retcode;
}

Lng32 ExpHbaseInterface::checkAndDeleteRow(
					   HbaseStr &tblName,
					   HbaseStr &row, 
					   const Text& columnToCheck,
					   const Text& colValToCheck,
					   const int64_t timestamp)
{
  Lng32 retcode = 0;

  retcode = rowExists(tblName, row);
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
		      row,
		      columns,
		      timestamp);
  
  return retcode;
}

Lng32 ExpHbaseInterface::insertRows(
				   HbaseStr &tblName,
				   std::vector<BatchMutation> & rows,
				   const int64_t timestamp,
				   NABoolean autoFlush)
{
  Lng32 retcode = 0;

  for (Lng32 i = 0; i < rows.size(); i++)
    {
      BatchMutation &bm = rows[i];

      const Text &row = bm.row;
      MutationVec &mutations = bm.mutations;

      HbaseStr insRow;
      insRow.val = (char *)row.data();
      insRow.len = row.size();
      retcode = insertRow(tblName, insRow, mutations, timestamp);
      if (retcode != HBASE_ACCESS_SUCCESS)
	return retcode;
    }

  return retcode;
}

Lng32 ExpHbaseInterface::deleteRows(
				   HbaseStr &tblName,
				   std::vector<BatchMutation> & rows,
				   const int64_t timestamp)
{
  Lng32 retcode = 0;

  for (Lng32 i = 0; i < rows.size(); i++)
    {
      BatchMutation &bm = rows[i];

      const Text &row = bm.row;
      //      MutationVec &mutations = bm.mutations;

      TextVec columns;
      HbaseStr delRow;
      delRow.val = (char *)row.data();
      delRow.len = row.size();
      retcode = deleteRow(tblName, delRow, columns, timestamp);
      if (retcode != HBASE_ACCESS_SUCCESS)
	return retcode;
    }

  return retcode;
}

Lng32  ExpHbaseInterface::fetchAllRows(
				       HbaseStr &tblName,
				       Lng32 numCols,
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

  const std::vector<Text> columns;
  retcode = scanOpen(tblName, "", "", columns, -1, FALSE, FALSE, 100, NULL, NULL, NULL);
  while (retcode == HBASE_ACCESS_SUCCESS)
    {
      retcode = fetchNextRow();
      if (retcode != HBASE_ACCESS_SUCCESS)
	continue;

      TRowResult rowResult;
      retcode = fetchRowVec(rowResult);
      if (retcode == HBASE_ACCESS_EOD)
	{
	  retcode = HBASE_ACCESS_SUCCESS;
	  continue;
	}
      
      if (retcode != HBASE_ACCESS_SUCCESS)
	continue;

      CellMap::const_iterator colIter = rowResult.columns.begin();
      for (Lng32 j = 0; j < numCols; j++)
	{
	  Text colName((char*)colIter->first.data(), colIter->first.length());
	  Text colValue((char*)colIter->second.value.data(), colIter->second.value.length());
	  if (colName == col1NameStr)
	    {
	      col1ValueList.insert(colValue);
	    }
	  else if (colName == col2NameStr)
	    {
	      col2ValueList.insert(colValue);
	    }
	  else if (colName == col3NameStr)
	    {
	      col3ValueList.insert(colValue);
	    }

	  colIter++;
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

// ===========================================================================
// ===== Class ExpHbaseInterface_Thrift
// ===========================================================================

ExpHbaseInterface_Thrift::ExpHbaseInterface_Thrift(CollHeap * heap, const char * server, const char * port)
  : ExpHbaseInterface(heap, server, port)
{
  bool isFramed = false;

  transport_ = new  boost::shared_ptr<TTransport>;

  socket_ = new boost::shared_ptr<TTransport>(new TSocket(server, boost::lexical_cast<int>(port_)));

  if (isFramed) {
    transport_->reset(new TFramedTransport(*socket_));
  } else {
    transport_->reset(new TBufferedTransport(*socket_));
  }

  protocol_ = new boost::shared_ptr<TProtocol> (new TBinaryProtocol(*transport_));

  client_ = new HbaseClient(*protocol_);
}
  
Lng32 ExpHbaseInterface_Thrift::init()
{
  try {
    (*transport_)->open();
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_OPEN_ERROR;
  }

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::close()
{
  try {
    (*transport_)->close();
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_CLOSE_ERROR;
  }

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::flushTable()
{
  return 0;
}

Lng32 ExpHbaseInterface_Thrift::cleanup()
{
  return 0;
}

Lng32 ExpHbaseInterface_Thrift::create(HbaseStr &tblName,
				       HBASE_NAMELIST& colFamNameList)
{
  Lng32 rc;
  std::string t(tblName.val);
  
  rc = init();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  ColVec columns;
  for (Lng32 i = 0; i < colFamNameList.entries(); i++)
    {
      columns.push_back(ColumnDescriptor());

      columns.back().name = colFamNameList[i].val;
      columns.back().maxVersions = 1;
    }

  try {
    
    try {
      client_->createTable(t, columns);
    } catch (const AlreadyExists &ae) {
      std::strcpy(errText_, ae.message.c_str());
      return -HBASE_CREATE_ERROR;
    }
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_CREATE_ERROR;
  }

  rc = close();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;
  
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::create(HbaseStr &tblName,
				       NAText * hbaseCreateOptionsArray,
                                       int numSplits, int keyLength,
                                       const char ** splitValues)
{
  Lng32 rc;
  std::string t(tblName.val);
  
  rc = init();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  ColVec columns;
  columns.push_back(ColumnDescriptor());
  
  // not all HBase create options are supported through the thrift interface
  if (! hbaseCreateOptionsArray[HBASE_NAME].empty())
    columns.back().name = hbaseCreateOptionsArray[HBASE_NAME];

  if (! hbaseCreateOptionsArray[HBASE_MAX_VERSIONS].empty())
    columns.back().maxVersions = 
      str_atoi(hbaseCreateOptionsArray[HBASE_MAX_VERSIONS].data(),
	       hbaseCreateOptionsArray[HBASE_MAX_VERSIONS].length());

  if (! hbaseCreateOptionsArray[HBASE_COMPRESSION].empty())
    columns.back().compression = hbaseCreateOptionsArray[HBASE_COMPRESSION];
  
  if (! hbaseCreateOptionsArray[HBASE_TTL].empty())
    columns.back().timeToLive = 
      str_atoi(hbaseCreateOptionsArray[HBASE_TTL].data(),
	       hbaseCreateOptionsArray[HBASE_TTL].length());

  if (! hbaseCreateOptionsArray[HBASE_IN_MEMORY].empty())
    {
      if (hbaseCreateOptionsArray[HBASE_IN_MEMORY] == "TRUE")
	columns.back().inMemory = true;
    }

  if (! hbaseCreateOptionsArray[HBASE_BLOCKCACHE].empty())
    {
      if (hbaseCreateOptionsArray[HBASE_BLOCKCACHE] == "TRUE")
	columns.back().blockCacheEnabled = true;
    }

  try {
    
    try {
      client_->createTable(t, columns);
    } catch (const AlreadyExists &ae) {
      std::strcpy(errText_, ae.message.c_str());
      return -HBASE_CREATE_ERROR;
    }
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_CREATE_ERROR;
  }

  rc = close();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;
  
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::drop(HbaseStr &tblName, NABoolean async)
{
  Lng32 rc;

  std::string t(tblName.val);

  rc = init();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  try {
    StrVec tables;
    client_->getTableNames(tables);
    bool found = false;
    for (StrVec::const_iterator it = tables.begin(); it != tables.end(); ++it) 
      {
	if (t == *it) 
	  {
	    found = true;
	    if (client_->isTableEnabled(*it)) 
	      {
		client_->disableTable(*it);
	      }
	    client_->deleteTable(*it);
	  }
      }
    if (not found)
      {
	strcpy(errText_, "table does not exist");
	return -HBASE_DROP_ERROR;
      }
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_DROP_ERROR;
  }

  rc = close();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;
    
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::exists(HbaseStr &tblName)
{
  Lng32 rc;

  std::string t(tblName.val);

  rc = init();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  bool found = false;
  try {
    StrVec tables;
    client_->getTableNames(tables);
    for (StrVec::const_iterator it = tables.begin(); 
	 ((not found) && (it != tables.end())); ++it) 
      {
	if (t == *it) 
	  {
	    found = true;
	  }
      }
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_ACCESS_ERROR;
  }

  rc = close();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;
   
  if (found)
    return -1; // table exists
  else
    return 0;
}

Lng32 ExpHbaseInterface_Thrift::getTable(HbaseStr &tblName)
{
  Lng32 rc = 0;
  while (1)
    {
      switch (getTableStep_)
	{
	case GET_TABLE_INIT_:
	  {
	    rc = init();
	    if (rc != HBASE_ACCESS_SUCCESS)
	      return rc;

	    getTableStep_ = GET_TABLE_OPEN_;
	  }
	  break;

	case GET_TABLE_OPEN_:
	  {
	    try {
	      client_->getTableNames(tables_);
	    } catch (const TException &tx) {
	      std::strcpy(errText_, tx.what());
	      return -HBASE_ACCESS_ERROR;
	    }
	    it_ = tables_.begin();
	    getTableStep_ = GET_TABLE_FETCH_;
	  }
	  break;
	  
	case GET_TABLE_FETCH_:
	  {
	    if (it_ == tables_.end())
	      {
		getTableStep_ = GET_TABLE_CLOSE_;
		break;
	      }
	    
	    tblName.val = (char*)(*it_).c_str();
	    tblName.len = strlen(tblName.val);

	    it_++;
	    return HBASE_ACCESS_SUCCESS;
	  }
	  break;
	  
	case GET_TABLE_CLOSE_:
	  {
	    rc = close();
	    if (rc != HBASE_ACCESS_SUCCESS)
	      return rc;
 
	    getTableStep_ = GET_TABLE_INIT_;

	    return HBASE_ACCESS_EOD;
	  }
	} // switch

    } // while

  return HBASE_ACCESS_SUCCESS;
}

void ExpHbaseInterface_Thrift::updateReturnValues(
					   HbaseStr &rowId, 
					   HbaseStr &colFamName,
					   HbaseStr &colName,
					   HbaseStr &colVal,
					   Int64 &timestamp)
{

  rowId.val = (char*)rowResultVec_[currRowIndex_].row.data();
  rowId.len = rowResultVec_[currRowIndex_].row.length();
  
  colName.val = (char*)colIter_->first.data();
  colName.len = colIter_->first.length();
  
  char * colFamEnd = strchr(colName.val, ':');
  if (colFamEnd)
    {
      colFamName.val = colName.val;
      colFamName.len = (colFamEnd - colFamName.val);
    }
  else
    {
      colFamName.val = NULL;
      colFamName.len = 0;
    }
  
  colVal.val = (char*)colIter_->second.value.data();
  colVal.len = colIter_->second.value.length();

  timestamp = colIter_->second.timestamp;
}

Lng32 ExpHbaseInterface_Thrift::getRowOpen(
	     HbaseStr &tblName,
	     const Text &row, 
	     const std::vector<Text> & columns,
	     const int64_t timestamp)
{
  std::string t(tblName.val);
  const std::map<Text, Text>  dummyAttributes; 

  try {
    try {
      if (timestamp == -1)
	{
	  if (columns.size() == 0)
	    client_->getRow(rowResultVec_, t, row, dummyAttributes);
	  else
	    client_->getRowWithColumns(rowResultVec_, t, row, columns, dummyAttributes);
	}
      else
	{
	  if (columns.size() == 0)
	    client_->getRowTs(rowResultVec_, t, row, timestamp, dummyAttributes);
	  else
	    client_->getRowWithColumnsTs(rowResultVec_, t, row, columns, timestamp, dummyAttributes);
	}
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: getRow raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  currRowIndex_ = 0;
  if (rowResultVec_.size() > 0)
    {
      colIter_ = rowResultVec_[currRowIndex_].columns.begin();
    }

  return 0;
}

Lng32 ExpHbaseInterface_Thrift::rowExists(
	     HbaseStr &tblName,
	     HbaseStr &row)
{
  Lng32 rc = 0;
  StrVec columns;
  
  rc = getRowOpen(tblName, row.val, columns, -1);
  if (rc < 0)
    return rc;

  if (rowResultVec_.size() > 0)
    return 1; // exists
  else
    return 0; // does not exist
}

Lng32 ExpHbaseInterface_Thrift::getRowsOpen(
	     HbaseStr &tblName,
	     const std::vector<Text> & rows, 
	     const std::vector<Text> & columns,
	     const int64_t timestamp)
{
  std::string t(tblName.val);
  const std::map<Text, Text>  dummyAttributes; 

  try {
    try {
      if (timestamp == -1)
	{
	  if (columns.size() == 0)
	    client_->getRows(rowResultVec_, t, rows, dummyAttributes);
	  else
	    client_->getRowsWithColumns(rowResultVec_, t, rows, columns, dummyAttributes);
	}
      else
	{
	  if (columns.size() == 0)
	    client_->getRowsTs(rowResultVec_, t, rows, timestamp, dummyAttributes);
	  else
	    client_->getRowsWithColumnsTs(rowResultVec_, t, rows, columns, timestamp, dummyAttributes);
	}
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: getRow raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  currRowIndex_ = 0;
  if (rowResultVec_.size() > 0)
    {
      colIter_ = rowResultVec_[currRowIndex_].columns.begin();
    }

  return 0;
}

Lng32 ExpHbaseInterface_Thrift::getFetch(
				  HbaseStr &rowId,
				  HbaseStr &colFamName,
				  HbaseStr &colName,
				  HbaseStr &colVal,
				  Int64 &timestamp)
{
  if (rowResultVec_.size() == 0) //if (isEod_)
    {
      return HBASE_ACCESS_EOD;
    }

  if (colIter_ == rowResultVec_[currRowIndex_].columns.end())
    {
      currRowIndex_++;

      if (currRowIndex_ == rowResultVec_.size())
	return HBASE_ACCESS_EOD;
      else
	{
	  colIter_ = rowResultVec_[currRowIndex_].columns.begin();
	}
    }

  updateReturnValues(rowId, colFamName, colName, colVal, timestamp);

  colIter_++;

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::fetchNextRow()
{
  try {
    try {
      client_->scannerGet(rowResultVec_, scanner_);
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: ScannerGet raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  if (rowResultVec_.size() == 0) //if (isEod_) 
    {
      return HBASE_ACCESS_EOD;
    }

  currRowIndex_ = 0;

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::fetchRowVec(
				     TRowResult  &rowResult
				     )
{
  if (currRowIndex_ == rowResultVec_.size())
    return HBASE_ACCESS_EOD;
 
  rowResult = rowResultVec_[currRowIndex_];
  currRowIndex_++;

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::getClose()
{
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::grant(
				      const Text& user, 
				      const Text& tblName,
				      const std::vector<Text> & actionCodes)
{
  // thrift doesn't support this method. Just say that it is granted :)
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::revoke(
				      const Text& user, 
				      const Text& tblName,
				      const std::vector<Text> & actionCodes)
{
  // thrift doesn't support this method. Just say that it is revoked :)
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::getRowInfo(
	     HbaseStr &tblName,
	     const Text& row, 
	     NAList<char*> &colNameList,
	     NAList<char*> &colValList)
{
  Lng32 rc = 0;
  std::string t(tblName.val);
  const std::map<Text, Text>  dummyAttributes; 

  rc = init();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  try {
    try {
      client_->getRow(rowResultVec_, t, row, dummyAttributes);
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: getRow raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  if (rowResultVec_.size() == 1) // only one row should exist
    {
      colIter_ = rowResultVec_[0].columns.begin();

      while (colIter_ != rowResultVec_[0].columns.end())
	{
	  char * colName = (char*)colIter_->first.c_str();
	  char * colVal = (char*)colIter_->second.value.c_str();

	  char * colNameNAS = new(heap_) char[strlen(colName) + 1];
	  char * colValNAS = new(heap_) char[strlen(colVal) + 1];

	  strcpy(colNameNAS, colName);
	  strcpy(colValNAS, colVal);

	  colNameList.insert(colNameNAS);
	  colValList.insert(colValNAS);
	  
	  colIter_++;
	}
    }

  rc = close();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  return 0;
}

Lng32 ExpHbaseInterface_Thrift::scanOpen(
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
                                  Float32 samplePercent)  // not used in this override
{
  const std::map<Text, Text>  dummyAttributes; // see HBASE-6806 HBASE-4658
  std::string t(tblName.val);
  try {
      if (timestamp == -1)
	{
	  if (stopRow == "")
	    scanner_ = client_->scannerOpen(t, startRow, columns, 
					    dummyAttributes);
	  else
	    scanner_ = client_->scannerOpenWithStop(t, startRow, stopRow, columns, 
						    dummyAttributes);
	}
      else
	{
	  if (stopRow == "")
	    scanner_ = client_->scannerOpenTs(t, startRow, columns, 
					      timestamp, dummyAttributes);
	  else
	    scanner_ = client_->scannerOpenWithStopTs(t, startRow, stopRow, columns, 
						      timestamp, dummyAttributes);
	}
    
  } catch (const TException &tx) {
    std::strcpy(errText_, tx.what());
    return -HBASE_OPEN_ERROR;
  }

  scanFetchStep_ = SCAN_FETCH_INIT_;
  scanFetchErr_ = 0;

  currRowIndex_ = 0;

  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::scanFetch(
				   HbaseStr &rowId,
				   HbaseStr &colFamName,
				   HbaseStr &colName,
				   HbaseStr &colVal,
				   Int64 &timestamp)
{
  while (1)
    {
      switch (scanFetchStep_)
	{
	case SCAN_FETCH_INIT_:
	  {
	    try {
	      try {
		client_->scannerGet(rowResultVec_, scanner_);
	      } catch (const TException &tx) {
		std::strcpy(errText_, tx.what());
		return -HBASE_ACCESS_ERROR;
	      }
	    } catch (const IOError &ioe) {
	      std::strcpy(errText_, "FATAL: ScannerGet raised IOError");
	      return -HBASE_ACCESS_ERROR;
	    }

	    if (rowResultVec_.size() == 0)
	      {
		scanFetchStep_ = SCAN_FETCH_CLOSE_;
		break;
	      }

	    currRowIndex_ = 0;
	    scanFetchStep_ = SCAN_FETCH_NEXT_ROW_;
	  } // case
	  break;
	  
	case SCAN_FETCH_NEXT_ROW_:
	  {
	    if (currRowIndex_ == rowResultVec_.size())
	      {
		scanFetchStep_ = SCAN_FETCH_INIT_;
		break;
	      }

	    colIter_ = rowResultVec_[currRowIndex_].columns.begin();
	    
	    scanFetchStep_ = SCAN_FETCH_NEXT_COL_;
	  }
	  break;
	  
	case SCAN_FETCH_NEXT_COL_:
	  {
	    if (colIter_ == rowResultVec_[currRowIndex_].columns.end())
	      {
		scanFetchStep_ = SCAN_FETCH_COL_END_;
		break;
	      }

	    updateReturnValues(rowId, colFamName, colName, colVal, timestamp);

	    colIter_++;
	    
	    return HBASE_ACCESS_SUCCESS;
	  }
	  break;
	  
	case SCAN_FETCH_COL_END_:
	  {
	    currRowIndex_++;
	    
	    scanFetchStep_ = SCAN_FETCH_NEXT_ROW_;
	  }
	  break;
	  
	case SCAN_FETCH_ERROR_:
	  {
	    return scanFetchErr_;
	  }
	  break;
	  
	case SCAN_FETCH_CLOSE_:
	  {
	    scanFetchStep_ = SCAN_FETCH_DONE_;
	  }
	  break;
	  
	case SCAN_FETCH_DONE_:
	  {
	    return HBASE_ACCESS_EOD;
	  }

	}// switch
    } // while
}

Lng32 ExpHbaseInterface_Thrift::scanClose()
{
  try {
    try {
      client_->scannerClose(scanner_);
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
      
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: ScannerGet raised IOError");
    return -HBASE_ACCESS_ERROR;
  }
  
  return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_Thrift::deleteRow(
	     HbaseStr &tblName,
	     HbaseStr &row, 
	     const std::vector<Text> & columns,
	     const int64_t timestamp)
{
  std::string t(tblName.val);
  const std::map<Text, Text>  dummyAttributes; 

  const Text column;

  try {
    try {
      if (timestamp == -1)
	{
	  if (column == "")
	    client_->deleteAllRow(t, row.val, dummyAttributes);
	  else
	    client_->deleteAll(t, row.val, column, dummyAttributes);
	}
      else
	{
	  if (column == "")
	    client_->deleteAllRowTs(t, row.val, timestamp, dummyAttributes);
	  else
	    client_->deleteAllTs(t, row.val, column, timestamp, dummyAttributes);
	}
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: deleteAll raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  return 0;
}

Lng32 ExpHbaseInterface_Thrift::insertRow(
				   HbaseStr &tblName,
				   HbaseStr &row, 
				   MutationVec & mutations,
				   const int64_t timestamp)
{
  std::string t(tblName.val);
  const std::map<Text, Text>  dummyAttributes; 

  try {
    try {
      if (timestamp == -1)
	{
	  client_->mutateRow(t, row.val, mutations, dummyAttributes);
	}
      else
	{
	  client_->mutateRowTs(t, row.val, mutations, timestamp, dummyAttributes);
	}
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: mutateRow raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  return 0;
}

Lng32 ExpHbaseInterface_Thrift::insertRows(
				   HbaseStr &tblName,
				   std::vector<BatchMutation> & rows,
				   const int64_t timestamp,
				   NABoolean autoFlush)
{
  std::string t(tblName.val);
  const std::map<Text, Text>  dummyAttributes; 

  try {
    try {
      if (timestamp == -1)
	{
	  client_->mutateRows(t, rows, dummyAttributes);
	}
      else
	{
	  client_->mutateRowsTs(t, rows, timestamp, dummyAttributes);
	}
    } catch (const TException &tx) {
      std::strcpy(errText_, tx.what());
      return -HBASE_ACCESS_ERROR;
    }
  } catch (const IOError &ioe) {
    std::strcpy(errText_, "FATAL: mutateRows raised IOError");
    return -HBASE_ACCESS_ERROR;
  }

  return 0;
}

Lng32 ExpHbaseInterface_Thrift::setWriteBufferSize(
                HbaseStr &tblName,
                Lng32 size)
{
  return 0;
}
Lng32 ExpHbaseInterface_Thrift::setWriteToWAL(
                HbaseStr &tblName,
                NABoolean v)
{
  return 0;
}

Lng32 ExpHbaseInterface_Thrift::initHBLC()
{

  assert(0);
  return 0;
}
Lng32 ExpHbaseInterface_Thrift::createHFile(HbaseStr &tblName,
                           Text& hFileLoc,
                           Text& hfileName)
{
   assert(0);
   return 0;
}

 Lng32 ExpHbaseInterface_Thrift::addToHFile( HbaseStr &tblName,
                                             std::vector<BatchMutation> & rows)
 {
   assert(0);
   return 0;
 }

 Lng32 ExpHbaseInterface_Thrift::closeHFile(HbaseStr &tblName)
 {
   assert(0);
   return 0;
 }

 Lng32 ExpHbaseInterface_Thrift::doBulkLoad(HbaseStr &tblName,
                          Text& location,
                          Text& tableName,
                          NABoolean quasiSecure,
                          NABoolean snapshot)
 {
   assert(0);
   return 0;
 }

 Lng32 ExpHbaseInterface_Thrift::bulkLoadCleanup(HbaseStr &tblName,
                          Text& location)
 {
   assert(0);
   return 0;
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

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::fetchRowVec(TRowResult& rowResult)
{
  retCode_ = htc_->fetchRowVec(rowResult);
  if (retCode_ == HBC_OK)
    return HBASE_ACCESS_SUCCESS;
  if (retCode_ == HTC_DONE_DATA)
    return HBASE_ACCESS_EOD;
  else if (retCode_ == HTC_DONE_RESULT)
    return HBASE_ACCESS_EOR;
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
   
//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::getRowInfo(
	     HbaseStr &tblName,
	     const Text& row, 
	     NAList<char*> &colNameList,
	     NAList<char*> &colValList)
{
  Lng32 rc = 0;

  rc = init();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  std::vector<Text> columns;
  rc = getRowOpen(tblName, row, columns, -1);
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  TRowResult rowResult;
  rc = fetchRowVec(rowResult);
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  CellMap::const_iterator colIter_ = rowResult.columns.begin();
  while (colIter_ != rowResult.columns.end())
  {
    char * colName = (char*)colIter_->first.c_str();
    char * colVal = (char*)colIter_->second.value.c_str();
  
    char * colNameNAS = new(heap_) char[strlen(colName) + 1];
    char * colValNAS = new(heap_) char[strlen(colVal) + 1];
  
    strcpy(colNameNAS, colName);
    strcpy(colValNAS, colVal);
  
    colNameList.insert(colNameNAS);
    colValList.insert(colValNAS);
    
    colIter_++;
  }

  rc = close();
  if (rc != HBASE_ACCESS_SUCCESS)
    return rc;

  return 0;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::deleteRows(
	  HbaseStr &tblName,
	  std::vector<BatchMutation> & rows,
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->deleteRows(transID, rows, timestamp);

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

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::insertRow(
	  HbaseStr &tblName,
	  HbaseStr &row, 
	  MutationVec & mutations,
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->insertRow(transID, row, mutations, timestamp);

  client_->releaseHTableClient(htc);

  if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

//----------------------------------------------------------------------------
Lng32 ExpHbaseInterface_JNI::insertRows(
	  HbaseStr &tblName,
	  std::vector<BatchMutation> & rows,
	  const int64_t timestamp,
	  NABoolean autoFlush)
{
  if (rows.size() == 0)
    return HBASE_ACCESS_SUCCESS;

  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  retCode_ = htc->insertRows(transID, rows, timestamp, autoFlush);

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

 Lng32 ExpHbaseInterface_JNI::addToHFile( HbaseStr &tblName,
                                          std::vector<BatchMutation> & rows)
 {
   if (hblc_ == NULL || client_ == NULL)
   {
     return -HBASE_ACCESS_ERROR;
   }

   retCode_ = hblc_->addToHFile(tblName, rows);
   //close();
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

  TRowResult rowResult;
  retCode_ = htc->fetchRowVec(rowResult);
  client_->releaseHTableClient(htc);

  if (retCode_ == HBC_OK)
    return 1; // exists
  else if (retCode_ == HTC_DONE)
    return 0; // does not exist
  else
    return -HBASE_ACCESS_ERROR;
}

Lng32 ExpHbaseInterface_JNI::checkAndInsertRow(
	  HbaseStr &tblName,
	  HbaseStr &row, 
	  MutationVec & mutations,
	  const int64_t timestamp)
{
  HTableClient_JNI* htc = client_->getHTableClient((NAHeap *)heap_, tblName.val, useTRex_);
  if (htc == NULL)
  {
    retCode_ = HBC_ERROR_GET_HTC_EXCEPTION;
    return HBASE_OPEN_ERROR;
  }
  
  Int64 transID = getTransactionIDFromContext();
  HTC_RetCode rc = htc->checkAndInsertRow(transID, row, mutations, timestamp);

  client_->releaseHTableClient(htc);

  if (rc == HTC_ERROR_CHECKANDINSERT_DUP_ROWID)
    return HBASE_DUP_ROW_ERROR;

  retCode_ = rc;

   if (retCode_ != HBC_OK)
    return -HBASE_ACCESS_ERROR;
  else
    return HBASE_ACCESS_SUCCESS;
}

Lng32 ExpHbaseInterface_JNI::checkAndUpdateRow(
	  HbaseStr &tblName,
	  HbaseStr &row, 
	  MutationVec & mutations,
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
  HTC_RetCode rc = htc->checkAndUpdateRow(transID, row, mutations, 
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

ByteArrayList* ExpHbaseInterface_Thrift::getRegionInfo(const char* tblName)
{ 
/*
  HTableClient* htc = client_->getHTableClient((NAHeap *)heap_, tblName, useTRex_);
  if (htc == NULL)
    return NULL;

   ByteArrayList* bal = htc_->getEndKeys();
   return bal;
*/
  return NULL;
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
