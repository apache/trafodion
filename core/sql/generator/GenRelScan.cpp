/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         GenRelScan.C
* Description:  Scan operators
*               
* Created:      5/17/94
* Language:     C++
*
*
******************************************************************************
*/

#include <algorithm>
#include <vector>

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "ControlDB.h"
#include "GenExpGenerator.h"
#include "ComTdbHdfsScan.h"
#include "ComTdbDDL.h"                  // for describe
#include "ComTdbHbaseAccess.h"
#include "HashRow.h"                    // for sizeof(HashRow)
#include "sql_buffer.h"
#include "sql_buffer_size.h"
#include "OptimizerSimulator.h"
#include "RelUpdate.h"
#include "HDFSHook.h"
#include "CmpSeabaseDDL.h"
#include "TrafDDLdesc.h"


/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   Describe::codeGen()
//   DP2Scan::codeGen()
//   FileScan::codeGen()
//
//////////////////////////////////////////////////////////////////////


short Describe::codeGen(Generator * generator)
{ 
  Space * space          = generator->getSpace();
  ExpGenerator * exp_gen = generator->getExpGenerator();
  
  // allocate a map table for the retrieved columns
  generator->appendAtEnd();
  
  ex_cri_desc * given_desc 
    = generator->getCriDesc(Generator::DOWN);
  
  ex_cri_desc * returned_desc 
    = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);

  ExpTupleDesc * tuple_desc = 0;
  ULng32 tupleLength;
  exp_gen->processValIdList(getTableDesc()->getClusteringIndex()->getIndexColumns(),
                            ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
			    tupleLength,
			    0, 
			    // where the fetched row will be
			    returned_desc->noTuples()-1, 
			    &tuple_desc, ExpTupleDesc::SHORT_FORMAT);

  // add this descriptor to the returned cri descriptor.
  returned_desc->setTupleDescriptor(returned_desc->noTuples()-1, tuple_desc);
  
  char * query = 
    space->allocateAndCopyToAlignedSpace(originalQuery_, 
					 strlen(originalQuery_),
					 0);

  ComTdbDescribe::DescribeType type = ComTdbDescribe::INVOKE_;
  ULng32 flags = 0;
  if (format_ == INVOKE_) type = ComTdbDescribe::INVOKE_;
  else if (format_ == SHOWSTATS_) type = ComTdbDescribe::SHOWSTATS_;
  else if (format_ == TRANSACTION_) type = ComTdbDescribe::TRANSACTION_;
  else if (format_ == SHORT_) type = ComTdbDescribe::SHORT_;
  else if (format_ == SHOWDDL_) type = ComTdbDescribe::LONG_;
  else if (format_ == PLAN_) type = ComTdbDescribe::PLAN_;
  else if (format_ == LABEL_) type = ComTdbDescribe::LABEL_;
  else if (format_ == SHAPE_) type = ComTdbDescribe::SHAPE_;
  else if (format_ == ENVVARS_) type = ComTdbDescribe::ENVVARS_;
  else if (format_ == LEAKS_) 
    {
      type = ComTdbDescribe::LEAKS_;
#ifdef NA_DEBUG_HEAPLOG	
      flags = flags_;
#else
      flags = 0;
#endif
    }

  ComTdbDescribe * desc_tdb = new(space) 
    ComTdbDescribe(query,
	          strlen(query),
	          (Int16)SQLCHARSETCODE_UTF8,
	          0, 0,
	          0,
	          tupleLength,
	          0, 0,
	          type,
		  flags,
	          given_desc,
	          returned_desc,
	          (queue_index)getDefault(GEN_DESC_SIZE_DOWN),
	          (queue_index)getDefault(GEN_DESC_SIZE_UP),
	          getDefault(GEN_DESC_NUM_BUFFERS),
	          getDefault(GEN_DESC_BUFFER_SIZE));
  generator->initTdbFields(desc_tdb);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(desc_tdb, 0, 0, generator));
  }

  generator->setCriDesc(returned_desc, Generator::UP);
  generator->setGenObj(this, desc_tdb);

  // Do not infer that any transaction started can 
  // be in READ ONLY mode for showddl and invoke.
  // This causes trouble for MP table access.
  generator->setNeedsReadWriteTransaction(TRUE);

  // don't need a transaction for showtransaction statement.

  if (maybeInMD())
  {
    BindWA * bindWA = generator->getBindWA();
    CorrName descCorrName = getDescribedTableName();
    Lng32 daMark = CmpCommon::diags()->mark();
    NATable * describedTable = bindWA->getSchemaDB()-> 
      getNATableDB()->get(descCorrName, bindWA, NULL);
    CmpCommon::diags()->rewind(daMark, TRUE);

    if (describedTable && describedTable->isSeabaseTable() &&
        describedTable->isEnabledForDDLQI())
      generator->objectUids().insert(
        describedTable->objectUid().get_value());
  }

  return 0;
}

/////////////////////////////////////////////////////////////
//
// FileScan::codeGen()
//
/////////////////////////////////////////////////////////////
short FileScan::codeGen(Generator * generator)
{
  if (getTableDesc()->getNATable()->isHiveTable())
  {
    short retval = codeGenForHive(generator);
    return retval ;
  }

  GenAssert(0, "FileScan::codeGen. Should never reach here.");

  return 0;
}

int HbaseAccess::createAsciiColAndCastExpr(Generator * generator,
                                           const NAType &givenType,
                                           ItemExpr *&asciiValue,
                                           ItemExpr *&castValue,
                                           NABoolean srcIsInt32Varchar)
{
  int result = 0;
  asciiValue = NULL;
  castValue = NULL;
  CollHeap * h = generator->wHeap();
  bool needTranslate = FALSE;
  UInt32 hiveScanMode = CmpCommon::getDefaultLong(HIVE_SCAN_SPECIAL_MODE);
 
  // if this is an upshifted datatype, remove the upshift attr.
  // We dont want to upshift data during retrievals or while building keys.
  // Data is only upshifted during insert or updates.
  const NAType * newGivenType = &givenType;
  if (newGivenType->getTypeQualifier() == NA_CHARACTER_TYPE &&
      ((CharType *)newGivenType)->isUpshifted())
    {
      newGivenType = newGivenType->newCopy(h);
      ((CharType*)newGivenType)->setUpshifted(FALSE);
    }

  if (newGivenType->getTypeQualifier() == NA_CHARACTER_TYPE &&
      (CmpCommon::getDefaultString(HIVE_FILE_CHARSET) == "GBK" || 
       CmpCommon::getDefaultString(HIVE_FILE_CHARSET) == "gbk") 
      && CmpCommon::getDefaultString(HIVE_DEFAULT_CHARSET) == "UTF8" )
    needTranslate = TRUE;
  
  // source ascii row is a varchar where the data is a pointer to the source data
  // in the hdfs buffer.
  NAType *asciiType = NULL;
  
  if (DFS2REC::isDoubleCharacter(newGivenType->getFSDatatype()))
    {
      asciiType =  
        new (h) SQLVarChar(h, sizeof(Int64)/2, newGivenType->supportsSQLnull(),
                           FALSE, FALSE, newGivenType->getCharSet(),
                           CharInfo::DefaultCollation,
                           CharInfo::COERCIBLE,
                           CharInfo::UnknownCharSet,
                           (srcIsInt32Varchar ? sizeof(Int32) : 0));
    }
  // set the source charset to GBK if HIVE_FILE_CHARSET is set
  // HIVE_FILE_CHARSET can only be empty or GBK
  else if (  needTranslate == TRUE )
    {
      asciiType =  
        new (h) SQLVarChar(h, sizeof(Int64), newGivenType->supportsSQLnull(),
                           FALSE, FALSE, CharInfo::GBK,
                           CharInfo::DefaultCollation,
                           CharInfo::COERCIBLE,
                           CharInfo::UnknownCharSet,
                           (srcIsInt32Varchar ? sizeof(Int32) : 0));
    }
  else
    {
      asciiType = 
        new (h) SQLVarChar(h, sizeof(Int64), newGivenType->supportsSQLnull(),
                           FALSE, FALSE,
                           CharInfo::DefaultCharSet,
                           CharInfo::DefaultCollation,
                           CharInfo::COERCIBLE,
                           CharInfo::UnknownCharSet,
                           (srcIsInt32Varchar ? sizeof(Int32) : 0));
    }

  asciiValue = new (h) NATypeToItem(asciiType->newCopy(h));

  castValue = new(h) Cast(asciiValue, newGivenType);
  ((Cast*)castValue)->setSrcIsVarcharPtr(TRUE);

  if(( hiveScanMode & 2 ) >0 ) 
    ((Cast*)castValue)->setConvertNullWhenError(TRUE);
  
  if (newGivenType->getTypeQualifier() == NA_INTERVAL_TYPE)
    ((Cast*)castValue)->setAllowSignInInterval(TRUE);

  if (DFS2REC::isBinaryString(givenType.getFSDatatype()))
    castValue = new(h) BuiltinFunction(ITM_DECODE_BASE64, h, 1, castValue);

  if (castValue && asciiValue)
    result = 1;

  return result;
}

int HbaseAccess::createAsciiColAndCastExpr2(Generator * generator,
					    ItemExpr * colNode,
					    const NAType &givenType,
					    ItemExpr *&asciiValue,
					    ItemExpr *&castValue,
                                            NABoolean alignedFormat)
{
  asciiValue = NULL;
  castValue = NULL;
  CollHeap * h = generator->wHeap();

  // if this is an upshifted datatype, remove the upshift attr.
  // We dont want to upshift data during retrievals or while building keys.
  // Data is only upshifted during insert or updates.
  const NAType * newGivenType = &givenType;
  if (newGivenType->getTypeQualifier() == NA_CHARACTER_TYPE &&
      ((CharType *)newGivenType)->isUpshifted())
    {
      newGivenType = newGivenType->newCopy(h);
      ((CharType*)newGivenType)->setUpshifted(FALSE);
    }

  NABoolean encodingNeeded = FALSE;
  asciiValue = new (h) NATypeToItem(newGivenType->newCopy(h));
  castValue = new(h) Cast(asciiValue, newGivenType); 

  if ((!alignedFormat) && HbaseAccess::isEncodingNeededForSerialization(colNode))
    {
      castValue = new(generator->wHeap()) CompDecode(castValue, 
						     newGivenType->newCopy(h),
						     FALSE, TRUE);
    }
  
  return 1;
}

int HbaseAccess::createAsciiColAndCastExpr3(Generator * generator,
					    const NAType &givenType,
					    ItemExpr *&asciiValue,
					    ItemExpr *&castValue)
{
  asciiValue = NULL;
  castValue = NULL;
  CollHeap * h = generator->wHeap();

  // if this is an upshifted datatype, remove the upshift attr.
  // We dont want to upshift data during retrievals or while building keys.
  // Data is only upshifted during insert or updates.
  const NAType * newGivenType = &givenType;
  if (newGivenType->getTypeQualifier() == NA_CHARACTER_TYPE &&
      ((CharType *)newGivenType)->isUpshifted())
    {
      newGivenType = newGivenType->newCopy(h);
      ((CharType*)newGivenType)->setUpshifted(FALSE);
    }

  NAType *asciiType = NULL;

  // source data is in string format.
  // Use cqd to determine max len of source if source type is not string.
  // If cqd is not set, use displayable length for the given datatype.
  Lng32 cvl = 0;
  if (NOT (DFS2REC::isAnyCharacter(newGivenType->getFSDatatype())))
    {
      cvl = 
        (Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_VAL_LENGTH);
      if (cvl <= 0)
        {
          // compute col val length
          cvl = newGivenType->getDisplayLength();
        }
    }
  else
    {
      cvl = newGivenType->getNominalSize();
    }

  asciiType = new (h) SQLVarChar(h, cvl, newGivenType->supportsSQLnull());

  //  asciiValue = new (h) NATypeToItem(newGivenType->newCopy(h));
  asciiValue = new (h) NATypeToItem(asciiType);
  castValue = new(h) Cast(asciiValue, newGivenType); 

  return 1;
}

NABoolean HbaseAccess::columnEnabledForSerialization(ItemExpr * colNode)
{
  NAColumn *nac = NULL;
  if (colNode->getOperatorType() == ITM_BASECOLUMN)
    {
      nac = ((BaseColumn*)colNode)->getNAColumn();
    }
  else if (colNode->getOperatorType() == ITM_INDEXCOLUMN)
    {
      nac = ((IndexColumn*)colNode)->getNAColumn();
    }

  return CmpSeabaseDDL::enabledForSerialization(nac);
}

NABoolean HbaseAccess::isEncodingNeededForSerialization(ItemExpr * colNode)
{
  NAColumn *nac = NULL;
  if (colNode->getOperatorType() == ITM_BASECOLUMN)
    {
      nac = ((BaseColumn*)colNode)->getNAColumn();
    }
  else if (colNode->getOperatorType() == ITM_INDEXCOLUMN)
    {
      nac = ((IndexColumn*)colNode)->getNAColumn();
    }

  return CmpSeabaseDDL::isEncodingNeededForSerialization(nac);
}

short FileScan::genForTextAndSeq(Generator * generator,
                                Queue * &hdfsFileInfoList,
                                Queue * &hdfsFileRangeBeginList,
                                Queue * &hdfsFileRangeNumList,
                                char* &hdfsHostName,
                                Int32 &hdfsPort,
                                NABoolean &useCursorMulti,
                                NABoolean &doSplitFileOpt,
                                NABoolean &isCompressedFile)
{
  Space * space          = generator->getSpace();

  const HHDFSTableStats* hTabStats = 
    getIndexDesc()->getNAFileSet()->getHHDFSTableStats();

  const NABoolean isSequenceFile = hTabStats->isSequenceFile();

  HiveFileIterator hfi;
  hdfsPort = 0;
  hdfsHostName = NULL;
  
  // determine host and port from dir name
  NAString dummy, hostName;
  NABoolean result = TableDesc::splitHiveLocation(
       hTabStats->tableDir().data(),
       hostName,
       hdfsPort,
       dummy,
       CmpCommon::diags(),
       hTabStats->getPortOverride());
  GenAssert(result, "Invalid Hive directory name");
  hdfsHostName = 
        space->AllocateAndCopyToAlignedSpace(hostName, 0);

  hdfsFileInfoList = new(space) Queue(space);
  hdfsFileRangeBeginList = new(space) Queue(space);
  hdfsFileRangeNumList = new(space) Queue(space);

  useCursorMulti = FALSE;
  if (CmpCommon::getDefault(HDFS_USE_CURSOR_MULTI) == DF_ON)
    useCursorMulti = TRUE;
    
  PartitioningFunction * mypart =
    getPhysicalProperty()->getPartitioningFunction();
  
  const NodeMap* nmap = (mypart ? mypart->getNodeMap() : NULL);

  doSplitFileOpt = 
    (useCursorMulti ? FALSE : TRUE);

  if  ((nmap && nmap->type() == NodeMap::HIVE) &&
       (mypart))
    {
      Lng32 entryNum = 0;
      for (CollIndex i=0; i < nmap->getNumEntries(); i++ )
	{
	  HiveNodeMapEntry* hEntry = (HiveNodeMapEntry*)(nmap->getNodeMapEntry(i));
	  LIST(HiveScanInfo)&  scanInfo = hEntry->getScanInfo();

	  Lng32 beginRangeNum = entryNum;
	  Lng32 numRanges = 0;
	  for (CollIndex j=0; j<scanInfo.entries(); j++ ) 
	    {
	      HHDFSFileStats* file = scanInfo[j].file_;
	      const char * fname = file->getFileName();
	      Int64 offset = scanInfo[j].offset_;
	      Int64 span = scanInfo[j].span_;

	      numRanges++;

	      char * fnameInList = 
		space->allocateAndCopyToAlignedSpace
		(fname, strlen(fname), 0);
	      
	      NABoolean fileIsSplitEnd = 
                (offset + span != file->getTotalSize());

	      HdfsFileInfo hfi;
	      hfi.flags_ = 0;
	      hfi.entryNum_ = entryNum;
	      entryNum++;

	      if (scanInfo[j].isLocal_)
		hfi.setFileIsLocal(TRUE);

	      if (offset > 0)
		hfi.setFileIsSplitBegin(TRUE);

	      if (fileIsSplitEnd)
		hfi.setFileIsSplitEnd(TRUE);

	      hfi.startOffset_ = offset;
	      hfi.bytesToRead_ = span;
	      hfi.fileName_ = fnameInList;
	      
              isCompressedFile = FALSE;
                  //if (file->getCompressionInfo().getCompressionMethod() != ComCompressionInfo::UNCOMPRESSED)
                  //   isCompressedFile = TRUE;
	      char * hfiInList = space->allocateAndCopyToAlignedSpace
		((char*)&hfi, sizeof(HdfsFileInfo));
	      
	      hdfsFileInfoList->insert((char*)hfiInList);
	    } // for

	  char * beginRangeInList = 
	    space->allocateAndCopyToAlignedSpace
	    ((char *)&beginRangeNum, sizeof(beginRangeNum), 0);

	  char * numRangeInList = 
	    space->allocateAndCopyToAlignedSpace
	    ((char *)&numRanges, sizeof(numRanges), 0);

	  hdfsFileRangeBeginList->insert(beginRangeInList);
	  hdfsFileRangeNumList->insert(numRangeInList);
	} // for nodemap
    } // if hive
  else
    {
      // not support for non-hives
      useCursorMulti = FALSE;
      doSplitFileOpt = FALSE;
    }

  return 0;
}

