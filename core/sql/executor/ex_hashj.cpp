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
* File:         ex_hashj.cpp
* Description:  Methods for the tdb and tcb of a hash join 
*               with the ordered queue protocol
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
// with a hybrid hash join
//

// beginning of regular compilation

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_hashj.h"
#include "ex_expr.h"
#include "str.h"
#include "BaseTypes.h"
#include "ExStats.h"
#include "ex_error.h"
#include "ex_exe_stmt_globals.h"
#include "memorymonitor.h"
#include "sql_buffer_size.h"
#include "logmxevent.h"
#include "exp_function.h"
#include "CommonStructs.h"

///////////////////////////////////////////////////////////////////////////////
// hashj state transition diagram source
//
// Please maintain this as you add states or make changes to the transitions
// between states.  The grammar is quite simple: "oldstate -> newstate" is all
// you need to be concerned with.
//
// To generate a nice diagram, copy this text into hj_std.dot,
// download http://www.graphviz.org/Download.php
// and run: dot -Tgif hj_std.dot >hj.gif
//
///////////////////////////////////////////////////////////////////////////////

/*
digraph hasjoin {
   EMPTY -> READ_OUTER
   EMPTY -> READ_INNER
   EMPTY -> DONE

   FLUSH_CLUSTER0 [ label="FLUSH_CLUSTER" ]
   FLUSH_CLUSTER1 [ label="FLUSH_CLUSTER" ]
   FLUSH_CLUSTER2 [ label="FLUSH_CLUSTER" ]
   FLUSH_CLUSTER3 [ label="FLUSH_CLUSTER" ]

   PROBE0 [ label="PROBE" ]
   PROBE1 [ label="PROBE" ]

   READ_BUFFER -> READ_OUTER_CLUSTER

   READ_OUTER_CLUSTER -> RETURN_RIGHT_ROWS
   READ_OUTER_CLUSTER -> READ_INNER_CLUSTER
   READ_OUTER_CLUSTER -> PROBE0
   READ_OUTER_CLUSTER -> READ_BUFFER
   PROBE0 -> READ_OUTER_CLUSTER

   RETURN_RIGHT_ROWS -> READ_INNER_CLUSTER
   RETURN_RIGHT_ROWS -> END_PHASE2

   READ_INNER -> END_PHASE1
   READ_INNER -> FLUSH_CLUSTER0
   FLUSH_CLUSTER0 -> READ_INNER

   READ_OUTER -> RETURN_RIGHT_ROWS
   READ_OUTER -> FLUSH_CLUSTER3
   READ_OUTER -> END_PHASE2
   FLUSH_CLUSTER3 -> READ_OUTER
   READ_OUTER -> PROBE1
   PROBE1 -> READ_OUTER

   not_EMPTY [ shape=plaintext ]
   not_EMPTY -> CANCELED
   not_EMPTY -> HASHJ_CANCEL_AFTER_INNER

   HASHJ_CANCEL_AFTER_INNER -> END_PHASE1

   CANCELED -> DONE

   READ_INNER_CLUSTER -> READ_BUFFER

   END_PHASE1 -> CANCELED [ label="oldState() == CANCEL_AFTER_INNER" ]
   END_PHASE1 -> READ_OUTER
   END_PHASE1 -> FLUSH_CLUSTER1
   FLUSH_CLUSTER1 -> END_PHASE1

   END_PHASE2 -> DONE
   END_PHASE2 -> FLUSH_CLUSTER2
   END_PHASE2 -> READ_INNER_CLUSTER
   FLUSH_CLUSTER2 -> END_PHASE2
}
*/

// In the reverse order of ex_hashj_tcb::HashJoinPhase
const char *ex_hashj_tcb::HashJoinPhaseStr[] = {
    "PHASE_END",
    "PHASE_3",
    "PHASE_2",
    "PHASE_1"
};

///////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
///////////////////////////////////////////////////////////////////////////////


//
// Build a hashj tcb
//
ex_tcb * ex_hashj_tdb::build(ex_globals * glob) {

  // first build the children
  ex_tcb * leftChildTcb = leftChildTdb_->build(glob);
  ex_tcb * rightChildTcb = rightChildTdb_->build(glob);

  ex_hashj_tcb * hashJoinTcb = NULL;

  // now build the hash join Tcb

  // If this Hash Join is configured to be a Unique Hash Join,
  // use the ExUniqueHashJoinTcb
  //
  if(isUniqueHashJoin()) {

    hashJoinTcb = new(glob->getSpace())
      ExUniqueHashJoinTcb(*this,
                          *leftChildTcb,
                          *rightChildTcb,
                          glob);
  } else {

    hashJoinTcb = new(glob->getSpace())
      ex_hashj_tcb(*this,
                   *leftChildTcb,
                   *rightChildTcb,
                   glob);
  }

  // add the hashJoinTcb to the schedule
  hashJoinTcb->registerSubtasks();
  hashJoinTcb->registerResizeSubtasks();

  return (hashJoinTcb);
}

void ex_hashj_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();
  ex_queue_pair pQueue = getParentQueue();

  // down queues are handled by workDown()
  // up queues are handled by workUp()
  // cancellations are handled by workCancel()

  sched->registerInsertSubtask(sWorkDown, this, pQueue.down, "DN");
  sched->registerCancelSubtask(sCancel,   this, pQueue.down, "CA");
  sched->registerUnblockSubtask(sWorkUp,  this, pQueue.up,   "UP");

  // Register the I/O event 
  ioEventHandler_ =  sched->registerNonQueueSubtask(sWorkUp,this);
  
  // We need to schedule workUp from workDown if we see a GET_NOMORE
  // in workDown in case nothing else scheules workUp.
  workUpTask_ = sched->registerNonQueueSubtask(sWorkUp, this, "UP");

  // register events for child queues
  sched->registerUnblockSubtask(sWorkDown,this, leftQueue_.down);
  sched->registerInsertSubtask(sWorkUp, this, leftQueue_.up);

  sched->registerUnblockSubtask(sWorkDown,this, rightQueue_.down);
  sched->registerInsertSubtask(sWorkUp, this, rightQueue_.up);
}

NABoolean ex_hashj_tcb::needStatsEntry()
{
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  // stats are collected for ALL and MEASURE options.
  if (statsType == ComTdb::ALL_STATS || statsType == ComTdb::OPERATOR_STATS)
    return TRUE;
  else
    return FALSE;
}


ExOperStats * ex_hashj_tcb::doAllocateStatsEntry(CollHeap *heap, ComTdb *tdb)
{
  ExBMOStats *stat;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
    stat = new (heap) ExBMOStats(heap, this, tdb);
  else
  {
    stat = (ExBMOStats *)new(heap) ExHashJoinStats(heap,
				   this,
				   tdb);
    hashJoinStats_ = (ExHashJoinStats *)stat;
  }
  ex_hashj_tdb *hashjTdb = (ex_hashj_tdb *)getTdb();
  bmoStats_ = stat;
  return stat;
}

//////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
//////////////////////////////////////////////////////////////////////////////

//
// Constructor for hashj_tcb
//
ex_hashj_tcb::ex_hashj_tcb(const ex_hashj_tdb & hashJoinTdb,  
			   const ex_tcb & leftChildTcb,    // left queue pair
			   const ex_tcb & rightChildTcb,   // right queue pair
			   ex_globals * glob)
  : ex_tcb(hashJoinTdb, 1, glob),
    space_(glob->getSpace()),
    leftChildTcb_(&leftChildTcb),
    rightChildTcb_(&rightChildTcb),
    bucketCount_(0),
    buckets_(NULL),
    outerMatchedInner_(FALSE),
    rc_(EXE_OK),
    hashValue_(0),
    clusterDb_(NULL),
    workAtp_(NULL),
    ioEventHandler_(NULL),
    currCluster_(NULL),
    flushedChainedCluster_(FALSE),
    totalRightRowsRead_(0),
    isAllOrNothing_(FALSE),
    anyRightRowsRead_(FALSE),
    doNotChainDup_(FALSE),
    hasFreeTupp_(FALSE),
    isRightOutputNeeded_(hashJoinTdb.rightRowLength_ > 0
			 // in some cases, even when rightRowLength_ is zero,
			 // the left join expr applies to the right tupp
			 // see soln 10-090107-8249
			 || hashJoinTdb.leftJoinExpr_ 
			 ),
    onlyReturnResultsWhenInnerMatched_( !hashJoinTdb.isLeftJoin() &&
				        !hashJoinTdb.isAntiSemiJoin() ) ,
    isInnerEmpty_(TRUE),
    nullPool_(NULL)
{
  bmoStats_ = NULL;
  hashJoinStats_ = NULL;
  heap_ = new (glob->getDefaultHeap()) NAHeap("Hash Join Heap", (NAHeap *)glob->getDefaultHeap());
  // set the memory monitor
  memMonitor_ = getGlobals()->castToExExeStmtGlobals()->getMemoryMonitor();

  // Allocate the buffer pool for result rows
  // this pool contains only one buffer
  Int32 numBuffers = hashJoinTdb.numBuffers_;

  // The default number of buffers is 1.
  // Regular Hash Join (not unique) adds more buffers on demand (it
  // does not return POOL_BLOCKED).
  // Unique Hash Join returns POOL_BLOCKED, so if this is a unique
  // hash join make sure there are at least 2 buffers to avoid dead
  // lock on a pinned buffer.
  if(hashJoinTdb.isUniqueHashJoin() && (numBuffers < 2)) {
    numBuffers = 2;
  }
  
  resultPool_ = new(space_) sql_buffer_pool(numBuffers,
					    (Lng32)hashJoinTdb.bufferSize_,
					    space_);
  

  if (hashJoinTdb.considerBufferDefrag())
  {
    assert(hashJoinTdb.useVariableLength());

    ULng32 defragLength = MAXOF(hashJoinTdb.leftRowLength_, hashJoinTdb.rightRowLength_);
    resultPool_->addDefragTuppDescriptor(defragLength);
  }

  // Copy all expression pointers
  minMaxExpr_ = hashJoinTdb.minMaxExpr_;
  rightHashExpr_ = hashJoinTdb.rightHashExpr_;
  rightMoveInExpr_ = hashJoinTdb.rightMoveInExpr_;
  rightMoveOutExpr_ = hashJoinTdb.rightMoveOutExpr_;
  rightSearchExpr_ = hashJoinTdb.rightSearchExpr_;
  leftHashExpr_ = hashJoinTdb.leftHashExpr_;
  leftMoveExpr_ = hashJoinTdb.leftMoveExpr_;
  leftMoveInExpr_ = hashJoinTdb.leftMoveInExpr_;
  leftMoveOutExpr_ = hashJoinTdb.leftMoveOutExpr_;
  probeSearchExpr1_ = hashJoinTdb.probeSearchExpr1_;
  probeSearchExpr2_ = hashJoinTdb.probeSearchExpr2_;
  leftJoinExpr_ = hashJoinTdb.leftJoinExpr_;
  nullInstForLeftJoinExpr_ = hashJoinTdb.nullInstForLeftJoinExpr_;
  
  rightJoinExpr_ = hashJoinTdb.rightJoinExpr_;
  nullInstForRightJoinExpr_ = hashJoinTdb.nullInstForRightJoinExpr_;
  
  beforeJoinPred1_ = hashJoinTdb.beforeJoinPred1_;
  beforeJoinPred2_ = hashJoinTdb.beforeJoinPred2_;
  afterJoinPred1_ = hashJoinTdb.afterJoinPred1_;
  afterJoinPred2_ = hashJoinTdb.afterJoinPred2_;
  afterJoinPred3_ = hashJoinTdb.afterJoinPred3_;
  afterJoinPred4_ = hashJoinTdb.afterJoinPred4_;
  afterJoinPred5_ = hashJoinTdb.afterJoinPred5_;
  checkInputPred_ = hashJoinTdb.checkInputPred_;
  moveInputExpr_  = hashJoinTdb.moveInputExpr_;
  checkInnerNullExpr_  = hashJoinTdb.checkInnerNullExpr_;
  checkOuterNullExpr_ = hashJoinTdb.checkOuterNullExpr_;

  // Set up a flag for the HashTable::position method calls,
  // for cases where it is valid to return only one match
  // from the inner table.  Keep this logic in sync with the 
  // multiple-match loop in  ex_hashj_tcb::workProbe.
  if (isSemiJoin()              &&
      beforeJoinPred1_ == NULL  && 
      beforeJoinPred2_ == NULL  && 
      afterJoinPred1_  == NULL  &&
      afterJoinPred2_  == NULL)
    doNotChainDup_ = TRUE;
  else if (isAntiSemiJoin()     &&
	   beforeJoinPred1_ == NULL  && 
	   beforeJoinPred2_ == NULL)
    doNotChainDup_ = TRUE;
  else 
    ; // see above in the ctor initializer that we have set this
  // flag to FALSE;
  
  // get the queues that left and right use to communicate with me
  leftQueue_  = leftChildTcb_->getParentQueue();
  rightQueue_  = rightChildTcb_->getParentQueue();
  // We don't need state in this up queues
  
  ex_cri_desc * fromParentCri = hashJoinTdb.criDescDown_;  
  ex_cri_desc * toParentCri = hashJoinTdb.criDescUp_;
  
  // Allocate the queue to communicate with parent (no PSTATE)
  allocateParentQueues(parentQueue_,FALSE);
  
  // Allocate the private state in each entry of the down queue
  ex_hashj_private_state *p = new(space_) ex_hashj_private_state();
  parentQueue_.down->allocatePstate(p, this);
  delete p;
  
  // fixup expressions
  if (minMaxExpr_)
    (void) minMaxExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (rightHashExpr_)
    (void) rightHashExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (rightMoveInExpr_)
    (void) rightMoveInExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (rightMoveOutExpr_) 
    (void) rightMoveOutExpr_->fixup(0, getExpressionMode(), this, 
				    space_, heap_, FALSE, glob);
  if (rightSearchExpr_) 
    (void) rightSearchExpr_->fixup(0, getExpressionMode(), this,
				   space_, heap_, FALSE, glob);
  else {
    // ASJ, and no join search expr and no before predicate -- then either
    // return all left rows, or none, based on having ANY right rows
    isAllOrNothing_ = isAntiSemiJoin() && ! beforeJoinPred1_ ;
  }
  if (leftHashExpr_) 
    (void) leftHashExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (leftMoveExpr_) 
    (void) leftMoveExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (leftMoveInExpr_) 
    (void) leftMoveInExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (leftMoveOutExpr_) 
    (void) leftMoveOutExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (probeSearchExpr1_) 
    (void) probeSearchExpr1_->fixup(0, getExpressionMode(), this,
				    space_, heap_, FALSE, glob);
  if (probeSearchExpr2_) 
    (void) probeSearchExpr2_->fixup(0, getExpressionMode(), this,space_, heap_, FALSE, glob);
  if (leftJoinExpr_) 
    (void) leftJoinExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (nullInstForLeftJoinExpr_) 
    (void) nullInstForLeftJoinExpr_->fixup(0, getExpressionMode(), this,
					   space_, heap_, FALSE, glob);
  if (rightJoinExpr_) 
    (void) rightJoinExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (nullInstForRightJoinExpr_) 
    (void) nullInstForRightJoinExpr_->fixup(0, getExpressionMode(), this,
					    space_, heap_, FALSE, glob);
  
  if (beforeJoinPred1_) 
    (void) beforeJoinPred1_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (beforeJoinPred2_) 
    (void) beforeJoinPred2_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (afterJoinPred1_) 

    (void) afterJoinPred1_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (afterJoinPred2_) 
    (void) afterJoinPred2_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (afterJoinPred3_) 
    (void) afterJoinPred3_->fixup(0, getExpressionMode(), this,space_, heap_, FALSE, glob);
  if (afterJoinPred4_) 
    (void) afterJoinPred4_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
  if (afterJoinPred5_) 
    (void) afterJoinPred5_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);

  if ( isReuse() ) {
    if (checkInputPred_)
      (void) checkInputPred_->fixup(0, getExpressionMode(),this,space_, heap_, FALSE, glob);
    
    if (moveInputExpr_)
      (void) moveInputExpr_->fixup(0, getExpressionMode(), this, space_, heap_, FALSE, glob);
    
    // allocate space to keep the current input values for next time
    Lng32 neededBufferSize = 
      (Lng32) SqlBufferNeededSize( 1, hashJoinTdb.inputValuesLen_ );
    inputPool_ = new(space_) sql_buffer_pool(1, neededBufferSize, space_);
    inputPool_->get_free_tuple(prevInputValues_, hashJoinTdb.inputValuesLen_);
  } // if isReuse
  
  if (checkInnerNullExpr_) 
    (void) checkInnerNullExpr_->fixup(0, getExpressionMode(), this,space_, heap_, FALSE, glob);

  if (checkOuterNullExpr_) 
    (void) checkOuterNullExpr_->fixup(0, getExpressionMode(), this,space_, heap_, FALSE, glob);

  // get initial head index
  nextRequest_ = parentQueue_.down->getHeadIndex();
  
  haveAllocatedClusters_ = FALSE; // no clusters yet (nor their hash tables)
  havePrevInput_ = FALSE;  // first time there is no previous input
  
  workAtp_ = allocateAtp(hashJoinTdb.workCriDesc_, space_);
  
  // initialize tupp descriptor for the hash value
  hashValueTupp_.init(sizeof(SimpleHashValue), NULL, (char *) (&hashValue_));
  
  // allocate tupp descriptors for tupps in the workAtp. tupps 0 and 1
  // are used for constants and temps. Don't allocate them here. Also
  // left row, right row and instantiated row are allocated in the
  // result buffer. Only allocate tupp descriptors for the extended rows
  
  workAtp_->getTupp(hashJoinTdb.extLeftRowAtpIndex_) =
    new(space_) tupp_descriptor;
  workAtp_->getTupp(hashJoinTdb.extRightRowAtpIndex1_) =
    new(space_) tupp_descriptor;
  workAtp_->getTupp(hashJoinTdb.extRightRowAtpIndex2_) = 
    new(space_) tupp_descriptor;
  
  // assign the hash value tupp
  workAtp_->getTupp(hashJoinTdb.hashValueAtpIndex_) = &hashValueTupp_;


  // Use one null tuple for both Left and Right joins;
  ULng32 nullLength = 0;  
  
  if(leftJoinExpr_ && nullInstForLeftJoinExpr_ && 
     (hashJoinTdb.instRowForLeftJoinLength_ > 0)) {
    nullLength = hashJoinTdb.instRowForLeftJoinLength_;
  }

  if (rightJoinExpr_ && nullInstForRightJoinExpr_ && 
      (hashJoinTdb.instRowForRightJoinLength_ > 0)) {
    nullLength = MAXOF(nullLength, hashJoinTdb.instRowForRightJoinLength_);
  }

  // Allocate a NULL tuple for use in null instantiation.
  // Allocate the MinMax tuple if min/max optimazation is being used.
  if((nullLength > 0) || (hashJoinTdb.minMaxRowLength_ > 0)) {
    
    ULng32 tupleSize = MAXOF(nullLength, hashJoinTdb.minMaxRowLength_);
    ULng32 numTuples = 
      ((nullLength > 0) && (hashJoinTdb.minMaxRowLength_ > 0)) ? 2 : 1;
    
    Lng32 neededBufferSize = 
      (Lng32) SqlBufferNeededSize( numTuples, tupleSize);

    nullPool_ = new(space_) sql_buffer_pool(1, neededBufferSize, space_);

    if (nullLength > 0) {

      if (nullPool_->get_free_tuple(nullData_, nullLength)) {
        ex_assert(0, "ex_hashj_tcb No space for null tuple");
      }

      // Fill tuple with NULL values.
      str_pad(nullData_.getDataPointer(), nullLength, '\377');
    }


    if (hashJoinTdb.minMaxRowLength_ > 0) {
      
      // Allocate ATP used to pass request (including min/max value)
      // to left child.
      toLeftChildAtp_ = allocateAtp(hashJoinTdb.leftDownCriDesc_, space_);

      // Allocate tuple used to compute min/max values.
      if (nullPool_->get_free_tuple(workAtp_->getTupp(hashJoinTdb.minMaxValsAtpIndex_),
                                    (Lng32) hashJoinTdb.minMaxRowLength_)) {
        ex_assert(0, "ex_hashj_tcb No space for minmax tuple");
      }

      // Initial min/max tuple to all null values.
      str_pad(workAtp_->getTupp(hashJoinTdb.minMaxValsAtpIndex_).getDataPointer(),
              (Lng32) hashJoinTdb.minMaxRowLength_,
              '\377');
    }

  }



};

