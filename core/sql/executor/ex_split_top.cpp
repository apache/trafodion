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
 *****************************************************************************
 *
 * File:         ex_split_top.cpp
 * Description:  Split top tdb and tcb (for parallel execution)
 *               
 *               
 * Created:      12/11/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ExCollections.h"
#include "BaseTypes.h"
#include "ComDiags.h"
#include "ex_stdh.h"
#include "ex_exe_stmt_globals.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_split_top.h"
#include "ex_send_top.h"
#include "key_range.h"
#include "ex_frag_rt.h"
#include "PartInputDataDesc.h"
#include "ex_expr.h"
#include "str.h"
#include "ExStats.h"
#include "sql_buffer_size.h"

// BertBert VV
#include "ExCextdecs.h"
// BertBert ^^
// #define TRACE_PAPA_DEQUEUE 1
#ifdef TRACE_PAPA_DEQUEUE
#endif

#define GET_PARENT_UP_QUEUE(i) childTcbsParentUpQ_[i]
#define GET_PARENT_DOWN_QUEUE(i) childTcbsParentDownQ_[i]

#define GET_PARENT_QUEUE(i) \
  (((childTcbs_[i]->getNodeType() != \
  ComTdb::ex_SEND_TOP)) ? \
  qParent_ : \
  (((ex_send_top_tcb*)childTcbs_[i])->getParentQueueForSendTop()))

// -----------------------------------------------------------------------
// Methods for class ex_split_top_tdb
// -----------------------------------------------------------------------

ex_tcb * ex_split_top_tdb::build(ex_globals * glob)
{
  ex_split_top_tcb *result = new(glob->getSpace())ex_split_top_tcb(
       *this,
       glob->castToExExeStmtGlobals());

  result->registerSubtasks();

  return result;
}


// -----------------------------------------------------------------------
// Methods for class ex_split_top_tcb
// -----------------------------------------------------------------------

ex_split_top_tcb::ex_split_top_tcb(
     const ex_split_top_tdb & splitTopTdb,
     ExExeStmtGlobals * glob) : 
    ex_tcb(splitTopTdb,1,glob),
    workAtp_(NULL),
    childTcbs_(glob->getSpace()),
    childTcbsParentUpQ_(glob->getSpace()),
    childTcbsParentDownQ_(glob->getSpace()),
    mergeSequence_(glob->getDefaultHeap()),
    statsArray_(NULL),
    partNumsReqSent_(glob->getDefaultHeap()),
    accumPartNumsReqSent_(glob->getDefaultHeap()),
    tcbState_(READY_TO_REQUEST),
    unmergedChildren_(glob->getDefaultHeap()),
    tempChildAtp_(NULL),
    sharedPool_(NULL)
{
  CollHeap * space = glob->getSpace();
  
  // If we are in the master executor and this is a parallel extract
  // producer query then we need to register the security key
  ExMasterStmtGlobals *masterGlob = glob->castToExMasterStmtGlobals();
  if (masterGlob != NULL && splitTopTdb.getExtractProducerFlag())
    masterGlob->insertExtractSecurityKey(splitTopTdb.getExtractSecurityKey());

  // Allocate the queues to communicate with parent
  allocateParentQueues(qParent_);

  // initialize work atp
  workAtp_ = allocateAtp(splitTopTdb.workCriDesc_, space);
  workAtp_->getTupp(splitTopTdb.inputPartAtpIndex_) = &partNumTupp_;
  workAtp_->getTupp(splitTopTdb.inputPartAtpIndex_).setDataPointer(
       (char *) (&calculatedPartNum_));

  // setup the sidPool_ that is needed for passing in
  // the SID value to the child PA.
  // Allocate and initialize tempChildAtp.
  if(splitTopTdb.isSystemIdentity()){
    // The defaults for splitTopTdb.numBuffers_ = 5
    // splitTopTdb.bufferSize_ = 512
    // sidBufferSize (splitTopTdb.bufferSize_ * maxNumChildren_;)
    // is dependent on the maximum number of children
    // that this node has to support.
    Lng32 sidNumBuffers = splitTopTdb.numBuffers_;
    ULng32 sidBufferSize = 
      (splitTopTdb.bufferSize_ * splitTopTdb.bottomNumParts_ /* maxNumChildren_ for PAPA */);
    
    // allocate space to keep sidTuple
    pool_  = new(space) sql_buffer_pool(sidNumBuffers, 
					sidBufferSize, 
					space);

    tempChildAtp_ = allocateAtp
		    (splitTopTdb.downCriDesc_, 
		     space
		     );

  } // if isSystemIdentity()

  // fixup expressions
  if (childInputPartFunction())
    (void) childInputPartFunction()->fixup(0, getExpressionMode(), this,
                                           glob->getSpace(), 
                                           glob->getDefaultHeap(), FALSE, glob);

  if (mergeKeyExpr())
    (void) mergeKeyExpr()->fixup(0, getExpressionMode(), this, 
                                 glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  processedInputs_ = qParent_.down->getHeadIndex();
  paPartNumTupps_  = NULL;
  firstPartNum_    = 0;
  childParts_      = NULL;

  // ---------------------------------------------------------------------
  // create one or more tcbs for the child tdb
  // ---------------------------------------------------------------------
  if (splitTopTdb.child_->getNodeType() == ComTdb::ex_SEND_TOP)
    {
      // child is a send top node, this means we are communicating with
      // other ESPs; look up # of ESPs in the run-time fragment dir
      ex_send_top_tdb *sendTopTdb = (ex_send_top_tdb *)
                                        (splitTopTdb.child_.getPointer());

      ExFragId childFragId = sendTopTdb->getChildFragId();
      maxNumChildren_ = glob->getNumOfInstances(childFragId);
      Lng32 myInstanceNum = glob->getMyInstanceNumber();

      // Parallel extract consumer query only has one child sendTop
      if (sendTopTdb->getExtractConsumerFlag()) 
      {
        maxNumChildren_ = 1;
      }
      else if (splitTopTdb.isMWayRepartition())
      {
        // ExEspStmtGlobals *espGlobals = glob->castToExEspStmtGlobals();
        maxNumChildren_ = numOfSourceESPs(glob->castToExMasterStmtGlobals()
                                            ? 1 : glob->getNumOfInstances(),
                                          glob->getNumOfInstances(childFragId),
                                          myInstanceNum);
        firstPartNum_ = myFirstSourceESP(glob->castToExMasterStmtGlobals()
                                            ? 1 : glob->getNumOfInstances(),
                                         glob->getNumOfInstances(childFragId),
                                         myInstanceNum);
      }

      // build the send top nodes, using the buildInstance() method
      for (Int32 i = 0; i < maxNumChildren_; i++)
        {
          ex_tcb *childTcb = sendTopTdb->buildInstance(glob,myInstanceNum,
                                                       firstPartNum_+i);

          childTcbs_.insertAt(i,childTcb);
          ((ex_send_top_tcb*)childTcbs_[i])->getParentQueueForSendTop().down->
	    allocateAtps(glob->getSpace());

	  childTcbsParentUpQ_.insertAt(i,
            ((ex_send_top_tcb*)childTcbs_[i])->getParentQueueForSendTop().up);

	  childTcbsParentDownQ_.insertAt(i,
            ((ex_send_top_tcb*)childTcbs_[i])->getParentQueueForSendTop().down);
        }
      numChildren_ = maxNumChildren_;
    }
  else
    {
      // for any other node, take the number of child instances from the
      // partition input data descriptor

      // once optimizer chooses the right value this can be enabled
      maxNumChildren_ = splitTopTdb.bottomNumParts_;
    } // child is not a send top node
  // initialize child states
  childStates_ = new(space) SplitTopChildState[maxNumChildren_];
  for (Int32 i = 0; i < maxNumChildren_; i++)
    {
      childStates_[i].numActiveRequests_ = 0;
      childStates_[i].highWaterMark_     = 0;
      childStates_[i].associatedPartNum_ = SPLIT_TOP_UNASSOCIATED;
    }

  // declare ready children list used (and initialized) in workUp method
  readyChildren_ = new (space) SplitTopReadyChild[maxNumChildren_];

  clearPartNumsReqSent();
  clearAccumPartNumsReqSent();

  if (!isPapaNode() && 
      maxNumChildren_ > 1)
      serializeRequests_ = TRUE;
  else 
      serializeRequests_ = FALSE;

  // allocate buffer for input data tupps, if needed
  inputDataTupps_ = NULL;
  if (splitTopTdb.needToSendInputData())
    {
      ExPartInputDataDesc *partInputDesc = splitTopTdb.partInputDataDesc_;
      Lng32 numParts = partInputDesc->getNumPartitions();
      Lng32 inputDataLength = partInputDesc->getPartInputDataLength();
      Lng32 numSlicesPerChild = numParts / maxNumChildren_;

      if (isPapaNode())
        {
          // A PAPA node does not use the partition input values as
          // set up by the optimizer. It uses direct partition numbers
          // instead. Create only one partition input value tupp that
          // spans the entire partitioning key range.
          numSlicesPerChild = numParts;
        }

      // allocate an SqlBuffer large enough to hold <numParts> tupps
      // with a length of <inputDataLength>
      Lng32 neededBufferSize = (Lng32) SqlBufferNeededSize(numParts+1,
						  inputDataLength,
						  SqlBuffer::NORMAL_);
      inputDataTupps_ = (SqlBuffer *) new(space) char [neededBufferSize];
      inputDataTupps_->driveInit(neededBufferSize, FALSE, SqlBuffer::NORMAL_);

      // now allocate the tupps inside the buffer and initialize them
      // with the (constant) partition input data from the tdb
      for (Lng32 fromPartition = 0;
           fromPartition < numParts;
           fromPartition += numSlicesPerChild)
        {
          tupp tp = inputDataTupps_->add_tuple_desc(inputDataLength);
          Lng32 toPartition =
            MINOF(fromPartition + numSlicesPerChild,numParts) - 1;

          partInputDesc->copyPartInputValue(fromPartition,
                                            toPartition,
                                            tp.getDataPointer(),
                                            inputDataLength);
        }
    }

  // allocate buffers for encoded keys of the children, if a merge is done
  mergeKeyTupps_ = NULL;
  if (mergeKeyExpr())
    {
      Lng32 neededBufferSize = (Lng32) SqlBufferNeededSize(maxNumChildren_,
						  splitTopTdb.mergeKeyLength_,
						  SqlBuffer::NORMAL_);
      mergeKeyTupps_ = (SqlBuffer *) new(space) char [neededBufferSize];
      mergeKeyTupps_->driveInit(neededBufferSize, FALSE, SqlBuffer::NORMAL_);

      for (Lng32 i = 0; i < maxNumChildren_; i++)
        mergeKeyTupps_->add_tuple_desc(splitTopTdb.mergeKeyLength_);
    }
  
  
  // set the stream timeout value to be used by this TCB
  if ( ! glob->getStreamTimeout( streamTimeout_ ) ) // dynamic not found ?
    streamTimeout_ = splitTopTdb.streamTimeout_ ; // then use the static value
  
}

ex_split_top_tcb::~ex_split_top_tcb()
{
  freeResources();
}

void ex_split_top_tcb::freeResources()
{
  delete qParent_.up;
  delete qParent_.down;
  if (workAtp_)
    {
      deallocateAtp(workAtp_, getSpace());
      workAtp_ = NULL;
    }

  if (tempChildAtp_)
    {
      deallocateAtp(tempChildAtp_, getSpace());
      tempChildAtp_ = NULL;
    }

  if (pool_)
    {
      delete pool_;
      pool_ = NULL;
    }
  
  if (sharedPool_)
    {
      delete sharedPool_;
      sharedPool_ = NULL;
    }
  
  // inputDataTupps_, paPartNumTupps_, mergeKeyTupps_, childStates_
  // go away with the space object from which they are allocated
}

void ex_split_top_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // down queues are handled by workDown()
  // up queues and cancellations are handled by workUp()
  // cancellations are handled by workCancel()

  sched->registerInsertSubtask(sWorkDown, this, qParent_.down, "DN");
  sched->registerCancelSubtask(sCancel,   this, qParent_.down, "CA");
  sched->registerUnblockSubtask(sWorkUp,  this, qParent_.up,   "UP");
  registerResizeSubtasks();

  // BertBert VVV
  // The GET_NEXT command causes the WorkDown function to be called.
  sched->registerNextSubtask(sWorkDown,   this, qParent_.down, "DN");
  // BertBert ^^^

  // sometimes it is necessary to schedule the workDown/Up tasks explicitly
  workDownTask_ = sched->registerNonQueueSubtask(sWorkDown, this, "DN");
  workUpTask_   = sched->registerNonQueueSubtask(sWorkUp,   this, "UP");

  // No tasks get registered here if the child TCBs aren't allocated yet
  for (CollIndex i = 0; i < childTcbs_.entries(); i++)
    {
      registerChildQueueSubtask(i);
    }
}