short FileScan::genForOrc(Generator * generator,
                         const HHDFSTableStats* hTabStats,
                         const PartitioningFunction * mypart,
                         Queue * &hdfsFileInfoList,
                         Queue * &hdfsFileRangeBeginList,
                         Queue * &hdfsFileRangeNumList,
                         char* &hdfsHostName,
                         Int32 &hdfsPort)
{
  Space * space          = generator->getSpace();

  const NABoolean isSequenceFile = hTabStats->isSequenceFile();

  hdfsPort = 0;
  hdfsHostName = NULL;

  hdfsFileInfoList = new(space) Queue(space);
  hdfsFileRangeBeginList = new(space) Queue(space);
  hdfsFileRangeNumList = new(space) Queue(space);

  const NodeMap* nmap = (mypart ? mypart->getNodeMap() : NULL);

  NABoolean emptyScan = FALSE;

  if ((! nmap) || (! mypart))
    emptyScan = TRUE;
  else 
    {
      HiveNodeMapEntry* hEntry = (HiveNodeMapEntry*)(nmap->getNodeMapEntry(0));
      LIST(HiveScanInfo)&  scanInfo = hEntry->getScanInfo();
      
      if (scanInfo.entries() == 0)
        emptyScan = TRUE;
    }

  char * beginRangeInList = NULL;
  char * numRangeInList = NULL;
  Lng32 beginRangeNum = 0;
  Lng32 numRanges = 0;

  // If no scanInfo entries are specified, then scan all rows.
  if (emptyScan)
    {
      const char * fname = hTabStats->tableDir();
      
      if (hTabStats->entries() > 0)
        {
          const HHDFSListPartitionStats * ps = (*hTabStats)[0];

          if (ps->entries() > 0)
            {
              const HHDFSBucketStats * bs = (*ps)[0];
              if (bs->entries() > 0)
                {
                  for (Lng32 i = 0; i < bs->entries(); i++)
                    {
                      const HHDFSFileStats * fs = (*bs)[i];

                      char * fnameInList = 
                        space->allocateAndCopyToAlignedSpace
                        (fs->getFileName().data(), fs->getFileName().length(), 0);
                      
                      HdfsFileInfo hfi;
                      hfi.flags_ = 0;
                      hfi.entryNum_ = 0;
                      
                      hfi.startOffset_ = 1; // start at rownum 1.
                      hfi.bytesToRead_ = -1; // stop at last row.
                      hfi.fileName_ = fnameInList;
                       
                      char * hfiInList = space->allocateAndCopyToAlignedSpace
                        ((char*)&hfi, sizeof(HdfsFileInfo));
                      
                      hdfsFileInfoList->insert((char*)hfiInList);
                      
                      beginRangeNum = 0;
                      numRanges = 1;
                      
                      beginRangeInList = 
                        space->allocateAndCopyToAlignedSpace
                        ((char *)&beginRangeNum, sizeof(beginRangeNum), 0);
                      
                      numRangeInList = 
                        space->allocateAndCopyToAlignedSpace
                        ((char *)&numRanges, sizeof(numRanges), 0);
                      
                      hdfsFileRangeBeginList->insert(beginRangeInList);
                      hdfsFileRangeNumList->insert(numRangeInList);
                    } // for

                } // if bs
            } // if ps

        } // if hTabStats

      if (hdfsFileInfoList->entries() == 0)
        {
          // append "000000_0" to fname
          NAString fnameStr(fname);
          fnameStr += "/000000_0";
          
          char * fnameInList = 
            space->allocateAndCopyToAlignedSpace
            (fnameStr.data(), fnameStr.length(), 0);
          
          HdfsFileInfo hfi;
          hfi.flags_ = 0;
          hfi.entryNum_ = 0;
          
          hfi.startOffset_ = 1; // start at rownum 1.
          hfi.bytesToRead_ = -1; // stop at last row.
          hfi.fileName_ = fnameInList;
          
          char * hfiInList = space->allocateAndCopyToAlignedSpace
            ((char*)&hfi, sizeof(HdfsFileInfo));
          
          hdfsFileInfoList->insert((char*)hfiInList);

          beginRangeNum = 0;
          numRanges = 1;
          
          beginRangeInList = 
            space->allocateAndCopyToAlignedSpace
            ((char *)&beginRangeNum, sizeof(beginRangeNum), 0);
          
          numRangeInList = 
            space->allocateAndCopyToAlignedSpace
            ((char *)&numRanges, sizeof(numRanges), 0);
          
          hdfsFileRangeBeginList->insert(beginRangeInList);
          hdfsFileRangeNumList->insert(numRangeInList);
        }
    }

  if  ((NOT emptyScan) &&
       ((nmap && nmap->type() == NodeMap::HIVE) &&
        (mypart)))
    {
      Lng32 entryNum = 0;
      for (CollIndex i=0; i < nmap->getNumEntries(); i++ )
	{
	  HiveNodeMapEntry* hEntry = (HiveNodeMapEntry*)(nmap->getNodeMapEntry(i));
	  LIST(HiveScanInfo)&  scanInfo = hEntry->getScanInfo();

	  beginRangeNum = entryNum;
	  numRanges = 0;

	  for (CollIndex j=0; j<scanInfo.entries(); j++ ) 
	    {
	      HHDFSFileStats* file = scanInfo[j].file_;
	      const char * fname = file->getFileName();
	      Int64 startRowNum = scanInfo[j].offset_; // start row num
	      Int64 numRows = scanInfo[j].span_;    // num rows

	      numRanges++;

	      char * fnameInList = 
		space->allocateAndCopyToAlignedSpace
		(fname, strlen(fname), 0);
	      
	      HdfsFileInfo hfi;
	      hfi.flags_ = 0;
	      hfi.entryNum_ = entryNum;
	      entryNum++;

              // for now, scan all rows.
	      hfi.startOffset_ = 1; // startRowNum
	      hfi.bytesToRead_ = -1; // numRows
	      hfi.fileName_ = fnameInList;
	      
	      char * hfiInList = space->allocateAndCopyToAlignedSpace
		((char*)&hfi, sizeof(HdfsFileInfo));
	      
	      hdfsFileInfoList->insert((char*)hfiInList);
	    } // for

	  beginRangeInList = 
	    space->allocateAndCopyToAlignedSpace
	    ((char *)&beginRangeNum, sizeof(beginRangeNum), 0);

	  numRangeInList = 
	    space->allocateAndCopyToAlignedSpace
	    ((char *)&numRanges, sizeof(numRanges), 0);

	  hdfsFileRangeBeginList->insert(beginRangeInList);
	  hdfsFileRangeNumList->insert(numRangeInList);
	} // for nodemap
    } // if hive

  return 0;
}

/*
Sequence of tuples used by a single row is as follows

R1 = Row in HDFS buffer
R2 = Row in asciiSqlRow tupp
R3 = Row in executorPred tupp
R4 = Row in projectOnly tupp

In R1 the data is in HDFS format. For textfiles an example row may with 
five columns may look like
create table test(col1 string, col2 int, col3 int, col4 string, col 5 string);  
a|1|22|hello there|bye now\n  --> R1

R2 is a exploded format row, tupp index 4 in work_cri_desc
Each column is of type varchar length 8. 
The contents of each column is a pointer to appropriate data in R1.
Null values in R1 are detected and the null indicator set in R2
The number of cols in R2 can range from 0 to 5. Only columns that are needed
by the output or selection pred. will be included. Columns that are not
referenced will be skipped. For count(*) type queries with no pred. the 
number of cols in R2 will be 0. R2 is constructed outside the expression 
framework, "by hand code".

R3 is an aligned format row, tupp index 3 in work_cri_desc
Each column if of type as defined in hive metadata.
Columns needed to evaluated the selection predicate are converted from ascii
and moved to R3 from R2. If there is no selection pred. R3 will have 0 columns.
The number of columns in R3 is less than equal to the number of columns in R2.
R3 is the target of the convertExpr. The source of convertExpr is R2.
R3 is also the source of the select_pred expr.

R4 is an aligned format row, tupp index 2 in work_cri_desc.
Each column is of type as defined in hive metadata.
Columns that are present only in the output expression (i.e. are not
present in selection predicate) are placed in this tupp.
This tupp is populated by move_cols_convert_expr, which is called 
only if the row returns TRUE for the selection predicate. The goal is 
to not convert columns for rows that do not pass the selection predicate.

R3 and R4 are the source for the move expression. The target of the 
move_expr is the outputTupp which is in the upEntry's atp.
*/

