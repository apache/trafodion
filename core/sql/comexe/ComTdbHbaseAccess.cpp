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

#include "ComTdbHbaseAccess.h"
#include "ComTdbCommon.h"

// Dummy constructor for "unpack" routines.
ComTdbHbaseAccess::ComTdbHbaseAccess():
 ComTdb(ComTdb::ex_HBASE_ACCESS, eye_HBASE_ACCESS)
{};

ComTdbHbaseAccess::ComTdbHbaseAccess(
				     ComTdbAccessType type,
				     char * tableName,
				     
				     ex_expr *convertExpr,
				     ex_expr *scanExpr,
				     ex_expr *rowIdExpr,
				     ex_expr *updateExpr,
				     ex_expr *mergeInsertExpr,
				     ex_expr *mergeInsertRowIdExpr,
				     ex_expr *mergeUpdScanExpr,
				     ex_expr *returnFetchExpr,
				     ex_expr *returnUpdateExpr,
				     ex_expr *returnMergeInsertExpr,
				     ex_expr *encodedKeyExpr,
				     ex_expr *keyColValExpr,
				     ex_expr *hbaseFilterExpr,

				     UInt32 asciiRowLen,
				     UInt32 convertRowLen,
				     UInt32 updateRowLen,
				     UInt32 mergeInsertRowLen,
				     UInt32 returnFetchedRowLen,
				     UInt32 returnUpdatedRowLen,
				     
				     UInt32 rowIdLen,
				     UInt32 outputRowLen,
				     UInt32 rowIdAsciiRowLen,
				     UInt32 keyLen,
				     UInt32 keyColValLen,
				     UInt32 hbaseFilterValRowLen,
				     
				     const UInt16 asciiTuppIndex,
				     const UInt16 convertTuppIndex,
				     const UInt16 updateTuppIndex,
				     const UInt16 mergeInsertTuppIndex,
				     const UInt16 mergeInsertRowIdTuppIndex,
				     const UInt16 returnedFetchedTuppIndex,
				     const UInt16 returnedUpdatedTuppIndex,

				     const UInt16 rowIdTuppIndex,
				     const UInt16 returnedTuppIndex,
				     const UInt16 rowIdAsciiTuppIndex,
				     const UInt16 keyTuppIndex,
				     const UInt16 keyColValTuppIndex,
				     const UInt16 hbaseFilterValTuppIndex,

                                     const UInt16 hbaseTimestampTuppIndex,
                                     const UInt16 hbaseVersionTuppIndex,
 
				     Queue * listOfScanRows,
				     Queue * listOfGetRows,
				     Queue * listOfFetchedColNames,
				     Queue * listOfUpDeldColNames,
				     Queue * listOfMergedColNames,

				     keyRangeGen * keyInfo,
				     char * keyColName,

				     ex_cri_desc *workCriDesc,
				     ex_cri_desc *criDescParentDown,
				     ex_cri_desc *criDescParentUp,
				     queue_index queueSizeDown,
				     queue_index queueSizeUp,
				     Cardinality expectedRows,
				     Lng32 numBuffers,
				     ULng32 bufferSize,
				     char * server,
                                     char * zkPort,
				     HbasePerfAttributes * hbasePerfAttributes,
				     Float32 samplingRate,
				     HbaseSnapshotScanAttributes * hbaseSnapshotScanAttributes,

                                     HbaseAccessOptions * hbaseAccessOptions,

                                     char * pkeyColName

				     )
: ComTdb( ComTdb::ex_HBASE_ACCESS,
	  eye_HBASE_ACCESS,
	  expectedRows,
	  criDescParentDown,
	  criDescParentUp,
	  queueSizeDown,
	  queueSizeUp,
	  numBuffers,  
	  bufferSize),   

  accessType_((UInt16)type),
  tableName_(tableName),

  convertExpr_(convertExpr),
  scanExpr_(scanExpr),
  rowIdExpr_(rowIdExpr),
  updateExpr_(updateExpr),
  mergeInsertExpr_(mergeInsertExpr),
  mergeInsertRowIdExpr_(mergeInsertRowIdExpr),
  mergeUpdScanExpr_(mergeUpdScanExpr),
  returnFetchExpr_(returnFetchExpr),
  returnUpdateExpr_(returnUpdateExpr),
  returnMergeInsertExpr_(returnMergeInsertExpr),
  encodedKeyExpr_(encodedKeyExpr),
  keyColValExpr_(keyColValExpr),
  insDelPreCondExpr_(NULL),
  insConstraintExpr_(NULL),
  updConstraintExpr_(NULL),
  hbaseFilterExpr_(hbaseFilterExpr),

  asciiRowLen_(asciiRowLen),
  convertRowLen_(convertRowLen),
  updateRowLen_(updateRowLen),
  mergeInsertRowLen_(mergeInsertRowLen),
  returnFetchedRowLen_(returnFetchedRowLen),
  returnUpdatedRowLen_(returnUpdatedRowLen),

  rowIdLen_(rowIdLen),
  outputRowLen_(outputRowLen),
  rowIdAsciiRowLen_(rowIdAsciiRowLen),
  keyLen_(keyLen),
  keyColValLen_(keyColValLen),
  hbaseFilterValRowLen_(hbaseFilterValRowLen),

  asciiTuppIndex_(asciiTuppIndex),
  convertTuppIndex_(convertTuppIndex),
  updateTuppIndex_(updateTuppIndex),
  mergeInsertTuppIndex_(mergeInsertTuppIndex),
  mergeInsertRowIdTuppIndex_(mergeInsertRowIdTuppIndex),
  returnedFetchedTuppIndex_(returnedFetchedTuppIndex),
  returnedUpdatedTuppIndex_(returnedUpdatedTuppIndex),

  rowIdTuppIndex_(rowIdTuppIndex),
  returnedTuppIndex_(returnedTuppIndex),
  rowIdAsciiTuppIndex_(rowIdAsciiTuppIndex),
  keyTuppIndex_(keyTuppIndex),
  keyColValTuppIndex_(keyColValTuppIndex),
  hbaseFilterValTuppIndex_(hbaseFilterValTuppIndex),

  hbaseTimestampTuppIndex_(hbaseTimestampTuppIndex),
  hbaseVersionTuppIndex_(hbaseVersionTuppIndex),

  listOfScanRows_(listOfScanRows),
  listOfGetRows_(listOfGetRows),
  listOfFetchedColNames_(listOfFetchedColNames),
  listOfUpDeldColNames_(listOfUpDeldColNames),
  listOfMergedColNames_(listOfMergedColNames),
  listOfIndexesAndTable_(NULL),

  keyInfo_(keyInfo),
  keyColName_(keyColName),

  colFamNameList_(NULL),

  workCriDesc_(workCriDesc),
  flags_(0),
  flags2_(0),
  server_(server),
  zkPort_(zkPort),
  hbasePerfAttributes_(hbasePerfAttributes),
  hbaseSnapshotScanAttributes_(hbaseSnapshotScanAttributes),
  LoadPrepLocation_ (NULL),
  errCountRowId_(NULL),
  errCountTab_(NULL),
  loggingLocation_(NULL),
  samplingRate_(samplingRate),
  sampleLocation_(NULL),
  hbaseRowsetVsbbSize_(0),
  trafLoadFlushSize_(0),
  hbaseAccessOptions_(hbaseAccessOptions),

  pkeyColName_(pkeyColName)
{};

