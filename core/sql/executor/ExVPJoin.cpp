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
* File:         ExVPJoin.h
* Description:  VP (Vertical Partition) Join
*               
* Created:      5/3/94
* Language:     C++
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExVPJoin.h"
#include "ex_expr.h"

// Exclude this code from coverage analysis since this feature is
// obsolete and not used.

ex_tcb * ExVPJoinTdb::build(ex_globals * glob)
{
  // first build the child TCBs
  //
  const ex_tcb **childTcbs = (const ex_tcb**) new (glob->getSpace()) ex_tcb* [numChildren_];
  short i;
  for (i=0; i<numChildren_; i++)
    {
      childTcbs[i] = childTdbs_[i]->build(glob);
    }

  // then VPJoin TCB
  //
  ExVPJoinTcb *vpjTcb = 
    new (glob->getSpace()) ExVPJoinTcb(*this, childTcbs, glob);
  
  // and finally register work tasks
  vpjTcb->registerSubtasks();
  
  return vpjTcb;
}


ExVPJoinTcb::ExVPJoinTcb(const ExVPJoinTdb & vpjTdb,
			 const ex_tcb ** childTcbs,
			 ex_globals *glob)
  : ex_tcb(vpjTdb, 1, glob),
    childTcbs_(childTcbs)
{
  CollHeap * space = glob->getSpace();

  numChildren_ = vpjTdb.numChildren();

  // Allocate array of child queue pairs (i.e., queue pairs for
  // communicating with children.
  //
  qChild_ = (ex_queue_pair*) new (space) ex_queue_pair[numChildren_];

  // Initialize array.
  //
  Int32 i;
  for (i=0; i<numChildren_; i++)
    {
      qChild_[i] = childTcbs_[i]->getParentQueue();
    }

  // Allocate queue for passing requests from parent to this node,
  // and initialize private state associated with each queue entry.
  //
  qParent_.down = new (space) ex_queue(ex_queue::DOWN_QUEUE,
				       vpjTdb.fromParent_,
				       vpjTdb.givenCriDesc_,
				       space);
  ExVPJoinPrivateState ps(this);
  qParent_.down->allocatePstate(&ps, this);

  // Allocate queue for passing replies from this node to parent.
  //
  qParent_.up = new (space) ex_queue(ex_queue::UP_QUEUE,
				     vpjTdb.toParent_,
				     vpjTdb.returnedCriDesc_,
				     space);

  if (filterPred())
    (void) filterPred()->fixup(0, getExpressionMode(), this,
                               glob->getSpace(), glob->getDefaultHeap());

  // Record index of next down request.
  nextReqIx_ = qParent_.down->getHeadIndex();
}

ExVPJoinTcb::~ExVPJoinTcb()
{
  delete qParent_.up;
  delete qParent_.down;

  freeResources();
}

ex_queue_pair ExVPJoinTcb::getParentQueue() const 
{
  return qParent_;
}
 
const ex_tcb* ExVPJoinTcb::getChild(Int32 pos) const
{
   ex_assert((pos >= 0) && (pos < numChildren()), ""); 
   return childTcbs_[pos];
}

Int32 ExVPJoinTcb::numChildren() const 
{ 
  return vpJoinTdb().numChildren();
} 



void ExVPJoinTcb::freeResources()
{}

void ExVPJoinTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // parent and child down queues are handled by workDown()
  sched->registerInsertSubtask(sWorkDown, this, qParent_.down, "Work Down");
  sched->registerCancelSubtask(sWorkCancel, this, qParent_.down, "Work Cancel");
  Int32 i;
  for (i=0; i<numChildren_; i++)
    {
      sched->registerUnblockSubtask(sWorkDown, this, qChild_[i].down);
    }

  // parent and child up queues are handled by workUp()
  sched->registerUnblockSubtask(sWorkUp, this, qParent_.up, "Work Up");
  for (i=0; i<numChildren_; i++)
    {
      sched->registerInsertSubtask(sWorkUp, this, qChild_[i].up, "Work Up");
    }

  // make an event through which we can tell the scheduler to call
  // workUp().
  exceptionEvent_ = sched->registerNonQueueSubtask(sWorkUp, this);
}

short ExVPJoinTcb::work()
{
  // The work procedures for VPJoin first pass a request from the
  // parent node to all child nodes.  Replies are then merged (the
  // i'th reply from each child is merged to form the i'th reply to
  // the parent) and passed to the parent.  Work procedure workDown()
  // handles the first stage, passing requests down to children, while
  // procedure workUp() handles the second.
  //
  // The work procedures can be called in any order.
  // The main work procedure (this one) is never called.
  //

  ex_assert(0, "Should never reach ExVPJoinTcb::work()");
  return WORK_OK;
}