/////////////////////////////////////////////////////////////
//
// FileScan::codeGenForHive()
//
/////////////////////////////////////////////////////////////
short FileScan::codeGenForHive(Generator * generator)
{

  Space * space          = generator->getSpace();
  ExpGenerator * exp_gen = generator->getExpGenerator();
  MapTable * last_map_table = generator->getLastMapTable();
 
  ValueIdSet outputCols =
    getGroupAttr()->getCharacteristicOutputs();
  outputCols -= getGroupAttr()->getCharacteristicInputs();
  const ValueIdList& hdfsVals = getIndexDesc()->getIndexColumns();
  ValueIdList neededHdfsVals;
  ValueIdList executorPredVals;
  ValueIdList projectExprOnlyVals;
  ValueId dummyVal;
  Int16 *convertSkipList = NULL;
  NABoolean inExecutorPred = FALSE;
  NABoolean inProjExpr = FALSE;
  convertSkipList = new(space) Int16[hdfsVals.entries()];

  for (CollIndex i = 0; i < hdfsVals.entries(); i++)
  {
    if (outputCols.referencesTheGivenValue(hdfsVals[i],dummyVal))
      inProjExpr = TRUE ;
    else
      inProjExpr = FALSE;

    if (executorPred().referencesTheGivenValue(hdfsVals[i],dummyVal))
      inExecutorPred = TRUE;
    else
      inExecutorPred = FALSE;
    
    if (inExecutorPred && inProjExpr)
    {
      convertSkipList[i] = 1;
      neededHdfsVals.insert(hdfsVals[i]);
      executorPredVals.insert(hdfsVals[i]);
    }
    else if (inExecutorPred && !inProjExpr)
    {
      convertSkipList[i] = 2;   
      neededHdfsVals.insert(hdfsVals[i]);
      executorPredVals.insert(hdfsVals[i]);
    }
    else if (!inExecutorPred && inProjExpr)
    {
      convertSkipList[i] = 3;   
      neededHdfsVals.insert(hdfsVals[i]);
      projectExprOnlyVals.insert(hdfsVals[i]);
    }
    else
    {
      convertSkipList[i] = 0;    
    }
    
  }

  setRetrievedCols(neededHdfsVals);

  ex_expr *executor_expr = 0;
  ex_expr *proj_expr = 0;
  ex_expr *convert_expr = 0;
  ex_expr *move_cols_convert_expr = 0;

  // set flag to enable pcode for indirect varchar
  NABoolean vcflag = exp_gen->handleIndirectVC();
  if (CmpCommon::getDefault(VARCHAR_PCODE) == DF_ON) {
    exp_gen->setHandleIndirectVC( TRUE );
  }
  
  ////////////////////////////////////////////////////////////////////////////
  //
  // Returned atp layout:
  //
  // |------------------------------|
  // | input data  |  sql table row |
  // | ( I tupps ) |  ( 1 tupp )    |
  // |------------------------------|
  // <-- returned row to parent ---->
  //
  // input data:        the atp input to this node by its parent. 
  // sql table row:     tupp where the row read from sql table is moved.
  //
  // Input to child:    I tupps
  //
  ////////////////////////////////////////////////////////////////////////////

  ex_cri_desc * given_desc 
    = generator->getCriDesc(Generator::DOWN);
  Int32 returned_atp_index = given_desc->noTuples();

  ex_cri_desc * returned_desc = NULL;
  // cri descriptor for work atp has 1 entry:
  // (plus the first two entries for consts and temps).
  // Entry 1(index #2) is the target of the convertExpr and contains the 
  // hdfs row in aligned format, with each column in binary representation. 
  // The 'where' predicate is evaluated here, and this entry is the source
  // for the project expr.
  // Entry 2(index #3) is the source for the convertExpr. It contains the hdfs
  // row in exploded format, with each column in ascii representation. It is 
  // pointed tobu work_atp_index + 1

  const Int32 work_atp = 1;
  const Int32 projectOnlyTuppIndex = 2;
  const Int32 executorPredTuppIndex = 3;
  const Int32 asciiTuppIndex = 4;
  ULng32 asciiRowLen; 
  ExpTupleDesc * asciiTupleDesc = 0;

  const Int32 origTuppIndex = 5;
  ExpTupleDesc * origTupleDesc = 0;

  ex_cri_desc * work_cri_desc = NULL;
  work_cri_desc = new(space) ex_cri_desc(6, space);
  returned_desc = new(space) ex_cri_desc(given_desc->noTuples() + 1, space);

  ExpTupleDesc::TupleDataFormat asciiRowFormat = ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  ExpTupleDesc::TupleDataFormat hdfsRowFormat = ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  ValueIdList asciiVids;
  ValueIdList executorPredCastVids;
  ValueIdList projectExprOnlyCastVids;

  // ORC row is returned by hdfs as len/value pair for each column.
  // Compute the length of this row.
  Lng32 orcRowLen = 0;

  // Create two new ValueId lists
  // - ASCII representation of each column from hdfs files
  // - Binary representation of each column from hdfs files
  //   that will be returned to the parent.
  //   This list contains cast nodes that will copy ascii
  //   values and perform any required type conversions.
  //   Note that ValueIds for our binary row were already
  //   created during binding. The cast nodes we introduce
  //   here will have different ValueIds. We will later 
  //   account for this when we generate the move expression
  //   by making sure that the output ValueIds created during
  //   binding refer to the outputs of the move expression
  ValueIdList origExprVids;

  NABoolean longVC = FALSE;
  for (int ii = 0; ii < (int)hdfsVals.entries();ii++)
  {
    if (convertSkipList[ii] == 0)
      continue;

    const NAType &givenType = hdfsVals[ii].getType();
    int res;    
    ItemExpr *asciiValue = NULL;
    ItemExpr *castValue = NULL;

    res = HbaseAccess::createAsciiColAndCastExpr(
				    generator,        // for heap
				    givenType,        // [IN] Actual type of HDFS column
				    asciiValue,       // [OUT] Returned expression for ascii rep.
				    castValue,        // [OUT] Returned expression for binary rep.
                                    TRUE // max src data len is sizeof(Int32)
                                    );
     
    GenAssert(res == 1 && asciiValue != NULL && castValue != NULL,
              "Error building expression tree for cast output value");
    asciiValue->synthTypeAndValueId();
    asciiValue->bindNode(generator->getBindWA());
    asciiVids.insert(asciiValue->getValueId());
      
    castValue->bindNode(generator->getBindWA());

    if (convertSkipList[ii] == 1 || convertSkipList[ii] == 2)
      executorPredCastVids.insert(castValue->getValueId());
    else
      projectExprOnlyCastVids.insert(castValue->getValueId());

    origExprVids.insert(hdfsVals[ii]);

    orcRowLen += sizeof(Lng32);
    orcRowLen += givenType.getDisplayLength();

    if ((DFS2REC::isAnyVarChar(givenType.getFSDatatype())) &&
        (givenType.getTotalSize() > 1024))
      longVC = TRUE;
  } // for (ii = 0; ii < hdfsVals; ii++)
    
  UInt32 hiveScanMode = CmpCommon::getDefaultLong(HIVE_SCAN_SPECIAL_MODE);
  //enhance pCode to handle this mode in the future
  //this is for JIRA 1920
  if((hiveScanMode & 2 ) > 0)   //if HIVE_SCAN_SPECIAL_MODE is 2, disable pCode
    exp_gen->setPCodeMode(ex_expr::PCODE_NONE);

  // use CIF if there are long varchars (> 1K length) and CIF has not
  // been explicitly turned off.
  if (longVC && (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) != DF_OFF))
    generator->setCompressedInternalFormat();
 
  // Add ascii columns to the MapTable. After this call the MapTable
  // has ascii values in the work ATP at index asciiTuppIndex.
  exp_gen->processValIdList(
       asciiVids,                             // [IN] ValueIdList
       asciiRowFormat,                        // [IN] tuple data format
       asciiRowLen,                           // [OUT] tuple length 
       work_atp,                              // [IN] atp number
       asciiTuppIndex,                        // [IN] index into atp
       &asciiTupleDesc,                       // [optional OUT] tuple desc
       ExpTupleDesc::LONG_FORMAT);             // [optional IN] desc format
    
  // Add the tuple descriptor for reply values to the work ATP
  work_cri_desc->setTupleDescriptor(asciiTuppIndex, asciiTupleDesc);
  
  ULng32 origRowLen; 
  exp_gen->processValIdList(
       origExprVids,                             // [IN] ValueIdList
       asciiRowFormat,                        // [IN] tuple data format
       origRowLen,                           // [OUT] tuple length 
       work_atp,                              // [IN] atp number
       origTuppIndex,                        // [IN] index into atp
       &origTupleDesc,                       // [optional OUT] tuple desc
       ExpTupleDesc::LONG_FORMAT);             // [optional IN] desc format
    
  // Add the tuple descriptor for reply values to the work ATP
  work_cri_desc->setTupleDescriptor(origTuppIndex, origTupleDesc);
  
  ExpTupleDesc * tuple_desc = 0;
  ExpTupleDesc * hdfs_desc = 0;
  ULng32 executorPredColsRecLength; 
  ULng32 projectOnlyColsRecLength;
  ExpHdrInfo hdrInfo;

  // Generate the expression to move reply values out of the message
  // buffer into an output buffer. After this call returns, the 
  // MapTable has reply values in ATP 0 at the last index.
  exp_gen->generateContiguousMoveExpr(
      executorPredCastVids,                 // [IN] source ValueIds
      FALSE,                                // [IN] add convert nodes?
      work_atp,                             // [IN] target atp number
      executorPredTuppIndex,                // [IN] target tupp index
      hdfsRowFormat,                        // [IN] target tuple format
      executorPredColsRecLength,            // [OUT] target tuple length
      &convert_expr,                        // [OUT] move expression
      &tuple_desc,                     // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT,       // [optional IN] target desc format
      NULL,
      NULL,
      0,
      NULL,
      FALSE,
      NULL,
      FALSE /* doBulkMove */);

  exp_gen->generateContiguousMoveExpr(
      projectExprOnlyCastVids,              // [IN] source ValueIds
      FALSE,                                // [IN] add convert nodes?
      work_atp,                             // [IN] target atp number
      projectOnlyTuppIndex,                 // [IN] target tupp index
      hdfsRowFormat,                        // [IN] target tuple format
      projectOnlyColsRecLength,             // [OUT] target tuple length
      &move_cols_convert_expr,                // [OUT] move expression
      &tuple_desc,                     // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT,       // [optional IN] target desc format
      NULL,
      NULL,
      0,
      NULL,
      FALSE,
      NULL,
      FALSE /* doBulkMove */);
  
  // We can now remove all appended map tables
  generator->removeAll(last_map_table);

  // Append a new map table and
  // add all columns from this table to it. All map tables (incl this
  // one just appended) will be removed before returning back from
  // this operator. A new map table containing only the retrieved
  // columns will be appended at the end. That will make sure that only
  // the retrieved columns are visible from this operator on.

  exp_gen->processValIdList(
                  executorPredVals,
                  hdfsRowFormat,
                  executorPredColsRecLength,
                  work_atp, 
                  executorPredTuppIndex, // where the fetched row will be
                  &hdfs_desc,
                  ExpTupleDesc::LONG_FORMAT,0,NULL,NULL,
                  FALSE, // isIndex
                  FALSE // placeGUOutputFunctionsAtEnd
                  );

   exp_gen->processValIdList(
                  projectExprOnlyVals,
                  hdfsRowFormat,
                  projectOnlyColsRecLength,
                  work_atp, 
                  projectOnlyTuppIndex, // where the fetched row will be
                  &tuple_desc,
                  ExpTupleDesc::LONG_FORMAT,0,NULL,NULL,
                  FALSE, // isIndex
                  FALSE // placeGUOutputFunctionsAtEnd
                  );

  // add this descriptor to the work cri descriptor.
  if (work_cri_desc)
  {
    work_cri_desc->setTupleDescriptor(executorPredTuppIndex, hdfs_desc);
    work_cri_desc->setTupleDescriptor(projectOnlyTuppIndex, tuple_desc);
  }
  
  // generate executor selection expression, if present
  if (! executorPred().isEmpty())
    {

      ItemExpr * newPredTree = executorPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      exp_gen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			    &executor_expr);
      generator->restoreGenLeanExpr();
    }
  
  // Return the projected retrieved row.
  ValueIdList rvidl;
  for (ValueId valId = outputCols.init();
       outputCols.next(valId);
       outputCols.advance(valId))
    {
      rvidl.insert(valId);
    }
 
  ULng32 returnedRowlen = 0;
  MapTable * returnedMapTable = 0;

  exp_gen->generateContiguousMoveExpr(rvidl, TRUE /*add conv nodes*/,
                                      0 /*atp*/, returned_atp_index,
                                      ExpTupleDesc::SQLMX_ALIGNED_FORMAT,
                                      returnedRowlen,
                                      &proj_expr, 
                                      &tuple_desc,
                                      ExpTupleDesc::SHORT_FORMAT,
                                      &returnedMapTable,
                                      NULL,
                                      0,
                                      NULL,
                                      FALSE /*disableConstFolding*/, 
                                      NULL /*colArray*/, 
                                      FALSE /* doBulkMoves */);
  generator->restoreGenLeanExpr();

  // describe the returned row.
  if (returned_desc)
  {
    returned_desc->setTupleDescriptor(returned_desc->noTuples() - 1,
                                      tuple_desc);
    returned_atp_index = returned_desc->noTuples() - 1 ;
  }

  // done with expressions at this operator. Remove the appended map tables.
  generator->removeAll(last_map_table);

  // append the map table containing the returned columns
  if (returnedMapTable)
    generator->appendAtEnd(returnedMapTable);

  //----------------------------------------------------------------------
  // *** DONE WITH MAP TABLES AND GENERATING EXPRESSIONS ***
  //----------------------------------------------------------------------
  
  // Now we can start preparing data that goes in the TDB.

  const HHDFSTableStats* hTabStats = 
    getIndexDesc()->getNAFileSet()->getHHDFSTableStats();
  Queue * hdfsFileInfoList = NULL;
  Queue * hdfsFileRangeBeginList = NULL;
  Queue * hdfsFileRangeNumList = NULL;
  Int64 expirationTimestamp = 0;
  char * hdfsHostName = NULL;
  Int32 hdfsPort = 0;
  char * errCountTab = NULL;
  char * logLocation = NULL;
  char * errCountRowId  = NULL;
  NAString errCountTabNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_LOAD_ERROR_COUNT_TABLE);
  if (errCountTabNAS.length() > 0)
  {
    errCountTab = space->allocateAlignedSpace(errCountTabNAS.length() + 1);
    strcpy(errCountTab, errCountTabNAS.data());
  }
  NAString logLocationNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_LOAD_ERROR_LOGGING_LOCATION);
  if (logLocationNAS.length() > 0)
  {
    logLocation = space->allocateAlignedSpace(logLocationNAS.length() + 1);
    strcpy(logLocation, logLocationNAS.data());
  }
  NAString errCountRowIdNAS = ActiveSchemaDB()->getDefaults().getValue(TRAF_LOAD_ERROR_COUNT_ID);
  if (errCountRowIdNAS.length() > 0)
  {
    errCountRowId = space->allocateAlignedSpace(errCountRowIdNAS.length() + 1);
    strcpy(errCountRowId, errCountRowIdNAS.data());
  }
  NABoolean useCursorMulti = FALSE;
  NABoolean doSplitFileOpt = FALSE;
  NABoolean isCompressedFile = FALSE;

  if ((hTabStats->isTextFile()) || (hTabStats->isSequenceFile()))
    {
      genForTextAndSeq(generator, 
                       hdfsFileInfoList, hdfsFileRangeBeginList, hdfsFileRangeNumList,
                       hdfsHostName, hdfsPort,
                       useCursorMulti, doSplitFileOpt, isCompressedFile);
    }
  else if (hTabStats->isOrcFile())
    {
      genForOrc(generator, 
                hTabStats,
                getPhysicalProperty()->getPartitioningFunction(),
                hdfsFileInfoList, hdfsFileRangeBeginList, hdfsFileRangeNumList,
                hdfsHostName, hdfsPort);
    }

  // set expiration timestamp
  expirationTimestamp = hTabStats->getValidationTimestamp();
  if (expirationTimestamp > 0)
    expirationTimestamp += 1000000 *
      (Int64) CmpCommon::getDefaultLong(HIVE_METADATA_REFRESH_INTERVAL);
  if (!getCommonSubExpr())
    generator->setPlanExpirationTimestamp(expirationTimestamp);

  short type = (short)ComTdbHdfsScan::UNKNOWN_;
  if (hTabStats->isTextFile())
    type = (short)ComTdbHdfsScan::TEXT_;
  else if (hTabStats->isSequenceFile())
    type = (short)ComTdbHdfsScan::SEQUENCE_;
  else if (hTabStats->isOrcFile())
    type = (short)ComTdbHdfsScan::ORC_;
  else {
    *CmpCommon::diags() << DgSqlCode(-7002);
      GenExit();
    return -1;
  }
  ULng32 buffersize = getDefault(GEN_DPSO_BUFFER_SIZE);
  queue_index upqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_UP);
  queue_index downqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_DOWN);
  Int32 numBuffers = getDefault(GEN_DPUO_NUM_BUFFERS);

  // Compute the buffer size based on upqueue size and row size.
  // Try to get enough buffer space to hold twice as many records
  // as the up queue.
  //
  
  UInt32 FiveM = 5*1024*1024;

  // If returnedrowlen > 5M, then set upqueue entries to 2.
  if (returnedRowlen > FiveM)
    upqueuelength = 2;

  ULng32 cbuffersize = 
    SqlBufferNeededSize((upqueuelength * 2 / numBuffers),
			returnedRowlen);
  // But use at least the default buffer size.
  //
  buffersize = buffersize > cbuffersize ? buffersize : cbuffersize;

  // Cap the buffer size at 5M and adjust upqueue entries.
  // Do this only if returnrowlen is not > 5M
  if ((returnedRowlen <= FiveM) && (buffersize > FiveM))
    {
      buffersize = FiveM;

      upqueuelength = ((buffersize / returnedRowlen) * numBuffers)/2;
    }

  // default value is in K bytes
  Int64 hdfsBufSize = 0;

