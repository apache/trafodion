/* -*-C++-*-
******************************************************************************
*
* File:         ExSimpleSample.cpp
* Description:  Node to do simple sampling. Select one of every n rows.
*
* Created:      1/21/98
* Language:     C++
*
*
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
*
*
******************************************************************************
*/

#if 0
// unused feature, done as part of SQ SQL code cleanup effort

#include "ExSimpleSample.h"
#include "ExSimpleSqlBuffer.h"

//////////////////////////////////////////////////////////////////////////////
//
//  TDB procedures
//
//////////////////////////////////////////////////////////////////////////////


// Build the TCB tree for this SimpleSample node and all of its descendents.
ex_tcb *ExSimpleSampleTdb::build(ex_globals *glob)
{
  ex_tcb *childTcb = childTdb_->build(glob);
  ExSimpleSampleTcb *tcb = new (glob->getSpace())
                             ExSimpleSampleTcb(*this,*childTcb,glob);
  tcb->registerSubtasks();
  return tcb;
}


//////////////////////////////////////////////////////////////////////////////
//
//  TCB procedures
//
//////////////////////////////////////////////////////////////////////////////

// Constructor.
ExSimpleSampleTcb::ExSimpleSampleTcb(const ExSimpleSampleTdb& tdb,
                                     const ex_tcb& childTcb,
                                     ex_globals *glob)
 : ex_tcb(tdb,1,glob)
{
  childTcb_ = &childTcb;
  Space* space = (glob ? glob->getSpace() : 0);
  CollHeap* heap = (glob ? glob->getDefaultHeap() : 0);

  // Allocate the buffer pool.
  // pool_ = new (space) ExSimpleSQLBuffer(tdb.noOfBuffers_,
  //                                       tdb.packTuppLen_,
  //                                       space);

  // Get the queue used by my child to communicate with me.
  qChild_ = childTcb_->getParentQueue();

  // Allocate the queue to communicate with parent.
  qParent_.down = new (space) ex_queue(ex_queue::DOWN_QUEUE,
                                       tdb.fromParent_,
                                       tdb.givenCriDesc_,
                                       space);

  // Allocate the private state in each entry of the down queue.
  // ExPackPrivateState privateState(this);
  // qParent_.down->allocatePstate(&privateState,this);

  // Initialize nextRqst_ to refer to the queue entry used next.
  nextRqst_ = qParent_.down->getHeadIndex();

  // Allocate a queue to communicate with the parent node.
  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                    tdb.toParent_,
                                    tdb.returnedCriDesc_,
                                    space);

  // Initialize the count to keep track of the state of sampling.
  initSampleCountDown();
}

// Method to initialize the value of sampleCountDown_.
void ExSimpleSampleTcb::initSampleCountDown()
{
  // sampleCountDown_ has overloaded meanings. For FIRST_N_ sampling, we
  // set it to the requested sample size at the beginning and decrement it
  // when a row is sampled. Thus, we stop when it drops to zero. For ONE_
  // OUT_OF_N_ sampling, it is set to 0 at the beginning. Whenever we
  // see the value to be 0, we sample the row received and reset it to N.
  // For the subsequent (N-1) rows, we decrement the value and when we get
  // back to 0, we sample the row and bump the value back to N again and so
  // on.
  //
  switch(simpleSampleTdb().method_)
  {
    // sample one out of n rows to keep sampling ratio.
    case ExSimpleSampleTdb::ONE_OUT_OF_N_:
      sampleCountDown_ = 0;
      break;

    case ExSimpleSampleTdb::FIRST_N_:
      // sample first n rows.
      sampleCountDown_ = simpleSampleTdb().sampleSize_;
      break;

    default:
      // we got a problem.
      ex_assert(0, "unknown sampling method");
  }
}

// Destructor.
ExSimpleSampleTcb::~ExSimpleSampleTcb()
{
  delete qParent_.up;
  delete qParent_.down;
  freeResources();
}

void ExSimpleSampleTcb::freeResources()
{
  // if(pool_) delete pool_;
  // pool_ = 0;
}