ExWorkProcRetcode ExVPJoinTcb::workDown()
{
  // Move requests from parent down queue to child down queues.
  // Data member nextReqIx_ points to the next request to be passed
  // down.  
  // 
  
  queue_index tail = qParent_.down->getTailIndex();
  
  for(; nextReqIx_!=tail; nextReqIx_++)
  {
    // Check that no child down queue is full.  This is the criterion used
    // for determining when to pass a new request to child nodes.  This is
    // a bit more strict than it has to be, as we could potentially start
    // some children, even though not all have space in their down queue.
    // However, the bookkeeping overhead and complexity of this more lenient 
    // approach hardly seems worth it.
    //
    Int32 i;
    for (i=0; i<numChildren_; i++)
      if (qChild_[i].down->isFull())
	return WORK_OK;

    ex_queue_entry * pentry = qParent_.down->getQueueEntry(nextReqIx_);
    const ex_queue::down_request request = pentry->downState.request;
    ExVPJoinPrivateState &pstate = *((ExVPJoinPrivateState*) pentry->pstate);
    pstate.init();

    if (request!=ex_queue::GET_NOMORE)
      {
	// Request has not been cancelled; pass down to children.
	//
	for (i=0; i<numChildren_; i++)
	  {
	    ex_queue_entry * centry = qChild_[i].down->getTailEntry();

	    // pass same request down.
	    centry->downState.request = request;
	    centry->downState.requestValue = pentry->downState.requestValue;
      
	    // remember the index of this request in the parent down queue
	    centry->downState.parentIndex = nextReqIx_;
      
	    // set the atp to the parent atp
	    centry->passAtp(pentry);
      
	    // finally, insert in child's down queue
	    qChild_[i].down->insert();
	  }

	pstate.started_ = 1;
      }
    else
      {
	// Request is cancelled; schedule workUp() to handle cancellation.
	//
	exceptionEvent_->schedule();
      }
  }

  return WORK_OK;
}

ExWorkProcRetcode ExVPJoinTcb::workUp()
{
  // Move replies from child nodes up to parent.  The replies at the
  // heads of child up-queues are merged into a single reply to the
  // parent.  If a filter predicate is specified, then a reply is sent
  // to the parent node only if the predicate expression evaluates to
  // TRUE.
  // 

  queue_index ix;

  for(; (ix=qParent_.down->getHeadIndex()) != nextReqIx_; 
      qParent_.down->removeHead())
    {
      ex_queue_entry * upEntry = 0;
      ex_queue_entry * pentry = qParent_.down->getHeadEntry();
      ExVPJoinPrivateState &pstate = *((ExVPJoinPrivateState*) pentry->pstate);
      const ex_queue::down_request & request = pentry->downState.request;
      Int32 requestCancelled = (request == ex_queue::GET_NOMORE);
      Int32 isGetNRowsRequest = (request == ex_queue::GET_N);
      Lng32 numRowsOfGetNRequest = pentry->downState.requestValue;
		      
      if (pstate.started_) 
	{
	  // Request has been started, which means that child nodes
	  // are producing replies for current request.  Read replies,
	  // merge them, and, if request hasn't been cancelled and
	  // filter expression evaluates to TRUE (if there is a filter
	  // expression), pass reply to parent.
	  //

	  Lng32 eodReply = 0; // is current reply from children end-ofdata?

	  // Loop until there are no more replies to process or until
	  // end-of-data is read for current request.
	  //
	  while (!eodReply)
	    {
	      // If a child up-queue is empty, then return.
	      //
	      Int32 i;
	      for (i=0; i<numChildren_; i++)
		if (qChild_[i].up->isEmpty())
		  return WORK_OK;

	      // Since all children produce the same number of results for
	      // any given request, we just need to look at the first child
	      // to determine if the current reply from each child is EOD.
	      //
	      eodReply = 
		qChild_[0].up->getHeadEntry()->upState.status==ex_queue::Q_NO_DATA;

	      if (!upEntry)
		{
		  // Get a pointer to the entry in this node's parent up queue
		  // that will receive the current reply.

		  if (qParent_.up->isFull())
		    // no room in parent up queue; try again later
		    return WORK_OK; 

		  upEntry = qParent_.up->getTailEntry();
		  upEntry->copyAtp(pentry);
		}

	      if (!requestCancelled && !eodReply)
		{
		  // Request isn't cancelled and it isn't end-of-data.
		  // Evaluate filter predicate, if there is one.
		  //
		  // Build ATP that is of the right structure both for
		  // predicate evaluation and replying to parent.
		  //
		  // The result ATP from each child looks as follows:
		  //
		  //   ------------------------------
		  //   |input data|child output data|
		  //   ------------------------------
		  // 
		  // Data member firstReplyAtpIndex_ records the atp index
		  // where output data starts.  Output from child nodes is
		  // placed in the result ATP in the following manner:
		  //
		  //   ----------------------------------------------
		  //   |input data|child 1 output|child 2 output|...|
		  //   ----------------------------------------------
		  //
		  // This is also the ATP structure that is assumed by
		  // the predicate expression.
		  //

		  atp_struct * resAtp = upEntry->getAtp(); // result ATP
		  short outIx = vpJoinTdb().firstReplyAtpIx_; 
                                                    // where output data begins, 
		                                    // both in result ATP and ATPs
		                                    // returned from child nodes
		  for (i=0; i<numChildren_; i++)
		    {
		      // current reply from i'th child
		      const ex_queue_entry *reply = qChild_[i].up->getHeadEntry();

		      // Append this child's output to result ATP.
		      //
		      short numResTups = reply->numTuples()-vpJoinTdb().firstReplyAtpIx_;
		      short dest_start_tupp = outIx;
		      short src_start_tupp = vpJoinTdb().firstReplyAtpIx_;
		      short src_end_tupp =  numResTups + vpJoinTdb().firstReplyAtpIx_ - 1;
		      

		      resAtp->copyPartialAtp(reply->getAtp(), // source atp
					     dest_start_tupp, // starting ix in resAtp
					     src_start_tupp, // starting ix in reply
					     src_end_tupp); // ending ix in reply
		      outIx += numResTups;
		    }
		  
		  if (!filterPred() ||
		      filterPred()->eval(resAtp,0)==ex_expr::EXPR_TRUE)
		    {
		      // There is either no filter predicate or there is one and
		      // it evaluated to TRUE for the current reply, so return reply
		      // to parent node.
		      //
		      upEntry->upState.parentIndex = pentry->downState.parentIndex;
		      upEntry->upState.downIndex = ix;
		      upEntry->upState.setMatchNo(pstate.matchCount_++);
		      upEntry->upState.status = ex_queue::Q_OK_MMORE;
		      qParent_.up->insert();
		      upEntry = 0;
		      
		      if (isGetNRowsRequest &&
			  (Lng32) pstate.matchCount_ >= numRowsOfGetNRequest)
			{
			  // Current request is of the type "get N rows" and
			  // we have returned at least N rows, so cancel the
			  // request.
			  //
			  cancelParentRequest(pentry, ix);
			  requestCancelled = 1;
			}
		    }
		}
	      
	      for (i=0; i<numChildren_; i++)
		qChild_[i].up->removeHead();
	    }
	}
      else
	{
	  // Request hasn't been started, which means that it is 
	  // cancelled and hasn't been sent to children.
	  //
	  ex_assert(request==ex_queue::GET_NOMORE,
		    "Expected request to be cancelled");
	  
	  if (qParent_.up->isFull())
	    // no room in parent up queue; try again later
	    return WORK_OK; 

	  upEntry = qParent_.up->getTailEntry();
	  upEntry->copyAtp(pentry);
	}

      // Return end-of-data to parent.
      upEntry->upState.parentIndex = pentry->downState.parentIndex;
      upEntry->upState.downIndex = ix;
      upEntry->upState.setMatchNo(pstate.matchCount_);
      upEntry->upState.status = ex_queue::Q_NO_DATA;
      qParent_.up->insert();
      upEntry = 0;
    }

    return WORK_OK;
}