if (hTabStats->isOrcFile())
  {
    hdfsBufSize = (orcRowLen + 1000) * 2; // alloc space for 2 rows plus some buffer
  }
 else
   {
     hdfsBufSize = (Int64)CmpCommon::getDefaultNumeric(HDFS_IO_BUFFERSIZE);
     hdfsBufSize = hdfsBufSize * 1024; // convert to bytes
     Int64 hdfsBufSizeTesting = (Int64)
       CmpCommon::getDefaultNumeric(HDFS_IO_BUFFERSIZE_BYTES);
     if (hdfsBufSizeTesting)
       hdfsBufSize = hdfsBufSizeTesting;
   }
  UInt16 hdfsIoByteArraySize = (UInt16)
      CmpCommon::getDefaultNumeric(HDFS_IO_INTERIM_BYTEARRAY_SIZE_IN_KB);
  UInt32 rangeTailIOSize = (UInt32)
      CmpCommon::getDefaultNumeric(HDFS_IO_RANGE_TAIL);
  if (rangeTailIOSize == 0) 
    {
      rangeTailIOSize = getTableDesc()->getNATable()->getRecordLength() +
	(getTableDesc()->getNATable()->getClusteringIndex()->
	 getAllColumns().entries())*2 + 16*1024;
      // for each range we look ahead in the next range upto the maximum
      // record length to find the end of record delimiter. The 16KB is 
      // old default setting which worked fine till we started testing
      // wide columns. We need to keep the 16 KB as additional fudge factor
      // as recordlength in compiler is different from what it would be
      // in a Hive text file
    }

  char * tablename = 
    space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getTableDesc()->getNATable()->getTableName()), 0);

  char * nullFormat = NULL;
  if (hTabStats->getNullFormat())
    {
      nullFormat = 
        space->allocateAndCopyToAlignedSpace(hTabStats->getNullFormat(),
                                             strlen(hTabStats->getNullFormat()),
                                             0);
    }

  // info needed to validate hdfs file structs
  char * hdfsRootDir = NULL;
  Int64 modTS = -1;
  Lng32 numOfPartLevels = -1;
  Queue * hdfsDirsToCheck = NULL;

  hdfsRootDir =
    space->allocateAndCopyToAlignedSpace(hTabStats->tableDir().data(),
                                         hTabStats->tableDir().length(),
                                         0);

  // Right now, timestamp info is not being generated correctly for
  // partitioned files. Skip data mod check for them.
  // Remove this check when partitioned info is set up correctly.

  if ((CmpCommon::getDefault(HIVE_DATA_MOD_CHECK) == DF_ON) &&
      (CmpCommon::getDefault(TRAF_SIMILARITY_CHECK) != DF_OFF) &&
      (hTabStats->numOfPartCols() <= 0) &&
      (!getCommonSubExpr()))
    {
      modTS = hTabStats->getModificationTSmsec();
      numOfPartLevels = hTabStats->numOfPartCols();

      // if specific directories are to checked based on the query struct
      // (for example, when certain partitions are explicitly specified), 
      // add them to hdfsDirsToCheck.
      // At runtime, only these dirs will be checked for data modification.
      // ** TBD **

      if ((CmpCommon::getDefault(TRAF_SIMILARITY_CHECK) == DF_ROOT) ||
          (CmpCommon::getDefault(TRAF_SIMILARITY_CHECK) == DF_ON))
        {
          char * tiName = new(generator->wHeap()) char[strlen(tablename)+1];
          strcpy(tiName, tablename);
          TrafSimilarityTableInfo * ti = 
            new(generator->wHeap()) TrafSimilarityTableInfo(
                 tiName,
                 TRUE, // isHive
                 (char*)hTabStats->tableDir().data(), // root dir
                 modTS,
                 numOfPartLevels,
                 hdfsDirsToCheck,
                 hdfsHostName, hdfsPort);
          
          generator->addTrafSimTableInfo(ti);
        }
      else
        {
          // sim check done at leaf operators.
          hdfsRootDir =
            space->allocateAndCopyToAlignedSpace(hTabStats->tableDir().data(),
                                                 hTabStats->tableDir().length(),
                                                 0);
        }
    }

  if (getTableDesc()->getNATable()->isEnabledForDDLQI())
    generator->objectUids().insert(
         getTableDesc()->getNATable()->objectUid().get_value());
  
  // create hdfsscan_tdb
  ComTdbHdfsScan *hdfsscan_tdb = new(space) 
    ComTdbHdfsScan(
		   tablename,
                   type,
		   executor_expr,
		   proj_expr,
		   convert_expr,
                   move_cols_convert_expr,
		   hdfsVals.entries(),      // size of convertSkipList
		   convertSkipList,
		   hdfsHostName, 
		   hdfsPort,      
		   hdfsFileInfoList,
		   hdfsFileRangeBeginList,
		   hdfsFileRangeNumList,
		   hTabStats->getRecordTerminator(),  // recordDelimiter
		   hTabStats->getFieldTerminator(),   // columnDelimiter,
                   nullFormat,
		   hdfsBufSize,
                   rangeTailIOSize,
		   executorPredColsRecLength,
		   returnedRowlen,
		   asciiRowLen,
                   projectOnlyColsRecLength,
		   returned_atp_index, 
		   asciiTuppIndex,
		   executorPredTuppIndex,
                   projectOnlyTuppIndex,
                   origTuppIndex,
		   work_cri_desc,
		   given_desc,
		   returned_desc,
		   downqueuelength,
		   upqueuelength,
		   (Cardinality)getEstRowsAccessed().getValue(),
		   numBuffers,
		   buffersize,
		   errCountTab,
		   logLocation,
		   errCountRowId,

                   hdfsRootDir, modTS, numOfPartLevels, hdfsDirsToCheck
		   );
  hdfsscan_tdb->setHdfsIoByteArraySize(hdfsIoByteArraySize);
  generator->initTdbFields(hdfsscan_tdb);

  hdfsscan_tdb->setUseCursorMulti(useCursorMulti);

  hdfsscan_tdb->setDoSplitFileOpt(doSplitFileOpt);

  hdfsscan_tdb->setHiveScanMode(hiveScanMode);

  if (getCommonSubExpr())
    hdfsscan_tdb->setAssignRangesAtRuntime(TRUE);

  NABoolean hdfsPrefetch = FALSE;
  if (CmpCommon::getDefault(HDFS_PREFETCH) == DF_ON)
    hdfsPrefetch = TRUE;
  hdfsscan_tdb->setHdfsPrefetch(hdfsPrefetch);

  if (CmpCommon::getDefault(HDFS_READ_CONTINUE_ON_ERROR) == DF_ON ||
      CmpCommon::getDefault(TRAF_LOAD_CONTINUE_ON_ERROR) == DF_ON)
    hdfsscan_tdb->setContinueOnError(TRUE);

  hdfsscan_tdb->setLogErrorRows(
      CmpCommon::getDefault(TRAF_LOAD_LOG_ERROR_ROWS) == DF_ON);
  hdfsscan_tdb->setMaxErrorRows(
      (UInt32) CmpCommon::getDefaultNumeric(TRAF_LOAD_MAX_ERROR_ROWS));

  NABoolean useCIF = (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) != DF_OFF );
  NABoolean useCIFDegrag = FALSE; //useCIF;  --set temporarily to OFF for testing only

  hdfsscan_tdb->setUseCif(useCIF);
  hdfsscan_tdb->setUseCifDefrag(useCIFDegrag);

  if (CmpCommon::getDefault(USE_LIBHDFS) == DF_ON)
     hdfsscan_tdb->setUseLibhdfsScan(TRUE);

  hdfsscan_tdb->setCompressedFile(isCompressedFile);

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(hdfsscan_tdb, 0, 0, generator));
  }

  if ((generator->computeStats()) && 
      (generator->collectStatsType() == ComTdb::PERTABLE_STATS
      || generator->collectStatsType() == ComTdb::OPERATOR_STATS))
    {
      hdfsscan_tdb->setPertableStatsTdbId((UInt16)generator->
                                          getPertableStatsTdbId());
    }

  generator->setCriDesc(given_desc, Generator::DOWN);
  generator->setCriDesc(returned_desc, Generator::UP);
  generator->setGenObj(this, hdfsscan_tdb);
  hdfsscan_tdb->setEstRowsAccessed((Cardinality)getEstRowsAccessed().getValue());

  // reset the handleIndirectVC flag to its initial value
  exp_gen->setHandleIndirectVC( vcflag );

  return 0;
}

/////////////////////////////////////////////////////////////
//
// HbaseAccess::validateVirtualTableDesc
//
/////////////////////////////////////////////////////////////
NABoolean HbaseAccess::validateVirtualTableDesc(NATable * naTable)
{
  if ((NOT naTable->isHbaseCellTable()) && (NOT naTable->isHbaseRowTable()))
    return FALSE;

  NABoolean isRW = naTable->isHbaseRowTable();

  const NAColumnArray &naColumnArray = naTable->getNAColumnArray();
  
  Lng32 v1 = 
    (Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_NAME_LENGTH);
  Lng32 v2 = 
    (Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_VAL_LENGTH);
  Lng32 v3 = 
    (Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_INFO_LENGTH);

  NAColumn * nac = NULL;
  for (Lng32 i = 0; i < naColumnArray.entries(); i++)
    {
      nac = naColumnArray[i];
      Lng32 length = nac->getType()->getNominalSize();
      if (isRW)
        {
          if (i == HBASE_ROW_ROWID_INDEX)
            {
              if (length != v1)
                return FALSE;
            }
          else if (i == HBASE_COL_DETAILS_INDEX)
            {
              if (length != v3)
                return FALSE;
            }
        }
      else
        {
          if ((i == HBASE_ROW_ID_INDEX) ||
              (i == HBASE_COL_NAME_INDEX) ||
              (i == HBASE_COL_FAMILY_INDEX))
            {
              if (length != v1)
                return FALSE;
            }
          else if (i == HBASE_COL_VALUE_INDEX)
            {
              if (length != v2)
                return FALSE;
            }
        }
    } // for
  
  return TRUE;
}

void populateRangeDescForBeginKey(char* buf, Int32 len, struct TrafDesc* target, NAMemory* heap)
{  
  target->nodetype = DESC_HBASE_RANGE_REGION_TYPE;
  target->hbaseRegionDesc()->beginKey = buf;
  target->hbaseRegionDesc()->beginKeyLen = len;
  target->hbaseRegionDesc()->endKey = NULL;
  target->hbaseRegionDesc()->endKeyLen = 0;   
}

TrafDesc *HbaseAccess::createVirtualTableDesc(const char * name,
  NABoolean isRW, NABoolean isCW, NAArray<HbaseStr>* beginKeys, NAMemory * heap)
{
  TrafDesc * table_desc = NULL;

  if (isRW)
    table_desc =
      Generator::createVirtualTableDesc(
					name,
					heap,
					ComTdbHbaseAccess::getVirtTableRowwiseNumCols(),
					ComTdbHbaseAccess::getVirtTableRowwiseColumnInfo(),
					ComTdbHbaseAccess::getVirtTableRowwiseNumKeys(),
					ComTdbHbaseAccess::getVirtTableRowwiseKeyInfo());
  else if (isCW)
    table_desc =
      Generator::createVirtualTableDesc(name,
					heap,
					ComTdbHbaseAccess::getVirtTableNumCols(),
					ComTdbHbaseAccess::getVirtTableColumnInfo(),
					ComTdbHbaseAccess::getVirtTableNumKeys(),
					ComTdbHbaseAccess::getVirtTableKeyInfo());

  if (table_desc)
    {
      struct TrafDesc* head = Generator::assembleDescs(beginKeys, heap);

      table_desc->tableDesc()->hbase_regionkey_desc = head;

      Lng32 v1 = 
	(Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_NAME_LENGTH);
      Lng32 v2 = 
	(Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_VAL_LENGTH);
      Lng32 v3 = 
	(Lng32) CmpCommon::getDefaultNumeric(HBASE_MAX_COLUMN_INFO_LENGTH);

      TrafDesc * cols_desc =  table_desc->tableDesc()->columns_desc;
      for (Lng32 i = 0; i < table_desc->tableDesc()->colcount; i++)
	{
	  if (isRW)
	    {
	      if (i == HBASE_ROW_ROWID_INDEX)
		cols_desc->columnsDesc()->length = v1;
	      else if (i == HBASE_COL_DETAILS_INDEX)
		cols_desc->columnsDesc()->length = v3;
	    }
	  else
	    {
	      if ((i == HBASE_ROW_ID_INDEX) ||
		  (i == HBASE_COL_NAME_INDEX) ||
		  (i == HBASE_COL_FAMILY_INDEX))
		cols_desc->columnsDesc()->length = v1;
	      else if (i == HBASE_COL_VALUE_INDEX)
		cols_desc->columnsDesc()->length = v2;
	    }
	  cols_desc = cols_desc->next;

	} // for
    }

  return table_desc;
}

TrafDesc *HbaseAccess::createVirtualTableDesc(const char * name,
						 NAList<char*> &colNameList,
						 NAList<char*> &colValList)
{
  TrafDesc * table_desc = NULL;

  Lng32 arrSize = colNameList.entries();
  ComTdbVirtTableColumnInfo * colInfoArray = (ComTdbVirtTableColumnInfo*)
    new char[arrSize * sizeof(ComTdbVirtTableColumnInfo)];

  // to be safe, allocate an array as big as the column array
  ComTdbVirtTableKeyInfo * keyInfoArray = (ComTdbVirtTableKeyInfo*)
    new char[arrSize * sizeof(ComTdbVirtTableKeyInfo)];

  // colList contains 3 column families:
  //  obj_type, col_details and key_details
  Lng32 numCols = 0;
  Lng32 numKeys = 0;
  Lng32 i = 0;
  char colFamily[100];
  for (i = 0; i < arrSize; i++)
    {
      char * colName = colNameList[i];
      char * val = colValList[i];

      // look for ":" and find the column family
      Lng32 cp = 0;
      while ((cp < strlen(colName)) && (colName[cp] != ':'))
	cp++;

      if (cp < strlen(colName))
	{
	  str_cpy_and_null(colFamily, colName, cp, '\0', ' ', TRUE);
	}
      else
	{
	  // must have a column family
	  return NULL;
	}

      if (strcmp(colFamily, "col_details") == 0)
	{
	  colInfoArray[numCols].colName = &colName[cp+1];

	  // val format: TTTT:LLLL:PPPP:SS:DS:DE:KK:NN
	  //                   type:length:precision:scale:datestart:dateend:primaryKey:nullable
	  
	  char buf[100];
	  Lng32 curPos = 0;
	  Lng32 type = (Lng32)str_atoi(&val[curPos], 4);
	  curPos += 4;
	  curPos++; // skip the ":"

	  Lng32 len = (Lng32)str_atoi(&val[curPos], 4);
	  curPos += 4;
	  curPos++; // skip the ":"
	  
	  Lng32 precision = (Lng32)str_atoi(&val[curPos], 4);
	  curPos += 4;
	  curPos++; // skip the ":"

	  Lng32 scale = (Lng32)str_atoi(&val[curPos], 2);
	  curPos += 2;
	  curPos++; // skip the ":"

	  Lng32 dtStart = (Lng32)str_atoi(&val[curPos], 2);
	  curPos += 2;
	  curPos++; // skip the ":"

	  Lng32 dtEnd = (Lng32)str_atoi(&val[curPos], 2);
	  curPos += 2;
	  curPos++; // skip the ":"

	  Lng32 pKey = (Lng32)str_atoi(&val[curPos], 2);
	  curPos += 2;
	  curPos++; // skip the ":"
	  
	  Lng32 nullHeaderSize = (Lng32)str_atoi(&val[curPos], 2);
	  
	  colInfoArray[numCols].datatype = type;
	  colInfoArray[numCols].length = len;
	  colInfoArray[numCols].nullable = (nullHeaderSize > 0 ? 1 : 0);
	  colInfoArray[numCols].charset = SQLCHARSETCODE_UNKNOWN;

	  colInfoArray[numCols].precision = precision;
	  colInfoArray[numCols].scale = scale;
	  colInfoArray[numCols].dtStart = dtStart;
	  colInfoArray[numCols].dtEnd = dtEnd;

	  numCols++;
	}
      else if (strcmp(colFamily, "key_details") == 0)
	{
	  Lng32 keySeq = (Lng32)str_atoi(&val[0], 4);
	  Lng32 colNum = (Lng32)str_atoi(&val[4+1], 4);

	  keyInfoArray[numKeys].colName = NULL;
	  keyInfoArray[numKeys].keySeqNum = keySeq;
	  keyInfoArray[numKeys].tableColNum = colNum;
	  keyInfoArray[numKeys].ordering = 0;

	  numKeys++;
	}
      else if (strcmp(colFamily, "obj_type") == 0)
	{
	  char * val = colValList[i];
	  if (strcmp(val, "BT") != 0)
	    {
	      // must be a base table
	      return NULL;
	    }
	}
    }

   table_desc =
      Generator::createVirtualTableDesc(name,
					NULL, // let it decide what heap to use
					numCols, //ComTdbHbaseAccess::getVirtTableNumCols(),
					colInfoArray, //ComTdbHbaseAccess::getVirtTableColumnInfo(),
					numKeys, //ComTdbHbaseAccess::getVirtTableNumKeys(),
					keyInfoArray); //ComTdbHbaseAccess::getVirtTableKeyInfo());

  return table_desc;
}

short HbaseAccess::genRowIdExpr(Generator * generator,
				const NAColumnArray & keyColumns,
				NAList<HbaseSearchKey*>& searchKeys,
				ex_cri_desc * work_cri_desc,
				const Int32 work_atp,
				const Int32 rowIdAsciiTuppIndex,
				const Int32 rowIdTuppIndex,
				ULng32 &rowIdAsciiRowLen,
				ExpTupleDesc* &rowIdAsciiTupleDesc,
				UInt32 &rowIdLength,
				ex_expr* &rowIdExpr,
                                NABoolean encodeKeys)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  rowIdAsciiRowLen = 0; 
  rowIdAsciiTupleDesc = 0;
  rowIdLength = 0;
  rowIdExpr = NULL;

  ValueIdList rowIdAsciiVids;
  if (searchKeys.entries() > 0)
    {
      ValueIdList keyConvVIDList;
      
      HbaseSearchKey* searchKey = searchKeys[0];
      
      for (CollIndex i = 0; i < searchKey->getKeyColumns().entries(); i++)
	{
	  ValueId vid = searchKey->getKeyColumns()[i];
	  const NAType &givenType = vid.getType();

	  int res;
	  ItemExpr * castVal = NULL;
	  ItemExpr * asciiVal = NULL;
	  res = createAsciiColAndCastExpr(generator,
					  givenType,
					  asciiVal, castVal);
	  GenAssert(res == 1 && asciiVal != NULL && castVal != NULL,
		    "Error building expression tree for cast output value");
	  
	  asciiVal->bindNode(generator->getBindWA());
	  rowIdAsciiVids.insert(asciiVal->getValueId());
	  
	  ItemExpr * ie = castVal;
	  if ((givenType.getVarLenHdrSize() > 0) &&
              (encodeKeys))
	    {
	      // Explode varchars by moving them to a fixed field
	      // whose length is equal to the max length of varchar.
	      // handle different character set cases.
	      const CharType& char_t = (CharType&)givenType;
	      ie = new(generator->wHeap())
		Cast (ie,
		      (new(generator->wHeap())
		       SQLChar(generator->wHeap(), CharLenInfo(char_t.getStrCharLimit(), char_t.getDataStorageSize()),
			       givenType.supportsSQLnull(),
			       FALSE, FALSE, FALSE,
			       char_t.getCharSet(),
			       char_t.getCollation(),
			       char_t.getCoercibility()
			       )));
	    }

	  NABoolean descFlag = TRUE;
	  if (keyColumns.isAscending(i))
	    descFlag = FALSE;
          if (encodeKeys)
            {
              ie = new(generator->wHeap())
                CompEncode(ie, descFlag);
            }

	  ie->bindNode(generator->getBindWA());
	  keyConvVIDList.insert(ie->getValueId());
	} // for
      
      expGen->processValIdList(
           rowIdAsciiVids,                             // [IN] ValueIdList
           ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
           rowIdAsciiRowLen,                        // [OUT] tuple length 
           work_atp,                                      // [IN] atp number
           rowIdAsciiTuppIndex,                    // [IN] index into atp
           &rowIdAsciiTupleDesc,                  // [optional OUT] tuple desc
           ExpTupleDesc::LONG_FORMAT);    // [optional IN] desc format
      
      work_cri_desc->setTupleDescriptor(rowIdAsciiTuppIndex, rowIdAsciiTupleDesc);
      
      expGen->generateContiguousMoveExpr(
					 keyConvVIDList, 0/*no conv nodes*/,
					 work_atp, rowIdTuppIndex, 
					 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					 rowIdLength,
					 &rowIdExpr);
    } // if

  return 0;
}

