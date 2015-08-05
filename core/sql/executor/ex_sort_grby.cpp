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
* File:         ex_sort_grby.cpp
* Description:  Methods for the tdb and tcb of sort aggregate/grby operator
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
// with a sort aggr/grby operation.
//

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_sort_grby.h"
#include "ex_expr.h"
#include "ExSimpleSqlBuffer.h"
#include "ExStats.h"
#include "exp_clause.h"
////////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
////////////////////////////////////////////////////////////////////////////////

//
// Build an sort_grby tcb
//
ex_tcb * ex_sort_grby_tdb::build(ex_globals * glob)
{
  // first build the child
  ex_tcb *  child_tcb;
  ex_sort_grby_tcb *sort_grby_tcb;
  
  child_tcb = tdbChild_->build(glob);
  
  sort_grby_tcb = new(glob->getSpace())
    ex_sort_grby_tcb(*this, *child_tcb, glob);
  
  // add the sort_grby_tcb to the schedule, allow dynamic queue resizing
  sort_grby_tcb->registerSubtasks();
  sort_grby_tcb->registerResizeSubtasks();

  return (sort_grby_tcb);
}


//////////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
//////////////////////////////////////////////////////////////////////////////////

//
// Constructor for sort_grby_tcb
//
ex_sort_grby_tcb::ex_sort_grby_tcb(const ex_sort_grby_tdb &  sort_grby_tdb, 
				   const ex_tcb &    child_tcb,   // child queue pair
				   ex_globals * glob
				    ) : 
    ex_tcb( sort_grby_tdb, 1, glob),
    pool_(NULL)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  
  childTcb_ = &child_tcb;

  // Allocate the buffer pool
#pragma nowarn(1506)   // warning elimination 
  pool_ = new(space) ExSimpleSQLBuffer(sort_grby_tdb.numBuffers_,
				       sort_grby_tdb.bufferSize_,
		                       recLen(),
				       space);
#pragma warn(1506)  // warning elimination 

  // get the child queue pair
  qchild_  = child_tcb.getParentQueue(); 

  // Allocate the queue to communicate with parent
  allocateParentQueues(qparent_);

  // Intialize processedInputs_ to the next request to process
  processedInputs_ = qparent_.down->getTailIndex();

  // allocate work atp
  workAtp_ = allocateAtp(sort_grby_tdb.criDescUp_,getSpace());

  // fixup aggr expression
  if (aggrExpr())
    (void) aggrExpr()->fixup(0, getExpressionMode(), this, space, heap,
			     glob->computeSpace(), glob);

  // fixup move expression
  if (moveExpr())
    (void) moveExpr()->fixup(0, getExpressionMode(), this, space, heap,
			     glob->computeSpace(), glob);

  // fixup grby expression
  if (grbyExpr())
    (void) grbyExpr()->fixup(0, getExpressionMode(), this, space, heap,
			     glob->computeSpace(), glob);

  // fixup having expression
  if (havingExpr())
    (void) havingExpr()->fixup(0, getExpressionMode(), this, space, heap,
			       glob->computeSpace(), glob);

}

///////////////////////////////////////////////////////////////////////////////
// Destructor for sort_grby_tcb
//
ex_sort_grby_tcb::~ex_sort_grby_tcb()
{
  delete qparent_.up;
  delete qparent_.down;
  freeResources();
}
  
///////////////////////////////////////////////////////////////////////////////
// Free Resources
//
void ex_sort_grby_tcb::freeResources()
{
  delete pool_;
  pool_ = 0;
}