///////////////////////////////////////////////////////////////////////////////
// Destructor for hashj_tcb
///////////////////////////////////////////////////////////////////////////////

ex_hashj_tcb::~ex_hashj_tcb() {
  freeResources();
  
  if (workAtp_)
    deallocateAtp(workAtp_, space_);
  
  workAtp_ = NULL;
  
  if (clusterDb_) {
    delete clusterDb_;
    clusterDb_ = NULL;
  };

  // delete the buckets
  if (buckets_) {
    heap_->deallocateMemory(buckets_);
    buckets_ = NULL;
  };
  delete heap_;
};
  
///////////////////////////////////////////////////////////////////////////////
// Free Resources
///////////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::freeResources() {
  if ( isReuse() ) {
    delete inputPool_;
    inputPool_ = 0;
  }

  if (nullPool_) {
    delete nullPool_;
    nullPool_ = NULL;
  }

  delete parentQueue_.up;
  delete parentQueue_.down;
};

///////////////////////////////////////////////////////////////////////////////
// release all result tupps in the workAtp_
///////////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::releaseResultTupps() {
  if (hashJoinTdb().leftRowLength_)
    workAtp_->getTupp(hashJoinTdb().leftRowAtpIndex_).release();

  if (hashJoinTdb().rightRowLength_)
    workAtp_->getTupp(hashJoinTdb().rightRowAtpIndex_).release();

  if (isLeftJoin() && leftJoinExpr_)
    workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_).release();

  if (isRightJoin() && rightJoinExpr_)
    workAtp_->getTupp(hashJoinTdb().instRowForRightJoinAtpIndex_).release();

};

///////////////////////////////////////////////////////////////////////////////
// insert a result row into the parents queue
// if dataPointer == NULL, we handle a null-ext left join or an AntiSemiJoin
///////////////////////////////////////////////////////////////////////////////
short ex_hashj_tcb::insertResult(HashRow * hashRow) {
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  atp_struct * downParentEntryAtp = downParentEntry->getAtp() ;
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);
  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  //ex_expr::exp_return_type retCode;

  atp_struct * leftRowAtp =
    ((pstate.getPhase() == PHASE_2) ?
     leftQueue_.up->getHeadEntry()->getAtp() : workAtp_);

  // first, we have to copy the input to this node
  upParentEntry->copyAtp(downParentEntryAtp);

  atp_struct * atp = upParentEntry->getAtp();

  // Allocate the tuples as they are needed so that they can be resized if needed.

  // if we are in phase 2, we get the left row directly from the queue,
  // otherwise we get it from a hash buffer

  if (hashJoinTdb().leftRowLength_) {
    if (pstate.getPhase() == PHASE_2) {
      
      SqlBufferHeader::moveStatus ms = 
        resultPool_->moveIn(leftRowAtp, 
                            workAtp_,
                            hashJoinTdb().leftRowAtpIndex_,
                            hashJoinTdb().leftRowLength_,
                            leftMoveExpr_,
                            true,
                            hashJoinTdb().bufferSize_);

      if (ms == SqlBufferHeader::MOVE_ERROR)
        {
          processError(leftRowAtp);
          return 1;
        }
    }
    else {
      
      char *rowPointer = 
        leftRowAtp->getTupp(hashJoinTdb().extLeftRowAtpIndex_).getDataPointer();

      UInt32 leftRowLength = hashJoinTdb().leftRowLength_;

      if(hashJoinTdb().useVariableLength()) {
        
        // For variable length rows, the row length is stored in the
        // first 4 bytes of the row
        leftRowLength = *((UInt32 *)rowPointer);
      }

      resultPool_->moveIn(workAtp_,
                          hashJoinTdb().leftRowAtpIndex_,
                          leftRowLength,
                          rowPointer,
                          true,
                          hashJoinTdb().bufferSize_);

    }
    // and assign the tupp to the parents queue entry
    atp->getTupp(hashJoinTdb().returnedLeftRowAtpIndex_) =
      workAtp_->getTupp(hashJoinTdb().leftRowAtpIndex_);    
  } // (hashJoinTdb().leftRowLength_);

  // Make the left row available at 
  // returnedInstRowForRightJoinAtpIndex_, so that the parent 
  // node can gets it. Also handle rightJoinExpr for Full Outer Joins.
  if (isRightJoin() && rightJoinExpr_)
    {
      SqlBufferHeader::moveStatus ms = 
        resultPool_->moveIn(downParentEntryAtp,
                            workAtp_,
                            hashJoinTdb().instRowForRightJoinAtpIndex_,
                            hashJoinTdb().instRowForRightJoinLength_,
                            rightJoinExpr_,
                            true,
                            hashJoinTdb().bufferSize_);

      if (ms == SqlBufferHeader::MOVE_ERROR)
        {
          processError(downParentEntryAtp);
          return 1;
        }

      // and add the row to the result.
      atp->getTupp(hashJoinTdb().returnedInstRowForRightJoinAtpIndex_) =
	workAtp_->getTupp(hashJoinTdb().instRowForRightJoinAtpIndex_);
    } // isRightJoin() &&  rightJoinExpr_

  // add the right row
  if (hashJoinTdb().rightRowLength_ && hashRow) {

    UInt32 rightRowLength = hashJoinTdb().rightRowLength_;

    if(hashRow && hashJoinTdb().useVariableLength()) {

      // get the actual length from the HashRow.
      rightRowLength = hashRow->getRowLength(); 
    }

    resultPool_->moveIn(workAtp_,
                        hashJoinTdb().rightRowAtpIndex_,
                        rightRowLength,
                        hashRow->getData(),
                        true,
                        hashJoinTdb().bufferSize_);

    atp->getTupp(hashJoinTdb().returnedRightRowAtpIndex_) =
      workAtp_->getTupp(hashJoinTdb().rightRowAtpIndex_);
  };

  // handle left joins
  if (isLeftJoin()) {
    if (hashRow) {

      // left join and we have a match, move instantiated row
      if (leftJoinExpr_) {

        SqlBufferHeader::moveStatus ms = 
          resultPool_->moveIn(downParentEntryAtp,
                              workAtp_,
                              hashJoinTdb().instRowForLeftJoinAtpIndex_,
                              hashJoinTdb().instRowForLeftJoinLength_,
                              leftJoinExpr_,
                              true,
                              hashJoinTdb().bufferSize_);

        if (ms == SqlBufferHeader::MOVE_ERROR)
          {
            processError(downParentEntryAtp);
            return 1;
          }

	// add the row 
	atp->getTupp(hashJoinTdb().returnedInstRowForLeftJoinAtpIndex_) =
	  workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_);
      }
    }
    else {
      if (nullInstForLeftJoinExpr_) {

        // Use the pre-allocated nullData.

        // Don't allocate the row for left join
        // Will use the pre-allocated nullData instead
        workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_) = nullData_;


        // add the row 
        atp->getTupp(hashJoinTdb().returnedInstRowForLeftJoinAtpIndex_) =
          workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_);
      }
    }
  }

  upParentEntry->upState.status = ex_queue::Q_OK_MMORE;
  upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
  upParentEntry->upState.downIndex = parentQueue_.down->getHeadIndex();
  
  // we got another result row
  pstate.matchCount_++;
  upParentEntry->upState.setMatchNo(pstate.matchCount_);

  // move left and/or right warnings to parent
  if (pstate.accumDiags_)
    {
      ComDiagsArea *accumulatedDiagsArea = 
	upParentEntry->getDiagsArea();

      if (accumulatedDiagsArea)
	accumulatedDiagsArea->mergeAfter(*pstate.accumDiags_);
      else
	{
	  upParentEntry->setDiagsArea(pstate.accumDiags_);
	  
	  // incr ref count after set so it doesn't get deallocated.
	  pstate.accumDiags_->incrRefCount();
	}

      pstate.accumDiags_ = NULL;
    } // left or right child returned a warning

  // insert into parent up queue
  parentQueue_.up->insert();
  
  // update operator stats
  if (bmoStats_)
    bmoStats_-> incActualRowsReturned();
  
  // we can forget about the row in the workAtp
  releaseResultTupps();

  if (bmoStats_)
    bmoStats_->setSpaceBufferCount(resultPool_->get_number_of_buffers());
  return 0;
};

///////////////////////////////////////////////////////////////////////////////
// the join is canceled. Consume the rows from a queue
///////////////////////////////////////////////////////////////////////////////
NABoolean ex_hashj_tcb::consumeForCancel(ex_queue_pair q) {

  // loop forever. exit via return
  while (TRUE) {
    if (q.up->isEmpty())
      // queue is empty, but we are not done yet. Come back later
      return FALSE;
    else {
      ex_queue_entry * entry = q.up->getHeadEntry();
      switch (entry->upState.status) {
      case ex_queue::Q_SQLERROR:
      case ex_queue::Q_OK_MMORE: {
	// consume the row
	q.up->removeHead();
      }; break;
      case ex_queue::Q_NO_DATA: {
	// we are done with this queue
	return TRUE;
      }; break;
      case ex_queue::Q_INVALID: {
	ex_assert(0, "ex_hashj_tcb::consumeForCancel() invalid state returned by child");
      }; break;
      }; // switch
    }; // else
  }; // while
};

short ex_hashj_tcb::processError(atp_struct* atp)
{
  ex_assert( ! parentQueue_.down->isEmpty(),
	     "ex_hashj_tcb::processError() Unexpected empty parent down queue" );
 
  ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
  ex_queue_entry *upEntry = parentQueue_.up->getTailEntry();
  ex_hashj_private_state &  pstate = 
	  *((ex_hashj_private_state*) pEntryDown->pstate);

  upEntry->copyAtp(atp);
  upEntry->upState.status      = ex_queue::Q_SQLERROR;
  upEntry->upState.downIndex = parentQueue_.down->getHeadIndex();
  upEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
  upEntry->upState.setMatchNo(pstate.matchCount_);

  // insert into parent
  parentQueue_.up->insert();

  leftQueue_.down->cancelRequestWithParentIndex(parentQueue_.down->getHeadIndex());
  rightQueue_.down->cancelRequestWithParentIndex(parentQueue_.down->getHeadIndex());
  pstate.setState(HASHJ_CANCELED);

  return 0;
};

ExWorkProcRetcode ex_hashj_tcb::work()
{
  ex_assert(0,"Should never call ex_hashj_tcb::work()");
  return WORK_OK;
}


///////////////////////////////////////////////////////////////////////////////
// workUp()
///////////////////////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_hashj_tcb::workUp() {

  // Check if there is still work to do
  if (parentQueue_.down->isEmpty())
    return WORK_OK;

  // A hybrid hash join never works on more than one parent request at a time
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();
  const ex_queue::down_request & request = downParentEntry->downState.request;
  ex_hashj_private_state &  pstate = 
	  *((ex_hashj_private_state*) downParentEntry->pstate);
  
  ULng32 xProductPreemptMax = 
    hashJoinTdb().getXproductPreemptMax() ? hashJoinTdb().getXproductPreemptMax() :
                                            UINT_MAX;
  numJoinedRowsRejected_ = 0;

  // loop forever, exit via return
  while (TRUE) {
    // if we have already given to the parent all the rows needed then cancel
    // the parent's request.
    if ((request == ex_queue::GET_NOMORE ||
	 request == ex_queue::GET_N && 
	 downParentEntry->downState.requestValue <= (Lng32) pstate.matchCount_) 
	&&  // and not canceled or done yet
	pstate.getState() != HASHJ_CANCELED &&
	pstate.getOldState() != HASHJ_CANCEL_AFTER_INNER &&
	pstate.getState() != HASHJ_DONE ) 
      {
	propagateCancel(parentQueue_.down->getHeadIndex(), pstate);
      }
    switch (pstate.getState()) {
    case HASHJ_READ_INNER:
    case HASHJ_CANCEL_AFTER_INNER: {
      if ( ! pstate.usingPreviousHT() && ! pstate.getHaveClusters() ) {
          if ( allocateClusters() ) {
             pstate.setState(HASHJ_ERROR);
             break;
          }
          pstate.setHaveClusters(TRUE); // now the clusters are allocated
      }

      if (rightQueue_.up->isEmpty() || 
          parentQueue_.up->isFull()) {
	return WORK_OK;
      };

      if (workReadInner())
        return WORK_OK;
    } break;
    case HASHJ_END_PHASE_1: {
      workEndPhase1();
      if ( pstate.getState() == HASHJ_END_PHASE_1 )
	return WORK_CALL_AGAIN; // more work, but let others run too
    } break;
    case HASHJ_READ_OUTER: {
      if (leftQueue_.up->isEmpty() || 
          parentQueue_.up->isFull()) {
	return WORK_OK;
      };
      // Query limits & suspend
      if (numJoinedRowsRejected_ > xProductPreemptMax)
          return WORK_HASHJ_PREEMPT;

      if (workReadOuter())
        return WORK_OK;
    } break;
    case HASHJ_END_PHASE_2: {
      workEndPhase2();
    } break;
    case HASHJ_FLUSH_CLUSTER: {
      workFlushCluster();
      if (pstate.getState() == HASHJ_FLUSH_CLUSTER) {
	// the cluster is not completely flushed. An I/O is pending
          return WORK_CALL_AGAIN; // $$$$ add I/O event, change to WORK_OK
      };
    } break;
    case HASHJ_READ_INNER_CLUSTER: {
      workReadInnerCluster();
      if (pstate.getState() == HASHJ_READ_INNER_CLUSTER) {
	// the cluster is not completely read. An I/O is pending

        return WORK_CALL_AGAIN; // $$$$ add I/O event, change to WORK_OK
      };
    } break;
    case HASHJ_READ_OUTER_CLUSTER: {
      // Query limits & suspend
      if (numJoinedRowsRejected_ > xProductPreemptMax)
          return WORK_HASHJ_PREEMPT;

      workReadOuterCluster();
    } break;
    case HASHJ_READ_BUFFER: {
      workReadBuffer();
      if (pstate.getState() == HASHJ_READ_BUFFER) {
	// the buffer is not read yet; the I/O is still pending
        return WORK_CALL_AGAIN;// $$$$ add I/O event, change to WORK_OK

      };
    } break;

    case HASHJ_RETURN_RIGHT_ROWS: {
      // Make sure that we have a free entry in the parents up queue.
      if (parentQueue_.up->isFull()) {
	// no free entry, come back later
	return WORK_OK;
      };
      
      if (workReturnRightRows())
        return WORK_OK;
    } break;

    case HASHJ_PROBE: {
      // this routine could potentially give a result row. Make sure
      // that we have a free entry in the parents up queue.
      if (parentQueue_.up->isFull()) {
	// no free entry, come back later
	return WORK_OK;
      };

      if (workProbe())
        return WORK_OK;
    } break;
    case HASHJ_DONE: {
      // make sure that we have a free slot in the parent's up queue
      if (parentQueue_.up->isFull()) {
	return WORK_OK;
      };
      workDone();

      // That's it, need to be called again if there are more requests
      // We need to schedule workDown from workUp when workUp processes
      // the last request it can but there's still some workDown can use.

      if (parentQueue_.down->isEmpty())
        return WORK_OK;
      else
        return WORK_CALL_AGAIN;
    } break;
    case HASHJ_CANCELED: {
      // the request was canceled. Both children were sent cancel requests.
      // Consume all up rows from the children and wait for Q_NO_DATA.
      // (Not needed if the left request was never sent).
      NABoolean leftDone = 
	pstate.delayedLeftRequest() ? TRUE : consumeForCancel(leftQueue_);

      // only check the right queue if this request built its own hash table 
      // (not reusing)
      NABoolean rightDone = 
	pstate.usingPreviousHT() ? TRUE : consumeForCancel(rightQueue_) ;

      if (!(leftDone && rightDone))
	// we are not done yet, come back again after queue inserts
	return WORK_OK;

      // all rows are consumed, we are done
      pstate.setState(HASHJ_DONE);
    } break;
    case HASHJ_ERROR: {
      // make sure that we have a free slot in the parent's up queue
      if (parentQueue_.up->isFull()) {
	return WORK_OK;
      };

      ex_assert( rc_ , "Missing error code"); 
      // we ran into a serious runtime error. Create Condition and
      // pass it to parent. rc_ has the error code and
      // oldState_ tells us in which state the error occurred
      ComDiagsArea *da = downParentEntry->getDiagsArea();
      if (!da  ||  !da->contains((Lng32) -rc_))
        {
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

              str_sprintf(msg, "Hash Join Scratch IO Error occurred. Scratch Error: %d, System Error: %d, System Error Detail: %d, Details: %s",
		          scratchError, scratchSysError, scratchSysErrorDetail, errorMsg);

              diags = ExRaiseSqlError(heap_, downParentEntry,
	                              (ExeErrorCode) -rc_,
                                      NULL,
                                      msg);
              
            }
          else
#endif
            diags = ExRaiseSqlError(heap_, downParentEntry,
	                           (ExeErrorCode) -rc_);


          downParentEntry->setDiagsArea(diags);
        }
      processError(downParentEntry->getAtp());
    } break;
    }
  }
};


