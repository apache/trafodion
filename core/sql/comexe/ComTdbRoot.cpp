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
****************************************************************************
*
* File:         ComTdbRoot.cpp
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#include "LateBindInfo.h"
#include "ComTdbRoot.h"
#include "ComTdbCommon.h"
#include "FragDir.h"
#include "ComQueue.h"

////////////////////////////////////////////////////////////////////////
//  TDB procedures
ComTdbRoot::ComTdbRoot()
     : ComTdb(ComTdb::ex_ROOT,eye_ROOT),
       childTdb(NULL), criDesc_(NULL), inputExpr_(NULL), outputExpr_(NULL),
       pkeyExpr_(NULL), firstNRows_(-1), stoiList_(NULL),
       predExpr_(NULL),
       uniqueExecuteIdOffset_(-1), compoundStmtsInfo_(0), qCacheInfo_(NULL),
       cacheVarsSize_(0), // Triggers
       triggersStatusOffset_(-1),
       triggersCount_(0),
       triggersList_(NULL),
       udrStoiList_(NULL),
       notAtomicFailureLimit_(0),
       queryCostInfo_(NULL),
       compilerStatsInfo_(NULL),
       maxResultSets_(0),
       uninitializedMvCount_(0),
       uninitializedMvList_(NULL),
       cpuLimit_(0),
       cpuLimitCheckFreq_(32),
       rwrsInfo_(NULL),
       rtFlags1_(0),
       rtFlags2_(0),
       rtFlags3_(0),
       rtFlags4_(0),
       rtFlags5_(0),
       compilationStatsData_(NULL),
       objectUidList_(NULL),
       cursorType_(SQL_READONLY_CURSOR),	
       bmoMemLimitPerNode_(0),
       estBmoMemPerNode_(0)
{
  //setPlanVersion(ComVersion_GetCurrentPlanVersion());
      
}

