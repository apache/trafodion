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
* File:         ex_union.C
* Description:  Methods for the tdb and tcb of a UNION ALL operation
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
// with a union operator
//

#include "ex_union.h"
#include "ExStats.h"
#include "ex_error.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////
//
// Build a union tcb
//
ex_tcb * ex_union_tdb::build(ex_globals * glob)
{
  // first build the children
  ex_tcb *  left_tcb;
  ex_tcb *  right_tcb;
  ex_union_tcb *union_tcb;

  ex_assert( !(isMergeUnion() && isOrderedUnion()), 
             "MergeUnion, OrderedUnion can't be simultaneously true" );

  //++ Triggers -
  ex_assert( !(isMergeUnion() && isBlockedUnion()), 
             "MergeUnion, BlockedUnion can't be simultaneously true" );

  ex_assert(numChildren() == 2, 
            "ex_union_tcb::build - invalid number of children");

  left_tcb = tdbLeft_->build(glob);
  right_tcb = (tdbRight_ != (ComTdbPtr)NULL) ? tdbRight_->build(glob): (ex_tcb *)NULL ;

  if (isMergeUnion())
    union_tcb = new(glob->getSpace()) 
      ex_m_union_tcb(*this, left_tcb, right_tcb, glob);
  else if (isOrderedUnion() || isBlockedUnion()) //++ Triggers - , add blocked union
    union_tcb = new(glob->getSpace()) 
      ex_o_union_tcb(*this, left_tcb, right_tcb, glob, isBlockedUnion(), hasNoOutputs());
  else if (isConditionalUnion())
    union_tcb = new(glob->getSpace()) 
      ex_c_union_tcb(*this, left_tcb, right_tcb, glob);
  else
    union_tcb = new(glob->getSpace()) 
      ex_union_tcb(*this, left_tcb, right_tcb, glob);

  // add the union_tcb to the schedule
  union_tcb->registerSubtasks();

  return (union_tcb);
}

//////////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
//////////////////////////////////////////////////////////////////////////////////

