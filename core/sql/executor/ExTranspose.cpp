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
* File:         ExTranspose.cpp
* Description:  Methods for the tdb and tcb of a TRANSPOSE operation
*               
*               
* Created:      4/1/97
* Language:     C++
*
*
*
*
******************************************************************************
*/

//
// This file contains all the generator and executor methods associated
// with a transpose operator
//

#include "ExTranspose.h"
#include "ExSimpleSqlBuffer.h"
#include "ExStats.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////


// ExTransposeTdb::build() --------------------------------------------
// Build the TCB tree for this node and all nodes below.
//
// Parameters
//
// ex_globals *glob - contains references to global executor information,
//                    notably the space object used to allocate objects.
//
// Returns - returns the TCB tree build for this node and all nodes below.
//
ex_tcb *
ExTransposeTdb::build(ex_globals *glob) 
{

  // Build the Tcb tree below the transpose node.
  //
  ex_tcb *childTcb = childTdb_->build(glob);

  // Build the transpose Tcb, given the TDB.
  //
  ExTransposeTcb *transTcb =
    new(glob->getSpace()) ExTransposeTcb(*this, *childTcb, glob);
  
  // add the transTcb and all its work procedures to the schedule
  //
  transTcb->registerSubtasks();
  
  // Return the result to my parent.
  //
  return transTcb;
}


/////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
/////////////////////////////////////////////////////////////////////////

// ExTransposeTcb::ExTransposeTcb() ------------------------------------
// Constructor for transpose TCB
//
ExTransposeTcb::ExTransposeTcb(const ExTransposeTdb &transTdbLocal,
			       const ex_tcb &childTcb,
			       ex_globals *glob) :
  ex_tcb(transTdbLocal, 1, glob) 
{
    
  childTcb_ = &childTcb;
  CollHeap *space = glob->getSpace();
  
  // Allocate the buffer pool
  //
  //pool_ = new(space) sql_buffer_pool(transTdb().numBuffers_,
  //transTdb().bufferSize_,
  //space);
  //
  pool_ = new(space) ExSimpleSQLBuffer(transTdb().numBuffers_,
				       transTdb().bufferSize_, 
				       transTdb().transRowLen_,
				       space);
 
  // get the queue used by my child to communicate with me
  //
  childQueue_  = childTcb_->getParentQueue();
  
  // the Transpose TCB adds input tupps to its down queues, therefore we
  // need to allocate new ATPs for its children
  //
  childQueue_.down->allocateAtps(glob->getSpace());
  
  // Allocate the queue to communicate with parent
 
  allocateParentQueues(qParent_,TRUE);
  // Initialize processedInputs_ to refer to the queue entry
  // which will be used next.
  //
  processedInputs_ = qParent_.down->getHeadIndex();
  

  // fixup all expressions expressions
  //

  // The transpose expressions.
  //
  for(Int32 i = 0; i < transTdb().numTransExprs_; i++) {
    if(transTdb().transColExprs_[i])
      transTdb().transColExprs_[i]->fixup(0,
					  getExpressionMode(), this,
					  glob->getSpace(),
					  glob->getDefaultHeap(), FALSE, glob);
  }

  // Predicates to be applied after perform the transpose.
  //
  if(afterTransPred())
    afterTransPred()->fixup(0,
			    getExpressionMode(), this,
			    glob->getSpace(),
			    glob->getDefaultHeap(), FALSE, glob);

}

// ExTransposeTcb::~ExTransposeTcb() ---------------------------------------
// Destructor for ExTransposeTcb
// Free up the queues and the sql buffer and the child TCB tree.
//
ExTransposeTcb::~ExTransposeTcb()
{
  delete qParent_.up;
  delete qParent_.down;
  freeResources();
}
  
// ExTransposeTcb::freeResources() ----------------------------------------
// Free any run time resources.
// For transpose, this means freeing up the buffer pool.
// The queues are not freed here. (Should they be?).
// This method is call by the TCB destructor.
//
void ExTransposeTcb::freeResources()
{
  if (pool_)
    delete pool_;
  
  pool_ = 0;
}

