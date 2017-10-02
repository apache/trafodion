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
* File:         ExPackedRows.cpp
* Description:  Methods for the tdb and tcb of a UNPACKROWS operation
*               
*               
* Created:      6/19/97
* Language:     C++
*
*
*
*
******************************************************************************
*/

//
// This file contains all the executor methods associated
// with a unPackRows operator
//

#include "ExPackedRows.h"
#include "ExSimpleSqlBuffer.h"
#include "ex_error.h"
#include "ExpError.h"


//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////


// ExUnPackRowsTdb::build() --------------------------------------------
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
ExUnPackRowsTdb::build(ex_globals *glob) 
{

  // Build the Tcb tree below the unPackRows node.
  //
  ex_tcb *childTcb = NULL;
  if (childTdb_)
    childTcb = childTdb_->build(glob);

  // Build the unPackRows Tcb, given the TDB.
  //
  ex_tcb *unPackRowsTcb = NULL;

  if (rowwiseRowset())
    unPackRowsTcb = 
      new(glob->getSpace()) ExUnPackRowwiseRowsTcb(*this, glob);
  else
    unPackRowsTcb = 
      new(glob->getSpace()) ExUnPackRowsTcb(*this, *childTcb, glob);
    
  // add the unPackRowsTcb and all its work procedures to the schedule
  //
  unPackRowsTcb->registerSubtasks();
  
  // Return the result to my parent.
  //
  return unPackRowsTcb;
}


/////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
/////////////////////////////////////////////////////////////////////////

// ExUnPackRowsTcb::ExUnPackRowsTcb() ------------------------------------
// Constructor for unPackRows TCB
//
ExUnPackRowsTcb::ExUnPackRowsTcb(const ExUnPackRowsTdb &unPackRowsTdbLocal,
                                 const ex_tcb &childTcb,
                                 ex_globals *glob) :
  ex_tcb(unPackRowsTdbLocal, 1, glob) 
{
    
  childTcb_ = &childTcb;
  CollHeap *space = glob->getSpace();
  
  // Allocate the buffer pool. Allocate a buffer big enough to hold
  // twice the size of my upqueue.
  //
  pool_ = new(space) ExSimpleSQLBuffer(unPackRowsTdb().queueSizeUp_ * 2,
                                       unPackRowsTdb().unPackColsTupleLen_,
                                       space);

  // get the queue used by my child to communicate with me
  //
  childQueue_  = childTcb_->getParentQueue();
  
  // Allocate the queue to communicate with parent
  // (Child allocates queue to communicate with Child)
  //
  qParent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
                                      unPackRowsTdb().queueSizeDown_,
                                      unPackRowsTdb().criDescDown_,
                                      space);

  // Allocate the private state in each entry of the down queue
  //
  ExUnPackRowsPrivateState privateState(this);
  qParent_.down->allocatePstate(&privateState, this);


  // Initialize processedInputs_ to refer to the queue entry
  // which will be used next.
  //
  processedInputs_ = qParent_.down->getHeadIndex();
  
  // Allocate a queue to communicate with the parent node.
  //
  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                    unPackRowsTdb().queueSizeUp_,
                                    unPackRowsTdb().criDescUp_,
                                    space);

  // fixup all expressions expressions
  //

  if(unPackColsExpr())
    unPackColsExpr()->fixup(0,
                            getExpressionMode(), this,
                            glob->getSpace(),
                            glob->getDefaultHeap(), FALSE, glob);

  // Initialize the workAtp_ and the Tupp used to hold the
  // index value. This index value is an integer local to the
  // workUp method.
  //
  indexValueTuppDesc_.init(4, 0, (char *)&indexValue_);
  indexValueTupp_.init();
  indexValueTupp_ = &indexValueTuppDesc_;
  workAtp_ = allocateAtp(unPackRowsTdb().workCriDesc_, 
                         glob->getSpace());

  // Must always have a packingFactor expression
  //
  ex_assert(packingFactor(),
            "ExUnPackRowsTcb::ExUnPackRowsTcb(): Internal Error");

  packingFactor()->fixup(0,
                         getExpressionMode(), this,
                         glob->getSpace(),
                         glob->getDefaultHeap(), FALSE, glob);

  // Initialize the numRowsAtp_ and the numRowsTupp.  These are used
  // to hold the result of the move expression (packingFactor()) used
  // to extract the packing Factor from one of the packed rows.
  //
  numRowsTuppDesc_.init(4, 0, (char *)&numRows_);
  numRowsTupp_.init();
  numRowsTupp_ = &numRowsTuppDesc_;
  numRowsAtp_ = allocateAtp(unPackRowsTdb().criDescUp_, 
                            glob->getSpace());

}