//
// Constructor for union_tcb
//
ex_union_tcb::ex_union_tcb(const ex_union_tdb &  union_tdb,  // 
			   const ex_tcb *    left_tcb,    // left queue pair
			   const ex_tcb *    right_tcb,   // right queue pair
			   ex_globals *glob
			   ) : 
    ex_tcb( union_tdb, 1, glob)
{
  tcbLeft_ = left_tcb;
  tcbRight_ = right_tcb;

  CollHeap * space = glob->getSpace();
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(union_tdb.numBuffers_,
				     union_tdb.bufferSize_,
				     space);
 
  // get the queues that left and right use to communicate with me
  childQueues_[0]  = left_tcb->getParentQueue();
  childQueues_[1] = right_tcb->getParentQueue();

  // the union TCB adds input tupps to its down queues, therefore we
  // need to allocate new ATPs for its children
  for (Int32 i = 0; i < numChildren(); i++ )
    childQueues_[i].down->allocateAtps(glob->getSpace());
  
  // The cri of the rows we get from our parent
  // is the same as we give to our left and right children
  ex_cri_desc * from_parent_cri = childQueues_[0].down->getCriDesc();  

  // The cri we return to our parent is the one returned by the left or
  // right child. They should both be the same.
  ex_cri_desc * to_parent_cri;
  to_parent_cri = childQueues_[0].up->getCriDesc();
  
  
  allocateParentQueues(qparent,TRUE);

  processedInputs_ = qparent.down->getHeadIndex();
  // fixup left expression
  if (moveExpr(0))
    (void) moveExpr(0)->fixup(0, getExpressionMode(), this,
			      glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  // fixup right expression
  if (moveExpr(1))
    (void) moveExpr(1)->fixup(0, getExpressionMode(), this, 
			      glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  // fixup merge expression
  if (mergeExpr())
    (void) mergeExpr()->fixup(0, getExpressionMode(), this, 
			      glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  // fixup conditional expression
  if (condExpr())
    (void) condExpr()->fixup(0, getExpressionMode(), this,
			     glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);

  // fixup triggered action exception expression
  if (trigExceptExpr())
    (void) trigExceptExpr()->fixup(0, getExpressionMode(), this,
			     glob->getSpace(), glob->getDefaultHeap(), FALSE, glob);
};

///////////////////////////////////////////////////////////////////////////////
// Destructor for union_tcb
//
ex_union_tcb::~ex_union_tcb()
{

  delete qparent.up;
  delete qparent.down;

  freeResources();
};


///////////////////////////////////////////////////////////////////////////////
// Free Resources
//
void ex_union_tcb::freeResources()
{
  delete pool_;
  pool_ = 0;
};

void ex_union_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // down queues are handled by workDown()
  sched->registerInsertSubtask(sWorkDown, this, qparent.down,"DN");
  sched->registerUnblockSubtask(sWorkDown, this, childQueues_[0].down);
  sched->registerUnblockSubtask(sWorkDown, this, childQueues_[1].down);
  // cancellations are handled by the complaints department
  sched->registerCancelSubtask(sCancel, this, qparent.down,"CN");

  // up queues are handled by workUp()
  sched->registerUnblockSubtask(sWorkUp, this, qparent.up,"UP");
  sched->registerInsertSubtask(sWorkUp, this, childQueues_[0].up);
  sched->registerInsertSubtask(sWorkUp, this, childQueues_[1].up);
  // the parent queues will be resizable, so register a resize subtask.
  registerResizeSubtasks();
}

///////////////////////////////////////////////////////////
// Starts up the next requested union operation by sending
// requests to both children. Child queue must have room!
///////////////////////////////////////////////////////////
void ex_union_tcb::start()
{
  // Advance processedInputs_ by one, all checks need to be done
  // by the caller. No down request is sent for a cancelled message.
  ex_queue_entry * lentry = childQueues_[0].down->getTailEntry();
  ex_queue_entry * rentry = childQueues_[1].down->getTailEntry();

  ex_queue_entry * pentry_down = qparent.down->getQueueEntry(processedInputs_);
  const ex_queue::down_request request = pentry_down->downState.request;
  ex_union_private_state &  pstate =
    *((ex_union_private_state*) pentry_down->pstate);  

  // pass same request down to both children
  lentry->downState.request = request;
  lentry->downState.requestValue = pentry_down->downState.requestValue;
  lentry->downState.parentIndex = processedInputs_;
  
  rentry->downState.request = request;
  rentry->downState.requestValue = pentry_down->downState.requestValue;
  rentry->downState.parentIndex = processedInputs_;
  
  // pass it to both children
  lentry->copyAtp(pentry_down);
  rentry->copyAtp(pentry_down);
  
  pstate.matchCount_ = 0; // no matches yet for this parent row
  
  childQueues_[0].down->insert();
  childQueues_[1].down->insert();
  
  pstate.childStates_[0] = STARTED_;
  pstate.childStates_[1] = STARTED_;
  
  if (request == ex_queue::GET_NOMORE)
    {
      // immediately cancel the request (requests are already in
      // cancelled state but the cancel callback isn't activated yet)
      childQueues_[0].down->cancelRequestWithParentIndex(processedInputs_);
      childQueues_[1].down->cancelRequestWithParentIndex(processedInputs_);
      pstate.childStates_[0] = CANCELLED_;
      pstate.childStates_[1] = CANCELLED_;

      // $$$$ should find a smarter way to do this
    }

  processedInputs_++;
}

///////////////////////////////////////////////
// All child rows have been returned.
// Now return an eof indication to parent.
///////////////////////////////////////////////
void ex_union_tcb::stop()
{
  // Remove the head entries of the parent down queue and the two
  // child up queues. All checks are done by the caller.

  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();  
  ex_union_private_state &  pstate =
    *((ex_union_private_state*) pentry_down->pstate);  
  ex_queue_entry * pentry = qparent.up->getTailEntry();

   ex_queue_entry * centry0 = childQueues_[0].up->getHeadEntry();
   ex_queue_entry * centry1 = childQueues_[1].up->getHeadEntry();
  
  pentry->upState.status = ex_queue::Q_NO_DATA;
  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
  pentry->upState.downIndex = qparent.down->getHeadIndex();
  pentry->upState.setMatchNo(centry0->upState.getMatchNo() +
			     centry1->upState.getMatchNo());

  ComDiagsArea *da = pentry->getDiagsArea();
  
 
  ComDiagsArea *c0da = centry0->getDiagsArea();
  ComDiagsArea *c1da = centry1->getDiagsArea();
   if (c0da || c1da)   
    if (da == NULL) 
     {
	da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
	pentry->setDiagsArea(da);
     }
    
  if (c0da)
    da->mergeAfter(*c0da);
  if (c1da)
    da->mergeAfter(*c1da);
  
  // insert into parent
  qparent.up->insert();
  
  // consume the left and right child row
  childQueues_[0].up->removeHead();	  
  childQueues_[1].up->removeHead();	  

  pstate.init();

  // this parent request has been processed. 
  // Hasta La Vista!
  qparent.down->removeHead();
}

ExWorkProcRetcode ex_union_tcb::processCancel()
{
  // Check the down queue from the parent for cancellations. Propagate
  // cancel requests and remove all requests that are completely cancelled.

  // loop over all requests that have been sent down
  // (the others are handled by start() later)
  queue_index ix = qparent.down->getHeadIndex();

  while (ix != processedInputs_)
    {
      ex_queue_entry * pentry_down = qparent.down->getQueueEntry(ix);
      
      // check whether the current down request is cancelled
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	{
	  // yes, cancel this down request if not already done so

	  ex_union_private_state &  pstate =
	    *((ex_union_private_state*) pentry_down->pstate);  

	  // cancel child requests if not already done so and if they
	  // aren't already finished
	  for (Int32 i = 0; i < 2; i++)
	    {
	      if (pstate.childStates_[i] == ex_union_tcb::STARTED_)
		{
		  // cancel request to this child
		  childQueues_[i].down->cancelRequestWithParentIndex(ix);
		  pstate.childStates_[i] = ex_union_tcb::CANCELLED_;
		}
	    } // for each child
	} // cancelled rows request

      ix++; // may wrap around
    } // parent queue is not empty

  // checked all active entries nothing more to do
//  return WORK_CALL_AGAIN;
  return WORK_OK;
}

// -----------------------------------------------------------------------
// Generic work procedure should never be called
// -----------------------------------------------------------------------
ExWorkProcRetcode ex_union_tcb::work()
{
  ex_assert(0,"Should never reach ex_union_tcb::work()");
  return WORK_BAD_ERROR;
}

// -----------------------------------------------------------------------
// Work procedure to send requests down
// -----------------------------------------------------------------------
ExWorkProcRetcode ex_union_tcb::workDown()
{
  // while we have unprocessed down requests and while both
  // children's down queues have room, start more child requests
  while (qparent.down->entryExists(processedInputs_) &&
	 ! childQueues_[0].down->isFull() &&
	 ! childQueues_[1].down->isFull())
    start();

  return WORK_OK;
}

// -----------------------------------------------------------------------
// generic up-processing for union
// (see method whichSide() for specific part)
// -----------------------------------------------------------------------

ExWorkProcRetcode ex_union_tcb::workUp()
{

  ExOperStats *statsEntry;
  statsEntry = getStatsEntry();
  // while there is a chance that we have work (may exit via return)
  while (qparent.down->getHeadIndex() != processedInputs_)
    {
      // get head entry and pstate
      ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
      ex_union_private_state &  pstate =
	*((ex_union_private_state*) pentry_down->pstate);

      Int32 side = -1;
      Int32 endOfData = 0;

      // while we have room in the up queue and while we can make up our
      // mind from which side to take the next row, move a row up to the
      // parent
      while (! qparent.up->isFull() &&
	     (pstate.getErrorCode() || whichSide(pstate,side,endOfData)))
	{
	  // whichSide() side-effects the variable side, to show which side 
          // actually has the next row. It guarantees that there actually is a row.
	  // and that the row is not an end-of-data indicator.  Note that if getErrorCode()
          // shows any error we cannot use the side-effected result from whichSide(),
          // because it never was called.

	  ex_queue_entry * pentry = qparent.up->getTailEntry();
          if (pstate.getErrorCode())
            {
                // The ex_c_union_tcb::workDown has seen an error when deciding
                // which child.  It never sent any request down.  Finish processing
                // the error now that the offending queue request is at the head
                // of the parent down queue.  The diags area is on that queue entry's
                // atp.
              pstate.setErrorCode(0);
              pentry->copyAtp(pentry_down);
              pentry->upState.status = ex_queue::Q_SQLERROR;
              pentry->upState.parentIndex = pentry_down->downState.parentIndex;
              pentry->upState.downIndex = qparent.down->getHeadIndex();
              pentry->upState.setMatchNo(pstate.matchCount_);
              qparent.up->insert();
              continue;
            }

	  ex_queue_entry * centry = childQueues_[side].up->getHeadEntry();

          if ((pstate.childStates_[side] != CANCELLED_) &&
              (centry->upState.status != ex_queue::Q_SQLERROR))
	    {

		  // ++Triggers -
		  // The union is a control node that does not return data.
		  // This code was entered to prevent problems in embedded Sql, where update
		  // operation stops after the first returned row.
		  if (hasNoOutputs() && 
				centry->upState.status == ex_queue::Q_OK_MMORE &&
				!pentry_down->downState.request == ex_queue::GET_N)
		  {
			// consume the child rows
			do{
				
			  childQueues_[side].up->removeHead();
			  if (!childQueues_[side].up->isEmpty())
				  centry = childQueues_[side].up->getHeadEntry();
			} while (!childQueues_[side].up->isEmpty() && 
					centry->upState.status == ex_queue::Q_OK_MMORE);

		        if (centry->upState.status == ex_queue::Q_OK_MMORE)
                        {
			   return WORK_OK;
                        }

			return WORK_CALL_AGAIN;
		  }
		  // --Triggers -

	      pentry->upState.status = centry->upState.status;
	      pentry->upState.parentIndex = pentry_down->downState.parentIndex;
	      pentry->upState.downIndex = qparent.down->getHeadIndex();
	      if (pentry_down->downState.request == ex_queue::GET_N &&
		  pentry_down->downState.requestValue <= (Lng32)pstate.matchCount_)
		{
		  // cancel this request
		  qparent.down->cancelRequest(qparent.down->getHeadIndex());
		  processCancel();
                  continue;
		}
	
	      // if stats are to be collected, collect them.
	      if (statsEntry)
		{
		  statsEntry->incActualRowsReturned();
		}    
	
	      
	      // Return the row to parent.
	      // First, Move/Convert it to union buffer, if needed.
	      if (moveExpr(side))
		{
		  // copy input tupps from parent request
		  pentry->copyAtp(pentry_down);

		  // allocate space to hold the unioned row
		  if (pool_->get_free_tuple(
		       pentry->getAtp()->getTupp(union_tdb().tuppIndex_),
		       union_tdb().unionReclen_))
		    {
		      // couldn't alloc., try again later.
		      return WORK_POOL_BLOCKED;
		    }
		  
		  // copy down entry ATP into up entry
		  
		  // copy child data to parent ATP
		  if (moveExpr(side)->eval(centry->getAtp(),
		                pentry->getAtp()) == ex_expr::EXPR_ERROR)
		    {
		      // handle errors - set the status to Q_SQLERROR.
                      processError(pstate, endOfData, centry->getAtp());
                      continue;
		    }
		  // Setting the diagnostics area for the up entry. This is
		  // to flow the diagnostics information to the parent.
                  ComDiagsArea * da = centry->getAtp()->getDiagsArea();
		  pentry->getAtp()->setDiagsArea(da);
                  if (da)
                    da->incrRefCount();
		}
	      else
		{
		  // copy child's atp to
		  // the output atp (to parent's up queue)
		  pentry->copyAtp(centry);
		}
	      
	      pentry->upState.setMatchNo(++pstate.matchCount_);

              // insert into parent up queue
	      qparent.up->insert();	  
	      
	    } // child state is not cancelled
	  else if ((pstate.childStates_[side] != CANCELLED_) &&
                   (centry->upState.status == ex_queue::Q_SQLERROR))
            {
              if (trigExceptExpr()) // trigs
              {
                  // Setting the diagnostics area for the up entry. This is
                  // to flow the diagnostics information to the parent.
                  ComDiagsArea * da = centry->getAtp()->getDiagsArea();
                  ex_assert((da != NULL), "should not be here");
                  if (da->returnIndex(-ComDiags_TrigActionExceptionSQLCODE) == 
                                       NULL_COLL_INDEX)
                  {
                      trigExceptExpr()->eval(centry->getAtp(), 0);
                  }
              }
              processError(pstate, endOfData, centry->getAtp());
              continue;
            }

	  // consume the child row
	  childQueues_[side].up->removeHead();

	  if (childQueues_[side].up->isEmpty() && (numChildren()>1))  
	  {   // -- Triggers: should not get in for conditional, single child unions.
	      // moving this row up made the queue empty, we can not
	      // continue if there isn't a non EOD row in the other queue
	      if (childQueues_[1-side].up->isEmpty() ||
		  pstate.childStates_[1-side] == DONE_)
		return WORK_OK;
	    }
	} // parent up queue is not full and we know the next side

      // Either the parent queue is full or we can't read any more data
      // rows from the children. This means we can return from this work
      // procedure, UNLESS we are at end of data for the current request
      // and can return EOD and possibly try another down request
      
      if (endOfData && ! qparent.up->isFull())
	stop();
      else
	return WORK_OK;

    } // while parent ComQueue.has entries

  // parent down queue is empty
  return WORK_OK;
}


void ex_union_tcb::processError(ex_union_private_state &pstate, Int32 &endOfData,
                                atp_struct* atp)
{
  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ex_queue_entry * pentry = qparent.up->getTailEntry();

  pentry->copyAtp(atp);
  pentry->upState.status = ex_queue::Q_SQLERROR;
  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
  pentry->upState.downIndex = qparent.down->getHeadIndex();
  pentry->upState.setMatchNo(pstate.matchCount_);
  qparent.up->insert();	  

  // cancel this request and cancel all the children.
  qparent.down->cancelRequest(qparent.down->getHeadIndex());
  processCancel();

  for (Int32 i=0; i < 2; i++)
    {
      while (!(childQueues_[i].up->isEmpty()) && 
              pstate.childStates_[i] != DONE_)
        {
	  ex_queue_entry * centry = childQueues_[i].up->getHeadEntry();
          switch (centry->upState.status)
            {
              case ex_queue::Q_OK_MMORE:
              case ex_queue::Q_SQLERROR:
                childQueues_[i].up->removeHead();
                break;
              case ex_queue::Q_NO_DATA:
	        pstate.childStates_[i] = DONE_;
                break;
              case ex_queue::Q_INVALID:
                ex_assert(0, "ex_m_union_tcb::processError() Invalid state returned by child");
                break;
            }
                    
        }
    }

  if (pstate.childStates_[0] == DONE_  &&  pstate.childStates_[1] == DONE_)
    endOfData = 1;
}





// -----------------------------------------------------------------------
// Vanilla union all:
// This tcb is used to return all rows from the left child and the right
// child. To distribute processing evenly, the left and right child queue
// is tried alternatively, but if they don't have data then the other
// queue gets a chance.
// -----------------------------------------------------------------------

Int32 ex_union_tcb::whichSide(ex_union_private_state &  pstate,
			    Int32 &side,
			    Int32 &endOfData)
{
  // switch sides, try treating both children the same
  pstate.whichSide_ = 1 - pstate.whichSide_;
  Int32 numEod = 0;

  // once for each child
  for (Int32 i = 0; i < 2; i++)
    {
      if (!childQueues_[pstate.whichSide_].up->isEmpty())
	{
	  if (childQueues_[pstate.whichSide_].up->
	      getHeadEntry()->upState.status == ex_queue::Q_NO_DATA)
	    {
	      // end of data, indicate that this child is done
	      pstate.childStates_[pstate.whichSide_] = DONE_;
	      numEod++;
	    }
	  else
	    {
	      // a good row in this child queue, that's it, return success
	      side = pstate.whichSide_;
	      return 1;
	    }
	} // child's up queue is not empty

      // flip sides
      pstate.whichSide_ = 1 - pstate.whichSide_;

    } // once for each child

  // didn't find a good data row, our last chance is two end of data rows
  endOfData = (numEod == 2);

  // indicate that no data row can be moved
  return 0;
}

// -----------------------------------------------------------------------
// Merge Union
// -----------------------------------------------------------------------

ex_m_union_tcb::ex_m_union_tcb(
     const ex_union_tdb &  union_tdb,  // 
     const ex_tcb *    left_tcb,    // left queue pair
     const ex_tcb *    right_tcb,   // right queue pair
     ex_globals *glob
     ) : ex_union_tcb( union_tdb, left_tcb, right_tcb, glob)
{
}

// -----------------------------------------------------------------------
// Up-processing for merge union:
// two sorted data streams are merged. A merge expression indicates
// whether the left (TRUE) or the right (FALSE/NULL) side contains the
// next row in the merge order. A merge union can only return a row if
// it has a data or EOD row on both of its children. Note that this
// procedure does not use or maintain pstate.whichSide_.
// -----------------------------------------------------------------------

Int32 ex_m_union_tcb::whichSide(ex_union_private_state &  pstate,
			      Int32 &side,
			      Int32 &endOfData)
{
  if (childQueues_[0].up->isEmpty() || childQueues_[1].up->isEmpty())
    {
      // need to wait until both children have a queue entry, this
      // is not the EOD case
      endOfData = 0;
      return 0;
    }

  // ----------------------------------------------------------------------------------
  // If either child has returned an error, then pass the error to the parent of the
  // merge-union operator. We make this check before evaluating the merge expr in order
  // to avoid evaluating this expression on a row with no meaningful data.
  // case 10-030904-5021
  //-----------------------------------------------------------------------------------

  ex_queue_entry * lentry = childQueues_[0].up->getHeadEntry();
  ex_queue_entry * rentry = childQueues_[1].up->getHeadEntry();

  if (lentry->upState.status == ex_queue::Q_SQLERROR)
    {
      processError(pstate, endOfData, lentry->getAtp());
      return 0;
    }

  if (rentry->upState.status == ex_queue::Q_SQLERROR)
    {
      processError(pstate, endOfData, rentry->getAtp());
      return 0;
    }


  // -----------------------------------------------------------------
  // Now we have a row on both children.
  // Check which ones are at end of data.
  // -----------------------------------------------------------------

  Int32 numEod = 0;
  
  for (Int32 i = 0; i < 2; i++)
    {
      if (childQueues_[i].up->getHeadEntry()->upState.status ==
	  ex_queue::Q_NO_DATA)
	{
	  // found an end of data up entry
	  pstate.childStates_[i] = DONE_;
	  numEod++;

	  // take the other side (its EOD status gets checked, too)
	  side = 1 - i;
	}
    }

  // now that we have looked at both children:

  switch (numEod)
    {
    case 0:
      // - if we found 0 EOD entries, use the merge expression to find
      //   out which side and return TRUE
      {	
	// Compare the two rows and return the one
	// which is lesser. The mergeExpr() has been generated to
	// return TRUE if left is lesser than the right.
	rentry->getAtp()->copyPartialAtp(lentry->getAtp());
	
	ex_expr::exp_return_type retCode = mergeExpr()->eval(rentry->getAtp(), 0);
	if (retCode == ex_expr::EXPR_ERROR)
	  {
            processError(pstate, endOfData, rentry->getAtp());
            return 0;
	  }
	else if (retCode == ex_expr::EXPR_TRUE)
	  {
	    side = 0; // LEFT
	  }
	else
	  {
	    side = 1; // RIGHT (in case of all other return values)
	  }
      }
      return 1;

    case 1:
      // - if we found 1 EOD entry, return TRUE, the side is already set
      //   to the child that isn't yet at EOD
      return 1;

    case 2:
      // - if we found 2 EOD entries, set the endOfData indicator and
      //   return FALSE
      endOfData = 1;
      return 0;

    default:
      ex_assert(0,"A very unusual error occurred in the executor");
    }
  return 0; // if you ever reach here I'll eat a broom with the stick
}

/////////////////////////////////////////////////////////////////
// Constructor for o_union_tcb
/////////////////////////////////////////////////////////////////
ex_o_union_tcb::ex_o_union_tcb(
     const ex_union_tdb &  union_tdb,  // 
     const ex_tcb *    left_tcb,    // left queue pair
     const ex_tcb *    right_tcb,   // right queue pair
     ex_globals *glob,
	 NABoolean blocked_union, // Triggers -, blocked union 
	 Int32 hasNoOutputs // Triggers - 
     ) : ex_union_tcb( union_tdb, left_tcb, right_tcb, glob),
         rprocessedInputs_(processedInputs_),
         lprocessedInputs_(processedInputs_),
         rightRequestCnt_(0),
         phase2Event_((ExSubtask *)0),
         phase3Event_((ExSubtask *)0), // ++ Triggers -
		 blockedUnion_(blocked_union), // ++ Triggers -
		 hasNoOutputs_(hasNoOutputs)   // ++ Triggers -
{ }

//////////////////////////////////////////////////////////////////////
// decide which side to choose for up-processing for ordered union
// (first left, then right). Also pipeline as many requests as possible
// for processing in the right child. Argument side is changed only if
// whichSide() returns 1.
//////////////////////////////////////////////////////////////////////

Int32 ex_o_union_tcb::whichSide(ex_union_private_state &  pstate,
			      Int32 &side,
			      Int32 &endOfData)
{

  if (childQueues_[pstate.whichSide_].up->isEmpty())
    {
      // sorry, we are fixed on that side, need to wait
      endOfData = 0;
      return 0;
    }

  // is the current side at end of data?
  if (childQueues_[pstate.whichSide_].up->getHeadEntry()->upState.status ==
      ex_queue::Q_NO_DATA)
    {
      if (pstate.whichSide_ == 0)
	{
          // schedule workroutine of Phase2 since EOD from left child is received
	  phase2Event_->schedule();

	  rightRequestCnt_++; // there is at least one request that we can send
			      // to right child

	  // perhaps there are more EODs on left child UPqueue? try to 
	  // pipeline as many requests to the right child as possible.
          // because of pipelining, we set pstate.whichSide_ in 
          // StartRightchild().
          // for the same reason, pstate.childStates[0] is set to DONE_ 
          // in startRightchild().

	  queue_index ix;
	  ex_queue* upQ;

	  upQ = childQueues_[pstate.whichSide_].up;	
	  ix = upQ->getHeadIndex();
	  ix++;
	  while ( upQ->entryExists(ix) && 
                  upQ->getQueueEntry(ix)->upState.status == ex_queue::Q_NO_DATA )
	    {
	     rightRequestCnt_++; // change the corresponding pstate.whichSide_;
                                 // this is done in startRightchild().
	     ix++;
	    } // while pipeline is possible

	  // go produce replies from right child for this request 
	  endOfData = 0;
	  return 0;
	}
      else
	{
	  // second child has EOD, first child must have returned EOD
	  // earlier, otherwise we wouldn't be workin on the second one
          pstate.childStates_[pstate.whichSide_] = DONE_;
	  endOfData = 1;
	  // ++ Triggers -
      // schedule workroutine of left child since EOD from right child is received, 
	  // and there might be some left entries that where blocked 
	  // till the end of the exection of the right child
	  if (blockedUnion_)
		  phase3Event_->schedule();
	  // -- Triggers -
	  return 0;
	}
    }

  // success, return side stored in pstate
  side = pstate.whichSide_;
  return 1;
} // ex_o_union_tcb::whichSide()

///////////////////////////////////////////////////////////////
//
// Work procedure to send requests down to left child
//
///////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_o_union_tcb::workDownLeft()
{
  // while we have unprocessed down requests and while left
  // child's down ComQueue.has room, start more child requests
  while (qparent.down->entryExists(lprocessedInputs_) &&
	 !childQueues_[0].down->isFull()) 
    startLeftchild();

  processedInputs_ = whichSideParentIndex();

  return WORK_OK;
} // ex_o_union_tcb::workDownLeft()

//++ Triggers -, blocked union 
///////////////////////////////////////////////////////////////////////
//
// Work procedure to send requests down to left child, in blocked union
//
///////////////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_o_union_tcb::workDownBlockedLeft()
{
  // if we have unprocessed down requests 
  // start one child requests only for the head qparent entry
  if ((qparent.down->getHeadIndex() == lprocessedInputs_) &&
	 qparent.down->entryExists(lprocessedInputs_))
  {
	 ex_assert(!childQueues_[0].down->isFull(), "Full child queue, should not occur in blocked union");
	 startLeftchild();
  }
  processedInputs_ = whichSideParentIndex();

  return WORK_OK;
} // ex_o_union_tcb::workBlockedDownLeft()
//-- Triggers -, blocked union 

///////////////////////////////////////////////////////////////
//
// Work procedure to send requests down to right child
//
///////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_o_union_tcb::workDownRight()
{
  // while we have unprocessed down requests and while right
  // child's down ComQueue.has room, start requests (at least one,
  // may be more) that have received EOD from the left child.

  while ( qparent.down->entryExists(rprocessedInputs_) && 
  	  rightRequestCnt_ > 0 && !childQueues_[1].down->isFull()) 
    {
     startRightchild();
     rightRequestCnt_--;
    }

  processedInputs_ = whichSideParentIndex();

  return WORK_OK;
} // ex_o_union_tcb::workDownRight()
///////////////////////////////////////////////////////////////
//
// Helper to determine which child's (left or right) parent is farther 
// from the parent down ex_queue's head_.  It encapsulates the complexity
// that the queue_indexes can wrap around at 4,294,967,296.
//
///////////////////////////////////////////////////////////////
queue_index ex_o_union_tcb::whichSideParentIndex() 
{
  if ((lprocessedInputs_ - qparent.down->getHeadIndex())  >
      (rprocessedInputs_ - qparent.down->getHeadIndex()))
    return lprocessedInputs_;
  else
    return rprocessedInputs_;
}
///////////////////////////////////////////////////////////////
//
// Send union request to left child; left child down queue
// must have room! handle error processing
//
///////////////////////////////////////////////////////////////
void ex_o_union_tcb::startLeftchild()
{
  ex_queue_entry * lentry = childQueues_[0].down->getTailEntry();

  ex_queue_entry * pentry_down = qparent.down->getQueueEntry(lprocessedInputs_);
  const ex_queue::down_request request = pentry_down->downState.request;
  ex_union_private_state &  pstate =
    *((ex_union_private_state*) pentry_down->pstate);

  // pass same request down to left  child
  lentry->downState.request = request;
  lentry->downState.requestValue = pentry_down->downState.requestValue;
  lentry->downState.parentIndex = lprocessedInputs_;

  // pass it to left child
  lentry->copyAtp(pentry_down);

  pstate.matchCount_ = 0; // no matches yet for this parent row

  childQueues_[0].down->insert();

  pstate.childStates_[0] = STARTED_;

  if (request == ex_queue::GET_NOMORE)
    {
    // immediately cancel the request (requests are already in
    // cancelled state but the cancel callback isn't activated yet)
    childQueues_[0].down->cancelRequestWithParentIndex(lprocessedInputs_);
    pstate.childStates_[0] = CANCELLED_;

    // $$$$ should find a smarter way to do this
    }
  lprocessedInputs_++;
} // ex_o_union_tcb::startLeftchild()

/////////////////////////////////////////////////////////////////
//
// Send request down to right child. Right child down queue must
// have room! handle error processing
//
/////////////////////////////////////////////////////////////////
void ex_o_union_tcb::startRightchild()
{
 ex_queue_entry * rentry = childQueues_[1].down->getTailEntry();

 ex_queue_entry * pentry_down = qparent.down->getQueueEntry(rprocessedInputs_);
 const ex_queue::down_request request = pentry_down->downState.request;
 ex_union_private_state &  pstate =
   *((ex_union_private_state*) pentry_down->pstate);

 ex_assert(pstate.whichSide_ == 0, 
           "ex_o_union_tcb::startRightchild() right child started too early");
 pstate.childStates_[0] = DONE_; // because of pipelining
 pstate.whichSide_ = 1; // because of pipelining

 // pass same request down to right child
 rentry->downState.request = request;
 rentry->downState.requestValue = pentry_down->downState.requestValue;
 rentry->downState.parentIndex = rprocessedInputs_;

 // pass it to right child
 rentry->copyAtp(pentry_down);

 // For NAR, we do not want to send down the diags area.
 if (union_tdb().inNotAtomicStmt())
   rentry->setDiagsArea(0);

 childQueues_[1].down->insert();

 pstate.childStates_[1] = STARTED_;

 if (request == ex_queue::GET_NOMORE)
   {
     // immediately cancel the request (requests are already in
     // cancelled state but the cancel callback isn't activated yet)
     childQueues_[1].down->cancelRequestWithParentIndex(rprocessedInputs_);
     pstate.childStates_[1] = CANCELLED_;

     // $$$$ should find a smarter way to do this
   }
 rprocessedInputs_++;
} // ex_o_union_tcb::startRightchild()

///////////////////////////////////////////////////////////////////////
//
// Register with the scheduler
//
///////////////////////////////////////////////////////////////////////
void ex_o_union_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // ++Triggers -
  if (blockedUnion_)
  {
	// in blocked union, there is only one entry served at a time
	// left down is blocked till right child is done

	// Phase1: down queues are handled by workDownBlockedLeft()
	// Only one entry can go down at a time
	sched->registerInsertSubtask(sWorkBlockedPhase1, this, qparent.down,"DN");
	sched->registerUnblockSubtask(sWorkBlockedPhase1, this, childQueues_[0].down);

	// Phase2 of Blockedunion is handled by workDownRight()
	// the entry is schdulde to the right side when the left side is done
	sched->registerUnblockSubtask(sWorkPhase2, this, childQueues_[1].down);
	phase2Event_ = sched->registerNonQueueSubtask(sWorkPhase2, this);

	// Phase3 of Blockedunion - rescedule left child when the right side is done
	phase3Event_ = sched->registerNonQueueSubtask(sWorkBlockedPhase1, this);

  } 
  else {
  // --Triggers -

	// Phase1: down queues are handled by workDownLeft()
	sched->registerInsertSubtask(sWorkPhase1, this, qparent.down,"DN");
	sched->registerUnblockSubtask(sWorkPhase1, this, childQueues_[0].down);

	//Phase2 of Orderedunion is handled by workDownRight()
	sched->registerUnblockSubtask(sWorkPhase2, this, childQueues_[1].down);
	phase2Event_ = sched->registerNonQueueSubtask(sWorkPhase2, this);

  } 

  // cancellations are handled by the complaints department
  sched->registerCancelSubtask(sCancel, this, qparent.down,"CN");

  // up queues are handled by default workUp() of parent class
  sched->registerUnblockSubtask(sWorkUp, this, qparent.up,"UP");
  sched->registerInsertSubtask(sWorkUp, this, childQueues_[0].up);
  sched->registerInsertSubtask(sWorkUp, this, childQueues_[1].up);
  // the parent queues will be resizable, so register a resize subtask.
  registerResizeSubtasks();
} // ex_o_union_tcb::registerSubtasks

//////////////////////////////////////////////////////////////
//
// Work routine for cancel processing for class ex_o_union_tcb
//
//////////////////////////////////////////////////////////////
ExWorkProcRetcode ex_o_union_tcb::processCancel()
{
  // Check the down queue from the parent for cancellations. Propagate
  // cancel requests and remove all requests that are completely cancelled.
  // Loop over all requests that have been sent down.

  queue_index lix = qparent.down->getHeadIndex();
  queue_index rix = lix;

  while (lix != lprocessedInputs_)
    {
      ex_queue_entry * pentry_down = qparent.down->getQueueEntry(lix);
      
      // check whether the current down request is cancelled
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	{
	  // yes, cancel this down request if not already done so

	  ex_union_private_state &  pstate =
	    *((ex_union_private_state*) pentry_down->pstate);  

	  // cancel child requests if not already done so and if they
	  // aren't already finished
	  if (pstate.childStates_[0] == ex_union_tcb::STARTED_)
	    {
		  // cancel request to this child
	      childQueues_[0].down->cancelRequestWithParentIndex(lix);
	      pstate.childStates_[0] = ex_union_tcb::CANCELLED_;
	    }
	} // cancelled rows request

      lix++; // may wrap around
    } // parent queue is not empty

  while (rix != rprocessedInputs_)
    {
      ex_queue_entry * pentry_down = qparent.down->getQueueEntry(rix);
      
      // check whether the current down request is cancelled
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	{
	  // yes, cancel this down request if not already done so

	  ex_union_private_state &  pstate =
	    *((ex_union_private_state*) pentry_down->pstate);  

	  // cancel child requests if not already done so and if they
	  // aren't already finished
	  if (pstate.childStates_[1] == ex_union_tcb::STARTED_)
	    {
		  // cancel request to this child
	      childQueues_[1].down->cancelRequestWithParentIndex(rix);
	      pstate.childStates_[1] = ex_union_tcb::CANCELLED_;
	    }
	} // cancelled rows request

      rix++; // may wrap around
    } // parent queue is not empty

  // checked all active entries nothing more to do
//  return WORK_CALL_AGAIN;
  return WORK_OK;
} // ex_o_union_tcb::processCancel()

// -----------------------------------------------------------------------
// Conditional Union
// -----------------------------------------------------------------------

ex_c_union_tcb::ex_c_union_tcb(
     const ex_union_tdb &union_tdb,
     const ex_tcb *left_tcb,   
     const ex_tcb *right_tcb, 
     ex_globals *glob) : 
  ex_union_tcb(union_tdb, left_tcb, right_tcb, glob)
{
}

void ex_c_union_tcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  //
  // Down queues are handled by CondWorkDown()
  //
  sched->registerInsertSubtask(sCondWorkDown, this, qparent.down,"DN");

  Int32 i = 0;
  for (; i < numChildren(); i++ )
    sched->registerUnblockSubtask(sCondWorkDown, this, childQueues_[i].down);


  //
  // Cancellations are handled by the complaints department.
  //
  sched->registerCancelSubtask(sCancel, this, qparent.down,"CN");

  //
  // Up queues are handled by workUp().
  //
  sched->registerUnblockSubtask(sWorkUp, this, qparent.up,"UP");

  for (i = 0; i < numChildren(); i++ )
    sched->registerInsertSubtask(sWorkUp, this, childQueues_[i].up);

  workUpEvent_ = sched->registerNonQueueSubtask(sWorkUp, this);
  // the parent queues will be resizable, so register a resize subtask.
  registerResizeSubtasks();
} // ex_union_tcb::registerSubtasks()

