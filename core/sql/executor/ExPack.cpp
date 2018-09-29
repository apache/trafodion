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
* File:         ExPack.cpp
* Description:  Methods for ExPackTdb and ExPackTcb
*               
* Created:      6/16/97
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ExPack.h"
#include "ExSimpleSqlBuffer.h"
#include "ex_exe_stmt_globals.h"
#include "ComDefs.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////


// Build the TCB tree for this Pack node and all of its descendents.
ex_tcb* ExPackRowsTdb::build(ex_globals* glob) 
{ 
  ex_tcb* childTcb = childTdb_->build(glob);
  ExPackRowsTcb* packTcb = new(glob->getSpace()) ExPackRowsTcb(*this,*childTcb,glob);
  packTcb->registerSubtasks();
  return packTcb;
}


//////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
//////////////////////////////////////////////////////////////////////////////

// Constructor.
ExPackRowsTcb::ExPackRowsTcb(const ExPackRowsTdb& packTdb,
                     const ex_tcb& childTcb,
                     ex_globals* glob)
 : ex_tcb(packTdb,1,glob) 
{
  childTcb_ = &childTcb;
  Space* space = (glob ? glob->getSpace() : 0);
  CollHeap* heap = (glob ? glob->getDefaultHeap() : 0);

  // Allocate the buffer pool.
  pool_ = new (space) ExSimpleSQLBuffer(packTdb.noOfBuffers_,
					packTdb.packTuppLen_,
					space);
 
  // Get the queue used by my child to communicate with me.
  qChild_ = childTcb_->getParentQueue();
  
  // Allocate an extra input tupp the Pack node adds to its down queues.
  qChild_.down->allocateAtps(glob->getSpace());
  
  // Allocate the queue to communicate with parent.
  qParent_.down = new (space) ex_queue(ex_queue::DOWN_QUEUE,
                                       packTdb.fromParent_,
                                       packTdb.givenCriDesc_,
                                       space);

  // Allocate the private state in each entry of the down queue.
  ExPackPrivateState privateState(this);
  qParent_.down->allocatePstate(&privateState,this);

  // Initialize nextRqst_ to refer to the queue entry used next.
  nextRqst_ = qParent_.down->getHeadIndex();
  
  // Allocate a queue to communicate with the parent node.
  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                    packTdb.toParent_,
                                    packTdb.returnedCriDesc_,
                                    space);
  // Fix up the expressions.
  packExpr()->fixup(0,getExpressionMode(),this,space,heap, FALSE, glob);
  if(predExpr()) predExpr()->fixup(0,getExpressionMode(),this,space,heap, FALSE, glob);
}

// Destructor.
ExPackRowsTcb::~ExPackRowsTcb()
{
  delete qParent_.up;
  delete qParent_.down;
  freeResources();
}

void ExPackRowsTcb::freeResources()
{
  if(pool_) delete pool_;
  pool_ = 0;
}

// -----------------------------------------------------------------------
// Register all the pack subtasks with the scheduler.
// -----------------------------------------------------------------------
void ExPackRowsTcb::registerSubtasks()
{
  ExScheduler* sched = getGlobals()->getScheduler();
  
  // Schedule this routine if a new entry is inserted into the parent's
  // down queue and we are waiting on an event.
  sched->registerInsertSubtask(sWorkDown,this,qParent_.down,"DN");

  // Schedule this routine if the child's down queue changes from being
  // full to being not full and we are waiting on an event.
  sched->registerUnblockSubtask(sWorkDown,this,qChild_.down,"DN");
  
  // Schedule this routine if a cancel request occurs on the parent's
  // down queue.
  sched->registerCancelSubtask(sCancel,this,qParent_.down,"CN");
  
  // Schedule this routine if the parent's up queue changes from being
  // full to being not full and we are waiting on an event.
  sched->registerUnblockSubtask(sWorkUp,this,qParent_.up,"UP");

  // Schedule this routine if a new entry is inserted into the child's
  // up queue and we are waiting on an event.
  sched->registerInsertSubtask(sWorkUp,this,qChild_.up);

  // BertBert VV
  // Schedule this routine if a GET_NEXT_N request is updated in the parent's
  // down queue and we are waiting on an event.
  sched->registerNextSubtask(sWorkDown,this,qParent_.down,"DN");
  // BertBert ^^
}
 