// ExUnPackRowsTcb::~ExUnPackRowsTcb() ---------------------------------------
// Destructor for ExUnPackRowsTcb
// Free up the queues and the sql buffer and the child TCB tree.
//
ExUnPackRowsTcb::~ExUnPackRowsTcb()
{
  delete qParent_.up;
  delete qParent_.down;
  freeResources();
}
  
// ExUnPackRowsTcb::freeResources() ----------------------------------------
// Free any run time resources.
// For unPackRows, this means freeing up the buffer pool.
// The queues are not freed here. (Should they be?).
// This method is call by the TCB destructor.
//
void ExUnPackRowsTcb::freeResources()
{
// Add this when space_ is added;
//   if (workAtp_) {
//     deallocateAtp(workAtp_, space_);
//     workAtp_ = NULL;
//   }

//   if (numRowsAtp_) {
//     deallocateAtp(numRowsAtp_, space_);
//     numRowsAtp_ = NULL;
//   }

  if (pool_) {
    delete pool_;
    pool_ = NULL;
  }
}

// ExUnPackRowsTcb::registerSubtasks() -------------------------------------
// Register all the unPackRows subtasks with the scheduler.
//
void ExUnPackRowsTcb::registerSubtasks()
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
}

// ExUnPackRowsTcb::start() ------------------------------------------
// Starts up the next requested operation by sending
// requests to children. Child queue must have room!
// Called by workDown().
//
void
ExUnPackRowsTcb::start()
{
  // Advance processedInputs_ by one, all checks need to be done
  // by the caller. No down request is sent for a cancelled message.
  //
  ex_queue_entry *childEntry = childQueue_.down->getTailEntry();
  
  ex_queue_entry *pEntryDown = qParent_.down->getQueueEntry(processedInputs_);
  const ex_queue::down_request request = pEntryDown->downState.request;
  
  ExUnPackRowsPrivateState &pState =
    *((ExUnPackRowsPrivateState*)pEntryDown->pstate);  

  // pass request to children
  //
  childEntry->downState.request = ex_queue::GET_ALL;
  childEntry->downState.requestValue = pEntryDown->downState.requestValue;
  childEntry->downState.parentIndex = processedInputs_;
  childEntry->passAtp(pEntryDown);
  
  // no matches yet for this parent row
  //
  pState.matchCount_ = 0;

  // Insert request in child queue.
  //
  childQueue_.down->insert();

  // Record the state of this request.
  //
  pState.childState_ = STARTED_;
    
  // If this is a CANCEL request, Cancel the request we just gave to
  // our child.
  //
  if (request == ex_queue::GET_NOMORE) {
    // immediately cancel the request (requests are already in
    // cancelled state but the cancel callback isn't activated yet)
    //
    childQueue_.down->cancelRequestWithParentIndex(processedInputs_);
    pState.childState_ = CANCELLED_;
    
    // $$$$ should find a smarter way to do this
  }

  // We are now ready to look at the next request.
  //
  processedInputs_++;
}