ExWorkProcRetcode ex_c_union_tcb::condWorkDown()
{

  //
  // Process the parent request.
  //
  while (qparent.down->entryExists(processedInputs_))
  {
    //
    // Get handles.
    //
    ex_queue_entry *pentry_down = 
      qparent.down->getQueueEntry(processedInputs_);

    ex_union_private_state &pstate = 
      *((ex_union_private_state*) pentry_down->pstate);  

    //
    // Evaluate the conditional expression. If it evaluates to TRUE,
    // the left child is started. If it evaluates to FALSE, and there is
    // a right child, it is started. Othewise, stop servicing the request.
    //
    ex_expr::exp_return_type retCode = 
      condExpr()->eval(pentry_down->getAtp(), 0);

    if (retCode == ex_expr::EXPR_TRUE) 
    {
      if (!(union_tdb().inNotAtomicStmt()))
	pstate.whichChild_ = 0;
      else
      {
	pstate.whichChild_ = -1;
	pstate.setErrorCode(1);
	processedInputs_++;
	ComDiagsArea *  da = ExRaiseSqlError(getGlobals()->getDefaultHeap(), 
					     pentry_down,
					     (ExeErrorCode)-EXE_NOTATOMIC_ENABLED_AFTER_TRIGGER);
	pentry_down->setDiagsArea(da);
	workUpEvent_->schedule();
	continue;
      }
    }
    else if ((retCode == ex_expr::EXPR_FALSE) ||
             (retCode == ex_expr::EXPR_NULL))
    {
      pstate.whichChild_ = 1;
    }
    else if (retCode == ex_expr::EXPR_ERROR)
    {
      pstate.whichChild_ = -1;
      pstate.setErrorCode(1);
      processedInputs_++;

      // raise a Triggered Action Exception if an error occurred while
      // evaluating the WHEN clause of a trigger.
      if (trigExceptExpr()) // triggers
      {
         ComDiagsArea * da = pentry_down->getAtp()->getDiagsArea();
         ex_assert((da != NULL) && (da->returnIndex(-ComDiags_TrigActionExceptionSQLCODE) == NULL_COLL_INDEX), "Should not be here");
         trigExceptExpr()->eval(pentry_down->getAtp(), 0);
      }
      // schedule the workUp method, b/c no child responses are guaranteed.
      workUpEvent_->schedule();
      continue;
    }
    else 
    {
      ex_assert(0, "Unexpected return from exp_expr::eval");
    }

    //
    // Pass the request to the appropirate child.
    //
    if (!childQueues_[pstate.whichChild_].down->isFull())
      start();
    else 
      break;
  }

  return WORK_OK;

} // ex_c_union_tcb::condWorkDown()