// -----------------------------------------------------------------------
// Send next request down to children. Called by workDown().
// -----------------------------------------------------------------------
void ExPackRowsTcb::start()
{
  // Caller needs to make sure we have room in the child's down queue.
  ex_queue_entry* cEntryDown = qChild_.down->getTailEntry();

  // Caller needs to make sure we have a new request in parent's down queue.
  ex_queue_entry* pEntryDown = qParent_.down->getQueueEntry(nextRqst_);

  const ex_queue::down_request request = pEntryDown->downState.request;
  ExPackPrivateState& pState = *((ExPackPrivateState*)pEntryDown->pstate);

  // don't forward cancelled request.
  // if(request == ex_queue::GET_NOMORE)
  // {
  //   pState.childState_ = CANCELLED_;
  //   nextRqst_++;
  //   return;
  // } 
  
  // Pass request to children.
  cEntryDown->downState.request = request;
  cEntryDown->downState.requestValue = pEntryDown->downState.requestValue;
  // BertBert VV
  // Note, there is no complete support for the GET_NEXT_N protocol in this operator.
  cEntryDown->downState.numGetNextsIssued = pEntryDown->downState.numGetNextsIssued;
  // BertBert ^^
  cEntryDown->downState.parentIndex = nextRqst_;
  cEntryDown->copyAtp(pEntryDown->getAtp());
  pState.matchCount_ = 0;
  qChild_.down->insert();
  pState.childState_ = STARTED_;

  // If this is a CANCEL request, cancel request we just gave to our child.
  if(request == ex_queue::GET_NOMORE)
  {
    // immediately cancel the request (requests are already in cancelled
    // state but the cancel callback isn't activated yet)
    qChild_.down->cancelRequestWithParentIndex(nextRqst_);
    pState.childState_ = CANCELLED_;
  }
  nextRqst_++;
}

// -----------------------------------------------------------------------
// All child rows have been returned. Return an EOD indication to parent.
// -----------------------------------------------------------------------
void ExPackRowsTcb::stop()
{
  // Caller needs to make sure there is an outstanding parent request.
  ex_queue_entry* pEntryDown = qParent_.down->getHeadEntry();  
  ExPackPrivateState& pState = *((ExPackPrivateState*)pEntryDown->pstate);

  // Caller needs to make sure we have room in the parent's up queue.
  ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();
 
  // Insert EOD to parent's up queue. 
  pEntryUp->upState.status = ex_queue::Q_NO_DATA;
  pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
  qParent_.up->insert();
 
  // child row removed outside of stop(). 
  // Consume the child row.
  qChild_.up->removeHead();	  

  // Reinitialize the state of this processed request.
  pState.init();

  // This parent request has been processed. 
  qParent_.down->removeHead();
}

// -----------------------------------------------------------------------
// For those requests my parent cancel, cancel the corresponding requests
// that have already been forwarded to my child.
// -----------------------------------------------------------------------
ExWorkProcRetcode ExPackRowsTcb::processCancel()
{
  queue_index ix = qParent_.down->getHeadIndex();

  // Loop over all requests that have been sent down.
  while(ix != nextRqst_)
  {
    ex_queue_entry* pEntryDown = qParent_.down->getQueueEntry(ix);
      
    // Check whether this is a request cancelled by the parent.
    if(pEntryDown->downState.request == ex_queue::GET_NOMORE)
    {
      ExPackPrivateState& pState =
                               *((ExPackPrivateState*)pEntryDown->pstate);

      // Cancel corresponding request for my child if it's been forwarded.
      if(pState.childState_ == ExPackRowsTcb::STARTED_)
      {
	qChild_.down->cancelRequestWithParentIndex(ix);
	pState.childState_ = ExPackRowsTcb::CANCELLED_;
      }
    }
    ix++;
  }
  return WORK_OK;
}

// -----------------------------------------------------------------------
// Generic work procedure should never be called
// -----------------------------------------------------------------------
ExWorkProcRetcode ExPackRowsTcb::work()
{
  ex_assert(0,"Should never reach ExPackRowsTcb::work()");
  return WORK_BAD_ERROR;
}

// -----------------------------------------------------------------------
// Work procedure to send requests down
// -----------------------------------------------------------------------
ExWorkProcRetcode ExPackRowsTcb::workDown()
{
  // While we have unprocessed down requests and the child's down queue
  // has room, start more child requests. 
  while(
     qParent_.down->entryExists(nextRqst_) && !qChild_.down->isFull())
     start();

  return WORK_OK;
}