// ExUnPackRowsTcb::stop() ----------------------------------------------
// All child rows have been returned.
// Now return an EOD indication to parent.
//
void ExUnPackRowsTcb::stop()
{
  // Remove the head entries of the parent down queue and the
  // child up queue. All checks are done by the caller.
  //
  ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();  

  ExUnPackRowsPrivateState &pState =
    *((ExUnPackRowsPrivateState*) pEntryDown->pstate);  

  ex_queue_entry *pEntry = qParent_.up->getTailEntry();
  
  pEntry->upState.status = ex_queue::Q_NO_DATA;
  pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
  pEntry->upState.downIndex = qParent_.down->getHeadIndex();
  pEntry->upState.setMatchNo(pState.matchCount_);
  
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
ExUnPackRowsTcb::processCancel()
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
      ExUnPackRowsPrivateState &pState =
        *((ExUnPackRowsPrivateState*) pEntryDown->pstate);  

      // cancel child requests if not already done so and if they
      // aren't already finished
      //
      if (pState.childState_ == ExUnPackRowsTcb::STARTED_) {
                
        // cancel request to this child
        //
        childQueue_.down->cancelRequestWithParentIndex(ix);
        pState.childState_ = ExUnPackRowsTcb::CANCELLED_;
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
ExWorkProcRetcode ExUnPackRowsTcb::work()
{
  ex_assert(0,"Should never reach ExUnPackRowsTcb::work()");
  return WORK_BAD_ERROR;
}

// -----------------------------------------------------------------------
// Work procedure to send requests down
// -----------------------------------------------------------------------
ExWorkProcRetcode ExUnPackRowsTcb::workDown()
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
// generic up-processing for unPackRows
// -----------------------------------------------------------------------

ExWorkProcRetcode
ExUnPackRowsTcb::workUp() 
{
        
  // while there is a chance that we have work (may exit via return)
  // (If processedInputs_ equals the HeadIndex(), then there are no
  // entries in the parent down queue.)
  //
  const NABoolean rowsetIterator = unPackRowsTdb().isRowsetIterator();
  const NABoolean tolerateNonFatalError = unPackRowsTdb().isNonFatalErrorTolerated();

  while (qParent_.down->getHeadIndex() != processedInputs_) {
        
    // get head entry and pState
    //
    ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();

    ExUnPackRowsPrivateState &pState =
      *((ExUnPackRowsPrivateState*)pEntryDown->pstate);


    // Set up the numRowsAtp_ to recieve the packing factor.
    //
    numRowsAtp_->copyAtp(pEntryDown->getAtp());
    
    numRowsAtp_->
      getTupp(unPackRowsTdb().criDescUp_->noTuples() - 1)
      = numRowsTupp_;

    // We are not returning any of the childs returned values.
    // So we copy from the parents request.
    //
    workAtp_->copyAtp(pEntryDown->getAtp());

    // Set up the Tupp for the index value. The index value is
    // a data member of this tcb and is input to the unPackExpr.
    //
    workAtp_->getTupp(unPackRowsTdb().indexValueAtpIndex_) 
      = indexValueTupp_;

    // Get the current value of the index for this request.
    //
    indexValue_  = pState.unPackCount_;

    // for non-atomic rowsets store the next rownumber where a CLI
    // error was raised. This row will be skipped later.
    if (tolerateNonFatalError) {
      ComDiagsArea * da = workAtp_->getDiagsArea();
      if (da) {
	    pState.nextCLIErrorRowNum_ = da->getNextRowNumber(indexValue_);
	  }
    }

    // while we have room in the up queue and rows to process
    //
    while (!qParent_.up->isFull() && !childQueue_.up->isEmpty()) {
                        
      ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();
      atp_struct * childAtp = cEntry->getAtp();

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
        processError(childAtp,FALSE,0);
        continue;
      } else if (pEntryDown->downState.request == ex_queue::GET_N &&
                 pEntryDown->downState.requestValue <= 
                 (Lng32)pState.matchCount_) {

        qParent_.down->cancelRequest(qParent_.down->getHeadIndex());
        processCancel();
        continue;

      } else {

        ex_expr::exp_return_type retCode = ex_expr::EXPR_TRUE;

        // If this is the first row to be unpacked from this packed row.
        //
        if(indexValue_ == 0) {

          // Extract the packing factor (numRows) from the packed column.
          //
          retCode = packingFactor()->eval(numRowsAtp_, childAtp);

          if(retCode == ex_expr::EXPR_ERROR) {
            processError(workAtp_,FALSE,0);
            continue;
          }

          // Register the numRows in the private state, in case we
          // start working on an other request.
          //
          pState.numRows_ = numRows_;

          // If the number of entries in the rowset is negative, we 
          // signal an error
          if (numRows_ < 0) {
              ComDiagsArea * da = workAtp_->getDiagsArea();
              if (!da) 
              {
                da = ComDiagsArea::allocate(getHeap());
                workAtp_->setDiagsArea(da);
              }
              *da << DgSqlCode(-EXE_ROWSET_INDEX_OUTOF_RANGE);
              processError(workAtp_,FALSE,0);
              continue;
          }

        }

        // Only allocate a new tuple if needed.  If a tuple has
        // been allocated, but not inserted into the up queue
        // because the predicate did not pass, reuse the tuple.
        //
        Int32 needTuple = (unPackColsExpr() ? TRUE : FALSE);

        // While we still have expressions to apply to the current
        // child entry and the parent can accept rows.
        // Each expression that is applied to the child entry
        // produces one row to be put onto the parents up queue.
        //
        while(indexValue_ < pState.numRows_ && !qParent_.up->isFull()) { 

	  // Handle GET_N processing
	  //
	  if (pEntryDown->downState.request == ex_queue::GET_N &&
	      pEntryDown->downState.requestValue <= (Lng32)pState.matchCount_) {
	    qParent_.down->cancelRequest(qParent_.down->getHeadIndex());
	    processCancel();
	    indexValue_ = pState.numRows_;
	    continue;
	  }

	  // For Non-atomic rowsets skips over rows that raised an error
	  // in the CLI
	  if (pState.nextCLIErrorRowNum_ == indexValue_) {
	    processSkippedRow(workAtp_);
	    indexValue_++;
	    ComDiagsArea * da = workAtp_->getDiagsArea();
	    pState.nextCLIErrorRowNum_ = da->getNextRowNumber(indexValue_);
	    continue ;
	  }

          // UnPackRows also adds one TP for the generated columns.  A
          // tuple must be allocated if one isn't already available.
          //
          if(needTuple) {
            if (pool_->
                getFreeTuple(workAtp_->
                             getTupp(unPackRowsTdb().unPackColsAtpIndex_))) {
              // Return. Will be called again when some space frees up.
              //
              pState.unPackCount_ = indexValue_;
              return WORK_POOL_BLOCKED;
            }

            // Set flag to indicate that we have allocated a tuple.
            //
            needTuple = FALSE;
          }

          // Apply any selection predicate. 
          //
          // The unPackColsExpr() will be NULL when there are no
          // columns to unPack, but we still need to produce the
          // proper number of rows.  For example count(*).
          //
          // Apply the unPack expression to this row.  This will
          // generate the proper values for the generated columns.
          //
          
	  Lng32 markValue = 0;
	  ComDiagsArea * da = workAtp_->getDiagsArea();
	  if (da)
	    markValue = da->mark();

          retCode = (unPackColsExpr() ? 
                     unPackColsExpr()->eval(workAtp_, childAtp) :
                     ex_expr::EXPR_TRUE);
          
          switch(retCode) {
          case ex_expr::EXPR_TRUE:
            {
              // Get the entry on the parent up queue.
              //
              ex_queue_entry *pEntry = qParent_.up->getTailEntry();
              
              pEntry->copyAtp(workAtp_);

	      // for non-atomic rowsets do NOT flow digas area which we get from parent
	      // this diags area contains errors from the CLI. We do not want to flow them up 
	      // with OK_MMORE replies as this will lead to duplication. 
	      // We will flow them up with the Q_REC_SKIPPED replies.
	      if (tolerateNonFatalError)
		pEntry->setDiagsArea(NULL);
              
              // Release the reference to the tupp in the workAtp_
              //
              workAtp_->getTupp(unPackRowsTdb().unPackColsAtpIndex_).release();
              
              // Set the flag to indicate that the tuple has been used and
              // a new tuple needs to be allocated.
              //
              needTuple = (unPackColsExpr() ? TRUE : FALSE);
              
              // Finialize the queue entry, then insert it
              //
              pEntry->upState.status = cEntry->upState.status;
              pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
              pEntry->upState.setMatchNo(++pState.matchCount_);
              qParent_.up->insert();
              break;
            }
          case ex_expr::EXPR_FALSE:
	    if (rowsetIterator)
	      processSkippedRow(NULL);
	    else {
            // Do nothing
	    }
            break;
          case ex_expr::EXPR_ERROR:
            processError(workAtp_,tolerateNonFatalError,markValue);

            // Release the reference to the tupp in the workAtp_
            //
            workAtp_->getTupp(unPackRowsTdb().unPackColsAtpIndex_).release();
            
            // Set the flag to indicate that the tuple has been used and
            // a new tuple needs to be allocated.
            //
            needTuple = (unPackColsExpr() ? TRUE : FALSE);
            break;
          default:
            break;
          }

          if(retCode == ex_expr::EXPR_ERROR && !tolerateNonFatalError)
            break;

          // Advance the expression counter.
          //
          indexValue_++;

        }

        // This will cause the EOD to be sent after error processing.
        //
        if(retCode == ex_expr::EXPR_ERROR && !tolerateNonFatalError)
          continue;
        
        if(indexValue_ < pState.numRows_) {
          // We still have more expressions to apply.  We must have
          // reached this point because the parent up queue is full.
          // Return, will be called again when the queue becomes non-full.
          //
          pState.unPackCount_ = indexValue_;
          return WORK_OK;
        }
      }
      // We have applied all the expressions on this child entry,
      // reset the expression counter, remove the child entry
      // and then see if there is another entry to be processed.
      //
      indexValue_ = 0;
      childQueue_.up->removeHead();
                        
    } // while parent up ComQueue.has room and child up ComQueue.has replies
  
    // If we got here because we finished a request and stop()ed,
    // then try again on a new request.  But if we got here because
    // the parent queue is full or the child queue is empty, then
    // we must return.
    //
    if(qParent_.up->isFull() || childQueue_.up->isEmpty()) {
      pState.unPackCount_ = indexValue_;
      return WORK_OK;
    }

    pState.unPackCount_ = indexValue_;

  } // while parent ComQueue.has entries

  // parent down queue is empty
  //
  return WORK_OK;
}

void
ExUnPackRowsTcb::processError(atp_struct *atp, NABoolean isNonFatalError, Lng32 markValue)
{
  ex_queue_entry * pEntryDown = qParent_.down->getHeadEntry();
  ex_queue_entry * pEntry = qParent_.up->getTailEntry();
  ExUnPackRowsPrivateState &pState =
    *((ExUnPackRowsPrivateState*) pEntryDown->pstate);  

  pEntry->upState.status = ex_queue::Q_SQLERROR;
  pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
  pEntry->upState.downIndex = qParent_.down->getHeadIndex();
  pEntry->upState.setMatchNo(pState.matchCount_);	  

  if (!isNonFatalError) {
    pEntry->copyAtp(atp);

    // cancel this request and cancel all the children.
    qParent_.down->cancelRequest(qParent_.down->getHeadIndex());
    pState.childState_ = CANCELLED_;

    while(!childQueue_.up->isEmpty() && 
	  (childQueue_.up->getHeadEntry()->upState.status 
	   != ex_queue::Q_NO_DATA))
      childQueue_.up->removeHead();
  }
  else {
      ComDiagsArea * fromDa = atp->getDiagsArea();
      ex_assert(fromDa, "We have an expression error in UnPack but no diags area");
      ComDiagsArea* toDa = ComDiagsArea::allocate(getHeap());
      fromDa->rewindAndMergeIfDifferent(markValue, toDa);
      pEntry->setDiagsArea(toDa);
      toDa->setAllRowNumber(ComCondition::NONFATAL_ERROR) ;     
  }
  qParent_.up->insert();
}

void
ExUnPackRowsTcb::processSkippedRow(atp_struct *atp)
{
  ex_queue_entry * pEntryDown = qParent_.down->getHeadEntry();
  ex_queue_entry * pEntry = qParent_.up->getTailEntry();
  ExUnPackRowsPrivateState &pState =
    *((ExUnPackRowsPrivateState*) pEntryDown->pstate);  

  pEntry->upState.status = ex_queue::Q_REC_SKIPPED;
  pEntry->upState.parentIndex = pEntryDown->downState.parentIndex;
  pEntry->upState.downIndex = qParent_.down->getHeadIndex();
  pEntry->upState.setMatchNo(pState.matchCount_);
  if (atp)
    pEntry->copyAtp(atp);
  qParent_.up->insert();
}

///////////////////////////////////////////////////////////////
//
//  Private state procedures
//
///////////////////////////////////////////////////////////////

// Constructor and destructor for ExUnPackRowsPrivateState
//
ExUnPackRowsPrivateState::ExUnPackRowsPrivateState(const ExUnPackRowsTcb *) 
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
ExUnPackRowsPrivateState::init() 
{
  matchCount_ = 0;
  unPackCount_ = 0;
  numRows_ = 0;
  childState_ = ExUnPackRowsTcb::EMPTY_;
  // row number is a zero based index, so initialize to -1.
  nextCLIErrorRowNum_ = -1;
}

ExUnPackRowsPrivateState::~ExUnPackRowsPrivateState()
{
}

ex_tcb_private_state *
ExUnPackRowsPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace())
    ExUnPackRowsPrivateState((ExUnPackRowsTcb *)tcb);
}