void ex_c_union_tcb::start()
{
  //
  // Get handles.
  //
  ex_queue_entry *pentry_down = qparent.down->getQueueEntry(processedInputs_);
 
  const ex_queue::down_request request = pentry_down->downState.request;

  ex_union_private_state &pstate = 
    *((ex_union_private_state*) pentry_down->pstate);  

  Int32 child = pstate.whichChild_;

  ex_assert(pstate.validChild(), "ex_c_union_tdb::start - invalid state");

  ex_queue_entry *entry = childQueues_[child].down->getTailEntry();

  //
  // Pass request down to child.
  //
  entry->downState.request = request;
  entry->downState.requestValue = pentry_down->downState.requestValue;
  entry->downState.parentIndex = processedInputs_;
  entry->copyAtp(pentry_down);
  childQueues_[child].down->insert();
  
  pstate.childStates_[child] = STARTED_;
  pstate.matchCount_ = 0;

  // 
  // Immediately cancel the request if required.
  //
  if (request == ex_queue::GET_NOMORE)
  {
    childQueues_[child].down->cancelRequestWithParentIndex(processedInputs_);
    pstate.childStates_[child] = CANCELLED_;
  }

  processedInputs_++;

} // ex_c_union_tcb::start()

void ex_c_union_tcb::stop()
{
  //
  // Get handles.
  //
  ex_queue_entry *pentry_down = qparent.down->getHeadEntry();  

  ex_union_private_state &pstate =
    *((ex_union_private_state*) pentry_down->pstate);  

  Int32 child = pstate.whichChild_;

  ex_queue_entry *pentry = qparent.up->getTailEntry();

  //
  // Return EOF to parent.
  //
  pentry->upState.status = ex_queue::Q_NO_DATA;
  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
  pentry->upState.downIndex = qparent.down->getHeadIndex();
  pentry->upState.setMatchNo(pstate.matchCount_);
  
  qparent.up->insert();

  //
  // Consume the child row.
  //
  if (pstate.validChild())
    childQueues_[child].up->removeHead();	  

  // 
  // This parent request has been processed. 
  //
  pstate.init();
  qparent.down->removeHead();


} // ex_c_union_tcb::stop()