///////////////////////////////////////////////////////////////////////////
// isSameInputAgain()
//   Check if the old hash table (from the previous input) can be reused
//   That is, match the current input with the previous input
///////////////////////////////////////////////////////////////////////////

NABoolean ex_hashj_tcb::isSameInputAgain( ex_queue_entry * downParentEntry ) {

  if ( havePrevInput_ == FALSE )  return FALSE;

  // insert the previous input into the appropriate tupp in the workAtp
  workAtp_->getTupp(hashJoinTdb().prevInputTuppIndex_) = prevInputValues_;
  
  NABoolean haveMark = FALSE;
  Lng32 oldDiagsAreaMark = 0;
  ComDiagsArea * da = downParentEntry->getAtp()->getDiagsArea();
  if ( da != NULL ) {
    oldDiagsAreaMark = da->mark();
    haveMark = TRUE;
  }
  
  // match the current input with the previous
  switch ( checkInputPred_->eval(downParentEntry->getAtp(), workAtp_ ) ) {
  case ex_expr::EXPR_TRUE :
    // the new input matches the previous --> reuse existing hash table
    return TRUE; // this HT is a reuse of a previous one
    break;
    
  case ex_expr::EXPR_ERROR :
    // expression evaluation failed; treat this like EXPR_FALSE -> no reuse
    
    // clean diags area (so this error won't show)
    if ( haveMark ) {
      downParentEntry->getAtp()->getDiagsArea()->rewind(oldDiagsAreaMark, TRUE);
    } else {
      downParentEntry->getAtp()->getDiagsArea()->clear();
    }
    break;
    
  case ex_expr::EXPR_FALSE :
    break;
    
  default:
    ex_assert(0, "ex_hashj_tcb::isSameInputAgain() Invalid expr evaluation.");
  }
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
// workDown()
//   Send the GET_ALL request to the left and possibly right child queue.
///////////////////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_hashj_tcb::workDown() {

  queue_index queue_tail = parentQueue_.down->getTailIndex();
  while (nextRequest_ != queue_tail ) {
    
    // we just received a new entry from our parent, start working
    ex_queue_entry * downParentEntry = 
      parentQueue_.down->getQueueEntry(nextRequest_);
    ex_hashj_private_state &  pstate =
      *((ex_hashj_private_state*) downParentEntry->pstate);
    
    NABoolean will_reuse_old_HT = FALSE;
    ex_queue_entry * leftEntry = NULL;

    switch ( downParentEntry->downState.request ) {
    case ex_queue::GET_N:
    case ex_queue::GET_ALL:

      if (rightQueue_.down->isFull() || leftQueue_.down->isFull() ) {
        // we can count on being scheduled again on queues unblocking
        return WORK_OK;
      }

      if ( isReuse() ) {
	// If new input matches the previous input, then reuse the old hash-table
	if ( will_reuse_old_HT = isSameInputAgain(downParentEntry) ) {
	  // the hash table is already prepared. Go on with phase 2 of the join
	  pstate.setPhase(PHASE_2, bmoStats_);
	  pstate.usePreviousHT();
	  pstate.reliesOnIdx = prevReqIdx_;
	  pstate.setState(HASHJ_READ_OUTER); // all set to start reading the outer
	}
	else { 
	  // can't reuse old hash table; need to build a new one
	  // remember the current input for next check.
	  if ( moveInputExpr_ ) {
	    havePrevInput_ = TRUE;
	    prevReqIdx_ = nextRequest_;
	  
	    // insert the previous input into the appropriate tupp in this atp
	    workAtp_->getTupp(hashJoinTdb().prevInputTuppIndex_) = prevInputValues_;
	  
	    // copy input into prevInputValues_ 
	    if ( moveInputExpr_->eval(downParentEntry->getAtp(), workAtp_ ) != 
		 ex_expr::EXPR_OK ) {
	      // a move expr should never fail; but if it does -- blow up this query
	      ex_assert(0, "ex_hashj_tcb::workDown() moveInputExpr failed");
	    }
	  }
	}
      } // if (isReuse())

      // Either no reuse or reuse with new input -- Start phase 1
      if ( ! will_reuse_old_HT ) {

	pstate.setState(HASHJ_READ_INNER);  // phase 1 
	pstate.reliesOnIdx = nextRequest_;
	
	// pass GET ALL request to right child as well. This is done
	// because all rows are needed (in worst case) to find even
	// one matching row.
	ex_queue_entry * rightEntry = rightQueue_.down->getTailEntry();
        
	rightEntry->downState.request = ex_queue::GET_ALL;
	rightEntry->downState.requestValue =
	  downParentEntry->downState.requestValue;
	rightEntry->downState.parentIndex = nextRequest_;
	rightEntry->passAtp(downParentEntry);
	rightQueue_.down->insert();
	
	// Build the hash table only if we "caught up" and
	// processed the head entry (without reuse)
	if (nextRequest_ == parentQueue_.down->getHeadIndex()) {
          if(hashJoinTdb().isUniqueHashJoin()) {
            // If this is a Unique Hash Join, allocate the hash table
            // structures, rather than the clusters.
            //
            ExUniqueHashJoinTcb *uHashJoinTcb = (ExUniqueHashJoinTcb *)this;
            if (uHashJoinTcb->allocateBufferHeap())
              pstate.setState(HASHJ_ERROR);
          } else {
            if (allocateClusters()) pstate.setState(HASHJ_ERROR);
          }
	  pstate.setHaveClusters(TRUE); // now the clusters are allocated
	}
      } // if (!will_reuse_old_HT)

      if ( isAllOrNothing_ ) {  // Special case: AntiSemi HJ, no search expr
	if ( will_reuse_old_HT ) {
	  // Even though we reuse, still need to finish up few things before
	  // phase two; e.g., handle delayed or non-delayed left request
	  pstate.setState(HASHJ_END_PHASE_1);
	  pstate.setPhase(PHASE_1, bmoStats_);  // needed ?  just in case ...
	}
	else { // no reuse
	  // Change the request: only one row from the right child is needed
	  ex_queue_entry * rightEntry = rightQueue_.down->getTailEntry();
	  rightEntry->downState.request = ex_queue::GET_N;
	  rightEntry->downState.requestValue = 1;
	}
	
	// Wait to find about right rows; don't send request to the left yet !!
	if ( hashJoinTdb().delayLeftRequest() ) {
	  pstate.setDelayedLeftRequest();

          // No requests were sent to either child, so need to reschedule
          // to allow workUp to finish this request.
          workUpTask_->schedule();   
	  break;
	}
      } // if (isAllOrNothing_)

      // If this HashJoin is doing min/max optimization, then we must
      // delay the left request.  The request will be sent during
      // workEndPhase1() after reading all rows from the right child
      // and after having computed the min and max values which will
      // be sent with the request to the left child.
      if (minMaxExpr_ && hashJoinTdb().delayLeftRequest() ) {
        pstate.setDelayedLeftRequest();
      } else {
        // pass GET_ALL request to the left child
        leftEntry = leftQueue_.down->getTailEntry();
        leftEntry->downState.request = ex_queue::GET_ALL;
        leftEntry->downState.requestValue = downParentEntry->downState.requestValue;
        leftEntry->downState.parentIndex = nextRequest_;
        leftEntry->passAtp(downParentEntry);
        leftQueue_.down->insert();
      }
       
      // set the space buffer size and initial count whenever ALL request is sent
      if (bmoStats_)
      { 
        ex_hashj_tdb *hashjTdb = (ex_hashj_tdb *)getTdb();
        bmoStats_->setSpaceBufferSize(hashjTdb->bufferSize_);
        bmoStats_->setSpaceBufferCount(resultPool_->get_number_of_buffers());
      }
      break;
      
    case ex_queue::GET_NOMORE:
      pstate.setState(HASHJ_DONE);   // We are done before we started.
      workUpTask_->schedule();       // Make sure workUp completes the request
      
      break;
      
    default:
      ex_assert(0, "ex_hashj_tcb::workDown() Invalid request.");
      break;

    } // switch (downParentEntry->downState.request)

    nextRequest_++;

  } // while (nextRequest_ != queue_tail)

  return WORK_OK ;
}

///////////////////////////////////////////////////////////////////////////
// allocateClusters()
//   Initialize the data structures for the hybrid hash join
///////////////////////////////////////////////////////////////////////////
NABoolean ex_hashj_tcb::allocateClusters() {

  if ( haveAllocatedClusters_ ) {
  
    // We can only get here when reusing (a previous HT), but the old hash table
    // data is stale, thus need to clean up that old memory
    if (clusterDb_) {
      // Yield all surplus memory quota (i.e., above the minimum quota) back to 
      // the global count of unused memory, so that other BMOs may use this quota
      clusterDb_->yieldAllMemoryQuota(); 

      delete clusterDb_;
      clusterDb_ = NULL;
    };
      
    // delete the buckets
    if (buckets_) {
      heap_->deallocateMemory(buckets_);
      buckets_ = NULL;
    };
  }

  // We use memUsagePercent of the physical memory for the hash join.
  ULng32 availableMemory = memMonitor_->getPhysMemInBytes() / 100
      * hashJoinTdb().memUsagePercent_;

  // if quota, and it's less than avail memory, then use that lower figure 
  if ( hashJoinTdb().memoryQuotaMB() > 0 &&
       hashJoinTdb().memoryQuotaMB() * ONE_MEG < availableMemory )
    availableMemory = hashJoinTdb().memoryQuotaMB() * ONE_MEG ;

  // size of inner table (including row headers and hash chains) in bytes
  // This may be a very large number, max out at 8 GB and take at
  // least 100 KB. Don't completely trust the optimizer ;-)
  // (With available memory usually at 100MB, that's a max of 80 clusters).
  NAFloat innerTableSizeF = hashJoinTdb().innerExpectedRows() *
    (NAFloat)(hashJoinTdb().extRightRowLength_ + sizeof(HashTableHeader));
  Int64 innerTableSize;
  if (innerTableSizeF > MAX_INPUT_SIZE ) innerTableSize = MAX_INPUT_SIZE ; 
  else
    innerTableSize =
      MAXOF( MIN_INPUT_SIZE, (Int64)innerTableSizeF);

  // we start with 4 buckets per cluster
  ULng32 bucketsPerCluster = 4;

  ULng32 noOfClusters = 0; 

  // Cross Product needs only one cluster / one bucket
  if ( rightSearchExpr_ == NULL ) {
    bucketsPerCluster = 1;
    noOfClusters = 1;
  }
  else if ( hashJoinTdb().isUniqueHashJoin() ) {} // UHJ uses zero clusters
  else {

    // required number of buffers for inner table
    ULng32 totalInnerBuffers = 
      (ULng32)(innerTableSize/hashJoinTdb().hashBufferSize_);
    if (innerTableSize % hashJoinTdb().hashBufferSize_)
      totalInnerBuffers++;

    // total number of buffers available
    ULng32 totalBuffers = availableMemory/hashJoinTdb().hashBufferSize_;

    noOfClusters = totalInnerBuffers/totalBuffers;

    if (totalInnerBuffers % totalBuffers)
      noOfClusters++;

    // Aim at an average cluster's size to be a quarter of the available memory
    // to leave some slack for underestimating or data skews
    noOfClusters *= 4;

    // Let the compiler force a higher number of clusters
    noOfClusters = MAXOF(noOfClusters, hashJoinTdb().numClusters() );

    // round it up to the nearest prime number (2,3,5,7,11,....)
    noOfClusters = ClusterDB::roundUpToPrime(noOfClusters);

    // the extreme case, each cluster has only one bucket and only one buffer
    ULng32 maxNoOfClusters = totalBuffers/bucketsPerCluster;
    noOfClusters = MINOF(noOfClusters, maxNoOfClusters);
  }

  // total number of buckets
  bucketCount_ = bucketsPerCluster * noOfClusters;

  // allocate the buckets		     
  ULng32 bucketIdx = 0;
  
  if ( bucketCount_ )
    buckets_ = (Bucket*)heap_->allocateMemory(bucketCount_ * sizeof(Bucket));
  
  for(bucketIdx = 0; bucketIdx < bucketCount_; bucketIdx++)
    buckets_[bucketIdx].init();


  ULng32 minMemQuotaMB = hashJoinTdb().isPossibleMultipleCalls() ?
    hashJoinTdb().memoryQuotaMB() : 0 ;

  ULng32 minB4Chk = hashJoinTdb().getBmoMinMemBeforePressureCheck() * ONE_MEG;
  // estimate memory needed in phase 1 (not incl. hash tables)
  Float32 memEstInMbPerCpu = (Float32)(innerTableSize / ONE_MEG) ;

  // Only set cross product optimizations on when there is no
  // right search expression 
  ClusterDB::HashOperator hashOperator = 
    rightSearchExpr_ == NULL ? ClusterDB::CROSS_PRODUCT : 
    hashJoinTdb().isUniqueHashJoin() ? ClusterDB::UNIQUE_HASH_JOIN :
    hashJoinTdb().isNoOverflow() ? ClusterDB::ORDERED_HASH_JOIN :
    ClusterDB::HYBRID_HASH_JOIN ;

  clusterDb_ = new(heap_) ClusterDB(hashOperator,
				    hashJoinTdb().hashBufferSize_,
				    workAtp_,
                                    tdb.getExplainNodeId(),
				    hashJoinTdb().extRightRowAtpIndex1_,
				    hashJoinTdb().extRightRowAtpIndex2_,
				    rightSearchExpr_,
				    buckets_,
				    bucketCount_,
				    availableMemory,
				    memMonitor_,
				    hashJoinTdb().pressureThreshold_,
				    getGlobals()->castToExExeStmtGlobals(),
				    &rc_,
				    hashJoinTdb().isNoOverflow(),
				    FALSE, /*isPartialGroupBy*/
				    hashJoinTdb().minBuffersToFlush_,
				    hashJoinTdb().numInBatch_,

				    hashJoinTdb().forceOverflowEvery(),
				    hashJoinTdb().forceHashLoopAfterNumBuffers()
				    ,hashJoinTdb().forceClusterSplitAfterMB(),
				    
				    ioEventHandler_,
				    this, // the calling tcb
				    hashJoinTdb().scratchThresholdPct_,

				    hashJoinTdb().logDiagnostics(),
				    hashJoinTdb().bufferedWrites(),
				    hashJoinTdb().disableCmpHintsOverflow(),
				    hashJoinTdb().memoryQuotaMB(),
				    minMemQuotaMB,
				    minB4Chk, // BmoMinMemBeforePressureCheck
				    hashJoinTdb().getBmoCitizenshipFactor(),
				    hashJoinTdb().getMemoryContingencyMB(),
				    // to estimate the error penalty
				    hashJoinTdb().hjGrowthPercent(),
				    memEstInMbPerCpu, // estimate mem needed

				    0,    // Hash-Table not resizable
				    getStatsEntry()
				    );

  if ( !clusterDb_ || rc_ != EXE_OK ) {
    if ( !clusterDb_ ) rc_ = EXE_NO_MEM_TO_EXEC;  // allocate ClusterDB failed
    else delete clusterDb_ ;
    return(TRUE);
  };

  clusterDb_->setScratchIOVectorSize(hashJoinTdb().scratchIOVectorSize());
  switch(hashJoinTdb().getOverFlowMode())
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
  clusterDb_->setBMOMaxMemThresholdMB(hashJoinTdb().getBMOMaxMemThresholdMB());
#endif 
  bucketIdx = 0;
  Cluster * cluster = NULL;
  ULng32 i;
  // allocate the (inner) clusters
  for (i = 0; i < noOfClusters; i++) {
    cluster = new(heap_) Cluster(Cluster::IN_MEMORY,
				 clusterDb_,
				 &buckets_[bucketIdx], 
				 bucketsPerCluster,
				 hashJoinTdb().extRightRowLength_,
                                 hashJoinTdb().useVariableLength(),
                                 hashJoinTdb().considerBufferDefrag(),
				 hashJoinTdb().extRightRowAtpIndex1_,
				 TRUE,
				 FALSE, // for now only left joins
				 cluster,
				 &rc_);

    if ( !cluster || rc_ != EXE_OK ) {
      if ( !cluster ) rc_ = EXE_NO_MEM_TO_EXEC; // could not allocate Cluster
      else delete cluster;
      return(TRUE);
    };

    bucketIdx += bucketsPerCluster;
  };
  // store head of the cluster list in the clusterDb_
  clusterDb_->setClusterList(cluster);


  haveAllocatedClusters_ = TRUE;

  return(FALSE);
};