// -----------------------------------------------------------------------
// Work procedure to process child rows.
// -----------------------------------------------------------------------
ExWorkProcRetcode ExPackRowsTcb::workUp() 
{
  // While there is an outstanding parent request not finished.
  while(qParent_.down->getHeadIndex() != nextRqst_)
  {
    // Get the request and retrieve its private state.
    ex_queue_entry* pEntryDown = qParent_.down->getHeadEntry();
    ExPackPrivateState& pState = *((ExPackPrivateState*)pEntryDown->pstate);

    // While we have room in the up queue and rows to process.
    while(!qParent_.up->isFull() && !qChild_.up->isEmpty())
    {
      // New row produced by child.
      ex_queue_entry* cEntryUp = qChild_.up->getHeadEntry();

      // The current request has already been cancelled.
      if(pState.childState_ == ExPackRowsTcb::CANCELLED_)
      {
        if(cEntryUp->upState.status == ex_queue::Q_NO_DATA)
        {
          // No more rows for the cancelled request.
          // stop() returns Q_NO_DATA and remove the cancelled request.
          stop();

          // Break out of the inner while-loop and check for next request.
          break;
        }
        else
        {
          // Child is returning data for a request already cancelled.
          // Just ignore them and continue to check whether child's up
          // ComQueue.has any more entries.
          qChild_.up->removeHead();
          continue;
        }
      }

      if(cEntryUp->upState.status == ex_queue::Q_NO_DATA)
      {
        // Apply selection predicates to and return row being partially
        // packed in buffer if there is one.
        workReturnRow();

	// Send EOD to parent and clean up the current request.
	stop();

	// Break out of the inner while-loop and check for next request.
	break;
      }
      else if(cEntryUp->upState.status == ex_queue::Q_SQLERROR)
      {
        // Child has produced an error. Copy diagnostic area to parent.
        ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();
        pEntryUp->copyAtp(cEntryUp);
        pEntryUp->upState.status = ex_queue::Q_SQLERROR;
        pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
        pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
        pEntryUp->upState.setMatchNo(pState.matchCount_); // (+1?)
        qParent_.up->insert();

        // Cancel the whole request on child. (may be optional)
        qChild_.down->cancelRequestWithParentIndex(
                                           qParent_.down->getHeadIndex());
        // Mark the corresponding request cancelled.
        pState.childState_ = ExPackRowsTcb::CANCELLED_;
        qChild_.up->removeHead();
        continue;
      }

      //
      // Now, we got a new row to be packed into the current packed row.
      // Here's the core stuff.

      // Check if the packTupp has been allocated. If not, allocate it.
      if(pState.packTupp_.getDataPointer() == NULL)
      {
        // Cannot allocate space. Call again when some space free up.
        if(pool_->getFreeTuple(pState.packTupp_))
          return WORK_POOL_BLOCKED;

        // A cheat to initialize contents of the packed row. We need this
        // since the logic of packExpr evaluation relies on the fact that
        // the no of rows already packed in a packed row (which is 0 to
        // begin with) is stored in the packed row itself, even initially.
        //
        char* packTuppPtr = pState.packTupp_.getDataPointer();
        for(Int32 i=0; i < packTdb().packTuppLen_; i++) packTuppPtr[i] = 0;

        // $$$ could potentially make it faster by initializing only the
        // $$$ first four bytes (which stores the no of rows) to 0.
      }

      // Put the pack tupp directly in child's returned ATP.
      atp_struct * cEntryUpAtp = cEntryUp->getAtp();
      cEntryUpAtp->getTupp(packTdb().packTuppIndex_) = pState.packTupp_;

      // Apply the pack expression to pack child row into packed row.
      ex_expr::exp_return_type retCode = packExpr()->eval(cEntryUpAtp,0);
      if(!cEntryUpAtp->getDiagsArea());
      else if(!pEntryDown->getDiagsArea())
	{
	  pEntryDown->shareDiagsArea(cEntryUpAtp->getDiagsArea());
	}
      else
	pEntryDown->getDiagsArea()->mergeAfter(*cEntryUpAtp->getDiagsArea());

      if(retCode == ex_expr::EXPR_TRUE)
      {
        // for debugging.
        // pState.printPackTupp();

        // eval() returns TRUE if the packed record is full after the
        // child row is packed into it. Return the full packed record.

         workReturnRow();
      }
      else
      {
        // for debugging.
        // pState.printPackTupp();

        // Error occurs during expression evaluation.
        if(retCode == ex_expr::EXPR_ERROR)
        {
          // There must be room in the up queue to have reached here.
          ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();

          // cEntryUp's atp has diagnostic area set up. copy it to parent.
          pEntryUp->copyAtp(cEntryUp);
          pEntryUp->upState.status = ex_queue::Q_SQLERROR;
          pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
          pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
          pEntryUp->upState.setMatchNo(pState.matchCount_); //(+1?)
          qParent_.up->insert();

          // mark this request cancelled in the down queue to child.
          qChild_.down->cancelRequestWithParentIndex(
                                           qParent_.down->getHeadIndex()); 
          pState.childState_ = ExPackRowsTcb::CANCELLED_;
        }
      }

      // Child row is no longer useful after the eval, get rid of it.
      qChild_.up->removeHead();

    }  // while parent's up queue is not full and child's up queue not empty.
      
    // If we got here because we finished a request and stop()ed, then
    // try again on a new request. If we got here because the parent
    // queue is full or the child queue is empty, then we must return.
    //
    if(qParent_.up->isFull() || qChild_.up->isEmpty()) {
      return WORK_OK;
    }

  } // while parent's down ComQueue.has entries.

  // Parent down queue is empty.
  return WORK_OK;

}