ComTdbHbaseAccess::ComTdbHbaseAccess(
				     ComTdbAccessType type,
				     char * tableName,
				     
				     const UInt16 returnedTuppIndex,
				     Queue * colFamNameList,

				     ex_cri_desc *workCriDesc,
				     ex_cri_desc *criDescParentDown,
				     ex_cri_desc *criDescParentUp,
				     queue_index queueSizeDown,
				     queue_index queueSizeUp,
				     Cardinality expectedRows,
				     Lng32 numBuffers,
				     ULng32 bufferSize,
				     char * server,
                                     char * zkPort
				     )
: ComTdb( ComTdb::ex_HBASE_ACCESS,
	  eye_HBASE_ACCESS,
	  expectedRows,
	  criDescParentDown,
	  criDescParentUp,
	  queueSizeDown,
	  queueSizeUp,
	  numBuffers,  
	  bufferSize),   

  accessType_((UInt16)type),
  tableName_(tableName),

  convertExpr_(NULL),
  scanExpr_(NULL),
  rowIdExpr_(NULL),
  updateExpr_(NULL),
  mergeInsertExpr_(NULL),
  mergeInsertRowIdExpr_(NULL),
  mergeUpdScanExpr_(NULL),
  returnFetchExpr_(NULL),
  returnUpdateExpr_(NULL),
  returnMergeInsertExpr_(NULL),
  encodedKeyExpr_(NULL),
  keyColValExpr_(NULL),
  insDelPreCondExpr_(NULL),
  insConstraintExpr_(NULL),
  updConstraintExpr_(NULL),
  hbaseFilterExpr_(NULL),

  asciiRowLen_(0),
  convertRowLen_(0),
  updateRowLen_(0),
  mergeInsertRowLen_(0),
  returnFetchedRowLen_(0),
  returnUpdatedRowLen_(0),

  rowIdLen_(0),
  outputRowLen_(0),
  rowIdAsciiRowLen_(0),
  keyLen_(0),
  keyColValLen_(0),
  hbaseFilterValRowLen_(0),

  asciiTuppIndex_(0),
  convertTuppIndex_(0),
  updateTuppIndex_(0),
  mergeInsertTuppIndex_(0),
  mergeInsertRowIdTuppIndex_(0),
  returnedFetchedTuppIndex_(0),
  returnedUpdatedTuppIndex_(0),

  rowIdTuppIndex_(0),
  rowIdAsciiTuppIndex_(0),
  keyTuppIndex_(0),
  keyColValTuppIndex_(0),
  hbaseFilterValTuppIndex_(0),

  hbaseTimestampTuppIndex_(0),
  hbaseVersionTuppIndex_(0),

  listOfScanRows_(NULL),
  listOfGetRows_(NULL),
  listOfFetchedColNames_(NULL),
  listOfUpDeldColNames_(NULL),
  listOfMergedColNames_(NULL),
  listOfIndexesAndTable_(NULL),
  listOfOmittedColNames_(NULL),

  keyInfo_(NULL),
  keyColName_(NULL),

  returnedTuppIndex_(returnedTuppIndex),

  colFamNameList_(colFamNameList),

  workCriDesc_(workCriDesc),
  flags_(0),
  flags2_(0),
  server_(server),
  zkPort_(zkPort),

  hbasePerfAttributes_(NULL),
  hbaseSnapshotScanAttributes_(NULL),
  LoadPrepLocation_(NULL),
  samplingRate_(-1),
  sampleLocation_(NULL),
  hbaseRowsetVsbbSize_(0),
  trafLoadFlushSize_(0),
  hbaseAccessOptions_(NULL),

  pkeyColName_(NULL)
{
}

