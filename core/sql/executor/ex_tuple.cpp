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
 * File:         ex_tuple.cpp
 * Description:  Tuple operator and EXPR operator (apply output expression
 *               on child rows, this is used as implementation of the
 *               map value id operator in the optimizer)
 * Created:      4/10/1998
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  "ex_stdh.h"

#include  "ComTdb.h"
#include  "ex_tcb.h"

#include  "ex_tuple.h"

#include  "ex_expr.h"

#include  "ExStats.h"
#include  "ComQueue.h"


 
///////////////////////////////////////
// class ExTupleLeafTdb
///////////////////////////////////////

ex_tcb * ExTupleLeafTdb::build(ex_globals * glob)
{
  ExTupleLeafTcb * tcb = new(glob->getSpace())
    ExTupleLeafTcb(*this, glob);

  tcb->registerSubtasks();
  tcb->registerResizeSubtasks();

  return tcb;
}

///////////////////////////////////////
// class ExTupleNonLeafTdb
///////////////////////////////////////

ex_tcb * ExTupleNonLeafTdb::build(ex_globals * glob)
{
  ex_tcb * childTcb = tdbChild_->build(glob);

  ExTupleNonLeafTcb * tcb = new(glob->getSpace())
    ExTupleNonLeafTcb(*this, *childTcb, glob);

  tcb->registerSubtasks();

  return tcb;
}

//////////////////////////////////////////
// class ExTupleTcb
//////////////////////////////////////////
ExTupleTcb::ExTupleTcb(const ComTdbTuple & tupleTdb,
		       ex_globals * glob)
     : ex_tcb( tupleTdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(tupleTdb.numBuffers_,
				     tupleTdb.bufferSize_,
				     space);

  // Allocate the queue to communicate with parent
  allocateParentQueues(qparent);

  tcbFlags_ = 0;

  // fixup tuple expressions
  if (tupleExprList())
    { 
      ex_expr * currExpr;
      tupleExprList()->position();
      while ((currExpr = (ex_expr *)(tupleExprList()->getNext())) != NULL)
	{
	  (void) currExpr->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);
	}
    }
  if (tupleTdb.predExpr_)
    (void) tupleTdb.predExpr_
      ->fixup(0, getExpressionMode(), this, space, heap, FALSE, glob);

  workAtp_ = allocateAtp(tupleTdb.criDescUp_, space);
};


ExTupleTcb::~ExTupleTcb()
{
  delete qparent.up;
  delete qparent.down;

  freeResources();
}

short ExTupleTcb::work()
{
  // should not reach here
  ex_assert(0, "ExTupleTcb::work() must be redefined");
  return WORK_OK;
}

void ExTupleTcb::freeResources()
{
  delete pool_;
  pool_ = NULL;
}