void ex_split_top_tcb::registerChildQueueSubtask(Int32 c)
{
  ExScheduler *sched = getGlobals()->getScheduler();
  ex_queue_pair cq = GET_PARENT_QUEUE(c);
  
  sched->registerUnblockSubtask(sWorkDown, this, cq.down, "DN");
  sched->registerInsertSubtask(sWorkUp,    this, cq.up,   "UP");
}

void ex_split_top_tcb::allocateStatsEntry(Int32 c, ex_tcb *childTcb)
{
  ex_globals * glob    = getGlobals();
  StatsGlobals *statsGlobals = getGlobals()->getStatsGlobals();
  Long semId;
  if (statsGlobals != NULL)
  {
    semId = glob->getSemId();
    int error = statsGlobals->getStatsSemaphore(semId, glob->getPid());
  }
  childTcb->allocateStatsEntry();
  if (childTcb->getStatsEntry()->getCollectStatsType() == ComTdb::ALL_STATS)
    childTcb->getStatsEntry()->setSubInstNum(c);
  if (statsGlobals != NULL)
    statsGlobals->releaseStatsSemaphore(semId, glob->getPid());
}

void ex_split_top_tcb::addChild(Int32 c, NABoolean atWorktime, ExOperStats* statsEntry)
{
  ex_assert(c == (Int32) childTcbs_.entries() &&
            c < maxNumChildren_,
            "Allocating out of sequence child for split top TCB");

  ex_globals * glob    = getGlobals();

  // always set the shared pool in ex_globals before adding a child
  // as the global shared pool pointer may be changed by other PAPA
  if (splitTopTdb().getSetupSharedPool())
    glob->setSharedPool(sharedPool_);

  ex_tcb     *childTcb = splitTopTdb().child_->build(glob);
 
  childTcbs_.insertAt(c,childTcb);
  GET_PARENT_QUEUE(c).down->allocateAtps(glob->getSpace());

  childTcbsParentUpQ_.insertAt(c, GET_PARENT_QUEUE(c).up);
  childTcbsParentDownQ_.insertAt(c, GET_PARENT_QUEUE(c).down);

  if (isPapaNode())
    {
      if (statsEntry)
	childTcb->setStatsEntry (statsEntry);
      else
	{
	  // allocate stats entry if one does not exist.
	  // The statsEntry is needed to record the open count during fixup.
	  if (atWorktime && glob->getStatsArea())
	    {
              allocateStatsEntry(c, childTcb);
            }
       	}
    }
  registerChildQueueSubtask(c);

  if (atWorktime)
    {
      // BertBert VV
      // propagate the holdable cursor flag to this new child.
      propagateHoldable(holdable_);
      // BertBert ^^

      // do both build and fixup when this method is called while
      // a work method is executing
      childTcb->fixup();
    }

  return;
}

ExWorkProcRetcode ex_split_top_tcb::work()
{
  ex_assert(0,"Should never call ex_split_top_tcb::work()");
  return WORK_OK;
}

