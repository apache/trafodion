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

#include "Platform.h"

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExHbaseAccess.h"
#include "ex_exe_stmt_globals.h"

#include "ExpLOBinterface.h"

#include "SQLTypeDefs.h"

#include "ExpHbaseInterface.h"
#include "QRLogger.h"

#include  "cli_stdh.h"
#include "exp_function.h"
#include "jni.h"
#include <random>
#include "HdfsClient_JNI.h"

// forward declare
Int64 generateUniqueValueFast ();

Int64 getTransactionIDFromContext()
{
  ExTransaction *ta = GetCliGlobals()->currContext()->getTransaction();
  Int64 currentXnId = (Int64) 0;
  Int64 transid = (Int64) 0;
  TmfPhandle_Struct currentTxHandle;

  short retcode = ta->getCurrentXnId((Int64 *)&currentXnId, (Int64 *)&transid, (short *)&currentTxHandle);

  return transid;
}

ex_tcb * ExHbaseAccessTdb::build(ex_globals * glob)
{
  ExExeStmtGlobals * exe_glob = glob->castToExExeStmtGlobals();
  
  ex_assert(exe_glob,"This operator cannot be in DP2");

  ExHbaseAccessTcb *tcb = NULL;

  if ((1) && //(sqHbaseTable()) &&
      ((accessType_ == UPDATE_) ||
       (accessType_ == MERGE_) ||
       (accessType_ == DELETE_)))
    {
      if (rowsetOper())
	tcb = new(exe_glob->getSpace()) 
	  ExHbaseAccessSQRowsetTcb(
				   *this,
				   exe_glob);
      else
	tcb = new(exe_glob->getSpace()) 
	  ExHbaseAccessUMDTcb(
				*this,
				exe_glob);
    }
  else if ((sqHbaseTable()) &&
	   (accessType_ == SELECT_) &&
	   (rowsetOper()))
    {
      tcb = new(exe_glob->getSpace()) 
	ExHbaseAccessSQRowsetTcb(
				 *this,
				 exe_glob);
    }
  else if (accessType_ == DELETE_)
    {
      /*
	tcb = new(exe_glob->getSpace()) 
	  ExHbaseAccessUMDHbaseTcb(
					*this,
					exe_glob);
      */
    }
  else if (accessType_ == SELECT_)
    {
      if (keyMDAMGen())
	{
	  // must be SQ Seabase table and no listOfScan/Get keys
	  if ((sqHbaseTable()) &&
	      (! listOfGetRows()) &&
	      (! listOfScanRows()))
	    {
	      tcb = new(exe_glob->getSpace()) 
		ExHbaseAccessMdamSelectTcb(
					   *this,
					   exe_glob);
	    }
	}
      else
	tcb = new(exe_glob->getSpace()) 
	  ExHbaseAccessSelectTcb(
				 *this,
				 exe_glob);
    }
  else if (accessType_ == COPROC_)
    {
      tcb = new(exe_glob->getSpace()) 
	ExHbaseAccessSelectTcb(
			       *this,
			       exe_glob);
    }
  else if ((accessType_ == INSERT_) ||
	   (accessType_ == UPSERT_) ||
	   (accessType_ == UPSERT_LOAD_))
    {
      if (sqHbaseTable())
	{
	  if ((vsbbInsert()) &&
	      (NOT hbaseSqlIUD()))
            if (this->getIsTrafodionLoadPrep())
              tcb = new(exe_glob->getSpace())
                ExHbaseAccessBulkLoadPrepSQTcb(
                                               *this,
                                               exe_glob);
            else
              tcb = new(exe_glob->getSpace()) 
                ExHbaseAccessUpsertVsbbSQTcb(
                                             *this,
                                             exe_glob);
	  else
	    tcb = new(exe_glob->getSpace()) 
	      ExHbaseAccessInsertSQTcb(
				       *this,
				       exe_glob);
	}
      else
	{
	  if (rowwiseFormat())
	    tcb = new(exe_glob->getSpace()) 
	      ExHbaseAccessInsertRowwiseTcb(
					    *this,
					    exe_glob);
	  else
	    tcb = new(exe_glob->getSpace()) 
	      ExHbaseAccessInsertTcb(
				     *this,
				     exe_glob);
	}
    }
  else if ((accessType_ == CREATE_) ||
	   (accessType_ == DROP_))
    {
      tcb = new(exe_glob->getSpace()) 
	ExHbaseAccessDDLTcb(
			     *this,
			     exe_glob);
    }
  else if (accessType_ == BULK_LOAD_TASK_)
    {
      tcb = new(exe_glob->getSpace())
        ExHbaseAccessBulkLoadTaskTcb(
                             *this,
                             exe_glob);
    }
  else if ((accessType_ == INIT_MD_) ||
	   (accessType_ == DROP_MD_))
    {
      tcb = new(exe_glob->getSpace()) 
	ExHbaseAccessInitMDTcb(
			     *this,
			     exe_glob);
    }
  else if (accessType_ == GET_TABLES_)
    {
      tcb = new(exe_glob->getSpace()) 
	ExHbaseAccessGetTablesTcb(
			     *this,
			     exe_glob);
    }

  ex_assert(tcb, "Error building ExHbaseAccessTcb.");

  return (tcb);
}

ex_tcb * ExHbaseCoProcAggrTdb::build(ex_globals * glob)
{
  ExExeStmtGlobals * exe_glob = glob->castToExExeStmtGlobals();
  
  ex_assert(exe_glob,"This operator cannot be in DP2");

  ExHbaseCoProcAggrTcb *tcb = NULL;

  tcb = new(exe_glob->getSpace()) 
    ExHbaseCoProcAggrTcb(
			   *this,
			   exe_glob);

  ex_assert(tcb, "Error building ExHbaseAggrTcb.");

  return (tcb);
}

////////////////////////////////////////////////////////////////
// Constructor and initialization.
////////////////////////////////////////////////////////////////

ExHbaseAccessTcb::ExHbaseAccessTcb(
          const ComTdbHbaseAccess &hbaseAccessTdb, 
          ex_globals * glob ) :
  ex_tcb( hbaseAccessTdb, 1, glob)
  , workAtp_(NULL)
  , pool_(NULL)
  , matches_(0)
  , rowIds_(glob ? glob->getDefaultHeap() : NULL)
  , columns_(glob ? glob->getDefaultHeap() : NULL)
  , deletedColumns_(glob ? glob->getDefaultHeap() : NULL)
  , hnl_(glob ? glob->getDefaultHeap() : NULL)
  , hbaseFilterColumns_(glob ? glob->getDefaultHeap() : NULL)
  , hbaseFilterOps_(glob ? glob->getDefaultHeap() : NULL)
  , hbaseFilterValues_(glob ? glob->getDefaultHeap() : NULL)
  , colValVec_(NULL)
  , colValVecSize_(0)
  , colValEntry_(0)
  , loggingErrorDiags_(NULL)
  , logFileHdfsClient_(NULL)
  , loggingFileCreated_(FALSE)
  , loggingFileName_(NULL)
{
  Space * space = (glob ? glob->getSpace() : NULL);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : NULL);

  pool_ = new(space) 
        sql_buffer_pool(hbaseAccessTdb.numBuffers_,
        hbaseAccessTdb.bufferSize_,
        space,
        SqlBufferBase::NORMAL_);

  pool_->setStaticMode(TRUE);
        
  // Allocate the queue to communicate with parent
  allocateParentQueues(qparent_);

  if (hbaseAccessTdb.workCriDesc_)
    {
      workAtp_ = allocateAtp(hbaseAccessTdb.workCriDesc_, space);
      if (hbaseAccessTdb.asciiTuppIndex_ > 0)
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.asciiTuppIndex_), 0);
      if (hbaseAccessTdb.convertTuppIndex_ > 0)
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.convertTuppIndex_), 0);
      if (hbaseAccessTdb.updateTuppIndex_ > 0)
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.updateTuppIndex_), 0);
      if (hbaseAccessTdb.mergeInsertTuppIndex_ > 0)
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.mergeInsertTuppIndex_), 0);
      if (hbaseAccessTdb.mergeInsertRowIdTuppIndex_ > 0)
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.mergeInsertRowIdTuppIndex_), 0);
      if (hbaseAccessTdb.mergeIUDIndicatorTuppIndex_ > 0)
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.mergeIUDIndicatorTuppIndex_), 0);

      if (hbaseAccessTdb.rowIdTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.rowIdTuppIndex_), 0);
      if (hbaseAccessTdb.rowIdAsciiTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.rowIdAsciiTuppIndex_), 0);
      if (hbaseAccessTdb.keyTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.keyTuppIndex_), 0);

      if (hbaseAccessTdb.keyColValTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.keyColValTuppIndex_), 0);

      if (hbaseAccessTdb.hbaseFilterValTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.hbaseFilterValTuppIndex_), 0);

      if (hbaseAccessTdb.hbaseTimestampTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.hbaseTimestampTuppIndex_), 0);

      if (hbaseAccessTdb.hbaseVersionTuppIndex_ > 0)      
	pool_->get_free_tuple(workAtp_->getTupp(hbaseAccessTdb.hbaseVersionTuppIndex_), 0);

    }

  keySubsetExeExpr_ = NULL;
  keyMdamExeExpr_ = NULL;
  if (hbaseAccessTdb.keySubsetGen())
    {
      // this constructor will also do the fixups of underlying expressions
      keySubsetExeExpr_ = new(glob->getSpace()) 
	keySingleSubsetEx(*hbaseAccessTdb.keySubsetGen(),
			  1, // version 1
			  pool_,
			  getGlobals(),
			  getExpressionMode(), this);
    }
  else if (hbaseAccessTdb.keyMDAMGen())
    {
      // this constructor will also do the fixups of underlying expressions
      keyMdamExeExpr_ = new(glob->getSpace()) 
	keyMdamEx(*hbaseAccessTdb.keyMDAMGen(),
			  1, // version 1
			  pool_,
			  getGlobals(),
			  this);
    }

  // fixup expressions
  if (convertExpr())
    convertExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (updateExpr())
    updateExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (mergeInsertExpr())
    mergeInsertExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (mergeInsertRowIdExpr())
    mergeInsertRowIdExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (mergeUpdScanExpr())
    mergeUpdScanExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (returnFetchExpr())
    returnFetchExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (returnUpdateExpr())
    returnUpdateExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (returnMergeInsertExpr())
    returnMergeInsertExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (scanExpr())
    scanExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (rowIdExpr())
    rowIdExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (encodedKeyExpr())
    encodedKeyExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (keyColValExpr())
    keyColValExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (insDelPreCondExpr())
    insDelPreCondExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (insConstraintExpr())
    insConstraintExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (updConstraintExpr())
    updConstraintExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (hbaseFilterValExpr())
    hbaseFilterValExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  
  // Register subtasks with the scheduler
  registerSubtasks();
  registerResizeSubtasks();

  ehi_ = ExpHbaseInterface::newInstance(glob->getDefaultHeap(),
					(char*)hbaseAccessTdb.server_, 
					(char*)hbaseAccessTdb.zkPort_);

  asciiRow_ = NULL;
  asciiRowMissingCols_ = NULL;
  latestTimestampForCols_ = NULL;
  latestVersionNumForCols_ = NULL;
  convertRow_ = NULL;
  updateRow_ = NULL;
  mergeInsertRow_ = NULL;
  rowIdRow_ = NULL;
  rowwiseRow_ = NULL;
  rowwiseRowLen_ = 0;
  beginRowIdRow_ = NULL;
  endRowIdRow_ = NULL;
  beginKeyRow_ = NULL;
  endKeyRow_ = NULL;
  encodedKeyRow_ = NULL;
  keyColValRow_ = NULL;
  hbaseFilterValRow_ = NULL;

  if (hbaseAccessTdb.asciiRowLen_ > 0)
    {
      asciiRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.asciiRowLen_];
      asciiRowMissingCols_ = 
	new(glob->getDefaultHeap()) 
	char[hbaseAccessTdb.workCriDesc_->getTupleDescriptor(hbaseAccessTdb.asciiTuppIndex_)->numAttrs()];
      latestTimestampForCols_ = new(glob->getDefaultHeap()) 
	long[hbaseAccessTdb.workCriDesc_->getTupleDescriptor(hbaseAccessTdb.asciiTuppIndex_)->numAttrs()] ;
      latestVersionNumForCols_ = new(glob->getDefaultHeap()) 
	long[hbaseAccessTdb.workCriDesc_->getTupleDescriptor(hbaseAccessTdb.asciiTuppIndex_)->numAttrs()] ;
    }
  
  convertRowLen_ = hbaseAccessTdb.convertRowLen();
  if (hbaseAccessTdb.convertRowLen() > 0)
    convertRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.convertRowLen()];

  if (hbaseAccessTdb.updateRowLen_ > 0)
    updateRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.updateRowLen_];

  if (hbaseAccessTdb.mergeInsertRowLen_ > 0)
    mergeInsertRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.mergeInsertRowLen_];

  if (hbaseAccessTdb.rowIdLen_ > 0)
    {
      // +2 for adding a char to exclude the begin/end row, and for null terminator
      rowIdRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.rowIdLen_ + 2];

      beginRowIdRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.rowIdLen_ + 2];

      endRowIdRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.rowIdLen_ + 2];
    }
      
  if (hbaseAccessTdb.convertRowLen() > 0)
    rowwiseRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.convertRowLen()];
  if (hbaseAccessTdb.rowIdAsciiRowLen_ > 0)
    rowIdAsciiRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.rowIdAsciiRowLen_];

  if (hbaseAccessTdb.keyLen_ > 0)
    {
      beginKeyRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.keyLen_];
      endKeyRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.keyLen_];
      encodedKeyRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.keyLen_];
    }

  if (hbaseAccessTdb.keyColValLen_ > 0)
    {
      keyColValRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.keyColValLen_];
    }

  if (hbaseAccessTdb.hbaseFilterValRowLen_ > 0)
    {
      hbaseFilterValRow_ = new(glob->getDefaultHeap()) char[hbaseAccessTdb.hbaseFilterValRowLen_];
    }

  prevRowIdMaxLen_ = 0;
  prevRowId_.val = NULL;
  prevRowId_.len = 0;
  isEOD_ = FALSE;
  rowIDAllocatedLen_ = 0;
  rowIDAllocatedVal_ = NULL;
  rowID_.val = NULL;
  rowID_.len = 0;

  directBufferRowNum_ = 0;
  directRowIDBuffer_ = NULL;
  directRowIDBufferLen_ = 0;
  dbRowID_.val = NULL;
  dbRowID_.len = 0;
  directRowBuffer_ = NULL;
  directRowBufferLen_ = 0;
  directBufferMaxRows_ = 0;
  row_.val = NULL;
  row_.len = 0;
  rows_.val = NULL;
  rows_.len = 0;
  colVal_.val = 0;
  colVal_.len = 0;
  asyncCompleteRetryCount_ = 0;
  asyncOperation_ = FALSE;
  asyncOperationTimeout_ = 2;
  resultArray_ = NULL;
}
    