// ExUnPackRowwiseRowsTcb::ExUnPackRowwiseRowsTcb() -------------------------
// 
ExUnPackRowwiseRowsTcb::ExUnPackRowwiseRowsTcb(
     const ExUnPackRowsTdb &unPackRowsTdbLocal,
     ex_globals *glob) :
  ex_tcb(unPackRowsTdbLocal, 1, glob) 
{
  CollHeap *space = glob->getSpace();

  pool_ = new(space) sql_buffer_pool(uprTdb().numBuffers_,
				     uprTdb().bufferSize_,
				     space);

  // Allocate the queue to communicate with parent
  // (Child allocates queue to communicate with Child)
  //
  qParent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
                                      uprTdb().queueSizeDown_,
                                      uprTdb().criDescDown_,
                                      space);

  // Allocate the private state in each entry of the down queue
  //
  ExUnPackRowwiseRowsPrivateState privateState(this);
  qParent_.down->allocatePstate(&privateState, this);

  // Allocate a queue to communicate with the parent node.
  //
  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                    uprTdb().queueSizeUp_,
                                    uprTdb().criDescUp_,
                                    space);

  // fixup all expressions 
  if (uprTdb().rwrsInputSizeExpr())
    uprTdb().rwrsInputSizeExpr()->fixup(0,
					getExpressionMode(), this,
					glob->getSpace(),
					glob->getDefaultHeap(), FALSE, glob);
  
  if (uprTdb().rwrsMaxInputRowlenExpr())
    uprTdb().rwrsMaxInputRowlenExpr()->fixup(0,
					     getExpressionMode(), this,
					     glob->getSpace(),
					     glob->getDefaultHeap(), FALSE, glob);
  
  if (uprTdb().rwrsBufferAddrExpr())
    uprTdb().rwrsBufferAddrExpr()->fixup(0,
					 getExpressionMode(), this,
					 glob->getSpace(),
					 glob->getDefaultHeap(), FALSE, glob);
  
  rwrsInputValuesTuppDesc_.init();
  //rwrsInputValuesTupp_.init();
  workAtp_ = allocateAtp(uprTdb().workCriDesc_, 
                         glob->getSpace());
  workAtp_->getTupp(uprTdb().rwrsWorkIndex()) =
    &rwrsInputValuesTuppDesc_;

  rwrsNumRows_ = 0;
  rwrsMaxInputRowlen_ = -1;
  rwrsBufferAddr_ = NULL;

  step_ = INITIAL_;
}