ExWorkProcRetcode ex_split_top_tcb::workDown()
{
  // if no parent request, return
  if (qParent_.down->isEmpty())
    return WORK_OK;

  // BertBert VVV
  // If the GET_NEXT_N protocol is not yet started (no fetch issued yet),
  //  then nothing to do yet.
  ex_queue_entry *pentry_temp = qParent_.down->getHeadEntry();
  if (((pentry_temp->downState.request == ex_queue::GET_NEXT_N) ||
       (pentry_temp->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT)) &&
      (pentry_temp->downState.numGetNextsIssued == 0))
      return WORK_OK;
  // BertBert ^^^

  const CollIndex numChildren = childTcbs_.entries();

  NABoolean continueWithDownRequests = TRUE;
  while (qParent_.down->entryExists(processedInputs_) AND
         continueWithDownRequests AND
         tcbState_ == READY_TO_REQUEST)
    {
      // yes, entry processedInputs_ exists; get its pstate
      ex_queue_entry * pentry = qParent_.down->getQueueEntry(processedInputs_);
      const ex_queue::down_request request = pentry->downState.request;
      ex_split_top_private_state *pstate =
        (ex_split_top_private_state *) pentry->pstate;

      // Get a PState pointer to an extended PState, in case we need to access the extended
      // fields.
      ex_split_top_private_state_ext *pstateExt = 
        splitTopTdb().getUseExtendedPState() ? (ex_split_top_private_state_ext *)pstate : NULL;

      // request better not be empty
      ex_assert(request != ex_queue::GET_EMPTY,
                "Empty entry inserted in parent queue");
          
      switch (pstate->getState())
        {
        case INITIAL:
          
          if (request == ex_queue::GET_NOMORE)
            {
              // if request has been cancelled don't bother children
              pstate->setState(CANCELLED);
              // But *do* schedule workUp, so it can finish this request.
              workUpTask_->schedule();
              // start over with the cancelled state
              continue;
            }

          // Calculate partition numbers of children who need a request.
              
          pstate->clearPartNumsToDo();
          if (isPapaNode())
            {
              // If this is a PAPA node, the partition numbers are actual
              // DP2 partition numbers. Calculate the maximum range of
              // partitions by using begin and end key expressions and
              // by asking the file system for the corresponding partition
              // numbers.
              if (request == ex_queue::GET_EOD)
                {
		  useAccumulatedPartNumsReqSent();

                  CollIndex p = getFirstPartNumReqSent();
                  if (p == NULL_COLL_INDEX)
                  {
                    // Bugzilla 1563:
                    // if no child got a GET_ALL, don't bother with the EOD.
                    pstate->setState(CANCELLED);
                    // But *do* schedule workUp, so it can finish this request.
                    workUpTask_->schedule();
                    continue;
                  }
                  else
                    do {
                      pstate->addPartNumToDo(p);
                      p = getNextPartNumReqSent(p+1);
                    } while (p != NULL_COLL_INDEX);

                  clearPartNumsReqSent();
		  clearAccumPartNumsReqSent();
                }
              else if (request == ex_queue::GET_EOD_NO_ST_COMMIT)
                {
                  CollIndex p = getFirstPartNumReqSent();
                  if (p == NULL_COLL_INDEX)
                  {
                    // Bugzilla 1563:
                    // if no child get a GET_ALL, don't bother with the EOD.
                    pstate->setState(CANCELLED);
                    // But *do* schedule workUp, so it can finish this request.
                    workUpTask_->schedule();
                    continue;
                  }
                  else
                    do {
                      pstate->addPartNumToDo(p);
                      p = getNextPartNumReqSent(p+1);
                    } while (p != NULL_COLL_INDEX);

		  accumulatePartNumsReqSent();

                  clearPartNumsReqSent();
                }
              else
                {
		  if (splitTopTdb().isSystemIdentity())
		    {
		      // copy the parent queue entry to the child
		      tempChildAtp_->copyAtp(pentry->getAtp());
		      
		      // Generate the sequence value before applying
		      // the beginPartSelectionExpr in getDP2PartitionsToDo.
		      short rc = generateSequenceNextValue(tempChildAtp_);
		      if( rc != WORK_OK)
			return rc; // for now this can only 
		      // be WORK_POOL_BLOCKED
		      if (getDP2PartitionsToDo(tempChildAtp_, pstate))
			{
			  
			  // add error to pstate
			  pstate->addDiagsArea(pentry->getAtp()->getDiagsArea());
			  pstate->setState( ERROR_BEFORE_SEND);
			  break;
			  
			}
		    }
		  else
                    {
		      
		      if(getDP2PartitionsToDo(pentry->getAtp(),pstate))
			{
			  
			  // add error to pstate
			  pstate->addDiagsArea(pentry->getAtp()->getDiagsArea());
			  pstate->setState( ERROR_BEFORE_SEND);
			  break;
			  
			}
                    }
		  
                  if (pstate->getFirstPartNumToDo() == NULL_COLL_INDEX)
                    {
                      // Request does not need to be sent to any DP2
                      // partitions.  However, we send the request to
                      // partition 0 to fix case 10-980901-0616.  In
                      // this case, a row containing out-of-range key
                      // values is inserted into a partitioned table.
                      // The out-of-range values are detected currently
                      // by the PA node.  But because of the out-of-range
                      // key values, the call above to getDP2PartitionsToDo
                      // determines that such a request does not need to
                      // be sent to any partitions, thus the checking done
                      // in the PA node is never performed.  To fix this
                      // problem, we send the request to partition 0.
                      // 
                      // Sending a request to partition 0 when the key
                      // predicates of a request evaluate to FALSE is
                      // also the approach taken in the PA node.  Thus,
                      // the PA and PAPA nodes handle such requests in
                      // a consistent manner.  
                      // 
                      pstate->addPartNumToDoRange(0,0);
                    }
                }
            }
          else
            {
              // in all other cases calculate the actual number(s)
              // of the child TCBs to send data to
                  
              // if an input partitioning function exists, calculate the
              // partition number (the evaluation result gets stored in
              // calculatedPartNum_)
              if (splitTopTdb().childInputPartFunction_)
                {
                  if (splitTopTdb().childInputPartFunction_->eval(
                       pentry->getAtp(),workAtp_) == ex_expr::EXPR_ERROR)
                    {
                      // add error to pstate
                      pstate->addDiagsArea(pentry->getAtp()->getDiagsArea());
                      cancelChildRequests(processedInputs_);
                      // for now
                      ex_assert(FALSE,
                                "error calculating input partition #");
                      return WORK_BAD_ERROR; // fix this later
                    }
                  else
                    {
                      ex_assert(
                           FALSE,
                           "input partitioning is not implemented yet");
                      
                      // is the calculated partition number valid?
                      ex_assert(
                           calculatedPartNum_ >= 0 AND
                           (CollIndex)calculatedPartNum_ < numChildren,
                           "invalid partition number for input value");
                      
                      // send to a single partition number only
                      pstate->addPartNumToDo(calculatedPartNum_);
                    }
                }
              else
                {
                  if (splitTopTdb().isMWayRepartition())
                    {
                      // set a range include only certain partitions
                      pstate->addPartNumToDoRange(firstPartNum_,
                                                  firstPartNum_
                                                + maxNumChildren_ - 1);
                    }
                  else
                    {
                      // set a range including all partitions
                      pstate->addPartNumToDoRange(0,numChildren-1);
                    }
                }
            }
              
          // BertBert VVV
          if (request == ex_queue::GET_NEXT_N)
            {
              if (!isPapaNode())
                ex_assert(FALSE, "GET_NEXT_N must be used in a Papa node");

              // Must use the extended PState below.  Make sure it is valid.
              ex_assert(pstateExt, "Bad PState");
              ex_split_top_private_state_ext *pstate = pstateExt;

              pstate->setState(PROCESS_GET_NEXT_N);
              pstate->activePartNum_ = pstate->getFirstPartNumToDo();  // Ext
              pstate->clearActivePartNumCmdSent();
              pstate->setMaintainGetNextCounters();
            }
          else if ((request == ex_queue::GET_NEXT_N_MAYBE_WAIT) &&
                   (pstate->partNumsToDo_.entries() > 1)) 
            {
              if (!isPapaNode())
                ex_assert(FALSE, "GET_NEXT_N_MAYBE_WAIT must be used in a Papa node");
              pstate->setMaintainGetNextCounters();
              pstate->setState(PROCESS_GET_NEXT_N_MAYBE_WAIT);
            }
          else if ((request == ex_queue::GET_NEXT_N_MAYBE_WAIT) &&
                   (pstate->partNumsToDo_.entries() == 1)) 
            {
              // This case is a special case of the previous case.  It can be executed
              //  a lot more efficient without GET_NEXT_0_MAYBE_WAIT commands.
              if (!isPapaNode())
                ex_assert(FALSE, "GET_NEXT_N_MAYBE_WAIT must be used in a Papa node");

              // Must use the extended PState below.  Make sure it is valid.
              ex_assert(pstateExt, "Bad PState");
              ex_split_top_private_state_ext *pstate = pstateExt;

              pstate->activePartNum_ = pstate->getFirstPartNumToDo();   // Ext
              pstate->setState(PROCESS_GET_NEXT_N_MAYBE_WAIT_ONE_PARTITION);
              pstate->setMaintainGetNextCounters();
            }
          else
            pstate->setState(PART_NUMS_CALCULATED);

          break;
          // BertBert ^^^
              
        case PART_NUMS_CALCULATED:
              
          {
            // now check whether there are child TCBs already associated
            // to required partition numbers and add our own request
            // to them
                
            for (CollIndex dp2PartNum = pstate->getFirstPartNumToDo();
                 dp2PartNum != NULL_COLL_INDEX;
                 dp2PartNum = pstate->getNextPartNumToDo(dp2PartNum+1))
              {
                // which TCB child does this request go to?
                CollIndex childIndex;
                    
                if (isPapaNode())
                  {
                    // for a PAPA node, search for a child that is already
                    // associated with that partition number, or for a
                    // child that can be associated with the partition
                    // number
                    childIndex = getAssociatedChildIndex(dp2PartNum);

                    // if no child that is already associated is found then
                    // wait for a child to free up, don't do anything now
                    // workUp() will reschedule once a child becomes avail.
                    if (childIndex == NULL_COLL_INDEX)
                      {
                        continueWithDownRequests = FALSE;

                        // if we are merging, all child requests for a
                        // given parent request have to be active at
                        // the same time in order to establish the
                        // right order
                        ex_assert(
                             mergeKeyExpr() == NULL,
                             "Not enough PA nodes to merge sorted results");
                        continue;   
                      }
                    else
                      {
                        // a child got associated with a partition. If we 
                        // collect statistics, we have to assign the corresponding
                        // stats entry to the child. If this is the very first
                        // association, the stats entry was allocated during build time
                        // and the child has the stats entry already. In all other
                        // cases the child does not have a stats entry.
                        if (getGlobals()->getStatsArea())
                          {
                            if (childTcbs_[childIndex]->getStatsEntry() == NULL)
                              {
                                if (statsArray_[dp2PartNum] != NULL)
                                  {
                                    // we accessed this partition
                                    // before, the corresponding stats
                                    // entry is availabale in the
                                    // statsArray.
                                    childTcbs_[childIndex]->
                                      setStatsEntry(statsArray_[dp2PartNum]);
                                  }
                                else
                                  {
                                    // this partition was never
                                    // accessed before. We have to
                                    // allocate a new stats
                                    // entry. Note that
                                    // allocateStatsEntry()
                                    // recursively allocates stats
                                    // entries for a tcb and all its
                                    // children.  We don't want this
                                    // here. It still will work ok,
                                    // because we are allocating a
                                    // stats entries for PAs, and PAs
                                    // can never have children (poor
                                    // guys).
				    ex_tcb *childTcb = childTcbs_[childIndex];
                                    allocateStatsEntry(childIndex, childTcb);
                                  } 
                              }
                          }
                        // if the request is NOT a GET_EOD,
                        // remember that it was sent to this partition.
                        if ((request != ex_queue::GET_EOD) &&
			    (request != ex_queue::GET_EOD_NO_ST_COMMIT))
                          addPartNumReqSent(dp2PartNum);
                      }
                  }
                else if (splitTopTdb().isMWayRepartition())
                  {
                    // for table M-Way Repar, find TCB associate to this partNum
                    childIndex = getAssociatedChildIndex(dp2PartNum);
                    ex_assert(childIndex != NULL_COLL_INDEX,
                              "M-Way repartition is unable to find send top!");
                  }
                else
                  // in all other cases, child TCB number "dp2PartNum"
                  // is the one
                  childIndex = dp2PartNum;

		ex_queue* pqDown = GET_PARENT_DOWN_QUEUE(childIndex);
                    
                if (continueWithDownRequests AND NOT (pqDown->isFull()))
                  {
                    ex_queue_entry * centry = pqDown->getTailEntry();
                        
                    // copy the parent queue entry to the child
                    centry->copyAtp(pentry);
                        
                    // add a tupp with partition input data, if needed
                    if (splitTopTdb().needToSendInputData())
                      {
                        if (isPapaNode())
                          centry->getTupp(
                               splitTopTdb().partInputDataAtpIndex_) =
                          inputDataTupps_->getTuppDescriptor(1);
                        else
                          centry->getTupp(
                               splitTopTdb().partInputDataAtpIndex_) =
                          inputDataTupps_->getTuppDescriptor(childIndex+1);
                      }
                        
                    // add a tupp with the partition number for a PA node
                    // if needed
                    if (isPapaNode())
                      {
                        centry->getTupp(
                             splitTopTdb().paPartNoAtpIndex_) =
                          paPartNumTupps_->getTuppDescriptor(dp2PartNum+1);
	
			 if(splitTopTdb().isSystemIdentity()){
			   // Copy the sidTuple in tempChildAtp_  to 
			   // centry sidTuple
			   Int32 sidAtpIndex = splitTopTdb().paPartNoAtpIndex_ + 1;
			   centry->getTupp(sidAtpIndex) = 
			     tempChildAtp_->getTupp(sidAtpIndex);
			   
			   tempChildAtp_->release();

			 } // if (splitTopTdb().isSystemIdentity())
                      }
                        
                    // massage child queue entry
                    centry->downState = pentry->downState;
                    centry->downState.parentIndex = processedInputs_;
                    pqDown->insert();
                    childStates_[childIndex].numActiveRequests_++;
                    childStates_[childIndex].highWaterMark_ =
                      processedInputs_;
                        
                    pstate->removePartNumToDo(dp2PartNum);
                    pstate->addActiveChild(childIndex);
                  }
                else if (isPapaNode())
                    // need to reset the partition association
                    resetAssociatedChildIndex(dp2PartNum);
              }
                
            if (pstate->partNumsToDoIsEmpty())
              {
                processedInputs_++;
                pstate->setState(ALL_SENT_DOWN);
                if (serializeRequests_)
                  tcbState_ = WAIT_FOR_ALL_REPLIES;
              }
            else
              {
                // Couldn't send down to all child TCBs or child
                // partitions that are supposed to get a request.
                // Since this is the ordered protocol we have to
                // make sure that we don't block all children with
                // later requests. For now, stop processing later
                // requests altogether and wait for child queues to
                // unblock or return EOD.
                continueWithDownRequests = FALSE;
              }
                
            // we've done as much as we could for this down request
          }
          break;

        // BertBert VVV
        case PROCESS_GET_NEXT_N_MAYBE_WAIT:
          {

            // Must use the extended PState below.  Make sure it is valid.
            ex_assert(pstateExt, "Bad PState");
            ex_split_top_private_state_ext *pstate = pstateExt;

            // We want to be able to timeout a stream, so we need to remember when we 
            // started.
            if ( streamTimeout_ >= 0 && 
                 pstate->time_of_stream_get_next_usec_ == 0 )   // Ext
              pstate->time_of_stream_get_next_usec_ = NA_JulianTimestamp();  // Ext

            // Look at all partitions and determine what to do with each partition.
            CollIndex dp2PartNum;       
            for (dp2PartNum = pstate->getFirstPartNumToDo();
                 dp2PartNum != NULL_COLL_INDEX;
                 dp2PartNum = pstate->getNextPartNumToDo(dp2PartNum+1))
              {
                // Which TCB child does this request go to?
                CollIndex childIndex;
                childIndex = getAssociatedChildIndex(dp2PartNum);

                // If no child that is already associated is found then
                // wait for a child to free up, don't do anything now
                // workUp() will reschedule once a child becomes avail.
                // Note that this is not good in the streaming destructive select case! $$$99
                if (childIndex == NULL_COLL_INDEX)
                  continueWithDownRequests = FALSE;

                if (continueWithDownRequests)
                  {
                    if (!pstate->commandSent_.contains(dp2PartNum) &&   //Ext
                        !pstate->rowsAvailable_.contains(dp2PartNum))     // Ext
                    {
                      /////////////////////////////////////////////////////////////////////////////
                      // There is no outstanding request to the current partition and we don't know
                      //  if that partition has rows available for us to get. Thus we need to send
                      //  down a GET_NEXT_0_MAYBE_WAIT request.
                      /////////////////////////////////////////////////////////////////////////////
                      ex_queue_entry * centry;
                      // init/no rows available (commendSent_==0, rowsAvailable_==0).
		      ex_queue* pqDown = GET_PARENT_DOWN_QUEUE(childIndex);

                      // Send a GET_NEXT_0_MAYBE_WAIT to that partition.
                      if (NOT (pqDown->isEmpty()))
                        {
                          // There is already a GET_NEXT_N-like request in the down queue, just update it.
                          // Decide for each partition individually to reset the fields to avoid
                          //    overflow.
                          centry = pqDown->getHeadEntry();
                          if (centry->downState.numGetNextsIssued > 100000)
                            pqDown->getNext0MaybeWaitRequestInit();
                          else
                            pqDown->getNext0MaybeWaitRequest();
                          // do GET_NEXT_N bookkeeping
                          pstate->commandSent_ += dp2PartNum;    // Ext
#ifdef TRACE_PAPA_DEQUEUE
                          cout << "Resending GET_NEXT_0 to " << dp2PartNum << childTcbs_[childIndex] << " " << getTable(childTcbs_[childIndex]) << endl;
#endif
                        }
                      else
                        {
                          // There is no request in the down queue, create one.
                          centry =  pqDown->getTailEntry();
                              
                          // copy the parent queue entry to the child
                          centry->copyAtp(pentry);
                              
                          // add a tupp with the partition number for a PA node
                          centry->getTupp(splitTopTdb().paPartNoAtpIndex_) =
                                paPartNumTupps_->getTuppDescriptor(dp2PartNum+1);
                              
                          // massage child queue entry
                          centry->downState = pentry->downState;
                          centry->downState.parentIndex = processedInputs_;
                          centry->downState.requestValue = 0;
                          centry->downState.numGetNextsIssued = 1;
                          centry->downState.request = ex_queue::GET_NEXT_0_MAYBE_WAIT;
                          childStates_[childIndex].numActiveRequests_++;
                          pqDown->insert();
                          pstate->addActiveChild(childIndex);   // so cancel works

                          // do GET_NEXT_N bookkeeping
                          pstate->commandSent_ += dp2PartNum;    //Ext
#ifdef TRACE_PAPA_DEQUEUE
                          cout << "Sending GET_NEXT_0 to " << dp2PartNum << childTcbs_[childIndex] << " " << getTable(childTcbs_[childIndex]) << endl;
#endif
                        }
                    } // no command send, no rows available
                  else
                    if ((pstate->activePartNum_ == NULL_COLL_INDEX) &&    // Ext
                        !pstate->commandSent_.contains(dp2PartNum) &&     // Ext
                        pstate->rowsAvailable_.contains(dp2PartNum))      // Ext
                    {
                      /////////////////////////////////////////////////////////////////////////////
                      // There is no outstanding request to the current partition and we know
                      //  that partition has rows available for us to get (unless these rows are already
                      //  exhausted, but we will find that out later). Also, we have not yet asked an
                      //  other partition to get us rows. Thus we need to send down a GET_NEXT_N request
                      //  to get rows.
                      /////////////////////////////////////////////////////////////////////////////
                      if (pentry->downState.numGetNextsIssued != pstate->satisfiedGetNexts_)   // Ext
                        {
                          // If we are resetting the GET_NEXT request fields in the down queue
                          //    in order to avoid an overflow, then reset the the pstate fields also
                          if (pentry->downState.numGetNextsIssued < pstate->satisfiedGetNexts_)  // Ext
                            {
                              pstate->satisfiedRequestValue_ = 0;   // Ext
                              pstate->satisfiedGetNexts_ = 0;       // Ext
                            }
                          // Decide for each partition individually to reset the fields to avoid
                          //    overflow.
		          ex_queue* pqDown = GET_PARENT_DOWN_QUEUE(childIndex);
                          ex_queue_entry * centry = pqDown->getHeadEntry();

                          if (centry->downState.requestValue > 100000)
                            pqDown->getNextNRequestInit(
                                     pentry->downState.requestValue - pstate->satisfiedRequestValue_);   // Ext
                          else
                            pqDown->getNextNRequest(
                                     pentry->downState.requestValue - pstate->satisfiedRequestValue_);    // Ext
                          // do GET_NEXT_N bookkeeping
                          pstate->activePartNum_ = dp2PartNum;   // Ext
                          pstate->commandSent_ += dp2PartNum;    // Ext
#ifdef TRACE_PAPA_DEQUEUE
                          cout << "GET_NEXT_N to " << dp2PartNum << " childIndex " << childIndex 
                               << " numGetNextsIssued = " << pentry->downState.numGetNextsIssued 
                               << " satisfiedGetNexts_ = " << pstate->satisfiedGetNexts_     // Ext
                               << " requestValue = " << pentry->downState.requestValue 
                               << " satisfiedRequestValue_ " << pstate->satisfiedRequestValue_   // Ext
                               << childTcbs_[childIndex] << " " << getTable(childTcbs_[childIndex]) 
                               << endl;
#endif

                        }

                    }
                  } // continueWithDownRequests
              } // for
            continueWithDownRequests = FALSE;
          } // case
          break;

        case PROCESS_GET_NEXT_N:
          {
              
            // Must use the extended PState below.  Make sure it is valid.
            ex_assert(pstateExt, "Bad PState");
            ex_split_top_private_state_ext *pstate = pstateExt;

            // pass the GET_NEXT_N command down to the active partition.                
            CollIndex dp2PartNum = pstate->activePartNum_;
            if (dp2PartNum == NULL_COLL_INDEX)
              {
                // We are done with this down request.
                // The processed down request will be removed in WorkUp().
                pstate->setState(END_OF_DATA);
                break;
              }

            // which TCB child does this request go to?
            CollIndex childIndex = getAssociatedChildIndex(dp2PartNum);

            if (childIndex == NULL_COLL_INDEX)
              {
                // This should never happen because we are only sending requests
                //  to one partition at a time.
                ex_assert(FALSE, "shouldn't run out of childIndex");
              }

            // If this operator doesn't have work to do, return immediately.
            if (pentry->downState.numGetNextsIssued == pstate->satisfiedGetNexts_)
              {
                continueWithDownRequests = FALSE;
                break;
              }

            // If we are resetting the GET_NEXT request fields in the down queue
            //  in order to avoid an overflow, then reset the the pstate fields also
            if (pentry->downState.numGetNextsIssued < pstate->satisfiedGetNexts_)
              {
                pstate->satisfiedRequestValue_ = 0;
                pstate->satisfiedGetNexts_ = 0;
              }

            
            // If there is no outstanding command to the active partition, send
            //  one now
            if (!pstate->activePartNumCmdSent())
              {
                ex_queue_entry * centry;
		ex_queue* pqDown = GET_PARENT_DOWN_QUEUE(childIndex);
                if (NOT (pqDown->isEmpty()))
                  {
                    centry = pqDown->getHeadEntry();
                    // Decide for each partition individually to reset the fields to avoid
                    //  overflow.
                    if (centry->downState.requestValue > 100000)
                      pqDown->getNextNRequestInit(
                        pentry->downState.requestValue - pstate->satisfiedRequestValue_);
                    else
                      pqDown->getNextNRequest(
                        pentry->downState.requestValue - pstate->satisfiedRequestValue_);
                    // do GET_NEXT_N bookkeeping
                    pstate->setActivePartNumCmdSent();
                  }
                else
                  {
                    centry = pqDown->getTailEntry();
                        
                    // copy the parent queue entry to the child
                    centry->copyAtp(pentry);
                        
                    // add a tupp with the partition number for a PA node
                    centry->getTupp(splitTopTdb().paPartNoAtpIndex_) =
                          paPartNumTupps_->getTuppDescriptor(dp2PartNum+1);
                        
                    // massage child queue entry
                    centry->downState = pentry->downState;
                    centry->downState.parentIndex = processedInputs_;
                    centry->downState.requestValue = pentry->downState.requestValue - pstate->satisfiedRequestValue_;
                    centry->downState.numGetNextsIssued = 1; // this is the first GET_NEXT_N to this partition
                    childStates_[childIndex].numActiveRequests_++;
                    pqDown->insert();
                    pstate->addActiveChild(childIndex); // so cancel works

                    // do GET_NEXT_N bookkeeping
                    pstate->setActivePartNumCmdSent();
                  }
              }
                
            // we've done as much as we could for this down request
            continueWithDownRequests = FALSE;
          }
          break;

        case PROCESS_GET_NEXT_N_MAYBE_WAIT_ONE_PARTITION:
          {
            // Must use the extended PState below.  Make sure it is valid.
            ex_assert(pstateExt, "Bad PState");
            ex_split_top_private_state_ext *pstate = pstateExt;

            // pass the GET_NEXT_N_MAYBE_WAIT command down to the only partition.               
            CollIndex dp2PartNum = pstate->activePartNum_;
            if (dp2PartNum == NULL_COLL_INDEX)
              {
                // This should never happen because we are only sending
		//  requests to one partition (the only partition) and we
		//  are streaming.
                ex_assert(FALSE, "should be actibe partition");
              }

            // which TCB child does this request go to?
            CollIndex childIndex = getAssociatedChildIndex(dp2PartNum);

            if (childIndex == NULL_COLL_INDEX)
              {
                // This should never happen because we are only sending requests
                //  to one partition (the only partition).
                ex_assert(FALSE, "should have a childIndex");
              }

            // If this operator doesn't have work to do, return immediately.
            if (pentry->downState.numGetNextsIssued == pstate->satisfiedGetNexts_)
              {
                continueWithDownRequests = FALSE;
                break;
              }
            
            // If we are resetting the GET_NEXT request fields in the down queue
            //  in order to avoid an overflow, then reset the the pstate fields also
            if (pentry->downState.numGetNextsIssued < pstate->satisfiedGetNexts_)
              {
                pstate->satisfiedRequestValue_ = 0;
                pstate->satisfiedGetNexts_ = 0;
              }

            // If there is no outstanding command to the active partition, send
            //  one now
            if (!pstate->activePartNumCmdSent())
              {
                ex_queue_entry * centry;
		ex_queue* pqDown = GET_PARENT_DOWN_QUEUE(childIndex);
                if (NOT (pqDown->isEmpty()))
                  {
                    centry = pqDown->getHeadEntry();
                    // Decide for each partition individually to reset the fields to avoid
                    //  overflow.
                    if (centry->downState.requestValue > 100000)
                      pqDown->getNextNMaybeWaitRequestInit(
                        pentry->downState.requestValue - pstate->satisfiedRequestValue_);
                    else
                      pqDown->getNextNMaybeWaitRequest(
                        pentry->downState.requestValue - pstate->satisfiedRequestValue_);
                    // do GET_NEXT_N bookkeeping
                    pstate->setActivePartNumCmdSent();
                  }
                else
                  {
                    centry = pqDown->getTailEntry();
                        
                    // copy the parent queue entry to the child
                    centry->copyAtp(pentry);
                        
                    // add a tupp with the partition number for a PA node
                    centry->getTupp(splitTopTdb().paPartNoAtpIndex_) =
                          paPartNumTupps_->getTuppDescriptor(dp2PartNum+1);
                        
                    // massage child queue entry
                    centry->downState = pentry->downState;
                    centry->downState.parentIndex = processedInputs_;
                    centry->downState.requestValue = pentry->downState.requestValue - pstate->satisfiedRequestValue_;
                    centry->downState.numGetNextsIssued = 1; // this is the first GET_NEXT_N to this partition
                    childStates_[childIndex].numActiveRequests_++;
                    pqDown->insert();
                    pstate->addActiveChild(childIndex); // so cancel works

                    // do GET_NEXT_N bookkeeping
                    pstate->setActivePartNumCmdSent();
                  }
              }         
            // we've done as much as we could for this down request
            continueWithDownRequests = FALSE;
          }
          break;
        // BertBert ^^^

        case ALL_SENT_DOWN:
	case MERGING:
        case END_OF_DATA:
        case CANCELLING:
        case CANCELLED:
          // this entry can be skipped
          processedInputs_++;
          break;
        case ERROR_BEFORE_SEND:
	  processedInputs_++;
	  workUpTask_->schedule();
	  break;

        default:
          ex_assert(0,"Internal error, invalid split top state");

        } // switch
    } // while down queue entry exists and continueWithDownRequests

  return WORK_OK;
}