ExHbaseAccessTcb::~ExHbaseAccessTcb()
{
  freeResources();
}

void ExHbaseAccessTcb::freeResources()
{
  if (workAtp_)
  {
    workAtp_->release();
    deallocateAtp(workAtp_, getSpace());
    workAtp_ = NULL;
  }
  if (pool_)
  {
    delete pool_;
    pool_ = NULL;
  }
  if (qparent_.up)
  {
    delete qparent_.up;
    qparent_.up = NULL;
  }
  if (qparent_.down)
  {
    delete qparent_.down;
    qparent_.down = NULL;
  }
  delete ehi_;
  if (rowIDAllocatedVal_)
     NADELETEBASIC(rowIDAllocatedVal_, getHeap());
  if (directRowIDBuffer_)
     NADELETEBASIC(directRowIDBuffer_, getHeap());
  if (directRowBuffer_)
     NADELETEBASIC(directRowBuffer_, getHeap());
  if (colVal_.val != NULL)
     NADELETEBASIC(colVal_.val, getHeap());
  if (logFileHdfsClient_ != NULL) 
     NADELETE(logFileHdfsClient_, HdfsClient, getHeap());
  if (loggingFileName_ != NULL)
     NADELETEBASIC(loggingFileName_, getHeap());
}


NABoolean ExHbaseAccessTcb::needStatsEntry()
{
  return TRUE;

  // stats are collected for ALL and OPERATOR options.
  if ((getGlobals()->getStatsArea()->getCollectStatsType() == 
       ComTdb::ALL_STATS) ||
      (getGlobals()->getStatsArea()->getCollectStatsType() == 
      ComTdb::OPERATOR_STATS))
    return TRUE;
  else if ( getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::PERTABLE_STATS)
    return TRUE;
  else
    return FALSE;
}

ExOperStats * ExHbaseAccessTcb::doAllocateStatsEntry(
                                                        CollHeap *heap,
                                                        ComTdb *tdb)
{
  ExEspStmtGlobals *espGlobals = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();
  StmtStats *ss; 
  if (espGlobals != NULL)
     ss = espGlobals->getStmtStats();
  else
     ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats(); 
  ExHbaseAccessStats *hbaseAccessStats =  new(heap) ExHbaseAccessStats(heap,
				   this,
				   tdb);
  if (ss != NULL)
     hbaseAccessStats->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  return hbaseAccessStats;
}

void ExHbaseAccessTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  sched->registerInsertSubtask(sWork,   this, qparent_.down,"PD");
  sched->registerUnblockSubtask(sWork,    this, qparent_.up,  "PU");
  sched->registerCancelSubtask(sWork,     this, qparent_.down,"CN");

}

ex_tcb_private_state *ExHbaseAccessTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ex_tcb_private_state> pa;
  return pa.allocatePstates(this, numElems, pstateLength);
}

ExWorkProcRetcode ExHbaseAccessTcb::work()
{
  ex_assert(0, "Should not reach ExHbaseAccessTcb::work()");

  return WORK_OK;
}

short ExHbaseAccessTcb::allocateUpEntryTupps
(Lng32 tupp1index, Lng32 tupp1length, Lng32 tupp2index, Lng32 tupp2length,
 NABoolean isVarchar,
 short * rc)
{
  tupp p1;
  tupp p2;
  if (tupp1index > 0)
    {
      if (pool_->get_free_tuple(p1, (Lng32)
				((isVarchar ? SQL_VARCHAR_HDR_SIZE : 0)
				 + tupp1length)))
	{
	  if (rc)
	    *rc = WORK_POOL_BLOCKED;
	  return -1;
	}
    }

  if (tupp2index > 0)
    {
      if (pool_->get_free_tuple(p2, (Lng32)
				((isVarchar ? SQL_VARCHAR_HDR_SIZE : 0)
				 + tupp2length)))
	{
	  if (rc)
	    *rc = WORK_POOL_BLOCKED;
	  return -1;
	}
    }

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();

  up_entry->copyAtp(pentry_down);

  if (tupp1index > 0)
    {
      up_entry->getAtp()->getTupp(tupp1index) = p1;
    }

  if (tupp2index > 0)
    {
      up_entry->getAtp()->getTupp(tupp2index) = p2;
    }

  return 0;
}

short ExHbaseAccessTcb::moveRowToUpQueue(short * rc)
{
  if (qparent_.up->isFull())
    {
      if (rc)
	*rc = WORK_OK;
      return -1;
    }

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  
  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(++matches_);
  up_entry->upState.status = ex_queue::Q_OK_MMORE;

  if (getHbaseAccessStats())
    getHbaseAccessStats()->incActualRowsReturned();

  // insert into parent
  qparent_.up->insert();

  return 0;
}

short ExHbaseAccessTcb::moveRowToUpQueue(const char * row, Lng32 len, 
					 short * rc, NABoolean isVarchar)
{
  if (qparent_.up->isFull())
    {
      if (rc)
	*rc = WORK_OK;
      return -1;
    }

  Lng32 length;
  if (len <= 0)
    length = strlen(row);
  else
    length = len;

  tupp p;
  if (pool_->get_free_tuple(p, (Lng32)
			    ((isVarchar ? SQL_VARCHAR_HDR_SIZE : 0)
			     + length)))
    {
      if (rc)
	*rc = WORK_POOL_BLOCKED;
      return -1;
    }
  
  char * dp = p.getDataPointer();
  if (isVarchar)
    {
      *(short*)dp = (short)length;
      str_cpy_all(&dp[SQL_VARCHAR_HDR_SIZE], row, length);
    }
  else
    {
      str_cpy_all(dp, row, length);
    }

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  
  up_entry->copyAtp(pentry_down);
  up_entry->getAtp()->getTupp((Lng32)hbaseAccessTdb().returnedTuppIndex_) = p;

  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(++matches_);
  up_entry->upState.status = ex_queue::Q_OK_MMORE;

  if (getHbaseAccessStats())
    getHbaseAccessStats()->incActualRowsReturned();

  // insert into parent
  qparent_.up->insert();

  return 0;
}

short ExHbaseAccessTcb::setupError(NAHeap *heap, ex_queue_pair &qparent, Lng32 retcode, const char * str, const char * str2)
{
  ContextCli *currContext = GetCliGlobals()->currContext();

  // Make sure retcode is positive.
  if (retcode < 0)
    retcode = -retcode;
    
  if ((ABS(retcode) >= HBASE_MIN_ERROR_NUM) &&
      (ABS(retcode) <= HBASE_MAX_ERROR_NUM))
    {
      ex_queue_entry *pentry_down = qparent.down->getHeadEntry();
 
      Lng32 cliError = 0;
      
      Lng32 intParam1 = -retcode;
      ComDiagsArea * diagsArea = NULL;
      ExRaiseSqlError(heap, &diagsArea, 
		      (ExeErrorCode)(8448), NULL, &intParam1, 
		      &cliError, NULL, 
		      (str ? (char*)str : (char*)" "),
		      getHbaseErrStr(retcode),
                      (str2 ? (char*)str2 : 
                      (char *)GetCliGlobals()->getJniErrorStr())); 
      pentry_down->setDiagsArea(diagsArea);
      return -1;
    }

  return 0;
}

short ExHbaseAccessTcb::setupError(Lng32 retcode, const char * str, const char * str2)
{
  ContextCli *currContext = GetCliGlobals()->currContext();
  // Make sure retcode is positive.
  if (retcode < 0)
    retcode = -retcode;
    
  if ((ABS(retcode) >= HBASE_MIN_ERROR_NUM) &&
      (ABS(retcode) <= HBASE_MAX_ERROR_NUM))
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
 
      Lng32 cliError = 0;
      
      Lng32 intParam1 = -retcode;
      ComDiagsArea * diagsArea = NULL;
      ExRaiseSqlError(getHeap(), &diagsArea, 
		      (ExeErrorCode)(8448), NULL, &intParam1, 
		      &cliError, NULL, 
		      (str ? (char*)str : (char*)" "),
		      getHbaseErrStr(retcode),
                      (str2 ? (char*)str2 : 
                      (char *)GetCliGlobals()->getJniErrorStr())); 
      pentry_down->setDiagsArea(diagsArea);
      return -1;
    }

  return 0;
}

short ExHbaseAccessTcb::raiseError(Lng32 errcode, 
                                   Lng32 * intParam1,
                                   const char * str1, 
                                   const char * str2)
{
  // Make sure retcode is positive.
  if (errcode < 0)
    errcode = -errcode;
    
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  
  ComDiagsArea * diagsArea = NULL;
  ExRaiseSqlError(getHeap(), &diagsArea, 
                  (ExeErrorCode)(errcode), NULL,
                  intParam1, NULL, NULL,
                  str1, str2, NULL);
  pentry_down->setDiagsArea(diagsArea);

  return 0;
}

short ExHbaseAccessTcb::handleError(short &rc)
{
  if (qparent_.up->isFull())
    {
      rc = WORK_OK;
      return -1;
    }

  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry *up_entry = qparent_.up->getTailEntry();
  up_entry->copyAtp(pentry_down);
  up_entry->upState.parentIndex =
    pentry_down->downState.parentIndex;
  up_entry->upState.downIndex = qparent_.down->getHeadIndex();
  up_entry->upState.status = ex_queue::Q_SQLERROR;
  qparent_.up->insert();

  return 0;
}

short ExHbaseAccessTcb::handleDone(ExWorkProcRetcode &rc, Int64 rowsAffected)
{
  if (qparent_.up->isFull())
    {
      rc = WORK_OK;
      return -1;
    }

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry *up_entry = qparent_.up->getTailEntry();
  up_entry->copyAtp(pentry_down);
  up_entry->upState.parentIndex =
    pentry_down->downState.parentIndex;
  up_entry->upState.downIndex = qparent_.down->getHeadIndex();
  up_entry->upState.status = ex_queue::Q_NO_DATA;
  up_entry->upState.setMatchNo(matches_);
  if (rowsAffected > 0 && hbaseAccessTdb().computeRowsAffected())
    {
      ExMasterStmtGlobals *g = getGlobals()->
        castToExExeStmtGlobals()->castToExMasterStmtGlobals();
      if (g)
        {
          g->setRowsAffected(g->getRowsAffected() + rowsAffected);
        }
      else
        {
          ComDiagsArea * diagsArea = up_entry->getDiagsArea();
          if (!diagsArea)
            {
              diagsArea = 
                ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
              up_entry->setDiagsArea(diagsArea);
            }
          diagsArea->addRowCount(rowsAffected);
        }
    }
  qparent_.up->insert();
  
  qparent_.down->removeHead();

  return 0;
}

// return:
// 0, if all ok.
// -1, if error.
short ExHbaseAccessTcb::createColumnwiseRow()
{
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().asciiTuppIndex_);
  
  for (Lng32 i = 0; i <  asciiSourceTD->numAttrs(); i++)
    {
      Attributes * attr = asciiSourceTD->getAttr(i);
      if (attr)
	{
	  switch (i)
	    {
	    case HBASE_ROW_ID_INDEX:
	      {
		*(short*)&asciiRow_[attr->getVCLenIndOffset()] = rowId_.len;
                str_cpy_all(&asciiRow_[attr->getOffset()],
                          rowId_.val, rowId_.len);
	      }
	      break;
	      
	    case HBASE_COL_FAMILY_INDEX:
	      {
		*(short*)&asciiRow_[attr->getVCLenIndOffset()] = colFamName_.len;
                str_cpy_all(&asciiRow_[attr->getOffset()],
                          colFamName_.val, colFamName_.len);
	      }
	      break;
	      
	    case HBASE_COL_NAME_INDEX:
	      {
		*(short*)&asciiRow_[attr->getVCLenIndOffset()] = colName_.len;
                str_cpy_all(&asciiRow_[attr->getOffset()],
                          colName_.val, colName_.len);
	      }
	      break;
	      
	    case HBASE_COL_VALUE_INDEX:
	      {
                if (attr->getVCIndicatorLength() == sizeof(short))
                  *(short*)&asciiRow_[attr->getVCLenIndOffset()] = colVal_.len;
                else
                  *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = colVal_.len;

                if (colVal_.len > attr->getLength())
                  {
                    // not enough space. Return error.
                    return -HBASE_COPY_ERROR;
                   }

                str_cpy_all(&asciiRow_[attr->getOffset()],
                                colVal_.val, colVal_.len);
	      }
	      break;
	      
	    case HBASE_COL_TS_INDEX:
	      {
		*(Int64*)&asciiRow_[attr->getOffset()] = colTS_;
	      }
	      break;
	      
	    } // switch
	} // if attr
    } // for
  
  workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
    .setDataPointer(convertRow_);
  workAtp_->getTupp(hbaseAccessTdb().asciiTuppIndex_) 
    .setDataPointer(asciiRow_);
  
  if (convertExpr())
    {
      ex_expr::exp_return_type evalRetCode =
	convertExpr()->eval(pentry_down->getAtp(), workAtp_);
      if (evalRetCode == ex_expr::EXPR_ERROR)
	{
	  return -1;
	}
    }

  return 0;
}