short HbaseAccess::genRowIdExprForNonSQ(Generator * generator,
					const NAColumnArray & keyColumns,
					NAList<HbaseSearchKey*>& searchKeys,
					ex_cri_desc * work_cri_desc,
					const Int32 work_atp,
					const Int32 rowIdAsciiTuppIndex,
					const Int32 rowIdTuppIndex,
					ULng32 &rowIdAsciiRowLen,
					ExpTupleDesc* &rowIdAsciiTupleDesc,
					UInt32 &rowIdLength,
					ex_expr* &rowIdExpr)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  rowIdAsciiRowLen = 0; 
  rowIdAsciiTupleDesc = 0;
  rowIdLength = 0;
  rowIdExpr = NULL;

  ValueIdList rowIdAsciiVids;
  if (searchKeys.entries() > 0)
    {
      ValueIdList keyConvVIDList;
      
      HbaseSearchKey* searchKey = searchKeys[0];
      
      for (CollIndex i = 0; i < searchKey->getKeyColumns().entries(); i++)
	{
	  ValueId vid = searchKey->getKeyColumns()[i];
	  const NAType &givenType = vid.getType();

	  int res;
	  ItemExpr * castVal = NULL;
	  ItemExpr * asciiVal = NULL;
	  res = createAsciiColAndCastExpr(generator,
					  givenType,
					  asciiVal, castVal);
	  GenAssert(res == 1 && asciiVal != NULL && castVal != NULL,
		    "Error building expression tree for cast output value");
	  
	  asciiVal->bindNode(generator->getBindWA());
	  rowIdAsciiVids.insert(asciiVal->getValueId());
	  
	  ItemExpr * ie = castVal;
	  const CharType& char_t = (CharType&)givenType;
	  ie = new(generator->wHeap())
	    Cast (ie,
		  (new(generator->wHeap())
		   ANSIChar(generator->wHeap(), char_t.getDataStorageSize(),
			    givenType.supportsSQLnull(),
			    FALSE, FALSE, 
			    char_t.getCharSet(),
			    char_t.getCollation(),
			    char_t.getCoercibility(),
			    CharInfo::UnknownCharSet
			    )));
	  
	  ie->bindNode(generator->getBindWA());
	  keyConvVIDList.insert(ie->getValueId());
	} // for
      
      expGen->processValIdList(
           rowIdAsciiVids,                             // [IN] ValueIdList
           ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
           rowIdAsciiRowLen,                        // [OUT] tuple length 
           work_atp,                                      // [IN] atp number
           rowIdAsciiTuppIndex,                    // [IN] index into atp
           &rowIdAsciiTupleDesc,                  // [optional OUT] tuple desc
           ExpTupleDesc::LONG_FORMAT);    // [optional IN] desc format
      
      work_cri_desc->setTupleDescriptor(rowIdAsciiTuppIndex, rowIdAsciiTupleDesc);
      
       expGen->generateContiguousMoveExpr(
					 keyConvVIDList, 0/*no conv nodes*/,
					 work_atp, rowIdTuppIndex, 
					 ExpTupleDesc::SQLARK_EXPLODED_FORMAT,
					 rowIdLength,
					 &rowIdExpr);
    } // if

  return 0;
}

short HbaseAccess::genListsOfRows(Generator * generator,
				  NAList<HbaseRangeRows> &listOfRangeRows,
				  NAList<HbaseUniqueRows> &listOfUniqueRows,
				  Queue* &tdbListOfRangeRows,
				  Queue* &tdbListOfUniqueRows)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  if (listOfRangeRows.entries() > 0)
    {
      tdbListOfRangeRows = new(space) Queue(space);
      
      for (Lng32 i = 0; i < listOfRangeRows.entries(); i++)
	{
	  HbaseRangeRows &hs = listOfRangeRows[i];
	  
	  Queue * colNamesList = new(space) Queue(space);
	  
	  char * beginRowIdInList = NULL;
	  if (hs.beginRowId_.length() == 0)
	    {
	      beginRowIdInList = 
		space->allocateAlignedSpace(sizeof(short));
	      *(short*)beginRowIdInList = 0;
	    }
	  else
	    beginRowIdInList = 
	      space->AllocateAndCopyToAlignedSpace(hs.beginRowId_, 0);

	  char * endRowIdInList = NULL;
	  if (hs.endRowId_.length() == 0)
	    {
	      endRowIdInList = 
		space->allocateAlignedSpace(sizeof(short));
	      *(short*)endRowIdInList = 0;
	    }
	  else
	    endRowIdInList = 
	      space->AllocateAndCopyToAlignedSpace(hs.endRowId_, 0);

	  for (Lng32 j = 0; j < hs.colNames_.entries(); j++)
	    {
	      NAString colName = "cf1:";
	      colName += hs.colNames_[j];
	      
	      char * colNameInList = 
		space->AllocateAndCopyToAlignedSpace(colName, 0);
	      
	      colNamesList->insert(colNameInList);
	    }

	  ComTdbHbaseAccess::HbaseScanRows * hsr = 
	    new(space) ComTdbHbaseAccess::HbaseScanRows();
	  hsr->beginRowId_ = beginRowIdInList;
	  hsr->endRowId_ = endRowIdInList;
	  hsr->beginKeyExclusive_ = (hs.beginKeyExclusive_ ? 1 : 0);
	  hsr->endKeyExclusive_ = (hs.endKeyExclusive_ ? 1 : 0);
	  hsr->colNames_ = colNamesList;
	  hsr->colTS_ = -1;

	  tdbListOfRangeRows->insert(hsr);
	} // for
    }

  if (listOfUniqueRows.entries() > 0)
    {
      tdbListOfUniqueRows = new(space) Queue(space);
      
      for (Lng32 i = 0; i < listOfUniqueRows.entries(); i++)
	{
	  HbaseUniqueRows &hg = listOfUniqueRows[i];

	  Queue * rowIdsList = new(space) Queue(space);
	  Queue * colNamesList = new(space) Queue(space);
	  
	  for (Lng32 j = 0; j < hg.rowIds_.entries(); j++)
	    {
	      NAString &rowId = hg.rowIds_[j];

	      char * rowIdInList = 
		space->AllocateAndCopyToAlignedSpace(rowId, 0);

	      rowIdsList->insert(rowIdInList);
	    }

	  for (Lng32 j = 0; j < hg.colNames_.entries(); j++)
	    {
	      NAString colName = "cf1:";
	      colName += hg.colNames_[j];

	      char * colNameInList = 
		space->AllocateAndCopyToAlignedSpace(colName, 0);
	      
	      colNamesList->insert(colNameInList);
	    }

	  ComTdbHbaseAccess::HbaseGetRows * hgr = 
	    new(space) ComTdbHbaseAccess::HbaseGetRows();
	  hgr->rowIds_ = rowIdsList;
	  hgr->colNames_ = colNamesList;
	  hgr->colTS_ = -1;

	  tdbListOfUniqueRows->insert(hgr);
	} // for
    }
  
  return 0;
}

short HbaseAccess::genColName(Generator * generator,
			      ItemExpr * col_node,
			      char* &colNameInList)
{      
  Space * space = generator->getSpace();

  NAString cnInList;
  colNameInList = NULL;
  if (! col_node)
    {
      createHbaseColId(NULL, cnInList);
      
      colNameInList = 
	space->AllocateAndCopyToAlignedSpace(cnInList, 0);
    }
  else if (col_node->getOperatorType() == ITM_BASECOLUMN)
    {
      const NAColumn *nac = 
	((BaseColumn*)col_node)->getNAColumn();
      
      createHbaseColId(nac, cnInList);
      
      colNameInList = 
	space->AllocateAndCopyToAlignedSpace(cnInList, 0);
    }
  else if (col_node->getOperatorType() == ITM_INDEXCOLUMN)
    {
      NAColumn *nac = ((IndexColumn*)col_node)->getNAColumn();
      
      createHbaseColId(nac, cnInList);
      
      colNameInList = 
	space->AllocateAndCopyToAlignedSpace(cnInList, 0);
    }
  else if (col_node->getOperatorType() == ITM_CONSTANT)
    {
      ConstValue * cv = (ConstValue*)col_node;

      char * colVal = (char*)cv->getConstValue();
      short len = cv->getStorageSize();

      cnInList.append((char*)&len, sizeof(short));
      cnInList.append(colVal, len);

      colNameInList = 
	space->AllocateAndCopyToAlignedSpace(cnInList, 0);
    }
  else if (col_node->getOperatorType() == ITM_REFERENCE)
    {
      GenAssert(0, "Should not have ColReference");
    }

  return 0;
}

short HbaseAccess::genListOfColNames(Generator * generator,
				     const IndexDesc * indexDesc,
				     ValueIdList &columnList,
				     Queue* &listOfColNames)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  const CollIndex numColumns = columnList.entries();

  listOfColNames = new(space) Queue(space);
  
  for (CollIndex c = 0; c < numColumns; c++)
    {
      ItemExpr * col_node = ((columnList[c]).getValueDesc())->getItemExpr();
      
      char * colNameInList = NULL;

      genColName(generator, col_node, colNameInList);

      listOfColNames->insert(colNameInList);
    }

  return 0;
}

// colIdformat:
//     2 bytes len, len bytes colname.
//
// colname format: 
//       <colfam>:<colQual>  (for base table access)
//       <colfam>:@<colQual>   (for index access)
//
//                colQual is 1,2,4 bytes
//
short HbaseAccess::createHbaseColId(const NAColumn * nac,
				    NAString &cid, 
				    NABoolean isSecondaryIndex)
{
  if (nac)
    cid = nac->getHbaseColFam();
  else
    cid = SEABASE_DEFAULT_COL_FAMILY;
  cid += ":";

  if (nac && nac->getNATable() && nac->getNATable()->isHbaseMapTable())
    {
      char * colQualPtr = (char*)nac->getColName().data();
      Lng32 colQualLen = nac->getHbaseColQual().length();

      cid.append(colQualPtr, colQualLen);
    }
  else if (nac)
    {
      char * colQualPtr = (char*)nac->getHbaseColQual().data();
      Lng32 colQualLen = nac->getHbaseColQual().length();
      if (colQualPtr[0] == '@')
	{
	  cid += "@";
	  colQualLen--;
	  colQualPtr++;
	}
      else if (isSecondaryIndex)
	cid += "@"; 
      
      Int64 colQval = str_atoi(colQualPtr, colQualLen);
      
      if (colQval <= UCHAR_MAX)
	{
	  unsigned char c = (unsigned char)colQval;
	  cid.append((char*)&c, 1);
	}
      else if (colQval <= USHRT_MAX)
	{
	  unsigned short s = (unsigned short)colQval;
	  cid.append((char*)&s, 2);
	}
      else if (colQval <= ULONG_MAX)
	{
	  Lng32 l = (Lng32)colQval;
	  cid.append((char*)&l, 4);
	}
      else
	cid.append((char*)&colQval, 8);
    }

  short len = cid.length();
  cid.prepend((char*)&len, sizeof(short));

  return 0;
}
NABoolean HbaseAccess::isSnapshotScanFeasible(
    LatestSnpSupportEnum snpSupport, char * tabName)
{
  if (snpType_ == SNP_NONE)
    return FALSE;

  if (snpType_ != SNP_NONE
      && (!getTableDesc()->getNATable()->isSeabaseTable()
          || getTableDesc()->getNATable()->isSeabaseMDTable()
          || (getTableDesc()->getNATable()->getTableName().getObjectName() == HBASE_HIST_NAME)
          || (getTableDesc()->getNATable()->getTableName().getObjectName() == HBASE_HISTINT_NAME)))
  {
    snpType_ = SNP_NONE;
  } else if (snpType_ == SNP_LATEST)
  {
    if ((snpSupport == latest_snp_index_table)
        || (snpSupport == latest_snp_no_snapshot_available)
        || (snpSupport == latest_snp_small_table)
        || (snpSupport == latest_snp_not_trafodion_table))
    {
      //revert to regular scan
      snpType_ = SNP_NONE;
      char msg[200];
      if (snpSupport == latest_snp_index_table)
      {
        sprintf(msg, "%s",
            "snapshot scan is not supported with index tables yet");
      } else if (snpSupport == latest_snp_no_snapshot_available)
      {
        sprintf(msg, "%s", "there are no snapshots associated with it");
      } else if (snpSupport == latest_snp_small_table)
      {
        sprintf(msg, "%s %d MBs",
            "its estimated size is less than the threshold of",
            getDefault(TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD));
      } else if (snpSupport == latest_snp_not_trafodion_table)
      {
        sprintf(msg, "%s", "it is not a Trafodion table");
      }
      //issue warning
      *CmpCommon::diags() << DgSqlCode(4372) << DgString0(tabName)
          << DgString1(msg);
    }
  }
  return (snpType_ != SNP_NONE);
}
short HbaseAccess::createSortValue(ItemExpr * col_node,
				   std::vector<SortValue> &myvector,
				   NABoolean isSecondaryIndex)
{
  if (col_node->getOperatorType() == ITM_BASECOLUMN)
    {
      NAColumn *nac = ((BaseColumn*)col_node)->getNAColumn();
      NAString cn;
      createHbaseColId(nac, cn);
      
      SortValue sv; 
      sv.str_ = cn;
      sv.vid_ = col_node->getValueId();
      myvector.push_back(sv);
    }
  else if (col_node->getOperatorType() == ITM_INDEXCOLUMN)
    {
      IndexColumn * idxCol = (IndexColumn*)col_node;
      
      NAColumn *nac = idxCol->getNAColumn();
      
      NAString cn;

      createHbaseColId(nac, cn, isSecondaryIndex);
      
      SortValue sv; 
      sv.str_ = cn;
      sv.vid_ = col_node->getValueId();
      myvector.push_back(sv);
    }
  else if (col_node->getOperatorType() == ITM_REFERENCE)
    {
      GenAssert(0, "Should not have ColReference");
    }

  return 0;
}

short HbaseAccess::returnDups(std::vector<HbaseAccess::SortValue> &myvector,
                              ValueIdList &srcVIDlist, ValueIdList &dupVIDlist)
{
  size_t startPos = 0;
  size_t currPos = 0;
  HbaseAccess::SortValue startVal = myvector[startPos];

  while (currPos <= myvector.size())
    {
      NABoolean greater = FALSE;
      if (currPos == myvector.size())
        greater = TRUE;
      else
        {
          HbaseAccess::SortValue currVal = myvector[currPos];
          if (currVal < startVal)
            return -1; // error, must be sorted

          if (NOT (currVal == startVal)) // currVal > startVal
            greater = TRUE;
        }

      if (greater)
        {
          if ((currPos - startPos) > 1)
            {
              for (size_t j = startPos+1; j < currPos; j++)
                {
                  srcVIDlist.insert(myvector[startPos].vid_);
                  dupVIDlist.insert(myvector[j].vid_);
                }
            }

          if (currPos < myvector.size())
            {
              startPos = currPos;
              startVal = myvector[startPos];
            }
        } // currVal > startVal

      currPos++;
    }

  return 0;
}