ExWorkProcRetcode ex_split_top_tcb::workUp()
{
  // Loop invariant definitions moved up for better code generation
  ex_expr* mergeExpr = mergeKeyExpr();
  ex_queue* qParentDown = qParent_.down;
  ex_queue* qParentUp = qParent_.up;

  ExOperStats *statsEntry = getStatsEntry();

  // ---------------------------------------------------------------------
  // Try to move data from the children up to the parent queue.
  // Loop over requests from parent queue
  // ---------------------------------------------------------------------
  while (1)
    {
      if (qParentDown->isEmpty())
        return WORK_OK;

      // get the head request from the parent's down queue
      // (the one request for which we currently return result rows)
      ex_queue_entry * pentry = qParentDown->getHeadEntry();
      queue_index pindex = qParentDown->getHeadIndex();
      const ex_queue::down_request request = pentry->downState.request;
      ex_split_top_private_state *pstate =
        (ex_split_top_private_state *) pentry->pstate;

      // Get a PState pointer to an extended PState, in case we need to access the extended
      // fields.
      ex_split_top_private_state_ext *pstateExt = 
        splitTopTdb().getUseExtendedPState() ? (ex_split_top_private_state_ext *)pstate : NULL;

      if (pstate->getState() == INITIAL)
        return WORK_OK;

      /**
      *** Create the doubly-linked readyChildren_ list of SplitTopReadyChildren
      *** links.  It keeps track of all children TCBs ready to deliver data up
      *** to their parents.  Init time is O(n), but access and update times are
      *** O(1).  Note, if there is only one child in the list, both the prev
      *** and next indexes point to the child.  A variable called
      *** readyChildrenListCnt is declared to keep track of the number of
      *** entries in the readyChildren_ list.
      ***
      *** Example of list created:
      ***
      *** [idx=0, cnt=3, pv=10, nx=1] <--> [idx=1, cnt=2, pv=0, nx=2] <-->
      *** [idx=2, cnt=8, pv=1, nx=3] <--> ... <--> [idx=10, cnt=3, pv=9, nx=0]
      **/

      queue_index readyChildrenListCnt = 0;

      // Find all active and data-ready children
      for (CollIndex i = pstate->getLastStaleChild();
           (i = pstate->getPrevActiveChild(i)) != NULL_COLL_INDEX;
           i--)
      {
        queue_index cnt = (GET_PARENT_UP_QUEUE(i))->getLength();

        // If we have a row to send up, create a link for the list.  Set up
        // prev and next indexes in a logical way - we'll fix them up later.
        if (cnt > 0) {
          readyChildren_[readyChildrenListCnt].index = i;
          readyChildren_[readyChildrenListCnt].entryCnt = cnt;
          readyChildren_[readyChildrenListCnt].next = readyChildrenListCnt + 1;
          readyChildren_[readyChildrenListCnt].prev = readyChildrenListCnt - 1;

          readyChildrenListCnt++;
        }
      }

      // Fix head & tail entries if at least one entry was added
      if (readyChildrenListCnt) {
        readyChildren_[0].prev = readyChildrenListCnt - 1;  // fix head
        readyChildren_[readyChildrenListCnt - 1].next = 0;  // fix tail
      }

      // -----------------------------------------------------------------
      // Loop through children which are ready to produce data and which
      // may produce a record, given the merge expression.  Start searching
      // from the beginning of the readyChildren_ list.
      // -----------------------------------------------------------------

      NABoolean parentQueueHasRoom = !qParentUp->isFull();

      CollIndex currChild;          // Current child tcb index
      queue_index currReady = 0;    // Current index into readyChildren_ list

      while (parentQueueHasRoom)
      {
        ex_queue *cqueue;

        // Counter & limit used to regulate amount of data sent up by child.
        queue_index iterLmt = 1;
        queue_index iterIdx = 1;

        NABoolean childAlreadyRemoved = FALSE;  // Child removed cuz of NO_DATA?

        // If we're dealing with a merge expression, call findNextReadyChild
        // since data must be ordered properly.

        if (mergeExpr) {
          currChild = findNextReadyChild(pstate);

          // If no child could be found, break out & process next request
          if (currChild == NULL_COLL_INDEX)
            break;

          cqueue = GET_PARENT_UP_QUEUE(currChild);
        }
        else {
          // Break out if there are no active children ready to deliver
          if (readyChildrenListCnt == 0)
            break;

          currChild = readyChildren_[currReady].index;
          cqueue = GET_PARENT_UP_QUEUE(currChild);

          // Attempt to improve throughput by processing more than just 1 entry
          // in the child's up queue.  However, to ensure fairness, don't allow
          // all entries of a child to be processed before moving to the next
          // child - this is done by enforcing a limit.  TODO: define a runtime
          // variable (CQD?) to determine what this limit should be.

          iterLmt = (readyChildren_[currReady].entryCnt > 3) ? 3 :
                     readyChildren_[currReady].entryCnt;
        }

        do
        {
          ex_queue_entry * centry = cqueue->getHeadEntry();

	  switch (centry->upState.status)
	    {
	    case ex_queue::Q_SQLERROR:
	      {
		// For LRU Operation, when you get an error; do not
		// cancel the request to other ESPs, instead
		// merge the diagnostics into the parents, 
		// consume the row and continue for LRU Operation 
		// for rest of the ESPs.
		if (splitTopTdb().isLRUOperation())
		  {

		    // accumulate all diagnostics info coming with
		    // Q_SQLERROR  entries (errors, warnings, rowcounts)
		    // in the pstate. The sum of all the info will be
		    // given to our parent with our Q_NO_DATA entry.
		    ComDiagsArea *da = centry->getDiagsArea();
		    pstate->addDiagsArea(da);
		    cqueue->removeHead();
		    break;
		  }
		
		// else, do what we do for Q_OK_MMORE
		// so, not need for a break; here. 

	      }
	    case ex_queue::Q_OK_MMORE:
	      {
		// create the upentry
		ex_queue_entry *upentry = qParentUp->getTailEntry();
		
		pstate->matchCountForGetN_++ ;
		
		// BertBert VVV
                if (pstate->maintainGetNextCounters())
                  {
                    // Must use the extended PState below.  Make sure it is valid.
                    ex_assert(pstateExt, "Bad PState");
                    pstateExt->satisfiedRequestValue_++;
                    // remember rows before Q_GET_DONE
                    pstateExt->rowsBeforeQGetDone_++;
                    // BertBert ^^
                  }
		upentry->upState.status      = centry->upState.status;
		upentry->upState.downIndex   = pindex;
		upentry->upState.setMatchNo(pstate->matchCountForGetN_);
		upentry->upState.parentIndex =
		  pentry->downState.parentIndex;

		// check for cancellations and insert row into up queue
		// if still needed
		if ((request == ex_queue::GET_N AND
		     (Lng32)pstate->matchCountForGetN_ >= 
                       pentry->downState.requestValue)
		    OR
		    request == ex_queue::GET_NOMORE)
		  {
		    // if not already done, cancel (note that an
		    // error from one child cancels all other children)
		    if (NOT (pstate->getState() == CANCELLING))
		      {
			if (request == ex_queue::GET_N AND
			    (Lng32)pstate->matchCountForGetN_ ==
                               pentry->downState.requestValue)
			  {
                            if (statsEntry)
                            {
                              statsEntry->incActualRowsReturned();
                            }
			    // this row is still needed, but we want to cancel
			    // now because no more rows will be required
		            upentry->copyAtp(centry->getAtp());
			    qParentUp->insert();
			  }

			// we got enough return tuples or we got a cancel,
			// cancel all outstanding requests
			cancelChildRequests(pindex);

                        // Don't loop around for more child entries
                        iterLmt = 1;
		      }
		  }
		else
		  {
                    if (statsEntry)
                    {
                      statsEntry->incActualRowsReturned();
                    }
                    upentry->copyAtp(centry->getAtp());
                    qParentUp->insert();
		  }

		cqueue->removeHead();

#ifdef TRACE_PAPA_DEQUEUE
                if (request == ex_queue::GET_NEXT_N_MAYBE_WAIT)
                cout << "PAPA gets a row " << currChild 
                     << " numGetNextsIssued = " << pentry->downState.numGetNextsIssued 
                     << " satisfiedGetNexts_ = " << pstateExt->satisfiedGetNexts_ 
                     << " requestValue = " << pentry->downState.requestValue 
                     << " satisfiedRequestValue_ " << pstateExt->satisfiedRequestValue_
                     << " up queue head " << qParentUp->getHeadIndex()
                     << " up queue tail " << qParentUp->getTailIndex()
                     << childTcbs_[currChild] << " " << getTable(childTcbs_[currChild]) 
                     << endl;
#endif
		// ---------------------------------------------------
		// If error, go cancel outstanding work, and get
		// into CANCELLING state, so that findNextReadyChild
		// will find any child w/ responses, not just children
		// in merge sequence.
		// ---------------------------------------------------
		if (centry->upState.status == ex_queue::Q_SQLERROR) {
		  cancelChildRequests(pindex);

                  // Don't loop around for more child entries
                  iterLmt = 1;
                }
	      }
	      break;

	      // BertBert VVV
	    case ex_queue::Q_GET_DONE:
	      {
                // Must use the extended PState below.  Make sure it is valid.
                ex_assert(pstateExt, "Bad PState");
                ex_split_top_private_state_ext *pstate = pstateExt;
		if ((pstate->getState() == PROCESS_GET_NEXT_N) || 
		    (pstate->getState() == PROCESS_GET_NEXT_N_MAYBE_WAIT_ONE_PARTITION))
		  {
		    /////////////////////////////////////////////////////////
		    // This is a destructive select (streaming or
		    //  non-streaming) that processes one partition at a time.
		    //  There is no GET_NEXT_0_MAYBE_WAIT involved.
		    /////////////////////////////////////////////////////////

		    ex_queue_entry *upentry = qParentUp->getTailEntry();
                        
		    upentry->upState.status      = centry->upState.status;
		    upentry->upState.parentIndex = pentry->downState.parentIndex;

		    // 
		    pstate->satisfiedGetNexts_++;
		    pstate->satisfiedRequestValue_ = pentry->downState.requestValue;
		    upentry->upState.setMatchNo(pstate->satisfiedRequestValue_);
                    // insert the result into the up queue
		    qParentUp->insert();

		    cqueue->removeHead();

		    // do GET_NEXT_N(_MAYBE_WAIT) bookkeeping
		    pstate->clearActivePartNumCmdSent();
		  }
		else
		  {
		    /////////////////////////////////////////////////////////
		    // This is a streaming destructive select to a partitioned
		    //  table, using the GET_NEXT_0_MAYBE_WAIT protocol.
		    /////////////////////////////////////////////////////////

		    if (!pstate->rowsAvailable_.contains(childStates_[currChild].associatedPartNum_))
		      {
			// The partition responded to the GET_NEXT_0_MAYBE_WAIT
			//  command with a Q_GET_DONE.  That means that the
			//  partition has now rows it could return (or that it
			//  timed out).
			// If the stream timed out, then return the Q_GET_DONE
			//  to our parent.
			// Note that this is a convenient time to check for
			//  stream_timeout, however, we must make sure that we
			//  don't have an outstanding GET_NEXT_N request.  This
			//  is done by testing activePartNum_.
                        Int64 wait_timeout = streamTimeout_;
      
			if ( pstate->activePartNum_ == NULL_COLL_INDEX  &&
			     wait_timeout >= 0 &&   
			     pstate->time_of_stream_get_next_usec_ + wait_timeout*10000 < NA_JulianTimestamp())
			  {
			    pstate->time_of_stream_get_next_usec_ = 0;
                            if (pstate->satisfiedGetNexts_ < pentry->downState.numGetNextsIssued)
                              {
			          ex_queue_entry *upentry = qParentUp->getTailEntry();
                                
			          upentry->upState.status      = centry->upState.status;
			          upentry->upState.parentIndex = pentry->downState.parentIndex;

			          pstate->satisfiedGetNexts_++;
			          pstate->satisfiedRequestValue_ = pentry->downState.requestValue;
			          upentry->upState.setMatchNo(pstate->satisfiedRequestValue_);
                                      // insert the result into the up queue
			          qParentUp->insert();
#ifdef TRACE_PAPA_DEQUEUE
                            cout << "PAPA timeout currChild " << currChild 
                                 << " numGetNextsIssued = " << pentry->downState.numGetNextsIssued 
                                 << " satisfiedGetNexts_ = " << pstate->satisfiedGetNexts_ 
                                 << " requestValue = " << pentry->downState.requestValue 
                                 << " satisfiedRequestValue_ " << pstate->satisfiedRequestValue_
                                 << " up queue head " << qParentUp->getHeadIndex()
                                 << " up queue tail " << qParentUp->getTailIndex()
                                 << childTcbs_[currChild] << " " << getTable(childTcbs_[currChild]) 
                                 << endl;
#endif
                              }
                            else
                              {
#ifdef TRACE_PAPA_DEQUEUE
                            cout << "PAPA unreported timeout currChild " << currChild 
                                 << endl;
#endif
                              }
			  }

			pstate->commandSent_ -= childStates_[currChild].associatedPartNum_;
			pstate->rowsAvailable_ += childStates_[currChild].associatedPartNum_;
			cqueue->removeHead();
		      }

		    else
		      {
			if (pstate->rowsBeforeQGetDone_)
			  {
                            /////////////////////////////////////////////////
                            // The partition responded to the GET_NEXT_N
			    //  command with some rows followed by a
			    //  Q_GET_DONE.
                            //////////////////////////////////////////////////

			    ex_queue_entry *upentry = qParentUp->getTailEntry();
                                
			    upentry->upState.status      = centry->upState.status;
			    upentry->upState.parentIndex = pentry->downState.parentIndex;


			    pstate->satisfiedGetNexts_++;
			    pstate->satisfiedRequestValue_ = pentry->downState.requestValue;
			    upentry->upState.setMatchNo(pstate->satisfiedRequestValue_);
                                // insert the result into the up queue
			    qParentUp->insert();

			    cqueue->removeHead();

                                // do GET_NEXT_N bookkeeping
			    pstate->commandSent_ -= childStates_[currChild].associatedPartNum_;
			    pstate->activePartNum_ = NULL_COLL_INDEX;
			    pstate->rowsBeforeQGetDone_ = 0;
#ifdef TRACE_PAPA_DEQUEUE
                          cout << "PAPA Q_GET_DONE after some rows " << currChild 
                               << " numGetNextsIssued = " << pentry->downState.numGetNextsIssued 
                               << " satisfiedGetNexts_ = " << pstate->satisfiedGetNexts_ 
                               << " requestValue = " << pentry->downState.requestValue 
                               << " satisfiedRequestValue_ " << pstate->satisfiedRequestValue_
                               << " up queue head " << qParentUp->getHeadIndex()
                               << " up queue tail " << qParentUp->getTailIndex()
                               << childTcbs_[currChild] << " " << getTable(childTcbs_[currChild]) 
                               << endl;
#endif
			  }
			else
			  {
                            ///////////////////////////////////////////////
                            // The partition responded to the GET_NEXT_N
			    // command with zero rows
                            //      followed by a Q_GET_DONE.
                            ///////////////////////////////////////////////

			    cqueue->removeHead();

			    pstate->commandSent_ -= childStates_[currChild].associatedPartNum_;
			    pstate->rowsAvailable_ -= childStates_[currChild].associatedPartNum_;
			    pstate->activePartNum_ = NULL_COLL_INDEX;
#ifdef TRACE_PAPA_DEQUEUE
                          cout << "PAPA Q_GET_DONE after no rows " << currChild 
                               << " numGetNextsIssued = " << pentry->downState.numGetNextsIssued 
                               << " satisfiedGetNexts_ = " << pstate->satisfiedGetNexts_ 
                               << " requestValue = " << pentry->downState.requestValue 
                               << " satisfiedRequestValue_ " << pstate->satisfiedRequestValue_
                               << " up queue head " << qParentUp->getHeadIndex()
                               << " up queue tail " << qParentUp->getTailIndex()
                               << childTcbs_[currChild] << " " << getTable(childTcbs_[currChild]) 
                               << endl;
#endif
			  }
		      } // GET_NEXT_N
		  }
		workDownTask_->schedule();

	      } // Q_GET_DONE
	      break;
	      // BertBert ^^

	    case ex_queue::Q_REC_SKIPPED:
	      {
		// This response can come only if the parent is 
		// an index join. So, we should be getting only one
		// record.
		pstate->setRecSkipped();
		// This is equivalent to a Q_NO_DATA
	      }
	    case ex_queue::Q_NO_DATA:
	      // propagate error info to parent queue entry
	      if (centry->getDiagsArea())
		{
		  // accumulate all diagnostics info coming with
		  // Q_NO_DATA entries (errors, warnings, rowcounts)
		  // in the pstate. The sum of all the info will be
		  // given to our parent with our Q_NO_DATA entry.
		  ComDiagsArea *da = centry->getDiagsArea();
		  pstate->addDiagsArea(da);

		  // give up on all other sibling requests if there
		  // is an error coming back with the Q_NO_DATA.
		  //don't cancel for if it's a LRU operation
		  if (da->mainSQLCODE() < 0 &&
		      NOT splitTopTdb().isLRUOperation()) 
		    cancelChildRequests(pindex);
		}
	      pstate->matchCount_ += centry->upState.getMatchNo();

	      // eat the end-of-data from the child and remove that
	      // child from the list of active children
	      cqueue->removeHead();

	      pstate->removeActiveChild(currChild);

              // Don't process any more entries from this child since it's
              // now inactive.
              iterLmt = 1;

              /**
              *** Need to fix up readyChildrenList since we deleted child.
              *** Indexes in currReady do not need to be deleted/reset since no
              *** link can reach it after we update the list.  In fact, they
              *** should *not* be deleted since we need them to tell us what the
              *** next currChild should be.
              **/
              readyChildren_[readyChildren_[currReady].prev].next =
                readyChildren_[currReady].next;

              readyChildren_[readyChildren_[currReady].next].prev =
                readyChildren_[currReady].prev;

              readyChildrenListCnt--;
              childAlreadyRemoved = TRUE;

	      // BertBert VVV
	      // do GET_NEXT_N bookkeeping
	      // For the GET_NEXT_N protocol, we need to move to the next
	      //   partition now
	      pstate->removePartNumToDo(childStates_[currChild].associatedPartNum_);

              if(pstateExt)
                pstateExt->activePartNum_ = pstate->getNextPartNumToDo(pstateExt->activePartNum_+1);
	      pstate->clearActivePartNumCmdSent();
	      // BertBert ^^^

	      // the child has one less request
	      childStates_[currChild].numActiveRequests_--;
	      if (childStates_[currChild].numActiveRequests_ == 0)
		// the child has no more work and is no longer
		// associated with a partition
		{
		  if (! splitTopTdb().bufferedInserts())
		    {
		      // if we collect stats, save the stats entry
		      // of this child in the stats array and set the
		      // stats entry in the child to NULL (it is not
		      // associated anymore, so it should not have a
		      // stats entry). Of course, we do all this only
		      // if we are a PAPA.
		      if ((statsEntry != NULL) && isPapaNode())
			{
			  statsArray_[childStates_[currChild].associatedPartNum_] =
			    childTcbs_[currChild]->getStatsEntry();
			  childTcbs_[currChild]->setStatsEntry(NULL);
			}
		      childStates_[currChild].associatedPartNum_ =
			  SPLIT_TOP_UNASSOCIATED;
		    }
		  // the workDown method may have blocked because it
		  // could not find more unassigned child PAs (PAPA
		  // case). No external or queue event will wake up
		  // this method, so schedule it here.
		  workDownTask_->schedule();
		}
	      break;

	    case ex_queue::Q_INVALID:
	    default:
	      ex_assert(FALSE,"Internal err. invalid up queue entry");

	    } // switch

          // If parent's up queue is full, set flag and bail out
          if (qParentUp->isFull()) {
            parentQueueHasRoom = FALSE;
            break;
          }
        } while (iterIdx++ < iterLmt);

        // Delete off the queue entries already removed from child
        readyChildren_[currReady].entryCnt -= (iterIdx - 1);

        // If 0 queue entries available, remove from ready list.  Don't remove
        // if it was already removed from a NO_DATA.
        if ((readyChildren_[currReady].entryCnt <= 0) && (!childAlreadyRemoved))
        {
          readyChildren_[readyChildren_[currReady].prev].next =
            readyChildren_[currReady].next;

          readyChildren_[readyChildren_[currReady].next].prev =
            readyChildren_[currReady].prev;

          readyChildrenListCnt--;
        }

        // Switch to new readyChildren_ entry
        currReady = readyChildren_[currReady].next;
      } // while parentQueueHasRoom

      ex_assert(pstate->getState() != INITIAL,
                "Somehow we got here by a shortcut");

      if (pstate->getNumActiveChildren() == 0 AND
          pstate->partNumsToDoIsEmpty())
        {
          // Sounds like we are done with this down request.
          // Also reach here for cancelled requests.
          pstate->setState(END_OF_DATA);
        }
      else
        {
          // still working on this request but can't find any more
          // ready data for the parent, so return to the scheduler
          return WORK_OK;
        }

      if (pstate->getState() == END_OF_DATA)
        {
          if (qParentUp->isFull())
            return WORK_OK;

          if (pstate->getDiagsArea() &&
              pstate->getDiagsArea()->mainSQLCODE() < 0)
            {
              // if there are errors accumulated in the pstate (from
              // calculating input part # or handling non-current down
              // requests) then return those errors now
              ex_queue_entry *upentry = qParentUp->getTailEntry();
              
              upentry->upState.status      = ex_queue::Q_SQLERROR;
              upentry->upState.downIndex   = pindex;
              upentry->upState.setMatchNo(pstate->matchCountForGetN_);
              upentry->upState.parentIndex = pentry->downState.parentIndex;
              // attach diagnostics area (refcount gets transferred to ATP)
              upentry->setDiagsArea(pstate->detachDiagsArea());
              qParentUp->insert();
              
              if (qParentUp->isFull())
                return WORK_OK;
            }

          // ---------------------------------------------------------
          // send one end-of-data entry back to the parent
          // ---------------------------------------------------------
          ex_queue_entry *upentry = qParentUp->getTailEntry();
          
	  if (pstate->recSkipped() == TRUE)
	    upentry->upState.status      = ex_queue::Q_REC_SKIPPED;
	  else
	    upentry->upState.status      = ex_queue::Q_NO_DATA;
          pstate->clearRecSkipped();
          upentry->upState.downIndex   = pindex;
          upentry->upState.setMatchNo(pstate->matchCount_);
          upentry->upState.parentIndex = pentry->downState.parentIndex;
          // error diagnostics have been sent in a separate entry, send
          // diags area with warnings and # of rows affected here
          upentry->setDiagsArea(pstate->detachDiagsArea());

          // insert into parent up queue and delete request
          qParentUp->insert();
          qParentDown->removeHead();
          ex_assert(pstate->getDiagsArea() == NULL,
                    "Left-over diags area in split top");

          pstate->init();
          if(pstateExt)
            pstateExt->init();
            
          tcbState_ = READY_TO_REQUEST;
          // BertBert VV
          // re-init processedInputs_ to the next entry (might be empty)
          processedInputs_ = qParentDown->getHeadIndex();
          // BertBert ^^
          if (!qParentDown->isEmpty())
            workDownTask_->schedule();
        }
    } // while not done
} // ex_split_top_tcb::workUp