//////////////////////////////////////////////////////////////
//
// isFirst
//
// Determines if a request is the first in a series, the hash
// table to be reused by subsequent requests.
//
// KBC: should we also look to see that the next request
//      isn't canceled too?  (change to a loop, stop
//      on the first non-cancelled with our reliesOnIdx -> true
//      or first reliesOnIdx != ix -> false)
//
//////////////////////////////////////////////////////////////

NABoolean ex_hashj_tcb::isFirst(queue_index ix,
                            ex_hashj_private_state &pstate) {
    return ((pstate.reliesOnIdx == ix) && 
            (parentQueue_.down->entryExists(ix+1)) &&
            (((ex_hashj_private_state*)parentQueue_.down->
                 getQueueEntry(ix+1)->pstate)->reliesOnIdx == ix));
}

//////////////////////////////////////////////////////////////
//
// isLast
//
// Determines if a request is the last in a series (or a
// singleton)
//
//////////////////////////////////////////////////////////////

NABoolean ex_hashj_tcb::isLast(queue_index ix,
                            ex_hashj_private_state &pstate) {
    return ( (! parentQueue_.down->entryExists(ix+1)) ||
            (((ex_hashj_private_state*)parentQueue_.down->
       getQueueEntry(ix+1)->pstate)->reliesOnIdx != pstate.reliesOnIdx));
}

//////////////////////////////////////////////////////////////
//
// propagateCancel
//
// propagate the cancellation according to the state of the
// request and reuse situation
//
//////////////////////////////////////////////////////////////
void ex_hashj_tcb::propagateCancel(queue_index ix, 
                            ex_hashj_private_state &pstate) {

   // This wasn't started... Just end it
   if (pstate.getState() == HASHJ_EMPTY)
      pstate.setState(HASHJ_DONE);
   else {
     // Propagate cancel down to the left
     leftQueue_.down->cancelRequestWithParentIndex(ix);
     pstate.setState(HASHJ_CANCELED);  // Middle, last or not reuse

     if(pstate.getPhase() == PHASE_1 ) {
       if ( isReuse() ) {
         if ( isFirst(ix, pstate)) {
           // If first, then we can't cancel the right request, since
           // subsequent requests might expect it.
           pstate.setState(HASHJ_CANCEL_AFTER_INNER);

         } else if ( isLast(ix,pstate) ) {
           // Singletons and last entries are done, and can
           // finish the cancel by propagating to the right
   
           if(pstate.usingPreviousHT()) {
             if(pstate.delayedLeftRequest()) {
               // In the case when we are reusing the previous HT and we
               // have delayed the left request and are still in PHASE1,
               // then we have not sent a request to either child.  So we
               // do not expect any activity on our child queues and
               // therefore we would not be rescheduled.  Force a
               // reschedule so that the HJ can complete the cancel and
               // send the Q_NO_DATA to the parent.
               pstate.setState(HASHJ_DONE);
               workUpTask_->schedule();
             }
           } else {
             // If we are still in PHASE_1 and we have an outstanding
             // request to the right child, then send cancel to right.
             rightQueue_.down->cancelRequestWithParentIndex(ix);

             // Since we cancelled the build of the HT, it can no longer
             // be used by subsequent requests.
             havePrevInput_ = FALSE;
           }
            
         }
       } else {
         rightQueue_.down->cancelRequestWithParentIndex(ix);
       }
     }
   }
}

//////////////////////////////////////////////////////////////
//
// Work routine for cancel processing for class ex_hashj_tcb
//
//////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_hashj_tcb::workCancel() {
  queue_index ix = parentQueue_.down->getHeadIndex();

  // Loop over all requests that have been sent down.
  while (ix != nextRequest_ )
  {
    ex_queue_entry* downParentEntry = parentQueue_.down->getQueueEntry(ix);
    const ex_queue::down_request & request = downParentEntry->downState.request;

    if (request == ex_queue::GET_NOMORE) {
      ex_hashj_private_state &  pstate =
           *((ex_hashj_private_state*) downParentEntry->pstate);

      if ( pstate.state_ != HASHJ_CANCELED && pstate.state_ != HASHJ_DONE )
            propagateCancel(ix, pstate);
    }
    ix++;
  }
  return WORK_OK;
}

///////////////////////////////////////////////////////////////////////////
// read a row from the right child and store it in the appropriate
// bucket/cluster
///////////////////////////////////////////////////////////////////////////
short ex_hashj_tcb::workReadInner() {
  ex_queue_entry * rightEntry = rightQueue_.up->getHeadEntry();
  ex_expr::exp_return_type retCode;

  ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate = 
    *((ex_hashj_private_state*) pEntryDown->pstate);

  switch (rightEntry->upState.status) {
  case ex_queue::Q_OK_MMORE: {

    if ( isAllOrNothing_ ) {  // Special case: AntiSemi HJ, no search expr
      pstate.setReadRightRow(); // this request actualy read a row from the right
      anyRightRowsRead_ = TRUE ; // Got a right row --> left rows are not needed !!
      rightQueue_.up->removeHead(); // remove that row from the right upqueue
      break;
    }
 
    isInnerEmpty_ = FALSE;

    if (pstate.getOldState() != HASHJ_FLUSH_CLUSTER) {
      // Calculate the hash value for the right row. If the oldstate_ is
      // HASHJ_FLUSH_CLUSTER, we tried to insert a row and had to flush a
      // cluster. The cluster is now flushed, but the row is not inserted
      // yet. Thus, in this case the hash value is still available and we
      // don't have to recalculate it.
      if (! rightHashExpr_)
        hashValue_ = 666;
      else
        {
          // If we are doing MIN/MAX optimization, then accumulate the
          // Min and Max values from the inner row.  Results are
          // accumulated in the min/max tuple of the workAtp_ (Can we
          // combine this expression with the rightHashExpr_ to avoid
          // some common overhead?)
          if (minMaxExpr_) {
            retCode = minMaxExpr_->eval(rightEntry->getAtp(), workAtp_);
            if (retCode == ex_expr::EXPR_ERROR)
              {
                processError(rightEntry->getAtp());
                return 0;
              }
          }

          retCode = rightHashExpr_->eval(rightEntry->getAtp(), workAtp_);
          if (retCode == ex_expr::EXPR_ERROR)
            {
              processError(rightEntry->getAtp());
              return 0;
            }
          hashValue_ = hashValue_ & MASK31;	

          // checkInnerNullExpr_ used in the case of hash anti semi join (transformation 
          // of NOT IN subquery). The checkInnerNullExpr_ expression tests if the value of the 
          // right child is null. the first null encountered while reading
          // from the inner table causes the operation to be canceled. 
          // The value 666654765 is the hash 
          // value of NULL and is used here as an optimization so that we do not
          // execute the checkInnerNullExpr_ expression if we are sure that the inner value 
          // is not a NULL value
          if (checkInnerNullExpr_ && 
            hashValue_ == ExHDPHash::nullHashValue /*666654765*/)
          {
            retCode = checkInnerNullExpr_->eval(rightEntry->getAtp(), workAtp_);
            switch ( retCode)
            {
              case ex_expr::EXPR_ERROR : 
              {
                return processError(rightEntry->getAtp()) ;
              }
              case ex_expr::EXPR_TRUE : 
              {
                propagateCancel(parentQueue_.down->getHeadIndex(), pstate);
                return 0;
              }
              default: 
                break;
            }
          }
        }
    }
    // set oldState_; just in case we came in from HASHJ_FLUSH_CLUSTER
    pstate.setOldState(HASHJ_READ_INNER);

    // determine the bucket, where the row is stored
    ULng32 bucketId = hashValue_ % bucketCount_;

    ComDiagsArea * da = rightEntry->getDiagsArea();
    if (da)
      {
	if (pstate.accumDiags_)
	  pstate.accumDiags_->mergeAfter(*da);
	else
	  {
	    pstate.accumDiags_ = da;
	    da->incrRefCount();
	  }
      }

    // insert the row in this bucket
    if (buckets_[bucketId].insert(rightEntry->getAtp(),
				  rightMoveInExpr_,
				  hashValue_,
				  &rc_,
				  hashJoinTdb().isNoOverflow()  )) {
     
      // row is inserted, remove it from queue
      rightQueue_.up->removeHead();
    }
    else {
      if (rc_) {
        // the insert caused an error
        // copy the diags from right entry to parent down entry from which
        // the diags will be copied later during HASHJ_ERROR state to
        // parent up entry.
        ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
        pEntryDown->copyAtp(rightEntry);
	pstate.setState(HASHJ_ERROR);
	return 0;
      };
      // Do not remove the row but set the oldstate_ and state_ accordingly
      pstate.setState(HASHJ_FLUSH_CLUSTER);
    }
  } break;
  case ex_queue::Q_NO_DATA: {

    // if inner table is empty then set checkOuterNullExpr_ to NULL so that
    // it does not get execute in workReadOuter() and outer NULL values don't 
    // get discarded.
      // if (isInnerEmpty_)
      // {
      //   checkOuterNullExpr_ = NULL;
      // }
    
    currCluster_ = clusterDb_->getClusterList(); // first cluster to process
    pstate.setState(HASHJ_END_PHASE_1);

    if ( isAllOrNothing_ ) {  // Special case: AntiSemi HJ, no search expr
      if ( ! pstate.readRightRow() )   // if this request did not read rows then
	anyRightRowsRead_ = FALSE ;  // clear (a possibly previously set) flag !!
    }
  } break;
  case ex_queue::Q_SQLERROR:
    {
      processError(rightEntry->getAtp());
    } break;
  case ex_queue::Q_INVALID: {
    ex_assert(0,
	      "ex_hashj_tcb::workReadInner() Invalid state returned by right child");
  } break;
  };
  return 0;
};

///////////////////////////////////////////////////////////////////////////
// flush a cluster
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workFlushCluster() {
  ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate = 
    *((ex_hashj_private_state*) pEntryDown->pstate);

  // maybe the state was changed to HASHJ_FLUSH_CLUSTER because of
  // a cluster split. In this case, there is no cluster to flush.
  if (clusterDb_->getClusterToFlush()) {
    if (clusterDb_->getClusterToFlush()->flush(&rc_)) {
      // all I/Os are done, go back to the state were we came from
      pstate.setState(pstate.getOldState());
      clusterDb_->setClusterToFlush(NULL);
    }
    else {
      if (rc_) {
	// flush caused an error
	pstate.setState(HASHJ_ERROR);
	return;
      }
    }
  }
  else {
    // no cluster to flush, return to the state where we came from
    pstate.setState(pstate.getOldState());
  }
};

///////////////////////////////////////////////////////////////////////////
// prepare clusters for phase 2 of the join
//
//  whenever this method returns without changing the state, it means that
//  we return to the scheduler to let other operators do some work. 
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workEndPhase1() {
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  queue_index parent_index = parentQueue_.down->getHeadIndex();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  if ( isAllOrNothing_ ) {  // Special case: AntiSemi HJ, no search expr
    if ( anyRightRowsRead_ ) { // got a right row -- all done, return nothing !
      if ( ! hashJoinTdb().delayLeftRequest() ) { // cancel left request
	leftQueue_.down->cancelRequestWithParentIndex(parent_index);
	pstate.setState(HASHJ_CANCELED); // cleanup left replies
	return;
      }
      pstate.setState(HASHJ_DONE); // this request returns nothing
      return;
    }
    else // no right rows -- need to return all left rows !!
      if ( hashJoinTdb().delayLeftRequest() ) { 
	// need to pass (a belated) GET_ALL request to the left child
	ex_queue_entry * leftEntry = leftQueue_.down->getTailEntry();
	leftEntry->downState.request = ex_queue::GET_ALL;
	leftEntry->downState.requestValue = downParentEntry->downState.requestValue;
	leftEntry->downState.parentIndex = parentQueue_.down->getHeadIndex();
	leftEntry->passAtp(downParentEntry);
	leftQueue_.down->insert();
	pstate.resetDelayedLeftRequest(); // delayed no more
	// From here on -- no special processing for "isAllOrNothing"
      }
  } // if allOrNothing_

  // Is this HashJoin doing MIN/MAX optimization.
  // If so, the left request was delayed until here.
  else if (minMaxExpr_ &&  pstate.delayedLeftRequest() ) {
    // We are doing min/max optimization and are done reading the rows
    // from the right child and have computed the min and max values.
    // Time to construct the request (including min max values) and
    // send it to the left child.

    // need to pass (a belated) GET_ALL request to the left child
    ex_queue_entry * leftEntry = leftQueue_.down->getTailEntry();
    leftEntry->downState.request = ex_queue::GET_ALL;
    leftEntry->downState.requestValue = downParentEntry->downState.requestValue;
    leftEntry->downState.parentIndex = parentQueue_.down->getHeadIndex();

    // Fill in the toLeftAtp with the tuples coming down from the
    // parent and with the computed min/max values.
    toLeftChildAtp_->copyAtp(downParentEntry->getAtp());

    toLeftChildAtp_->getTupp(hashJoinTdb().leftDownCriDesc_->noTuples()-1) =
      workAtp_->getTupp(hashJoinTdb().minMaxValsAtpIndex_);

    // Pass this ATP to the child.
    leftEntry->passAtp(toLeftChildAtp_);

    leftQueue_.down->insert();

    // delayed no more. From here on -- no special processing
    pstate.resetDelayedLeftRequest(); 
  }

  while (currCluster_) {
    switch (currCluster_->getState()) {
    case Cluster::FLUSHED: {
      if (currCluster_->getFirstBuffer() && currCluster_->getRowsInBuffer()) {
	// the cluster is in state FLUSHED but the buffer still holds
	// some rows. Flush it now.
	clusterDb_->setClusterToFlush(currCluster_);
	pstate.setState(HASHJ_FLUSH_CLUSTER);
	return;
      };
      // this inner cluster is all flushed; set up a matching outer cluster
      currCluster_->setUpOuter(hashJoinTdb().extLeftRowLength_,
			       hashJoinTdb().extLeftRowAtpIndex_,
			       // bitMapEnable : in the following 3 cases
			       isLeftJoin() ||isSemiJoin() ||isAntiSemiJoin());
    } break;
    case Cluster::IN_MEMORY: {
      if (currCluster_->getRowCount()) { // chain the cluster only if has rows

	// No-overflow would force chaining (i.e., skip memory pressure check
	// and return rc_ == EXE_NO_MEM_TO_EXEC if actual allocation failed.)
	if ( ! currCluster_->chain( hashJoinTdb().isNoOverflow(), &rc_)) {
	  if (rc_) {
	    // chain caused an error
	    pstate.setState(HASHJ_ERROR);
	    return;
	  };
	  // the cluster is in memory, but there is not enough space
	  // for the hash table. Flush the cluster.
	  clusterDb_->setClusterToFlush(currCluster_);
	  pstate.setState(HASHJ_FLUSH_CLUSTER);
	  return;
	};
	// if it was a big cluster; take a break, let other operators run
	if ( currCluster_->getRowCount() > 10000  
	     && currCluster_->getNext() ) // skip breaking for the last cluster
	  {
	    return; // go back to the scheduler with WORK_CALL_AGAIN
	  }
      }
    } break;
    case Cluster::CHAINED: {
      // the cluster is in memory and it is chained. nothing to do
    } break;
    };

    // The special case of a cluster flushed after being already chained
    // we only entered the loop to set up an outer cluster for this cluster.
    if ( flushedChainedCluster_ ) {
      flushedChainedCluster_ = FALSE;
      currCluster_ = NULL ;
      break;
    }

    // add this inner cluster's number of rows to the total
    totalRightRowsRead_ += currCluster_->getRowCount(); 
    currCluster_ = currCluster_->getNext();
  }; // while ( currCluster_ )

  // Handle efficiently the rare case of an empty inner table
  // don't cancel if the left side has an Insert/Update/Delete operation to complete
  if ( 0 == totalRightRowsRead_ &&   // was the inner table empty ?
       onlyReturnResultsWhenInnerMatched_ &&
       (!leftSideIUD()) ) 
    {
      // cancel request to outer; no rows would be returned
      leftQueue_.down->cancelRequestWithParentIndex(parent_index);
      pstate.setState(HASHJ_CANCELED); // this request returns nothing
      return;
    }

#if !defined(__EID)
  // Yield memory quota (or if needed, flush another in-memory cluster 
  // to free more memory for phase 2) .
  //    first get some information about the clusters (number, sizes, etc.)
  ULng32 numClusters = 0;
  ULng32 numFlushed = 0;
  ULng32 totalSize = 0;
  ULng32 maxSize = 0;
  ULng32 minSize = UINT_MAX;
  if ( clusterDb_->sawPressure() ) { // only if an overflow happened
    Cluster * currCl, * inMemoryCluster = NULL;
    for (numClusters = 0, numFlushed = 0 , totalSize = 0, 
	   maxSize = 0, minSize = UINT_MAX,
	   currCl = clusterDb_->getClusterList(); 
	 currCl ;
	 currCl = currCl->getNext(), ++numClusters ) 
      if ( currCl->isInner() ) {
	if ( currCl->getState() == Cluster::FLUSHED ) ++numFlushed ;
	// pick any in-memory cluster
	if ( currCl->getState() == Cluster::CHAINED ) inMemoryCluster = currCl;
	ULng32 clusterSizeMB = 
	  (ULng32) (currCl->clusterSize() / ONE_MEG ) ;
	totalSize += clusterSizeMB ;
	if ( maxSize < clusterSizeMB ) maxSize = clusterSizeMB ;
	if ( minSize > clusterSizeMB ) minSize = clusterSizeMB ;
      }
    // End of phase 1 - Calculate max memory quota needed, yield excess
    // if not enough comfortable spare memory is left for phase 2, 
    // then we flush an in-memory cluster to have more memory
    if ( clusterDb_->checkQuotaOrYield(numFlushed,maxSize) &&
	 inMemoryCluster ) { // if none, we'll do with whatever mem we have
      // Flush this chained cluster, and when we return to this method make it
      // so that this inner cluster would be handled like as if it was flushed
      // (i.e. it would get an outer cluster)
      inMemoryCluster->removeHashTable();
      clusterDb_->setClusterToFlush(inMemoryCluster);
      flushedChainedCluster_ = TRUE; // remember that special case
      currCluster_ = inMemoryCluster ; // make it get an outer cluster later
      pstate.setState(HASHJ_FLUSH_CLUSTER);
      return; // go flush this cluster; return here for possible more
    }
  } // if saw pressure
  else
    // End of phase 1 - Calculate max memory quota needed, and yield the rest
    clusterDb_->yieldUnusedMemoryQuota();
  
  // Log -- end of phase 1
  if ( hashJoinTdb().logDiagnostics() ) {

    // Report memory quota grabbing during phase 1
    if ( clusterDb_->memoryQuotaMB() > hashJoinTdb().memoryQuotaMB() ) {
      char msg[512];
      str_sprintf(msg, "HJ End Phase 1 (%d). GRABBED additional quota %d MB",
		  0, // NA_64BIT, use instance id later
		  clusterDb_->memoryQuotaMB() - hashJoinTdb().memoryQuotaMB());
      // log an EMS event and continue
      SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
    }

    if ( clusterDb_->sawPressure() ) { // only if an overflow happened
      char msg[1024];
      
      str_sprintf(msg, 
		  "HJ End Phase 1 (%d). #inner rows: %Ld, #buckets: %d, #clusters: %d, #flushed: %d, total size %d MB, cluster size max- %d MB min- %d, variable size records: %s",
		  0, // NA_64BIT, use instance id later
		  totalRightRowsRead_,
		  bucketCount_ , numClusters, numFlushed, 
		  totalSize, maxSize, minSize,
                  hashJoinTdb().useVariableLength() ? "y" : "n");
      // log an EMS event and continue
      SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
    }
  } // if logDiagnostics

#endif
  // all clusters are prepared. Go on with phase 2 of the join
  pstate.setPhase(PHASE_2, bmoStats_);

  // from now on, we want to charge the next phase with time
  if (hashJoinStats_)
    hashJoinStats_->incPhase();

  if (pstate.getOldState() == HASHJ_CANCEL_AFTER_INNER) {
    rightQueue_.down->cancelRequestWithParentIndex(parent_index);
    pstate.setState(HASHJ_CANCELED);
  } else
    pstate.setState(HASHJ_READ_OUTER);
};