// -----------------------------------------------------------------------
// Work procedure to apply the selection predicates on the current packed
// record and return it.
// -----------------------------------------------------------------------
ExWorkProcRetcode ExPackRowsTcb::workReturnRow()
{
  // Caller needs to make sure there is an outerstanding request.
  ex_queue_entry* pEntryDown = qParent_.down->getHeadEntry();
  ExPackPrivateState& pState = *((ExPackPrivateState*)pEntryDown->pstate);

  // Nothing in packed record. Nothing to return.
  if(pState.packTupp_.getDataPointer() == NULL) return WORK_OK;

  // Copy parent's ATP from down queue to its up queue and add last tupp.
  ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();
  atp_struct * pEntryUpAtp = pEntryUp->getAtp();
  pEntryUpAtp->copyAtp(pEntryDown->getAtp());
  pEntryUpAtp->getTupp(packTdb().packTuppIndex_) = pState.packTupp_;

  // Release the tupp in the private state so that it stops referencing
  // the packed record.
  pState.packTupp_.release();

  // Evaluate the selection predicates.
  ex_expr::exp_return_type predVal = ex_expr::EXPR_TRUE;
  if(predExpr()) predVal = predExpr()->eval(pEntryUpAtp,0);
  if(predVal)
  {
    // Formally insert the ATP into the parent's up queue.
    pEntryUp->upState.status = ex_queue::Q_OK_MMORE;
    pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
    pEntryUp->upState.setMatchNo(pState.matchCount_ + 1);
    pState.matchCount_++;
    qParent_.up->insert();
  }
  else
  {
    // Release the ATP formed in the parent's up queue entry.
    pEntryUpAtp->release();
  }
  return WORK_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Private state procedures
//
//////////////////////////////////////////////////////////////////////////////

// Constructor.
ExPackPrivateState::ExPackPrivateState(const ExPackRowsTcb *) 
{
  packTupp_.init();
  init();
}

// Destructor.
ExPackPrivateState::~ExPackPrivateState()
{}

// Initialization.
void ExPackPrivateState::init() 
{
  matchCount_ = 0;
  childState_ = ExPackRowsTcb::EMPTY_;
  packTupp_.release();
}

// Allocation.
ex_tcb_private_state* ExPackPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExPackPrivateState((ExPackRowsTcb *)tcb);
}

void ExPackPrivateState::printPackTupp()
{ 
  /*
  char* env = getenv("DEBUG_PACKING");
  if(env != NULL)
  {
    char* data = packTupp_.getDataPointer();
    printf("Pack tupp in HEX:\n");
    if(data == NULL) return;
    int len = packTupp_.getAllocatedSize();
    for(int i=0;i<len;i++) printf("%x ",data[i]);
    printf("\n");
  }
  */
}