void ExVPJoinTcb::cancelParentRequest(ex_queue_entry * pentry, 
				      queue_index pix)
{
  ExVPJoinPrivateState & pstate = *((ExVPJoinPrivateState*) pentry->pstate);
  
  pentry->downState.request = ex_queue::GET_NOMORE;
  
  if (pstate.started_)
    {
      Int32 i;
      for (i=0; i<numChildren_; i++)
	{
	  qChild_[i].down->cancelRequestWithParentIndex(pix);
	}
    }
}

ExWorkProcRetcode ExVPJoinTcb::workCancel()
{
  // Walk through already processed down requests and cancel those
  // with request type GET_NOMORE (even if this was already done)

  for (queue_index ci = qParent_.down->getHeadIndex();
       ci < nextReqIx_;
       ci++)
  {
    ex_queue_entry * pentry = qParent_.down->getQueueEntry(ci);

    if (pentry->downState.request == ex_queue::GET_NOMORE)
      cancelParentRequest(pentry, ci);
  }

  return WORK_OK;
}

short ExVPJoinTcb::sWorkDown(ex_tcb *tcb)
   { return ((ExVPJoinTcb *) tcb)->workDown(); }
short ExVPJoinTcb::sWorkUp(ex_tcb *tcb) 
   { return ((ExVPJoinTcb *) tcb)->workUp(); }
short ExVPJoinTcb::sWorkCancel(ex_tcb *tcb)
   { return ((ExVPJoinTcb *) tcb)->workCancel(); }

ExVPJoinPrivateState::ExVPJoinPrivateState(const ExVPJoinTcb * tcb)
{
  init();
}

ex_tcb_private_state * ExVPJoinPrivateState::allocate_new(const ex_tcb * tcb)
{
  return new (((ex_tcb*)tcb)->getSpace()) ExVPJoinPrivateState((ExVPJoinTcb*) tcb);
}

ExVPJoinPrivateState::~ExVPJoinPrivateState()
{}