// ExTransposeTcb::registerSubtasks() -------------------------------------
// Register all the transpose subtasks with the scheduler.
//
void ExTransposeTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();
  
  // down queues are handled by workDown()
  //

  // Schedule this routine if a new entry is inserted into the
  // parents down queue and we are waiting on an event.
  // (had returned WORK_OK).
  //
  sched->registerInsertSubtask(sWorkDown, this, qParent_.down, "DN");

  // Schedule this routine if the child's down queue changes from being
  // full to being not full and we are waiting on an event.
  // (had returned WORK_OK).
  //
  sched->registerUnblockSubtask(sWorkDown, this, childQueue_.down, "DN");
  
  // Schedule this routine if a cancel request occurs on the 
  // parents down queue.
  //
  sched->registerCancelSubtask(sCancel, this, qParent_.down, "CN");
  
  // up queues are handled by workUp()
  //

  // Schedule this routine if the parent's up queue changes from being
  // full to being not full and we are waiting on an event.
  // (had returned WORK_OK).
  //
  sched->registerUnblockSubtask(sWorkUp, this, qParent_.up,"UP");

  // Schedule this routine if a new entry is inserted into the
  // child's up queue and we are waiting on an event.
  // (had returned WORK_OK).
  //
  sched->registerInsertSubtask(sWorkUp, this, childQueue_.up);
 // the parent queues will be resizable, so register a resize subtask.
  registerResizeSubtasks();
}

// ExTransposeTcb::start() ------------------------------------------
// Starts up the next requested operation by sending
// requests to children. Child queue must have room!
// Called by workDown().
//
void
ExTransposeTcb::start()
{
  // Advance processedInputs_ by one, all checks need to be done
  // by the caller. No down request is sent for a cancelled message.
  //
  ex_queue_entry *childEntry = childQueue_.down->getTailEntry();
  
  ex_queue_entry *pEntryDown = qParent_.down->getQueueEntry(processedInputs_);
  const ex_queue::down_request request = pEntryDown->downState.request;
  
  ExTransposePrivateState *pState =
    ((ExTransposePrivateState*)pEntryDown->pstate);

  // pass request to children
  //
  childEntry->downState.request = ex_queue::GET_ALL;
  childEntry->downState.requestValue = pEntryDown->downState.requestValue;
  childEntry->downState.parentIndex = processedInputs_;
  childEntry->copyAtp(pEntryDown);
  
  // no matches yet for this parent row
  //
  pState->matchCount_ = 0;

  // Insert request in child queue.
  //
  childQueue_.down->insert();

  // Record the state of this request.
  //
  pState->childState_ = STARTED_;
    
  // If this is a CANCEL request, Cancel the request we just gave to
  // our child.
  //
  if (request == ex_queue::GET_NOMORE) {
    // immediately cancel the request (requests are already in
    // cancelled state but the cancel callback isn't activated yet)
    //
    childQueue_.down->cancelRequestWithParentIndex(processedInputs_);
    pState->childState_ = CANCELLED_;
    
    // $$$$ should find a smarter way to do this
  }

  // We are now ready to look at the next request.
  //
  processedInputs_++;
}

// ExTransposeTcb::stop() ----------------------------------------------
// All child rows have been returned.
// Now return an EOD indication to parent.
//
void ExTransposeTcb::stop()
{
  // Remove the head entries of the parent down queue and the
  // child up queue. All checks are done by the caller.
  //
  ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();  

  ExTransposePrivateState &pState =
    *((ExTransposePrivateState*) pEntryDown->pstate);  

  ex_queue_entry *pEntry = qParent_.up->getTailEntry();
  
  pEntry->upState.status = ex_queue::Q_NO_DATA;
  pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;

  if (getStatsEntry()) {
    getStatsEntry()->setActualRowsReturned(pState.matchCount_); 
  }
  
  // insert EOD into parent
  //
  qParent_.up->insert();
  
  // consume the child row
  //
  childQueue_.up->removeHead();	  

  // Reinitialize the state of this processed request.
  // Pstate will be initialized for when this entry gets used again.
  //
  pState.init();

  // this parent request has been processed. 
  //
  qParent_.down->removeHead();
}