///////////////////////////////////////////////////////////////////////////
// read a row from the left child.
///////////////////////////////////////////////////////////////////////////
short ex_hashj_tcb::workReadOuter() {
  ex_queue_entry * leftEntry = leftQueue_.up->getHeadEntry();
  ex_expr::exp_return_type retCode;
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  switch (leftEntry->upState.status) {
  case ex_queue::Q_OK_MMORE: {
   if (pstate.getOldState() != HASHJ_FLUSH_CLUSTER) {
      // Calculate the hash value for the left row. If the oldstate_ is
      // HASHJ_FLUSH_CLUSTER, we tried to insert a row and had to flush a
      // cluster. The cluster is now flushed, but the row is not inserted
      // yet. Thus, in this case  the hash value is still available and
      // we don't have to recalculate it.
      if (! leftHashExpr_)
	hashValue_ = 666;
      else
	{
	  retCode = leftHashExpr_->eval(leftEntry->getAtp(), workAtp_);
	  if (retCode == ex_expr::EXPR_ERROR)
	    {
	      processError(leftEntry->getAtp());
	      return 0;
	    }
	  hashValue_ = hashValue_ & MASK31;	
          // checkOuterNullExpr_ used in the case of hash anti semi join (transformation 
          // of NOT IN subquery). The checkOuterNullExpr_ expression tests if the value of the 
          // left child is null.
          // checkOuterNullExpr_ is set to NULL in workReadInner if the inner table is empty
          // When checkOuterNullExpr_ is not NULL, outer NULL values are discraded
          if (!isInnerEmpty_ &&
              checkOuterNullExpr_  &&
              hashValue_ == ExHDPHash::nullHashValue /*666654765*/)
          {
            retCode = checkOuterNullExpr_->eval(leftEntry->getAtp(), workAtp_);
            switch (retCode)
            {
              case  ex_expr::EXPR_TRUE:
              {
                leftQueue_.up->removeHead();
                return 0;
              }
              break;
              case ex_expr::EXPR_ERROR : 
              {
                return processError(leftEntry->getAtp()) ;
              }
              default:
                break;
            }
          }
      }
    }

    // determine the bucket, where the row is stored
    ULng32 bucketId = hashValue_ % bucketCount_;
    
    // determine the corresponding outer Cluster
    Cluster * oCluster = buckets_[bucketId].getOuterCluster();

    // set oldState_; just in case we came in from HASHJ_FLUSH_CLUSTER
    pstate.setOldState(HASHJ_READ_OUTER);

    ComDiagsArea * da = leftEntry->getDiagsArea();
    if (da)
      {
	ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
	ex_hashj_private_state &  pstate = 
	  *((ex_hashj_private_state*) pEntryDown->pstate);
	
	if (pstate.accumDiags_)
	  pstate.accumDiags_->mergeAfter(*da);
	else
	  {
	    pstate.accumDiags_ = da;
	    da->incrRefCount();
	  }
      }

    if (oCluster) {
      
      
       // the outer cluster exists. Thus, the corresponding inner cluster
      // is FLUSHED. Store row in the outer cluster, we probe it during
      // phase 3 of the join
      if (oCluster->insert(leftEntry->getAtp(),
			   leftMoveInExpr_,
			   hashValue_,
			   &rc_)) {
	leftQueue_.up->removeHead();
      }
      else {
	if (rc_) {
          // the insert caused an error
          // copy the diags from left entry to parent down entry from which
          // the diags will be copied later during HASHJ_ERROR state to
          // parent up entry.
          ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
          pEntryDown->copyAtp(leftEntry);
	  pstate.setState(HASHJ_ERROR);
	  return 0;
	};
	// row is not inserted. This can only mean, that we have to flush
	// the cluster. Do not remove the row and set state_ accordingly
	pstate.setState(HASHJ_FLUSH_CLUSTER);
	return 0;
      };
    }
    else {
      // no outer cluster. If the corresponding inner bucket contains rows or
      // if it is a left join or an AntiSemiJoin, then probe this row.  Note:
      //  workProbe() will take care of removing the row from the up queue
      if (buckets_[bucketId].getRowCount() ||
	  ! onlyReturnResultsWhenInnerMatched_ ) {
	// set the cursor on the first matching row
        Cluster * iCluster = buckets_[bucketId].getInnerCluster();
        HashTable * ht = iCluster->getHashTable();
        if (ht) {
          if (rightSearchExpr_ == NULL) {
            ht->positionSingleChain(&cursor_);
          } else {
            ht->position(&cursor_,
                         leftEntry->getAtp(),
                         workAtp_,
                         hashJoinTdb().extRightRowAtpIndex1_,
                         probeSearchExpr1_,
                         hashValue_,
                         doNotChainDup_,
			 hashJoinTdb().isReturnRightOrdered() );
          }
	  if ((cursor_.getBeginRow() == NULL) && hashJoinStats_) {
	    hashJoinStats_->incEmptyChains();
	  }
	}
	else
	  cursor_.init();
	clusterDb_->setClusterToProbe(iCluster);
	pstate.setState(HASHJ_PROBE);
	return 0;
      };
      // the row can't find a matching row, because the inner bucket is
      // empty and it is not a left join. Thus, we can forget about
      // this row.
      leftQueue_.up->removeHead();
    };
  } break;
  case ex_queue::Q_NO_DATA: {
    // reset the isINnerEmpty flag to TRUE
    isInnerEmpty_ = TRUE;
    //checkOuterNullExpr_ = hashJoinTdb().checkOuterNullExpr_;

    if (isRightJoin()) 
      pstate.setState(HASHJ_RETURN_RIGHT_ROWS);
    else
    pstate.setState(HASHJ_END_PHASE_2);
  } break;
  case ex_queue::Q_SQLERROR:
    {
      processError(leftEntry->getAtp());
    } break;
  case ex_queue::Q_INVALID: {
    ex_assert(0,
	      "ex_hashj_tcb::workReadOuter() Invalid state returned by left child");
  } break;
  };
  return 0;
};

///////////////////////////////////////////////////////////////////////////
// prepare clusters for phase 3 of the join
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workEndPhase2() {
  Cluster * iCluster = clusterDb_->getClusterList();
  Cluster * prevICluster = iCluster;

  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  // When Reuse is applied, then there is no overflow (i.e., no outer clusters
  // are kept) and the inner clusters should be kept and not released !
  if ( isReuse() ) {
    // still need to collect stats (first time only, not when reusing)
    for ( ; ! pstate.usingPreviousHT() && iCluster ; iCluster = iCluster->getNext() )
      if (hashJoinStats_)
	hashJoinStats_->
	  addClusterStats(iCluster->getStats(hashJoinStats_->getHeap()));
 
    pstate.setState(HASHJ_DONE);
    return;
  }

  while (iCluster) {
    Cluster * originalInnerCluster = iCluster;
    Cluster * oCluster = iCluster->getOuterCluster();

    if ( oCluster && (oCluster->getRowCount() || isRightJoin()) ) {
      // the inner cluster is FLUSHED. If we have rows in the corresponding
      // outer cluster or it is a right join, keep the clusters. If the
      // outer has rows in the last buffer, flush the last buffer of the
      // outer to the temporary file. Release all buffers.

      if (oCluster->getFirstBuffer() &&
	  oCluster->getRowsInBuffer()) {
	// outer cluster exists and contains rows. If there are still a
	// few rows in the buffer, flush them.
	clusterDb_->setClusterToFlush(oCluster);
	pstate.setState(HASHJ_FLUSH_CLUSTER);
	return;
      };

      // Then release the hashbuffers of both clusters
      oCluster->releaseAllHashBuffers();
      iCluster->releaseAllHashBuffers();

      // switch to the next pair of clusters
      prevICluster = iCluster;
      iCluster = iCluster->getNext();
    }
    else if (isRightJoin() && !oCluster && (iCluster->getState() == Cluster::FLUSHED)) {
      // This is an inner Cluster that used to have an outer cluster,
      // but it was removed on a previous call to workEndPhase2()
      // sinceit was empty. This method was called again because a
      // subsequent iCluster had an oCluster that needed to be
      // flushed.  After the flush, it returns here, but this time,
      // this iCluster does not have an oCluster.  However, it still
      // needs to be read in and chained and we need to return any
      // rows that were not matched on the outer since this is a right
      // join (presumably all of them since the original outer was
      // empty) (Bugzilla #2603)
      
      // No need to release the hashbuffers of the inner cluster since that
      // was done on a previous call to workEndPhase2().

      // switch to the next pair of clusters
      prevICluster = iCluster;
      iCluster = iCluster->getNext();
    }
    else {  
      // Basically no outer -- no PHASE_3 -- remove this inner cluster
      if (iCluster == clusterDb_->getClusterList()) {
	// the cluster to remove is the first one in the list
	clusterDb_->setClusterList(iCluster->getNext());
	prevICluster = iCluster->getNext();
	iCluster = prevICluster;
      }
      else {
	prevICluster->setNext(iCluster->getNext());
	iCluster = prevICluster->getNext();
      };
      // before we release the cluster, we collect some stats about it
      if (hashJoinStats_)
	hashJoinStats_->
	  addClusterStats(originalInnerCluster->getStats(hashJoinStats_->
							 getHeap()));
      delete originalInnerCluster;
    };

    // Remove an empty outer -- this cluster pair needs not PHASE_3, except
    // right join which has special code to handle a NULL outer.
    if ( oCluster && ! oCluster->getRowCount() ) {
      // before we release the cluster, we collect some stats about it
      if (hashJoinStats_)
	hashJoinStats_->
	  addClusterStats(oCluster->getStats(hashJoinStats_->getHeap()));
      delete oCluster;
      if ( isRightJoin() ) originalInnerCluster->setOuterCluster(NULL);
    };
      
  };  // while (iCluster)

  if (!clusterDb_->getClusterList()) {
    // no clusters left. We are done with the join 
    pstate.setState(HASHJ_DONE);
    return;
  }

  // End of phase 2 - Calculate max memory needed, and yield the rest of quota
  clusterDb_->yieldUnusedMemoryQuota();

  // Log -- end of phase 2
#if !defined(__EID)
  if ( hashJoinTdb().logDiagnostics() /* && clusterDb_->sawPressure() */ ) {
    // L O G
    //   count the number of clusters 
    ULng32 totalSize, maxSize, minSize;

    Cluster * iCluster = clusterDb_->getClusterList();

    for ( iCluster = clusterDb_->getClusterList(), totalSize = 0, 
	    maxSize = 0, minSize = UINT_MAX ;
	  iCluster; 
	  iCluster = iCluster->getNext()
	  ) 
      {
	Cluster * oCluster = iCluster->getOuterCluster();
	ULng32 clusterSizeMB =  NULL == oCluster  ?  0  :
	  (ULng32) (oCluster->clusterSize() / (1024*1024)) ;
	totalSize += clusterSizeMB ;
	if ( maxSize < clusterSizeMB ) maxSize = clusterSizeMB ;
	if ( minSize > clusterSizeMB ) minSize = clusterSizeMB ;
      }

    char msg[1024];
    str_sprintf(msg, 
		"HJ End of Phase 2 (%d). Total outer size %d MB, cluster size max- %d MB min- %d MB, variable size records: %s",
		0, // NA_64BIT, use instance id later
                totalSize, maxSize, minSize,
                hashJoinTdb().useVariableLength() ? "y" : "n");
    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
  
  }
#endif

  // all clusters are prepared. Go on with phase 3 of the join
  // set the first inner cluster to read
  clusterDb_->setClusterToRead(clusterDb_->getClusterList());
  pstate.setPhase(PHASE_3, bmoStats_);
  // from now on, we want to charge the next phase with time
  if (hashJoinStats_)
    hashJoinStats_->incPhase();
  pstate.setState(HASHJ_READ_INNER_CLUSTER);
};


///////////////////////////////////////////////////////////////////////////
// A helper routine to reset the cluster for the case of hash loop.
// This method is called from workReadOuterCluster() and 
// workReturnRightRows() (during PHASE_3).
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::resetClusterForHashLoop(Cluster *iCluster)
{
  // reset for hash loop.
  iCluster->resetForHashLoop();
  
  // Read the next buffers from the inner cluster.
  clusterDb_->setClusterToRead(iCluster);
  return;
}

///////////////////////////////////////////////////////////////////////////
// A helper routine which deletes the existing cluster pair and
// gathers statistics, thus preparing for the next pair of clusters.
// Called during workReturnRightRows() (during PHASE_3) and 
// workReadOuterCluster().
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::prepareForNextPairOfClusters(Cluster *iCluster)
{

#if !defined(__EID)
  if ( hashJoinTdb().logDiagnostics() /* && clusterDb_->sawPressure() */ ) {
    // L O G
    Int64 currTime = NA_JulianTimestamp();
    char msg[1024];
#ifdef TEMP_DISABLE
    Int64 timeTook = currTime - iCluster->startTimePhase3() ;
    if ( iCluster->numLoops() ) {
      str_sprintf(msg, 
		  "HJ Finished cluster (%d) in Phase 3 (%d) with %d Hash-Loop iterations in %d seconds.",
		  (ULng32)iCluster & 0xFFF, 
		  (ULng32)clusterDb_ & 0xFFF,  
		  iCluster->numLoops() + 1,   // add uncounted last iter
		  (ULng32) ( timeTook / 1000000 ) ); 
      
      // log an EMS event and continue
      SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
      
    }
    else 
      {
	clusterDb_->updatePhase3Time(timeTook);
      }
#else
  if ( iCluster->numLoops() ) 
    {
      str_sprintf(msg, 
		  "HJ Finished cluster (%d) in Phase 3 (%d) with %d Hash-Loop iterations.",
		  0, // NA_64BIT, use some id later (ULng32)iCluster & 0xFFF, 
		  0, // NA_64BIT, use some id later (ULng32)clusterDb_ & 0xFFF,  
		  iCluster->numLoops() + 1);   // add uncounted last iter
      // log an EMS event and continue
      SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
    }
#endif
  }

#endif

  // we are done with this pair of clusters; delete them and process
  // the next pair;

  clusterDb_->setClusterList(iCluster->getNext());
  Cluster * oCluster = iCluster->getOuterCluster();
  // before we release the clusters, we collect some stats about them
  if (hashJoinStats_) {
    hashJoinStats_->
      addClusterStats(iCluster->getStats(hashJoinStats_->getHeap()));
    if ( oCluster )
      hashJoinStats_->
	addClusterStats(oCluster->getStats(hashJoinStats_->getHeap()));
  };
  delete iCluster;
  if ( oCluster ) delete  oCluster;
  oCluster = NULL;
  iCluster = clusterDb_->getClusterList();
  clusterDb_->setClusterToRead(iCluster);
  return;
}