void ComTdbRoot::init(ComTdb * child_tdb,
		      ex_cri_desc * cri_desc,
		      InputOutputExpr * input_expr, 
		      InputOutputExpr * output_expr,
		      Lng32 input_vars_size,
		      ex_expr * pkey_expr,
		      ULng32 pkey_len,
                      ex_expr * pred_expr,
		      ex_cri_desc * work_cri_desc,
		      ExFragDir *fragDir,
		      TransMode * transMode,
		      char * fetchedCursorName,
		      short fetchedCursorHvar,
		      NABoolean delCurrOf,
		      Lng32 numUpdateCol,
		      Lng32 *updateColList,
		      NABoolean selectInto,
		      short tableCount,
		      Int64 firstNRows,
		      NABoolean userInputVars,
		      double cost,
		      SqlTableOpenInfo **stoiList,
		      LateNameInfoList * lateNameInfoList,
		      Queue *viewStoiList,
		      TrafQuerySimilarityInfo * qsi,
		      Space *space,
                      Lng32 uniqueExecuteIdOffset, // ++Triggers -
		      Lng32 triggersStatusOffset,
		      short triggersCount,
		      Int64 *triggersList,
		      short tempTableCount,
		      short baseTablenamePosition,
		      NABoolean updDelInsert,
		      NABoolean retryableStmt,
		      NABoolean streamScan,
		      NABoolean embeddedUpdateOrDelete,
		      Int32 streamTimeout,
		      Int64 explainPlanId,
		      NABasicPtr qCacheInfo,
		      Int32 cacheVarsSize,
		      SqlTableOpenInfo **udrStoiList,
                      short udrCount,
                      short maxResultSets,
		      NABasicPtr queryCostInfo,
		      UninitializedMvName *uninitializedMvList,
		      short uninitializedMvCount,
		      NABasicPtr compilerStatsInfo,
		      NABasicPtr rwrsInfo,
                      Int32 numObjectUIDs,
                      Int64 *objectUIDs,
                      CompilationStatsData *compilationStatsData,
                      char * snapTmpLocation,
                      Queue * listOfSnapshotscanTables)
{
  rtFlags1_ = 0;
  rtFlags2_ = 0;
  rtFlags3_ = 0;
  rtFlags4_ = 0;
  rtFlags5_ = 0;

  childTdb = child_tdb;
  criDesc_ = cri_desc;

  inputExpr_  = input_expr;

  outputExpr_ = output_expr;

  inputVarsSize_ = input_vars_size;

  pkeyExpr_ = pkey_expr;
  pkeyLen_  = pkey_len;

  predExpr_ = pred_expr;
  workCriDesc_ = work_cri_desc;

  fragDir_ = fragDir;

  transMode_ = transMode;

  fetchedCursorName_ = fetchedCursorName;
  fetchedCursorHvar_ = fetchedCursorHvar;

  snapshotscanTempLocation_=snapTmpLocation;
  listOfSnapshotScanTables_=listOfSnapshotscanTables;

  baseTablenamePosition_ = baseTablenamePosition;
  if ((fetchedCursorName_) ||
      (fetchedCursorHvar_ >= 0))
    {
      rtFlags1_ |= UPDATE_CURRENT_OF;
      if (delCurrOf)
	rtFlags1_ |= DELETE_CURRENT_OF;

    }

  
  if (userInputVars)
    rtFlags1_ |= USER_INPUT_VARS;

  if (selectInto)
    rtFlags1_ |= SELECT_INTO;

  if (updDelInsert)
  {
    rtFlags1_ |= UPD_DEL_INSERT;
    setMayAlterDb(TRUE);
  }

  if (retryableStmt)
    rtFlags1_ |= RETRYABLE_STMT;

  numUpdateCol_ = numUpdateCol;
  updateColList_ = updateColList;

  tableCount_ = tableCount;
  firstNRows_ = firstNRows;
  p_cost_ = cost;

  // stoiList_ has to be reallocated since it's an array of 64-bit pointer
  // objects while the given stoiList is an array of 32-bit pointers on a
  // 32-bit platform.
  //

  stoiList_.allocateAndCopyPtrArray(space,(void **)stoiList,tableCount);
  /*
  if (stoiList)
  {
    stoiList_  = (SqlTableOpenInfoPtr *)
      space->allocateAlignedSpace(tableCount * sizeof(SqlTableOpenInfoPtr));
    for(short i=0; i < tableCount; i++) stoiList_[i] = stoiList[i];
  }
  else stoiList_ = NULL;
  */

  viewStoiList_ = viewStoiList;
  if ( NULL != udrStoiList )
  {  
    udrStoiList_.allocateAndCopyPtrArray(space,(void **)udrStoiList,udrCount);
    setMayAlterDb(TRUE);
  }
  
  qsi_ = qsi;
  lateNameInfoList_ = lateNameInfoList;

  uniqueExecuteIdOffset_ = uniqueExecuteIdOffset; // Triggers
  triggersStatusOffset_ = triggersStatusOffset;
  triggersCount_ = triggersCount;
  triggersList_ = triggersList;
  tempTableCount_ = tempTableCount;

  if (streamScan)
    rtFlags1_ |= STREAM_SCAN;
  if (embeddedUpdateOrDelete)
    rtFlags1_ |= EMBEDDED_UPDATE_OR_DELETE;
  streamTimeout_ = streamTimeout;
  compoundStmtsInfo_ = 0;
  // for query caching
  qCacheInfo_ = qCacheInfo;
  cacheVarsSize_ = cacheVarsSize;

  queryCostInfo_ = queryCostInfo;

  compilerStatsInfo_ = compilerStatsInfo;

  rwrsInfo_ = rwrsInfo;

  numObjectUids_ = numObjectUIDs;
  objectUidList_ = objectUIDs; 

  compilationStatsData_ = compilationStatsData;

  explainPlanId_ = explainPlanId;

  // Version of plan fragment
  //setPlanVersion(ComVersion_GetCurrentPlanVersion());

  // UDR count
  udrCount_ = udrCount;

  // If this is a CALL statement, we store the maximum number of
  // result sets the procedure can return
  maxResultSets_ = maxResultSets;

  uninitializedMvList_ = uninitializedMvList;
  uninitializedMvCount_ = uninitializedMvCount;
  cursorType_ = SQL_READONLY_CURSOR;
  queryType_ = SQL_UNKNOWN;
  subqueryType_ = SQL_STMT_NA;
};