ComTdbHbaseAccess::~ComTdbHbaseAccess()
{};

// Return the number of expressions held by the explain TDB (2)
// They are enumerated as: 0 - scanPred, 1 - paramsExpr
Int32
ComTdbHbaseAccess::numExpressions() const
{
  return 18;
}
 
// Return the expression names of the explain TDB based on some 
// enumeration. 0 - scanPred, 1 - paramsExpr
const char *
ComTdbHbaseAccess::getExpressionName(Int32 expNum) const
{
  switch(expNum)
    {
    case 0:
      return "Convert Expr";
    case 1:
      return "ScanExpr";
    case 2:
      return "RowIdExpr";
    case 3: 
      return "UpdateExpr";
    case 4: 
      return ((getAccessType() == DELETE_) ? "LobDeleteExpr" : "MergeInsertExpr");
    case 5:
      return "LowKeyExpr";
    case 6:
      return "HighKeyExpr";
    case 7:
      return "ReturnFetchExpr";
    case 8:
      return "ReturnUpdateExpr";
    case 9:
      return "ReturnMergeInsertExpr";
    case 10:
      return "mergeUpdScanExpr";
    case 11:
      return "mergeInsertRowIdExpr";
    case 12:
      return "encodedKeyExpr";
    case 13:
      return "keyColValExpr";
    case 14:
      return "hbaseFilterExpr";
    case 15:
      return "preCondExpr";
    case 16:
      return "insConstraintExpr";
    case 17:
      return "updConstraintExpr";
    default:
      return 0;
    }  
}

// Return the expressions of the explain TDB based on some 
// enumeration. 0 - scanPred, 1 - paramsExpr
ex_expr *
ComTdbHbaseAccess::getExpressionNode(Int32 expNum)
{
  switch(expNum)
    {
    case 0:
      return convertExpr_;
    case 1:
      return scanExpr_;
    case 2:
      return rowIdExpr_;
    case 3:
      return updateExpr_;
    case 4:
      return mergeInsertExpr_;
    case 5:if (keyInfo_) return keyInfo_->getExpressionNode(0);
      else return NULL;
    case 6:if (keyInfo_) return keyInfo_->getExpressionNode(1);
      else return NULL;
    case 7:
      return returnFetchExpr_;
    case 8:
      return returnUpdateExpr_;
    case 9:
      return returnMergeInsertExpr_;
    case 10:
      return mergeUpdScanExpr_;
    case 11:
      return mergeInsertRowIdExpr_;
    case 12:
      return encodedKeyExpr_;
    case 13:
      return keyColValExpr_;
    case 14:
      return hbaseFilterExpr_;
    case 15:
      return insDelPreCondExpr_;
    case 16:
      return insConstraintExpr_;
    case 17:
      return updConstraintExpr_;
    default:
      return NULL;
    }  
}

