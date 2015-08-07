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
* File:         $File$
* RCS:          $Id$
* Description:  
* Created:      
* Language:     C++
* Status:       $State$
*
*
*
*
******************************************************************************
*/

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExSample.h"
#include "ex_expr.h"
#include "ExSimpleSqlBuffer.h"
#include "ExStats.h"

// build - construct the TCB subtree for this TDB.
//
ex_tcb * ExSampleTdb::build(ex_globals * glob)
 {
  // first build the child
  ex_tcb *  child_tcb;
  ExSampleTcb *sampTcb;
  
  child_tcb = tdbChild_->build(glob);
  
  sampTcb = new(glob->getSpace()) ExSampleTcb(*this, *child_tcb, glob);
  
  sampTcb->registerSubtasks();
  
  return (sampTcb);
}


// ExSampleTcb constructor
//
// 1. Allocate buffer pool.
// 2. Allocate parent queues and initialize private state.
// 3. Fixup expressions.
//
ExSampleTcb::ExSampleTcb
(const ExSampleTdb &  myTdb, 
 const ex_tcb &    child_tcb,
 ex_globals * glob
 ) : 
  ex_tcb(myTdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  
  childTcb_ = &child_tcb;

   // get the queue that child use to communicate with me
  qchild_  = child_tcb.getParentQueue(); 

  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
							myTdb.queueSizeDown_,
				     myTdb.criDescDown_,
				     space);
  
  // Allocate the private state in each entry of the down queue
  ExSamplePrivateState *p 
    = new(space) ExSamplePrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;


  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    myTdb.queueSizeUp_,
				    myTdb.criDescUp_,
				    space);

  // Intialized processedInputs_ to the next request to process
  processedInputs_ = qparent_.down->getTailIndex();

  // Fixup the sample expression
  //
  if (initExpr())
    initExpr()->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);

  if (balanceExpr())
    balanceExpr()->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);

  if (postPred())
    postPred()->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);
}

// Destructor
//
//
ExSampleTcb::~ExSampleTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  freeResources();
}
  
// Free Resources
//
//
void ExSampleTcb::freeResources()
{
}