ComTdbRoot::~ComTdbRoot()
{
  childTdb = (ComTdbPtr)NULL;
  
  rtFlags1_ = 0;
};

Int32 ComTdbRoot::orderedQueueProtocol() const 
{
  return 0;
}

Long ComTdbRoot::pack(void * space)
{
  if (childTdb.isNull())
  {
     // Check if the child tdb was null after code generation. If it was not 
     // null then, something happened between then and now, when the plan
     // is being packed! Abort!
     if (!childTdbIsNull())
       abort();
  }
  childTdb.pack(space);
  if (childTdb.isNull())
  {
     // Check if the child tdb was null after code generation. If it was not 
     // null then, something happened during packing...Abort!
     if (!childTdbIsNull())
       abort();
  }
  criDesc_.pack(space);
  inputExpr_.pack(space);
  outputExpr_.pack(space);
  pkeyExpr_.pack(space);
  predExpr_.pack(space);
  workCriDesc_.pack(space);
  fragDir_.pack(space);
  transMode_.pack(space);
  fetchedCursorName_.pack(space);
  updateColList_.pack(space);
  triggersList_.pack(space); // Triggers
  uninitializedMvList_.pack(space);
  if(tableCount_ > 0) stoiList_.pack(space,tableCount_);
  lateNameInfoList_.pack(space);
  qsi_.pack(space);
  if (qCacheInfoIsClass())
    qcInfo()->pack(space);
  qCacheInfo_.pack(space);

  queryCostInfo_.pack(space);

  compilerStatsInfo_.pack(space);

  rwrsInfo_.pack(space);
  objectUidList_.pack(space);

  compilationStatsData_.pack(space);

  // Pack the queue backbone as well as the stoi objects in the queue.
  PackQueueOfNAVersionedObjects(viewStoiList_,space,SqlTableOpenInfo);

  // Pack the UDR stoi list
  if (udrCount_ > 0) udrStoiList_.pack (space, udrCount_);

  sikPtr_.pack(space);
  snapshotscanTempLocation_.pack(space);
  listOfSnapshotScanTables_.pack(space);
  return ComTdb::pack(space);
}