short ExHbaseAccessTcb::copyCell()
{
    // values are stored in the following format:
    //  colNameLen(2 bytes)
    //  colName for colNameLen bytes
    //  colValueLen(4 bytes)
    //  colValue for colValueLen bytes.
    //
    //  Attribute values are not necessarily null-terminated. 
    // Cannot use string functions.
    //
    //
    short colNameLen = colName_.len + colFamName_.len + 1;
    Lng32 colValueLen = colVal_.len;

    Lng32 neededLen = 
      sizeof(colNameLen) + colNameLen + sizeof(colValueLen) + colValueLen;

    if (rowwiseRowLen_ + neededLen > hbaseAccessTdb().convertRowLen())
      {
        // not enough space. Return error.
        return -HBASE_COPY_ERROR;
      }

    char* pos = rowwiseRow_ + rowwiseRowLen_;
    memcpy(pos, (char*)&colNameLen, sizeof(short));
    pos += sizeof(short);

    memcpy(pos, colFamName_.val, colFamName_.len);
    pos += colFamName_.len;
    *pos++ = ':';
    memcpy(pos, colName_.val, colName_.len);
    pos += colName_.len;

    memcpy(pos, (char*)&colValueLen, sizeof(Lng32));
    pos += sizeof(Lng32);

    memcpy(pos, colVal_.val, colValueLen);
    pos += colValueLen;

    rowwiseRowLen_ += sizeof(short) + colNameLen + sizeof(Lng32) + colValueLen;
    return 0;
}

// return:
// 0, if all ok.
// -1, if error.
short ExHbaseAccessTcb::createRowwiseRow()
{
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().asciiTuppIndex_);
  
  for (Lng32 i = 0; i <  asciiSourceTD->numAttrs(); i++)
    {
      Attributes * attr = asciiSourceTD->getAttr(i);
      if (attr)
	{
	  switch (i)
	    {
	    case HBASE_ROW_ROWID_INDEX:
	      {
		*(short*)&asciiRow_[attr->getVCLenIndOffset()] = rowId_.len;
	        str_cpy_all(&asciiRow_[attr->getOffset()], rowId_.val, rowId_.len);
	      }
	      break;
	      
	    case HBASE_COL_DETAILS_INDEX:
	      {
                if (attr->getVCIndicatorLength() == sizeof(short))
                  *(short*)&asciiRow_[attr->getVCLenIndOffset()] = rowwiseRowLen_;
                else
                  *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = rowwiseRowLen_;   
		str_cpy_all(&asciiRow_[attr->getOffset()], rowwiseRow_, rowwiseRowLen_);
	      }
	      break;
	      
	    } // switch
	} // if
    } // for
  
  workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
    .setDataPointer(convertRow_);
  workAtp_->getTupp(hbaseAccessTdb().asciiTuppIndex_) 
    .setDataPointer(asciiRow_);
  
  if (convertExpr())
    {
      ex_expr::exp_return_type evalRetCode =
	convertExpr()->eval(pentry_down->getAtp(), workAtp_);
      if (evalRetCode == ex_expr::EXPR_ERROR)
	{
	  return -1;
	}
    }

  return 0;
}

// return 1, if found. 0, if not found. -1, if error.
// Columns retrieved from hbase are in sorted order.
// Columns in listOfColNames may or may not be in sorted order.
// This method advances LOCN until a match is found or a full
// loop of LOCN is done.
// If cols in LOCN are in sorted order, then the search will be faster.
// 
short ExHbaseAccessTcb::getColPos(char * colName, Lng32 colNameLen, Lng32 &idx)
{
  NABoolean found = FALSE;

  Lng32 numCompares = hbaseAccessTdb().listOfFetchedColNames()->numEntries();
  while ((numCompares > 0) &&
            (NOT found))
    {
      if (hbaseAccessTdb().listOfFetchedColNames()->atEnd())
      {
	hbaseAccessTdb().listOfFetchedColNames()->position();
        idx = -1;
      }

      // currNamePtr points to:
      // 2 bytes len, len bytes colname.
      char * currNamePtr = (char*)hbaseAccessTdb().listOfFetchedColNames()->getCurr();
      short len = *(short*)currNamePtr;
      currNamePtr += sizeof(short);
      char * currName = currNamePtr;

      if ((colNameLen == len) &&
	  (memcmp(colName, currName, len) == 0))
	{
	  found = TRUE;
	}
      else
	numCompares--;

      hbaseAccessTdb().listOfFetchedColNames()->advance();
      idx++;
    }
  
  if (found)
    return 1;
  else
    return 0;
}

Lng32 ExHbaseAccessTcb::createSQRowDirect(Int64 *latestRowTimestamp)
{
  short retcode = 0;
     
  if (hbaseAccessTdb().alignedFormat())
    retcode = createSQRowFromAlignedFormat(latestRowTimestamp);
  else
    {
      if (hbaseAccessTdb().multiVersions()) {
        if (latestRowTimestamp != NULL)
           *latestRowTimestamp = -1;
        retcode = createSQRowFromHbaseFormatMulti();
      }
      else
        retcode = createSQRowFromHbaseFormat(latestRowTimestamp);
    }

   return retcode;
}

Lng32 ExHbaseAccessTcb::createSQRowFromHbaseFormat(Int64 *latestRowTimestamp)
{
  // no columns are being fetched from hbase, do not create a row.
  if (hbaseAccessTdb().listOfFetchedColNames()->numEntries() == 0)
    return 0;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().asciiTuppIndex_);

  ExpTupleDesc * convertTuppTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().convertTuppIndex_);
  
  Attributes * attr = NULL;

  // initialize as missing cols.
  // TBD: can optimize to skip this step if there are no nullable and no added cols
  memset(asciiRowMissingCols_, 1, asciiSourceTD->numAttrs());

  // initialize latest timestamp to 0 for every column
  memset(latestTimestampForCols_, 0, (asciiSourceTD->numAttrs()*sizeof(long)));
  if (latestRowTimestamp != NULL)
     *latestRowTimestamp = -1;
  // initialize latest version to 0 for every column
  memset(latestVersionNumForCols_, 0, (asciiSourceTD->numAttrs()*sizeof(long)));
  
  hbaseAccessTdb().listOfFetchedColNames()->position();
  Lng32 idx = -1;

  NABoolean hbmTab = hbaseAccessTdb().hbaseMapTable();

  BYTE *colVal; 
  Lng32 colValLen;
  long timestamp;
  char *colName;
  short colNameLen;
  BYTE nullVal;
  Lng32 retcode;
  int numCells; 
  retcode = ehi_->getNumCellsPerRow(numCells);
  if (retcode == HBASE_ACCESS_NO_ROW)
     return retcode ;
  if (retcode != HBASE_ACCESS_SUCCESS)
  {
     setupError(retcode, "", "getNumCellsPerRow()");
     return retcode;
  }

  // For an hbase mapped table, the primary key column may not exist as
  // a separate hbase cell/column. In that case, the rowID of the retrieved row
  // is the primary key column value.
  // Also, if a primary key col value exists as a separate cell/column, then
  // the contents of rowID and pkey col value must be the same.
  // Retrieve rowID for hbase mapped table. It will be used later to validate
  // the abovementioned 2 cases.
  char * pkeyColInfo = hbaseAccessTdb().getPkeyColName();
  Lng32 pkeyColInfoLen = 0;
  HbaseStr rowID;
  NABoolean hbmPkeyColCheck = FALSE;
  if ((hbaseAccessTdb().hbaseMapTable()) &&
      (pkeyColInfo))
    {
      hbmPkeyColCheck = TRUE;

      pkeyColInfo = hbaseAccessTdb().getPkeyColName();
      pkeyColInfoLen = *(short*)pkeyColInfo;
      pkeyColInfo += sizeof(short);

      retcode = ehi_->getRowID(rowID);
      if (retcode != HBASE_ACCESS_SUCCESS)
        {
          setupError(retcode, "", "getRowID()");
          return retcode;
        }
    }      
 
  for (int colNo= 0; colNo < numCells; colNo++)
  {
      retcode = ehi_->getColName(colNo, &colName, colNameLen, timestamp);
      if (retcode != HBASE_ACCESS_SUCCESS)
      {
         setupError(retcode, "", "getColName()");
         return retcode;
      }

      if (! getColPos(colName, colNameLen, idx)) // not found
      { 
	  ex_assert(FALSE, "Error in getColPos()");
      }

      // not missing any more
      asciiRowMissingCols_[idx] = 0;
      if (timestamp > latestTimestampForCols_[idx])
        latestTimestampForCols_[idx] = timestamp;

      if (latestRowTimestamp != NULL && timestamp  > *latestRowTimestamp)
         *latestRowTimestamp = timestamp;
      latestVersionNumForCols_[idx] = 1;

      Attributes * attr = asciiSourceTD->getAttr(idx);
      if (! attr)
         ex_assert(FALSE, "Attr not found -1");

      // copy to asciiRow only if this is the latest version seen so far
      // for this column. On 6/10/2014 we get two versions for a newly
      // updated column that has not been committed yet.
      if (timestamp == latestTimestampForCols_[idx]) 
      {
         colVal = (BYTE *)&asciiRow_[attr->getOffset()];
         colValLen = attr->getLength();
         BYTE nullVal;

         if (hbmTab)
           {
             Lng32 ij = 0;
           }

         retcode = ehi_->getColVal(colNo, colVal, colValLen, 
                                   (NOT hbaseAccessTdb().hbaseMapTable() 
                                    ? attr->getNullFlag() : FALSE), nullVal);
         if (retcode != HBASE_ACCESS_SUCCESS)
         {
            setupError(retcode, "", "getColVal()");
            return retcode;
         }

         if (colValLen > attr->getLength())
         {
           // col val wont fit. Return error.
           char buf[1000];
           str_sprintf(buf, " Details: actual column value length of %d is greater than the expected max buffer size of %d.",
                       colValLen, attr->getLength());
           raiseError(EXE_HBASE_ACCESS_ERROR, NULL, 
                      hbaseAccessTdb().getTableName(), buf);
           return -1;
         }

         if ((hbmPkeyColCheck) &&
             (pkeyColInfoLen == colNameLen) &&
             (str_cmp(pkeyColInfo, colName, colNameLen) == 0))
         {
           // validate that this col value is same as the pkey value for this
           // hbase mapped table.
           if (NOT ((rowID.len == colValLen) &&
                    (str_cmp(rowID.val, (char*)colVal, colValLen) == 0)))
             {
               char buf[1000];
               str_sprintf(buf, " Details: HBase rowID content must match the primary key column content.");
               raiseError(EXE_HBASE_ACCESS_ERROR, NULL, 
                          hbaseAccessTdb().getTableName(), buf);
               return -1;
             }
         }
         
         if (attr->getNullFlag())
         {
            if (nullVal)
               *(short*)&asciiRow_[attr->getNullIndOffset()] = -1;
            else
               *(short*)&asciiRow_[attr->getNullIndOffset()] = 0;
         }
         if (attr->getVCIndicatorLength() > 0)
         {
             if (attr->getVCIndicatorLength() == sizeof(short))
                *(short*)&asciiRow_[attr->getVCLenIndOffset()] = colValLen; 
             else
                *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = colValLen; 
         }
      }
  } // for

  // fill in null or default values for missing cols.
  for (idx = 0; idx < asciiSourceTD->numAttrs(); idx++)
    {
      if (asciiRowMissingCols_[idx] == 1) // missing
	{
	  attr = asciiSourceTD->getAttr(idx);
	  if (! attr)
             ex_assert(FALSE, "Attr not found -2");

	  char * defVal = attr->getDefaultValue();
          char * colInfo = NULL;
          Lng32 colInfoLen = 0;
          if (hbmPkeyColCheck || (! defVal))
            {
              colInfo = 
                (char*)hbaseAccessTdb().listOfFetchedColNames()->get(idx);
              colInfoLen = *(short*)colInfo;
              colInfo += sizeof(short);
            }

          // if this missing col is the pkey col, fill in values
          // from the retrieved rowID.
          if (hbmPkeyColCheck &&
              (pkeyColInfoLen == colInfoLen) &&
              (str_cmp(pkeyColInfo, colInfo, colInfoLen) == 0))
            {
              char * colValLoc = (char *)&asciiRow_[attr->getOffset()];
              Lng32  colValMaxLen = attr->getLength();
              if (rowID.len > colValMaxLen)
                {
                  // rowID val wont fit. Return error.
                  char buf[1000];
                  str_sprintf(buf, " Details: retrieved rowID of length %d is larger than the specified key size of %d.",
                              rowID.len, colValMaxLen);
                  raiseError(EXE_HBASE_ACCESS_ERROR, NULL, 
                             hbaseAccessTdb().getTableName(), buf);
                  return -1;
                }

              str_cpy_all(colValLoc, rowID.val, rowID.len);

              *(short*)&asciiRow_[attr->getNullIndOffset()] = 0;
              if (attr->getVCIndicatorLength() > 0)
                {
                  if (attr->getVCIndicatorLength() == sizeof(short))
                    *(short*)&asciiRow_[attr->getVCLenIndOffset()] = rowID.len; 
                  else
                    *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = rowID.len; 
                }

              // dont check for other columns once pkey col has been found.
              hbmPkeyColCheck = FALSE;
            } // missing pkey val
          else 
            {
              if (! defVal)
                {
                  Text colFam, colName;
                  extractColFamilyAndName(colInfo, colFam, colName);

                  char buf[20];
                  if (NOT hbaseAccessTdb().hbaseMapTable())
                    {
                      Int64 v = 0;
                      if (colName.length() == sizeof(char))
                        v = *(char*)colName.data();
                      else if (colName.length() == sizeof(UInt16))
                        v = *(UInt16*)colName.data();
                      else if (colName.length() == sizeof(ULng32))
                        v = *(ULng32*)colName.data();
                      
                      str_sprintf(buf, "%ld", v);
                    }
                  
                  ComDiagsArea * diagsArea = NULL;
                  ExRaiseSqlError(
                       getHeap(), &diagsArea,
                       (ExeErrorCode) EXE_DEFAULT_VALUE_INCONSISTENT_ERROR,
                       NULL, NULL, NULL, NULL, 
                       (hbaseAccessTdb().hbaseMapTable() ? colName.data() : buf),
                       hbaseAccessTdb().getTableName());
                  pentry_down->setDiagsArea(diagsArea);
                  return -1;
                }

              char * defValPtr = defVal;
              short nullVal = 0;
              if (attr->getNullFlag())
                {
                  nullVal = *(short*)defVal;
                  *(short*)&asciiRow_[attr->getNullIndOffset()] = nullVal;
                  
                  defValPtr += 2;
                }
              
              Lng32 copyLen;
              if (! nullVal)
                {
                  if (attr->getVCIndicatorLength() > 0)
                    {
                      copyLen = *(short*)defValPtr;
                      if (attr->getVCIndicatorLength() == sizeof(short))
                        *(short*)&asciiRow_[attr->getVCLenIndOffset()] = copyLen; 
                      else
                        *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = copyLen;
                      defValPtr += attr->getVCIndicatorLength();
                    }
                  else
                    {
                      copyLen = attr->getLength();
                    }
                  char *destPtr = &asciiRow_[attr->getOffset()];
                  str_cpy_all(destPtr, defValPtr, copyLen);
                } // not nullVal
            }
        } // missing col
    } // for
  
  workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
    .setDataPointer(convertRow_);
  workAtp_->getTupp(hbaseAccessTdb().asciiTuppIndex_) 
    .setDataPointer(asciiRow_);
  if (hbaseAccessTdb().hbaseTimestampTuppIndex_ > 0)
    {
      workAtp_->getTupp(hbaseAccessTdb().hbaseTimestampTuppIndex_) 
        .setDataPointer((char*)latestTimestampForCols_);
    }
  if (hbaseAccessTdb().hbaseVersionTuppIndex_ > 0)
    {
      workAtp_->getTupp(hbaseAccessTdb().hbaseVersionTuppIndex_) 
        .setDataPointer((char*)latestVersionNumForCols_);
    }

  if (convertExpr())
    {
      convertRowLen_ = hbaseAccessTdb().convertRowLen();
      UInt32 rowLen = convertRowLen_;
      ex_expr::exp_return_type evalRetCode =
	convertExpr()->eval(pentry_down->getAtp(), workAtp_, 0, -1, &rowLen);
      if (evalRetCode == ex_expr::EXPR_ERROR)
	{
	  return -1;
	}
      if (hbaseAccessTdb().getUseCif() && rowLen < convertRowLen_)
        convertRowLen_= rowLen;
    }

  return 0;
}