ExWorkProcRetcode ex_split_top_tcb::workCancel()
{
  // check for cancelled queue entries
  queue_index e = qParent_.down->getHeadIndex();
  while (qParent_.down->entryExists(e))
    {
      // regardless of whether the entry is already cancelled or not,
      // simply call cancelChildRequests() for each down request that
      // got cancelled
      if (qParent_.down->getQueueEntry(e)->downState.request ==
          ex_queue::GET_NOMORE)
        cancelChildRequests(e);
      e++;
    }
  return WORK_OK;
}

void ex_split_top_tcb::cancelChildRequests(queue_index parentIndex)
{
  // sanity check: does the entry "parentIndex" exist?
  if (qParent_.down->entryExists(parentIndex))
    {
      ex_queue_entry * pentry = qParent_.down->getQueueEntry(parentIndex);
      ex_split_top_private_state *pstate =
        (ex_split_top_private_state *) pentry->pstate;

      switch (pstate->getState())
        {
        case INITIAL:
          pstate->setState(CANCELLED);
          // nothing happened yet, no cleanup to do
          // But *do* schedule workUp, so it can finish this request.
          workUpTask_->schedule();
          break;

        case CANCELLING:
          // why do you call me? the cancellation is no news
          break;

        default:
          {
            pstate->setState(CANCELLING);
            pstate->clearPartNumsToDo();
            for (CollIndex i = pstate->getLastActiveChild();
                 i != NULL_COLL_INDEX;
                 i = pstate->getPrevActiveChild(i-1))
              GET_PARENT_DOWN_QUEUE(i)->
                cancelRequestWithParentIndex(parentIndex);
            // May have to schedule workUp in case of static PA
            // affinity to finish this request
            workUpTask_->schedule();
          }
          break;
        }
    }
}