// work - doit...
//
//
short ExSampleTcb::work()
{
  // If there are no parent requests on the queue, then there cannot
  // be anything to do here.
  //
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down;
  ExSamplePrivateState * pstate;
  ex_queue::down_request request;

  // Take any new parent requests and pass them on to the child as long
  // as the child's queue is not full. processedInputs_ maintains the 
  // Queue index of the last request that was passed on.
  // 
  for(queue_index tail = qparent_.down->getTailIndex(); 
      (processedInputs_ != tail) && (!qchild_.down->isFull());
      processedInputs_++ )
    {
      pentry_down = qparent_.down->getQueueEntry(processedInputs_);
      pstate = (ExSamplePrivateState*) pentry_down->pstate;
      request = pentry_down->downState.request;

      // If the request has already been cancelled don't pass it to the
      // child. Instead, just mark the request as done. This will trigger
      // a EOD reply when this request gets worked on.
      //
      if (request == ex_queue::GET_NOMORE)
        {
          pstate->step_ = ExSamp_DONE;
        }
      else
        {
          pstate->step_ = ExSamp_PREWORK;

	  // Pass the request to the child
	  //
          ex_queue_entry * centry = qchild_.down->getTailEntry();
          centry->downState.request = ex_queue::GET_ALL;
          centry->downState.requestValue = 11;
          centry->downState.parentIndex = processedInputs_;
          centry->passAtp(pentry_down);
          qchild_.down->insert();

          // Copy the input atp to the work atp for this request
	  //
          pstate->workAtp_->copyAtp(pentry_down->getAtp());
        }
    } // end for processedInputs_

  pentry_down = qparent_.down->getHeadEntry();
  pstate = (ExSamplePrivateState*) pentry_down->pstate;
  request = pentry_down->downState.request;

  // If the request has not been worked on yet, then
  // initialize the presistent expression variable data and
  // switch to the WORKING state.
  //
  if(pstate->step_ == ExSamp_PREWORK)
    {
      pstate->step_ = ExSamp_WORKING;
      if(balanceExpr()) balanceExpr()->initializePersistentData();
    }
  
  ExOperStats *statsEntry = getStatsEntry(); 
  // Take any child replies and process them. Return the processed
  // rows as long the parent queue has room.
  //
  while (1)
    {
      // If we have satisfied the parent request (or it was cancelled),
      // then stop processing rows, cancel any outstanding child
      // requests, and set this request to the CANCELLED state.
      //
      if((pstate->step_ == ExSamp_WORKING) ||
	 (pstate->step_ == ExSamp_RETURNINGROWS))
	{
	  if ((request == ex_queue::GET_NOMORE) ||
	      ((request == ex_queue::GET_N) &&
	       (pentry_down->downState.requestValue 
		<= (Lng32)pstate->matchCount_)))
	    {
	      qchild_.down->cancelRequestWithParentIndex
		(qparent_.down->getHeadIndex());
	      pstate->step_ = ExSamp_CANCELLED;
	    }
	}

      switch (pstate->step_)
	{
	  // ExSamp_CANCELLED
	  //
	  // Transition to this state from ...
	  // 1. ExSamp_Error - After the error has been processed.
	  // 2. ExSamp_Working - If enough rows have been returned.
	  // 3. ExSamp_Working - If the request was cancelled.
	  //
	  // Remain in this state until ..
	  // 1. All rows from the child including EOD are consumed
	  //
	  // Transition from this state to ...
	  // 1. ExSamp_DONE - In all cases.
	  //
	case ExSamp_CANCELLED:
	  {
	    // There are no extra rows to process from the child yet,
	    // so try again later.
	    //
	    if (qchild_.up->isEmpty())
	      {
		return WORK_OK;
	      }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    ex_queue::up_status child_status = centry->upState.status;

	    // If this is the EOD, transition to the ExSamp_DONE state.
	    //
	    if (child_status == ex_queue::Q_NO_DATA)
		  pstate->step_ = ExSamp_DONE;

	    // Discard the child row.
	    qchild_.up->removeHead();	    
	    break;
	  }

	// ExSamp_ERROR
	//
	// Transition to this state from ...
	// 1. ExSamp_WORKING - a child reply with the type SQLERROR.
	//
	// Remain in this state until ..
	// 1. The error row has been returned to the parent.
	//
	// Transition from this state to ...
	// 1. ExSamp_CANCELLED - In all cases.
	//
	case ExSamp_ERROR:
	  {
            // If there is no room in the parent queue for the reply,
	    // try again later.
	    //
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    ex_queue_entry *pentry_up = qparent_.up->getTailEntry();
	    ex_queue_entry * centry = qchild_.up->getHeadEntry();

	    // Cancel the child request - there must be a child request in
	    // progress to get to the ExSamp_ERROR state.
	    //
	    qchild_.down->cancelRequestWithParentIndex
	      (qparent_.down->getHeadIndex());

	    // Construct and return the error row.
	    //
            pentry_up->copyAtp(centry);
	    pentry_up->upState.status = ex_queue::Q_SQLERROR;
	    pentry_up->upState.parentIndex 
	      = pentry_down->downState.parentIndex;
	    pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
            pentry_up->upState.setMatchNo(pstate->matchCount_);
            qparent_.up->insert();

	    // Transition to the ExSamp_CANCELLED state.
	    //
	    pstate->step_ = ExSamp_CANCELLED;
	    break;
	  }

	// ExSamp_WORKING
	//
	// Transition to this state from ...
	// 1. ExSamp_EMPTY - If a request is started.
	//
	// Remain in this state until ...
	// 1. All child replies including EOD have been processed.
	// 2. A SQLERROR row is received.
	// 3. Enough rows have been returned.
	// 4. The request is cancelled.
	//
	// Transition from this state to ...
	// 1. ExSamp_DONE - If all the child rows including EOD have 
	//    been processed.
	// 2. ExSamp_ERROR - If an SQLERROR rows is received.
	// 3. ExSamp_CANCELLED - If enough rows have been returned.
	// 3. ExSamp_CANCELLED - If the request is cancelled.
	//
	case ExSamp_WORKING:
	  {
	    // If there is not room in the parent Queue for the reply,
	    // try again later.
	    //
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // If there are no replies, try again later.
	    //
	    if (qchild_.up->isEmpty())
	      return WORK_OK;

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    switch (centry->upState.status)
	      {
		
		// A data row from the child.
		//
	      case ex_queue::Q_OK_MMORE:
		{
		  // Apply the sampling predicate if it exists and extract
		  // the sampling factor.
		  //
		  ex_expr::exp_return_type retCode = ex_expr::EXPR_TRUE;
		  Int32 samplingFactor = 1;
		  if(balanceExpr())
		    {
		      retCode = balanceExpr()->eval
			(centry->getAtp(), centry->getAtp());
		      
		      if(retCode == ex_expr::EXPR_OK)
			{
			  samplingFactor = 
			    *(Lng32*)balanceExpr()->getPersistentData
			    (returnFactorOffset());
		      
			  // If the sampling factor is less than 0, then
			  // we are done with this request. Mark the
			  // request as get-no-more. Forces the child to be
                          // cancelled
			  //
			  if(samplingFactor < 0)
			    {
                              // Cancel the rest of this
                              // request.
			      pentry_down->downState.request 
				= ex_queue::GET_NOMORE;
			      request = ex_queue::GET_NOMORE;
                              
                              // Return no rows
			      retCode = ex_expr::EXPR_FALSE;
			    }
			}
		    }
		  
		  // If the row passed the sampling predicate, apply the 
		  // selection predicate if it exists.
		  //
		  if ((samplingFactor > 0) && 
		      (retCode == ex_expr::EXPR_OK) && 
		      postPred()) 
		    retCode = postPred()->eval
		      (centry->getAtp(), centry->getAtp());

		  // Act on the result of the selection predicate.
		  //
		  switch(retCode) {
		    // If the selection predicate returns TRUE,
		    // return the row to the parent the number
		    // of times indicated by the sampling factor as long
		    // as there is room in the parent queue.
		    //
		  case ex_expr::EXPR_TRUE:
		  case ex_expr::EXPR_OK:
		    while(!qparent_.up->isFull() && (samplingFactor > 0))
		      {
			// Copy the child ATP to the parent ATP -- the 
			// row images are exactly the same.
			//
			ex_queue_entry * pentry_up 
			  = qparent_.up->getTailEntry();
			pentry_up->copyAtp(centry);

			// Fixup the up state.
			//
			pentry_up->upState.status = ex_queue::Q_OK_MMORE;
			pentry_up->upState.parentIndex 
			  = pentry_down->downState.parentIndex;
			pentry_up->upState.downIndex 
			  = qparent_.down->getHeadIndex();
			pstate->matchCount_++;
			pentry_up->upState.setMatchNo(pstate->matchCount_);

			// Commit the entry.
			//
			qparent_.up->insert();
                        if (statsEntry)
                           statsEntry->incActualRowsReturned();
			samplingFactor--;

			// If we have satisfied a GET_N request, then
			// break out of here so we can stop processing.
			//
			if((request == ex_queue::GET_N) &&
			   (pentry_down->downState.requestValue 
			    <= (Lng32)pstate->matchCount_))
			  {
			    samplingFactor = 0;
			    break;
			  }
		      }

		    // If all of the rows are returned, then we are done
		    // with this entry and can proceed. 
		    //
		    if(samplingFactor == 0)
		      {
			qchild_.up->removeHead();
		      }
		    // Otherwise, the queue must have become full so we 
		    // have to switch to the RETURNINGROWS state to finish 
		    // the job.
		    else
		      {
			pstate->rowsToReturn_ = samplingFactor;
			pstate->step_ = ExSamp_RETURNINGROWS;
			return WORK_OK;
		      }
		      
		    break;
		    
		    // If the selection predicate returns FALSE,
		    // do not return the child row.
		    //
		  case ex_expr::EXPR_FALSE:
		    qchild_.up->removeHead();
		    break;

		    // If the selection predicate returns an ERROR,
		    // go to the error processing state.
		    //
		  case ex_expr::EXPR_ERROR:
		    pstate->step_ = ExSamp_ERROR;
		    break;
		  }
		}
		break;

		// The EOD from the child. Transition to ExSamp_DONE.
		//
	      case ex_queue::Q_NO_DATA:
		pstate->step_ = ExSamp_DONE;
		qchild_.up->removeHead();
		break;

		// An SQLERROR from the child. Transition to ExSamp_ERROR.
		//
	      case ex_queue::Q_SQLERROR:
		pstate->step_ = ExSamp_ERROR;
		break;
	      }
	  }
	break;

	// ExSamp_RETURNINGROWS
	//
	// Transistion to this state from ...
	// 1. ExSamp_WORKING - if up queue becomes full when returning 
	//                    multiple rows from oversampling.
	//
	// Remain in this state until ...
	// 1. the multiple oversampled rows have been returned.
	//
	case ExSamp_RETURNINGROWS:
	  {
	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    while(!qparent_.up->isFull() && (pstate->rowsToReturn_ > 0))
	      {
		// Copy the child ATP to the parent ATP -- the row
		// images are exactly the same.
		//
		ex_queue_entry * pentry_up = qparent_.up->getTailEntry();
		pentry_up->copyAtp(centry);
		
		// Fixup the up state.
		//
		pentry_up->upState.status = ex_queue::Q_OK_MMORE;
		pentry_up->upState.parentIndex 
		  = pentry_down->downState.parentIndex;
		pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
		pstate->matchCount_++;
		pentry_up->upState.setMatchNo(pstate->matchCount_);
		
		// Commit the entry.
		//
		qparent_.up->insert();
                if (statsEntry)
                   statsEntry->incActualRowsReturned();

		pstate->rowsToReturn_--;

		// If we have satisfied a GET_N request, then
		// break out of here so we can stop processing.
		//
		if((request == ex_queue::GET_N) &&
		   (pentry_down->downState.requestValue 
		    <= (Lng32)pstate->matchCount_))
		  {
		    pstate->rowsToReturn_ = 0;
		    break;
		  }
	      }

	    // If all of the rows were returned, remove the child's reply
	    // and go back to the WORKING state.
	    //
	    if(pstate->rowsToReturn_ == 0)
	      {
		qchild_.up->removeHead();
		pstate->step_ = ExSamp_WORKING;
	      }
	    // Otherwise, we need to come back later to finish up.
	    //
	    else
	      {
		return WORK_OK;
	      }
	  }
	break;

	// ExSamp_DONE
	//
	// Transition to the state from ...
	// 1. ExSamp_WORKING - if all child rows have been processed.
	// 2. ExSamp_CANCELLED - if all child rows have been consumed.
	// 3. ExSamp_EMPTY - if the request was DOA.
	//
	// Remain in this state until ...
	// 1. The EOD is returned to the parent.
	//
	// Transition from this state to ...
	// 1. ExSamp_EMPTY - In all cases.
	//
	case ExSamp_DONE:
	  {
	    // If there is not any room in the parent's queue, 
	    // try again later.
	    //
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    
	    ex_queue_entry * pentry_up = qparent_.up->getTailEntry();
	    pentry_up->upState.status = ex_queue::Q_NO_DATA;
	    pentry_up->upState.parentIndex 
	      = pentry_down->downState.parentIndex;
	    pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
	    pentry_up->upState.setMatchNo(pstate->matchCount_);
	    
	    qparent_.down->removeHead();
	    qparent_.up->insert();

            // Re-initialize pstate
	    //
	    pstate->step_ = ExSamp_EMPTY;
            pstate->matchCount_ = 0;
	    pstate->rowsToReturn_ = 0;
            pstate->workAtp_->release();

	    // If there are no more requests, simply return.
	    //
            if (qparent_.down->isEmpty())
              return WORK_OK;

            // If we haven't given to our child the new head
            // index return and ask to be called again.
	    //
            if (qparent_.down->getHeadIndex() == processedInputs_)
	      return WORK_CALL_AGAIN;

            // Postion at the new head of the request queue.
	    //
            pentry_down = qparent_.down->getHeadEntry();
            pstate = (ExSamplePrivateState*) pentry_down->pstate;
            request = pentry_down->downState.request;

	    // If the request has not been worked on yet, then initialize the
	    // presistent expression variable data and switch to the 
	    // WORKING state.
	    //
	    if(pstate->step_ == ExSamp_PREWORK)
	      {
		pstate->step_ = ExSamp_WORKING;
		if(balanceExpr()) balanceExpr()->initializePersistentData();
	      }
	  }
	break;
	} // switch pstate->step_
    } // while
}


// Constructor and destructor private state
//
//
ExSamplePrivateState::ExSamplePrivateState
(const ExSampleTcb *  tcb)
{
  matchCount_ = 0;
  step_ = ExSampleTcb::ExSamp_EMPTY;
  workAtp_ = allocateAtp(tcb->myTdb().criDescUp_, ((ex_tcb*)tcb)->getSpace());
}

ExSamplePrivateState::~ExSamplePrivateState()
{
};

ex_tcb_private_state * ExSamplePrivateState::allocate_new
(const ex_tcb *tcb)
{
  return new(((ex_tcb*)tcb)->getSpace()) 
    ExSamplePrivateState((ExSampleTcb*) tcb);
};