////////////////////////////////////////////////////////////////////////
// hbase JNI returns all versions of cell values for each row id as a single list.
//
// Example:
//  a row has 3 columns, C1, C2, C3.
//  C1 has 3 cell versions, C2 has 1 version, C3 has 2 versions.
//  Returned cell values from hbase will be:
//    C1V1, C1V2, C1V3, C2V1, C3V1, C3V2
//
// These cell values will be transformed and returned as 3 rows:
//    RowVersion1: C1V1  C2V1 C3V1
//    RowVersion2: C1V2  C2V1 C3V2
//    RowVersion3: C1V3  C2V1 C3V2
//
// Method setupSQMultiVersionRow does this transformation and creates 
// an array with multiple rows.
// Method createSQRowFromHbaseFormatMulti() loops through this array
// and returns these 3 rows.
// 
// colValVec:  one entry for each fetched column. Each column can have
//                   max numVers versions. numVers is specified by user at compile time.
//
// For each column entry:
//          colValVec_[i].numVersions_: number of versions 
//          colValVec_[i].versionArray_:  array of numVers pointer entries.
//                                                Each entry points to a column version, if one exists
//          colValVec_[i].timestampArray_:  array of timestamp values, if timestamp is
//                                                to be returned.
//                                                Each entry contains timestamp for that version.
//          colValVec_[i].versionNumArray_:  array of version numbers, if version is
//                                                to be returned.
//                                                Each entry contains version num for that version.
//
//
/////////////////////////////////////////////////////////////////////////
Lng32 ExHbaseAccessTcb::setupSQMultiVersionRow()
{
  // no columns are being fetched from hbase, do not create a row. TBD for RAW.
  Lng32 numFetchedCols = hbaseAccessTdb().listOfFetchedColNames()->numEntries();
  if (hbaseAccessTdb().listOfFetchedColNames()->numEntries() == 0)
    return 0;

  hbaseAccessTdb().listOfFetchedColNames()->position();
  Lng32 idx = -1;

  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().asciiTuppIndex_);
  Attributes * attr = NULL;

  char *colVal; 
  Lng32 colValLen;
  long timestamp;
  char *colName;
  short colNameLen;
  BYTE nullVal;
  Lng32 retcode;
  int numCells; 
  retcode = ehi_->getNumCellsPerRow(numCells);
  if (retcode == HBASE_ACCESS_NO_ROW)
    return retcode ;
  
  if (retcode != HBASE_ACCESS_SUCCESS)
    {
      setupError(retcode, "", "getNumCellsPerRow()");
      return retcode;
    }
  
  if (colValVec_ == NULL)
    {
      colValVec_ = new(getHeap()) ColValVec[numFetchedCols];
        
      for (int i = 0; i < numFetchedCols; i++)
        {
          Lng32 numVers = hbaseAccessTdb().getHbaseAccessOptions()->getNumVersions();
          if (numVers == -2)
            numVers = 100;
          char ** v = new(getHeap()) char*[numVers];

          colValVec_[i].numVersions_ = 0;
          colValVec_[i].versionArray_ = v;
          if (hbaseAccessTdb().isHbaseTimestampNeeded())
            {
              colValVec_[i].timestampArray_ = new(getHeap()) Int64[numVers];
            }

          if (hbaseAccessTdb().isHbaseVersionNeeded())
            {
              colValVec_[i].versionNumArray_ = new(getHeap()) Int64[numVers];
            }

          for (int j = 0; j < numVers; j++)
            {
              colValVec_[i].versionArray_[j] = NULL;

              if (hbaseAccessTdb().isHbaseTimestampNeeded())
                colValVec_[i].timestampArray_[j] = 0;

              if (hbaseAccessTdb().isHbaseVersionNeeded())
                colValVec_[i].versionNumArray_[j] = 0;
            }
        }
    }
  else
    {
      for (int i = 0; i < numFetchedCols; i++)
        {
          colValVec_[i].numVersions_ = 0;
        }
    }
  colValVecSize_ = 0;

  std::string prevColName;
  int colValPos = 0;
  for (int colNo= 0; colNo < numCells; colNo++)
    {
      retcode = ehi_->getColName(colNo, &colName, colNameLen, timestamp);
      if (retcode != HBASE_ACCESS_SUCCESS)
        {
          setupError(retcode, "", "getColName()");
          return retcode;
        }
      
     if (prevColName.empty())
        {
          prevColName.assign(colName, colNameLen);
          colValPos = 0;
        }
      else if  (std::string(colName, colNameLen) == prevColName)
        {
          colValPos++;
        }
      else // colName != prevColName
        {
          colValPos = 0;
          prevColName = std::string(colName, colNameLen);
        }

     // if col name changed, find idx into attributes array.
     // otherwise, use previous idx. This avoids looping over listOfFetchedColNames
     // in getColPos.
     if (colValPos == 0) 
       {
         if (! getColPos(colName, colNameLen, idx)) // not found
           { 
             ex_assert(FALSE, "Error in getColPos()");
           }
       }

      Attributes * attr = asciiSourceTD->getAttr(idx);
      if (! attr)
         ex_assert(FALSE, "Attr not found -1");
      
       char ** colValEntries = colValVec_[idx].versionArray_;
      if (colValEntries[colValPos] == NULL)
        {
          Lng32 colValAllocLen = ((attr->getNullFlag() ? sizeof(short) : 0) +
                                  sizeof(Lng32) + attr->getLength());
          colVal = new(getHeap()) char[colValAllocLen];
          colValEntries[colValPos] = colVal;
          colValVec_[idx].numVersions_++;
        }
      else
        {
          colVal = colValEntries[colValPos];
          colValVec_[idx].numVersions_++;
        }

      if (colValVec_[idx].numVersions_ > colValVecSize_)
        colValVecSize_ = colValVec_[idx].numVersions_;
      
      char * nullData = (attr->getNullFlag() ? (char*)colVal : NULL);
      char * vcLen = (char*)colVal + (nullData ? sizeof(short) : 0);
      char * colData = vcLen + sizeof(Lng32);
      colValLen = attr->getLength();
      retcode = ehi_->getColVal(colNo, (BYTE*)colData, colValLen, 
                                attr->getNullFlag(), nullVal);

      if (retcode != HBASE_ACCESS_SUCCESS)
        {
          setupError(retcode, "", "getColVal()");
          return retcode;
        }

      if (nullData)
        {
          if (nullVal)
            *(short*)nullData = -1;
          else
            *(short*)nullData = 0;
        }
      *(Lng32*)vcLen = colValLen;

      if (hbaseAccessTdb().isHbaseTimestampNeeded())
        colValVec_[idx].timestampArray_[colValPos] = timestamp;

      if (hbaseAccessTdb().isHbaseVersionNeeded())
        colValVec_[idx].versionNumArray_[colValPos] = colValPos+1;
    }

  colValEntry_ = 0;

  return 0;
}

Lng32 ExHbaseAccessTcb::createSQRowFromHbaseFormatMulti()
{
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  Lng32 numFetchedCols = hbaseAccessTdb().listOfFetchedColNames()->numEntries();

  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().asciiTuppIndex_);

  ExpTupleDesc * convertTuppTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().convertTuppIndex_);

  if (colValEntry_ >= colValVecSize_)
    return HBASE_ACCESS_NO_ROW;

  // initialize as missing cols.
  memset(asciiRowMissingCols_, 1, asciiSourceTD->numAttrs());

  Lng32 retcode;

  Attributes * attr = NULL;
  for (int idx = 0; idx < numFetchedCols; idx++)
    {
      attr = asciiSourceTD->getAttr(idx);
 
      char ** colValEntries = colValVec_[idx].versionArray_;
      Lng32 numColValEntries = colValVec_[idx].numVersions_;
      if (numColValEntries > 0)
        {
          // not missing any more
          asciiRowMissingCols_[idx] = 0;

          Lng32 entryPos =
            (colValEntry_ >= numColValEntries ? colValEntry_ - 1 : colValEntry_);
          char *s = colValEntries[entryPos];
          char * colVal = (char*)s;
          char * nullData = NULL;
          if (attr->getNullFlag())
            {
              nullData = colVal;
              if (*(short*)nullData)
                *(short*)&asciiRow_[attr->getNullIndOffset()] = -1;
              else
                *(short*)&asciiRow_[attr->getNullIndOffset()] = 0;
            }
          
          char * vcLen = (char*)colVal + (nullData ? sizeof(short) : 0);
          Lng32 colDataLen = *(Lng32*)vcLen;
          if (attr->getVCIndicatorLength() > 0)
            {
              if (attr->getVCIndicatorLength() == sizeof(short))
                *(short*)&asciiRow_[attr->getVCLenIndOffset()] = colDataLen; 
              else
                *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = colDataLen; 
            }

          char * colData = vcLen + sizeof(Lng32);

          colVal = &asciiRow_[attr->getOffset()];
          memcpy(colVal, colData, colDataLen);

          if (hbaseAccessTdb().isHbaseTimestampNeeded())
            latestTimestampForCols_[idx] = colValVec_[idx].timestampArray_[entryPos];

          if (hbaseAccessTdb().isHbaseVersionNeeded())
            latestVersionNumForCols_[idx] = colValVec_[idx].versionNumArray_[entryPos];
        }
    }

  // fill in null or default values for missing cols.
  for (int idx = 0; idx < asciiSourceTD->numAttrs(); idx++)
    {
      if (asciiRowMissingCols_[idx] == 1) // missing
	{
	  attr = asciiSourceTD->getAttr(idx);
	  if (! attr)
             ex_assert(FALSE, "Attr not found -2");
	  
	  char * defVal = attr->getDefaultValue();
	  char * defValPtr = defVal;
	  short nullVal = 0;
	  if (attr->getNullFlag())
	    {
	      nullVal = *(short*)defVal;
	      *(short*)&asciiRow_[attr->getNullIndOffset()] = nullVal;
	      
	      defValPtr += 2;
	    }
	  
          Lng32 copyLen;
	  if (! nullVal)
	    {
	      if (attr->getVCIndicatorLength() > 0)
		{
		 copyLen = *(short*)defValPtr;
		  if (attr->getVCIndicatorLength() == sizeof(short))
		    *(short*)&asciiRow_[attr->getVCLenIndOffset()] = copyLen; 
		  else
		    *(Lng32*)&asciiRow_[attr->getVCLenIndOffset()] = copyLen;
		  defValPtr += attr->getVCIndicatorLength();
		}
	        else
		{
		  copyLen = attr->getLength();
                }
		char *destPtr = &asciiRow_[attr->getOffset()];
		str_cpy_all(destPtr, defValPtr, copyLen);
	    } // not nullVal
	} // missing col
    }

  workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
    .setDataPointer(convertRow_);
  workAtp_->getTupp(hbaseAccessTdb().asciiTuppIndex_) 
    .setDataPointer(asciiRow_);
  if (hbaseAccessTdb().hbaseTimestampTuppIndex_ > 0)
    {
      workAtp_->getTupp(hbaseAccessTdb().hbaseTimestampTuppIndex_) 
        .setDataPointer((char*)latestTimestampForCols_);
    }
  if (hbaseAccessTdb().hbaseVersionTuppIndex_ > 0)
    {
      workAtp_->getTupp(hbaseAccessTdb().hbaseVersionTuppIndex_) 
        .setDataPointer((char*)latestVersionNumForCols_);
    }
  
  if (convertExpr())
    {
      ex_expr::exp_return_type evalRetCode =
	convertExpr()->eval(pentry_down->getAtp(), workAtp_);
      if (evalRetCode == ex_expr::EXPR_ERROR)
	{
	  return -1;
	}
    }

  colValEntry_++;

  return 0;
}