short HbaseAccess::sortValues (const ValueIdList &inList,
			       ValueIdList &sortedList,
			       NABoolean isSecondaryIndex)
{
  std::vector<SortValue> myvector;

  for (CollIndex i = 0; i < inList.entries(); i++)
    {
      ItemExpr * col_node = inList[i].getValueDesc()->getItemExpr();

      createSortValue(col_node, myvector, isSecondaryIndex);
    }

  // using object as comp
  std::sort (myvector.begin(), myvector.end()); 

  myvector.erase( unique( myvector.begin(), myvector.end() ), myvector.end() );

  for (size_t ii = 0; ii < myvector.size(); ii++)
    {
      sortedList.insert(myvector[ii].vid_);
    }

  return 0;
}

short HbaseAccess::sortValues (const ValueIdSet &inSet,
			       ValueIdList &uniqueSortedList,
                               ValueIdList &srcVIDlist,
                               ValueIdList &dupVIDlist,
			       NABoolean isSecondaryIndex)
{
  std::vector<SortValue> myvector;

  for (ValueId vid = inSet.init(); inSet.next(vid); inSet.advance(vid))
    {
      ItemExpr * col_node = vid.getValueDesc()->getItemExpr();

      createSortValue(col_node, myvector, isSecondaryIndex);
    }

  // using object as comp
  std::sort (myvector.begin(), myvector.end()); 

  if (isSecondaryIndex)
    {
      returnDups(myvector, srcVIDlist, dupVIDlist);
    }

  myvector.erase( unique( myvector.begin(), myvector.end() ), myvector.end() );

  for (size_t ii = 0; ii < myvector.size(); ii++)
    {
      uniqueSortedList.insert(myvector[ii].vid_);
    }

  return 0;
}

short HbaseAccess::sortValues (NASet<NAString> &inSet,
			       NAList<NAString> &sortedList)
{
  std::vector<NAString> myvector;

  for (Lng32 i = 0; i < inSet.entries(); i++)
    {
      NAString &colName = inSet[i];

      myvector.push_back(colName);
    }

  // using object as comp
  std::sort (myvector.begin(), myvector.end()); 

  myvector.erase( unique( myvector.begin(), myvector.end() ), myvector.end() );

  for (size_t ii = 0; ii < myvector.size(); ii++)
    {
      sortedList.insert(myvector[ii]);
    }

  return 0;
}

// infoType: 0, timestamp. 1, version. 2, security label.
static short HbaseAccess_updateHbaseInfoNode(
                                             ValueIdList &hbiVIDlist,
                                             const NAString &colName,
                                             Lng32 colIndex)
{
  for (Lng32 i = 0; i < hbiVIDlist.entries(); i++)
    {
      ValueId &vid = hbiVIDlist[i];
      ItemExpr * ie = vid.getItemExpr();
      
      const ItemExpr * col_node = NULL;
      HbaseTimestamp * hbt = NULL;
      HbaseVersion * hbv = NULL;
      if (ie->getOperatorType() == ITM_HBASE_TIMESTAMP)
        {
          hbt = (HbaseTimestamp*)ie;
      
          col_node = hbt->col();
        }
      else if (ie->getOperatorType() == ITM_HBASE_VERSION)
        {
          hbv = (HbaseVersion*)ie;
      
          col_node = hbv->col();
        }
        
      NAColumn * nac = NULL;
      if (col_node->getOperatorType() == ITM_BASECOLUMN)
        {
          nac = ((BaseColumn*)col_node)->getNAColumn();
        }
      
      if (nac && (nac->getColName() == colName))
        {
          if (ie->getOperatorType() == ITM_HBASE_TIMESTAMP)
            hbt->setColIndex(colIndex);
          else
            hbv->setColIndex(colIndex);
        }
    }
  
  return 0;
}