ExWorkProcRetcode
ExTransposeTcb::processCancel()
{
  // Check the down queue from the parent for cancellations. Propagate
  // cancel requests and remove all requests that are completely cancelled.

  // loop over all requests that have been sent down
  // (the others are handled by start() later)
  queue_index ix = qParent_.down->getHeadIndex();

  while (ix != processedInputs_) {
    ex_queue_entry *pEntryDown = qParent_.down->getQueueEntry(ix);
      
    // check whether the current down request is cancelled
    //
    if (pEntryDown->downState.request == ex_queue::GET_NOMORE) {
	
      // yes, cancel this down request if not already done so
      //
      ExTransposePrivateState &pState =
	*((ExTransposePrivateState*) pEntryDown->pstate);  

      // cancel child requests if not already done so and if they
      // aren't already finished
      //
      if (pState.childState_ == ExTransposeTcb::STARTED_) {
		
	// cancel request to this child
	//
	childQueue_.down->cancelRequestWithParentIndex(ix);
	pState.childState_ = ExTransposeTcb::CANCELLED_;
      }
	  
    } // cancelled rows request

    ix++; // may wrap around
  } // parent queue is not empty
  
  // checked all active entries nothing more to do
  return WORK_OK;
}

// -----------------------------------------------------------------------
// Generic work procedure should never be called
// -----------------------------------------------------------------------
ExWorkProcRetcode ExTransposeTcb::work()
{
  ex_assert(0,"Should never reach ExTransposeTcb::work()");
  return WORK_BAD_ERROR;
}

// -----------------------------------------------------------------------
// Work procedure to send requests down
// -----------------------------------------------------------------------
ExWorkProcRetcode ExTransposeTcb::workDown()
{
  // while we have unprocessed down requests and while
  // child's down ComQueue.has room, start more child requests
  //
  while (qParent_.down->entryExists(processedInputs_) &&
	 ! childQueue_.down->isFull())
    start();

  return WORK_OK;
}

// -----------------------------------------------------------------------
// generic up-processing for transpose
// -----------------------------------------------------------------------

ExWorkProcRetcode
ExTransposeTcb::workUp() 
{
	
  // while there is a chance that we have work (may exit via return)
  // (If processedInputs_ equals the HeadIndex(), then there are no
  // entries in the parent down queue.)
  //
  while (qParent_.down->getHeadIndex() != processedInputs_) {
	
    // get head entry and pState
    //
    ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();

    ExTransposePrivateState &pState =
      *((ExTransposePrivateState*)pEntryDown->pstate);

    // while we have room in the up queue and rows to process
    //
    while (!qParent_.up->isFull() && !childQueue_.up->isEmpty()) {
			
      ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();
      ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();

      if(cEntry->upState.status == ex_queue::Q_NO_DATA) {

	// Send EOD to parent and clean up the current request.
	// (will loop back to to of routine and try the next
	// request if there is one)
	//
	stop();

	// Must break here (rather than continue, so that we will
	// reset the pEntryDown and pState.)
	//
	break;
      } else if(pState.childState_ == CANCELLED_) {
      } else if(cEntry->upState.status == ex_queue::Q_SQLERROR) {
        pEntryUp->copyAtp(cEntry);
        processError();
        continue;
      } else if (pEntryDown->downState.request == ex_queue::GET_N &&
                 pEntryDown->downState.requestValue <= 
                 (Lng32)pState.matchCount_) {

        qParent_.down->cancelRequest(qParent_.down->getHeadIndex());
        processCancel();
        continue;

      } else {

        ex_expr::exp_return_type retCode = ex_expr::EXPR_TRUE;
        Int32 cancelled = FALSE;

        // While we still have expressions to apply to the current
        // child entry and the parent can accept rows.
        // Each expression that is applied to the child entry
        // produces one row to be put onto the parents up queue.
        //
        while(pState.transCount_ < transTdb().numTransExprs()
              && !qParent_.up->isFull()) { 

          if (pEntryDown->downState.request == ex_queue::GET_N &&
              pEntryDown->downState.requestValue <= 
              (Lng32)pState.matchCount_) {

            qParent_.down->cancelRequest(qParent_.down->getHeadIndex());
            processCancel();
            cancelled = TRUE;
            break;
          }

          // Get the entry on the parent up queue.
          //
          ex_queue_entry *pEntry = qParent_.up->getTailEntry();

          // Copy all the TP of the child to the parent.
          //

          if(transTdb().criDescUp_->noTuples() == 
             transTdb().criDescDown_->noTuples() + 1)
            pEntry->copyAtp(pEntryDown);
          else
            pEntry->copyAtp(cEntry);

          // Transpose also adds one TP for the generated column.  A
          // tuple must be allocated.
          //
          if (pool_->getFreeTuple(pEntry->getTupp(transTdb().transTuppIndex_))) {
            // Return. Will be called again when some space frees up.
            // Before returning, release acquired references.
            pEntry->getAtp()->release();
            return WORK_POOL_BLOCKED;
          }

          // Apply the proper expression to this row.  This will
          // generate the proper values for the generated columns.
          //
          retCode = transColExpr(pState.transCount_)->
            eval(pEntry->getAtp(), cEntry->getAtp());

          if(retCode == ex_expr::EXPR_ERROR) {
            processError();
            break;
          }

          // Advance the expression counter.
          //
          pState.transCount_++;

          // Apply any selection predicate. This predicate is applied
          // after the transpose columns have been generated and this
          // predicate will likely involve this generated columns 
          // (otherwise, it could have been pushed down to the child).
          //
          retCode = ex_expr::EXPR_TRUE;
      
          if (afterTransPred()) 
            retCode = 
              afterTransPred()->eval(pEntry->getAtp(), cEntry->getAtp());
      
          switch(retCode) {
          case ex_expr::EXPR_TRUE:
            
            // Finialize the queue entry, then insert it
            //
            pEntry->upState.status = cEntry->upState.status;
            pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
            pEntry->upState.setMatchNo(pState.matchCount_ + 1);
            pState.matchCount_++;
            qParent_.up->insert();
            break;
          case ex_expr::EXPR_FALSE:
            // Since this entry will not be inserted into the parent up queue, 
            // release acquired references  It was failure to do so that 
            // caused solution 10-090427-1169.
            pEntry->getAtp()->release();
            break;
          case ex_expr::EXPR_ERROR:
            processError();
            break;
          default:
            break;
          }
          
          if(retCode == ex_expr::EXPR_ERROR)
            break;
        }
	
        if(retCode == ex_expr::EXPR_ERROR) {
          // Should cause stop() to be called....
          //
          continue;
        }

        if(cancelled == TRUE)
          continue;

        // We still have more expressions to apply.  We must have
        // reached this point because the parent up queue is full.
        // Return, will be call again when the queue becomes non-full.
        //
        if(pState.transCount_ < transTdb().numTransExprs())
          return WORK_OK;
      }
      // We have applied all the expressions on this child entry,
      // reset the expression counter, remove the child entry
      // and then see if there is another entry to be processed.
      //
      pState.transCount_ = 0;
      childQueue_.up->removeHead();
			
    } // while parent up ComQueue.has room and child up ComQueue.has replies
  
    // If we got here because we finished a request and stop()ed,
    // then try again on a new request.  But if we got here because
    // the parent queue is full or the child queue is empty, then
    // we must return.
    //
    if(qParent_.up->isFull() || childQueue_.up->isEmpty())
      return WORK_OK;

  } // while parent ComQueue.has entries

  // parent down queue is empty
  //
  return WORK_OK;
}