// return TRUE is values are missing in aligned format row.
// Added columns are always added after all the existing columns
// in a table.
// This methods checks for added columns from in the list of attributes.
// Check is done from last to first attr.
// If an attr is a specialField (added col), then check is done to see if
// it is missing. TRUE is returned in that case.
// If a non-added field is reached, then no cols are missing. 
// FALSE is returned in that case.
NABoolean ExHbaseAccessTcb::missingValuesInAlignedFormatRow(
     ExpTupleDesc * tdesc, char * alignedRow, Lng32 alignedRowLen)
{
  if (NOT tdesc->addedFieldPresent())
    return FALSE;

  UInt32 ff = ExpTupleDesc::getFirstFixedOffset
    (alignedRow, ExpTupleDesc::SQLMX_ALIGNED_FORMAT);

  Attributes * attr = NULL;
  for (int idx = tdesc->numAttrs()-1; idx >= 0; idx--)
    {
      attr = tdesc->getAttr(idx);

      // if a regular (non-added) attr is reached, we are done.
      // There cannot be any added columns to the left of this attr.
      if (NOT attr->isAddedCol())
        return FALSE;

      if (Attributes::isDefaultValueNeeded(
               attr, tdesc, ff, 
               attr->getVCIndicatorLength(), attr->getVoaOffset(), 
               alignedRow, alignedRowLen))
        return TRUE;

    } // for

  return FALSE;
}

Lng32 ExHbaseAccessTcb::createSQRowFromAlignedFormat(Int64 *latestRowTimestamp)
{
  // no columns are being fetched from hbase, do not create a row.
  if (hbaseAccessTdb().listOfFetchedColNames()->numEntries() == 0)
    return 0;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().asciiTuppIndex_);

  ExpTupleDesc * convertTuppTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().convertTuppIndex_);
  
  Attributes * attr = NULL;

  // initialize latest timestamp to 0 for every column
  memset(latestTimestampForCols_, 0, (asciiSourceTD->numAttrs()*sizeof(long)));
  if (latestRowTimestamp != NULL)
      *latestRowTimestamp = -1;

  hbaseAccessTdb().listOfFetchedColNames()->position();
  Lng32 idx = -1;

  BYTE *colVal; 
  Lng32 colValLen;
  long timestamp;
  char *colName;
  short colNameLen;
  BYTE nullVal;
  Lng32 retcode;
  int numCells; 
  retcode = ehi_->getNumCellsPerRow(numCells);
  if (retcode == HBASE_ACCESS_NO_ROW)
     return retcode ;

  if (retcode != HBASE_ACCESS_SUCCESS)
  {
     setupError(retcode, "", "getNumCellsPerRow()");
     return retcode;
  }

  // aligned format should only return one column.
  if (numCells > 2)
    {
      // error
      return -HBASE_CREATE_ROW_ERROR;
    }

  Lng32 asciiRowLen = hbaseAccessTdb().asciiRowLen_;
  for (int colNo= 0; colNo < numCells; colNo++)
  {
    retcode = ehi_->getColName(colNo, &colName, colNameLen, timestamp);
    if (retcode != HBASE_ACCESS_SUCCESS)
      {
        setupError(retcode, "", "getColName()");
        return retcode;
      }
    
    if (! getColPos(colName, colNameLen, idx)) // not found
      { 
        ex_assert(FALSE, "Error in getColPos()");
      }
    
    if (timestamp > latestTimestampForCols_[idx])
      latestTimestampForCols_[idx] = timestamp;

    if (latestRowTimestamp != NULL && timestamp > *latestRowTimestamp)
        *latestRowTimestamp = timestamp;

    // copy to asciiRow only if this is the latest version seen so far
    // for this column. On 6/10/2014 we get two versions for a newly
    // updated column that has not been committed yet.
    if (timestamp == latestTimestampForCols_[idx]) 
      {
        colVal = (BYTE*)asciiRow_;
        retcode = ehi_->getColVal(colNo, colVal, asciiRowLen,
                                  FALSE, nullVal);
        if (retcode != HBASE_ACCESS_SUCCESS)
          {
            setupError(retcode, "", "getColVal()");
            return retcode;
          }
      }
  }

  // check to see if added columns are missing from this row.
  // If they are, pcode evaluation cannot be used.
  NABoolean doEvalClauses = FALSE;
  if ((convertExpr()) &&
      (convertExpr()->getPCodeBinary()) &&
      (asciiSourceTD->addedFieldPresent()) &&
      (missingValuesInAlignedFormatRow(asciiSourceTD, asciiRow_, asciiRowLen)))
    doEvalClauses = TRUE;

  workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
    .setDataPointer(convertRow_);
  workAtp_->getTupp(hbaseAccessTdb().asciiTuppIndex_) 
    .setDataPointer(asciiRow_);
  
  if (convertExpr())
    {
      convertRowLen_ = hbaseAccessTdb().convertRowLen();
      UInt32 rowLen = convertRowLen_;
      ex_expr::exp_return_type evalRetCode = ex_expr::EXPR_OK;

      if (doEvalClauses)
        {
          evalRetCode = 
            convertExpr()->evalClauses(convertExpr()->getClauses(),
                                       pentry_down->getAtp(), workAtp_, 
                                       asciiRowLen, &rowLen,
                                       NULL, NULL);
        }
      else
        {
          evalRetCode = 
            convertExpr()->eval(pentry_down->getAtp(), workAtp_, NULL, 
                                asciiRowLen, &rowLen);
        }

      if (evalRetCode == ex_expr::EXPR_ERROR)
	{
	  return -1;
	}
      if (hbaseAccessTdb().getUseCif() &&
          rowLen < convertRowLen_)
        convertRowLen_=rowLen;
    }

  return 0;
}

// returns:
// 0, if expr is false
// 1, if no pred or expr is true
// -1, if expr error
short ExHbaseAccessTcb::applyPred(ex_expr * expr, UInt16 tuppIndex,
				  char * tuppRow)
{
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  if (! expr)
    {
      return 1;
    }
  
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  
  if ((tuppRow) && (tuppIndex > 0))
    workAtp_->getTupp(tuppIndex)
      .setDataPointer(tuppRow);
  else
    workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
      .setDataPointer(convertRow_);
  
  exprRetCode =
    expr->eval(pentry_down->getAtp(), workAtp_);
  
  if (exprRetCode == ex_expr::EXPR_ERROR)
    {
      return -1;
    }
  
  if (exprRetCode == ex_expr::EXPR_TRUE)
    {
      return 1;
    }
  
  return 0;
}

short ExHbaseAccessTcb::extractColFamilyAndName(char * input, 
						Text &colFam, Text &colName)
{
  return ExFunctionHbaseColumnLookup::extractColFamilyAndName(input, 
                                                              -1,
							      TRUE,
							      colFam, colName);
}

short ExHbaseAccessTcb::evalKeyColValExpr(HbaseStr &columnToCheck, HbaseStr &colValToCheck)
{
  if (! keyColValExpr())
    return -1;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  
  workAtp_->getTupp(hbaseAccessTdb().keyColValTuppIndex_)
    .setDataPointer(keyColValRow_);
  
  exprRetCode =
    keyColValExpr()->eval(pentry_down->getAtp(), workAtp_);
  
  if (exprRetCode == ex_expr::EXPR_ERROR)
    {
      return -1;
    }
  
  memcpy(colValToCheck.val, keyColValRow_, hbaseAccessTdb().keyColValLen_);
  colValToCheck.len = hbaseAccessTdb().keyColValLen_;

  char * keyColNamePtr = hbaseAccessTdb().keyColName();
  short nameLen = *(short*)keyColNamePtr;
  char * keyColName = &keyColNamePtr[sizeof(short)];
  memcpy(columnToCheck.val, keyColName, nameLen);
  columnToCheck.len = nameLen;

  return 0;
}

short ExHbaseAccessTcb::evalEncodedKeyExpr()
{
  if (! encodedKeyExpr())
    return 0;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  
  workAtp_->getTupp(hbaseAccessTdb().keyTuppIndex_)
    .setDataPointer(encodedKeyRow_);
  
  exprRetCode =
    encodedKeyExpr()->eval(pentry_down->getAtp(), workAtp_);
  
  if (exprRetCode == ex_expr::EXPR_ERROR)
    {
      return -1;
    }
  
  return 0;
}

short ExHbaseAccessTcb::evalRowIdExpr(NABoolean isVarchar)
{
  if (! rowIdExpr())
    return 0;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  
  workAtp_->getTupp(hbaseAccessTdb().rowIdTuppIndex_)
    .setDataPointer(rowIdRow_);
  
  exprRetCode =
    rowIdExpr()->eval(pentry_down->getAtp(), workAtp_);
  
  if (exprRetCode == ex_expr::EXPR_ERROR)
    {
      return -1;
    }
  
  if (isVarchar)
    {
      rowId_.val = &rowIdRow_[SQL_VARCHAR_HDR_SIZE];
      rowId_.len = *(short*)rowIdRow_;
    }
  else
    {
      rowId_.val = rowIdRow_;
      rowId_.len = hbaseAccessTdb().rowIdLen_;
    }

  return 0;
}

short ExHbaseAccessTcb::evalInsDelPreCondExpr()
{
  if (! insDelPreCondExpr()) {
     return 1;
  }

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  
  exprRetCode =
    insDelPreCondExpr()->eval(pentry_down->getAtp(), workAtp_);

  switch (exprRetCode) {
     case ex_expr::EXPR_FALSE:
        return 0;
     case ex_expr::EXPR_TRUE:
        return 1;
     default:
        return -1;
  }
  return 0; 
}

short ExHbaseAccessTcb::evalConstraintExpr(ex_expr *expr, UInt16 tuppIndex,
                                  char * tuppRow)
{
  if (expr == NULL) {
     return 1;
  }
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  if ((tuppRow) && (tuppIndex > 0))
    workAtp_->getTupp(tuppIndex).setDataPointer(tuppRow);
  else
    workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_).setDataPointer(convertRow_);
  exprRetCode = expr->eval(pentry_down->getAtp(), workAtp_);
  if (exprRetCode == ex_expr::EXPR_ERROR)
     return -1;
  else if (exprRetCode == ex_expr::EXPR_TRUE)
     return 1;
  else
     return 0;
}

short ExHbaseAccessTcb::evalRowIdAsciiExpr(const char * inputRowIdVals,
					   char * rowIdBuf, // input: buffer where rowid is created
					   char* &outputRowIdPtr,  // output: ptr to rowid.
					   Lng32 excludeKey,
					   Lng32 &outputRowIdLen)
{
  if (! rowIdExpr())
    return -1;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  
  ExpTupleDesc * asciiSourceTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (hbaseAccessTdb().rowIdAsciiTuppIndex_);
  
  Lng32 currPos = 0;
  for (CollIndex i = 0; i < asciiSourceTD->numAttrs(); i++)
    {
      Attributes * attr = asciiSourceTD->getAttr(i);
      if (! attr)
	{
	  // error
	  return -1;
	}
  
      short inputRowIdValLen = *(short*)&inputRowIdVals[currPos];
      currPos += sizeof(short);

      short nullVal = 0;
      if (attr->getNullFlag())
	{
	  inputRowIdValLen -= sizeof(short);
	  
	  nullVal = *(short*)&inputRowIdVals[currPos];
	  
	  if (nullVal)
	    {
	      *(short*)&rowIdAsciiRow_[attr->getNullIndOffset()] = -1;
	    }
	  else
	    {
	      *(short*)&rowIdAsciiRow_[attr->getNullIndOffset()] = 0;
	    }
	  
	  currPos += sizeof(short);
	}
      
      const char * inputRowIdVal = &inputRowIdVals[currPos];
      
      if (attr->getVCIndicatorLength() > 0)
	{
	  *(short*)&rowIdAsciiRow_[attr->getVCLenIndOffset()] = inputRowIdValLen;
	  *(Int64*)&rowIdAsciiRow_[attr->getOffset()] = (Int64)inputRowIdVal;
	}
      else
	{
	  char * srcPtr = &rowIdAsciiRow_[attr->getOffset()];
	  Lng32 copyLen = MINOF(attr->getLength(), inputRowIdValLen);
	  str_cpy_all(srcPtr, (char*)inputRowIdVal, copyLen);
	}
      
      currPos += inputRowIdValLen;
    }
  
  workAtp_->getTupp(hbaseAccessTdb().rowIdTuppIndex_)
    .setDataPointer(rowIdBuf);
  workAtp_->getTupp(hbaseAccessTdb().rowIdAsciiTuppIndex_)
    .setDataPointer(rowIdAsciiRow_);
  
  exprRetCode =
    rowIdExpr()->eval(pentry_down->getAtp(), workAtp_);
  
  if (exprRetCode == ex_expr::EXPR_ERROR)
    {
      return -1;
    }

  // For non-SQ tables,
  // rowid is created as a null terminated string in rowIdBuf. (ANSI char type)
  // At this point, there are 2 extra bytes allocated in the beginning. Need to
  // figure out why that is and if it is needed.
  // Skip the first 2 bytes and point to the actual row.
  // If exclude flag is set, then add a null byte and increment the length by 1.
  if (hbaseAccessTdb().sqHbaseTable())
    {
      if (hbaseAccessTdb().keyInVCformat())
        {
          // Key is in varchar format.
          outputRowIdLen = *(short*)rowIdBuf;
          outputRowIdPtr = rowIdBuf + sizeof(short);
        }
      else
        {
          outputRowIdPtr = rowIdBuf; 
          outputRowIdLen = hbaseAccessTdb().getRowIDLen();
        }
    }
  else
    {
      outputRowIdPtr = &rowIdBuf[2]; 
      outputRowIdLen = strlen(outputRowIdPtr);
    }

  if (excludeKey)
    {
      outputRowIdPtr[outputRowIdLen] = '\0';
      outputRowIdLen++;
    }

  return 0;
}