///////////////////////////////////////////////////////////////////////////
// An utility routine called only by workProbe() when the left row is not
// needed any more and we should continue to the next outer/left row
// (only used there in the form "return cleanupLeftRow()" )
///////////////////////////////////////////////////////////////////////////
short ex_hashj_tcb::cleanupLeftRow() {
  Cluster * iCluster = clusterDb_->getClusterToProbe();
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  outerMatchedInner_ = FALSE;  // next left row yet unmatched

  // if we are in PHASE_2 remove the entry from the left child
  if (pstate.getPhase() == PHASE_2) 
    leftQueue_.up->removeHead();
  else
    // it is PHASE_3, we have processed another row from the outer
    iCluster->getOuterCluster()->incReadCount();
  
  // we are done with this probe
  clusterDb_->setClusterToProbe(NULL);

  // return to the previous state in order to read the next outer row
  // (the prev state is either READ_OUTER or READ_OUTER_CLUSTER)
  pstate.setState(pstate.getOldState());

  return 0;
}

///////////////////////////////////////////////////////////////////////////
// probe a hash table. This can happen during PHASE_2; in this case the
// hash table is probed with a row from the left child. If the probe
// happens during PHASE_3, the hash table is probed with an "extended"
// left row (in case of cluster exchange with an "extended" right row).
// workProbe() assumes, that the cursor of the tcb points to potential
// matches in the hash table.
// If the probe happens during PHASE_2, workProbe() removes the row from
// left up queue after the matching hash table rows are examined.
// If the probe happens during PHASE_3, workProbe() expects that the
// appropriate tupp in the workAtp_ points already to the "extended" left
// row.
///////////////////////////////////////////////////////////////////////////
short ex_hashj_tcb::workProbe() {
  Cluster * iCluster = clusterDb_->getClusterToProbe();
  ex_assert(iCluster, "ex_hashj_tcb::workProbe() No cluster to probe");
  ex_expr * beforeJoinPred, * afterJoinPred ;
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);
  NABoolean phase2 = pstate.getPhase() == PHASE_2 ; // else - it's phase 3

  // make sure that we don't have a right tupp in the workAtp
  workAtp_->getTupp(hashJoinTdb().extRightRowAtpIndex1_).setDataPointer(NULL);

  // The first Atp parameter for evaluating Before and After Join predicates
  // should always contain the input. In PHASE_2, it is the Atp from the left
  // up-queue, and also the left row is taken from that Atp. In PHASE_3, it is
  // the Atp from the parent's down queue (the left row would be taken from 
  // the second Atp parameter -- the workAtp).
  atp_struct * inputAndPhase2LeftRowAtp = phase2 ?
          leftQueue_.up->getHeadEntry()->getAtp() : downParentEntry->getAtp() ;

  Cluster * oCluster = iCluster->getOuterCluster();
  HashTable * hashTable = iCluster->getHashTable();

  // LOOP: For this outer row, loop and probe the the inner hash table where
  //       multiple matches may be found
  for ( HashRow * hashRow = hashTable ? hashTable->getNext(&cursor_) : NULL ;
	hashRow ;  // found a match ?
	hashRow = hashTable->getNext(&cursor_) ) { // get next match in HT
    
    // if parent canceled while we loop then return
    if ( downParentEntry->downState.request == ex_queue::GET_NOMORE ) return 0;

    // we found a potential match; move the hash table row to the workAtp_

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtp_->getTupp(hashJoinTdb().extRightRowAtpIndex1_).
      setDataPointer(hashRow->getData());

    if (bmoStats_)
       bmoStats_->incInterimRowCount();

    // evaluate beforeJoinPredicate
    beforeJoinPred = phase2 ? beforeJoinPred1_ : beforeJoinPred2_ ;

    if ( beforeJoinPred ) {
      switch ( beforeJoinPred->eval(inputAndPhase2LeftRowAtp, workAtp_) ) {
      case ex_expr::EXPR_ERROR : return processError(inputAndPhase2LeftRowAtp);
      case ex_expr::EXPR_TRUE  : break;   // Before Predicate Passed !
      default:
	// The BeforePredicate failed, thus no match found, 
	// try the next match for this left row
	continue ;
      };
    };

    // the join has produced a row, the row may yet disappear via the
    // afterJoinPredicate, but we should no longer null instantiate
    outerMatchedInner_ = TRUE;

    // if right-outer join, mark this right row
    if (isRightJoin())  
      hashRow->setBit(TRUE);
   
    // if this is a left outer join or semi join and if we are in a hash loop,
    // the outer cluster has a bitmap. Update this bitmap now
    if( oCluster && oCluster->hasBitMap() )  oCluster->updateBitMap();

    // At this point ASJ "failed" per this left row; try the next left row
    if ( isAntiSemiJoin() ) return cleanupLeftRow();

    // evaluate the afterJoinPredicate
    afterJoinPred = phase2 ? afterJoinPred1_ : afterJoinPred2_;

    if ( afterJoinPred ) {
      switch ( afterJoinPred->eval(inputAndPhase2LeftRowAtp, workAtp_) ) {
      case ex_expr::EXPR_ERROR : return processError(inputAndPhase2LeftRowAtp);
      case ex_expr::EXPR_TRUE : break; // After Predicate passed !
      default:
	// After Predicate failed, try the next matching inner row
        numJoinedRowsRejected_++;
	continue;
      };
    };
    
    // we have a match, insert the row into the parent's up queue
    if ( insertResult(hashRow) ) {
      // expression evaluation error
      return 0;
    }

    // Semi join need not match another inner row; go to next outer
    if ( isSemiJoin() ) return cleanupLeftRow();

    // If it is a GET_N request and the number was satisfied then return
    if ( downParentEntry->downState.request == ex_queue::GET_N  &&
	 downParentEntry->downState.requestValue <= 
              (Lng32)pstate.matchCount_ ) 
      return 0;

    // if no room for the next result in the up queue, then come back later
    if (parentQueue_.up->isFull()) return 1;

  } // ------ END OF LOOP: go over matching inner rows for a given outer row

  // at this point: no (more) matches in the inner table . Go to the next
  // left row unless it's a failed (i.e., no matches) ASJ or LJ 
  if ( outerMatchedInner_ || onlyReturnResultsWhenInnerMatched_ )
    return cleanupLeftRow();

  // Evaluate the after join predicate
  afterJoinPred = phase2 ? afterJoinPred3_ : afterJoinPred4_ ;

  if ( afterJoinPred ) {

    // Don't allocate the row for left join
    // Will use the pre-allocated nullData instead

    if ( !isAntiSemiJoin() && nullInstForLeftJoinExpr_) { 
      // Need to nullinst cos afterJoinPred can reference a NULL-inst col.

      // use the pre-allocated nullData.
      workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_) = nullData_;
    }
    
    switch ( afterJoinPred->eval(inputAndPhase2LeftRowAtp, workAtp_) )
      {
      case ex_expr::EXPR_ERROR : 
	return processError(inputAndPhase2LeftRowAtp) ;
      case ex_expr::EXPR_TRUE : 
	break ; // After Predicate passed !
      default: // failed : on to the next left row
	if (leftJoinExpr_)
	  workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_).release();
	return cleanupLeftRow();
      }
  }

  // afterJoinPredicate passed (or wasn't defined) -- return result unless 
  // it's the MIDDLE of a Hash Loop, or the bitmap was set for this left row
  if ( phase2 ||                      // in phase 2, or
       ! oCluster->hasBitMap() ||     // phase 3, and not in a hash loop, or
       // (at the last iteration of the Hash Loop, isHashLoop() becomes FALSE)
       ! clusterDb_->isHashLoop() &&  // end of Hash Loop, and
       ! oCluster->testBitMap()  )    // the bit in bitmap was not set
    {
      if (insertResult(NULL)) {
	// expression evaluation error
	return 0;
      }
    }
       
  // Now we're done with this left row - prepare for the next left row
  return cleanupLeftRow();
};

///////////////////////////////////////////////////////////////////////////
// Find the unmarked right rows, null extend them
// and return to the parent.
///////////////////////////////////////////////////////////////////////////
short ex_hashj_tcb::workReturnRightRows() {

  Cluster * iCluster = NULL;

  // start from where we left off.
  if (clusterDb_->getClusterReturnRightRows()) // either PHASE_3 or returning
                                               // to pick up from the cluster
                                               // I left off during PHASE_2.
    iCluster = clusterDb_->getClusterReturnRightRows();
  else
    {
      iCluster = clusterDb_->getClusterList(); // start from the beginning.
      ex_assert(iCluster, 
		"ex_hashj_tcb::workReturnRightRows No cluster to return rows");
    }

  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  atp_struct * downParentEntryAtp = downParentEntry->getAtp() ;
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  while (iCluster) {
    if (iCluster->getRowCount() && iCluster->isInner() &&
	(iCluster->getState() != Cluster::FLUSHED) )
      {
	clusterDb_->setClusterReturnRightRows(iCluster);
	// Found a cluster with rows
	// Get the hash table to access the rows
	HashTable * hashTable = iCluster->getHashTable();
	hashTable->position(&cursor_);
	
	ex_assert (hashTable, 
		   "ex_hashj_tcb::workReturnRightRows - hashtable must exist");
	
	// If it's an unmatched row; a row for which a match was not 
	// found during the probe of an left/outer (workProbe()), then
	// null extend for the left table and return the row 
	
	// if parent canceled while we loop then return	
	if ( downParentEntry->downState.request == ex_queue::GET_NOMORE ) 
	  return 0;
	
	for ( HashRow * hashRow = hashTable->getNext(&cursor_);
	      hashRow ;  // all rows in the inner (right) table
	      hashRow = hashTable->getNext(&cursor_) ) // get next HT match
	  {

	    ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
	    //ex_expr::exp_return_type retCode;
	    atp_struct * atp = NULL;

            ULng32 bucketId = hashRow->hashValue() % bucketCount_;

            if ( buckets_[bucketId].getInnerCluster() != iCluster) {
              bucketId = hashRow->hashValue() % bucketCount_;
            }
        
            if ( buckets_[bucketId].getInnerCluster() == iCluster &&
                 !hashRow->bitSet()) // if row was unmatched
	      {

		// first, we have to copy the input to this node
		upParentEntry->copyAtp(downParentEntryAtp);
                atp = upParentEntry->getAtp();

                // The max length of the right row.
                UInt32 rightRowLength = hashJoinTdb().rightRowLength_;

                if(rightRowLength) {
                  if(hashJoinTdb().useVariableLength()) {

                    // get the actual length from the HashRow.
                    rightRowLength = hashRow->getRowLength();
                  }

                  resultPool_->moveIn(workAtp_,
                                      hashJoinTdb().rightRowAtpIndex_,
                                      rightRowLength,
                                      hashRow->getData(),
                                      true,
                                      hashJoinTdb().bufferSize_);
		
                  atp->getTupp(hashJoinTdb().returnedRightRowAtpIndex_) =
                    workAtp_->getTupp(hashJoinTdb().rightRowAtpIndex_);
		}

		if (leftJoinExpr_)
		  {

                    SqlBufferHeader::moveStatus ms = 
                      resultPool_->moveIn(downParentEntryAtp,
                                          workAtp_,
                                          hashJoinTdb().instRowForLeftJoinAtpIndex_,
                                          hashJoinTdb().instRowForLeftJoinLength_,
                                          leftJoinExpr_,
                                          true,
                                          hashJoinTdb().bufferSize_);


                    if (ms == SqlBufferHeader::MOVE_ERROR)
                      {
                        processError(downParentEntryAtp);
                        return 1;
                      }

		  }
		
		// Null instantiate the left row.
		if (nullInstForRightJoinExpr_) {
                  // Use the pre-allocated nullData.
                  workAtp_->getTupp(hashJoinTdb().instRowForRightJoinAtpIndex_) = nullData_;
	        }
	
		// Now that we have processed the row -
		// either sending the row to the parent or
		// discarding the row because the row
		// did not satisfy the afterJoinPred5_, set the bit.
		hashRow->setBit(TRUE);

		NABoolean afterJoinPredPassed = TRUE;
		if (afterJoinPred5_)
		  {
		    afterJoinPredPassed = FALSE;
		    ex_expr::exp_return_type retCode;
		    retCode = afterJoinPred5_->eval(downParentEntryAtp,
						    workAtp_);
		    switch (retCode)
		      {
		      case ex_expr::EXPR_ERROR :
			return processError(downParentEntryAtp) ;
		      case ex_expr::EXPR_TRUE: // predicate passed
			afterJoinPredPassed = TRUE;
			break;
		      default: // predicate failed
			break;
		      }
		  } // (afterJoinPred5_) 
	      
		if (afterJoinPredPassed == TRUE)
		  {
		    
		    // add the right row to the result ONLY if the
		    // afterJoinPredicate5 - for now is satisfied
		    if (leftJoinExpr_ )	
		      atp->getTupp(hashJoinTdb().returnedInstRowForLeftJoinAtpIndex_) =
			workAtp_->getTupp(hashJoinTdb().instRowForLeftJoinAtpIndex_);
		    
		    // and add the left row (null instantiated) to the result.
		    if (nullInstForRightJoinExpr_)
		      atp->getTupp(hashJoinTdb().returnedInstRowForRightJoinAtpIndex_) =
			workAtp_->getTupp(hashJoinTdb().instRowForRightJoinAtpIndex_);
		    
		    upParentEntry->upState.status = ex_queue::Q_OK_MMORE;
		    upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
		    upParentEntry->upState.downIndex = parentQueue_.down->getHeadIndex();
		    
		    // we got another result row
		    pstate.matchCount_++;
		    upParentEntry->upState.setMatchNo(pstate.matchCount_);
		
		// insert into parent up queue
		parentQueue_.up->insert();
                if (bmoStats_)
                  bmoStats_-> incActualRowsReturned();
                
                // we can forget about the row in the workAtp
                releaseResultTupps();
		
		// if no room for the next result in the up queue, 
		// then come back later
		if (parentQueue_.up->isFull()) 
		  return 1;
		  }
		
	      } // (!hashRow->bitSet()) // if the row was unmatched
	    
	  } // for ( HashRow * hashRow..
	
      } // (iCluster->getRowCount() && iCluster->isInner())
    
    // Unlike PHASE_2, for PHASE_3 - there is only one cluster.
    // Hence, move on to the next cluster only if it's PHASE_2.
    // Else break;
    if (pstate.getPhase() == PHASE_2)
      iCluster =iCluster->getNext();
    else // PHASE_3
      break; 
  } // while (iCluster)

  if (pstate.getPhase() == PHASE_2)
    pstate.setState(HASHJ_END_PHASE_2);
  else
    {
      // pstate.getPhase() == PHASE_3
      if (clusterDb_->isHashLoop())
	{
	  resetClusterForHashLoop(iCluster);
          pstate.setState(HASHJ_READ_INNER_CLUSTER);
	}
      else
	{
	  // we are done with this pair of clusters; delete them and process
	  // the next pair;
	  prepareForNextPairOfClusters(iCluster);
          pstate.setState(HASHJ_READ_INNER_CLUSTER);
	} // else - clusterDb_->isHashLoop()
    } // else - Phase_ == PHASE_2
  
  // do not leave allocated tupps for the next cluster
  releaseResultTupps();

  clusterDb_->setClusterReturnRightRows(NULL);

  if (bmoStats_)
    bmoStats_->setSpaceBufferCount(resultPool_->get_number_of_buffers());

 return 0; // success
};


///////////////////////////////////////////////////////////////////////////
// read an inner cluster
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workReadInnerCluster() {
  Cluster * iCluster = clusterDb_->getClusterToRead();
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);


  if (iCluster) {
    if (iCluster->read(&rc_)) {
      // all I/Os are done, chain the cluster
      if (!iCluster->chain(TRUE, &rc_)) {
	// chaining the cluster failed.
	pstate.setState(HASHJ_ERROR);
	return;
      };

      if ( iCluster->getOuterCluster() ) {
	// now read a buffer from outer
	pstate.setState(HASHJ_READ_BUFFER);
	clusterDb_->setOuterClusterToRead(iCluster->getOuterCluster(), &rc_);
	if (rc_) {  // memory allocation error
	  pstate.setState(HASHJ_ERROR);
	  return;
	};
      }
      else {
	// Special case: Right join with no outer cluster
	// ( right join PHASE_3 is handled by workReadOuterCluster() )
	pstate.setState(HASHJ_READ_OUTER_CLUSTER);
	clusterDb_->setClusterToRead(NULL);
      }
    }
    else {
      if (rc_) {
	// the read caused an error
	pstate.setState(HASHJ_ERROR);
	return;
      };
    }
  }
  else {
    // no inner Cluster to read, we are done with the join
    pstate.setState(HASHJ_DONE);
  };
};

///////////////////////////////////////////////////////////////////////////
// read ONE buffer of an outer cluster
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workReadBuffer() {
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  Cluster * oCluster = clusterDb_->getClusterToRead();
  if (oCluster->read(&rc_)) {
    // the I/O is complete, position on the first row of the buffer
    oCluster->positionOnFirstRowInFirstOuterBuffer();
    pstate.setState(HASHJ_READ_OUTER_CLUSTER);
    clusterDb_->setClusterToRead(NULL);
  }
  else {
    if (rc_) {
      // the read caused an error
      pstate.setState(HASHJ_ERROR);
      return;
    }
  }
};