Int32 ex_c_union_tcb::whichSide(ex_union_private_state &pstate,
			      Int32 &side,
			      Int32 &endOfData)
{

  if (!pstate.validChild())
    {
    endOfData = 1;
    return 0;
    }

  Int32 child = pstate.whichChild_;

  //
  // Check if the child has any data.
  //
  if (childQueues_[child].up->isEmpty())
    return 0;

  //
  // Check if the child's at the end of its data stream.
  //
  if (childQueues_[child].up->
      getHeadEntry()->upState.status == ex_queue::Q_NO_DATA)
  {
   // before setting the endofData flag and the child state to DONE_ we check to
   // see if  EXE_CS_EOD/EXE_CS_EOD_ROLLBACK_ERROR type warning/error needs 
   // to be issued.  If the childstate is in a CANCELLED_ state an error has 
   // occurred in the child and we do not need to handle 
   // EXE_CS_EOD/EXE_CS_EOD_ROLLBACK_ERROR type warning/error. The childstate 
   // is set to CANCELLED_ by calling processError() which calls 
   // processCancel().
   // If there was error while evaluating the conditional expression then 
   // whichSide() is not called at all, which is the appropriate behaviour 
   // to handle this error/warning.

    if (pstate.childStates_[child] != CANCELLED_) {
      NABoolean expectingRows ;
      if (child == 0)
        expectingRows = union_tdb().expectingLeftRows() ;
      else
        expectingRows = union_tdb().expectingRightRows() ;
        
      if (expectingRows && !pstate.matchCount_){
        if (union_tdb().afterUpdate()) {
          processEODErrorOrWarning(FALSE);
        }
        else {
          processEODErrorOrWarning(TRUE);
        }
      }
    }

    pstate.childStates_[child] = DONE_;
    endOfData = 1;
   
    return 0;
  }

  //
  // The child has real data.
  //
  side = child;
  return 1;

} // ex_c_union_tcb::whichSide()

