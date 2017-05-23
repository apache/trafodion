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
/*
******************************************************************************
*
* File:         ex_hash_grby.c
* Description:  Methods for the tdb and tcb of hash aggregate/grby operator
*               
*               
* Created:      5/3/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

//
// This file contains all the generator and executor methods associated
// with a hash aggr/grby operation.
//

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_hash_grby.h"
#include "ex_expr.h"
#include "ExSimpleSqlBuffer.h"
#include "ExTrieTable.h"
#include "ExBitMapTable.h"
#include "ExStats.h"
#include "ex_error.h"
#include "ex_exe_stmt_globals.h"
#include "memorymonitor.h"
#include "logmxevent.h"

#include "sql_buffer_size.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////

// In the reverse order of ex_hash_grbytcb::HashGrbyPhase
// Limit to 11 characters
const char *ex_hash_grby_tcb::HashGrbyPhaseStr[] = {
    "PHASE_END",
    "READ_SPILL",
    "RETURN_ROWS",
    "READ_ROWS"
}; 

NABoolean ex_hash_grby_tcb::needStatsEntry()
{
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  // stats are collected for ALL and MEASURE options.
  if (statsType == ComTdb::ALL_STATS || statsType == ComTdb::OPERATOR_STATS)
    return TRUE;
  else
    return FALSE;
}

ExOperStats * ex_hash_grby_tcb::doAllocateStatsEntry(CollHeap *heap,
                                                     ComTdb *tdb)
{
  ExBMOStats *stat;
  ComTdbHashGrby *comTdbHashGrby = (ComTdbHashGrby *)tdb;
  if (comTdbHashGrby->isNonBMOPartialGroupBy()) 
     return ex_tcb::doAllocateStatsEntry(heap, tdb); 
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
    stat =  new (heap) ExBMOStats(heap, this, tdb);
  else
  {
    stat = (ExBMOStats *)new(heap) ExHashGroupByStats(heap,
				      this,
                                     tdb);
    hashGroupByStats_ = (ExHashGroupByStats *)stat;
  }
  bmoStats_ = stat;
  return stat;
}

//
// Build an hash_grby tcb
//
ex_tcb * ex_hash_grby_tdb::build(ex_globals * glob) {
  // first build the child
  ex_tcb           * childTcb;
  ex_hash_grby_tcb * hash_grby_tcb;
  
  childTcb = childTdb_->build(glob);
  
  hash_grby_tcb = new(glob->getSpace())
    ex_hash_grby_tcb(*this, *childTcb, glob);
  
  // add the hash_grby_tcb to the schedule
  hash_grby_tcb->registerSubtasks();

  // the parent queues will be resizable, so register a resize subtask.
  hash_grby_tcb->registerResizeSubtasks();

  return (hash_grby_tcb);
};


//////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
//////////////////////////////////////////////////////////////////////////////

//
// Constructor for hash_grby_tcb
//
ex_hash_grby_tcb::ex_hash_grby_tcb(const ex_hash_grby_tdb &  hash_grby_tdb, 
				   const ex_tcb &    childTcb,
				   ex_globals * glob)
  : ex_tcb( hash_grby_tdb, 1, glob),
    space_(glob->getSpace()),
    childTcb_(&childTcb),
    bucketCount_(0),
    buckets_(NULL),
    rCluster_(NULL),
    ofClusterList_(NULL),
    lastOfCluster_(NULL),
    clusterDb_(NULL),
    matchCount_(0),
    partialGroupbyMissCounter_(0),
    state_(HASH_GRBY_EMPTY),
    oldState_(HASH_GRBY_EMPTY),
    readingChild_(FALSE),
    hasFreeTupp_(FALSE),
    hashValue_(0),
    rc_(EXE_OK),
    ioEventHandler_(NULL),
    workAtp_(NULL),
    tempUpAtp_(NULL),
    resultPool_(NULL),
    bitMuxTable_(NULL),
    bitMuxBuffer_(NULL),
    numIOChecks_(0),
    defragTd_(NULL)
{ 
  bmoStats_ = NULL;
  hashGroupByStats_ = NULL;
#ifndef __EID
  heap_ = new (glob->getDefaultHeap()) NAHeap("Hash Groupby Heap", (NAHeap *)glob->getDefaultHeap());

  // set the memory monitor
  memMonitor_ = getGlobals()->castToExExeStmtGlobals()->getMemoryMonitor();
#else
  heap_ = glob->getDefaultHeap();
#endif
  
  // Copy all expression pointers. This must be done first because 
  // some of the pointers are used subsequently (i.e. bitMuxExpr_) in
  // branch conditions before fixup.
  //
  hashExpr_ = hash_grby_tdb.hashExpr_;
  bitMuxExpr_ = hash_grby_tdb.bitMuxExpr_;
  bitMuxAggrExpr_ = hash_grby_tdb.bitMuxAggrExpr_;
  hbMoveInExpr_ = hash_grby_tdb.hbMoveInExpr_;
  ofMoveInExpr_ = hash_grby_tdb.ofMoveInExpr_;
  resMoveInExpr_ = hash_grby_tdb.resMoveInExpr_;

  hbAggrExpr_ = hash_grby_tdb.hbAggrExpr();
  ofAggrExpr_ = hash_grby_tdb.ofAggrExpr();
  resAggrExpr_ = hash_grby_tdb.resAggrExpr();

  havingExpr_ = hash_grby_tdb.havingExpr_;
  moveOutExpr_ = hash_grby_tdb.moveOutExpr_;
  hbSearchExpr_ = hash_grby_tdb.hbSearchExpr_;
  ofSearchExpr_ = hash_grby_tdb.ofSearchExpr_;
  
  // Ditto for Atp Indexes.
  //
  hbRowAtpIndex_ = hash_grby_tdb.hbRowAtpIndex_;
  ofRowAtpIndex_ = hash_grby_tdb.ofRowAtpIndex_;
  hashValueAtpIndex_ = hash_grby_tdb.hashValueAtpIndex_;
  bitMuxAtpIndex_ = hash_grby_tdb.bitMuxAtpIndex_;
  bitMuxCountOffset_ = hash_grby_tdb.bitMuxCountOffset_;
  resultRowAtpIndex_ = hash_grby_tdb.resultRowAtpIndex_;
  returnedAtpIndex_ = hash_grby_tdb.returnedAtpIndex_;
  
  // allocate the buffer for the result rows. This is 
  // controlled by GEN_HGBY_NUM_BUFFERS CQD at compile time.
  Lng32 numBuffers = ((hash_grby_tdb.numBuffers_ == 0) ? 5 : hash_grby_tdb.numBuffers_);

  resultPool_ = new(space_)
    sql_buffer_pool(numBuffers,
                    (Int32)hash_grby_tdb.bufferSize_,
                    space_);

  if (hash_grby_tdb.considerBufferDefrag())
  {
    assert(hash_grby_tdb.useVariableLength());
    defragTd_ = resultPool_->addDefragTuppDescriptor(hashGrbyTdb().resultRowLength_);
  }
  // Allocate an ExTrieTable for rows that map directly to groups using
  // bitMux fast mapping.
  // LCOV_EXCL_START  

  if (bitMuxExpr_) {
    // Try to get about 1Mb of memory for the Trie table.
    Int32 memSize = 1000 * 1024;
    bitMuxTable_ = new(space_) 
      ExBitMapTable((Int32) hashGrbyTdb().keyLength_,
		    (Int32) hashGrbyTdb().extGroupedRowLength_,
		    (Int32) bitMuxCountOffset_,
		    memSize, 
		    space_);
    bitMuxBuffer_ = (char *)space_->allocateMemory
      ((size_t) hashGrbyTdb().extGroupedRowLength_);
    
    if (bitMuxTable_ && bitMuxBuffer_ &&
	(bitMuxTable_->getMaximumNumberGroups() > 0)) {
      // Initialize the bitMuxBuffer_ -- this must be done for now because
      // the actual key extracted by the bitMux expression may be shorter
      // that the grouped row length.
      //
      for (Int32 i=0; i<(Int32)hashGrbyTdb().extGroupedRowLength_; i++)
	bitMuxBuffer_[i] = 0;
      
      // Initialize the tupp descriptor for the bitMux buffer if
      // bitMux'ing is on.
      //
      bitMuxTupp_.init((Int32)hashGrbyTdb().extGroupedRowLength_, 
		       NULL, 
		       bitMuxBuffer_);
    }
    else {
      // If resource allocation fails for any reason, simply turn off 
      // bitMux'ing and free any of the resources that where allocated.
      //
      if (bitMuxTable_) {
	delete bitMuxTable_;
	bitMuxTable_ = 0;
      }
      
      if (bitMuxBuffer_) {
	space_->deallocateMemory(bitMuxBuffer_);
	bitMuxBuffer_ = 0;
      }
      bitMuxExpr_ = 0;
    }
  }

  // For performance testing.
  //
  if(bitMuxExpr_ && !hbAggrExpr_ && !hashExpr_)
    bitMuxExpr_ = NULL;
  // LCOV_EXCL_STOP

  // get the queue that child use to communicate with me
  childQueue_  = childTcb_->getParentQueue(); 
  
  // Allocate the queues to communicate with parent (no pstate needed)
  allocateParentQueues(parentQueue_,FALSE);

  // Now, fixup the expressions     // LCOV_EXCL_START
  //
  if (hashExpr_)
    (void) hashExpr_->fixup(0, getExpressionMode(), this,
			    space_, heap_, glob->computeSpace(), glob);
  if (bitMuxExpr_) {
    (void) bitMuxExpr_->fixup(0, getExpressionMode(), this, 
			      space_, heap_, glob->computeSpace(), glob);
    if (bitMuxAggrExpr_)
      (void) bitMuxAggrExpr_->fixup(0, getExpressionMode(), this,
				    space_, heap_, glob->computeSpace(), glob);
  }
  if (hbMoveInExpr_)
    (void) hbMoveInExpr_->fixup(0, getExpressionMode(), this,
				space_, heap_, glob->computeSpace(), glob);
  if (ofMoveInExpr_)
    (void) ofMoveInExpr_->fixup(0, getExpressionMode(), this,
				space_, heap_, glob->computeSpace(), glob);
  if (resMoveInExpr_)
    (void) resMoveInExpr_->fixup(0, getExpressionMode(), this,
				 space_, heap_, glob->computeSpace(), glob);
  if (hbAggrExpr_)
    (void) hbAggrExpr_->fixup(0, getExpressionMode(), this,
			      space_, heap_, glob->computeSpace(), glob);
  if (ofAggrExpr_)
    (void) ofAggrExpr_->fixup(0, getExpressionMode(), this,
			      space_, heap_, glob->computeSpace(), glob);
  if (resAggrExpr_)
    (void) resAggrExpr_->fixup(0, getExpressionMode(), this,
			       space_, heap_, glob->computeSpace(), glob);
  if (havingExpr_)
    (void) havingExpr_->fixup(0, getExpressionMode(), this,
			      space_, heap_, glob->computeSpace(), glob);
  if (moveOutExpr_)
    (void) moveOutExpr_->fixup(0, getExpressionMode(), this,
			       space_, heap_, glob->computeSpace(), glob);
  if (hbSearchExpr_)
    (void) hbSearchExpr_->fixup(0, getExpressionMode(), this,
				space_, heap_, glob->computeSpace(), glob);
  if (ofSearchExpr_)
    (void) ofSearchExpr_->fixup(0, getExpressionMode(), this,
				space_, heap_, glob->computeSpace(), glob);
  // LCOV_EXCL_STOP
  // Allocate the ATP's that are used internally in the work
  // methods.
  //
  workAtp_ = allocateAtp(hash_grby_tdb.workCriDesc_, space_);
  tempUpAtp_ = allocateAtp(hash_grby_tdb.criDescUp_, space_);
  
  // Initalize the tupp descriptor for the hash value.
  //
  hashValueTupp_.init(sizeof(SimpleHashValue),
		      NULL,
		      (char *) (&hashValue_));
  
  // Allocate tupp descriptors for tupps in the workAtp. tupps 0 and 1
  // are used for constants and temps. Don't allocate them here.
  // The hashValue and bitMux tupps are part of the TCB so don't allocate
  // them either.
  // I.e., just allocate the tupp descriptors for the rows in the has
  // buffer
  
  workAtp_->getTupp(hash_grby_tdb.hbRowAtpIndex_) =
    new(space_) tupp_descriptor;
  workAtp_->getTupp(hash_grby_tdb.ofRowAtpIndex_) =
    new(space_) tupp_descriptor;
  
  // Set the hashValue and bitMux tupp's
  //
  workAtp_->getTupp(hash_grby_tdb.hashValueAtpIndex_) = &hashValueTupp_;
  workAtp_->getTupp(hash_grby_tdb.bitMuxAtpIndex_) = &bitMuxTupp_;
};

///////////////////////////////////////////////////////////////////////////////
// Destructor for hash_grby_tcb
//  // LCOV_EXCL_START
ex_hash_grby_tcb::~ex_hash_grby_tcb() {

  delete parentQueue_.up;
  delete parentQueue_.down;
  freeResources();
  delete heap_;
}; 

//////////////////////////////////////////////////////////////////////////
// Free Resources
//
void ex_hash_grby_tcb::freeResources() {
  
  if (workAtp_) {
    deallocateAtp(workAtp_, space_);
    workAtp_ = NULL;
  };

  if (tempUpAtp_) {
    deallocateAtp(tempUpAtp_, space_);
    tempUpAtp_ = NULL;
  };

  while (ofClusterList_) {
    Cluster * p = ofClusterList_->getNext();
    delete ofClusterList_;
    ofClusterList_ = p;
  };
  lastOfCluster_ = NULL;


  // remove all clusters
  if (clusterDb_) {
    delete clusterDb_;
    clusterDb_ = NULL;
  };

  if (resultPool_)
    delete resultPool_;

  resultPool_ = NULL;

  if(bitMuxTable_) delete bitMuxTable_;
  bitMuxTable_ = NULL;

  if(bitMuxBuffer_) NADELETEBASIC(bitMuxBuffer_, space_);
  bitMuxBuffer_ = NULL;
};
// LCOV_EXCL_STOP  

void ex_hash_grby_tcb::registerSubtasks()
{
    ExScheduler *sched = getGlobals()->getScheduler();
    ex_tcb :: registerSubtasks();
    // Regsiter the I/O event
    ioEventHandler_ =  sched->registerNonQueueSubtask(sWork,this);  
};

///////////////////////////////////////////////////////////////////////////////
// work: this is where the action is.
///////////////////////////////////////////////////////////////////////////////
short ex_hash_grby_tcb::work() {
  // if no parent request, return
  if (parentQueue_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();
  const ex_queue::down_request & request =
    downParentEntry->downState.request;
 
  // loop forever; exit via return
  while (TRUE) {

    // Cancel request if parent cancelled and not already cancelled.
    if ( state_ != HASH_GRBY_CANCELED && state_ != HASH_GRBY_DONE && 
	 request == ex_queue::GET_NOMORE )
      setState(HASH_GRBY_CANCELED); 

    switch(state_) {
    case HASH_GRBY_EMPTY: {
      workInitialize();
    } break;

    case HASH_GRBY_READ_CHILD: {
      if (childQueue_.up->isEmpty())
	    return WORK_OK;

      if (parentQueue_.up->isFull())
	    return WORK_OK;

      // in case of a partial hash grouping we might generate results
      // even if we are in the read phase. Thus, we have to make sure that
      // we have a free entry in the parent queue. And we have to allocate
      // space in the result buffer
      //   ditto for no-aggregates (return results during workReadChild() )
      if (hashGrbyTdb().isPartialGroup_ || ! hbAggrExpr_ ) {
    	if (!hasFreeTupp_) {
	      // allocate space for the result tuple
          if (resultPool_->get_free_tuple(workAtp_->getTupp(resultRowAtpIndex_),
                                          (Int32) hashGrbyTdb().resultRowLength_))

            // not enough space in pool. Return for now
    	    return WORK_POOL_BLOCKED;
          else
            hasFreeTupp_ = TRUE;
    	};
      };
      if (bmoStats_)
         bmoStats_->setBmoPhase(PHASE_END-HGB_READ_PHASE);

      // now read the next row from the child queue
      workReadChild();
    } break;

#ifndef __EID

    case HASH_GRBY_READ_OF_ROW: {
      if (parentQueue_.up->isFull())
	    return WORK_OK;

      // In case of no-aggregates (return results during workReadOFRow() )
      // we may return up a row. Thus, we have to make sure that
      // we have a free entry in the parent queue. And we have to allocate
      // space in the result buffer
      if ( ! hbAggrExpr_ && ! hasFreeTupp_ ) {
	// allocate space for the result tuple
	if (resultPool_->get_free_tuple(workAtp_->getTupp(resultRowAtpIndex_),
                                        (Int32) hashGrbyTdb().resultRowLength_))
	  // not enough space in pool. Return for now
	  return WORK_POOL_BLOCKED;
	else
	  hasFreeTupp_ = TRUE;
      };

      workReadOverFlowRow();
    } break;

    case HASH_GRBY_SPILL: {
      workSpill();
      if (state_ == HASH_GRBY_SPILL)
	    // I/O is not done yet
            return WORK_CALL_AGAIN;
    } break;

    case HASH_GRBY_READ_BUFFER: {
       if (bmoStats_ && (bmoStats_->getBmoPhase() == (PHASE_END-HGB_READ_PHASE)))
         bmoStats_->setBmoPhase(PHASE_END-HGB_READ_SPILL_PHASE);

      workReadBuffer();
      if (state_ == HASH_GRBY_READ_BUFFER)
	// I/O is not done yet
        return WORK_CALL_AGAIN;
    } break;

#endif

    case HASH_GRBY_EVALUATE: {
      workEvaluate();
    } break;

    case HASH_GRBY_RETURN_PARTIAL_GROUPS:
    case HASH_GRBY_RETURN_BITMUX_ROWS:
    case HASH_GRBY_RETURN_ROWS: {

      ex_assert ( hbAggrExpr_ , "DISTINCT rows must be returned immediately");

      // if we have given to the parent all the rows needed,
      // we are done
      if ((request == ex_queue::GET_N) &&
	  (downParentEntry->downState.requestValue <= (Lng32)matchCount_))
	setState(HASH_GRBY_DONE);
      else {

	// make sure that we have space in the up queue
	if (parentQueue_.up->isFull())
	  return WORK_OK;
        NABoolean tryToDefrag = FALSE;
	if (!hasFreeTupp_)
	{
          //if havingExpr_ is not null then we can preallocate the actual row size
          // and the row get discarded from results (havingExpr_ discards it)
          // but we can't use the pre-allocated space for the next row
          if (!havingExpr_ &&
              defragTd_ &&
              !resultPool_->currentBufferHasEnoughSpace(hashGrbyTdb().resultRowLength_))
          {
            tryToDefrag = TRUE;
          }
          else
	  // allocate space for the result tuple
	  if (resultPool_->get_free_tuple(workAtp_->getTupp(resultRowAtpIndex_),
                                          (Int32) hashGrbyTdb().resultRowLength_))
	    // not enough space in pool. return for now
	    return WORK_POOL_BLOCKED;
	  else
	    hasFreeTupp_ = TRUE;
	};
  
       if (bmoStats_ && (bmoStats_->getBmoPhase() == (PHASE_END-HGB_READ_PHASE)))
         bmoStats_->setBmoPhase(PHASE_END-HGB_RETURN_PHASE);
	
	// now we are ready to return a row
	ULng32 retCode = workReturnRows(tryToDefrag);


	// if retCode = 0. Do nothing. Continue in the current state.
	// if retCode = 1 && state_ == HASH_GRBY_RETURN_BITMUX_ROWS
	//            -- set 
	// if retCode = 1 && state_ = HASH_GRBY_RETURN_PARTIAL_GROUPS
	//           -- delete the cluster (already done in workReturnRows()) 
	//           -- and re-allocate it.
	//           -- set the state_ to HASH_GRBY_READ_CHILD. 

	// if retCode = 1 && state_ = HASH_GRBY_RETURN_ROWS
	//            -- delete the cluster (already done in workReturnRows())
	//            -- and set the state_ to HASH_GRBY_EVALUATE. 
	// if retcode == 2 return WORK_POOL_BLOCKED

	if (retCode == 2)
          return WORK_POOL_BLOCKED;

	if ( (retCode == 1) && (state_ == HASH_GRBY_RETURN_BITMUX_ROWS))
	  setState(HASH_GRBY_DONE);
	    
	if ( (retCode == 1) && (state_ == HASH_GRBY_RETURN_PARTIAL_GROUPS))
	  resetClusterAndReadFromChild();
	    
	if ( (retCode == 1) && (state_ == HASH_GRBY_RETURN_ROWS))
	  setState(HASH_GRBY_EVALUATE);;

      };
    } break;

    case HASH_GRBY_DONE: {
      // make sure that we have space in the up queue
      if (parentQueue_.up->isFull())
	    return WORK_OK;
      workDone();
      if (parentQueue_.down->isEmpty())
        return WORK_OK;
      else
        return WORK_CALL_AGAIN; // need to work on more requests
    } break;

    case HASH_GRBY_CANCELED: {
      if (isReadingFromChild())
        {
          // If we are still reading from the child up queue, 
          // cancel the request.  
          queue_index parentIndex = parentQueue_.down->getHeadIndex();
          childQueue_.down->cancelRequestWithParentIndex(parentIndex);

          // Consume all rows from the child up queue.  Note that 
          // we can temporarily exhaust this queue, and that we'll
          // return to the scheduler and be re-dispatched when the 
          // child produces more rows.  That means that the call above
          // to cancelRequestWithParentIndex will be executed again,
          // but this only wastes a little CPU because the child should 
          // finish its request and return Q_NO_DATA after its next 
          // dispatch assuming that it handle's cancel properly.
          while (state_ == HASH_GRBY_CANCELED) 
            {
              if (childQueue_.up->isEmpty())
                return WORK_OK;

              ex_queue_entry * childEntry = childQueue_.up->getHeadEntry();
              ex_queue::up_status childStatus = childEntry->upState.status;

              if (childStatus == ex_queue::Q_NO_DATA)
                {
                  // When we are finished consuming all rows,
                  // change state to HASH_GRBY_DONE.
                  setState(HASH_GRBY_DONE);
                  doneReadingFromChild();
                }  

              childQueue_.up->removeHead();
            }  // while
        } // if 
      else
        {
          // On the other hand, the request to cancel might happen
          // after we've read all child up queue rows.  In that 
          // case, proceed immediately to HASH_GRBY_DONE.  
          setState(HASH_GRBY_DONE);
        }
    } break;

    case HASH_GRBY_ERROR: {   // LCOV_EXCL_START
      // make sure that we have a free slot in the parent's up queue
      if (parentQueue_.up->isFull()) {
	return WORK_OK;
      };

      // we ran into a serious runtime error. Create Condition and
      // pass it to parent. rc_ has the error code and
      // oldState_ tells us in which state the error occured
      ComDiagsArea * diags = NULL;
#ifndef __EID
      if(rc_ == EXE_SORT_ERROR)
        {
          char msg[512];
          char errorMsg[100];
          Lng32 scratchError = 0;
          Lng32 scratchSysError = 0;
          Lng32 scratchSysErrorDetail = 0;

          if(clusterDb_)
            clusterDb_->getScratchErrorDetail(scratchError,
                                   scratchSysError,
                                   scratchSysErrorDetail,
                                   errorMsg);

          str_sprintf(msg, "Hash GroupBy Scratch IO Error occurred. Scratch Error: %d, System Error: %d, System Error Detail: %d, Details: %s",
		      scratchError, scratchSysError, scratchSysErrorDetail, errorMsg);

          diags = ExRaiseSqlError(heap_, downParentEntry,
	                          (ExeErrorCode) -rc_,
                                  NULL,
                                  msg);
        }
      else
#endif
        diags = ExRaiseSqlError(heap_, downParentEntry, (ExeErrorCode)-rc_); 

      downParentEntry->setDiagsArea(diags);
      workHandleError(downParentEntry->getAtp());
    } break;
    };    // LCOV_EXCL_STOP
  };
};

/////////////////////////////////////////////////////////////////////////////
// work: initialize the data structures for the hash grouping
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workInitialize() {

  // Initialize the BitMux table if it exists.
  // 
  if(bitMuxTable_) bitMuxTable_->reset();

  if (bmoStats_)
  {
    ex_hash_grby_tdb *hashGrbyTdb = (ex_hash_grby_tdb *)getTdb();
    bmoStats_->setSpaceBufferSize(hashGrbyTdb->bufferSize_);
    bmoStats_->setSpaceBufferCount(resultPool_->get_number_of_buffers());

  }
  ex_queue_entry * parentEntry = parentQueue_.up->getTailEntry();
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();
  ex_queue_entry * childEntry = childQueue_.down->getTailEntry();

  childEntry->downState.request = ex_queue::GET_ALL;
  childEntry->downState.requestValue = 11;
  childEntry->downState.parentIndex = parentQueue_.down->getHeadIndex();
  // copy the input atp to the tempUp atp for this request
  tempUpAtp_->copyAtp(downParentEntry->getAtp());
  
  // pass the tempUp atp as child's input atp
  childEntry->passAtp(tempUpAtp_);

  matchCount_ = 0;

  childQueue_.down->insert();
  inReadingFromChild();

  // For now, we don't do cluster splits for hash grouping. Thus, we have
  // just one bucket per cluster from the beginning
  ULng32 bucketsPerCluster = 1;

  ULng32 noOfClusters = 0;

#ifndef __EID
  // We use memUsagePercent_ of the physical memory for the HGB.
  ULng32 availableMemory = memMonitor_->getPhysMemInBytes() / 100
      * hashGrbyTdb().memUsagePercent_;

  // Available memory should not exceed the quota (if set) because the initial
  // hash tables allocated (one per CHAINED cluster) may consume a large
  // portion of the available memory. (So to not exceed the quota)
  if ( hashGrbyTdb().memoryQuotaMB() > 10 && // mem quota set? (10MB, 2 B safe)
       hashGrbyTdb().memoryQuotaMB() * ONE_MEG < availableMemory ) 
    availableMemory = hashGrbyTdb().memoryQuotaMB() * ONE_MEG ; 
#else
  // in DP2 just use 10 MB
  ULng32 availableMemory = 10 * ONE_MEG;
#endif

  haveSpilled_ = FALSE;
#if !defined (__EID)
  ioTimer_.resetTimer();
#endif

  if (!hashGrbyTdb().isPartialGroup_) {

#ifndef __EID
    // size of inner table (including row headers and hash chains) in bytes
    // This may be a very large number, max out at 8 GB and take at
    // least 100 KB. Don't completely trust the optimizer ;-)
    // (With available memory usually at 100MB, that's a max of 80 clusters).
    NAFloat innerTableSizeF = hashGrbyTdb().getEstRowsUsed() *
      (NAFloat)(hashGrbyTdb().extGroupedRowLength_);
    Int64 innerTableSize;
    if (innerTableSizeF > MAX_INPUT_SIZE ) innerTableSize = MAX_INPUT_SIZE ; 
    else
      innerTableSize =
	MAXOF( MIN_INPUT_SIZE , (Int64)innerTableSizeF);

    // required number of buffers for table
    ULng32 totalResBuffers = 
      (ULng32) (innerTableSize/hashGrbyTdb().bufferSize_);

    if (!totalResBuffers)
      totalResBuffers++;

    // total number of buffers available
    ULng32 totalBuffers = availableMemory/hashGrbyTdb().bufferSize_;

    noOfClusters = totalResBuffers/totalBuffers;
    if (totalResBuffers % totalBuffers)
      noOfClusters++;

    // Aim at an average cluster's size to be a quarter of the available memory
    // to leave some slack for underestimating or data skews
    noOfClusters *= 4;

    // round it up to the nearest prime number (3,5,7,11,13,....)
    noOfClusters = ClusterDB::roundUpToPrime(noOfClusters);
    
    // the extreme case, each cluster has only one bucket and only one buffer
    ULng32 maxNoOfClusters = totalBuffers/bucketsPerCluster;
    noOfClusters = MINOF(noOfClusters, maxNoOfClusters);
#endif
  }
  else {
    // partial group by. we need only one cluster. If this cluster is full
    // (we can't allocate additional buffers), we return the incomming row
    // to the parent.
    noOfClusters = 1;

    // Memory to be used for pHGB.
    //
#ifndef __EID

    // For Partial Groupby in ESP, use a limited amount of memory
    // Based on CQD EXE_MEMORY_FOR_PARTIALHGB_MB.
    // Default to 100 MB, minimum setting 10 MB
    //
    availableMemory = hashGrbyTdb().partialGrbyMemoryMB() * ONE_MEG ; 
    if(availableMemory == 0) {   // LCOV_EXCL_START
      availableMemory = 100 * ONE_MEG; 
    } else if(availableMemory < 10 * ONE_MEG) {
      availableMemory = 10 * ONE_MEG;  
    } // LCOV_EXCL_STOP  
#endif
    
    // reset the counter for every run.
    partialGroupbyMissCounter_ = 0;
  };
  
  // total number of buckets
  bucketCount_ = bucketsPerCluster * noOfClusters;

  // allocate the buckets
  buckets_ = (Bucket *) heap_->allocateMemory(bucketCount_ * sizeof(Bucket));

  ULng32 bucketIdx = 0;
  for (; bucketIdx < bucketCount_; bucketIdx++)
    buckets_[bucketIdx].init();


  // if multiple inputs then don't yield memory below the original quota
  ULng32 minMemQuotaMB = hashGrbyTdb().isPossibleMultipleCalls() ?
    hashGrbyTdb().memoryQuotaMB() : 0 ;

  ULng32 minB4Chk = hashGrbyTdb().getBmoMinMemBeforePressureCheck() * ONE_MEG;

  clusterDb_ = new(heap_) ClusterDB(ClusterDB::HASH_GROUP_BY,
				    hashGrbyTdb().bufferSize_,
				    workAtp_,
                                    hashGrbyTdb().getExplainNodeId(),
				    0,       // not used for HGB
				    0,       // not used for HGB
				    NULL,    // not used for HGB
				    buckets_,
				    bucketCount_,
				    availableMemory,
#ifndef __EID
				    memMonitor_,
				    hashGrbyTdb().pressureThreshold_,
				    getGlobals()->castToExExeStmtGlobals(),
#endif
				    &rc_,
				    hashGrbyTdb().isPartialGroup_,  // no O/F
				    hashGrbyTdb().isPartialGroup_,
#ifndef __EID
				    hashGrbyTdb().minBuffersToFlush_,
				    hashGrbyTdb().numInBatch_,

				    hashGrbyTdb().forceOverflowEvery(),
				    FALSE,   // forceHashLoop 
				    FALSE,   // forceClusterSPlit

				    ioEventHandler_,
				    this, // the calling tcb
				    hashGrbyTdb().scratchThresholdPct_,
				    hashGrbyTdb().logDiagnostics(),
				    FALSE,   // bufferedWrites 
				    TRUE, // No early overflow based on hints

				    hashGrbyTdb().memoryQuotaMB() ,
				    minMemQuotaMB, 
				    minB4Chk, // BmoMinMemBeforePressureCheck
				    // next 4 are for early overflow (not used)
				    0,
				    0,
				    0,
				    0,
#endif
				    hashGrbyTdb().initialHashTableSize_,
				    getStatsEntry()
                                    );

  if ( !clusterDb_ || rc_ != EXE_OK ) {    // LCOV_EXCL_START
    if ( !clusterDb_ ) rc_ = EXE_NO_MEM_TO_EXEC;  // new() couldn't allocate
    else delete clusterDb_; 
    setState(HASH_GRBY_ERROR);
    return;
  };    // LCOV_EXCL_STOP

  clusterDb_->setScratchIOVectorSize(hashGrbyTdb().scratchIOVectorSize());
  switch(hashGrbyTdb().getOverFlowMode())
  {
    case SQLCLI_OFM_SSD_TYPE: 
      clusterDb_->setScratchOverflowMode(SCRATCH_SSD);
      break;
    case SQLCLI_OFM_MMAP_TYPE: 
      clusterDb_->setScratchOverflowMode(SCRATCH_MMAP);
      break;
    default:
    case SQLCLI_OFM_DISK_TYPE:
      clusterDb_->setScratchOverflowMode(SCRATCH_DISK);
      break;
  }

#ifndef __EID
  clusterDb_->setBMOMaxMemThresholdMB(hashGrbyTdb().getBMOMaxMemThresholdMB());
#endif 

  Cluster * cluster = NULL;
  ULng32 i;
  bucketIdx = 0;
  // allocate the clusters
  for (i = 0; i < noOfClusters; i++) {
    cluster = new(heap_) Cluster(Cluster::CHAINED,
				 clusterDb_,
				 &buckets_[bucketIdx],
				 bucketsPerCluster,
				 hashGrbyTdb().extGroupedRowLength_,
                                 hashGrbyTdb().useVariableLength(),
                                 hashGrbyTdb().considerBufferDefrag(),
				 hashGrbyTdb().hbRowAtpIndex_,
				 TRUE,
				 FALSE, // not used for group by
				 cluster,
				 &rc_);  

    if ( !cluster || rc_ != EXE_OK ) {    // LCOV_EXCL_START
      // we could not allocate the Cluster
      if ( !cluster ) rc_ = EXE_NO_MEM_TO_EXEC;
      else delete cluster;
      setState(HASH_GRBY_ERROR);
      return;
    };     // LCOV_EXCL_STOP

    bucketIdx += bucketsPerCluster;
  };
  
  // store head of the cluster list in the clusterDb_
  clusterDb_->setClusterList(cluster);

  // Initialize the group by state.
  //
  setState(HASH_GRBY_READ_CHILD);


};

/////////////////////////////////////////////////////////////////////////////
// returnResultCurrentRow(): Return the current row (as a group) to parent
// If dataPointer not null, then get the row from there (i.e. the hash buffer)
// Sets the state to CANCELED and returns if either: 
//   1. Returned enough rows to satisty a GET_N request
//   2. An Error occurred
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::returnResultCurrentRow(HashRow * dataPointer) 
{
  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();

  // in case of GET_N : we are done if we returned enough 
  if ( downParentEntry->downState.request == ex_queue::GET_N   &&
       downParentEntry->downState.requestValue <= (Lng32)matchCount_ ) {
    setState(HASH_GRBY_CANCELED);
    return;
  }

  // move current row into the work atp
  if ( NULL == dataPointer ) {  // get current row from child's up-queue
    // We cannot construct the result row dieectly using multiple
    // expressions (initializeAggr, aggrExpr) if there are any varchar
    // aggregates in Aligned Format (CIF).  The second expression may
    // need to change the size of the varchar and this would require
    // rewriting and resizing the whole tuple. This issue is solved in
    // the HashBuffer by forcing these varchar aggregates to be fixed
    // fields.  However, we do not do this for the return row to limit
    // the use of 'forceFixed' to the internal buffers of HashGroupby.
    // The solution here is to construct the return row as a
    // HashBuffer row and then copy this HashBuffer row to the result
    // using one expression.  This means that we need three
    // expressions (2 to construct the HashBuffer row and one to move
    // it to the result).
    // 
    // TDB - Change this to use one new expression which constructs
    // the result row in one shot.
    //
    char buffer[hashGrbyTdb().extGroupedRowLength_];
    workAtp_->getTupp(hbRowAtpIndex_).setDataPointer(buffer);

    ex_queue_entry * childEntry = childQueue_.up->getHeadEntry();

    // move grouping columns from child to the result buffer
     if ( hbMoveInExpr_->eval(childEntry->getAtp(), workAtp_)
	 == ex_expr::EXPR_ERROR ) {
      workHandleError(childEntry->getAtp());
      return;
    }
  
    // when called from EID (with aggr) - turn curr row into a partial group
    if (hbAggrExpr_) { 
      hbAggrExpr_->initializeAggr(workAtp_);
      if ((hbAggrExpr_->perrecExpr()) &&
          (hbAggrExpr_->perrecExpr()->eval(childEntry->getAtp(), workAtp_) ==
           ex_expr::EXPR_ERROR)) {
	workHandleError(childEntry->getAtp());
	return; 
      }
    }

    // Move this row/group from the hash buffer to the result buffer
    if (moveOutExpr_->eval(workAtp_, workAtp_) == ex_expr::EXPR_ERROR) {
      workHandleError(workAtp_);
      return;
    }

  } else {  // take row from hash buffer

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtp_->getTupp(hbRowAtpIndex_).setDataPointer(dataPointer->getData());

    // Move this row/group from the hash buffer to the result buffer
    if (moveOutExpr_->eval(workAtp_, workAtp_) == ex_expr::EXPR_ERROR) {
      workHandleError(workAtp_);
      return;
    }
  }    

  // copy the input to this node to up queue
  upParentEntry->copyAtp(downParentEntry->getAtp());
  
  atp_struct * atp = upParentEntry->getAtp();
  
  // and assign the tupp to the parents queue entry
  atp->getTupp(returnedAtpIndex_) = workAtp_->getTupp(resultRowAtpIndex_);
  
  // Before returning the row -- check HAVING clause for that row
  if ( havingExpr_ ) {
    // Normally there should be no Having-expression, since this is either
    // a partial leaf (i.e., aggr can only be done at the root) or a distinct
    // (i.e. no aggregation). But in some rare cases the compiler generates 
    // such an expression (it should not; it should push the predicate down!)
    //  ( soln 10-090706-2833 )
    switch ( havingExpr_->eval(atp, 0) ) {
    case ex_expr::EXPR_ERROR :
      workHandleError(atp);
      return;
    case ex_expr::EXPR_TRUE :
      // passed the check -- continue
      break;
    default:  // the HAVING predicate failed; release the tuple
      atp->release();

      // need to do anything else ??
      return;
    }
  }

  upParentEntry->upState.status = ex_queue::Q_OK_MMORE;
  upParentEntry->upState.parentIndex 
    = downParentEntry->downState.parentIndex;
  upParentEntry->upState.downIndex = parentQueue_.down->getHeadIndex();
  
  // if stats are to be collected, collect them.
  if (bmoStats_)
    bmoStats_->incActualRowsReturned();
  if (hashGroupByStats_)
    hashGroupByStats_->incPartialGroupsReturned();
  
  // we had a free tupp in the result buffer and use it now
  hasFreeTupp_ = FALSE;
  // we can forget about this tupp now
  workAtp_->getTupp(resultRowAtpIndex_).release();
    
  // we got another result row
  matchCount_++;
  upParentEntry->upState.setMatchNo(matchCount_);

  // insert result into parent up queue
  parentQueue_.up->insert();
}

/////////////////////////////////////////////////////////////////////////////
// workRead: read row from child and group it into the hash buffer
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workReadChild() {
  // If the bitMux expression is set do bitMuxing. If workReadChildBitMux()
  // returns a non-zero value, then bitMuxing failed so continue with the
  // standard hashing scheme.
  //
  if( bitMuxTable_ && !workReadChildBitMux() ) return;

  ex_queue_entry * childEntry = childQueue_.up->getHeadEntry();

  switch (childEntry->upState.status) {
  case ex_queue::Q_OK_MMORE: {
    if (oldState_ != HASH_GRBY_SPILL)
      {

        if (hashGrbyTdb().isPartialGroup_ &&
            hashGrbyTdb().isPassPartialRows()) {

	  returnResultCurrentRow();

          // remove the row from the child's queue
          childQueue_.up->removeHead();
          break;

        }


	// calculate the hash value for this child row. After the evaluation,
	// the hash value is available in hashValue_. The calculation is done
	// only, if we don't come back from spilling a cluster. In this case,
	// the insertion of the row was not sucsessfull. Therefore, we
	// have calculated the hash value already
	if (hashExpr_->eval(childEntry->getAtp(), workAtp_)
	    == ex_expr::EXPR_ERROR)
	  {
            workHandleError(childEntry->getAtp());
	    return;
	  }
	  hashValue_ = hashValue_ & MASK31;
      }
    else
      // reset oldState_
      oldState_ = HASH_GRBY_READ_CHILD;
    
    // determine the bucket and cluster where the row belongs
    ULng32 bucketId = hashValue_ % bucketCount_;
    Cluster * cluster = buckets_[bucketId].getInnerCluster();
    HashTable * ht = cluster->getHashTable();
    ex_assert (ht, "ex_hash_grby_tcb::work() cluster without hash table");

    // find out if there is already an existing group for this row
    ht->position(&cursor_,
		 childEntry->getAtp(),
		 workAtp_,
		 hbRowAtpIndex_,
		 hbSearchExpr_,
		 hashValue_);
    HashRow * hashRow = ht->getNext(&cursor_);
    if (hashRow) {
      // the group exists already. Aggregate the child row into the group
      if (hbAggrExpr_) {
        // The expression does not expect the HashRow to be part of the row.
        // Adjust the datapointer in the work atp to point beyond the HashRow.
	workAtp_->getTupp(hbRowAtpIndex_).setDataPointer(hashRow->getData());
	if ((hbAggrExpr_->perrecExpr()) &&
            (hbAggrExpr_->perrecExpr()->eval(childEntry->getAtp(), workAtp_) ==
             ex_expr::EXPR_ERROR))
	  {
            workHandleError(childEntry->getAtp());
	    return;
	  }

	// Reset the "missed to find a group in the exisitng hash table",
	// since we are looking for consecutive misses before we 
	// clear (send to parent) the hash table.
	partialGroupbyMissCounter_ = 0;
      };
    }
    else {
      // the row belongs to a new group. Initialize a new group in the
      // hash table
      if (buckets_[bucketId].insert(childEntry->getAtp(),
				    hbMoveInExpr_,
				    hashValue_,
				    &rc_)) {
	// row is inserted. Initialize group and aggregate row
	if (hbAggrExpr_) {
	  hbAggrExpr_->initializeAggr(workAtp_);
	  if ((hbAggrExpr_->perrecExpr()) &&
              (hbAggrExpr_->perrecExpr()->eval(childEntry->getAtp(), workAtp_) ==
               ex_expr::EXPR_ERROR))
	  {
            workHandleError(childEntry->getAtp());
	    return;
	  }
	} else // (distinct) New group, no aggregates
	  if ( cluster->getState() == Cluster::CHAINED ) // not spilled yet
            #ifdef __EID
              returnResultCurrentRow();
            #else
	      returnResultCurrentRow(cluster->getLastDataPointer()); // return this row now up to the parent
            #endif
      }
      else {
	// we couldn't insert the row. If we got an error, handle it.
	// EXE_NO_MEM_TO_EXEC is not an error
	if (rc_ && !(rc_ == EXE_NO_MEM_TO_EXEC)) {   // LCOV_EXCL_START
	  setState(HASH_GRBY_ERROR);
	  return;
	};    // LCOV_EXCL_STOP

	// we ran out of memory. Handle this situation. In case of a
	// partial grouping we return the row as a new group to the parent.
	// In case of a regular group by this means we have to spill a
	// cluster
	rc_ = EXE_OK; 
	if (hashGrbyTdb().isPartialGroup_) {
	  // missed to find a group in the existing hash table.
	  // increment the overflow counter and send the row up.
          partialGroupbyMissCounter_++;
	  returnResultCurrentRow(); // Overflow in EID, pass this row up  

	  if (hbAggrExpr_ && (partialGroupbyMissCounter_ > hashGrbyTdb().partialGrbyFlushThreshold_))
	    { 
	      // We need to do the following:
	      // (1) pass the rows in the current cluster
	      //     to the parent and clear the cluster to make 
	      //     space for new groups.
	      // (2) Clears and re-initializes the cluster, so that
	      //     new partial groups can be created.
	      
	      rCluster_ = cluster;
	      // position on the first row in the cluster
	      rCluster_->positionOnFirstRowInBuffer();
	      
	      setState(HASH_GRBY_RETURN_PARTIAL_GROUPS);
              // reset the counter
              partialGroupbyMissCounter_ = 0;
	    }
	}
	else {
	  // it is a regular group by and we couldn't insert the group. We
	  // have to spill a cluster
	  setState(HASH_GRBY_SPILL);

	  return;
	};
      };      
    };

    // remove the row from the child's queue
    childQueue_.up->removeHead();
  } break;

  case ex_queue::Q_NO_DATA: {
    // we are done with reading the child rows.
    setState(HASH_GRBY_EVALUATE);
    doneReadingFromChild();

    // remove the row from the child's queue
    childQueue_.up->removeHead();

#ifndef __EID
    if ( hashGrbyTdb().logDiagnostics() ) { // LOG

      // Report memory quota allocation grabbing while reading child rows
      if ( clusterDb_->memoryQuotaMB() > hashGrbyTdb().memoryQuotaMB() ) {
	char msg[512];
	sprintf(msg, "HGB End reading input (%u). GRABBED additional quota %u MB",
		    0, // NA_64BIT, use instance id later
		    clusterDb_->memoryQuotaMB() - 
		    hashGrbyTdb().memoryQuotaMB());
	// log an EMS event and continue
	SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
      }

      // if had overflow, then provide some data about the clusters
      if ( haveSpilled_ ) {
	ULng32 ind, numSpilled = 0;
	Int64 nonSpilledSize = 0, spilledSize = 0;
	Int64 maxSpilledSize = 0, minSpilledSize = -1;
	Int64 maxNonSpilledSize = 0, minNonSpilledSize = -1;
	// traverse all clusters
	for ( ind = 0; ind < bucketCount_; ind++ ) {
	  Cluster * cluster = buckets_[ind].getInnerCluster();
	  Int64 clusterSize = cluster->getRowCount() *
	    hashGrbyTdb().extGroupedRowLength_ ;
	  if ( cluster->getState() == Cluster::FLUSHED ) {
	    spilledSize += clusterSize;
	    numSpilled++;
	    if ( minSpilledSize == -1 || minSpilledSize > clusterSize )
	      minSpilledSize = clusterSize; 
	    if ( maxSpilledSize < clusterSize )
	      maxSpilledSize = clusterSize; 
	  } else {  // non spilled
	    nonSpilledSize += clusterSize;
	    if ( minNonSpilledSize == -1 || minNonSpilledSize > clusterSize )
	      minNonSpilledSize = clusterSize; 
	    if ( maxNonSpilledSize < clusterSize )
	      maxNonSpilledSize = clusterSize; 
	  }
	}
	char msg[512];
	Int64 MB = 1024 * 1024 ;
	Lng32 nonSpilledSizeMB = (Lng32) (nonSpilledSize / MB) ,
	  spilledSizeMB = (Lng32) (spilledSize / MB),
	  maxSpilledSizeMB = (Lng32) (maxSpilledSize / MB),
	  minSpilledSizeMB = (Lng32) (minSpilledSize / MB),
	  maxNonSpilledSizeMB = (Lng32) (maxNonSpilledSize / MB),
	  minNonSpilledSizeMB = (Lng32) (minNonSpilledSize / MB);
	
	if ( bucketCount_ == numSpilled ) 
	  sprintf(msg, 
		  "HASH GRBY finished reading input; all %d clusters spilled; size in MB - total: %d , average: %d , max: %d , min: %d , variable length records: %s", 
		  numSpilled, spilledSizeMB, spilledSizeMB / numSpilled ,
		  maxSpilledSizeMB, minSpilledSizeMB,
                  hashGrbyTdb().useVariableLength() ? "y" : "n");
	else
	  sprintf(msg, 
		  "HASH GRBY finished reading input; #Clusters total: %u , spilled: %d ; Spilled size in MB - total: %d , average: %d , max: %d , min: %d ; NonSpilled size - total: %d , average: %d , max: %d , min: %d , variable length records: %s", 
		  bucketCount_, numSpilled, spilledSizeMB, 
		  spilledSizeMB / numSpilled ,
		  maxSpilledSizeMB, minSpilledSizeMB,
		  nonSpilledSizeMB, 
		  nonSpilledSizeMB / (bucketCount_ - numSpilled) ,
		  maxNonSpilledSizeMB, minNonSpilledSizeMB,
                  hashGrbyTdb().useVariableLength() ? "y" : "n");
	
	SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, 
					hashGrbyTdb().getExplainNodeId());
      }
    } // if logDiagnostics

    // Finished reading input; try and yield some memory allocated back to 
    // global count of unused memory, so that other BMOs may use this memory
    clusterDb_->yieldUnusedMemoryQuota(ofClusterList_, bucketCount_ ); 
    
#endif

  } break;

  case ex_queue::Q_SQLERROR: {
    // Pass the diagnostics area to the up queue and 
    // set the state to HASH_GRBY_CANCELED.
    workHandleError(childEntry->getAtp());
  } break;
    
  case ex_queue::Q_INVALID: {
    ex_assert(0, "ex_hash_grby_tcb::work() invalid state returned by child");
  } break;
  };
};

// workReadChildBitMux
//
// LCOV_EXCL_START  
Int32 ex_hash_grby_tcb::workReadChildBitMux() {
  ex_queue_entry * childEntry;
  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();

  do {
    // Get a handle on the child queue's head entry
    //
    childEntry = childQueue_.up->getHeadEntry();
    if(childEntry->upState.status != ex_queue::Q_OK_MMORE)
      return 1;
    // Use the bitMux expression to extract the binary key for the grouping
    // into the bitMux tupp. This tupp references the bitMuxBuffer_.
    //
    if(bitMuxExpr_) {
      if (bitMuxExpr_->eval(childEntry->getAtp(), workAtp_)
	  == ex_expr::EXPR_ERROR) {
	// Error returned as a result eval.
	// pass the diagnostics area to the parents up queue.
	// and set the state_ to HASH_GRBY_CANCELED.
	workHandleError(childEntry->getAtp());
	return 0;
      }
    }

    // Attempt to find a match for the data in the bitMuxBuffer_
    // in the bitMux table.
    //
    Int32 result = bitMuxTable_->findOrAdd(bitMuxBuffer_);

    // Set the hbRowAtpIndex tupp to reference the found group
    //
    workAtp_->getTupp(hbRowAtpIndex_).setDataPointer(bitMuxTable_->getData());

    // If the group could not be added to the table, then revert to 
    // standard hashing. This is indicated by returning non-zero
    // to the caller.
    //
    if(result == 0) return 1;

    // Otherwise, if the group did not exist before and was added to
    // the table, the group must be initialized.
    //
    else if(result < 0) {
      // Move the data to the new grouped row
      //
      if(hbMoveInExpr_) {
	if(hbMoveInExpr_->eval(childEntry->getAtp(), workAtp_) ==
	   ex_expr::EXPR_ERROR) {
	  workHandleError(childEntry->getAtp());
	  return 0;
	}
      }

      // Initialize the aggregate for the new grouped row.
      //
      if(bitMuxAggrExpr_)
	((AggrExpr *)(bitMuxAggrExpr_))->initializeAggr(workAtp_);

      // New group and no aggregation -- return this row now!
      // ( Note: Check for "no aggregation" by testing hbAggrExpr_ and not
      //   bitMuxAggrExpr_ because the COUNT() aggregates are missing there  )
      if ( ! hbAggrExpr_ ) {
	// return up this row/group now; 
	returnResultCurrentRow() ;
	// if GET_N was fulfilled (or an error) then quit
	if ( state_ == HASH_GRBY_CANCELED ) return 0 ;
	
	// We are returning rows as they are read; so stop if up-queue is full
	if (parentQueue_.up->isFull()) {
	  childQueue_.up->removeHead(); // this row was consumed
	  return 0;
	}

	// Free entry in parent queue was consumed; allocate a new one
	if (resultPool_->get_free_tuple(workAtp_->getTupp(resultRowAtpIndex_),
                                        (Int32) hashGrbyTdb().resultRowLength_)) {
	  // not enough space in pool. This would be checked again before the
	  // next call to workReadChild(), and then a WORK_POOL_BLOCKED would
	  // be returned by work() 
	  childQueue_.up->removeHead();  // this row was consumed
	  return 0;
	}
	else
	  hasFreeTupp_ = TRUE;
      }
    }

    // Apply the aggregate expression to the grouped row (which is
    // now referenced by the work Atp).
    //
    if(bitMuxAggrExpr_) {
      if(bitMuxAggrExpr_->eval(childEntry->getAtp(), workAtp_) ==
	 ex_expr::EXPR_ERROR) {
	workHandleError(childEntry->getAtp());
	return 0;
      }
    }

    // We have consumed the row from the child. Advance to the next
    // entry in the child queue.
    //
    childQueue_.up->removeHead();

    // Repeat until the child queue is empty
    //
  } while (!childQueue_.up->isEmpty());

  // If we were able to handle the row using the bitMux buffer, then
  // we are done so return 0 to indicate to caller that no hashing needs
  // to be done.
  //
  return 0;
}
// LCOV_EXCL_STOP

#ifndef __EID

/////////////////////////////////////////////////////////////////////////////
// read rows from the overflow buffer and aggregate them into the
// hash buffer
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workReadOverFlowRow() {

  Cluster * oCluster = clusterDb_->getClusterList()->getOuterCluster();
  HashRow * ofDataPointer;
  NABoolean isRowFromBuffer = FALSE;

  if (oldState_ == HASH_GRBY_SPILL)
    // we had to spill a cluster. Thus, the last row is still in the workAtp.
    // process it now
    ofDataPointer = 
      (HashRow *)(workAtp_->getTupp(ofRowAtpIndex_).getDataPointer() - sizeof(HashRow));
  else
    // advance to the next row only, if we really have processed the last
    // one.
    ofDataPointer = oCluster->advance();


  if (ofDataPointer) {
    // get the hash value of this row and change it. Whenever
    // we read a row more than once, we have to change the hash value.
    // Otherwise, the row would always end up in the same bucket.
    if (oldState_ != HASH_GRBY_SPILL)
      {
	// the hash-values are now re-spread over up to 2^10 new buckets
	SimpleHashValue hv = ofDataPointer->hashValue() ;
	SimpleHashValue delta = (hv & 0x03ff00) >> 8 ;
	hashValue_ = hv + delta ;
	hashValue_ = hashValue_ & MASK31;
	isRowFromBuffer = TRUE;
      }
    else
      oldState_ = HASH_GRBY_READ_OF_ROW;

    // determine the bucket and cluster where the row belongs
    ULng32 bucketId = hashValue_ % bucketCount_;
    Cluster * cluster = buckets_[bucketId].getInnerCluster();
    HashTable * ht = cluster->getHashTable();
    ex_assert (ht, "ex_hash_grby_tcb::work() cluster without hash table");
    
    // find out if there is already an existing group for this row

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtp_->getTupp(ofRowAtpIndex_).setDataPointer(ofDataPointer->getData());
    ht->position(&cursor_,
		 workAtp_,
		 workAtp_,
		 hbRowAtpIndex_,
		 ofSearchExpr_,
		 hashValue_);

    HashRow * hashRow = ht->getNext(&cursor_);
    if (hashRow) {
      // the group exists already. Aggregate the child row into the group
      if (ofAggrExpr_ && ofAggrExpr_->perrecExpr()) {

        // The expression does not expect the HashRow to be part of the row.
        // Adjust the datapointer in the work atp to point beyond the HashRow.
	workAtp_->getTupp(hbRowAtpIndex_).setDataPointer(hashRow->getData());
	if (ofAggrExpr_->perrecExpr()->eval(workAtp_, workAtp_) == ex_expr::EXPR_ERROR)
	  {
            workHandleError(workAtp_);
	    return;
	  }
      };
    }
    else { // there is no group for this row yet. Start a new group

      // (when no-aggr) can't let a secondary overflow happen before all the
      // tail part of the outer was read. (extremely unlikely case; only if
      // the memory available shrinks by much during execution of this node.)
      NABoolean doNotOverflow = ! hbAggrExpr_ &&
	! oCluster->returnOverflowRows() ;

      if (buckets_[bucketId].insert(workAtp_,
				    ofMoveInExpr_,
				    hashValue_,
				    &rc_,
				    doNotOverflow )) {
	// row is inserted. Initialize group and aggregate row
	if (hbAggrExpr_)
	  hbAggrExpr_->initializeAggr(workAtp_);
	else  
	  // no aggr: then return now the row that was just inserted, but only
	  // if we now read the unreturned part (i.e., the head of the list) 
	  // and no secondary overflow (i.e., the inner hasn't O.F. yet)
	  if ( cluster->getState() == Cluster::CHAINED && // inner not spilled
	       oCluster->returnOverflowRows() ) // reading HEAD from outer
	    returnResultCurrentRow(cluster->getLastDataPointer()) ;

	if ((ofAggrExpr_) &&
            (ofAggrExpr_->perrecExpr()) &&
	    (ofAggrExpr_->perrecExpr()->eval(workAtp_, workAtp_) == ex_expr::EXPR_ERROR))
	  {
            workHandleError(workAtp_);
	    return;
	  }
      }
      else {
	if (rc_) {
	  setState(HASH_GRBY_ERROR);
	  return;
	};

	// we couldn't insert the row. We have to spill a cluster
	setState(HASH_GRBY_SPILL);

	// Secondary spill (spill while reading from an overflown cluster)
	if ( hashGrbyTdb().logDiagnostics() && isRowFromBuffer ) {
	  char msg[256];
	  sprintf(msg, "HASH GRBY secondary spill" );
	  SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, 
					  hashGrbyTdb().getExplainNodeId());
	}
	
      };
    };
  }
  else {
    // we have processed all rows of the current overflow buffer. Advance
    // to the next overflow buffer of this cluster
    if (!oCluster->endOfCluster()) {
      clusterDb_->setOuterClusterToRead(oCluster, &rc_);
      setState(HASH_GRBY_READ_BUFFER);
      return;
    };
    
    // no more rows in this cluster to process.
    // before we release the cluster, we collect some stats about it
    if (hashGroupByStats_)
      hashGroupByStats_->
	addClusterStats(oCluster->getStats(hashGroupByStats_->getHeap()));
    delete oCluster;
    // Soln 10-100607-0922: Nullify pointer to the deleted oCluster to avoid
    // the possibility of deallocating that cluster again
    clusterDb_->getClusterList()->setOuterCluster(NULL);
    setState(HASH_GRBY_EVALUATE);
  };
};

/////////////////////////////////////////////////////////////////////////////
// spill a cluster
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workSpill() {

  Cluster * cluster = clusterDb_->getClusterToFlush();

  if (!cluster) {
    // we could not find a cluster to spill. The only
    // reson for this situation is that we decided to
    // spill even before we were able to allocate one
    // hashbuffer in any of the clusters. I.e., we are
    // very, very tight on memory. We have to give up.
    rc_ = EXE_NO_MEM_TO_EXEC;
    setState(HASH_GRBY_ERROR);
    return;
  };

  if (cluster->spill(&rc_, ! hbAggrExpr_ )) {
    // all I/Os are done, go back to the state were we came from
    setState(oldState_);
    clusterDb_->setClusterToFlush(NULL);
#ifndef __EID
    if ( hashGrbyTdb().logDiagnostics() ) {
      Int64 elapsedIOTime = ioTimer_.endTimer();   // stop timing, get current total
      if (!haveSpilled_) {
	char msg[256];
	sprintf(msg, 
		"HASH GRBY finished first spill, numChecks: %d., elapsed time = " PF64, 
		numIOChecks_, elapsedIOTime );
	SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, 
		   hashGrbyTdb().getExplainNodeId());
	haveSpilled_ = TRUE;
      }
    }
#endif
  }
  else {
    numIOChecks_++;
#ifndef __EID
    if ( hashGrbyTdb().logDiagnostics() ) {
      ioTimer_.startTimer();                    // start accumulating time
    }
#endif
    if (rc_)
      setState(HASH_GRBY_ERROR);
  };
};

/////////////////////////////////////////////////////////////////////////////
// read one overflow buffer
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workReadBuffer() {
  Cluster * oCluster = clusterDb_->getClusterToRead();
  if (oCluster->read(&rc_)) {
    // the I/O is complete, position on the first row of the buffer
    oCluster->positionOnFirstRowInFirstOuterBuffer();
    setState(HASH_GRBY_READ_OF_ROW);
    clusterDb_->setClusterToRead(NULL);
  }
  else {
    if (rc_)
      setState(HASH_GRBY_ERROR);
  };
};

#endif

/////////////////////////////////////////////////////////////////////////////
// workEvaluate: prepare clusters for next phase
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workEvaluate() {

  Cluster * cluster = clusterDb_->getClusterList();
  while(cluster) {
    if (cluster->getState() == Cluster::CHAINED) {

      if ( cluster->getRowCount() && hbAggrExpr_ ) {
 	
	// This cluster is CHAINED. remove it from the clusterList and
	// return all groups in this cluster to the parent
	clusterDb_->setClusterList(cluster->getNext());
	cluster->setNext(NULL);
	rCluster_ = cluster;
	// position on the first row in the cluster
	rCluster_->positionOnFirstRowInBuffer();

	// and return the rows
	setState(HASH_GRBY_RETURN_ROWS);
	return;
      }
      else {
	// No aggregation, so this cluster was already returned to parent or
	// this cluster contains no groups. so forget about it
	clusterDb_->setClusterList(cluster->getNext());
	// before we release the cluster, we collect some stats about it
	if (hashGroupByStats_)
	  hashGroupByStats_->
            addClusterStats(cluster->getStats(hashGroupByStats_->getHeap()));
	delete cluster;
      }
    }

#ifndef __EID

    else {
      // the cluster is FLUSHED/SPILLED.

      // if we haven't done it yet, write the last buffer to temporary
      // file
      if (cluster->getRowsInBuffer()) {
	clusterDb_->setClusterToFlush(cluster);
	setState(HASH_GRBY_SPILL);
	return;
      };

      //Remove it from the clusterList and add
      // it to the overFlowList.
      clusterDb_->setClusterList(cluster->getNext());

      if ( ! hbAggrExpr_ ) 
	// For non-aggr: Start reading the already returned rows first
	cluster->initializeReadPosition();

      cluster->setNext(NULL);
      if (!ofClusterList_)
	ofClusterList_ = cluster;
      else 
	lastOfCluster_->setNext(cluster);

      lastOfCluster_ = cluster;

      // delete the hash table and buffer for this cluster, we don't
      // need it anymore
      cluster->removeHashTable();
      cluster->releaseAllHashBuffers();
    };

#endif

    cluster = clusterDb_->getClusterList();
  };

#ifndef __EID

  // all clusters are now either processed or on the overFlowList.

  // Before handling another spilled cluster, try and yield some memory 
  // allocated back to global count of unused memory, so that other BMOs 
  // may use this memory
  clusterDb_->yieldUnusedMemoryQuota(ofClusterList_, bucketCount_ );

  if (ofClusterList_) {

    // take the first cluster form the overflow list and process
    // it now, i.e., use it as input.
    Cluster * inputCluster = ofClusterList_;
    ofClusterList_ = ofClusterList_->getNext();
    inputCluster->setNext(NULL);
    
    ULng32 i;

    // re-initialize all buckets
    for (i = 0; i < bucketCount_; i++)
      buckets_[i].init();

    // allocate new clusters. Currently, a cluster
    // contains always one bucket. This might change in the future?
    ULng32 bucketIdx = 0;
    ULng32 noOfClusters = bucketCount_;
    ULng32 bucketsPerCluster = bucketCount_/noOfClusters;

    Cluster * newCluster = NULL;

    for (i = 0; i < noOfClusters; i++) {
      newCluster = new(heap_) Cluster(Cluster::CHAINED,
				      clusterDb_,
				      &buckets_[bucketIdx],
				      bucketsPerCluster,
				      hashGrbyTdb().extGroupedRowLength_,
                                      hashGrbyTdb().useVariableLength(),
                                      hashGrbyTdb().considerBufferDefrag(),
				      hashGrbyTdb().hbRowAtpIndex_,
				      TRUE,
				      FALSE, // not used for group by
				      newCluster,
				      &rc_);  

      if ( !newCluster || rc_ != EXE_OK ) {
	// we could not allocate the Cluster
	if ( !newCluster ) rc_ = EXE_NO_MEM_TO_EXEC;
	else delete newCluster;
	setState(HASH_GRBY_ERROR);
	return;
      };

      bucketIdx += bucketsPerCluster;
    };
  
    // store head of the cluster list in the clusterDb_
    clusterDb_->setClusterList(newCluster);

    // The inputCluster is to be marked as an "outer" so that its buffers
    // would be read one at a time (for inner, all buffers are read at once.)
    // the input rows come from the outer cluster of the first
    // cluster in the cluster list. Set this outer cluster accordingly.
    newCluster->setOuterCluster(inputCluster);

    // read the first buffer of the input
    clusterDb_->setOuterClusterToRead(inputCluster, &rc_);
    if ( rc_ ) {
      setState(HASH_GRBY_ERROR);
      return;
    }
    
    setState(HASH_GRBY_READ_BUFFER);
    return;
  }

#endif
  // no more clusters to process. We are done unless there are bitmux
  // rows to return (and aggregation is used, so these rows weren't returned)
  if ( bitMuxTable_ && hbAggrExpr_ )
    setState(HASH_GRBY_RETURN_BITMUX_ROWS);
  else
    setState(HASH_GRBY_DONE);
};

/////////////////////////////////////////////////////////////////////////////
// workReturnRows: return rows to parent 
/////////////////////////////////////////////////////////////////////////////
ULng32 ex_hash_grby_tcb::workReturnRows(NABoolean tryToDefrag) {
  HashRow * dataPointer = NULL;

  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();

  // If we are returning rows from the bitmux buffer, then search through
  // the bitmux pointers for the first return row.
  //
  if(state_ == HASH_GRBY_RETURN_BITMUX_ROWS) {
    dataPointer = (HashRow *)(bitMuxTable_->getReturnGroup());
    bitMuxTable_->advanceReturnGroup();
  }
  // Otherwise, we are returning rows from the cluster so advance to the
  // next row.
  //
  else {
    if (!tryToDefrag)
    {
      dataPointer = rCluster_->advance();
    }
    else
    {// don' t advance in this case
      // we may exit the proc without procesing if we don't get a free tuple
      dataPointer = rCluster_->getCurrentRow();
    }
  }
  
  while (dataPointer) {

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtp_->getTupp(hbRowAtpIndex_).setDataPointer(dataPointer->getData());

    // this is a result group. Move it from the hash buffer to the
    // result buffer
    
    ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();
    ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();

    UInt32 resMaxRowLen = hashGrbyTdb().resultRowLength_;
    UInt32 rowLen = resMaxRowLen;
    UInt32 *rowLenPtr = &rowLen;

    if (tryToDefrag)
    {
#if defined(_DEBUG)
      assert(hasFreeTupp_ == FALSE && !havingExpr_);
#endif

      workAtp_->getTupp(resultRowAtpIndex_)= defragTd_;
      defragTd_->setReferenceCount(1);
      if (moveOutExpr_->eval(workAtp_, workAtp_, 0, -1, rowLenPtr) == ex_expr::EXPR_ERROR)
      {
        workHandleError(workAtp_);
        return 0;
      }
      if (resultPool_->get_free_tuple(workAtp_->getTupp(resultRowAtpIndex_),*rowLenPtr))
        return 2 ; //WORK_POOL_BLOCKED;
#if (defined (NA_LINUX) && defined(_DEBUG) && !defined(__EID))
      char txt[] = "hashgrpby";
      SqlBuffer * buf = resultPool_->getCurrentBuffer();
      sql_buffer_pool::logDefragInfo(txt,
          SqlBufferGetTuppSize(hashGrbyTdb().resultRowLength_,buf->bufType()),
          SqlBufferGetTuppSize(*rowLenPtr,buf->bufType()),
          buf->getFreeSpace(),
          buf,
          buf->getTotalTuppDescs());

#endif
      //advance now
      rCluster_->advance();
      char * defragDataPointer = defragTd_->getTupleAddress();
      char * destDataPointer = workAtp_->getTupp(resultRowAtpIndex_).getDataPointer();
      str_cpy_all(destDataPointer,
                  defragDataPointer,
                  *rowLenPtr);
    }
    else
    if (moveOutExpr_->eval(workAtp_, workAtp_, 0, -1, rowLenPtr) == ex_expr::EXPR_ERROR)
      {
        workHandleError(workAtp_);
        return 0;
      }
    
    upParentEntry->copyAtp(downParentEntry->getAtp());
    
    atp_struct * atp = upParentEntry->getAtp();
    
    // and assign the tupp to the parents queue entry
    atp->getTupp(returnedAtpIndex_) =
      workAtp_->getTupp(resultRowAtpIndex_);

    ex_expr::exp_return_type evalRetCode = ex_expr::EXPR_TRUE;
    if (havingExpr_) {
      evalRetCode = havingExpr_->eval(atp, 0);
    }
    
    if ( evalRetCode == ex_expr::EXPR_TRUE ) { // e.g.  havingExpr_ == NULL

      // Resize the row if the actual size is different from the max size (resMaxRowLen)
      if(rowLen != resMaxRowLen) {
        resultPool_->resizeLastTuple(rowLen, 
                                     workAtp_->getTupp(resultRowAtpIndex_).getDataPointer());
      }
      

      // if stats are to be collected, collect them.
      if (bmoStats_) {
	bmoStats_->incActualRowsReturned();
      }    

      upParentEntry->upState.status = ex_queue::Q_OK_MMORE;
      upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
      upParentEntry->upState.downIndex = parentQueue_.down->getHeadIndex();
      
      hasFreeTupp_ = FALSE;
      
      // we can forget about this tupp now
      workAtp_->getTupp(resultRowAtpIndex_).release();
      
      // we got another result row
      matchCount_++;
      upParentEntry->upState.setMatchNo(matchCount_);

#ifndef __EID
      if ( matchCount_ == 1 && haveSpilled_ ) {
	// haveSpilled_ indicates that an overflow took place (Only for distinct
	// HGB the overflow occurs after the return of the first row, but that
	// happens at the very begining, so it is not of interest for us.)
	if ( hashGrbyTdb().logDiagnostics() )
	  SQLMXLoggingArea::logExecRtInfo(NULL, 0, 
					  "HASH GRBY returns first row.", 
					  hashGrbyTdb().getExplainNodeId());
      }
#endif
      
      // insert into parent up queue
      parentQueue_.up->insert();
      
      return 0;
    }
    else if (evalRetCode == ex_expr::EXPR_ERROR)
      {
        workHandleError(atp);
        return 0;
      }
    else {
      // no hit. release the returned tuple
      atp->release();
      
      // Try to advance to the next return row
      //
      dataPointer = NULL;

      // If we are returning rows from the bitmux buffer, then 
      // get the next return row.
      //
      if(state_ == HASH_GRBY_RETURN_BITMUX_ROWS) {
	dataPointer = (HashRow *)(bitMuxTable_->getReturnGroup());
	bitMuxTable_->advanceReturnGroup();
      }
      // Otherwise, we are returning rows from the cluster so advance to the
      // next row.
      //
      else {
	dataPointer = rCluster_->advance();
      }
    };
  } // while (dataPointer)
  
  // If we were returning rows from the bitmux buffer, then the buffer
  // must be empty so set the state to done.
  //
  if(state_ == HASH_GRBY_RETURN_BITMUX_ROWS) {
    bitMuxTable_->resetReturnGroup();
  }
  // Otherwise, we are done returning rows from a cluster. Go back to the
  // evaluate phase.
  //
  else {
    // no more rows in this cluster. Delete cluster.
    // before we release the cluster, we collect some stats about it
    if (hashGroupByStats_ &&
                state_ !=  HASH_GRBY_RETURN_PARTIAL_GROUPS)
      hashGroupByStats_->
	addClusterStats(rCluster_->getStats(hashGroupByStats_->getHeap()));
    delete rCluster_;
    rCluster_ = NULL;
  }
  return 1;
};

/////////////////////////////////////////////////////////////////////////////
// resetClusterAndReadFromChild: creates a new cluster and prepares
// it for reading from child.
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::resetClusterAndReadFromChild()
{

  // There must be a better way to reuse
  // the rCluster_ without deleteing and re-allocating 
  // the cluster.
  // 
  Cluster * cluster = NULL;
  ULng32 bucketIdx = 0;

  buckets_[bucketIdx].init();


  // allocate the cluster
  cluster = new(heap_) Cluster(Cluster::CHAINED,
			       clusterDb_,
			       &buckets_[bucketIdx],
			       1, // bucketsPerCluster,
			       hashGrbyTdb().extGroupedRowLength_,
                               hashGrbyTdb().useVariableLength(),
                               hashGrbyTdb().considerBufferDefrag(),
			       hashGrbyTdb().hbRowAtpIndex_,
			       TRUE,
			       FALSE, // not used for group by
			       cluster,
			       &rc_);  

  if ( !cluster || rc_ != EXE_OK ) {
    // we could not allocate the Cluster
    if ( !cluster ) rc_ = EXE_NO_MEM_TO_EXEC;
    else delete cluster;
    setState(HASH_GRBY_ERROR);
    return;
  };
  
  // store head of the cluster list in the clusterDb_
  clusterDb_->setClusterList(cluster);
  
  // Initialize the group by state.
  //
  setState(HASH_GRBY_READ_CHILD);
  
};	    


/////////////////////////////////////////////////////////////////////////////
// workDone: cleanup and return
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workDone() {
  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();

  upParentEntry->upState.status = ex_queue::Q_NO_DATA;
  upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
  upParentEntry->upState.downIndex = parentQueue_.up->getHeadIndex();
  upParentEntry->upState.setMatchNo(matchCount_);

  parentQueue_.down->removeHead();

#ifndef __EID
  if ( haveSpilled_ && hashGrbyTdb().logDiagnostics() ) {
    char msg[256];
    Int64 elapsedIOTime = ioTimer_.endTimer();// stop timing, get current total

    sprintf(msg, 
	    "HASH GRBY returns last row; for spills, numChecks: %d , elapsed time = " PF64, 
	    numIOChecks_, elapsedIOTime );
    SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, 
	       hashGrbyTdb().getExplainNodeId());
    numIOChecks_ = 0;
  }
#endif

  setState(HASH_GRBY_EMPTY);
  parentQueue_.up->insert();

  if (rCluster_) {
    delete rCluster_;
    rCluster_ = NULL;
  };

  // Soln 10-100607-0922: Delete flushed clusters before deleting clusterDb_
  // We'd have flushed clusters here only if the query was canceled while
  // processing overflow
  while (ofClusterList_) {
    Cluster * p = ofClusterList_->getNext();
    delete ofClusterList_;
    ofClusterList_ = p;
  };

  if (clusterDb_) {
#ifndef __EID
    // Yield memory allocated back to global count of unused memory, so that
    // other BMOs may use this memory
    clusterDb_->yieldAllMemoryQuota();  // yield all of the alloc memory
#endif

    delete clusterDb_;
    clusterDb_ = NULL;
  };

  // delete the buckets
  if (buckets_) {
    heap_->deallocateMemory(buckets_);
    buckets_ = NULL;
  };

  if (hasFreeTupp_) {
    workAtp_->getTupp(resultRowAtpIndex_).release();
    hasFreeTupp_ = FALSE;
  };

  // fix for CR 10-010713-3879: release tempUpAtp_ when done
  // to decrease tupp descriptor reference count
  tempUpAtp_->release();

};

/////////////////////////////////////////////////////////////////////////////
// workHandleError: insert Q_SQLERROR into parent's up queue.
/////////////////////////////////////////////////////////////////////////////
void ex_hash_grby_tcb::workHandleError(atp_struct* atp) 
{
  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();

  if (atp)
    upParentEntry->getAtp()->copyAtp(atp);
  upParentEntry->upState.status = ex_queue::Q_SQLERROR;
  upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
  upParentEntry->upState.downIndex = parentQueue_.up->getHeadIndex();
  upParentEntry->upState.setMatchNo(matchCount_);
  
  parentQueue_.up->insert();

  // Cancel all the this parent's request, and cleanup child queue, if needed
  setState(HASH_GRBY_CANCELED);
};

//////////////////////////////////////////////////////////////////////////
// The hash groupby operator does not need private state per queue entry
//////////////////////////////////////////////////////////////////////////