///////////////////////////////////////////////////////////////////////////
// read one buffer from an outer cluster and probe for PHASE_3.
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workReadOuterCluster() {
  Cluster * iCluster = clusterDb_->getClusterList();
  Cluster * oCluster = iCluster->getOuterCluster();
  HashRow * row = (HashRow *)( oCluster ? oCluster->advance() : NULL );
  NABoolean moreBuffersToRead = oCluster ? ! oCluster->endOfCluster() : FALSE;
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  while (row) {
    
    if (isSemiJoin() &&
	oCluster->hasBitMap() &&
	oCluster->testBitMap()) {
      // Skip all matching rows that have been returned in a
      // a prior hash loop indicated by a bit set in bitMap.
      // Apparently testBitMap will be always FALSE in the first
      // round of outer cluster loop.
      row = (HashRow *)(oCluster->advance());
      oCluster->incReadCount();
      continue;
    }
      
    // we found another outer row to probe, put it in the correct tupp
    // of the workAtp_

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workAtp_->getTupp(hashJoinTdb().extLeftRowAtpIndex_).
      setDataPointer(row->getData());
    // get the inner clusters hash table and position on the first
    // potential match
    HashTable * ht = iCluster->getHashTable();
    if (rightSearchExpr_ == NULL) {
      ht->positionSingleChain(&cursor_);
    } else {
      ht->position(&cursor_,
                   workAtp_,
                   workAtp_,
                   hashJoinTdb().extRightRowAtpIndex1_,
                   probeSearchExpr2_,
                   row->hashValue(), 
                   doNotChainDup_);
    }
    if (cursor_.getBeginRow() == NULL && onlyReturnResultsWhenInnerMatched_ ) {
      // Not matched.  Get the next row.
      row = (HashRow *)(oCluster->advance());
      oCluster->incReadCount();
      continue;
    }
    // Prepare to probe and eval before/after join predicates.
    // Exit the while loop.
    clusterDb_->setClusterToProbe(iCluster);
    pstate.setState(HASHJ_PROBE);
    return;
  };  // while more rows in the current buffer.

  // All rows in the current buffer have been read.
  // Switch to the next buffer or cluster pair.

  if ( moreBuffersToRead ) {
    // Read the next buffer(s) in the outer cluster.
    clusterDb_->setOuterClusterToRead(oCluster, &rc_); // ignore rc_
    pstate.setState(HASHJ_READ_BUFFER);
    return;
  };

  // no more buffers from the outer cluster; 

  // Return the right rows if it is a right join for this buffer 
  // in the case of hashLoop and cluster otherwise.
  if(isRightJoin())
  {
    clusterDb_->setClusterReturnRightRows(iCluster);
    pstate.setState(HASHJ_RETURN_RIGHT_ROWS);
    return;
  }
  
  // if we are in a hash loop, we have to prepare the clusters for the
  // next loop.
  if (clusterDb_->isHashLoop()) {
    resetClusterForHashLoop(iCluster);
    pstate.setState(HASHJ_READ_INNER_CLUSTER);
    return;
  };
  
  // we are done with this pair of clusters; delete them and process
  // the next pair;
  prepareForNextPairOfClusters(iCluster);
  pstate.setState(HASHJ_READ_INNER_CLUSTER);
  return;
};

///////////////////////////////////////////////////////////////////////////
// we are done with the request. Do some cleanup
///////////////////////////////////////////////////////////////////////////
void ex_hashj_tcb::workDone() {

  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();
  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  ex_hashj_private_state &  pstate = 
    *((ex_hashj_private_state*) downParentEntry->pstate);
  queue_index indexOfParent = parentQueue_.down->getHeadIndex();

  // all rows returned, tell parent that we don't have data left
  upParentEntry->upState.status = ex_queue::Q_NO_DATA;
  upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
  upParentEntry->upState.downIndex = indexOfParent ;
  upParentEntry->upState.setMatchNo(pstate.matchCount_);

  parentQueue_.up->insert();

  // remove the last rows (Q_NO_DATA) from the childrens queue
  if (!leftQueue_.up->isEmpty() &&
      leftQueue_.up->getHeadEntry()->upState.parentIndex == indexOfParent )
    leftQueue_.up->removeHead();

  if ( ! pstate.usingPreviousHT() ) { // so we may have rows in the right queue
    if ( ! rightQueue_.up->isEmpty() && 
         rightQueue_.up->getHeadEntry()->upState.parentIndex == indexOfParent )
      rightQueue_.up->removeHead();
  }

  // initialize back to HASHJ_EMPTY, PHASE_1, 
  // accumDiags_=NULL, matchCount_=0, usingPreviousHT=FALSE, readRightRow=FALSE
  pstate.initState();

  ex_assert(nextRequest_ != indexOfParent , 
            "ex_hashj_tcb::workDone() expected nextRequest++ already");

  parentQueue_.down->removeHead();

  if(!hashJoinTdb().isUniqueHashJoin())
    releaseResultTupps();

#if !defined(__EID)
  if ( hashJoinTdb().logDiagnostics() && 
       !hashJoinTdb().isUniqueHashJoin()
       /* && clusterDb_->sawPressure() */ ) {
    // L O G
    char msg[1024];
#ifdef TEMP_DISABLE
    if ( clusterDb_->numClustersNoHashLoop() ) {
      str_sprintf(msg, 
		  "HJ Finished Phase 3 (%d). Time (sec): total %d (w/o HL) - max %d min %d avg %d (%d clusters)",
		  (ULng32)clusterDb_ & 0xFFF,
		  (ULng32)(clusterDb_->totalPhase3TimeNoHL() / 1000000),
		  (ULng32)(clusterDb_->maxPhase3Time()/ 1000000),
		  (ULng32)(clusterDb_->minPhase3Time()/ 1000000),
		  (ULng32)(clusterDb_->totalPhase3TimeNoHL() / 1000000) 
		     / clusterDb_->numClustersNoHashLoop() ,
		  clusterDb_->numClustersNoHashLoop()
		  );
    }
    else {
      str_sprintf(msg, 
		  "HJ Finished Phase 3 (%d) - All clusters used Hash-Loop!",
		  (ULng32)clusterDb_ & 0xFFF );
    }
#else
    str_sprintf(msg, 
		"HJ Finished Phase 3 (%d)",
		0 // NA_64BIT, use instance id later
                );

#endif
    // log an EMS event and continue
    SQLMXLoggingArea::logExecRtInfo(NULL, 0, msg, tdb.getExplainNodeId());
   
  }
#endif

  // When Reuse is applied the inner clusters/buckets should be kept!
  if ( ! isReuse() ) {
    if(hashJoinTdb().isUniqueHashJoin())
      {
        ((ExUniqueHashJoinTcb *)this)->freeResources();
      }

    haveAllocatedClusters_ = FALSE;

    // In the normal case, nothing else needs to be cleaned up. The
    // clusters were deleted right after they were processed. This also
    // deleted all buffers associated with a cluster
    // But in case of a cancel or a GET_N, there might be still
    // some clusters around. Delete them now!
    if (clusterDb_) {
      // Yield all memory quota back to global count of unused memory, so that
      // other BMOs may use this quota
      clusterDb_->yieldAllMemoryQuota(); // yield all of the alloc memory
      
      delete clusterDb_;
      clusterDb_ = NULL;
    };
    
    // delete the buckets
    if (buckets_) {
      heap_->deallocateMemory(buckets_);
      buckets_ = NULL;
    };

    totalRightRowsRead_ = 0;
  }

  // reset data members, in case we are called again for a new request.
  outerMatchedInner_ = FALSE;
  hashValue_ = 0;
};

ex_hashj_private_state::ex_hashj_private_state()
{
    initState();
}
 
ex_hashj_private_state::~ex_hashj_private_state()  // destructor
{
}

ex_tcb_private_state * ex_hashj_private_state::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ex_hashj_private_state();
};



//////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures for the Unique Hash Join.
//
//////////////////////////////////////////////////////////////////////////////

// Register the Unique Hash Join substasks with the scheduler
//
void ExUniqueHashJoinTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();
  ex_queue_pair pQueue = getParentQueue();

  // down queues are handled by workDown()
  // up queues are handled by workUp()
  // cancellations are handled by workCancel()

  sched->registerInsertSubtask(sWorkDown, this, pQueue.down, "DN");
  sched->registerCancelSubtask(sCancel,   this, pQueue.down, "CA");
  sched->registerUnblockSubtask(sWorkUp,  this, pQueue.up,   "UP");

  // We need to schedule workUp from workDown if we see a GET_NOMORE
  // in workDown in case nothing else scheules workUp.
  workUpTask_ = sched->registerNonQueueSubtask(sWorkUp, this, "UP");

  // register events for child queues
  sched->registerUnblockSubtask(sWorkDown,this, leftQueue_.down);
  sched->registerInsertSubtask(sWorkUp, this, leftQueue_.up);

  sched->registerUnblockSubtask(sWorkDown,this, rightQueue_.down);
  sched->registerInsertSubtask(sWorkUp, this, rightQueue_.up);
}

// Free resources allocated by the TCB
//
void ExUniqueHashJoinTcb::freeResources()
{
  if(bufferHeap_) 
    {
      delete bufferHeap_;
      bufferHeap_ = NULL;
      hashTable_ = NULL;
      bufferPool_ = NULL;
      chainedBufferPool_ = NULL;
      availRows_ = 0;
      totalRightRowsRead_ = 0;
    }
}