short HbaseAccess::codeGen(Generator * generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  // allocate a map table for the retrieved columns
  //  generator->appendAtEnd();
  MapTable * last_map_table = generator->getLastMapTable();
 
  ex_expr *scanExpr = 0;
  ex_expr *proj_expr = 0;
  ex_expr *convert_expr = NULL;
  ex_expr *hbaseFilterValExpr = NULL;

  ex_cri_desc * givenDesc 
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc = NULL;

  const Int32 work_atp = 1;
  const Int32 convertTuppIndex = 2;
  const Int32 rowIdTuppIndex = 3;
  const Int32 asciiTuppIndex = 4;
  const Int32 rowIdAsciiTuppIndex = 5;
  const Int32 keyTuppIndex = 6;
  const Int32 hbaseFilterValTuppIndex = 7;
  const Int32 hbaseTimestampTuppIndex = 8;
  const Int32 hbaseVersionTuppIndex = 9;

  ULng32 asciiRowLen; 
  ExpTupleDesc * asciiTupleDesc = 0;
  ExpTupleDesc * convertTupleDesc = NULL;
  ExpTupleDesc * hbaseFilterValTupleDesc = NULL;

  ULng32 hbaseFilterValRowLen = 0;

  ex_cri_desc * work_cri_desc = NULL;
  work_cri_desc = new(space) ex_cri_desc(10, space);

  returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  ExpTupleDesc::TupleDataFormat hbaseRowFormat ;
  ValueIdList asciiVids;
  ValueIdList executorPredCastVids;
  ValueIdList convertExprCastVids;

  NABoolean addDefaultValues = TRUE;
  NABoolean hasAddedColumns = FALSE;
  if (getTableDesc()->getNATable()->hasAddedColumn())
    hasAddedColumns = TRUE;

  NABoolean isSecondaryIndex = FALSE;
  if (getIndexDesc()->getNAFileSet()->getKeytag() != 0)
    {
      isSecondaryIndex = TRUE;
      hasAddedColumns = FALSE; // secondary index doesn't have added cols
    }

  NABoolean isAlignedFormat = getTableDesc()->getNATable()->isAlignedFormat(getIndexDesc());
  NABoolean isHbaseMapFormat = getTableDesc()->getNATable()->isHbaseMapTable();

  // If CIF is not OFF use aligned format, except when table is
  // not aligned and it has added columns. Support for added columns
  // in not aligned tables is doable, but is turned off now due to
  // this case causing a regression failure.
  hbaseRowFormat = ((hasAddedColumns && !isAlignedFormat) || 
		    (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) == DF_OFF )) ? 
    ExpTupleDesc::SQLARK_EXPLODED_FORMAT : ExpTupleDesc::SQLMX_ALIGNED_FORMAT ;

  // build key information
  keyRangeGen * keyInfo = 0;
  NABoolean reverseScan = getReverseScan(); 

  expGen->buildKeyInfo(&keyInfo, // out
		       generator,
		       getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
		       getIndexDesc()->getIndexKey(),
		       getBeginKeyPred(),
		       (searchKey() && searchKey()->isUnique() ? NULL : getEndKeyPred()),
		       searchKey(),
		       getMdamKeyPtr(),
		       reverseScan,
		       ExpTupleDesc::SQLMX_KEY_FORMAT);

  const ValueIdList &retColumnList = retColRefSet_; 
  // Always get the index name -- it will be the base tablename for
  // primary access if it is trafodion table.
  char * tablename = NULL;
  char * snapshotName = NULL;
  LatestSnpSupportEnum  latestSnpSupport=  latest_snp_supported;
  if ((getTableDesc()->getNATable()->isHbaseRowTable()) ||
      (getTableDesc()->getNATable()->isHbaseCellTable()) ||
      (getTableName().getQualifiedNameObj().isHbaseMappedName()))
    {
      tablename =
        space->AllocateAndCopyToAlignedSpace(
                                             GenGetQualifiedName(getTableName().getQualifiedNameObj().getObjectName()), 0);
      latestSnpSupport = latest_snp_not_trafodion_table;
    }
  else
    {
      if (getIndexDesc() && getIndexDesc()->getNAFileSet())
      {
         tablename = space->AllocateAndCopyToAlignedSpace(GenGetQualifiedName(getIndexDesc()->getNAFileSet()->getFileSetName()), 0);
         if (getIndexDesc()->isClusteringIndex())
         {
            //base table
            snapshotName = (char*)getTableDesc()->getNATable()->getSnapshotName() ;
           if (snapshotName == NULL)
             latestSnpSupport = latest_snp_no_snapshot_available;
          }
          else
            latestSnpSupport = latest_snp_index_table;
      }
    }

  if (! tablename) 
     tablename =
        space->AllocateAndCopyToAlignedSpace(
                                           GenGetQualifiedName(getTableName()), 0);

  ValueIdList columnList;
  if ((getTableDesc()->getNATable()->isSeabaseTable()) &&
      (NOT isAlignedFormat) &&
      (NOT isHbaseMapFormat))
    sortValues(retColumnList, columnList,
	       (getIndexDesc()->getNAFileSet()->getKeytag() != 0));
  else
    {
      for (Lng32 i = 0; i < getIndexDesc()->getIndexColumns().entries(); i++)
	{
	  columnList.insert(getIndexDesc()->getIndexColumns()[i]);
	}
     }

  ValueIdList hbTsVIDlist;
  ValueIdList hbVersVIDlist;
  for (CollIndex hi = 0; hi < retColumnList.entries(); hi++)
    {
      ValueId vid = retColumnList[hi];
      
      if ((vid.getItemExpr()->getOperatorType() != ITM_HBASE_TIMESTAMP) &&
          (vid.getItemExpr()->getOperatorType() != ITM_HBASE_VERSION))
        continue;
      
      if (vid.getItemExpr()->getOperatorType() == ITM_HBASE_TIMESTAMP)
        hbTsVIDlist.insert(vid);
      else
        hbVersVIDlist.insert(vid);

      ValueId tsValsVID = 
        (vid.getItemExpr()->getOperatorType() == ITM_HBASE_TIMESTAMP 
         ? ((HbaseTimestamp*)vid.getItemExpr())->tsVals()->getValueId()
         : ((HbaseVersion*)vid.getItemExpr())->tsVals()->getValueId());

      Attributes * attr  = generator->addMapInfo(tsValsVID, 0)->getAttr();
      attr->setAtp(work_atp);
      if (vid.getItemExpr()->getOperatorType() == ITM_HBASE_TIMESTAMP)
        attr->setAtpIndex(hbaseTimestampTuppIndex);
      else
         attr->setAtpIndex(hbaseVersionTuppIndex);

      attr->setTupleFormat(ExpTupleDesc::SQLARK_EXPLODED_FORMAT);
      attr->setOffset(0);
      attr->setNullIndOffset(-1);
    }

  // contains source values corresponding to the key columns that will be used
  // to create the encoded key.  
  ValueIdArray encodedKeyExprVidArr(getIndexDesc()->getIndexKey().entries());
  const CollIndex numColumns = columnList.entries();

  NABoolean longVC = FALSE;
  for (CollIndex ii = 0; ii < numColumns; ii++)
    {
     ItemExpr * col_node = ((columnList[ii]).getValueDesc())->getItemExpr();

     const NAType &givenType = col_node->getValueId().getType();
     int res;    
     ItemExpr *asciiValue = NULL;
     ItemExpr *castValue = NULL;
     
     if ((isHbaseMapFormat) && 
         (getTableDesc()->getNATable()->isHbaseDataFormatString()))
       {
         res = createAsciiColAndCastExpr3
           (
                generator,       // for heap
                givenType,       // [IN] Actual type of column
                asciiValue,      // [OUT] Returned expression for ascii rep.
                castValue        // [OUT] Returned expression for binary rep.
            );

       }
     else
       {
         res = createAsciiColAndCastExpr2
           (
                generator,       // for heap
                col_node,
                givenType,       // [IN] Actual type of HDFS column
                asciiValue,      // [OUT] Returned expression for ascii rep.
                castValue,       // [OUT] Returned expression for binary rep.
                isAlignedFormat
            );
       }

     GenAssert(res == 1 && castValue != NULL,
	       "Error building expression tree for cast output value");
     if (asciiValue)
       {
	 asciiValue->synthTypeAndValueId();
	 asciiValue->bindNode(generator->getBindWA());
	 asciiVids.insert(asciiValue->getValueId());
       }

     castValue->bindNode(generator->getBindWA());
     convertExprCastVids.insert(castValue->getValueId());

     NAColumn * nac = NULL;
     if (col_node->getOperatorType() == ITM_BASECOLUMN)
       {
	 nac = ((BaseColumn*)col_node)->getNAColumn();
       }
     else if (col_node->getOperatorType() == ITM_INDEXCOLUMN)
       {
	 nac = ((IndexColumn*)col_node)->getNAColumn();
       }

     if (getMdamKeyPtr() && nac)
       {
	 // find column position of the key col and add that value id to the vidlist
	 Lng32 colPos = getIndexDesc()->getNAFileSet()->getIndexKeyColumns().getColumnPosition(*nac);
	 if (colPos != -1)
	   encodedKeyExprVidArr.insertAt(colPos, castValue->getValueId());
       }

     // if any hbase info functions are specified(hbase_timestamp, hbase_version),
     // then update them with index of the col.
     // At runtime, ts and version values are populated in a ts/vers array that has
     // one entry for each column. The index values updated here is used to access
     // that info at runtime.
     HbaseAccess_updateHbaseInfoNode(hbTsVIDlist, nac->getColName(), ii);
     HbaseAccess_updateHbaseInfoNode(hbVersVIDlist, nac->getColName(), ii);

     if ((DFS2REC::isAnyVarChar(givenType.getFSDatatype())) &&
          (givenType.getTotalSize() > 1024))
        longVC = TRUE;

    } // for (ii = 0; ii < numCols; ii++)

  // use CIF if there are long varchars (> 1K length) and CIF has not
  // been explicitly turned off.
  if (longVC && (CmpCommon::getDefault(COMPRESSED_INTERNAL_FORMAT) != DF_OFF))
    generator->setCompressedInternalFormat();

  ValueIdList encodedKeyExprVids(encodedKeyExprVidArr);

  ExpTupleDesc::TupleDataFormat asciiRowFormat = 
    (isAlignedFormat ?
     ExpTupleDesc::SQLMX_ALIGNED_FORMAT :
     ExpTupleDesc::SQLARK_EXPLODED_FORMAT);

  // Add ascii columns to the MapTable. After this call the MapTable
  // has ascii values in the work ATP at index asciiTuppIndex.
  const NAColumnArray * colArray = NULL;
  unsigned short pcm = expGen->getPCodeMode();
  if ((asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) &&
      (hasAddedColumns))
    {
      colArray = &getIndexDesc()->getAllColumns();

      // current pcode does not handle added columns in aligned format rows.
      // Generated pcode assumes that the row does not have missing columns.
      // Missing columns can only be evaluated using regular clause expressions.
      // Set this flag so both pcode and clauses are saved in generated expr.
      // At runtime, if a row has missing/added columns, the clause expression 
      // is evaluated. Otherwise it is evaluated using pcode.
      // See ExHbaseAccess.cpp::missingValuesInAlignedFormatRow for details.
      expGen->setSaveClausesInExpr(TRUE);
    }

  expGen->processValIdList(
			   asciiVids,                             // [IN] ValueIdList
			   asciiRowFormat,                        // [IN] tuple data format
			   asciiRowLen,                           // [OUT] tuple length 
			   work_atp,                              // [IN] atp number
			   asciiTuppIndex,                        // [IN] index into atp
			   &asciiTupleDesc,                       // [optional OUT] tuple desc
			   ExpTupleDesc::LONG_FORMAT,            // [optional IN] desc format
                           0,
                           NULL,
                           (NAColumnArray*)colArray,
                           isSecondaryIndex);
  
  work_cri_desc->setTupleDescriptor(asciiTuppIndex, asciiTupleDesc);

  // add hbase info nodes to convert list.
  convertExprCastVids.insert(hbTsVIDlist);
  convertExprCastVids.insert(hbVersVIDlist);
  for (CollIndex i = 0; i < columnList.entries(); i++) 
    {
      ValueId colValId = columnList[i];
      generator->addMapInfo(colValId, 0)->getAttr();
    } // for

  ExpTupleDesc * tuple_desc = 0;
  ExpTupleDesc * hdfs_desc = 0;
  ULng32 executorPredColsRecLength; 
  ULng32 convertRowLen = 0;
  ExpHdrInfo hdrInfo;

  expGen->generateContiguousMoveExpr(
      convertExprCastVids,              // [IN] source ValueIds
      FALSE,                                // [IN] add convert nodes?
      work_atp,                             // [IN] target atp number
      convertTuppIndex,                 // [IN] target tupp index
      hbaseRowFormat,                        // [IN] target tuple format
      convertRowLen,             // [OUT] target tuple length
      &convert_expr,                // [OUT] move expression
      &convertTupleDesc,                     // [optional OUT] target tuple desc
      ExpTupleDesc::LONG_FORMAT,       // [optional IN] target desc format
      NULL,
      NULL,
      0,
      NULL,
      FALSE,
      NULL,
      FALSE /* doBulkMove */);

  if ((asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT) &&
      (hasAddedColumns))
    {
      expGen->setPCodeMode(pcm);
      expGen->setSaveClausesInExpr(FALSE);
    }

  work_cri_desc->setTupleDescriptor(convertTuppIndex, convertTupleDesc);

  for (CollIndex i = 0; i < columnList.entries(); i++) 
    {
      ValueId colValId = columnList[i];
      ValueId castValId = convertExprCastVids[i];

      Attributes * colAttr = (generator->getMapInfo(colValId))->getAttr();
      Attributes * castAttr = (generator->getMapInfo(castValId))->getAttr();

      colAttr->copyLocationAttrs(castAttr);

    } // for

  for (CollIndex i = 0; i < hbTsVIDlist.entries(); i++) 
    {
      ValueId vid = hbTsVIDlist[i];
      
      generator->getMapInfo(vid)->codeGenerated();
    } // for

  for (CollIndex i = 0; i < hbVersVIDlist.entries(); i++) 
    {
      ValueId vid = hbVersVIDlist[i];
      
      generator->getMapInfo(vid)->codeGenerated();
    } // for

  if (addDefaultValues) 
    {
      expGen->addDefaultValues(columnList,
			       getIndexDesc()->getAllColumns(),
			       convertTupleDesc,
			       TRUE);
  
      if (asciiRowFormat == ExpTupleDesc::SQLMX_ALIGNED_FORMAT)
        {
          expGen->addDefaultValues(columnList,
                                   getIndexDesc()->getAllColumns(),
                                   asciiTupleDesc,
                                   TRUE);
        }
      else
        {
          // copy default values from convertTupleDesc to asciiTupleDesc
          expGen->copyDefaultValues(asciiTupleDesc, convertTupleDesc);
        }
    }

  ex_expr * encodedKeyExpr = NULL;
  ULng32 encodedKeyLen = 0;
  if (getMdamKeyPtr())
    {
      ULng32 firstKeyColumnOffset = 0;
      expGen->generateKeyEncodeExpr(getIndexDesc(),
				    work_atp, // (IN) Destination Atp
				    keyTuppIndex,
				    ExpTupleDesc::SQLMX_KEY_FORMAT,
				    encodedKeyLen,
				    &encodedKeyExpr,
				    FALSE,
				    firstKeyColumnOffset,
				    &encodedKeyExprVids);
    }

  Queue * listOfFetchedColNames = NULL;
  char * pkeyColName = NULL;
  if ((getTableDesc()->getNATable()->isSeabaseTable()) &&
      (isAlignedFormat))
    {
      listOfFetchedColNames = new(space) Queue(space);

      NAString cnInList(SEABASE_DEFAULT_COL_FAMILY);
      cnInList += ":";
      unsigned char c = 1;
      cnInList.append((char*)&c, 1);
      short len = cnInList.length();
      cnInList.prepend((char*)&len, sizeof(short));
      
      char * colNameInList =
        space->AllocateAndCopyToAlignedSpace(cnInList, 0);
      
      listOfFetchedColNames->insert(colNameInList);
    }
  else if ((getTableDesc()->getNATable()->isSeabaseTable()) &&
           (NOT isHbaseMapFormat))
    {
      listOfFetchedColNames = new(space) Queue(space);

      for (CollIndex c = 0; c < numColumns; c++)
	{
	  ItemExpr * col_node = ((columnList[c]).getValueDesc())->getItemExpr();

	  NAString cnInList;
	  char * colNameInList = NULL;
	  if (col_node->getOperatorType() == ITM_BASECOLUMN)
	    {
	      const NAColumn *nac = 
		((BaseColumn*)col_node)->getNAColumn();

	      HbaseAccess::createHbaseColId(nac, cnInList);

	      colNameInList = 
		space->AllocateAndCopyToAlignedSpace(cnInList, 0);
	    }
	  else if (col_node->getOperatorType() == ITM_INDEXCOLUMN)
	    {
	      const NAColumn *nac = 
		((IndexColumn*)col_node)->getNAColumn();

	      HbaseAccess::createHbaseColId(nac, cnInList,
					    (getIndexDesc()->getNAFileSet()->getKeytag() != 0));

	      colNameInList = 
		space->AllocateAndCopyToAlignedSpace(cnInList, 0);
	    }
	  else if (col_node->getOperatorType() == ITM_REFERENCE)
	    {
              GenAssert(0, "HbaseAccess::codeGen. Should not reach here.");

	      const NAString &cn =
		((ColReference*)col_node)->getCorrNameObj().getQualifiedNameObj().getObjectName();

	      cnInList = SEABASE_DEFAULT_COL_FAMILY;
	      cnInList += ":";
	      cnInList += cn;
	      colNameInList = 
		space->AllocateAndCopyToAlignedSpace(cnInList, 0);
	    }
	  
	  listOfFetchedColNames->insert(colNameInList);
	}
    }
  else if ((getTableDesc()->getNATable()->isSeabaseTable()) &&
           (isHbaseMapFormat))
    {
      listOfFetchedColNames = new(space) Queue(space);

      for (CollIndex c = 0; c < numColumns; c++)
	{
	  ItemExpr * col_node = ((columnList[c]).getValueDesc())->getItemExpr();

	  NAString cnInList;
	  char * colNameInList = NULL;
	  if ((col_node->getOperatorType() == ITM_BASECOLUMN) ||
              (col_node->getOperatorType() == ITM_INDEXCOLUMN))
	    {
	      const NAColumn *nac = NULL;
              if (col_node->getOperatorType() == ITM_BASECOLUMN)
		nac = ((BaseColumn*)col_node)->getNAColumn();
              else
                nac = ((IndexColumn*)col_node)->getNAColumn();

              cnInList = nac->getHbaseColFam();
              cnInList += ":";
              cnInList += nac->getColName();

              short len = cnInList.length();
              cnInList.prepend((char*)&len, sizeof(short));
	      colNameInList = 
		space->AllocateAndCopyToAlignedSpace(cnInList, 0);

              if ((nac->isPrimaryKey()) &&
                  (getTableDesc()->getNATable()->isHbaseDataFormatString()))
                pkeyColName = colNameInList;
	    }
	  else if (col_node->getOperatorType() == ITM_REFERENCE)
	    {
              GenAssert(0, "HbaseAccess::codeGen. Should not reach here.");
	    }
	  
	  listOfFetchedColNames->insert(colNameInList);
	}
    }
  else if ((getTableDesc()->getNATable()->isHbaseRowTable()) &&
	   (retHbaseColRefSet_.entries() > 0))
    {
      listOfFetchedColNames = new(space) Queue(space);

      NAList<NAString> sortedColList(generator->wHeap());
      sortValues(retHbaseColRefSet_, sortedColList);
      
      for (Lng32 ij = 0; ij < sortedColList.entries(); ij++)
	{
	  NAString colName = sortedColList[ij];
	  char * colNameInList = NULL;

	  short len = colName.length();
	  colName.prepend((char*)&len, sizeof(short));

	  colNameInList = 
	    space->AllocateAndCopyToAlignedSpace(colName, 0);

	  listOfFetchedColNames->insert(colNameInList);
	}

    }

 // generate explain selection expression, if present
  if (! executorPred().isEmpty())
    {
      ItemExpr * newPredTree = executorPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &scanExpr);
    }

  if (getOptStoi() && getOptStoi()->getStoi())
    generator->addSqlTableOpenInfo(getOptStoi()->getStoi());

  LateNameInfo* lateNameInfo = new(generator->wHeap()) LateNameInfo();
  char * compileTimeAnsiName = (char*)getOptStoi()->getStoi()->ansiName();

  lateNameInfo->setCompileTimeName(compileTimeAnsiName, space);
  lateNameInfo->setLastUsedName(compileTimeAnsiName, space);
  lateNameInfo->setNameSpace(COM_TABLE_NAME);
  if (getIndexDesc()->getNAFileSet()->getKeytag() != 0)
    // is an index.
    {
      lateNameInfo->setIndex(TRUE);
      lateNameInfo->setNameSpace(COM_INDEX_NAME);
    }
  generator->addLateNameInfo(lateNameInfo);

 // generate filter value expression, if present
  Queue * hbaseFilterColNames = NULL;
  Queue * hbaseCompareOpList = NULL;
  if (! hbaseFilterValueVIDlist_.isEmpty())
    {
      expGen->generateContiguousMoveExpr(
					 hbaseFilterValueVIDlist_,      // [IN] source ValueIds
					 FALSE,                                // [IN] add convert nodes?
					 work_atp,                            // [IN] target atp number
					 hbaseFilterValTuppIndex,    // [IN] target tupp index
					 asciiRowFormat,                        // [IN] target tuple format
					 hbaseFilterValRowLen,             // [OUT] target tuple length
					 &hbaseFilterValExpr,                // [OUT] move expression
					 &hbaseFilterValTupleDesc,                     // [optional OUT] target tuple desc
					 ExpTupleDesc::LONG_FORMAT);     // [optional IN] target desc format

      work_cri_desc->setTupleDescriptor(hbaseFilterValTuppIndex, hbaseFilterValTupleDesc);
    }
  if (!hbaseFilterColVIDlist_.isEmpty()){// with unary operator we can have column without value
      genListOfColNames(generator, getIndexDesc(), hbaseFilterColVIDlist_,
			hbaseFilterColNames);

      hbaseCompareOpList = new(generator->getSpace()) Queue(generator->getSpace());
      for (Lng32 i = 0; i < opList_.entries(); i++)
	{
	  char * op = space->AllocateAndCopyToAlignedSpace(opList_[i], 0);
	  hbaseCompareOpList->insert(op);
	}
    }

  ULng32 rowIdAsciiRowLen = 0; 
  ExpTupleDesc * rowIdAsciiTupleDesc = 0;
  ex_expr * rowIdExpr = NULL;
  ULng32 rowIdLength = 0;
  Queue * tdbListOfRangeRows = NULL;
  Queue * tdbListOfUniqueRows = NULL;

  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      // dont encode keys for hbase mapped tables since these tables
      // could be populated from outside of traf.
      NABoolean encodeKeys = TRUE;
      if (getTableDesc()->getNATable()->isHbaseMapTable())
        encodeKeys = FALSE;

      genRowIdExpr(generator,
		   getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
		   getHbaseSearchKeys(), 
		   work_cri_desc, work_atp,
		   rowIdAsciiTuppIndex, rowIdTuppIndex,
		   rowIdAsciiRowLen, rowIdAsciiTupleDesc,
		   rowIdLength, 
		   rowIdExpr,
                   encodeKeys);
    }
  else
    {
     genRowIdExprForNonSQ(generator,
		   getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
		   getHbaseSearchKeys(), 
		   work_cri_desc, work_atp,
		   rowIdAsciiTuppIndex, rowIdTuppIndex,
		   rowIdAsciiRowLen, rowIdAsciiTupleDesc,
		   rowIdLength, 
		   rowIdExpr);
    }

  genListsOfRows(generator,
		 listOfRangeRows_,
		 listOfUniqueRows_,
		 tdbListOfRangeRows,
		 tdbListOfUniqueRows);

  // The hbase row will be returned as the last entry of the returned atp.
  // Change the atp and atpindex of the returned values to indicate that.
  expGen->assignAtpAndAtpIndex(columnList,
			       0, returnedDesc->noTuples()-1);
  
  if (NOT hbTsVIDlist.isEmpty())
    expGen->assignAtpAndAtpIndex(hbTsVIDlist,
                                 0, returnedDesc->noTuples()-1);

  if (NOT hbVersVIDlist.isEmpty())
    expGen->assignAtpAndAtpIndex(hbVersVIDlist,
                                 0, returnedDesc->noTuples()-1);
 
  Cardinality expectedRows = (Cardinality) getEstRowsUsed().getValue();
  ULng32 buffersize = 3 * getDefault(GEN_DPSO_BUFFER_SIZE);
  queue_index upqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_UP);
  queue_index downqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_DOWN);
  Int32 numBuffers = getDefault(GEN_DPUO_NUM_BUFFERS);

  // Compute the buffer size based on upqueue size and row size.
  // Try to get enough buffer space to hold twice as many records
  // as the up queue.
  //
  // This should be more sophisticate than this, and should maybe be done
  // within the buffer class, but for now this will do.
  // 
  UInt32 FiveM = 5*1024*1024;

  // If returnedrowlen > 5M, then set upqueue entries to 2.
  UInt32 bufRowlen = MAXOF(convertRowLen, 1000);
  if (bufRowlen > FiveM)
    upqueuelength = 2;

  ULng32 cbuffersize = 
    SqlBufferNeededSize((upqueuelength * 2 / numBuffers),
			bufRowlen); //returnedRowlen);
  // But use at least the default buffer size.
  //
  buffersize = buffersize > cbuffersize ? buffersize : cbuffersize;

  // Cap the buffer size at 5M and adjust upqueue entries.
  // Do this only if returnrowlen is not > 5M
  if ((bufRowlen <= FiveM) && (buffersize > FiveM))
    {
      buffersize = FiveM;

      upqueuelength = ((buffersize / bufRowlen) * numBuffers)/2;
    }

  Int32 computedHBaseRowSizeFromMetaData = getTableDesc()->getNATable()->computeHBaseRowSizeFromMetaData();
  if (computedHBaseRowSizeFromMetaData * getEstRowsAccessed().getValue()  <
                getDefault(TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD)*1024*1024)
    latestSnpSupport = latest_snp_small_table;

  NAString serverNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_SERVER);
  NAString zkPortNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_ZOOKEEPER_PORT);
  char * server = space->allocateAlignedSpace(serverNAS.length() + 1);
  strcpy(server, serverNAS.data());
  char * zkPort = space->allocateAlignedSpace(zkPortNAS.length() + 1);
  strcpy(zkPort, zkPortNAS.data());



  NAString snapNameNAS;
  char * snapName= NULL;
  NAString tmpLocNAS ;
  char * tmpLoc = NULL;
  NAString snapScanRunIdNAS;
  char * snapScanRunId = NULL;
  NAString snapScanHelperTabNAS ;
  char * snapScanHelperTab = NULL;


  ComTdbHbaseAccess::HbaseSnapshotScanAttributes * snapAttrs =
       new (space) ComTdbHbaseAccess::HbaseSnapshotScanAttributes();

  if (isSnapshotScanFeasible( latestSnpSupport,tablename))
  {
    snapAttrs->setUseSnapshotScan(TRUE );
    if (snpType_ == SNP_LATEST)
    {
      CMPASSERT(snapshotName != NULL);
      snapNameNAS = snapshotName;
    }
    else if (snpType_ == SNP_SUFFIX)
    {
      //this case is mainly used with bulk unload
      snapNameNAS= tablename;
      snapNameNAS.append("_");
      snapNameNAS.append( ActiveSchemaDB()->getDefaults().getValue(
              TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX));
    }
    snapName= space->allocateAlignedSpace(snapNameNAS.length() + 1);
    strcpy(snapName, snapNameNAS.data());
    snapAttrs->setSnapshotName(snapName);

    tmpLocNAS = generator->getSnapshotScanTmpLocation();
    CMPASSERT(tmpLocNAS[tmpLocNAS.length()-1] =='/');
    tmpLoc = space->allocateAlignedSpace(tmpLocNAS.length() + 1);
    strcpy(tmpLoc, tmpLocNAS.data());

    snapAttrs->setSnapScanTmpLocation(tmpLoc);

    snapAttrs->setSnapshotScanTimeout(getDefault(TRAF_TABLE_SNAPSHOT_SCAN_TIMEOUT));
    //create the strings from generator heap
    NAString  * tbl = new NAString( tablename,generator->wHeap());
    generator->objectNames().insert(*tbl);//verified that it detects duplicate names in the NASet
  }
  ComTdbHbaseAccess::HbasePerfAttributes * hbpa =
    new(space) ComTdbHbaseAccess::HbasePerfAttributes();
  if (CmpCommon::getDefault(COMP_BOOL_184) == DF_ON)
    hbpa->setUseMinMdamProbeSize(TRUE);

  Lng32 hbaseRowSize;
  Lng32 hbaseBlockSize;
   if(getIndexDesc() && getIndexDesc()->getNAFileSet())
   {
     const NAFileSet * fileset = getIndexDesc()->getNAFileSet() ;
     hbaseRowSize = fileset->getRecordLength();
     hbaseRowSize += ((NAFileSet *)fileset)->getEncodedKeyLength();
     hbaseBlockSize = fileset->getBlockSize();
   }
   else
   {
     hbaseRowSize = computedHBaseRowSizeFromMetaData;
     hbaseBlockSize = CmpCommon::getDefaultLong(HBASE_BLOCK_SIZE);
   }

  generator->setHBaseNumCacheRows(MAXOF(getEstRowsAccessed().getValue(),
                                        getMaxCardEst().getValue()), 
                                  hbpa, hbaseRowSize,samplePercent()) ;
  generator->setHBaseCacheBlocks(hbaseRowSize,
                                 getEstRowsAccessed().getValue(),hbpa);
  generator->setHBaseSmallScanner(hbaseRowSize,
                                  getEstRowsAccessed().getValue(),
                                  hbaseBlockSize,
                                  hbpa);
  generator->setHBaseParallelScanner(hbpa);


  ComTdbHbaseAccess::HbaseAccessOptions * hbo = NULL;
  if (getHbaseAccessOptions())
    {
      hbo = new(space) ComTdbHbaseAccess::HbaseAccessOptions
        (getHbaseAccessOptions()->getHbaseVersions());
    }

  // create hbasescan_tdb
  ComTdbHbaseAccess *hbasescan_tdb = new(space) 
    ComTdbHbaseAccess(
		      ComTdbHbaseAccess::SELECT_,
		      tablename,

		      convert_expr,
		      scanExpr,
		      rowIdExpr,
		      NULL, // updateExpr
		      NULL, // mergeInsertExpr
		      NULL, // mergeInsertRowIdExpr
		      NULL, // mergeUpdScanExpr
		      NULL, // projExpr
		      NULL, // returnedUpdatedExpr
		      NULL, // returnMergeUpdateExpr
		      encodedKeyExpr, 
		      NULL, // keyColValExpr
		      hbaseFilterValExpr,

		      asciiRowLen,
		      convertRowLen,
		      0, // updateRowLen
		      0, // mergeInsertRowLen
		      0, // fetchedRowLen
		      0, // returnedRowLen

		      rowIdLength,
		      convertRowLen,
		      rowIdAsciiRowLen,
		      (keyInfo ? keyInfo->getKeyLength() : 0),
		      0, // keyColValLen
		      hbaseFilterValRowLen,

		      asciiTuppIndex,
		      convertTuppIndex,
		      0, // updateTuppIndex
		      0, // mergeInsertTuppIndex
		      0, // mergeInsertRowIdTuppIndex
		      0, // mergeIUDIndicatorTuppIndex
		      0, // returnedFetchedTuppIndex
		      0, // returnedUpdatedTuppIndex

		      rowIdTuppIndex,
		      returnedDesc->noTuples()-1,
		      rowIdAsciiTuppIndex,
		      keyTuppIndex,
		      0, // keyColValTuppIndex
		      hbaseFilterValTuppIndex,

		      (hbTsVIDlist.entries() > 0 ? hbaseTimestampTuppIndex : 0),
		      (hbVersVIDlist.entries() > 0 ? hbaseVersionTuppIndex : 0),

		      tdbListOfRangeRows,
		      tdbListOfUniqueRows,
		      listOfFetchedColNames,
		      hbaseFilterColNames,
		      hbaseCompareOpList,

		      keyInfo,
		      NULL,

		      work_cri_desc,
		      givenDesc,
		      returnedDesc,
		      downqueuelength,
		      upqueuelength,
                      expectedRows,
		      numBuffers,
		      buffersize,

		      server,
                      zkPort,
		      hbpa,
		      samplePercent(),
		      snapAttrs,

                      hbo,

                      pkeyColName
		      );

  generator->initTdbFields(hbasescan_tdb);

  if (getTableDesc()->getNATable()->isHbaseRowTable()) //rowwiseHbaseFormat())
    hbasescan_tdb->setRowwiseFormat(TRUE);

  hbasescan_tdb->setUseCif(hbaseRowFormat == 
			   ExpTupleDesc::SQLMX_ALIGNED_FORMAT);

  if (getTableDesc()->getNATable()->isSeabaseTable())
    {
      hbasescan_tdb->setSQHbaseTable(TRUE);

      if (isAlignedFormat)
        hbasescan_tdb->setAlignedFormat(TRUE);

      if (isHbaseMapFormat)
        {
          hbasescan_tdb->setHbaseMapTable(TRUE);

          if (getTableDesc()->getNATable()->getClusteringIndex()->hasSingleColVarcharKey())
            hbasescan_tdb->setKeyInVCformat(TRUE);
        }

      if (getTableDesc()->getNATable()->isEnabledForDDLQI())
        generator->objectUids().insert(
          getTableDesc()->getNATable()->objectUid().get_value());
    }
 
  if (keyInfo && searchKey() && searchKey()->isUnique())
    hbasescan_tdb->setUniqueKeyInfo(TRUE);

  if (uniqueRowsetHbaseOper())
    {
      hbasescan_tdb->setRowsetOper(TRUE);
      hbasescan_tdb->setHbaseRowsetVsbbSize(getDefault(HBASE_ROWSET_VSBB_SIZE));
      
    }

  if ((accessOptions().userSpecified()) &&
    //      (accessOptions().getDP2LockFlags().getConsistencyLevel() == DP2LockFlags::READ_UNCOMMITTED))
      (accessOptions().accessType() == TransMode::READ_UNCOMMITTED_ACCESS_))
    {
      hbasescan_tdb->setReadUncommittedScan(TRUE);
    }

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(hbasescan_tdb, 0, 0, generator));
  }

  if ((generator->computeStats()) && 
      (generator->collectStatsType() == ComTdb::PERTABLE_STATS
      || generator->collectStatsType() == ComTdb::OPERATOR_STATS))
    {
      hbasescan_tdb->setPertableStatsTdbId((UInt16)generator->
					   getPertableStatsTdbId());
    }

  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, hbasescan_tdb);

  return 0;
}