Lng32 ComTdbRoot::unpack(void * base, void * reallocator)
{
  if (childTdb.isNull())
  {
     // Check if the child tdb was null after code generation. If it was not 
     // null then, something happened between then and now, when the plan
     // is being unpacked! Abort!
     if (!childTdbIsNull())
       abort();
  }
  if(childTdb.unpack(base, reallocator)) return -1;
  if(criDesc_.unpack(base, reallocator)) return -1;
  if(inputExpr_.unpack(base, reallocator)) return -1;
  if(outputExpr_.unpack(base, reallocator)) return -1;
  if(pkeyExpr_.unpack(base, reallocator)) return -1;
  if(predExpr_.unpack(base, reallocator)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;
  if(fragDir_.unpack(base, reallocator)) return -1;
  if(transMode_.unpack(base, reallocator)) return -1;
  if(fetchedCursorName_.unpack(base)) return -1;
  if(updateColList_.unpack(base)) return -1;
  if(triggersList_.unpack(base)) return -1;  
  if(uninitializedMvList_.unpack(base)) return -1;
  if(tableCount_ > 0) if(stoiList_.unpack(base,tableCount_,reallocator)) return -1;
  if(lateNameInfoList_.unpack(base, reallocator)) return -1; 
  if(qsi_.unpack(base, reallocator)) return -1;
  if(qCacheInfo_.unpack(base)) return -1;
  if(qCacheInfoIsClass())
    if (qcInfo()->unpack(base)) return -1;

  if(queryCostInfo_.unpack(base)) return -1;

  if(compilerStatsInfo_.unpack(base)) return -1;

  if(rwrsInfo_.unpack(base)) return -1;
  if(objectUidList_.unpack(base)) return -1;

  if(compilationStatsData_.unpack(base, reallocator)) return -1;

  // Unpack the queue backbone as well as the stoi objects in the queue.
  UnpackQueueOfNAVersionedObjects(viewStoiList_,base,SqlTableOpenInfo,reallocator);

  // Unpack the UDR stoi list
  if(udrStoiList_.unpack (base,udrCount_, reallocator)) return -1;

  if (sikPtr_.unpack(base, reallocator)) return -1;
  if (snapshotscanTempLocation_.unpack(base)) return -1;
  if (listOfSnapshotScanTables_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);
}

NABoolean ComTdbRoot::isUpdateCol(const ComTdbRoot *updateTdb)
{
  // All columns are updateable.
  if (numUpdateCol_ == -1)
    return TRUE;

  // Determine if all columns in the update TDB are contained in
  // this TDB which represents the root of a cursor declaration.
  Int32 numFound = 0;

  for (Int32 i = 0; i < updateTdb->numUpdateCol_; i++) 
  {
    Lng32 updateCol = updateTdb->updateColList_[i];

    for (Int32 j = 0; j < numUpdateCol_; j++) 
    {
      if (updateCol == updateColList_[j])
      {
        numFound++;
	break;
      }
    }
  }

  return numFound == updateTdb->numUpdateCol_;
}

void ComTdbRoot::setDisplayExecution(Int32 flag)
{
  if (flag == 1) 
    rtFlags1_ |= DISPLAY_EXECUTION;
  else if (flag == 2)
    rtFlags1_ |= DISPLAY_EXECUTION_USING_MSGUI;
}

Int32 ComTdbRoot::displayExecution() const
{
  if (rtFlags1_ & DISPLAY_EXECUTION)
    return 1;
  else if (rtFlags1_ & DISPLAY_EXECUTION_USING_MSGUI)
    return 2;
  else 
    return 0;
}

void ComTdbRoot::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);
  
  if(flag & 0x00000008)
    {
      char buf[1000];
      str_sprintf(buf, "\nFor ComTdbRoot :\nFirstNRows = %ld, baseTablenamePosition = %d ",
		  firstNRows_,baseTablenamePosition_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      str_sprintf(buf, "queryType_ = %d, planVersion_ = %d ",
		  queryType_, planVersion_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      UInt32 lFlags = rtFlags1_%65536;
      UInt32 hFlags = (rtFlags1_- lFlags)/65536;
      str_sprintf(buf,"rtFlags1_ = %x%x ",hFlags,lFlags);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      lFlags = rtFlags2_%65536;
      hFlags = (rtFlags2_- lFlags)/65536;
      str_sprintf(buf,"rtFlags2_ = %x%x ",hFlags,lFlags);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf,"rtFlags3_ = %x ", rtFlags3_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      lFlags = rtFlags4_%65536;
      hFlags = (rtFlags4_- lFlags)/65536;
      str_sprintf(buf,"rtFlags4_ = %x%x ",hFlags,lFlags);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      lFlags = rtFlags5_%65536;
      hFlags = (rtFlags5_- lFlags)/65536;
      str_sprintf(buf,"rtFlags5_ = %x%x ",hFlags,lFlags);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "queryType_ = %d", (Int32) queryType_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      str_sprintf(buf, "inputVarsSize_ = %d", inputVarsSize());
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      
      if(numUpdateCol_ != 0)
	{
	  str_sprintf(buf, "numUpdateCol = %d",numUpdateCol_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (compoundStmtsInfo_ != 0)
	{
	  str_sprintf(buf,"compoundStmtsInfo_ = %x ",compoundStmtsInfo_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (udrCount_ > 0 || maxResultSets_ > 0)
      {
        str_sprintf(buf, "UDR count = %d, Max Result Sets = %d",
                    (Int32) udrCount_, (Int32) maxResultSets_);
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      }

      if( uninitializedMvCount_ > 0 )
      {
        str_sprintf(buf, 
                    "Uninitialized MV count = %d",
                    (Int32) uninitializedMvCount_);
        space->allocateAndCopyToAlignedSpace(buf, 
                                             str_len(buf), 
                                             sizeof(short));

        for( Int32 i = 0; i < uninitializedMvCount_; i++ )
        {
            UninitializedMvName currentMv = uninitializedMvList_[i];
            str_sprintf(buf, 
                    "Uninitialized MV (physical=%s,ansi=%s)\n",
                    currentMv.getPhysicalName(), currentMv.getAnsiName());
            space->allocateAndCopyToAlignedSpace(buf, 
                                                 str_len(buf), 
                                                 sizeof(short));
        }
      }
      
      if (hasCallStmtExpressions())
      {
        str_sprintf(buf, "Has CALL Statement Expressions = YES");
        space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
      }
    
      if (getRWRSInfo())
	{
	  str_sprintf(buf, "rwrsMaxSize_ = %d", getRWRSInfo()->rwrsMaxSize());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  str_sprintf(buf, "rwrsInputSizeIndex_ = %d, rwrsMaxInputRowlenIndex_ = %d, ", 
		      getRWRSInfo()->rwrsInputSizeIndex_, getRWRSInfo()->rwrsMaxInputRowlenIndex_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	  str_sprintf(buf, "rwrsBufferAddrIndex_ = %d, rwrsPartnNumIndex_ = %d",
		      getRWRSInfo()->rwrsBufferAddrIndex_, getRWRSInfo()->rwrsPartnNumIndex_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	  str_sprintf(buf, "rwrsMaxInternalRowlen_ = %d",
		      getRWRSInfo()->rwrsMaxInternalRowlen_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	  
	  str_sprintf(buf,"flags_ = %x ", getRWRSInfo()->flags_);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
	}

      if (querySimilarityInfo() && querySimilarityInfo()->siList())
        {
	  str_sprintf(buf,"querySimilarityInfo()->siList()->numEntries() = %d ",
                      querySimilarityInfo()->siList()->entries());
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      Lng32 fragOffset;
      Lng32 fragLen;
      Lng32 topNodeOffset;
      if (getFragDir()->getExplainFragDirEntry
          (fragOffset, fragLen, topNodeOffset) == 0)
        {
          char buf[64];
          str_sprintf(buf, "explain_plan_size = %d", fragLen);
	  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

    }
  
  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

NABoolean ComTdbRoot::hasCallStmtExpressions() const
{
  return (inputExpr_ && inputExpr_->isCall()) ||
    (outputExpr_ && outputExpr_->isCall());
}

NABoolean ComTdbRoot::containsUdrInteractions() const
{
  if (udrCount_ > 0)
    return TRUE;
  if (queryType_ == SQL_SP_RESULT_SET)
    return TRUE;
  return FALSE;
}

const char *ComTdbRoot::getQueryTypeText(Lng32 queryType)
{
  switch (queryType)
  {
  case SQL_OTHER: 
    return "SQL_OTHER";
  case SQL_UNKNOWN:
    return "SQL_UNKNOWN";
  case SQL_SELECT_UNIQUE:
    return "SQL_SELECT_UNIQUE";
  case SQL_SELECT_NON_UNIQUE:
    return "SQL_SELECT_NON_UNIQUE";
  case SQL_INSERT_UNIQUE:
    return "SQL_INSERT_UNIQUE";
  case SQL_INSERT_NON_UNIQUE: 
    return "SQL_INSERT_NON_UNIQUE";
  case SQL_UPDATE_UNIQUE: 
    return "SQL_UPDATE_UNIQUE";
  case SQL_UPDATE_NON_UNIQUE: 
    return "SQL_UPDATE_NON_UNIQUE";
  case SQL_DELETE_UNIQUE: 
    return "SQL_DELETE_UNIQUE";
  case SQL_DELETE_NON_UNIQUE: 
    return "SQL_DELETE_NON_UNIQUE";
  case SQL_CONTROL: 
    return "SQL_CONTROL";
  case SQL_SET_TRANSACTION: 
    return "SQL_SET_TRANSACTION";
  case SQL_SET_CATALOG: 
    return "SQL_SET_CATALOG";
  case SQL_SET_SCHEMA: 
    return "SQL_SET_SCHEMA";
  case SQL_CALL_NO_RESULT_SETS: 
    return "SQL_CALL_NO_RESULT_SETS";
  case SQL_CALL_WITH_RESULT_SETS: 
    return "SQL_CALL_WITH_RESULT_SETS";
  case SQL_SP_RESULT_SET: 
    return "SQL_SP_RESULT_SET";
  case SQL_INSERT_ROWSET_SIDETREE:
    return "SQL_INSERT_ROWSET_SIDETREE";
  case SQL_CAT_UTIL:
    return "SQL_CAT_UTIL";
  case SQL_EXE_UTIL:
    return "SQL_EXE_UTIL";
  case SQL_SELECT_UNLOAD:
    return "SQL_SELECT_UNLOAD";
  default:
    return "TYPE_MISSED_OUT";
  }
}

const char *ComTdbRoot::getSubqueryTypeText(Int16 subqueryType)
{
  switch (subqueryType)
  {
  case SQL_STMT_NA:
    return "SQL_STMT_NA";
  case SQL_STMT_CTAS:
    return "SQL_STMT_CTAS";
  case SQL_STMT_GET_STATISTICS:
    return "SQL_STMT_GET_STATISTICS";
  default:
    return "TYPE_MISSED_OUT";
  }
}

NABoolean ComTdbRoot::aqrEnabledForSqlcode(Lng32 sqlcode)
{
  if ( (rtFlags1_ & AQR_ENABLED)              ||
        (sqlcode == -CLI_INVALID_QUERY_PRIVS) ||
        (sqlcode == -CLI_DDL_REDEFINED) )
    return TRUE;
  else
    return FALSE;
}

Int32 ComTdbRoot::getNumberOfUnpackedSecKeys( char * base )
{
  // Since plan is "packed" when this routine is called, we must 
  // find "real" pointer
  SecurityInvKeyInfo * SikInfoP = 
    (SecurityInvKeyInfo *)(base - (char *)sikPtr_.getPointer()) ;
  return ( SikInfoP->getNumSiks() );
}


const ComSecurityKey * ComTdbRoot::getPtrToUnpackedSecurityInvKeys( 
                                                            char * base )
{
  // Since plan is "packed" when this routine is called, we must 
  // find "real" pointers
  SecurityInvKeyInfo * SikInfoP = 
    (SecurityInvKeyInfo *)(base - (char *)sikPtr_.getPointer()) ;
  return ( (ComSecurityKey *)( base - (char *)(SikInfoP->getSikValues()) ) );
}

// -----------------------------------------------------------------------
// Methods for class SecurityInvKeyInfo
// -----------------------------------------------------------------------

Long SecurityInvKeyInfo::pack(void * space)
{
  if (sikValues_.pack(space)) return -1;
  return NAVersionedObject::pack(space);
}

Lng32 SecurityInvKeyInfo::unpack(void * base, void * reallocator)
{
  if (sikValues_.unpack(base)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