// -----------------------------------------------------------------------
// Register all the subtasks with the scheduler.
// -----------------------------------------------------------------------
void ExSimpleSampleTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  // Schedule this routine if a new entry is inserted into the parent's
  // down queue and we are waiting on an event.
  sched->registerInsertSubtask(sWorkDown,this,qParent_.down,"DN");

  // Schedule this routine if the child's down queue changes from being
  // full to being not full and we are waiting on an event.
  sched->registerUnblockSubtask(sWorkDown,this,qChild_.down,"DN");

  // Schedule this routine if a cancel request occurs on the parent's
  // down queue.
  // sched->registerCancelSubtask(sCancel,this,qParent_.down,"CN");

  // Schedule this routine if the parent's up queue changes from being
  // full to being not full and we are waiting on an event.
  sched->registerUnblockSubtask(sWorkUp,this,qParent_.up,"UP");

  // Schedule this routine if a new entry is inserted into the child's
  // up queue and we are waiting on an event.
  sched->registerInsertSubtask(sWorkUp,this,qChild_.up);
}

// -----------------------------------------------------------------------
// Generic work procedure should never be called
// -----------------------------------------------------------------------
ExWorkProcRetcode ExSimpleSampleTcb::work()
{
  ex_assert(0,"Should never reach ExSimpleSampleTcb::work()");
  return WORK_BAD_ERROR;
}

// -----------------------------------------------------------------------
// Work method to pass requests from parent down to child.
// -----------------------------------------------------------------------
ExWorkProcRetcode ExSimpleSampleTcb::workDown()
{
  // While we have unprocessed down requests and the child's down queue
  // has room, start more child requests.
  while(qParent_.down->entryExists(nextRqst_) && !qChild_.down->isFull())
  {
    ex_queue_entry* cEntryDown = qChild_.down->getTailEntry();
    ex_queue_entry* pEntryDown = qParent_.down->getQueueEntry(nextRqst_);
    const ex_queue::down_request request = pEntryDown->downState.request;

    // Pass request to child.
    cEntryDown->downState.request = request;
    cEntryDown->downState.requestValue = pEntryDown->downState.requestValue;
    cEntryDown->downState.parentIndex = nextRqst_;
    cEntryDown->passAtp(pEntryDown);
    qChild_.down->insert();
    // pState.childState_ = STARTED_;

    // If this is a CANCEL request, cancel request we just gave to our child.
    if(request == ex_queue::GET_NOMORE)
    {
      // immediately cancel the request (requests are already in cancelled
      // state but the cancel callback isn't activated yet)
      // qChild_.down->cancelRequestWithParentIndex(nextRqst_);
      processCancel();
      // pState.childState_ = CANCELLED_;
    }
    nextRqst_++;
  }
  return WORK_OK;
}