void ex_split_top_tcb::resetAssociatedChildIndex(Lng32 partNo)
{
  // used for static PA-partition association only
  if (splitTopTdb().isStaticPaAffinity())
    {
      Int32 p = partNo % maxNumChildren_;
      CollIndex c = childParts_[p].childTcbIndex_;
      ex_assert(partNo == childStates_[c].associatedPartNum_,
                "Reset wrong association!");
      if (childStates_[c].numActiveRequests_ == 0)
        childStates_[c].associatedPartNum_ = SPLIT_TOP_UNASSOCIATED;
    }
}

CollIndex ex_split_top_tcb::getAssociatedChildIndex(Lng32 partNo)
{
  // ---------------------------------------------------------------------
  // This procedure gets called when the split top node works as parent
  // of PA nodes (called PAPA node). PA nodes like it when they receive
  // multiple requests that span no more than one DP2 partition and all
  // go to the same partition. Therefore the PAPA node tries to assign
  // an affinity to each of its PA children to a single partition.
  // Requests from the parent of the split top get broken up into multiple
  // requests, each of which spans only one partition number.
  //
  // If the PAPA node does not have static affinity, PA children are
  // never associated with more than one partition at a time. They lose
  // that association as soon as the PAPA node sees that they are idle.
  //
  // If the PAPA node has static affinity, each PA child is associated
  // to one or more partitions. One PA can serves only one partition at
  // a time and new partition request would have to wait.
  //
  // This procedure returns the index to child (PA) TCB.
  // ---------------------------------------------------------------------

  // Static PA-partition association part:
  if (splitTopTdb().isStaticPaAffinity())
    {
       Int32 p = partNo % maxNumChildren_;
       CollIndex c = childParts_[p].childTcbIndex_;
       if (c == NULL_COLL_INDEX)
         { // have not accessed this partition before
           CollIndex nc = childTcbs_.entries();
           if (numChildren_ == 0 && nc == 1)
             { // the first PA already created for partition
               // calculation (see constructor above), use it
               numChildren_ ++; // first PA is used
               childStates_[0].associatedPartNum_ = partNo;
               childParts_[p].childTcbIndex_ = 0;
               return 0;
             }
           else
             { // create new PA
               ex_assert(nc < (CollIndex)maxNumChildren_,
                         "Creating too many PAs!");
               addChild((Int32) nc, TRUE, statsArray_[partNo]);
               numChildren_ ++; // one more PA is used
               childStates_[nc].associatedPartNum_ = partNo;
               childParts_[p].childTcbIndex_ = nc;
               return nc;
             }
         }
       else 
         { // a PA has accessed this partition before
           Lng32 pn = childStates_[c].associatedPartNum_;
           if (pn == partNo)
             return c; // it is accessing it now
           else if (pn == SPLIT_TOP_UNASSOCIATED)
             { // this PA is free to work on this partition again
               ex_assert(childStates_[c].numActiveRequests_ == 0,
                         "Working on two partitions at the same time!")
               childStates_[c].associatedPartNum_ = partNo;
               return c;
             }
           else
             // the child is currently serving other partition
             return NULL_COLL_INDEX;
         }
    }

  // Dynamic PA-partition association part:
  CollIndex newAssociation = NULL_COLL_INDEX;
  CollIndex nc = childTcbs_.entries();
  CollIndex ixOfFreeChild = NULL_COLL_INDEX;

  // ---------------------------------------------------------------------
  // Try to find a child TCB that is already associated with the part #
  // ---------------------------------------------------------------------
  for (CollIndex i = 0; i < nc; i++)
    {
      Lng32 pn = childStates_[i].associatedPartNum_;
      if (pn == partNo)
        return i;
      else if (pn == SPLIT_TOP_UNASSOCIATED)
        ixOfFreeChild = i;
    }

  // ---------------------------------------------------------------------
  // Try to associate a free child with the partition #
  // ---------------------------------------------------------------------
  if (ixOfFreeChild != NULL_COLL_INDEX)
    {
      childStates_[ixOfFreeChild].associatedPartNum_ = partNo;
      return ixOfFreeChild;
    }

  // ---------------------------------------------------------------------
  // Try to build a new child TCB tree if we haven't reached the max yet
  // ---------------------------------------------------------------------
  if (nc < (CollIndex) maxNumChildren_)
    {
      addChild((Int32) nc, TRUE, statsArray_[partNo]);
      childStates_[nc].associatedPartNum_ = partNo;
      return nc;
    }

  // At this point we could just take any child and change its association
  // to the new partition number. The PA child would then have to switch
  // from one partition to another. On the other hand, if we don't make
  // the association now, the work method will try later and we will find
  // a child PA node that got done early. This provides better load
  // balancing.
  return NULL_COLL_INDEX;
}