ExWorkProcRetcode ex_c_union_tcb::processCancel()
{
  // 
  // Similar to base class processCancel but only concerned with 1 child.
  //
  queue_index ix = qparent.down->getHeadIndex();

  while (ix != processedInputs_)
  {
    ex_queue_entry *pentry_down = qparent.down->getQueueEntry(ix);

    ex_union_private_state &pstate =
      *((ex_union_private_state*) pentry_down->pstate);  

    Int32 child = pstate.whichChild_;

    if (pentry_down->downState.request == ex_queue::GET_NOMORE &&
        pstate.validChild() &&
        pstate.childStates_[child] == ex_union_tcb::STARTED_)
    {
      childQueues_[child].down->cancelRequestWithParentIndex(ix);
      pstate.childStates_[child] = ex_union_tcb::CANCELLED_;
    }

    ix++; 
  }

  return WORK_OK;

} // ex_c_union_tcb::processCancel()

void ex_c_union_tcb::processError(ex_union_private_state &pstate, 
                                  Int32 &endOfData,
                                  atp_struct* atp)
{
  // 
  // Similiar to base class processError but only concerned with 1 child.
  //
  ex_queue_entry * pentry_down = qparent.down->getHeadEntry();
  ex_queue_entry * pentry = qparent.up->getTailEntry();

  Int32 child = pstate.whichChild_;

  pentry->copyAtp(atp);
  pentry->upState.status = ex_queue::Q_SQLERROR;
  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
  pentry->upState.downIndex = qparent.down->getHeadIndex();
  pentry->upState.setMatchNo(pstate.matchCount_);
  qparent.up->insert();	  

  qparent.down->cancelRequest(qparent.down->getHeadIndex());
  processCancel();

  if (!(childQueues_[child].up->isEmpty()) && 
      pstate.validChild() &&
      pstate.childStates_[child] != DONE_)
  {
    ex_queue_entry *centry = childQueues_[child].up->getHeadEntry();

    switch (centry->upState.status) 
    {
    case ex_queue::Q_OK_MMORE:
    case ex_queue::Q_SQLERROR:
      childQueues_[child].up->removeHead();
      break;
    case ex_queue::Q_NO_DATA:
      pstate.childStates_[child] = DONE_;
      break;
    case ex_queue::Q_INVALID:
      ex_assert(0, "ex_c_union_tcb::processError() Invalid state");
      break;
    }
  }

  endOfData = (pstate.childStates_[child] == DONE_);

} // ex_c_union_tcb::processError