void ExHbaseAccessTcb::setupPrevRowId()
{
  if (prevRowIdMaxLen_ < rowId_.len)
    {
      if (prevRowId_.val)
	NADELETEBASIC(prevRowId_.val, getHeap());
      prevRowIdMaxLen_ = rowId_.len;
      prevRowId_.val = new(getHeap())  char[prevRowIdMaxLen_];
    }
  
  str_cpy_all(prevRowId_.val, rowId_.val, rowId_.len);
  prevRowId_.len = rowId_.len;
  if (rowwiseRow_)
    {
      rowwiseRowLen_ = 0;
    }
}  

void ExHbaseAccessTcb::extractColNameFields
(char * inValPtr, short &colNameLen, char* &colName)
{
  // inValPtr points to:
  // 2 bytes len, len bytes colname.
  char * currPtr = inValPtr;
  colNameLen = *(short*)currPtr;
  currPtr += sizeof(short);
  colName = currPtr;
}

Lng32 ExHbaseAccessTcb::setupListOfColNames(Queue * listOfColNames,
					    LIST(HbaseStr) &columns)
{
  if (columns.isEmpty() && listOfColNames)
    {
      listOfColNames->position();
      HbaseStr colNameText;
      for (Lng32 i = 0; i < listOfColNames->numEntries(); i++)
	{
          short colNameLen;
          char * colName;
          extractColNameFields((char*)listOfColNames->getCurr(),
                               colNameLen, colName);
 
	  listOfColNames->advance();
	  
	  colNameText.val = colName;
          colNameText.len = colNameLen;
	  columns.insert(colNameText);
	}
    }
  
  return 0;
}

Lng32 ExHbaseAccessTcb::setupListOfColNames(Queue * listOfColNames,
					    LIST(NAString) &columns)
{
  if (columns.isEmpty() && listOfColNames)
    {
      listOfColNames->position();
      for (Lng32 i = 0; i < listOfColNames->numEntries(); i++)
	{
          short colNameLen;
          char * colName;
          extractColNameFields((char*)listOfColNames->getCurr(),
                               colNameLen, colName);
 
	  listOfColNames->advance();
	  
          NAString colNameText(colName, colNameLen, getHeap());
	  columns.insert(colNameText);
	}
    }
  
  return 0;
}

Lng32 ExHbaseAccessTcb::setupUniqueRowIdsAndCols
(ComTdbHbaseAccess::HbaseGetRows*hgr)
{
  if ((! hgr->rowIds()) ||
      (hgr->rowIds()->numEntries() == 0))
    {
      setupError(-HBASE_OPEN_ERROR, "", "RowId list is empty");
      return -1;
    }
  
  HbaseStr rowIdRowText;
  hgr->rowIds()->position();
  rowIds_.clear();
  for (Lng32 i = 0; i < hgr->rowIds()->numEntries(); i++)
    {
      if (! rowIdExpr())
	{
	  setupError(-HBASE_OPEN_ERROR, "", "RowId Expr is empty");
	  return -1;
	}
      
      const char * inputRowIdVals = (char*)hgr->rowIds()->getNext();
      char * rowIdRow = NULL;
      if (hgr->rowIds()->numEntries() == 1)
	{
	  rowIdRow = rowIdRow_;
	}
      else
	{
	  // need to be deleted. TBD.
	  rowIdRow = new(getGlobals()->getDefaultHeap()) char[hbaseAccessTdb().rowIdLen_ + 2];
	}
      
      Lng32 rowIdLen = 0;
      char * rowIdPtr = NULL;
      if (evalRowIdAsciiExpr(inputRowIdVals, 
			     rowIdRow, 
			     rowIdPtr,
			     0, 
			     rowIdLen) == -1)
	{
	  return -1;
	}
      
      rowIdRowText.val = rowIdPtr;
      rowIdRowText.len = rowIdLen;
      rowIds_.insert(rowIdRowText);
    }

  setupListOfColNames(hbaseAccessTdb().listOfFetchedColNames(), columns_);

  return 0;
}

// sets up beginRowId_, endRowId_ and columns_ fields.
Lng32 ExHbaseAccessTcb::setupSubsetRowIdsAndCols
(ComTdbHbaseAccess::HbaseScanRows* hsr)
{
  Lng32 rowIdLen;
  char * rowIdPtr = NULL;

  const char * inputBeginRowIdVals = (char*)hsr->beginRowId_;
  rowIdLen = *(short*)inputBeginRowIdVals;
  if (rowIdLen > 0)
    {
      if (evalRowIdAsciiExpr(inputBeginRowIdVals, 
			     beginRowIdRow_,
			     rowIdPtr,
			     hsr->beginKeyExclusive_, 
			     rowIdLen) == -1)
	{
	  return -1;
	}

      beginRowId_.assign(rowIdPtr, rowIdLen);
    }
  else
    {
      beginRowId_.assign(hsr->beginRowId_, rowIdLen);
    }
  
  const char * inputEndRowIdVals = (char*)hsr->endRowId_;
  rowIdLen = *(short*)inputEndRowIdVals;
  if (rowIdLen > 0)
    {
      if (evalRowIdAsciiExpr(inputEndRowIdVals, 
			     endRowIdRow_,
			     rowIdPtr,
			     (!hsr->endKeyExclusive_), 
			     rowIdLen) == -1)
	{
	  return -1;
	}
      
      endRowId_.assign(rowIdPtr, rowIdLen);
    }
  else
    {
      endRowId_.assign(hsr->endRowId_, rowIdLen);
    }

  setupListOfColNames(hbaseAccessTdb().listOfFetchedColNames(), columns_);

  return 0;
}

Lng32 ExHbaseAccessTcb::initNextKeyRange(sql_buffer_pool *pool,
			        	 atp_struct * atp)
{
  if (keyExeExpr())
    keyExeExpr()->initNextKeyRange(pool, atp);
  else
    return -1;

  return 0;
}

Lng32 ExHbaseAccessTcb::setupUniqueKeyAndCols(NABoolean doInit)
{
  if (doInit)
    rowIds_.clear();

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  keyRangeEx::getNextKeyRangeReturnType keyRangeStatus;

  initNextKeyRange();

  keyRangeStatus = 
    keySubsetExeExpr_->getNextKeyRange(pentry_down->getAtp(), FALSE, TRUE);

  if (keyRangeStatus == keyRangeEx::EXPRESSION_ERROR)
    {
      return -1;
    }

  tupp &keyData = keySubsetExeExpr_->getBkData();
  char * beginKeyRow = keyData.getDataPointer();

  HbaseStr rowIdRowText;
  if ((NOT hbaseAccessTdb().sqHbaseTable()) ||
      (hbaseAccessTdb().keyInVCformat()))
    {
      // Key is in varchar format.
      short keyLen = *(short*)beginKeyRow;
      rowIdRowText.val = beginKeyRow + sizeof(short);
      rowIdRowText.len = keyLen;
    }
  else
    {
      rowIdRowText.val = beginKeyRow;
      rowIdRowText.len = hbaseAccessTdb().keyLen_;
    }

  if (keyRangeStatus == keyRangeEx::NO_MORE_RANGES)
    {
      // At the end of range, added an extra byte with "0" value to
      // make the key exclusive which is required by hbase scan.
      // Note that the key buffer should have 2 extra bytes allocated
      rowIdRowText.val[rowIdRowText.len] = '\0';
      rowIdRowText.len += 1;
    }

  rowIds_.insert(rowIdRowText);

  if (doInit)
    setupListOfColNames(hbaseAccessTdb().listOfFetchedColNames(), columns_);

  return 0;
}


keyRangeEx::getNextKeyRangeReturnType ExHbaseAccessTcb::setupSubsetKeys
(NABoolean fetchRangeHadRows)
{
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  keyRangeEx::getNextKeyRangeReturnType keyRangeStatus;
  
  keyRangeStatus = 
    keyExeExpr()->getNextKeyRange(pentry_down->getAtp(), fetchRangeHadRows, 
				  (hbaseAccessTdb().sqHbaseTable() ? TRUE : FALSE));
  
  if (keyRangeStatus == keyRangeEx::EXPRESSION_ERROR)
    {
      return keyRangeEx::EXPRESSION_ERROR;
    }

  tupp &beginKeyData = keyExeExpr()->getBkData();
  char * beginKeyRow = beginKeyData.getDataPointer();

  tupp &endKeyData = keyExeExpr()->getEkData();
  char * endKeyRow = endKeyData.getDataPointer();

  short beginKeyLen = hbaseAccessTdb().keyLen_;
  short endKeyLen = hbaseAccessTdb().keyLen_;

  if ((NOT hbaseAccessTdb().sqHbaseTable()) ||
      (hbaseAccessTdb().keyInVCformat()))
    {
      // key in varchar format
      beginKeyLen = *(short*)beginKeyRow;
      beginKeyRow += sizeof(short);
      if (beginKeyRow[0] == '\0')
	beginKeyLen = 1;
      
      endKeyLen = *(short*)endKeyRow;
      endKeyRow += sizeof(short);
      if (endKeyRow[0] == '\0')
	endKeyLen = 1;
    }

  if (keyRangeStatus == keyRangeEx::NO_MORE_RANGES)
    {
      memset(beginKeyRow, '\377',  beginKeyLen);
      beginRowId_.assign(beginKeyRow, beginKeyLen);
      beginRowId_.append(1, '\0');

      memset(endKeyRow, '\0',  endKeyLen);
      endRowId_.assign(endKeyRow, endKeyLen);

      return keyRangeEx::NO_MORE_RANGES;
    }
  else
    {
      beginRowId_.assign(beginKeyRow, beginKeyLen);
      endRowId_.assign(endKeyRow, endKeyLen);
      
      if (keyExeExpr()->getBkExcludeFlag())
	beginRowId_.append(1, '\0');
      
      if (! keyExeExpr()->getEkExcludeFlag())
	endRowId_.append(1, '\0');
    }

  return keyRangeStatus;
}

Lng32 ExHbaseAccessTcb::setupSubsetKeysAndCols()
{
  initNextKeyRange();
  
  if (setupSubsetKeys() == keyRangeEx::EXPRESSION_ERROR)
    return -1;

  setupListOfColNames(hbaseAccessTdb().listOfFetchedColNames(), columns_);

  return 0;
}

Lng32 ExHbaseAccessTcb::genAndAssignSyskey(UInt16 tuppIndex, char * tuppRow)
{
  if (hbaseAccessTdb().addSyskeyTS())
    {
      char * syskeyPtr;
      if (hbaseAccessTdb().alignedFormat())
        {
          // syskey is the first attribute
          ExpTupleDesc * rowTD =
            hbaseAccessTdb().workCriDesc_->getTupleDescriptor
            (tuppIndex);
          
          Attributes * attr = rowTD->getAttr(0);
	  syskeyPtr = &tuppRow[attr->getOffset()];
        }
      else
        {
          syskeyPtr = tuppRow;
        }

      *(Int64*)syskeyPtr = generateUniqueValueFast();
    }
  
  return 0;
}

void ExHbaseAccessTcb::allocateDirectBufferForJNI(UInt32 rowLen)
{

  if (rowLen > directRowBufferLen_)
  {
     if (directRowBuffer_ != NULL)
     {
        NADELETEBASIC(directRowBuffer_, getHeap());
        directRowBuffer_ = NULL;
     }
  }
  if (directRowBuffer_ == NULL)
  {
     directRowBuffer_ = new (getHeap()) BYTE[rowLen];
     directRowBufferLen_ = rowLen;
  }
  row_.val = (char *)directRowBuffer_;
  row_.len = 0;
}


void ExHbaseAccessTcb::allocateDirectRowBufferForJNI(
                      short numCols, short maxRows)
{
  UInt32 directBufferOverhead;
  UInt32 maxRowLen;
  UInt32 rowLen = hbaseAccessTdb().getRowLen();

  if (directRowBuffer_ == NULL)
  {
     directBufferOverhead = sizeof(numCols) + // Number of columns in the row
                         (numCols * (2 + // for name len 
                                    5  +  // for colname len - To accomadate upto 32767 column nos
                                    4 )) // for col value len 
                        + numCols; // 1 byte null value
     maxRowLen  = rowLen + directBufferOverhead;

     // limit direct buffer len to 1G
     Int64 tempDirectBuflen = (Int64)maxRowLen * (Int64)maxRows;
     Int64 maxDirectBuflen = 1024*1024*1024;

     if (tempDirectBuflen > maxDirectBuflen)
       {
         directRowBufferLen_ = MINOF(tempDirectBuflen, maxDirectBuflen);
         directBufferMaxRows_ = (Int64)directRowBufferLen_ / (Int64)maxRowLen;
       }
     else
       {
         directRowBufferLen_ = (maxRowLen * maxRows);
         directBufferMaxRows_ = maxRows;
       }

     directRowBuffer_ = new (getHeap()) BYTE[directRowBufferLen_];
     rows_.val = (char *)directRowBuffer_;
     rows_.len = 0;
  }
  if (directBufferRowNum_ == 0)
  {
     row_.val = rows_.val;
     row_.len = 0;
     rows_.len = 0;
  }
  else
  {
    rows_.len += row_.len;
    ex_assert(rows_.len < directRowBufferLen_, "Direct row buffer overflow");
    row_.val = rows_.val + rows_.len;
    row_.len = 0;
  } 
  return;
}

short ExHbaseAccessTcb::patchDirectRowBuffers()
{
  ex_assert(rows_.val != NULL, "Direct row buffer not allocaed");
  rows_.len += row_.len;
  ex_assert(rows_.len < directRowBufferLen_, "Direct row buffer overflow");
  row_.val = NULL;
  row_.len = 0; 
  return patchDirectRowIDBuffers();
}