Lng32 ex_split_top_tcb::getDP2PartitionsToDo(atp_struct * parentAtp,
                                            ex_split_top_private_state *pstate)
{
  pstate->setCurrActiveChild(NULL_COLL_INDEX);
  return 0; 
}

// Return WORK_OK, if all is ok,
// else return reason for failure.
// Right now it only returns WORK_POOL_BLOCKED
short ex_split_top_tcb::generateSequenceNextValue(atp_struct *atp)
{
  return WORK_OK; // success;

} // generateSequenceNextValue

CollIndex ex_split_top_tcb::findNextReadyChild(
     ex_split_top_private_state *pstate)
{
  // ---------------------------------------------------------------------
  // The first time we reach here for each request, initialize the
  // merge sequence and the unmerged children, since there is only one
  // copy of these two objects.
  // ---------------------------------------------------------------------
  if (pstate->getState() == ALL_SENT_DOWN AND mergeKeyExpr())
    {
      mergeSequence_.clear();
      unmergedChildren_ = pstate->getAllActiveChildren();
      pstate->setState(MERGING);
    }

  // ---------------------------------------------------------------------
  // Find the next child TCB that is ready to produce a row for the
  // parent. If data are merged, select the child TCB that has the next
  // row in the correct merge sequence.
  // ---------------------------------------------------------------------

  if (mergeKeyExpr() == NULL OR
      pstate->getState() == CANCELLING)
    {
      // -----------------------------------------------------------------
      // We're not merging, find any child that can produce data. To avoid
      // treating children differently, start the search each time where
      // we left off the last time. Otherwise the children with lower
      // indexes might be able to run faster than their siblings since
      // the split top node asks them for data first. On the other hand,
      // we do allow one child to monopolize the process if it produces
      // rows faster than the parent can consume them. If that turns out
      // to be a problem, we could force a switch of children every n-th
      // row.
      // -----------------------------------------------------------------

      // start with the child that returned data previously or with its
      // successor and remember this starting position
      CollIndex currChild = pstate->getPrevActiveChild(
           pstate->getCurrActiveChild()-1);
      CollIndex startingPosition = currChild;

      NABoolean wrappedAround = FALSE;

      while (TRUE)
        {
          if (currChild == NULL_COLL_INDEX)
            {
              // reached the end after wraparound
              if (wrappedAround)
                return currChild;

              // try to wrap around
              currChild = pstate->getLastActiveChild();
              wrappedAround = TRUE;
              if (currChild == NULL_COLL_INDEX)
                // no more children found after wraparound
                return currChild;
            }

          // at this point we are positioned on the next active child TCB
          if (NOT GET_PARENT_UP_QUEUE(currChild)->isEmpty())
            {
              // found an active child TCB that has produced an up entry
              pstate->setCurrActiveChild(currChild);
              return currChild;
            }

          // try to advance to the next active child TCB
          currChild = pstate->getPrevActiveChild(currChild-1);

          if (currChild == startingPosition)
            // gone around once
            return NULL_COLL_INDEX;
        }
    } // not merging
  else
    {

      // -----------------------------------------------------------------
      // Merging sorted streams. We keep a sequence of child indexes that
      // describes their current sequence. As soon as all active children
      // are part of the sequence we know that we can return the top of
      // the sequence.
      // -----------------------------------------------------------------
      CollIndex result;

      // if the merge sequence is not complete, try to complete it
      // by looking for data from the yet unmerged children

      for (CollIndex ch = unmergedChildren_.getLastStaleBit();
           (ch = unmergedChildren_.prevUsed(ch)) != NULL_COLL_INDEX; ch--)
        {
          ex_queue *cqueue = GET_PARENT_UP_QUEUE(ch);

          if (NOT cqueue->isEmpty())
            {
              // there is a new entry from this child
              ex_queue_entry *centry = cqueue->getHeadEntry();

              switch (centry->upState.status)
                {
                case ex_queue::Q_OK_MMORE:
                  {
                    // Do a binary search to find the position in the
                    // merge sequence.
                    CollIndex lowIndex = 0;
                    CollIndex highIndex = mergeSequence_.entries();
                    
                    // encode the merge key into the tupp reserved
                    // for this purpose
                    workAtp_->getTupp(mergeKeyAtpIndex()) = 
                      mergeKeyTupps_->getTuppDescriptor(ch+1);

                    if (mergeKeyExpr()->eval(centry->getAtp(),workAtp_) ==
                        ex_expr::EXPR_ERROR)
                      {
                        // Assume that error got stored in the main ATP.
                        // Store error row at the beginning of the merge
                        // sequence and make it into an error queue entry.
                        lowIndex = highIndex = 0;
                        centry->upState.status = ex_queue::Q_SQLERROR;
                      }

                    char * newKey = mergeKeyTupps_->
                      getTuppDescriptor(ch+1)->getTupleAddress();

                    // do until we found the exact position
                    while (highIndex > lowIndex)
                      {
                        CollIndex testIndex = (lowIndex+highIndex) / 2;
                        
                        ex_queue_entry *tentry =
                          GET_PARENT_UP_QUEUE(mergeSequence_[testIndex])->getHeadEntry();
                        // pointer to the key to test (note that it
                        // must already have been encoded if it is
                        // a member of the merge sequence)
                        char *testKey = mergeKeyTupps_->
                          getTuppDescriptor(mergeSequence_[testIndex]+1)->
                          getTupleAddress();

                        // evaluate whether the new key to insert is less
                        // or equal than the key in entry "textIndex"
                        // and adjust the intervals
                        if (str_cmp(newKey,testKey,(Int32)mergeKeyLength()) <= 0)
                          {
                            // centry <= tentry, centry must be inserted
                            // BEFORE tentry
                            highIndex = testIndex;
                          }
                        else
                          {
                            // centry > tentry, or centry is NULL,
                            // centry must be inserted AFTER tentry
                            lowIndex = testIndex + 1;
                          }
                      } // while
                    
                    // now lowIndex == highIndex
                    // inserting at position highIndex actually inserts
                    // an entry before an existing position highIndex
                    // if such a position exists
                    mergeSequence_.insertAt(highIndex,ch);

                    // this child is no longer unmerged
                    unmergedChildren_ -= ch;
                  }
                  break;

                case ex_queue::Q_NO_DATA:
                case ex_queue::Q_SQLERROR:
                  // Let caller take this child TCB out of the game
                  // for this round. An error is always delivered first
                  // and once we detect an error we stop merging rows.
                  unmergedChildren_ -= ch;
                  return ch;

                default:
                  ex_assert(0,"Internal error, invalid queue entry");
                  break;
                } // switch

            } // child has new queue entry
        } // for loop over unmerged children

      if (mergeSequence_.entries() == pstate->getNumActiveChildren() AND
          mergeSequence_.entries() > 0)
        {
          // we have a complete merge sequence, return its top element
          // NOTE: we are removing the top element, so the caller better
          // uses it between now and the next call of this method!!!
          mergeSequence_.getFirst(result);
          unmergedChildren_ += result;
          return result;
        }
      else
        {
          // merge sequence was still incomplete or we've reached EOD
          return NULL_COLL_INDEX;
        }
    } // we're merging

  ex_assert(0,"Should never reach here");
  return NULL_COLL_INDEX;
}