ex_tcb_private_state * ExTupleTcb::allocatePstates(
     Lng32 &numElems,
     Lng32 &pstateLength)
{
  PstateAllocator<ExTuplePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

///////////////////////////////////////////////
// class ExTupleLeafTcb
///////////////////////////////////////////////
ExTupleLeafTcb::ExTupleLeafTcb(const ExTupleLeafTdb & tupleTdb,
			       ex_globals * glob)
     : ExTupleTcb(tupleTdb, glob)
{
}

ExWorkProcRetcode ExTupleLeafTcb::work()
{
  // if no parent request, return
  if (qparent.down->isEmpty())
    return WORK_OK;

#ifdef _DEBUG
  //
  // This block of code is for UDR testing only. It lets us create a 
  // situation where a non-UDR node in the dataflow tree returns
  // WORK_BAD_ERROR while a UDR transactional message is outstanding.
  //
  if (getenv("UDR_INJECT_TUPLE_ERROR"))
  {
    return WORK_BAD_ERROR;
  }
#endif // _DEBUG

  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ExTuplePrivateState *  pstate 
    = (ExTuplePrivateState*)pentry_down->pstate;
  ex_queue::down_request request = pentry_down->downState.request;

  ExOperStats *statsEntry = getStatsEntry();

  while (TRUE) // exit via return
    {
      if ((pstate->step_ != ExTupleTcb::CANCEL_REQUEST) &&
	  (pstate->step_ != ExTupleTcb::ALL_DONE) &&
	  ((request == ex_queue::GET_NOMORE) ||
	   ((request == ex_queue::GET_N) && 
	    (pentry_down->downState.requestValue <= (Lng32)pstate->matchCount_))))  
	{
	  pstate->step_ = ExTupleTcb::CANCEL_REQUEST;
	}
      
      switch (pstate->step_)
	{
	case ExTupleTcb::TUPLE_EMPTY:
	  { 
	    
	    tupleExprList()->position();
	    
	    pstate->step_ = ExTupleTcb::RETURN_TUPLE;
	  }
	break;
	
	case ExTupleTcb::GET_NEXT_ROW:
	  {
	  }
	break;
	
	case ExTupleTcb::RETURN_TUPLE:
	  {
            if (qparent.up->isFull())
              return WORK_OK;

            ex_queue_entry * up_entry = qparent.up->getTailEntry();
            ex_expr * tupleExpr = (ex_expr *)(tupleExprList()->getCurr());

            // If there is a Tuple Expression, evaluate it first
            // The conditional predicate may need to use its results.
            up_entry->copyAtp(pentry_down);

            if (tupleExpr != NULL)
            {
              // allocate space to hold the tuple to be returned
              tupp p;
              if (pool_->get_free_tuple(p, tupleTdb().tupleLen_))
                return WORK_POOL_BLOCKED; // couldn't allocate, try again later.
              up_entry->getTupp(tupleTdb().tuppIndex_) = p;

              if (tupleExpr->eval(up_entry->getAtp(), 0) == ex_expr::EXPR_ERROR)
              {
                // handle errors
                pstate->step_ = ExTupleTcb::HANDLE_SQLERROR;
                break;
                }
            } // tupleExpr present

            // If there is a conditional predicate - evaluate it next.
            // No predicate is the same as predicate is TRUE.

            NABoolean isPredicateSatisfied = TRUE;
            if (tupleTdb().predExpr_)
            {
              ex_expr::exp_return_type retCode =
                    (tupleTdb().predExpr_->eval(up_entry->getAtp(), 0));
              if (retCode == ex_expr::EXPR_ERROR)
              {
                // handle errors
                pstate->step_ = ExTupleTcb::HANDLE_SQLERROR;
                break;
              }
              if (retCode == ex_expr::EXPR_FALSE)
                isPredicateSatisfied = FALSE;
            }

            // If the predicate is FALSE - return nothing.
            if (isPredicateSatisfied)
            {
              // The results of evaluating the tuple expression are in up_entry
              // If there is no tupleExpr, return an empty row.
              // insert into parent
              up_entry->upState.status = ex_queue::Q_OK_MMORE;
              up_entry->upState.parentIndex
                  = pentry_down->downState.parentIndex;
              up_entry->upState.downIndex = qparent.down->getHeadIndex();

              pstate->matchCount_++;
              up_entry->upState.setMatchNo(pstate->matchCount_);
              if (statsEntry)
                statsEntry->incActualRowsReturned();
              qparent.up->insert();
            }

            tupleExprList()->advance();
            if (tupleExprList()->atEnd())
              pstate->step_ = ExTupleTcb::ALL_DONE;
	  }
        break;

	case ExTupleTcb::ALL_DONE:
	  {
	    if (qparent.up->isFull())
	      return WORK_OK;
	    
	    ex_queue_entry * up_entry2 = qparent.up->getTailEntry();
	    
	    up_entry2->upState.status = ex_queue::Q_NO_DATA;
	    up_entry2->upState.parentIndex 
	      = pentry_down->downState.parentIndex;	      
	    up_entry2->upState.downIndex = qparent.down->getHeadIndex();
	    up_entry2->upState.setMatchNo(pstate->matchCount_);
	    
	    // tuple into parent
	    qparent.up->insert();
	    
	    qparent.down->removeHead();
	    
	    pstate->step_ = ExTupleTcb::TUPLE_EMPTY;
            pstate->matchCount_ = 0;//++MV Bug fix

	    if (qparent.down->isEmpty())
	      return WORK_OK;
	    else
	      //return WORK_CALL_AGAIN; // there are more parent requests
	      {	// get more requests from parent
		pentry_down = qparent.down->getHeadEntry();
		pstate = (ExTuplePrivateState*)pentry_down->pstate;
		request = pentry_down->downState.request;
	      }
	  }
	break;
	
	case ExTupleTcb::CANCEL_REQUEST:
	  {
	    pstate->step_ = ExTupleTcb::ALL_DONE;
	  }
	  break;

	case ExTupleTcb::HANDLE_SQLERROR:
	  {
	    ex_queue_entry * up_entry3 = qparent.up->getTailEntry();

	    up_entry3->upState.status = ex_queue::Q_SQLERROR;
	    up_entry3->upState.parentIndex = pentry_down->downState.parentIndex;
	    up_entry3->upState.downIndex = qparent.down->getHeadIndex();
            up_entry3->upState.setMatchNo(pstate->matchCount_);

	    qparent.up->insert();

	    pstate->step_ = ExTupleTcb::CANCEL_REQUEST;
	  }
	break;

	default:
	  break;

	} // switch
    } // while
  return WORK_OK;
}

///////////////////////////////////////////////
// class ExTupleNonLeafTcb
///////////////////////////////////////////////
ExTupleNonLeafTcb::ExTupleNonLeafTcb(const ExTupleNonLeafTdb & tupleTdb,
				     const ex_tcb & tcbChild,
				     ex_globals * glob)
     : ExTupleTcb(tupleTdb, glob)
{
  tcbChild_ = &tcbChild;
  qchild_  = tcbChild.getParentQueue();
  nextToSendDown_ = qparent.down->getHeadIndex();
}

ExWorkProcRetcode ExTupleNonLeafTcb::work()
{
  // if no parent request, return
  if (qparent.down->isEmpty())
    return WORK_OK;

  // look for new requests from the parent
  while (qparent.down->entryExists(nextToSendDown_))
    {
      ex_queue_entry * pentry1 = qparent.down->getQueueEntry(nextToSendDown_);
      ExTuplePrivateState & pstate1 
	= *((ExTuplePrivateState *) pentry1->pstate);
      ex_queue_entry * centry1 = qchild_.down->getTailEntry();

      ex_assert(pstate1.step_ == ExTupleTcb::TUPLE_EMPTY, 
		"Uninitialized pstate object");

      
      if (nextToSendDown_ == qparent.down->getHeadIndex())
	{
	  // sending down the first request, initialize return logic
	  tupleExprList()->position();
	}

      if (pentry1->downState.request != ex_queue::GET_NOMORE)
	{
	  // send the new request down to the child

	  centry1->downState.request = pentry1->downState.request;
	  centry1->downState.requestValue = pentry1->downState.requestValue;
	  centry1->downState.parentIndex = nextToSendDown_;
	  centry1->passAtp(pentry1);
      
	  pstate1.step_ = ExTupleTcb::GET_NEXT_ROW;
	  pstate1.matchCount_ = 0;
	  qchild_.down->insert();
	}
      nextToSendDown_++;
      
    }

  // now start with the first request from the parent and check
  // for cancellations and rows coming up from the child
  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ExTuplePrivateState  * pstate =
    (ExTuplePrivateState*) pentry_down->pstate;
  ex_queue::down_request request = pentry_down->downState.request;

  ExOperStats *statsEntry = getStatsEntry();

  while (TRUE) // exit via return
    {
      if ((pstate->step_ != ExTupleTcb::CANCEL_REQUEST) &&
	  (pstate->step_ != ExTupleTcb::ALL_DONE) &&
	  ((request == ex_queue::GET_NOMORE) ||
	   ((request == ex_queue::GET_N) && 
	    (pentry_down->downState.requestValue <=
	     (Lng32) pstate->matchCount_))))  
	{
          if (pstate->step_ == ExTupleTcb::TUPLE_EMPTY)
            pstate->step_ = ExTupleTcb::ALL_DONE;
          else if ((pstate->step_ != ExTupleTcb::CANCEL_REQUEST) &&
                   (pstate->step_ != ExTupleTcb::ALL_DONE))
            {
              qchild_.down->cancelRequestWithParentIndex (qparent.down->getHeadIndex());
              pstate->step_ = ExTupleTcb::CANCEL_REQUEST;
            }
	}
      
      switch (pstate->step_)
	{
	case ExTupleTcb::GET_NEXT_ROW:
	  {
	    if (qchild_.up->isEmpty())
	      return WORK_OK;
	    
	    ex_queue_entry * centry2 = qchild_.up->getHeadEntry();
	    
	    switch (centry2->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
		// row was returned from child. 
		// Compute and insert into parent queue.
		// Copy the child atp into the returned Atp
		pstate->step_ = ExTupleTcb::RETURN_TUPLE;
		break;
		
	      case ex_queue::Q_NO_DATA:
                {
                  pstate->step_ = ExTupleTcb::ALL_DONE;
                  qchild_.up->removeHead();
                }
		break;
		
	      case ex_queue::Q_SQLERROR:
		pstate->step_ = ExTupleTcb::HANDLE_SQLERROR;
		break;
		
	      case ex_queue::Q_INVALID:
	      default:
		ex_assert(0,"ExTupleTcb::work() Invalid state returned by child");
		break;  
		
	      }
	  }
	break;
	
	case ExTupleTcb::RETURN_TUPLE:
	  {
	    if (qparent.up->isFull())
	      return WORK_OK;
    
	    ex_queue_entry * up_entry4 = qparent.up->getTailEntry(); 
	    ex_queue_entry * centry3 = qchild_.up->getHeadEntry();

	    up_entry4->copyAtp(centry3);
	    
	    ex_expr * tupleExpr = (ex_expr *)(tupleExprList()->getCurr());
	    
	    if (tupleExpr != NULL)
	      {
		// allocate space to hold the tuple to be returned
		tupp p;
		if (pool_->get_free_tuple(p, (Lng32) tupleTdb().tupleLen_))
                {
                  up_entry4->getAtp()->release();
		  return WORK_POOL_BLOCKED; // couldn't allocate, try again later.
                }

		up_entry4->getTupp(tupleTdb().tuppIndex_) = p;

		if (tupleExpr->eval(up_entry4->getAtp(), 0) == ex_expr::EXPR_ERROR)
		  {
		    // handle errors
		    pstate->step_ = ExTupleTcb::HANDLE_SQLERROR;
		    break;
		  }	  
	      } // tupleExpr present
	    
	    // insert into parent
	    up_entry4->upState.status = ex_queue::Q_OK_MMORE;
	    up_entry4->upState.parentIndex 
	      = pentry_down->downState.parentIndex;	      
	    up_entry4->upState.downIndex = qparent.down->getHeadIndex();
	    
	    pstate->matchCount_++;
	    up_entry4->upState.setMatchNo(pstate->matchCount_); 
	    
            if (statsEntry)
              statsEntry->incActualRowsReturned();
	    qparent.up->insert();
	    qchild_.up->removeHead();

	    pstate->step_ = ExTupleTcb::GET_NEXT_ROW;
	  }
	break;

	case ExTupleTcb::ALL_DONE:
	  {
	    if (qparent.up->isFull())
	      return WORK_OK;

	    ex_queue_entry * up_entry5 = qparent.up->getTailEntry();

	    up_entry5->upState.status = ex_queue::Q_NO_DATA;
	    up_entry5->upState.parentIndex 
	      = pentry_down->downState.parentIndex;	      
	    up_entry5->upState.downIndex = qparent.down->getHeadIndex();
	    up_entry5->upState.setMatchNo(pstate->matchCount_);

	    // tuple into parent
	    qparent.up->insert();

	    qparent.down->removeHead();

	    pstate->step_ = ExTupleTcb::TUPLE_EMPTY;
            pstate->matchCount_ = 0;

	    // get ready for the next parent request
	    tupleExprList()->position();

	    if (qparent.down->isEmpty())
	      return WORK_OK;
	    else
	      {
		// re-initialize request values
		pentry_down = qparent.down->getHeadEntry();
		pstate      = (ExTuplePrivateState *) pentry_down->pstate;
		request     = pentry_down->downState.request;
	      }
	  }
	  break;

	case ExTupleTcb::CANCEL_REQUEST:
	  {
	    // request was cancelled. Child was sent a cancel
	    // request. Ignore all up rows from child.
	    // Wait for EOF.
	    if (qchild_.up->isEmpty())
	      return WORK_OK;
	    
	    switch(qchild_.up->getHeadEntry()->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
	      case ex_queue::Q_SQLERROR:
		{
		  // do nothing. We want to ignore this row.
		  qchild_.up->removeHead();
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
                 pstate->step_ = ExTupleTcb::ALL_DONE;
                 qchild_.up->removeHead();
		}
	      break;
	      
	      case ex_queue::Q_INVALID:
	      default:
		ex_assert(0,
			  "ExTupleTcb::work() Invalid state returned by child");
		break;
		
	      } // switch
	  }
	break;
	
	case ExTupleTcb::HANDLE_SQLERROR:
	  {
	    ex_queue_entry * up_entry6 = qparent.up->getTailEntry();

            ex_queue_entry * centry4 = qchild_.up->getHeadEntry();

	    up_entry6->upState.status = ex_queue::Q_SQLERROR;
	    up_entry6->upState.parentIndex = pentry_down->downState.parentIndex;
	    up_entry6->upState.downIndex = qparent.down->getHeadIndex();
            up_entry6->upState.setMatchNo(pstate->matchCount_);

	    // if up_entry6 contains a diags area, save it.
	    // Otherwises copyAtp will overwrite it. Merge it after any
	    // diags area returned by child.
	    ComDiagsArea * tempDA = up_entry6->getDiagsArea();
	    if (tempDA)
	      tempDA->incrRefCount(); //increment ref count so that tempDA
	                              //does not get deleted
            up_entry6->copyAtp(centry4);
	    if (tempDA)
	      {
                if (up_entry6->getDiagsArea()) //check if child has a Diags Area
                  {
		    up_entry6->getDiagsArea()->mergeAfter(*tempDA);
                    tempDA->decrRefCount();//decrement ref count so that tempDa
                                           //can be deleted
                  }
                else
                  {
                    up_entry6->setDiagsArea(tempDA);//child did not have diags area
                                                   //use the Diags Area generated 
                                                   //by the error in the expression
                  }
	      }

	    qparent.up->insert();

	    pstate->step_ = ExTupleTcb::CANCEL_REQUEST;
	  }
	break;

	default:
	  break;

	} // switch
    } // while
  return WORK_OK;
}

/////////////////////////////////////////////
// class ExTuplePrivateState
/////////////////////////////////////////////
ExTuplePrivateState::ExTuplePrivateState()
{
  matchCount_ = 0;

  step_ = ExTupleTcb::TUPLE_EMPTY;
}

ExTuplePrivateState::~ExTuplePrivateState()
{
}