void
ExTransposeTcb::processError()
{
  ex_queue_entry * pEntryDown = qParent_.down->getHeadEntry();
  ex_queue_entry * pEntry = qParent_.up->getTailEntry();
  ExTransposePrivateState &pState =
    *((ExTransposePrivateState*) pEntryDown->pstate);  

  pEntry->upState.status = ex_queue::Q_SQLERROR;
  pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
  pEntry->upState.downIndex = qParent_.down->getHeadIndex();
  pEntry->upState.setMatchNo(pState.matchCount_);
  qParent_.up->insert();	  

  // cancel this request and cancel all the children.
  qParent_.down->cancelRequest(qParent_.down->getHeadIndex());
  pState.childState_ = CANCELLED_;

  while(!childQueue_.up->isEmpty() && 
	(childQueue_.up->getHeadEntry()->upState.status 
	 != ex_queue::Q_NO_DATA))
    childQueue_.up->removeHead();
}

///////////////////////////////////////////////////////////////
//
//  Private state procedures
//
///////////////////////////////////////////////////////////////

// Constructor and destructor for ExTransposePrivateState
//
ExTransposePrivateState::ExTransposePrivateState() 
{
  init();
}


// Initialize the data members of the private state.
// Called by the constructor when the private state is
// initially allocated and when we are now with a parent
// request, so that the next time the queue entry is used,
// it's private state is already initialized.
//
void
ExTransposePrivateState::init() 
{
  matchCount_ = 0;
  transCount_ = 0;
  childState_ = ExTransposeTcb::EMPTY_;
}

ExTransposePrivateState::~ExTransposePrivateState()
{
}

ex_tcb_private_state *
ExTransposePrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace())
    ExTransposePrivateState();
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExTransposeTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExTransposePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}