//
// Constructor for ExUniqueHashJoinTcb
//
ExUniqueHashJoinTcb::ExUniqueHashJoinTcb(const ex_hashj_tdb & hashJoinTdb,  
                                     const ex_tcb & leftChildTcb,
                                     const ex_tcb & rightChildTcb,
                                     ex_globals * glob)
  : ex_hashj_tcb(hashJoinTdb, leftChildTcb, rightChildTcb, glob),
    hashTable_(NULL),
    bufferSize_(hashJoinTdb.hashBufferSize_),
    extRowSize_(hashJoinTdb.extRightRowLength_),
    rowSize_((Lng32)hashJoinTdb.rightRowLength_),
    returnedRightRowAtpIndex_(hashJoinTdb.returnedRightRowAtpIndex_),
    availRows_(0),
    bufferPool_(NULL),
    chainedBufferPool_(NULL),
    bufferHeap_(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
// Destructor for ExUniqueHashJoinTcb
///////////////////////////////////////////////////////////////////////////////

ExUniqueHashJoinTcb::~ExUniqueHashJoinTcb() 
{
  freeResources();
}

///////////////////////////////////////////////////////////////////////////////
// workUp()
///////////////////////////////////////////////////////////////////////////////
ExWorkProcRetcode ExUniqueHashJoinTcb::workUp()
{

  // Check if there is still work to do
  if (parentQueue_.down->isEmpty())
    return WORK_OK;

  // A hash join workUp never works on more than one parent request at a time
  ex_queue_entry * downParentEntry = parentQueue_.down->getHeadEntry();

  const ex_queue::down_request & request = downParentEntry->downState.request;
  ex_hashj_private_state &  pstate = 
	  *((ex_hashj_private_state*) downParentEntry->pstate);
  
  // loop forever, exit via return
  while (TRUE) {

    // if we have already given to the parent all the rows needed then
    // cancel the parent's request.
    if ( (request == ex_queue::GET_N) && 
         (downParentEntry->downState.requestValue <= (Lng32)pstate.matchCount_) &&
         (pstate.getState() != HASHJ_CANCELED) &&
	 (pstate.getState() != HASHJ_DONE) ) 
      {
        propagateCancel(parentQueue_.down->getHeadIndex(), pstate);
      }

    switch (pstate.getState())
      {
      
      case HASHJ_READ_OUTER:
        {
          if (leftQueue_.up->isEmpty() || parentQueue_.up->isFull())
            {
              // no work to do or no free entry, come back later
              return WORK_OK;
            }

          // Read from the outer, probe the hash table and potentially
          // return a row.  
          // Can return WORK_OK or WORK_POOL_BLOCK, it which case we
          // must return
          //
          short rc = workReadOuter();

          if (rc != WORK_OK)
            return rc;
      
          break;
        }

      case HASHJ_READ_INNER:
      case HASHJ_CANCEL_AFTER_INNER: 
        {
          if (rightQueue_.up->isEmpty())
            {
              return WORK_OK;
            }

          // If we haven't already allocated a hash table for this
          // request, then do so now
          //
          if ( ! pstate.usingPreviousHT() && ! pstate.getHaveClusters() )
            {
              if ( allocateBufferHeap() ) 
                {
                  pstate.setState(HASHJ_ERROR);
                  break;
                }
              pstate.setHaveClusters(TRUE); // now the clusters are allocated
            }

          // If there are no more avail able rows, check again in case
          // we can add more due to variable size rows.
          if(availRows_ == 0 && bufferPool_) {
            availRows_ = bufferPool_->castToSerial()->getRemainingNumFullRows();
          }

          UInt32 defragLength = 0;
          // If we do not have any rows available in the current HashBuffer
          // allocate a new one (or the first one).
          //
          if(availRows_ == 0)
            {

              if (hashJoinTdb().considerBufferDefrag() &&
                  bufferPool_ &&
                  (defragLength=computeDefragLength()) >0)
              {
                //availaRows is 0 but we can still fit at least one row after computing the actual rowa
                // row size. set the availRows_ to one so we can processed it in the workReadInner function
                availRows_ = 1;
              }
              else
              {
                HashBuffer * newBuffer = NULL;

                if ( clusterDb_->enoughMemory( bufferSize_ ) ) // got mem ?
                  newBuffer =
                    new(bufferHeap_, FALSE) HashBuffer(bufferSize_,
                                                       extRowSize_,
                                                       hashJoinTdb().useVariableLength(),
                                                       bufferHeap_,
                                                       clusterDb_,
                                                       &rc_);

                if (!newBuffer || rc_)
                  {
                    // If we couldn't allocate, or not enough mem, we give up
                    // If newBuffer was allocated, it will be deleted
                    // by deleted the whole bufferHeap_ in DONE.

                    rc_ = EXE_NO_MEM_FOR_IN_MEM_JOIN;
                    pstate.setState(HASHJ_ERROR);
                    freeResources(); // free memory to allocate DiagsCondition
                    break;
                  }
                else
                  {

                    // we got a new buffer. Chain it into the list of buffers
                    newBuffer->setNext(bufferPool_);
                    bufferPool_ = newBuffer;
                    availRows_ = newBuffer->castToSerial()->getMaxNumFullRows();

                    if (hashJoinStats_)
                      {
                        hashJoinStats_
                          ->updMemorySize(bufferHeap_->getAllocSize());
                      }
                  }
              }
            }

          // Read rows from the inner side and put them into the bufferPool.
          //
          if(workReadInner(defragLength))
            return WORK_OK;
 
          break;
        }

      case HASHJ_END_PHASE_1: 
        {
          ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();

          ex_hashj_private_state &  pstate = 
            *((ex_hashj_private_state*) pEntryDown->pstate);

	  queue_index parent_index = parentQueue_.down->getHeadIndex();

          // Is this HashJoin doing MIN/MAX optimization.  If so, the
          // left request was delayed until here.
          if (minMaxExpr_ &&  pstate.delayedLeftRequest() ) {
            // We are doing min/max optimization and are done reading
            // the rows from the right child and have computed the min
            // and max values.  Time to construct the request
            // (including min max values) and send it to the left
            // child.

            // need to pass (a belated) GET_ALL request to the left child
            ex_queue_entry * leftEntry = leftQueue_.down->getTailEntry();
            leftEntry->downState.request = ex_queue::GET_ALL;
            leftEntry->downState.requestValue = downParentEntry->downState.requestValue;
            leftEntry->downState.parentIndex = parentQueue_.down->getHeadIndex();

            // Fill in the toLeftAtp with the tuples coming down from
            // the parent and with the computed min/max values.
            toLeftChildAtp_->copyAtp(downParentEntry->getAtp());
            toLeftChildAtp_->getTupp(hashJoinTdb().leftDownCriDesc_->noTuples()-1) =
              workAtp_->getTupp(hashJoinTdb().minMaxValsAtpIndex_);
            leftEntry->passAtp(toLeftChildAtp_);

            leftQueue_.down->insert();

            // delayed no more. From here on -- no special processing
            pstate.resetDelayedLeftRequest();

          }

          if(workChainInner()) 
            {
              // Failed to allocate the hash table.
              pstate.setState(HASHJ_ERROR);
              break;
            }

          if(bufferPool_)
            {
              // Still rows to chain
              // Return to scheduler with CALL AGAIN
              //
              return WORK_CALL_AGAIN;
            }

	  // Handle efficiently the rare case of an empty inner table
          // don't cancel if the left side has an Insert/Update/Delete operation to complete
	  if ( 0 == totalRightRowsRead_ &&   // was the inner table empty ?
              !leftSideIUD()
	       // Uncomment below if UHJ is changed to handle Left-Join or ASJ
	       // && onlyReturnResultsWhenInnerMatched_ 
	       )
	    {
	      // cancel request to outer; no rows would be returned
	      leftQueue_.down->cancelRequestWithParentIndex(parent_index);
	      pstate.setState(HASHJ_CANCELED); // this request returns nothing
	      break;
	    }

          if (pstate.getOldState() == HASHJ_CANCEL_AFTER_INNER)
            {
              rightQueue_.down->cancelRequestWithParentIndex(parent_index);
              pstate.setState(HASHJ_CANCELED);
            } 
          else
            {
              pstate.setState(HASHJ_READ_OUTER);
	      pstate.setPhase(PHASE_2, bmoStats_);

              // from now on, we want to charge the next phase with time
              if (hashJoinStats_)
                hashJoinStats_->incPhase();

              // Leave Q_NO_DATA in queue.  Will be removed in DONE
            }
          break;
        }
      case HASHJ_DONE:
        {
          // Need to return Q_NODATA, so make sure that we have a free
          // slot in the parent's up queue
          //
          if (parentQueue_.up->isFull())
            {
              return WORK_OK;
            }

          workDone();

          // That's it, need to be called again if there are more requests
          //
          if (parentQueue_.down->isEmpty())
            return WORK_OK;
          else
            return WORK_CALL_AGAIN;

          break;
        }

      case HASHJ_CANCELED:
        {

          // the request was canceled. Both children were sent cancel requests.
          // Consume all up rows from the children and wait for Q_NO_DATA.
          //
          NABoolean leftDone = pstate.delayedLeftRequest() ? TRUE : 
            consumeForCancel(leftQueue_);


          // only check the right queue if this request built its own
          // hash table (not reusing)
          NABoolean rightDone = 
            pstate.usingPreviousHT() ? TRUE : consumeForCancel(rightQueue_) ;

          if (!(leftDone && rightDone))
            // we are not done yet, come back again after queue inserts
            return WORK_OK;

          // all rows are consumed, we are done
          pstate.setState(HASHJ_DONE);

          break;
        }

      case HASHJ_ERROR:
        {

          // make sure that we have a free slot in the parent's up queue
          if (parentQueue_.up->isFull())
            {
              return WORK_OK;
            }

          ex_assert( rc_ , "Missing error code"); 

          // we ran into a serious runtime error. Create Condition and
          // pass it to parent. rc_ has the error code.
          ComDiagsArea *da = downParentEntry->getDiagsArea();
          if(!da) {
            da = ComDiagsArea::allocate(heap_);
            downParentEntry->setDiagsArea(da);
          }
            
          if (!da->contains((Lng32) -rc_))
            {
              *da << DgSqlCode(-rc_);
            }

          processError(downParentEntry->getAtp());
          break;      
        }
      }
  }
}

///////////////////////////////////////////////////////////////////////////
// allocateBufferHeap()
//   Initialize the data structures for the hash join
///////////////////////////////////////////////////////////////////////////
NABoolean ExUniqueHashJoinTcb::allocateBufferHeap() 
{

  freeResources();

  // allocate a clusterDB, to be used for memory pressure checks, and quota
  if ( allocateClusters() ) return TRUE; // use original rc_ error 

  // Setup our own bufferHeap. We want at least 10 buffers in each
  // block of this heap. Also add a few bytes to the buffer size to
  // account for some memory management overhead.
  bufferHeap_ = new(heap_, FALSE) NAHeap("Buffer Heap",
                                         (NAHeap *)heap_,
                                         10 * ((Lng32)bufferSize_));

  if (!bufferHeap_)
    {
      rc_ = EXE_NO_MEM_FOR_IN_MEM_JOIN;
      return TRUE;
    }

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
// read a row from the right child and store it in the hashtable
///////////////////////////////////////////////////////////////////////////
short ExUniqueHashJoinTcb::workReadInner(UInt32 defragLength)
{
  ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;

  ex_queue_entry * rightEntry = NULL;
  
  // A reference to the tupp to hold the extended right row.
  //
  tupp &workTupp = workAtp_->getTupp(hashJoinTdb().extRightRowAtpIndex1_);

  ex_queue *upQueue = rightQueue_.up;

  // A local copy of the number of available rows in the current Hash Buffer
  //
  Lng32 availRows = availRows_;
  // for CIF (with variable size)when there no room to hold max size rows we try to compute
  // the actual size of the row and determine if there is room for the actual size.
  // computeDefragLength computes the actual row size

  while(!upQueue->isEmpty() &&
        (availRows > 0 ||
         (defragLength > 0 &&
         (defragLength= computeDefragLength())>0)))
  {
    rightEntry = upQueue->getHeadEntry();

    // If it is some other type of entry besides Q_OK_MMORE, break
    // We will handle it outside the loop.
    //
    if(rightEntry->upState.status != ex_queue::Q_OK_MMORE)
      break;

    HashRow *dataPointer;
    if (defragLength == 0){
    // Get an available row from the buffer.
       dataPointer = bufferPool_->castToSerial()->getFreeRow();
    }
    else
    {
      dataPointer = bufferPool_->castToSerial()->getFreeRow(ROUND4(defragLength) + sizeof(HashRow));
    }

    // Initialize the workAtp extended right row tupp.

    // The expression does not expect the HashRow to be part of the row.
    // Adjust the datapointer in the work atp to point beyond the HashRow.
    workTupp.setDataPointer(dataPointer->getData());

    // Hash the right row, hash value goes into datamember hashValue_
    //
    retCode = rightHashExpr_->eval(rightEntry->getAtp(), workAtp_);
    if (retCode == ex_expr::EXPR_ERROR)
      break;

    // If we are doing MIN/MAX optimization, then accumulate the Min
    // and Max values from the inner row.  Results are accumulated in
    // the min/max tuple of the workAtp_ 
    // (Can we combine this expression with the rightHashExpr_ to
    // avoid some common overhead?)
    if (minMaxExpr_) 
      {
        retCode = minMaxExpr_->eval(rightEntry->getAtp(), workAtp_);
        if (retCode == ex_expr::EXPR_ERROR)
          break;
      }

    // Move the right row into the current row of the Hash Buffer.
    //
    if(rightMoveInExpr_) 
      {
        if (defragLength == 0 )
        {
          UInt32 maxDataLen = bufferPool_->getMaxRowLength() - sizeof(HashRow);
          UInt32 rowLen = extRowSize_ -  sizeof(HashRow);
          UInt32 *rowLenPtr = &rowLen;

          retCode = rightMoveInExpr_->eval(rightEntry->getAtp(), workAtp_, 0, -1, rowLenPtr);
          if (retCode == ex_expr::EXPR_ERROR)
            break;
          // Resize the row if the actual size is different from the max size (maxDataLen)
          if(hashJoinTdb().useVariableLength()) {
            bufferPool_->castToSerial()->setRowLength(dataPointer, rowLen);

            if(rowLen != maxDataLen) {
              bufferPool_->castToSerial()->resizeLastRow(rowLen, dataPointer);
            }
          }
        }
        else
        {
          str_cpy_all(dataPointer->getData(),
                      resultPool_->getDefragTd()->getTupleAddress(),
                      defragLength);

          if(hashJoinTdb().useVariableLength())
          {
            bufferPool_->castToSerial()->setRowLength(dataPointer, defragLength);
          }

#if (defined (NA_LINUX) && defined(_DEBUG) && !defined(__EID))
          char txt[] = "hashjU";
          sql_buffer_pool::logDefragInfo(txt,bufferPool_->getMaxRowLength(),
                                         ROUND4(defragLength) + sizeof(HashRow),
                                         bufferPool_->getFreeSpace(),
                                         bufferPool_,
                                         bufferPool_->getRowCount());
#endif
        }
      }

    dataPointer->setHashValueRaw(hashValue_);

    // One less row available in the current Hash Buffer
    //
    availRows--;

    // row is inserted, remove it from queue
    //
    upQueue->removeHead();
  }


  totalRightRowsRead_ += (availRows_ - availRows);
  if (availRows < 0)
  {
    availRows = 0;
  }
  availRows_ = availRows ;

  ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();

  ex_hashj_private_state &  pstate = 
    *((ex_hashj_private_state*) pEntryDown->pstate);

  // If there was an error from the expression, process the error.
  //
  if (retCode == ex_expr::EXPR_ERROR)
    {
      rc_ = (ExeErrorCode)(-rightEntry->getDiagsArea()->mainSQLCODE());

      pEntryDown->copyAtp(rightEntry);
      pstate.setState(HASHJ_ERROR);
      return 0;
    }

  // If the queue is not empty, then it might be something other than
  // Q_OK_MMORE.  Check for this case here.
  //
  if(!rightQueue_.up->isEmpty()) 
    {
      ex_queue_entry * rightEntry = rightQueue_.up->getHeadEntry();

      switch(rightEntry->upState.status)
        {
        case ex_queue::Q_NO_DATA:
          {
            pstate.setState(HASHJ_END_PHASE_1);
            break;
          }
        case ex_queue::Q_SQLERROR:
          {
            rc_ = (ExeErrorCode)(-rightEntry->getDiagsArea()->mainSQLCODE());

            pEntryDown->copyAtp(rightEntry);
            pstate.setState(HASHJ_ERROR);
            break;
          } 
        case ex_queue::Q_INVALID:
          {
            ex_assert(0,
                      "ExUniqueHashJoinTcb::workReadInner() "
                      "Invalid state returned by right child");
            break;
          }

          // If it was none of these then simply fall through.
          // We may have run out of HashBuffer space (availRows_ == 0)
        }
    }
  else
    {
      // Right Up queue is emtpy, return 1 to indicate return to scheduler.
      return 1;
    }
  
  return 0;
}

NABoolean ExUniqueHashJoinTcb::workChainInner()
{

  if(!hashTable_)
    {
      ULng32 hashEntries = (ULng32) totalRightRowsRead_;

      // Allow for %50 more entries in the hash table to avoid long chains.
      //
      hashEntries += (hashEntries >> 1);

      if ( clusterDb_->enoughMemory( hashEntries * sizeof(HashTableHeader) ) )  
	hashTable_ = new(bufferHeap_, FALSE) HashTable(hashEntries, 
						       FALSE, INT_MAX);

      if (!hashTable_) // not enough memory for a hash table, or new() failed
        {
          rc_ = EXE_NO_MEM_FOR_IN_MEM_JOIN;
	  freeResources(); // free memory (needed to allocate DiagsCondition)
          return TRUE;
        }
    }

  // Keep track of how many rows have been chained.
  //
  ULng32 numRows = 0;

  HashBuffer *buffer = bufferPool_;
  while (buffer)
    {
      HashRow *dataPointer = buffer->castToSerial()->getFirstRow();
      for ( ULng32 rowIndex = 0 ;
            rowIndex < buffer->getRowCount() ;
            rowIndex++)
        {
          // Insert the right row into the hash table.
          //
          hashTable_->insertUniq(dataPointer);

          dataPointer = buffer->castToSerial()->getNextRow(dataPointer);
        }

      numRows += buffer->getRowCount();

      // Finished with this buffer, move it to the front of the chainBufferPool list
      // and get the next buffer from the bufferPool_.
      //
      HashBuffer *chainedBuffer = buffer;
      buffer = buffer->getNext();
      chainedBuffer->setNext(chainedBufferPool_);
      chainedBufferPool_ = chainedBuffer;
      bufferPool_ = buffer;
      
      // Only work so long before returning to the scheduler
      //
      if(numRows > 10000)
        return FALSE;

    }

  return FALSE;
}




///////////////////////////////////////////////////////////////////////////
// read a row from the left child, probe the hash table and
// potentially return a row
///////////////////////////////////////////////////////////////////////////
short ExUniqueHashJoinTcb::workReadOuter()
{

  ex_queue_entry * leftEntry = NULL;
  atp_struct *leftRowAtp = NULL;
  ex_expr::exp_return_type retCode = ex_expr::EXPR_OK;

  // The index to the tupp that holds the extended right row.
  //
  short atpIndex = hashJoinTdb().extRightRowAtpIndex1_;
  
  while(!leftQueue_.up->isEmpty() && !parentQueue_.up->isFull()) {

    leftEntry = leftQueue_.up->getHeadEntry();

    // If it is some other type of entry besides Q_OK_MMORE, break
    // We will handle it outside the loop.
    //
    if(leftEntry->upState.status != ex_queue::Q_OK_MMORE)
      break;

    leftRowAtp = leftEntry->getAtp();

    // Hash the left row, hash value goes into datamember hashValue_
    //
    retCode = leftHashExpr_->eval(leftRowAtp, workAtp_);
    if(retCode == ex_expr::EXPR_ERROR)
      break;

    // Look for a matching row in the hash table.
    // Expect either one row or no rows.
    //
    HashRow *dataPointer;
    retCode = hashTable_->positionUniq(&dataPointer,
                                       leftRowAtp,
                                       workAtp_,
                                       atpIndex,
                                       probeSearchExpr1_,
                                       hashValue_);
    if(retCode == ex_expr::EXPR_TRUE)
      {

        // If found a match, return the composite row.
        // Can return WORK_OK or WORK_POOL_BLOCKED, or
        // WORK_CALL_AGAIN.  For WORK_CALL_AGAIN (and WORK_OK), need
        // to return to workUp, but not scheduler. For
        // WORK_POOL_BLOCKED need to return all the way to the
        // scheduler
        //
        short rc = workReturnRow(dataPointer, leftRowAtp);
        if(rc != WORK_OK)
          return ((rc == WORK_CALL_AGAIN) ? WORK_OK : rc);
      }
    else if(retCode == ex_expr::EXPR_ERROR)
      break;

    leftQueue_.up->removeHead();
  }

  // If we broke out of the loop due to an expression error,
  // handle it here.
  if(retCode == ex_expr::EXPR_ERROR)
    {
      // Since we know the parent up queue is not full, we can call
      // processError() directly.
      //
      processError(leftRowAtp);
      return WORK_OK;
    }

  // If we are not blocked on either of the queues, then it might be
  // something other than Q_OK_MMORE.  Check for this case here.
  //
  if(!leftQueue_.up->isEmpty() && !parentQueue_.up->isFull())
    {
      switch (leftEntry->upState.status)
        {
        case ex_queue::Q_NO_DATA:
          {
            ex_queue_entry *pEntryDown = parentQueue_.down->getHeadEntry();
            ex_hashj_private_state &  pstate = 
              *((ex_hashj_private_state*) pEntryDown->pstate);

            // Leave the NO_DATA in the queue. Will be removed in DONE.
            //
            pstate.setState(HASHJ_DONE);
            break;
          }
        case ex_queue::Q_SQLERROR:
          {
            
            // Since we know the parent up queue is not full, we can call
            // processError() directly.
            //
            processError(leftEntry->getAtp());
            break;
          }
        default: 
          { // Do not expect a Q_INVALID or Q_OK_MMORE
            ex_assert(0,
                      "ExUniqueHashJoinTcb::workReadOuter() "
                      "Invalid state returned by left child");
            break;
          }
        }
    }
    
  return WORK_OK;
}

///////////////////////////////////////////////////////////////////////////
// Return a row to the parent
///////////////////////////////////////////////////////////////////////////
short ExUniqueHashJoinTcb::workReturnRow(HashRow *dataPointer,
                                         atp_struct *leftRowAtp)
{

  ex_queue_entry * upParentEntry = parentQueue_.up->getTailEntry();
  ex_queue_entry * downParentEntry =  parentQueue_.down->getHeadEntry();
  ex_hashj_private_state &  pstate =
          *((ex_hashj_private_state*) downParentEntry->pstate);

  // Start by passing the left row to the parent up queue entry.
  //
  upParentEntry->copyAtp(leftRowAtp);
  
  // If a right row is needed.
  //
  if (isRightOutputNeeded_) 
    {
      
      // The max length of the right row.
      UInt32 size = rowSize_;

      if(hashJoinTdb().useVariableLength()) {

        // get the actual length from the HashRow.
        size = dataPointer->getRowLength();
      }

      // Allocate a right row from the resultPool
      // and put it in the parent up queue entry.
      //
      tupp &rightTupp = 
        upParentEntry->getAtp()->getTupp(returnedRightRowAtpIndex_);

      // Allocate the tuple based on the actual length of the row;
      if (resultPool_->get_free_tuple(rightTupp, size))
        {
          upParentEntry->getAtp()->release();
          return WORK_POOL_BLOCKED;
        }

      // Copy the right row to the new tuple 
      //
      str_cpy_all(rightTupp.getDataPointer(),
                  dataPointer->getData(),
                  size);
    }

  // Prepare the up entry.
  //
  upParentEntry->upState.status = ex_queue::Q_OK_MMORE;
  upParentEntry->upState.parentIndex = downParentEntry->downState.parentIndex;
  upParentEntry->upState.downIndex = parentQueue_.down->getHeadIndex();
  
  // we got another result row
  pstate.matchCount_++;
  upParentEntry->upState.setMatchNo(pstate.matchCount_);
    
  // insert into parent up queue
  parentQueue_.up->insert();

  if (bmoStats_)
    bmoStats_-> incActualRowsReturned();

  // If it is a GET_N request and the number was satisfied then return
  NABoolean getN = (downParentEntry->downState.request == ex_queue::GET_N);
  if (getN &&
      downParentEntry->downState.requestValue <= (Lng32)pstate.matchCount_ ) 
    return WORK_CALL_AGAIN;

  return WORK_OK;
}

// for CIF (with variable size)when there no room to hold max size rows we try to compute
// the actual size of the row and determine if there is room for the actual size.
// computeDefragLength computes the actual row size by applying the move expression.
// the result of the move is saved in a side/temporary buffer and can be used to copy the data
// to the final buffer
UInt32 ExUniqueHashJoinTcb::computeDefragLength()
{
#if defined(_DEBUG)
  assert((availRows_ == 0 || availRows_==1) && hashJoinTdb().considerBufferDefrag());
#endif


  if (!hashJoinTdb().considerBufferDefrag() ||
      rightQueue_.up->isEmpty())
  {
    return 0;
  }


  ex_queue_entry * rightEntry = rightQueue_.up->getHeadEntry();

  if(rightEntry->upState.status != ex_queue::Q_OK_MMORE)
  {
     return 0;
  }

  UInt32 defragLength = 0;
  // A reference to the tupp to hold the extended right row.
  //
  tupp &workTupp = workAtp_->getTupp(hashJoinTdb().extRightRowAtpIndex1_);

  // need to verify the allocated size???
  workTupp.setDataPointer(resultPool_->getDefragTd()->getTupleAddress());

  UInt32 rowLen = extRowSize_ -  sizeof(HashRow);
  UInt32 *rowLenPtr = &rowLen;
  if(rightMoveInExpr_)
  {
    ex_expr::exp_return_type retCode =
                 rightMoveInExpr_->eval(rightEntry->getAtp(), workAtp_, 0, -1, rowLenPtr);
    if (retCode != ex_expr::EXPR_ERROR &&
        rowLen != 0 &&
        (ROUND4(rowLen) + sizeof(HashRow)<= bufferPool_->getFreeSpace()))
    {
      defragLength = rowLen;
    }
  }
  return defragLength;
}