ExUnPackRowwiseRowsTcb::~ExUnPackRowwiseRowsTcb()
{
  delete qParent_.up;
  delete qParent_.down;
}

ExWorkProcRetcode
ExUnPackRowwiseRowsTcb::work() 
{
  ex_expr::exp_return_type retCode;

  // if no parent request, return
  if (qParent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qParent_.down->getHeadEntry();
  ExUnPackRowwiseRowsPrivateState *  pstate 
    = (ExUnPackRowwiseRowsPrivateState*)pentry_down->pstate;
  ex_queue::down_request request = pentry_down->downState.request;

  while (TRUE) // exit via return
    {
      switch (step_)
	{
	case INITIAL_:
	  { 
	    rwrsNumRows_ = 0;
	    rwrsMaxInputRowlen_ = -1;
	    rwrsBufferAddr_ = NULL;
	    
	    currentRowNum_ = 0;

	    step_ = GET_INPUT_VALUES_;
	  }
	break;
	
	case GET_INPUT_VALUES_:
	  {
	    // get number of rows in the input rwrs buffer
	    workAtp_->getTupp(uprTdb().rwrsWorkIndex()).
	      setDataPointer((char *)&rwrsNumRows_);
	    retCode = 
	      uprTdb().rwrsInputSizeExpr()->eval(pentry_down->getAtp(), 
						 workAtp_);
	    if (retCode == ex_expr::EXPR_ERROR)
	      {
		step_ = ERROR_;
		break;
	      }
	    
	    if (rwrsNumRows_ == 0)
	      {
		step_ = DONE_;
		break;
	      }

	    if (rwrsNumRows_ < 0)
	      {
		ExHandleErrors(qParent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       EXE_NUMERIC_OVERFLOW,
			       NULL,
			       NULL
			       );
		step_ = DONE_;
		break;
	      }

	    // get max length of each row
	    workAtp_->getTupp(uprTdb().rwrsWorkIndex()).
	      setDataPointer((char *)&rwrsMaxInputRowlen_);
	    retCode = 
	      uprTdb().rwrsMaxInputRowlenExpr()->eval(pentry_down->getAtp(), 
						      workAtp_);
	    if (retCode == ex_expr::EXPR_ERROR)
	      {
		step_ = ERROR_;
		break;
	      }
	    
	    if (rwrsMaxInputRowlen_ <= 0)
	      {
		ExHandleErrors(qParent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       EXE_NUMERIC_OVERFLOW,
			       NULL,
			       NULL
			       );
		step_ = DONE_;
		break;
	      }

	    // get address of rwrs buffer
	    workAtp_->getTupp(uprTdb().rwrsWorkIndex()).
	      setDataPointer((char *)&rwrsBufferAddr_);
	    retCode = 
	      uprTdb().rwrsBufferAddrExpr()->eval(pentry_down->getAtp(), 
						  workAtp_);
	    if (retCode == ex_expr::EXPR_ERROR)
	      {
		step_ = ERROR_;
		break;
	      }

	    step_ = RETURN_ROW_;
	  }
	break;

	case RETURN_ROW_:
	  {
	    if (qParent_.up->isFull())
	      return WORK_OK;
    
	    ex_queue_entry * up_entry = qParent_.up->getTailEntry(); 

	    // get pointer to current row
	    char * currRow = (char*)(rwrsBufferAddr_ + 
				     currentRowNum_ * rwrsMaxInputRowlen_);

	    // allocate an empty tupp descriptor
	    tupp_descriptor * td = pool_->get_free_tupp_descriptor(0);
	    if (! td)
	      return WORK_POOL_BLOCKED; // couldn't allocate, try again later.

	    // initialize it with the addr of the row to be returned
	    td->init(rwrsMaxInputRowlen_, NULL, currRow);

	    up_entry->copyAtp(pentry_down);

	    // move this row to up queue
	    up_entry->getTupp(uprTdb().unPackColsAtpIndex_) = td;
	    up_entry->upState.status = ex_queue::Q_OK_MMORE;
	    up_entry->upState.parentIndex 
	      = pentry_down->downState.parentIndex;	      
	    up_entry->upState.downIndex = qParent_.down->getHeadIndex();
	    up_entry->upState.setMatchNo(1);
	    
	    qParent_.up->insert();
	    
	    currentRowNum_++;
	    if (currentRowNum_ == rwrsNumRows_)
	      {
		step_ = DONE_;
		break;
	      }
	    
	  }
	break;

	case ERROR_:
	  {
	    if (qParent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qParent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_SQLERROR;

	    ComDiagsArea *diagsArea = pentry_down->getDiagsArea();
	    
            if (diagsArea != up_entry->getDiagsArea())
              {
	        up_entry->setDiagsArea (diagsArea);
                if (diagsArea != NULL)
                  diagsArea->incrRefCount();
	      }
	    // insert into parent
	    qParent_.up->insert();
	    
	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    if (qParent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qParent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qParent_.up->insert();
	    
	    step_ = INITIAL_;
	    qParent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;
	
	}
    } // while
  
  return WORK_OK;
}     


///////////////////////////////////////////////////////////////
//
//  Private state procedures
//
///////////////////////////////////////////////////////////////

// Constructor and destructor for ExUnPackRowsPrivateState
//
ExUnPackRowwiseRowsPrivateState::ExUnPackRowwiseRowsPrivateState(const ExUnPackRowwiseRowsTcb *) 
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
ExUnPackRowwiseRowsPrivateState::init() 
{

}

ExUnPackRowwiseRowsPrivateState::~ExUnPackRowwiseRowsPrivateState()
{
}

ex_tcb_private_state *
ExUnPackRowwiseRowsPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace())
    ExUnPackRowwiseRowsPrivateState((ExUnPackRowwiseRowsTcb *)tcb);
}