////////////////////////////////////////////////////////////////////////////
// This is where the action is.
////////////////////////////////////////////////////////////////////////////
short ex_sort_grby_tcb::work()
{
   // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down;
  ex_sort_grby_private_state * pstate;
  ex_queue::down_request request;

  queue_index    tail = qparent_.down->getTailIndex();

  for( ;
       (processedInputs_ != tail) && (!qchild_.down->isFull());
       processedInputs_++ )
    {
      pentry_down = qparent_.down->getQueueEntry(processedInputs_);
      pstate = (ex_sort_grby_private_state*) pentry_down->pstate;
      request = pentry_down->downState.request;

      ex_assert(pstate->step_ == ex_sort_grby_tcb::SORT_GRBY_EMPTY,
                "Invalid initial state in ex_sort_grby_tcb::work()");

      // if request has been cancelled don't bother child
      if (request == ex_queue::GET_NOMORE)
        {
          pstate->step_ = SORT_GRBY_NEVER_STARTED;
        }
      else
        {
          ex_queue_entry * centry = qchild_.down->getTailEntry();

          if ((sort_grby_tdb().firstNRows() >= 0) &&
	      ((pentry_down->downState.request != ex_queue::GET_N) ||
	       (pentry_down->downState.requestValue == sort_grby_tdb().firstNRows())))
            {
		centry->downState.request = ex_queue::GET_N;
		centry->downState.requestValue = sort_grby_tdb().firstNRows();
	    }
          else
            {
                centry->downState.request = ex_queue::GET_ALL;
                centry->downState.requestValue = 11;
            }
          
          centry->downState.parentIndex = processedInputs_;

	  pstate->oneRowAggr_ = FALSE;

          // set the child's input atp
          centry->passAtp(pentry_down->getAtp());

          qchild_.down->insert();

          pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_NEW_GROUP;
        }
    } // end for processedInputs_

  pentry_down = qparent_.down->getHeadEntry();
  pstate = (ex_sort_grby_private_state*) pentry_down->pstate;
  request = pentry_down->downState.request;

  while (1) // exit via return
    {
      // if we have already given to the parent all the rows needed cancel the
      // parent's request. Also cancel it if the parent cancelled
      if ((pstate->step_ != ex_sort_grby_tcb::SORT_GRBY_CANCELLED) &&
	  (pstate->step_ != ex_sort_grby_tcb::SORT_GRBY_DONE) &&
	  (pstate->step_ != ex_sort_grby_tcb::SORT_GRBY_NEVER_STARTED) &&
	  ((request == ex_queue::GET_NOMORE) ||
	   ((request == ex_queue::GET_N) &&
	    (pentry_down->downState.requestValue <= (Lng32)pstate->matchCount_))))
	{
	  qchild_.down->cancelRequestWithParentIndex(qparent_.down->getHeadIndex());
	  pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_CANCELLED;
	}

      switch (pstate->step_)
	{
	case ex_sort_grby_tcb::SORT_GRBY_CANCELLED:
	  {
	    // request was cancelled. Child was sent a cancel
	    // request. Ignore all up rows from child.
	    // Wait for EOF.
	    if (qchild_.up->isEmpty())
	      {
		// nothing returned from child. Get outta here.
		return WORK_OK;
	      }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();

            ex_assert(centry->upState.parentIndex == qparent_.down->getHeadIndex(),
                      "ex_sort_grby_tcb::work() child's reply out of sync");

	    ex_queue::up_status child_status = centry->upState.status;
	    switch(child_status)
	      {
	      case ex_queue::Q_OK_MMORE:
	      case ex_queue::Q_SQLERROR:
    		{
	    	  // just consume the child row
                  qchild_.up->removeHead();	    
		}
		break;

	      case ex_queue::Q_NO_DATA:
		{
		  // return EOF to parent.
		  pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_DONE;
		}
    		break;

	      case ex_queue::Q_INVALID:
	    	ex_assert(0,"ex_sort_grby_tcb::work() Invalid state returned by child");
		break;

              }; // end of switch on status of child queue
	  } // request was cancelled
	  break;

	case ex_sort_grby_tcb::SORT_GRBY_NEW_GROUP:
	  {
	    // Start of a new group.
	    //
	    // If grouping is being done and a row is returned from child,
	    // create space for the grouped/aggregated row and move the
	    // grouping column value to it.
	    //
	    // If grouping is not being done, create space for the
	    // aggregated row and initialize it.
	    //

	    if (qchild_.up->isEmpty())
	      {
		// nothing returned from child. Get outta here.
		return WORK_OK;
	      }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();

            ex_assert(centry->upState.parentIndex ==
		      qparent_.down->getHeadIndex(),
                      "ex_sort_grby_tcb::work() child's reply out of sync");

	    if (centry->upState.status == ex_queue::Q_SQLERROR)
	      {
		pstate->step_ = SORT_GRBY_CHILD_ERROR;
		break;
	      }

	    if ((!grbyExpr()) ||
		((grbyExpr()) && 
		 (centry->upState.status == ex_queue::Q_OK_MMORE)))
	      {
		// copy the down atp to the work atp for the current request
		workAtp_->copyAtp(pentry_down->getAtp());
		// get space to hold the grouped/aggregated row.
		if (pool_->getFreeTuple(workAtp_->getTupp(
		     ((ex_sort_grby_tdb &)tdb).tuppIndex_)))
		  return WORK_POOL_BLOCKED; // couldn't allocate, try again.

		// move the group by value to this new row, if grouping
		// is being done.
		if (grbyExpr())
		  {
		    if (moveExpr()->eval(centry->getAtp(), workAtp_) ==
			ex_expr::EXPR_ERROR)
		      {
                        pstate->step_ =
			  SORT_GRBY_LOCAL_ERROR;
                        break;
		      }
		  }

		// initialize the aggregate
		if (aggrExpr())	 
		  {
		    if (((AggrExpr *)aggrExpr())->initializeAggr(workAtp_) ==
			ex_expr::EXPR_ERROR)
		      {
                        pstate->step_ =
			  SORT_GRBY_LOCAL_ERROR;
                        break;
		      }
		    // a flag in the expression is set at compilation time
		    // if this is a single row aggregate
		    pstate->oneRowAggr_ =
		      ((AggrExpr *)(aggrExpr()))->isOneRowAggr();
		  }

		// if group by not being done(scalar aggrs) and no rows were 
		// found, move in null value for those aggrs that return
		// null on an empty set (like, min, sum...).
		if ((!grbyExpr()) && 
		    (centry->upState.status == ex_queue::Q_NO_DATA))
		  {
		    if (((AggrExpr *)aggrExpr())->finalizeNullAggr(workAtp_)
			== ex_expr::EXPR_ERROR)
		      {
                        pstate->step_ =
			  SORT_GRBY_LOCAL_ERROR;
                        break;
		      }
		  }

		pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_STARTED;
	      }
	    else
	      {
		// no rows returned for a group by. Nothing to be returned(except,
		// of course, Q_NO_DATA).
		pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_DONE;
	      }

	  } // start of a new group
	break;

	case ex_sort_grby_tcb::SORT_GRBY_STARTED:
	  {
	    if (qchild_.up->isEmpty())
	      {
		// nothing returned from child. Get outta here.
		return WORK_OK;
	      }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();

            ex_assert(centry->upState.parentIndex == qparent_.down->getHeadIndex(),
                      "ex_sort_grby_tcb::work() child's reply out of sync");

	    ex_queue::up_status child_status = centry->upState.status;
	    switch(child_status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{
		  // row returned from child. Aggregate it.
		  // If the new child row belongs
		  // to this group, then aggregate it.

		  ex_expr::exp_return_type grbyExprRetCode;
		  if(grbyExpr())
		    {
		      grbyExprRetCode = grbyExpr()->eval(centry->getAtp(),
							 workAtp_);
		    }

		  if (grbyExpr() &&
		      ((grbyExprRetCode == ex_expr::EXPR_FALSE) ||
		       (grbyExprRetCode == ex_expr::EXPR_ERROR)))
		    {
		      if (grbyExprRetCode == ex_expr::EXPR_FALSE)
			pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_FINALIZE;
		      else
			{
			  pstate->step_ =
			    SORT_GRBY_LOCAL_ERROR;
			}
		    }
		  else
		    {
		      // aggregate the row.
		      if (aggrExpr())
			{
                          ex_expr::exp_return_type retcode;
                          retcode = aggrExpr()->eval(centry->getAtp(),
						     workAtp_);
                          if ( retcode == ex_expr::EXPR_OK &&
			       pstate->oneRowAggr_ &&
                               moveExpr() &&
                               !grbyExpr() )
                          {
                            retcode = moveExpr()->eval( centry->getAtp(),
                                                        workAtp_ );
                          }
                            // Only want to short circuit if we have a ScalarAgg.
			  if ((retcode == ex_expr::EXPR_TRUE) && (!grbyExpr()))
			    {
			      // don't do any further processing. Short
			      // circuit and return. This is the case
			      // of ANY_TRUE, ONE_TRUE or short circuit 
			      // aggregate evaluation (like, count(*) > 5)
			      // Send GET_NOMORE request to child.
			      qchild_.down->cancelRequestWithParentIndex(qparent_.down->getHeadIndex());
			      pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_FINALIZE_CANCEL;
			      break;
			    }
			  else
			    if (retcode == ex_expr::EXPR_ERROR)
			      {
				pstate->step_ = SORT_GRBY_LOCAL_ERROR;
                                break;
			      }
			}

		      // consume the child row only if the next 
		      // step - pstate->step_ is not the following
		      //  - SORT_GRBY_LOCAL_ERROR;
                      //  - SORT_GRBY_CHILD_ERROR;
		      //  - SORT_GRBY_FINALIZE_CANCEL;
		      //  - SORT_GRBY_FINALIZE;
		      // The qchild_.up->removeHead() is done as part of 
                      // that step.

		      // Here I check for only SORT_GRBY_FINALIZE_CANCEL 
                      // because 
		      // the logic is such that when we get here, the 
                      // state cannot be SORT_GRBY_LOCAL_ERROR,
		      // SORT_GRBY_CHILD_ERROR or SORT_GRBY_FINALIZE.  
		      if (pstate->step_ != ex_sort_grby_tcb::SORT_GRBY_FINALIZE_CANCEL)
			qchild_.up->removeHead();	    
		    }
		}
		break;
		
	      case ex_queue::Q_NO_DATA:
		{
		  // return current group to parent
		  pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_FINALIZE;
		}
		break;
		
	      case ex_queue::Q_SQLERROR:
		{
		  pstate->step_ = SORT_GRBY_CHILD_ERROR;
		}
                break;
		
	      case ex_queue::Q_INVALID:
		ex_assert(0,"ex_sort_grby_tcb::work() Invalid state returned by child");
		break;
		
	      }; // end of switch on status of child queue
	    
	  }
	  break;
	  
	  
        case SORT_GRBY_CHILD_ERROR:
        case SORT_GRBY_LOCAL_ERROR:
	  {
            qchild_.down->cancelRequestWithParentIndex(
                                  qparent_.down->getHeadIndex());

            // check if we've got room in the up queue
            if (qparent_.up->isFull())
              return WORK_OK; // parent queue is full. Just return

            ex_queue_entry *pentry_up = qparent_.up->getTailEntry();
            ex_queue_entry * centry = qchild_.up->getHeadEntry();

            pentry_up->copyAtp(centry);
            pentry_up->upState.parentIndex = 
              pentry_down->downState.parentIndex;
            pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
            pentry_up->upState.setMatchNo(pstate->matchCount_);

            if ((sort_grby_tdb().isNonFatalErrorTolerated()) && 
                (SORT_GRBY_LOCAL_ERROR == pstate->step_))
              pentry_up->upState.status = ex_queue::Q_OK_MMORE;
            else
              pentry_up->upState.status = ex_queue::Q_SQLERROR;

            qparent_.up->insert();
            pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_CANCELLED;
            break;
	  }
	  
	case ex_sort_grby_tcb::SORT_GRBY_FINALIZE:
	case ex_sort_grby_tcb::SORT_GRBY_FINALIZE_CANCEL:
	  {
	    // check if we've got room in the up queue
	    if (qparent_.up->isFull())
	      return WORK_OK; // parent queue is full. Just return

	    ex_expr::exp_return_type evalRetCode = ex_expr::EXPR_OK;
	    if (havingExpr())
	      {
		evalRetCode = havingExpr()->eval(workAtp_, 0);
	      }
    
	    if ((!havingExpr()) ||
		((havingExpr()) && (evalRetCode == ex_expr::EXPR_TRUE)))
	      {
		// if stats are to be collected, collect them.
		if (getStatsEntry())
		  {
		    getStatsEntry()->incActualRowsReturned();
		  }    

		// return to parent
		ex_queue_entry * pentry_up = qparent_.up->getTailEntry();
		
		pentry_up->copyAtp(workAtp_);
		
		pentry_up->upState.status = ex_queue::Q_OK_MMORE;
		pentry_up->upState.parentIndex = pentry_down->downState.parentIndex;
	        pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
		
		pstate->matchCount_++;
		pentry_up->upState.setMatchNo(pstate->matchCount_);
		
		// insert into parent up queue
		qparent_.up->insert();
	      }
	    else if (evalRetCode == ex_expr::EXPR_ERROR)
	      {
                // The SORT_GRBY_LOCAL_ERROR state expects the
                // diags area to be in the ATP of the head entry of
                // the child's queue.  It is currently in the workAtp_,
                // so copy it to the childs head ATP.  
                // SORT_GRBY_LOCAL_ERROR will copy it from the
                // child's queue entry to the parents up queue.
                //
                ex_queue_entry * centry = qchild_.up->getHeadEntry();
                centry->copyAtp(workAtp_);
                pstate->step_ = SORT_GRBY_LOCAL_ERROR;
                break;
	      }
	    
	    // start a new group, if more rows in child's up queue
	    if (pstate->step_ == SORT_GRBY_FINALIZE_CANCEL)
	      pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_CANCELLED;
	    else
	      if (qchild_.up->getHeadEntry()->upState.status !=
		  ex_queue::Q_OK_MMORE)
		pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_DONE;
	      else
		pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_NEW_GROUP;
	    
	    workAtp_->getTupp(sort_grby_tdb().tuppIndex_).release();
	    
	    break;
	  }	
	  
	case ex_sort_grby_tcb::SORT_GRBY_DONE:
	case ex_sort_grby_tcb::SORT_GRBY_NEVER_STARTED:
	  {
	    // check if we've got room in the up queue
	    if (qparent_.up->isFull())
	      return WORK_OK; // parent queue is full. Just return
	    
	    workAtp_->release();
	    
	    ex_queue_entry * pentry_up = qparent_.up->getTailEntry();
	    
	    pentry_up->upState.status = ex_queue::Q_NO_DATA;
	    pentry_up->upState.parentIndex = pentry_down->downState.parentIndex;
	    pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
	    pentry_up->upState.setMatchNo(pstate->matchCount_);

            if (pstate->step_ != SORT_GRBY_NEVER_STARTED)
              {
	        ex_queue_entry * centry = qchild_.up->getHeadEntry();
                ex_assert(centry->upState.status == ex_queue::Q_NO_DATA,
                              "ex_sort_grby_tcb::work() expecting Q_NO_DATA");
                ex_assert(centry->upState.parentIndex == qparent_.down->getHeadIndex(),
                      "ex_sort_grby_tcb::work() child's reply out of sync");
	        qchild_.up->removeHead();
              }

            // remove the down entry
	    qparent_.down->removeHead();

	    // insert into parent up queue
	    qparent_.up->insert();

            // re-initialize pstate
	    pstate->step_ = ex_sort_grby_tcb::SORT_GRBY_EMPTY;
            pstate->matchCount_ = 0;
            workAtp_->release();

            if (qparent_.down->isEmpty())
              return WORK_OK;

            // If we haven't given to our child the new head
            // index return and ask to be called again.
            if (qparent_.down->getHeadIndex() == processedInputs_)
	      return WORK_CALL_AGAIN;

            // postion at the new head
            pentry_down = qparent_.down->getHeadEntry();
            pstate = (ex_sort_grby_private_state*) pentry_down->pstate;
            request = pentry_down->downState.request;

            ex_assert(pstate->step_ != ex_sort_grby_tcb::SORT_GRBY_EMPTY,
		"ex_sort_grby_tcb::work() invalid step");
	  }
	  break;

	default:
          ex_assert(0, "Invalid pstate->step_ in ex_sort_grby_tcb::work()");
	  break;
	  
	} // switch pstate->step_
      
    } // while
}

ex_tcb_private_state * ex_sort_grby_tcb::allocatePstates(
     Lng32 &numElems,
     Lng32 &pstateLength)
{
  PstateAllocator<ex_sort_grby_private_state> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

//////////////////////////////////////////////////////////////////////////
// Constructor and destructor for aggr_private_state
///////////////////////////////////////////////////////////////////////////////

ex_sort_grby_private_state::ex_sort_grby_private_state()
{
  matchCount_        = 0;
  step_              = ex_sort_grby_tcb::SORT_GRBY_EMPTY;
  oneRowAggr_        = FALSE;
}

ex_sort_grby_private_state::~ex_sort_grby_private_state()
{
}