short ExHbaseAccessTcb::patchDirectRowIDBuffers()
{
  ex_assert(rowIDs_.val != NULL, "Direct rowIDs buffer not allocaed");
  short *numRows = (short *)rowIDs_.val;
  *numRows = bswap_16(directBufferRowNum_);
  short numRowsInBuffer = directBufferRowNum_; 
  directBufferRowNum_ = 0;
  return numRowsInBuffer;
}

void ExHbaseAccessTcb::allocateDirectRowIDBufferForJNI(short maxRows)
{
   UInt32 rowIDLen;
   UInt32 maxRowIDLen;
   rowIDLen = hbaseAccessTdb().getRowIDLen();
   if (directRowIDBuffer_ == NULL)
   {
      directRowIDBufferLen_  = ((rowIDLen+1)* maxRows) + sizeof(short); // For no. of Rows
      directRowIDBuffer_ = new (getHeap()) BYTE[directRowIDBufferLen_];
      rowIDs_.val = (char *)directRowIDBuffer_; 
      rowIDs_.len = sizeof(short); // To store num of Rows
   }
   if (directBufferRowNum_ == 0)
   {
      rowIDs_.len = sizeof(short);  
      dbRowID_.val = rowIDs_.val + sizeof(short);
      dbRowID_.len = 0;
   }
}

short ExHbaseAccessTcb::createDirectRowBuffer(Text &colFamily,
                         Text &colName, 
                         Text &colVal)
{
   short colNameLen = colFamily.size() + colName.size() + 1;
   Int32 colValLen = colVal.size();
   UInt32 rowLen = sizeof(short) + sizeof(short) + // numCols, column length
                     colNameLen +
                     sizeof(Int32) + colValLen;
   if (rowLen > directRowBufferLen_)
   {
       if (directRowBuffer_ != NULL)
       {
          NADELETEBASIC(directRowBuffer_, getHeap());
          directRowBuffer_ = NULL;
       }
   }
   if (directRowBuffer_ == NULL)
   {
       directRowBuffer_ = new (getHeap()) BYTE[rowLen];
       directRowBufferLen_  = rowLen;
   }
   row_.val = (char *)directRowBuffer_;
   BYTE *temp = (BYTE *)row_.val;
   short numCols = 1;
   *(short *)temp = bswap_16(numCols);
   temp += sizeof(numCols);
   *(short *)temp = bswap_16(colNameLen);
   temp += sizeof(colNameLen);
   memcpy(temp, colFamily.data(), colFamily.size());
   temp += colFamily.size();
   *temp = ':';
   temp ++;
   memcpy(temp, colName.data(), colName.size());
   temp += colName.size();
   *(Int32 *)temp = bswap_32(colValLen);
   temp += sizeof(colValLen);
   memcpy(temp, colVal.data(), colValLen);
   temp += colValLen;
   row_.len =  (temp-(BYTE *)row_.val);
   return 0;
}


Lng32 ExHbaseAccessTcb::copyColToDirectBuffer( BYTE *rowCurPtr, 
                char *colName, short colNameLen, 
                NABoolean prependNullVal, char nullVal, 
                char *colVal, Int32 colValLen)
{
   Int32 bytesCopied;
   assert(directRowBufferLen_ >= (row_.len+colNameLen+colValLen+4));
   BYTE *temp = rowCurPtr;
   *(short *)temp = bswap_16(colNameLen);
   temp += sizeof(colNameLen);
   memcpy(temp, colName, colNameLen);
   temp += colNameLen;
   *(Int32 *)temp = bswap_32(colValLen+prependNullVal);
   temp += sizeof(colValLen);
   if (prependNullVal)
      *temp++ = nullVal;
   memcpy(temp, colVal, colValLen);
   temp += colValLen;
   bytesCopied = (temp-rowCurPtr); 
   ex_assert((bytesCopied > 0), "Corrupted buffer while copying column");
   return bytesCopied;
}

short ExHbaseAccessTcb::copyRowIDToDirectBuffer(HbaseStr &rowID)
{
   if (directBufferRowNum_ == 0)
   {
     rowIDs_.len = sizeof(short);
     dbRowID_.val = rowIDs_.val + sizeof(short);
     dbRowID_.len = 0;
   }
   UInt32 keyLen = hbaseAccessTdb().getRowIDLen();
   if (keyLen == rowID.len)
      *dbRowID_.val = '0';
   else if ((keyLen+1) == rowID.len)
      *dbRowID_.val = '1';
   else
      ex_assert(FALSE, "Invalid RowID length");
   dbRowID_.val++;  
   rowIDs_.len++;
   memcpy(dbRowID_.val, rowID.val, rowID.len);
   dbRowID_.val += rowID.len;
   dbRowID_.len = 0;
   rowIDs_.len += rowID.len; 
   directBufferRowNum_++;
   return 0;
}

short ExHbaseAccessTcb::createDirectRowBuffer( UInt16 tuppIndex, 
                 char * tuppRow,
                  Queue * listOfColNames, 
                  Queue * listOfOmittedColNames,
                  NABoolean isUpdate,
                  std::vector<UInt32> * posVec,
                  double samplingRate )
{
  char hiveBuff[500]; //@ZXtemp
  size_t hiveBuffInx = 0;
  NABoolean includeInSample = (samplingRate > 0 && (rand() / (double)RAND_MAX) < samplingRate);
  Int16 datatype;
  Int16 scale = 0;
  union
  {
    Int32 i;
    UInt32 ui;
    Int64 i64;
    Int16 i16;
    float f;
    double d;
  } numUnion;

  if (hbaseAccessTdb().alignedFormat())
    return createDirectAlignedRowBuffer(tuppIndex, tuppRow, listOfColNames,
                                        isUpdate, posVec);
  ExpTupleDesc * rowTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (tuppIndex);
  
  short colNameLen;
  char * colName;
  short nullVal = 0;
  short nullValLen = 0;
  Int32 colValLen;
  char *colVal;
  char *str;
  NABoolean prependNullVal;
  char nullValChar = 0;
  Attributes * attr;
  int numCols = 0;
  short *numColsPtr;
  char *str_1;
  NABoolean omittedColFound;
  allocateDirectRowBufferForJNI(rowTD->numAttrs());

  BYTE *rowCurPtr = (BYTE *)row_.val;
  numColsPtr = (short *)rowCurPtr;
  row_.len += sizeof(short);
  rowCurPtr += sizeof(short);
  listOfColNames->position();

  for (Lng32 i = 0; i <  rowTD->numAttrs(); i++)
    {
       
    Attributes * attr;
   
      if (!posVec)
        attr = rowTD->getAttr(i);
      else
      {
        attr = rowTD->getAttr((*posVec)[i] -1);
      }

      if (attr)
	{
         if (!posVec)
         {
           extractColNameFields((char*)listOfColNames->getCurr(),
                                colNameLen, colName);
           if (listOfOmittedColNames != NULL) {
              omittedColFound = FALSE;            
              listOfOmittedColNames->position();
              while ((str_1 = (char *)listOfOmittedColNames->getNext()) != NULL) {
                 str = (char*)listOfColNames->getCurr();
                 if (memcmp(str, str_1, colNameLen+2) == 0) {
                    omittedColFound = TRUE;
                    break;
                 }
              }
              if (omittedColFound) {
                 listOfColNames->advance();
                 continue ;
              }
           }
         }
         else
         {
           str = (char*)listOfColNames->getCurr();
           colNameLen = *(short*)(str + sizeof(UInt32));
           colName = str + sizeof(short) + sizeof(UInt32);
         }

	  colVal = &tuppRow[attr->getOffset()];

          if (includeInSample)
            {
              datatype = attr->getDatatype();
              if (DFS2REC::isNumeric(datatype))
                {
                  scale = attr->getScale();
                  strncpy((char*)&numUnion, colVal, 8);
                }
              switch (datatype)
              {
                case REC_BIN32_SIGNED:
                  hiveBuffInx += snprintf(hiveBuff+hiveBuffInx, 500 - hiveBuffInx, "%d|", numUnion.i);
                  break;
                case REC_BIN32_UNSIGNED:
                  hiveBuffInx += snprintf(hiveBuff+hiveBuffInx, 500 - hiveBuffInx, "%d|", numUnion.ui);
                  break;
                case REC_BIN64_SIGNED:
                  //printf("Scale of column %d is %d\n", i, attr->getScale());
                  hiveBuffInx += snprintf(hiveBuff+hiveBuffInx, 500 - hiveBuffInx, PFP64 "|", attr->getScale(), numUnion.i64);
                  if (scale)
                    {
                      hiveBuffInx -= (scale + 1);
                      memmove(hiveBuff+hiveBuffInx+1, hiveBuff+hiveBuffInx, scale + 1);
                      hiveBuff[hiveBuffInx] = '.';
                      hiveBuffInx += (scale + 2);
                    }
                  break;
                case REC_BYTE_F_ASCII:
                  strncpy(hiveBuff+hiveBuffInx, colVal, attr->getLength());
                  hiveBuffInx += attr->getLength();
                  hiveBuff[hiveBuffInx++] = '|';
                  break;
                case REC_BYTE_V_ASCII:
                  hiveBuffInx += snprintf(hiveBuff+hiveBuffInx, 500 - hiveBuffInx, "%s|", colVal);
                  break;
                case REC_DATETIME:
                  //@ZXbl -- need to account for datetime code
                  strncpy((char*)&numUnion, colVal, 2);
                  hiveBuffInx += snprintf(hiveBuff+hiveBuffInx, 500 - hiveBuffInx,
                                          "%.4d-%.2d-%.2d|", numUnion.i16,
                                          *(colVal+1), *(colVal+2));
                  break;
                default:
                  hiveBuffInx += sprintf(hiveBuff+hiveBuffInx, "???|");
                  break;
              }
            }

	  prependNullVal = FALSE;
	  nullVal = 0;
	  if (attr->getNullFlag())
	    {
	      nullVal = *(short*)&tuppRow[attr->getNullIndOffset()];

              if (hbaseAccessTdb().hbaseMapTable())
                prependNullVal = FALSE;
	      else if ((attr->getDefaultClass() != Attributes::DEFAULT_NULL) &&
                       (nullVal))
		prependNullVal = TRUE;
	      else if (isUpdate && nullVal)
		prependNullVal = TRUE;
	      else if (! nullVal)
		prependNullVal = TRUE;

	      if ((NOT prependNullVal) && (nullVal))
		goto label_end1;
	    }
          if (prependNullVal)
          {
             nullValChar = 0;
             if (nullVal)
                nullValChar = -1;
          }
     	  colValLen =  attr->getLength(&tuppRow[attr->getVCLenIndOffset()]);
          Int32 bytesCopied = copyColToDirectBuffer(rowCurPtr, colName, 
                colNameLen, prependNullVal, nullValChar, colVal, colValLen);
          rowCurPtr += bytesCopied;
          row_.len += bytesCopied;
          numCols++;
        }	  
      else
	{
	  ex_assert(false, "Unable to obtain column descriptor");
	}
    label_end1:		  
      listOfColNames->advance();
    }	// for
  *numColsPtr = bswap_16(numCols);

  if (includeInSample)
    {
      // Overwrite trailing delimiter with newline.
      hiveBuff[hiveBuffInx-1] = '\n';
      HDFS_Client_RetCode hdfsClientRetcode;
      sampleFileHdfsClient()->hdfsWrite(hiveBuff, hiveBuffInx, hdfsClientRetcode);
    }
  return 0;
}

short ExHbaseAccessTcb::createDirectAlignedRowBuffer( UInt16 tuppIndex, 
                                                      char * tuppRow,
                                                      Queue * listOfColNames, 
                                                      NABoolean isUpdate,
                                                      std::vector<UInt32> * posVec )
{
  
  ExpTupleDesc * rowTD =
    hbaseAccessTdb().workCriDesc_->getTupleDescriptor
    (tuppIndex);
  
  short colNameLen;
  char * colName;
  short nullVal = 0;
  short nullValLen = 0;
  Int32 colValLen;
  char *colVal;
  char *str;
  short * numColsPtr;

  //  allocateDirectRowBufferForJNI(rowTD->numAttrs());
  allocateDirectRowBufferForJNI(1);

  BYTE *rowCurPtr = (BYTE *)row_.val;
  numColsPtr = (short *)rowCurPtr;
  row_.len += sizeof(short);
  rowCurPtr += sizeof(short);
  listOfColNames->position();
  
  extractColNameFields((char*)listOfColNames->getCurr(),
                       colNameLen, colName);
  
  colVal = tuppRow; 
  
  colValLen =  insertRowlen_;
  Int32 bytesCopied = 
    copyColToDirectBuffer(rowCurPtr, colName, colNameLen, 
                          FALSE, 0,
                          colVal, colValLen);
  rowCurPtr += bytesCopied;
  row_.len += bytesCopied;
  
  *numColsPtr = bswap_16(1);

  return 0;
}

short ExHbaseAccessTcb::createDirectRowwiseBuffer(char * inputRow)
{
  short numEntries;
  short colNameLen;
  Int32 colValLen;
  short nullVal;
  char *colName;
  char *colVal;
  Int32 bytesCopied;
  char nullValChar = 0;
  Int32 rowLen;
  char *curPtr;
  short maxColNameLen;
  short colValVCIndLen;
  Int32 maxColValLen;

  curPtr = inputRow;
  numEntries = *((short *)curPtr);
  curPtr += sizeof(short);
  maxColNameLen = *((short *)curPtr);
  curPtr += sizeof(short);
  colValVCIndLen = *((short *)curPtr);
  curPtr += sizeof(short);
  maxColValLen = *((Int32 *)curPtr);
  rowLen = sizeof(short) + // For row number
           sizeof(short) // For number of columns
      + (numEntries * (ROUND2(maxColNameLen)+ ROUND2(maxColValLen)
                       + (sizeof(short) +  sizeof(Int32)))); // Store colNameLen and colValLen
  short numCols = 0;
  short *numColsPtr ;
  allocateDirectBufferForJNI(rowLen);
  BYTE *rowCurPtr = (BYTE *)row_.val;
  numColsPtr = (short *)rowCurPtr;
  rowCurPtr += sizeof(short); // To store numCols later
  row_.len += sizeof(short); 
  
  curPtr = inputRow; 
  curPtr += (3*sizeof(short) + sizeof(Int32)); // skip numEntries, maxColNameLen, 
                                                                // colValVCIndLen, maxColValLen
  for (Lng32 ij = 0; ij < numEntries; ij++)
  {
      colNameLen = *(short*)curPtr;
      curPtr += sizeof(short);
      colName = curPtr;
      curPtr += ROUND2(maxColNameLen);
      
      nullVal = *(short*)curPtr;
      curPtr += sizeof(short);

      if (colValVCIndLen == sizeof(short))
        {
          colValLen = *(short*)curPtr;
          curPtr += sizeof(short);
        }
      else
       {
         curPtr = (char*)ROUND4((Int64)curPtr);
         colValLen = *(Int32*)curPtr;
         curPtr += sizeof(Int32);
       }

      colVal = curPtr;
      curPtr += ROUND2(maxColValLen);

      if (! nullVal)
      {
         bytesCopied = copyColToDirectBuffer(rowCurPtr, colName,
              colNameLen, FALSE, nullValChar, colVal, colValLen);
         rowCurPtr += bytesCopied;
         row_.len += bytesCopied;
         numCols++;
      }
   }
  *numColsPtr = bswap_16(numCols);
   return 0;
} 