Long ComTdbHbaseAccess::pack(void * space)
{
  tableName_.pack(space);
  convertExpr_.pack(space);
  updateExpr_.pack(space);
  mergeInsertExpr_.pack(space);
  mergeInsertRowIdExpr_.pack(space);
  mergeUpdScanExpr_.pack(space);
  returnFetchExpr_.pack(space);
  returnUpdateExpr_.pack(space);
  returnMergeInsertExpr_.pack(space);
  scanExpr_.pack(space);
  rowIdExpr_.pack(space);
  encodedKeyExpr_.pack(space);
  keyColValExpr_.pack(space);
  insDelPreCondExpr_.pack(space);
  insConstraintExpr_.pack(space);
  updConstraintExpr_.pack(space);
  hbaseFilterExpr_.pack(space);
  colFamNameList_.pack(space);
  workCriDesc_.pack(space);
  listOfFetchedColNames_.pack(space);
  listOfUpDeldColNames_.pack(space);
  listOfMergedColNames_.pack(space);
  listOfOmittedColNames_.pack(space);
  listOfIndexesAndTable_.pack(space);
  keyInfo_.pack(space);
  keyColName_.pack(space);
  server_.pack(space);
  zkPort_.pack(space);
  hbasePerfAttributes_.pack(space);
  sampleLocation_.pack(space);
  LoadPrepLocation_.pack(space);
  hbaseSnapshotScanAttributes_.pack(space);
  hbaseAccessOptions_.pack(space);

  pkeyColName_.pack(space);

  // pack elements in listOfScanRows_
  if (listOfScanRows() && listOfScanRows()->numEntries() > 0)
    {
      listOfScanRows()->position();
      for (Lng32 i = 0; i < listOfScanRows()->numEntries(); i++)
	{
	  HbaseScanRows * hsr = (HbaseScanRows*)listOfScanRows()->getNext();
	  //	  hsr->pack(space);
	  hsr->beginRowId_.pack(space);
	  hsr->endRowId_.pack(space);
	  hsr->colNames_.pack(space);
	}
    }
  listOfScanRows_.pack(space);

 // pack elements in listOfGetRows_
  if (listOfGetRows() && listOfGetRows()->numEntries() > 0)
    {
      listOfGetRows()->position();
      for (Lng32 i = 0; i < listOfGetRows()->numEntries(); i++)
	{
	  HbaseGetRows * hgr = (HbaseGetRows*)listOfGetRows()->getNext();
	  //	  hgr->pack(space);
	  hgr->rowIds_.pack(space);
	  hgr->colNames_.pack(space);
	}
    }
  listOfGetRows_.pack(space);
  errCountRowId_.pack(space);
  errCountTab_.pack(space);
  loggingLocation_.pack(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbHbaseAccess::unpack(void * base, void * reallocator)
{
  if(tableName_.unpack(base)) return -1;
  if(convertExpr_.unpack(base, reallocator)) return -1;
  if(updateExpr_.unpack(base, reallocator)) return -1;
  if(mergeInsertExpr_.unpack(base, reallocator)) return -1;
  if(mergeInsertRowIdExpr_.unpack(base, reallocator)) return -1;
  if(returnFetchExpr_.unpack(base, reallocator)) return -1;
  if(returnUpdateExpr_.unpack(base, reallocator)) return -1;
  if(returnMergeInsertExpr_.unpack(base, reallocator)) return -1;
  if(mergeUpdScanExpr_.unpack(base, reallocator)) return -1;
  if(scanExpr_.unpack(base, reallocator)) return -1;
  if(rowIdExpr_.unpack(base, reallocator)) return -1;
  if(encodedKeyExpr_.unpack(base, reallocator)) return -1;
  if(keyColValExpr_.unpack(base, reallocator)) return -1;
  if(insDelPreCondExpr_.unpack(base, reallocator)) return -1;
  if(insConstraintExpr_.unpack(base, reallocator)) return -1;
  if(updConstraintExpr_.unpack(base, reallocator)) return -1;
  if(hbaseFilterExpr_.unpack(base, reallocator)) return -1;
  if(colFamNameList_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(listOfFetchedColNames_.unpack(base, reallocator)) return -1;
  if(listOfUpDeldColNames_.unpack(base, reallocator)) return -1;
  if(listOfMergedColNames_.unpack(base, reallocator)) return -1;
  if(listOfOmittedColNames_.unpack(base, reallocator)) return -1;
  if(listOfIndexesAndTable_.unpack(base, reallocator)) return -1;
  if(keyInfo_.unpack(base, reallocator)) return -1;
  if(keyColName_.unpack(base)) return -1;
  if(server_.unpack(base)) return -1;
  if(zkPort_.unpack(base)) return -1;
  if(hbasePerfAttributes_.unpack(base, reallocator)) return -1;
  if(sampleLocation_.unpack(base)) return -1;
  if(LoadPrepLocation_.unpack(base)) return -1;
  if (hbaseSnapshotScanAttributes_.unpack(base,reallocator)) return -1;
  if (hbaseAccessOptions_.unpack(base, reallocator)) return -1;
  if(pkeyColName_.unpack(base)) return -1;

  // unpack elements in listOfScanRows_
  if(listOfScanRows_.unpack(base, reallocator)) return -1;
  if (listOfScanRows() && listOfScanRows()->numEntries() > 0)
    {
      listOfScanRows()->position();
      for (Lng32 i = 0; i < listOfScanRows()->numEntries(); i++)
	{
	  HbaseScanRows * hsr = (HbaseScanRows*)listOfScanRows()->getNext();

	  if (hsr->beginRowId_.unpack(base)) return -1;
	  if (hsr->endRowId_.unpack(base)) return -1;
	  if (hsr->colNames_.unpack(base, reallocator)) return -1;
	  //	  if (hsr->unpack(base, reallocator)) return -1;
	}
    }

  // unpack elements in listOfGetRows_
  if(listOfGetRows_.unpack(base, reallocator)) return -1;
  if (listOfGetRows() && listOfGetRows()->numEntries() > 0)
    {
      listOfGetRows()->position();
      for (Lng32 i = 0; i < listOfGetRows()->numEntries(); i++)
	{
	  HbaseGetRows * hgr = (HbaseGetRows*)listOfGetRows()->getNext();
	  if (hgr->rowIds_.unpack(base, reallocator)) return -1;
	  if (hgr->colNames_.unpack(base, reallocator)) return -1;
	  //	  if (hgr->unpack(base, reallocator)) return -1;
	}
    }
  if (errCountRowId_.unpack(base)) return -1;
  if (errCountTab_.unpack(base)) return -1;
  if (loggingLocation_.unpack(base)) return -1;

  return ComTdb::unpack(base, reallocator);
}

void ComTdbHbaseAccess::displayRowId(Space * space, char * inputRowIdBuf)
{
  char buf[100];
  char keyVal[41];
  Lng32 keyValLen = 0;
  
  ExpTupleDesc * asciiSourceTD =
    workCriDesc_->getTupleDescriptor(rowIdAsciiTuppIndex_);
  
  Lng32 currPos = 0;
  if (asciiSourceTD)
    {
      for (CollIndex i = 0; i < asciiSourceTD->numAttrs(); i++)
	{
	  Attributes * attr = asciiSourceTD->getAttr(i);
	  
	  short inputRowIdValLen = *(short*)&inputRowIdBuf[currPos];
	  currPos += sizeof(short);
	  
	  if (inputRowIdValLen > 0)
	    {
	      NABoolean nullVal = FALSE;
	      if (attr->getNullFlag())
		{
		  if (*(short*)&inputRowIdBuf[currPos] != 0) // null val
		    {
		      nullVal = TRUE;
		      strcpy(keyVal, "NULL");
		      keyValLen = strlen("NULL");
		    }
		  else
		    keyValLen = inputRowIdValLen - sizeof(short);
		  
		  currPos += sizeof(short);
		}
	      else
		keyValLen = inputRowIdValLen;
	  
	      if (NOT nullVal)
		{
		  const char * inputRowIdVal = &inputRowIdBuf[currPos];
		  // print max 20 bytes from the key value
		  Int32 fieldWidth = (MINOF(keyValLen, 20) + 1) / 2;
		  for (Int32 idx = 0; idx < fieldWidth; idx++)
		    {
		      // print each byte in 2-digit hex value
		      sprintf(keyVal + 2*idx, "%02x", *(inputRowIdVal + idx));
		    }
		  keyVal[fieldWidth*2] = 0;  // null terminate
		}
	    }
	  else
	    {
	      keyValLen = 0;
	      strcpy(keyVal, "<missing>");
	    }

	  keyValLen = MINOF(keyValLen, 40);
	  keyVal[keyValLen] = 0;
	  str_sprintf(buf, "        %d:%s", keyValLen, keyVal);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  
	  currPos += inputRowIdValLen;
	}
    }
}

static void showColNames(Queue * listOfColNames, Space * space)
{
  char buf[1000];

  listOfColNames->position();
  for (Lng32 j = 0; j < listOfColNames->numEntries(); j++)
    {
      char * currPtr = (char*)listOfColNames->getCurr();
      
      Lng32 currPos = 0;
      Lng32 jj = 0;
      short colNameLen = *(short*)currPtr;
      currPos += sizeof(short);
      char colFam[100];
      while (currPtr[currPos] != ':')
	{
	  currPos++;
	  jj++;
	}
      jj++;
      currPos++;
      snprintf(colFam,sizeof(colFam),"%.*s",jj,currPtr+sizeof(short));
      colNameLen -= jj;
      
      NABoolean withAt = FALSE;
      char * colName = &currPtr[currPos];
      if (colName[0] == '@')
	{
	  colNameLen--;
	  colName++;
	  withAt = TRUE;
	}
      
      Int64 v;
      if (colNameLen == sizeof(char))
	v = *(char*)colName;
      else if (colNameLen == sizeof(unsigned short))
	v = *(UInt16*)colName;
      else if (colNameLen == sizeof(Lng32))
	v = *(ULng32*)colName;
      else
	v = 0;
      
      str_sprintf(buf, "  Entry #%d: %s%s%Ld",
		  j+1, 
		  colFam, 
		  (withAt ? "@" : ""),
		  v);
      
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      listOfColNames->advance();
    } // for
}

static void showStrColNames(Queue * listOfColNames, Space * space,
			    NABoolean nullTerminated = FALSE)
{
  char buf[1000];

  listOfColNames->position();
  for (Lng32 j = 0; j < listOfColNames->numEntries(); j++)
    {
      char * currPtr = (char*)listOfColNames->getCurr();
      
      char * colNamePtr = NULL;
      if (nullTerminated)
	{
	  colNamePtr = currPtr;
	}
      else
	{
	  short colNameLen = *(short*)currPtr;
	  char colName[500];
	  snprintf(colName,sizeof(colName),"%.*s",colNameLen,currPtr+sizeof(short));
	  colNamePtr = colName;
	}

      str_sprintf(buf, "  Entry #%d: %s",
		  j+1, 
		  colNamePtr);
      
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      listOfColNames->advance();
    } // for
}

const char *
ComTdbHbaseAccess::getNodeName() const
{
  if ((sqHbaseTable()))
    {
      switch (accessType_)
        {
          case  SELECT_:
            {
              if (keyMDAMGen())
                {
                  if ((! listOfGetRows()) &&
                      (! listOfScanRows()))
                    return (rowsetOper()? "EX_TRAF_MDAM_ROWSET_SELECT":
                                          "EX_TRAF_MDAM_SELECT");
                  // else?
                }
              else if (keyInfo_)
                return (rowsetOper()? "EX_TRAF_KEY_ROWSET_SELECT":
                                      "EX_TRAF_KEY_SELECT");
              else
                return (rowsetOper()? "EX_TRAF_ROWSET_SELECT":
                                      "EX_TRAF_SELECT");
            }
            break;

          case INSERT_:
            {
              if ((vsbbInsert()) && (NOT hbaseSqlIUD()))
                return ("EX_TRAF_VSBB_UPSERT");
              else
                return ("EX_TRAF_INSERT");
            }
            break;

          case UPSERT_:
            {
              if ((vsbbInsert()) && (NOT hbaseSqlIUD()))
                return ("EX_TRAF_VSBB_UPSERT");
              else
                return ("EX_TRAF_UPSERT");
            }
            break;

          case UPSERT_LOAD_:
            {
              if (getIsTrafodionLoadPrep())
                return ("EX_TRAF_LOAD_PREPARATION");
              else if ((vsbbInsert()) && (NOT hbaseSqlIUD()))
                return ("EX_TRAF_VSBB_UPSERT_LOAD");
              else
                return ("EX_TRAF_UPSERT_LOAD");
            }
            break;

          case UPDATE_:
            return (rowsetOper()? "EX_TRAF_VSBB_UPDATE": "EX_TRAF_UPDATE");
            break;

          case MERGE_:
            return (rowsetOper()? "EX_TRAF_VSBB_MERGE": "EX_TRAF_MERGE");
            break;

          case DELETE_:
            return (rowsetOper()? "EX_TRAF_VSBB_DELETE": "EX_TRAF_DELETE");
            break;

          case COPROC_:
            return ("EX_TRAF_COPROC_AGGR");
            break;

          default:
            // any other Trafodion table operations
            return ("EX_TRAF_ACCESS");
        }  // switch accessType_
    }  // isHbaseTable

  // other (or non-trafodion) operations
  switch (accessType_)
    {
      case SELECT_:
        {
          if (keyMDAMGen())
            {
              // must be SQ Seabase table and no listOfScan/Get keys
              if ((! listOfGetRows()) &&
                  (! listOfScanRows()))
                return ("EX_HBASE_MDAM_SELECT");
              // missing else?
            }
          else if (keyInfo_)
            return ("EX_HBASE_KEY_SELECT");
          else
            return ("EX_HBASE_SELECT");
        }
	break;

      case INSERT_:
        {
          if ((vsbbInsert()) && (NOT hbaseSqlIUD()))
            return ("EX_HBASE_VSBB_INSERT");
          else
	    return ("EX_HBASE_INSERT");
        }
        break;

      case UPSERT_:
        {
          if ((vsbbInsert()) && (NOT hbaseSqlIUD()))
            return ("EX_HBASE_VSBB_UPSERT");
          else
            return ("EX_HBASE_UPSERT");
        }
        break;

      case UPSERT_LOAD_:
        {
          if ((vsbbInsert()) && (NOT hbaseSqlIUD()))
            return ("EX_HBASE_VSBB_UPSERT_LOAD");
          else
            return ("EX_HBASE_UPSERT_LOAD");
        }
        break;

      case UPDATE_:
        return (rowsetOper()? "EX_HBASE_VSBB_UPDATE": "EX_HBASE_UPDATE");
        break;

      case MERGE_:
        return (rowsetOper()? "EX_HBASE_VSBB_MERGE": "EX_HBASE_MERGE");
        break;

      case DELETE_:
        return (rowsetOper()? "EX_HBASE_VSBB_DELETE": "EX_HBASE_DELETE");
        break;

      case COPROC_:
        return ("EX_HBASE_COPROC_AGGR");
        break;

      case CREATE_:
        return ("EX_TRAF_CREATE");
        break;

      case DROP_:
        return ("EX_TRAF_DROP");
        break;

      case GET_TABLES_:
        return ("EX_TRAF_GET_TABLES");
        break;

      case INIT_MD_:
        return ("EX_INIT_TRAF_METADATA");
        break;

      case DROP_MD_:
        return ("EX_DROP_TRAF_METADATA");
        break;

      case UPGRADE_MD_:
        return ("EX_UPGRADE_TRAF_METADATA");
        break;

      case BULK_LOAD_PREP_:
        return ("EX_TRAF_BULK_LOAD_PREP");
        break;

      case BULK_LOAD_TASK_:
        return ("EX_TRAF_BULK_LOAD_TASK");
        break;

    }

  // all else
  return ("EX_HBASE_ACCESS");  // default name 
}

void ComTdbHbaseAccess::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

  if(flag & 0x00000008)
    {
      char buf[1000];

      str_sprintf(buf, "\nFor ComTdbHbaseAccess :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "accessType_ = %s", (char*)getAccessTypeStr(accessType_));
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "accessDetail_ = %s", getNodeName());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (samplingRate_ > 0)
        {
          // str_printf does not handle %f correctly, format as string first.
          char sbuf[20];
          snprintf(sbuf, sizeof(sbuf), "%f", samplingRate_);
          str_sprintf(buf, "samplingRate_ = %s", sbuf);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      if (tableName_)
	{
	  str_sprintf(buf, "tableName_ = %s", (char*)tableName_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      str_sprintf(buf, "asciiTI_ = %d, convertTI_ = %d, rowIdTI_ = %d, returnedTI_ = %d",
		  asciiTuppIndex_, convertTuppIndex_, rowIdTuppIndex_, returnedTuppIndex_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "rowIdAsciiTI_ = %d, updateTI_ = %d, mergeInsertTI_ = %d",
		  rowIdAsciiTuppIndex_, updateTuppIndex_, mergeInsertTuppIndex_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "returnedFetchedTI_ = %d, returnedUpdatedTI_ = %d, mergeInsertRowIdTI_ = %d",
		  returnedFetchedTuppIndex_, returnedUpdatedTuppIndex_,
		  mergeInsertRowIdTuppIndex_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "hbaseTimestampTI_ = %d, hbaseVersionTI_ = %d",
                  hbaseTimestampTuppIndex_, hbaseVersionTuppIndex_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "asciiRowLen_ = %d, convertRowLen_ = %d, rowIdLen_ = %d, outputRowLen_ = %d", 
		  asciiRowLen_, convertRowLen_, rowIdLen_, outputRowLen_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "updateRowLen_ = %d, returnFetchedRowLen_ = %d, returnUpdateedRowLen_ = %d", 
		  updateRowLen_, returnFetchedRowLen_, returnUpdatedRowLen_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "mergeInsertRowLen_ = %d, keyLen_ = %d", 
		  mergeInsertRowLen_, keyLen_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "Flag = %b",flags_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "server_ = %s, zkPort_ = %s", server(), zkPort());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if ((getHbaseAccessOptions()) && (getHbaseAccessOptions()->multiVersions()))
        {
          str_sprintf(buf, "numVersions = %d", getHbaseAccessOptions()->getNumVersions());
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      if (listOfFetchedColNames())
	{
	  str_sprintf(buf, "\nlistOfFetchedColNames_(numEntries = %d):\n",
		      listOfFetchedColNames()->numEntries());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	  if ((sqHbaseTable()) && (NOT hbaseMapTable()))
	    showColNames(listOfFetchedColNames(), space);
	  else
	    showStrColNames(listOfFetchedColNames(), space);
	}

      if (listOfUpDeldColNames())
	{
	  str_sprintf(buf, "\nlistOfUpDeldColNames_(numEntries = %d):\n",
		      listOfUpDeldColNames()->numEntries());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  
	  if (sqHbaseTable())
	    showColNames(listOfUpDeldColNames(), space);
	  else
	    showStrColNames(listOfUpDeldColNames(), space);

	  /*
	  if (updelColnameIsStr())
	    showStrColNames(listOfUpDeldColNames(), space);
	  else
	    showColNames(listOfUpDeldColNames(), space);
	  */
	}

      if (0)//listOfMergedColNames())
	{
	  str_sprintf(buf, "\nlistOfMergedColNames_(numEntries = %d):\n",
		      listOfMergedColNames()->numEntries());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  
	  showColNames(listOfMergedColNames(), space);
	}

     if (listOfScanRows())
	{
	  str_sprintf(buf, "\nlistOfScanRows_(numEntries = %d):",
		      listOfScanRows()->numEntries());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	  listOfScanRows()->position();
	  for (Lng32 i = 0; i < listOfScanRows()->numEntries(); i++)
	    {
	      HbaseScanRows * hsr = (HbaseScanRows*)listOfScanRows()->getNext();

	      str_sprintf(buf, "\n  Entry #%d:", i+1);
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	      str_sprintf(buf, "    beginRowId_%s = ",
			  (hsr->beginKeyExclusive_ ? "(excl)" : "(incl)")); 
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	      
	      displayRowId(space, hsr->beginRowId());
	      
	      str_sprintf(buf, "    endRowId_%s = ",
			  (hsr->endKeyExclusive_ ? "(excl)" : "(incl)")); 			  
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	      displayRowId(space, hsr->endRowId());

	      if (0) //hsr->colNames())
		{
		  str_sprintf(buf, "\n    colNames_(numEntries = %d):",
			      hsr->colNames()->numEntries());
		  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		  
		  hsr->colNames()->position();
		  for (Lng32 j = 0; j < hsr->colNames()->numEntries(); j++)
		    {
		      str_sprintf(buf, "\n      Entry #%d:", j+1);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		      
		      char * colName = (char*)hsr->colNames()->getNext();
		      str_sprintf(buf, "        colName='%s'", colName);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		    } // for
		} // if colNames

	      str_sprintf(buf, "\n    colTS_=%Ld",
			  hsr->colTS_);
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	    } // for
	} // if

      if (listOfGetRows())
	{
	  str_sprintf(buf, "\nlistOfGetRows_(numEntries = %d):",
		      listOfGetRows()->numEntries());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	  listOfGetRows()->position();
	  for (Lng32 i = 0; i < listOfGetRows()->numEntries(); i++)
	    {
	      HbaseGetRows * hgr = (HbaseGetRows*)listOfGetRows()->getNext();

	      str_sprintf(buf, "\n  Entry #%d:", i+1);
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	      if (hgr->rowIds())
		{
		  str_sprintf(buf, "\n    rowIds_(numEntries = %d):",
			      hgr->rowIds()->numEntries());
		  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		  
		  hgr->rowIds()->position();
		  for (Lng32 j = 0; j < hgr->rowIds()->numEntries(); j++)
		    {
		      str_sprintf(buf, "      Entry #%d:", j+1);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		      
		      char * rowId = (char*)hgr->rowIds()->getNext();

		      ExpTupleDesc * asciiSourceTD =
			workCriDesc_->getTupleDescriptor(rowIdAsciiTuppIndex_);
		      
		      str_sprintf(buf, "      rowId_= ");
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

		      displayRowId(space, rowId);
		    } // for
		  
		} // if

	      if (0) //hgr->colNames())
		{
		  str_sprintf(buf, "\n    colNames_(numEntries = %d):",
			      hgr->colNames()->numEntries());
		  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		  
		  hgr->colNames()->position();
		  for (Lng32 j = 0; j < hgr->colNames()->numEntries(); j++)
		    {
		      str_sprintf(buf, "      Entry #%d:", j+1);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		      
		      char * colName = (char*)hgr->colNames()->getNext();
		      str_sprintf(buf, "        colName='%s'", colName);
		      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
		    } // for
		} // if

	      str_sprintf(buf, "\n    colTS_=%Ld",
			  hgr->colTS_);
	      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	    } // for
	} // if
      if (getHbaseSnapshotScanAttributes() &&
          getHbaseSnapshotScanAttributes()->getUseSnapshotScan())
      {
        str_sprintf(buf, "use_snapshot_scan = %s, snapshot_name = %s, snapshot_temp_location = %s",
                    "TRUE",
                    getHbaseSnapshotScanAttributes()->getSnapshotName(),
                    getHbaseSnapshotScanAttributes()->getSnapScanTmpLocation());
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      }
      
    }
  
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

/////////////////////////////////////////////////
// class ComTdbHbaseCoProcAccess
/////////////////////////////////////////////////
ComTdbHbaseCoProcAccess::ComTdbHbaseCoProcAccess():
  ComTdbHbaseAccess()
{
};

ComTdbHbaseCoProcAccess::ComTdbHbaseCoProcAccess(
						 char * tableName,
						 CoProcType type,
						 
						 ex_expr * projExpr,
						 UInt32 projRowLen,
						 const UInt16 projTuppIndex,
						 const UInt16 returnedTuppIndex, 

						 Queue * listOfColNames,

						 ex_cri_desc *workCriDesc,
						 ex_cri_desc *criDescParentDown,
						 ex_cri_desc *criDescParentUp,
						 queue_index queueSizeDown,
						 queue_index queueSizeUp,
						 Cardinality expectedRows,
						 Lng32 numBuffers,
						 ULng32 bufferSize,
						 char * server,
						 char * zkPort,
						 HbasePerfAttributes * hbasePerfAttributes
						 )
  : ComTdbHbaseAccess(COPROC_,
		      tableName,
		      projExpr,
		      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		      0, projRowLen, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		      0, projTuppIndex, 0, 0, 0, 0, 0, 0, returnedTuppIndex, 0, 0, 0, 0, 0, 0,
		      NULL, NULL, listOfColNames, NULL, NULL, 
		      NULL, NULL,
		      workCriDesc,
		      criDescParentDown,
		      criDescParentUp,
		      queueSizeDown,
		      queueSizeUp,
		      expectedRows,
		      numBuffers,  
		      bufferSize,
		      server, zkPort,
		      hbasePerfAttributes),
  coProcType_((UInt16)type)
{
}

/////////////////////////////////////////////////
// class ComTdbHbaseCoProcAggr
/////////////////////////////////////////////////
ComTdbHbaseCoProcAggr::ComTdbHbaseCoProcAggr():
  ComTdbHbaseCoProcAccess()
{
  setNodeType(ex_HBASE_COPROC_AGGR);
  setEyeCatcher(eye_HBASE_COPROC_AGGR);
};

ComTdbHbaseCoProcAggr::ComTdbHbaseCoProcAggr(
						 char * tableName,

						 ex_expr * projExpr,
						 UInt32 projRowLen,
						 const UInt16 projTuppIndex,
						 const UInt16 returnedTuppIndex, 

						 Queue * listOfAggrTypes,
						 Queue * listOfAggrColNames,

						 ex_cri_desc *workCriDesc,
						 ex_cri_desc *criDescParentDown,
						 ex_cri_desc *criDescParentUp,
						 queue_index queueSizeDown,
						 queue_index queueSizeUp,
						 Cardinality expectedRows,
						 Lng32 numBuffers,
						 ULng32 bufferSize,
						 char * server,
						 char * zkPort,
						 HbasePerfAttributes * hbasePerfAttributes
						 )
  : ComTdbHbaseCoProcAccess(
			    tableName,
			    ComTdbHbaseCoProcAccess::AGGR_,
			    projExpr,
			    projRowLen,
			    projTuppIndex, 
			    returnedTuppIndex, 
			    listOfAggrColNames,
			    workCriDesc,
			    criDescParentDown,
			    criDescParentUp,
			    queueSizeDown,
			    queueSizeUp,
			    expectedRows,
			    numBuffers,  
			    bufferSize,
			    server, zkPort,
			    hbasePerfAttributes),
    listOfAggrTypes_(listOfAggrTypes)
{
  setNodeType(ex_HBASE_COPROC_AGGR);
  setEyeCatcher(eye_HBASE_COPROC_AGGR);
}

Long ComTdbHbaseCoProcAggr::pack(void * space)
{
  listOfAggrTypes_.pack(space);

  return ComTdbHbaseCoProcAccess::pack(space);
}

Lng32 ComTdbHbaseCoProcAggr::unpack(void * base, void * reallocator)
{
  if (listOfAggrTypes_.unpack(base, reallocator)) return -1;

  return ComTdbHbaseCoProcAccess::unpack(base, reallocator);
}

///////////////////////////////////////////////////////////////////
// ComTdbHbaseAccess::HbaseScanRows
///////////////////////////////////////////////////////////////////
Long ComTdbHbaseAccess::HbaseScanRows::pack(void * space)
{
  if (beginRowId_)
    beginRowId_.pack(space);

  if (endRowId_)
    endRowId_.pack(space);

  colNames_.pack(space);

  return NAVersionedObject::pack(space);
}

Lng32 ComTdbHbaseAccess::HbaseScanRows::unpack(void * base, void * reallocator)
{
  if(beginRowId_.unpack(base)) return -1;
  if(endRowId_.unpack(base)) return -1;

  if(colNames_.unpack(base, reallocator)) return -1;

  return NAVersionedObject::unpack(base, reallocator);
}

Long ComTdbHbaseAccess::HbaseGetRows::pack(void * space)
{
  rowIds_.pack(space);
  colNames_.pack(space);

  return NAVersionedObject::pack(space);
}

Lng32 ComTdbHbaseAccess::HbaseGetRows::unpack(void * base, void * reallocator)
{
  if(rowIds_.unpack(base, reallocator)) return -1;
  if(colNames_.unpack(base, reallocator)) return -1;

  return NAVersionedObject::unpack(base, reallocator);
}

Long ComTdbHbaseAccess::HbasePerfAttributes::pack(void * space)
{
  return NAVersionedObject::pack(space);
}

Lng32 ComTdbHbaseAccess::HbasePerfAttributes::unpack(void * base, void * reallocator)
{
  return NAVersionedObject::unpack(base, reallocator);
}
///////////////////////////////////////////////////////////////////
// ComTdbHbaseAccess::HbaseSnapshotScanAttributes
///////////////////////////////////////////////////////////////////
Long ComTdbHbaseAccess::HbaseSnapshotScanAttributes::pack(void * space)
{
  if (snapScanTmpLocation_)
    snapScanTmpLocation_.pack(space);
  if (snapshotName_)
    snapshotName_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 ComTdbHbaseAccess::HbaseSnapshotScanAttributes::unpack(void * base, void * reallocator)
{
  if(snapScanTmpLocation_.unpack(base)) return -1;
  if(snapshotName_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}
