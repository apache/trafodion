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
* File:         ex_onlj.C
* Description:  Methods for the tdb and tcb of a nested loop join with
*               the ordered queue protocol
* Created:      5/3/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_onlj.h"
#include "ex_expr.h"
#include "str.h"
#include "ExStats.h"

#include "ex_exe_stmt_globals.h"
#include "sql_buffer_size.h"

/////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
/////////////////////////////////////////////////////////////////////////


//
// Build a nlj tcb
//
ex_tcb * ExOnljTdb::build(ex_globals * glob)
{
  // first build the children
  ex_tcb *  leftTcb;
  ex_tcb *  rightTcb;
  ExOnljTcb *nljTcb;
  
  leftTcb = tdbLeft_->build(glob);
  rightTcb = tdbRight_->build(glob);
  
  nljTcb = new(glob->getSpace()) ExOnljTcb(*this, *leftTcb, *rightTcb, glob);
  
  // add the nljTcb to the schedule
  nljTcb->registerSubtasks();
  
  return (nljTcb);
}

///////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
///////////////////////////////////////////////////////////////////////////

//
// Constructor for nljTcb
//
ExOnljTcb::ExOnljTcb(const ExOnljTdb &  nljTdb,  // 
                         const ex_tcb &    leftTcb,    // left queue pair
                         const ex_tcb &    rightTcb,   // right queue pair
                         ex_globals *glob
                         ) : 
      ex_tcb( nljTdb, 1, glob)
{
  CollHeap * space = glob->getSpace();

  nullPool_ = NULL;

  // Allocate the buffer pool, if 'special' left join
  if ((isLeftJoin()) && (nljTdb.ljExpr_)) {
    pool_ = new(glob->getSpace()) sql_buffer_pool(nljTdb.numBuffers_,
                                                  nljTdb.bufferSize_,
                                                  glob->getSpace());

    // Allocate a NULL tuple for use in null instantiation.
    if (onljTdb().ljRecLen_ > 0) {
      ULng32 nullLength = onljTdb().ljRecLen_;

      Lng32 neededBufferSize = 
        (Lng32) SqlBufferNeededSize( 1, nullLength);
      nullPool_ = new(glob->getSpace()) sql_buffer_pool(1, neededBufferSize, glob->getSpace());
      nullPool_->get_free_tuple(nullData_, nullLength);

      // Fill tuple with NULL values.
      str_pad(nullData_.getDataPointer(), nullLength, '\377');
    }

  }
  
  // Copy predicate pointers
  beforePred_ = nljTdb.preJoinPred_;
  afterPred_ = nljTdb.postJoinPred_;

  // QSTUFF
  // we disable it right now but should later on put in a check which
  // only disables it in the presence of embedded updates and deletes
  // ex_assert(afterPred_ == NULL || isLeftJoin(), "Post join predicate only allowed on outer join");

  tcbLeft_ = &leftTcb;
  tcbRight_ = &rightTcb;

  // get the queues that left and right use to communicate with me
  qleft_  = leftTcb.getParentQueue();
  qright_  = rightTcb.getParentQueue();
  // We don't need state in this up queues

  ex_cri_desc * fromParentCri = nljTdb.criDescDown_;  
  ex_cri_desc * toParentCri = nljTdb.criDescUp_;

  // Allocate the queue to communicate with parent
  allocateParentQueues(qparent_);
  
  phaseOne_ = qparent_.down->getHeadIndex();
  phaseTwo_ = qleft_.up->getHeadIndex();

  // clear state in queues
  
  // fixup expressions
  if (beforePred_)
    (void) beforePred_->fixup(0, getExpressionMode(), this,
                         glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
  if (afterPred_)
    (void) afterPred_->fixup(0, getExpressionMode(), this,
                         glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
  if (onljTdb().ljExpr_)
    (void) onljTdb().ljExpr_->fixup(0, getExpressionMode(), this,
                                    glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
}

/////////////////////////////////////////////////////////////////////////////
// Destructor for nljTcb
//
ExOnljTcb::~ExOnljTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  freeResources();
}
  
///////////////////////////////////////////////////////////////////////////////
// Free Resources
//
void ExOnljTcb::freeResources()
{
  if (pool_)
    delete pool_;
  
  pool_ = 0;

  if (nullPool_) {
    delete nullPool_;
    nullPool_ = NULL;
  }
}

void ExOnljTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // parent down queue and left down queue are handled by work proc 1
  sched->registerInsertSubtask(sWorkPhase1, this, qparent_.down,"P1");
  sched->registerCancelSubtask(sCancel,     this, qparent_.down,"CN");
  sched->registerUnblockSubtask(sWorkPhase1, this, qleft_.down);
  // BertBert VVV
  // The GET_NEXT command causes the sWorkPhase1 function to be called.
  sched->registerNextSubtask(sWorkPhase1, this, qparent_.down,"GN");
  // BertBert ^^^

  // up queue from left and down queue to right are handled by work proc 2
  sched->registerInsertSubtask(sWorkPhase2, this, qleft_.up,"P2");
  sched->registerUnblockSubtask(sWorkPhase2, this, qright_.down);
  
  // up queue from right and up queue to parent are handled by work proc 3
  sched->registerInsertSubtask(sWorkPhase3, this, qright_.up,"P3");
  sched->registerUnblockSubtask(sWorkPhase3, this, qparent_.up);

  // make events through which we can tell the scheduler to call
  // work_phase1() and work_phase3()
  exceptionEvent1_ = sched->registerNonQueueSubtask(sWorkPhase1, this);
  exceptionEvent3_ = sched->registerNonQueueSubtask(sWorkPhase3, this);

  // allow dynamic resizing of parent queues
  registerResizeSubtasks();
}

ex_tcb_private_state * ExOnljTcb::allocatePstates(
       Lng32 &numElems,
       Lng32 &pstateLength)
{
  PstateAllocator<ExOnljPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

short ExOnljTcb::work()
{
  //
  // The work procedures for a NLJ take each parent row through three
  // phases.
  //    + The parent row has to be given to the left child
  //    + For each row returned by the left task for this parent
  //      row, give it to the right child
  //    + For each row returned by the right task for this parent-left
  //      row combination, return it to the parent row.
  //
  // phase three on a parent row may start before phase two finishes
  //
  // The work procedures can be called in any order.
  // The main work procedure (this one) is never called.

  ex_assert(0,"Should never reach ExOnljTcb::work()");
  return WORK_OK;
}

//private procedure to cancel a request
void ExOnljTcb::cancelParentRequest(ex_queue_entry * pentry)
{
  ExOnljPrivateState & pstate = *((ExOnljPrivateState*) pentry->pstate);
  
  pentry->downState.request = ex_queue::GET_NOMORE;
  
  switch(pstate.leftStep_)
  {
  case NLJ_LEFT_NOT_EMPTY:
    // cancel request to left child
    qleft_.down->cancelRequest(pstate.leftIndex_);

    // fall through
  case NLJ_LEFT_DONE:
    // Cancel each row already given to right
    for(;
        pstate.startRightIndex_ != pstate.endRightIndex_;
        pstate.startRightIndex_++ )
    {
      // increment startRightIndex_ in the process to not do it twice
      // The request being cancelled may have been deleted by child
      // already
      qright_.down->cancelRequest(pstate.startRightIndex_);
    }
    break;

  case NLJ_LEFT_STARTED:
    // cancel request to left child
    qleft_.down->cancelRequest(pstate.leftIndex_);
    break;

  case NLJ_LEFT_EMPTY:
  case NLJ_LEFT_CANCELLED:
    // nothing to do. Request was cancelled before
    break;

  } // end switch

}

void ExOnljTcb::handleErrorsFromEval(ex_queue_entry *pentry, ex_queue_entry *uentry)
{

  ExOnljPrivateState & pstate = *((ExOnljPrivateState*) pentry->pstate);

  cancelParentRequest (pentry);

  uentry->upState.status      = ex_queue::Q_SQLERROR;
  uentry->upState.downIndex   = qparent_.down->getHeadIndex();
  uentry->upState.parentIndex = pentry->downState.parentIndex;
  uentry->upState.setMatchNo(pstate.matchCount_);
      
  // insert into parent up queue
  qparent_.up->insert();    
}



ExWorkProcRetcode ExOnljTcb::work_phase1()
{
  ////////////////////////////////////////////////////////////////////////////
  // phase one
  // move rows from parent to left
  // 
  // The class variable phaseOne_ remembers which is the last row for which
  // phase one has been completed.
  
  queue_index    tail = qparent_.down->getTailIndex();
  
  for(; phaseOne_ != tail; phaseOne_++ )
  {
    // if the left down queue is full no more parent rows can be
    // moved to phase one
    if (qleft_.down->isFull())
      return WORK_OK;

    ex_queue_entry * pentry = qparent_.down->getQueueEntry(phaseOne_);
    ex_queue_entry * lentry = qleft_.down->getTailEntry();
    ExOnljPrivateState &pstate = *((ExOnljPrivateState*) pentry->pstate);
    // Initialize private state
    pstate.init();

    const ex_queue::down_request request = pentry->downState.request;

    // request better not be empty
    ex_assert(request != ex_queue::GET_EMPTY,
              "Empty entry inserted in parent queue");
    
    // if request has been cancelled don't bother children
    if (request == ex_queue::GET_NOMORE)
    {
      pstate.leftStep_ = NLJ_LEFT_CANCELLED;

      // to make sure that an end of data queue entry is generated
      // for this request, schedule a task to do this
      if (phaseOne_ == qparent_.down->getHeadIndex())
        exceptionEvent3_->schedule();
    }
    else
    // BertBert VVV
    if (((pentry->downState.request == ex_queue::GET_NEXT_N) ||
         (pentry->downState.request == ex_queue::GET_NEXT_0_MAYBE_WAIT) ||
         (pentry->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT)) &&
        (pentry->downState.numGetNextsIssued == 0))
    {
      // The cursor was opened but not yet fetched. We should not yet work on
      //  this GET_NEXT request because nothing was requested yet (because
      //  numGetNextsIssued is still 0). Don't send this request down (yet).
      return WORK_OK;
    }
    // BertBert ^^^
    else
    {
      // pass same request down.
      lentry->downState.request = request;
      lentry->downState.requestValue = pentry->downState.requestValue;
      // BertBert VVV
      lentry->downState.numGetNextsIssued = pentry->downState.numGetNextsIssued;
      // BertBert ^^^
      
      // if  asking for less than all, ask for all unless it is an outer join
      // with no post join predicate
      if( request == ex_queue::GET_N &&
          !(isLeftJoin() && afterPred_ == NULL))
        lentry->downState.request = ex_queue::GET_ALL;
      
      // remember the entry from where this request came from
      lentry->downState.parentIndex = phaseOne_; // index in parent down q.
      
      // set the atp to the parent atp, init private state for parent row
      lentry->passAtp(pentry);
      pstate.leftIndex_ = qleft_.down->getTailIndex();
      pstate.leftStep_ = NLJ_LEFT_STARTED; // we have just begun

      // Pubsub index join with skip conflict case. 
      if (pstate.rightRecSkipped_ == ExConstants::EX_TRUE) {
	if (pentry->downState.request == ex_queue::GET_NEXT_N)
	  lentry->downState.request = ex_queue::GET_NEXT_N_SKIP;
	else if (pentry->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT)
	  lentry->downState.request = ex_queue::GET_NEXT_N_MAYBE_WAIT_SKIP;
	  }
      
      // insert left entry into left queue
      qleft_.down->insert();

      // BertBert VVV
      if ((pentry->downState.request == ex_queue::GET_NEXT_N) ||
          (pentry->downState.request == ex_queue::GET_NEXT_0_MAYBE_WAIT) ||
          (pentry->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT))
        {
          // The GET_NEXT request is send down to the child node.
          pstate.pushedDownGetNexts_++;
        }
      // BertBert ^^^
    }
  }

  // BertBert VVV
  // If the parent command is a GET_NEXT_N, then we have to watch for updates to that
  //  command. This is the "steady-state" of the GET_NEXT protocol.
  if (!qparent_.down->isEmpty())
    {
      ex_queue_entry * pentry1 = qparent_.down->getHeadEntry();
      ExOnljPrivateState &pstate1 = *((ExOnljPrivateState*) pentry1->pstate);
      // If a new GET_NEXT_N came down and we are not waiting for things to settle so
      //  we can push up a Q_GET_DONE, then send the updated GET_NEXT_N down.
      //  This actually can not happen with the current code because a GET_NEXT_N will 
      //  not be send before a Q_GET_DONE is received for the previous GET_NEXT_N. This
      //  can be changed in the future to allow more asynchronous behavior.
      if ((pentry1->downState.numGetNextsIssued != pstate1.pushedDownGetNexts_) &&
          !pstate1.qGetDoneFromLeft_)
        {
          ex_queue_entry * lentry1 = qleft_.down->getHeadEntry();
          // Check if we are resetting the counts to avoid a rollover of the counts.
          if (pstate1.pushedDownGetNexts_ > pentry1->downState.numGetNextsIssued)
            {
              // We are 'resetting' the request values because we want to prevent overflow of the downstate
              //  fields.  Reinit our pstate fields that are in sync with the downstate fields.
              pstate1.satisfiedGetNexts_ = 0;
              pstate1.satisfiedRequestValue_ = 0;
              pstate1.pushedDownGetNexts_ = 0;
              // Re-init the GET_NEXT request

              if (pentry1->downState.request == ex_queue::GET_NEXT_N || pentry1->downState.request == ex_queue::GET_NEXT_N_SKIP)
                {
		  if (pstate1.rightRecSkipped_ == ExConstants::EX_TRUE)
		    qleft_.down->getNextNSkipRequestInit(pentry1->downState.requestValue);
		  else
		    qleft_.down->getNextNRequestInit(pentry1->downState.requestValue);
                  pstate1.pushedDownGetNexts_++;
                }
              else
              if (pentry1->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT || pentry1->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT_SKIP)
                {
		  if (pstate1.rightRecSkipped_ == ExConstants::EX_TRUE)
		    qleft_.down->getNextNMaybeWaitSkipRequestInit(pentry1->downState.requestValue);
		  else 
		    qleft_.down->getNextNMaybeWaitRequestInit(pentry1->downState.requestValue);
                  pstate1.pushedDownGetNexts_++;
		  }
              else
              if (pentry1->downState.request == ex_queue::GET_NEXT_0_MAYBE_WAIT)
                {
                  qleft_.down->getNext0MaybeWaitRequestInit();
                  pstate1.pushedDownGetNexts_++;
                }
            }
          else
            {
#ifdef TRACE_PAPA_DEQUEUE
              if ((pentry1->downState.numGetNextsIssued - pstate1.pushedDownGetNexts_ ) > 1)
                {
                   cout << "ExOnljTcb::work_phase1, too many requests." 
                        << " pentry1->downState.numGetNextsIssued = " 
                        <<   pentry1->downState.numGetNextsIssued 
                        << " pstate1.pushedDownGetNexts_ = "
                        <<   pstate1.pushedDownGetNexts_ 
                        << endl;
                }
#endif
              // Push down the GET_NEXT request

              if (pentry1->downState.request == ex_queue::GET_NEXT_N || pentry1->downState.request == ex_queue::GET_NEXT_N_SKIP)
                {
		  if (pstate1.rightRecSkipped_ == ExConstants::EX_TRUE)
		    qleft_.down->getNextNSkipRequest(pentry1->downState.requestValue - lentry1->downState.requestValue);
		  else 
		    qleft_.down->getNextNRequest(pentry1->downState.requestValue - lentry1->downState.requestValue);
                  pstate1.pushedDownGetNexts_++;
                }
              else
              if (pentry1->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT || pentry1->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT_SKIP)
                {
		  if (pstate1.rightRecSkipped_ == ExConstants::EX_TRUE)
		    qleft_.down->getNextNMaybeWaitSkipRequest(pentry1->downState.requestValue - lentry1->downState.requestValue);
		  else
		    qleft_.down->getNextNMaybeWaitRequest(pentry1->downState.requestValue - lentry1->downState.requestValue);
                  pstate1.pushedDownGetNexts_++;
                }
		else
		  if (pentry1->downState.request == ex_queue::GET_NEXT_0_MAYBE_WAIT)
		    {
		      qleft_.down->getNext0MaybeWaitRequest();
		      pstate1.pushedDownGetNexts_++;
		    }
            }
        }
      //reset the skipped record flag.
	pstate1.rightRecSkipped_ = ExConstants::EX_FALSE;
    } 
  // BertBert ^^^

  return WORK_OK;
}  // end of ExOnljTcb::work_phase1()

ExWorkProcRetcode ExOnljTcb::work_phase2()
{
  //////////////////////////////////////////////////////////////////////////
  // PHASE TWO
  // move rows from left to right
  //
  // Will process the replies from left for each parent row.
  
  queue_index    tail = qleft_.up->getTailIndex();
  ex_expr::exp_return_type retCode;

  for( ; phaseTwo_ != tail; phaseTwo_++)
  {
    // get pointer to the left and right queue entries and to the parent
    // queue entry for this request
    ex_queue_entry * lentry = qleft_.up->getQueueEntry(phaseTwo_);
    ex_queue_entry * rentry = qright_.down->getTailEntry();
    ex_queue_entry * pentry = qparent_.down->getQueueEntry(
         lentry->upState.parentIndex);
    ExOnljPrivateState &pstate = *((ExOnljPrivateState*) pentry->pstate);
   
    // The left down request for this parent request may have been deleted
    // already by the left child. Be careful to not try to access it

    // if parent doesn't want any more rows then abort processing 
    // of this request
    const ex_queue::down_request & request = pentry->downState.request;

    // ignore data rows except EOD that belong to cancelled parent requests
    if (request == ex_queue::GET_NOMORE &&
        (lentry->upState.status == ex_queue::Q_OK_MMORE ||
         lentry->upState.status == ex_queue::Q_SQLERROR))
    {
      pstate.leftOnlyRows_++;
      if (pstate.leftOnlyRows_ == 1)
      {
        // to make sure that an end of data queue entry is generated
        // for this request, schedule a task to do this
        exceptionEvent3_->schedule();
      }

      if (pstate.leftStep_ == NLJ_LEFT_STARTED)
      {
        // first row returned from left for this parent row
        pstate.leftStep_ = NLJ_LEFT_NOT_EMPTY;

        // Set startRightIndex_ and endRightIndex_ so that cancelRequest
        // doesn't do anything.
        pstate.startRightIndex_ = qright_.down->getTailIndex();
        pstate.endRightIndex_ = qright_.down->getTailIndex();
      }
      continue;  // go to next request
    }
   
    ComDiagsArea *da = NULL;
    // if request was not cancelled let's check what status was returned
    switch(lentry->upState.status)
    {
    case ex_queue::Q_OK_MMORE:
      // row was returned from left insert into right

      // if the right down queue is full no more parent rows can be
      // moved to phase two
      if (qright_.down->isFull())
        return WORK_OK;
    
      if (pstate.leftStep_ == NLJ_LEFT_STARTED)
      {
        // first row returned from left for this parent row
        pstate.leftStep_ = NLJ_LEFT_NOT_EMPTY;
        // the startRightIndex_ is the index of the oldest request (that
        // has not been cancelled) on the right down queue for this
        // parent row.
        pstate.startRightIndex_ = qright_.down->getTailIndex();
      }

      // The endRightIndex_ is the first entry in the right down queue that
      // does not correspond to this parent row
      pstate.endRightIndex_ = qright_.down->getTailIndex() + 1;   

      // we will use the parentIndex of the right down entry to hold the 
      // index of the row that we are sending down to the right. 
      // The scrRequestCount_ in pstate indicates which row 
      //from the original rowset we  are sending to the right. 

      if (onljTdb().isRowsetIterator() && onljTdb().isNonFatalErrorTolerated())
	{
	  rentry->downState.parentIndex  = (Lng32)pstate.srcRequestCount_ + 1 ;
	}
      else
	{
	  ex_assert(pstate.leftOnlyRows_ == 0,
                "Trying to send rows to right after error or cancel");

	  // associate right down request with the left up entry
	  rentry->downState.parentIndex = phaseTwo_;
	}
	          
      // BertBert VVV
      if ((pentry->downState.request == ex_queue::GET_NEXT_N) ||
          (pentry->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT) ||
          (pentry->downState.request == ex_queue::GET_NEXT_0_MAYBE_WAIT))
      {
        // Don't send GET_NEXT_N commands down to the right child because we can't have
        //  two streams!  Only the left child is a stream. The right child can only return one 
        //  row per GET_N request.

	// If this is an index join and a destructive pubsub query , then the 
	// child can indicate that it skipped a record. Change the 
	// request to GET_N_RETURN_SKIPPED. The nested join can handle the 
	// Q_REC_SKIPPED that will be returned onlyf for GET_N_RETURN_SKIPPED
	// request.
	if (onljTdb().isIndexJoin())
	  rentry->downState.request = ex_queue::GET_N_RETURN_SKIPPED;
	else 
	  rentry->downState.request = ex_queue::GET_N;
        rentry->downState.requestValue = 1;
      }
      else
      // BertBert ^^^
      // If we have a semi-join we only want to ask for one
      // row from the right.
      if (isSemiJoin())
      {
          // If it is a semi-join (including anti-semi-join)
          // ask only for one row
          rentry->downState.request = ex_queue::GET_N;
          rentry->downState.requestValue = 1;
      }
      else
      {
        // Otherwise pass down the same request passed by the parent
        rentry->downState.request = request;
  
        if (request == ex_queue::GET_N)
        {
          // compute how many rows to ask for
          rentry->downState.requestValue =
            pentry->downState.requestValue - (Lng32) pstate.matchCount_;
        }
        else
          // request == ex_queue::GET_ALL
          rentry->downState.requestValue = pentry->downState.requestValue;
      }
       if  (onljTdb().isNonFatalErrorTolerated())
	{
          ComDiagsArea *lda = lentry->getDiagsArea();
	  if (lda  && (lda->mainSQLCODE() < 0))
	    {
	      // This will be true if an insert into index raises a nonfatal error
	      // In that case, don't want to pass this to the right tree
	      // which in this case will be a union of undos. This is
	      // because we don't want to see the same diags twice.
	      // save the diags from the left in the pstate
	      if (pstate.accumDiags_)
		{
		  pstate.accumDiags_->mergeAfter(*lda);
		}
	      else
		{
		  pstate.accumDiags_ = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
		  pstate.accumDiags_->mergeAfter(*lda);
		}

	    }
	  // Note: SET behavior (ignore dup rows for indexes is not enabled so
          // the case where (lda->mainSQLCODE == 0)  won't get executed.
          // If it is enabled in the future,  we would get an empty diags
	  // with the OK_MMORE reply.  In that case, don't merge anything into
	  // pstate diags but send the entry to the undo tree to be
	  // undone from base table and possibly other indices.
	  if (lda && ((lda->mainSQLCODE() <0) || (lda->mainSQLCODE()) == 0))
	    {
	      // Don't pass diags area to right
	      lentry->setDiagsAreax(NULL);
	      // Adjust the rowcount in the statement globals
	      // Subtract the rows that were "undone"
              //if we have already subtracted rowcount once for  this
	      // parent entry don't do it again. We only have to do it once per parent entry row.
	      if (!pstate.rowAlreadyRaisedNFError_)
		{
		  ExMasterStmtGlobals *g = getGlobals()->
		    castToExExeStmtGlobals()->castToExMasterStmtGlobals();
		  Int64 rowsAffected = g->getRowsAffected();
		  if(g)
		    {
		      g->setRowsAffected(rowsAffected - 1);
		      pstate.rowAlreadyRaisedNFError_ = TRUE;
		    }
		  else
		    ex_assert(g, "Rowset insert has a flow node that is not in the master executor");
		}
	    }
	}
      // set the atp of the right child to be the same as the left atp
      rentry->passAtp(lentry);

    
      // Apply predicate one on uentry
      // predicate1/beforePred is a predicate that only references the input
      // values and the the output of the first child.
      retCode = ex_expr::EXPR_TRUE;
      if (beforePred_)
        retCode = beforePred_->eval(lentry->getAtp(), 0);

      if (retCode != ex_expr::EXPR_ERROR)
        {
          if(retCode != ex_expr::EXPR_TRUE)
            {
              // If the predicate is not true insert a cancelled
              // request to the second child's down queue
              rentry->downState.request = ex_queue::GET_NOMORE;
            }

          // insert right entry into right queue
          pstate.leftMatches_++;    // increment count
	  pstate.srcRequestCount_++; //increment srcRequestCount, if no expression was evaluated 
	  //or was evaluated without any error.
          qright_.down->insert();
          break;
        }
      // otherwise fall through to the error case

      // .......aaaaaaaaaaaaaaaaiiiiiiiiiiiiii -- I'm falling
    case ex_queue::Q_SQLERROR:
      da = lentry->getDiagsArea();
     
      if (onljTdb().isRowsetIterator() && 
	  onljTdb().isNonFatalErrorTolerated() && 
	  da &&
	  ((da->getNextRowNumber(ComCondition::NONFATAL_ERROR) == 
	    ComCondition::NONFATAL_ERROR) ) )
	{
	  // Check the DA to see if the child node has set this flag
	  if ((da->getNonFatalErrorIndexToBeSet() ) && 
	      (onljTdb().isSetNFErrorJoin())) 
	    {	 
	      // if the da flag is set, the rowcount needs to be set and
	      // this needs to be merged into the pstate diags too
	      da->setAllRowNumber((Lng32)pstate.srcRequestCount_+1);    	    
	      pstate.nonFatalErrorSeen_ = TRUE; 	
	 
	      if (pstate.accumDiags_)
		{	      
		  if (!(pstate.accumDiags_->canAcceptMoreErrors()))	
		    {
		      pstate.nonFatalErrorSeen_ = FALSE;
		      if ( !(da->canAcceptMoreErrors())) 
			da->removeLastErrorCondition();
	    			
		      *da << DgSqlCode(-EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED) 
			  << DgInt0(pstate.accumDiags_->getLengthLimit());
		    }
		  else
		    // pstate.accumDiags->removeLastNonFatalCondition();
		    pstate.accumDiags_->mergeAfter(*da);		
		}
	      else
		{
		  pstate.accumDiags_ = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
		  pstate.accumDiags_->mergeAfter(*da);
		
		  		    
		  ComDiagsArea *cliDiagsArea = pentry->getDiagsArea();
		  pstate.accumDiags_->setLengthLimit(cliDiagsArea->getLengthLimit());
		    	   
		}
	     
	      if (pstate.nonFatalErrorSeen_)
		{
		  pstate.srcRequestCount_++;
		  // one more entry in the left up queue that wasn't sent to the right
		  pstate.leftOnlyRows_++;	 
		  // wake up work_phase3() which may now be able to proceed
		  exceptionEvent3_->schedule();     
		  break;
		}	      
	    }	
	  else
	    // if the da nonfatal error to be flag is not set, leave this entry in
	    // the queue as is. processNonFatalErrorsInLeftUpQueue will
	    // process this entry in phase 3 
	    {
	      pstate.srcRequestCount_++;
	      //one more entry in the left up queue that wasn't sent to the right
	      pstate.leftOnlyRows_++;	 
	      // wake up work_phase3() which may now be able to proceed
	      exceptionEvent3_->schedule();  
	      if (! (onljTdb().isSetNFErrorJoin()))
		da->setNonFatalErrorIndexToBeSet(FALSE);
	      break;
	    }
	}
  
	 	   
	
      if (pstate.leftStep_ == NLJ_LEFT_STARTED)
	{
	  // first row returned from left for this parent row
	  pstate.leftStep_ = NLJ_LEFT_NOT_EMPTY;
	  // the startRightIndex_ is the index of the oldest request (that
	  // has not been cancelled) on the right down queue for this
	  // parent row.
	  pstate.startRightIndex_ = qright_.down->getTailIndex();
	}
      // Set row index for the case when a rowset is used, but 
      // isNonFatalErrorTolerated is FALSE.

      if(onljTdb().isRowsetIterator())
	{
	  ex_assert(da, "To set RowNumber, an error condition must be present in the diags area");
	  da->setAllRowNumber((Lng32)pstate.srcRequestCount_+1);
	}
      // The endRightIndex_ is the first entry in the right down queue that
      // does not correspond to this parent row
      pstate.endRightIndex_ = qright_.down->getTailIndex();

      // no more rows needed, keep the error row in the queue w/o
      // sending it to the right, let phase3 proc handle it     
      cancelParentRequest(pentry);

      // don't send error entry to the right, but remember that there is now
      // one more entry in the left up queue that wasn't sent to the right
      pstate.leftOnlyRows_++;
      
      pstate.srcRequestCount_++;
     
      // wake up work_phase3() which may now be able to proceed
      exceptionEvent3_->schedule();
      break;

    case ex_queue::Q_NO_DATA:
      switch(pstate.leftStep_)
      {
        case NLJ_LEFT_STARTED:
          pstate.leftStep_ = NLJ_LEFT_EMPTY;

          // Set startRightIndex_ and endRightIndex_ so that cancelRequest
          // doesn't do anything.
          pstate.startRightIndex_ = qright_.down->getTailIndex();
          pstate.endRightIndex_ = qright_.down->getTailIndex();

	  // the NO_DATA row from the left child needs to be taken out
	  // of the queue in phase 3 after everything else was processed
	  pstate.leftOnlyRows_++;

          break;
        case NLJ_LEFT_NOT_EMPTY:
	  // MV --
	  // If the right child is a VSBB Insert, and the parent request
	  // was not cancelled, send the GET_EOD request.
	  if (!vsbbInsertOn() ||
	      pentry->downState.request == ex_queue::GET_NOMORE)
	  {
	    // the NO_DATA row from the left child needs to be taken out
	    // of the queue in phase 3 after everything else was processed
	    pstate.leftOnlyRows_++;
            pstate.leftStep_ = NLJ_LEFT_DONE;
	    break;
	  }
          pstate.leftStep_ = NLJ_SEND_EOD;
	  // Fall through to next state.
	case NLJ_SEND_EOD:
	  // MV --
	  // Send the GET_EOD request to the right child.
          if (qright_.down->isFull())
            return WORK_CALL_AGAIN;

	  rentry->downState.request = ex_queue::GET_EOD;
	  rentry->downState.requestValue = pentry->downState.requestValue;
	  rentry->downState.parentIndex = qparent_.down->getHeadIndex();
	  rentry->downState.parentIndex = phaseTwo_;
	  rentry->passAtp(lentry);
	
	  pstate.leftMatches_++;    // increment count
	  pstate.endRightIndex_ = qright_.down->getTailIndex() + 1;
	  
	  qright_.down->insert();

          pstate.leftStep_ = NLJ_LEFT_DONE;
	  break;
        case NLJ_LEFT_EMPTY:
        case NLJ_LEFT_DONE:
        case NLJ_LEFT_CANCELLED:
          ex_assert(0, 
            "ExOnljTcb::work_phase2() Invalid leftStep_ state for Q_NO_DATA");
          break;
      }

      pstate.rowCount_ += lentry->upState.getMatchNo();

      // BertBert VVV
      // remember receiving a Q_NO_DATA from the left child
      pstate.qNoDataFromLeft_ = TRUE;
      // BertBert ^^^

      if (pstate.leftOnlyRows_ > 0 && pstate.leftMatches_ == 0)
      {
        // wake up work_phase3() which may now be able to proceed
        exceptionEvent3_->schedule();
      }

      break;

    // BertBert VVV
    case ex_queue::Q_GET_DONE:
      {
        if (pstate.leftStep_ != NLJ_LEFT_NOT_EMPTY)
        {
          ex_assert(pstate.leftStep_== NLJ_LEFT_STARTED,
            "ExOnljTcb::work_phase2() Invalid leftStep_ state for Q_GET_DONE");
          //
          // Got no rows from the left, so will flow no rows to the right.
          // Set startRightIndex_ and endRightIndex_ so that cancelRequest
          // doesn't do anything.
          pstate.startRightIndex_ = qright_.down->getTailIndex();
          pstate.endRightIndex_ = qright_.down->getTailIndex();
          //
          // Setting leftStep_ to NLJ_LEFT_NOT_EMPTY prevents phase3 from 
          // 'ending' this scan and removing the parent head-entry.
          pstate.leftStep_ = NLJ_LEFT_NOT_EMPTY;
        }
        // remember receiving a Q_GET_DONE from the left child
        pstate.qGetDoneFromLeft_ = TRUE;
        // leave this entry in this queue until the right child has processed all
        //  its rows.  At phase3, this entry will be moved to the parent.
        pstate.leftOnlyRows_++;
        if (pstate.leftOnlyRows_ == 1 && pstate.leftMatches_ == 0)
        {
          // wake up work_phase3() which may now be able to proceed
          exceptionEvent3_->schedule();
        }
      }
      break;

    // BertBert ^^^

     case ex_queue::Q_REC_SKIPPED:
      {
	pstate.srcRequestCount_++;
	// don't send rec_skipped entry to the right, but remember that there is now
	// one more entry in the left up queue that wasn't sent to the right
	pstate.leftOnlyRows_++;

	if (onljTdb().getRowsetRowCountArraySize() > 0)
	{
	  ex_queue_entry * uentry = qparent_.up->getTailEntry();
	  ComDiagsArea *da = uentry->getDiagsArea();
	  if (da == NULL)
	  {
	    da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
	    uentry->setDiagsAreax(da);
	  }
	  da->insertIntoRowsetRowCountArray(((Lng32) pstate.srcRequestCount_),
					      0,
					      onljTdb().getRowsetRowCountArraySize(),
					      getGlobals()->getDefaultHeap());
	}

      }
      if (onljTdb().isRowsetIterator() &&
	  onljTdb().isNonFatalErrorTolerated())
	{
	  // These maybe nonfatal errors which need to be consumed
	  // schedule work phase 3 for this
	  exceptionEvent3_->schedule();
       	}
      break;

    case ex_queue::Q_INVALID:
      ex_assert(0,"ExOnljTcb::work_phase2() INVALID returned by left queue");
      break;
    } // end of switch on status from right
  } // end of for

  return WORK_OK;
}  // end of ExOnljTcb::work_phase2()

ExWorkProcRetcode ExOnljTcb::work_phase3()
{
  ////////////////////////////////////////////////////////////////////////////
  // PHASE THREE
  
  queue_index    phaseThree;  // intialized in the for

  for(;
      (phaseThree = qparent_.down->getHeadIndex()) != phaseOne_;
      qparent_.down->removeHead())
  {
    ex_queue_entry * uentry = qparent_.up->getTailEntry();
    ex_queue_entry * pentry = qparent_.down->getHeadEntry();
    ExOnljPrivateState &pstate = *((ExOnljPrivateState*) pentry->pstate);
    const ex_queue::down_request & request = pentry->downState.request;

    // Try to process the NF errors in the left queue. If there is no room
    // in left up queue then  return and try later
    if (onljTdb().isRowsetIterator() && 
	onljTdb().isNonFatalErrorTolerated())
      {
	NABoolean projected = FALSE;
	NABoolean  consumed = FALSE;
	if (!processNonFatalErrorsInLeftUpQueue(projected,consumed))
	  return WORK_OK;
	if (projected)
	  // restore a good value for uentry
	  uentry = qparent_.up->getTailEntry();

      }
       
    if (pstate.leftStep_ == NLJ_LEFT_STARTED)
      return WORK_OK;  // no phase 3 work for this parent row yet

    ExOperStats *statsEntry = getStatsEntry();
    for( ;
         !qright_.up->isEmpty() && pstate.leftMatches_ > 0;
         qright_.up->removeHead() )
    {
      // At least one row was inserted into right for this parent row and
      // Q_NO_DATA has not been sent yet; try to move rows from right to
      // parent. Even if we cancel the requests to the right we will get the
      // no_data back.

      // get pointer to the left and right queue entries and to the parent
      // queue entry for this request
      ex_queue_entry * rentry = qright_.up->getHeadEntry();
      uentry = qparent_.up->getTailEntry();

      // Try to process the NF errors in the left quue. If we can't return 
      //and try later
      if (onljTdb().isRowsetIterator() && 
	  onljTdb().isNonFatalErrorTolerated())
	{
	  NABoolean projected = FALSE;
	  NABoolean consumed = FALSE; 
	  if (!processNonFatalErrorsInLeftUpQueue(projected, consumed))
	    return WORK_OK;
	  if (projected)
	    {
	      // restore a good value for uentry
		uentry = qparent_.up->getTailEntry();
	    }
	   
	}
    
      // The parent index # given to the right child is the index
      // into the left up queue that generated the down right request.
      // If you know what I mean.

      //Relax this check if this is a rowset iterator since we have changed
      // the meaning of parentIndex of the the rentry to indicate the 
      // row index being sent to the right.

      if (!(onljTdb().isRowsetIterator() && 
	    onljTdb().isNonFatalErrorTolerated()))
	ex_assert(rentry->upState.parentIndex == qleft_.up->getHeadIndex(),   
		  "ex_queue::work3() right up queue out of sync");
    
      ex_queue::up_status right_status = rentry->upState.status;
      ex_expr::exp_return_type retCode;
      ComDiagsArea *da = NULL;
      ComDiagsArea *rda = NULL;      
 
      switch(right_status)
      {
      case ex_queue::Q_OK_MMORE:

        // if parent doesn't want any more rows then ignore this row
        if (request == ex_queue::GET_NOMORE)
          continue;

        if (request == ex_queue::GET_N &&
          pentry->downState.requestValue <= (Lng32)pstate.matchCount_)
        {
          cancelParentRequest(pentry);
          continue;
        }
	rda = rentry->getDiagsArea();

       	// The Insert operator returns a OK_MMORE with a DA that contains a
	//negative sqlcode only in case of non fatal errors.
	//Until then, this assumption is fine
	if (rda && onljTdb().isRowsetIterator() &&
	    onljTdb().isNonFatalErrorTolerated() &&
	    rda->mainSQLCODE() < 0)
	  {
	    // Project this NF error to the parent as a Q_SQLERROR
	    // set the row index to NONFATAL_ERROR and allow the parent node
	    // to set the actual row index
	    rda->setAllRowNumber(ComCondition::NONFATAL_ERROR);
	    if (!qparent_.up->isFull())
	      {
		// set the non fatal error tobe set flag
		rda->setNonFatalErrorIndexToBeSet(TRUE);
		uentry->shareDiagsArea(rda);
		uentry->upState.status = ex_queue::Q_SQLERROR;
		uentry->upState.downIndex = phaseThree;
		uentry->upState.parentIndex = pentry->downState.parentIndex;
		// insert into parent up queue
		qparent_.up->insert();
		continue;
	      }
	    else
	      return WORK_OK;
	  }

	//in some cases when we are ignoring dup keys, we may get diags
	// with no sqlcode - i.e an empty diags area.
	// In that case, project the entry to the TSJ2 as a
	// Q_REC_SKIPPED. This way in phase2 of TSJ2 the counting will be
	// accurate for any other NF errors other than dup that unpack  or
	// the PA have raised.
	// In phase 3 of TSJ2 it will get consumed.
        if (rda && onljTdb().isRowsetIterator() &&
	    onljTdb().isNonFatalErrorTolerated() &&
	    (rda->mainSQLCODE() == 0))
	  {
	      if (!qparent_.up->isFull())
	      {
		uentry->setDiagsAreax(NULL);
		uentry->upState.status = ex_queue::Q_REC_SKIPPED;
		uentry->upState.downIndex = phaseThree;
		uentry->upState.parentIndex = pentry->downState.parentIndex;
		// insert into parent up queue
		qparent_.up->insert();
		continue;
	      }
	    else
	      return WORK_OK;
	  }

        // row returned from right. Give to parent
        ex_assert(!isSemiJoin() || rentry->upState.getMatchNo() == 1,
            "ex_queue::work3() Right returned > 1 row for Semi-join");

        // Apply before predicate on rentry
        if (beforePred_)
        {
          // Apply predicate 1. if not true continue
          retCode = beforePred_->eval(rentry->getAtp(), 0);

          // Insert the error into the qparent_.up only if its not full.
          // If its full then return WORK_OK.
          if (retCode == ex_expr::EXPR_ERROR)
          {
            if (!qparent_.up->isFull())
            {
              uentry->copyAtp(rentry);
              handleErrorsFromEval (pentry, uentry);
              continue;
            }
            else
              return WORK_OK;
          }

          if (retCode != ex_expr::EXPR_TRUE)
          {
            continue;
          }
        }

        // The join has produced a row
        // the row may yet disappear via afterPred_ but we
        // should no longer null instatiate
        pstate.outerMatched_ = ExConstants::EX_TRUE;

        // copy the atp to be the same as the right up atp
        // If it is a semi-join not all tupps will be copied
        uentry->copyAtp(rentry);

        if (onljTdb().ljExpr_)
        {
          if (pool_->get_free_tuple(uentry->getAtp()->getTupp(
                  onljTdb().criDescUp_->noTuples() - 1),
                  onljTdb().ljRecLen_))
          {
            uentry->getAtp()->release();
            return WORK_POOL_BLOCKED; // couldn't allocate, try again later
          }

          retCode = onljTdb().ljExpr_->eval(uentry->getAtp(), 0);

          if (retCode == ex_expr::EXPR_ERROR) 
          {
            if (!qparent_.up->isFull())
            {
              handleErrorsFromEval (pentry, uentry);
              continue;
            }
            else
            {
              uentry->getAtp()->release();
              return WORK_OK;
            }
          }
        }
          
        // Apply predicate two on uentry
        if (afterPred_)
        {
          // Apply predicate 2. if not true continue
          retCode = afterPred_->eval(uentry->getAtp(), 0); 

          if (retCode == ex_expr::EXPR_ERROR) 
          {
            if (!qparent_.up->isFull())
            {
              handleErrorsFromEval (pentry, uentry);
              continue;
            }
            else
            {
              uentry->getAtp()->release();
              return WORK_OK;
            }
          }
              
          if(retCode != ex_expr::EXPR_TRUE)
          {             
            // pred2 is not true. Nothing to be returned.
            uentry->getAtp()->release();
            continue;
          }
        }

        if (! isAntiJoin())
        {
          // check if we've got room in the up queue
          if (qparent_.up->isFull())
          {
            uentry->getAtp()->release();
            return WORK_OK;
          }
          
	  uentry->upState.status = ex_queue::Q_OK_MMORE;
          uentry->upState.downIndex = phaseThree;
          uentry->upState.parentIndex = pentry->downState.parentIndex;
          pstate.matchCount_++;
          uentry->upState.setMatchNo(pstate.matchCount_);

          if (statsEntry)
            statsEntry->incActualRowsReturned();

          // insert into parent up queue
          qparent_.up->insert();
        }

        break;

      case ex_queue::Q_REC_SKIPPED:
	// This case is also treated just like a Q_NO_DATA
	// The record in the right is skipped. This needs to be notified to the left child 
	// so that we can release any locks held for the left node for the skipped record.
	pstate.rightRecSkipped_ = ExConstants::EX_TRUE;

      case ex_queue::Q_NO_DATA:

        // outer join processing
        // If we already returned the number of rows requested
        // cancel the parent request
        if (request == ex_queue::GET_N &&
            pentry->downState.requestValue <= (Lng32)pstate.matchCount_)
          {
            cancelParentRequest(pentry);
          }

	if (onljTdb().getRowsetRowCountArraySize() > 0)
	  {
	    da = uentry->getDiagsArea();
	    if (da == NULL)
	      {
		da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
		uentry->setDiagsAreax(da);
	      }
	    Int64 rowsAffected = 0;
	    if (rentry->getDiagsArea()) {
	      rowsAffected = rentry->getDiagsArea()->getRowCount();
	      rentry->getDiagsArea()->setRowCount(0);
	    }

	    da->insertIntoRowsetRowCountArray(((Lng32) pstate.tgtRequestCount_+1),
					      rowsAffected,
					      onljTdb().getRowsetRowCountArraySize(),
					      getGlobals()->getDefaultHeap());

	  }

        // Add Null instatiated row if no matches for this left up row
        // and the request has not been cancelled
	// -- MVs, If the status of the reply from the left child is
	// Q_NO_DATA, it's probably because this reply from the right child
	// was to a GET_EOD request from us. In this case ignore it.
        if ((isLeftJoin() || isAntiJoin())
            && (pstate.outerMatched_ == ExConstants::EX_FALSE)
            && request != ex_queue::GET_NOMORE
	    && qleft_.up->getHeadEntry()->upState.status != ex_queue::Q_NO_DATA)
	  {
	    // check if we've got room in the up queue
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // copy the atp to be the same as the left up atp
	    // extra tupp's will be set to null
	    uentry->copyAtp(qleft_.up->getHeadEntry());

            // If we have a nullPool, use the pre-allocated nullData.
            if(isLeftJoin() && onljTdb().ljExpr_ && nullPool_) {
              uentry->getAtp()->getTupp(onljTdb().criDescUp_->noTuples() - 1) = nullData_;
            }

	    // make sure we don't null instatiate again
	    pstate.outerMatched_ = ExConstants::EX_TRUE;

	    retCode = ex_expr::EXPR_TRUE;
	    // If no afterPred_, move row to parent, otherwise apply it
	    if (afterPred_)
	      retCode = afterPred_->eval(uentry->getAtp(), 0);

	    if (retCode == ex_expr::EXPR_TRUE)
	      {
		uentry->upState.status = ex_queue::Q_OK_MMORE;
		uentry->upState.downIndex = phaseThree;
		uentry->upState.parentIndex = pentry->downState.parentIndex;
		pstate.matchCount_++;
		uentry->upState.setMatchNo(pstate.matchCount_);

                if (statsEntry)
                  statsEntry->incActualRowsReturned();

		// insert into parent up queue
		qparent_.up->insert();

		// position uentry at the new tail of the up queue
		uentry = qparent_.up->getTailEntry();  
	      }
	    else if (retCode == ex_expr::EXPR_ERROR)
	      {
		handleErrorsFromEval (pentry, uentry);

		// CR 10-010813-4521: Have to remove left up entry as
		// no more data from right that match it. So go ahead
		// to the rest of case statements..

		// continue;
	      }
	    else
	      {
		// afterPred_ is not true. Nothing to be returned.
		uentry->getAtp()->release();
	      }
	  }
	    
        if ( request != ex_queue::GET_NOMORE
	     && rentry->getDiagsArea() )
          {
	    ComDiagsArea *rda = rentry->getDiagsArea();
	   
	    // A Q_NO_DATA and a diags area means a split_top node
	    // is sending back row count.  It could also be a union sending 
	    // back NF errors.  Accumulate this in the
	    // pstate, then send the accumulated diags back with 
	    // the ONLJ Q_NO_DATA.
	    if (onljTdb().isRowsetIterator() && 
		onljTdb().isNonFatalErrorTolerated())
	      {
	      rda->setAllRowNumber(rentry->upState.parentIndex);
	      if (pstate.accumDiags_)
		{
		   if (!(pstate.accumDiags_->canAcceptMoreErrors()))
		    {
		      pstate.nonFatalErrorSeen_ = FALSE;
		  		   
		      if (!rda->canAcceptMoreErrors())
			rda->removeLastErrorCondition();
					   
		      *rda << DgSqlCode(-EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED) 
			      << DgInt0(pstate.accumDiags_->getLengthLimit());
		    }
		    else 
		    pstate.accumDiags_->mergeAfter(*rda);		
		}
	      else
		{
		  pstate.accumDiags_ = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
		  pstate.accumDiags_->mergeAfter(*rda);
		 
		  if (onljTdb().isNonFatalErrorTolerated()) {
		    ComDiagsArea *cliDiagsArea = pentry->getDiagsArea();
		    pstate.accumDiags_->setLengthLimit(cliDiagsArea->getLengthLimit());
		  }	    
		}
	      }
	    else
	      if (pstate.accumDiags_)
		pstate.accumDiags_->mergeAfter(*rda);
	      else
		{
		  pstate.accumDiags_ = rda;
		  rda->incrRefCount();
		}	    
	  }
  
	// now consume the left entry corresponding to the right entry 
	// that we just processed	
	qleft_.up->removeHead();

        // decrement number of left rows in progress
        pstate.leftMatches_--;

        pstate.tgtRequestCount_++;

        pstate.rowCount_ += rentry->upState.getMatchNo();

        // Be ready to null instantiate the new left row
        pstate.outerMatched_ = ExConstants::EX_FALSE;

        break;

      case ex_queue::Q_SQLERROR:

        // if parent doesn't want any more rows then ignore this row
        // even if it is an error
        if (request == ex_queue::GET_NOMORE)
          continue;

        // check if we've got room in the up queue
        if (qparent_.up->isFull())
	  {
	    return WORK_OK;
	  }

        // Copy the child ATP to the parent ATP, but only if this
        // is not a left join, which would change the format of the
        // tupps coming from the right. For a left join,
        // copy the diagnostics area to the parent ATP and copy the
        // tupps from the left only.

        if (isLeftJoin())
        {
          uentry->copyAtp(qleft_.up->getHeadEntry());
          uentry->shareDiagsArea(rentry->getDiagsArea());
        }
        else
        {
          uentry->copyAtp(rentry);
        }

        uentry->upState.status      = rentry->upState.status;
        uentry->upState.downIndex   = phaseThree;
        uentry->upState.parentIndex = pentry->downState.parentIndex;
        uentry->upState.setMatchNo(pstate.matchCount_);

	if(onljTdb().isRowsetIterator())
	  {
	    ComDiagsArea *da = rentry->getDiagsArea();
	    ex_assert(da, "To set RowNumber, an error condition must be present in the diags area");
	    da->setAllRowNumber((Lng32)(pstate.tgtRequestCount_)+1);
	  }

	// insert into parent up queue
	qparent_.up->insert();

        if (! onljTdb().isNonFatalErrorTolerated())
          cancelParentRequest(pentry);

        break;

      default:
        ex_assert(
             0,"ExOnljTcb::work_phase3() Invalid state returned by right");
        break;

      } // end of switch on status
    } // end of for-loop while rows in the right up queue

    // If we need to wait for replies from the second child return now
    if (pstate.leftMatches_ > 0)
      return WORK_OK;

    // BertBert VVV
    // If this is a GET_NEXT_N protocol, then check if it is time to move a Q_GET_DONE
    //  to our parent: The left child must be done generating rows and the right child
    //  must have processed all the rows generated by the left child.
    // This is not done very well.  It uses the error code (while loop below) to remove
    //  the Q_GET_DONE from the left child and to create a Q_GET_DONE for the parent.
    //  This causes queues-out-of-sync errors if multiple GET_NEXT_N requests are 
    //  happening (will not happen now though because there is no code to make it happen).
    //  
    if ((pentry->downState.request == ex_queue::GET_NEXT_N) ||
        (pentry->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT) ||
        (pentry->downState.request == ex_queue::GET_NEXT_0_MAYBE_WAIT) ||
        (pentry->downState.request == ex_queue::GET_NEXT_N_SKIP) ||
        (pentry->downState.request == ex_queue::GET_NEXT_N_MAYBE_WAIT_SKIP))
      {
        // note that pstate.leftMatches_ == 0 at this point
        if (!pstate.qGetDoneFromLeft_ && !pstate.qNoDataFromLeft_)
          return WORK_OK;
        // now go move the Q_GET_DONE from the left child's up-queue to the parent's up-queue.
      }
    // BertBert ^^^

    // If a request was cancelled or had an error in the left queue
    // there may be a number of entries at the head of the left up queue
    // that were never given to the right. Remove them from the left up
    // queue.
    while (pstate.leftOnlyRows_)
      {
        // All rows given to the right for this parent row have been
        // processed. Any remaining rows from the left up queue, if any,
        // have not been given to the right child. Ignore these
        // rows, unless they contain an error.

        ex_queue_entry * lentry = qleft_.up->getHeadEntry();
        ex_assert(lentry->upState.parentIndex == phaseThree,
                  "ex_queue::work3() left up queue out of sync");
        // if this was an error returned from the left child then
        // pass the error on to the parent (right request has already
        // been cancelled)
        if ((lentry->upState.status == ex_queue::Q_SQLERROR) ||
	    (lentry->upState.status == ex_queue::Q_REC_SKIPPED)
	    )
          {
	    if (onljTdb().isRowsetIterator() && 
		onljTdb().isNonFatalErrorTolerated())
	      {
		// Try to process the NF errors in the left queue
		NABoolean projected = FALSE;
		NABoolean consumed = FALSE;
		if (!processNonFatalErrorsInLeftUpQueue(projected,consumed))
		  return WORK_OK;
		if (consumed)
		  continue;
		if (projected)
                         {
		  
		    // restore a good value for uentry
		    uentry = qparent_.up->getTailEntry();
		    continue;
		  }
                 
                 
	      }
	    else
	      {
		if (qparent_.up->isFull())
		  return WORK_OK;

		uentry->copyAtp(lentry);
		uentry->upState.status = lentry->upState.status;
		uentry->upState.downIndex = phaseThree;
		uentry->upState.parentIndex = pentry->downState.parentIndex;
		uentry->upState.setMatchNo(pstate.matchCount_);

		// insert into parent up queue
		qparent_.up->insert();

		// restore a good value for uentry
		uentry = qparent_.up->getTailEntry();
	      }
          }
        else if (lentry->upState.status == ex_queue::Q_NO_DATA)
          {
            // if a warning was returned from left with EOD, return that
            // warning to parent.

            ComDiagsArea *da = lentry->getDiagsArea();
            if (da)
              {
		ComDiagsArea *accumulatedDiagsArea = uentry->getDiagsArea();
		if (accumulatedDiagsArea)
		  accumulatedDiagsArea->mergeAfter(*da);
		else
		  {
		    uentry->shareDiagsArea(da);
		  }
              }
          } // EOD from left

        // BertBert VVV
        if (lentry->upState.status == ex_queue::Q_GET_DONE)
          {
            if (qparent_.up->isFull())
              return WORK_OK;

            uentry->copyAtp(lentry);
            uentry->upState.status = lentry->upState.status;
            uentry->upState.downIndex = phaseThree;
            uentry->upState.parentIndex = pentry->downState.parentIndex;
            uentry->upState.setMatchNo(lentry->upState.getMatchNo());

            // do GET_NEXT_N bookkeeping
            pstate.satisfiedRequestValue_ = lentry->upState.getMatchNo();
            pstate.satisfiedGetNexts_++;
            pstate.qGetDoneFromLeft_ = FALSE;

            // insert into parent up queue
            qparent_.up->insert();

            // restore a good value for uentry
            uentry = qparent_.up->getTailEntry();
          }
        // BertBert ^^^

	qleft_.up->removeHead() ;

        pstate.leftOnlyRows_--;
      }

    // If we haven't seen the EOD from the first child for
    // request then nothing more to do.
    if (pstate.leftStep_ == NLJ_LEFT_NOT_EMPTY)
      return WORK_OK;

    // if the parent up queue is full no more rows can be
    // moved to parent
    if (qparent_.up->isFull())
      return WORK_OK;

    ComDiagsArea *parentDiagsArea = pentry->getDiagsArea();
    // pentry contains all the errors sent in the parent GET_ALL request
    // If there are errors raised in the CLI they will be here. Merge these
    // into the upentry's diags area
    if (onljTdb().isRowsetIterator() && onljTdb().isNonFatalErrorTolerated() )
      {
	if (parentDiagsArea) // This may have been set by the right child
	  // TSJ1 in the daigs below if there were nonfatal errors
	  {
	    // remove the EXE_NONFATAL.. condition/s if any
	    parentDiagsArea->removeLastNonFatalCondition();
	    ComDiagsArea *upDiags = uentry->getDiagsArea();
	    if (upDiags)
	      {
		upDiags->mergeAfter(*parentDiagsArea);
	      }
	    else
	      {
		uentry->shareDiagsArea(parentDiagsArea);
	      }
	  }
      }


    // merge any diags from the upentry with any diags that may be in the
    // parent entry diags area.
    NABoolean anyRowsAffected = FALSE;
    ExMasterStmtGlobals *g = getGlobals()->
      castToExExeStmtGlobals()->castToExMasterStmtGlobals();

    // if a  split-top is returning a diags area,
    // set the accumulated diags in the parent entry.
    // It could also be that the IM and undo nodes raised a few nonfatal errors
    // and all these are being returned in the IM and undo tree's parent node
    //  TSJ3's  pstate.accumDiags_ with a Q_NO_DATA.
    // Or it could be TSJ2 returning NF errors with it's Q_NO_DATA.

    //                                                     Root
    //                                                       |
    //                                                      TSJ2
    //                                                    /    \
    //                                               TSJ1       TSJ3
    //                                            /    \         /\
    //                                     Unpack       Insert  IM Undo
    //                                                  base

      if (pstate.accumDiags_ )
	{
	  ComDiagsArea *upDiags = uentry->getDiagsArea();
	  if (upDiags)
	    {
	      upDiags->mergeAfter(*pstate.accumDiags_);
	      pstate.accumDiags_->decrRefCount();
	    }
	  else
	    // move diags area to the ATP, reference count
	    // in the diags area stays the same
	    uentry->setDiagsAreax(pstate.accumDiags_);
	}
      pstate.accumDiags_ = NULL;
      // Compute the rows affected and set the NF warning for any errors coming from the left or right.
      if (onljTdb().isRowsetIterator() && onljTdb().isNonFatalErrorTolerated() && onljTdb().isSetNFErrorJoin())
	{
	  if (g)
	    {
	      Int64 rowsAffected = g->getRowsAffected();
	      if (g->getRowsAffected() > 0)
		anyRowsAffected = TRUE;
	    }
	  else
	    ex_assert(g, "Rowset insert has a flow node that is not in the master executor");
	  ComDiagsArea *mergedDiags = uentry->getDiagsArea();

	  // if it's a diags just containing rowcount and no error/warning
	  // don't set the nonfatal error code.
	  if ((mergedDiags) && (mergedDiags->mainSQLCODE() < 0))
	    {
	      if (anyRowsAffected)
		*mergedDiags << DgSqlCode(EXE_NONFATAL_ERROR_SEEN);
	      else
		*mergedDiags << DgSqlCode(EXE_NONFATAL_ERROR_ON_ALL_ROWS);
	      mergedDiags->setNonFatalErrorSeen(TRUE);
	    }
	} //if (onljTdb().isRowsetIterator() && onljTdb().isNonFatalErrorTolerated())

      uentry->upState.status = ex_queue::Q_NO_DATA;
      uentry->upState.downIndex = phaseThree;
      uentry->upState.setMatchNo(pstate.rowCount_);
      uentry->upState.parentIndex = pentry->downState.parentIndex;

      // insert into parent up queue
      qparent_.up->insert();
      // done with this parent row.
  } // end of for while rows in the parent down queue

  return WORK_OK;

}  // end of ExOnljTcb::work_phase3()

ExWorkProcRetcode ExOnljTcb::workCancel()
{
  // walk through already processed down requests and cancel those
  // with request type GET_NOMORE (even if this was already done)

  for (queue_index ci = qparent_.down->getHeadIndex();
       ci != phaseOne_;
       ci++)
  {
    ex_queue_entry * pentry = qparent_.down->getQueueEntry(ci);

    if (pentry->downState.request == ex_queue::GET_NOMORE)
      cancelParentRequest(pentry);
  }
  // Make sure that work_phase1 gets a chance to start the cleanup.
  // This is in case the request was canceled before it ever incremented
  // phaseOne_ -- this can easily happen with GET_NEXT_N requests,
  // because when they are first inserted into the parent down 
  // queue, their pstate's numGetNextsIssued will be zero, so 
  // work_phase1 will not have done any work, hence the phaseOne_
  // counter will not be incremented.  
  exceptionEvent1_->schedule();
  return WORK_OK;
}
// This method processes Q_SQLERROR entries and Q_REC_SKIPPED
// in the left queue for nonfatal errors.
// There are 2 levels of NJs when there is IM. 
// The TSJ1 has an unpack node on the left and PA doing an insert on right.
// This TSJ1 identifies the nonfatal error rows and projects them up
//  to TSJ2. The way the Q_SQLERROR entry is marked as NF error is 
// by marking the DA with a flag saying "non fatal error seen".
// The top NJ is the one that sets correct row indexes for all nonfatal errors.
// Q_REC_SKIPPED is sent only for correct counting of rows. The DA associated
// with the Q_REC_SKIPPED is in the parent entry GET_ALL request. The DA was 
// created in the CLI, flowed down to this NJ and even to the unpack, so that 
//the unpack could make a Q_REC_SKIPPED entry and project it up to TSJ2.
// So we don't  process the DA associated with the Q_REC_SKIPPED entries.

NABoolean ExOnljTcb::processNonFatalErrorsInLeftUpQueue(NABoolean &project,
							NABoolean &consumed)
{ 

  queue_index    phaseThree;
  project = FALSE; consumed = FALSE;
  phaseThree = qparent_.down->getHeadIndex();
  ex_queue_entry * uentry = qparent_.up->getTailEntry();
  ex_queue_entry * pentry = qparent_.down->getHeadEntry();
  ExOnljPrivateState &pstate = *((ExOnljPrivateState*) pentry->pstate);
  if ((onljTdb().isRowsetIterator() &&
       onljTdb().isNonFatalErrorTolerated()))
    {

      while (!qleft_.up->isEmpty() && pstate.leftOnlyRows_ &&
	     ((qleft_.up->getHeadEntry()->upState.status == ex_queue::Q_SQLERROR)
	     ||
	      (qleft_.up->getHeadEntry()->upState.status == ex_queue::Q_REC_SKIPPED)
	      )
	     )
        {
	  ComDiagsArea *lentryDa = qleft_.up->getHeadEntry()->getDiagsArea();
	  // If this error has not yet been projected to the top TSJ for setting
	  // the correct row index, project it (This is done by the first level
	  // TSJ
	  if ( lentryDa && !lentryDa->getNonFatalErrorIndexToBeSet() )
	    {
	      if (!qparent_.up->isFull())
		{
		  if ( qleft_.up->getHeadEntry()->upState.status== ex_queue::Q_REC_SKIPPED)
                    uentry->setDiagsAreax(NULL);
                  else
		    {
		      lentryDa->setNonFatalErrorIndexToBeSet(TRUE);
		      uentry->shareDiagsArea(lentryDa);
		    }
		  uentry->upState.status = qleft_.up->getHeadEntry()->upState.status;
		  uentry->upState.downIndex = phaseThree;
		  uentry->upState.parentIndex = pentry->downState.parentIndex;
		  // insert into parent up queue
		  qparent_.up->insert();

                  // consume the left up entry - it's no longer needed
		  qleft_.up->removeHead();
		  pstate.leftOnlyRows_--;
		  project = TRUE;

                   // restore a good value for uentry
		  uentry = qparent_.up->getTailEntry();
		}
	      else
		return FALSE;
	    }
	  else
	    {

	      // else if it has been identified/processed already, consume it
	      // (This is done by the second level TSJ)
	      qleft_.up->removeHead();
	      pstate.leftOnlyRows_--;
	      consumed = TRUE;
	    }
	}
    }
  return  TRUE;
}


/////////////////////////////////////////////////////////////////////////
//
//  Private state procedures
//
/////////////////////////////////////////////////////////////////////////

// Constructor and destructor for nlj_private_state
//
ExOnljPrivateState::ExOnljPrivateState() 
{
  init();
}

void ExOnljPrivateState::init() 
{
  matchCount_      = 0;
  rowCount_	   = 0;
  leftMatches_     = 0;
  leftOnlyRows_    = 0;
  leftIndex_       = 0;
  startRightIndex_ = 0;
  endRightIndex_   = 0;  
  leftStep_        = ExOnljTcb::NLJ_LEFT_DONE;
  outerMatched_    = ExConstants::EX_FALSE;
  rightRecSkipped_ = ExConstants::EX_FALSE;
  accumDiags_      = NULL;
  // rownumber is a 0-based index, therefore the following two counters start at -1
  srcRequestCount_ = -1;  
  tgtRequestCount_ = -1;
  nonFatalErrorSeen_ = FALSE;
  rowAlreadyRaisedNFError_ = FALSE;
  // BertBert VVV
  satisfiedRequestValue_ = 0;
  satisfiedGetNexts_ = 0;
  pushedDownGetNexts_ = 0;
  qGetDoneFromLeft_ = FALSE;
  qNoDataFromLeft_ = FALSE;
  // BertBert ^^^
}

ExOnljPrivateState::~ExOnljPrivateState()
{
}