ex_queue_pair  ex_split_top_tcb::getParentQueue() const
{
  return qParent_;
}

ex_tcb_private_state * ex_split_top_tcb::allocatePstates(
     Lng32 &numElems,
     Lng32 &pstateLength)
{
  ex_split_top_private_state *result;
  CollHeap *h = getGlobals()->getDefaultHeap();

  NABoolean useExt = splitTopTdb().getUseExtendedPState();

  if(useExt) {
    PstateAllocator<ex_split_top_private_state_ext> pexta;
    result = (ex_split_top_private_state *) pexta.allocatePstates
      (this, numElems, pstateLength);
  } else {
    PstateAllocator<ex_split_top_private_state> pa;
    result = (ex_split_top_private_state *) pa.allocatePstates
      (this, numElems, pstateLength);
  }

  // The heap for the NABitVector objects is not yet set. Set it now
  // before the bit vectors overflow and run into the stub or assert
  // for global operator new.  Note that we can't set the heap in the
  // constructor because a PSTATE needs to have a default constructor
  // if we want to use the PstateAllocator.
  for (Lng32 i = 0; i < numElems; i++)
    if(useExt) {
      ((ex_split_top_private_state_ext *)result)[i].setHeap(h);
    } else {
      result[i].setHeap(h);
    }

  return result;
}

Int32 ex_split_top_tcb::numChildren() const
{
  return childTcbs_.entries();
}   

const ex_tcb* ex_split_top_tcb::getChild(Int32 pos) const
{
  ex_assert((pos >= 0), "");
  if (pos <= (Int32)childTcbs_.entries())
    return childTcbs_[pos];
  else
    return NULL;
}

ExOperStats * ex_split_top_tcb::doAllocateStatsEntry(CollHeap *heap,
						     ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
  {
    stat =  ex_tcb::doAllocateStatsEntry(heap, tdb);
  }
  else
  {
    stat = new(heap) ExSplitTopStats(heap,
				   this,
				   tdb);
  }
  
  if (stat)
  {
    // Set the right child to -2, so that ex_tcb::propagateTdbIdForStats
    // can set it to -1 even when there more than one children in 
    // split_top_tcb
    stat->setRightChildTdbId(-2);
  }
  return stat;
}

CollIndex ex_split_top_tcb::getNextPartNumReqSent(CollIndex prev) const
{
  if (partNumsReqSent_.nextUsedFast(prev))
    return prev;
  else
    return NULL_COLL_INDEX;
}

void ex_split_top_tcb::accumulatePartNumsReqSent()
{
  accumPartNumsReqSent_ += partNumsReqSent_;
}

void ex_split_top_tcb::useAccumulatedPartNumsReqSent()
{
  if (NOT accumPartNumsReqSent_.isEmpty())
    {
      clearPartNumsReqSent();
 
      partNumsReqSent_ = accumPartNumsReqSent_;
    }
}

// -----------------------------------------------------------------------
// Methods for class ex_split_top_private_state
// -----------------------------------------------------------------------

ex_split_top_private_state::ex_split_top_private_state()
  : partNumsToDo_(NULL),
    activeChildren_(NULL)
{
  init();
}

ex_split_top_private_state::~ex_split_top_private_state()
{
}

void ex_split_top_private_state::init()
{
  state_           = ex_split_top_tcb::INITIAL;
  splitTopPStateFlags_ = 0;
  currActiveChild_ = 0;
  matchCount_      = 0;
  matchCountForGetN_ = 0;
  diagsArea_       = NULL;
  // BertBert VVV
  clearActivePartNumCmdSent();
  clearMaintainGetNextCounters();
  clearRecSkipped();
  // BertBert ^^^
}

void ex_split_top_private_state::setHeap(CollHeap *heap)
{
  partNumsToDo_.setHeap(heap);
  activeChildren_.setHeap(heap);
}

CollIndex ex_split_top_private_state::getNextPartNumToDo(CollIndex prev) const
{
  if (partNumsToDo_.nextUsedFast(prev))
    return prev;
  else
    return NULL_COLL_INDEX;
}


CollIndex ex_split_top_private_state::getNextActiveChild(CollIndex prev)
{
  if (activeChildren_.nextUsedFast(prev))
    return prev;
  else
    return NULL_COLL_INDEX;
}

void ex_split_top_private_state::setState(ex_split_top_tcb::workState s)
{
  // could do some sanity checks
  state_ = s;
}

void ex_split_top_private_state::addDiagsArea(ComDiagsArea * diagsArea)
{
  if (diagsArea_ == NULL)
    {
      diagsArea_ = diagsArea;
      // we now own a refcount to the diags area
      diagsArea_->incrRefCount();
    }
  else if (diagsArea != NULL)
    {
      diagsArea_->mergeAfter(*diagsArea);
      // this doesn't increment the refcount to diagsArea
    }
}

ex_split_top_private_state_ext::ex_split_top_private_state_ext()
  : ex_split_top_private_state(),
    commandSent_(NULL),
    rowsAvailable_(NULL)
{
  init();
}

ex_split_top_private_state_ext::~ex_split_top_private_state_ext()
{
}

void ex_split_top_private_state_ext::init()
{
  // BertBert VVV
  activePartNum_ = NULL_COLL_INDEX;
  commandSent_.clear();
  rowsAvailable_.clear();
  rowsBeforeQGetDone_ = 0;
  satisfiedRequestValue_ = 0;
  satisfiedGetNexts_ = 0;
  time_of_stream_get_next_usec_ = 0;
  // BertBert ^^^
}

void ex_split_top_private_state_ext::setHeap(CollHeap *heap)
{
  rowsAvailable_.setHeap(heap);
  commandSent_.setHeap(heap);

  ex_split_top_private_state::setHeap(heap);

}