// -----------------------------------------------------------------------
// Work procedure to process child rows.
// -----------------------------------------------------------------------
ExWorkProcRetcode ExSimpleSampleTcb::workUp()
{
  // While there is an outstanding parent request not finished.
  while(qParent_.down->getHeadIndex() != nextRqst_)
  {
    // Get the request and retrieve its private state.
    ex_queue_entry* pEntryDown = qParent_.down->getHeadEntry();
    // ExPackPrivateState& pState = *((ExPackPrivateState*)pEntryDown->pstate);

    // While we have room in the up queue and rows to process.
    while(!qParent_.up->isFull() && !qChild_.up->isEmpty())
    {
      // New row produced by child.
      ex_queue_entry* cEntryUp = qChild_.up->getHeadEntry();

      // The current request has already been cancelled.
      // if(pState.childState_ == ExPackTcb::CANCELLED_)
      // {
      //   if(cEntryUp->upState.status == ex_queue::Q_NO_DATA)
      //   {
      //     // No more rows for the cancelled request.
      //     // stop() returns Q_NO_DATA and remove the cancelled request.
      //     stop();

      //     // Break out of the inner while-loop and check for next request.
      //     break;
      //   }
      //   else
      //   {
      //     // Child is returning data for a request already cancelled.
      //     // Just ignore them and continue to check whether child's up
      //     // ComQueue.has any more entries.
      //     qChild_.up->removeHead();
      //     continue;
      //   }
      // }

      if(cEntryUp->upState.status == ex_queue::Q_NO_DATA)
      {
        // Send EOD to parent and clean up the current request.
        ex_queue_entry* pEntryDown = qParent_.down->getHeadEntry();
        ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();

        // Insert EOD to parent's up queue.
        pEntryUp->upState.status = ex_queue::Q_NO_DATA;
        pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
        qParent_.up->insert();

        // Consume the child row.
        qChild_.up->removeHead();

        // Finished processing this request.
        qParent_.down->removeHead();

        // Reset count down for next request.
        initSampleCountDown();

        // Break out of the inner while-loop and check for next request.
        break;
      }

      if(cEntryUp->upState.status == ex_queue::Q_SQLERROR)
      {
        // Child has produced an error. Copy diagnostic area to parent.
        ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();
        pEntryUp->passAtp(cEntryUp);
        pEntryUp->upState.status = ex_queue::Q_SQLERROR;
        pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
        pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
        qParent_.up->insert();

        // Cancel the whole request on child. (may be optional)
        qChild_.down->cancelRequestWithParentIndex(
                                           qParent_.down->getHeadIndex());
        // Mark the corresponding request cancelled.
        // pState.childState_ = ExPackTcb::CANCELLED_;
        qChild_.up->removeHead();
        continue;
      }

      // Now we've got a row from a child. See whether it's fortunate enough
      // to be sampled.
      //
      ex_queue_entry* pEntryUp = qParent_.up->getTailEntry();
      switch(simpleSampleTdb().method_)
      {
        case ExSimpleSampleTdb::ONE_OUT_OF_N_:

          if(sampleCountDown_ == 0)
          {
            pEntryUp->copyAtp(cEntryUp);
            pEntryUp->upState.status = ex_queue::Q_OK_MMORE;
            pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
            pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
            qParent_.up->insert();
            sampleCountDown_ = (Lng32)(((float)1E6 /
                                   (float)simpleSampleTdb().samplingRatio_) -
                                   1.);
            if(sampleCountDown_ < 0) sampleCountDown_ = 0;
          }
          else sampleCountDown_--;
          break;

        case ExSimpleSampleTdb::FIRST_N_:

          if(sampleCountDown_ == 0)
          {
            // In this case we're going to stop sampling. Ignore this row and
            // cancel the corresponding request to child.
            //
            qChild_.down->cancelRequestWithParentIndex(
                                           qParent_.down->getHeadIndex());

            // Eventually (hopefully very soon), child is going to send back
            // a EOD, this EOD will be processed as usual in the first if-case
            // above.
          }
          else
          {
            // This is one of the first N rows. Return it.
            pEntryUp->copyAtp(cEntryUp);
            pEntryUp->upState.status = ex_queue::Q_OK_MMORE;
            pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
            pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
            qParent_.up->insert();
            sampleCountDown_--;
          }
          break;

        default:
          // we got a problem.
          ex_assert(0, "unknown sampling method");

      } // ENDOF switch(simpleSampleTdb().method_)

      // Child row is no longer useful after the eval, get rid of it.
      qChild_.up->removeHead();

    }  // while parent's up queue is not full and child's up queue not empty.

    // If we got here because we finished a request and stop()ed, then
    // try again on a new request. If we got here because the parent
    // queue is full or the child queue is empty, then we must return.
    //
    if(qParent_.up->isFull() || qChild_.up->isEmpty()) return WORK_OK;

  } // while parent's down ComQueue.has entries.

  // Parent down queue is empty.
  return WORK_OK;

}

// -----------------------------------------------------------------------
// For those requests my parent cancel, cancel the corresponding requests
// that have already been forwarded to my child.
// -----------------------------------------------------------------------
void ExSimpleSampleTcb::processCancel()
{
  queue_index ix = qParent_.down->getHeadIndex();

  // Loop over all requests that have been sent down.
  do
  {
    ex_queue_entry* pEntryDown = qParent_.down->getQueueEntry(ix);

    // Check whether this is a request cancelled by the parent.
    if(pEntryDown->downState.request == ex_queue::GET_NOMORE)
    {
      // ExPackPrivateState& pState =
      //                         *((ExPackPrivateState*)pEntryDown->pstate);

      // Cancel corresponding request for my child if it's been forwarded.
      // if(pState.childState_ == ExPackTcb::STARTED_)
      // {

      qChild_.down->cancelRequestWithParentIndex(ix);

      //  pState.childState_ = ExPackTcb::CANCELLED_;
      // }
    }
    ix++;
  }
  while(ix <= nextRqst_);
}


#endif // if 0