//issue a EXE_CS_EOD or a EXE_CS_EOD_ROLLBACK_ERROR type error/warning for
//conditional union
void ex_c_union_tcb::processEODErrorOrWarning(NABoolean isWarning)
{

  ex_queue_entry *pdentry = qparent.down->getHeadEntry();
  ex_queue_entry *puentry = qparent.up->getTailEntry();

  ComDiagsArea * da ;

  if (isWarning)
     da = ExRaiseSqlError(getGlobals()->getDefaultHeap(), puentry,
                         (ExeErrorCode)-EXE_CS_EOD);
  else
     da = ExRaiseSqlError(getGlobals()->getDefaultHeap(), puentry,
                         (ExeErrorCode)-EXE_CS_EOD_ROLLBACK_ERROR);

  puentry->setDiagsArea(da);
  puentry->upState.status = ex_queue::Q_SQLERROR;
  puentry->upState.parentIndex = pdentry->downState.parentIndex;
  puentry->upState.downIndex = qparent.down->getHeadIndex();
  puentry->upState.setMatchNo(0);
  
  qparent.up->insert();
} //ex_c_union_tcb::processEODErrorOrWarning


///////////////////////////////////////////////////////////////
//
//  Private state procedures
//
///////////////////////////////////////////////////////////////

// Constructor and destructor for union_private_state
//
ex_union_private_state::ex_union_private_state() 
{
  init();
}

void ex_union_private_state::init() 
{
  matchCount_     = 0;  // number of rows returned for this parent row
  childStates_[0] = ex_union_tcb::EMPTY_;
  childStates_[1] = ex_union_tcb::EMPTY_;
  whichSide_      = 0;  // start with left (important for ordered union)
  whichChild_     = -1; // determined by conditional union (0 or 1).
}

ex_union_private_state::~ex_union_private_state()
{
}

ex_tcb_private_state * ex_union_private_state::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ex_union_private_state();
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ex_union_tcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ex_union_private_state> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}