short ExHbaseAccessTcb::setupHbaseFilterPreds()
{
  if ((!hbaseAccessTdb().listOfHbaseFilterColNames()) ||
      (hbaseAccessTdb().listOfHbaseFilterColNames()->numEntries() == 0))
    return 0;

  if (hbaseFilterValExpr()){// with pushdown V2 it can be null if we have only unary operation
          ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();

          workAtp_->getTupp(hbaseAccessTdb().hbaseFilterValTuppIndex_)
            .setDataPointer(hbaseFilterValRow_);

          ex_expr::exp_return_type evalRetCode =
            hbaseFilterValExpr()->eval(pentry_down->getAtp(), workAtp_);
          if (evalRetCode == ex_expr::EXPR_ERROR)
            {
              return -1;
            }

          ExpTupleDesc * hfrTD =
            hbaseAccessTdb().workCriDesc_->getTupleDescriptor
            (hbaseAccessTdb().hbaseFilterValTuppIndex_);

          hbaseFilterValues_.clear();
          //for each evaluated value, populate the corresponding hBaseFilterValue
          for (Lng32 i = 0; i <  hfrTD->numAttrs(); i++)
          {
              Attributes * attr = hfrTD->getAttr(i);

            if (attr)
                {
                  NAString value(getHeap());
                  if (attr->getNullFlag())
                    {
                      char nullValChar = 0;

                      short nullVal = *(short*)&hbaseFilterValRow_[attr->getNullIndOffset()];

                      if (nullVal)
                          nullValChar = -1;
                      value.append((char*)&nullValChar, sizeof(char));
                    }

                  char * colVal = &hbaseFilterValRow_[attr->getOffset()];

                  value.append(colVal,
                           attr->getLength(&hbaseFilterValRow_[attr->getVCLenIndOffset()]));

                  hbaseFilterValues_.insert(value);
                }
            }
  }
  setupListOfColNames(hbaseAccessTdb().listOfHbaseFilterColNames(),
		      hbaseFilterColumns_);

  hbaseFilterOps_.clear();
  hbaseAccessTdb().listOfHbaseCompareOps()->position();
  while (NOT hbaseAccessTdb().listOfHbaseCompareOps()->atEnd())
    {
      char * op = (char*)hbaseAccessTdb().listOfHbaseCompareOps()->getCurr();

      hbaseFilterOps_.insert(op);
      hbaseAccessTdb().listOfHbaseCompareOps()->advance();
    }

  return 0;
}

void ExHbaseAccessTcb::setRowID(char *rowId, Lng32 rowIdLen)
{
   if (rowId == NULL)
   {
      rowID_.val = NULL;
      rowID_.len = 0;
      return;
   }
   if (rowIdLen > rowIDAllocatedLen_) 
   {
      if (rowIDAllocatedVal_)
      {
         NADELETEBASIC(rowIDAllocatedVal_, getHeap());
      }
      rowIDAllocatedVal_ = new (getHeap()) char[rowIdLen];
      rowIDAllocatedLen_ = rowIdLen;

   }
   rowID_.val = rowIDAllocatedVal_;
   rowID_.len = rowIdLen;
   memcpy(rowID_.val, rowId, rowIdLen);
}

void ExHbaseAccessTcb::buildLoggingFileName(NAHeap *heap,
                             const char * currCmdLoggingLocation,
                             const char *tableName,
                             const char * loggingFileNamePrefix,
                             Lng32 instId,
                             char *&loggingFileName)
{
  if (loggingFileName != NULL)
     NADELETEBASIC(loggingFileName, heap);
  loggingFileName = NULL;
  if (currCmdLoggingLocation == NULL)
     return;
  short logLen = strlen(currCmdLoggingLocation)+strlen(loggingFileNamePrefix)+strlen(tableName)+100;
  loggingFileName = new (heap) char[logLen];
  sprintf(loggingFileName, "%s/%s_%s_%d",
                  currCmdLoggingLocation, loggingFileNamePrefix, tableName, instId);
}

void ExHbaseAccessTcb::buildLoggingPath(
                             const char *loggingLocation,
                             char * logId,
                             const char * tableName,
                             char *currCmdLoggingLocation)
{
  time_t t;
  char logId_tmp[30];

  if (logId == NULL || strlen(logId) == 0)
  {
     time(&t);
     struct tm * curgmtime = gmtime(&t);
     strftime(logId_tmp, sizeof(logId_tmp)-1, "%Y%m%d_%H%M%S", curgmtime);
     logId = logId_tmp;
  } 
  sprintf(currCmdLoggingLocation, "%s/ERR_%s_%s",
                  loggingLocation, tableName, logId);
}

void ExHbaseAccessTcb::handleException(NAHeap *heap,
                                    char *logErrorRow,
                                    Lng32 logErrorRowLen,
                                    ComCondition *errorCond)
{
  Lng32 errorMsgLen = 0;
  charBuf *cBuf = NULL;
  char *errorMsg;
  HDFS_Client_RetCode hdfsClientRetcode;

  if (loggingErrorDiags_ != NULL)
     return;

  if (!loggingFileCreated_) {
     logFileHdfsClient_ = HdfsClient::newInstance((NAHeap *)getHeap(), NULL, hdfsClientRetcode);
     if (hdfsClientRetcode == HDFS_CLIENT_OK)
        hdfsClientRetcode = logFileHdfsClient_->hdfsCreate(loggingFileName_, TRUE, FALSE, FALSE);
     if (hdfsClientRetcode == HDFS_CLIENT_OK)
        loggingFileCreated_ = TRUE;
     else 
        goto logErrorReturn;
  }
  
  logFileHdfsClient_->hdfsWrite(logErrorRow, logErrorRowLen, hdfsClientRetcode);
  if (hdfsClientRetcode != HDFS_CLIENT_OK) 
     goto logErrorReturn;
  if (errorCond != NULL) {
     errorMsgLen = errorCond->getMessageLength();
     const NAWcharBuf wBuf((NAWchar*)errorCond->getMessageText(), errorMsgLen, heap);
     cBuf = unicodeToISO88591(wBuf, heap, cBuf);
     errorMsg = (char *)cBuf->data();
     errorMsgLen = cBuf -> getStrLen();
     errorMsg[errorMsgLen]='\n';
     errorMsgLen++;
  }
  else {
     errorMsg = (char *)"[UNKNOWN EXCEPTION]\n";
     errorMsgLen = strlen(errorMsg);
  }
  logFileHdfsClient_->hdfsWrite(errorMsg, errorMsgLen, hdfsClientRetcode);
logErrorReturn:
  if (hdfsClientRetcode != HDFS_CLIENT_OK) {
     loggingErrorDiags_ = ComDiagsArea::allocate(heap);
     *loggingErrorDiags_ << DgSqlCode(EXE_ERROR_WHILE_LOGGING)
                 << DgString0(loggingFileName_)
                 << DgString1((char *)GetCliGlobals()->getJniErrorStr());
  }
  return;
}

void ExHbaseAccessTcb::incrErrorCount( ExpHbaseInterface * ehi,Int64 & totalExceptionCount,
                                    const char * tabName, const char * rowId )
{
  Lng32 retcode;
  retcode = ehi->incrCounter(tabName, rowId, (const char*)"ERRORS",(const char*)"ERROR_COUNT",1, totalExceptionCount);
}

void ExHbaseAccessTcb::getErrorCount( ExpHbaseInterface * ehi,Int64 & totalExceptionCount,
                                    const char * tabName, const char * rowId )
{
  Lng32 retcode;
  retcode = ehi->incrCounter(tabName, rowId, (const char*)"ERRORS",(const char*)"ERROR_COUNT",0, totalExceptionCount);
}

int ExHbaseAccessTcb::compareRowIds()
{
   UInt32 rowIdLen = hbaseAccessTdb().getRowIDLen();
   if (beginRowId_.size() == 0)
      return 0;
   if (endRowId_.size() == 0)
      return 0;
   return endRowId_.compare(0, rowIdLen, beginRowId_);
}

static const char * const BatchSizeEnvvar = 
  getenv("SQL_CANCEL_BATCH_SIZE");
static const Lng32 BatchSize = (BatchSizeEnvvar &&
                               atoi(BatchSizeEnvvar) > 0 )?
                               atoi(BatchSizeEnvvar) : 8192;

ExHbaseTaskTcb::ExHbaseTaskTcb(ExHbaseAccessTcb * tcb, NABoolean rowsetTcb)
  : tcb_(tcb)
  , batchSize_(BatchSize)
  , rowsetTcb_(rowsetTcb)
{}

ExWorkProcRetcode ExHbaseTaskTcb::work(short &rc)
{
  ex_assert(0, "Should not reach ExHbaseTaskTcb::work()");

  return WORK_OK;
}

ExHbaseAccessBulkLoadTaskTcb::ExHbaseAccessBulkLoadTaskTcb(const ExHbaseAccessTdb &hbaseAccessTdb, ex_globals * glob) :
    ExHbaseAccessTcb(hbaseAccessTdb, glob), step_(NOT_STARTED)
{
}

ExWorkProcRetcode ExHbaseAccessBulkLoadTaskTcb::work()
{
  short retcode = 0;
  short rc = 0;
  Queue * indexList = NULL; // this list includes the base table too.
  char * indexName ;
  NABoolean cleanupAfterComplete = FALSE;
  Text tabName;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  while (1)
  {
    switch (step_)
    {
      case NOT_STARTED:
      {
        matches_ = 0;

        retcode = ehi_->initHBLC();
        if (setupError(retcode, "ExpHbaseInterface::initHBLC"))
        {
          step_ = HANDLE_ERROR_AND_CLOSE;
          break;
        }

        indexList = hbaseAccessTdb().listOfIndexesAndTable();
        if (!indexList) {
          if (setupError(-1, "listOfIndexesAndTable is empty"))
            {
              step_ = HANDLE_ERROR_AND_CLOSE;
              break;
            }
        }
          
        indexList->position();
        indexName = NULL;
        cleanupAfterComplete = FALSE;
        step_ = GET_NAME;
      }
      break;

      case GET_NAME:
      {
        char * indexName = (char*)indexList->getNext();
        table_.val = indexName;
        table_.len = strlen(indexName);
        hBulkLoadPrepPath_ = std::string(((ExHbaseAccessTdb&) hbaseAccessTdb()).getLoadPrepLocation())
          + indexName ;
        tabName = indexName;

        if (((ExHbaseAccessTdb&) hbaseAccessTdb()).getIsTrafLoadCleanup() || cleanupAfterComplete )
          step_ = LOAD_CLEANUP;
        else
          step_ = COMPLETE_LOAD;
      }
      break;

      case LOAD_CLEANUP:
      {
        //cleanup
        retcode = ehi_->bulkLoadCleanup(table_, hBulkLoadPrepPath_);

        if (setupError(retcode, "ExpHbaseInterface::bulkLoadCleanup"))
        {
          step_ = HANDLE_ERROR_AND_CLOSE;
          break;
        }
        if (!indexList->atEnd())
          step_ = GET_NAME;
        else
          step_ = LOAD_CLOSE;
      }
        break;

      case COMPLETE_LOAD:
      {
        retcode = ehi_->doBulkLoad(table_,
                                   hBulkLoadPrepPath_,
                                   tabName,
                                   hbaseAccessTdb().getUseQuasiSecure(),
                                   hbaseAccessTdb().getTakeSnapshot());

        if (setupError(retcode, "ExpHbaseInterface::doBulkLoad"))
        {
          step_ = HANDLE_ERROR_AND_CLOSE;
          break;
        }

        if (((ExHbaseAccessTdb&) hbaseAccessTdb()).getIsTrafLoadKeepHFiles())
        {
          if (!indexList->atEnd())
            step_ = GET_NAME;
          else
            step_ = LOAD_CLOSE;
          break;
        }
        if (!indexList->atEnd())
          step_ = GET_NAME;
        else {
          indexList->position();
          cleanupAfterComplete = TRUE;
          step_ = GET_NAME;
        }
      }
        break;

      case LOAD_CLOSE:
      {
        retcode = ehi_->close();
        if (setupError(retcode, "ExpHbaseInterface::close"))
        {
          step_ = HANDLE_ERROR;
          break;
        }
        step_ = DONE;
      }
      break;

      case HANDLE_ERROR:
      case HANDLE_ERROR_AND_CLOSE:
      {
        if (handleError(rc))
          return rc;
        if (step_==HANDLE_ERROR_AND_CLOSE)
        {
          step_ = LOAD_CLOSE;
          break;
        }
        step_ = DONE;
      }
        break;

      case DONE:
      {
        if (handleDone(rc))
          return rc;

        step_ = NOT_STARTED;
        retcode = ehi_->close();

        return WORK_OK;
      }
        break;

    } // switch

  } // while
}