short HbaseAccessCoProcAggr::codeGen(Generator * generator)
{
  Space * space          = generator->getSpace();
  ExpGenerator * expGen = generator->getExpGenerator();

  // allocate a map table for the retrieved columns
  //  generator->appendAtEnd();
  MapTable * last_map_table = generator->getLastMapTable();
 
  ex_expr *scanExpr = 0;
  ex_expr *proj_expr = 0;
  ex_expr *convert_expr = NULL;

  ex_cri_desc * givenDesc 
    = generator->getCriDesc(Generator::DOWN);

  ex_cri_desc * returnedDesc = NULL;

  const Int32 work_atp = 1;
  const Int32 convertTuppIndex = 2;
  const Int32 rowIdTuppIndex = 3;
  const Int32 asciiTuppIndex = 4;
  const Int32 rowIdAsciiTuppIndex = 5;
  const Int32 keyTuppIndex = 6;

  ULng32 asciiRowLen; 
  ExpTupleDesc * asciiTupleDesc = 0;
  ExpTupleDesc * convertTupleDesc = NULL;

  ex_cri_desc * work_cri_desc = NULL;
  work_cri_desc = new(space) ex_cri_desc(7, space);

  returnedDesc = new(space) ex_cri_desc(givenDesc->noTuples() + 1, space);

  ExpTupleDesc::TupleDataFormat asciiRowFormat = 
    ExpTupleDesc::SQLARK_EXPLODED_FORMAT;
  ExpTupleDesc::TupleDataFormat hbaseRowFormat = 
    ExpTupleDesc::SQLMX_ALIGNED_FORMAT;
  ValueIdList asciiVids;
  ValueIdList executorPredCastVids;
  ValueIdList convertExprCastVids;

  NABoolean addDefaultValues = TRUE;
  NABoolean hasAddedColumns = FALSE;
  if (getTableDesc()->getNATable()->hasAddedColumn())
    hasAddedColumns = TRUE;

  // build key information
  keyRangeGen * keyInfo = 0;
  NABoolean reverseScan = getReverseScan(); 

  expGen->buildKeyInfo(&keyInfo, // out
		       generator,
		       getIndexDesc()->getNAFileSet()->getIndexKeyColumns(),
		       getIndexDesc()->getIndexKey(),
		       getBeginKeyPred(),
		       (searchKey() && searchKey()->isUnique() ? NULL : getEndKeyPred()),
		       searchKey(),
		       getMdamKeyPtr(),
		       reverseScan,
		       ExpTupleDesc::SQLMX_KEY_FORMAT);

  //  const ValueIdList &retColumnList = getIndexDesc()->getIndexColumns();
  const ValueIdList &retColumnList = retColRefSet_;

  // generate explain selection expression, if present
  if (! executorPred().isEmpty())
    {
      ItemExpr * newPredTree = executorPred().rebuildExprTree(ITM_AND,TRUE,TRUE);
      expGen->generateExpr(newPredTree->getValueId(), ex_expr::exp_SCAN_PRED,
			   &scanExpr);
    }

  Cardinality expectedRows = (Cardinality) getEstRowsUsed().getValue();
  ULng32 buffersize = 3 * getDefault(GEN_DPSO_BUFFER_SIZE);
  queue_index upqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_UP);
  queue_index downqueuelength = (queue_index)getDefault(GEN_DPSO_SIZE_DOWN);
  Int32 numBuffers = getDefault(GEN_DPUO_NUM_BUFFERS);

  // Compute the buffer size based on upqueue size and row size.
  // Try to get enough buffer space to hold twice as many records
  // as the up queue.
  //
  // This should be more sophisticate than this, and should maybe be done
  // within the buffer class, but for now this will do.
  // 
  ULng32 cbuffersize = 
    SqlBufferNeededSize((upqueuelength * 2 / numBuffers),
			1000); //returnedRowlen);
  // But use at least the default buffer size.
  //
  buffersize = buffersize > cbuffersize ? buffersize : cbuffersize;
  
  // Always get the index name -- it will be the base tablename for
  // primary access.
  char * tablename = NULL;

  if (getTableDesc()->getNATable()->isHbaseMapTable())
    {
      tablename =
        space->AllocateAndCopyToAlignedSpace(
             GenGetQualifiedName(getTableName().getQualifiedNameObj().getObjectName()), 0);
    }
  else
    {
      tablename = 
        space->AllocateAndCopyToAlignedSpace(
             GenGetQualifiedName(getTableName()), 0);
    }

  NAString serverNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_SERVER);
  NAString zkPortNAS = ActiveSchemaDB()->getDefaults().getValue(HBASE_ZOOKEEPER_PORT);
  char * server = space->allocateAlignedSpace(serverNAS.length() + 1);
  strcpy(server, serverNAS.data());
  char * zkPort = space->allocateAlignedSpace(zkPortNAS.length() + 1);
  strcpy(zkPort, zkPortNAS.data());

  ComTdbHbaseAccess::HbasePerfAttributes * hbpa =
    new(space) ComTdbHbaseAccess::HbasePerfAttributes();
  if (CmpCommon::getDefault(HBASE_CACHE_BLOCKS) == DF_ON)
    hbpa->setCacheBlocks(TRUE);
  hbpa->setNumCacheRows(CmpCommon::getDefaultNumeric(HBASE_NUM_CACHE_ROWS_MIN));

  // create hdfsscan_tdb
  ComTdbHbaseAccess *hbasescan_tdb = new(space) 
    ComTdbHbaseAccess(
		      ComTdbHbaseAccess::COPROC_,
		      tablename,

		      NULL, // convert_expr,
		      NULL, // scanExpr,
		      NULL, // rowIdExpr,
		      NULL, // updateExpr
		      NULL, // mergeInsertExpr
		      NULL, // mergeInsertRowIdExpr
		      NULL, // mergeUpdScanExpr
		      NULL, // projExpr
		      NULL, // returnedUpdatedExpr
		      NULL, // returnMergeUpdateExpr
		      NULL, // encodedKeyExpr, 
		      NULL, // keyColValExpr
		      NULL, // hbaseFilterExpr

		      0, // asciiRowLen,
		      0, // convertRowLen,
		      0, // updateRowLen
		      0, // mergeInsertRowLen
		      0, // fetchedRowLen
		      0, // returnedRowLen

		      0, // rowIdLength,
		      0, // convertRowLen,
		      0, // rowIdAsciiRowLen,
		      0, 
		      0, // keyColValLen
		      0, // hbaseFilterValLen

		      0, // asciiTuppIndex,
		      0, // convertTuppIndex,
		      0, // updateTuppIndex
		      0, // mergeInsertTuppIndex
		      0, // mergeInsertRowIdTuppIndex
		      0, // mergeIUDIndicatorTuppIndex
		      0, // returnedFetchedTuppIndex
		      0, // returnedUpdatedTuppIndex

		      0, // rowIdTuppIndex,
		      0, // returnedDesc->noTuples()-1,
		      0, // rowIdAsciiTuppIndex,
		      0, // keyTuppIndex,
		      0, // keyColValTuppIndex
		      0, // hbaseFilterValTuppIndex

                      0, // hbaseTimestampTuppIndex
                      0, // hbaseVersionTuppIndex

		      NULL, // tdbListOfRangeRows,
		      NULL, // tdbListOfUniqueRows,
		      NULL, // listOfFetchedColNames,
		      NULL,
		      NULL,

		      NULL, // keyInfo,
		      NULL,

		      work_cri_desc,
		      givenDesc,
		      returnedDesc,
		      downqueuelength,
		      upqueuelength,
                      expectedRows,
		      numBuffers,
		      buffersize,

		      server,
                      zkPort,
		      hbpa
		      );

  generator->initTdbFields(hbasescan_tdb);

  if (getTableDesc()->getNATable()->isSeabaseTable())
  {
    hbasescan_tdb->setSQHbaseTable(TRUE);
    if (getTableDesc()->getNATable()->isEnabledForDDLQI())
      generator->objectUids().insert(
        getTableDesc()->getNATable()->objectUid().get_value());
  }

  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(hbasescan_tdb, 0, 0, generator));
  }

  if ((generator->computeStats()) && 
      (generator->collectStatsType() == ComTdb::PERTABLE_STATS
      || generator->collectStatsType() == ComTdb::OPERATOR_STATS))
    {
      hbasescan_tdb->setPertableStatsTdbId((UInt16)generator->
					   getPertableStatsTdbId());
    }

  generator->setCriDesc(givenDesc, Generator::DOWN);
  generator->setCriDesc(returnedDesc, Generator::UP);
  generator->setGenObj(this, hbasescan_tdb);

  return 0;
}
